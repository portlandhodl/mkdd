use std::cell::RefCell;
use std::collections::{HashMap, VecDeque};
use std::fmt::{Display, Write as _};
use std::ops::Range;
use std::rc::Rc;

use glam::{Mat4, Vec3};
use serde_json::{Value, json};
use wasm_bindgen::{JsCast, JsValue, prelude::wasm_bindgen};
use web_sys::HtmlCanvasElement;

use crate::catalog;
use crate::error::{FormatError, Result as FormatResult};
use crate::formats::bco::Bco;
use crate::formats::bmd::BmdModel;
use crate::formats::bol::{BolDocument, Inspection, MapObject};
use crate::formats::bti::BtiImage;
use crate::formats::compression::{self, Compression};
use crate::formats::{j3d, rarc::RarcArchive};
use crate::renderer::{LayerGroup, ModelInstance, ModelLayer, Renderer, ShadingMode};
use crate::scene::{
    self, Bounds, GizmoHandle, GizmoMode, HelperInput, LAYER_NAMES, LayerVisibility, OrbitCamera,
    bmd_bounds,
};
use crate::transform::{self, EntityTransform};

const HISTORY_LIMIT: usize = 64;
const LOOSE_ASSET_BASE: usize = 1_000_000;
const PICK_RADIUS_CSS_PX: f32 = 14.0;
const GIZMO_SIZE_PX: f32 = 110.0;
const GIZMO_PICK_PX: f32 = 9.0;
const FILE_HEX_PREVIEW_BYTES: usize = 256;

#[derive(Clone, Debug, Default)]
enum Selection {
    #[default]
    None,
    Course {
        kind: String,
        index: usize,
    },
    File(usize),
}

#[derive(Clone, Debug)]
struct LooseAsset {
    name: String,
    data: Vec<u8>,
    original: Vec<u8>,
}

#[derive(Clone, Debug)]
struct HistoryState {
    course: Option<BolDocument>,
    archive: Option<Vec<u8>>,
    archive_dirty: bool,
    collision: Option<Bco>,
    collision_archive_index: Option<usize>,
    course_model: Option<Rc<BmdModel>>,
    course_model_archive_index: Option<usize>,
    course_model_warning: Option<String>,
    course_model_bounds: Option<Bounds>,
    loose_assets: Vec<LooseAsset>,
    selection: Selection,
    archive_bol_index: Option<usize>,
}

#[wasm_bindgen]
pub struct Editor {
    course: Option<BolDocument>,
    archive: Option<RarcArchive>,
    collision: Option<Bco>,
    collision_archive_index: Option<usize>,
    course_model: Option<Rc<BmdModel>>,
    course_model_archive_index: Option<usize>,
    course_model_warning: Option<String>,
    course_model_bounds: Option<Bounds>,
    loose_assets: Vec<LooseAsset>,
    selection: Selection,
    archive_bol_index: Option<usize>,
    file_name: Option<String>,
    status: String,
    camera: OrbitCamera,
    scene_bounds: Option<Bounds>,
    static_bounds: Option<Option<Bounds>>,
    static_collision: Option<usize>,
    marker: f32,
    sky_model: Option<Rc<BmdModel>>,
    library: ModelLibrary,
    object_models: Vec<Option<(String, Bounds)>>,
    gizmo_mode: GizmoMode,
    gizmo_hidden: bool,
    hover_handle: Option<GizmoHandle>,
    drag: Option<DragState>,
    snap: SnapSettings,
    renderer: Renderer,
    canvas: HtmlCanvasElement,
    visibility: LayerVisibility,
    undo: VecDeque<HistoryState>,
    redo: VecDeque<HistoryState>,
    // A restored archive's bytes can themselves represent a dirty state. This
    // records that baseline independently of changes relative to those bytes.
    archive_base_dirty: bool,
}

#[wasm_bindgen]
impl Editor {
    #[wasm_bindgen(constructor)]
    pub fn new(canvas_id: String) -> Result<Self, JsValue> {
        console_error_panic_hook::set_once();
        let window = web_sys::window()
            .ok_or_else(|| JsValue::from_str("The browser window is unavailable"))?;
        let document = window
            .document()
            .ok_or_else(|| JsValue::from_str("The browser document is unavailable"))?;
        let element = document.get_element_by_id(&canvas_id).ok_or_else(|| {
            JsValue::from_str(&format!("Canvas element '{canvas_id}' was not found"))
        })?;
        let canvas = element
            .dyn_into::<HtmlCanvasElement>()
            .map_err(|_| JsValue::from_str(&format!("Element '{canvas_id}' is not a canvas")))?;
        let renderer = Renderer::new(canvas.clone()).map_err(js_error)?;
        Ok(Self {
            course: None,
            archive: None,
            collision: None,
            collision_archive_index: None,
            course_model: None,
            course_model_archive_index: None,
            course_model_warning: None,
            course_model_bounds: None,
            loose_assets: Vec::new(),
            selection: Selection::None,
            archive_bol_index: None,
            file_name: None,
            status: "Ready. Open a BOL or ARC course to begin.".to_owned(),
            camera: OrbitCamera::default(),
            scene_bounds: None,
            static_bounds: None,
            static_collision: None,
            marker: 40.0,
            sky_model: None,
            library: ModelLibrary::default(),
            object_models: Vec::new(),
            gizmo_mode: GizmoMode::Translate,
            gizmo_hidden: false,
            hover_handle: None,
            drag: None,
            snap: SnapSettings::default(),
            renderer,
            canvas,
            visibility: LayerVisibility::default(),
            undo: VecDeque::new(),
            redo: VecDeque::new(),
            archive_base_dirty: false,
        })
    }

    pub fn open_file(&mut self, name: String, data: &[u8]) -> Result<String, JsValue> {
        self.open_file_inner(name, data).map_err(js_error)
    }

    pub fn load_related(&mut self, name: String, data: &[u8]) -> Result<String, JsValue> {
        self.load_related_inner(name, data).map_err(js_error)
    }

    pub fn snapshot(&self) -> Result<String, JsValue> {
        self.snapshot_json().map_err(js_error)
    }

    pub fn select(&mut self, kind: String, index: usize) -> Result<String, JsValue> {
        self.select_inner(&kind, index).map_err(js_error)
    }

    pub fn edit(
        &mut self,
        kind: String,
        index: usize,
        key: String,
        value: JsValue,
    ) -> Result<String, JsValue> {
        let value = js_value_to_json(&value)?;
        self.edit_inner(&kind, index, &key, value).map_err(js_error)
    }

    pub fn edit_header(&mut self, key: String, value: JsValue) -> Result<String, JsValue> {
        let value = js_value_to_json(&value)?;
        self.edit_inner("header", 0, &key, value).map_err(js_error)
    }

    pub fn add_object(&mut self) -> Result<String, JsValue> {
        self.add_object_of(1)
    }

    pub fn duplicate_selected(&mut self) -> Result<String, JsValue> {
        let index = self.selected_object_index().map_err(js_error)?;
        let course = self
            .course
            .as_ref()
            .ok_or_else(|| JsValue::from_str("Open a BOL course before duplicating an object"))?;
        let mut next = course.clone();
        let new_index = next.duplicate_object(index).map_err(js_error)?;
        if let Some(transform) = transform::read(&next, "object", new_index) {
            let offset = self.camera.right() * self.marker * 3.0;
            transform::write(&mut next, "object", new_index, &transform.translated(offset))
                .map_err(js_error)?;
        }
        let before = self.capture_history().map_err(js_error)?;
        self.course = Some(next);
        self.selection = Selection::Course {
            kind: "object".to_owned(),
            index: new_index,
        };
        self.push_undo(before);
        self.finish_scene_change(format!("Duplicated object {index} as object {new_index}"))
            .map_err(js_error)
    }

    pub fn delete_selected(&mut self) -> Result<String, JsValue> {
        let index = self.selected_object_index().map_err(js_error)?;
        let course = self
            .course
            .as_ref()
            .ok_or_else(|| JsValue::from_str("Open a BOL course before deleting an object"))?;
        let mut next = course.clone();
        next.delete_object(index).map_err(js_error)?;
        let before = self.capture_history().map_err(js_error)?;
        self.course = Some(next);
        self.selection = Selection::None;
        self.push_undo(before);
        self.finish_scene_change(format!("Deleted object {index}"))
            .map_err(js_error)
    }

    pub fn undo(&mut self) -> Result<String, JsValue> {
        let Some(previous) = self.undo.pop_back() else {
            self.status = "Nothing to undo.".to_owned();
            return self.snapshot();
        };
        let current = match self.capture_history() {
            Ok(current) => current,
            Err(error) => {
                self.undo.push_back(previous);
                return Err(js_error(error));
            }
        };
        if let Err(error) = self.restore_history(previous) {
            return Err(js_error(error));
        }
        push_bounded(&mut self.redo, current);
        self.status = "Undid the last edit.".to_owned();
        self.rebuild_scene().map_err(js_error)?;
        self.snapshot()
    }

    pub fn redo(&mut self) -> Result<String, JsValue> {
        let Some(next) = self.redo.pop_back() else {
            self.status = "Nothing to redo.".to_owned();
            return self.snapshot();
        };
        let current = match self.capture_history() {
            Ok(current) => current,
            Err(error) => {
                self.redo.push_back(next);
                return Err(js_error(error));
            }
        };
        if let Err(error) = self.restore_history(next) {
            return Err(js_error(error));
        }
        push_bounded(&mut self.undo, current);
        self.status = "Redid the edit.".to_owned();
        self.rebuild_scene().map_err(js_error)?;
        self.snapshot()
    }

    pub fn export_bol(&self) -> Result<Vec<u8>, JsValue> {
        self.course
            .as_ref()
            .ok_or_else(|| JsValue::from_str("No BOL course is open"))?
            .export()
            .map_err(js_error)
    }

    pub fn export_original_bol(&self) -> Result<Vec<u8>, JsValue> {
        Ok(self
            .course
            .as_ref()
            .ok_or_else(|| JsValue::from_str("No BOL course is open"))?
            .original_bytes()
            .to_vec())
    }

    pub fn export_archive(&self) -> Result<Vec<u8>, JsValue> {
        let archive = self
            .archive
            .as_ref()
            .ok_or_else(|| JsValue::from_str("No RARC archive is open"))?;
        let bol_index = self
            .archive_bol_index
            .ok_or_else(|| JsValue::from_str("The open archive has no associated course BOL"))?;
        let bol = self
            .course
            .as_ref()
            .ok_or_else(|| JsValue::from_str("No BOL course is open"))?
            .export()
            .map_err(js_error)?;

        // RarcArchive intentionally is not Clone. Serializing and reparsing gives
        // us an independent archive without changing the live one or dropping
        // unrelated entries; replacement then injects the current edited BOL.
        let archived_bytes = archive.export().map_err(js_error)?;
        let mut exported = RarcArchive::new(archived_bytes).map_err(js_error)?;
        exported.replace_file(bol_index, bol).map_err(js_error)?;
        exported.export().map_err(js_error)
    }

    pub fn asset_data(&self, index: usize) -> Result<Vec<u8>, JsValue> {
        Ok(self.asset_bytes(index).map_err(js_error)?.to_vec())
    }

    pub fn replace_asset(&mut self, index: usize, data: &[u8]) -> Result<String, JsValue> {
        self.replace_asset_inner(index, data).map_err(js_error)
    }

    pub fn texture_width(&self, index: usize) -> Result<u16, JsValue> {
        Ok(self.decode_texture(index).map_err(js_error)?.width)
    }

    pub fn texture_height(&self, index: usize) -> Result<u16, JsValue> {
        Ok(self.decode_texture(index).map_err(js_error)?.height)
    }

    pub fn texture_rgba(&self, index: usize) -> Result<Vec<u8>, JsValue> {
        Ok(self.decode_texture(index).map_err(js_error)?.rgba)
    }

