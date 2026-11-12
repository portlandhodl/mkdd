//! GameCube GX fixed-function material state decoded from J3D MAT3 blocks.
//!
//! These types mirror the hardware state J3D loads for a material: color
//! channels, texture-coordinate generators, texture matrices, TEV stages,
//! alpha test, blending, and depth mode. `crate::gx_shader` turns them into
//! GLSL so course and object models render with their authored combiners.

pub const MAX_TEX_GENS: usize = 8;
pub const MAX_TEX_MATRICES: usize = 10;
pub const MAX_TEXTURES: usize = 8;
pub const MAX_TEV_STAGES: usize = 16;

pub const TEXGEN_MATRIX_IDENTITY: u8 = 60;
pub const TEXGEN_MATRIX_FIRST: u8 = 30;

pub const TG_POS: u8 = 0;
pub const TG_NRM: u8 = 1;
pub const TG_TEX0: u8 = 4;
pub const TG_TEX7: u8 = 11;
pub const TG_COLOR0: u8 = 19;
pub const TG_COLOR1: u8 = 20;

pub const TG_MTX3X4: u8 = 0;
pub const TG_MTX2X4: u8 = 1;
pub const TG_SRTG: u8 = 10;

#[derive(Clone, Copy, Debug, PartialEq)]
pub struct GxColorChannel {
    pub lighting: bool,
    pub material_from_vertex: bool,
    pub ambient_from_vertex: bool,
    pub light_mask: u8,
    pub diffuse_function: u8,
    pub attenuation_function: u8,
}

impl Default for GxColorChannel {
    fn default() -> Self {
        Self {
            lighting: false,
            material_from_vertex: true,
            ambient_from_vertex: false,
            light_mask: 0,
            diffuse_function: 0,
            attenuation_function: 2,
        }
    }
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct GxTexGen {
    pub kind: u8,
    pub source: u8,
    pub matrix: u8,
}

impl Default for GxTexGen {
    fn default() -> Self {
        Self {
            kind: TG_MTX2X4,
            source: TG_TEX0,
            matrix: TEXGEN_MATRIX_IDENTITY,
        }
    }
}

/// Which camera-relative matrix J3D concatenates after the authored texture
/// matrix (`param_0` in `J3DTexMtx::calcTexMtx`).
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum TexMatrixInput {
    /// The authored matrix is used as-is.
    None,
    /// Model matrix; vertices are already in model space, so this is identity.
    Model,
    /// Model-view matrix (camera-projected texture maps).
    View,
    /// Model-view rotation only (environment/sphere maps from normals).
    ViewRotation,
}

#[derive(Clone, Copy, Debug, PartialEq)]
pub struct GxTexMatrixInfo {
    pub projection: u8,
    pub info: u8,
    pub center: [f32; 3],
    pub scale: [f32; 2],
    pub rotation: i16,
    pub translation: [f32; 2],
    pub effect: [[f32; 4]; 4],
}

impl Default for GxTexMatrixInfo {
    fn default() -> Self {
        Self {
            projection: 0,
            info: 0,
            center: [0.5, 0.5, 0.5],
            scale: [1.0, 1.0],
            rotation: 0,
            translation: [0.0, 0.0],
            effect: IDENTITY4,
        }
    }
}

const IDENTITY4: [[f32; 4]; 4] = [
    [1.0, 0.0, 0.0, 0.0],
    [0.0, 1.0, 0.0, 0.0],
    [0.0, 0.0, 1.0, 0.0],
    [0.0, 0.0, 0.0, 1.0],
];

impl GxTexMatrixInfo {
    /// Matrix mode (`mInfo & 0x3f`) as consumed by `J3DTexMtx::calcTexMtx`.
    pub fn mode(&self) -> u8 {
        self.info & 0x3f
    }

    pub fn maya(&self) -> bool {
        (self.info >> 7) & 1 != 0
    }

    pub fn input(&self) -> TexMatrixInput {
        match self.mode() {
            1 | 5 | 6 | 7 | 11 => TexMatrixInput::ViewRotation,
            2 | 8 => TexMatrixInput::Model,
            3 | 9 | 10 => TexMatrixInput::View,
            _ => TexMatrixInput::None,
        }
    }

