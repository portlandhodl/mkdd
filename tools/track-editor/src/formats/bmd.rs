#![forbid(unsafe_code)]

use crate::error::{FormatError, Result};
use crate::formats::be;
use crate::formats::bti::BtiImage;
use crate::formats::gx::{
    GxAlphaTest, GxBlend, GxColorChannel, GxMaterial, GxTevOrder, GxTevStage, GxTexGen,
    GxTexMatrixInfo, GxZMode, MAX_TEV_STAGES, MAX_TEX_GENS,
};

const J3D_HEADER_SIZE: usize = 0x20;
const BLOCK_HEADER_SIZE: usize = 8;
const VTX1_HEADER_SIZE: usize = 0x40;
const VTX_FORMAT_SIZE: usize = 0x10;
const SHP1_HEADER_SIZE: usize = 0x2c;
const SHAPE_INIT_SIZE: usize = 0x28;
const MATERIAL_INIT_SIZE: usize = 0x14c;
const MAX_HIERARCHY_NODES: usize = 1_000_000;
const MAX_MESHES: usize = 65_535;
const MAX_VERTICES: usize = 2_000_000;
const MAX_INDICES: usize = 12_000_000;
const MAX_TEXTURES: usize = 2_048;
const MAX_TEXTURE_RGBA_BYTES: usize = 256 * 1024 * 1024;
const MAX_INFLUENCES: usize = 1_000_000;
const MAX_WARNINGS: usize = 512;
const GX_VA_PNMTXIDX: u8 = 0;
const GX_VA_POS: u8 = 9;
const GX_VA_NRM: u8 = 10;
const GX_VA_CLR0: u8 = 11;
const GX_VA_CLR1: u8 = 12;
const GX_VA_TEX0: u8 = 13;
const GX_VA_TEX7: u8 = 20;
const GX_VA_NBT: u8 = 25;
const GX_VA_NULL: u8 = 0xff;

#[derive(Debug, Clone, PartialEq)]
pub struct BmdModel {
    pub meshes: Vec<BmdMesh>,
    pub textures: Vec<BtiImage>,
    pub warnings: Vec<String>,
}

