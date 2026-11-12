//! Spatial helpers shared by the viewport, gizmos, and inspector.
//!
//! BOL records store orientation as two s16 direction vectors scaled by
//! 10000: `forward` (the model Z axis) and `up` (Y). The model X axis is
//! `up × forward`, matching `GeographyObj::getAssignedModelMatrix`.

use glam::{Mat3, Mat4, Vec3};
use serde_json::{Value, json};

use crate::error::{FormatError, Result};
use crate::formats::bol::BolDocument;

pub const DIRECTION_SCALE: f32 = 10_000.0;

pub fn decode_direction(value: [i16; 3]) -> Vec3 {
    Vec3::new(
        f32::from(value[0]),
        f32::from(value[1]),
        f32::from(value[2]),
    ) / DIRECTION_SCALE
}

pub fn encode_direction(value: Vec3) -> [i16; 3] {
    let normalized = value.normalize_or_zero();
    (normalized * DIRECTION_SCALE)
        .round()
        .clamp(Vec3::splat(-32_768.0), Vec3::splat(32_767.0))
        .to_array()
        .map(|component| component as i16)
}

/// Orthonormal (x, y, z) axes from BOL forward/up vectors, tolerating
/// degenerate or non-orthogonal input.
pub fn basis(forward: Vec3, up: Vec3) -> (Vec3, Vec3, Vec3) {
    let z = forward.try_normalize().unwrap_or(Vec3::Z);
    let mut x = up.cross(z);
    if x.length_squared() < 1e-8 {
        x = if z.y.abs() < 0.99 {
            Vec3::Y.cross(z)
        } else {
            Vec3::X
        };
    }
    let x = x.normalize();
    let y = z.cross(x).normalize();
    (x, y, z)
}

pub fn model_matrix(position: Vec3, forward: Vec3, up: Vec3, scale: Vec3) -> Mat4 {
    let (x, y, z) = basis(forward, up);
    Mat4::from_cols(
        (x * scale.x).extend(0.0),
        (y * scale.y).extend(0.0),
        (z * scale.z).extend(0.0),
        position.extend(1.0),
    )
}

/// Yaw (about Y), pitch (about X), roll (about Z) in degrees for a rotation
/// `Ry(yaw) * Rx(pitch) * Rz(roll)` that maps +Z to forward and +Y to up.
pub fn euler_degrees(forward: Vec3, up: Vec3) -> [f32; 3] {
    let (_, y, z) = basis(forward, up);
    let yaw = z.x.atan2(z.z);
    let pitch = (-z.y).atan2((z.x * z.x + z.z * z.z).sqrt());
    let frame = Mat3::from_rotation_y(yaw) * Mat3::from_rotation_x(pitch);
    let local_up = frame.transpose() * y;
    let roll = (-local_up.x).atan2(local_up.y);
    [yaw, pitch, roll].map(|angle| tidy_degrees(angle.to_degrees()))
}

pub fn directions_from_euler(degrees: [f32; 3]) -> (Vec3, Vec3) {
    let [yaw, pitch, roll] = degrees.map(f32::to_radians);
    let rotation =
        Mat3::from_rotation_y(yaw) * Mat3::from_rotation_x(pitch) * Mat3::from_rotation_z(roll);
    (rotation * Vec3::Z, rotation * Vec3::Y)
}

fn tidy_degrees(value: f32) -> f32 {
    let rounded = (value * 100.0).round() / 100.0;
    if rounded == -0.0 { 0.0 } else { rounded }
}

/// Editable spatial state of one BOL record.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct EntityTransform {
    pub position: Vec3,
    /// `(forward, up)` for records with an orientation.
    pub directions: Option<(Vec3, Vec3)>,
    pub scale: Option<Vec3>,
    /// A second point that moves with the record (checkpoint end, camera
    /// target, area/start have none).
    pub secondary: Option<Vec3>,
}

impl EntityTransform {
    pub fn matrix(&self) -> Mat4 {
        let (forward, up) = self.directions.unwrap_or((Vec3::Z, Vec3::Y));
        model_matrix(self.position, forward, up, self.scale.unwrap_or(Vec3::ONE))
    }

    pub fn translated(mut self, delta: Vec3) -> Self {
        self.position += delta;
        if let Some(secondary) = &mut self.secondary {
            *secondary += delta;
        }
        self
    }

    pub fn rotated(mut self, axis: Vec3, angle: f32) -> Self {
        let rotation = Mat3::from_axis_angle(axis.normalize_or(Vec3::Y), angle);
        if let Some((forward, up)) = &mut self.directions {
            *forward = rotation * *forward;
            *up = rotation * *up;
        }
        if let Some(secondary) = &mut self.secondary {
            *secondary = self.position + rotation * (*secondary - self.position);
        }
        self
    }
}

