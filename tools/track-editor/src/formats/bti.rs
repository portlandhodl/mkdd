use crate::error::{FormatError, Result};

const HEADER_SIZE: usize = 0x20;
const MAX_RGBA_BYTES: usize = 256 * 1024 * 1024;
const MAX_PALETTE_ENTRIES: usize = 0x4000;

type Rgba = [u8; 4];

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct BtiImage {
    pub width: u16,
    pub height: u16,
    pub format: u8,
    pub format_name: String,
    pub rgba: Vec<u8>,
    pub transparency: u8,
    pub wrap_s: u8,
    pub wrap_t: u8,
    pub palette_format: u8,
    pub lut_format: u8,
    pub palette_entry_count: u16,
    pub palette_offset: u32,
    pub mipmap: bool,
    pub edge_lod: bool,
    pub bias_clamp: bool,
    pub max_anisotropy: u8,
    pub min_filter: u8,
    pub mag_filter: u8,
    pub min_lod: i8,
    pub max_lod: i8,
    pub image_count: u8,
    pub unknown_19: u8,
    pub lod_bias: i16,
    pub image_data_offset: u32,
}

#[derive(Clone, Copy, PartialEq, Eq)]
enum FormatKind {
    Direct,
    Indexed,
    Cmpr,
}

#[derive(Clone, Copy)]
struct FormatInfo {
    name: &'static str,
    kind: FormatKind,
    block_width: usize,
    block_height: usize,
    block_size: usize,
}

struct Palette<'a> {
    data: &'a [u8],
    format: u8,
    entries: usize,
}

impl BtiImage {
    pub fn decode(data: &[u8]) -> Result<Self> {
        Self::decode_at(data, 0)
    }

