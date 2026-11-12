//! Mario Kart: Double Dash BOL (version 0015) course objects.
//!
//! Fixed-size edits are applied directly to the input image. Consequently an
//! untouched document, including section gaps and trailing bytes, round-trips
//! exactly. Structural edits use the canonical contiguous BOL layout.

use crate::error::{FormatError, Result};
use crate::formats::be;
use serde::{Deserialize, Serialize};
use serde_json::Value;

pub const BOL_MAGIC: &[u8; 4] = b"0015";
pub const HEADER_SIZE: usize = 0x7c;
pub const CENTERLINE_SIZE: usize = 0x20;
pub const CHECKPOINT_GROUP_SIZE: usize = 0x14;
pub const CHECKPOINT_SIZE: usize = 0x1c;
pub const PATH_SIZE: usize = 0x10;
pub const ROUTE_POINT_SIZE: usize = 0x20;
pub const OBJECT_SIZE: usize = 0x40;
pub const START_SIZE: usize = 0x28;
pub const AREA_SIZE: usize = 0x38;
pub const CAMERA_SIZE: usize = 0x48;
pub const RESPAWN_SIZE: usize = 0x20;
pub const LIGHT_SIZE: usize = 0x14;
pub const MG_SIZE: usize = 0x08;

#[derive(Clone, Debug, Default, PartialEq, Serialize, Deserialize)]
pub struct BolCounts {
    pub centerline: usize,
    pub checkpoint_groups: usize,
    pub checkpoints: usize,
    pub paths: usize,
    pub route_points: usize,
    pub objects: usize,
    pub starts: usize,
    pub areas: usize,
    pub cameras: usize,
    pub respawns: usize,
    pub lights: usize,
    pub mg: usize,
}

#[derive(Clone, Copy, Debug, Default, PartialEq, Eq, Serialize, Deserialize)]
pub struct SectionOffsets {
    pub centerline: u32,
    pub checkpoints: u32,
    pub paths: u32,
    pub route_points: u32,
    pub objects: u32,
    pub starts: u32,
    pub areas: u32,
    pub cameras: u32,
    pub respawns: u32,
    pub lights: u32,
    pub mg: u32,
}

#[derive(Clone, Debug, PartialEq, Serialize, Deserialize)]
pub struct BolHeader {
    pub tilt: u8,
    pub ambient_color: [u8; 3],
    pub light_color: [u8; 4],
    pub light_position: [f32; 3],
    pub lap_count: u8,
    pub course_id: u8,
    pub fog_type: u8,
    pub fog_color: [u8; 3],
    pub fog_start_z: f32,
    pub fog_end_z: f32,
    pub tex_lod_bias: u8,
    pub dummy_start_line: u8,
    pub snow_enabled: u8,
    pub shadow_opacity: u8,
    pub shadow_color: [u8; 3],
    pub sky_follow: u8,
    pub unknown_3f: u8,
    pub file_start: u32,
    pub reserved: [u8; 12],
}

#[derive(Clone, Debug, PartialEq, Serialize, Deserialize)]
pub struct CenterlinePoint {
    pub position: [f32; 3],
    pub drift_direction: u16,
    pub link: i16,
    pub scale: f32,
    pub swerve: i8,
    pub group_setting: u8,
    pub group: u8,
    pub drift_acuteness: u8,
    pub drift_duration: u8,
    pub drift_supplement: u8,
    pub no_mushroom_zone: u8,
    pub padding: [u8; 5],
}

#[derive(Clone, Debug, PartialEq, Serialize, Deserialize)]
pub struct Checkpoint {
    pub start: [f32; 3],
    pub end: [f32; 3],
    pub shortcut_id: u8,
    pub unknown_19: u8,
    pub reverse_search: u8,
    pub unknown_1b: u8,
}

#[derive(Clone, Debug, PartialEq, Serialize, Deserialize)]
pub struct CheckpointGroup {
    pub link: u16,
    pub previous: [i16; 4],
    pub next: [i16; 4],
    pub points: Vec<Checkpoint>,
}

#[derive(Clone, Debug, PartialEq, Serialize, Deserialize)]
pub struct Path {
    pub point_count: u16,
    pub point_start: u16,
    pub unknown: u16,
    pub flags: u16,
    pub padding: [u8; 8],
}

impl Path {
    pub fn closed(&self) -> bool {
        self.flags & 1 != 0
    }
}

#[derive(Clone, Debug, PartialEq, Serialize, Deserialize)]
pub struct RoutePoint {
    pub position: [f32; 3],
    pub link_point: i32,
    pub padding: [u8; 16],
}

impl Default for RoutePoint {
    fn default() -> Self {
        Self {
            position: [0.0; 3],
            link_point: 0,
            padding: [0; 16],
        }
    }
}

#[derive(Clone, Debug, PartialEq, Serialize, Deserialize)]
pub struct MapObject {
    pub position: [f32; 3],
    pub scale: [f32; 3],
    pub forward: [i16; 3],
    pub up: [i16; 3],
    pub id: u16,
    pub path_id: i16,
    pub unknown_28: u16,
    pub point_idx: u16,
    pub presence_filter: u8,
    pub presence: u8,
    pub collision_flag: u8,
    pub unknown_2f: u8,
    pub params: [i16; 8],
}

impl Default for MapObject {
    fn default() -> Self {
        Self {
            position: [0.0; 3],
            scale: [1.0; 3],
            forward: [10_000, 0, 0],
            up: [0, 10_000, 0],
            id: 0,
            path_id: -1,
            unknown_28: 0,
            point_idx: u16::MAX,
            presence_filter: 0x8f,
            presence: 3,
            collision_flag: 0,
            unknown_2f: 0,
            params: [0; 8],
        }
    }
}

#[derive(Clone, Debug, PartialEq, Serialize, Deserialize)]
pub struct StartPoint {
    pub position: [f32; 3],
    pub scale: [f32; 3],
    pub forward: [i16; 3],
    pub up: [i16; 3],
    pub pole_position: u8,
    pub player_id: u8,
    pub jugem_offset_y: i16,
}

#[derive(Clone, Debug, PartialEq, Serialize, Deserialize)]
pub struct Area {
    pub position: [f32; 3],
    pub scale: [f32; 3],
    pub forward: [i16; 3],
    pub up: [i16; 3],
    pub shape: u8,
    pub area_type: u8,
    pub camera_id: i16,
    pub feather: [i32; 2],
    pub echo_scale: i16,
    pub unknown_32: i16,
    pub shadow_id: i16,
    pub light_param_id: i16,
}

#[derive(Clone, Debug, PartialEq, Serialize, Deserialize)]
pub struct Camera {
    pub position: [f32; 3],
    pub direction: [i16; 3],
    pub padding_12: [u8; 6],
    pub target_position2: [f32; 3],
    pub target_position: [f32; 3],
    pub kind: u16,
    pub start_fov: u16,
    pub duration: i16,
    pub start_camera: u16,
    pub shimmer_z0: i16,
    pub shimmer_z1: i16,
    pub path_id: i16,
    pub path_speed: i16,
    pub end_fov: u16,
    pub next_camera_id: i16,
    pub name: [u8; 4],
}

#[derive(Clone, Debug, PartialEq, Serialize, Deserialize)]
pub struct RespawnPoint {
    pub position: [f32; 3],
    pub forward: [i16; 3],
    pub up: [i16; 3],
    pub id: u16,
    pub cl_point_id: u16,
    pub camera_id: i16,
    pub sector_id: i16,
}

#[derive(Clone, Debug, PartialEq, Serialize, Deserialize)]
pub struct LightParam {
    pub color: [u8; 4],
    pub position: [f32; 3],
    pub ambient_color: [u8; 4],
}

#[derive(Clone, Debug, PartialEq, Serialize, Deserialize)]
pub struct MgParam {
    pub rabbit_win_sec: i16,
    pub rabbit_min_sec: i16,
    pub rabbit_dec_sec: i16,
    pub padding: i16,
}

#[derive(Clone, Debug, PartialEq, Serialize, Deserialize)]
pub struct SceneEntity {
    pub kind: &'static str,
    pub index: usize,
    pub position: [f32; 3],
    pub secondary: Option<[f32; 3]>,
    pub label: String,
}

#[derive(Clone, Debug, PartialEq, Serialize, Deserialize)]
pub struct ScenePath {
    pub kind: &'static str,
    pub index: usize,
    pub points: Vec<[f32; 3]>,
    pub closed: bool,
}

#[derive(Clone, Debug, PartialEq, Serialize, Deserialize)]
pub struct FieldMetadata {
    pub key: String,
    pub value: Value,
    pub value_type: String,
    pub editable: bool,
}

#[derive(Clone, Debug, PartialEq, Serialize, Deserialize)]
pub struct Inspection {
    pub kind: String,
    pub index: usize,
    pub fields: Vec<FieldMetadata>,
}

#[derive(Clone, Debug, PartialEq, Eq, Serialize, Deserialize)]
pub struct ValidationIssue {
    pub code: String,
    pub message: String,
    pub kind: Option<String>,
    pub index: Option<usize>,
}

#[derive(Clone, Debug, Default, PartialEq, Eq, Serialize, Deserialize)]
pub struct ValidationReport {
    pub warnings: Vec<ValidationIssue>,
    pub errors: Vec<ValidationIssue>,
}

#[derive(Clone, Debug)]
pub struct BolDocument {
    pub header: BolHeader,
    pub counts: BolCounts,
    pub offsets: SectionOffsets,
    pub centerline: Vec<CenterlinePoint>,
    pub checkpoint_groups: Vec<CheckpointGroup>,
    pub paths: Vec<Path>,
    pub route_points: Vec<RoutePoint>,
    pub objects: Vec<MapObject>,
    pub starts: Vec<StartPoint>,
    pub areas: Vec<Area>,
    pub cameras: Vec<Camera>,
    pub respawns: Vec<RespawnPoint>,
    pub lights: Vec<LightParam>,
    pub mg: Vec<MgParam>,
    original: Vec<u8>,
    current: Vec<u8>,
    modified: bool,
    canonical: bool,
}

fn error(message: impl Into<String>) -> FormatError {
    FormatError::new(message)
}

fn read_array<const N: usize>(data: &[u8], offset: usize) -> Result<[u8; N]> {
    Ok(be::bytes(data, offset, N)?.try_into().unwrap())
}

fn read_i16_vec3(data: &[u8], offset: usize) -> Result<[i16; 3]> {
    Ok([
        be::i16_at(data, offset)?,
        be::i16_at(data, offset + 2)?,
        be::i16_at(data, offset + 4)?,
    ])
}

fn put_u8(data: &mut [u8], offset: usize, value: u8) -> Result<()> {
    be::bytes_mut(data, offset, 1)?[0] = value;
    Ok(())
}

fn put_i8(data: &mut [u8], offset: usize, value: i8) -> Result<()> {
    put_u8(data, offset, value as u8)
}

fn put_bytes(data: &mut [u8], offset: usize, value: &[u8]) -> Result<()> {
    be::bytes_mut(data, offset, value.len())?.copy_from_slice(value);
    Ok(())
}

fn put_i8_array(data: &mut [u8], offset: usize, value: i8) -> Result<()> {
    put_i8(data, offset, value)
}

// Raw bit writes are intentional here. Parsing and no-op/structural export must
// retain even unusual NaN payloads. JSON field edits can only introduce finite
// JSON numbers, while be::put_f32 remains available to callers that need it.
fn put_f32_raw(data: &mut [u8], offset: usize, value: f32) -> Result<()> {
    be::put_u32(data, offset, value.to_bits())
}

