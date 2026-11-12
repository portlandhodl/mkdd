use std::collections::{HashMap, HashSet};

use crate::error::{FormatError, Result};
use crate::formats::be;
use crate::formats::compression::{self, Compression};

const RARC_MAGIC: &[u8; 4] = b"RARC";
const MAIN_HEADER_SIZE: usize = 0x20;
const DATA_INFO_SIZE: usize = 0x20;
const NODE_SIZE: usize = 0x10;
const ENTRY_SIZE: usize = 0x14;
const ALIGNMENT: usize = 0x20;

const ATTR_DIRECTORY: u8 = 0x02;
const ATTR_COMPRESSED: u8 = 0x04;
const ATTR_YAZ0: u8 = 0x80;
const SPECIAL_ID: u16 = 0xffff;

// These limits primarily stop corrupt counts from causing large allocations. The
// on-disk next_free_file_id also limits a canonical archive to 65,535 files.
const MAX_NODES: usize = 65_535;
const MAX_ENTRIES: usize = 1_000_000;
const MAX_FILES: usize = 65_535;
const MAX_DECODED_FILE_SIZE: usize = 256 * 1024 * 1024;
const MAX_DECODED_TOTAL_SIZE: usize = 512 * 1024 * 1024;

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct RarcFile {
    pub index: usize,
    pub id: u16,
    pub path: String,
    pub name: String,
    pub attributes: u8,
    pub stored_size: u32,
    pub compressed: bool,
}

#[derive(Clone, Debug)]
struct RawNode {
    kind: u32,
    name: Vec<u8>,
    first_entry: usize,
    entry_count: usize,
}

#[derive(Clone, Debug)]
struct RawEntry {
    id: u16,
    attributes: u8,
    name: Vec<u8>,
    offset_or_node: u32,
    stored_size: u32,
}

#[derive(Clone, Debug)]
struct ArchiveNode {
    kind: u32,
    name: Vec<u8>,
    entries: Vec<ArchiveEntry>,
}

#[derive(Clone, Debug)]
struct ArchiveEntry {
    id: u16,
    attributes: u8,
    name: Vec<u8>,
    kind: EntryKind,
}

#[derive(Clone, Copy, Debug)]
enum EntryKind {
    File(usize),
    Directory(usize),
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
enum VisitState {
    Unvisited,
    Visiting,
    Visited,
}

#[derive(Debug)]
struct VisitFrame {
    node: usize,
    parent: usize,
    next_entry: usize,
}

pub struct RarcArchive {
    original: Vec<u8>,
    outer_compression: Compression,
    files: Vec<RarcFile>,
    data: Vec<Vec<u8>>,
    original_data: Vec<Option<Vec<u8>>>,
    nodes: Vec<ArchiveNode>,
    dirty_files: usize,
}

impl RarcArchive {
    pub fn new(data: impl AsRef<[u8]>) -> Result<Self> {
        let original = data.as_ref().to_vec();
        let outer_compression = compression::detect(&original);
        let decoded = if original.starts_with(RARC_MAGIC) {
            original.clone()
        } else {
            decompress_checked(&original)?
        };
        let (files, file_data, nodes) = parse_rarc(&decoded)?;
        let original_data = vec![None; file_data.len()];
        Ok(Self {
            original,
            outer_compression,
            files,
            data: file_data,
            original_data,
            nodes,
            dirty_files: 0,
        })
    }

    pub fn parse(data: &[u8]) -> Result<Self> {
        Self::new(data)
    }

    pub fn files(&self) -> &[RarcFile] {
        &self.files
    }

    pub fn file_data(&self, index: usize) -> Result<&[u8]> {
        self.data
            .get(index)
            .map(Vec::as_slice)
            .ok_or_else(|| FormatError::new(format!("RARC file index {index} is out of range")))
    }

    pub fn replace_file(&mut self, index: usize, data: impl AsRef<[u8]>) -> Result<()> {
        let replacement = data.as_ref();
        u32::try_from(replacement.len())
            .map_err(|_| FormatError::new("RARC files cannot exceed u32::MAX bytes"))?;
        if replacement.len() > MAX_DECODED_FILE_SIZE {
            return Err(FormatError::new(
                "RARC replacement exceeds the decoded-size limit",
            ));
        }
        let current = self
            .data
            .get(index)
            .ok_or_else(|| FormatError::new(format!("RARC file index {index} is out of range")))?;
        if self.original_data[index].as_deref() == Some(replacement) {
            self.data[index] = self.original_data[index].take().unwrap();
            self.dirty_files -= 1;
            return Ok(());
        }
        if current.as_slice() == replacement {
            return Ok(());
        }
        let replacement = replacement.to_vec();
        if self.original_data[index].is_none() {
            let original = std::mem::replace(&mut self.data[index], replacement);
            self.original_data[index] = Some(original);
            self.dirty_files += 1;
        } else {
            self.data[index] = replacement;
        }
        Ok(())
    }

    pub fn export(&self) -> Result<Vec<u8>> {
        if !self.is_dirty() {
            return Ok(self.original.clone());
        }
        export_canonical(&self.nodes, &self.data)
    }

    pub fn original_bytes(&self) -> &[u8] {
        &self.original
    }

    pub fn is_dirty(&self) -> bool {
        self.dirty_files != 0
    }