    /// Decodes the base mip level of a ResTIMG header at `base`. Header offsets
    /// are relative to `base`, as they are for textures embedded in TEX1.
    pub fn decode_at(data: &[u8], base: usize) -> Result<Self> {
        let header = checked_slice(data, base, HEADER_SIZE, "ResTIMG header")?;
        let format = header[0x00];
        let info = format_info(format).ok_or_else(|| {
            FormatError::at(base, format!("unsupported BTI format 0x{format:02x}"))
        })?;
        let width = be16(header, 0x02);
        let height = be16(header, 0x04);
        if width == 0 || height == 0 {
            return Err(FormatError::at(base + 2, "BTI dimensions must be non-zero"));
        }

        let width_usize = usize::from(width);
        let height_usize = usize::from(height);
        let pixel_count = width_usize
            .checked_mul(height_usize)
            .ok_or_else(|| FormatError::at(base + 2, "BTI dimensions overflow"))?;
        let rgba_len = pixel_count
            .checked_mul(4)
            .ok_or_else(|| FormatError::at(base + 2, "RGBA size overflow"))?;
        if rgba_len > MAX_RGBA_BYTES {
            return Err(FormatError::at(
                base + 2,
                format!(
                    "decoded RGBA image is too large ({rgba_len} bytes; limit is {MAX_RGBA_BYTES})"
                ),
            ));
        }

        let tiles_x = div_round_up(width_usize, info.block_width);
        let tiles_y = div_round_up(height_usize, info.block_height);
        let image_len = tiles_x
            .checked_mul(tiles_y)
            .and_then(|count| count.checked_mul(info.block_size))
            .ok_or_else(|| FormatError::at(base + 0x1c, "BTI image size overflow"))?;
        let stored_image_offset = be32(header, 0x1c);
        let image_offset = if stored_image_offset == 0 {
            HEADER_SIZE as u32
        } else {
            stored_image_offset
        };
        let image_start = relative_offset(base, image_offset, "image data")?;
        let image = checked_slice(data, image_start, image_len, "BTI image data")?;

        let palette_entry_count = be16(header, 0x0a);
        let palette_offset = be32(header, 0x0c);
        let lut_format = header[0x09];
        let palette = if info.kind == FormatKind::Indexed {
            let entries = usize::from(palette_entry_count);
            if entries == 0 {
                return Err(FormatError::at(
                    base + 0x0a,
                    "indexed BTI image has no palette entries",
                ));
            }
            if entries > MAX_PALETTE_ENTRIES {
                return Err(FormatError::at(
                    base + 0x0a,
                    format!("BTI palette has too many entries ({entries})"),
                ));
            }
            if lut_format > 2 {
                return Err(FormatError::at(
                    base + 0x09,
                    format!("unsupported BTI palette format {lut_format}"),
                ));
            }
            let palette_len = entries
                .checked_mul(2)
                .ok_or_else(|| FormatError::at(base + 0x0a, "BTI palette size overflow"))?;
            let palette_start = relative_offset(base, palette_offset, "palette data")?;
            Some(Palette {
                data: checked_slice(data, palette_start, palette_len, "BTI palette")?,
                format: lut_format,
                entries,
            })
        } else {
            None
        };

        let mut rgba = Vec::new();
        rgba.try_reserve_exact(rgba_len).map_err(|_| {
            FormatError::at(
                base + 2,
                format!("unable to allocate {rgba_len} bytes for decoded BTI image"),
            )
        })?;
        rgba.resize(rgba_len, 0);

        for tile_y in 0..tiles_y {
            for tile_x in 0..tiles_x {
                let block_index = tile_y * tiles_x + tile_x;
                let block_start = block_index * info.block_size;
                let block = &image[block_start..block_start + info.block_size];
                for in_y in 0..info.block_height {
                    let y = tile_y * info.block_height + in_y;
                    if y >= height_usize {
                        continue;
                    }
                    for in_x in 0..info.block_width {
                        let x = tile_x * info.block_width + in_x;
                        if x >= width_usize {
                            continue;
                        }
                        let texel = in_y * info.block_width + in_x;
                        let color = match info.kind {
                            FormatKind::Direct => decode_direct(format, block, texel)?,
                            FormatKind::Indexed => {
                                let index = decode_index(format, block, texel)?;
                                let palette = palette.as_ref().ok_or_else(|| {
                                    FormatError::at(base, "missing palette for indexed BTI image")
                                })?;
                                palette.color(index).ok_or_else(|| {
                                    FormatError::at(
                                        base,
                                        format!(
                                            "palette index {index} at ({x}, {y}) exceeds {} entries",
                                            palette.entries
                                        ),
                                    )
                                })?
                            }
                            FormatKind::Cmpr => decode_cmpr(block, in_x, in_y)?,
                        };
                        let output = (y * width_usize + x) * 4;
                        rgba[output..output + 4].copy_from_slice(&color);
                    }
                }
            }
        }

        Ok(Self {
            width,
            height,
            format,
            format_name: info.name.to_owned(),
            rgba,
            transparency: header[0x01],
            wrap_s: header[0x06],
            wrap_t: header[0x07],
            palette_format: header[0x08],
            lut_format,
            palette_entry_count,
            palette_offset,
            mipmap: header[0x10] != 0,
            edge_lod: header[0x11] != 0,
            bias_clamp: header[0x12] != 0,
            max_anisotropy: header[0x13],
            min_filter: header[0x14],
            mag_filter: header[0x15],
            min_lod: header[0x16] as i8,
            max_lod: header[0x17] as i8,
            image_count: header[0x18],
            unknown_19: header[0x19],
            lod_bias: be16(header, 0x1a) as i16,
            image_data_offset: stored_image_offset,
        })
    }
}

impl Palette<'_> {
    fn color(&self, index: u16) -> Option<Rgba> {
        let index = usize::from(index);
        if index >= self.entries {
            return None;
        }
        let value = be16(self.data, index * 2);
        Some(match self.format {
            0 => {
                let alpha = (value >> 8) as u8;
                let intensity = value as u8;
                [intensity, intensity, intensity, alpha]
            }
            1 => rgb565(value),
            2 => rgb5a3(value),
            _ => return None,
        })
    }
}