    pub fn set_layer(&mut self, name: String, visible: bool) -> Result<String, JsValue> {
        if !self.visibility.set(&name, visible) {
            return Err(JsValue::from_str(&format!(
                "Unknown viewport layer '{name}'"
            )));
        }
        if !self.visibility.shows_entity(&self.selected_kind().unwrap_or_default()) {
            self.selection = Selection::None;
            self.rebuild_dynamic().map_err(js_error)?;
        }
        let label = LAYER_NAMES
            .iter()
            .find(|(key, _)| *key == name)
            .map_or(name.as_str(), |(_, label)| label);
        self.status = format!("{} {label}.", if visible { "Showing" } else { "Hiding" });
        self.snapshot()
    }

    pub fn orbit(&mut self, dx: f32, dy: f32) {
        self.camera.orbit(dx, dy);
    }

    pub fn look(&mut self, dx: f32, dy: f32) {
        self.camera.look(dx, dy);
    }

    /// Camera-relative flight in world units.
    pub fn fly(&mut self, forward: f32, right: f32, up: f32) {
        self.camera.fly(forward, right, up);
    }

    pub fn pan(&mut self, dx: f32, dy: f32) {
        self.camera.pan(
            dx,
            dy,
            self.canvas.client_width() as f32,
            self.canvas.client_height() as f32,
        );
    }

    pub fn zoom(&mut self, delta: f32) {
        self.camera.zoom(delta);
    }

    /// Dolly toward whatever is under the cursor.
    pub fn zoom_at(&mut self, delta: f32, x: f32, y: f32) {
        let point = self
            .raycast_world(x, y)
            .unwrap_or_else(|| self.camera.target());
        self.camera.zoom_toward(delta, point);
    }

    pub fn frame_all(&mut self) {
        if let Some(bounds) = self.scene_bounds {
            self.camera.frame(bounds);
        }
    }

    /// Frames the selection, or the whole course when nothing is selected.
    pub fn focus_selected(&mut self) {
        match self.selected_transform() {
            Some((kind, index, transform)) => {
                let radius = self.entity_radius(&kind, index, &transform);
                self.camera.focus(transform.position, radius * 2.0);
            }
            None => self.frame_all(),
        }
    }

    pub fn view_top(&mut self) {
        self.camera.top_view();
    }

    pub fn set_camera_angles(&mut self, yaw_degrees: f32, pitch_degrees: f32) {
        self.camera
            .set_angles(yaw_degrees.to_radians(), pitch_degrees.to_radians());
    }

    pub fn camera_info(&self) -> String {
        let position = self.camera.position();
        json!({
            "position": position.to_array(),
            "target": self.camera.target().to_array(),
            "distance": self.camera.distance(),
            "scene_radius": self.camera.scene_radius(),
            "yaw": self.camera.yaw().to_degrees(),
            "pitch": self.camera.pitch().to_degrees(),
        })
        .to_string()
    }

    pub fn resize(&self) {
        self.renderer.resize_if_needed();
    }

    pub fn render(&mut self) -> Result<(), JsValue> {
        self.update_overlay().map_err(js_error)?;
        self.renderer
            .render(&self.camera, &self.visibility)
            .map_err(js_error)
    }

    pub fn pick(&mut self, x: f32, y: f32) -> Result<Option<String>, JsValue> {
        self.pick_inner(x, y).map_err(js_error)
    }

    /// World position under the cursor (collision, then course model, then
    /// the grid plane).
    pub fn world_at(&self, x: f32, y: f32) -> Option<Vec<f32>> {
        self.raycast_world(x, y).map(|point| point.to_array().to_vec())
    }

    pub fn set_gizmo_mode(&mut self, mode: String) -> Result<String, JsValue> {
        self.gizmo_hidden = mode == "select";
        self.gizmo_mode = match mode.as_str() {
            "select" => self.gizmo_mode,
            "translate" | "move" => GizmoMode::Translate,
            "rotate" => GizmoMode::Rotate,
            "scale" => GizmoMode::Scale,
            _ => return Err(JsValue::from_str(&format!("Unknown transform mode '{mode}'"))),
        };
        self.hover_handle = None;
        self.status = format!("{} tool active.", humanize(&mode));
        self.snapshot()
    }

    pub fn set_snap(&mut self, enabled: bool, translate: f32, rotate: f32) -> Result<String, JsValue> {
        self.snap = SnapSettings {
            enabled,
            translate: if translate.is_finite() && translate > 0.0 { translate } else { self.snap.translate },
            rotate: if rotate.is_finite() && rotate > 0.0 { rotate } else { self.snap.rotate },
        };
        self.status = if enabled {
            format!("Snapping to {} units / {}°.", self.snap.translate, self.snap.rotate)
        } else {
            "Snapping off.".to_owned()
        };
        self.snapshot()
    }

    pub fn set_shading(&mut self, mode: String) -> Result<String, JsValue> {
        self.renderer.set_shading(match mode.as_str() {
            "simple" | "unlit" => ShadingMode::Simple,
            _ => ShadingMode::Material,
        });
        self.status = match self.renderer.shading() {
            ShadingMode::Material => "Material shading (GX TEV).".to_owned(),
            ShadingMode::Simple => "Simple shading (base texture × vertex color).".to_owned(),
        };
        self.snapshot()
    }

    /// Returns the gizmo handle under the cursor and begins dragging it.
    pub fn pointer_down(&mut self, x: f32, y: f32) -> Result<Option<String>, JsValue> {
        let Some(handle) = self.gizmo_hit(x, y) else {
            return Ok(None);
        };
        let Some((kind, index, start)) = self.selected_transform() else {
            return Ok(None);
        };
        let before = self.capture_history().map_err(js_error)?;
        self.drag = Some(DragState {
            kind,
            index,
            handle,
            mode: self.effective_gizmo_mode(&start),
            start,
            before,
            start_mouse: (x, y),
            anchor: None,
            size: self.gizmo_world_size(start.position),
        });
        self.hover_handle = Some(handle);
        Ok(Some(handle.name().to_owned()))
    }

    /// Updates an active drag, or hover highlighting. Returns true when the
    /// viewport needs repainting.
    pub fn pointer_move(&mut self, x: f32, y: f32, snap_override: bool) -> Result<bool, JsValue> {
        if self.drag.is_some() {
            self.update_drag(x, y, snap_override).map_err(js_error)?;
            return Ok(true);
        }
        let hover = self.gizmo_hit(x, y);
        let changed = hover != self.hover_handle;
        self.hover_handle = hover;
        Ok(changed)
    }

    pub fn pointer_up(&mut self) -> Result<Option<String>, JsValue> {
        let Some(drag) = self.drag.take() else {
            return Ok(None);
        };
        let changed = self
            .course
            .as_ref()
            .zip(drag.before.course.as_ref())
            .is_some_and(|(now, before)| now.bytes() != before.bytes());
        if changed {
            self.push_undo(drag.before);
            self.status = format!(
                "{} {} {}.",
                match drag.mode {
                    GizmoMode::Translate => "Moved",
                    GizmoMode::Rotate => "Rotated",
                    GizmoMode::Scale => "Scaled",
                },
                humanize(&drag.kind).to_lowercase(),
                drag.index
            );
        }
        self.rebuild_dynamic().map_err(js_error)?;
        self.snapshot().map(Some)
    }

    pub fn cancel_drag(&mut self) -> Result<Option<String>, JsValue> {
        let Some(drag) = self.drag.take() else {
            return Ok(None);
        };
        self.course = drag.before.course;
        self.status = "Transform cancelled.".to_owned();
        self.rebuild_dynamic().map_err(js_error)?;
        self.snapshot().map(Some)
    }

    pub fn set_debug_draws(&self, enabled: bool) {
        crate::renderer::DEBUG_DRAWS.with(|flag| flag.set(enabled));
    }

    pub fn is_dragging(&self) -> bool {
        self.drag.is_some()
    }

    /// The gizmo handle under the cursor after the last `pointer_move`.
    pub fn gizmo_hover(&self) -> Option<String> {
        self.hover_handle.map(|handle| handle.name().to_owned())
    }

    /// Adds an object of `id` where the cursor ray meets the course.
    pub fn add_object_at(&mut self, id: u16, x: f32, y: f32) -> Result<String, JsValue> {
        let position = self
            .raycast_world(x, y)
            .unwrap_or_else(|| self.camera.target());
        self.add_object_with(id, position).map_err(js_error)
    }

    pub fn add_object_of(&mut self, id: u16) -> Result<String, JsValue> {
        let position = self
            .raycast_world(
                self.canvas.client_width() as f32 * 0.5,
                self.canvas.client_height() as f32 * 0.5,
            )
            .unwrap_or_else(|| self.camera.target());
        self.add_object_with(id, position).map_err(js_error)
    }

    /// Drops the selection onto the collision surface below it.
    pub fn drop_to_floor(&mut self) -> Result<String, JsValue> {
        self.drop_to_floor_inner().map_err(js_error)
    }

    /// Attaches an ARC as a read-only model library (for example MRAM.arc,
    /// which holds item boxes and other shared models).
    pub fn add_library(&mut self, name: String, data: &[u8]) -> Result<String, JsValue> {
        let archive = RarcArchive::new(data).map_err(js_error)?;
        let count = self.library.add_shared(&name, &archive);
        self.status = format!("Model library {name}: {count} models available.");
        self.rebuild_dynamic().map_err(js_error)?;
        self.snapshot()
    }

    pub fn remove_library(&mut self, name: String) -> Result<String, JsValue> {
        self.library.remove_shared(&name);
        self.status = format!("Removed model library {name}.");
        self.rebuild_dynamic().map_err(js_error)?;
        self.snapshot()
    }
}

impl Editor {
    fn open_file_inner(&mut self, name: String, data: &[u8]) -> FormatResult<String> {
        if data.is_empty() {
            return Err(FormatError::new("The selected file is empty"));
        }
        let decompressed = match compression::detect(data) {
            Compression::None => None,
            Compression::Yaz0 | Compression::Yay0 => Some(compression::decompress(data)?),
        };
        let probe = decompressed.as_deref().unwrap_or(data);

        let (
            course,
            archive,
            collision,
            collision_archive_index,
            archive_bol_index,
            course_model,
            course_model_archive_index,
            course_model_warning,
        ) = if probe.starts_with(b"RARC") {
            let archive = RarcArchive::new(data)?;
            let (
                course,
                bol_index,
                collision,
                bco_index,
                course_model,
                course_model_archive_index,
                course_model_warning,
            ) = open_archive_course(&archive)?;
            (
                course,
                Some(archive),
                collision,
                bco_index,
                Some(bol_index),
                course_model,
                course_model_archive_index,
                course_model_warning,
            )
        } else if probe.starts_with(b"0015") {
            (
                BolDocument::parse(probe)?,
                None,
                None,
                None,
                None,
                None,
                None,
                None,
            )
        } else {
            return Err(FormatError::new(
                "Unsupported course file; expected BOL 0015 or RARC data",
            ));
        };

        self.course = Some(course);
        self.archive = archive;
        self.refresh_archive_models();
        self.collision = collision;
        self.collision_archive_index = collision_archive_index;
        self.set_course_model(
            course_model,
            course_model_archive_index,
            course_model_warning,
        );
        self.loose_assets.clear();
        self.selection = Selection::None;
        self.archive_bol_index = archive_bol_index;
        self.file_name = Some(if name.trim().is_empty() {
            "course.bol".to_owned()
        } else {
            name
        });
        self.status = format!("Opened {}", self.file_name.as_deref().unwrap_or("course"));
        self.camera = OrbitCamera::default();
        self.undo.clear();
        self.redo.clear();
        self.archive_base_dirty = false;
        self.rebuild_scene()?;
        self.frame_all();
        self.snapshot_json()
    }

