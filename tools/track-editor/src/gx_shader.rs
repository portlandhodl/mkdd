//! Generates WebGL2 GLSL for a GX material: per-vertex color channels and
//! texture-coordinate generation, then the authored TEV combiner stages and
//! alpha test per fragment.
//!
//! Everything structural (inputs, swizzles, compare functions) is baked into
//! the source so identical materials share one program. Colors and texture
//! matrices stay uniforms so they can be animated later.

use std::fmt::Write as _;

use crate::formats::gx::{
    GxColorChannel, GxMaterial, GxTevStage, MAX_TEX_GENS, TEXGEN_MATRIX_FIRST,
    TEXGEN_MATRIX_IDENTITY, TG_COLOR0, TG_COLOR1, TG_MTX3X4, TG_NRM, TG_POS, TG_SRTG, TG_TEX0,
    TG_TEX7, TexMatrixInput,
};

/// Vertex attribute locations shared with the renderer.
pub const ATTR_POSITION: u32 = 0;
pub const ATTR_NORMAL: u32 = 1;
pub const ATTR_COLOR0: u32 = 2;
pub const ATTR_COLOR1: u32 = 3;
pub const ATTR_TEX0: u32 = 4;

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct GxProgramSource {
    pub vertex: String,
    pub fragment: String,
}

pub fn generate(material: &GxMaterial) -> GxProgramSource {
    GxProgramSource {
        vertex: vertex_shader(material),
        fragment: fragment_shader(material),
    }
}

fn tex_gen_count(material: &GxMaterial) -> usize {
    usize::from(material.tex_gen_count).min(MAX_TEX_GENS)
}

fn stage_count(material: &GxMaterial) -> usize {
    usize::from(material.stage_count).clamp(1, material.stages.len())
}

