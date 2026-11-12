#![forbid(unsafe_code)]

use std::ops::Range;

use crate::error::{FormatError, Result};
use crate::formats::be;

pub const BCO_VERSION: [u8; 4] = *b"0003";
pub const BCO_HEADER_SIZE: usize = 0x2c;
pub const BCO_TRIANGLE_SIZE: usize = 0x24;
pub const BCO_VERTEX_SIZE: usize = 0x0c;
pub const BCO_MATERIAL_SIZE: usize = 0x0c;
pub const BCO_GRID_CELL_SIZE: usize = 0x08;

#[derive(Clone, Debug, PartialEq)]
pub struct Bco {
    pub vertices: Vec<[f32; 3]>,
    pub triangles: Vec<BcoTriangle>,
    pub materials: Vec<BcoMaterial>,
    pub grid: BcoGridMetadata,
    pub grid_indices: Vec<u16>,
}

#[derive(Clone, Debug, PartialEq)]
pub struct BcoTriangle {
    pub indices: [u32; 3],
    pub direction: f32,
    pub normal: [i16; 3],
    pub attribute: u8,
    pub attribute_index: u8,
    pub min_max_indices: u8,
    pub camera_code: u8,
    pub neighbors: [u16; 3],
    pub thickness: u8,
    pub item_type: u8,
    pub unknown_21: u8,
    pub spiral_code: u8,
    pub stagger_code: u8,
    pub splash_code: u8,
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct BcoMaterial {
    pub col_flag: u8,
    pub sound_id: u8,
    pub field_2: u16,
    pub field_4: u32,
    pub field_8: u32,
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct BcoGridMetadata {
    pub x_count: u16,
    pub z_count: u16,
    pub x_start: i32,
    pub z_start: i32,
    pub x_cell_size: u32,
    pub z_cell_size: u32,
    pub cells: Vec<BcoGridCell>,
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct BcoGridCell {
    pub triangle_count: u8,
    pub ground_index: i32,
}

impl Bco {
    pub fn parse(data: &[u8]) -> Result<Self> {
        if be::bytes(data, 0, 4)? != BCO_VERSION {
            return Err(FormatError::at(0, "BCO version must be 0003"));
        }
        be::bytes(data, 0, BCO_HEADER_SIZE)?;

        let x_count = be::u16_at(data, 0x04)?;
        let z_count = be::u16_at(data, 0x06)?;
        let x_start = be::i32_at(data, 0x08)?;
        let z_start = be::i32_at(data, 0x0c)?;
        let x_cell_size = be::u32_at(data, 0x10)?;
        let z_cell_size = be::u32_at(data, 0x14)?;
        let material_count = usize::from(be::u16_at(data, 0x18)?);

        let index_offset = absolute_offset(data, 0x1c)?;
        let ground_offset = absolute_offset(data, 0x20)?;
        let vertex_offset = absolute_offset(data, 0x24)?;
        let material_offset = absolute_offset(data, 0x28)?;

        let cell_count = usize::from(x_count)
            .checked_mul(usize::from(z_count))
            .ok_or_else(|| FormatError::at(0x04, "grid dimensions overflow"))?;
        let grid_bytes = cell_count
            .checked_mul(BCO_GRID_CELL_SIZE)
            .ok_or_else(|| FormatError::at(0x04, "grid table size overflow"))?;
        let grid_range = be::range(data, BCO_HEADER_SIZE, grid_bytes)?;
        if grid_range.end > index_offset {
            return Err(FormatError::at(
                index_offset,
                "index table overlaps the grid table",
            ));
        }

        let index_range = table_range(data, index_offset, ground_offset, 2, "triangle index")?;
        let ground_range = table_range(
            data,
            ground_offset,
            vertex_offset,
            BCO_TRIANGLE_SIZE,
            "ground",
        )?;
        let vertex_range = table_range(
            data,
            vertex_offset,
            material_offset,
            BCO_VERTEX_SIZE,
            "vertex",
        )?;
        let material_bytes = material_count
            .checked_mul(BCO_MATERIAL_SIZE)
            .ok_or_else(|| FormatError::at(0x18, "material table size overflow"))?;
        let material_range = be::range(data, material_offset, material_bytes)?;

        let mut cells = reserved_vec(cell_count, BCO_HEADER_SIZE, "grid cells")?;
        for offset in grid_range.step_by(BCO_GRID_CELL_SIZE) {
            cells.push(BcoGridCell {
                triangle_count: be::u8_at(data, offset)?,
                ground_index: be::i32_at(data, offset + 4)?,
            });
        }

        let mut grid_indices = reserved_vec(index_range.len() / 2, index_offset, "indices")?;
        for offset in index_range.step_by(2) {
            grid_indices.push(be::u16_at(data, offset)?);
        }

        let mut triangles = reserved_vec(
            ground_range.len() / BCO_TRIANGLE_SIZE,
            ground_offset,
            "triangles",
        )?;
        for offset in ground_range.step_by(BCO_TRIANGLE_SIZE) {
            triangles.push(parse_triangle(data, offset)?);
        }

        let mut vertices = reserved_vec(
            vertex_range.len() / BCO_VERTEX_SIZE,
            vertex_offset,
            "vertices",
        )?;
        for offset in vertex_range.step_by(BCO_VERTEX_SIZE) {
            vertices.push(be::vec3_at(data, offset)?);
        }

        let mut materials = reserved_vec(material_count, material_offset, "materials")?;
        for offset in material_range.step_by(BCO_MATERIAL_SIZE) {
            materials.push(BcoMaterial {
                col_flag: be::u8_at(data, offset)?,
                sound_id: be::u8_at(data, offset + 1)?,
                field_2: be::u16_at(data, offset + 2)?,
                field_4: be::u32_at(data, offset + 4)?,
                field_8: be::u32_at(data, offset + 8)?,
            });
        }

        for (triangle_index, triangle) in triangles.iter().enumerate() {
            for &vertex_index in &triangle.indices {
                if usize::try_from(vertex_index).map_or(true, |index| index >= vertices.len()) {
                    return Err(FormatError::at(
                        ground_offset + triangle_index * BCO_TRIANGLE_SIZE,
                        format!(
                            "triangle {triangle_index} references invalid vertex {vertex_index}"
                        ),
                    ));
                }
            }
        }

        for (cell_index, cell) in cells.iter().enumerate() {
            let Some(indices) = cell.indices(&grid_indices) else {
                return Err(FormatError::at(
                    BCO_HEADER_SIZE + cell_index * BCO_GRID_CELL_SIZE,
                    "grid cell has an invalid triangle index range",
                ));
            };
            if let Some(&invalid) = indices
                .iter()
                .find(|&&index| usize::from(index) >= triangles.len())
            {
                return Err(FormatError::at(
                    BCO_HEADER_SIZE + cell_index * BCO_GRID_CELL_SIZE,
                    format!("grid cell references invalid triangle {invalid}"),
                ));
            }
        }

        Ok(Self {
            vertices,
            triangles,
            materials,
            grid: BcoGridMetadata {
                x_count,
                z_count,
                x_start,
                z_start,
                x_cell_size,
                z_cell_size,
                cells,
            },
            grid_indices,
        })
    }

    pub fn mesh_indices(&self) -> Vec<u32> {
        self.triangles
            .iter()
            .flat_map(|triangle| triangle.indices)
            .collect()
    }

    pub fn triangle_vertices(&self, triangle_index: usize) -> Option<[[f32; 3]; 3]> {
        let triangle = self.triangles.get(triangle_index)?;
        Some([
            *self
                .vertices
                .get(usize::try_from(triangle.indices[0]).ok()?)?,
            *self
                .vertices
                .get(usize::try_from(triangle.indices[1]).ok()?)?,
            *self
                .vertices
                .get(usize::try_from(triangle.indices[2]).ok()?)?,
        ])
    }

    pub fn material_for_triangle(&self, triangle: &BcoTriangle) -> Option<&BcoMaterial> {
        self.materials.iter().find(|material| {
            material.col_flag == triangle.attribute && material.sound_id == triangle.attribute_index
        })
    }
}

impl BcoTriangle {
    pub fn surface_name(&self) -> &'static str {
        surface_name(self.attribute)
    }

    pub fn surface_color(&self) -> [f32; 4] {
        surface_color(self.attribute)
    }
}

impl BcoMaterial {
    pub fn surface_name(&self) -> &'static str {
        surface_name(self.col_flag)
    }

    pub fn surface_color(&self) -> [f32; 4] {
        surface_color(self.col_flag)
    }
}

impl BcoGridCell {
    pub fn indices<'a>(&self, table: &'a [u16]) -> Option<&'a [u16]> {
        let length = usize::from(self.triangle_count);
        if length == 0 {
            return Some(&table[0..0]);
        }
        let start = usize::try_from(self.ground_index).ok()?;
        let end = start.checked_add(length)?;
        table.get(start..end)
    }
}

