use std::{
    env,
    ffi::OsStr,
    fs,
    path::{Path, PathBuf},
    sync::OnceLock,
};

use mkdd_track_studio::formats::{
    bco::Bco,
    bmd::BmdModel,
    bol::{BolDocument, MapObject},
    rarc::RarcArchive,
};

const COURSE_DIR_ENV: &str = "MKDD_TEST_COURSE_DIR";
const MAX_DIRECTORY_DEPTH: usize = 12;
const MAX_DIRECTORY_COUNT: usize = 4_096;
const MAX_VISITED_ENTRIES: usize = 50_000;

#[derive(Debug)]
struct Corpus {
    bol: Vec<PathBuf>,
    bco: Vec<PathBuf>,
    bmd: Vec<PathBuf>,
    arc: Vec<PathBuf>,
}

impl Corpus {
    fn discover(root: &Path) -> Self {
        let root = fs::canonicalize(root).unwrap_or_else(|error| {
            panic!("cannot resolve {}: {error}", root.display());
        });
        assert!(
            root.is_dir(),
            "{COURSE_DIR_ENV} must name a directory, got {}",
            root.display()
        );

        let mut corpus = Self {
            bol: Vec::new(),
            bco: Vec::new(),
            bmd: Vec::new(),
            arc: Vec::new(),
        };
        let mut pending = vec![(root, 0_usize)];
        let mut directory_count = 0_usize;
        let mut visited_entries = 0_usize;

        while let Some((directory, depth)) = pending.pop() {
            directory_count += 1;
            assert!(
                directory_count <= MAX_DIRECTORY_COUNT,
                "course discovery exceeded {MAX_DIRECTORY_COUNT} directories; point \
                 {COURSE_DIR_ENV} at the extracted course directory instead of a larger tree"
            );

            let entries = fs::read_dir(&directory).unwrap_or_else(|error| {
                panic!("cannot read directory {}: {error}", directory.display());
            });
            for entry in entries {
                let entry = entry.unwrap_or_else(|error| {
                    panic!("cannot read an entry in {}: {error}", directory.display());
                });
                visited_entries += 1;
                assert!(
                    visited_entries <= MAX_VISITED_ENTRIES,
                    "course discovery exceeded {MAX_VISITED_ENTRIES} entries; point \
                     {COURSE_DIR_ENV} at the extracted course directory instead of a larger tree"
                );

                let file_type = entry.file_type().unwrap_or_else(|error| {
                    panic!("cannot inspect {}: {error}", entry.path().display());
                });
                if file_type.is_symlink() {
                    continue;
                }
                if file_type.is_dir() {
                    if should_skip_directory(&entry.file_name()) {
                        continue;
                    }
                    assert!(
                        depth < MAX_DIRECTORY_DEPTH,
                        "course discovery exceeded depth {MAX_DIRECTORY_DEPTH} below {}; point \
                         {COURSE_DIR_ENV} closer to the extracted course files",
                        directory.display()
                    );
                    pending.push((entry.path(), depth + 1));
                    continue;
                }
                if !file_type.is_file() {
                    continue;
                }

                let path = entry.path();
                if has_extension(&path, "bol") {
                    corpus.bol.push(path);
                } else if has_extension(&path, "bco") {
                    corpus.bco.push(path);
                } else if has_extension(&path, "bmd") {
                    corpus.bmd.push(path);
                } else if has_extension(&path, "arc") {
                    corpus.arc.push(path);
                }
            }
        }

        corpus.bol.sort();
        corpus.bco.sort();
        corpus.bmd.sort();
        corpus.arc.sort();
        assert!(
            !corpus.bol.is_empty()
                || !corpus.bco.is_empty()
                || !corpus.bmd.is_empty()
                || !corpus.arc.is_empty(),
            "found no .bol, .bco, .bmd, or .arc files beneath the configured course directory"
        );
        corpus
    }
}

fn has_extension(path: &Path, expected: &str) -> bool {
    path.extension()
        .and_then(OsStr::to_str)
        .is_some_and(|extension| extension.eq_ignore_ascii_case(expected))
}

fn should_skip_directory(name: &OsStr) -> bool {
    name.to_str().is_some_and(|name| {
        matches!(
            name,
            ".git" | ".cache" | "build" | "node_modules" | "target" | "__pycache__"
        )
    })
}

