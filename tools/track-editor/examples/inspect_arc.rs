//! Lists ARC contents and summarizes how each BMD/BDL decodes.
//!
//! Usage:
//!   cargo run --example inspect_arc -- <file.arc> [--warnings]
//!   cargo run --example inspect_arc -- --survey <file.arc>...
//!
//! `--survey` parses every model in every archive and prints aggregate GX
//! feature usage, which is useful for checking renderer coverage against a
//! user-owned corpus.
use std::collections::BTreeMap;

use mkdd_track_studio::formats::{bmd::BmdModel, rarc::RarcArchive};

fn is_model(path: &str) -> bool {
    let lower = path.to_ascii_lowercase();
    lower.ends_with(".bmd") || lower.ends_with(".bdl")
}

fn main() {
    let args: Vec<String> = std::env::args().skip(1).collect();
    if args.first().map(String::as_str) == Some("--dump") {
        dump_model(&args[1], &args[2]);
        return;
    }
    if args.first().map(String::as_str) == Some("--objects") {
        objects(&args[1..]);
        return;
    }
    if args.first().map(String::as_str) == Some("--survey") {
        survey(&args[1..]);
        return;
    }
    let path = args
        .first()
        .expect("usage: inspect_arc <file.arc> [--warnings]");
    let show_warnings = args.iter().any(|arg| arg == "--warnings");
    let archive =
        RarcArchive::new(std::fs::read(path).expect("read archive")).expect("parse archive");
    for file in archive.files() {
        let data = archive.file_data(file.index).unwrap_or_default();
        println!("{:4} {:8} {}", file.index, data.len(), file.path);
        if !is_model(&file.path) {
            continue;
        }
        match BmdModel::parse(data) {
            Ok(model) => {
                println!(
                    "       meshes {} textures {} warnings {}",
                    model.meshes.len(),
                    model.textures.len(),
                    model.warnings.len()
                );
                if show_warnings {
                    for warning in &model.warnings {
                        println!("       ! {warning}");
                    }
                }
            }
            Err(error) => println!("       BMD error: {error}"),
        }
    }
}

fn survey(paths: &[String]) {
    let mut models = 0usize;
    let mut failures = Vec::new();
    let mut features: BTreeMap<String, usize> = BTreeMap::new();
    let mut bump = |key: String| *features.entry(key).or_default() += 1;
    for path in paths {
        let Ok(bytes) = std::fs::read(path) else {
            failures.push(format!("{path}: unreadable"));
            continue;
        };
        let archive = match RarcArchive::new(bytes) {
            Ok(archive) => archive,
            Err(error) => {
                failures.push(format!("{path}: {error}"));
                continue;
            }
        };
        for file in archive.files().iter().filter(|file| is_model(&file.path)) {
            let data = archive.file_data(file.index).unwrap_or_default();
            let model = match BmdModel::parse(data) {
                Ok(model) => model,
                Err(error) => {
                    failures.push(format!("{path}:{}: {error}", file.path));
                    continue;
                }
            };
            models += 1;
            for mesh in &model.meshes {
                let gx = &mesh.material.gx;
                bump(format!("stages={}", gx.stage_count));
                bump(format!("channels={}", gx.channel_count));
                for texgen in &gx.tex_gens[..usize::from(gx.tex_gen_count)] {
                    bump(format!("texgen kind={} src={}", texgen.kind, texgen.source));
                    if let Some(slot) = texgen.matrix.checked_sub(30).map(|value| value / 3)
                        && texgen.matrix != 60
                        && let Some(Some(info)) = gx.tex_matrices.get(usize::from(slot))
                    {
                        bump(format!("texmtx mode={} maya={}", info.mode(), info.maya()));
                    }
                }
                for stage in &gx.stages[..usize::from(gx.stage_count)] {
                    if stage.color_op > 1 || stage.alpha_op > 1 {
                        bump(format!(
                            "compare op c={} a={}",
                            stage.color_op, stage.alpha_op
                        ));
                    }
                }
                for channel in &gx.channels[..usize::from(gx.channel_count) * 2] {
                    if channel.lighting {
                        bump(format!("lit mask={:#x}", channel.light_mask));
                    }
                }
                bump(format!(
                    "blend={} {}->{}",
                    gx.blend.mode, gx.blend.source, gx.blend.destination
                ));
                if !gx.alpha_test.always_passes() {
                    bump("alpha test".to_owned());
                }
                if gx.uses_indirect {
                    bump("indirect".to_owned());
                }
            }
        }
    }
    println!("models parsed: {models}");
    for (feature, count) in &features {
        println!("{count:7}  {feature}");
    }
    for failure in &failures {
        println!("FAILED {failure}");
    }
    if !failures.is_empty() {
        std::process::exit(1);
    }
}