pub const fn surface_name(attribute: u8) -> &'static str {
    match attribute {
        0x00 => "DIRT",
        0x01 => "ROAD",
        0x02 => "WALL",
        0x03 => "GRASS",
        0x04 => "ICE",
        0x05 => "RESQ",
        0x06 => "SLIP",
        0x07 => "DASHJ",
        0x08 => "DASHB",
        0x09 => "CANNON",
        0x0a => "VALLEY",
        0x0e => "RESQD",
        0x0f => "RESQS",
        0x11 => "BEACH",
        0x12 => "OUTF",
        0xff => "NONE",
        _ => "UNKNOWN",
    }
}

/// Returns an editor display color; colors are not stored in BCO materials.
pub const fn surface_color(attribute: u8) -> [f32; 4] {
    match attribute {
        0x00 => [0.58, 0.40, 0.20, 1.0],
        0x01 => [0.52, 0.55, 0.60, 1.0],
        0x02 | 0x12 => [0.90, 0.25, 0.25, 1.0],
        0x03 => [0.22, 0.68, 0.28, 1.0],
        0x04 => [0.35, 0.82, 0.95, 1.0],
        0x05 | 0x0a | 0x0e | 0x0f => [0.80, 0.35, 0.85, 1.0],
        0x06 => [0.48, 0.35, 0.65, 1.0],
        0x07..=0x09 => [0.98, 0.65, 0.12, 1.0],
        0x11 => [0.90, 0.78, 0.45, 1.0],
        0xff => [0.45, 0.45, 0.45, 0.35],
        _ => [0.80, 0.30, 0.65, 1.0],
    }
}

