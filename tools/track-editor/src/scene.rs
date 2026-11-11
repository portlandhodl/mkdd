use glam::{Mat4, Vec3, Vec4};

use crate::catalog;
use crate::formats::bco::Bco;
use crate::formats::bol::BolDocument;
use crate::transform::{self, decode_direction};

#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub enum LayerKind {
    Grid,
    Collision,
    Objects,
    Routes,
    Checkpoints,
    Centerline,
    Starts,
    Areas,
    Cameras,
    Respawns,
    Gizmo,
}

#[derive(Clone, Copy, Debug)]
pub struct Vertex {
    pub position: [f32; 3],
    pub color: [f32; 4],
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Primitive {
    Triangles,
    Lines,
}

#[derive(Clone, Debug)]
pub struct SceneLayer {
    pub kind: LayerKind,
    pub primitive: Primitive,
    pub vertices: Vec<Vertex>,
    pub indices: Vec<u32>,
}

#[derive(Clone, Copy, Debug, PartialEq)]
pub struct Bounds {
    pub min: Vec3,
    pub max: Vec3,
}

impl Bounds {
    pub fn center(self) -> Vec3 {
        (self.min + self.max) * 0.5
    }

    pub fn radius(self) -> f32 {
        (self.max - self.min).length().max(1.0) * 0.5
    }

    pub fn union(self, other: Bounds) -> Bounds {
        Bounds {
            min: self.min.min(other.min),
            max: self.max.max(other.max),
        }
    }

    pub fn transformed(self, matrix: Mat4) -> Bounds {
        let mut result: Option<Bounds> = None;
        for corner in 0..8 {
            let point = Vec3::new(
                if corner & 1 == 0 {
                    self.min.x
                } else {
                    self.max.x
                },
                if corner & 2 == 0 {
                    self.min.y
                } else {
                    self.max.y
                },
                if corner & 4 == 0 {
                    self.min.z
                } else {
                    self.max.z
                },
            );
            include_point(&mut result, matrix.transform_point3(point));
        }
        result.unwrap_or(self)
    }
}

#[derive(Clone, Debug, Default)]
pub struct Scene {
    pub layers: Vec<SceneLayer>,
    pub bounds: Option<Bounds>,
}

#[derive(Clone, Debug)]
pub struct LayerVisibility {
    pub grid: bool,
    pub collision: bool,
    pub course_model: bool,
    pub sky: bool,
    pub objects: bool,
    pub routes: bool,
    pub checkpoints: bool,
    pub centerline: bool,
    pub starts: bool,
    pub areas: bool,
    pub cameras: bool,
    pub respawns: bool,
}

impl Default for LayerVisibility {
    fn default() -> Self {
        Self {
            grid: true,
            collision: false,
            course_model: true,
            sky: true,
            objects: true,
            routes: true,
            checkpoints: true,
            centerline: false,
            starts: true,
            areas: false,
            cameras: false,
            respawns: true,
        }
    }
}

pub const LAYER_NAMES: [(&str, &str); 12] = [
    ("course_model", "Course Model"),
    ("sky", "Sky"),
    ("collision", "Collision"),
    ("grid", "Grid"),
    ("objects", "Objects"),
    ("starts", "Start Points"),
    ("checkpoints", "Checkpoints"),
    ("routes", "Routes"),
    ("centerline", "Centerline"),
    ("respawns", "Respawns"),
    ("areas", "Areas"),
    ("cameras", "Cameras"),
];

impl LayerVisibility {
    pub fn contains(&self, kind: LayerKind) -> bool {
        match kind {
            LayerKind::Grid => self.grid,
            LayerKind::Collision => self.collision,
            LayerKind::Objects => self.objects,
            LayerKind::Routes => self.routes,
            LayerKind::Checkpoints => self.checkpoints,
            LayerKind::Centerline => self.centerline,
            LayerKind::Starts => self.starts,
            LayerKind::Areas => self.areas,
            LayerKind::Cameras => self.cameras,
            LayerKind::Respawns => self.respawns,
            LayerKind::Gizmo => true,
        }
    }

    pub fn get(&self, name: &str) -> Option<bool> {
        Some(match name {
            "grid" => self.grid,
            "collision" => self.collision,
            "course_model" => self.course_model,
            "sky" => self.sky,
            "objects" => self.objects,
            "routes" => self.routes,
            "checkpoints" => self.checkpoints,
            "centerline" => self.centerline,
            "starts" => self.starts,
            "areas" => self.areas,
            "cameras" => self.cameras,
            "respawns" => self.respawns,
            _ => return None,
        })
    }

    /// Whether the BOL record kind is currently drawn (and therefore pickable).
    pub fn shows_entity(&self, kind: &str) -> bool {
        match kind {
            "object" => self.objects,
            "route_point" | "path" => self.routes,
            "checkpoint" => self.checkpoints,
            "centerline" => self.centerline,
            "start" => self.starts,
            "area" => self.areas,
            "camera" => self.cameras,
            "respawn" => self.respawns,
            _ => true,
        }
    }

    pub fn set(&mut self, name: &str, visible: bool) -> bool {
        match name {
            "grid" => self.grid = visible,
            "collision" => self.collision = visible,
            "course_model" => self.course_model = visible,
            "sky" => self.sky = visible,
            "objects" => self.objects = visible,
            "routes" => self.routes = visible,
            "checkpoints" => self.checkpoints = visible,
            "centerline" => self.centerline = visible,
            "starts" => self.starts = visible,
            "areas" => self.areas = visible,
            "cameras" => self.cameras = visible,
            "respawns" => self.respawns = visible,
            _ => return false,
        }
        true
    }
}

pub const CAMERA_FOV: f32 = 55.0_f32.to_radians();
const MIN_DISTANCE: f32 = 0.05;
const MAX_DISTANCE: f32 = 1.0e9;
const MAX_PITCH: f32 = std::f32::consts::FRAC_PI_2 - 0.001;

/// An editor camera that supports both orbiting a pivot and first-person
/// flight. The pivot (`target`) sits `distance` in front of the eye.
#[derive(Clone, Copy, Debug)]
pub struct OrbitCamera {
    target: Vec3,
    distance: f32,
    yaw: f32,
    pitch: f32,
    scene_radius: f32,
}

impl Default for OrbitCamera {
    fn default() -> Self {
        Self {
            target: Vec3::ZERO,
            distance: 1_000.0,
            yaw: std::f32::consts::FRAC_PI_4,
            pitch: 0.65,
            scene_radius: 1_000.0,
        }
    }
}

impl OrbitCamera {
    pub fn orbit(&mut self, dx: f32, dy: f32) {
        if !dx.is_finite() || !dy.is_finite() {
            return;
        }
        self.yaw = (self.yaw + dx * 0.008).rem_euclid(std::f32::consts::TAU);
        self.pitch = (self.pitch + dy * 0.008).clamp(-MAX_PITCH, MAX_PITCH);
    }