    fn load_related_inner(&mut self, name: String, data: &[u8]) -> FormatResult<String> {
        if self.course.is_none() {
            return Err(FormatError::new(
                "Open a course before loading related assets",
            ));
        }
        if data.is_empty() {
            return Err(FormatError::new("The related file is empty"));
        }
        let decoded = compression::decompress(data)?;
        if decoded.starts_with(b"RARC") {
            let archive = RarcArchive::new(data)?;
            let has_bol = archive
                .files()
                .iter()
                .any(|file| extension(&file.name).as_deref() == Some("bol"));
            if self.archive.is_some() || !has_bol {
                // Extra archives (MRAM.arc, other courses) supply object models.
                let count = self.library.add_shared(&name, &archive);
                self.status = format!("Added {count} models from {name} to the model library");
                self.rebuild_dynamic()?;
                return self.snapshot_json();
            }
            let bol_index = archive
                .files()
                .iter()
                .find(|file| file.name.to_ascii_lowercase().ends_with("_course.bol"))
                .or_else(|| {
                    archive
                        .files()
                        .iter()
                        .find(|file| extension(&file.name).as_deref() == Some("bol"))
                })
                .map(|file| file.index);
            let before = self.capture_history()?;
            self.archive_bol_index = bol_index;
            self.collision_archive_index =
                bol_index.and_then(|index| matching_bco(&archive, index));
            if self.collision.is_none()
                && let Some(bco_index) = self.collision_archive_index
            {
                self.collision = Some(Bco::parse(archive.file_data(bco_index)?)?);
            }
            let course_model_update = self.course_model.is_none().then(|| {
                let model_index = bol_index.and_then(|index| matching_bmd(&archive, index));
                match model_index {
                    Some(index) => {
                        let (model, warning) = archive_course_model(&archive, index);
                        (model, Some(index), warning)
                    }
                    None => (None, None, None),
                }
            });
            if let Some((model, model_index, warning)) = course_model_update {
                self.set_course_model(model, model_index, warning);
            }
            self.archive = Some(archive);
            self.refresh_archive_models();
            self.push_undo(before);
            self.status = format!("Attached assets from {name}; the current BOL was kept");
            return self.finish_scene_change(self.status.clone());
        }

        if decoded.starts_with(b"0003") || extension(&name).as_deref() == Some("bco") {
            let collision = Bco::parse(&decoded)?;
            if self.collision.as_ref() == Some(&collision) {
                self.status = format!("{} is already loaded", name);
                return self.snapshot_json();
            }
            let before = self.capture_history()?;
            self.collision = Some(collision);
            self.collision_archive_index = None;
            self.push_undo(before);
            return self.finish_scene_change(format!("Loaded collision from {name}"));
        }

        if extension(&name).as_deref() == Some("bmd") {
            let (model, warning) = course_model_preview(&name, &decoded);
            let Some(model) = model else {
                self.course_model_warning = warning;
                self.status = format!(
                    "Could not load a course model from {name}; the current preview was kept"
                );
                return self.snapshot_json();
            };
            let before = self.capture_history()?;
            self.set_course_model(Some(model), None, warning);
            self.push_undo(before);
            return self.finish_scene_change(format!("Loaded course model from {name}"));
        }

        if extension(&name).as_deref() == Some("bti") || BtiImage::decode(&decoded).is_ok() {
            BtiImage::decode(&decoded)?;
            let index = LOOSE_ASSET_BASE + self.loose_assets.len();
            let before = self.capture_history()?;
            self.loose_assets.push(LooseAsset {
                name: name.clone(),
                data: decoded.clone(),
                original: decoded,
            });
            self.selection = Selection::File(index);
            self.push_undo(before);
            self.rebuild_scene()?;
            self.status = format!("Loaded texture {name}");
            return self.snapshot_json();
        }

        Err(FormatError::new(format!(
            "Unsupported related file '{name}'; expected BCO, BTI, BMD, or ARC data"
        )))
    }

    fn select_inner(&mut self, kind: &str, index: usize) -> FormatResult<String> {
        let normalized = kind.to_ascii_lowercase();
        if matches!(normalized.as_str(), "none" | "clear") {
            self.selection = Selection::None;
        } else if matches!(normalized.as_str(), "file" | "asset") {
            self.asset_bytes(index)?;
            self.selection = Selection::File(index);
        } else {
            let course = self
                .course
                .as_ref()
                .ok_or_else(|| FormatError::new("No BOL course is open"))?;
            let inspection = course.inspect(&normalized, index)?;
            self.selection = Selection::Course {
                kind: inspection.kind,
                index: inspection.index,
            };
        }
        self.status = "Selection updated.".to_owned();
        self.rebuild_scene()?;
        self.snapshot_json()
    }

    fn edit_inner(
        &mut self,
        kind: &str,
        index: usize,
        key: &str,
        value: Value,
    ) -> FormatResult<String> {
        let course = self
            .course
            .as_ref()
            .ok_or_else(|| FormatError::new("No BOL course is open"))?;
        let mut next = course.clone();
        apply_field_edit(&mut next, kind, index, key, value)?;
        if next.bytes() == course.bytes() {
            self.status = "The field already had that value.".to_owned();
            return self.snapshot_json();
        }
        let before = self.capture_history()?;
        self.course = Some(next);
        self.push_undo(before);
        self.finish_scene_change(format!("Updated {kind} {index} {key}"))
    }

    fn replace_asset_inner(&mut self, index: usize, data: &[u8]) -> FormatResult<String> {
        if Some(index) == self.archive_bol_index {
            return Err(FormatError::new(
                "The course BOL cannot be replaced as an asset; open a replacement course instead",
            ));
        }
        if data.is_empty() {
            return Err(FormatError::new("The replacement asset is empty"));
        }
        let current = self.asset_bytes(index)?.to_vec();
        let name = self.asset_name(index)?;
        let decoded = compression::decompress(data)?;
        validate_replacement(&name, &current, &decoded)?;
        if current == decoded {
            self.status = format!("{name} already contains those bytes");
            return self.snapshot_json();
        }

        let replacement_collision = if asset_kind(&name, &current) == "bco" {
            Some(Bco::parse(&decoded)?)
        } else {
            None
        };
        let replacement_course_model = (self.course_model_archive_index == Some(index))
            .then(|| course_model_preview(&name, &decoded));
        let before = self.capture_history()?;
        let mut collision_changed = false;
        let mut course_model_changed = false;
        if index >= LOOSE_ASSET_BASE {
            let loose_index = index - LOOSE_ASSET_BASE;
            let asset = self.loose_assets.get_mut(loose_index).ok_or_else(|| {
                FormatError::new(format!("Loose asset index {index} is out of range"))
            })?;
            asset.data = decoded;
        } else {
            self.archive
                .as_mut()
                .ok_or_else(|| FormatError::new("No RARC archive is open"))?
                .replace_file(index, decoded)?;
            self.refresh_archive_models();
            if self.collision_archive_index == Some(index) {
                self.collision = replacement_collision;
                collision_changed = true;
            }
            if let Some((model, warning)) = replacement_course_model {
                self.set_course_model(model, Some(index), warning);
                course_model_changed = true;
            }
        }
        self.push_undo(before);
        self.status = format!("Replaced {name}");
        if collision_changed || course_model_changed {
            self.rebuild_scene()?;
        }
        self.snapshot_json()
    }

    fn decode_texture(&self, index: usize) -> FormatResult<BtiImage> {
        let bytes = self.asset_bytes(index)?;
        let name = self.asset_name(index)?;
        if asset_kind(&name, bytes) == "j3d" {
            let info = j3d::inspect(bytes)?;
            let offset = info.bti_offsets.first().copied().ok_or_else(|| {
                FormatError::new(format!("J3D asset '{name}' contains no BTI textures"))
            })?;
            BtiImage::decode_at(bytes, offset)
        } else {
            BtiImage::decode(bytes)
        }
    }

    fn pick_inner(&mut self, x: f32, y: f32) -> FormatResult<Option<String>> {
        if !x.is_finite() || !y.is_finite() {
            return Err(FormatError::new("Pick coordinates must be finite"));
        }
        let (width, height) = self.viewport_size();
        if width <= 0.0 || height <= 0.0 {
            return Ok(None);
        }
        let Some(course) = self.course.as_ref() else {
            return Ok(None);
        };
        let (origin, direction) = self.camera.ray(x, y, width, height);
        // (ray distance, priority, kind, index); lower priority wins ties.
        let mut best: Option<(f32, u8, String, usize)> = None;
        let mut consider = |distance: f32, priority: u8, kind: &str, index: usize| {
            let better = best.as_ref().is_none_or(|(best_distance, best_priority, _, _)| {
                priority < *best_priority || (priority == *best_priority && distance < *best_distance)
            });
            if better {
                best = Some((distance, priority, kind.to_owned(), index));
            }
        };

        if self.visibility.objects {
            for index in 0..course.objects.len() {
                let Some(transform) = transform::read(course, "object", index) else {
                    continue;
                };
                let hit = match self.object_models.get(index).and_then(Option::as_ref) {
                    Some((_, bounds)) => transform::ray_box(
                        origin,
                        direction,
                        transform.matrix(),
                        bounds.min,
                        bounds.max,
                    ),
                    None => {
                        let bounds = scene::object_proxy_bounds(self.marker);
                        let rigid = EntityTransform { scale: None, ..transform };
                        transform::ray_box(origin, direction, rigid.matrix(), bounds.min, bounds.max)
                    }
                };
                if let Some(distance) = hit {
                    consider(distance, 0, "object", index);
                }
            }
        }

        // Point-like records: screen-space radius test, priority below
        // directly hit objects only when they are farther away.
        for entity in course.scene_entities() {
            if entity.kind == "object" || entity.kind == "area" || !self.visibility.shows_entity(entity.kind) {
                continue;
            }
            let point = Vec3::from(entity.position);
            let Some((screen_x, screen_y, depth)) = self.camera.project(point, width, height) else {
                continue;
            };
            let dx = screen_x - x;
            let dy = screen_y - y;
            if dx * dx + dy * dy <= PICK_RADIUS_CSS_PX * PICK_RADIUS_CSS_PX {
                consider(depth, 0, entity.kind, entity.index);
            }
        }

        if self.visibility.areas {
            for (index, area) in course.areas.iter().enumerate() {
                if let Some(transform) = transform::read(course, "area", index) {
                    let rigid = EntityTransform { scale: None, ..transform };
                    let bounds = scene::area_bounds(area.scale);
                    if let Some(distance) =
                        transform::ray_box(origin, direction, rigid.matrix(), bounds.min, bounds.max)
                    {
                        consider(distance, 1, "area", index);
                    }
                }
            }
        }

        self.selection = match best {
            Some((_, _, kind, index)) => Selection::Course { kind, index },
            None => Selection::None,
        };
        self.status = if matches!(self.selection, Selection::None) {
            "Selection cleared.".to_owned()
        } else {
            "Viewport selection updated.".to_owned()
        };
        self.rebuild_dynamic()?;
        self.snapshot_json().map(Some)
    }

    fn viewport_size(&self) -> (f32, f32) {
        (
            self.canvas.client_width().max(0) as f32,
            self.canvas.client_height().max(0) as f32,
        )
    }

    fn selected_kind(&self) -> Option<String> {
        match &self.selection {
            Selection::Course { kind, .. } => Some(kind.clone()),
            _ => None,
        }
    }

    fn selected_transform(&self) -> Option<(String, usize, EntityTransform)> {
        let Selection::Course { kind, index } = &self.selection else {
            return None;
        };
        let course = self.course.as_ref()?;
        transform::read(course, kind, *index).map(|transform| (kind.clone(), *index, transform))
    }

    fn entity_radius(&self, kind: &str, index: usize, transform: &EntityTransform) -> f32 {
        if kind == "object"
            && let Some(Some((_, bounds))) = self.object_models.get(index)
        {
            return bounds.transformed(transform.matrix()).radius().max(self.marker * 4.0);
        }
        if kind == "area"
            && let Some(area) = self.course.as_ref().and_then(|course| course.areas.get(index))
        {
            return scene::area_bounds(area.scale).radius();
        }
        if let Some(secondary) = transform.secondary {
            return (secondary - transform.position).length().max(self.marker * 4.0) * 0.6;
        }
        self.marker * 8.0
    }

    /// Rotation and scale gizmos only apply to records that store them.
    fn effective_gizmo_mode(&self, transform: &EntityTransform) -> GizmoMode {
        match self.gizmo_mode {
            GizmoMode::Rotate if transform.directions.is_none() && transform.secondary.is_none() => {
                GizmoMode::Translate
            }
            GizmoMode::Scale if transform.scale.is_none() => GizmoMode::Translate,
            mode => mode,
        }
    }

    fn gizmo_world_size(&self, center: Vec3) -> f32 {
        let (_, height) = self.viewport_size();
        self.camera.world_per_pixel(center, height) * GIZMO_SIZE_PX
    }

