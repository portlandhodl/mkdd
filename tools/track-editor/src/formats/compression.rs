use crate::error::{FormatError, Result};

/// Maximum size of a decompression result. Change this constant to adjust the limit.
pub const MAX_DECOMPRESSED_SIZE: usize = 512 * 1024 * 1024;

const HEADER_SIZE: usize = 16;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Compression {
    None,
    Yaz0,
    Yay0,
}

pub fn detect(data: &[u8]) -> Compression {
    if data.starts_with(b"Yaz0") {
        Compression::Yaz0
    } else if data.starts_with(b"Yay0") {
        Compression::Yay0
    } else {
        Compression::None
    }
}

/// Returns the size declared by a recognized compression header, or `None` for
/// uncompressed data. Declared sizes above [`MAX_DECOMPRESSED_SIZE`] are rejected.
pub fn decoded_size(data: &[u8]) -> Result<Option<usize>> {
    match detect(data) {
        Compression::None => Ok(None),
        Compression::Yaz0 | Compression::Yay0 => checked_decoded_size(data).map(Some),
    }
}

/// Decompresses Yaz0/SZS and Yay0/SZP streams. Uncompressed input is cloned.
pub fn decompress(data: &[u8]) -> Result<Vec<u8>> {
    match detect(data) {
        Compression::None => {
            let mut output = allocate_output(data.len())?;
            output.extend_from_slice(data);
            Ok(output)
        }
        Compression::Yaz0 => decompress_yaz0(data),
        Compression::Yay0 => decompress_yay0(data),
    }
}

/// Encodes a valid literal-only Yaz0 stream. This deliberately simple encoding
/// favors reliability over compression ratio and is suitable for game SZS files.
pub fn compress_yaz0(data: &[u8]) -> Result<Vec<u8>> {
    if data.len() > MAX_DECOMPRESSED_SIZE {
        return Err(FormatError::new(format!(
            "input size {} exceeds the {} byte limit",
            data.len(),
            MAX_DECOMPRESSED_SIZE
        )));
    }
    let decoded_size = u32::try_from(data.len())
        .map_err(|_| FormatError::new("input is too large for a Yaz0 header"))?;
    let group_count = data
        .len()
        .checked_add(7)
        .ok_or_else(|| FormatError::new("encoded size overflow"))?
        / 8;
    let encoded_size = HEADER_SIZE
        .checked_add(data.len())
        .and_then(|size| size.checked_add(group_count))
        .ok_or_else(|| FormatError::new("encoded size overflow"))?;

    let mut encoded = Vec::new();
    encoded
        .try_reserve_exact(encoded_size)
        .map_err(|_| FormatError::new("could not allocate Yaz0 output"))?;
    encoded.extend_from_slice(b"Yaz0");
    encoded.extend_from_slice(&decoded_size.to_be_bytes());
    encoded.extend_from_slice(&[0; 8]);

    for group in data.chunks(8) {
        let mut flags = 0u8;
        for index in 0..group.len() {
            flags |= 0x80 >> index;
        }
        encoded.push(flags);
        encoded.extend_from_slice(group);
    }
    Ok(encoded)
}

fn checked_decoded_size(data: &[u8]) -> Result<usize> {
    let size = usize::try_from(read_u32(data, 4)?)
        .map_err(|_| FormatError::at(4, "decoded size does not fit in memory"))?;
    check_output_size(size)?;
    Ok(size)
}

fn check_output_size(size: usize) -> Result<()> {
    if size > MAX_DECOMPRESSED_SIZE {
        return Err(FormatError::new(format!(
            "decoded size {size} exceeds the {MAX_DECOMPRESSED_SIZE} byte limit"
        )));
    }
    Ok(())
}

fn allocate_output(size: usize) -> Result<Vec<u8>> {
    check_output_size(size)?;
    let mut output = Vec::new();
    output
        .try_reserve_exact(size)
        .map_err(|_| FormatError::new("could not allocate decompression output"))?;
    Ok(output)
}

fn require_header(data: &[u8], name: &str) -> Result<()> {
    if data.len() < HEADER_SIZE {
        return Err(FormatError::at(
            data.len(),
            format!("truncated {name} header"),
        ));
    }
    Ok(())
}