fn vertex_shader(material: &GxMaterial) -> String {
    let mut s = String::with_capacity(4096);
    s.push_str(
        "#version 300 es\nprecision highp float;\n\
         layout(location = 0) in vec3 a_position;\n\
         layout(location = 1) in vec3 a_normal;\n\
         layout(location = 2) in vec4 a_color0;\n\
         layout(location = 3) in vec4 a_color1;\n",
    );
    for set in 0..8 {
        let _ = writeln!(
            s,
            "layout(location = {}) in vec2 a_tex{set};",
            ATTR_TEX0 + set
        );
    }
    s.push_str(
        "uniform mat4 u_view_projection;\nuniform mat4 u_model;\nuniform mat4 u_view;\n\
         uniform vec4 u_mat_color[2];\nuniform vec4 u_amb_color[2];\n\
         uniform vec3 u_light_direction;\nuniform vec4 u_light_color;\n\
         uniform vec4 u_tex_mtx[30];\n\
         out vec4 v_color0;\nout vec4 v_color1;\nout vec3 v_world_normal;\nout float v_depth;\n",
    );
    for texgen in 0..tex_gen_count(material) {
        let _ = writeln!(s, "out vec3 v_tex_coord{texgen};");
    }
    s.push_str(
        "vec3 tex_matrix(int base, vec4 p) {\n\
         \x20   return vec3(dot(u_tex_mtx[base], p), dot(u_tex_mtx[base + 1], p), dot(u_tex_mtx[base + 2], p));\n\
         }\n\
         void main() {\n\
         \x20   vec4 world = u_model * vec4(a_position, 1.0);\n\
         \x20   gl_Position = u_view_projection * world;\n\
         \x20   v_depth = 1.0 + gl_Position.w;\n\
         \x20   vec3 normal = mat3(u_model) * a_normal;\n\
         \x20   float normal_length = dot(normal, normal);\n\
         \x20   normal = normal_length > 1e-12 ? normal * inversesqrt(normal_length) : vec3(0.0, 1.0, 0.0);\n\
         \x20   v_world_normal = normal;\n\
         \x20   float lambert = dot(normal, -u_light_direction);\n",
    );
    for (output, (color, alpha)) in [(0usize, (0usize, 1usize)), (1, (2, 3))] {
        let _ = writeln!(
            s,
            "    v_color{output} = vec4({}.rgb, {}.a);",
            channel_expression(&material.channels[color], output),
            channel_expression(&material.channels[alpha], output),
        );
    }
    for texgen_index in 0..tex_gen_count(material) {
        let texgen = material.tex_gens[texgen_index];
        let source = match texgen.source {
            TG_POS => "vec4(a_position, 1.0)".to_owned(),
            TG_NRM => "vec4(a_normal, 1.0)".to_owned(),
            TG_TEX0..=TG_TEX7 => format!("vec4(a_tex{}, 1.0, 1.0)", texgen.source - TG_TEX0),
            TG_COLOR0 => "vec4(v_color0.rg, 1.0, 1.0)".to_owned(),
            TG_COLOR1 => "vec4(v_color1.rg, 1.0, 1.0)".to_owned(),
            // Binormal/tangent and bump-map sources have no NBT data here.
            _ => "vec4(a_tex0, 1.0, 1.0)".to_owned(),
        };
        let mut expression = format!("{source}.xyz");
        if texgen.kind != TG_SRTG
            && texgen.matrix != TEXGEN_MATRIX_IDENTITY
            && let Some(slot) = texgen
                .matrix
                .checked_sub(TEXGEN_MATRIX_FIRST)
                .filter(|offset| offset % 3 == 0)
                .map(|offset| usize::from(offset / 3))
                .filter(|slot| *slot < material.tex_matrices.len())
        {
            let input =
                material.tex_matrices[slot].map_or(TexMatrixInput::None, |info| info.input());
            let transformed = match input {
                TexMatrixInput::None => source,
                TexMatrixInput::Model => format!("(u_model * {source})"),
                TexMatrixInput::View => format!("(u_view * u_model * {source})"),
                TexMatrixInput::ViewRotation => {
                    format!("vec4(mat3(u_view) * mat3(u_model) * {source}.xyz, 1.0)")
                }
            };
            expression = format!("tex_matrix({}, {transformed})", slot * 3);
        }
        if texgen.kind == TG_MTX3X4 {
            let _ = writeln!(s, "    v_tex_coord{texgen_index} = {expression};");
        } else {
            let _ = writeln!(
                s,
                "    v_tex_coord{texgen_index} = vec3(({expression}).xy, 1.0);"
            );
        }
    }
    s.push_str("}\n");
    s
}

/// GX lighting: `material * clamp(ambient + diffuse * light)` when enabled.
fn channel_expression(channel: &GxColorChannel, output: usize) -> String {
    let material = if channel.material_from_vertex {
        format!("a_color{output}")
    } else {
        format!("u_mat_color[{output}]")
    };
    if !channel.lighting {
        return material;
    }
    let ambient = if channel.ambient_from_vertex {
        format!("a_color{output}")
    } else {
        format!("u_amb_color[{output}]")
    };
    let diffuse = match channel.diffuse_function {
        0 => "1.0",
        1 => "lambert",
        _ => "max(lambert, 0.0)",
    };
    if channel.light_mask == 0 {
        format!("({material} * clamp({ambient}, 0.0, 1.0))")
    } else {
        format!("({material} * clamp({ambient} + {diffuse} * u_light_color, 0.0, 1.0))")
    }
}