    fn update_overlay(&mut self) -> FormatResult<()> {
        let visible = !self.gizmo_hidden || self.drag.is_some();
        let layers = match self.selected_transform().filter(|_| visible) {
            Some((_, _, transform)) => {
                let mode = self
                    .drag
                    .as_ref()
                    .map_or_else(|| self.effective_gizmo_mode(&transform), |drag| drag.mode);
                let size = self.gizmo_world_size(transform.position);
                let active = self.drag.as_ref().map(|drag| drag.handle).or(self.hover_handle);
                scene::build_gizmo(mode, transform.position, size, active)
            }
            None => Vec::new(),
        };
        self.renderer.set_layers(LayerGroup::Overlay, &layers)
    }

    fn gizmo_hit(&self, x: f32, y: f32) -> Option<GizmoHandle> {
        if self.gizmo_hidden {
            return None;
        }
        let (_, _, transform) = self.selected_transform()?;
        let (width, height) = self.viewport_size();
        let mode = self.effective_gizmo_mode(&transform);
        let center = transform.position;
        let size = self.gizmo_world_size(center);
        let project = |point: Vec3| {
            self.camera
                .project(point, width, height)
                .map(|(sx, sy, _)| glam::Vec2::new(sx, sy))
        };
        let mouse = glam::Vec2::new(x, y);
        let segment_distance = |a: glam::Vec2, b: glam::Vec2| {
            let ab = b - a;
            let t = ((mouse - a).dot(ab) / ab.length_squared().max(1e-6)).clamp(0.0, 1.0);
            (a + ab * t).distance(mouse)
        };
        let screen_center = project(center)?;
        let mut best: Option<(f32, GizmoHandle)> = None;
        let mut consider = |distance: f32, handle: GizmoHandle| {
            if distance <= GIZMO_PICK_PX && best.is_none_or(|(d, _)| distance < d) {
                best = Some((distance, handle));
            }
        };
        match mode {
            GizmoMode::Translate | GizmoMode::Scale => {
                for handle in [GizmoHandle::X, GizmoHandle::Y, GizmoHandle::Z] {
                    let axis = handle.axis().unwrap();
                    let length = if mode == GizmoMode::Translate { 1.22 } else { 1.06 };
                    if let (Some(a), Some(b)) =
                        (project(center + axis * size * 0.12), project(center + axis * size * length))
                    {
                        consider(segment_distance(a, b), handle);
                    }
                }
                if mode == GizmoMode::Translate {
                    let corners = [(0.2, 0.2), (0.45, 0.2), (0.45, 0.45), (0.2, 0.45)]
                        .map(|(a, b)| project(center + Vec3::new(a * size, 0.0, b * size)));
                    if let [Some(a), Some(b), Some(c), Some(d)] = corners
                        && point_in_quad(mouse, [a, b, c, d])
                    {
                        consider(GIZMO_PICK_PX * 0.5, GizmoHandle::PlaneXz);
                    }
                } else if screen_center.distance(mouse) <= GIZMO_PICK_PX * 1.4 {
                    consider(0.0, GizmoHandle::Center);
                }
            }
            GizmoMode::Rotate => {
                for handle in [GizmoHandle::X, GizmoHandle::Y, GizmoHandle::Z] {
                    let (a, b) = scene::perpendiculars(handle.axis().unwrap());
                    let mut previous = None;
                    let mut nearest = f32::INFINITY;
                    for step in 0..=48 {
                        let angle = step as f32 / 48.0 * std::f32::consts::TAU;
                        let point = project(center + (a * angle.cos() + b * angle.sin()) * size);
                        if let (Some(previous), Some(point)) = (previous, point) {
                            nearest = nearest.min(segment_distance(previous, point));
                        }
                        previous = point;
                    }
                    consider(nearest, handle);
                }
            }
        }
        best.map(|(_, handle)| handle)
    }

    fn update_drag(&mut self, x: f32, y: f32, snap_override: bool) -> FormatResult<()> {
        let (width, height) = self.viewport_size();
        let (origin, direction) = self.camera.ray(x, y, width, height);
        let snapping = self.snap.enabled != snap_override;
        let Some(drag) = self.drag.as_mut() else {
            return Ok(());
        };
        let start = drag.start;
        let center = start.position;
        let mut next = start;
        if drag.anchor.is_none() {
            // Measure from where the button went down so no motion is lost.
            let (start_x, start_y) = drag.start_mouse;
            let (start_origin, start_direction) = self.camera.ray(start_x, start_y, width, height);
            drag.anchor =
                drag_point(drag.mode, drag.handle, center, start_origin, start_direction, drag.size);
        }
        match drag.mode {
            GizmoMode::Translate => {
                let measure = match drag.handle {
                    GizmoHandle::PlaneXz | GizmoHandle::Center => {
                        ray_plane(origin, direction, center, Vec3::Y)
                    }
                    handle => {
                        let axis = handle.axis().unwrap();
                        closest_on_axis(origin, direction, center, axis).map(|t| center + axis * t)
                    }
                };
                if let Some(point) = measure {
                    let anchor = *drag.anchor.get_or_insert(point);
                    let mut delta = point - anchor;
                    if let Some(axis) = drag.handle.axis() {
                        delta = axis * delta.dot(axis);
                    } else {
                        delta.y = 0.0;
                    }
                    let mut position = center + delta;
                    if snapping {
                        let step = self.snap.translate;
                        for component in 0..3 {
                            if delta[component] != 0.0 {
                                position[component] = (position[component] / step).round() * step;
                            }
                        }
                    }
                    next = start.translated(position - center);
                }
            }
            GizmoMode::Rotate => {
                let axis = drag.handle.axis().unwrap_or(Vec3::Y);
                let angle = match ray_plane(origin, direction, center, axis) {
                    Some(point) if (point - center).length() > drag.size * 0.05 => {
                        let anchor = *drag.anchor.get_or_insert(point);
                        let a = (anchor - center).normalize_or_zero();
                        let b = (point - center).normalize_or_zero();
                        a.cross(b).dot(axis).atan2(a.dot(b))
                    }
                    _ => (x - drag.start_mouse.0) * 0.01,
                };
                let angle = if snapping {
                    let step = self.snap.rotate.to_radians();
                    (angle / step).round() * step
                } else {
                    angle
                };
                next = start.rotated(axis, angle);
            }
            GizmoMode::Scale => {
                let factor = match drag.handle.axis() {
                    Some(axis) => closest_on_axis(origin, direction, center, axis).map(|t| {
                        let anchor = *drag.anchor.get_or_insert(center + axis * t);
                        let anchor_t = (anchor - center).dot(axis);
                        1.0 + (t - anchor_t) / drag.size.max(1e-3)
                    }),
                    None => Some(
                        ((x - drag.start_mouse.0) - (y - drag.start_mouse.1)).mul_add(0.01, 0.0).exp(),
                    ),
                }
                .unwrap_or(1.0)
                .max(0.01);
                if let Some(scale) = start.scale {
                    let mut scaled = match drag.handle.axis() {
                        Some(axis) => scale * (Vec3::ONE + axis * (factor - 1.0)),
                        None => scale * factor,
                    };
                    if snapping {
                        scaled = (scaled * 10.0).round() / 10.0;
                    }
                    next.scale = Some(scaled);
                }
            }
        }
        let kind = drag.kind.clone();
        let index = drag.index;
        let mut course = drag
            .before
            .course
            .clone()
            .ok_or_else(|| FormatError::new("The course closed during a transform"))?;
        transform::write(&mut course, &kind, index, &next)?;
        self.course = Some(course);
        self.rebuild_dynamic()
    }

    fn raycast_world(&self, x: f32, y: f32) -> Option<Vec3> {
        let (width, height) = self.viewport_size();
        if width <= 0.0 || height <= 0.0 {
            return None;
        }
        let (origin, direction) = self.camera.ray(x, y, width, height);
        if let Some(distance) = self.raycast_geometry(origin, direction) {
            return Some(origin + direction * distance);
        }
        let ground = self.scene_bounds.map_or(0.0, |bounds| bounds.min.y);
        ray_plane(origin, direction, Vec3::new(0.0, ground, 0.0), Vec3::Y)
    }

    fn raycast_geometry(&self, origin: Vec3, direction: Vec3) -> Option<f32> {
        let mut best: Option<f32> = None;
        let mut test = |triangle: [Vec3; 3]| {
            if let Some(distance) = transform::ray_triangle(origin, direction, triangle)
                && best.is_none_or(|best| distance < best)
            {
                best = Some(distance);
            }
        };
        if let Some(collision) = self.collision.as_ref() {
            for triangle in &collision.triangles {
                let points = triangle.indices.map(|index| {
                    collision
                        .vertices
                        .get(index as usize)
                        .copied()
                        .map(Vec3::from_array)
                });
                if let [Some(a), Some(b), Some(c)] = points {
                    test([a, b, c]);
                }
            }
        } else if let Some(model) = self.course_model.as_deref() {
            for mesh in &model.meshes {
                for triangle in mesh.indices.chunks_exact(3) {
                    let points = [0, 1, 2].map(|corner| {
                        Vec3::from_array(mesh.vertices[triangle[corner] as usize].position)
                    });
                    test(points);
                }
            }
        }
        best
    }

    fn add_object_with(&mut self, id: u16, position: Vec3) -> FormatResult<String> {
        let course = self
            .course
            .as_ref()
            .ok_or_else(|| FormatError::new("Open a BOL course before adding an object"))?;
        let mut next = course.clone();
        let heading = Vec3::new(self.camera.forward().x, 0.0, self.camera.forward().z)
            .try_normalize()
            .unwrap_or(Vec3::Z);
        let object = MapObject {
            id,
            position: position.to_array(),
            forward: transform::encode_direction(heading),
            ..MapObject::default()
        };
        let index = next.add_object(object)?;
        let before = self.capture_history()?;
        self.course = Some(next);
        self.selection = Selection::Course {
            kind: "object".to_owned(),
            index,
        };
        self.push_undo(before);
        let name = catalog::object_name(id).unwrap_or("object");
        self.finish_scene_change(format!("Placed {name} as object {index}"))
    }

    fn drop_to_floor_inner(&mut self) -> FormatResult<String> {
        let (kind, index, transform) = self
            .selected_transform()
            .ok_or_else(|| FormatError::new("Select something with a position to drop it"))?;
        let origin = transform.position + Vec3::Y * self.marker * 2.0;
        let distance = self
            .raycast_geometry(origin, Vec3::NEG_Y)
            .ok_or_else(|| FormatError::new("No collision surface was found below the selection"))?;
        let floor = origin - Vec3::Y * distance;
        let mut next = self
            .course
            .clone()
            .ok_or_else(|| FormatError::new("No BOL course is open"))?;
        let moved = if kind == "checkpoint" {
            transform.translated(Vec3::Y * (floor.y - transform.position.y))
        } else {
            EntityTransform {
                position: floor,
                ..transform
            }
        };
        transform::write(&mut next, &kind, index, &moved)?;
        if next.bytes() == self.course.as_ref().map_or(&[][..], |course| course.bytes()) {
            self.status = "Already resting on the floor.".to_owned();
            return self.snapshot_json();
        }
        let before = self.capture_history()?;
        self.course = Some(next);
        self.push_undo(before);
        self.finish_scene_change(format!("Dropped {} {index} to the floor", humanize(&kind).to_lowercase()))
    }

    fn refresh_archive_models(&mut self) {
        self.library.set_course(self.archive.as_ref());
        self.sky_model = self.library.sky();
        let keep_course = |key: &str| !key.starts_with("object:") && key != "sky";
        self.renderer.retain_models(keep_course);
    }

    fn selected_object_index(&self) -> FormatResult<usize> {
        match &self.selection {
            Selection::Course { kind, index } if kind == "object" => Ok(*index),
            _ => Err(FormatError::new(
                "Select a course object before using this command",
            )),
        }
    }

    fn finish_scene_change(&mut self, status: String) -> FormatResult<String> {
        self.rebuild_scene()?;
        self.status = status;
        self.snapshot_json()
    }