fn put_vec3_raw(data: &mut [u8], offset: usize, value: [f32; 3]) -> Result<()> {
    for (i, component) in value.into_iter().enumerate() {
        put_f32_raw(data, offset + i * 4, component)?;
    }
    Ok(())
}

fn put_i16_vec3(data: &mut [u8], offset: usize, value: [i16; 3]) -> Result<()> {
    for (i, component) in value.into_iter().enumerate() {
        be::put_i16(data, offset + i * 2, component)?;
    }
    Ok(())
}

fn region(
    data: &[u8],
    offset: u32,
    count: usize,
    size: usize,
    name: &str,
) -> Result<std::ops::Range<usize>> {
    let length = count
        .checked_mul(size)
        .ok_or_else(|| error(format!("{name} byte length overflow")))?;
    be::range(data, offset as usize, length).map_err(|mut e| {
        e.message = format!("{name}: {}", e.message);
        e
    })
}

impl BolDocument {
    pub fn parse(data: &[u8]) -> Result<Self> {
        if be::bytes(data, 0, HEADER_SIZE).is_err() {
            return Err(FormatError::at(0, "BOL header is truncated"));
        }
        if be::bytes(data, 0, 4)? != BOL_MAGIC {
            return Err(FormatError::at(0, "invalid BOL magic; expected 0015"));
        }

        let header = BolHeader {
            tilt: be::u8_at(data, 0x04)?,
            ambient_color: read_array(data, 0x05)?,
            light_color: read_array(data, 0x08)?,
            light_position: be::vec3_at(data, 0x0c)?,
            lap_count: be::u8_at(data, 0x18)?,
            course_id: be::u8_at(data, 0x19)?,
            fog_type: be::u8_at(data, 0x28)?,
            fog_color: read_array(data, 0x29)?,
            fog_start_z: be::f32_at(data, 0x2c)?,
            fog_end_z: be::f32_at(data, 0x30)?,
            tex_lod_bias: be::u8_at(data, 0x34)?,
            dummy_start_line: be::u8_at(data, 0x35)?,
            snow_enabled: be::u8_at(data, 0x36)?,
            shadow_opacity: be::u8_at(data, 0x37)?,
            shadow_color: read_array(data, 0x38)?,
            sky_follow: be::u8_at(data, 0x3c)?,
            unknown_3f: be::u8_at(data, 0x3f)?,
            file_start: be::u32_at(data, 0x40)?,
            reserved: read_array(data, 0x70)?,
        };
        let offsets = SectionOffsets {
            centerline: be::u32_at(data, 0x44)?,
            checkpoints: be::u32_at(data, 0x48)?,
            paths: be::u32_at(data, 0x4c)?,
            route_points: be::u32_at(data, 0x50)?,
            objects: be::u32_at(data, 0x54)?,
            starts: be::u32_at(data, 0x58)?,
            areas: be::u32_at(data, 0x5c)?,
            cameras: be::u32_at(data, 0x60)?,
            respawns: be::u32_at(data, 0x64)?,
            lights: be::u32_at(data, 0x68)?,
            mg: be::u32_at(data, 0x6c)?,
        };

        let cl_count = be::u16_at(data, 0x1a)? as usize;
        let cp_group_count = be::u16_at(data, 0x1c)? as usize;
        let object_count = be::u16_at(data, 0x1e)? as usize;
        let area_count = be::u16_at(data, 0x20)? as usize;
        let camera_count = be::u16_at(data, 0x22)? as usize;
        let path_count = be::u16_at(data, 0x24)? as usize;
        let respawn_count = be::u16_at(data, 0x26)? as usize;
        let start_count = be::u8_at(data, 0x3b)? as usize;
        let light_count = be::u8_at(data, 0x3d)? as usize;
        let mg_count = be::u8_at(data, 0x3e)? as usize;

        region(
            data,
            offsets.centerline,
            cl_count,
            CENTERLINE_SIZE,
            "centerline",
        )?;
        region(
            data,
            offsets.checkpoints,
            cp_group_count,
            CHECKPOINT_GROUP_SIZE,
            "checkpoint groups",
        )?;
        region(data, offsets.paths, path_count, PATH_SIZE, "paths")?;
        region(data, offsets.objects, object_count, OBJECT_SIZE, "objects")?;
        region(
            data,
            offsets.starts,
            start_count,
            START_SIZE,
            "starting points",
        )?;
        region(data, offsets.areas, area_count, AREA_SIZE, "areas")?;
        region(data, offsets.cameras, camera_count, CAMERA_SIZE, "cameras")?;
        region(
            data,
            offsets.respawns,
            respawn_count,
            RESPAWN_SIZE,
            "respawns",
        )?;
        region(
            data,
            offsets.lights,
            light_count,
            LIGHT_SIZE,
            "light parameters",
        )?;
        region(data, offsets.mg, mg_count, MG_SIZE, "MG parameters")?;

        let mut centerline = Vec::with_capacity(cl_count);
        for i in 0..cl_count {
            let base = offsets.centerline as usize + i * CENTERLINE_SIZE;
            centerline.push(CenterlinePoint {
                position: be::vec3_at(data, base)?,
                drift_direction: be::u16_at(data, base + 0x0c)?,
                link: be::i16_at(data, base + 0x0e)?,
                scale: be::f32_at(data, base + 0x10)?,
                swerve: be::i8_at(data, base + 0x14)?,
                group_setting: be::u8_at(data, base + 0x15)?,
                group: be::u8_at(data, base + 0x16)?,
                drift_acuteness: be::u8_at(data, base + 0x17)?,
                drift_duration: be::u8_at(data, base + 0x18)?,
                drift_supplement: be::u8_at(data, base + 0x19)?,
                no_mushroom_zone: be::u8_at(data, base + 0x1a)?,
                padding: read_array(data, base + 0x1b)?,
            });
        }

        let cp_group_base = offsets.checkpoints as usize;
        let mut checkpoint_groups = Vec::with_capacity(cp_group_count);
        let mut cp_counts = Vec::with_capacity(cp_group_count);
        let mut checkpoint_total = 0usize;
        for i in 0..cp_group_count {
            let base = cp_group_base + i * CHECKPOINT_GROUP_SIZE;
            let point_count = be::u16_at(data, base)? as usize;
            checkpoint_total = checkpoint_total
                .checked_add(point_count)
                .ok_or_else(|| error("checkpoint count overflow"))?;
            cp_counts.push(point_count);
            let mut previous = [0i16; 4];
            let mut next = [0i16; 4];
            for j in 0..4 {
                previous[j] = be::i16_at(data, base + 0x04 + j * 2)?;
                next[j] = be::i16_at(data, base + 0x0c + j * 2)?;
            }
            checkpoint_groups.push(CheckpointGroup {
                link: be::u16_at(data, base + 0x02)?,
                previous,
                next,
                points: Vec::new(),
            });
        }
        region(
            data,
            offsets.checkpoints,
            cp_group_count,
            CHECKPOINT_GROUP_SIZE,
            "checkpoint groups",
        )?;
        let checkpoint_start = cp_group_base
            .checked_add(
                cp_group_count
                    .checked_mul(CHECKPOINT_GROUP_SIZE)
                    .ok_or_else(|| error("checkpoint group offset overflow"))?,
            )
            .ok_or_else(|| error("checkpoint offset overflow"))?;
        let checkpoint_bytes = checkpoint_total
            .checked_mul(CHECKPOINT_SIZE)
            .ok_or_else(|| error("checkpoint byte length overflow"))?;
        be::range(data, checkpoint_start, checkpoint_bytes).map_err(|mut e| {
            e.message = format!("checkpoints: {}", e.message);
            e
        })?;
        let mut flat_checkpoint = 0usize;
        for (group, point_count) in checkpoint_groups.iter_mut().zip(cp_counts) {
            group.points.try_reserve_exact(point_count).map_err(|_| {
                error("could not allocate checkpoint records for the declared group size")
            })?;
            for _ in 0..point_count {
                let base = checkpoint_start + flat_checkpoint * CHECKPOINT_SIZE;
                group.points.push(Checkpoint {
                    start: be::vec3_at(data, base)?,
                    end: be::vec3_at(data, base + 0x0c)?,
                    shortcut_id: be::u8_at(data, base + 0x18)?,
                    unknown_19: be::u8_at(data, base + 0x19)?,
                    reverse_search: be::u8_at(data, base + 0x1a)?,
                    unknown_1b: be::u8_at(data, base + 0x1b)?,
                });
                flat_checkpoint += 1;
            }
        }

        let mut paths = Vec::with_capacity(path_count);
        let mut route_point_count = 0usize;
        for i in 0..path_count {
            let base = offsets.paths as usize + i * PATH_SIZE;
            let point_count = be::u16_at(data, base)?;
            let point_start = be::u16_at(data, base + 0x02)?;
            let end = (point_start as usize)
                .checked_add(point_count as usize)
                .ok_or_else(|| FormatError::at(base, "path point range overflow"))?;
            route_point_count = route_point_count.max(end);
            paths.push(Path {
                point_count,
                point_start,
                unknown: be::u16_at(data, base + 0x04)?,
                flags: be::u16_at(data, base + 0x06)?,
                padding: read_array(data, base + 0x08)?,
            });
        }
        region(
            data,
            offsets.route_points,
            route_point_count,
            ROUTE_POINT_SIZE,
            "route points",
        )?;
        let mut route_points = Vec::with_capacity(route_point_count);
        for i in 0..route_point_count {
            let base = offsets.route_points as usize + i * ROUTE_POINT_SIZE;
            route_points.push(RoutePoint {
                position: be::vec3_at(data, base)?,
                link_point: be::i32_at(data, base + 0x0c)?,
                padding: read_array(data, base + 0x10)?,
            });
        }

        let mut objects = Vec::with_capacity(object_count);
        for i in 0..object_count {
            let base = offsets.objects as usize + i * OBJECT_SIZE;
            let mut params = [0i16; 8];
            for (j, param) in params.iter_mut().enumerate() {
                *param = be::i16_at(data, base + 0x30 + j * 2)?;
            }
            objects.push(MapObject {
                position: be::vec3_at(data, base)?,
                scale: be::vec3_at(data, base + 0x0c)?,
                forward: read_i16_vec3(data, base + 0x18)?,
                up: read_i16_vec3(data, base + 0x1e)?,
                id: be::u16_at(data, base + 0x24)?,
                path_id: be::i16_at(data, base + 0x26)?,
                unknown_28: be::u16_at(data, base + 0x28)?,
                point_idx: be::u16_at(data, base + 0x2a)?,
                presence_filter: be::u8_at(data, base + 0x2c)?,
                presence: be::u8_at(data, base + 0x2d)?,
                collision_flag: be::u8_at(data, base + 0x2e)?,
                unknown_2f: be::u8_at(data, base + 0x2f)?,
                params,
            });
        }

        let mut starts = Vec::with_capacity(start_count);
        for i in 0..start_count {
            let base = offsets.starts as usize + i * START_SIZE;
            starts.push(StartPoint {
                position: be::vec3_at(data, base)?,
                scale: be::vec3_at(data, base + 0x0c)?,
                forward: read_i16_vec3(data, base + 0x18)?,
                up: read_i16_vec3(data, base + 0x1e)?,
                pole_position: be::u8_at(data, base + 0x24)?,
                player_id: be::u8_at(data, base + 0x25)?,
                jugem_offset_y: be::i16_at(data, base + 0x26)?,
            });
        }

        let mut areas = Vec::with_capacity(area_count);
        for i in 0..area_count {
            let base = offsets.areas as usize + i * AREA_SIZE;
            areas.push(Area {
                position: be::vec3_at(data, base)?,
                scale: be::vec3_at(data, base + 0x0c)?,
                forward: read_i16_vec3(data, base + 0x18)?,
                up: read_i16_vec3(data, base + 0x1e)?,
                shape: be::u8_at(data, base + 0x24)?,
                area_type: be::u8_at(data, base + 0x25)?,
                camera_id: be::i16_at(data, base + 0x26)?,
                feather: [
                    be::i32_at(data, base + 0x28)?,
                    be::i32_at(data, base + 0x2c)?,
                ],
                echo_scale: be::i16_at(data, base + 0x30)?,
                unknown_32: be::i16_at(data, base + 0x32)?,
                shadow_id: be::i16_at(data, base + 0x34)?,
                light_param_id: be::i16_at(data, base + 0x36)?,
            });
        }

        let mut cameras = Vec::with_capacity(camera_count);
        for i in 0..camera_count {
            let base = offsets.cameras as usize + i * CAMERA_SIZE;
            cameras.push(Camera {
                position: be::vec3_at(data, base)?,
                direction: read_i16_vec3(data, base + 0x0c)?,
                padding_12: read_array(data, base + 0x12)?,
                target_position2: be::vec3_at(data, base + 0x18)?,
                target_position: be::vec3_at(data, base + 0x24)?,
                kind: be::u16_at(data, base + 0x30)?,
                start_fov: be::u16_at(data, base + 0x32)?,
                duration: be::i16_at(data, base + 0x34)?,
                start_camera: be::u16_at(data, base + 0x36)?,
                shimmer_z0: be::i16_at(data, base + 0x38)?,
                shimmer_z1: be::i16_at(data, base + 0x3a)?,
                path_id: be::i16_at(data, base + 0x3c)?,
                path_speed: be::i16_at(data, base + 0x3e)?,
                end_fov: be::u16_at(data, base + 0x40)?,
                next_camera_id: be::i16_at(data, base + 0x42)?,
                name: read_array(data, base + 0x44)?,
            });
        }

        let mut respawns = Vec::with_capacity(respawn_count);
        for i in 0..respawn_count {
            let base = offsets.respawns as usize + i * RESPAWN_SIZE;
            respawns.push(RespawnPoint {
                position: be::vec3_at(data, base)?,
                forward: read_i16_vec3(data, base + 0x0c)?,
                up: read_i16_vec3(data, base + 0x12)?,
                id: be::u16_at(data, base + 0x18)?,
                cl_point_id: be::u16_at(data, base + 0x1a)?,
                camera_id: be::i16_at(data, base + 0x1c)?,
                sector_id: be::i16_at(data, base + 0x1e)?,
            });
        }

        let mut lights = Vec::with_capacity(light_count);
        for i in 0..light_count {
            let base = offsets.lights as usize + i * LIGHT_SIZE;
            lights.push(LightParam {
                color: read_array(data, base)?,
                position: be::vec3_at(data, base + 0x04)?,
                ambient_color: read_array(data, base + 0x10)?,
            });
        }

        let mut mg = Vec::with_capacity(mg_count);
        for i in 0..mg_count {
            let base = offsets.mg as usize + i * MG_SIZE;
            mg.push(MgParam {
                rabbit_win_sec: be::i16_at(data, base)?,
                rabbit_min_sec: be::i16_at(data, base + 0x02)?,
                rabbit_dec_sec: be::i16_at(data, base + 0x04)?,
                padding: be::i16_at(data, base + 0x06)?,
            });
        }

        let counts = BolCounts {
            centerline: cl_count,
            checkpoint_groups: cp_group_count,
            checkpoints: checkpoint_total,
            paths: path_count,
            route_points: route_point_count,
            objects: object_count,
            starts: start_count,
            areas: area_count,
            cameras: camera_count,
            respawns: respawn_count,
            lights: light_count,
            mg: mg_count,
        };
        Ok(Self {
            header,
            counts,
            offsets,
            centerline,
            checkpoint_groups,
            paths,
            route_points,
            objects,
            starts,
            areas,
            cameras,
            respawns,
            lights,
            mg,
            original: data.to_vec(),
            current: data.to_vec(),
            modified: false,
            canonical: false,
        })
    }