fn fragment_shader(material: &GxMaterial) -> String {
    let stages = stage_count(material);
    let mut s = String::with_capacity(8192);
    s.push_str(
        "#version 300 es\nprecision highp float;\nprecision highp sampler2D;\n\
         in vec4 v_color0;\nin vec4 v_color1;\nin vec3 v_world_normal;\nin float v_depth;\n",
    );
    for texgen in 0..tex_gen_count(material) {
        let _ = writeln!(s, "in vec3 v_tex_coord{texgen};");
    }
    let mut maps = [false; 8];
    for order in &material.orders[..stages] {
        if usize::from(order.tex_map) < maps.len() {
            maps[usize::from(order.tex_map)] = true;
        }
    }
    for (map, used) in maps.iter().enumerate() {
        if *used {
            let _ = writeln!(s, "uniform sampler2D u_texture{map};");
        }
    }
    s.push_str(
        "uniform vec4 u_registers[4];\nuniform vec4 u_konst[4];\nuniform vec4 u_highlight;\n\
         uniform float u_log_depth;\n\
         out vec4 out_color;\n\
         vec2 project(vec3 coord) { return abs(coord.z) > 1e-6 ? coord.xy / coord.z : coord.xy; }\n\
         float pack16(vec3 v) { vec3 b = floor(v * 255.0 + 0.5); return b.r + b.g * 256.0; }\n\
         float pack24(vec3 v) { vec3 b = floor(v * 255.0 + 0.5); return b.r + b.g * 256.0 + b.b * 65536.0; }\n\
         void main() {\n\
         \x20   gl_FragDepth = log2(max(v_depth, 1e-6)) * u_log_depth;\n\
         \x20   vec4 r_prev = u_registers[0];\n\
         \x20   vec4 r_c0 = u_registers[1];\n\
         \x20   vec4 r_c1 = u_registers[2];\n\
         \x20   vec4 r_c2 = u_registers[3];\n\
         \x20   vec4 tex = vec4(1.0);\n\
         \x20   vec4 ras = vec4(0.0);\n\
         \x20   vec4 konst = vec4(1.0);\n",
    );
    let mut last = (0u8, 0u8);
    for stage_index in 0..stages {
        let order = material.orders[stage_index];
        let stage = material.stages[stage_index];
        let (ras_swap, tex_swap) = material.swap_modes[stage_index];
        let _ = writeln!(s, "    // stage {stage_index}");
        if usize::from(order.tex_map) < 8 {
            let coord = if usize::from(order.tex_coord) < tex_gen_count(material) {
                format!("project(v_tex_coord{})", order.tex_coord)
            } else {
                "vec2(0.0)".to_owned()
            };
            let _ = writeln!(
                s,
                "    tex = texture(u_texture{}, {coord}).{};",
                order.tex_map,
                swizzle(material.swap_tables[usize::from(tex_swap & 3)])
            );
        } else {
            s.push_str("    tex = vec4(1.0);\n");
        }
        let raster = match order.channel {
            0 | 4 => "v_color0",
            1 | 5 => "v_color1",
            2 => "vec4(v_color0.aaa, v_color0.a)",
            3 => "vec4(v_color1.aaa, v_color1.a)",
            _ => "vec4(0.0)",
        };
        let _ = writeln!(
            s,
            "    ras = ({raster}).{};",
            swizzle(material.swap_tables[usize::from(ras_swap & 3)])
        );
        let _ = writeln!(
            s,
            "    konst = vec4({}, {});",
            konst_color(material.konst_color_select[stage_index]),
            konst_alpha(material.konst_alpha_select[stage_index])
        );
        write_color_stage(&mut s, &stage);
        write_alpha_stage(&mut s, &stage);
        last = (stage.color_reg, stage.alpha_reg);
    }
    let _ = writeln!(
        s,
        "    vec4 result = clamp(vec4({}.rgb, {}.a), 0.0, 1.0);",
        register(last.0),
        register(last.1)
    );
    let test = material.alpha_test;
    if !test.always_passes() {
        let a = alpha_compare(test.compare0, test.reference0);
        let b = alpha_compare(test.compare1, test.reference1);
        let combined = match test.op {
            0 => format!("({a}) && ({b})"),
            1 => format!("({a}) || ({b})"),
            2 => format!("({a}) != ({b})"),
            _ => format!("({a}) == ({b})"),
        };
        let _ = writeln!(s, "    float alpha = floor(result.a * 255.0 + 0.5);");
        let _ = writeln!(s, "    if (!({combined})) {{ discard; }}");
    }
    s.push_str(
        "    result.rgb = mix(result.rgb, u_highlight.rgb, u_highlight.a);\n\
         \x20   out_color = result;\n}\n",
    );
    s
}