    fn set_course_model(
        &mut self,
        model: Option<Rc<BmdModel>>,
        archive_index: Option<usize>,
        warning: Option<String>,
    ) {
        self.course_model_bounds = model.as_deref().and_then(bmd_bounds);
        self.course_model = model;
        self.course_model_archive_index = archive_index;
        self.course_model_warning = warning;
    }

    fn rebuild_scene(&mut self) -> FormatResult<()> {
        self.rebuild_with(true)
    }

    /// Rebuilds helpers and model instances, but not grid/collision.
    fn rebuild_dynamic(&mut self) -> FormatResult<()> {
        self.rebuild_with(false)
    }

    fn rebuild_with(&mut self, full: bool) -> FormatResult<()> {
        let Some(course) = self.course.as_ref() else {
            self.scene_bounds = None;
            self.object_models.clear();
            self.renderer.set_layers(LayerGroup::Static, &[])?;
            self.renderer.set_layers(LayerGroup::Helpers, &[])?;
            self.renderer.set_instances(Vec::new());
            self.renderer.retain_models(|_| false);
            self.static_bounds = None;
            return Ok(());
        };

        if full || self.scene_bounds.is_none() {
            let mut model_bounds = self.course_model_bounds;
            if let Some(sky) = self.sky_model.as_deref().and_then(bmd_bounds) {
                // The sky sets the far plane but should not drive framing.
                self.camera.set_scene_radius(sky.radius());
            }
            let bounds = scene::scene_bounds(course, self.collision.as_ref(), model_bounds.take());
            self.scene_bounds = bounds;
            self.marker = scene::marker_size(bounds);
            let radius = bounds.map_or(1_000.0, Bounds::radius);
            if self.sky_model.is_none() || self.camera.scene_radius() < radius {
                self.camera.set_scene_radius(radius);
            }
        }

        let collision_identity = self.collision.as_ref().map(|collision| collision as *const Bco as usize);
        if self.static_bounds != Some(self.scene_bounds) || self.static_collision != collision_identity {
            let layers = scene::build_static_layers(self.collision.as_ref(), self.scene_bounds);
            self.renderer.set_layers(LayerGroup::Static, &layers)?;
            self.static_bounds = Some(self.scene_bounds);
            self.static_collision = collision_identity;
        }

        // Resolve and upload object models.
        let mut object_models = Vec::with_capacity(course.objects.len());
        let mut used = vec!["course".to_owned(), "sky".to_owned()];
        for object in &course.objects {
            let resolved = catalog::model_names(object.id)
                .into_iter()
                .find_map(|name| self.library.get(name).map(|model| (name, model)));
            let entry = match resolved {
                Some((name, model)) => {
                    let key = format!("object:{name}");
                    match bmd_bounds(&model) {
                        Some(bounds) => {
                            if !self.renderer.has_model(&key) {
                                self.renderer.set_model(&key, &model)?;
                            }
                            if !used.contains(&key) {
                                used.push(key.clone());
                            }
                            Some((key, bounds))
                        }
                        None => None,
                    }
                }
                None => None,
            };
            object_models.push(entry);
        }
        self.object_models = object_models;

        let selected = match &self.selection {
            Selection::Course { kind, index } => Some((kind.as_str(), *index)),
            _ => None,
        };
        let helpers = scene::build_helper_layers(&HelperInput {
            course,
            selected,
            object_models: &self
                .object_models
                .iter()
                .map(|entry| entry.as_ref().map(|(_, bounds)| *bounds))
                .collect::<Vec<_>>(),
            marker: self.marker,
        });
        self.renderer.set_layers(LayerGroup::Helpers, &helpers)?;

        let mut instances = Vec::new();
        match self.course_model.as_ref() {
            Some(model) => {
                self.renderer.set_model("course", model)?;
                instances.push(ModelInstance {
                    model: "course".to_owned(),
                    transform: Mat4::IDENTITY,
                    highlight: [0.0; 4],
                    layer: ModelLayer::Course,
                });
            }
            None => self.renderer.remove_model("course"),
        }
        match self.sky_model.as_ref() {
            Some(model) => {
                self.renderer.set_model("sky", model)?;
                instances.push(ModelInstance {
                    model: "sky".to_owned(),
                    transform: Mat4::IDENTITY,
                    highlight: [0.0; 4],
                    layer: ModelLayer::Sky,
                });
            }
            None => self.renderer.remove_model("sky"),
        }
        for (index, entry) in self.object_models.iter().enumerate() {
            let Some((key, _)) = entry else { continue };
            let Some(mut transform) = transform::read(course, "object", index) else {
                continue;
            };
            let object = &course.objects[index];
            transform.position.y =
                catalog::display_height(object.id, &object.params, transform.position.y);
            let highlight = if selected == Some(("object", index)) {
                [1.0, 0.78, 0.1, 0.28]
            } else {
                [0.0; 4]
            };
            let matrix = transform.matrix();
            instances.push(ModelInstance {
                model: key.clone(),
                transform: matrix,
                highlight,
                layer: ModelLayer::Object,
            });
            for name in catalog::companion_models(object.id) {
                let Some(model) = self.library.get(name) else { continue };
                let companion = format!("object:{name}");
                if !self.renderer.has_model(&companion) {
                    self.renderer.set_model(&companion, &model)?;
                }
                if !used.contains(&companion) {
                    used.push(companion.clone());
                }
                instances.push(ModelInstance {
                    model: companion,
                    transform: matrix,
                    highlight,
                    layer: ModelLayer::Object,
                });
            }
        }
        self.renderer.set_instances(instances);
        self.renderer.retain_models(|key| used.iter().any(|used| used == key));

        let header = &course.header;
        let sky_radius = self
            .sky_model
            .as_deref()
            .and_then(bmd_bounds)
            .map_or(0.0, |bounds| bounds.max.x.abs().max(bounds.min.x.abs()));
        self.renderer.set_sky(header.sky_follow != 0, sky_radius);
        let fog = header.fog_color.map(|channel| f32::from(channel) / 255.0);
        // Courses without a sky model show their fog color as the backdrop.
        self.renderer.set_clear_color(if self.sky_model.is_none() && fog.iter().any(|c| *c > 0.0) {
            fog
        } else {
            [0.105, 0.115, 0.135]
        });
        let light = Vec3::from_array(header.light_position);
        let color = header.light_color.map(|channel| f32::from(channel) / 255.0);
        let usable_color = color[..3].iter().any(|channel| *channel > 0.05);
        self.renderer.set_light(
            if light.length_squared() > 1.0 { -light } else { Vec3::new(0.45, -0.85, -0.35) },
            if usable_color { [color[0], color[1], color[2], 1.0] } else { [1.0; 4] },
        );
        Ok(())
    }

    fn capture_history(&self) -> FormatResult<HistoryState> {
        Ok(HistoryState {
            course: self.course.clone(),
            archive: self.archive.as_ref().map(RarcArchive::export).transpose()?,
            archive_dirty: self.archive_is_dirty(),
            collision: self.collision.clone(),
            collision_archive_index: self.collision_archive_index,
            course_model: self.course_model.clone(),
            course_model_archive_index: self.course_model_archive_index,
            course_model_warning: self.course_model_warning.clone(),
            course_model_bounds: self.course_model_bounds,
            loose_assets: self.loose_assets.clone(),
            selection: self.selection.clone(),
            archive_bol_index: self.archive_bol_index,
        })
    }

    fn restore_history(&mut self, state: HistoryState) -> FormatResult<()> {
        let archive = state.archive.map(RarcArchive::new).transpose()?;
        let archive_changed = self.archive.as_ref().map(RarcArchive::export).transpose()?
            != archive.as_ref().map(RarcArchive::export).transpose()?;
        self.course = state.course;
        self.archive = archive;
        if archive_changed {
            self.refresh_archive_models();
        }
        self.archive_base_dirty = state.archive_dirty;
        self.collision = state.collision;
        self.collision_archive_index = state.collision_archive_index;
        self.course_model = state.course_model;
        self.course_model_archive_index = state.course_model_archive_index;
        self.course_model_warning = state.course_model_warning;
        self.course_model_bounds = state.course_model_bounds;
        self.loose_assets = state.loose_assets;
        self.selection = state.selection;
        self.archive_bol_index = state.archive_bol_index;
        Ok(())
    }

    fn push_undo(&mut self, state: HistoryState) {
        push_bounded(&mut self.undo, state);
        self.redo.clear();
    }

    fn archive_is_dirty(&self) -> bool {
        self.archive_base_dirty || self.archive.as_ref().is_some_and(RarcArchive::is_dirty)
    }

    fn course_is_dirty(&self) -> bool {
        self.course
            .as_ref()
            .is_some_and(|course| course.bytes() != course.original_bytes())
    }

    fn is_dirty(&self) -> bool {
        self.course_is_dirty()
            || self.archive_is_dirty()
            || self
                .loose_assets
                .iter()
                .any(|asset| asset.data != asset.original)
    }

    fn asset_bytes(&self, index: usize) -> FormatResult<&[u8]> {
        if index >= LOOSE_ASSET_BASE {
            self.loose_assets
                .get(index - LOOSE_ASSET_BASE)
                .map(|asset| asset.data.as_slice())
                .ok_or_else(|| {
                    FormatError::new(format!("Loose asset index {index} is out of range"))
                })
        } else {
            self.archive
                .as_ref()
                .ok_or_else(|| FormatError::new("No RARC archive is open"))?
                .file_data(index)
        }
    }

    fn asset_name(&self, index: usize) -> FormatResult<String> {
        if index >= LOOSE_ASSET_BASE {
            self.loose_assets
                .get(index - LOOSE_ASSET_BASE)
                .map(|asset| asset.name.clone())
                .ok_or_else(|| {
                    FormatError::new(format!("Loose asset index {index} is out of range"))
                })
        } else {
            self.archive
                .as_ref()
                .and_then(|archive| archive.files().get(index))
                .map(|file| file.path.clone())
                .ok_or_else(|| FormatError::new(format!("RARC file index {index} is out of range")))
        }
    }

    fn snapshot_json(&self) -> FormatResult<String> {
        serde_json::to_string(&self.snapshot_value()?).map_err(|error| {
            FormatError::new(format!("Could not serialize editor snapshot: {error}"))
        })
    }

    fn snapshot_value(&self) -> FormatResult<Value> {
        let selection = self.selection_value()?;
        let selected_asset = self.selected_asset_value()?;
        let archive = self.archive_value()?;
        let course = self.course_value()?;
        let entities = self.entities_value()?;
        let issues = self.issues_value();
        let course_model = self.course_model_value();
        let collision = self.collision.as_ref().map(|collision| {
            json!({
                "vertices": collision.vertices.len(),
                "triangles": collision.triangles.len(),
                "materials": collision.materials.len(),
            })
        });
        Ok(json!({
            "file_name": self.file_name,
            "status": self.status,
            "dirty": self.is_dirty(),
            "can_undo": !self.undo.is_empty(),
            "can_redo": !self.redo.is_empty(),
            "course": course,
            "archive": archive,
            "collision": collision,
            "course_model": course_model,
            "selection": selection,
            "selected_asset": selected_asset,
            "entities": entities,
            "issues": issues,
            "layers": self.layers_value(),
            "library": self.library.value(),
            "view": {
                "gizmo_mode": match self.gizmo_mode {
                    GizmoMode::Translate => "translate",
                    GizmoMode::Rotate => "rotate",
                    GizmoMode::Scale => "scale",
                },
                "snap": {
                    "enabled": self.snap.enabled,
                    "translate": self.snap.translate,
                    "rotate": self.snap.rotate,
                },
                "shading": match self.renderer.shading() {
                    ShadingMode::Material => "material",
                    ShadingMode::Simple => "simple",
                },
                "marker": self.marker,
                "scene_radius": self.scene_bounds.map(Bounds::radius),
                "sky": self.sky_model.is_some(),
                "object_models": self.object_models.iter().filter(|entry| entry.is_some()).count(),
                "shader_programs": self.renderer.program_count(),
                "shader_failures": self.renderer.program_failures(),
            },
        }))
    }