#[derive(Debug, Clone, PartialEq)]
pub struct BmdMesh {
    pub vertices: Vec<BmdVertex>,
    pub indices: Vec<u32>,
    pub material: BmdMaterial,
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub struct BmdVertex {
    pub position: [f32; 3],
    pub normal: [f32; 3],
    pub color: [f32; 4],
    pub color1: [f32; 4],
    pub tex_coords: [[f32; 2]; 8],
}

impl Default for BmdVertex {
    fn default() -> Self {
        Self {
            position: [0.0; 3],
            normal: [0.0; 3],
            color: [1.0; 4],
            color1: [1.0; 4],
            tex_coords: [[0.0; 2]; 8],
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub struct BmdMaterial {
    pub color: [f32; 4],
    pub texture: Option<usize>,
    pub cull_mode: BmdCullMode,
    pub use_vertex_color: bool,
    pub translucent: bool,
    /// Full GX state; `color`/`texture`/`use_vertex_color` are its basic summary.
    pub gx: GxMaterial,
}

impl Default for BmdMaterial {
    fn default() -> Self {
        Self {
            color: [1.0; 4],
            texture: None,
            cull_mode: BmdCullMode::None,
            use_vertex_color: true,
            translucent: false,
            gx: GxMaterial::default(),
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum BmdCullMode {
    None,
    Front,
    Back,
    All,
}

#[derive(Clone, Copy)]
struct Block {
    tag: [u8; 4],
    start: usize,
    end: usize,
}

impl Block {
    fn absolute(self, relative: usize, length: usize, what: &str) -> Result<usize> {
        let absolute = self
            .start
            .checked_add(relative)
            .ok_or_else(|| FormatError::at(self.start, format!("{what} offset overflow")))?;
        let end = absolute
            .checked_add(length)
            .ok_or_else(|| FormatError::at(absolute, format!("{what} range overflow")))?;
        if end > self.end {
            return Err(FormatError::at(
                absolute,
                format!(
                    "{what} needs {length} bytes beyond {:?} block end 0x{:x}",
                    String::from_utf8_lossy(&self.tag),
                    self.end
                ),
            ));
        }
        Ok(absolute)
    }

    fn relative_offset(self, relative: u32, length: usize, what: &str) -> Result<usize> {
        self.absolute(relative as usize, length, what)
    }

    fn bytes<'a>(
        self,
        data: &'a [u8],
        relative: usize,
        length: usize,
        what: &str,
    ) -> Result<&'a [u8]> {
        let absolute = self.absolute(relative, length, what)?;
        be::bytes(data, absolute, length)
    }

    fn u8(self, data: &[u8], relative: usize) -> Result<u8> {
        be::u8_at(data, self.absolute(relative, 1, "u8")?)
    }

    fn u16(self, data: &[u8], relative: usize) -> Result<u16> {
        be::u16_at(data, self.absolute(relative, 2, "u16")?)
    }

    fn i16(self, data: &[u8], relative: usize) -> Result<i16> {
        be::i16_at(data, self.absolute(relative, 2, "i16")?)
    }

    fn u32(self, data: &[u8], relative: usize) -> Result<u32> {
        be::u32_at(data, self.absolute(relative, 4, "u32")?)
    }

    fn f32(self, data: &[u8], relative: usize) -> Result<f32> {
        finite_f32(data, self.absolute(relative, 4, "f32")?)
    }
}

fn finite_f32(data: &[u8], offset: usize) -> Result<f32> {
    let value = be::f32_at(data, offset)?;
    if value.is_finite() {
        Ok(value)
    } else {
        Err(FormatError::at(offset, "non-finite floating-point value"))
    }
}

fn add_offset(base: usize, relative: usize, what: &str) -> Result<usize> {
    base.checked_add(relative)
        .ok_or_else(|| FormatError::at(base, format!("{what} offset overflow")))
}

fn mul_size(value: usize, stride: usize, offset: usize, what: &str) -> Result<usize> {
    value
        .checked_mul(stride)
        .ok_or_else(|| FormatError::at(offset, format!("{what} size overflow")))
}

fn reserve<T>(vec: &mut Vec<T>, additional: usize, what: &str) -> Result<()> {
    vec.try_reserve(additional)
        .map_err(|_| FormatError::new(format!("unable to allocate {what}")))
}

fn warn(warnings: &mut Vec<String>, message: impl Into<String>) {
    if warnings.len() < MAX_WARNINGS {
        warnings.push(message.into());
    } else if warnings.len() == MAX_WARNINGS {
        warnings.push("additional BMD warnings omitted".into());
    }
}

impl BmdModel {
    pub fn parse(data: &[u8]) -> Result<Self> {
        let blocks = parse_blocks(data)?;
        let info_block = unique_block(&blocks, *b"INF1")?
            .ok_or_else(|| FormatError::new("BMD has no INF1 block"))?;
        let vertex_block = unique_block(&blocks, *b"VTX1")?
            .ok_or_else(|| FormatError::new("BMD has no VTX1 block"))?;
        let shape_block = unique_block(&blocks, *b"SHP1")?
            .ok_or_else(|| FormatError::new("BMD has no SHP1 block"))?;
        let material_block = unique_block(&blocks, *b"MAT3")?;
        let old_material_block = unique_block(&blocks, *b"MAT2")?;
        let texture_block = unique_block(&blocks, *b"TEX1")?;
        let joint_block = unique_block(&blocks, *b"JNT1")?;
        let draw_block = unique_block(&blocks, *b"DRW1")?;
        let envelope_block = unique_block(&blocks, *b"EVP1")?;

        let info = Info::parse(data, info_block)?;
        let vertices = VertexData::parse(data, vertex_block)?;
        let shapes = ShapeData::parse(data, shape_block)?;
        let joints = JointData::parse(data, joint_block)?;
        let envelopes = EnvelopeData::parse(data, envelope_block)?;
        let draws = DrawData::parse(data, draw_block)?;
        let hierarchy = build_hierarchy(&info, &joints)?;

        let mut warnings = Vec::new();
        if old_material_block.is_some() && material_block.is_none() {
            warn(
                &mut warnings,
                "MAT2 materials are not decoded; all geometry uses neutral materials",
            );
        } else if material_block.is_none() {
            warn(
                &mut warnings,
                "BMD has no MAT3 block; all geometry uses neutral materials",
            );
        }

        let (textures, texture_map) = match texture_block {
            Some(block) => decode_textures(data, block, &mut warnings)?,
            None => (Vec::new(), Vec::new()),
        };
        let materials = match material_block {
            Some(block) => MaterialData::parse_all(data, block, &texture_map, &mut warnings)?,
            None => Vec::new(),
        };
        let material_table = MaterialData::new(material_block, &materials);
        let meshes = build_meshes(
            data,
            &info,
            &vertices,
            &shapes,
            &joints,
            &envelopes,
            &draws,
            &hierarchy,
            material_table,
            &mut warnings,
        )?;

        if meshes.is_empty() && !hierarchy.shapes.is_empty() {
            warn(
                &mut warnings,
                "INF1 shapes produced no renderable triangles",
            );
        }
        if hierarchy.shapes.len() != info.packet_count as usize {
            warn(
                &mut warnings,
                format!(
                    "INF1 declares {} packets but references {} shape occurrences",
                    info.packet_count,
                    hierarchy.shapes.len()
                ),
            );
        }

        Ok(Self {
            meshes,
            textures,
            warnings,
        })
    }
}

fn parse_blocks(data: &[u8]) -> Result<Vec<Block>> {
    let header = be::bytes(data, 0, J3D_HEADER_SIZE)?;
    let container = &header[0..4];
    let revision = &header[4..8];
    if container != b"J3D2" {
        return Err(FormatError::at(
            0,
            "unsupported BMD container (expected J3D2)",
        ));
    }
    if !matches!(revision, b"bmd2" | b"bmd3" | b"bdl3" | b"bdl4") {
        return Err(FormatError::at(
            4,
            format!(
                "unsupported BMD/BDL revision {:?}",
                String::from_utf8_lossy(revision)
            ),
        ));
    }

    let declared_size = be::u32_at(data, 8)? as usize;
    if declared_size < J3D_HEADER_SIZE || declared_size > data.len() {
        return Err(FormatError::at(
            8,
            format!("invalid declared J3D size 0x{declared_size:x}"),
        ));
    }
    let count = be::u32_at(data, 12)? as usize;
    let maximum = (declared_size - J3D_HEADER_SIZE) / BLOCK_HEADER_SIZE;
    if count > maximum {
        return Err(FormatError::at(12, "J3D block count exceeds file size"));
    }

    let mut blocks = Vec::new();
    reserve(&mut blocks, count, "BMD block table")?;
    let mut cursor = J3D_HEADER_SIZE;
    for _ in 0..count {
        let tag: [u8; 4] = be::bytes(data, cursor, 4)?
            .try_into()
            .map_err(|_| FormatError::at(cursor, "invalid four-byte J3D block tag"))?;
        let size = be::u32_at(data, cursor + 4)? as usize;
        if size < BLOCK_HEADER_SIZE {
            return Err(FormatError::at(cursor + 4, "J3D block is too small"));
        }
        let end = cursor
            .checked_add(size)
            .ok_or_else(|| FormatError::at(cursor + 4, "J3D block size overflow"))?;
        if end > declared_size {
            return Err(FormatError::at(
                cursor + 4,
                "J3D block extends beyond declared file size",
            ));
        }
        blocks.push(Block {
            tag,
            start: cursor,
            end,
        });
        cursor = end;
    }
    Ok(blocks)
}

fn unique_block(blocks: &[Block], tag: [u8; 4]) -> Result<Option<Block>> {
    let mut found = None;
    for block in blocks.iter().copied().filter(|block| block.tag == tag) {
        if found.is_some() {
            return Err(FormatError::at(
                block.start,
                format!("duplicate {:?} block", String::from_utf8_lossy(&tag)),
            ));
        }
        found = Some(block);
    }
    Ok(found)
}

#[derive(Clone, Copy)]
struct HierarchyNode {
    kind: u16,
    value: u16,
}

struct Info {
    flags: u16,
    packet_count: u32,
    nodes: Vec<HierarchyNode>,
}

impl Info {
    fn parse(data: &[u8], block: Block) -> Result<Self> {
        block.absolute(0, 0x18, "INF1 header")?;
        let flags = block.u16(data, 8)?;
        if flags & 0x0f > 2 {
            return Err(FormatError::at(
                block.start + 8,
                "unsupported INF1 matrix calculation mode",
            ));
        }
        let packet_count = block.u32(data, 0x0c)?;
        let hierarchy_offset = block.u32(data, 0x14)? as usize;
        block.absolute(hierarchy_offset, 4, "INF1 hierarchy")?;

        let mut nodes = Vec::new();
        let mut cursor = hierarchy_offset;
        loop {
            if nodes.len() >= MAX_HIERARCHY_NODES {
                return Err(FormatError::at(
                    block.start + 0x14,
                    "INF1 hierarchy exceeds node limit",
                ));
            }
            let kind = block.u16(data, cursor)?;
            let value = block.u16(data, cursor + 2)?;
            cursor = cursor
                .checked_add(4)
                .ok_or_else(|| FormatError::at(block.start, "INF1 hierarchy overflow"))?;
            if kind == 0 {
                break;
            }
            reserve(&mut nodes, 1, "INF1 hierarchy")?;
            nodes.push(HierarchyNode { kind, value });
        }

        Ok(Self {
            flags,
            packet_count,
            nodes,
        })
    }
}

#[derive(Clone, Copy)]
struct Joint {
    scale: [f32; 3],
    rotation: [i16; 3],
    translation: [f32; 3],
    ignore_parent_scale: bool,
}

struct JointData {
    joints: Vec<Joint>,
}

impl JointData {
    fn parse(data: &[u8], block: Option<Block>) -> Result<Self> {
        let Some(block) = block else {
            return Ok(Self { joints: Vec::new() });
        };
        block.absolute(0, 0x18, "JNT1 header")?;
        let count = usize::from(block.u16(data, 8)?);
        let init_offset = block.u32(data, 0x0c)?;
        let remap_offset = block.u32(data, 0x10)?;
        block.relative_offset(
            remap_offset,
            mul_size(count, 2, block.start + 0x10, "JNT1 remap")?,
            "JNT1 remap",
        )?;

        let mut joints = Vec::new();
        reserve(&mut joints, count, "JNT1 joints")?;
        for index in 0..count {
            let mapped = usize::from(be::u16_at(
                data,
                block.relative_offset(remap_offset, 0, "JNT1 remap")? + index * 2,
            )?);
            let base = add_offset(
                init_offset as usize,
                mul_size(mapped, 0x40, block.start + 0x0c, "JNT1 init index")?,
                "JNT1 init data",
            )?;
            block.absolute(base, 0x40, "JNT1 init data")?;
            joints.push(Joint {
                scale: [
                    block.f32(data, base + 4)?,
                    block.f32(data, base + 8)?,
                    block.f32(data, base + 0x0c)?,
                ],
                rotation: [
                    block.i16(data, base + 0x10)?,
                    block.i16(data, base + 0x12)?,
                    block.i16(data, base + 0x14)?,
                ],
                translation: [
                    block.f32(data, base + 0x18)?,
                    block.f32(data, base + 0x1c)?,
                    block.f32(data, base + 0x20)?,
                ],
                ignore_parent_scale: block.u8(data, base + 2)? == 1,
            });
        }
        Ok(Self { joints })
    }
}

type Matrix = [[f32; 4]; 3];
const IDENTITY: Matrix = [
    [1.0, 0.0, 0.0, 0.0],
    [0.0, 1.0, 0.0, 0.0],
    [0.0, 0.0, 1.0, 0.0],
];

fn concat(a: Matrix, b: Matrix) -> Result<Matrix> {
    let mut out = [[0.0; 4]; 3];
    for (row, out_row) in out.iter_mut().enumerate() {
        for (column, value) in out_row.iter_mut().enumerate() {
            let mut sum = 0.0;
            for inner in 0..3 {
                sum += a[row][inner] * b[inner][column];
            }
            if column == 3 {
                sum += a[row][3];
            }
            if !sum.is_finite() {
                return Err(FormatError::new("non-finite BMD matrix result"));
            }
            *value = sum;
        }
    }
    Ok(out)
}

fn rotation_translation(joint: &Joint) -> Matrix {
    let angle = |value: i16| f32::from(value) * (std::f32::consts::TAU / 65536.0);
    let (sx, cx) = angle(joint.rotation[0]).sin_cos();
    let (sy, cy) = angle(joint.rotation[1]).sin_cos();
    let (sz, cz) = angle(joint.rotation[2]).sin_cos();
    let mut matrix = [
        [
            cz * cy,
            sx * cz * sy - cx * sz,
            cx * cz * sy + sx * sz,
            joint.translation[0],
        ],
        [
            sz * cy,
            sx * sz * sy + cx * cz,
            cx * sz * sy - sx * cz,
            joint.translation[1],
        ],
        [-sy, cy * sx, cy * cx, joint.translation[2]],
    ];
    for row in &mut matrix {
        for (column, value) in row.iter_mut().enumerate().take(3) {
            *value *= joint.scale[column];
        }
    }
    matrix
}

#[derive(Clone, Copy)]
struct WorldJoint {
    world: Matrix,
    base: Matrix,
    accumulated_scale: [f32; 3],
    joint_scale: [f32; 3],
    visited: bool,
}

impl WorldJoint {
    fn unvisited() -> Self {
        Self {
            world: IDENTITY,
            base: IDENTITY,
            accumulated_scale: [1.0; 3],
            joint_scale: [1.0; 3],
            visited: false,
        }
    }

    fn root() -> Self {
        Self {
            visited: true,
            ..Self::unvisited()
        }
    }

    fn child(self, joint: &Joint, mode: u16) -> Result<Self> {
        let mut next = Self::root();
        next.accumulated_scale = [
            self.accumulated_scale[0] * joint.scale[0],
            self.accumulated_scale[1] * joint.scale[1],
            self.accumulated_scale[2] * joint.scale[2],
        ];
        if !next.accumulated_scale.into_iter().all(f32::is_finite) {
            return Err(FormatError::new("non-finite JNT1 accumulated scale"));
        }

        match mode {
            0 | 2 => {
                let mut local = rotation_translation(joint);
                if mode == 2 && joint.ignore_parent_scale {
                    let parent_scale = self.joint_scale;
                    for (row, scale) in parent_scale.into_iter().enumerate() {
                        if scale == 0.0 || !scale.is_finite() {
                            return Err(FormatError::new(
                                "cannot compensate a zero/non-finite parent JNT1 scale",
                            ));
                        }
                        for value in local[row].iter_mut().take(3) {
                            *value /= scale;
                        }
                    }
                }
                next.world = concat(self.world, local)?;
                next.base = next.world;
            }
            1 => {
                let mut local_joint = *joint;
                local_joint.scale = [1.0; 3];
                for (axis, translation) in local_joint.translation.iter_mut().enumerate() {
                    *translation *= self.accumulated_scale[axis];
                }
                next.base = concat(self.base, rotation_translation(&local_joint))?;
                let mut scaled = next.base;
                for row in &mut scaled {
                    for (column, value) in row.iter_mut().enumerate().take(3) {
                        *value *= next.accumulated_scale[column];
                    }
                }
                next.world = scaled;
            }
            _ => return Err(FormatError::new("unsupported JNT1 matrix calculation mode")),
        }
        next.joint_scale = joint.scale;
        Ok(next)
    }
}

#[derive(Clone, Copy)]
struct ShapeLink {
    shape: u16,
    material: Option<u16>,
    joint: Option<u16>,
}

struct Hierarchy {
    shapes: Vec<ShapeLink>,
    worlds: Vec<WorldJoint>,
}

fn build_hierarchy(info: &Info, joints: &JointData) -> Result<Hierarchy> {
    let mode = info.flags & 0x0f;
    let mut worlds = Vec::new();
    reserve(&mut worlds, joints.joints.len(), "joint world matrices")?;
    worlds.resize(joints.joints.len(), WorldJoint::unvisited());
    let mut shapes = Vec::new();
    reserve(
        &mut shapes,
        info.nodes.len().min(MAX_MESHES),
        "INF1 shape links",
    )?;
    let mut stack: Vec<(Option<u16>, Option<u16>)> = Vec::new();
    let mut current_joint = None;
    let mut current_material = None;

    for node in &info.nodes {
        match node.kind {
            1 => {
                if stack.len() >= 4096 {
                    return Err(FormatError::new("INF1 hierarchy is too deep"));
                }
                reserve(&mut stack, 1, "INF1 hierarchy stack")?;
                stack.push((current_joint, current_material));
            }
            2 => {
                let Some((joint, material)) = stack.pop() else {
                    return Err(FormatError::new("unbalanced INF1 close node"));
                };
                current_joint = joint;
                current_material = material;
            }
            0x10 => {
                let index = usize::from(node.value);
                if worlds.get(index).is_none() {
                    return Err(FormatError::new(format!(
                        "INF1 references missing JNT1 joint {}",
                        node.value
                    )));
                }
                if worlds[index].visited {
                    return Err(FormatError::new(format!(
                        "INF1 joint {} occurs more than once",
                        node.value
                    )));
                }
                let parent = match current_joint {
                    Some(parent) => worlds
                        .get(usize::from(parent))
                        .copied()
                        .filter(|world| world.visited)
                        .ok_or_else(|| {
                            FormatError::new("INF1 parent joint has no world transform")
                        })?,
                    None => WorldJoint::root(),
                };
                worlds[index] = parent.child(&joints.joints[index], mode)?;
                current_joint = Some(node.value);
            }
            0x11 => current_material = Some(node.value),
            0x12 => {
                if shapes.len() >= MAX_MESHES {
                    return Err(FormatError::new("INF1 shape count exceeds mesh limit"));
                }
                shapes.push(ShapeLink {
                    shape: node.value,
                    material: current_material,
                    joint: current_joint,
                });
            }
            _ => {
                return Err(FormatError::new(format!(
                    "unknown INF1 hierarchy command 0x{:04x}",
                    node.kind
                )));
            }
        }
    }
    if !stack.is_empty() {
        return Err(FormatError::new("unterminated INF1 hierarchy scope"));
    }
    Ok(Hierarchy { shapes, worlds })
}

#[derive(Clone, Copy)]
struct EnvelopeRange {
    first: usize,
    count: usize,
}

struct EnvelopeData {
    block: Option<Block>,
    ranges: Vec<EnvelopeRange>,
}

impl EnvelopeData {
    fn parse(data: &[u8], block: Option<Block>) -> Result<Self> {
        let Some(block) = block else {
            return Ok(Self {
                block: None,
                ranges: Vec::new(),
            });
        };
        block.absolute(0, 0x1c, "EVP1 header")?;
        let count = usize::from(block.u16(data, 8)?);
        let counts_offset = block.u32(data, 0x0c)?;
        let indices_offset = block.u32(data, 0x10)?;
        let weights_offset = block.u32(data, 0x14)?;
        block.relative_offset(counts_offset, count, "EVP1 influence counts")?;
        let counts_base = block.relative_offset(counts_offset, 0, "EVP1 influence counts")?;

        let mut ranges = Vec::new();
        reserve(&mut ranges, count, "EVP1 envelopes")?;
        let mut influence_count = 0usize;
        for index in 0..count {
            let influences = usize::from(be::u8_at(data, counts_base + index)?);
            if influences == 0 {
                return Err(FormatError::at(
                    counts_base + index,
                    "EVP1 envelope has no influences",
                ));
            }
            influence_count = influence_count
                .checked_add(influences)
                .ok_or_else(|| FormatError::at(counts_base + index, "EVP1 influence overflow"))?;
            if influence_count > MAX_INFLUENCES {
                return Err(FormatError::at(
                    counts_base + index,
                    "EVP1 exceeds total influence limit",
                ));
            }
            ranges.push(EnvelopeRange {
                first: influence_count - influences,
                count: influences,
            });
        }
        block.relative_offset(
            indices_offset,
            mul_size(influence_count, 2, block.start + 0x10, "EVP1 indices")?,
            "EVP1 joint indices",
        )?;
        block.relative_offset(
            weights_offset,
            mul_size(influence_count, 4, block.start + 0x14, "EVP1 weights")?,
            "EVP1 weights",
        )?;
        Ok(Self {
            block: Some(block),
            ranges,
        })
    }

    fn matrix(&self, data: &[u8], envelope: usize, worlds: &[WorldJoint]) -> Result<Matrix> {
        let block = self
            .block
            .ok_or_else(|| FormatError::new("DRW1 needs an EVP1 block"))?;
        let range = *self.ranges.get(envelope).ok_or_else(|| {
            FormatError::new(format!("DRW1 references missing envelope {envelope}"))
        })?;
        let indices = block.relative_offset(block.u32(data, 0x10)?, 0, "EVP1 indices")?;
        let weights = block.relative_offset(block.u32(data, 0x14)?, 0, "EVP1 weights")?;
        let inverse = block.relative_offset(block.u32(data, 0x18)?, 0, "EVP1 inverse matrices")?;
        let mut result = [[0.0; 4]; 3];
        for influence in range.first..range.first + range.count {
            let joint = usize::from(be::u16_at(data, indices + influence * 2)?);
            let world = worlds
                .get(joint)
                .filter(|world| world.visited)
                .ok_or_else(|| {
                    FormatError::new(format!("EVP1 references unavailable joint {joint}"))
                })?
                .world;
            let weight = finite_f32(data, weights + influence * 4)?;
            let inverse_base = add_offset(
                inverse,
                mul_size(joint, 48, inverse, "EVP1 inverse matrix")?,
                "EVP1 inverse matrix",
            )?;
            block.absolute(inverse_base - block.start, 48, "EVP1 inverse matrix")?;
            let mut bind_inverse = [[0.0; 4]; 3];
            for (row, bind_row) in bind_inverse.iter_mut().enumerate() {
                for (column, value) in bind_row.iter_mut().enumerate() {
                    *value = finite_f32(data, inverse_base + row * 16 + column * 4)?;
                }
            }
            let skin = concat(world, bind_inverse)?;
            for (row, result_row) in result.iter_mut().enumerate() {
                for (column, value) in result_row.iter_mut().enumerate() {
                    let prior = if influence == range.first {
                        0.0
                    } else {
                        *value
                    };
                    *value = prior + weight * skin[row][column];
                    if !value.is_finite() {
                        return Err(FormatError::at(
                            weights + influence * 4,
                            "non-finite weighted EVP1 matrix",
                        ));
                    }
                }
            }
        }
        Ok(result)
    }
}

struct DrawData {
    block: Option<Block>,
    count: usize,
}

impl DrawData {
    fn parse(data: &[u8], block: Option<Block>) -> Result<Self> {
        let Some(block) = block else {
            return Ok(Self {
                block: None,
                count: 0,
            });
        };
        block.absolute(0, 0x18, "DRW1 header")?;
        let count = usize::from(block.u16(data, 8)?);
        let flags = block.u32(data, 0x0c)?;
        let indices = block.u32(data, 0x10)?;
        block.relative_offset(flags, count, "DRW1 flags")?;
        block.relative_offset(
            indices,
            mul_size(count, 2, block.start + 0x14, "DRW1 indices")?,
            "DRW1 indices",
        )?;
        let flags_base = block.relative_offset(flags, 0, "DRW1 flags")?;
        for index in 0..count {
            if be::u8_at(data, flags_base + index)? > 1 {
                return Err(FormatError::at(
                    flags_base + index,
                    "unknown DRW1 matrix kind",
                ));
            }
        }
        Ok(Self {
            block: Some(block),
            count,
        })
    }

    fn matrix(
        &self,
        data: &[u8],
        index: usize,
        envelopes: &EnvelopeData,
        worlds: &[WorldJoint],
    ) -> Result<Matrix> {
        let block = self
            .block
            .ok_or_else(|| FormatError::new("shape matrix index has no DRW1 table"))?;
        if index >= self.count {
            return Err(FormatError::new(format!(
                "SHP1 matrix index {index} exceeds {} DRW1 entries",
                self.count
            )));
        }
        let flags = block.relative_offset(block.u32(data, 0x0c)?, 0, "DRW1 flags")?;
        let indices = block.relative_offset(block.u32(data, 0x10)?, 0, "DRW1 indices")?;
        let weighted = be::u8_at(data, flags + index)? == 1;
        let mapped = usize::from(be::u16_at(data, indices + index * 2)?);
        if weighted {
            envelopes.matrix(data, mapped, worlds)
        } else {
            worlds
                .get(mapped)
                .filter(|world| world.visited)
                .map(|world| world.world)
                .ok_or_else(|| {
                    FormatError::new(format!("DRW1 references unavailable joint {mapped}"))
                })
        }
    }
}

#[derive(Clone, Copy)]
struct AttributeFormat {
    attr: u8,
    count: u8,
    component_type: u8,
    fraction: u8,
}

struct VertexData {
    block: Block,
    formats: Vec<AttributeFormat>,
}

impl VertexData {
    fn parse(data: &[u8], block: Block) -> Result<Self> {
        block.absolute(0, VTX1_HEADER_SIZE, "VTX1 header")?;
        let formats_offset = block.u32(data, 8)?;
        if formats_offset < VTX1_HEADER_SIZE as u32 {
            return Err(FormatError::at(
                block.start + 8,
                "VTX1 format list overlaps its header",
            ));
        }
        let mut formats = Vec::new();
        let mut seen = [false; 26];
        for index in 0..32usize {
            let cursor = add_offset(
                formats_offset as usize,
                index * VTX_FORMAT_SIZE,
                "VTX1 format list",
            )?;
            let attr = block.u32(data, cursor)?;
            if attr == u32::from(GX_VA_NULL) {
                let result = Self { block, formats };
                result.validate_array_offsets(data)?;
                return Ok(result);
            }
            if attr > 25 || !valid_vertex_attribute(attr as u8) {
                return Err(FormatError::at(
                    block.start + cursor,
                    format!("unsupported VTX1 attribute {attr}"),
                ));
            }
            let attr = attr as u8;
            if seen[usize::from(attr)] {
                return Err(FormatError::at(
                    block.start + cursor,
                    "duplicate VTX1 attribute format",
                ));
            }
            seen[usize::from(attr)] = true;
            let count = block.u32(data, cursor + 4)?;
            let component_type = block.u32(data, cursor + 8)?;
            let fraction = block.u8(data, cursor + 12)?;
            if count > 2 || component_type > 5 || fraction > 31 {
                return Err(FormatError::at(
                    block.start + cursor,
                    "invalid VTX1 component count/type/fraction",
                ));
            }
            let format = AttributeFormat {
                attr,
                count: count as u8,
                component_type: component_type as u8,
                fraction,
            };
            validate_attribute_format(&format, block.start + cursor)?;
            reserve(&mut formats, 1, "VTX1 formats")?;
            formats.push(format);
        }
        Err(FormatError::at(
            block.start + formats_offset as usize,
            "unterminated VTX1 format list",
        ))
    }

    fn validate_array_offsets(&self, data: &[u8]) -> Result<()> {
        for attr in 9..=25u8 {
            if let Some(field) = array_offset_field(attr) {
                let offset = self.block.u32(data, field)?;
                if offset != 0 && offset < VTX1_HEADER_SIZE as u32 {
                    return Err(FormatError::at(
                        self.block.start + field,
                        format!("VTX1 attribute {attr} array overlaps its header"),
                    ));
                }
                if offset != 0 {
                    self.block.relative_offset(offset, 0, "VTX1 array")?;
                }
            }
        }
        Ok(())
    }

    fn format(&self, attr: u8) -> Option<AttributeFormat> {
        self.formats
            .iter()
            .copied()
            .find(|format| format.attr == attr)
            .or_else(|| {
                if attr == GX_VA_NBT {
                    self.formats
                        .iter()
                        .copied()
                        .find(|format| format.attr == GX_VA_NRM)
                } else {
                    None
                }
            })
    }

    fn array_offset(&self, data: &[u8], attr: u8) -> Result<Option<usize>> {
        let field = array_offset_field(attr)
            .ok_or_else(|| FormatError::new(format!("attribute {attr} has no VTX1 array")))?;
        let offset = self.block.u32(data, field)?;
        if offset == 0 {
            Ok(None)
        } else {
            Ok(Some(offset as usize))
        }
    }

    fn decode_attribute(
        &self,
        data: &[u8],
        cursor: &mut Cursor,
        descriptor: Descriptor,
        vertex: &mut BmdVertex,
        pn_matrix: &mut Option<u16>,
    ) -> Result<()> {
        let attr = descriptor.attr;
        if attr == GX_VA_PNMTXIDX {
            *pn_matrix = Some(cursor.index(data, descriptor.form)?);
            return Ok(());
        }
        if (1..=8).contains(&attr) {
            let _ = cursor.index(data, descriptor.form)?;
            return Ok(());
        }
        if attr == GX_VA_POS {
            let (values, count) = self.numeric_attribute(data, cursor, descriptor)?;
            vertex.position = [
                values[0],
                values[1],
                if count > 2 { values[2] } else { 0.0 },
            ];
        } else if attr == GX_VA_NRM || attr == GX_VA_NBT {
            let (values, _) = self.numeric_attribute(data, cursor, descriptor)?;
            vertex.normal = [values[0], values[1], values[2]];
        } else if attr == GX_VA_CLR0 || attr == GX_VA_CLR1 {
            let color = self.color_attribute(data, cursor, descriptor)?;
            if attr == GX_VA_CLR0 {
                vertex.color = color;
            } else {
                vertex.color1 = color;
            }
        } else if (GX_VA_TEX0..=GX_VA_TEX7).contains(&attr) {
            let (values, count) = self.numeric_attribute(data, cursor, descriptor)?;
            vertex.tex_coords[usize::from(attr - GX_VA_TEX0)] =
                [values[0], if count > 1 { values[1] } else { 0.0 }];
        } else {
            return Err(FormatError::new(format!(
                "unsupported SHP1 vertex attribute {attr}"
            )));
        }
        Ok(())
    }

    fn numeric_attribute(
        &self,
        data: &[u8],
        cursor: &mut Cursor,
        descriptor: Descriptor,
    ) -> Result<([f32; 9], usize)> {
        let format = self.format(descriptor.attr).ok_or_else(|| {
            FormatError::new(format!(
                "missing VTX1 format for attribute {}",
                descriptor.attr
            ))
        })?;
        if format.component_type > 4 {
            return Err(FormatError::new(format!(
                "attribute {} does not have a numeric VTX1 format",
                descriptor.attr
            )));
        }
        let count = component_count(descriptor.attr, format.count)?;
        let component_bytes = numeric_size(format.component_type)?;
        let stride = mul_size(count, component_bytes, cursor.position, "VTX1 array stride")?;
        let mut values = [0.0; 9];
        if descriptor.form == 1 {
            for (index, value) in values.iter_mut().take(count).enumerate() {
                *value = cursor.numeric(
                    data,
                    format.component_type,
                    format.fraction,
                    index * component_bytes,
                )?;
            }
            cursor.skip(count * component_bytes)?;
        } else {
            let array_index = usize::from(cursor.index(data, descriptor.form)?);
            let array = self.array_offset(data, descriptor.attr)?.ok_or_else(|| {
                FormatError::new(format!(
                    "missing VTX1 array for attribute {}",
                    descriptor.attr
                ))
            })?;
            let start = add_offset(
                array,
                mul_size(array_index, stride, cursor.position, "VTX1 array index")?,
                "VTX1 array index",
            )?;
            self.block
                .absolute(start, stride, "VTX1 indexed array element")?;
            for (index, value) in values.iter_mut().take(count).enumerate() {
                *value = numeric_at(
                    data,
                    self.block.start + start + index * component_bytes,
                    format.component_type,
                    format.fraction,
                )?;
            }
        }
        Ok((values, count))
    }

    fn color_attribute(
        &self,
        data: &[u8],
        cursor: &mut Cursor,
        descriptor: Descriptor,
    ) -> Result<[f32; 4]> {
        let format = self.format(descriptor.attr).ok_or_else(|| {
            FormatError::new(format!(
                "missing VTX1 color format for attribute {}",
                descriptor.attr
            ))
        })?;
        if format.component_type > 5 {
            return Err(FormatError::new("invalid VTX1 color component type"));
        }
        let size = color_size(format.component_type);
        if descriptor.form == 1 {
            let bytes = cursor.take(data, size)?;
            decode_color(bytes, format.component_type, cursor.position - size)
        } else {
            let array_index = usize::from(cursor.index(data, descriptor.form)?);
            let array = self.array_offset(data, descriptor.attr)?.ok_or_else(|| {
                FormatError::new(format!(
                    "missing VTX1 array for color attribute {}",
                    descriptor.attr
                ))
            })?;
            let start = add_offset(
                array,
                mul_size(array_index, size, cursor.position, "VTX1 color index")?,
                "VTX1 color index",
            )?;
            let bytes = self.block.bytes(data, start, size, "VTX1 color")?;
            decode_color(bytes, format.component_type, self.block.start + start)
        }
    }
}

fn valid_vertex_attribute(attr: u8) -> bool {
    (GX_VA_POS..=GX_VA_TEX7).contains(&attr) || attr == GX_VA_NBT
}

fn array_offset_field(attr: u8) -> Option<usize> {
    match attr {
        GX_VA_POS => Some(0x0c),
        GX_VA_NRM => Some(0x10),
        GX_VA_NBT => Some(0x14),
        GX_VA_CLR0 | GX_VA_CLR1 => Some(0x18 + usize::from(attr - GX_VA_CLR0) * 4),
        GX_VA_TEX0..=GX_VA_TEX7 => Some(0x20 + usize::from(attr - GX_VA_TEX0) * 4),
        _ => None,
    }
}

fn validate_attribute_format(format: &AttributeFormat, offset: usize) -> Result<()> {
    match format.attr {
        GX_VA_POS | GX_VA_NRM | GX_VA_NBT | GX_VA_TEX0..=GX_VA_TEX7 => {
            if format.component_type > 4 {
                return Err(FormatError::at(
                    offset,
                    "numeric VTX1 attribute uses a color type",
                ));
            }
            match format.attr {
                GX_VA_POS => {
                    if format.count > 1 {
                        return Err(FormatError::at(offset, "invalid position component count"));
                    }
                }
                GX_VA_NRM | GX_VA_NBT => {
                    if format.attr == GX_VA_NRM && format.count > 2 {
                        return Err(FormatError::at(offset, "invalid normal component count"));
                    }
                }
                _ => {
                    if format.count > 1 {
                        return Err(FormatError::at(offset, "invalid texture-coordinate count"));
                    }
                }
            }
        }
        GX_VA_CLR0 | GX_VA_CLR1 => {
            if format.count > 1 {
                return Err(FormatError::at(offset, "invalid color component count"));
            }
        }
        _ => {
            return Err(FormatError::at(
                offset,
                "attribute has no VTX1 array format",
            ));
        }
    }
    Ok(())
}

fn component_count(attr: u8, count: u8) -> Result<usize> {
    match attr {
        GX_VA_POS => Ok(if count == 0 { 2 } else { 3 }),
        GX_VA_NRM => Ok(3),
        GX_VA_NBT => {
            if count == 0 {
                Err(FormatError::new(
                    "GX_VA_NBT requires an NBT component count",
                ))
            } else {
                Ok(9)
            }
        }
        GX_VA_TEX0..=GX_VA_TEX7 => Ok(if count == 0 { 1 } else { 2 }),
        _ => Err(FormatError::new(format!(
            "attribute {attr} has no numeric component count"
        ))),
    }
}

fn numeric_size(component_type: u8) -> Result<usize> {
    match component_type {
        0 | 1 => Ok(1),
        2 | 3 => Ok(2),
        4 => Ok(4),
        _ => Err(FormatError::new(format!(
            "unsupported numeric component type {component_type}"
        ))),
    }
}

fn numeric_at(data: &[u8], offset: usize, component_type: u8, fraction: u8) -> Result<f32> {
    let (value, integer) = match component_type {
        0 => (f32::from(be::u8_at(data, offset)?), true),
        1 => (f32::from(be::i8_at(data, offset)?), true),
        2 => (f32::from(be::u16_at(data, offset)?), true),
        3 => (f32::from(be::i16_at(data, offset)?), true),
        4 => (be::f32_at(data, offset)?, false),
        _ => {
            return Err(FormatError::at(
                offset,
                "unsupported numeric component type",
            ));
        }
    };
    let scale = if integer {
        2.0f32.powi(-i32::from(fraction))
    } else {
        1.0
    };
    let value = value * scale;
    if value.is_finite() {
        Ok(value)
    } else {
        Err(FormatError::at(offset, "non-finite VTX1 numeric value"))
    }
}

fn color_size(component_type: u8) -> usize {
    match component_type {
        0 | 3 => 2,
        1 => 3,
        2 | 5 => 4,
        4 => 3,
        _ => 0,
    }
}

fn decode_color(data: &[u8], component_type: u8, offset: usize) -> Result<[f32; 4]> {
    let unit = 1.0 / 255.0;
    let color = match component_type {
        0 => {
            let value = be::u16_at(data, 0)?;
            [
                f32::from(((value >> 11) & 0x1f) as u8) / 31.0,
                f32::from(((value >> 5) & 0x3f) as u8) / 63.0,
                f32::from((value & 0x1f) as u8) / 31.0,
                1.0,
            ]
        }
        1 => [
            f32::from(be::u8_at(data, 0)?) * unit,
            f32::from(be::u8_at(data, 1)?) * unit,
            f32::from(be::u8_at(data, 2)?) * unit,
            1.0,
        ],
        2 => [
            f32::from(be::u8_at(data, 0)?) * unit,
            f32::from(be::u8_at(data, 1)?) * unit,
            f32::from(be::u8_at(data, 2)?) * unit,
            1.0,
        ],
        3 => {
            let value = be::u16_at(data, 0)?;
            [
                f32::from(((value >> 12) & 0x0f) as u8) / 15.0,
                f32::from(((value >> 8) & 0x0f) as u8) / 15.0,
                f32::from(((value >> 4) & 0x0f) as u8) / 15.0,
                f32::from((value & 0x0f) as u8) / 15.0,
            ]
        }
        4 => {
            let r = be::u8_at(data, 0)?;
            let g = be::u8_at(data, 1)?;
            let b = be::u8_at(data, 2)?;
            [
                f32::from(r >> 2) / 63.0,
                f32::from(((r & 3) << 4) | (g >> 4)) / 63.0,
                f32::from(((g & 0x0f) << 2) | (b >> 6)) / 63.0,
                f32::from(b & 0x3f) / 63.0,
            ]
        }
        5 => [
            f32::from(be::u8_at(data, 0)?) * unit,
            f32::from(be::u8_at(data, 1)?) * unit,
            f32::from(be::u8_at(data, 2)?) * unit,
            f32::from(be::u8_at(data, 3)?) * unit,
        ],
        _ => return Err(FormatError::at(offset, "unsupported VTX1 color type")),
    };
    Ok(color)
}

struct Cursor {
    position: usize,
    end: usize,
}

impl Cursor {
    fn new(position: usize, end: usize) -> Self {
        Self { position, end }
    }

    fn take<'a>(&mut self, data: &'a [u8], length: usize) -> Result<&'a [u8]> {
        let next = self
            .position
            .checked_add(length)
            .ok_or_else(|| FormatError::at(self.position, "display-list cursor overflow"))?;
        if next > self.end || next > data.len() {
            return Err(FormatError::at(
                self.position,
                "display-list command exceeds its packet",
            ));
        }
        let bytes = be::bytes(data, self.position, length)?;
        self.position = next;
        Ok(bytes)
    }

    fn skip(&mut self, length: usize) -> Result<()> {
        let next = self
            .position
            .checked_add(length)
            .ok_or_else(|| FormatError::at(self.position, "display-list cursor overflow"))?;
        if next > self.end {
            return Err(FormatError::at(
                self.position,
                "display-list vertex exceeds its packet",
            ));
        }
        self.position = next;
        Ok(())
    }

    fn u8(&mut self, data: &[u8]) -> Result<u8> {
        let position = self.position;
        self.take(data, 1).and_then(|bytes| {
            bytes
                .first()
                .copied()
                .ok_or_else(|| FormatError::at(position, "missing display-list byte"))
        })
    }

    fn u16(&mut self, data: &[u8]) -> Result<u16> {
        let position = self.position;
        let bytes = self.take(data, 2)?;
        be::u16_at(bytes, 0).map_err(|_| FormatError::at(position, "missing display-list u16"))
    }

    fn index(&mut self, data: &[u8], form: u8) -> Result<u16> {
        match form {
            1 | 2 => Ok(u16::from(self.u8(data)?)),
            3 => self.u16(data),
            _ => Err(FormatError::at(
                self.position,
                "invalid SHP1 attribute form",
            )),
        }
    }

    fn numeric(
        &mut self,
        data: &[u8],
        component_type: u8,
        fraction: u8,
        relative: usize,
    ) -> Result<f32> {
        let size = numeric_size(component_type)?;
        let offset = add_offset(self.position, relative, "direct vertex attribute")?;
        let end = offset
            .checked_add(size)
            .ok_or_else(|| FormatError::at(offset, "direct vertex attribute overflow"))?;
        if end > self.end {
            return Err(FormatError::at(
                offset,
                "direct vertex attribute exceeds its packet",
            ));
        }
        numeric_at(data, offset, component_type, fraction)
    }
}

#[derive(Clone, Copy)]
struct Descriptor {
    attr: u8,
    form: u8,
}

struct ShapeData {
    block: Block,
    count: usize,
}

impl ShapeData {
    fn parse(data: &[u8], block: Block) -> Result<Self> {
        block.absolute(0, SHP1_HEADER_SIZE, "SHP1 header")?;
        let count = usize::from(block.u16(data, 8)?);
        let remap = block.u32(data, 0x10)?;
        block.relative_offset(
            remap,
            mul_size(count, 2, block.start + 0x10, "SHP1 remap")?,
            "SHP1 remap",
        )?;
        Ok(Self { block, count })
    }

    fn descriptors(&self, data: &[u8], byte_offset: u16) -> Result<Vec<Descriptor>> {
        let lists = self.block.u32(data, 0x18)?;
        let start = add_offset(
            lists as usize,
            usize::from(byte_offset),
            "SHP1 descriptor list",
        )?;
        let mut descriptors = Vec::new();
        let mut seen = [false; 26];
        for index in 0..32usize {
            let cursor = add_offset(start, index * 8, "SHP1 descriptor list")?;
            let attr = self.block.u32(data, cursor)?;
            if attr == u32::from(GX_VA_NULL) {
                return Ok(descriptors);
            }
            if attr > 25 {
                return Err(FormatError::at(
                    self.block.start + cursor,
                    format!("unsupported SHP1 attribute {attr}"),
                ));
            }
            let attr = attr as u8;
            let form = self.block.u32(data, cursor + 4)?;
            if form > 3 {
                return Err(FormatError::at(
                    self.block.start + cursor + 4,
                    "unsupported SHP1 attribute form",
                ));
            }
            if attr > 8 && !valid_vertex_attribute(attr) {
                return Err(FormatError::at(
                    self.block.start + cursor,
                    "SHP1 attribute is not a per-vertex array attribute",
                ));
            }
            if seen[usize::from(attr)] {
                return Err(FormatError::at(
                    self.block.start + cursor,
                    "duplicate SHP1 vertex attribute",
                ));
            }
            seen[usize::from(attr)] = true;
            reserve(&mut descriptors, 1, "SHP1 descriptors")?;
            descriptors.push(Descriptor {
                attr,
                form: form as u8,
            });
        }
        Err(FormatError::at(
            self.block.start + start,
            "unterminated SHP1 descriptor list",
        ))
    }

    #[allow(clippy::too_many_arguments)]
    fn emit_shape(
        &self,
        data: &[u8],
        link: ShapeLink,
        info: &Info,
        vertices: &VertexData,
        hierarchy: &Hierarchy,
        envelopes: &EnvelopeData,
        draws: &DrawData,
        material: BmdMaterial,
        totals: &mut OutputTotals,
        warnings: &mut Vec<String>,
    ) -> Result<Option<BmdMesh>> {
        let shape = usize::from(link.shape);
        if shape >= self.count {
            return Err(FormatError::new(format!(
                "INF1 references missing SHP1 shape {}",
                link.shape
            )));
        }
        let remap_offset = self.block.u32(data, 0x10)?;
        let remap = self.block.relative_offset(remap_offset, 0, "SHP1 remap")?;
        let init_index = usize::from(be::u16_at(data, remap + shape * 2)?);
        let init_relative = add_offset(
            self.block.u32(data, 0x0c)? as usize,
            mul_size(init_index, SHAPE_INIT_SIZE, remap, "SHP1 init index")?,
            "SHP1 init data",
        )?;
        self.block
            .absolute(init_relative, SHAPE_INIT_SIZE, "SHP1 init data")?;
        let matrix_type = self.block.u8(data, init_relative)?;
        let group_count = usize::from(self.block.u16(data, init_relative + 2)?);
        let descriptor_offset = self.block.u16(data, init_relative + 4)?;
        let matrix_init_index = usize::from(self.block.u16(data, init_relative + 6)?);
        let draw_init_index = usize::from(self.block.u16(data, init_relative + 8)?);
        for offset in [0x0c, 0x10, 0x14, 0x18, 0x1c, 0x20, 0x24] {
            let _ = self.block.f32(data, init_relative + offset)?;
        }
        if matrix_type > 3 {
            return Err(FormatError::at(
                self.block.start + init_relative,
                "unknown SHP1 matrix type",
            ));
        }
        let mut descriptors = self.descriptors(data, descriptor_offset)?;
        descriptors.retain(|descriptor| descriptor.form != 0);
        if !descriptors
            .iter()
            .any(|descriptor| descriptor.attr == GX_VA_POS)
        {
            return Err(FormatError::at(
                self.block.start + init_relative + 4,
                "SHP1 shape has no position attribute",
            ));
        }
        let has_pn_matrix = descriptors
            .iter()
            .any(|descriptor| descriptor.attr == GX_VA_PNMTXIDX);
        if matches!(matrix_type, 1 | 2) && info.flags & 0x100 == 0 {
            warn(
                warnings,
                format!(
                    "SHP1 shape {} is a billboard; using its bind-pose transform (no camera billboard)",
                    link.shape
                ),
            );
        }

        let mut mesh = BmdMesh {
            vertices: Vec::new(),
            indices: Vec::new(),
            material,
        };
        let display_lists = self.block.u32(data, 0x20)?;
        let matrix_inits = self.block.u32(data, 0x24)?;
        let draw_inits = self.block.u32(data, 0x28)?;
        let matrix_table = self.block.u32(data, 0x1c)?;
        // GX matrix memory persists between packets; 0xFFFF table entries keep
        // whatever the previous packet loaded into that slot.
        let mut multi_matrices = [u16::MAX; 10];

        for group in 0..group_count {
            let matrix_init = add_offset(
                matrix_inits as usize,
                mul_size(
                    matrix_init_index + group,
                    8,
                    self.block.start + init_relative,
                    "SHP1 matrix init",
                )?,
                "SHP1 matrix init",
            )?;
            self.block.absolute(matrix_init, 8, "SHP1 matrix init")?;
            let use_matrix = usize::from(self.block.u16(data, matrix_init)?);
            let use_count = usize::from(self.block.u16(data, matrix_init + 2)?);
            let first_matrix = self.block.u32(data, matrix_init + 4)? as usize;

            let draw_init = add_offset(
                draw_inits as usize,
                mul_size(
                    draw_init_index + group,
                    8,
                    self.block.start + init_relative,
                    "SHP1 draw init",
                )?,
                "SHP1 draw init",
            )?;
            self.block.absolute(draw_init, 8, "SHP1 draw init")?;
            let display_size = self.block.u32(data, draw_init)? as usize;
            let display_offset = self.block.u32(data, draw_init + 4)?;
            let display_relative = add_offset(
                display_lists as usize,
                display_offset as usize,
                "SHP1 display list",
            )?;
            let display =
                self.block
                    .absolute(display_relative, display_size, "SHP1 display list")?;
            let mut cursor = Cursor::new(display, display + display_size);

            if matrix_type == 3 {
                if use_count == 0 || use_count > multi_matrices.len() {
                    return Err(FormatError::at(
                        self.block.start + matrix_init,
                        "SHP1 multi-matrix packet has an invalid matrix count",
                    ));
                }
                let table = add_offset(
                    matrix_table as usize,
                    mul_size(
                        first_matrix,
                        2,
                        self.block.start + matrix_init,
                        "SHP1 matrix table",
                    )?,
                    "SHP1 matrix table",
                )?;
                self.block
                    .absolute(table, use_count * 2, "SHP1 matrix table")?;
                for (slot, entry) in multi_matrices.iter_mut().take(use_count).enumerate() {
                    let loaded = self.block.u16(data, table + slot * 2)?;
                    if loaded != u16::MAX {
                        *entry = loaded;
                    }
                }
            }

            while cursor.position < cursor.end {
                let opcode_offset = cursor.position;
                let opcode = cursor.u8(data)?;
                if opcode == 0 {
                    continue;
                }
                let primitive = opcode & 0xf8;
                if !matches!(primitive, 0x80 | 0x90 | 0x98 | 0xa0) {
                    return Err(FormatError::at(
                        opcode_offset,
                        format!("unsupported SHP1 display-list opcode 0x{opcode:02x}"),
                    ));
                }
                let count = usize::from(cursor.u16(data)?);
                match primitive {
                    0x80 if count % 4 != 0 => {
                        return Err(FormatError::at(
                            opcode_offset,
                            "quad vertex count is not divisible by four",
                        ));
                    }
                    0x90 if count % 3 != 0 => {
                        return Err(FormatError::at(
                            opcode_offset,
                            "triangle vertex count is not divisible by three",
                        ));
                    }
                    0x98 | 0xa0 if count != 0 && count < 3 => {
                        return Err(FormatError::at(
                            opcode_offset,
                            "strip/fan has fewer than three vertices",
                        ));
                    }
                    _ => {}
                }
                let triangle_count = match primitive {
                    0x80 => count / 2,
                    0x90 => count / 3,
                    0x98 | 0xa0 => count.saturating_sub(2),
                    _ => 0,
                };
                totals.reserve_vertices(&mut mesh.vertices, count)?;
                totals.reserve_indices(
                    &mut mesh.indices,
                    triangle_count.checked_mul(3).ok_or_else(|| {
                        FormatError::at(opcode_offset, "triangle index count overflow")
                    })?,
                )?;
                let first_vertex = mesh.vertices.len();

                for _ in 0..count {
                    let mut vertex = BmdVertex::default();
                    let mut pn_matrix = None;
                    for descriptor in &descriptors {
                        vertices.decode_attribute(
                            data,
                            &mut cursor,
                            *descriptor,
                            &mut vertex,
                            &mut pn_matrix,
                        )?;
                    }
                    let matrix = self.vertex_matrix(
                        data,
                        info,
                        link,
                        hierarchy,
                        envelopes,
                        draws,
                        matrix_type,
                        use_matrix,
                        &multi_matrices[..use_count.min(multi_matrices.len())],
                        has_pn_matrix,
                        pn_matrix,
                    )?;
                    vertex.position = transform_point(matrix, vertex.position)?;
                    vertex.normal = transform_normal(matrix, vertex.normal)?;
                    mesh.vertices.push(vertex);
                }

                let index = |stream: usize| -> Result<u32> {
                    let absolute = first_vertex.checked_add(stream).ok_or_else(|| {
                        FormatError::at(opcode_offset, "mesh vertex index overflow")
                    })?;
                    if absolute >= mesh.vertices.len() {
                        return Err(FormatError::at(
                            opcode_offset,
                            "internal mesh vertex index error",
                        ));
                    }
                    u32::try_from(absolute)
                        .map_err(|_| FormatError::at(opcode_offset, "mesh exceeds u32 indices"))
                };
                match primitive {
                    0x80 => {
                        for quad in 0..count / 4 {
                            let start = quad * 4;
                            mesh.indices.extend_from_slice(&[
                                index(start)?,
                                index(start + 1)?,
                                index(start + 2)?,
                            ]);
                            mesh.indices.extend_from_slice(&[
                                index(start)?,
                                index(start + 2)?,
                                index(start + 3)?,
                            ]);
                        }
                    }
                    0x90 => {
                        for stream in 0..count {
                            mesh.indices.push(index(stream)?);
                        }
                    }
                    0x98 => {
                        for stream in 2..count {
                            if stream % 2 == 0 {
                                mesh.indices.extend_from_slice(&[
                                    index(stream - 2)?,
                                    index(stream - 1)?,
                                    index(stream)?,
                                ]);
                            } else {
                                mesh.indices.extend_from_slice(&[
                                    index(stream)?,
                                    index(stream - 1)?,
                                    index(stream - 2)?,
                                ]);
                            }
                        }
                    }
                    0xa0 => {
                        for stream in 2..count {
                            mesh.indices.extend_from_slice(&[
                                index(0)?,
                                index(stream - 1)?,
                                index(stream)?,
                            ]);
                        }
                    }
                    _ => {
                        return Err(FormatError::at(
                            opcode_offset,
                            "unknown primitive after validation",
                        ));
                    }
                }
            }
        }

        if mesh.indices.is_empty() {
            Ok(None)
        } else {
            Ok(Some(mesh))
        }
    }

    #[allow(clippy::too_many_arguments)]
    fn vertex_matrix(
        &self,
        data: &[u8],
        info: &Info,
        link: ShapeLink,
        hierarchy: &Hierarchy,
        envelopes: &EnvelopeData,
        draws: &DrawData,
        matrix_type: u8,
        single_matrix: usize,
        multi_matrices: &[u16],
        has_pn_matrix: bool,
        pn_matrix: Option<u16>,
    ) -> Result<Matrix> {
        if info.flags & 0x100 != 0 {
            return Ok(IDENTITY);
        }
        let draw_index = if matrix_type == 3 {
            let slot = if has_pn_matrix {
                let index = pn_matrix.ok_or_else(|| {
                    FormatError::new("multi-matrix vertex has no position-matrix index")
                })?;
                if index % 3 != 0 {
                    return Err(FormatError::new(format!(
                        "position-matrix index {index} is not a multiple of three"
                    )));
                }
                usize::from(index / 3)
            } else {
                0
            };
            let entry = *multi_matrices.get(slot).ok_or_else(|| {
                FormatError::new(format!("position-matrix slot {slot} exceeds its packet"))
            })?;
            if entry == u16::MAX {
                return Err(FormatError::new(format!(
                    "position-matrix slot {slot} has no matrix load"
                )));
            }
            usize::from(entry)
        } else {
            single_matrix
        };

        if draws.block.is_some() {
            draws.matrix(data, draw_index, envelopes, &hierarchy.worlds)
        } else if let Some(joint) = link.joint {
            hierarchy
                .worlds
                .get(usize::from(joint))
                .filter(|world| world.visited)
                .map(|world| world.world)
                .ok_or_else(|| FormatError::new(format!("shape joint {joint} has no world matrix")))
        } else if draw_index == 0 {
            Ok(IDENTITY)
        } else {
            Err(FormatError::new(format!(
                "shape matrix {draw_index} has neither DRW1 nor an attached joint"
            )))
        }
    }
}

struct OutputTotals {
    vertices: usize,
    indices: usize,
}

impl OutputTotals {
    fn reserve_vertices(&mut self, output: &mut Vec<BmdVertex>, additional: usize) -> Result<()> {
        self.vertices = self
            .vertices
            .checked_add(additional)
            .ok_or_else(|| FormatError::new("BMD vertex count overflow"))?;
        if self.vertices > MAX_VERTICES {
            return Err(FormatError::new(format!(
                "BMD exceeds decoded vertex limit of {MAX_VERTICES}"
            )));
        }
        reserve(output, additional, "decoded BMD vertices")
    }

    fn reserve_indices(&mut self, output: &mut Vec<u32>, additional: usize) -> Result<()> {
        self.indices = self
            .indices
            .checked_add(additional)
            .ok_or_else(|| FormatError::new("BMD index count overflow"))?;
        if self.indices > MAX_INDICES {
            return Err(FormatError::new(format!(
                "BMD exceeds decoded index limit of {MAX_INDICES}"
            )));
        }
        reserve(output, additional, "decoded BMD indices")
    }
}

fn transform_point(matrix: Matrix, point: [f32; 3]) -> Result<[f32; 3]> {
    let mut result = [0.0; 3];
    for (row, output) in result.iter_mut().enumerate() {
        *output = matrix[row][0] * point[0]
            + matrix[row][1] * point[1]
            + matrix[row][2] * point[2]
            + matrix[row][3];
        if !output.is_finite() {
            return Err(FormatError::new("non-finite transformed BMD position"));
        }
    }
    Ok(result)
}

fn transform_normal(matrix: Matrix, normal: [f32; 3]) -> Result<[f32; 3]> {
    if normal == [0.0; 3] {
        return Ok(normal);
    }
    let m00 = matrix[0][0];
    let m01 = matrix[0][1];
    let m02 = matrix[0][2];
    let m10 = matrix[1][0];
    let m11 = matrix[1][1];
    let m12 = matrix[1][2];
    let m20 = matrix[2][0];
    let m21 = matrix[2][1];
    let m22 = matrix[2][2];
    let cofactor = [
        m11 * m22 - m12 * m21,
        m12 * m20 - m10 * m22,
        m10 * m21 - m11 * m20,
        m02 * m21 - m01 * m22,
        m00 * m22 - m02 * m20,
        m01 * m20 - m00 * m21,
        m01 * m12 - m02 * m11,
        m02 * m10 - m00 * m12,
        m00 * m11 - m01 * m10,
    ];
    let determinant = m00 * cofactor[0] + m01 * cofactor[3] + m02 * cofactor[6];
    let mut transformed = if determinant.is_finite() && determinant.abs() > 1e-20 {
        [
            cofactor[0] * normal[0] + cofactor[1] * normal[1] + cofactor[2] * normal[2],
            cofactor[3] * normal[0] + cofactor[4] * normal[1] + cofactor[5] * normal[2],
            cofactor[6] * normal[0] + cofactor[7] * normal[1] + cofactor[8] * normal[2],
        ]
        .map(|component| component / determinant)
    } else {
        [
            m00 * normal[0] + m01 * normal[1] + m02 * normal[2],
            m10 * normal[0] + m11 * normal[1] + m12 * normal[2],
            m20 * normal[0] + m21 * normal[1] + m22 * normal[2],
        ]
    };
    let length_squared = transformed.iter().map(|value| value * value).sum::<f32>();
    if length_squared.is_finite() && length_squared > 0.0 {
        let inverse_length = length_squared.sqrt().recip();
        for value in &mut transformed {
            *value *= inverse_length;
        }
        Ok(transformed)
    } else {
        Ok([0.0; 3])
    }
}

struct MaterialData<'a> {
    block: Option<Block>,
    materials: &'a [BmdMaterial],
}

impl<'a> MaterialData<'a> {
    fn new(block: Option<Block>, materials: &'a [BmdMaterial]) -> Self {
        Self { block, materials }
    }