fn swizzle(table: [u8; 4]) -> String {
    table
        .iter()
        .map(|component| ['r', 'g', 'b', 'a'][usize::from(component & 3)])
        .collect()
}

fn register(id: u8) -> &'static str {
    match id & 3 {
        0 => "r_prev",
        1 => "r_c0",
        2 => "r_c1",
        _ => "r_c2",
    }
}

fn konst_fraction(select: u8) -> Option<&'static str> {
    Some(match select {
        0 => "1.0",
        1 => "0.875",
        2 => "0.75",
        3 => "0.625",
        4 => "0.5",
        5 => "0.375",
        6 => "0.25",
        7 => "0.125",
        _ => return None,
    })
}

fn konst_color(select: u8) -> String {
    if let Some(fraction) = konst_fraction(select) {
        return format!("vec3({fraction})");
    }
    let slot = select & 3;
    match select {
        0x0c..=0x0f => format!("u_konst[{slot}].rgb"),
        0x10..=0x13 => format!("vec3(u_konst[{slot}].r)"),
        0x14..=0x17 => format!("vec3(u_konst[{slot}].g)"),
        0x18..=0x1b => format!("vec3(u_konst[{slot}].b)"),
        0x1c..=0x1f => format!("vec3(u_konst[{slot}].a)"),
        _ => "vec3(1.0)".to_owned(),
    }
}

fn konst_alpha(select: u8) -> String {
    if let Some(fraction) = konst_fraction(select) {
        return fraction.to_owned();
    }
    let slot = select & 3;
    match select {
        0x10..=0x13 => format!("u_konst[{slot}].r"),
        0x14..=0x17 => format!("u_konst[{slot}].g"),
        0x18..=0x1b => format!("u_konst[{slot}].b"),
        0x1c..=0x1f => format!("u_konst[{slot}].a"),
        _ => "1.0".to_owned(),
    }
}

fn color_input(input: u8) -> &'static str {
    match input {
        0 => "r_prev.rgb",
        1 => "r_prev.aaa",
        2 => "r_c0.rgb",
        3 => "r_c0.aaa",
        4 => "r_c1.rgb",
        5 => "r_c1.aaa",
        6 => "r_c2.rgb",
        7 => "r_c2.aaa",
        8 => "tex.rgb",
        9 => "tex.aaa",
        10 => "ras.rgb",
        11 => "ras.aaa",
        12 => "vec3(1.0)",
        13 => "vec3(0.5)",
        14 => "konst.rgb",
        _ => "vec3(0.0)",
    }
}

fn alpha_input(input: u8) -> &'static str {
    match input {
        0 => "r_prev.a",
        1 => "r_c0.a",
        2 => "r_c1.a",
        3 => "r_c2.a",
        4 => "tex.a",
        5 => "ras.a",
        6 => "konst.a",
        _ => "0.0",
    }
}

fn bias(value: u8) -> &'static str {
    match value {
        1 => " + 0.5",
        2 => " - 0.5",
        _ => "",
    }
}

fn scale(value: u8) -> &'static str {
    match value & 3 {
        1 => " * 2.0",
        2 => " * 4.0",
        3 => " * 0.5",
        _ => "",
    }
}

fn clamp_range(clamp: bool) -> &'static str {
    if clamp {
        "0.0, 1.0"
    } else {
        "-4.0156863, 4.0117647"
    }
}