fn objects(paths: &[String]) {
    use mkdd_track_studio::{catalog, formats::bol::BolDocument};
    let base = |path: &str| {
        let name = path.rsplit('/').next().unwrap_or(path).to_ascii_lowercase();
        name.rsplit_once('.')
            .map_or(name.clone(), |(stem, _)| stem.to_owned())
    };
    let mut library = std::collections::HashSet::new();
    let mut archives = Vec::new();
    for path in paths {
        let archive = RarcArchive::new(std::fs::read(path).expect("read")).expect("parse");
        for file in archive.files().iter().filter(|file| is_model(&file.path)) {
            if path.contains("MRAM") || path.contains("ARAM") {
                library.insert(base(&file.path));
            }
        }
        archives.push((path.clone(), archive));
    }
    let mut missing: BTreeMap<u16, (usize, Vec<String>)> = BTreeMap::new();
    let mut resolved = 0usize;
    for (path, archive) in &archives {
        let local: std::collections::HashSet<String> = archive
            .files()
            .iter()
            .filter(|file| is_model(&file.path))
            .map(|file| base(&file.path))
            .collect();
        let Some(bol) = archive
            .files()
            .iter()
            .find(|file| file.path.ends_with(".bol"))
        else {
            continue;
        };
        let Ok(course) = BolDocument::parse(archive.file_data(bol.index).unwrap()) else {
            continue;
        };
        for object in &course.objects {
            let names = catalog::model_names(object.id);
            if names
                .iter()
                .any(|name| local.contains(*name) || library.contains(*name))
            {
                resolved += 1;
            } else {
                let entry = missing.entry(object.id).or_default();
                entry.0 += 1;
                let course_name = path.rsplit('/').next().unwrap_or(path).to_owned();
                if !entry.1.contains(&course_name) {
                    entry.1.push(course_name);
                }
            }
        }
    }
    println!("resolved object placements: {resolved}");
    for (id, (count, courses)) in missing {
        let name = catalog::object_info(id).map_or("?", |info| info.class_name);
        println!(
            "missing 0x{id:04x} {name:28} x{count:<4} {}",
            courses.join(" ")
        );
    }
}

/// Prints GX state for every mesh of one model inside an archive.
pub fn dump_model(archive_path: &str, model_name: &str) {
    let archive = RarcArchive::new(std::fs::read(archive_path).expect("read")).expect("parse");
    let file = archive
        .files()
        .iter()
        .find(|file| file.path.ends_with(model_name))
        .expect("model not found");
    let model = BmdModel::parse(archive.file_data(file.index).unwrap()).expect("bmd");
    for (index, texture) in model.textures.iter().enumerate() {
        println!(
            "tex {index}: {}x{} {} wrap {}/{}",
            texture.width, texture.height, texture.format_name, texture.wrap_s, texture.wrap_t
        );
        if texture.width as usize * texture.height as usize <= 256 {
            let pixels: Vec<String> = texture
                .rgba
                .chunks(4)
                .step_by(8)
                .map(|p| format!("{:02x}{:02x}{:02x}{:02x}", p[0], p[1], p[2], p[3]))
                .collect();
            println!("   pixels {}", pixels.join(" "));
        }
    }
    for (index, mesh) in model.meshes.iter().enumerate() {
        let gx = &mesh.material.gx;
        let mut lo = [f32::MAX; 3];
        let mut hi = [f32::MIN; 3];
        for vertex in &mesh.vertices {
            for axis in 0..3 {
                lo[axis] = lo[axis].min(vertex.position[axis]);
                hi[axis] = hi[axis].max(vertex.position[axis]);
            }
        }
        let max_index = mesh.indices.iter().copied().max().unwrap_or(0);
        println!(
            "   indices {} max {} (verts {})",
            mesh.indices.len(),
            max_index,
            mesh.vertices.len()
        );
        println!(
            "mesh {index}: verts {} mode {} cull {:?} blend {:?} z {:?} alpha {:?} chans {} {:?}",
            mesh.vertices.len(),
            gx.mode,
            mesh.material.cull_mode,
            gx.blend,
            gx.z_mode,
            gx.alpha_test,
            gx.channel_count,
            &gx.channels[..2]
        );
        for vertex in mesh.vertices.iter().take(3) {
            println!(
                "   vertex {:?} uv0 {:?}",
                vertex.position, vertex.tex_coords[0]
            );
        }
        println!(
            "   bounds {lo:?}..{hi:?} textures {:?} matcolor {:?}",
            gx.textures, gx.material_colors[0]
        );
        for texgen in &gx.tex_gens[..usize::from(gx.tex_gen_count)] {
            let matrix = texgen
                .matrix
                .checked_sub(30)
                .map(|v| v / 3)
                .and_then(|slot| gx.tex_matrices.get(usize::from(slot)).copied().flatten());
            println!(
                "   texgen {texgen:?} mtx {:?}",
                matrix.map(|m| (m.mode(), m.scale, m.translation, m.rotation))
            );
        }
        for stage in 0..usize::from(gx.stage_count) {
            println!(
                "   stage {stage}: order {:?} {:?} kc {:#x} ka {:#x} swap {:?}",
                gx.orders[stage],
                gx.stages[stage],
                gx.konst_color_select[stage],
                gx.konst_alpha_select[stage],
                gx.swap_modes[stage]
            );
        }
        println!("   regs {:?} konst {:?}", gx.registers, gx.konst_colors);
    }
}