    /// Mouse-look: rotates the view about the eye instead of the pivot.
    pub fn look(&mut self, dx: f32, dy: f32) {
        if !dx.is_finite() || !dy.is_finite() {
            return;
        }
        let eye = self.position();
        self.yaw = (self.yaw + dx * 0.004).rem_euclid(std::f32::consts::TAU);
        self.pitch = (self.pitch + dy * 0.004).clamp(-MAX_PITCH, MAX_PITCH);
        self.target = eye - self.offset_direction() * self.distance;
    }

    /// Moves eye and pivot together, in camera-relative units.
    pub fn fly(&mut self, forward: f32, right: f32, up: f32) {
        let delta = self.forward() * forward + self.right() * right + Vec3::Y * up;
        if delta.is_finite() {
            self.target += delta;
        }
    }

    pub fn pan(&mut self, dx: f32, dy: f32, _viewport_width: f32, viewport_height: f32) {
        if !dx.is_finite() || !dy.is_finite() || !viewport_height.is_finite() {
            return;
        }
        let right = self.right();
        let up = right.cross(self.forward()).normalize_or_zero();
        let units_per_pixel =
            2.0 * self.distance * (CAMERA_FOV * 0.5).tan() / viewport_height.max(1.0);
        self.target += (right * -dx + up * dy) * units_per_pixel;
    }

    pub fn zoom(&mut self, delta: f32) {
        if !delta.is_finite() {
            return;
        }
        let factor = (delta * 0.001).clamp(-8.0, 8.0).exp();
        self.distance = (self.distance * factor).clamp(MIN_DISTANCE, MAX_DISTANCE);
    }

    /// Dolly toward a world point under the cursor, keeping it fixed on screen.
    pub fn zoom_toward(&mut self, delta: f32, point: Vec3) {
        if !delta.is_finite() || !point.is_finite() {
            return;
        }
        let factor = (delta * 0.001).clamp(-8.0, 8.0).exp();
        let eye = self.position();
        let new_eye = point + (eye - point) * factor;
        let new_distance = (self.distance * factor).clamp(MIN_DISTANCE, MAX_DISTANCE);
        self.distance = new_distance;
        self.target = new_eye - self.offset_direction() * new_distance;
    }

    pub fn frame(&mut self, bounds: Bounds) {
        self.focus(bounds.center(), bounds.radius());
    }

    pub fn focus(&mut self, center: Vec3, radius: f32) {
        if center.is_finite() {
            self.target = center;
        }
        if radius.is_finite() {
            self.distance = (radius.max(1.0) * 1.15 / (CAMERA_FOV * 0.5).sin())
                .clamp(MIN_DISTANCE, MAX_DISTANCE);
        }
    }

    pub fn set_scene_radius(&mut self, radius: f32) {
        if radius.is_finite() && radius > 0.0 {
            self.scene_radius = radius;
        }
    }

    pub fn scene_radius(&self) -> f32 {
        self.scene_radius
    }

    pub fn top_view(&mut self) {
        self.pitch = MAX_PITCH;
    }

    pub fn set_angles(&mut self, yaw: f32, pitch: f32) {
        if yaw.is_finite() && pitch.is_finite() {
            self.yaw = yaw.rem_euclid(std::f32::consts::TAU);
            self.pitch = pitch.clamp(-MAX_PITCH, MAX_PITCH);
        }
    }

    pub fn clip_planes(&self) -> (f32, f32) {
        // Shaders write logarithmic depth, so a tiny near plane costs no
        // precision; it only needs to avoid clipping nearby geometry.
        let near = (self.distance * 0.001).clamp(0.05, 5.0);
        let far = (self.scene_radius * 10.0 + self.distance * 4.0).max(near * 1_000.0);
        (near, far)
    }

    pub fn projection(&self, aspect: f32) -> Mat4 {
        let aspect = if aspect.is_finite() && aspect > 0.0 {
            aspect
        } else {
            1.0
        };
        let (near, far) = self.clip_planes();
        Mat4::perspective_rh_gl(CAMERA_FOV, aspect, near, far)
    }

    pub fn view(&self) -> Mat4 {
        let up = if self.pitch.abs() > MAX_PITCH - 0.01 {
            // Looking straight down: use the yaw heading as screen-up.
            -self.offset_direction_flat()
        } else {
            Vec3::Y
        };
        Mat4::look_at_rh(self.position(), self.target, up)
    }

    pub fn view_projection(&self, aspect: f32) -> Mat4 {
        self.projection(aspect) * self.view()
    }

    /// World-space ray through a CSS-pixel position in a viewport.
    pub fn ray(&self, x: f32, y: f32, width: f32, height: f32) -> (Vec3, Vec3) {
        let ndc_x = x / width.max(1.0) * 2.0 - 1.0;
        let ndc_y = 1.0 - y / height.max(1.0) * 2.0;
        let inverse = self.view_projection(width / height.max(1.0)).inverse();
        let near = inverse * Vec4::new(ndc_x, ndc_y, -1.0, 1.0);
        let far = inverse * Vec4::new(ndc_x, ndc_y, 1.0, 1.0);
        let near = near.truncate() / near.w;
        let far = far.truncate() / far.w;
        (near, (far - near).normalize_or(self.forward()))
    }

    /// Projects a world point to CSS pixels; `None` when behind the camera.
    pub fn project(&self, point: Vec3, width: f32, height: f32) -> Option<(f32, f32, f32)> {
        let clip = self.view_projection(width / height.max(1.0)) * point.extend(1.0);
        if !clip.is_finite() || clip.w <= 1e-6 {
            return None;
        }
        let ndc = clip.truncate() / clip.w;
        Some((
            (ndc.x * 0.5 + 0.5) * width,
            (0.5 - ndc.y * 0.5) * height,
            clip.w,
        ))
    }

    pub fn target(&self) -> Vec3 {
        self.target
    }

    pub fn distance(&self) -> f32 {
        self.distance
    }

    pub fn yaw(&self) -> f32 {
        self.yaw
    }

    pub fn pitch(&self) -> f32 {
        self.pitch
    }

    fn offset_direction(&self) -> Vec3 {
        let (pitch_sin, pitch_cos) = self.pitch.sin_cos();
        let (yaw_sin, yaw_cos) = self.yaw.sin_cos();
        Vec3::new(yaw_cos * pitch_cos, pitch_sin, yaw_sin * pitch_cos)
    }

    fn offset_direction_flat(&self) -> Vec3 {
        let (yaw_sin, yaw_cos) = self.yaw.sin_cos();
        Vec3::new(yaw_cos, 0.0, yaw_sin)
    }

    pub fn forward(&self) -> Vec3 {
        -self.offset_direction()
    }

    pub fn right(&self) -> Vec3 {
        let flat = -self.offset_direction_flat();
        flat.cross(Vec3::Y).normalize_or(Vec3::X)
    }

