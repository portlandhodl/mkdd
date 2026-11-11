use crate::error::{FormatError, Result};
use crate::formats::be;

pub const J3D_HEADER_SIZE: usize = 0x20;
pub const J3D_BLOCK_HEADER_SIZE: usize = 8;
pub const BTI_HEADER_SIZE: usize = 0x20;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum J3dKind {
    Model,
    Animation,
    Other,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct J3dBlock {
    pub tag: String,
    pub offset: usize,
    pub size: usize,
}

impl J3dBlock {
    pub fn end_offset(&self) -> usize {
        self.offset + self.size
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct J3dInfo {
    /// The four-byte J3D container version, such as `J3D2`.
    pub container: String,
    /// The four-byte file revision, such as `bmd2` or `bck1`.
    pub revision: String,
    pub declared_size: u32,
    pub declared_blocks: u32,
    pub blocks: Vec<J3dBlock>,
    pub kind: J3dKind,
    /// Bytes left after the declared block sequence, including bytes inside or
    /// beyond the declared file size.
    pub trailing_bytes: usize,
    /// Absolute offsets of the 0x20-byte BTI headers found in TEX1 blocks.
    pub bti_offsets: Vec<usize>,
    /// Absolute image-data offsets resolved against each corresponding BTI
    /// header. Entries have the same order and indices as `bti_offsets`.
    pub bti_data_offsets: Vec<usize>,
}

impl J3dInfo {
    pub fn is_model(&self) -> bool {
        self.kind == J3dKind::Model
    }

    pub fn is_animation(&self) -> bool {
        self.kind == J3dKind::Animation
    }
}

pub fn inspect(data: &[u8]) -> Result<J3dInfo> {
    be::bytes(data, 0, J3D_HEADER_SIZE)?;

    let container_bytes = be::bytes(data, 0, 4)?;
    if container_bytes != b"J3D1" && container_bytes != b"J3D2" {
        return Err(FormatError::at(0, "unsupported J3D container"));
    }

    let revision_bytes: [u8; 4] = be::bytes(data, 4, 4)?.try_into().unwrap();
    let container = String::from_utf8_lossy(container_bytes).into_owned();
    let revision = String::from_utf8_lossy(&revision_bytes).into_owned();
    let declared_size = be::u32_at(data, 8)?;
    let declared_blocks = be::u32_at(data, 12)?;
    let file_size = declared_size as usize;

    if file_size < J3D_HEADER_SIZE {
        return Err(FormatError::at(
            8,
            format!("declared J3D size 0x{file_size:x} is smaller than the header"),
        ));
    }
    if file_size > data.len() {
        return Err(FormatError::at(
            8,
            format!(
                "declared J3D size 0x{file_size:x} exceeds available 0x{:x} bytes",
                data.len()
            ),
        ));
    }

    let block_count = declared_blocks as usize;
    let maximum_blocks = (file_size - J3D_HEADER_SIZE) / J3D_BLOCK_HEADER_SIZE;
    if block_count > maximum_blocks {
        return Err(FormatError::at(
            12,
            format!(
                "declared block count {declared_blocks} exceeds the {maximum_blocks} block headers that fit"
            ),
        ));
    }

    let mut blocks = Vec::new();
    let mut bti_offsets = Vec::new();
    let mut bti_data_offsets = Vec::new();
    let mut offset = J3D_HEADER_SIZE;

    for index in 0..block_count {
        be::range(&data[..file_size], offset, J3D_BLOCK_HEADER_SIZE).map_err(|_| {
            FormatError::at(
                offset,
                format!("missing header for block {index} of {declared_blocks}"),
            )
        })?;

        let tag = String::from_utf8_lossy(be::bytes(data, offset, 4)?).into_owned();
        let size = be::u32_at(data, offset + 4)? as usize;
        if size < J3D_BLOCK_HEADER_SIZE {
            return Err(FormatError::at(
                offset + 4,
                format!("{tag} block size {size} is smaller than 8 bytes"),
            ));
        }

        let next = offset
            .checked_add(size)
            .ok_or_else(|| FormatError::at(offset + 4, "block offset overflow"))?;
        if next > file_size {
            return Err(FormatError::at(
                offset + 4,
                format!(
                    "{tag} block ends at 0x{next:x}, beyond declared file size 0x{file_size:x}"
                ),
            ));
        }
        if next <= offset {
            return Err(FormatError::at(offset + 4, "block parser made no progress"));
        }

        let block = J3dBlock { tag, offset, size };
        if block.tag == "TEX1" {
            inspect_tex1(data, &block, &mut bti_offsets, &mut bti_data_offsets)?;
        }
        offset = next;
        blocks.push(block);
    }

    Ok(J3dInfo {
        container,
        kind: classify_revision(&revision_bytes),
        revision,
        declared_size,
        declared_blocks,
        blocks,
        trailing_bytes: data.len() - offset,
        bti_offsets,
        bti_data_offsets,
    })
}

fn classify_revision(revision: &[u8; 4]) -> J3dKind {
    match &revision[..3] {
        b"bmd" | b"bdl" => J3dKind::Model,
        b"bck" | b"bca" | b"blk" | b"bla" | b"bpk" | b"bpa" | b"btk" | b"brk" | b"btp" | b"bva"
        | b"bxk" | b"bxa" => J3dKind::Animation,
        _ => J3dKind::Other,
    }
}

fn inspect_tex1(
    data: &[u8],
    block: &J3dBlock,
    bti_offsets: &mut Vec<usize>,
    bti_data_offsets: &mut Vec<usize>,
) -> Result<()> {
    const TEX1_HEADER_SIZE: usize = 0x14;
    const IMAGE_DATA_OFFSET: usize = 0x1c;

    if block.size < TEX1_HEADER_SIZE {
        return Err(FormatError::at(
            block.offset + 4,
            "TEX1 block is smaller than its header",
        ));
    }

    let count = be::u16_at(data, block.offset + 8)? as usize;
    if count == 0 {
        return Ok(());
    }

    let headers_relative = be::u32_at(data, block.offset + 12)? as usize;
    if headers_relative < TEX1_HEADER_SIZE {
        return Err(FormatError::at(
            block.offset + 12,
            "TEX1 texture headers overlap the TEX1 header",
        ));
    }

    let headers_size = count
        .checked_mul(BTI_HEADER_SIZE)
        .ok_or_else(|| FormatError::at(block.offset + 8, "TEX1 texture count overflow"))?;
    let headers_start = relative_to_block(block, block.offset, headers_relative)?;
    relative_to_block(block, headers_start, headers_size)?;

    for index in 0..count {
        let relative = index
            .checked_mul(BTI_HEADER_SIZE)
            .ok_or_else(|| FormatError::at(block.offset + 8, "TEX1 header offset overflow"))?;
        let header_offset = headers_start
            .checked_add(relative)
            .ok_or_else(|| FormatError::at(block.offset + 8, "TEX1 header offset overflow"))?;
        let stored_data_relative = be::u32_at(data, header_offset + IMAGE_DATA_OFFSET)? as usize;
        let data_relative = if stored_data_relative == 0 {
            BTI_HEADER_SIZE
        } else {
            stored_data_relative
        };
        let data_offset = header_offset.checked_add(data_relative).ok_or_else(|| {
            FormatError::at(
                header_offset + IMAGE_DATA_OFFSET,
                "BTI data offset overflow",
            )
        })?;
        if data_offset > block.end_offset() {
            return Err(FormatError::at(
                header_offset + IMAGE_DATA_OFFSET,
                format!(
                    "BTI data offset 0x{data_offset:x} is beyond TEX1 block end 0x{:x}",
                    block.end_offset()
                ),
            ));
        }

        bti_offsets.push(header_offset);
        bti_data_offsets.push(data_offset);
    }

    Ok(())
}

fn relative_to_block(block: &J3dBlock, base: usize, relative: usize) -> Result<usize> {
    let result = base
        .checked_add(relative)
        .ok_or_else(|| FormatError::at(block.offset, "TEX1 offset overflow"))?;
    if result < block.offset || result > block.end_offset() {
        return Err(FormatError::at(
            block.offset,
            format!(
                "TEX1 offset 0x{result:x} is outside block 0x{:x}..0x{:x}",
                block.offset,
                block.end_offset()
            ),
        ));
    }
    Ok(result)
}

#[cfg(test)]
mod tests {
    use super::*;

    fn put_u16(data: &mut [u8], offset: usize, value: u16) {
        data[offset..offset + 2].copy_from_slice(&value.to_be_bytes());
    }

    fn put_u32(data: &mut [u8], offset: usize, value: u32) {
        data[offset..offset + 4].copy_from_slice(&value.to_be_bytes());
    }

    fn block(tag: &[u8; 4], size: usize) -> Vec<u8> {
        let mut data = vec![0; size];
        data[..4].copy_from_slice(tag);
        put_u32(&mut data, 4, size as u32);
        data
    }

    fn file(
        container: &[u8; 4],
        revision: &[u8; 4],
        blocks: &[&[u8]],
        declared_tail: usize,
        physical_tail: usize,
    ) -> Vec<u8> {
        let blocks_size = blocks.iter().map(|block| block.len()).sum::<usize>();
        let declared_size = J3D_HEADER_SIZE + blocks_size + declared_tail;
        let mut data = vec![0; declared_size + physical_tail];
        data[..4].copy_from_slice(container);
        data[4..8].copy_from_slice(revision);
        put_u32(&mut data, 8, declared_size as u32);
        put_u32(&mut data, 12, blocks.len() as u32);
        let mut offset = J3D_HEADER_SIZE;
        for block in blocks {
            data[offset..offset + block.len()].copy_from_slice(block);
            offset += block.len();
        }
        data
    }

    #[test]
    fn reads_model_blocks_and_embedded_bti_offsets() {
        let mut tex1 = block(b"TEX1", 0x84);
        put_u16(&mut tex1, 8, 2);
        put_u32(&mut tex1, 12, 0x20);
        put_u32(&mut tex1, 0x20 + 0x1c, 0x40);
        put_u32(&mut tex1, 0x40 + 0x1c, 0x22);
        let data = file(b"J3D2", b"bmd2", &[&tex1], 0, 0);

        let info = inspect(&data).unwrap();
        assert_eq!(info.container, "J3D2");
        assert_eq!(info.revision, "bmd2");
        assert_eq!(info.declared_size, 0xa4);
        assert_eq!(info.declared_blocks, 1);
        assert!(info.is_model());
        assert!(!info.is_animation());
        assert_eq!(
            info.blocks,
            vec![J3dBlock {
                tag: "TEX1".into(),
                offset: 0x20,
                size: 0x84,
            }]
        );
        assert_eq!(info.bti_offsets, [0x40, 0x60]);
        assert_eq!(info.bti_data_offsets, [0x80, 0x82]);
        assert_eq!(info.trailing_bytes, 0);
    }

    #[test]
    fn allows_unknown_blocks_and_reports_all_trailing_bytes() {
        let unknown = block(b"WXYZ", 8);
        let data = file(b"J3D2", b"bmt3", &[&unknown], 8, 3);

        let info = inspect(&data).unwrap();
        assert_eq!(info.kind, J3dKind::Other);
        assert_eq!(info.blocks[0].tag, "WXYZ");
        assert_eq!(info.blocks[0].offset, 0x20);
        assert_eq!(info.blocks[0].size, 8);
        assert_eq!(info.trailing_bytes, 11);
        assert!(info.bti_offsets.is_empty());
    }

    #[test]
    fn classifies_model_and_animation_revisions() {
        for revision in [b"bmd1", b"bmd2", b"bmd3", b"bdl3", b"bdl4"] {
            let data = file(b"J3D2", revision, &[], 0, 0);
            assert!(inspect(&data).unwrap().is_model(), "{revision:?}");
        }

        for revision in [
            b"bck1", b"bca1", b"blk1", b"bpk1", b"btk1", b"brk1", b"btp1", b"bva1",
        ] {
            let data = file(b"J3D1", revision, &[], 0, 0);
            assert!(inspect(&data).unwrap().is_animation(), "{revision:?}");
        }
    }

    #[test]
    fn rejects_short_or_unsupported_headers() {
        assert!(inspect(&[0; 0x1f]).is_err());

        let data = file(b"RARC", b"bmd2", &[], 0, 0);
        let error = inspect(&data).unwrap_err();
        assert_eq!(error.offset, Some(0));

        let data = file(b"J3D3", b"bmd2", &[], 0, 0);
        assert!(inspect(&data).is_err());
    }

    #[test]
    fn rejects_bad_declared_sizes_and_impossible_block_counts() {
        let mut data = file(b"J3D2", b"bmd2", &[], 0, 0);
        put_u32(&mut data, 8, 0x1f);
        assert_eq!(inspect(&data).unwrap_err().offset, Some(8));

        let mut data = file(b"J3D2", b"bmd2", &[], 0, 0);
        put_u32(&mut data, 8, 0x24);
        assert_eq!(inspect(&data).unwrap_err().offset, Some(8));

        let unknown = block(b"WXYZ", 8);
        let mut data = file(b"J3D2", b"bmd2", &[&unknown], 0, 0);
        put_u32(&mut data, 12, 2);
        assert_eq!(inspect(&data).unwrap_err().offset, Some(12));

        let mut data = file(b"J3D2", b"bmd2", &[], 0, 0);
        put_u32(&mut data, 12, u32::MAX);
        assert!(inspect(&data).is_err());
    }

    #[test]
    fn rejects_zero_small_and_overrunning_blocks() {
        for size in [0, 4] {
            let mut invalid = block(b"WXYZ", 8);
            put_u32(&mut invalid, 4, size);
            let data = file(b"J3D2", b"bmd2", &[&invalid], 0, 0);
            assert_eq!(inspect(&data).unwrap_err().offset, Some(0x24));
        }

        let invalid = block(b"WXYZ", 12);
        let mut data = file(b"J3D2", b"bmd2", &[&invalid], 0, 0);
        put_u32(&mut data, 8, 0x28);
        assert_eq!(inspect(&data).unwrap_err().offset, Some(0x24));
    }

    #[test]
    fn rejects_invalid_tex1_tables_and_bti_data_offsets() {
        let mut tex1 = block(b"TEX1", 0x34);
        put_u16(&mut tex1, 8, 1);
        put_u32(&mut tex1, 12, 0x20);
        let data = file(b"J3D2", b"bmd2", &[&tex1], 0, 0);
        assert!(inspect(&data).is_err());

        put_u32(&mut tex1, 12, 0x10);
        let data = file(b"J3D2", b"bmd2", &[&tex1], 0, 0);
        assert!(inspect(&data).is_err());

        let mut tex1 = block(b"TEX1", 0x40);
        put_u16(&mut tex1, 8, 1);
        put_u32(&mut tex1, 12, 0x20);
        put_u32(&mut tex1, 0x20 + 0x1c, 0x21);
        let data = file(b"J3D2", b"bmd2", &[&tex1], 0, 0);
        assert!(inspect(&data).is_err());
    }

    #[test]
    fn accepts_empty_tex1_blocks() {
        let tex1 = block(b"TEX1", 0x14);
        let data = file(b"J3D2", b"bdl4", &[&tex1], 0, 0);
        let info = inspect(&data).unwrap();
        assert!(info.is_model());
        assert!(info.bti_offsets.is_empty());
        assert!(info.bti_data_offsets.is_empty());
    }
}