    fn course_model_value(&self) -> Value {
        let model = self.course_model.as_deref();
        json!({
            "loaded": model.is_some(),
            "archive_index": self.course_model_archive_index,
            "warning": self.course_model_warning,
            "meshes": model.map_or(0, |model| model.meshes.len()),
            "vertices": model.map_or(0, |model| {
                model.meshes.iter().map(|mesh| mesh.vertices.len()).sum()
            }),
            "triangles": model.map_or(0, |model| {
                model.meshes.iter().map(|mesh| mesh.indices.len() / 3).sum()
            }),
            "textures": model.map_or(0, |model| model.textures.len()),
        })
    }

    fn course_value(&self) -> FormatResult<Value> {
        let Some(course) = self.course.as_ref() else {
            return Ok(Value::Null);
        };
        let header = course.inspect("header", 0)?;
        let counts = &course.counts;
        let counts = [
            ("centerline", "Centerline points", counts.centerline),
            (
                "checkpoint_group",
                "Checkpoint groups",
                counts.checkpoint_groups,
            ),
            ("checkpoint", "Checkpoints", counts.checkpoints),
            ("path", "Paths", counts.paths),
            ("route_point", "Route points", counts.route_points),
            ("object", "Objects", counts.objects),
            ("start", "Start points", counts.starts),
            ("area", "Areas", counts.areas),
            ("camera", "Cameras", counts.cameras),
            ("respawn", "Respawns", counts.respawns),
            ("light", "Light parameters", counts.lights),
            ("mg", "MG parameters", counts.mg),
        ]
        .into_iter()
        .map(|(kind, label, count)| json!({"kind": kind, "label": label, "count": count}))
        .collect::<Vec<_>>();
        Ok(json!({
            "dirty": self.course_is_dirty(),
            "canonical_layout": course.uses_canonical_layout(),
            "course_id": course.header.course_id,
            "course_name": catalog::course_name(course.header.course_id),
            "archive_bol_index": self.archive_bol_index,
            "counts": counts,
            "header": inspection_fields(course, &header),
            "object_catalog": object_catalog(&self.library),
        }))
    }

    fn archive_value(&self) -> FormatResult<Value> {
        if self.archive.is_none() && self.loose_assets.is_empty() {
            return Ok(Value::Null);
        }
        let mut files = Vec::new();
        if let Some(archive) = self.archive.as_ref() {
            for file in archive.files() {
                let data = archive.file_data(file.index)?;
                files.push(json!({
                    "index": file.index,
                    "path": file.path,
                    "name": file.name,
                    "size": data.len(),
                    "decoded_size": data.len(),
                    "stored_size": file.stored_size,
                    "compressed": file.compressed,
                    "kind": asset_kind(&file.path, data),
                    "loose": false,
                }));
            }
        }
        for (offset, asset) in self.loose_assets.iter().enumerate() {
            let index = LOOSE_ASSET_BASE + offset;
            files.push(json!({
                "index": index,
                "path": asset.name,
                "name": asset.name,
                "size": asset.data.len(),
                "decoded_size": asset.data.len(),
                "stored_size": asset.data.len(),
                "compressed": false,
                "kind": asset_kind(&asset.name, &asset.data),
                "loose": true,
            }));
        }
        let selected_file = match self.selection {
            Selection::File(index) => Some(index),
            _ => None,
        };
        Ok(json!({
            "files": files,
            "selected_file": selected_file,
            "dirty": self.archive_is_dirty(),
            "bol_index": self.archive_bol_index,
            "outer_compression": self.archive.as_ref().map(|archive| compression_name(archive.original_outer_compression())),
        }))
    }

    fn entities_value(&self) -> FormatResult<Value> {
        let Some(course) = self.course.as_ref() else {
            return Ok(Value::Array(Vec::new()));
        };
        let mut entities = serde_json::to_value(course.scene_entities())
            .map_err(|error| {
                FormatError::new(format!("Could not serialize scene entities: {error}"))
            })?
            .as_array()
            .cloned()
            .unwrap_or_default();
        for entity in &mut entities {
            let Some(entity) = entity.as_object_mut() else {
                continue;
            };
            let kind = entity.get("kind").and_then(Value::as_str).unwrap_or_default().to_owned();
            let index = entity.get("index").and_then(Value::as_u64).unwrap_or(0) as usize;
            if kind == "object"
                && let Some(object) = course.objects.get(index)
            {
                let info = catalog::object_info(object.id);
                entity.insert(
                    "label".to_owned(),
                    json!(info.map_or_else(|| format!("Object 0x{:04x}", object.id), |info| info.name.to_owned())),
                );
                entity.insert("category".to_owned(), json!(info.map_or("Unknown", |info| info.category)));
                entity.insert("object_id".to_owned(), json!(object.id));
                entity.insert(
                    "has_model".to_owned(),
                    json!(self.object_models.get(index).is_some_and(Option::is_some)),
                );
            }
        }
        for (index, path) in course.paths.iter().enumerate() {
            let position = course
                .route_points_for(index)
                .ok()
                .and_then(|points| points.first())
                .map(|point| point.position);
            entities.push(json!({
                "kind": "path",
                "index": index,
                "label": format!("Path {index} ({} points)", path.point_count),
                "position": position,
                "secondary": Value::Null,
            }));
        }
        for (index, group) in course.checkpoint_groups.iter().enumerate() {
            entities.push(json!({
                "kind": "checkpoint_group",
                "index": index,
                "label": format!("Checkpoint group {index} ({} points)", group.points.len()),
                "position": group.points.first().map(|point| point.start),
                "secondary": Value::Null,
            }));
        }
        for (index, light) in course.lights.iter().enumerate() {
            entities.push(json!({
                "kind": "light",
                "index": index,
                "label": format!("Light parameter {index}"),
                "position": light.position,
                "secondary": Value::Null,
            }));
        }
        for (index, _) in course.mg.iter().enumerate() {
            entities.push(json!({
                "kind": "mg",
                "index": index,
                "label": format!("MG parameter {index}"),
                "position": Value::Null,
                "secondary": Value::Null,
            }));
        }
        Ok(Value::Array(entities))
    }

    fn issues_value(&self) -> Value {
        let Some(course) = self.course.as_ref() else {
            return Value::Array(Vec::new());
        };
        let report = course.validate();
        let mut issues = Vec::new();
        for (severity, values) in [("error", &report.errors), ("warning", &report.warnings)] {
            issues.extend(values.iter().map(|issue| {
                json!({
                    "severity": severity,
                    "code": issue.code,
                    "message": issue.message,
                    "kind": issue.kind,
                    "index": issue.index,
                })
            }));
        }
        if let Some(warning) = self.course_model_warning.as_ref() {
            issues.push(json!({
                "severity": "warning",
                "code": "course_model_preview",
                "message": warning,
                "kind": "course_model",
                "index": self.course_model_archive_index,
            }));
        }
        Value::Array(issues)
    }

    fn selection_value(&self) -> FormatResult<Value> {
        match &self.selection {
            Selection::None => Ok(Value::Null),
            Selection::Course { kind, index } => {
                let course = self
                    .course
                    .as_ref()
                    .ok_or_else(|| FormatError::new("No BOL course is open"))?;
                let inspection = course.inspect(kind, *index)?;
                let fields = inspection_fields(course, &inspection);
                let title = selection_title(course, &inspection);
                let raw_hex = record_range(course, kind, *index)
                    .and_then(|range| course.bytes().get(range))
                    .map(|bytes| hex_preview(bytes, usize::MAX))
                    .unwrap_or_default();
                Ok(json!({
                    "title": title,
                    "kind": inspection.kind,
                    "index": inspection.index,
                    "fields": fields,
                    "raw_hex": raw_hex,
                }))
            }
            Selection::File(index) => {
                let index = *index;
                let name = self.asset_name(index)?;
                let data = self.asset_bytes(index)?;
                let kind = asset_kind(&name, data);
                let (stored_size, compressed) = if index < LOOSE_ASSET_BASE {
                    self.archive
                        .as_ref()
                        .and_then(|archive| archive.files().get(index))
                        .map(|file| (file.stored_size, file.compressed))
                        .unwrap_or((data.len() as u32, false))
                } else {
                    (data.len() as u32, false)
                };
                let fields = vec![
                    read_only_field("path", "Path", "string", json!(name)),
                    read_only_field("index", "Archive index", "usize", json!(index)),
                    read_only_field("size", "Decoded size", "usize", json!(data.len())),
                    read_only_field("stored_size", "Stored size", "u32", json!(stored_size)),
                    read_only_field("compressed", "Compressed", "bool", json!(compressed)),
                    read_only_field("kind", "Kind", "string", json!(kind)),
                ];
                Ok(json!({
                    "title": name,
                    "kind": "file",
                    "index": index,
                    "fields": fields,
                    "raw_hex": hex_preview(data, FILE_HEX_PREVIEW_BYTES),
                }))
            }
        }
    }

    fn selected_asset_value(&self) -> FormatResult<Value> {
        let Selection::File(index) = self.selection else {
            return Ok(Value::Null);
        };
        let name = self.asset_name(index)?;
        let data = self.asset_bytes(index)?;
        match asset_kind(&name, data) {
            "bti" => match BtiImage::decode(data) {
                Ok(image) => Ok(json!({
                    "index": index,
                    "name": name,
                    "kind": "bti",
                    "width": image.width,
                    "height": image.height,
                    "format": image.format_name,
                    "format_code": image.format,
                    "info": {
                        "transparency": image.transparency,
                        "wrap_s": image.wrap_s,
                        "wrap_t": image.wrap_t,
                        "palette_format": image.palette_format,
                        "lut_format": image.lut_format,
                        "palette_entry_count": image.palette_entry_count,
                        "palette_offset": image.palette_offset,
                        "mipmap": image.mipmap,
                        "edge_lod": image.edge_lod,
                        "bias_clamp": image.bias_clamp,
                        "max_anisotropy": image.max_anisotropy,
                        "min_filter": image.min_filter,
                        "mag_filter": image.mag_filter,
                        "min_lod": image.min_lod,
                        "max_lod": image.max_lod,
                        "image_count": image.image_count,
                        "lod_bias": image.lod_bias,
                        "image_data_offset": image.image_data_offset,
                    }
                })),
                Err(error) => Ok(json!({
                    "index": index,
                    "name": name,
                    "kind": "bti",
                    "info": format!("BTI decode error: {error}"),
                })),
            },
            "j3d" => match j3d::inspect(data) {
                Ok(info) => {
                    let blocks = info
                        .blocks
                        .iter()
                        .map(|block| {
                            json!({
                                "tag": block.tag,
                                "offset": block.offset,
                                "size": block.size,
                                "end_offset": block.end_offset(),
                            })
                        })
                        .collect::<Vec<_>>();
                    let kind = if info.is_model() {
                        "model"
                    } else if info.is_animation() {
                        "animation"
                    } else {
                        "other"
                    };
                    Ok(json!({
                        "index": index,
                        "name": name,
                        "kind": "j3d",
                        "format": info.revision,
                        "container": info.container,
                        "revision": info.revision,
                        "j3d_kind": kind,
                        "declared_size": info.declared_size,
                        "declared_blocks": info.declared_blocks,
                        "trailing_bytes": info.trailing_bytes,
                        "blocks": blocks,
                        "bti_offsets": info.bti_offsets,
                        "bti_data_offsets": info.bti_data_offsets,
                        "info": {
                            "container": info.container,
                            "revision": info.revision,
                            "kind": kind,
                            "declared_size": info.declared_size,
                            "declared_blocks": info.declared_blocks,
                            "trailing_bytes": info.trailing_bytes,
                            "texture_count": info.bti_offsets.len(),
                        }
                    }))
                }
                Err(error) => Ok(json!({
                    "index": index,
                    "name": name,
                    "kind": "j3d",
                    "info": format!("J3D inspection error: {error}"),
                })),
            },
            _ => Ok(Value::Null),
        }
    }

    fn layers_value(&self) -> Value {
        LAYER_NAMES
            .iter()
            .map(|(name, label)| {
                json!({"name": name, "label": label, "visible": self.visibility.get(name).unwrap_or(false)})
            })
            .collect()
    }
}

fn push_bounded(history: &mut VecDeque<HistoryState>, state: HistoryState) {
    while history.len() >= HISTORY_LIMIT {
        history.pop_front();
    }
    history.push_back(state);
}