    pub fn original_bytes(&self) -> &[u8] {
        &self.original
    }

    pub fn bytes(&self) -> &[u8] {
        &self.current
    }

    pub fn is_modified(&self) -> bool {
        self.modified
    }

    pub fn uses_canonical_layout(&self) -> bool {
        self.canonical
    }

    pub fn export(&self) -> Result<Vec<u8>> {
        Ok(self.current.clone())
    }

    pub fn to_bytes(&self) -> Result<Vec<u8>> {
        self.export()
    }

    fn refresh_counts(&mut self) {
        self.counts = BolCounts {
            centerline: self.centerline.len(),
            checkpoint_groups: self.checkpoint_groups.len(),
            checkpoints: self.checkpoint_groups.iter().map(|g| g.points.len()).sum(),
            paths: self.paths.len(),
            route_points: self.route_points.len(),
            objects: self.objects.len(),
            starts: self.starts.len(),
            areas: self.areas.len(),
            cameras: self.cameras.len(),
            respawns: self.respawns.len(),
            lights: self.lights.len(),
            mg: self.mg.len(),
        };
    }

    pub fn checkpoint_location(&self, mut index: usize) -> Option<(usize, usize)> {
        for (group_index, group) in self.checkpoint_groups.iter().enumerate() {
            if index < group.points.len() {
                return Some((group_index, index));
            }
            index -= group.points.len();
        }
        None
    }

    pub fn checkpoint(&self, index: usize) -> Option<&Checkpoint> {
        let (group, point) = self.checkpoint_location(index)?;
        self.checkpoint_groups[group].points.get(point)
    }

    pub fn route_points_for(&self, path_index: usize) -> Result<&[RoutePoint]> {
        let path = self
            .paths
            .get(path_index)
            .ok_or_else(|| error(format!("path index {path_index} out of bounds")))?;
        let start = path.point_start as usize;
        let end = start
            .checked_add(path.point_count as usize)
            .ok_or_else(|| error("path point range overflow"))?;
        self.route_points.get(start..end).ok_or_else(|| {
            error(format!(
                "path {path_index} references points outside the route table"
            ))
        })
    }

    fn normalize_paths(&mut self) -> Result<()> {
        let old_points = std::mem::take(&mut self.route_points);
        let old_paths = self.paths.clone();
        let mut new_points = Vec::new();
        for (path, old) in self.paths.iter_mut().zip(old_paths) {
            let start = old.point_start as usize;
            let end = start
                .checked_add(old.point_count as usize)
                .ok_or_else(|| error("path point range overflow"))?;
            let source = old_points
                .get(start..end)
                .ok_or_else(|| error("path point range is outside the route table"))?;
            path.point_start = u16::try_from(new_points.len())
                .map_err(|_| error("canonical route table exceeds 65535 points"))?;
            path.point_count = u16::try_from(source.len())
                .map_err(|_| error("path contains more than 65535 points"))?;
            new_points.extend_from_slice(source);
        }
        self.route_points = new_points;
        Ok(())
    }

    fn take_offset(cursor: &mut usize, length: usize, name: &str) -> Result<u32> {
        let start = *cursor;
        *cursor = cursor
            .checked_add(length)
            .ok_or_else(|| error(format!("{name} offset overflow")))?;
        u32::try_from(start)
            .map_err(|_| error(format!("{name} exceeds the BOL 32-bit address space")))
    }

    fn checked_len(len: usize, max: usize, name: &str) -> Result<()> {
        if len > max {
            Err(error(format!("{name} count {len} exceeds {max}")))
        } else {
            Ok(())
        }
    }

    fn rebuild_preserving_route_points(&mut self) -> Result<()> {
        self.rebuild_impl(false)
    }

    fn rebuild(&mut self) -> Result<()> {
        self.rebuild_impl(true)
    }

    fn rebuild_impl(&mut self, normalize_route_points: bool) -> Result<()> {
        if normalize_route_points {
            self.normalize_paths()?;
        }
        self.refresh_counts();
        Self::checked_len(self.centerline.len(), u16::MAX as usize, "centerline")?;
        Self::checked_len(
            self.checkpoint_groups.len(),
            u16::MAX as usize,
            "checkpoint groups",
        )?;
        Self::checked_len(self.paths.len(), u16::MAX as usize, "paths")?;
        Self::checked_len(self.objects.len(), u16::MAX as usize, "objects")?;
        Self::checked_len(self.areas.len(), u16::MAX as usize, "areas")?;
        Self::checked_len(self.cameras.len(), u16::MAX as usize, "cameras")?;
        Self::checked_len(self.respawns.len(), u16::MAX as usize, "respawns")?;
        Self::checked_len(self.starts.len(), u8::MAX as usize, "starting points")?;
        Self::checked_len(self.lights.len(), u8::MAX as usize, "lights")?;
        Self::checked_len(self.mg.len(), u8::MAX as usize, "MG parameters")?;
        for (i, group) in self.checkpoint_groups.iter().enumerate() {
            Self::checked_len(
                group.points.len(),
                u16::MAX as usize,
                &format!("checkpoint group {i}"),
            )?;
        }
        for (i, path) in self.paths.iter().enumerate() {
            Self::checked_len(
                path.point_count as usize,
                u16::MAX as usize,
                &format!("path {i}"),
            )?;
        }

        let mut cursor = HEADER_SIZE;
        let mut offsets = SectionOffsets {
            centerline: Self::take_offset(
                &mut cursor,
                self.centerline.len() * CENTERLINE_SIZE,
                "centerline",
            )?,
            ..SectionOffsets::default()
        };
        let cp_len = self.checkpoint_groups.len() * CHECKPOINT_GROUP_SIZE
            + self
                .checkpoint_groups
                .iter()
                .map(|g| g.points.len())
                .sum::<usize>()
                * CHECKPOINT_SIZE;
        offsets.checkpoints = Self::take_offset(&mut cursor, cp_len, "checkpoints")?;
        offsets.paths = Self::take_offset(&mut cursor, self.paths.len() * PATH_SIZE, "paths")?;
        offsets.route_points = Self::take_offset(
            &mut cursor,
            self.route_points.len() * ROUTE_POINT_SIZE,
            "route points",
        )?;
        offsets.objects =
            Self::take_offset(&mut cursor, self.objects.len() * OBJECT_SIZE, "objects")?;
        offsets.starts = Self::take_offset(
            &mut cursor,
            self.starts.len() * START_SIZE,
            "starting points",
        )?;
        offsets.areas = Self::take_offset(&mut cursor, self.areas.len() * AREA_SIZE, "areas")?;
        offsets.cameras =
            Self::take_offset(&mut cursor, self.cameras.len() * CAMERA_SIZE, "cameras")?;
        offsets.respawns =
            Self::take_offset(&mut cursor, self.respawns.len() * RESPAWN_SIZE, "respawns")?;
        offsets.lights = Self::take_offset(&mut cursor, self.lights.len() * LIGHT_SIZE, "lights")?;
        offsets.mg = Self::take_offset(&mut cursor, self.mg.len() * MG_SIZE, "MG parameters")?;

        let mut output = vec![0u8; cursor];
        write_header(&mut output, &self.header, &self.counts, &offsets)?;
        for (i, record) in self.centerline.iter().enumerate() {
            write_centerline(
                &mut output,
                offsets.centerline as usize + i * CENTERLINE_SIZE,
                record,
            )?;
        }
        let mut cp_base =
            offsets.checkpoints as usize + self.checkpoint_groups.len() * CHECKPOINT_GROUP_SIZE;
        for (i, group) in self.checkpoint_groups.iter().enumerate() {
            write_checkpoint_group(
                &mut output,
                offsets.checkpoints as usize + i * CHECKPOINT_GROUP_SIZE,
                group,
            )?;
            for point in &group.points {
                write_checkpoint(&mut output, cp_base, point)?;
                cp_base += CHECKPOINT_SIZE;
            }
        }
        for (i, path) in self.paths.iter().enumerate() {
            write_path(&mut output, offsets.paths as usize + i * PATH_SIZE, path)?;
        }
        for (i, point) in self.route_points.iter().enumerate() {
            write_route_point(
                &mut output,
                offsets.route_points as usize + i * ROUTE_POINT_SIZE,
                point,
            )?;
        }
        for (i, object) in self.objects.iter().enumerate() {
            write_object(
                &mut output,
                offsets.objects as usize + i * OBJECT_SIZE,
                object,
            )?;
        }
        for (i, start) in self.starts.iter().enumerate() {
            write_start(&mut output, offsets.starts as usize + i * START_SIZE, start)?;
        }
        for (i, area) in self.areas.iter().enumerate() {
            write_area(&mut output, offsets.areas as usize + i * AREA_SIZE, area)?;
        }
        for (i, camera) in self.cameras.iter().enumerate() {
            write_camera(
                &mut output,
                offsets.cameras as usize + i * CAMERA_SIZE,
                camera,
            )?;
        }
        for (i, respawn) in self.respawns.iter().enumerate() {
            write_respawn(
                &mut output,
                offsets.respawns as usize + i * RESPAWN_SIZE,
                respawn,
            )?;
        }
        for (i, light) in self.lights.iter().enumerate() {
            write_light(&mut output, offsets.lights as usize + i * LIGHT_SIZE, light)?;
        }
        for (i, param) in self.mg.iter().enumerate() {
            write_mg(&mut output, offsets.mg as usize + i * MG_SIZE, param)?;
        }
        self.offsets = offsets;
        self.current = output;
        self.modified = true;
        self.canonical = true;
        Ok(())
    }