fn write_color_stage(s: &mut String, stage: &GxTevStage) {
    let [a, b, c, d] = stage.color_in.map(color_input);
    let _ = writeln!(
        s,
        "    {{ vec3 a = clamp({a}, 0.0, 1.0); vec3 b = clamp({b}, 0.0, 1.0); vec3 c = clamp({c}, 0.0, 1.0); vec3 d = {d};"
    );
    let value = match stage.color_op {
        0 => format!(
            "(d + mix(a, b, c){}){}",
            bias(stage.color_bias),
            scale(stage.color_scale)
        ),
        1 => format!(
            "(d - mix(a, b, c){}){}",
            bias(stage.color_bias),
            scale(stage.color_scale)
        ),
        8 => "d + (a.r > b.r + 0.001 ? c : vec3(0.0))".to_owned(),
        9 => "d + (abs(a.r - b.r) < 0.002 ? c : vec3(0.0))".to_owned(),
        10 => "d + (pack16(a) > pack16(b) ? c : vec3(0.0))".to_owned(),
        11 => "d + (pack16(a) == pack16(b) ? c : vec3(0.0))".to_owned(),
        12 => "d + (pack24(a) > pack24(b) ? c : vec3(0.0))".to_owned(),
        13 => "d + (pack24(a) == pack24(b) ? c : vec3(0.0))".to_owned(),
        14 => "d + mix(vec3(0.0), c, vec3(greaterThan(a, b + 0.001)))".to_owned(),
        15 => "d + mix(vec3(0.0), c, vec3(lessThan(abs(a - b), vec3(0.002))))".to_owned(),
        _ => "d".to_owned(),
    };
    let _ = writeln!(
        s,
        "      {}.rgb = clamp({value}, {}); }}",
        register(stage.color_reg),
        clamp_range(stage.color_clamp)
    );
}

fn write_alpha_stage(s: &mut String, stage: &GxTevStage) {
    let [a, b, c, d] = stage.alpha_in.map(alpha_input);
    let _ = writeln!(
        s,
        "    {{ float a = clamp({a}, 0.0, 1.0); float b = clamp({b}, 0.0, 1.0); float c = clamp({c}, 0.0, 1.0); float d = {d};"
    );
    let value = match stage.alpha_op {
        0 => format!(
            "(d + mix(a, b, c){}){}",
            bias(stage.alpha_bias),
            scale(stage.alpha_scale)
        ),
        1 => format!(
            "(d - mix(a, b, c){}){}",
            bias(stage.alpha_bias),
            scale(stage.alpha_scale)
        ),
        8 | 10 | 12 | 14 => "d + (a > b + 0.001 ? c : 0.0)".to_owned(),
        9 | 11 | 13 | 15 => "d + (abs(a - b) < 0.002 ? c : 0.0)".to_owned(),
        _ => "d".to_owned(),
    };
    let _ = writeln!(
        s,
        "      {}.a = clamp({value}, {}); }}",
        register(stage.alpha_reg),
        clamp_range(stage.alpha_clamp)
    );
}