    pub fn original_outer_compression(&self) -> Compression {
        self.outer_compression
    }
}

fn decompress_checked(data: &[u8]) -> Result<Vec<u8>> {
    let declared_size = be::u32_at(data, 4)? as usize;
    if declared_size > MAX_DECODED_FILE_SIZE {
        return Err(FormatError::new(format!(
            "compressed size declaration of {declared_size} bytes exceeds the RARC safety limit"
        )));
    }
    let decoded = compression::decompress(data)?;
    if decoded.len() != declared_size {
        return Err(FormatError::new(format!(
            "compressed stream declared {declared_size} bytes but decoded to {}",
            decoded.len()
        )));
    }
    Ok(decoded)
}

type ParsedRarc = (Vec<RarcFile>, Vec<Vec<u8>>, Vec<ArchiveNode>);

fn parse_rarc(decoded: &[u8]) -> Result<ParsedRarc> {
    if be::bytes(decoded, 0, 4)? != RARC_MAGIC {
        return Err(FormatError::at(0, "expected RARC magic"));
    }
    let declared_file_size = to_usize(be::u32_at(decoded, 4)?, "RARC file size")?;
    if declared_file_size < MAIN_HEADER_SIZE || declared_file_size > decoded.len() {
        return Err(FormatError::at(
            4,
            format!(
                "RARC file size {declared_file_size} is outside the {} byte input",
                decoded.len()
            ),
        ));
    }
    let raw = &decoded[..declared_file_size];
    let info = to_usize(be::u32_at(raw, 8)?, "RARC header length")?;
    if info < MAIN_HEADER_SIZE {
        return Err(FormatError::at(
            8,
            "RARC main header is shorter than 0x20 bytes",
        ));
    }
    be::bytes(raw, info, DATA_INFO_SIZE)?;

    let data_start = relative(raw, info, be::u32_at(raw, 0x0c)?, "file data")?.start;
    let data_size = to_usize(be::u32_at(raw, 0x10)?, "file data size")?;
    be::bytes(raw, data_start, data_size)?;

    let node_count = checked_count(be::u32_at(raw, info)?, MAX_NODES, "directory nodes")?;
    if node_count == 0 {
        return Err(FormatError::at(info, "RARC has no root directory node"));
    }
    let nodes_relative = be::u32_at(raw, info + 4)?;
    let nodes_start = relative(raw, info, nodes_relative, "directory nodes")?.start;
    be::bytes(raw, nodes_start, checked_mul(node_count, NODE_SIZE)?)?;

    let entry_count = checked_count(be::u32_at(raw, info + 8)?, MAX_ENTRIES, "file entries")?;
    let entries_relative = be::u32_at(raw, info + 0x0c)?;
    let entries_start = relative(raw, info, entries_relative, "file entries")?.start;
    be::bytes(raw, entries_start, checked_mul(entry_count, ENTRY_SIZE)?)?;

    let string_table_size = to_usize(be::u32_at(raw, info + 0x10)?, "string table size")?;
    let strings_relative = be::u32_at(raw, info + 0x14)?;
    let strings_start = relative(raw, info, strings_relative, "string table")?.start;
    let strings = be::bytes(raw, strings_start, string_table_size)?;

    let mut raw_nodes = Vec::with_capacity(node_count);
    for index in 0..node_count {
        let offset = nodes_start + index * NODE_SIZE;
        let name_offset = to_usize(be::u32_at(raw, offset + 4)?, "node name offset")?;
        let name = read_string(strings, name_offset)?;
        if name.is_empty() {
            return Err(FormatError::at(
                offset + 4,
                "directory node has an empty name",
            ));
        }
        let node_entry_count = be::u16_at(raw, offset + 0x0a)? as usize;
        let first_entry = to_usize(be::u32_at(raw, offset + 0x0c)?, "first file entry")?;
        let end = first_entry
            .checked_add(node_entry_count)
            .ok_or_else(|| FormatError::at(offset + 0x0c, "file entry range overflow"))?;
        if end > entry_count {
            return Err(FormatError::at(
                offset + 0x0c,
                "directory node links outside the file entry table",
            ));
        }
        raw_nodes.push(RawNode {
            kind: be::u32_at(raw, offset)?,
            name: name.to_vec(),
            first_entry,
            entry_count: node_entry_count,
        });
    }

    let mut owners = vec![false; entry_count];
    let mut covered_entries = 0usize;
    for (node_index, node) in raw_nodes.iter().enumerate() {
        let mut names = HashSet::new();
        let mut display_names = HashSet::new();
        for (entry_index, owner) in owners
            .iter_mut()
            .enumerate()
            .skip(node.first_entry)
            .take(node.entry_count)
        {
            if *owner {
                return Err(FormatError::new(format!(
                    "file entry {entry_index} is linked by multiple directory nodes"
                )));
            }
            *owner = true;
            covered_entries += 1;

            let offset = entries_start + entry_index * ENTRY_SIZE;
            let name_offset = (be::u32_at(raw, offset + 4)? & 0x00ff_ffff) as usize;
            let name = read_string(strings, name_offset)?;
            if name.is_empty() {
                return Err(FormatError::at(offset + 4, "file entry has an empty name"));
            }
            if !names.insert(fold_name(name)) {
                return Err(FormatError::at(
                    offset + 4,
                    format!("duplicate name in directory node {node_index}"),
                ));
            }
            if !display_names.insert(String::from_utf8_lossy(name).into_owned()) {
                return Err(FormatError::at(
                    offset + 4,
                    "names collide after lossy UTF-8 conversion",
                ));
            }
        }
    }
    if covered_entries != entry_count {
        return Err(FormatError::new("RARC contains unlinked file entries"));
    }

    let mut entries = Vec::with_capacity(entry_count);
    for index in 0..entry_count {
        let offset = entries_start + index * ENTRY_SIZE;
        let flags_and_name = be::u32_at(raw, offset + 4)?;
        let name_offset = (flags_and_name & 0x00ff_ffff) as usize;
        entries.push(RawEntry {
            id: be::u16_at(raw, offset)?,
            attributes: (flags_and_name >> 24) as u8,
            name: read_string(strings, name_offset)?.to_vec(),
            offset_or_node: be::u32_at(raw, offset + 8)?,
            stored_size: be::u32_at(raw, offset + 0x0c)?,
        });
    }

    let mut nodes = raw_nodes
        .iter()
        .map(|node| ArchiveNode {
            kind: node.kind,
            name: node.name.clone(),
            entries: Vec::with_capacity(node.entry_count),
        })
        .collect::<Vec<_>>();
    let mut files = Vec::new();
    let mut file_data = Vec::new();
    let mut paths = vec![String::new(); node_count];
    paths[0] = String::from_utf8_lossy(&raw_nodes[0].name).into_owned();
    let mut states = vec![VisitState::Unvisited; node_count];
    let mut ids = HashSet::new();
    let mut decoded_total = 0usize;
    states[0] = VisitState::Visiting;
    let mut stack = vec![VisitFrame {
        node: 0,
        parent: 0,
        next_entry: raw_nodes[0].first_entry,
    }];

    while let Some(frame) = stack.last_mut() {
        let node_index = frame.node;
        let raw_node = &raw_nodes[node_index];
        let end = raw_node.first_entry + raw_node.entry_count;
        if frame.next_entry == end {
            states[node_index] = VisitState::Visited;
            stack.pop();
            continue;
        }

        let entry_index = frame.next_entry;
        frame.next_entry += 1;
        let parent = frame.parent;
        let raw_entry = &entries[entry_index];
        let is_directory = raw_entry.attributes & ATTR_DIRECTORY != 0;
        let target = raw_entry.offset_or_node as usize;

        if raw_entry.name == b"." || raw_entry.name == b".." {
            let expected = if raw_entry.name == b"." {
                node_index
            } else {
                parent
            };
            let root_parent_sentinel =
                raw_entry.name == b".." && node_index == 0 && raw_entry.offset_or_node == u32::MAX;
            if !is_directory || (target != expected && !root_parent_sentinel) {
                return Err(FormatError::new(format!(
                    "malformed special directory link in entry {entry_index}"
                )));
            }
            if !root_parent_sentinel && target >= node_count {
                return Err(FormatError::new(format!(
                    "special directory link in entry {entry_index} is out of range"
                )));
            }
            nodes[node_index].entries.push(ArchiveEntry {
                id: raw_entry.id,
                attributes: raw_entry.attributes,
                name: raw_entry.name.clone(),
                kind: EntryKind::Directory(target),
            });
            continue;
        }

        let name = String::from_utf8_lossy(&raw_entry.name).into_owned();
        let path = join_path(&paths[node_index], &name);
        if is_directory {
            if target >= node_count {
                return Err(FormatError::new(format!(
                    "directory entry {entry_index} links to missing node {target}"
                )));
            }
            match states[target] {
                VisitState::Visiting => {
                    return Err(FormatError::new(format!(
                        "directory cycle through entry {entry_index}"
                    )));
                }
                VisitState::Visited => {
                    return Err(FormatError::new(format!(
                        "directory node {target} has multiple parents"
                    )));
                }
                VisitState::Unvisited => {}
            }
            paths[target] = path;
            nodes[node_index].entries.push(ArchiveEntry {
                id: raw_entry.id,
                attributes: raw_entry.attributes,
                name: raw_entry.name.clone(),
                kind: EntryKind::Directory(target),
            });
            states[target] = VisitState::Visiting;
            stack.push(VisitFrame {
                node: target,
                parent: node_index,
                next_entry: raw_nodes[target].first_entry,
            });
            continue;
        }

        if files.len() >= MAX_FILES {
            return Err(FormatError::new("RARC contains too many files"));
        }
        if !ids.insert(raw_entry.id) {
            return Err(FormatError::new(format!(
                "duplicate file id 0x{:04x} in entry {entry_index}",
                raw_entry.id
            )));
        }
        let file_offset = data_start
            .checked_add(raw_entry.offset_or_node as usize)
            .ok_or_else(|| FormatError::new("file data offset overflow"))?;
        let stored_size = raw_entry.stored_size as usize;
        be::bytes(raw, file_offset, stored_size)?;
        let relative_end = raw_entry
            .offset_or_node
            .checked_add(raw_entry.stored_size)
            .ok_or_else(|| FormatError::new("file data range overflow"))?;
        if relative_end as usize > data_size {
            return Err(FormatError::new(format!(
                "file entry {entry_index} extends beyond the RARC data section"
            )));
        }
        let stored = be::bytes(raw, file_offset, stored_size)?;
        if stored_size > MAX_DECODED_FILE_SIZE {
            return Err(FormatError::new(
                "stored RARC file exceeds the decoded-size limit",
            ));
        }
        let decoded = if raw_entry.attributes & ATTR_COMPRESSED != 0 {
            let expected_magic: &[u8; 4] = if raw_entry.attributes & ATTR_YAZ0 != 0 {
                b"Yaz0"
            } else {
                b"Yay0"
            };
            if !stored.starts_with(expected_magic) {
                return Err(FormatError::new(format!(
                    "compressed file entry {entry_index} has the wrong stream type"
                )));
            }
            decompress_checked(stored)?
        } else {
            stored.to_vec()
        };
        decoded_total = decoded_total
            .checked_add(decoded.len())
            .ok_or_else(|| FormatError::new("decoded RARC file size overflow"))?;
        if decoded_total > MAX_DECODED_TOTAL_SIZE {
            return Err(FormatError::new(
                "decoded RARC files exceed the safety limit",
            ));
        }

        let index = files.len();
        files.push(RarcFile {
            index,
            id: raw_entry.id,
            path,
            name,
            attributes: raw_entry.attributes,
            stored_size: raw_entry.stored_size,
            compressed: raw_entry.attributes & ATTR_COMPRESSED != 0,
        });
        file_data.push(decoded);
        nodes[node_index].entries.push(ArchiveEntry {
            id: raw_entry.id,
            attributes: raw_entry.attributes,
            name: raw_entry.name.clone(),
            kind: EntryKind::File(index),
        });
    }

    if let Some(unreachable) = states
        .iter()
        .position(|state| *state != VisitState::Visited)
    {
        return Err(FormatError::new(format!(
            "directory node {unreachable} is unreachable from the root"
        )));
    }
    Ok((files, file_data, nodes))
}

fn read_string(strings: &[u8], offset: usize) -> Result<&[u8]> {
    let remainder = strings
        .get(offset..)
        .ok_or_else(|| FormatError::at(offset, "string offset is outside the RARC string table"))?;
    let length = remainder
        .iter()
        .position(|byte| *byte == 0)
        .ok_or_else(|| FormatError::at(offset, "unterminated RARC string"))?;
    Ok(&remainder[..length])
}

fn fold_name(name: &[u8]) -> Vec<u8> {
    name.iter().map(u8::to_ascii_lowercase).collect()
}

fn name_hash(name: &[u8]) -> u16 {
    name.iter().fold(0u16, |hash, byte| {
        hash.wrapping_mul(3)
            .wrapping_add(byte.to_ascii_lowercase() as u16)
    })
}

fn join_path(parent: &str, name: &str) -> String {
    if parent.is_empty() {
        name.to_owned()
    } else {
        format!("{parent}/{name}")
    }
}

fn checked_count(value: u32, maximum: usize, description: &str) -> Result<usize> {
    let value = to_usize(value, description)?;
    if value > maximum {
        return Err(FormatError::new(format!(
            "RARC {description} count {value} exceeds the limit {maximum}"
        )));
    }
    Ok(value)
}

fn to_usize(value: u32, description: &str) -> Result<usize> {
    usize::try_from(value).map_err(|_| FormatError::new(format!("RARC {description} is too large")))
}

fn checked_mul(left: usize, right: usize) -> Result<usize> {
    left.checked_mul(right)
        .ok_or_else(|| FormatError::new("RARC table size overflow"))
}

fn relative(
    data: &[u8],
    base: usize,
    offset: u32,
    description: &str,
) -> Result<std::ops::Range<usize>> {
    let start = base
        .checked_add(offset as usize)
        .ok_or_else(|| FormatError::new(format!("RARC {description} offset overflow")))?;
    be::range(data, start, 0)
}

fn align_up(value: usize) -> Result<usize> {
    value
        .checked_add(ALIGNMENT - 1)
        .map(|value| value & !(ALIGNMENT - 1))
        .ok_or_else(|| FormatError::new("RARC alignment overflow"))
}

fn export_canonical(nodes: &[ArchiveNode], file_data: &[Vec<u8>]) -> Result<Vec<u8>> {
    if nodes.is_empty() || nodes.len() > MAX_NODES || file_data.len() > MAX_FILES {
        return Err(FormatError::new("invalid RARC directory or file count"));
    }

    let mut string_table = StringTable::default();
    for node in nodes {
        string_table.intern(&node.name)?;
    }
    for node in nodes {
        if node.entries.len() > u16::MAX as usize {
            return Err(FormatError::new(
                "a RARC directory has more than 65,535 entries",
            ));
        }
        for entry in &node.entries {
            string_table.intern(&entry.name)?;
        }
    }

    let info = MAIN_HEADER_SIZE;
    let nodes_start = info + DATA_INFO_SIZE;
    let nodes_size = checked_mul(nodes.len(), NODE_SIZE)?;
    let entries_start = align_up(nodes_start + nodes_size)?;
    let entry_count = nodes.iter().try_fold(0usize, |count, node| {
        count
            .checked_add(node.entries.len())
            .ok_or_else(|| FormatError::new("RARC entry count overflow"))
    })?;
    if entry_count > MAX_ENTRIES {
        return Err(FormatError::new("RARC contains too many file entries"));
    }
    let entries_size = checked_mul(entry_count, ENTRY_SIZE)?;
    let strings_start = align_up(entries_start + entries_size)?;
    let data_start = align_up(strings_start + string_table.bytes.len())?;

    let mut data_size = 0usize;
    let mut referenced_files = vec![false; file_data.len()];
    for node in nodes {
        for entry in &node.entries {
            if let EntryKind::File(index) = entry.kind {
                let data = file_data
                    .get(index)
                    .ok_or_else(|| FormatError::new("RARC file reference is out of range"))?;
                if referenced_files[index] {
                    return Err(FormatError::new("a RARC file is linked more than once"));
                }
                referenced_files[index] = true;
                data_size = align_up(
                    data_size
                        .checked_add(data.len())
                        .ok_or_else(|| FormatError::new("RARC data size overflow"))?,
                )?;
            }
        }
    }
    if referenced_files.iter().any(|referenced| !referenced) {
        return Err(FormatError::new("RARC contains an unlinked file"));
    }
    let file_size = data_start
        .checked_add(data_size)
        .ok_or_else(|| FormatError::new("RARC file size overflow"))?;
    let file_size_u32 = u32::try_from(file_size)
        .map_err(|_| FormatError::new("exported RARC exceeds u32::MAX bytes"))?;
    let mut output = vec![0u8; file_size];

    be::bytes_mut(&mut output, 0, 4)?.copy_from_slice(RARC_MAGIC);
    be::put_u32(&mut output, 4, file_size_u32)?;
    be::put_u32(&mut output, 8, MAIN_HEADER_SIZE as u32)?;
    be::put_u32(
        &mut output,
        0x0c,
        u32::try_from(data_start - MAIN_HEADER_SIZE).unwrap(),
    )?;
    be::put_u32(&mut output, 0x10, u32::try_from(data_size).unwrap())?;
    be::put_u32(&mut output, 0x14, u32::try_from(data_size).unwrap())?;

    be::put_u32(&mut output, info, nodes.len() as u32)?;
    be::put_u32(&mut output, info + 4, (nodes_start - info) as u32)?;
    be::put_u32(&mut output, info + 8, entry_count as u32)?;
    be::put_u32(&mut output, info + 0x0c, (entries_start - info) as u32)?;
    be::put_u32(
        &mut output,
        info + 0x10,
        u32::try_from(string_table.bytes.len()).unwrap(),
    )?;
    be::put_u32(&mut output, info + 0x14, (strings_start - info) as u32)?;
    let mut next_free_id = 0u16;
    for entry in nodes.iter().flat_map(|node| &node.entries) {
        if matches!(entry.kind, EntryKind::File(_)) && entry.id != SPECIAL_ID {
            next_free_id = next_free_id.max(
                entry
                    .id
                    .checked_add(1)
                    .ok_or_else(|| FormatError::new("RARC file id space is exhausted"))?,
            );
        }
    }
    be::put_u16(&mut output, info + 0x18, next_free_id)?;
    output[info + 0x1a] = 1;

    let mut first_entry = 0usize;
    for (index, node) in nodes.iter().enumerate() {
        let offset = nodes_start + index * NODE_SIZE;
        be::put_u32(&mut output, offset, node.kind)?;
        be::put_u32(&mut output, offset + 4, string_table.offset(&node.name)?)?;
        be::put_u16(&mut output, offset + 8, name_hash(&node.name))?;
        be::put_u16(&mut output, offset + 0x0a, node.entries.len() as u16)?;
        be::put_u32(&mut output, offset + 0x0c, first_entry as u32)?;
        first_entry += node.entries.len();
    }

    let mut entry_index = 0usize;
    let mut data_cursor = data_start;
    let mut ids_synchronized = true;
    for node in nodes {
        for entry in &node.entries {
            let offset = entries_start + entry_index * ENTRY_SIZE;
            let name_offset = string_table.offset(&entry.name)?;
            match entry.kind {
                EntryKind::File(index) => {
                    let data = &file_data[index];
                    be::put_u16(&mut output, offset, entry.id)?;
                    ids_synchronized &= usize::from(entry.id) == entry_index;
                    let attributes = (entry.attributes & !(ATTR_COMPRESSED | ATTR_YAZ0)) | 1;
                    let flags_and_name = ((attributes as u32) << 24) | name_offset;
                    be::put_u32(&mut output, offset + 4, flags_and_name)?;
                    be::put_u32(
                        &mut output,
                        offset + 8,
                        u32::try_from(data_cursor - data_start).unwrap(),
                    )?;
                    be::put_u32(&mut output, offset + 0x0c, data.len() as u32)?;
                    be::bytes_mut(&mut output, data_cursor, data.len())?.copy_from_slice(data);
                    data_cursor += data.len();
                    data_cursor = align_up(data_cursor)?;
                }
                EntryKind::Directory(target) => {
                    let root_parent_sentinel = target == u32::MAX as usize && entry.name == b"..";
                    if !root_parent_sentinel && target >= nodes.len() {
                        return Err(FormatError::new("RARC directory link is out of range"));
                    }
                    be::put_u16(&mut output, offset, SPECIAL_ID)?;
                    let flags_and_name = ((entry.attributes as u32) << 24) | name_offset;
                    be::put_u32(&mut output, offset + 4, flags_and_name)?;
                    be::put_u32(&mut output, offset + 8, target as u32)?;
                }
            }
            be::put_u16(&mut output, offset + 2, name_hash(&entry.name))?;
            entry_index += 1;
        }
    }
    output[info + 0x1a] = u8::from(ids_synchronized);
    be::bytes_mut(&mut output, strings_start, string_table.bytes.len())?
        .copy_from_slice(&string_table.bytes);
    debug_assert_eq!(entry_index, entry_count);
    debug_assert_eq!(data_cursor, file_size);
    Ok(output)
}

#[derive(Default)]
struct StringTable {
    bytes: Vec<u8>,
    offsets: HashMap<Vec<u8>, u32>,
}

impl StringTable {
    fn intern(&mut self, name: &[u8]) -> Result<u32> {
        if let Some(offset) = self.offsets.get(name) {
            return Ok(*offset);
        }
        let offset = u32::try_from(self.bytes.len())
            .map_err(|_| FormatError::new("RARC string table is too large"))?;
        if offset > 0x00ff_ffff {
            return Err(FormatError::new(
                "RARC string offset exceeds the 24-bit file entry field",
            ));
        }
        self.bytes.extend_from_slice(name);
        self.bytes.push(0);
        self.offsets.insert(name.to_vec(), offset);
        Ok(offset)
    }