    fn parse_all(
        data: &[u8],
        block: Block,
        texture_map: &[Option<usize>],
        warnings: &mut Vec<String>,
    ) -> Result<Vec<BmdMaterial>> {
        block.absolute(0, 0x84, "MAT3 header")?;
        let count = usize::from(block.u16(data, 8)?);
        let remap = block.u32(data, 0x10)?;
        block.relative_offset(
            remap,
            mul_size(count, 2, block.start + 0x10, "MAT3 remap")?,
            "MAT3 remap",
        )?;
        let remap_base = block.relative_offset(remap, 0, "MAT3 remap")?;
        let mut materials = Vec::new();
        reserve(&mut materials, count, "MAT3 materials")?;
        for index in 0..count {
            let unique = usize::from(be::u16_at(data, remap_base + index * 2)?);
            materials.push(Self::parse_one(
                data,
                block,
                index,
                unique,
                texture_map,
                warnings,
            )?);
        }
        Ok(materials)
    }

    fn get(&self, material: Option<u16>) -> Result<BmdMaterial> {
        let Some(index) = material else {
            return Ok(BmdMaterial::default());
        };
        if self.block.is_none() {
            return Ok(BmdMaterial::default());
        }
        self.materials
            .get(usize::from(index))
            .copied()
            .ok_or_else(|| {
                FormatError::new(format!("INF1 references missing MAT3 material {index}"))
            })
    }