#[allow(clippy::type_complexity)]
fn open_archive_course(
    archive: &RarcArchive,
) -> FormatResult<(
    BolDocument,
    usize,
    Option<Bco>,
    Option<usize>,
    Option<Rc<BmdModel>>,
    Option<usize>,
    Option<String>,
)> {
    let mut candidates = Vec::new();
    for file in archive.files() {
        if file.name.to_ascii_lowercase().ends_with("_course.bol") {
            candidates.push(file.index);
        }
    }
    for file in archive.files() {
        if extension(&file.name).as_deref() == Some("bol") && !candidates.contains(&file.index) {
            candidates.push(file.index);
        }
    }
    for file in archive.files() {
        if archive
            .file_data(file.index)
            .is_ok_and(|data| data.starts_with(b"0015"))
            && !candidates.contains(&file.index)
        {
            candidates.push(file.index);
        }
    }

    let mut first_error = None;
    for index in candidates {
        let data = archive.file_data(index)?;
        match BolDocument::parse(data) {
            Ok(course) => {
                let bco_index = matching_bco(archive, index);
                let collision = match bco_index {
                    Some(bco_index) => {
                        let path = archive
                            .files()
                            .get(bco_index)
                            .map(|file| file.path.as_str())
                            .unwrap_or("BCO");
                        let collision =
                            Bco::parse(archive.file_data(bco_index)?).map_err(|error| {
                                FormatError::new(format!(
                                    "Could not parse matching BCO '{path}': {error}"
                                ))
                            })?;
                        Some(collision)
                    }
                    None => None,
                };
                let bmd_index = matching_bmd(archive, index);
                let (course_model, course_model_warning) = match bmd_index {
                    Some(bmd_index) => archive_course_model(archive, bmd_index),
                    None => (None, None),
                };
                return Ok((
                    course,
                    index,
                    collision,
                    bco_index,
                    course_model,
                    bmd_index,
                    course_model_warning,
                ));
            }
            Err(error) => {
                if first_error.is_none() {
                    let path = archive
                        .files()
                        .get(index)
                        .map(|file| file.path.as_str())
                        .unwrap_or("BOL");
                    first_error = Some(format!("{path}: {error}"));
                }
            }
        }
    }
    Err(FormatError::new(match first_error {
        Some(error) => format!("No readable BOL course was found in the ARC ({error})"),
        None => "The ARC does not contain a BOL course file".to_owned(),
    }))
}

fn archive_course_model(
    archive: &RarcArchive,
    index: usize,
) -> (Option<Rc<BmdModel>>, Option<String>) {
    let path = archive
        .files()
        .get(index)
        .map(|file| file.path.as_str())
        .unwrap_or("BMD");
    match archive.file_data(index) {
        Ok(data) => course_model_preview(path, data),
        Err(error) => (
            None,
            Some(format!(
                "Could not read matching course BMD '{path}': {error}"
            )),
        ),
    }
}

fn course_model_preview(name: &str, data: &[u8]) -> (Option<Rc<BmdModel>>, Option<String>) {
    match BmdModel::parse(data) {
        Ok(model) => {
            let warning = (!model.warnings.is_empty()).then(|| {
                format!(
                    "Course BMD '{name}' uses approximate preview rendering: {}",
                    model.warnings.join("; ")
                )
            });
            (Some(Rc::new(model)), warning)
        }
        Err(error) => (
            None,
            Some(format!("Could not parse course BMD '{name}': {error}")),
        ),
    }
}

fn matching_bmd(archive: &RarcArchive, bol_index: usize) -> Option<usize> {
    let bol = archive.files().get(bol_index)?;
    let bol_path = bol.path.replace('\\', "/");
    let (directory, file_name) = bol_path
        .rsplit_once('/')
        .map_or(("", bol_path.as_str()), |(directory, name)| {
            (directory, name)
        });
    let stem = file_name
        .rsplit_once('.')
        .filter(|(_, extension)| extension.eq_ignore_ascii_case("bol"))
        .map_or(file_name, |(stem, _)| stem);
    let mut desired = vec![format!("{stem}.bmd")];
    if stem.to_ascii_lowercase().ends_with("_course") {
        let short_stem = &stem[..stem.len() - "_course".len()];
        desired.push(format!("{short_stem}.bmd"));
    }
    for wanted in desired {
        for file in archive.files() {
            let path = file.path.replace('\\', "/");
            let (other_directory, other_name) = path
                .rsplit_once('/')
                .map_or(("", path.as_str()), |(directory, name)| (directory, name));
            if other_directory.eq_ignore_ascii_case(directory)
                && other_name.eq_ignore_ascii_case(&wanted)
            {
                return Some(file.index);
            }
        }
    }
    None
}

fn matching_bco(archive: &RarcArchive, bol_index: usize) -> Option<usize> {
    let bol = archive.files().get(bol_index)?;
    let bol_path = bol.path.replace('\\', "/");
    let (directory, file_name) = bol_path
        .rsplit_once('/')
        .map_or(("", bol_path.as_str()), |(directory, name)| {
            (directory, name)
        });
    let stem = file_name
        .strip_suffix(".bol")
        .or_else(|| file_name.strip_suffix(".BOL"))
        .unwrap_or(file_name);
    let mut desired = vec![format!("{stem}.bco")];
    if let Some(short_stem) = stem.strip_suffix("_course") {
        desired.push(format!("{short_stem}.bco"));
    }
    for wanted in desired {
        for file in archive.files() {
            let path = file.path.replace('\\', "/");
            let (other_directory, other_name) = path
                .rsplit_once('/')
                .map_or(("", path.as_str()), |(directory, name)| (directory, name));
            if other_directory.eq_ignore_ascii_case(directory)
                && other_name.eq_ignore_ascii_case(&wanted)
            {
                return Some(file.index);
            }
        }
    }

    let mut same_directory = archive.files().iter().filter(|file| {
        let path = file.path.replace('\\', "/");
        let (other_directory, _) = path
            .rsplit_once('/')
            .map_or(("", path.as_str()), |(directory, name)| (directory, name));
        other_directory.eq_ignore_ascii_case(directory)
            && extension(&file.name).as_deref() == Some("bco")
    });
    let first = same_directory.next();
    if first.is_some() && same_directory.next().is_none() {
        return first.map(|file| file.index);
    }
    let mut all = archive
        .files()
        .iter()
        .filter(|file| extension(&file.name).as_deref() == Some("bco"));
    let first = all.next();
    if first.is_some() && all.next().is_none() {
        first.map(|file| file.index)
    } else {
        None
    }
}

fn apply_field_edit(
    course: &mut BolDocument,
    kind: &str,
    index: usize,
    key: &str,
    value: Value,
) -> FormatResult<()> {
    if matches!(kind, "object" | "objects")
        && let Some(slot) = key
            .strip_prefix("params.")
            .or_else(|| key.strip_prefix("param."))
    {
        let slot = slot
            .parse::<usize>()
            .map_err(|_| FormatError::new(format!("Invalid object parameter slot '{slot}'")))?;
        if slot >= 8 {
            return Err(FormatError::new(format!(
                "Object parameter slot {slot} is outside 0..8"
            )));
        }
        let parameter = serde_json::from_value::<i16>(value).map_err(|error| {
            FormatError::new(format!("Invalid signed 16-bit object parameter: {error}"))
        })?;
        let object = course
            .objects
            .get(index)
            .ok_or_else(|| FormatError::new(format!("object index {index} out of bounds")))?;
        let mut params = object.params;
        params[slot] = parameter;
        let values = params.into_iter().map(Value::from).collect();
        return course.set_field(kind, index, "params", Value::Array(values));
    }
    if let Some(component) = key.strip_prefix("rotation.") {
        let axis = match component {
            "yaw" => 0,
            "pitch" => 1,
            "roll" => 2,
            _ => return Err(FormatError::new(format!("Unknown rotation component '{component}'"))),
        };
        let degrees = value
            .as_f64()
            .filter(|value| value.is_finite())
            .ok_or_else(|| FormatError::new("Rotation angles must be finite numbers"))?
            as f32;
        let current = transform::read(course, kind, index)
            .ok_or_else(|| FormatError::new(format!("{kind} {index} has no rotation")))?;
        let (forward, up) = current
            .directions
            .ok_or_else(|| FormatError::new(format!("{kind} records have no rotation")))?;
        let mut euler = transform::euler_degrees(forward, up);
        euler[axis] = degrees;
        let updated = EntityTransform {
            directions: Some(transform::directions_from_euler(euler)),
            ..current
        };
        return transform::write(course, kind, index, &updated);
    }
    course.set_field(kind, index, key, value)
}

fn validate_replacement(name: &str, current: &[u8], replacement: &[u8]) -> FormatResult<()> {
    match asset_kind(name, current) {
        "bti" => {
            BtiImage::decode(replacement)?;
        }
        "j3d" => {
            let current_info = j3d::inspect(current)?;
            let replacement_info = j3d::inspect(replacement)?;
            if current_info.revision != replacement_info.revision {
                return Err(FormatError::new(format!(
                    "J3D revision changed from '{}' to '{}'; choose a replacement of the same asset type",
                    current_info.revision, replacement_info.revision
                )));
            }
        }
        "bco" => {
            Bco::parse(replacement)?;
        }
        "bol" => {
            BolDocument::parse(replacement)?;
        }
        "arc" => {
            RarcArchive::new(replacement)?;
        }
        _ => {}
    }
    Ok(())
}

fn inspection_fields(course: &BolDocument, inspection: &Inspection) -> Vec<Value> {
    let object_id = if inspection.kind == "object" {
        course.objects.get(inspection.index).map(|object| object.id)
    } else {
        None
    };
    let parameter_labels = object_id.map(catalog::parameter_labels);
    let mut fields = Vec::new();
    if let Some(transform) = transform::read(course, &inspection.kind, inspection.index)
        && let Some((forward, up)) = transform.directions
    {
        let euler = transform::euler_degrees(forward, up);
        for (axis, (name, label)) in [("yaw", "Yaw"), ("pitch", "Pitch"), ("roll", "Roll")]
            .into_iter()
            .enumerate()
        {
            let mut field = editable_field(
                &format!("rotation.{name}"),
                &format!("Rotation {label}"),
                "f32",
                json!(euler[axis]),
                true,
            );
            if let Some(object) = field.as_object_mut() {
                object.insert("group".to_owned(), json!("Transform"));
                object.insert("step".to_owned(), json!(1));
                object.insert("unit".to_owned(), json!("°"));
            }
            fields.push(field);
        }
    }
    let group_of = |key: &str| -> &'static str {
        match key {
            "position" | "scale" | "start" | "end" | "target_position" | "target_position2" => "Transform",
            "forward" | "up" | "direction" => "Advanced",
            "params" => "Parameters",
            _ if key.starts_with("padding") || key.starts_with("unknown") => "Advanced",
            _ => "Properties",
        }
    };
    for field in &inspection.fields {
        if matches!(field.value_type.as_str(), "f32_vec3" | "i16_vec3")
            && let Some(values) = field.value.as_array()
        {
            for (component, label) in ["x", "y", "z"].into_iter().enumerate() {
                if let Some(value) = values.get(component) {
                    fields.push(editable_field(
                        &format!("{}.{}", field.key, label),
                        &format!("{} {label}", humanize(&field.key)),
                        if field.value_type == "f32_vec3" {
                            "f32"
                        } else {
                            "i16"
                        },
                        value.clone(),
                        field.editable,
                    ));
                }
            }
            continue;
        }
        if field.key == "params"
            && inspection.kind == "object"
            && let Some(values) = field.value.as_array()
        {
            for (slot, value) in values.iter().take(8).enumerate() {
                let label = parameter_labels
                    .as_ref()
                    .map(|labels| labels[slot])
                    .unwrap_or("Object parameter");
                let mut result = editable_field(
                    &format!("params.{slot}"),
                    label,
                    "i16",
                    value.clone(),
                    field.editable,
                );
                if let Some(object) = result.as_object_mut() {
                    object.insert("min".to_owned(), json!(i16::MIN));
                    object.insert("max".to_owned(), json!(i16::MAX));
                    object.insert("step".to_owned(), json!(1));
                }
                fields.push(result);
            }
            continue;
        }
        let label = if inspection.kind == "object" && field.key == "id" {
            "Object ID".to_owned()
        } else {
            humanize(&field.key)
        };
        let mut result = editable_field(
            &field.key,
            &label,
            &field.value_type,
            field.value.clone(),
            field.editable,
        );
        if inspection.kind == "object"
            && field.key == "id"
            && let Some(object) = result.as_object_mut()
        {
            object.insert("options".to_owned(), json!("object_catalog"));
            if let Some(name) = object_id.and_then(catalog::object_name) {
                object.insert("description".to_owned(), json!(name));
            }
        }
        fields.push(result);
    }
    for value in &mut fields {
        if let Some(object) = value.as_object_mut() {
            let key = object.get("key").and_then(Value::as_str).unwrap_or_default();
            let group = group_of(key.split('.').next().unwrap_or(key));
            object.entry("group").or_insert_with(|| json!(group));
        }
    }
    fields
}