fn corpus() -> Option<&'static Corpus> {
    static CORPUS: OnceLock<Corpus> = OnceLock::new();

    let root = PathBuf::from(env::var_os(COURSE_DIR_ENV)?);
    Some(CORPUS.get_or_init(|| Corpus::discover(&root)))
}

fn read_file(path: &Path) -> Vec<u8> {
    fs::read(path).unwrap_or_else(|error| panic!("cannot read {}: {error}", path.display()))
}

fn assert_same_bytes(actual: &[u8], expected: &[u8], label: &str) {
    if actual == expected {
        return;
    }
    let common_length = actual.len().min(expected.len());
    let offset = (0..common_length)
        .find(|&offset| actual[offset] != expected[offset])
        .unwrap_or(common_length);
    let actual_byte = actual.get(offset);
    let expected_byte = expected.get(offset);
    panic!(
        "{label} differs at offset 0x{offset:x}: expected {expected_byte:?}, got \
         {actual_byte:?} (expected {} bytes, got {} bytes)",
        expected.len(),
        actual.len()
    );
}

#[test]
fn real_bol_noop_exports_are_byte_identical() {
    let Some(corpus) = corpus() else {
        eprintln!("skipping real BOL tests; set {COURSE_DIR_ENV} to an extracted course directory");
        return;
    };
    if corpus.bol.is_empty() {
        eprintln!("no .bol files found beneath the configured course directory");
        return;
    }

    for path in &corpus.bol {
        let original = read_file(path);
        let document = BolDocument::parse(&original)
            .unwrap_or_else(|error| panic!("cannot parse {}: {error}", path.display()));
        let exported = document
            .export()
            .unwrap_or_else(|error| panic!("cannot export {}: {error}", path.display()));
        assert_same_bytes(
            &exported,
            &original,
            &format!("no-op BOL export for {}", path.display()),
        );
    }
}

#[test]
fn real_bol_nonstructural_edits_roundtrip() {
    let Some(corpus) = corpus() else {
        eprintln!(
            "skipping real BOL edit tests; set {COURSE_DIR_ENV} to an extracted course directory"
        );
        return;
    };
    if corpus.bol.is_empty() {
        eprintln!("no .bol files found beneath the configured course directory");
        return;
    }

    for path in &corpus.bol {
        let original = read_file(path);
        BolDocument::parse(&original)
            .unwrap_or_else(|error| panic!("cannot parse {}: {error}", path.display()));

        // BOL 0015 stores the lap count at 0x18. Change only that in-memory byte;
        // this neither changes any table extents nor writes to the source file.
        let mut edited = original.clone();
        assert!(
            edited.starts_with(b"0015") && edited.len() > 0x18,
            "{} is not a complete BOL 0015 document",
            path.display()
        );
        edited[0x18] = if edited[0x18] == 3 { 4 } else { 3 };
        let document = BolDocument::parse(&edited)
            .unwrap_or_else(|error| panic!("cannot parse edited BOL {}: {error}", path.display()));
        let exported = document
            .export()
            .unwrap_or_else(|error| panic!("cannot export edited BOL {}: {error}", path.display()));
        assert_same_bytes(
            &exported,
            &edited,
            &format!("edited BOL export for {}", path.display()),
        );

        let reparsed = BolDocument::parse(&exported).unwrap_or_else(|error| {
            panic!("cannot reparse edited BOL {}: {error}", path.display())
        });
        let reexported = reparsed.export().unwrap_or_else(|error| {
            panic!("cannot re-export edited BOL {}: {error}", path.display())
        });
        assert_same_bytes(
            &reexported,
            &exported,
            &format!("second edited BOL export for {}", path.display()),
        );
    }
}