    fn parse_one(
        data: &[u8],
        block: Block,
        material_index: usize,
        unique: usize,
        texture_map: &[Option<usize>],
        warnings: &mut Vec<String>,
    ) -> Result<BmdMaterial> {
        let init = add_offset(
            block.u32(data, 0x0c)? as usize,
            mul_size(
                unique,
                MATERIAL_INIT_SIZE,
                block.start + 0x0c,
                "MAT3 init index",
            )?,
            "MAT3 init data",
        )?;
        block.absolute(init, MATERIAL_INIT_SIZE, "MAT3 init data")?;
        let table = Mat3Tables { data, block, init };
        let mut material = BmdMaterial::default();
        let mut gx = GxMaterial::default();
        let material_mode = block.u8(data, init)?;
        gx.mode = material_mode;
        material.translucent = material_mode == 4;
        if !matches!(material_mode, 1 | 2 | 4) {
            warn(
                warnings,
                format!(
                    "MAT3 material {material_index} has unrecognized material mode {material_mode}; using cutout-style depth writes"
                ),
            );
        }

        if let Some(entry) = table.entry_u8(0x01, 0x1c, 4, "MAT3 cull mode")? {
            material.cull_mode = match block.u32(data, entry)? {
                0 => BmdCullMode::None,
                1 => BmdCullMode::Front,
                2 => BmdCullMode::Back,
                3 => BmdCullMode::All,
                cull => {
                    return Err(FormatError::at(
                        block.start + init + 1,
                        format!("unknown MAT3 cull mode {cull}"),
                    ));
                }
            };
        }

        if let Some(entry) = table.entry_u8(0x02, 0x24, 1, "MAT3 color channel count")? {
            gx.channel_count = block.u8(data, entry)?.min(2);
        }
        for (slot, channel) in gx.channels.iter_mut().enumerate() {
            if let Some(entry) =
                table.entry_u16(0x0c + slot * 2, 0x28, 8, "MAT3 color channel")?
            {
                let bytes = block.bytes(data, entry, 8, "MAT3 color channel")?;
                *channel = GxColorChannel {
                    lighting: bytes[0] != 0,
                    material_from_vertex: bytes[1] == 1,
                    light_mask: bytes[2],
                    diffuse_function: bytes[3],
                    attenuation_function: bytes[4],
                    ambient_from_vertex: bytes[5] == 1,
                };
            }
        }
        material.use_vertex_color = gx.channels[0].material_from_vertex;

        for slot in 0..2 {
            if let Some(entry) = table.entry_u16(0x08 + slot * 2, 0x20, 4, "MAT3 material color")? {
                gx.material_colors[slot] = rgba8(block.bytes(data, entry, 4, "MAT3 material color")?);
            }
            if let Some(entry) = table.entry_u16(0x14 + slot * 2, 0x2c, 4, "MAT3 ambient color")? {
                gx.ambient_colors[slot] = rgba8(block.bytes(data, entry, 4, "MAT3 ambient color")?);
            }
        }
        material.color = gx.material_colors[0];

        if let Some(entry) = table.entry_u8(0x03, 0x34, 1, "MAT3 texgen count")? {
            gx.tex_gen_count = block.u8(data, entry)?.min(MAX_TEX_GENS as u8);
        }
        for (slot, texgen) in gx.tex_gens.iter_mut().enumerate() {
            if let Some(entry) = table.entry_u16(0x28 + slot * 2, 0x38, 4, "MAT3 texgen")? {
                let bytes = block.bytes(data, entry, 4, "MAT3 texgen")?;
                if bytes[0] > 10 || bytes[1] > 20 {
                    return Err(FormatError::at(
                        block.start + entry,
                        "invalid MAT3 texgen type/source",
                    ));
                }
                *texgen = GxTexGen {
                    kind: bytes[0],
                    source: bytes[1],
                    matrix: bytes[2],
                };
            }
        }
        for (slot, matrix) in gx.tex_matrices.iter_mut().enumerate() {
            if let Some(entry) = table.entry_u16(0x48 + slot * 2, 0x40, 0x64, "MAT3 texture matrix")? {
                let mut info = GxTexMatrixInfo {
                    projection: block.u8(data, entry)?,
                    info: block.u8(data, entry + 1)?,
                    center: [
                        block.f32(data, entry + 0x04)?,
                        block.f32(data, entry + 0x08)?,
                        block.f32(data, entry + 0x0c)?,
                    ],
                    scale: [block.f32(data, entry + 0x10)?, block.f32(data, entry + 0x14)?],
                    rotation: block.i16(data, entry + 0x18)?,
                    translation: [block.f32(data, entry + 0x1c)?, block.f32(data, entry + 0x20)?],
                    ..GxTexMatrixInfo::default()
                };
                for (row, values) in info.effect.iter_mut().enumerate() {
                    for (column, value) in values.iter_mut().enumerate() {
                        *value = block.f32(data, entry + 0x24 + (row * 4 + column) * 4)?;
                    }
                }
                *matrix = Some(info);
            }
        }

        for (unit, texture) in gx.textures.iter_mut().enumerate() {
            if let Some(entry) = table.entry_u16(0x84 + unit * 2, 0x48, 2, "MAT3 texture table")? {
                let original = block.u16(data, entry)?;
                if original == u16::MAX {
                    continue;
                }
                *texture = texture_map.get(usize::from(original)).copied().flatten();
                if texture.is_none() {
                    warn(
                        warnings,
                        format!(
                            "MAT3 material {material_index} references unavailable TEX1 texture {original}; that map samples white"
                        ),
                    );
                }
            }
        }

        if let Some(entry) = table.entry_u8(0x04, 0x58, 1, "MAT3 TEV stage count")? {
            let count = block.u8(data, entry)?;
            if usize::from(count) > MAX_TEV_STAGES {
                return Err(FormatError::at(
                    block.start + init + 4,
                    "MAT3 has more than 16 TEV stages",
                ));
            }
            gx.stage_count = count.max(1);
        }
        for stage in 0..MAX_TEV_STAGES {
            if let Some(entry) = table.entry_u16(0xbc + stage * 2, 0x4c, 4, "MAT3 TEV order")? {
                let bytes = block.bytes(data, entry, 4, "MAT3 TEV order")?;
                if (bytes[1] != 0xff && bytes[1] > 7) || (bytes[0] != 0xff && bytes[0] > 7) {
                    return Err(FormatError::at(
                        block.start + entry,
                        "invalid MAT3 TEV order texture map/coordinate",
                    ));
                }
                gx.orders[stage] = GxTevOrder {
                    tex_coord: bytes[0],
                    tex_map: bytes[1],
                    channel: bytes[2],
                };
            }
            if let Some(entry) = table.entry_u16(0xe4 + stage * 2, 0x5c, 0x14, "MAT3 TEV stage")? {
                let b = block.bytes(data, entry, 0x14, "MAT3 TEV stage")?;
                gx.stages[stage] = GxTevStage {
                    color_in: [b[1], b[2], b[3], b[4]].map(|value| value.min(15)),
                    color_op: b[5],
                    color_bias: b[6],
                    color_scale: b[7] & 3,
                    color_clamp: b[8] != 0,
                    color_reg: b[9] & 3,
                    alpha_in: [b[10], b[11], b[12], b[13]].map(|value| value.min(7)),
                    alpha_op: b[14],
                    alpha_bias: b[15],
                    alpha_scale: b[16] & 3,
                    alpha_clamp: b[17] != 0,
                    alpha_reg: b[18] & 3,
                };
            }
            gx.konst_color_select[stage] = match block.u8(data, init + 0x9c + stage)? {
                0xff => 0x0c,
                value => value,
            };
            gx.konst_alpha_select[stage] = match block.u8(data, init + 0xac + stage)? {
                0xff => 0x1c,
                value => value,
            };
            if let Some(entry) = table.entry_u16(0x104 + stage * 2, 0x60, 4, "MAT3 TEV swap mode")? {
                let bytes = block.bytes(data, entry, 2, "MAT3 TEV swap mode")?;
                gx.swap_modes[stage] = (bytes[0] & 3, bytes[1] & 3);
            }
        }
        for slot in 0..4 {
            if let Some(entry) = table.entry_u16(0xdc + slot * 2, 0x50, 8, "MAT3 TEV color")? {
                let mut color = [0.0; 4];
                for (channel, value) in color.iter_mut().enumerate() {
                    *value = f32::from(block.i16(data, entry + channel * 2)?) / 255.0;
                }
                // J3D loads entries 0..2 into GX_TEVREG0..2; entry 3 seeds PREV.
                gx.registers[(slot + 1) % 4] = color;
            }
            if let Some(entry) = table.entry_u16(0x94 + slot * 2, 0x54, 4, "MAT3 TEV konst color")? {
                gx.konst_colors[slot] = rgba8(block.bytes(data, entry, 4, "MAT3 TEV konst color")?);
            }
            if let Some(entry) =
                table.entry_u16(0x124 + slot * 2, 0x64, 4, "MAT3 TEV swap table")?
            {
                let bytes = block.bytes(data, entry, 4, "MAT3 TEV swap table")?;
                gx.swap_tables[slot] = [bytes[0] & 3, bytes[1] & 3, bytes[2] & 3, bytes[3] & 3];
            }
        }

        if let Some(entry) = table.entry_u16(0x146, 0x6c, 8, "MAT3 alpha compare")? {
            let b = block.bytes(data, entry, 5, "MAT3 alpha compare")?;
            gx.alpha_test = GxAlphaTest {
                compare0: b[0] & 7,
                reference0: b[1],
                op: b[2] & 3,
                compare1: b[3] & 7,
                reference1: b[4],
            };
        }
        if let Some(entry) = table.entry_u16(0x148, 0x70, 4, "MAT3 blend")? {
            let b = block.bytes(data, entry, 4, "MAT3 blend")?;
            gx.blend = GxBlend {
                mode: b[0] & 3,
                source: b[1] & 7,
                destination: b[2] & 7,
                logic_op: b[3] & 15,
            };
        }
        if let Some(entry) = table.entry_u8(0x06, 0x74, 4, "MAT3 z mode")? {
            let b = block.bytes(data, entry, 3, "MAT3 z mode")?;
            gx.z_mode = GxZMode {
                test: b[0] != 0,
                function: b[1] & 7,
                write: b[2] != 0,
            };
        }
        gx.uses_indirect = indirect_stage_count(data, block, material_index)? > 0;
        if gx.uses_indirect {
            warn(
                warnings,
                format!(
                    "MAT3 material {material_index} uses indirect texturing; texture warping is not previewed"
                ),
            );
        }

        material.texture = basic_texture(&gx);
        material.gx = gx;
        Ok(material)
    }
}

/// The first texture a simple renderer should show for a material: the map
/// sampled by the earliest TEV stage.
fn basic_texture(gx: &GxMaterial) -> Option<usize> {
    gx.orders[..usize::from(gx.stage_count)]
        .iter()
        .find_map(|order| {
            (order.tex_map != 0xff)
                .then(|| gx.textures.get(usize::from(order.tex_map)).copied().flatten())
                .flatten()
        })
}

fn indirect_stage_count(data: &[u8], block: Block, material_index: usize) -> Result<u8> {
    let table = block.u32(data, 0x18)? as usize;
    let header = block.u32(data, 0x0c)? as usize;
    // Older or stripped models may omit the indirect table, or point it at the
    // material init data when it is empty.
    if table == 0 || table >= block.end - block.start || table == header {
        return Ok(0);
    }
    let entry = add_offset(table, mul_size(material_index, 0x138, block.start, "MAT3 indirect index")?, "MAT3 indirect data")?;
    if block.absolute(entry, 2, "MAT3 indirect data").is_err() {
        return Ok(0);
    }
    if block.u8(data, entry)? == 0 {
        return Ok(0);
    }
    block.u8(data, entry + 1)
}

fn rgba8(bytes: &[u8]) -> [f32; 4] {
    [0, 1, 2, 3].map(|channel| f32::from(bytes[channel]) / 255.0)
}

/// Resolves MAT3 per-material indices into entries of the shared value tables.
struct Mat3Tables<'a> {
    data: &'a [u8],
    block: Block,
    init: usize,
}

impl Mat3Tables<'_> {
    fn entry_u8(&self, field: usize, table: usize, size: usize, what: &str) -> Result<Option<usize>> {
        let index = self.block.u8(self.data, self.init + field)?;
        if index == 0xff {
            return Ok(None);
        }
        self.entry(usize::from(index), table, size, what)
    }