    /// Rows of the 3x4 matrix J3D builds before concatenating `param_0`.
    pub fn matrix(&self) -> [[f32; 4]; 3] {
        self.matrix_with_srt(self.scale, self.rotation, self.translation)
    }

    pub fn matrix_with_srt(
        &self,
        scale: [f32; 2],
        rotation: i16,
        translation: [f32; 2],
    ) -> [[f32; 4]; 3] {
        const Q: [[f32; 4]; 4] = [
            [0.5, 0.0, 0.5, 0.0],
            [0.0, -0.5, 0.5, 0.0],
            [0.0, 0.0, 1.0, 0.0],
            [0.0, 0.0, 0.0, 1.0],
        ];
        const Q2: [[f32; 4]; 4] = [
            [0.5, 0.0, 0.0, 0.5],
            [0.0, -0.5, 0.0, 0.5],
            [0.0, 0.0, 1.0, 0.0],
            [0.0, 0.0, 0.0, 1.0],
        ];
        let srt =
            |old: bool| texture_srt(scale, rotation, translation, self.center, self.maya(), old);
        let result = match self.mode() {
            8 | 9 | 11 => mul4(mul4(srt(false), Q), self.effect),
            7 => mul4(srt(false), Q),
            10 => mul4(mul4(srt(true), Q2), self.effect),
            6 => mul4(srt(true), Q2),
            2..=5 => mul4(srt(true), self.effect),
            _ => srt(true),
        };
        [result[0], result[1], result[2]]
    }
}

/// `J3DGetTextureMtx[Old]` / `J3DGetTextureMtxMaya[Old]` as a 4x4 row-major
/// matrix. "Old" variants place translation in column 3 (for 2D texture
/// coordinates); the others use column 2 (for projected coordinates).
fn texture_srt(
    scale: [f32; 2],
    rotation: i16,
    translation: [f32; 2],
    center: [f32; 3],
    maya: bool,
    old: bool,
) -> [[f32; 4]; 4] {
    let angle = f32::from(rotation) * std::f32::consts::PI / 32_768.0;
    let (sr, cr) = angle.sin_cos();
    let mut m = IDENTITY4;
    let column = if old { 3 } else { 2 };
    if old {
        m[2][2] = 1.0;
    }
    if maya {
        let tx = translation[0] - 0.5;
        let ty = translation[1] - 0.5;
        m[0][0] = scale[0] * cr;
        m[0][1] = scale[1] * sr;
        m[0][column] = tx * cr - sr * (ty + scale[1]) + 0.5;
        m[1][0] = -scale[0] * sr;
        m[1][1] = scale[1] * cr;
        m[1][column] = -tx * sr - cr * (ty + scale[1]) + 0.5;
    } else {
        let cx = scale[0] * cr;
        let sx = scale[0] * sr;
        let sy = scale[1] * sr;
        let cy = scale[1] * cr;
        m[0][0] = cx;
        m[0][1] = -sx;
        m[0][column] = (-cx * center[0] + sx * center[1]) + center[0] + translation[0];
        m[1][0] = sy;
        m[1][1] = cy;
        m[1][column] = (-sy * center[0] - cy * center[1]) + center[1] + translation[1];
    }
    if !old {
        m[0][3] = 0.0;
        m[1][3] = 0.0;
    }
    m[2] = [0.0, 0.0, 1.0, 0.0];
    m
}

fn mul4(a: [[f32; 4]; 4], b: [[f32; 4]; 4]) -> [[f32; 4]; 4] {
    let mut out = [[0.0; 4]; 4];
    for (row, out_row) in out.iter_mut().enumerate() {
        for (column, value) in out_row.iter_mut().enumerate() {
            *value = (0..4).map(|k| a[row][k] * b[k][column]).sum();
        }
    }
    out
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct GxTevOrder {
    pub tex_coord: u8,
    pub tex_map: u8,
    pub channel: u8,
}

impl Default for GxTevOrder {
    fn default() -> Self {
        Self {
            tex_coord: 0xff,
            tex_map: 0xff,
            channel: 0xff,
        }
    }
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct GxTevStage {
    pub color_in: [u8; 4],
    pub color_op: u8,
    pub color_bias: u8,
    pub color_scale: u8,
    pub color_clamp: bool,
    pub color_reg: u8,
    pub alpha_in: [u8; 4],
    pub alpha_op: u8,
    pub alpha_bias: u8,
    pub alpha_scale: u8,
    pub alpha_clamp: bool,
    pub alpha_reg: u8,
}

impl Default for GxTevStage {
    /// GX_PASSCLR / rasterized color passthrough.
    fn default() -> Self {
        Self {
            color_in: [15, 15, 15, 10],
            color_op: 0,
            color_bias: 0,
            color_scale: 0,
            color_clamp: true,
            color_reg: 0,
            alpha_in: [7, 7, 7, 5],
            alpha_op: 0,
            alpha_bias: 0,
            alpha_scale: 0,
            alpha_clamp: true,
            alpha_reg: 0,
        }
    }
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct GxAlphaTest {
    pub compare0: u8,
    pub reference0: u8,
    pub op: u8,
    pub compare1: u8,
    pub reference1: u8,
}

impl Default for GxAlphaTest {
    fn default() -> Self {
        Self {
            compare0: 7,
            reference0: 0,
            op: 0,
            compare1: 7,
            reference1: 0,
        }
    }
}

impl GxAlphaTest {
    pub fn always_passes(&self) -> bool {
        let pass = |compare: u8, reference: u8| {
            compare == 7 || (compare == 6 && reference == 0) || (compare == 3 && reference == 255)
        };
        match self.op {
            0 => pass(self.compare0, self.reference0) && pass(self.compare1, self.reference1),
            1 => pass(self.compare0, self.reference0) || pass(self.compare1, self.reference1),
            _ => false,
        }
    }
}

pub const BLEND_NONE: u8 = 0;
pub const BLEND_BLEND: u8 = 1;
pub const BLEND_LOGIC: u8 = 2;
pub const BLEND_SUBTRACT: u8 = 3;

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct GxBlend {
    pub mode: u8,
    pub source: u8,
    pub destination: u8,
    pub logic_op: u8,
}

impl Default for GxBlend {
    fn default() -> Self {
        Self {
            mode: BLEND_NONE,
            source: 1,
            destination: 0,
            logic_op: 3,
        }
    }
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct GxZMode {
    pub test: bool,
    pub function: u8,
    pub write: bool,
}

impl Default for GxZMode {
    fn default() -> Self {
        Self {
            test: true,
            function: 3,
            write: true,
        }
    }
}

#[derive(Clone, Copy, Debug, PartialEq)]
pub struct GxMaterial {
    pub mode: u8,
    pub material_colors: [[f32; 4]; 2],
    pub ambient_colors: [[f32; 4]; 2],
    pub channel_count: u8,
    /// COLOR0, ALPHA0, COLOR1, ALPHA1.
    pub channels: [GxColorChannel; 4],
    pub tex_gen_count: u8,
    pub tex_gens: [GxTexGen; MAX_TEX_GENS],
    pub tex_matrices: [Option<GxTexMatrixInfo>; MAX_TEX_MATRICES],
    /// Indices into `BmdModel::textures`, per GX texture map.
    pub textures: [Option<usize>; MAX_TEXTURES],
    pub stage_count: u8,
    pub orders: [GxTevOrder; MAX_TEV_STAGES],
    pub stages: [GxTevStage; MAX_TEV_STAGES],
    /// TEV registers in `GXTevRegID` order: PREV, REG0, REG1, REG2.
    pub registers: [[f32; 4]; 4],
    pub konst_colors: [[f32; 4]; 4],
    pub konst_color_select: [u8; MAX_TEV_STAGES],
    pub konst_alpha_select: [u8; MAX_TEV_STAGES],
    /// `(rasterized swap table, texture swap table)` per stage.
    pub swap_modes: [(u8, u8); MAX_TEV_STAGES],
    pub swap_tables: [[u8; 4]; 4],
    pub alpha_test: GxAlphaTest,
    pub blend: GxBlend,
    pub z_mode: GxZMode,
    pub uses_indirect: bool,
}

impl Default for GxMaterial {
    fn default() -> Self {
        Self {
            mode: 1,
            material_colors: [[1.0; 4]; 2],
            ambient_colors: [[0.0, 0.0, 0.0, 1.0]; 2],
            channel_count: 1,
            channels: [GxColorChannel::default(); 4],
            tex_gen_count: 0,
            tex_gens: [GxTexGen::default(); MAX_TEX_GENS],
            tex_matrices: [None; MAX_TEX_MATRICES],
            textures: [None; MAX_TEXTURES],
            stage_count: 1,
            orders: [GxTevOrder {
                tex_coord: 0xff,
                tex_map: 0xff,
                channel: 4,
            }; MAX_TEV_STAGES],
            stages: [GxTevStage::default(); MAX_TEV_STAGES],
            registers: [[1.0; 4]; 4],
            konst_colors: [[1.0; 4]; 4],
            konst_color_select: [0x0c; MAX_TEV_STAGES],
            konst_alpha_select: [0x1c; MAX_TEV_STAGES],
            swap_modes: [(0, 0); MAX_TEV_STAGES],
            swap_tables: [[0, 1, 2, 3]; 4],
            alpha_test: GxAlphaTest::default(),
            blend: GxBlend::default(),
            z_mode: GxZMode::default(),
            uses_indirect: false,
        }
    }
}

impl GxMaterial {
    /// Highest vertex texture-coordinate set consumed by the texgens, plus one.
    pub fn uv_sets_used(&self) -> usize {
        self.tex_gens[..usize::from(self.tex_gen_count).min(MAX_TEX_GENS)]
            .iter()
            .filter(|texgen| (TG_TEX0..=TG_TEX7).contains(&texgen.source))
            .map(|texgen| usize::from(texgen.source - TG_TEX0) + 1)
            .max()
            .unwrap_or(0)
    }

    pub fn uses_color1(&self) -> bool {
        self.channel_count > 1
    }

    pub fn is_translucent(&self) -> bool {
        self.mode == 4
            || matches!(self.blend.mode, BLEND_BLEND | BLEND_SUBTRACT)
            || !self.z_mode.write
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn apply(m: [[f32; 4]; 3], s: f32, t: f32) -> [f32; 2] {
        let v = [s, t, 1.0, 1.0];
        let row = |r: [f32; 4]| r.iter().zip(v).map(|(a, b)| a * b).sum::<f32>();
        [row(m[0]), row(m[1])]
    }

    #[test]
    fn default_srt_matrix_scales_and_translates_uvs() {
        let info = GxTexMatrixInfo {
            scale: [4.0, 2.0],
            translation: [0.25, 0.5],
            center: [0.0; 3],
            ..GxTexMatrixInfo::default()
        };
        assert_eq!(info.input(), TexMatrixInput::None);
        let [s, t] = apply(info.matrix(), 0.5, 0.5);
        assert!((s - 2.25).abs() < 1e-5 && (t - 1.5).abs() < 1e-5, "{s} {t}");
    }

    #[test]
    fn quarter_rotation_about_center_swaps_axes() {
        let info = GxTexMatrixInfo {
            rotation: 16_384,
            ..GxTexMatrixInfo::default()
        };
        let [s, t] = apply(info.matrix(), 1.0, 0.5);
        assert!((s - 0.5).abs() < 1e-5 && (t - 1.0).abs() < 1e-5, "{s} {t}");
    }

    #[test]
    fn environment_modes_request_camera_matrices() {
        let env = GxTexMatrixInfo {
            info: 6,
            ..GxTexMatrixInfo::default()
        };
        assert_eq!(env.input(), TexMatrixInput::ViewRotation);
        let matrix = env.matrix();
        // qMtx2 maps a view-space normal of (0, 0, 1) to the sphere-map center.
        let [s, t] = apply(matrix, 0.0, 0.0);
        assert!((s - 0.5).abs() < 1e-5 && (t - 0.5).abs() < 1e-5, "{s} {t}");
    }

    #[test]
    fn alpha_test_detects_trivial_pass() {
        assert!(GxAlphaTest::default().always_passes());
        let cutout = GxAlphaTest {
            compare0: 6,
            reference0: 128,
            op: 0,
            compare1: 7,
            reference1: 0,
        };
        assert!(!cutout.always_passes());
    }
}