fn format_info(format: u8) -> Option<FormatInfo> {
    let info = match format {
        0x0 => FormatInfo {
            name: "I4",
            kind: FormatKind::Direct,
            block_width: 8,
            block_height: 8,
            block_size: 32,
        },
        0x1 => FormatInfo {
            name: "I8",
            kind: FormatKind::Direct,
            block_width: 8,
            block_height: 4,
            block_size: 32,
        },
        0x2 => FormatInfo {
            name: "IA4",
            kind: FormatKind::Direct,
            block_width: 8,
            block_height: 4,
            block_size: 32,
        },
        0x3 => FormatInfo {
            name: "IA8",
            kind: FormatKind::Direct,
            block_width: 4,
            block_height: 4,
            block_size: 32,
        },
        0x4 => FormatInfo {
            name: "RGB565",
            kind: FormatKind::Direct,
            block_width: 4,
            block_height: 4,
            block_size: 32,
        },
        0x5 => FormatInfo {
            name: "RGB5A3",
            kind: FormatKind::Direct,
            block_width: 4,
            block_height: 4,
            block_size: 32,
        },
        0x6 => FormatInfo {
            name: "RGBA8",
            kind: FormatKind::Direct,
            block_width: 4,
            block_height: 4,
            block_size: 64,
        },
        0x8 => FormatInfo {
            name: "C4",
            kind: FormatKind::Indexed,
            block_width: 8,
            block_height: 8,
            block_size: 32,
        },
        0x9 => FormatInfo {
            name: "C8",
            kind: FormatKind::Indexed,
            block_width: 8,
            block_height: 4,
            block_size: 32,
        },
        0xa => FormatInfo {
            name: "C14X2",
            kind: FormatKind::Indexed,
            block_width: 4,
            block_height: 4,
            block_size: 32,
        },
        0xe => FormatInfo {
            name: "CMPR",
            kind: FormatKind::Cmpr,
            block_width: 8,
            block_height: 8,
            block_size: 32,
        },
        _ => return None,
    };
    Some(info)
}

fn decode_direct(format: u8, block: &[u8], texel: usize) -> Result<Rgba> {
    let color = match format {
        0x0 => {
            let byte = block[texel / 2];
            let nibble = if texel.is_multiple_of(2) {
                byte >> 4
            } else {
                byte & 0x0f
            };
            let intensity = expand4(nibble);
            // GX replicates intensity into alpha for I4/I8 (TEXA == I).
            [intensity; 4]
        }
        0x1 => {
            let intensity = block[texel];
            [intensity; 4]
        }
        0x2 => {
            let value = block[texel];
            let alpha = expand4(value >> 4);
            let intensity = expand4(value & 0x0f);
            [intensity, intensity, intensity, alpha]
        }
        0x3 => {
            let alpha = block[texel * 2];
            let intensity = block[texel * 2 + 1];
            [intensity, intensity, intensity, alpha]
        }
        0x4 => rgb565(be16(block, texel * 2)),
        0x5 => rgb5a3(be16(block, texel * 2)),
        0x6 => {
            let ar = texel * 2;
            let gb = 32 + texel * 2;
            [block[ar + 1], block[gb], block[gb + 1], block[ar]]
        }
        _ => {
            return Err(FormatError::new(format!(
                "format 0x{format:02x} is not a direct BTI format"
            )));
        }
    };
    Ok(color)
}

fn decode_index(format: u8, block: &[u8], texel: usize) -> Result<u16> {
    match format {
        0x8 => {
            let byte = block[texel / 2];
            Ok(u16::from(if texel.is_multiple_of(2) {
                byte >> 4
            } else {
                byte & 0x0f
            }))
        }
        0x9 => Ok(u16::from(block[texel])),
        0xa => Ok(be16(block, texel * 2) & 0x3fff),
        _ => Err(FormatError::new(format!(
            "format 0x{format:02x} is not an indexed BTI format"
        ))),
    }
}