pub fn read(course: &BolDocument, kind: &str, index: usize) -> Option<EntityTransform> {
    let v = Vec3::from_array;
    let oriented = |position, forward, up, scale: Option<[f32; 3]>| EntityTransform {
        position: v(position),
        directions: Some((decode_direction(forward), decode_direction(up))),
        scale: scale.map(v),
        secondary: None,
    };
    let point = |position| EntityTransform {
        position: v(position),
        directions: None,
        scale: None,
        secondary: None,
    };
    Some(match kind {
        "object" => {
            let o = course.objects.get(index)?;
            oriented(o.position, o.forward, o.up, Some(o.scale))
        }
        "start" => {
            let s = course.starts.get(index)?;
            oriented(s.position, s.forward, s.up, Some(s.scale))
        }
        "area" => {
            let a = course.areas.get(index)?;
            oriented(a.position, a.forward, a.up, Some(a.scale))
        }
        "respawn" => {
            let r = course.respawns.get(index)?;
            oriented(r.position, r.forward, r.up, None)
        }
        "camera" => {
            let c = course.cameras.get(index)?;
            EntityTransform {
                secondary: Some(v(c.target_position)),
                ..point(c.position)
            }
        }
        "route_point" => point(course.route_points.get(index)?.position),
        "centerline" => {
            let c = course.centerline.get(index)?;
            EntityTransform {
                scale: Some(Vec3::splat(c.scale)),
                ..point(c.position)
            }
        }
        "checkpoint" => {
            let c = course.checkpoint(index)?;
            EntityTransform {
                position: v(c.start),
                secondary: Some(v(c.end)),
                directions: None,
                scale: None,
            }
        }
        _ => return None,
    })
}

/// Field edits that turn `old` into `new` for the given record kind.
pub fn field_edits(
    kind: &str,
    old: &EntityTransform,
    new: &EntityTransform,
) -> Vec<(&'static str, Value)> {
    let mut edits = Vec::new();
    let position_key = if kind == "checkpoint" {
        "start"
    } else {
        "position"
    };
    if old.position != new.position {
        edits.push((position_key, json!(new.position.to_array())));
    }
    if let (Some(old), Some(new)) = (old.directions, new.directions)
        && old != new
    {
        let (forward, up) = new;
        let (_, y, z) = basis(forward, up);
        edits.push(("forward", json!(encode_direction(z))));
        edits.push(("up", json!(encode_direction(y))));
    }
    if let (Some(old), Some(new)) = (old.scale, new.scale)
        && old != new
    {
        if kind == "centerline" {
            edits.push(("scale", json!(new.x)));
        } else {
            edits.push(("scale", json!(new.to_array())));
        }
    }
    if let (Some(old), Some(new)) = (old.secondary, new.secondary)
        && old != new
    {
        let key = if kind == "checkpoint" {
            "end"
        } else {
            "target_position"
        };
        edits.push((key, json!(new.to_array())));
    }
    edits
}

pub fn write(
    course: &mut BolDocument,
    kind: &str,
    index: usize,
    transform: &EntityTransform,
) -> Result<()> {
    let old = read(course, kind, index)
        .ok_or_else(|| FormatError::new(format!("{kind} {index} has no editable transform")))?;
    for (key, value) in field_edits(kind, &old, transform) {
        course.set_field(kind, index, key, value)?;
    }
    Ok(())
}

/// Ray-triangle intersection (Möller–Trumbore); returns the ray distance.
pub fn ray_triangle(origin: Vec3, direction: Vec3, triangle: [Vec3; 3]) -> Option<f32> {
    let [a, b, c] = triangle;
    let edge1 = b - a;
    let edge2 = c - a;
    let p = direction.cross(edge2);
    let determinant = edge1.dot(p);
    if determinant.abs() < 1e-9 {
        return None;
    }
    let inverse = 1.0 / determinant;
    let t_vector = origin - a;
    let u = t_vector.dot(p) * inverse;
    if !(0.0..=1.0).contains(&u) {
        return None;
    }
    let q = t_vector.cross(edge1);
    let v = direction.dot(q) * inverse;
    if v < 0.0 || u + v > 1.0 {
        return None;
    }
    let t = edge2.dot(q) * inverse;
    (t > 1e-4).then_some(t)
}