    fn entry_u16(&self, field: usize, table: usize, size: usize, what: &str) -> Result<Option<usize>> {
        let index = self.block.u16(self.data, self.init + field)?;
        if index == u16::MAX {
            return Ok(None);
        }
        self.entry(usize::from(index), table, size, what)
    }

    fn entry(&self, index: usize, table: usize, size: usize, what: &str) -> Result<Option<usize>> {
        let base = self.block.u32(self.data, table)? as usize;
        if base == 0 {
            return Ok(None);
        }
        let entry = add_offset(base, mul_size(index, size, self.block.start + table, what)?, what)?;
        self.block.absolute(entry, size, what)?;
        Ok(Some(entry))
    }
}

#[allow(clippy::too_many_arguments)]
fn build_meshes(
    data: &[u8],
    info: &Info,
    vertices: &VertexData,
    shapes: &ShapeData,
    _joints: &JointData,
    envelopes: &EnvelopeData,
    draws: &DrawData,
    hierarchy: &Hierarchy,
    materials: MaterialData<'_>,
    warnings: &mut Vec<String>,
) -> Result<Vec<BmdMesh>> {
    let mut meshes = Vec::new();
    reserve(
        &mut meshes,
        hierarchy.shapes.len().min(MAX_MESHES),
        "decoded BMD meshes",
    )?;
    let mut totals = OutputTotals {
        vertices: 0,
        indices: 0,
    };
    for link in &hierarchy.shapes {
        let material = materials.get(link.material)?;
        if let Some(mesh) = shapes.emit_shape(
            data,
            *link,
            info,
            vertices,
            hierarchy,
            envelopes,
            draws,
            material,
            &mut totals,
            warnings,
        )? {
            meshes.push(mesh);
        }
    }
    Ok(meshes)
}