fn decode_cmpr(block: &[u8], x: usize, y: usize) -> Result<Rgba> {
    if x >= 8 || y >= 8 {
        return Err(FormatError::new("CMPR texel is outside its 8x8 block"));
    }
    let subblock = (y / 4) * 2 + x / 4;
    let offset = subblock * 8;
    let data = &block[offset..offset + 8];
    let color0 = be16(data, 0);
    let color1 = be16(data, 2);
    let first = rgb565(color0);
    let second = rgb565(color1);
    let colors = if color0 > color1 {
        [
            first,
            second,
            interpolate(first, second, 2, 1, 3),
            interpolate(first, second, 1, 2, 3),
        ]
    } else {
        [
            first,
            second,
            interpolate(first, second, 1, 1, 2),
            [0, 0, 0, 0],
        ]
    };
    let local_x = x % 4;
    let local_y = y % 4;
    let index = (data[4 + local_y] >> (6 - local_x * 2)) & 3;
    Ok(colors[usize::from(index)])
}

fn interpolate(
    first: Rgba,
    second: Rgba,
    first_weight: u16,
    second_weight: u16,
    divisor: u16,
) -> Rgba {
    let mut result = [0, 0, 0, 255];
    for (channel, output) in result.iter_mut().take(3).enumerate() {
        *output = ((u16::from(first[channel]) * first_weight
            + u16::from(second[channel]) * second_weight)
            / divisor) as u8;
    }
    result
}

fn rgb565(value: u16) -> Rgba {
    [
        expand5(((value >> 11) & 0x1f) as u8),
        expand6(((value >> 5) & 0x3f) as u8),
        expand5((value & 0x1f) as u8),
        255,
    ]
}

fn rgb5a3(value: u16) -> Rgba {
    if value & 0x8000 != 0 {
        [
            expand5(((value >> 10) & 0x1f) as u8),
            expand5(((value >> 5) & 0x1f) as u8),
            expand5((value & 0x1f) as u8),
            255,
        ]
    } else {
        [
            expand4(((value >> 8) & 0x0f) as u8),
            expand4(((value >> 4) & 0x0f) as u8),
            expand4((value & 0x0f) as u8),
            expand3(((value >> 12) & 0x07) as u8),
        ]
    }
}

fn expand3(value: u8) -> u8 {
    (value << 5) | (value << 2) | (value >> 1)
}

fn expand4(value: u8) -> u8 {
    value * 17
}

fn expand5(value: u8) -> u8 {
    (value << 3) | (value >> 2)
}

fn expand6(value: u8) -> u8 {
    (value << 2) | (value >> 4)
}

fn div_round_up(value: usize, divisor: usize) -> usize {
    value / divisor + usize::from(!value.is_multiple_of(divisor))
}

fn be16(data: &[u8], offset: usize) -> u16 {
    u16::from_be_bytes([data[offset], data[offset + 1]])
}

fn be32(data: &[u8], offset: usize) -> u32 {
    u32::from_be_bytes([
        data[offset],
        data[offset + 1],
        data[offset + 2],
        data[offset + 3],
    ])
}

fn relative_offset(base: usize, relative: u32, name: &str) -> Result<usize> {
    base.checked_add(relative as usize)
        .ok_or_else(|| FormatError::at(base, format!("{name} offset overflow")))
}

fn checked_slice<'a>(data: &'a [u8], offset: usize, length: usize, name: &str) -> Result<&'a [u8]> {
    let end = offset
        .checked_add(length)
        .ok_or_else(|| FormatError::at(offset, format!("{name} range overflow")))?;
    data.get(offset..end).ok_or_else(|| {
        FormatError::at(
            offset,
            format!(
                "{name} needs {length} bytes, only {} remain",
                data.len().saturating_sub(offset)
            ),
        )
    })
}

#[cfg(test)]
mod tests {
    use super::*;

    fn header(format: u8, width: u16, height: u16, image_offset: u32) -> Vec<u8> {
        let mut data = vec![0; HEADER_SIZE];
        data[0] = format;
        data[2..4].copy_from_slice(&width.to_be_bytes());
        data[4..6].copy_from_slice(&height.to_be_bytes());
        data[0x18] = 1;
        data[0x1c..0x20].copy_from_slice(&image_offset.to_be_bytes());
        data
    }

    fn direct(format: u8, width: u16, height: u16, image: &[u8]) -> BtiImage {
        let mut data = header(format, width, height, HEADER_SIZE as u32);
        data.extend_from_slice(image);
        BtiImage::decode(&data).unwrap()
    }