    fn validate_object_path(&self, index: usize) -> Result<()> {
        let object = self
            .objects
            .get(index)
            .ok_or_else(|| error(format!("object index {index} out of bounds")))?;
        if object.path_id < -1 {
            return Err(error(format!(
                "object {index} has invalid path ID {}",
                object.path_id
            )));
        }
        if object.path_id >= 0 {
            let path = self.paths.get(object.path_id as usize).ok_or_else(|| {
                error(format!(
                    "object {index} references missing path {}",
                    object.path_id
                ))
            })?;
            if object.point_idx != u16::MAX && object.point_idx >= path.point_count {
                return Err(error(format!(
                    "object {index} references point {} outside path {}",
                    object.point_idx, object.path_id
                )));
            }
        }
        Ok(())
    }

    pub fn add_object(&mut self, object: MapObject) -> Result<usize> {
        let mut next = self.clone();
        let index = next.objects.len();
        next.objects.push(object);
        next.validate_object_path(index)?;
        next.rebuild_preserving_route_points()?;
        *self = next;
        Ok(index)
    }

    pub fn duplicate_object(&mut self, index: usize) -> Result<usize> {
        let object = self
            .objects
            .get(index)
            .cloned()
            .ok_or_else(|| error(format!("object index {index} out of bounds")))?;
        let mut next = self.clone();
        let new_index = index + 1;
        next.objects.insert(new_index, object);
        next.rebuild_preserving_route_points()?;
        *self = next;
        Ok(new_index)
    }

    pub fn delete_object(&mut self, index: usize) -> Result<MapObject> {
        if index >= self.objects.len() {
            return Err(error(format!("object index {index} out of bounds")));
        }
        let mut next = self.clone();
        let removed = next.objects.remove(index);
        next.rebuild_preserving_route_points()?;
        *self = next;
        Ok(removed)
    }

    pub fn insert_route_point(
        &mut self,
        path_index: usize,
        point_index: usize,
        point: RoutePoint,
    ) -> Result<usize> {
        let mut next = self.clone();
        next.normalize_paths()?;
        let path = next
            .paths
            .get(path_index)
            .ok_or_else(|| error(format!("path index {path_index} out of bounds")))?;
        if point_index > path.point_count as usize {
            return Err(error(format!(
                "insertion index {point_index} exceeds path length {}",
                path.point_count
            )));
        }
        if path.point_count == u16::MAX {
            return Err(error("path cannot contain more than 65535 points"));
        }
        for object in &mut next.objects {
            if object.path_id == path_index as i16
                && object.point_idx != u16::MAX
                && object.point_idx as usize >= point_index
            {
                object.point_idx = object
                    .point_idx
                    .checked_add(1)
                    .ok_or_else(|| error("object point index overflow"))?;
            }
        }
        let flat_index = path.point_start as usize + point_index;
        next.route_points.insert(flat_index, point);
        next.paths[path_index].point_count += 1;
        for path in next.paths.iter_mut().skip(path_index + 1) {
            path.point_start = path
                .point_start
                .checked_add(1)
                .ok_or_else(|| error("path point offset overflow"))?;
        }
        next.rebuild()?;
        *self = next;
        Ok(flat_index)
    }

    pub fn delete_route_point(
        &mut self,
        path_index: usize,
        point_index: usize,
    ) -> Result<RoutePoint> {
        let mut next = self.clone();
        next.normalize_paths()?;
        let path = next
            .paths
            .get(path_index)
            .ok_or_else(|| error(format!("path index {path_index} out of bounds")))?;
        if point_index >= path.point_count as usize {
            return Err(error(format!(
                "point index {point_index} exceeds path length {}",
                path.point_count
            )));
        }
        for (i, object) in &mut next.objects.iter_mut().enumerate() {
            if object.path_id == path_index as i16 && object.point_idx != u16::MAX {
                if object.point_idx as usize == point_index {
                    return Err(error(format!(
                        "cannot delete route point referenced by object {i}"
                    )));
                }
                if object.point_idx as usize > point_index {
                    object.point_idx -= 1;
                }
            }
        }
        let flat_index = path.point_start as usize + point_index;
        let removed = next.route_points.remove(flat_index);
        next.paths[path_index].point_count -= 1;
        for path in next.paths.iter_mut().skip(path_index + 1) {
            path.point_start -= 1;
        }
        next.rebuild()?;
        *self = next;
        Ok(removed)
    }

    pub fn scene_entities(&self) -> Vec<SceneEntity> {
        let mut entities = Vec::new();
        for (index, object) in self.objects.iter().enumerate() {
            entities.push(SceneEntity {
                kind: "object",
                index,
                position: object.position,
                secondary: None,
                label: format!("Object {index} (0x{:04x})", object.id),
            });
        }
        for (index, point) in self.centerline.iter().enumerate() {
            entities.push(SceneEntity {
                kind: "centerline",
                index,
                position: point.position,
                secondary: None,
                label: format!("Centerline {index}"),
            });
        }
        let mut cp_index = 0;
        for (group_index, group) in self.checkpoint_groups.iter().enumerate() {
            for (local_index, point) in group.points.iter().enumerate() {
                entities.push(SceneEntity {
                    kind: "checkpoint",
                    index: cp_index,
                    position: point.start,
                    secondary: Some(point.end),
                    label: format!(
                        "Checkpoint {cp_index} (group {group_index}, point {local_index})"
                    ),
                });
                cp_index += 1;
            }
        }
        for (index, point) in self.route_points.iter().enumerate() {
            entities.push(SceneEntity {
                kind: "route_point",
                index,
                position: point.position,
                secondary: None,
                label: format!("Route point {index}"),
            });
        }
        for (index, start) in self.starts.iter().enumerate() {
            entities.push(SceneEntity {
                kind: "start",
                index,
                position: start.position,
                secondary: None,
                label: format!("Start {index} (player {})", start.player_id),
            });
        }
        for (index, area) in self.areas.iter().enumerate() {
            entities.push(SceneEntity {
                kind: "area",
                index,
                position: area.position,
                secondary: None,
                label: format!("Area {index} (type {})", area.area_type),
            });
        }
        for (index, camera) in self.cameras.iter().enumerate() {
            entities.push(SceneEntity {
                kind: "camera",
                index,
                position: camera.position,
                secondary: Some(camera.target_position),
                label: format!("Camera {index}"),
            });
        }
        for (index, respawn) in self.respawns.iter().enumerate() {
            entities.push(SceneEntity {
                kind: "respawn",
                index,
                position: respawn.position,
                secondary: None,
                label: format!("Respawn {index} (ID {})", respawn.id),
            });
        }
        entities
    }

    pub fn scene_paths(&self) -> Vec<ScenePath> {
        let mut result = Vec::new();
        if !self.centerline.is_empty() {
            let mut start = 0usize;
            while start < self.centerline.len() {
                let group = self.centerline[start].group;
                let mut end = start + 1;
                while end < self.centerline.len() && self.centerline[end].group == group {
                    end += 1;
                }
                let points = self.centerline[start..end]
                    .iter()
                    .map(|p| p.position)
                    .collect();
                let closed = end - start > 1
                    && self.centerline[start].link >= 0
                    && self.centerline[start].link == self.centerline[end - 1].link;
                result.push(ScenePath {
                    kind: "centerline",
                    index: start,
                    points,
                    closed,
                });
                start = end;
            }
        }
        for (index, path) in self.paths.iter().enumerate() {
            if let Ok(points) = self.route_points_for(index) {
                result.push(ScenePath {
                    kind: "route_point",
                    index,
                    points: points.iter().map(|p| p.position).collect(),
                    closed: path.closed(),
                });
            }
        }
        let mut checkpoint_start = 0usize;
        for (index, group) in self.checkpoint_groups.iter().enumerate() {
            let points = group
                .points
                .iter()
                .map(|p| {
                    [
                        (p.start[0] + p.end[0]) * 0.5,
                        (p.start[1] + p.end[1]) * 0.5,
                        (p.start[2] + p.end[2]) * 0.5,
                    ]
                })
                .collect();
            let closed = group.next[..group.next.iter().position(|v| *v < 0).unwrap_or(4)]
                .contains(&(index as i16));
            result.push(ScenePath {
                kind: "checkpoint",
                index: checkpoint_start,
                points,
                closed,
            });
            checkpoint_start += group.points.len();
        }
        result
    }

    pub fn count(&self, kind: &str) -> Option<usize> {
        match canonical_kind(kind)? {
            "centerline" => Some(self.centerline.len()),
            "checkpoint_group" => Some(self.checkpoint_groups.len()),
            "checkpoint" => Some(self.counts.checkpoints),
            "path" => Some(self.paths.len()),
            "route_point" => Some(self.route_points.len()),
            "object" => Some(self.objects.len()),
            "start" => Some(self.starts.len()),
            "area" => Some(self.areas.len()),
            "camera" => Some(self.cameras.len()),
            "respawn" => Some(self.respawns.len()),
            "light" => Some(self.lights.len()),
            "mg" => Some(self.mg.len()),
            "header" => Some(1),
            _ => None,
        }
    }