/// Ray against an oriented box given as a model matrix and local bounds.
pub fn ray_box(origin: Vec3, direction: Vec3, matrix: Mat4, min: Vec3, max: Vec3) -> Option<f32> {
    let inverse = matrix.inverse();
    if !inverse.is_finite() {
        return None;
    }
    let local_origin = inverse.transform_point3(origin);
    let local_direction = inverse.transform_vector3(direction);
    let mut near = f32::NEG_INFINITY;
    let mut far = f32::INFINITY;
    for axis in 0..3 {
        let o = local_origin[axis];
        let d = local_direction[axis];
        if d.abs() < 1e-12 {
            if o < min[axis] || o > max[axis] {
                return None;
            }
            continue;
        }
        let a = (min[axis] - o) / d;
        let b = (max[axis] - o) / d;
        near = near.max(a.min(b));
        far = far.min(a.max(b));
    }
    if near > far || far < 0.0 {
        return None;
    }
    // Distances are in local parameter space, which equals world space along
    // the same ray because only the origin/direction were transformed.
    Some(near.max(0.0))
}

#[cfg(test)]
mod tests {
    use super::*;

    fn close(a: Vec3, b: Vec3) -> bool {
        (a - b).length() < 1e-3
    }

    #[test]
    fn directions_round_trip_through_euler_angles() {
        for angles in [
            [0.0, 0.0, 0.0],
            [90.0, 0.0, 0.0],
            [35.0, -20.0, 10.0],
            [-120.0, 45.0, -30.0],
        ] {
            let (forward, up) = directions_from_euler(angles);
            let decoded = euler_degrees(forward, up);
            for (a, b) in angles.iter().zip(decoded) {
                assert!((a - b).abs() < 0.05, "{angles:?} -> {decoded:?}");
            }
        }
    }

    #[test]
    fn default_bol_directions_face_positive_x() {
        // MapObject::default uses forward = +X; yaw is therefore +90 degrees.
        let euler = euler_degrees(
            decode_direction([10_000, 0, 0]),
            decode_direction([0, 10_000, 0]),
        );
        assert_eq!(euler, [90.0, 0.0, 0.0]);
    }

    #[test]
    fn model_matrix_matches_geography_obj_axes() {
        let matrix = model_matrix(
            Vec3::new(1.0, 2.0, 3.0),
            Vec3::X,
            Vec3::Y,
            Vec3::new(2.0, 1.0, 1.0),
        );
        // X axis = up × forward = Y × X = -Z, scaled by 2.
        assert!(close(matrix.x_axis.truncate(), Vec3::new(0.0, 0.0, -2.0)));
        assert!(close(matrix.z_axis.truncate(), Vec3::X));
        assert!(close(matrix.w_axis.truncate(), Vec3::new(1.0, 2.0, 3.0)));
    }

    #[test]
    fn direction_encoding_is_unit_scaled_and_clamped() {
        assert_eq!(encode_direction(Vec3::new(0.0, 0.0, 5.0)), [0, 0, 10_000]);
        assert_eq!(encode_direction(Vec3::ZERO), [0, 0, 0]);
    }

    #[test]
    fn rays_hit_triangles_and_boxes() {
        let triangle = [
            Vec3::new(-1.0, 0.0, -1.0),
            Vec3::new(1.0, 0.0, -1.0),
            Vec3::new(0.0, 0.0, 1.0),
        ];
        assert_eq!(
            ray_triangle(Vec3::new(0.0, 5.0, 0.0), Vec3::NEG_Y, triangle),
            Some(5.0)
        );
        assert_eq!(
            ray_triangle(Vec3::new(5.0, 5.0, 0.0), Vec3::NEG_Y, triangle),
            None
        );
        let matrix = Mat4::from_translation(Vec3::new(10.0, 0.0, 0.0));
        let hit = ray_box(Vec3::ZERO, Vec3::X, matrix, Vec3::splat(-1.0), Vec3::ONE).unwrap();
        assert!((hit - 9.0).abs() < 1e-4);
        assert!(ray_box(Vec3::ZERO, Vec3::Y, matrix, Vec3::splat(-1.0), Vec3::ONE).is_none());
    }

    #[test]
    fn transforms_move_secondary_points_with_the_record() {
        let transform = EntityTransform {
            position: Vec3::ZERO,
            directions: Some((Vec3::Z, Vec3::Y)),
            scale: None,
            secondary: Some(Vec3::new(0.0, 0.0, 10.0)),
        };
        let moved = transform.translated(Vec3::X);
        assert_eq!(moved.secondary, Some(Vec3::new(1.0, 0.0, 10.0)));
        let turned = transform.rotated(Vec3::Y, std::f32::consts::FRAC_PI_2);
        assert!(close(turned.secondary.unwrap(), Vec3::new(10.0, 0.0, 0.0)));
        assert!(close(turned.directions.unwrap().0, Vec3::X));
    }
}