    fn indexed(
        format: u8,
        lut_format: u8,
        palette: &[u8],
        width: u16,
        height: u16,
        image: &[u8],
    ) -> BtiImage {
        let image_offset = HEADER_SIZE + palette.len();
        let mut data = header(format, width, height, image_offset as u32);
        data[8] = 1;
        data[9] = lut_format;
        data[0x0a..0x0c].copy_from_slice(&((palette.len() / 2) as u16).to_be_bytes());
        data[0x0c..0x10].copy_from_slice(&(HEADER_SIZE as u32).to_be_bytes());
        data.extend_from_slice(palette);
        data.extend_from_slice(image);
        BtiImage::decode(&data).unwrap()
    }

    #[test]
    fn decodes_intensity_and_alpha_formats() {
        let mut body = [0; 32];
        body[0] = 0x1f;
        let image = direct(0, 2, 1, &body);
        assert_eq!(&image.rgba[..8], &[17, 17, 17, 17, 255, 255, 255, 255]);
        assert_eq!(image.format_name, "I4");

        body = [0; 32];
        body[0] = 0x7f;
        let image = direct(1, 1, 1, &body);
        assert_eq!(image.rgba, [127, 127, 127, 127]);

        body = [0; 32];
        body[0] = 0x2d;
        let image = direct(2, 1, 1, &body);
        assert_eq!(image.rgba, [221, 221, 221, 34]);

        body = [0; 32];
        body[0..2].copy_from_slice(&[0x40, 0x80]);
        let image = direct(3, 1, 1, &body);
        assert_eq!(image.rgba, [128, 128, 128, 64]);
    }

    #[test]
    fn decodes_big_endian_color_formats() {
        let mut body = [0; 32];
        body[0..2].copy_from_slice(&0xf81f_u16.to_be_bytes());
        let image = direct(4, 1, 1, &body);
        assert_eq!(image.rgba, [255, 0, 255, 255]);

        body = [0; 32];
        body[0..2].copy_from_slice(&0xfc00_u16.to_be_bytes());
        body[2..4].copy_from_slice(&0x2f80_u16.to_be_bytes());
        let image = direct(5, 2, 1, &body);
        assert_eq!(&image.rgba[..8], &[255, 0, 0, 255, 255, 136, 0, 73]);
    }

    #[test]
    fn decodes_rgba8_ar_then_gb_subblocks() {
        let mut body = [0; 64];
        body[0..2].copy_from_slice(&[0x12, 0x34]);
        body[32..34].copy_from_slice(&[0x56, 0x78]);
        body[2..4].copy_from_slice(&[0x9a, 0xbc]);
        body[34..36].copy_from_slice(&[0xde, 0xf0]);
        let image = direct(6, 2, 1, &body);
        assert_eq!(
            &image.rgba[..8],
            &[0x34, 0x56, 0x78, 0x12, 0xbc, 0xde, 0xf0, 0x9a]
        );
    }

    #[test]
    fn decodes_all_palette_formats_and_index_widths() {
        let mut body = [0; 32];
        body[0] = 0x01;
        let image = indexed(8, 1, &[0xf8, 0x00, 0x07, 0xe0], 2, 1, &body);
        assert_eq!(&image.rgba[..8], &[255, 0, 0, 255, 0, 255, 0, 255]);

        body = [0; 32];
        body[0..2].copy_from_slice(&[0, 1]);
        let image = indexed(9, 0, &[0x10, 0x20, 0xf0, 0xe0], 2, 1, &body);
        assert_eq!(&image.rgba[..8], &[32, 32, 32, 16, 224, 224, 224, 240]);

        body = [0; 32];
        body[0..4].copy_from_slice(&[0xc0, 0x00, 0xc0, 0x01]);
        let image = indexed(10, 2, &[0x80, 0x00, 0x2f, 0x80], 2, 1, &body);
        assert_eq!(&image.rgba[..8], &[0, 0, 0, 255, 255, 136, 0, 73]);
    }