    pub fn inspect(&self, kind: &str, index: usize) -> Result<Inspection> {
        let kind = canonical_kind(kind)
            .ok_or_else(|| error(format!("unknown BOL record kind '{kind}'")))?;
        let mut value = match kind {
            "header" if index == 0 => {
                let mut value = to_value(&self.header)?;
                let object = value.as_object_mut().unwrap();
                object.insert("magic".into(), Value::String("0015".into()));
                object.insert("counts".into(), to_value(&self.counts)?);
                object.insert("offsets".into(), to_value(self.offsets)?);
                value
            }
            "centerline" => to_value(
                self.centerline
                    .get(index)
                    .ok_or_else(|| bounds(kind, index))?,
            )?,
            "checkpoint_group" => {
                let group = self
                    .checkpoint_groups
                    .get(index)
                    .ok_or_else(|| bounds(kind, index))?;
                let mut value = to_value(group)?;
                let object = value.as_object_mut().unwrap();
                object.remove("points");
                object.insert("point_count".into(), Value::from(group.points.len()));
                value
            }
            "checkpoint" => to_value(self.checkpoint(index).ok_or_else(|| bounds(kind, index))?)?,
            "path" => to_value(self.paths.get(index).ok_or_else(|| bounds(kind, index))?)?,
            "route_point" => to_value(
                self.route_points
                    .get(index)
                    .ok_or_else(|| bounds(kind, index))?,
            )?,
            "object" => to_value(self.objects.get(index).ok_or_else(|| bounds(kind, index))?)?,
            "start" => to_value(self.starts.get(index).ok_or_else(|| bounds(kind, index))?)?,
            "area" => to_value(self.areas.get(index).ok_or_else(|| bounds(kind, index))?)?,
            "camera" => to_value(self.cameras.get(index).ok_or_else(|| bounds(kind, index))?)?,
            "respawn" => to_value(
                self.respawns
                    .get(index)
                    .ok_or_else(|| bounds(kind, index))?,
            )?,
            "light" => to_value(self.lights.get(index).ok_or_else(|| bounds(kind, index))?)?,
            "mg" => to_value(self.mg.get(index).ok_or_else(|| bounds(kind, index))?)?,
            _ => return Err(bounds(kind, index)),
        };
        let fields = value
            .as_object_mut()
            .unwrap()
            .iter_mut()
            .map(|(key, value)| {
                let key = key.clone();
                let editable = !matches!(
                    key.as_str(),
                    "magic"
                        | "counts"
                        | "offsets"
                        | "point_count"
                        | "point_start"
                        | "points"
                        | "padding"
                        | "padding_12"
                        | "reserved"
                        | "file_start"
                );
                FieldMetadata {
                    value_type: metadata_type(&key, value).to_string(),
                    value: value.take(),
                    key,
                    editable,
                }
            })
            .collect();
        Ok(Inspection {
            kind: kind.to_string(),
            index,
            fields,
        })
    }

    pub fn inspect_json(&self, kind: &str, index: usize) -> Result<String> {
        serde_json::to_string(&self.inspect(kind, index)?)
            .map_err(|e| error(format!("could not serialize inspection: {e}")))
    }

    pub fn set_header_field(&mut self, key: &str, value: Value) -> Result<()> {
        self.set_field("header", 0, key, value)
    }

    /// Edit one serializable field and patch only its fixed-size record.
    /// Vector components can also be addressed as `position.x`, `scale.y`, etc.
    pub fn set_field(&mut self, kind: &str, index: usize, key: &str, value: Value) -> Result<()> {
        let kind = canonical_kind(kind)
            .ok_or_else(|| error(format!("unknown BOL record kind '{kind}'")))?;
        if matches!(
            key,
            "magic"
                | "counts"
                | "offsets"
                | "point_count"
                | "point_start"
                | "points"
                | "file_start"
        ) {
            return Err(error(format!(
                "field '{key}' is structural/read-only; use a structural editing operation"
            )));
        }
        let key = normalize_key(kind, key);
        let mut next = self.clone();
        match kind {
            "header" if index == 0 => {
                let offset = 0;
                edit_serialized(&mut next.header, key, value)?;
                write_header(&mut next.current, &next.header, &next.counts, &next.offsets)?;
                debug_assert_eq!(offset, 0);
            }
            "centerline" => {
                let base = record_base(
                    next.offsets.centerline,
                    index,
                    CENTERLINE_SIZE,
                    next.centerline.len(),
                    kind,
                )?;
                edit_serialized(next.centerline.get_mut(index).unwrap(), key, value)?;
                write_centerline(&mut next.current, base, &next.centerline[index])?;
            }
            "checkpoint_group" => {
                let base = record_base(
                    next.offsets.checkpoints,
                    index,
                    CHECKPOINT_GROUP_SIZE,
                    next.checkpoint_groups.len(),
                    kind,
                )?;
                edit_serialized(next.checkpoint_groups.get_mut(index).unwrap(), key, value)?;
                next.checkpoint_groups[index].points = self.checkpoint_groups[index].points.clone();
                write_checkpoint_group(&mut next.current, base, &next.checkpoint_groups[index])?;
            }
            "checkpoint" => {
                let (group, point) = next
                    .checkpoint_location(index)
                    .ok_or_else(|| bounds(kind, index))?;
                let base = next.offsets.checkpoints as usize
                    + next.checkpoint_groups.len() * CHECKPOINT_GROUP_SIZE
                    + index * CHECKPOINT_SIZE;
                edit_serialized(&mut next.checkpoint_groups[group].points[point], key, value)?;
                write_checkpoint(
                    &mut next.current,
                    base,
                    &next.checkpoint_groups[group].points[point],
                )?;
            }
            "path" => {
                let base =
                    record_base(next.offsets.paths, index, PATH_SIZE, next.paths.len(), kind)?;
                edit_serialized(next.paths.get_mut(index).unwrap(), key, value)?;
                if next.paths[index].point_count != self.paths[index].point_count
                    || next.paths[index].point_start != self.paths[index].point_start
                {
                    return Err(error(
                        "path counts/starts are read-only; use route point insertion/deletion",
                    ));
                }
                write_path(&mut next.current, base, &next.paths[index])?;
            }
            "route_point" => {
                let base = record_base(
                    next.offsets.route_points,
                    index,
                    ROUTE_POINT_SIZE,
                    next.route_points.len(),
                    kind,
                )?;
                edit_serialized(next.route_points.get_mut(index).unwrap(), key, value)?;
                write_route_point(&mut next.current, base, &next.route_points[index])?;
            }
            "object" => {
                let base = record_base(
                    next.offsets.objects,
                    index,
                    OBJECT_SIZE,
                    next.objects.len(),
                    kind,
                )?;
                edit_serialized(next.objects.get_mut(index).unwrap(), key, value)?;
                next.validate_object_path(index)?;
                write_object(&mut next.current, base, &next.objects[index])?;
            }
            "start" => {
                let base = record_base(
                    next.offsets.starts,
                    index,
                    START_SIZE,
                    next.starts.len(),
                    kind,
                )?;
                edit_serialized(next.starts.get_mut(index).unwrap(), key, value)?;
                write_start(&mut next.current, base, &next.starts[index])?;
            }
            "area" => {
                let base =
                    record_base(next.offsets.areas, index, AREA_SIZE, next.areas.len(), kind)?;
                edit_serialized(next.areas.get_mut(index).unwrap(), key, value)?;
                validate_camera_ref(next.areas[index].camera_id, next.cameras.len(), "area")?;
                write_area(&mut next.current, base, &next.areas[index])?;
            }
            "camera" => {
                let base = record_base(
                    next.offsets.cameras,
                    index,
                    CAMERA_SIZE,
                    next.cameras.len(),
                    kind,
                )?;
                edit_serialized(next.cameras.get_mut(index).unwrap(), key, value)?;
                let camera = &next.cameras[index];
                validate_path_ref(camera.path_id, next.paths.len(), "camera")?;
                validate_camera_ref(camera.next_camera_id, next.cameras.len(), "next camera")?;
                write_camera(&mut next.current, base, camera)?;
            }
            "respawn" => {
                let base = record_base(
                    next.offsets.respawns,
                    index,
                    RESPAWN_SIZE,
                    next.respawns.len(),
                    kind,
                )?;
                edit_serialized(next.respawns.get_mut(index).unwrap(), key, value)?;
                validate_camera_ref(
                    next.respawns[index].camera_id,
                    next.cameras.len(),
                    "respawn camera",
                )?;
                write_respawn(&mut next.current, base, &next.respawns[index])?;
            }
            "light" => {
                let base = record_base(
                    next.offsets.lights,
                    index,
                    LIGHT_SIZE,
                    next.lights.len(),
                    kind,
                )?;
                edit_serialized(next.lights.get_mut(index).unwrap(), key, value)?;
                write_light(&mut next.current, base, &next.lights[index])?;
            }
            "mg" => {
                let base = record_base(next.offsets.mg, index, MG_SIZE, next.mg.len(), kind)?;
                edit_serialized(next.mg.get_mut(index).unwrap(), key, value)?;
                write_mg(&mut next.current, base, &next.mg[index])?;
            }
            _ => return Err(bounds(kind, index)),
        }
        next.modified = true;
        *self = next;
        Ok(())
    }

    pub fn set_field_json(
        &mut self,
        kind: &str,
        index: usize,
        key: &str,
        value: &str,
    ) -> Result<()> {
        let value = serde_json::from_str(value)
            .map_err(|e| error(format!("invalid JSON field value: {e}")))?;
        self.set_field(kind, index, key, value)
    }