#[test]
fn real_bol_structural_add_delete_preserves_other_sections() {
    let Some(corpus) = corpus() else {
        return;
    };
    for path in &corpus.bol {
        let original = BolDocument::parse(&read_file(path))
            .unwrap_or_else(|error| panic!("cannot parse {}: {error}", path.display()));
        let mut edited = original.clone();
        let added = edited
            .add_object(MapObject {
                id: 1,
                ..MapObject::default()
            })
            .unwrap_or_else(|error| panic!("cannot add an object to {}: {error}", path.display()));
        let with_object = BolDocument::parse(&edited.export().unwrap())
            .unwrap_or_else(|error| panic!("cannot reparse modified {}: {error}", path.display()));
        assert_eq!(with_object.objects.len(), original.objects.len() + 1);
        assert_eq!(
            &with_object.objects[..original.objects.len()],
            original.objects
        );
        assert_eq!(with_object.centerline, original.centerline);
        assert_eq!(with_object.checkpoint_groups, original.checkpoint_groups);
        assert_eq!(with_object.paths, original.paths);
        assert_eq!(with_object.route_points, original.route_points);

        edited.delete_object(added).unwrap();
        let restored = BolDocument::parse(&edited.export().unwrap()).unwrap();
        assert_eq!(restored.header, original.header);
        assert_eq!(restored.counts, original.counts);
        assert_eq!(restored.centerline, original.centerline);
        assert_eq!(restored.checkpoint_groups, original.checkpoint_groups);
        assert_eq!(restored.paths, original.paths);
        assert_eq!(restored.route_points, original.route_points);
        assert_eq!(restored.objects, original.objects);
        assert_eq!(restored.starts, original.starts);
        assert_eq!(restored.areas, original.areas);
        assert_eq!(restored.cameras, original.cameras);
        assert_eq!(restored.respawns, original.respawns);
        assert_eq!(restored.lights, original.lights);
        assert_eq!(restored.mg, original.mg);
    }
}

#[test]
fn real_bco_indices_are_valid() {
    let Some(corpus) = corpus() else {
        eprintln!("skipping real BCO tests; set {COURSE_DIR_ENV} to an extracted course directory");
        return;
    };
    if corpus.bco.is_empty() {
        eprintln!("no .bco files found beneath the configured course directory");
        return;
    }

    for path in &corpus.bco {
        let bytes = read_file(path);
        let bco = Bco::parse(&bytes)
            .unwrap_or_else(|error| panic!("cannot parse {}: {error}", path.display()));

        assert_eq!(
            bco.grid.cells.len(),
            usize::from(bco.grid.x_count) * usize::from(bco.grid.z_count),
            "grid dimensions do not match its cells in {}",
            path.display()
        );
        for (triangle_index, triangle) in bco.triangles.iter().enumerate() {
            for &vertex_index in &triangle.indices {
                assert!(
                    (vertex_index as usize) < bco.vertices.len(),
                    "triangle {triangle_index} references vertex {vertex_index}, but {} has \
                     {} vertices",
                    path.display(),
                    bco.vertices.len()
                );
            }
            for &neighbor in &triangle.neighbors {
                assert!(
                    neighbor == u16::MAX || usize::from(neighbor) < bco.triangles.len(),
                    "triangle {triangle_index} references invalid neighbor {neighbor} in {}",
                    path.display()
                );
            }
        }
        for &triangle_index in &bco.grid_indices {
            assert!(
                usize::from(triangle_index) < bco.triangles.len(),
                "grid index {triangle_index} is outside {} triangles in {}",
                bco.triangles.len(),
                path.display()
            );
        }
        for (cell_index, cell) in bco.grid.cells.iter().enumerate() {
            let indices = cell.indices(&bco.grid_indices).unwrap_or_else(|| {
                panic!(
                    "grid cell {cell_index} has an invalid index range in {}",
                    path.display()
                )
            });
            for &triangle_index in indices {
                assert!(
                    usize::from(triangle_index) < bco.triangles.len(),
                    "grid cell {cell_index} references triangle {triangle_index}, but {} has \
                     {} triangles",
                    path.display(),
                    bco.triangles.len()
                );
            }
        }
    }
}