    fn offset(&self, name: &[u8]) -> Result<u32> {
        self.offsets
            .get(name)
            .copied()
            .ok_or_else(|| FormatError::new("missing interned RARC string"))
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn tiny_rarc(stored: &[u8], decoded_attributes: u8) -> Vec<u8> {
        let info = 0x20;
        let nodes_start = 0x40;
        let entries_start = 0x60;
        let strings_start = 0xa0;
        let data_start = 0xc0;
        let strings = b"root\0.\0..\0hello.txt\0";
        let data_size = (stored.len() + 0x1f) & !0x1f;
        let file_size = data_start + data_size;
        let mut bytes = vec![0u8; file_size];
        bytes[0..4].copy_from_slice(b"RARC");
        be::put_u32(&mut bytes, 4, file_size as u32).unwrap();
        be::put_u32(&mut bytes, 8, 0x20).unwrap();
        be::put_u32(&mut bytes, 0x0c, (data_start - 0x20) as u32).unwrap();
        be::put_u32(&mut bytes, 0x10, data_size as u32).unwrap();
        be::put_u32(&mut bytes, 0x14, data_size as u32).unwrap();
        be::put_u32(&mut bytes, info, 1).unwrap();
        be::put_u32(&mut bytes, info + 4, 0x20).unwrap();
        be::put_u32(&mut bytes, info + 8, 3).unwrap();
        be::put_u32(&mut bytes, info + 0x0c, 0x40).unwrap();
        be::put_u32(&mut bytes, info + 0x10, strings.len() as u32).unwrap();
        be::put_u32(&mut bytes, info + 0x14, 0x80).unwrap();
        be::put_u16(&mut bytes, info + 0x18, 1).unwrap();
        bytes[info + 0x1a] = 1;

        bytes[nodes_start..nodes_start + 4].copy_from_slice(b"ROOT");
        be::put_u16(&mut bytes, nodes_start + 8, name_hash(b"ROOT")).unwrap();
        be::put_u16(&mut bytes, nodes_start + 0x0a, 3).unwrap();
        for (index, (id, attributes, name, name_offset, data_offset, size)) in [
            (SPECIAL_ID, ATTR_DIRECTORY, &b"."[..], 5u32, 0u32, 0u32),
            (SPECIAL_ID, ATTR_DIRECTORY, &b".."[..], 7u32, 0u32, 0u32),
            (
                0,
                decoded_attributes,
                &b"hello.txt"[..],
                10u32,
                0u32,
                stored.len() as u32,
            ),
        ]
        .into_iter()
        .enumerate()
        {
            let offset = entries_start + index * ENTRY_SIZE;
            be::put_u16(&mut bytes, offset, id).unwrap();
            be::put_u16(&mut bytes, offset + 2, name_hash(name)).unwrap();
            be::put_u32(
                &mut bytes,
                offset + 4,
                ((attributes as u32) << 24) | name_offset,
            )
            .unwrap();
            be::put_u32(&mut bytes, offset + 8, data_offset).unwrap();
            be::put_u32(&mut bytes, offset + 0x0c, size).unwrap();
        }
        bytes[strings_start..strings_start + strings.len()].copy_from_slice(strings);
        bytes[data_start..data_start + stored.len()].copy_from_slice(stored);
        bytes
    }

    fn nested_rarc() -> Vec<u8> {
        let info = 0x20;
        let nodes_start = 0x40;
        let entries_start = 0x60;
        let strings_start = 0xe0;
        let data_start = 0x100;
        let strings = b"root\0sub\0.\0..\0hello.txt\0";
        let stored = b"nested";
        let file_size = data_start + ALIGNMENT;
        let mut bytes = vec![0u8; file_size];
        bytes[0..4].copy_from_slice(b"RARC");
        be::put_u32(&mut bytes, 4, file_size as u32).unwrap();
        be::put_u32(&mut bytes, 8, 0x20).unwrap();
        be::put_u32(&mut bytes, 0x0c, (data_start - 0x20) as u32).unwrap();
        be::put_u32(&mut bytes, 0x10, ALIGNMENT as u32).unwrap();
        be::put_u32(&mut bytes, 0x14, ALIGNMENT as u32).unwrap();
        be::put_u32(&mut bytes, info, 2).unwrap();
        be::put_u32(&mut bytes, info + 4, 0x20).unwrap();
        be::put_u32(&mut bytes, info + 8, 6).unwrap();
        be::put_u32(&mut bytes, info + 0x0c, 0x40).unwrap();
        be::put_u32(&mut bytes, info + 0x10, strings.len() as u32).unwrap();
        be::put_u32(&mut bytes, info + 0x14, 0xc0).unwrap();
        be::put_u16(&mut bytes, info + 0x18, 1).unwrap();
        bytes[info + 0x1a] = 1;

        for (index, (kind, name, name_offset, first_entry)) in [
            (*b"ROOT", &b"root"[..], 0u32, 0u32),
            (*b"SUB ", &b"sub"[..], 5u32, 3u32),
        ]
        .into_iter()
        .enumerate()
        {
            let offset = nodes_start + index * NODE_SIZE;
            bytes[offset..offset + 4].copy_from_slice(&kind);
            be::put_u32(&mut bytes, offset + 4, name_offset).unwrap();
            be::put_u16(&mut bytes, offset + 8, name_hash(name)).unwrap();
            be::put_u16(&mut bytes, offset + 0x0a, 3).unwrap();
            be::put_u32(&mut bytes, offset + 0x0c, first_entry).unwrap();
        }

        for (index, (id, attributes, name, name_offset, data_offset, size)) in [
            (SPECIAL_ID, ATTR_DIRECTORY, &b"."[..], 9u32, 0u32, 0u32),
            (SPECIAL_ID, ATTR_DIRECTORY, &b".."[..], 11u32, 0u32, 0u32),
            (SPECIAL_ID, ATTR_DIRECTORY, &b"sub"[..], 5u32, 1u32, 0u32),
            (SPECIAL_ID, ATTR_DIRECTORY, &b"."[..], 9u32, 1u32, 0u32),
            (SPECIAL_ID, ATTR_DIRECTORY, &b".."[..], 11u32, 0u32, 0u32),
            (0, 0x11, &b"hello.txt"[..], 14u32, 0u32, stored.len() as u32),
        ]
        .into_iter()
        .enumerate()
        {
            let offset = entries_start + index * ENTRY_SIZE;
            be::put_u16(&mut bytes, offset, id).unwrap();
            be::put_u16(&mut bytes, offset + 2, name_hash(name)).unwrap();
            be::put_u32(
                &mut bytes,
                offset + 4,
                ((attributes as u32) << 24) | name_offset,
            )
            .unwrap();
            be::put_u32(&mut bytes, offset + 8, data_offset).unwrap();
            be::put_u32(&mut bytes, offset + 0x0c, size).unwrap();
        }
        bytes[strings_start..strings_start + strings.len()].copy_from_slice(strings);
        bytes[data_start..data_start + stored.len()].copy_from_slice(stored);
        bytes
    }

    fn literal_yaz0(decoded: &[u8]) -> Vec<u8> {
        assert!(decoded.len() <= u32::MAX as usize);
        let mut output = Vec::new();
        output.extend_from_slice(b"Yaz0");
        output.extend_from_slice(&(decoded.len() as u32).to_be_bytes());
        output.extend_from_slice(&[0; 8]);
        for chunk in decoded.chunks(8) {
            output.push(0xff << (8 - chunk.len()));
            output.extend_from_slice(chunk);
        }
        output
    }

    #[test]
    fn parses_replaces_and_reparses_synthetic_rarc() {
        let original = tiny_rarc(b"old!", 0x11);
        let mut archive = RarcArchive::new(&original).unwrap();
        assert_eq!(archive.files().len(), 1);
        assert_eq!(archive.files()[0].path, "root/hello.txt");
        assert_eq!(archive.files()[0].name, "hello.txt");
        assert_eq!(archive.files()[0].stored_size, 4);
        assert!(!archive.files()[0].compressed);
        assert_eq!(archive.file_data(0).unwrap(), b"old!");
        assert_eq!(compression::detect(&original), Compression::None);
        assert_eq!(archive.original_outer_compression(), Compression::None);
        assert!(!archive.is_dirty());
        assert_eq!(archive.export().unwrap(), original);

        archive.replace_file(0, b"old!").unwrap();
        assert!(!archive.is_dirty());
        archive.replace_file(0, b"temporary").unwrap();
        assert!(archive.is_dirty());
        archive.replace_file(0, b"old!").unwrap();
        assert!(!archive.is_dirty());
        assert_eq!(archive.export().unwrap(), original);
        archive.replace_file(0, b"replacement bytes").unwrap();
        assert!(archive.is_dirty());
        assert_eq!(archive.original_bytes(), original);
        assert!(archive.file_data(1).is_err());
        assert!(archive.replace_file(1, b"bad").is_err());

        let exported = archive.export().unwrap();
        assert!(exported.starts_with(b"RARC"));
        assert_eq!(exported.len() % ALIGNMENT, 0);
        assert_eq!(be::u16_at(&exported, 0x38).unwrap(), 1);
        let reparsed = RarcArchive::parse(&exported).unwrap();
        assert_eq!(reparsed.files().len(), 1);
        assert_eq!(reparsed.files()[0].path, "root/hello.txt");
        assert_eq!(reparsed.file_data(0).unwrap(), b"replacement bytes");
        assert_eq!(reparsed.export().unwrap(), exported);
    }

    #[test]
    fn preserves_nested_directories_and_aligned_sections() {
        let original = nested_rarc();
        let mut archive = RarcArchive::new(&original).unwrap();
        assert_eq!(archive.files()[0].path, "root/sub/hello.txt");
        assert_eq!(archive.file_data(0).unwrap(), b"nested");
        archive.replace_file(0, b"nested replacement").unwrap();

        let exported = archive.export().unwrap();
        let info = be::u32_at(&exported, 8).unwrap() as usize;
        for relative_offset in [0x0c, info + 4, info + 0x0c, info + 0x14] {
            let absolute = info + be::u32_at(&exported, relative_offset).unwrap() as usize;
            assert_eq!(absolute % ALIGNMENT, 0);
        }
        assert_eq!(be::u16_at(&exported, info + 0x18).unwrap(), 1);
        let reparsed = RarcArchive::new(&exported).unwrap();
        assert_eq!(reparsed.files()[0].path, "root/sub/hello.txt");
        assert_eq!(reparsed.file_data(0).unwrap(), b"nested replacement");
        assert_eq!(reparsed.export().unwrap(), exported);
    }

    #[test]
    fn preserves_raw_names_for_lossy_utf8_reexport() {
        let mut original = tiny_rarc(b"old!", 0x11);
        original[0xa0 + 10] = 0xff;
        let mut archive = RarcArchive::new(&original).unwrap();
        let lossy_name = archive.files()[0].name.clone();
        assert!(lossy_name.contains('\u{fffd}'));
        archive.replace_file(0, b"new contents").unwrap();
        let exported = archive.export().unwrap();
        let reparsed = RarcArchive::new(&exported).unwrap();
        assert_eq!(reparsed.files()[0].name, lossy_name);
        assert_eq!(reparsed.file_data(0).unwrap(), b"new contents");
        assert_eq!(reparsed.export().unwrap(), exported);
    }

    #[test]
    fn decodes_compressed_entries_and_outer_yaz0() {
        let compressed = literal_yaz0(b"inner contents");
        let original = tiny_rarc(&compressed, 1 | ATTR_COMPRESSED | ATTR_YAZ0);
        let archive = RarcArchive::new(&original).unwrap();
        assert!(archive.files()[0].compressed);
        assert_eq!(archive.files()[0].stored_size as usize, compressed.len());
        assert_eq!(archive.file_data(0).unwrap(), b"inner contents");

        let yay0 = [
            b'Y', b'a', b'y', b'0', 0, 0, 0, 4, 0, 0, 0, 20, 0, 0, 0, 20, 0xf0, 0, 0, 0, b'Y',
            b'a', b'y', b'0',
        ];
        let yay0_archive = RarcArchive::new(tiny_rarc(&yay0, 1 | ATTR_COMPRESSED)).unwrap();
        assert_eq!(yay0_archive.file_data(0).unwrap(), b"Yay0");

        let outer = literal_yaz0(&original);
        let mut outer_archive = RarcArchive::new(&outer).unwrap();
        assert_eq!(
            outer_archive.original_outer_compression(),
            Compression::Yaz0
        );
        assert_eq!(outer_archive.export().unwrap(), outer);
        outer_archive.replace_file(0, b"raw replacement").unwrap();
        let exported = outer_archive.export().unwrap();
        assert!(exported.starts_with(b"RARC"));
        let reparsed = RarcArchive::new(&exported).unwrap();
        assert_eq!(reparsed.file_data(0).unwrap(), b"raw replacement");
        assert!(!reparsed.files()[0].compressed);
    }

    #[test]
    fn rejects_cycles_duplicate_ids_and_excessive_counts() {
        let mut cycle = tiny_rarc(b"file", 0x11);
        let file_entry = 0x60 + 2 * ENTRY_SIZE;
        be::put_u32(
            &mut cycle,
            file_entry + 4,
            (ATTR_DIRECTORY as u32) << 24 | 10,
        )
        .unwrap();
        assert!(RarcArchive::new(&cycle).is_err());

        let mut duplicate = nested_rarc();
        let directory_entry = 0x60 + 2 * ENTRY_SIZE;
        be::put_u16(&mut duplicate, directory_entry, 0).unwrap();
        be::put_u32(&mut duplicate, directory_entry + 4, 0x1100_0005).unwrap();
        assert!(RarcArchive::new(&duplicate).is_err());

        let mut overlap = nested_rarc();
        be::put_u32(&mut overlap, 0x40 + NODE_SIZE + 0x0c, 0).unwrap();
        assert!(RarcArchive::new(&overlap).is_err());

        let mut excessive = tiny_rarc(b"file", 0x11);
        be::put_u32(&mut excessive, 0x20, (MAX_NODES + 1) as u32).unwrap();
        assert!(RarcArchive::new(&excessive).is_err());

        let mut malformed = tiny_rarc(b"file", 0x11);
        malformed.truncate(0x30);
        assert!(RarcArchive::new(&malformed).is_err());
        assert!(RarcArchive::new(b"not an archive").is_err());
    }

    #[test]
    fn hashes_names_case_insensitively() {
        assert_eq!(name_hash(b"Hello.TXT"), name_hash(b"hello.txt"));
        let hash = b"abc".iter().fold(0u16, |hash, byte| {
            hash.wrapping_mul(3).wrapping_add(*byte as u16)
        });
        assert_eq!(name_hash(b"ABC"), hash);
    }
}