fn read_u32(data: &[u8], offset: usize) -> Result<u32> {
    let end = offset
        .checked_add(4)
        .ok_or_else(|| FormatError::at(offset, "integer offset overflow"))?;
    let bytes = data.get(offset..end).ok_or_else(|| {
        FormatError::at(
            offset,
            format!(
                "truncated big-endian u32: {} bytes remain",
                data.len().saturating_sub(offset)
            ),
        )
    })?;
    let bytes = bytes
        .try_into()
        .map_err(|_| FormatError::at(offset, "invalid big-endian u32"))?;
    Ok(u32::from_be_bytes(bytes))
}

struct Cursor<'a> {
    data: &'a [u8],
    position: usize,
}

impl<'a> Cursor<'a> {
    fn new(data: &'a [u8], position: usize) -> Self {
        Self { data, position }
    }

    fn take<const N: usize>(&mut self) -> Result<[u8; N]> {
        let start = self.position;
        let end = start
            .checked_add(N)
            .ok_or_else(|| FormatError::at(start, "compressed input offset overflow"))?;
        let bytes = self.data.get(start..end).ok_or_else(|| {
            FormatError::at(
                start,
                format!(
                    "truncated compressed input: need {N} bytes, {} remain",
                    self.data.len().saturating_sub(start)
                ),
            )
        })?;
        self.position = end;
        bytes
            .try_into()
            .map_err(|_| FormatError::at(start, "invalid compressed input"))
    }

    fn byte(&mut self) -> Result<u8> {
        let [byte] = self.take()?;
        Ok(byte)
    }

    fn u16(&mut self) -> Result<u16> {
        Ok(u16::from_be_bytes(self.take()?))
    }

    fn u32(&mut self) -> Result<u32> {
        Ok(u32::from_be_bytes(self.take()?))
    }
}

fn copy_match(
    output: &mut Vec<u8>,
    decoded_size: usize,
    distance: usize,
    length: usize,
    offset: usize,
    name: &str,
) -> Result<()> {
    let source = output.len().checked_sub(distance).ok_or_else(|| {
        FormatError::at(
            offset,
            format!(
                "invalid {name} back-reference: distance {distance} exceeds {} decoded bytes",
                output.len()
            ),
        )
    })?;
    let end = output
        .len()
        .checked_add(length)
        .ok_or_else(|| FormatError::at(offset, "decoded output size overflow"))?;
    if end > decoded_size {
        return Err(FormatError::at(
            offset,
            format!("{name} back-reference exceeds the declared decoded size"),
        ));
    }

    // Reading one byte at a time intentionally supports overlapping matches.
    for source in (source..).take(length) {
        let byte = output.get(source).copied().ok_or_else(|| {
            FormatError::at(offset, format!("invalid {name} back-reference source"))
        })?;
        output.push(byte);
    }
    Ok(())
}

fn decompress_yaz0(data: &[u8]) -> Result<Vec<u8>> {
    require_header(data, "Yaz0")?;
    let decoded_size = checked_decoded_size(data)?;
    let mut input = Cursor::new(data, HEADER_SIZE);
    let mut output = allocate_output(decoded_size)?;
    let mut flags = 0u8;
    let mut bits_left = 0u8;

    while output.len() < decoded_size {
        if bits_left == 0 {
            flags = input.byte()?;
            bits_left = 8;
        }

        if flags & 0x80 != 0 {
            output.push(input.byte()?);
        } else {
            let token_offset = input.position;
            let first = input.byte()?;
            let second = input.byte()?;
            let distance = (((usize::from(first) & 0x0f) << 8) | usize::from(second)) + 1;
            let length = if first >> 4 == 0 {
                usize::from(input.byte()?) + 0x12
            } else {
                usize::from(first >> 4) + 2
            };
            copy_match(
                &mut output,
                decoded_size,
                distance,
                length,
                token_offset,
                "Yaz0",
            )?;
        }
        flags <<= 1;
        bits_left -= 1;
    }
    Ok(output)
}