fn parse_triangle(data: &[u8], offset: usize) -> Result<BcoTriangle> {
    let mut indices = [0_u32; 3];
    for (component, index) in indices.iter_mut().enumerate() {
        let index_offset = offset + component * 4;
        let value = be::i32_at(data, index_offset)?;
        *index = u32::try_from(value)
            .map_err(|_| FormatError::at(index_offset, format!("negative vertex index {value}")))?;
    }

    let packed_20 = be::u8_at(data, offset + 0x20)?;
    let packed_22 = be::u8_at(data, offset + 0x22)?;
    Ok(BcoTriangle {
        indices,
        direction: be::f32_at(data, offset + 0x0c)?,
        normal: [
            be::i16_at(data, offset + 0x10)?,
            be::i16_at(data, offset + 0x12)?,
            be::i16_at(data, offset + 0x14)?,
        ],
        attribute: be::u8_at(data, offset + 0x16)?,
        attribute_index: be::u8_at(data, offset + 0x17)?,
        min_max_indices: be::u8_at(data, offset + 0x18)?,
        camera_code: be::u8_at(data, offset + 0x19)?,
        neighbors: [
            be::u16_at(data, offset + 0x1a)?,
            be::u16_at(data, offset + 0x1c)?,
            be::u16_at(data, offset + 0x1e)?,
        ],
        thickness: packed_20 >> 4,
        item_type: packed_20 & 0x0f,
        unknown_21: be::u8_at(data, offset + 0x21)?,
        spiral_code: packed_22 >> 4,
        stagger_code: packed_22 & 0x0f,
        splash_code: be::u8_at(data, offset + 0x23)?,
    })
}

fn absolute_offset(data: &[u8], field_offset: usize) -> Result<usize> {
    let raw = be::u32_at(data, field_offset)?;
    let offset = usize::try_from(raw)
        .map_err(|_| FormatError::at(field_offset, "absolute offset does not fit usize"))?;
    if offset < BCO_HEADER_SIZE || offset > data.len() {
        return Err(FormatError::at(
            field_offset,
            format!("absolute offset 0x{raw:x} is outside the file"),
        ));
    }
    Ok(offset)
}

fn table_range(
    data: &[u8],
    start: usize,
    end: usize,
    entry_size: usize,
    name: &str,
) -> Result<Range<usize>> {
    let length = end.checked_sub(start).ok_or_else(|| {
        FormatError::at(
            start,
            format!("{name} table starts after its following table"),
        )
    })?;
    if length % entry_size != 0 {
        return Err(FormatError::at(
            start,
            format!("{name} table size is not a multiple of 0x{entry_size:x}"),
        ));
    }
    be::range(data, start, length)
}