    pub fn validate(&self) -> ValidationReport {
        let mut report = ValidationReport::default();
        if self.header.lap_count == 0 {
            warn(
                &mut report,
                "zero_laps",
                "lap count is zero".into(),
                Some("header"),
                Some(0),
            );
        }
        if self.header.tilt > 2 {
            warn(
                &mut report,
                "unknown_tilt",
                format!("unknown tilt setting {}", self.header.tilt),
                Some("header"),
                Some(0),
            );
        }
        if self.header.fog_start_z.is_finite()
            && self.header.fog_end_z.is_finite()
            && self.header.fog_end_z < self.header.fog_start_z
        {
            warn(
                &mut report,
                "fog_range",
                "fog end Z precedes fog start Z".into(),
                Some("header"),
                Some(0),
            );
        }
        for (i, point) in self.centerline.iter().enumerate() {
            if point.drift_direction > 2 {
                warn(
                    &mut report,
                    "drift_direction",
                    format!("unexpected drift direction {}", point.drift_direction),
                    Some("centerline"),
                    Some(i),
                );
            }
            if !(-3..=3).contains(&point.swerve) {
                warn(
                    &mut report,
                    "swerve",
                    format!("swerve {} is outside -3..=3", point.swerve),
                    Some("centerline"),
                    Some(i),
                );
            }
            if point.group_setting > 1 {
                warn(
                    &mut report,
                    "group_setting",
                    "group setting is normally 0 or 1".into(),
                    Some("centerline"),
                    Some(i),
                );
            }
            check_finite(&mut report, "centerline", i, "position", point.position);
            check_finite(&mut report, "centerline", i, "scale", [point.scale; 3]);
        }
        for (group_index, group) in self.checkpoint_groups.iter().enumerate() {
            if group.points.is_empty() {
                warn(
                    &mut report,
                    "empty_checkpoint_group",
                    "checkpoint group has no points".into(),
                    Some("checkpoint_group"),
                    Some(group_index),
                );
            }
            for (slot, links) in [("previous", &group.previous), ("next", &group.next)] {
                for (j, link) in links.iter().enumerate() {
                    if *link < -1 || (*link >= 0 && *link as usize >= self.checkpoint_groups.len())
                    {
                        issue(
                            &mut report.errors,
                            "invalid_checkpoint_link",
                            format!("{slot}[{j}] references missing group {link}"),
                            "checkpoint_group",
                            group_index,
                        );
                    }
                }
            }
            for (point_index, point) in group.points.iter().enumerate() {
                let index = self.checkpoint_groups[..group_index]
                    .iter()
                    .map(|g| g.points.len())
                    .sum::<usize>()
                    + point_index;
                check_finite(&mut report, "checkpoint", index, "start", point.start);
                check_finite(&mut report, "checkpoint", index, "end", point.end);
                if point.reverse_search > 1 {
                    warn(
                        &mut report,
                        "reverse_search",
                        "reverse-search flag is normally 0 or 1".into(),
                        Some("checkpoint"),
                        Some(index),
                    );
                }
            }
        }
        let mut covered = vec![false; self.route_points.len()];
        for (i, path) in self.paths.iter().enumerate() {
            if path.point_count == 0 {
                warn(
                    &mut report,
                    "empty_path",
                    "path has no points".into(),
                    Some("path"),
                    Some(i),
                );
            }
            let start = path.point_start as usize;
            let end = start.saturating_add(path.point_count as usize);
            if end > self.route_points.len() {
                issue(
                    &mut report.errors,
                    "path_range",
                    format!(
                        "point range {start}..{end} is outside {} route points",
                        self.route_points.len()
                    ),
                    "path",
                    i,
                );
            } else {
                for used in &mut covered[start..end] {
                    if *used {
                        warn(
                            &mut report,
                            "overlapping_paths",
                            "multiple paths share a flat route point".into(),
                            Some("path"),
                            Some(i),
                        );
                    }
                    *used = true;
                }
            }
        }
        if covered.iter().any(|used| !used) {
            warn(
                &mut report,
                "unreferenced_route_points",
                "route table contains points not referenced by a path".into(),
                Some("route_point"),
                None,
            );
        }
        for (i, object) in self.objects.iter().enumerate() {
            check_finite(&mut report, "object", i, "position", object.position);
            check_finite(&mut report, "object", i, "scale", object.scale);
            if object.path_id < -1
                || (object.path_id >= 0 && object.path_id as usize >= self.paths.len())
            {
                issue(
                    &mut report.errors,
                    "object_path",
                    format!("invalid path ID {}", object.path_id),
                    "object",
                    i,
                );
            } else if object.path_id >= 0 && object.point_idx != u16::MAX {
                let path = &self.paths[object.path_id as usize];
                if object.point_idx >= path.point_count {
                    issue(
                        &mut report.errors,
                        "object_point",
                        format!(
                            "point {} is outside path {}",
                            object.point_idx, object.path_id
                        ),
                        "object",
                        i,
                    );
                }
            }
        }
        for (i, start) in self.starts.iter().enumerate() {
            check_finite(&mut report, "start", i, "position", start.position);
            check_finite(&mut report, "start", i, "scale", start.scale);
            if start.pole_position > 1 {
                warn(
                    &mut report,
                    "pole_position",
                    "pole position is normally 0 or 1".into(),
                    Some("start"),
                    Some(i),
                );
            }
        }
        for (i, area) in self.areas.iter().enumerate() {
            check_finite(&mut report, "area", i, "position", area.position);
            check_finite(&mut report, "area", i, "scale", area.scale);
            if area.shape > 1 {
                warn(
                    &mut report,
                    "area_shape",
                    "area shape is normally 0 (box) or 1 (cylinder)".into(),
                    Some("area"),
                    Some(i),
                );
            }
            if area.camera_id >= 0 && area.camera_id as usize >= self.cameras.len() {
                issue(
                    &mut report.errors,
                    "area_camera",
                    format!("invalid camera ID {}", area.camera_id),
                    "area",
                    i,
                );
            }
            if area.light_param_id >= 0 && area.light_param_id as usize >= self.lights.len() {
                issue(
                    &mut report.errors,
                    "area_light",
                    format!("invalid light parameter ID {}", area.light_param_id),
                    "area",
                    i,
                );
            }
        }
        for (i, camera) in self.cameras.iter().enumerate() {
            check_finite(&mut report, "camera", i, "position", camera.position);
            check_finite(
                &mut report,
                "camera",
                i,
                "target_position",
                camera.target_position,
            );
            check_finite(
                &mut report,
                "camera",
                i,
                "target_position2",
                camera.target_position2,
            );
            if camera.path_id < -1
                || (camera.path_id >= 0 && camera.path_id as usize >= self.paths.len())
            {
                issue(
                    &mut report.errors,
                    "camera_path",
                    format!("invalid path ID {}", camera.path_id),
                    "camera",
                    i,
                );
            }
            if camera.next_camera_id >= 0 && camera.next_camera_id as usize >= self.cameras.len() {
                issue(
                    &mut report.errors,
                    "next_camera",
                    format!("invalid next camera ID {}", camera.next_camera_id),
                    "camera",
                    i,
                );
            }
        }
        for (i, point) in self.route_points.iter().enumerate() {
            check_finite(&mut report, "route_point", i, "position", point.position);
        }
        for (i, point) in self.respawns.iter().enumerate() {
            check_finite(&mut report, "respawn", i, "position", point.position);
            if point.cl_point_id != u16::MAX && point.cl_point_id as usize >= self.centerline.len()
            {
                issue(
                    &mut report.errors,
                    "respawn_centerline",
                    format!("invalid centerline point ID {}", point.cl_point_id),
                    "respawn",
                    i,
                );
            }
            if point.camera_id >= 0 && point.camera_id as usize >= self.cameras.len() {
                issue(
                    &mut report.errors,
                    "respawn_camera",
                    format!("invalid camera ID {}", point.camera_id),
                    "respawn",
                    i,
                );
            }
            if point.sector_id >= 0 && point.sector_id as usize >= self.counts.checkpoints {
                issue(
                    &mut report.errors,
                    "respawn_sector",
                    format!("invalid checkpoint/sector ID {}", point.sector_id),
                    "respawn",
                    i,
                );
            }
        }
        for (i, light) in self.lights.iter().enumerate() {
            check_finite(&mut report, "light", i, "position", light.position);
        }
        check_finite(
            &mut report,
            "header",
            0,
            "light_position",
            self.header.light_position,
        );
        check_finite(
            &mut report,
            "header",
            0,
            "fog_start_z",
            [self.header.fog_start_z; 3],
        );
        check_finite(
            &mut report,
            "header",
            0,
            "fog_end_z",
            [self.header.fog_end_z; 3],
        );
        report
    }
}

fn warn(
    report: &mut ValidationReport,
    code: &str,
    message: String,
    kind: Option<&str>,
    index: Option<usize>,
) {
    report.warnings.push(ValidationIssue {
        code: code.into(),
        message,
        kind: kind.map(str::to_string),
        index,
    });
}

fn issue(errors: &mut Vec<ValidationIssue>, code: &str, message: String, kind: &str, index: usize) {
    errors.push(ValidationIssue {
        code: code.into(),
        message,
        kind: Some(kind.into()),
        index: Some(index),
    });
}

fn check_finite(
    report: &mut ValidationReport,
    kind: &str,
    index: usize,
    name: &str,
    value: [f32; 3],
) {
    if value.iter().any(|v| !v.is_finite()) {
        issue(
            &mut report.errors,
            "non_finite",
            format!("{name} contains a non-finite component"),
            kind,
            index,
        );
    }
}

fn canonical_kind(kind: &str) -> Option<&'static str> {
    Some(match kind {
        "header" => "header",
        "centerline" | "cl" | "cl_point" => "centerline",
        "checkpoint_group" | "check_path" | "gate_group" => "checkpoint_group",
        "checkpoint" | "gate" => "checkpoint",
        "path" | "route" | "route_group" => "path",
        "route_point" | "path_point" => "route_point",
        "object" | "objects" => "object",
        "start" | "start_point" | "starts" => "start",
        "area" => "area",
        "camera" => "camera",
        "respawn" | "jugem" => "respawn",
        "light" | "light_param" => "light",
        "mg" | "mg_param" => "mg",
        _ => return None,
    })
}

fn normalize_key<'a>(kind: &str, key: &'a str) -> &'a str {
    match (kind, key) {
        ("object", "object_id") => "id",
        ("object" | "camera", "route_id") => "path_id",
        ("respawn", "route_point_id") => "cl_point_id",
        ("header", "laps") => "lap_count",
        ("header", "lod_bias") => "tex_lod_bias",
        _ => key,
    }
}

fn bounds(kind: &str, index: usize) -> FormatError {
    error(format!("{kind} index {index} out of bounds"))
}

fn record_base(offset: u32, index: usize, size: usize, len: usize, kind: &str) -> Result<usize> {
    if index >= len {
        return Err(bounds(kind, index));
    }
    let delta = index
        .checked_mul(size)
        .ok_or_else(|| error("record offset overflow"))?;
    (offset as usize)
        .checked_add(delta)
        .ok_or_else(|| error("record offset overflow"))
}

fn to_value<T: Serialize>(value: T) -> Result<Value> {
    serde_json::to_value(value).map_err(|e| error(format!("could not serialize BOL record: {e}")))
}

fn metadata_type(key: &str, value: &Value) -> &'static str {
    match value {
        Value::Null => "non_finite_or_null",
        Value::Bool(_) => "bool",
        Value::String(_) => "string",
        Value::Number(_) => "number",
        Value::Object(_) => "object",
        Value::Array(values) => {
            if matches!(
                key,
                "position"
                    | "scale"
                    | "start"
                    | "end"
                    | "target_position"
                    | "target_position2"
                    | "light_position"
            ) && values.len() == 3
            {
                "f32_vec3"
            } else if matches!(key, "forward" | "up" | "direction") && values.len() == 3 {
                "i16_vec3"
            } else if values.iter().all(Value::is_number) {
                "number_array"
            } else {
                "array"
            }
        }
    }
}

fn edit_serialized<T>(record: &mut T, key: &str, value: Value) -> Result<()>
where
    T: Serialize + for<'de> Deserialize<'de>,
{
    let mut json = to_value(&*record)?;
    let target = json
        .as_object_mut()
        .ok_or_else(|| error("BOL record did not serialize as an object"))?;
    if let Some((field, component)) = key.split_once('.') {
        let component = match component {
            "x" | "r" => 0,
            "y" | "g" => 1,
            "z" | "b" => 2,
            "w" | "a" => 3,
            _ => return Err(error(format!("unknown vector component '{component}'"))),
        };
        ensure_finite_json(&value)?;
        let vector = target
            .get_mut(field)
            .and_then(Value::as_array_mut)
            .ok_or_else(|| error(format!("'{field}' is not an editable vector field")))?;
        let slot = vector
            .get_mut(component)
            .ok_or_else(|| error(format!("component {component} is outside '{field}'")))?;
        if !slot.is_number() && !slot.is_null() {
            return Err(error(format!("'{field}' is not numeric")));
        }
        *slot = value;
    } else {
        if !target.contains_key(key) {
            return Err(error(format!("unknown field '{key}'")));
        }
        ensure_finite_json(&value)?;
        target.insert(key.to_string(), value);
    }
    let edited = serde_json::from_value(json)
        .map_err(|e| error(format!("invalid value for '{key}': {e}")))?;
    *record = edited;
    Ok(())
}

fn ensure_finite_json(value: &Value) -> Result<()> {
    match value {
        Value::Number(number) => {
            if number.as_f64().is_some_and(f64::is_finite) {
                Ok(())
            } else {
                Err(error("floating-point values must be finite"))
            }
        }
        Value::Array(values) => {
            for value in values {
                ensure_finite_json(value)?;
            }
            Ok(())
        }
        Value::Object(values) => {
            for value in values.values() {
                ensure_finite_json(value)?;
            }
            Ok(())
        }
        _ => Ok(()),
    }
}

fn validate_path_ref(id: i16, paths: usize, owner: &str) -> Result<()> {
    if id < -1 || (id >= 0 && id as usize >= paths) {
        Err(error(format!("{owner} references invalid path ID {id}")))
    } else {
        Ok(())
    }
}