fn decompress_yay0(data: &[u8]) -> Result<Vec<u8>> {
    require_header(data, "Yay0")?;
    let decoded_size = checked_decoded_size(data)?;
    let link_offset = usize::try_from(read_u32(data, 8)?)
        .map_err(|_| FormatError::at(8, "Yay0 link table offset is too large"))?;
    let data_offset = usize::try_from(read_u32(data, 12)?)
        .map_err(|_| FormatError::at(12, "Yay0 data offset is too large"))?;
    if link_offset > data.len() {
        return Err(FormatError::at(8, "Yay0 link table is outside the input"));
    }
    if data_offset > data.len() {
        return Err(FormatError::at(12, "Yay0 data stream is outside the input"));
    }

    let mut chunks = Cursor::new(data, HEADER_SIZE);
    let mut links = Cursor::new(data, link_offset);
    let mut literals = Cursor::new(data, data_offset);
    let mut output = allocate_output(decoded_size)?;
    let mut flags = 0u32;
    let mut bits_left = 0u8;

    while output.len() < decoded_size {
        if bits_left == 0 {
            flags = chunks.u32()?;
            bits_left = 32;
        }

        if flags & 0x8000_0000 != 0 {
            output.push(literals.byte()?);
        } else {
            let token_offset = links.position;
            let link = links.u16()?;
            let distance = usize::from(link & 0x0fff) + 1;
            let length = if link >> 12 == 0 {
                usize::from(literals.byte()?) + 0x12
            } else {
                usize::from(link >> 12) + 2
            };
            let length = length.min(decoded_size - output.len());
            copy_match(
                &mut output,
                decoded_size,
                distance,
                length,
                token_offset,
                "Yay0",
            )?;
        }
        flags <<= 1;
        bits_left -= 1;
    }
    Ok(output)
}

#[cfg(test)]
mod tests {
    use super::*;

    // Fixed byte-for-byte streams: "Hello, SZS!" encoded as two literal groups.
    const YAZ0_LITERALS: &[u8] = &[
        b'Y', b'a', b'z', b'0', 0, 0, 0, 11, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, b'H', b'e', b'l', b'l',
        b'o', b',', b' ', b'S', 0xe0, b'Z', b'S', b'!',
    ];

    // ABC, followed by an overlapping nine-byte match at distance three.
    const YAZ0_BACKREF: &[u8] = &[
        b'Y', b'a', b'z', b'0', 0, 0, 0, 12, 0, 0, 0, 0, 0, 0, 0, 0, 0xe0, b'A', b'B', b'C', 0x70,
        0x02,
    ];

    const YAY0_LITERALS: &[u8] = &[
        b'Y', b'a', b'y', b'0', 0, 0, 0, 4, 0, 0, 0, 20, 0, 0, 0, 20, 0xf0, 0, 0, 0, b'Y', b'a',
        b'y', b'0',
    ];

    // The link table is at 0x14 and the literal stream immediately follows it.
    const YAY0_BACKREF: &[u8] = &[
        b'Y', b'a', b'y', b'0', 0, 0, 0, 12, 0, 0, 0, 20, 0, 0, 0, 22, 0xe0, 0, 0, 0, 0x70, 0x02,
        b'A', b'B', b'C',
    ];

    #[test]
    fn detects_only_complete_magic() {
        assert_eq!(detect(b""), Compression::None);
        assert_eq!(detect(b"Yaz"), Compression::None);
        assert_eq!(detect(b"Yaz0"), Compression::Yaz0);
        assert_eq!(detect(b"Yay0"), Compression::Yay0);
        assert_eq!(detect(b"Yax0"), Compression::None);
        assert_eq!(decoded_size(b"plain").unwrap(), None);
        assert_eq!(decompress(b"plain").unwrap(), b"plain");
    }

    #[test]
    fn decodes_literal_only_vectors() {
        assert_eq!(decoded_size(YAZ0_LITERALS).unwrap(), Some(11));
        assert_eq!(decompress(YAZ0_LITERALS).unwrap(), b"Hello, SZS!");
        assert_eq!(decoded_size(YAY0_LITERALS).unwrap(), Some(4));
        assert_eq!(decompress(YAY0_LITERALS).unwrap(), b"Yay0");
    }

    #[test]
    fn decodes_overlapping_back_reference_vectors() {
        assert_eq!(decompress(YAZ0_BACKREF).unwrap(), b"ABCABCABCABC");
        assert_eq!(decompress(YAY0_BACKREF).unwrap(), b"ABCABCABCABC");
    }