fn reserved_vec<T>(count: usize, offset: usize, name: &str) -> Result<Vec<T>> {
    let mut values = Vec::new();
    values
        .try_reserve_exact(count)
        .map_err(|_| FormatError::at(offset, format!("cannot allocate {name} table")))?;
    Ok(values)
}

#[cfg(test)]
mod tests {
    use super::*;

    fn push_u16(data: &mut Vec<u8>, value: u16) {
        data.extend_from_slice(&value.to_be_bytes());
    }

    fn push_i16(data: &mut Vec<u8>, value: i16) {
        data.extend_from_slice(&value.to_be_bytes());
    }

    fn push_u32(data: &mut Vec<u8>, value: u32) {
        data.extend_from_slice(&value.to_be_bytes());
    }

    fn set_u32(data: &mut [u8], offset: usize, value: u32) {
        data[offset..offset + 4].copy_from_slice(&value.to_be_bytes());
    }

    fn fixture() -> Vec<u8> {
        let index_offset = BCO_HEADER_SIZE + BCO_GRID_CELL_SIZE;
        let ground_offset = index_offset + 2;
        let vertex_offset = ground_offset + BCO_TRIANGLE_SIZE;
        let material_offset = vertex_offset + 3 * BCO_VERTEX_SIZE;

        let mut data = Vec::new();
        data.extend_from_slice(&BCO_VERSION);
        push_u16(&mut data, 1);
        push_u16(&mut data, 1);
        push_u32(&mut data, (-100_i32) as u32);
        push_u32(&mut data, 200);
        push_u32(&mut data, 500);
        push_u32(&mut data, 600);
        push_u16(&mut data, 1);
        push_u16(&mut data, 0);
        push_u32(&mut data, index_offset as u32);
        push_u32(&mut data, ground_offset as u32);
        push_u32(&mut data, vertex_offset as u32);
        push_u32(&mut data, material_offset as u32);

        data.extend_from_slice(&[1, 0x12, 0x34, 0x56]);
        push_u32(&mut data, 0);
        push_u16(&mut data, 0);

        push_u32(&mut data, 0);
        push_u32(&mut data, 1);
        push_u32(&mut data, 2);
        push_u32(&mut data, 42.5_f32.to_bits());
        push_i16(&mut data, 1);
        push_i16(&mut data, -2);
        push_i16(&mut data, 3);
        data.extend_from_slice(&[3, 7, 0xe4, 9]);
        push_u16(&mut data, 0);
        push_u16(&mut data, 0xffff);
        push_u16(&mut data, 2);
        data.extend_from_slice(&[0xab, 0xcc, 0xde, 0xf0]);

        for vertex in [[0.0, 1.0, 2.0], [3.0, 4.0, 5.0], [6.0, 7.0, 8.0]] {
            for component in vertex {
                push_u32(&mut data, f32::to_bits(component));
            }
        }

        data.extend_from_slice(&[3, 7]);
        push_u16(&mut data, 0x1234);
        push_u32(&mut data, 0x5678_9abc);
        push_u32(&mut data, 0xdef0_1234);
        data
    }

    fn empty_fixture() -> Vec<u8> {
        let mut data = Vec::new();
        data.extend_from_slice(&BCO_VERSION);
        push_u16(&mut data, 1);
        push_u16(&mut data, 1);
        push_u32(&mut data, 0);
        push_u32(&mut data, 0);
        push_u32(&mut data, 0);
        push_u32(&mut data, 0);
        push_u16(&mut data, 0);
        push_u16(&mut data, 0);
        for _ in 0..4 {
            push_u32(&mut data, (BCO_HEADER_SIZE + BCO_GRID_CELL_SIZE) as u32);
        }
        data.extend_from_slice(&[0, 0, 0, 0]);
        push_u32(&mut data, u32::MAX);
        data
    }