fn alpha_compare(function: u8, reference: u8) -> String {
    let reference = f32::from(reference);
    match function & 7 {
        0 => "false".to_owned(),
        1 => format!("alpha < {reference:.1}"),
        2 => format!("alpha == {reference:.1}"),
        3 => format!("alpha <= {reference:.1}"),
        4 => format!("alpha > {reference:.1}"),
        5 => format!("alpha != {reference:.1}"),
        6 => format!("alpha >= {reference:.1}"),
        _ => "true".to_owned(),
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::formats::gx::{GxAlphaTest, GxTevOrder, GxTexGen, GxTexMatrixInfo};

    fn textured() -> GxMaterial {
        let mut material = GxMaterial {
            tex_gen_count: 1,
            ..GxMaterial::default()
        };
        material.orders[0] = GxTevOrder {
            tex_coord: 0,
            tex_map: 0,
            channel: 4,
        };
        // Modulate texture by rasterized color.
        material.stages[0].color_in = [15, 8, 10, 15];
        material.stages[0].alpha_in = [7, 4, 5, 7];
        material
    }

    #[test]
    fn default_material_passes_raster_color() {
        let source = generate(&GxMaterial::default());
        assert!(source.fragment.contains("ras = (v_color0).rgba;"));
        assert!(
            source
                .fragment
                .contains("r_prev.rgb = clamp((d + mix(a, b, c)), 0.0, 1.0)")
        );
        assert!(!source.fragment.contains("u_texture"));
        assert!(!source.fragment.contains("discard"));
    }

    #[test]
    fn textured_stage_samples_its_texgen_and_map() {
        let source = generate(&textured());
        assert!(
            source
                .vertex
                .contains("v_tex_coord0 = vec3((vec4(a_tex0, 1.0, 1.0).xyz).xy, 1.0);")
        );
        assert!(source.fragment.contains("uniform sampler2D u_texture0;"));
        assert!(
            source
                .fragment
                .contains("tex = texture(u_texture0, project(v_tex_coord0)).rgba;")
        );
        assert!(
            source
                .fragment
                .contains("vec3 b = clamp(tex.rgb, 0.0, 1.0)")
        );
    }

    #[test]
    fn texture_matrices_and_environment_inputs_are_applied() {
        let mut material = textured();
        material.tex_gens[0] = GxTexGen {
            kind: 1,
            source: TG_NRM,
            matrix: TEXGEN_MATRIX_FIRST + 3,
        };
        material.tex_matrices[1] = Some(GxTexMatrixInfo {
            info: 6,
            ..GxTexMatrixInfo::default()
        });
        let source = generate(&material);
        assert!(source.vertex.contains(
            "tex_matrix(3, vec4(mat3(u_view) * mat3(u_model) * vec4(a_normal, 1.0).xyz, 1.0))"
        ));
    }

    #[test]
    fn alpha_test_swap_tables_and_konst_selection_are_baked() {
        let mut material = textured();
        material.alpha_test = GxAlphaTest {
            compare0: 4,
            reference0: 127,
            op: 0,
            compare1: 7,
            reference1: 0,
        };
        material.swap_tables[1] = [0, 0, 0, 3];
        material.swap_modes[0] = (0, 1);
        material.konst_color_select[0] = 0x0e;
        material.konst_alpha_select[0] = 0x1d;
        let source = generate(&material);
        assert!(
            source
                .fragment
                .contains("if (!((alpha > 127.0) && (true))) { discard; }")
        );
        assert!(source.fragment.contains(".rrra;"));
        assert!(
            source
                .fragment
                .contains("konst = vec4(u_konst[2].rgb, u_konst[1].a);")
        );
    }

    #[test]
    fn lit_channels_use_ambient_and_light() {
        let mut material = GxMaterial::default();
        material.channels[0].lighting = true;
        material.channels[0].light_mask = 1;
        material.channels[0].diffuse_function = 2;
        material.channels[0].material_from_vertex = false;
        let source = generate(&material);
        assert!(source.vertex.contains(
            "(u_mat_color[0] * clamp(u_amb_color[0] + max(lambert, 0.0) * u_light_color, 0.0, 1.0)).rgb"
        ));
    }

    #[test]
    fn multi_stage_output_uses_last_destination() {
        let mut material = textured();
        material.stage_count = 2;
        material.stages[0].color_reg = 1;
        material.stages[1].color_in = [15, 15, 15, 2];
        material.stages[1].color_reg = 0;
        let source = generate(&material);
        assert!(source.fragment.contains("r_c0.rgb = clamp"));
        assert!(source.fragment.contains("vec3 d = r_c0.rgb;"));
        assert!(
            source
                .fragment
                .contains("vec4 result = clamp(vec4(r_prev.rgb, r_prev.a), 0.0, 1.0);")
        );
    }
}