fn validate_camera_ref(id: i16, cameras: usize, owner: &str) -> Result<()> {
    if id < -1 || (id >= 0 && id as usize >= cameras) {
        Err(error(format!("{owner} references invalid camera ID {id}")))
    } else {
        Ok(())
    }
}

fn write_header(
    data: &mut [u8],
    header: &BolHeader,
    counts: &BolCounts,
    offsets: &SectionOffsets,
) -> Result<()> {
    put_bytes(data, 0, BOL_MAGIC)?;
    put_u8(data, 0x04, header.tilt)?;
    put_bytes(data, 0x05, &header.ambient_color)?;
    put_bytes(data, 0x08, &header.light_color)?;
    put_vec3_raw(data, 0x0c, header.light_position)?;
    put_u8(data, 0x18, header.lap_count)?;
    put_u8(data, 0x19, header.course_id)?;
    be::put_u16(data, 0x1a, counts.centerline as u16)?;
    be::put_u16(data, 0x1c, counts.checkpoint_groups as u16)?;
    be::put_u16(data, 0x1e, counts.objects as u16)?;
    be::put_u16(data, 0x20, counts.areas as u16)?;
    be::put_u16(data, 0x22, counts.cameras as u16)?;
    be::put_u16(data, 0x24, counts.paths as u16)?;
    be::put_u16(data, 0x26, counts.respawns as u16)?;
    put_u8(data, 0x28, header.fog_type)?;
    put_bytes(data, 0x29, &header.fog_color)?;
    put_f32_raw(data, 0x2c, header.fog_start_z)?;
    put_f32_raw(data, 0x30, header.fog_end_z)?;
    put_u8(data, 0x34, header.tex_lod_bias)?;
    put_u8(data, 0x35, header.dummy_start_line)?;
    put_u8(data, 0x36, header.snow_enabled)?;
    put_u8(data, 0x37, header.shadow_opacity)?;
    put_bytes(data, 0x38, &header.shadow_color)?;
    put_u8(data, 0x3b, counts.starts as u8)?;
    put_u8(data, 0x3c, header.sky_follow)?;
    put_u8(data, 0x3d, counts.lights as u8)?;
    put_u8(data, 0x3e, counts.mg as u8)?;
    put_u8(data, 0x3f, header.unknown_3f)?;
    be::put_u32(data, 0x40, header.file_start)?;
    be::put_u32(data, 0x44, offsets.centerline)?;
    be::put_u32(data, 0x48, offsets.checkpoints)?;
    be::put_u32(data, 0x4c, offsets.paths)?;
    be::put_u32(data, 0x50, offsets.route_points)?;
    be::put_u32(data, 0x54, offsets.objects)?;
    be::put_u32(data, 0x58, offsets.starts)?;
    be::put_u32(data, 0x5c, offsets.areas)?;
    be::put_u32(data, 0x60, offsets.cameras)?;
    be::put_u32(data, 0x64, offsets.respawns)?;
    be::put_u32(data, 0x68, offsets.lights)?;
    be::put_u32(data, 0x6c, offsets.mg)?;
    put_bytes(data, 0x70, &header.reserved)
}

fn write_centerline(data: &mut [u8], base: usize, point: &CenterlinePoint) -> Result<()> {
    put_vec3_raw(data, base, point.position)?;
    be::put_u16(data, base + 0x0c, point.drift_direction)?;
    be::put_i16(data, base + 0x0e, point.link)?;
    put_f32_raw(data, base + 0x10, point.scale)?;
    put_i8_array(data, base + 0x14, point.swerve)?;
    put_u8(data, base + 0x15, point.group_setting)?;
    put_u8(data, base + 0x16, point.group)?;
    put_u8(data, base + 0x17, point.drift_acuteness)?;
    put_u8(data, base + 0x18, point.drift_duration)?;
    put_u8(data, base + 0x19, point.drift_supplement)?;
    put_u8(data, base + 0x1a, point.no_mushroom_zone)?;
    put_bytes(data, base + 0x1b, &point.padding)
}

fn write_checkpoint_group(data: &mut [u8], base: usize, group: &CheckpointGroup) -> Result<()> {
    let count = u16::try_from(group.points.len())
        .map_err(|_| error("checkpoint group exceeds 65535 points"))?;
    be::put_u16(data, base, count)?;
    be::put_u16(data, base + 0x02, group.link)?;
    for i in 0..4 {
        be::put_i16(data, base + 0x04 + i * 2, group.previous[i])?;
        be::put_i16(data, base + 0x0c + i * 2, group.next[i])?;
    }
    Ok(())
}

fn write_checkpoint(data: &mut [u8], base: usize, point: &Checkpoint) -> Result<()> {
    put_vec3_raw(data, base, point.start)?;
    put_vec3_raw(data, base + 0x0c, point.end)?;
    put_u8(data, base + 0x18, point.shortcut_id)?;
    put_u8(data, base + 0x19, point.unknown_19)?;
    put_u8(data, base + 0x1a, point.reverse_search)?;
    put_u8(data, base + 0x1b, point.unknown_1b)
}

fn write_path(data: &mut [u8], base: usize, path: &Path) -> Result<()> {
    be::put_u16(data, base, path.point_count)?;
    be::put_u16(data, base + 0x02, path.point_start)?;
    be::put_u16(data, base + 0x04, path.unknown)?;
    be::put_u16(data, base + 0x06, path.flags)?;
    put_bytes(data, base + 0x08, &path.padding)
}

fn write_route_point(data: &mut [u8], base: usize, point: &RoutePoint) -> Result<()> {
    put_vec3_raw(data, base, point.position)?;
    be::put_i32(data, base + 0x0c, point.link_point)?;
    put_bytes(data, base + 0x10, &point.padding)
}

fn write_object(data: &mut [u8], base: usize, object: &MapObject) -> Result<()> {
    put_vec3_raw(data, base, object.position)?;
    put_vec3_raw(data, base + 0x0c, object.scale)?;
    put_i16_vec3(data, base + 0x18, object.forward)?;
    put_i16_vec3(data, base + 0x1e, object.up)?;
    be::put_u16(data, base + 0x24, object.id)?;
    be::put_i16(data, base + 0x26, object.path_id)?;
    be::put_u16(data, base + 0x28, object.unknown_28)?;
    be::put_u16(data, base + 0x2a, object.point_idx)?;
    put_u8(data, base + 0x2c, object.presence_filter)?;
    put_u8(data, base + 0x2d, object.presence)?;
    put_u8(data, base + 0x2e, object.collision_flag)?;
    put_u8(data, base + 0x2f, object.unknown_2f)?;
    for (i, param) in object.params.into_iter().enumerate() {
        be::put_i16(data, base + 0x30 + i * 2, param)?;
    }
    Ok(())
}

fn write_start(data: &mut [u8], base: usize, start: &StartPoint) -> Result<()> {
    put_vec3_raw(data, base, start.position)?;
    put_vec3_raw(data, base + 0x0c, start.scale)?;
    put_i16_vec3(data, base + 0x18, start.forward)?;
    put_i16_vec3(data, base + 0x1e, start.up)?;
    put_u8(data, base + 0x24, start.pole_position)?;
    put_u8(data, base + 0x25, start.player_id)?;
    be::put_i16(data, base + 0x26, start.jugem_offset_y)
}

fn write_area(data: &mut [u8], base: usize, area: &Area) -> Result<()> {
    put_vec3_raw(data, base, area.position)?;
    put_vec3_raw(data, base + 0x0c, area.scale)?;
    put_i16_vec3(data, base + 0x18, area.forward)?;
    put_i16_vec3(data, base + 0x1e, area.up)?;
    put_u8(data, base + 0x24, area.shape)?;
    put_u8(data, base + 0x25, area.area_type)?;
    be::put_i16(data, base + 0x26, area.camera_id)?;
    be::put_i32(data, base + 0x28, area.feather[0])?;
    be::put_i32(data, base + 0x2c, area.feather[1])?;
    be::put_i16(data, base + 0x30, area.echo_scale)?;
    be::put_i16(data, base + 0x32, area.unknown_32)?;
    be::put_i16(data, base + 0x34, area.shadow_id)?;
    be::put_i16(data, base + 0x36, area.light_param_id)
}

fn write_camera(data: &mut [u8], base: usize, camera: &Camera) -> Result<()> {
    put_vec3_raw(data, base, camera.position)?;
    put_i16_vec3(data, base + 0x0c, camera.direction)?;
    put_bytes(data, base + 0x12, &camera.padding_12)?;
    put_vec3_raw(data, base + 0x18, camera.target_position2)?;
    put_vec3_raw(data, base + 0x24, camera.target_position)?;
    be::put_u16(data, base + 0x30, camera.kind)?;
    be::put_u16(data, base + 0x32, camera.start_fov)?;
    be::put_i16(data, base + 0x34, camera.duration)?;
    be::put_u16(data, base + 0x36, camera.start_camera)?;
    be::put_i16(data, base + 0x38, camera.shimmer_z0)?;
    be::put_i16(data, base + 0x3a, camera.shimmer_z1)?;
    be::put_i16(data, base + 0x3c, camera.path_id)?;
    be::put_i16(data, base + 0x3e, camera.path_speed)?;
    be::put_u16(data, base + 0x40, camera.end_fov)?;
    be::put_i16(data, base + 0x42, camera.next_camera_id)?;
    put_bytes(data, base + 0x44, &camera.name)
}

fn write_respawn(data: &mut [u8], base: usize, point: &RespawnPoint) -> Result<()> {
    put_vec3_raw(data, base, point.position)?;
    put_i16_vec3(data, base + 0x0c, point.forward)?;
    put_i16_vec3(data, base + 0x12, point.up)?;
    be::put_u16(data, base + 0x18, point.id)?;
    be::put_u16(data, base + 0x1a, point.cl_point_id)?;
    be::put_i16(data, base + 0x1c, point.camera_id)?;
    be::put_i16(data, base + 0x1e, point.sector_id)
}

fn write_light(data: &mut [u8], base: usize, light: &LightParam) -> Result<()> {
    put_bytes(data, base, &light.color)?;
    put_vec3_raw(data, base + 0x04, light.position)?;
    put_bytes(data, base + 0x10, &light.ambient_color)
}

fn write_mg(data: &mut [u8], base: usize, param: &MgParam) -> Result<()> {
    be::put_i16(data, base, param.rabbit_win_sec)?;
    be::put_i16(data, base + 0x02, param.rabbit_min_sec)?;
    be::put_i16(data, base + 0x04, param.rabbit_dec_sec)?;
    be::put_i16(data, base + 0x06, param.padding)
}

#[cfg(test)]
mod tests {
    use super::*;

    const CL: u32 = HEADER_SIZE as u32;
    const CP: u32 = CL + CENTERLINE_SIZE as u32 + 8;
    const PATH: u32 = CP + CHECKPOINT_GROUP_SIZE as u32 + CHECKPOINT_SIZE as u32;
    const RP: u32 = PATH + PATH_SIZE as u32;
    const OBJ: u32 = RP + ROUTE_POINT_SIZE as u32;
    const START: u32 = OBJ + OBJECT_SIZE as u32;
    const AREA: u32 = START + START_SIZE as u32;
    const CAMERA: u32 = AREA + AREA_SIZE as u32;
    const RESPAWN: u32 = CAMERA + CAMERA_SIZE as u32;
    const LIGHT: u32 = RESPAWN + RESPAWN_SIZE as u32;
    const MG: u32 = LIGHT + LIGHT_SIZE as u32;
    const END: usize = MG as usize + MG_SIZE;