    #[test]
    fn decodes_cmpr_four_color_interpolation() {
        let mut body = [0; 32];
        body[0..2].copy_from_slice(&0xf800_u16.to_be_bytes());
        body[2..4].copy_from_slice(&0x07e0_u16.to_be_bytes());
        body[4] = 0x1b;
        let image = direct(14, 4, 1, &body);
        assert_eq!(
            &image.rgba[..16],
            &[
                255, 0, 0, 255, 0, 255, 0, 255, 170, 85, 0, 255, 85, 170, 0, 255,
            ]
        );
    }

    #[test]
    fn decodes_cmpr_three_color_interpolation_and_transparency() {
        let mut body = [0; 32];
        body[0..2].copy_from_slice(&0x0000_u16.to_be_bytes());
        body[2..4].copy_from_slice(&0xffff_u16.to_be_bytes());
        body[4] = 0x1b;
        let image = direct(14, 4, 1, &body);
        assert_eq!(
            &image.rgba[..16],
            &[
                0, 0, 0, 255, 255, 255, 255, 255, 127, 127, 127, 255, 0, 0, 0, 0,
            ]
        );
    }

    #[test]
    fn walks_tiled_image_and_cmpr_subblocks() {
        let mut body = [0; 64];
        body[32] = 0xfe;
        let image = direct(1, 9, 1, &body);
        assert_eq!(&image.rgba[8 * 4..9 * 4], &[254, 254, 254, 254]);

        let mut body = [0; 32];
        body[2..4].copy_from_slice(&0xffff_u16.to_be_bytes());
        body[8..10].copy_from_slice(&0xf800_u16.to_be_bytes());
        body[10..12].copy_from_slice(&0x07e0_u16.to_be_bytes());
        body[16..18].copy_from_slice(&0x07e0_u16.to_be_bytes());
        body[18..20].copy_from_slice(&0x0000_u16.to_be_bytes());
        body[24..26].copy_from_slice(&0x001f_u16.to_be_bytes());
        body[26..28].copy_from_slice(&0x0000_u16.to_be_bytes());
        let image = direct(14, 8, 8, &body);
        assert_eq!(&image.rgba[0..4], &[0, 0, 0, 255]);
        assert_eq!(&image.rgba[4 * 4..5 * 4], &[255, 0, 0, 255]);
        assert_eq!(&image.rgba[4 * 8 * 4..4 * 8 * 4 + 4], &[0, 255, 0, 255]);
        assert_eq!(
            &image.rgba[(4 * 8 + 4) * 4..(4 * 8 + 5) * 4],
            &[0, 0, 255, 255]
        );
    }

    #[test]
    fn supports_embedded_headers_and_zero_image_offset() {
        let mut data = vec![0xaa; 7];
        let start = data.len();
        data.extend_from_slice(&header(1, 1, 1, 0));
        data.resize(start + HEADER_SIZE + 32, 0);
        data[start + HEADER_SIZE] = 0x5a;
        let image = BtiImage::decode_at(&data, start).unwrap();
        assert_eq!(image.rgba, [90, 90, 90, 90]);
        assert_eq!(image.image_data_offset, 0);
    }

    #[test]
    fn rejects_malformed_inputs_without_large_allocations() {
        assert!(BtiImage::decode(&[0; 31]).is_err());
        assert!(BtiImage::decode(&header(7, 1, 1, 0x20)).is_err());
        assert!(BtiImage::decode(&header(1, 0, 1, 0x20)).is_err());
        assert!(BtiImage::decode(&header(1, u16::MAX, u16::MAX, 0x20)).is_err());
        assert!(BtiImage::decode(&header(1, 8, 4, 0x20)).is_err());
        assert!(BtiImage::decode_at(&[0; 32], usize::MAX).is_err());

        let mut data = header(9, 1, 1, 0x22);
        data[8] = 1;
        data[9] = 1;
        data[0x0a..0x0c].copy_from_slice(&1_u16.to_be_bytes());
        data[0x0c..0x10].copy_from_slice(&0x20_u32.to_be_bytes());
        data.extend_from_slice(&[0xf8, 0x00]);
        let mut body = [0; 32];
        body[0] = 1;
        data.extend_from_slice(&body);
        assert!(BtiImage::decode(&data).is_err());
    }
}