#[test]
fn real_course_bmd_geometry_decodes_completely() {
    let Some(corpus) = corpus() else {
        return;
    };
    let mut decoded = 0usize;
    for path in corpus.bmd.iter().filter(|path| {
        path.file_name()
            .and_then(OsStr::to_str)
            .is_some_and(|name| name.to_ascii_lowercase().ends_with("_course.bmd"))
    }) {
        let model = BmdModel::parse(&read_file(path))
            .unwrap_or_else(|error| panic!("cannot parse {}: {error}", path.display()));
        assert!(!model.meshes.is_empty(), "no meshes in {}", path.display());
        for mesh in &model.meshes {
            assert!(
                !mesh.vertices.is_empty(),
                "empty mesh in {}",
                path.display()
            );
            assert_eq!(mesh.indices.len() % 3, 0);
            assert!(
                mesh.indices
                    .iter()
                    .all(|index| (*index as usize) < mesh.vertices.len())
            );
            assert!(mesh.vertices.iter().all(|vertex| {
                vertex.position.iter().all(|value| value.is_finite())
                    && vertex.normal.iter().all(|value| value.is_finite())
                    && vertex.color.iter().all(|value| value.is_finite())
                    && vertex.tex_coords.iter().flatten().all(|value| value.is_finite())
            }));
        }
        for texture in &model.textures {
            assert_eq!(
                texture.rgba.len(),
                usize::from(texture.width) * usize::from(texture.height) * 4
            );
        }
        if path.file_name().and_then(OsStr::to_str) == Some("mario_course.bmd")
            && path
                .parent()
                .and_then(Path::file_name)
                .and_then(OsStr::to_str)
                == Some("Mario")
        {
            assert_eq!(model.meshes.len(), 22);
            assert_eq!(
                model
                    .meshes
                    .iter()
                    .map(|mesh| mesh.vertices.len())
                    .sum::<usize>(),
                20_641
            );
            assert_eq!(
                model
                    .meshes
                    .iter()
                    .map(|mesh| mesh.indices.len() / 3)
                    .sum::<usize>(),
                13_021
            );
        }
        decoded += 1;
    }
    assert!(
        decoded > 0,
        "no *_course.bmd models found in configured corpus"
    );
}

#[test]
fn real_rarc_file_ranges_and_decoding_are_valid() {
    let Some(corpus) = corpus() else {
        eprintln!(
            "skipping real RARC tests; set {COURSE_DIR_ENV} to an extracted course directory"
        );
        return;
    };
    if corpus.arc.is_empty() {
        eprintln!("no .arc files found beneath the configured course directory");
        return;
    }

    for path in &corpus.arc {
        let bytes = read_file(path);
        let archive = RarcArchive::parse(&bytes)
            .unwrap_or_else(|error| panic!("cannot parse {}: {error}", path.display()));
        assert_same_bytes(
            &archive.export().unwrap(),
            &bytes,
            &format!("no-op RARC export for {}", path.display()),
        );

        for (position, file) in archive.files().iter().enumerate() {
            assert_eq!(
                file.index,
                position,
                "file indices are not contiguous in {}",
                path.display()
            );
            assert!(
                !file.name.is_empty() && !file.path.is_empty(),
                "file {position} has an empty name or path in {}",
                path.display()
            );
            let decoded = archive.file_data(file.index).unwrap_or_else(|error| {
                panic!(
                    "cannot read file {} from {}: {error}",
                    file.path,
                    path.display()
                )
            });
            if !file.compressed {
                assert_eq!(
                    decoded.len(),
                    file.stored_size as usize,
                    "uncompressed file {} has the wrong size in {}",
                    file.path,
                    path.display()
                );
            }
        }
        assert!(
            archive.file_data(archive.files().len()).is_err(),
            "an out-of-range file index unexpectedly succeeded for {}",
            path.display()
        );
    }
}

#[test]
fn real_rarc_data_replacements_repack_without_losing_files() {
    let Some(corpus) = corpus() else {
        return;
    };
    for path in &corpus.arc {
        let bytes = read_file(path);
        let mut archive = RarcArchive::parse(&bytes)
            .unwrap_or_else(|error| panic!("cannot parse {}: {error}", path.display()));
        if archive.files().is_empty() {
            continue;
        }
        let mut replacement = archive.file_data(0).unwrap().to_vec();
        replacement.extend_from_slice(b"MKDD editor test");
        archive.replace_file(0, &replacement).unwrap();
        let packed = archive
            .export()
            .unwrap_or_else(|error| panic!("cannot repack modified {}: {error}", path.display()));
        let reparsed = RarcArchive::parse(&packed)
            .unwrap_or_else(|error| panic!("cannot reparse modified {}: {error}", path.display()));
        assert_eq!(reparsed.files().len(), archive.files().len());
        for original in archive.files() {
            let exported = reparsed
                .files()
                .get(original.index)
                .unwrap_or_else(|| panic!("missing file after repacking {}", path.display()));
            assert_eq!(
                exported.id,
                original.id,
                "file ID changed in {}",
                path.display()
            );
            assert_eq!(exported.path, original.path);
            let expected = if original.index == 0 {
                replacement.as_slice()
            } else {
                archive.file_data(original.index).unwrap()
            };
            assert_eq!(reparsed.file_data(original.index).unwrap(), expected);
        }
    }
}