    #[test]
    fn extracts_triangle_and_material_data() {
        let data = fixture();
        let bco = Bco::parse(&data).unwrap();

        assert_eq!(
            bco.vertices,
            vec![[0.0, 1.0, 2.0], [3.0, 4.0, 5.0], [6.0, 7.0, 8.0]]
        );
        assert_eq!(bco.mesh_indices(), vec![0, 1, 2]);
        assert_eq!(
            bco.triangle_vertices(0),
            Some([[0.0, 1.0, 2.0], [3.0, 4.0, 5.0], [6.0, 7.0, 8.0]])
        );
        assert_eq!(bco.triangle_vertices(1), None);

        let triangle = &bco.triangles[0];
        assert_eq!(triangle.indices, [0, 1, 2]);
        assert_eq!(triangle.direction, 42.5);
        assert_eq!(triangle.normal, [1, -2, 3]);
        assert_eq!(triangle.attribute, 3);
        assert_eq!(triangle.attribute_index, 7);
        assert_eq!(triangle.min_max_indices, 0xe4);
        assert_eq!(triangle.camera_code, 9);
        assert_eq!(triangle.neighbors, [0, 0xffff, 2]);
        assert_eq!(triangle.thickness, 0x0a);
        assert_eq!(triangle.item_type, 0x0b);
        assert_eq!(triangle.unknown_21, 0xcc);
        assert_eq!(triangle.spiral_code, 0x0d);
        assert_eq!(triangle.stagger_code, 0x0e);
        assert_eq!(triangle.splash_code, 0xf0);
        assert_eq!(triangle.surface_name(), "GRASS");
        assert_eq!(triangle.surface_color(), surface_color(3));

        let material = bco.material_for_triangle(triangle).unwrap();
        assert_eq!(material.col_flag, 3);
        assert_eq!(material.sound_id, 7);
        assert_eq!(material.field_2, 0x1234);
        assert_eq!(material.field_4, 0x5678_9abc);
        assert_eq!(material.field_8, 0xdef0_1234);
        assert_eq!(material.surface_name(), "GRASS");

        assert_eq!(bco.grid.x_count, 1);
        assert_eq!(bco.grid.z_count, 1);
        assert_eq!(bco.grid.x_start, -100);
        assert_eq!(bco.grid.z_start, 200);
        assert_eq!(bco.grid.x_cell_size, 500);
        assert_eq!(bco.grid.z_cell_size, 600);
        assert_eq!(bco.grid.cells[0].indices(&bco.grid_indices), Some(&[0][..]));
    }

    #[test]
    fn accepts_empty_tables_and_unused_negative_grid_indices() {
        let bco = Bco::parse(&empty_fixture()).unwrap();
        assert!(bco.vertices.is_empty());
        assert!(bco.triangles.is_empty());
        assert!(bco.materials.is_empty());
        assert!(bco.grid_indices.is_empty());
        assert_eq!(bco.grid.cells.len(), 1);
        assert_eq!(bco.grid.cells[0].triangle_count, 0);
        assert_eq!(bco.grid.cells[0].ground_index, -1);
        assert_eq!(bco.grid.cells[0].indices(&[]), Some(&[][..]));
    }

    #[test]
    fn rejects_negative_and_out_of_range_vertex_indices() {
        let original = fixture();
        let ground_offset = be::u32_at(&original, 0x20).unwrap() as usize;

        let mut negative = original.clone();
        set_u32(&mut negative, ground_offset + 4, u32::MAX);
        assert!(Bco::parse(&negative).is_err());

        let mut out_of_range = original;
        set_u32(&mut out_of_range, ground_offset + 8, 3);
        assert!(Bco::parse(&out_of_range).is_err());
    }

    #[test]
    fn rejects_invalid_grid_indices() {
        let original = fixture();
        let index_offset = be::u32_at(&original, 0x1c).unwrap() as usize;

        let mut bad_triangle = original;
        bad_triangle[index_offset..index_offset + 2].copy_from_slice(&1_u16.to_be_bytes());
        assert!(Bco::parse(&bad_triangle).is_err());

        let mut bad_range = fixture();
        bad_range[BCO_HEADER_SIZE + 4..BCO_HEADER_SIZE + 8]
            .copy_from_slice(&u32::MAX.to_be_bytes());
        assert!(Bco::parse(&bad_range).is_err());
    }

    #[test]
    fn malformed_input_never_panics() {
        assert!(Bco::parse(&[]).is_err());

        let original = fixture();
        for length in 0..original.len() {
            assert!(Bco::parse(&original[..length]).is_err());
        }

        let mut wrong_version = original.clone();
        wrong_version[0..4].copy_from_slice(b"0004");
        assert!(Bco::parse(&wrong_version).is_err());

        let mut overflowing = original.clone();
        set_u32(&mut overflowing, 0x1c, u32::MAX);
        assert!(Bco::parse(&overflowing).is_err());

        let mut overlapping = original;
        let vertex_offset = be::u32_at(&overlapping, 0x24).unwrap();
        set_u32(&mut overlapping, 0x20, vertex_offset + 1);
        assert!(Bco::parse(&overlapping).is_err());
    }
}
