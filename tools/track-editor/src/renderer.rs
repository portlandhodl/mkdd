use std::collections::HashMap;
use std::rc::Rc;

use glam::{Mat4, Vec3};
use js_sys::{Float32Array, Object, Reflect, Uint32Array};
use wasm_bindgen::{JsCast, JsValue};
use web_sys::{
    HtmlCanvasElement, WebGl2RenderingContext as Gl, WebGlBuffer, WebGlProgram, WebGlShader,
    WebGlTexture, WebGlUniformLocation, WebGlVertexArrayObject,
};

use crate::error::{FormatError, Result};
use crate::formats::{
    bmd::{BmdCullMode, BmdMesh, BmdModel},
    bti::BtiImage,
    gx::{BLEND_BLEND, BLEND_SUBTRACT, GxMaterial},
};
use crate::gx_shader::{self, ATTR_COLOR0, ATTR_COLOR1, ATTR_NORMAL, ATTR_POSITION, ATTR_TEX0};
use crate::scene::{LayerKind, LayerVisibility, OrbitCamera, Primitive, SceneLayer};

const HELPER_VERTEX_SHADER: &str = r#"#version 300 es
precision highp float;
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec4 a_color;
uniform mat4 u_view_projection;
out vec3 v_world_position;
out vec4 v_color;
out float v_depth;
void main() {
    v_world_position = a_position;
    v_color = a_color;
    gl_Position = u_view_projection * vec4(a_position, 1.0);
    v_depth = 1.0 + gl_Position.w;
}
"#;

const HELPER_FRAGMENT_SHADER: &str = r#"#version 300 es
precision highp float;
in vec3 v_world_position;
in vec4 v_color;
in float v_depth;
uniform bool u_lit;
uniform float u_alpha_scale;
uniform float u_log_depth;
out vec4 out_color;
void main() {
    gl_FragDepth = log2(max(v_depth, 1e-6)) * u_log_depth;
    vec4 color = v_color;
    if (u_lit) {
        vec3 normal = cross(dFdx(v_world_position), dFdy(v_world_position));
        float normal_length_squared = dot(normal, normal);
        if (normal_length_squared > 1e-12) {
            normal *= inversesqrt(normal_length_squared);
            float diffuse = abs(dot(normal, normalize(vec3(-0.45, 0.85, 0.35))));
            color.rgb *= 0.45 + 0.55 * diffuse;
        }
    }
    color.a *= u_alpha_scale;
    if (color.a <= 0.0) {
        discard;
    }
    out_color = color;
}
"#;

/// Used when a generated GX program fails to compile on this driver.
const FALLBACK_VERTEX_SHADER: &str = r#"#version 300 es
precision highp float;
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec4 a_color0;
layout(location = 4) in vec2 a_tex0;
uniform mat4 u_view_projection;
uniform mat4 u_model;
out vec3 v_normal;
out vec4 v_color;
out vec2 v_tex_coord;
out float v_depth;
void main() {
    v_normal = mat3(u_model) * a_normal;
    v_color = a_color0;
    v_tex_coord = a_tex0;
    gl_Position = u_view_projection * u_model * vec4(a_position, 1.0);
    v_depth = 1.0 + gl_Position.w;
}
"#;

const FALLBACK_FRAGMENT_SHADER: &str = r#"#version 300 es
precision highp float;
precision highp sampler2D;
in vec3 v_normal;
in vec4 v_color;
in vec2 v_tex_coord;
in float v_depth;
uniform float u_log_depth;
uniform vec4 u_material_color;
uniform bool u_use_vertex_color;
uniform sampler2D u_texture0;
uniform vec4 u_highlight;
out vec4 out_color;
void main() {
    gl_FragDepth = log2(max(v_depth, 1e-6)) * u_log_depth;
    vec4 raster = u_use_vertex_color ? v_color : u_material_color;
    vec4 color = texture(u_texture0, v_tex_coord) * raster;
    vec3 normal = v_normal;
    float diffuse = 0.0;
    if (dot(normal, normal) > 1e-12) {
        normal = normalize(gl_FrontFacing ? normal : -normal);
        diffuse = max(dot(normal, normalize(vec3(-0.45, 0.85, 0.35))), 0.0);
    }
    color.rgb *= 0.45 + 0.55 * diffuse;
    if (color.a <= 0.5) {
        discard;
    }
    color.rgb = mix(color.rgb, u_highlight.rgb, u_highlight.a);
    out_color = color;
}
"#;

const HELPER_STRIDE: i32 = 7 * 4;