    pub fn position(&self) -> Vec3 {
        self.target + self.offset_direction() * self.distance
    }

    /// World units covering `pixels` at `point` for a viewport height.
    pub fn world_per_pixel(&self, point: Vec3, viewport_height: f32) -> f32 {
        let depth = (point - self.position())
            .dot(self.forward())
            .max(self.clip_planes().0);
        2.0 * depth * (CAMERA_FOV * 0.5).tan() / viewport_height.max(1.0)
    }
}

pub const SELECTED_COLOR: [f32; 4] = [1.0, 0.78, 0.1, 1.0];
const ROUTE_COLOR: [f32; 4] = [0.48, 0.68, 1.0, 1.0];
const CHECKPOINT_COLOR: [f32; 4] = [1.0, 0.62, 0.12, 1.0];
const CENTERLINE_COLOR: [f32; 4] = [0.12, 0.82, 0.92, 1.0];
const START_COLOR: [f32; 4] = [0.22, 0.95, 0.40, 1.0];
const AREA_COLOR: [f32; 4] = [0.24, 0.64, 0.94, 1.0];
const CAMERA_COLOR: [f32; 4] = [0.84, 0.38, 0.94, 1.0];
const RESPAWN_COLOR: [f32; 4] = [1.0, 0.32, 0.24, 1.0];

pub fn category_color(category: &str) -> [f32; 4] {
    match category {
        "Items" => [1.0, 0.78, 0.22, 1.0],
        "Track Feature" => [0.35, 0.72, 1.0, 1.0],
        "Scenery" => [0.42, 0.86, 0.45, 1.0],
        "Hazard" => [1.0, 0.38, 0.3, 1.0],
        "Traffic" => [0.98, 0.58, 0.24, 1.0],
        "Effects" => [0.8, 0.55, 1.0, 1.0],
        "Generator" => [0.62, 0.62, 0.98, 1.0],
        "Gameplay" => [0.3, 0.9, 0.85, 1.0],
        "Water" => [0.3, 0.6, 1.0, 1.0],
        "Audio" => [0.95, 0.5, 0.75, 1.0],
        _ => [0.66, 0.68, 0.72, 1.0],
    }
}

fn with_alpha(color: [f32; 4], alpha: f32) -> [f32; 4] {
    [color[0], color[1], color[2], alpha]
}

/// Overall extents used for framing, grid sizing, and marker sizing.
pub fn scene_bounds(
    course: &BolDocument,
    collision: Option<&Bco>,
    model_bounds: Option<Bounds>,
) -> Option<Bounds> {
    let mut bounds = model_bounds;
    if let Some(collision) = collision {
        for vertex in &collision.vertices {
            include_point(&mut bounds, Vec3::from_array(*vertex));
        }
    }
    for entity in course.scene_entities() {
        include_point(&mut bounds, Vec3::from(entity.position));
        if let Some(secondary) = entity.secondary {
            include_point(&mut bounds, Vec3::from(secondary));
        }
    }
    bounds
}

/// Marker size in world units for helpers that have no authored size.
pub fn marker_size(bounds: Option<Bounds>) -> f32 {
    bounds.map_or(40.0, |bounds| (bounds.radius() * 0.0025).clamp(10.0, 300.0))
}

/// Grid and collision; rebuilt only when collision or bounds change.
pub fn build_static_layers(collision: Option<&Bco>, bounds: Option<Bounds>) -> Vec<SceneLayer> {
    let mut builder = SceneBuilder::default();
    add_grid(builder.layer(LayerKind::Grid, Primitive::Lines), bounds);
    if let Some(collision) = collision {
        add_collision(
            builder.layer(LayerKind::Collision, Primitive::Triangles),
            collision,
        );
    }
    builder.finish()
}

pub struct HelperInput<'a> {
    pub course: &'a BolDocument,
    pub selected: Option<(&'a str, usize)>,
    /// Local model bounds per object index, when a model is drawn for it.
    pub object_models: &'a [Option<Bounds>],
    pub marker: f32,
}

/// Local bounds of the proxy box drawn for an object without a model.
pub fn object_proxy_bounds(marker: f32) -> Bounds {
    Bounds {
        min: Vec3::new(-marker, 0.0, -marker),
        max: Vec3::new(marker, marker * 2.0, marker),
    }
}

/// Editor markers for every BOL record; rebuilt on edits and selection.
pub fn build_helper_layers(input: &HelperInput<'_>) -> Vec<SceneLayer> {
    let course = input.course;
    let selected = input.selected;
    let marker = input.marker;
    let mut builder = SceneBuilder::default();

    for path in course.scene_paths() {
        let kind = match path.kind {
            "centerline" => LayerKind::Centerline,
            "checkpoint" => LayerKind::Checkpoints,
            _ => LayerKind::Routes,
        };
        let is_selected = match path.kind {
            "route_point" => {
                selection_matches(selected, "path", path.index)
                    || selected.is_some_and(|(kind, index)| {
                        kind == "route_point"
                            && course.paths.get(path.index).is_some_and(|route| {
                                let start = usize::from(route.point_start);
                                (start..start + path.points.len()).contains(&index)
                            })
                    })
            }
            "centerline" | "checkpoint" => selected.is_some_and(|(kind, index)| {
                kind == path.kind && (path.index..path.index + path.points.len()).contains(&index)
            }),
            _ => false,
        };
        let color = if is_selected {
            SELECTED_COLOR
        } else {
            match kind {
                LayerKind::Centerline => CENTERLINE_COLOR,
                LayerKind::Checkpoints => with_alpha(CHECKPOINT_COLOR, 0.55),
                _ => ROUTE_COLOR,
            }
        };
        let points: Vec<Vec3> = path.points.iter().copied().map(Vec3::from).collect();
        push_polyline(
            builder.layer(kind, Primitive::Lines),
            &points,
            path.closed,
            color,
        );
        if kind != LayerKind::Checkpoints {
            push_path_arrows(
                builder.layer(kind, Primitive::Triangles),
                &points,
                path.closed,
                marker * 0.9,
                color,
            );
        }
    }

    for (index, object) in course.objects.iter().enumerate() {
        let is_selected = selection_matches(selected, "object", index);
        let position = Vec3::from(object.position);
        if !position.is_finite() {
            continue;
        }
        let matrix = transform::model_matrix(
            position,
            decode_direction(object.forward),
            decode_direction(object.up),
            Vec3::from(object.scale),
        );
        let category = catalog::object_info(object.id).map_or("Unknown", |info| info.category);
        let color = if is_selected {
            SELECTED_COLOR
        } else {
            category_color(category)
        };
        match input.object_models.get(index).copied().flatten() {
            Some(bounds) => {
                if is_selected {
                    push_wire_box(
                        builder.layer(LayerKind::Objects, Primitive::Lines),
                        matrix,
                        bounds,
                        SELECTED_COLOR,
                    );
                }
            }
            None => {
                let rigid = transform::model_matrix(
                    position,
                    decode_direction(object.forward),
                    decode_direction(object.up),
                    Vec3::ONE,
                );
                let bounds = object_proxy_bounds(marker);
                push_solid_box(
                    builder.layer(LayerKind::Objects, Primitive::Triangles),
                    rigid,
                    bounds,
                    with_alpha(color, 0.55),
                );
                push_wire_box(
                    builder.layer(LayerKind::Objects, Primitive::Lines),
                    rigid,
                    bounds,
                    color,
                );
                push_flat_arrow(
                    builder.layer(LayerKind::Objects, Primitive::Triangles),
                    rigid,
                    Vec3::new(0.0, marker * 0.05, marker),
                    marker * 1.4,
                    marker * 0.9,
                    color,
                );
            }
        }
    }

    for (index, checkpoint) in course
        .checkpoint_groups
        .iter()
        .flat_map(|group| group.points.iter())
        .enumerate()
    {
        let is_selected = selection_matches(selected, "checkpoint", index);
        let start = Vec3::from(checkpoint.start);
        let end = Vec3::from(checkpoint.end);
        if !start.is_finite() || !end.is_finite() {
            continue;
        }
        let color = if is_selected {
            SELECTED_COLOR
        } else {
            CHECKPOINT_COLOR
        };
        let height = Vec3::Y * marker * 6.0;
        push_quad(
            builder.layer(LayerKind::Checkpoints, Primitive::Triangles),
            [start, end, end + height, start + height],
            with_alpha(color, if is_selected { 0.35 } else { 0.14 }),
        );
        let lines = builder.layer(LayerKind::Checkpoints, Primitive::Lines);
        push_line(lines, start, end, color);
        push_line(lines, start, start + height, color);
        push_line(lines, end, end + height, color);
        push_line(lines, start + height, end + height, with_alpha(color, 0.5));
        push_diamond(
            builder.layer(LayerKind::Checkpoints, Primitive::Triangles),
            start,
            marker * 0.6,
            color,
        );
    }

    for (index, point) in course.route_points.iter().enumerate() {
        let is_selected = selection_matches(selected, "route_point", index);
        push_diamond(
            builder.layer(LayerKind::Routes, Primitive::Triangles),
            Vec3::from(point.position),
            marker * if is_selected { 1.0 } else { 0.55 },
            if is_selected {
                SELECTED_COLOR
            } else {
                ROUTE_COLOR
            },
        );
    }

    for (index, point) in course.centerline.iter().enumerate() {
        let is_selected = selection_matches(selected, "centerline", index);
        push_diamond(
            builder.layer(LayerKind::Centerline, Primitive::Triangles),
            Vec3::from(point.position),
            marker * if is_selected { 1.0 } else { 0.5 },
            if is_selected {
                SELECTED_COLOR
            } else {
                CENTERLINE_COLOR
            },
        );
    }

    for (index, start) in course.starts.iter().enumerate() {
        let is_selected = selection_matches(selected, "start", index);
        let color = if is_selected {
            SELECTED_COLOR
        } else {
            START_COLOR
        };
        let matrix = transform::model_matrix(
            Vec3::from(start.position),
            decode_direction(start.forward),
            decode_direction(start.up),
            Vec3::ONE,
        );
        // The start line spans the track; show it as a bar with a heading arrow.
        let width = marker * 12.0 * start.scale[0].abs().max(0.1);
        push_solid_box(
            builder.layer(LayerKind::Starts, Primitive::Triangles),
            matrix,
            Bounds {
                min: Vec3::new(-width, 0.0, -marker * 0.4),
                max: Vec3::new(width, marker * 0.3, marker * 0.4),
            },
            with_alpha(color, 0.8),
        );
        push_flat_arrow(
            builder.layer(LayerKind::Starts, Primitive::Triangles),
            matrix,
            Vec3::new(0.0, marker * 0.35, marker * 0.4),
            marker * 5.0,
            marker * 2.4,
            color,
        );
    }

    for (index, respawn) in course.respawns.iter().enumerate() {
        let is_selected = selection_matches(selected, "respawn", index);
        let color = if is_selected {
            SELECTED_COLOR
        } else {
            RESPAWN_COLOR
        };
        let matrix = transform::model_matrix(
            Vec3::from(respawn.position),
            decode_direction(respawn.forward),
            decode_direction(respawn.up),
            Vec3::ONE,
        );
        push_flat_arrow(
            builder.layer(LayerKind::Respawns, Primitive::Triangles),
            matrix,
            Vec3::new(0.0, marker * 0.2, -marker),
            marker * 3.2,
            marker * 2.0,
            color,
        );
        push_line(
            builder.layer(LayerKind::Respawns, Primitive::Lines),
            matrix.transform_point3(Vec3::ZERO),
            matrix.transform_point3(Vec3::Y * marker * 4.0),
            color,
        );
        push_diamond(
            builder.layer(LayerKind::Respawns, Primitive::Triangles),
            matrix.transform_point3(Vec3::Y * marker * 4.0),
            marker * 0.7,
            color,
        );
    }

    for (index, area) in course.areas.iter().enumerate() {
        let is_selected = selection_matches(selected, "area", index);
        let color = if is_selected {
            SELECTED_COLOR
        } else {
            AREA_COLOR
        };
        let matrix = transform::model_matrix(
            Vec3::from(area.position),
            decode_direction(area.forward),
            decode_direction(area.up),
            Vec3::ONE,
        );
        let bounds = area_bounds(area.scale);
        if area.shape == 1 {
            push_cylinder(
                &mut builder,
                LayerKind::Areas,
                matrix,
                bounds.max.x,
                bounds.min.y,
                bounds.max.y,
                color,
                is_selected,
            );
        } else {
            push_solid_box(
                builder.layer(LayerKind::Areas, Primitive::Triangles),
                matrix,
                bounds,
                with_alpha(color, if is_selected { 0.22 } else { 0.1 }),
            );
            push_wire_box(
                builder.layer(LayerKind::Areas, Primitive::Lines),
                matrix,
                bounds,
                color,
            );
        }
    }

    for (index, camera) in course.cameras.iter().enumerate() {
        let is_selected = selection_matches(selected, "camera", index);
        let color = if is_selected {
            SELECTED_COLOR
        } else {
            CAMERA_COLOR
        };
        let position = Vec3::from(camera.position);
        let target = Vec3::from(camera.target_position);
        let forward = (target - position).try_normalize().unwrap_or(Vec3::Z);
        let matrix = transform::model_matrix(position, forward, Vec3::Y, Vec3::ONE);
        push_frustum(&mut builder, matrix, marker * 2.5, color);
        push_line(
            builder.layer(LayerKind::Cameras, Primitive::Lines),
            position,
            target,
            with_alpha(color, 0.6),
        );
        push_diamond(
            builder.layer(LayerKind::Cameras, Primitive::Triangles),
            target,
            marker * 0.5,
            with_alpha(color, 0.8),
        );
    }

    builder.finish()
}

/// `Course::Area` extents: x/z are `scale * 50` half-widths, y spans
/// `-100 ..= scale.y * 100`.
pub fn area_bounds(scale: [f32; 3]) -> Bounds {
    let x = (scale[0] * 50.0).abs();
    let z = (scale[2] * 50.0).abs();
    Bounds {
        min: Vec3::new(-x, -100.0, -z),
        max: Vec3::new(x, (scale[1] * 100.0).max(-99.0), z),
    }
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum GizmoMode {
    Translate,
    Rotate,
    Scale,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum GizmoHandle {
    X,
    Y,
    Z,
    PlaneXz,
    Center,
}

impl GizmoHandle {
    pub fn axis(self) -> Option<Vec3> {
        match self {
            GizmoHandle::X => Some(Vec3::X),
            GizmoHandle::Y => Some(Vec3::Y),
            GizmoHandle::Z => Some(Vec3::Z),
            _ => None,
        }
    }

    pub fn name(self) -> &'static str {
        match self {
            GizmoHandle::X => "x",
            GizmoHandle::Y => "y",
            GizmoHandle::Z => "z",
            GizmoHandle::PlaneXz => "xz",
            GizmoHandle::Center => "center",
        }
    }
}

const AXIS_COLORS: [[f32; 4]; 3] = [
    [0.95, 0.27, 0.27, 1.0],
    [0.42, 0.88, 0.3, 1.0],
    [0.3, 0.52, 1.0, 1.0],
];
const ACTIVE_COLOR: [f32; 4] = [1.0, 0.9, 0.2, 1.0];

pub fn build_gizmo(
    mode: GizmoMode,
    center: Vec3,
    size: f32,
    active: Option<GizmoHandle>,
) -> Vec<SceneLayer> {
    let mut builder = SceneBuilder::default();
    let handles = [GizmoHandle::X, GizmoHandle::Y, GizmoHandle::Z];
    for (axis_index, handle) in handles.into_iter().enumerate() {
        let axis = handle.axis().unwrap();
        let color = if active == Some(handle) {
            ACTIVE_COLOR
        } else {
            AXIS_COLORS[axis_index]
        };
        let (side_a, side_b) = perpendiculars(axis);
        match mode {
            GizmoMode::Translate | GizmoMode::Scale => {
                let shaft = size * 0.018;
                let tip = center + axis * size;
                push_prism(
                    builder.layer(LayerKind::Gizmo, Primitive::Triangles),
                    center + axis * size * 0.12,
                    tip,
                    side_a * shaft,
                    side_b * shaft,
                    color,
                );
                if mode == GizmoMode::Translate {
                    push_cone(
                        builder.layer(LayerKind::Gizmo, Primitive::Triangles),
                        tip,
                        tip + axis * size * 0.22,
                        side_a * size * 0.07,
                        side_b * size * 0.07,
                        color,
                    );
                } else {
                    let half = size * 0.06;
                    push_aabb(
                        builder.layer(LayerKind::Gizmo, Primitive::Triangles),
                        tip - Vec3::splat(half),
                        tip + Vec3::splat(half),
                        color,
                    );
                }
            }
            GizmoMode::Rotate => {
                push_ring(
                    builder.layer(LayerKind::Gizmo, Primitive::Triangles),
                    center,
                    side_a,
                    side_b,
                    size,
                    size * 0.035,
                    color,
                );
            }
        }
    }
    match mode {
        GizmoMode::Translate => {
            let color = if active == Some(GizmoHandle::PlaneXz) {
                with_alpha(ACTIVE_COLOR, 0.6)
            } else {
                [0.95, 0.85, 0.35, 0.35]
            };
            let a = size * 0.2;
            let b = size * 0.45;
            push_quad(
                builder.layer(LayerKind::Gizmo, Primitive::Triangles),
                [
                    center + Vec3::new(a, 0.0, a),
                    center + Vec3::new(b, 0.0, a),
                    center + Vec3::new(b, 0.0, b),
                    center + Vec3::new(a, 0.0, b),
                ],
                color,
            );
        }
        GizmoMode::Scale => {
            let color = if active == Some(GizmoHandle::Center) {
                ACTIVE_COLOR
            } else {
                [0.92, 0.92, 0.92, 1.0]
            };
            let half = size * 0.08;
            push_aabb(
                builder.layer(LayerKind::Gizmo, Primitive::Triangles),
                center - Vec3::splat(half),
                center + Vec3::splat(half),
                color,
            );
        }
        GizmoMode::Rotate => {}
    }
    builder.finish()
}

pub fn perpendiculars(axis: Vec3) -> (Vec3, Vec3) {
    if axis == Vec3::X {
        (Vec3::Y, Vec3::Z)
    } else if axis == Vec3::Y {
        (Vec3::Z, Vec3::X)
    } else if axis == Vec3::Z {
        (Vec3::X, Vec3::Y)
    } else {
        let a = axis.any_orthonormal_vector();
        (a, axis.cross(a))
    }
}

#[derive(Default)]
struct SceneBuilder {
    layers: Vec<SceneLayer>,
}

impl SceneBuilder {
    fn layer(&mut self, kind: LayerKind, primitive: Primitive) -> &mut SceneLayer {
        if let Some(index) = self
            .layers
            .iter()
            .position(|layer| layer.kind == kind && layer.primitive == primitive)
        {
            return &mut self.layers[index];
        }
        self.layers.push(SceneLayer {
            kind,
            primitive,
            vertices: Vec::new(),
            indices: Vec::new(),
        });
        self.layers.last_mut().unwrap()
    }

    fn finish(mut self) -> Vec<SceneLayer> {
        self.layers.retain(|layer| !layer.vertices.is_empty());
        self.layers
    }
}

pub fn bmd_bounds(model: &crate::formats::bmd::BmdModel) -> Option<Bounds> {
    let mut bounds = None;
    for mesh in &model.meshes {
        for vertex in &mesh.vertices {
            include_point(&mut bounds, Vec3::from_array(vertex.position));
        }
    }
    bounds
}

pub fn include_point(bounds: &mut Option<Bounds>, point: Vec3) {
    if !point.is_finite() {
        return;
    }
    if let Some(bounds) = bounds {
        bounds.min = bounds.min.min(point);
        bounds.max = bounds.max.max(point);
    } else {
        *bounds = Some(Bounds {
            min: point,
            max: point,
        });
    }
}

fn selection_matches(selected: Option<(&str, usize)>, kind: &str, index: usize) -> bool {
    selected.is_some_and(|(selected_kind, selected_index)| {
        selected_kind == kind && selected_index == index
    })
}

fn add_collision(layer: &mut SceneLayer, collision: &Bco) {
    for triangle in &collision.triangles {
        let points = triangle.indices.map(|index| {
            usize::try_from(index)
                .ok()
                .and_then(|index| collision.vertices.get(index))
                .copied()
                .map(Vec3::from_array)
                .filter(|point| point.is_finite())
        });
        let [Some(a), Some(b), Some(c)] = points else {
            continue;
        };
        let [red, green, blue, _] = triangle.surface_color();
        push_triangle(layer, [a, b, c], [red, green, blue, 1.0]);
    }
}

fn add_grid(layer: &mut SceneLayer, bounds: Option<Bounds>) {
    const MINOR: [f32; 4] = [0.36, 0.4, 0.46, 0.25];
    const MAJOR: [f32; 4] = [0.5, 0.55, 0.62, 0.5];

    let (min, max, y) = if let Some(bounds) = bounds {
        let span = bounds.max - bounds.min;
        let step = grid_step(span.x.abs().max(span.z.abs()) / 20.0);
        let min = Vec3::new(
            (bounds.min.x / step).floor() * step - 2.0 * step,
            0.0,
            (bounds.min.z / step).floor() * step - 2.0 * step,
        );
        let max = Vec3::new(
            (bounds.max.x / step).ceil() * step + 2.0 * step,
            0.0,
            (bounds.max.z / step).ceil() * step + 2.0 * step,
        );
        (min, max, bounds.min.y - step * 0.05)
    } else {
        (
            Vec3::new(-100.0, 0.0, -100.0),
            Vec3::new(100.0, 0.0, 100.0),
            0.0,
        )
    };

    let step = if bounds.is_some() {
        grid_step((max.x - min.x).max(max.z - min.z) / 24.0)
    } else {
        10.0
    };
    add_grid_axis(layer, min, max, step, y, true, MINOR, MAJOR);
    add_grid_axis(layer, min, max, step, y, false, MINOR, MAJOR);
}

pub fn grid_step(target: f32) -> f32 {
    let target = target.max(1.0);
    let power = 10.0_f32.powf(target.log10().floor());
    let normalized = target / power;
    let multiplier = if normalized <= 1.0 {
        1.0
    } else if normalized <= 2.0 {
        2.0
    } else if normalized <= 5.0 {
        5.0
    } else {
        10.0
    };
    multiplier * power
}

#[allow(clippy::too_many_arguments)]
fn add_grid_axis(
    layer: &mut SceneLayer,
    min: Vec3,
    max: Vec3,
    step: f32,
    y: f32,
    along_x: bool,
    minor: [f32; 4],
    major: [f32; 4],
) {
    let axis_min = if along_x { min.x } else { min.z };
    let axis_max = if along_x { max.x } else { max.z };
    let first = (axis_min / step).round() as i64;
    let count = ((axis_max - axis_min) / step).round().clamp(0.0, 64.0) as i64;
    for offset in 0..=count {
        let grid_index = first + offset;
        let coordinate = grid_index as f32 * step;
        let color = if grid_index % 5 == 0 { major } else { minor };
        let (a, b) = if along_x {
            (
                Vec3::new(coordinate, y, min.z),
                Vec3::new(coordinate, y, max.z),
            )
        } else {
            (
                Vec3::new(min.x, y, coordinate),
                Vec3::new(max.x, y, coordinate),
            )
        };
        push_line(layer, a, b, color);
    }
}

fn push_polyline(layer: &mut SceneLayer, points: &[Vec3], closed: bool, color: [f32; 4]) {
    let points: Vec<Vec3> = points.iter().copied().filter(|p| p.is_finite()).collect();
    if points.len() < 2 {
        return;
    }
    let Some(base) = index_base(layer, points.len()) else {
        return;
    };
    layer.vertices.extend(points.iter().map(|point| Vertex {
        position: point.to_array(),
        color,
    }));
    for index in 0..points.len() - 1 {
        let index = base + index as u32;
        layer.indices.extend_from_slice(&[index, index + 1]);
    }
    if closed && points.len() > 2 {
        layer
            .indices
            .extend_from_slice(&[base + points.len() as u32 - 1, base]);
    }
}

/// Small upright chevrons at segment midpoints showing travel direction.
fn push_path_arrows(
    layer: &mut SceneLayer,
    points: &[Vec3],
    closed: bool,
    size: f32,
    color: [f32; 4],
) {
    let count = points.len();
    if count < 2 {
        return;
    }
    let segments = if closed { count } else { count - 1 };
    for segment in 0..segments {
        let a = points[segment];
        let b = points[(segment + 1) % count];
        let direction = b - a;
        if direction.length() < size * 3.0 {
            continue;
        }
        let forward = direction.normalize();
        let matrix = transform::model_matrix((a + b) * 0.5, forward, Vec3::Y, Vec3::ONE);
        push_flat_arrow(
            layer,
            matrix,
            Vec3::new(0.0, 0.0, -size * 0.5),
            size,
            size * 0.9,
            color,
        );
    }
}

pub fn push_line(layer: &mut SceneLayer, a: Vec3, b: Vec3, color: [f32; 4]) {
    if !a.is_finite() || !b.is_finite() {
        return;
    }
    let Some(base) = index_base(layer, 2) else {
        return;
    };
    layer.vertices.extend([a, b].map(|point| Vertex {
        position: point.to_array(),
        color,
    }));
    layer.indices.extend_from_slice(&[base, base + 1]);
}

fn push_triangle(layer: &mut SceneLayer, points: [Vec3; 3], color: [f32; 4]) {
    let Some(base) = index_base(layer, 3) else {
        return;
    };
    layer.vertices.extend(points.map(|point| Vertex {
        position: point.to_array(),
        color,
    }));
    layer.indices.extend_from_slice(&[base, base + 1, base + 2]);
}

fn push_quad(layer: &mut SceneLayer, points: [Vec3; 4], color: [f32; 4]) {
    push_indexed(layer, &points, &[0, 1, 2, 0, 2, 3], color);
}

fn push_diamond(layer: &mut SceneLayer, center: Vec3, radius: f32, color: [f32; 4]) {
    let x = Vec3::X * radius;
    let y = Vec3::Y * radius * 1.35;
    let z = Vec3::Z * radius;
    let vertices = [
        center + x,
        center - x,
        center + y,
        center - y,
        center + z,
        center - z,
    ];
    const INDICES: [u32; 24] = [
        2, 4, 0, 2, 0, 5, 2, 5, 1, 2, 1, 4, 3, 0, 4, 3, 5, 0, 3, 1, 5, 3, 4, 1,
    ];
    push_indexed(layer, &vertices, &INDICES, color);
}

fn box_corners(matrix: Mat4, bounds: Bounds) -> [Vec3; 8] {
    let (min, max) = (bounds.min, bounds.max);
    [
        Vec3::new(min.x, min.y, min.z),
        Vec3::new(max.x, min.y, min.z),
        Vec3::new(max.x, min.y, max.z),
        Vec3::new(min.x, min.y, max.z),
        Vec3::new(min.x, max.y, min.z),
        Vec3::new(max.x, max.y, min.z),
        Vec3::new(max.x, max.y, max.z),
        Vec3::new(min.x, max.y, max.z),
    ]
    .map(|corner| matrix.transform_point3(corner))
}

const BOX_TRIANGLES: [u32; 36] = [
    0, 1, 2, 0, 2, 3, 4, 6, 5, 4, 7, 6, 0, 4, 5, 0, 5, 1, 1, 5, 6, 1, 6, 2, 2, 6, 7, 2, 7, 3, 3, 7,
    4, 3, 4, 0,
];
const BOX_EDGES: [u32; 24] = [
    0, 1, 1, 2, 2, 3, 3, 0, 4, 5, 5, 6, 6, 7, 7, 4, 0, 4, 1, 5, 2, 6, 3, 7,
];

fn push_solid_box(layer: &mut SceneLayer, matrix: Mat4, bounds: Bounds, color: [f32; 4]) {
    push_indexed(layer, &box_corners(matrix, bounds), &BOX_TRIANGLES, color);
}

fn push_wire_box(layer: &mut SceneLayer, matrix: Mat4, bounds: Bounds, color: [f32; 4]) {
    push_indexed(layer, &box_corners(matrix, bounds), &BOX_EDGES, color);
}

fn push_aabb(layer: &mut SceneLayer, min: Vec3, max: Vec3, color: [f32; 4]) {
    push_solid_box(layer, Mat4::IDENTITY, Bounds { min, max }, color);
}

/// A flat arrow on the local XZ plane pointing along +Z from `base`.
fn push_flat_arrow(
    layer: &mut SceneLayer,
    matrix: Mat4,
    base: Vec3,
    length: f32,
    width: f32,
    color: [f32; 4],
) {
    let shaft = width * 0.22;
    let head = length * 0.45;
    let local = [
        base + Vec3::new(-shaft, 0.0, 0.0),
        base + Vec3::new(shaft, 0.0, 0.0),
        base + Vec3::new(shaft, 0.0, length - head),
        base + Vec3::new(-shaft, 0.0, length - head),
        base + Vec3::new(-width * 0.5, 0.0, length - head),
        base + Vec3::new(width * 0.5, 0.0, length - head),
        base + Vec3::new(0.0, 0.0, length),
    ];
    let points = local.map(|point| matrix.transform_point3(point));
    push_indexed(layer, &points, &[0, 1, 2, 0, 2, 3, 4, 5, 6], color);
}

#[allow(clippy::too_many_arguments)]
fn push_cylinder(
    builder: &mut SceneBuilder,
    kind: LayerKind,
    matrix: Mat4,
    radius: f32,
    bottom: f32,
    top: f32,
    color: [f32; 4],
    selected: bool,
) {
    const SIDES: usize = 24;
    let ring = |y: f32| -> Vec<Vec3> {
        (0..SIDES)
            .map(|side| {
                let angle = side as f32 / SIDES as f32 * std::f32::consts::TAU;
                matrix.transform_point3(Vec3::new(angle.cos() * radius, y, angle.sin() * radius))
            })
            .collect()
    };
    let lower = ring(bottom);
    let upper = ring(top);
    let fill = with_alpha(color, if selected { 0.22 } else { 0.1 });
    for side in 0..SIDES {
        let next = (side + 1) % SIDES;
        push_quad(
            builder.layer(kind, Primitive::Triangles),
            [lower[side], lower[next], upper[next], upper[side]],
            fill,
        );
        let lines = builder.layer(kind, Primitive::Lines);
        push_line(lines, lower[side], lower[next], color);
        push_line(lines, upper[side], upper[next], color);
        if side % 6 == 0 {
            push_line(lines, lower[side], upper[side], color);
        }
    }
}

fn push_frustum(builder: &mut SceneBuilder, matrix: Mat4, size: f32, color: [f32; 4]) {
    let apex = matrix.transform_point3(Vec3::ZERO);
    let corners = [(-1.0, -0.6), (1.0, -0.6), (1.0, 0.6), (-1.0, 0.6)]
        .map(|(x, y)| matrix.transform_point3(Vec3::new(x * size * 0.7, y * size * 0.7, size)));
    let layer = builder.layer(LayerKind::Cameras, Primitive::Triangles);
    for side in 0..4 {
        push_triangle(
            layer,
            [apex, corners[side], corners[(side + 1) % 4]],
            with_alpha(color, 0.35),
        );
    }
    let lines = builder.layer(LayerKind::Cameras, Primitive::Lines);
    for side in 0..4 {
        push_line(lines, apex, corners[side], color);
        push_line(lines, corners[side], corners[(side + 1) % 4], color);
    }
}

fn push_prism(layer: &mut SceneLayer, from: Vec3, to: Vec3, a: Vec3, b: Vec3, color: [f32; 4]) {
    let points = [
        from - a - b,
        from + a - b,
        from + a + b,
        from - a + b,
        to - a - b,
        to + a - b,
        to + a + b,
        to - a + b,
    ];
    push_indexed(layer, &points, &BOX_TRIANGLES, color);
}

fn push_cone(layer: &mut SceneLayer, base: Vec3, tip: Vec3, a: Vec3, b: Vec3, color: [f32; 4]) {
    const SIDES: usize = 12;
    let mut points = vec![tip, base];
    for side in 0..SIDES {
        let angle = side as f32 / SIDES as f32 * std::f32::consts::TAU;
        points.push(base + a * angle.cos() + b * angle.sin());
    }
    let mut indices = Vec::with_capacity(SIDES * 6);
    for side in 0..SIDES as u32 {
        let current = 2 + side;
        let next = 2 + (side + 1) % SIDES as u32;
        indices.extend_from_slice(&[0, current, next, 1, next, current]);
    }
    push_indexed(layer, &points, &indices, color);
}

fn push_ring(
    layer: &mut SceneLayer,
    center: Vec3,
    a: Vec3,
    b: Vec3,
    radius: f32,
    width: f32,
    color: [f32; 4],
) {
    const SEGMENTS: usize = 64;
    let normal = a.cross(b);
    let mut points = Vec::with_capacity(SEGMENTS * 4);
    for segment in 0..SEGMENTS {
        let angle = segment as f32 / SEGMENTS as f32 * std::f32::consts::TAU;
        let direction = a * angle.cos() + b * angle.sin();
        points.push(center + direction * (radius - width));
        points.push(center + direction * (radius + width));
        points.push(center + direction * radius + normal * width);
        points.push(center + direction * radius - normal * width);
    }
    let mut indices = Vec::with_capacity(SEGMENTS * 12);
    for segment in 0..SEGMENTS as u32 {
        let current = segment * 4;
        let next = ((segment + 1) % SEGMENTS as u32) * 4;
        // A flat band plus a perpendicular band, so the ring reads edge-on too.
        indices.extend_from_slice(&[current, current + 1, next + 1, current, next + 1, next]);
        indices.extend_from_slice(&[
            current + 2,
            current + 3,
            next + 3,
            current + 2,
            next + 3,
            next + 2,
        ]);
    }
    push_indexed(layer, &points, &indices, color);
}

fn push_indexed(layer: &mut SceneLayer, points: &[Vec3], indices: &[u32], color: [f32; 4]) {
    if points.iter().any(|point| !point.is_finite()) {
        return;
    }
    let Some(base) = index_base(layer, points.len()) else {
        return;
    };
    layer.vertices.extend(points.iter().map(|point| Vertex {
        position: point.to_array(),
        color,
    }));
    layer
        .indices
        .extend(indices.iter().map(|index| base + index));
}

fn index_base(layer: &SceneLayer, additional: usize) -> Option<u32> {
    let base = u32::try_from(layer.vertices.len()).ok()?;
    let last = base.checked_add(u32::try_from(additional).ok()?)?;
    if last == u32::MAX { None } else { Some(base) }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn layer(primitive: Primitive) -> SceneLayer {
        SceneLayer {
            kind: LayerKind::Objects,
            primitive,
            vertices: Vec::new(),
            indices: Vec::new(),
        }
    }

    const WHITE: [f32; 4] = [1.0; 4];

    #[test]
    fn polyline_indices_share_vertices_and_close_the_loop() {
        let mut layer = layer(Primitive::Lines);
        let points = [Vec3::ZERO, Vec3::X, Vec3::Y, Vec3::Z];
        push_polyline(&mut layer, &points, true, WHITE);
        assert_eq!(layer.vertices.len(), 4);
        assert_eq!(layer.indices, [0, 1, 1, 2, 2, 3, 3, 0]);

        push_line(&mut layer, Vec3::ZERO, Vec3::ONE, WHITE);
        assert_eq!(&layer.indices[8..], [4, 5]);
    }

    #[test]
    fn indexed_helpers_offset_each_mesh() {
        let mut layer = layer(Primitive::Triangles);
        push_triangle(&mut layer, [Vec3::ZERO, Vec3::X, Vec3::Y], WHITE);
        push_diamond(&mut layer, Vec3::ZERO, 2.0, WHITE);
        assert_eq!(layer.vertices.len(), 9);
        assert_eq!(&layer.indices[..3], [0, 1, 2]);
        assert_eq!(&layer.indices[3..9], [5, 7, 3, 5, 3, 8]);
        assert!(layer.indices.iter().all(|index| *index < 9));
    }

    #[test]
    fn grid_is_finite_and_scaled_to_content() {
        let bounds = Bounds {
            min: Vec3::new(1_000.0, 5.0, -40.0),
            max: Vec3::new(1_100.0, 15.0, 60.0),
        };
        let mut layer = layer(Primitive::Lines);
        add_grid(&mut layer, Some(bounds));
        assert!(!layer.vertices.is_empty());
        assert!(layer.vertices.iter().all(|vertex| {
            let point = Vec3::from_array(vertex.position);
            point.is_finite()
                && (900.0..=1_200.0).contains(&point.x)
                && (-100.0..=100.0).contains(&point.z)
                && point.y < bounds.min.y
        }));
    }

    #[test]
    fn camera_orbit_clamps_pitch_and_zoom_clamps_distance() {
        let mut camera = OrbitCamera::default();
        let original_distance = camera.position().distance(camera.target);
        camera.orbit(0.0, 100_000.0);
        assert_eq!(camera.pitch, MAX_PITCH);
        assert!((camera.position().distance(camera.target) - original_distance).abs() < 0.01);

        for _ in 0..2 {
            camera.zoom(-100_000.0);
        }
        assert_eq!(camera.distance, MIN_DISTANCE);
        for _ in 0..4 {
            camera.zoom(100_000.0);
        }
        assert_eq!(camera.distance, MAX_DISTANCE);
    }

    #[test]
    fn camera_frames_bounds_and_pans_in_camera_space() {
        let bounds = Bounds {
            min: Vec3::new(-10.0, -2.0, -20.0),
            max: Vec3::new(30.0, 8.0, 20.0),
        };
        let mut camera = OrbitCamera::default();
        camera.frame(bounds);
        assert_eq!(camera.target, bounds.center());
        assert!(camera.position().is_finite());

        let old_target = camera.target;
        camera.pan(10.0, -5.0, 800.0, 600.0);
        assert!(camera.target.is_finite());
        assert_ne!(camera.target, old_target);

        let matrix = camera.view_projection(800.0 / 600.0);
        assert!(matrix.is_finite());
        assert!(camera.view_projection(0.0).is_finite());
    }

    #[test]
    fn look_keeps_the_eye_fixed_and_fly_moves_forward() {
        let mut camera = OrbitCamera::default();
        let eye = camera.position();
        camera.look(120.0, -40.0);
        assert!((camera.position() - eye).length() < 1e-2);
        let forward = camera.forward();
        camera.fly(10.0, 0.0, 0.0);
        assert!((camera.position() - (eye + forward * 10.0)).length() < 1e-2);
    }

    #[test]
    fn rays_unproject_through_the_viewport_center() {
        let mut camera = OrbitCamera::default();
        camera.set_scene_radius(5_000.0);
        let (origin, direction) = camera.ray(400.0, 300.0, 800.0, 600.0);
        assert!((direction - camera.forward()).length() < 1e-3);
        let (x, y, _) = camera
            .project(origin + direction * 100.0, 800.0, 600.0)
            .unwrap();
        assert!((x - 400.0).abs() < 0.5 && (y - 300.0).abs() < 0.5);
        camera.top_view();
        assert!(camera.view().is_finite());
    }

    #[test]
    fn area_bounds_follow_course_area_scaling() {
        let bounds = area_bounds([2.0, 3.0, 4.0]);
        assert_eq!(bounds.min, Vec3::new(-100.0, -100.0, -200.0));
        assert_eq!(bounds.max, Vec3::new(100.0, 300.0, 200.0));
    }

    #[test]
    fn gizmo_builds_geometry_for_every_mode() {
        for mode in [GizmoMode::Translate, GizmoMode::Rotate, GizmoMode::Scale] {
            let layers = build_gizmo(mode, Vec3::ZERO, 100.0, Some(GizmoHandle::X));
            assert!(!layers.is_empty());
            assert!(layers.iter().all(|layer| layer.kind == LayerKind::Gizmo));
            assert!(layers.iter().all(|layer| {
                layer
                    .indices
                    .iter()
                    .all(|index| (*index as usize) < layer.vertices.len())
            }));
        }
    }
}