fn decode_textures(
    data: &[u8],
    block: Block,
    warnings: &mut Vec<String>,
) -> Result<(Vec<BtiImage>, Vec<Option<usize>>)> {
    block.absolute(0, 0x14, "TEX1 header")?;
    let count = usize::from(block.u16(data, 8)?);
    if count > MAX_TEXTURES {
        return Err(FormatError::at(
            block.start + 8,
            format!("TEX1 has {count} textures; limit is {MAX_TEXTURES}"),
        ));
    }
    let headers = block.u32(data, 0x0c)?;
    if count != 0 && headers < 0x14 {
        return Err(FormatError::at(
            block.start + 0x0c,
            "TEX1 header table overlaps its header",
        ));
    }
    block.relative_offset(
        headers,
        mul_size(count, 0x20, block.start + 0x0c, "TEX1 header table")?,
        "TEX1 header table",
    )?;
    let headers_base = block.relative_offset(headers, 0, "TEX1 header table")?;
    let mut textures = Vec::new();
    let mut map = Vec::new();
    reserve(&mut textures, count, "decoded TEX1 textures")?;
    reserve(&mut map, count, "TEX1 texture index map")?;
    let mut rgba_bytes = 0usize;

    for index in 0..count {
        let absolute = add_offset(headers_base, index * 0x20, "TEX1 header table")?;
        let width = usize::from(be::u16_at(data, absolute + 2)?);
        let height = usize::from(be::u16_at(data, absolute + 4)?);
        let image_rgba = width
            .checked_mul(height)
            .and_then(|pixels| pixels.checked_mul(4))
            .ok_or_else(|| FormatError::at(absolute, "TEX1 image dimensions overflow"))?;
        rgba_bytes = rgba_bytes
            .checked_add(image_rgba)
            .ok_or_else(|| FormatError::at(absolute, "total TEX1 RGBA size overflow"))?;
        if rgba_bytes > MAX_TEXTURE_RGBA_BYTES {
            return Err(FormatError::at(
                absolute,
                format!("TEX1 decoded RGBA data exceeds {MAX_TEXTURE_RGBA_BYTES} bytes"),
            ));
        }
        match BtiImage::decode_at(&data[..block.end], absolute) {
            Ok(image) => {
                reserve(&mut textures, 1, "decoded TEX1 texture")?;
                map.push(Some(textures.len()));
                textures.push(image);
            }
            Err(error) => {
                warn(
                    warnings,
                    format!("TEX1 texture {index} was skipped: {error}"),
                );
                map.push(None);
            }
        }
    }
    Ok((textures, map))
}