thread_local! {
    /// Checks `getError` after every mesh draw; enable from the console via
    /// `Editor.set_debug_draws(true)` to locate a failing material.
    pub static DEBUG_DRAWS: std::cell::Cell<bool> = const { std::cell::Cell::new(false) };
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum ShadingMode {
    /// Authored GX materials.
    Material,
    /// Single texture times vertex color; useful to inspect geometry/UVs.
    Simple,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum ModelLayer {
    Sky,
    Course,
    Object,
}

#[derive(Clone, Debug)]
pub struct ModelInstance {
    pub model: String,
    pub transform: Mat4,
    pub highlight: [f32; 4],
    pub layer: ModelLayer,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum LayerGroup {
    /// Grid and collision.
    Static,
    /// BOL record markers.
    Helpers,
    /// Gizmo; drawn last without depth testing.
    Overlay,
}

pub struct Renderer {
    canvas: HtmlCanvasElement,
    gl: Gl,
    helper_program: HelperProgram,
    fallback_program: Rc<MeshProgram>,
    programs: HashMap<String, Option<Rc<MeshProgram>>>,
    program_failures: Vec<String>,
    white: GpuTexture,
    anisotropy: Option<(u32, f32)>,
    static_layers: Vec<GpuLayer>,
    helper_layers: Vec<GpuLayer>,
    overlay_layers: Vec<GpuLayer>,
    models: HashMap<String, (usize, GpuModel)>,
    instances: Vec<ModelInstance>,
    shading: ShadingMode,
    light_direction: Vec3,
    light_color: [f32; 4],
    reported_draw_error: std::cell::Cell<bool>,
    sky_follow: bool,
    sky_radius: f32,
    clear_color: [f32; 3],
}

impl Renderer {
    pub fn new(canvas: HtmlCanvasElement) -> Result<Self> {
        let options = Object::new();
        for (key, value) in [
            ("alpha", false),
            ("antialias", true),
            ("premultipliedAlpha", false),
            ("preserveDrawingBuffer", true),
        ] {
            Reflect::set(
                &options,
                &JsValue::from_str(key),
                &JsValue::from_bool(value),
            )
            .map_err(|error| FormatError::new(javascript_error(error)))?;
        }
        let context = canvas
            .get_context_with_context_options("webgl2", &options)
            .map_err(|error| {
                FormatError::new(format!(
                    "Failed to create a WebGL2 context: {}",
                    javascript_error(error)
                ))
            })?
            .ok_or_else(|| FormatError::new("WebGL2 is not available for this canvas"))?;
        let gl = context
            .dyn_into::<Gl>()
            .map_err(|_| FormatError::new("The browser returned an unexpected WebGL2 context"))?;
        let helper_program = HelperProgram::new(&gl)?;
        let fallback_program = Rc::new(MeshProgram::new(
            &gl,
            FALLBACK_VERTEX_SHADER,
            FALLBACK_FRAGMENT_SHADER,
        )?);
        let white = GpuTexture::white(&gl)?;
        let anisotropy = gl
            .get_extension("EXT_texture_filter_anisotropic")
            .ok()
            .flatten()
            .map(|_| {
                const MAX_ANISOTROPY: u32 = 0x84FF;
                const TEXTURE_MAX_ANISOTROPY: u32 = 0x84FE;
                let maximum = gl
                    .get_parameter(MAX_ANISOTROPY)
                    .ok()
                    .and_then(|value| value.as_f64())
                    .unwrap_or(1.0) as f32;
                (TEXTURE_MAX_ANISOTROPY, maximum.min(8.0))
            });
        gl.enable(Gl::DEPTH_TEST);
        gl.depth_func(Gl::LEQUAL);
        Ok(Self {
            canvas,
            gl,
            helper_program,
            fallback_program,
            programs: HashMap::new(),
            program_failures: Vec::new(),
            white,
            anisotropy,
            static_layers: Vec::new(),
            helper_layers: Vec::new(),
            overlay_layers: Vec::new(),
            models: HashMap::new(),
            instances: Vec::new(),
            shading: ShadingMode::Material,
            light_direction: Vec3::new(0.45, -0.85, -0.35).normalize(),
            light_color: [1.0, 1.0, 1.0, 1.0],
            reported_draw_error: std::cell::Cell::new(false),
            sky_follow: false,
            sky_radius: 0.0,
            clear_color: [0.105, 0.115, 0.135],
        })
    }

    pub fn resize_if_needed(&self) -> bool {
        let pixel_ratio = web_sys::window()
            .map(|window| window.device_pixel_ratio())
            .filter(|ratio| ratio.is_finite() && *ratio > 0.0)
            .unwrap_or(1.0);
        let width = scaled_dimension(self.canvas.client_width(), pixel_ratio);
        let height = scaled_dimension(self.canvas.client_height(), pixel_ratio);
        if self.canvas.width() == width && self.canvas.height() == height {
            return false;
        }
        self.canvas.set_width(width);
        self.canvas.set_height(height);
        true
    }

    pub fn set_layers(&mut self, group: LayerGroup, layers: &[SceneLayer]) -> Result<()> {
        if self.gl.is_context_lost() {
            return Err(FormatError::new(
                "Cannot upload a scene to a lost WebGL2 context",
            ));
        }
        let mut uploaded = Vec::with_capacity(layers.len());
        for layer in layers {
            uploaded.push(GpuLayer::new(&self.gl, layer)?);
        }
        match group {
            LayerGroup::Static => self.static_layers = uploaded,
            LayerGroup::Helpers => self.helper_layers = uploaded,
            LayerGroup::Overlay => self.overlay_layers = uploaded,
        }
        Ok(())
    }

    /// Uploads `model` under `key` unless the same model is already resident.
    pub fn set_model(&mut self, key: &str, model: &Rc<BmdModel>) -> Result<()> {
        let identity = Rc::as_ptr(model) as usize;
        if self
            .models
            .get(key)
            .is_some_and(|(existing, _)| *existing == identity)
        {
            return Ok(());
        }
        if self.gl.is_context_lost() {
            return Err(FormatError::new(
                "Cannot upload a model to a lost WebGL2 context",
            ));
        }
        let gpu = GpuModel::new(&self.gl, model, self.anisotropy)?;
        let mut meshes = Vec::with_capacity(gpu.meshes.len());
        for mesh in &gpu.meshes {
            meshes.push(self.program_for(&mesh.gx));
        }
        let mut gpu = gpu;
        for (mesh, program) in gpu.meshes.iter_mut().zip(meshes) {
            mesh.program = program;
        }
        self.models.insert(key.to_owned(), (identity, gpu));
        let error = self.gl.get_error();
        if error != Gl::NO_ERROR {
            return Err(FormatError::new(format!(
                "WebGL upload of model '{key}' failed (error 0x{error:04x})"
            )));
        }
        Ok(())
    }

    pub fn remove_model(&mut self, key: &str) {
        self.models.remove(key);
    }

    pub fn retain_models(&mut self, keep: impl Fn(&str) -> bool) {
        self.models.retain(|key, _| keep(key));
    }

    pub fn has_model(&self, key: &str) -> bool {
        self.models.contains_key(key)
    }

    pub fn set_instances(&mut self, instances: Vec<ModelInstance>) {
        self.instances = instances;
    }

    pub fn set_shading(&mut self, shading: ShadingMode) {
        self.shading = shading;
    }

    pub fn shading(&self) -> ShadingMode {
        self.shading
    }

    /// `Course` centers the sky on the camera when the BOL sets `sky_follow`.
    /// The editor also follows once the camera leaves the dome, so the sky
    /// stays a backdrop instead of a ring around the course.
    pub fn set_sky(&mut self, follow: bool, radius: f32) {
        self.sky_follow = follow;
        self.sky_radius = radius;
    }

    pub fn set_clear_color(&mut self, color: [f32; 3]) {
        self.clear_color = color;
    }

    pub fn set_light(&mut self, direction: Vec3, color: [f32; 4]) {
        if let Some(direction) = direction.try_normalize() {
            self.light_direction = direction;
        }
        self.light_color = color;
    }

    /// Distinct GX shader compile/link failures seen so far.
    pub fn program_failures(&self) -> &[String] {
        &self.program_failures
    }

    pub fn program_count(&self) -> usize {
        self.programs
            .values()
            .filter(|program| program.is_some())
            .count()
    }

    fn program_for(&mut self, material: &GxMaterial) -> Option<Rc<MeshProgram>> {
        let source = gx_shader::generate(material);
        let key = format!("{}\u{0}{}", source.vertex, source.fragment);
        if let Some(program) = self.programs.get(&key) {
            return program.clone();
        }
        let program = match MeshProgram::new(&self.gl, &source.vertex, &source.fragment) {
            Ok(program) => Some(Rc::new(program)),
            Err(error) => {
                if self.program_failures.len() < 16 {
                    self.program_failures.push(error.to_string());
                }
                web_sys::console::warn_1(&JsValue::from_str(&format!(
                    "GX material shader fell back to the simple shader: {error}"
                )));
                None
            }
        };
        self.programs.insert(key, program.clone());
        program
    }

    pub fn render(&self, camera: &OrbitCamera, visibility: &LayerVisibility) -> Result<()> {
        if self.gl.is_context_lost() {
            return Err(FormatError::new("Cannot render with a lost WebGL2 context"));
        }
        self.resize_if_needed();
        let gl = &self.gl;
        let width = self.canvas.width().min(i32::MAX as u32) as i32;
        let height = self.canvas.height().min(i32::MAX as u32) as i32;
        let aspect = width as f32 / height.max(1) as f32;
        let view = camera.view();
        let view_projection = camera.projection(aspect) * view;
        let frame = FrameState {
            view_projection: view_projection.to_cols_array(),
            view: view.to_cols_array(),
            eye: camera.position(),
            log_depth: 1.0 / (camera.clip_planes().1 + 1.0).log2(),
        };

        gl.viewport(0, 0, width, height);
        gl.depth_mask(true);
        gl.color_mask(true, true, true, true);
        gl.disable(Gl::BLEND);
        let [red, green, blue] = self.clear_color;
        gl.clear_color(red, green, blue, 1.0);
        gl.clear_depth(1.0);
        gl.clear(Gl::COLOR_BUFFER_BIT | Gl::DEPTH_BUFFER_BIT);
        gl.front_face(Gl::CW);

        let visible = |layer: ModelLayer| match layer {
            ModelLayer::Sky => visibility.sky,
            ModelLayer::Course => visibility.course_model,
            ModelLayer::Object => visibility.objects,
        };

        // The sky never occludes the course.
        let eye = frame.eye;
        let outside_sky = Vec3::new(eye.x, 0.0, eye.z).length() > self.sky_radius * 0.9
            || eye.y.abs() > self.sky_radius * 0.5;
        for instance in self.instances.iter().filter(|i| i.layer == ModelLayer::Sky) {
            if !visible(instance.layer) {
                continue;
            }
            if let Some((_, model)) = self.models.get(&instance.model) {
                let mut placed = instance.clone();
                if self.sky_follow || outside_sky {
                    placed.transform = Mat4::from_translation(eye) * instance.transform;
                }
                for mesh in &model.meshes {
                    self.draw_mesh(model, mesh, &placed, &frame, DepthOverride::Disabled);
                }
            }
        }
        gl.clear(Gl::DEPTH_BUFFER_BIT);

        let mut translucent = Vec::new();
        for instance in self.instances.iter().filter(|i| i.layer != ModelLayer::Sky) {
            if !visible(instance.layer) {
                continue;
            }
            let Some((_, model)) = self.models.get(&instance.model) else {
                continue;
            };
            for mesh in &model.meshes {
                if mesh.translucent {
                    let center = instance.transform.transform_point3(mesh.center);
                    translucent.push((center.distance_squared(frame.eye), instance, model, mesh));
                } else {
                    self.draw_mesh(model, mesh, instance, &frame, DepthOverride::Material);
                }
            }
        }

        self.draw_helper_group(&self.static_layers, visibility, &frame, true);
        self.draw_helper_group(&self.helper_layers, visibility, &frame, true);

        translucent.sort_by(|a, b| b.0.total_cmp(&a.0));
        for (_, instance, model, mesh) in translucent {
            self.draw_mesh(model, mesh, instance, &frame, DepthOverride::Material);
        }
        reset_state(gl);

        self.draw_helper_group(&self.static_layers, visibility, &frame, false);
        self.draw_helper_group(&self.helper_layers, visibility, &frame, false);

        if !self.overlay_layers.is_empty() {
            gl.clear(Gl::DEPTH_BUFFER_BIT);
            self.use_helper_program(&frame, 1.0);
            gl.enable(Gl::BLEND);
            gl.blend_func(Gl::SRC_ALPHA, Gl::ONE_MINUS_SRC_ALPHA);
            for layer in &self.overlay_layers {
                self.draw_layer(layer);
            }
        }

        gl.bind_vertex_array(None);
        reset_state(gl);
        let error = gl.get_error();
        if error != Gl::NO_ERROR && !self.reported_draw_error.replace(true) {
            // One bad material should not take down the whole viewport.
            web_sys::console::warn_1(&JsValue::from_str(&format!(
                "WebGL reported error 0x{error:04x} while drawing; call Editor.set_debug_draws(true) to locate it"
            )));
        }
        Ok(())
    }

    fn use_helper_program(&self, frame: &FrameState, alpha_scale: f32) {
        let gl = &self.gl;
        gl.use_program(Some(&self.helper_program.handle));
        gl.uniform_matrix4fv_with_f32_array(
            Some(&self.helper_program.view_projection),
            false,
            &frame.view_projection,
        );
        gl.uniform1f(Some(&self.helper_program.alpha_scale), alpha_scale);
        gl.uniform1f(Some(&self.helper_program.log_depth), frame.log_depth);
    }

    /// Opaque helpers depth-test against the course; translucent helpers are
    /// drawn afterwards without depth writes.
    fn draw_helper_group(
        &self,
        layers: &[GpuLayer],
        visibility: &LayerVisibility,
        frame: &FrameState,
        opaque_pass: bool,
    ) {
        let gl = &self.gl;
        self.use_helper_program(frame, 1.0);
        gl.disable(Gl::CULL_FACE);
        gl.enable(Gl::DEPTH_TEST);
        gl.depth_func(Gl::LEQUAL);
        if opaque_pass {
            gl.disable(Gl::BLEND);
            gl.depth_mask(true);
        } else {
            gl.enable(Gl::BLEND);
            gl.blend_func_separate(
                Gl::SRC_ALPHA,
                Gl::ONE_MINUS_SRC_ALPHA,
                Gl::ONE,
                Gl::ONE_MINUS_SRC_ALPHA,
            );
            gl.depth_mask(false);
        }
        for layer in layers {
            if layer.opaque == opaque_pass && visibility.contains(layer.kind) {
                self.draw_layer(layer);
            }
        }
        // Occluded markers are still useful: draw a faint pass through geometry.
        if !opaque_pass {
            gl.depth_func(Gl::GREATER);
            self.use_helper_program(frame, 0.22);
            for layer in layers {
                if layer.kind != LayerKind::Grid
                    && layer.kind != LayerKind::Collision
                    && visibility.contains(layer.kind)
                {
                    self.draw_layer(layer);
                }
            }
            gl.depth_func(Gl::LEQUAL);
        }
    }

    fn draw_layer(&self, layer: &GpuLayer) {
        if layer.index_count == 0 {
            return;
        }
        self.gl.bind_vertex_array(Some(&layer.vertex_array));
        self.gl.uniform1i(
            Some(&self.helper_program.lit),
            i32::from(layer.primitive == Primitive::Triangles && layer.kind != LayerKind::Gizmo),
        );
        let mode = match layer.primitive {
            Primitive::Triangles => Gl::TRIANGLES,
            Primitive::Lines => Gl::LINES,
        };
        self.gl
            .draw_elements_with_i32(mode, layer.index_count, Gl::UNSIGNED_INT, 0);
    }

    fn draw_mesh(
        &self,
        model: &GpuModel,
        mesh: &GpuMesh,
        instance: &ModelInstance,
        frame: &FrameState,
        depth: DepthOverride,
    ) {
        if mesh.index_count == 0 || matches!(mesh.cull_mode, BmdCullMode::All) {
            return;
        }
        let gl = &self.gl;
        let (program, gx_shading) = match (&mesh.program, self.shading) {
            (Some(program), ShadingMode::Material) => (program.as_ref(), true),
            _ => (self.fallback_program.as_ref(), false),
        };
        gl.use_program(Some(&program.handle));
        let u = &program.uniforms;
        let set_matrix = |location: &Option<WebGlUniformLocation>, value: &[f32]| {
            if location.is_some() {
                gl.uniform_matrix4fv_with_f32_array(location.as_ref(), false, value);
            }
        };
        set_matrix(&u.view_projection, &frame.view_projection);
        set_matrix(&u.view, &frame.view);
        set_matrix(&u.model, &instance.transform.to_cols_array());
        set_vec4(gl, &u.highlight, &instance.highlight);
        if let Some(location) = &u.log_depth {
            gl.uniform1f(Some(location), frame.log_depth);
        }

        let gx = &mesh.gx;
        if gx_shading {
            set_vec4(gl, &u.mat_color, &flatten(&gx.material_colors));
            set_vec4(gl, &u.amb_color, &flatten(&gx.ambient_colors));
            set_vec4(gl, &u.registers, &flatten(&gx.registers));
            set_vec4(gl, &u.konst, &flatten(&gx.konst_colors));
            set_vec4(gl, &u.light_color, &self.light_color);
            if let Some(location) = &u.light_direction {
                gl.uniform3fv_with_f32_array(Some(location), &self.light_direction.to_array());
            }
            set_vec4(gl, &u.tex_mtx, &mesh.tex_matrices);
        } else {
            set_vec4(gl, &u.material_color, &mesh.basic_color);
            if let Some(location) = &u.use_vertex_color {
                gl.uniform1i(Some(location), i32::from(mesh.basic_use_vertex_color));
            }
        }
        for (map, location) in u.textures.iter().enumerate() {
            let Some(location) = location else {
                continue;
            };
            let texture = if gx_shading {
                mesh.textures[map]
            } else {
                mesh.basic_texture
            };
            let handle = texture
                .and_then(|index| model.textures.get(index))
                .unwrap_or(&self.white);
            gl.active_texture(Gl::TEXTURE0 + map as u32);
            gl.bind_texture(Gl::TEXTURE_2D, Some(&handle.handle));
            gl.uniform1i(Some(location), map as i32);
        }

        match mesh.cull_mode {
            BmdCullMode::None | BmdCullMode::All => gl.disable(Gl::CULL_FACE),
            BmdCullMode::Front => {
                gl.enable(Gl::CULL_FACE);
                gl.cull_face(Gl::FRONT);
            }
            BmdCullMode::Back => {
                gl.enable(Gl::CULL_FACE);
                gl.cull_face(Gl::BACK);
            }
        }
        if DEBUG_DRAWS.with(|flag| flag.get()) {
            let error = gl.get_error();
            if error != Gl::NO_ERROR {
                web_sys::console::error_1(&JsValue::from_str(&format!(
                    "uniform/texture setup for '{}' failed with 0x{error:04x}",
                    instance.model
                )));
            }
        }
        apply_blend(gl, gx, mesh.translucent, gx_shading);
        match depth {
            DepthOverride::Disabled => {
                gl.disable(Gl::DEPTH_TEST);
                gl.depth_mask(false);
            }
            DepthOverride::Material => {
                if gx.z_mode.test {
                    gl.enable(Gl::DEPTH_TEST);
                    gl.depth_func(gx_compare(gx.z_mode.function));
                } else {
                    gl.disable(Gl::DEPTH_TEST);
                }
                gl.depth_mask(gx.z_mode.write && (gx_shading || !mesh.translucent));
            }
        }
        gl.bind_vertex_array(Some(&mesh.vertex_array));
        gl.draw_elements_with_i32(Gl::TRIANGLES, mesh.index_count, Gl::UNSIGNED_INT, 0);
        if cfg!(debug_assertions) || DEBUG_DRAWS.with(|flag| flag.get()) {
            let error = gl.get_error();
            if error != Gl::NO_ERROR {
                web_sys::console::error_1(&JsValue::from_str(&format!(
                    "draw of '{}' failed with 0x{error:04x}; program {}; textures {:?}; uv sets {}",
                    instance.model,
                    if gx_shading { "gx" } else { "fallback" },
                    mesh.textures,
                    mesh.gx.uv_sets_used()
                )));
            }
        }
    }
}

#[derive(Clone, Copy)]
enum DepthOverride {
    Disabled,
    Material,
}

struct FrameState {
    view_projection: [f32; 16],
    view: [f32; 16],
    eye: Vec3,
    /// `1 / log2(far + 1)`: maps view depth to [0, 1] logarithmically.
    log_depth: f32,
}

fn reset_state(gl: &Gl) {
    gl.disable(Gl::CULL_FACE);
    gl.disable(Gl::BLEND);
    gl.blend_equation(Gl::FUNC_ADD);
    gl.enable(Gl::DEPTH_TEST);
    gl.depth_func(Gl::LEQUAL);
    gl.depth_mask(true);
}

fn flatten<const N: usize>(values: &[[f32; 4]; N]) -> Vec<f32> {
    values.iter().flatten().copied().collect()
}

fn set_vec4(gl: &Gl, location: &Option<WebGlUniformLocation>, values: &[f32]) {
    if let Some(location) = location {
        gl.uniform4fv_with_f32_array(Some(location), values);
    }
}

fn gx_compare(function: u8) -> u32 {
    match function & 7 {
        0 => Gl::NEVER,
        1 => Gl::LESS,
        2 => Gl::EQUAL,
        3 => Gl::LEQUAL,
        4 => Gl::GREATER,
        5 => Gl::NOTEQUAL,
        6 => Gl::GEQUAL,
        _ => Gl::ALWAYS,
    }
}

fn gx_blend_factor(factor: u8, source: bool) -> u32 {
    match factor & 7 {
        0 => Gl::ZERO,
        1 => Gl::ONE,
        2 if source => Gl::DST_COLOR,
        2 => Gl::SRC_COLOR,
        3 if source => Gl::ONE_MINUS_DST_COLOR,
        3 => Gl::ONE_MINUS_SRC_COLOR,
        4 => Gl::SRC_ALPHA,
        5 => Gl::ONE_MINUS_SRC_ALPHA,
        6 => Gl::DST_ALPHA,
        _ => Gl::ONE_MINUS_DST_ALPHA,
    }
}

fn apply_blend(gl: &Gl, gx: &GxMaterial, translucent: bool, gx_shading: bool) {
    if !gx_shading {
        if translucent {
            gl.enable(Gl::BLEND);
            gl.blend_equation(Gl::FUNC_ADD);
            gl.blend_func(Gl::SRC_ALPHA, Gl::ONE_MINUS_SRC_ALPHA);
        } else {
            gl.disable(Gl::BLEND);
        }
        return;
    }
    match gx.blend.mode {
        BLEND_BLEND => {
            gl.enable(Gl::BLEND);
            gl.blend_equation(Gl::FUNC_ADD);
            // The canvas has no alpha channel, so keep destination alpha at 1.
            gl.blend_func_separate(
                gx_blend_factor(gx.blend.source, true),
                gx_blend_factor(gx.blend.destination, false),
                Gl::ZERO,
                Gl::ONE,
            );
        }
        BLEND_SUBTRACT => {
            gl.enable(Gl::BLEND);
            gl.blend_equation(Gl::FUNC_REVERSE_SUBTRACT);
            gl.blend_func(Gl::ONE, Gl::ONE);
        }
        _ => gl.disable(Gl::BLEND),
    }
}

struct HelperProgram {
    gl: Gl,
    handle: WebGlProgram,
    view_projection: WebGlUniformLocation,
    lit: WebGlUniformLocation,
    alpha_scale: WebGlUniformLocation,
    log_depth: WebGlUniformLocation,
}

impl HelperProgram {
    fn new(gl: &Gl) -> Result<Self> {
        let handle = link_program(gl, HELPER_VERTEX_SHADER, HELPER_FRAGMENT_SHADER)?;
        let uniforms = (|| -> Result<_> {
            Ok((
                required_uniform(gl, &handle, "u_view_projection")?,
                required_uniform(gl, &handle, "u_lit")?,
                required_uniform(gl, &handle, "u_alpha_scale")?,
                required_uniform(gl, &handle, "u_log_depth")?,
            ))
        })();
        match uniforms {
            Ok((view_projection, lit, alpha_scale, log_depth)) => Ok(Self {
                gl: gl.clone(),
                handle,
                view_projection,
                lit,
                alpha_scale,
                log_depth,
            }),
            Err(error) => {
                gl.delete_program(Some(&handle));
                Err(error)
            }
        }
    }
}

impl Drop for HelperProgram {
    fn drop(&mut self) {
        self.gl.delete_program(Some(&self.handle));
    }
}

#[derive(Default)]
struct MeshUniforms {
    view_projection: Option<WebGlUniformLocation>,
    model: Option<WebGlUniformLocation>,
    view: Option<WebGlUniformLocation>,
    mat_color: Option<WebGlUniformLocation>,
    amb_color: Option<WebGlUniformLocation>,
    light_direction: Option<WebGlUniformLocation>,
    light_color: Option<WebGlUniformLocation>,
    tex_mtx: Option<WebGlUniformLocation>,
    registers: Option<WebGlUniformLocation>,
    konst: Option<WebGlUniformLocation>,
    highlight: Option<WebGlUniformLocation>,
    log_depth: Option<WebGlUniformLocation>,
    material_color: Option<WebGlUniformLocation>,
    use_vertex_color: Option<WebGlUniformLocation>,
    textures: [Option<WebGlUniformLocation>; 8],
}

struct MeshProgram {
    gl: Gl,
    handle: WebGlProgram,
    uniforms: MeshUniforms,
}

impl MeshProgram {
    fn new(gl: &Gl, vertex: &str, fragment: &str) -> Result<Self> {
        let handle = link_program(gl, vertex, fragment)?;
        let find = |name: &str| gl.get_uniform_location(&handle, name);
        let uniforms = MeshUniforms {
            view_projection: find("u_view_projection"),
            model: find("u_model"),
            view: find("u_view"),
            mat_color: find("u_mat_color"),
            amb_color: find("u_amb_color"),
            light_direction: find("u_light_direction"),
            light_color: find("u_light_color"),
            tex_mtx: find("u_tex_mtx"),
            registers: find("u_registers"),
            konst: find("u_konst"),
            highlight: find("u_highlight"),
            log_depth: find("u_log_depth"),
            material_color: find("u_material_color"),
            use_vertex_color: find("u_use_vertex_color"),
            textures: std::array::from_fn(|map| find(&format!("u_texture{map}"))),
        };
        Ok(Self {
            gl: gl.clone(),
            handle,
            uniforms,
        })
    }
}

impl Drop for MeshProgram {
    fn drop(&mut self) {
        self.gl.delete_program(Some(&self.handle));
    }
}

struct GpuLayer {
    gl: Gl,
    vertex_array: WebGlVertexArrayObject,
    vertex_buffer: WebGlBuffer,
    index_buffer: WebGlBuffer,
    kind: LayerKind,
    primitive: Primitive,
    index_count: i32,
    opaque: bool,
}

impl GpuLayer {
    fn new(gl: &Gl, source: &SceneLayer) -> Result<Self> {
        let index_count = i32::try_from(source.indices.len())
            .map_err(|_| FormatError::new("A scene layer has too many indices for WebGL"))?;
        let mut vertices = Vec::with_capacity(source.vertices.len() * 7);
        let mut opaque = true;
        for vertex in &source.vertices {
            vertices.extend_from_slice(&vertex.position);
            vertices.extend_from_slice(&vertex.color);
            opaque &= vertex.color[3] >= 1.0;
        }
        let (vertex_array, vertex_buffer, index_buffer) =
            create_vertex_objects(gl, &vertices, &source.indices)?;
        gl.bind_vertex_array(Some(&vertex_array));
        gl.bind_buffer(Gl::ARRAY_BUFFER, Some(&vertex_buffer));
        gl.enable_vertex_attrib_array(0);
        gl.vertex_attrib_pointer_with_i32(0, 3, Gl::FLOAT, false, HELPER_STRIDE, 0);
        gl.enable_vertex_attrib_array(1);
        gl.vertex_attrib_pointer_with_i32(1, 4, Gl::FLOAT, false, HELPER_STRIDE, 12);
        gl.bind_vertex_array(None);
        gl.bind_buffer(Gl::ARRAY_BUFFER, None);
        Ok(Self {
            gl: gl.clone(),
            vertex_array,
            vertex_buffer,
            index_buffer,
            kind: source.kind,
            primitive: source.primitive,
            index_count,
            opaque: opaque && source.kind != LayerKind::Grid,
        })
    }
}

impl Drop for GpuLayer {
    fn drop(&mut self) {
        self.gl.delete_buffer(Some(&self.index_buffer));
        self.gl.delete_buffer(Some(&self.vertex_buffer));
        self.gl.delete_vertex_array(Some(&self.vertex_array));
    }
}

fn create_vertex_objects(
    gl: &Gl,
    vertices: &[f32],
    indices: &[u32],
) -> Result<(WebGlVertexArrayObject, WebGlBuffer, WebGlBuffer)> {
    let vertex_array = gl
        .create_vertex_array()
        .ok_or_else(|| FormatError::new("Failed to allocate a WebGL vertex array"))?;
    let Some(vertex_buffer) = gl.create_buffer() else {
        gl.delete_vertex_array(Some(&vertex_array));
        return Err(FormatError::new("Failed to allocate a WebGL vertex buffer"));
    };
    let Some(index_buffer) = gl.create_buffer() else {
        gl.delete_buffer(Some(&vertex_buffer));
        gl.delete_vertex_array(Some(&vertex_array));
        return Err(FormatError::new("Failed to allocate a WebGL index buffer"));
    };
    gl.bind_vertex_array(Some(&vertex_array));
    gl.bind_buffer(Gl::ARRAY_BUFFER, Some(&vertex_buffer));
    gl.buffer_data_with_array_buffer_view(
        Gl::ARRAY_BUFFER,
        &Float32Array::from(vertices),
        Gl::STATIC_DRAW,
    );
    gl.bind_buffer(Gl::ELEMENT_ARRAY_BUFFER, Some(&index_buffer));
    gl.buffer_data_with_array_buffer_view(
        Gl::ELEMENT_ARRAY_BUFFER,
        &Uint32Array::from(indices),
        Gl::STATIC_DRAW,
    );
    gl.bind_vertex_array(None);
    gl.bind_buffer(Gl::ARRAY_BUFFER, None);
    gl.bind_buffer(Gl::ELEMENT_ARRAY_BUFFER, None);
    Ok((vertex_array, vertex_buffer, index_buffer))
}

struct GpuModel {
    meshes: Vec<GpuMesh>,
    textures: Vec<GpuTexture>,
}

impl GpuModel {
    fn new(gl: &Gl, model: &BmdModel, anisotropy: Option<(u32, f32)>) -> Result<Self> {
        let mut textures = Vec::with_capacity(model.textures.len());
        for image in &model.textures {
            textures.push(GpuTexture::from_bti(gl, image, anisotropy)?);
        }
        let mut meshes = Vec::with_capacity(model.meshes.len());
        for source in &model.meshes {
            meshes.push(GpuMesh::new(gl, source, model.textures.len())?);
        }
        Ok(Self { meshes, textures })
    }
}

struct GpuMesh {
    gl: Gl,
    vertex_array: WebGlVertexArrayObject,
    vertex_buffer: WebGlBuffer,
    index_buffer: WebGlBuffer,
    index_count: i32,
    gx: GxMaterial,
    program: Option<Rc<MeshProgram>>,
    textures: [Option<usize>; 8],
    tex_matrices: Vec<f32>,
    basic_texture: Option<usize>,
    basic_color: [f32; 4],
    basic_use_vertex_color: bool,
    cull_mode: BmdCullMode,
    translucent: bool,
    center: Vec3,
}

impl GpuMesh {
    fn new(gl: &Gl, source: &BmdMesh, texture_count: usize) -> Result<Self> {
        let gx = source.material.gx;
        let index_count = i32::try_from(source.indices.len())
            .map_err(|_| FormatError::new("A model mesh has too many indices for WebGL"))?;
        let uv_sets = gx.uv_sets_used().max(1);
        let floats = 14 + uv_sets * 2;
        let stride = (floats * 4) as i32;
        let mut vertices = Vec::with_capacity(source.vertices.len() * floats);
        let mut sum = Vec3::ZERO;
        for vertex in &source.vertices {
            vertices.extend_from_slice(&vertex.position);
            vertices.extend_from_slice(&vertex.normal);
            vertices.extend_from_slice(&vertex.color);
            vertices.extend_from_slice(&vertex.color1);
            for set in &vertex.tex_coords[..uv_sets] {
                vertices.extend_from_slice(set);
            }
            sum += Vec3::from_array(vertex.position);
        }
        let center = if source.vertices.is_empty() {
            Vec3::ZERO
        } else {
            sum / source.vertices.len() as f32
        };
        let (vertex_array, vertex_buffer, index_buffer) =
            create_vertex_objects(gl, &vertices, &source.indices)?;
        gl.bind_vertex_array(Some(&vertex_array));
        gl.bind_buffer(Gl::ARRAY_BUFFER, Some(&vertex_buffer));
        let mut attributes = vec![
            (ATTR_POSITION, 3, 0),
            (ATTR_NORMAL, 3, 12),
            (ATTR_COLOR0, 4, 24),
            (ATTR_COLOR1, 4, 40),
        ];
        for set in 0..uv_sets {
            attributes.push((ATTR_TEX0 + set as u32, 2, 56 + set as i32 * 8));
        }
        for (location, size, offset) in attributes {
            gl.enable_vertex_attrib_array(location);
            gl.vertex_attrib_pointer_with_i32(location, size, Gl::FLOAT, false, stride, offset);
        }
        // Constant defaults for attributes this mesh does not carry.
        for set in uv_sets..8 {
            gl.vertex_attrib4f(ATTR_TEX0 + set as u32, 0.0, 0.0, 0.0, 1.0);
        }
        gl.bind_vertex_array(None);
        gl.bind_buffer(Gl::ARRAY_BUFFER, None);

        let mut tex_matrices = vec![0.0; 30 * 4];
        for (slot, info) in gx.tex_matrices.iter().enumerate() {
            let rows = info.map_or(
                [
                    [1.0, 0.0, 0.0, 0.0],
                    [0.0, 1.0, 0.0, 0.0],
                    [0.0, 0.0, 1.0, 0.0],
                ],
                |info| info.matrix(),
            );
            for (row, values) in rows.iter().enumerate() {
                tex_matrices[(slot * 3 + row) * 4..(slot * 3 + row) * 4 + 4]
                    .copy_from_slice(values);
            }
        }
        let textures = gx
            .textures
            .map(|texture| texture.filter(|index| *index < texture_count));
        let translucent = gx.is_translucent() || source.material.translucent;
        Ok(Self {
            gl: gl.clone(),
            vertex_array,
            vertex_buffer,
            index_buffer,
            index_count,
            gx,
            program: None,
            textures,
            tex_matrices,
            basic_texture: source
                .material
                .texture
                .filter(|index| *index < texture_count),
            basic_color: source.material.color,
            basic_use_vertex_color: source.material.use_vertex_color,
            cull_mode: source.material.cull_mode,
            translucent,
            center,
        })
    }
}

impl Drop for GpuMesh {
    fn drop(&mut self) {
        self.gl.delete_buffer(Some(&self.index_buffer));
        self.gl.delete_buffer(Some(&self.vertex_buffer));
        self.gl.delete_vertex_array(Some(&self.vertex_array));
    }
}

struct GpuTexture {
    gl: Gl,
    handle: WebGlTexture,
}

impl GpuTexture {
    fn white(gl: &Gl) -> Result<Self> {
        Self::new(
            gl,
            1,
            1,
            &[u8::MAX; 4],
            Gl::CLAMP_TO_EDGE,
            Gl::CLAMP_TO_EDGE,
            Gl::NEAREST,
            Gl::NEAREST,
            false,
            None,
        )
    }

    fn from_bti(gl: &Gl, image: &BtiImage, anisotropy: Option<(u32, f32)>) -> Result<Self> {
        let mipmapped = image.mipmap || (2..=5).contains(&image.min_filter);
        // GX textures in courses are authored for mipmapped trilinear sampling
        // at grazing angles; mipmap even when only the base level is stored.
        let min_filter = match image.min_filter {
            0 => Gl::NEAREST,
            2 => Gl::NEAREST_MIPMAP_NEAREST,
            3 => Gl::LINEAR_MIPMAP_NEAREST,
            4 => Gl::NEAREST_MIPMAP_LINEAR,
            _ => Gl::LINEAR_MIPMAP_LINEAR,
        };
        Self::new(
            gl,
            image.width,
            image.height,
            &image.rgba,
            texture_wrap(image.wrap_s),
            texture_wrap(image.wrap_t),
            min_filter,
            if image.mag_filter == 0 {
                Gl::NEAREST
            } else {
                Gl::LINEAR
            },
            mipmapped || min_filter != Gl::NEAREST,
            anisotropy,
        )
    }

    #[allow(clippy::too_many_arguments)]
    fn new(
        gl: &Gl,
        width: u16,
        height: u16,
        rgba: &[u8],
        wrap_s: u32,
        wrap_t: u32,
        min_filter: u32,
        mag_filter: u32,
        mipmapped: bool,
        anisotropy: Option<(u32, f32)>,
    ) -> Result<Self> {
        if width == 0 || height == 0 {
            return Err(FormatError::new("A texture's dimensions must be non-zero"));
        }
        let expected = usize::from(width) * usize::from(height) * 4;
        if rgba.len() != expected {
            return Err(FormatError::new(format!(
                "A texture has {} RGBA bytes; expected {expected}",
                rgba.len()
            )));
        }
        let handle = gl
            .create_texture()
            .ok_or_else(|| FormatError::new("Failed to allocate a WebGL texture"))?;
        gl.active_texture(Gl::TEXTURE0);
        gl.bind_texture(Gl::TEXTURE_2D, Some(&handle));
        gl.pixel_storei(Gl::UNPACK_ALIGNMENT, 1);
        gl.tex_parameteri(Gl::TEXTURE_2D, Gl::TEXTURE_WRAP_S, wrap_s as i32);
        gl.tex_parameteri(Gl::TEXTURE_2D, Gl::TEXTURE_WRAP_T, wrap_t as i32);
        let min_filter = if mipmapped {
            min_filter
        } else {
            match min_filter {
                Gl::NEAREST | Gl::NEAREST_MIPMAP_NEAREST | Gl::NEAREST_MIPMAP_LINEAR => Gl::NEAREST,
                _ => Gl::LINEAR,
            }
        };
        gl.tex_parameteri(Gl::TEXTURE_2D, Gl::TEXTURE_MIN_FILTER, min_filter as i32);
        gl.tex_parameteri(Gl::TEXTURE_2D, Gl::TEXTURE_MAG_FILTER, mag_filter as i32);
        if mipmapped && let Some((parameter, amount)) = anisotropy {
            gl.tex_parameterf(Gl::TEXTURE_2D, parameter, amount);
        }
        gl.tex_image_2d_with_i32_and_i32_and_i32_and_format_and_type_and_opt_u8_array(
            Gl::TEXTURE_2D,
            0,
            Gl::RGBA as i32,
            i32::from(width),
            i32::from(height),
            0,
            Gl::RGBA,
            Gl::UNSIGNED_BYTE,
            Some(rgba),
        )
        .map_err(|error| {
            FormatError::new(format!(
                "Texture upload failed: {}",
                javascript_error(error)
            ))
        })?;
        if mipmapped {
            gl.generate_mipmap(Gl::TEXTURE_2D);
        }
        gl.bind_texture(Gl::TEXTURE_2D, None);
        Ok(Self {
            gl: gl.clone(),
            handle,
        })
    }
}

impl Drop for GpuTexture {
    fn drop(&mut self) {
        self.gl.delete_texture(Some(&self.handle));
    }
}

fn texture_wrap(wrap: u8) -> u32 {
    match wrap {
        1 => Gl::REPEAT,
        2 => Gl::MIRRORED_REPEAT,
        _ => Gl::CLAMP_TO_EDGE,
    }
}

fn link_program(gl: &Gl, vertex: &str, fragment: &str) -> Result<WebGlProgram> {
    let vertex_shader = compile_shader(gl, Gl::VERTEX_SHADER, vertex, "vertex")?;
    let fragment_shader = match compile_shader(gl, Gl::FRAGMENT_SHADER, fragment, "fragment") {
        Ok(shader) => shader,
        Err(error) => {
            gl.delete_shader(Some(&vertex_shader));
            return Err(error);
        }
    };
    let Some(handle) = gl.create_program() else {
        gl.delete_shader(Some(&vertex_shader));
        gl.delete_shader(Some(&fragment_shader));
        return Err(FormatError::new(
            "Failed to allocate a WebGL shader program",
        ));
    };
    gl.attach_shader(&handle, &vertex_shader);
    gl.attach_shader(&handle, &fragment_shader);
    gl.link_program(&handle);
    let linked = gl
        .get_program_parameter(&handle, Gl::LINK_STATUS)
        .as_bool()
        .unwrap_or(false);
    let log = (!linked).then(|| diagnostic(gl.get_program_info_log(&handle)));
    gl.detach_shader(&handle, &vertex_shader);
    gl.detach_shader(&handle, &fragment_shader);
    gl.delete_shader(Some(&vertex_shader));
    gl.delete_shader(Some(&fragment_shader));
    if let Some(log) = log {
        gl.delete_program(Some(&handle));
        return Err(FormatError::new(format!(
            "WebGL shader program link failed: {log}"
        )));
    }
    Ok(handle)
}

fn compile_shader(gl: &Gl, shader_type: u32, source: &str, label: &str) -> Result<WebGlShader> {
    let shader = gl
        .create_shader(shader_type)
        .ok_or_else(|| FormatError::new(format!("Failed to allocate the WebGL {label} shader")))?;
    gl.shader_source(&shader, source);
    gl.compile_shader(&shader);
    let compiled = gl
        .get_shader_parameter(&shader, Gl::COMPILE_STATUS)
        .as_bool()
        .unwrap_or(false);
    if !compiled {
        let log = diagnostic(gl.get_shader_info_log(&shader));
        gl.delete_shader(Some(&shader));
        return Err(FormatError::new(format!(
            "WebGL {label} shader compilation failed: {log}"
        )));
    }
    Ok(shader)
}

fn required_uniform(gl: &Gl, program: &WebGlProgram, name: &str) -> Result<WebGlUniformLocation> {
    gl.get_uniform_location(program, name)
        .ok_or_else(|| FormatError::new(format!("WebGL shader uniform `{name}` is unavailable")))
}

fn diagnostic(log: Option<String>) -> String {
    log.filter(|message| !message.trim().is_empty())
        .unwrap_or_else(|| "no driver log provided".to_owned())
}

fn javascript_error(error: JsValue) -> String {
    error.as_string().unwrap_or_else(|| format!("{error:?}"))
}

fn scaled_dimension(css_size: i32, pixel_ratio: f64) -> u32 {
    if css_size <= 0 {
        return 1;
    }
    let scaled = f64::from(css_size) * pixel_ratio;
    if !scaled.is_finite() {
        return i32::MAX as u32;
    }
    scaled.round().clamp(1.0, i32::MAX as f64) as u32
}