    #[test]
    fn decodes_extended_match_lengths() {
        // One literal followed by an eighteen-byte match. A zero high nibble
        // selects the third token byte, whose value is length - 0x12.
        let yaz0 = [
            b'Y', b'a', b'z', b'0', 0, 0, 0, 19, 0, 0, 0, 0, 0, 0, 0, 0, 0x80, b'x', 0x00, 0x00,
            0x00,
        ];
        assert_eq!(decompress(&yaz0).unwrap(), vec![b'x'; 19]);

        // Two literals followed by an eighteen-byte match at distance two.
        let yay0 = [
            b'Y', b'a', b'y', b'0', 0, 0, 0, 20, 0, 0, 0, 20, 0, 0, 0, 22, 0xc0, 0, 0, 0, 0x00,
            0x01, b'A', b'B', 0x00,
        ];
        assert_eq!(decompress(&yay0).unwrap(), b"ABABABABABABABABABAB");
    }

    #[test]
    fn yay0_clips_the_final_match_to_the_declared_size() {
        let stream = [
            b'Y', b'a', b'y', b'0', 0, 0, 0, 5, 0, 0, 0, 20, 0, 0, 0, 22, 0xe0, 0, 0, 0, 0x70,
            0x02, b'A', b'B', b'C',
        ];
        assert_eq!(decompress(&stream).unwrap(), b"ABCAB");
    }

    #[test]
    fn literal_only_compressor_round_trips() {
        for data in [
            &b""[..],
            &b"a"[..],
            &b"12345678"[..],
            &b"123456789"[..],
            &b"ABCABCABCABCABCABCABCABC"[..],
        ] {
            let encoded = compress_yaz0(data).unwrap();
            assert!(encoded.starts_with(b"Yaz0"));
            assert_eq!(decompress(&encoded).unwrap(), data);
        }
    }

    #[test]
    fn rejects_bad_back_references_and_output_overruns() {
        let invalid_yaz0 = [
            b'Y', b'a', b'z', b'0', 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0x00, 0x10, 0x00,
        ];
        assert!(decompress(&invalid_yaz0).is_err());

        let invalid_yay0 = [
            b'Y', b'a', b'y', b'0', 0, 0, 0, 1, 0, 0, 0, 20, 0, 0, 0, 22, 0x00, 0, 0, 0, 0x10, 0x00,
        ];
        assert!(decompress(&invalid_yay0).is_err());

        // The match is valid by itself but would produce three bytes after the
        // initial literal although the header declares only two output bytes.
        let overrunning_yaz0 = [
            b'Y', b'a', b'z', b'0', 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0x80, b'A', 0x10, 0x00,
        ];
        assert!(decompress(&overrunning_yaz0).is_err());
    }

    #[test]
    fn rejects_truncation_and_invalid_yay0_offsets() {
        for valid in [YAZ0_LITERALS, YAZ0_BACKREF, YAY0_LITERALS, YAY0_BACKREF] {
            for end in 0..valid.len() {
                let truncated = &valid[..end];
                if detect(truncated) == Compression::None {
                    assert_eq!(decompress(truncated).unwrap(), truncated);
                } else {
                    assert!(decompress(truncated).is_err(), "accepted {end} bytes");
                }
            }
        }

        let mut invalid = YAY0_LITERALS.to_vec();
        invalid[8..12].copy_from_slice(&u32::MAX.to_be_bytes());
        assert!(decompress(&invalid).is_err());
        invalid[8..12].copy_from_slice(&20u32.to_be_bytes());
        invalid[12..16].copy_from_slice(&u32::MAX.to_be_bytes());
        assert!(decompress(&invalid).is_err());
    }

    #[test]
    fn enforces_the_output_limit_before_allocating() {
        let oversized = (MAX_DECOMPRESSED_SIZE as u32) + 1;
        for magic in [b"Yaz0", b"Yay0"] {
            let mut stream = vec![0; HEADER_SIZE];
            stream[..4].copy_from_slice(magic);
            stream[4..8].copy_from_slice(&oversized.to_be_bytes());
            assert!(decoded_size(&stream).is_err());
            assert!(decompress(&stream).is_err());
        }
    }
}