// Tests use only synthetic data; no external dump or opt-in is required.
#[cfg(test)]
mod tests {
    use super::*;

    fn joint(rotation: [i16; 3]) -> Joint {
        Joint {
            scale: [1.0; 3],
            rotation,
            translation: [0.0; 3],
            ignore_parent_scale: false,
        }
    }

    fn apply(matrix: Matrix, point: [f32; 3]) -> [f32; 3] {
        transform_point(matrix, point).unwrap()
    }

    fn close(a: [f32; 3], b: [f32; 3]) -> bool {
        a.iter().zip(b).all(|(a, b)| (a - b).abs() < 1e-4)
    }

    #[test]
    fn joint_rotation_is_identity_at_rest_and_composes_zyx() {
        assert!(close(apply(rotation_translation(&joint([0; 3])), [1.0, 2.0, 3.0]), [1.0, 2.0, 3.0]));
        // 90 degrees about X maps +Y to +Z.
        assert!(close(apply(rotation_translation(&joint([16_384, 0, 0])), [0.0, 1.0, 0.0]), [0.0, 0.0, 1.0]));
        // 90 degrees about Z maps +X to +Y.
        assert!(close(apply(rotation_translation(&joint([0, 0, 16_384])), [1.0, 0.0, 0.0]), [0.0, 1.0, 0.0]));
        // Rz * Ry * Rx: X first, then Y, then Z.
        let matrix = rotation_translation(&joint([16_384, 16_384, 0]));
        assert!(close(apply(matrix, [0.0, 1.0, 0.0]), [1.0, 0.0, 0.0]));
    }
}