fn editable_field(key: &str, label: &str, value_type: &str, value: Value, editable: bool) -> Value {
    json!({
        "key": key,
        "label": label,
        "type": value_type,
        "value": value,
        "disabled": !editable,
        "read_only": !editable,
        "readonly": !editable,
    })
}

fn read_only_field(key: &str, label: &str, value_type: &str, value: Value) -> Value {
    editable_field(key, label, value_type, value, false)
}

fn selection_title(course: &BolDocument, inspection: &Inspection) -> String {
    if inspection.kind == "header" {
        return "Course header".to_owned();
    }
    if inspection.kind == "object"
        && let Some(object) = course.objects.get(inspection.index)
    {
        if let Some(name) = catalog::object_name(object.id) {
            return name.to_owned();
        }
        return format!("Object {} (0x{:04x})", inspection.index, object.id);
    }
    format!("{} {}", humanize(&inspection.kind), inspection.index)
}

fn object_catalog(library: &ModelLibrary) -> Vec<Value> {
    catalog::objects()
        .iter()
        .map(|info| {
            let model = catalog::model_names(info.id)
                .into_iter()
                .find(|name| library.contains(name));
            json!({
                "model": model,
                "id": info.id,
                "value": info.id,
                "name": info.name,
                "label": format!("0x{:04x} — {}", info.id, info.name),
                "class_name": info.class_name,
                "category": info.category,
                "placeable": info.placeable,
            })
        })
        .collect()
}

fn record_range(course: &BolDocument, kind: &str, index: usize) -> Option<Range<usize>> {
    let offsets = course.offsets;
    let (base, size) = match kind {
        "header" if index == 0 => (0, 0x7c),
        "centerline" => (offsets.centerline as usize, 0x20),
        "checkpoint_group" => (offsets.checkpoints as usize, 0x14),
        "checkpoint" => (
            (offsets.checkpoints as usize)
                .checked_add(course.checkpoint_groups.len().checked_mul(0x14)?)?,
            0x1c,
        ),
        "path" => (offsets.paths as usize, 0x10),
        "route_point" => (offsets.route_points as usize, 0x20),
        "object" => (offsets.objects as usize, 0x40),
        "start" => (offsets.starts as usize, 0x28),
        "area" => (offsets.areas as usize, 0x38),
        "camera" => (offsets.cameras as usize, 0x48),
        "respawn" => (offsets.respawns as usize, 0x20),
        "light" => (offsets.lights as usize, 0x14),
        "mg" => (offsets.mg as usize, 0x08),
        _ => return None,
    };
    let start = base.checked_add(index.checked_mul(size)?)?;
    let end = start.checked_add(size)?;
    Some(start..end)
}

fn hex_preview(data: &[u8], maximum: usize) -> String {
    let shown = data.len().min(maximum);
    let mut output = String::new();
    for (index, byte) in data[..shown].iter().enumerate() {
        if index != 0 {
            output.push(' ');
        }
        let _ = write!(output, "{byte:02x}");
    }
    if shown < data.len() {
        let _ = write!(output, " … ({} more bytes)", data.len() - shown);
    }
    output
}

fn asset_kind(name: &str, data: &[u8]) -> &'static str {
    if data.starts_with(b"0015") {
        "bol"
    } else if data.starts_with(b"0003") {
        "bco"
    } else if data.starts_with(b"RARC") || data.starts_with(b"Yaz0") && name.ends_with(".arc") {
        "arc"
    } else if data.starts_with(b"J3D1") || data.starts_with(b"J3D2") {
        "j3d"
    } else {
        match extension(name).as_deref() {
            Some("bol") => "bol",
            Some("bco") => "bco",
            Some("bti") => "bti",
            Some("bmd" | "bdl" | "bck" | "bca" | "blk" | "bpk" | "btk" | "brk" | "btp" | "bva") => {
                "j3d"
            }
            Some("arc" | "rarc" | "szs") => "arc",
            _ => "data",
        }
    }
}

fn extension(name: &str) -> Option<String> {
    let file_name = name.rsplit(['/', '\\']).next().unwrap_or(name);
    let (_, extension) = file_name.rsplit_once('.')?;
    if extension.is_empty() {
        None
    } else {
        Some(extension.to_ascii_lowercase())
    }
}

fn compression_name(compression: Compression) -> &'static str {
    match compression {
        Compression::None => "none",
        Compression::Yaz0 => "yaz0",
        Compression::Yay0 => "yay0",
    }
}

fn humanize(value: &str) -> String {
    value
        .split(['_', '-'])
        .filter(|part| !part.is_empty())
        .map(|part| {
            let mut characters = part.chars();
            match characters.next() {
                Some(first) => format!("{}{}", first.to_uppercase(), characters.as_str()),
                None => String::new(),
            }
        })
        .collect::<Vec<_>>()
        .join(" ")
}

fn js_value_to_json(value: &JsValue) -> Result<Value, JsValue> {
    let text = js_sys::JSON::stringify(value).map_err(|error| {
        JsValue::from_str(&format!(
            "The field value is not JSON-serializable: {}",
            javascript_error(error)
        ))
    })?;
    let text = text.as_string().ok_or_else(|| {
        JsValue::from_str("The field value must be JSON-serializable (received undefined)")
    })?;
    serde_json::from_str(&text)
        .map_err(|error| JsValue::from_str(&format!("The field value is not valid JSON: {error}")))
}

fn javascript_error(error: JsValue) -> String {
    error.as_string().unwrap_or_else(|| format!("{error:?}"))
}

fn js_error(error: impl Display) -> JsValue {
    JsValue::from_str(&error.to_string())
}

#[derive(Clone, Copy, Debug)]
struct SnapSettings {
    enabled: bool,
    translate: f32,
    rotate: f32,
}

impl Default for SnapSettings {
    fn default() -> Self {
        Self {
            enabled: false,
            translate: 50.0,
            rotate: 15.0,
        }
    }
}

struct DragState {
    kind: String,
    index: usize,
    handle: GizmoHandle,
    mode: GizmoMode,
    start: EntityTransform,
    before: HistoryState,
    start_mouse: (f32, f32),
    /// First valid hit on the drag axis/plane; movement is measured from it.
    anchor: Option<Vec3>,
    size: f32,
}

struct LibraryModel {
    data: Rc<[u8]>,
    parsed: RefCell<Option<Option<Rc<BmdModel>>>>,
}

impl LibraryModel {
    fn model(&self) -> Option<Rc<BmdModel>> {
        self.parsed
            .borrow_mut()
            .get_or_insert_with(|| BmdModel::parse(&self.data).ok().map(Rc::new))
            .clone()
    }
}

/// BMD models available for drawing course objects, keyed by lower-case
/// base file name. Course-archive models shadow shared libraries.
#[derive(Default)]
struct ModelLibrary {
    course: HashMap<String, LibraryModel>,
    shared: Vec<(String, HashMap<String, LibraryModel>)>,
}

impl ModelLibrary {
    fn models_in(archive: &RarcArchive) -> HashMap<String, LibraryModel> {
        let mut models = HashMap::new();
        for file in archive.files() {
            let lower = file.name.to_ascii_lowercase();
            let Some(stem) = lower
                .strip_suffix(".bmd")
                .or_else(|| lower.strip_suffix(".bdl"))
            else {
                continue;
            };
            if let Ok(data) = archive.file_data(file.index) {
                models.entry(stem.to_owned()).or_insert_with(|| LibraryModel {
                    data: Rc::from(data),
                    parsed: RefCell::new(None),
                });
            }
        }
        models
    }

    fn set_course(&mut self, archive: Option<&RarcArchive>) {
        self.course = archive.map(Self::models_in).unwrap_or_default();
    }

    fn add_shared(&mut self, name: &str, archive: &RarcArchive) -> usize {
        let models = Self::models_in(archive);
        let count = models.len();
        self.shared.retain(|(existing, _)| existing != name);
        self.shared.push((name.to_owned(), models));
        count
    }

    fn remove_shared(&mut self, name: &str) {
        self.shared.retain(|(existing, _)| existing != name);
    }

    fn contains(&self, name: &str) -> bool {
        self.course.contains_key(name) || self.shared.iter().any(|(_, models)| models.contains_key(name))
    }

    fn get(&self, name: &str) -> Option<Rc<BmdModel>> {
        if let Some(model) = self.course.get(name).and_then(LibraryModel::model) {
            return Some(model);
        }
        self.shared
            .iter()
            .rev()
            .find_map(|(_, models)| models.get(name).and_then(LibraryModel::model))
    }

    fn sky(&self) -> Option<Rc<BmdModel>> {
        let mut names: Vec<&String> = self.course.keys().filter(|name| name.ends_with("_sky")).collect();
        names.sort();
        names.first().and_then(|name| self.course.get(*name)).and_then(LibraryModel::model)
    }

    fn value(&self) -> Value {
        json!({
            "course_models": self.course.len(),
            "archives": self.shared.iter().map(|(name, models)| json!({
                "name": name,
                "models": models.len(),
            })).collect::<Vec<_>>(),
        })
    }
}

/// The point a gizmo handle measures drags against: the hit on its axis line
/// or plane.
fn drag_point(
    mode: GizmoMode,
    handle: GizmoHandle,
    center: Vec3,
    origin: Vec3,
    direction: Vec3,
    size: f32,
) -> Option<Vec3> {
    match (mode, handle.axis()) {
        (GizmoMode::Translate, None) => ray_plane(origin, direction, center, Vec3::Y),
        (GizmoMode::Translate | GizmoMode::Scale, Some(axis)) => {
            closest_on_axis(origin, direction, center, axis).map(|t| center + axis * t)
        }
        (GizmoMode::Rotate, axis) => ray_plane(origin, direction, center, axis.unwrap_or(Vec3::Y))
            .filter(|point| (*point - center).length() > size * 0.05),
        (GizmoMode::Scale, None) => None,
    }
}

fn ray_plane(origin: Vec3, direction: Vec3, point: Vec3, normal: Vec3) -> Option<Vec3> {
    let denominator = direction.dot(normal);
    if denominator.abs() < 1e-6 {
        return None;
    }
    let t = (point - origin).dot(normal) / denominator;
    (t > 0.0).then(|| origin + direction * t)
}

/// Parameter along `axis` (through `point`) closest to the ray.
fn closest_on_axis(origin: Vec3, direction: Vec3, point: Vec3, axis: Vec3) -> Option<f32> {
    let w = origin - point;
    let a = direction.dot(direction);
    let b = direction.dot(axis);
    let c = axis.dot(axis);
    let d = direction.dot(w);
    let e = axis.dot(w);
    let denominator = a * c - b * b;
    if denominator.abs() < 1e-6 {
        return None;
    }
    Some((a * e - b * d) / denominator)
}

fn point_in_quad(point: glam::Vec2, quad: [glam::Vec2; 4]) -> bool {
    let mut sign = 0.0f32;
    for corner in 0..4 {
        let a = quad[corner];
        let b = quad[(corner + 1) % 4];
        let cross = (b - a).perp_dot(point - a);
        if cross.abs() < 1e-6 {
            continue;
        }
        if sign == 0.0 {
            sign = cross.signum();
        } else if cross.signum() != sign {
            return false;
        }
    }
    true
}