    fn sample() -> Vec<u8> {
        let offsets = SectionOffsets {
            centerline: CL,
            checkpoints: CP,
            paths: PATH,
            route_points: RP,
            objects: OBJ,
            starts: START,
            areas: AREA,
            cameras: CAMERA,
            respawns: RESPAWN,
            lights: LIGHT,
            mg: MG,
        };
        let counts = BolCounts {
            centerline: 1,
            checkpoint_groups: 1,
            checkpoints: 1,
            paths: 1,
            route_points: 1,
            objects: 1,
            starts: 1,
            areas: 1,
            cameras: 1,
            respawns: 1,
            lights: 1,
            mg: 1,
        };
        let header = BolHeader {
            tilt: 0,
            ambient_color: [0x11, 0x22, 0x33],
            light_color: [0xaa, 0xbb, 0xcc, 0xdd],
            light_position: [1.0, 2.0, 3.0],
            lap_count: 3,
            course_id: 0x21,
            fog_type: 2,
            fog_color: [4, 5, 6],
            fog_start_z: 100.0,
            fog_end_z: 900.0,
            tex_lod_bias: 1,
            dummy_start_line: 0,
            snow_enabled: 0,
            shadow_opacity: 80,
            shadow_color: [7, 8, 9],
            sky_follow: 1,
            unknown_3f: 0xa5,
            file_start: 0,
            reserved: *b"unknown hdr!",
        };
        let mut data = vec![0u8; END + 21];
        data[CL as usize + CENTERLINE_SIZE..CP as usize].copy_from_slice(b"GAPDATA!");
        data[END..].copy_from_slice(b"trailing data is kept");
        write_header(&mut data, &header, &counts, &offsets).unwrap();
        write_centerline(
            &mut data,
            CL as usize,
            &CenterlinePoint {
                position: [10.0, 20.0, 30.0],
                drift_direction: 1,
                link: 0,
                scale: 500.0,
                swerve: -2,
                group_setting: 0,
                group: 0,
                drift_acuteness: 100,
                drift_duration: 60,
                drift_supplement: 2,
                no_mushroom_zone: 1,
                padding: *b"clpad",
            },
        )
        .unwrap();
        write_checkpoint_group(
            &mut data,
            CP as usize,
            &CheckpointGroup {
                link: 0,
                previous: [-1; 4],
                next: [0, -1, -1, -1],
                points: vec![Checkpoint::default_for_test()],
            },
        )
        .unwrap();
        write_checkpoint(
            &mut data,
            CP as usize + CHECKPOINT_GROUP_SIZE,
            &Checkpoint::default_for_test(),
        )
        .unwrap();
        write_path(
            &mut data,
            PATH as usize,
            &Path {
                point_count: 1,
                point_start: 0,
                unknown: 0x55aa,
                flags: 1,
                padding: *b"pathpad!",
            },
        )
        .unwrap();
        write_route_point(
            &mut data,
            RP as usize,
            &RoutePoint {
                position: [40.0, 50.0, 60.0],
                link_point: 0x1234_5678,
                padding: *b"route-point-pad!",
            },
        )
        .unwrap();
        write_object(
            &mut data,
            OBJ as usize,
            &MapObject {
                position: [1.0, 2.0, 3.0],
                id: 0x1234,
                path_id: 0,
                point_idx: 0,
                params: [1, 2, 3, 4, 5, 6, 7, 8],
                ..MapObject::default()
            },
        )
        .unwrap();
        write_start(
            &mut data,
            START as usize,
            &StartPoint {
                position: [3.0, 4.0, 5.0],
                scale: [1.0; 3],
                forward: [10_000, 0, 0],
                up: [0, 10_000, 0],
                pole_position: 1,
                player_id: 0xff,
                jugem_offset_y: -20,
            },
        )
        .unwrap();
        write_area(
            &mut data,
            AREA as usize,
            &Area {
                position: [5.0, 6.0, 7.0],
                scale: [100.0; 3],
                forward: [10_000, 0, 0],
                up: [0, 10_000, 0],
                shape: 1,
                area_type: 1,
                camera_id: 0,
                feather: [100, 200],
                echo_scale: 2,
                unknown_32: -3,
                shadow_id: 4,
                light_param_id: 0,
            },
        )
        .unwrap();
        write_camera(
            &mut data,
            CAMERA as usize,
            &Camera {
                position: [7.0, 8.0, 9.0],
                direction: [10_000, 0, 0],
                padding_12: *b"campad",
                target_position2: [11.0, 12.0, 13.0],
                target_position: [14.0, 15.0, 16.0],
                kind: 2,
                start_fov: 60,
                duration: 300,
                start_camera: 1,
                shimmer_z0: -1,
                shimmer_z1: -2,
                path_id: 0,
                path_speed: 4,
                end_fov: 70,
                next_camera_id: -1,
                name: *b"para",
            },
        )
        .unwrap();
        write_respawn(
            &mut data,
            RESPAWN as usize,
            &RespawnPoint {
                position: [17.0, 18.0, 19.0],
                forward: [10_000, 0, 0],
                up: [0, 10_000, 0],
                id: 0,
                cl_point_id: 0,
                camera_id: 0,
                sector_id: 0,
            },
        )
        .unwrap();
        write_light(
            &mut data,
            LIGHT as usize,
            &LightParam {
                color: [1, 2, 3, 4],
                position: [20.0, 21.0, 22.0],
                ambient_color: [5, 6, 7, 8],
            },
        )
        .unwrap();
        write_mg(
            &mut data,
            MG as usize,
            &MgParam {
                rabbit_win_sec: 60,
                rabbit_min_sec: 20,
                rabbit_dec_sec: 2,
                padding: -99,
            },
        )
        .unwrap();
        data
    }

    impl Checkpoint {
        fn default_for_test() -> Self {
            Self {
                start: [-10.0, 0.0, 0.0],
                end: [10.0, 0.0, 0.0],
                shortcut_id: 3,
                unknown_19: 4,
                reverse_search: 1,
                unknown_1b: 5,
            }
        }
    }

    #[test]
    fn no_op_roundtrip_preserves_every_byte() {
        let original = sample();
        let document = BolDocument::parse(&original).unwrap();
        assert_eq!(document.counts.checkpoints, 1);
        assert_eq!(document.route_points[0].padding, *b"route-point-pad!");
        assert_eq!(document.export().unwrap(), original);
        let cloned = document.clone();
        assert_eq!(cloned.export().unwrap(), original);
    }

    #[test]
    fn field_edit_patches_one_field_and_preserves_layout() {
        let original = sample();
        let mut document = BolDocument::parse(&original).unwrap();
        document
            .set_field("object", 0, "position.x", serde_json::json!(123.5))
            .unwrap();
        let edited = document.export().unwrap();
        assert_eq!(edited.len(), original.len());
        for (offset, (before, after)) in original.iter().zip(&edited).enumerate() {
            if (OBJ as usize..OBJ as usize + 4).contains(&offset) {
                continue;
            }
            assert_eq!(before, after, "unexpected byte change at 0x{offset:x}");
        }
        assert_eq!(be::f32_at(&edited, OBJ as usize).unwrap(), 123.5);
        assert_eq!(
            &edited[CL as usize + CENTERLINE_SIZE..CP as usize],
            b"GAPDATA!"
        );
        assert_eq!(&edited[END..], b"trailing data is kept");
        assert_eq!(document.objects[0].position[0], 123.5);
    }

    #[test]
    fn structural_object_edits_rebuild_offsets_and_counts() {
        let original = sample();
        let mut document = BolDocument::parse(&original).unwrap();
        let duplicate = document.duplicate_object(0).unwrap();
        assert_eq!(duplicate, 1);
        assert_eq!(document.counts.objects, 2);
        assert!(document.uses_canonical_layout());
        let reparsed = BolDocument::parse(&document.export().unwrap()).unwrap();
        assert_eq!(reparsed.objects.len(), 2);
        assert_eq!(reparsed.objects[0], reparsed.objects[1]);
        assert_eq!(reparsed.offsets.starts, OBJ + OBJECT_SIZE as u32 * 2 - 8);

        let removed = document.delete_object(0).unwrap();
        assert_eq!(removed.id, 0x1234);
        assert_eq!(document.counts.objects, 1);
        let reparsed = BolDocument::parse(&document.export().unwrap()).unwrap();
        assert_eq!(reparsed.objects.len(), 1);
        assert_eq!(reparsed.checkpoint_groups[0].points.len(), 1);
        assert_eq!(reparsed.paths[0].point_count, 1);
    }

    #[test]
    fn object_edits_retain_unreferenced_route_points() {
        let mut document = BolDocument::parse(&sample()).unwrap();
        let marker = RoutePoint {
            position: [777.0, 888.0, 999.0],
            link_point: 0x1234,
            ..RoutePoint::default()
        };
        document.route_points.insert(0, marker.clone());
        document.paths[0].point_start = 1;
        document.add_object(MapObject::default()).unwrap();
        assert_eq!(document.route_points.len(), 2);
        assert_eq!(document.route_points[0], marker);
        let reparsed = BolDocument::parse(&document.export().unwrap()).unwrap();
        assert_eq!(reparsed.route_points.len(), 2);
        assert_eq!(reparsed.route_points[0], marker);
    }

    #[test]
    fn route_insertion_and_deletion_update_references() {
        let mut document = BolDocument::parse(&sample()).unwrap();
        document
            .insert_route_point(
                0,
                0,
                RoutePoint {
                    position: [9.0; 3],
                    ..RoutePoint::default()
                },
            )
            .unwrap();
        assert_eq!(document.paths[0].point_count, 2);
        assert_eq!(document.objects[0].point_idx, 1);
        assert_eq!(document.route_points.len(), 2);
        document.delete_route_point(0, 0).unwrap();
        assert_eq!(document.paths[0].point_count, 1);
        assert_eq!(document.objects[0].point_idx, 0);
        assert!(document.delete_route_point(0, 0).is_err());
    }

    #[test]
    fn metadata_scene_and_validation_are_generic() {
        let document = BolDocument::parse(&sample()).unwrap();
        let inspection = document.inspect("object", 0).unwrap();
        assert!(inspection.fields.iter().any(|field| field.key == "path_id"));
        assert_eq!(document.count("objects"), Some(1));
        assert!(
            document
                .scene_entities()
                .iter()
                .any(|entity| entity.kind == "checkpoint" && entity.secondary.is_some())
        );
        assert!(
            document
                .scene_paths()
                .iter()
                .any(|path| path.kind == "route_point" && path.closed)
        );
        assert!(document.validate().errors.is_empty());
        serde_json::to_string(&inspection).unwrap();
    }

    #[test]
    fn malformed_bounds_are_rejected() {
        assert!(BolDocument::parse(&sample()[..HEADER_SIZE - 1]).is_err());
        let mut wrong_magic = sample();
        wrong_magic[0] = b'X';
        assert!(BolDocument::parse(&wrong_magic).is_err());
        let mut bad_offset = sample();
        be::put_u32(&mut bad_offset, 0x54, u32::MAX - 3).unwrap();
        assert!(BolDocument::parse(&bad_offset).is_err());
        let mut bad_checkpoint = sample();
        be::put_u16(&mut bad_checkpoint, CP as usize, u16::MAX).unwrap();
        assert!(BolDocument::parse(&bad_checkpoint).is_err());
        let mut bad_path = sample();
        be::put_u16(&mut bad_path, PATH as usize + 2, u16::MAX).unwrap();
        assert!(BolDocument::parse(&bad_path).is_err());
        let mut missing_checkpoint = sample();
        be::put_u32(
            &mut missing_checkpoint,
            0x48,
            (END - CHECKPOINT_GROUP_SIZE - 4) as u32,
        )
        .unwrap();
        assert!(BolDocument::parse(&missing_checkpoint).is_err());
    }
}
