use crate::error::{FormatError, Result};

pub fn range(data: &[u8], offset: usize, length: usize) -> Result<std::ops::Range<usize>> {
    let end = offset
        .checked_add(length)
        .ok_or_else(|| FormatError::at(offset, "offset overflow"))?;
    if end > data.len() {
        return Err(FormatError::at(
            offset,
            format!(
                "need {length} bytes, only {} remain",
                data.len().saturating_sub(offset)
            ),
        ));
    }
    Ok(offset..end)
}

pub fn bytes(data: &[u8], offset: usize, length: usize) -> Result<&[u8]> {
    Ok(&data[range(data, offset, length)?])
}

pub fn u8_at(data: &[u8], offset: usize) -> Result<u8> {
    data.get(offset)
        .copied()
        .ok_or_else(|| FormatError::at(offset, "missing u8"))
}

pub fn i8_at(data: &[u8], offset: usize) -> Result<i8> {
    Ok(u8_at(data, offset)? as i8)
}

pub fn u16_at(data: &[u8], offset: usize) -> Result<u16> {
    Ok(u16::from_be_bytes(
        bytes(data, offset, 2)?.try_into().unwrap(),
    ))
}

pub fn i16_at(data: &[u8], offset: usize) -> Result<i16> {
    Ok(i16::from_be_bytes(
        bytes(data, offset, 2)?.try_into().unwrap(),
    ))
}

pub fn u32_at(data: &[u8], offset: usize) -> Result<u32> {
    Ok(u32::from_be_bytes(
        bytes(data, offset, 4)?.try_into().unwrap(),
    ))
}

pub fn i32_at(data: &[u8], offset: usize) -> Result<i32> {
    Ok(i32::from_be_bytes(
        bytes(data, offset, 4)?.try_into().unwrap(),
    ))
}

pub fn f32_at(data: &[u8], offset: usize) -> Result<f32> {
    Ok(f32::from_bits(u32_at(data, offset)?))
}

pub fn vec3_at(data: &[u8], offset: usize) -> Result<[f32; 3]> {
    Ok([
        f32_at(data, offset)?,
        f32_at(data, offset + 4)?,
        f32_at(data, offset + 8)?,
    ])
}

pub fn put_u16(data: &mut [u8], offset: usize, value: u16) -> Result<()> {
    bytes_mut(data, offset, 2)?.copy_from_slice(&value.to_be_bytes());
    Ok(())
}

pub fn put_i16(data: &mut [u8], offset: usize, value: i16) -> Result<()> {
    bytes_mut(data, offset, 2)?.copy_from_slice(&value.to_be_bytes());
    Ok(())
}

pub fn put_u32(data: &mut [u8], offset: usize, value: u32) -> Result<()> {
    bytes_mut(data, offset, 4)?.copy_from_slice(&value.to_be_bytes());
    Ok(())
}

pub fn put_i32(data: &mut [u8], offset: usize, value: i32) -> Result<()> {
    bytes_mut(data, offset, 4)?.copy_from_slice(&value.to_be_bytes());
    Ok(())
}

pub fn put_f32(data: &mut [u8], offset: usize, value: f32) -> Result<()> {
    if !value.is_finite() {
        return Err(FormatError::new("floating-point values must be finite"));
    }
    put_u32(data, offset, value.to_bits())
}

pub fn put_vec3(data: &mut [u8], offset: usize, value: [f32; 3]) -> Result<()> {
    for (component, value) in value.into_iter().enumerate() {
        put_f32(data, offset + component * 4, value)?;
    }
    Ok(())
}

pub fn bytes_mut(data: &mut [u8], offset: usize, length: usize) -> Result<&mut [u8]> {
    let byte_range = range(data, offset, length)?;
    Ok(&mut data[byte_range])
}

#[derive(Clone, Copy)]
pub struct Reader<'a> {
    data: &'a [u8],
    position: usize,
}

impl<'a> Reader<'a> {
    pub fn new(data: &'a [u8]) -> Self {
        Self { data, position: 0 }
    }

    pub fn with_position(data: &'a [u8], position: usize) -> Result<Self> {
        range(data, position, 0)?;
        Ok(Self { data, position })
    }

    pub fn position(&self) -> usize {
        self.position
    }

    pub fn seek(&mut self, position: usize) -> Result<()> {
        range(self.data, position, 0)?;
        self.position = position;
        Ok(())
    }

    pub fn skip(&mut self, length: usize) -> Result<()> {
        let next = self
            .position
            .checked_add(length)
            .ok_or_else(|| FormatError::new("cursor overflow"))?;
        self.seek(next)
    }

    pub fn take(&mut self, length: usize) -> Result<&'a [u8]> {
        let result = bytes(self.data, self.position, length)?;
        self.position += length;
        Ok(result)
    }

    pub fn u8(&mut self) -> Result<u8> {
        Ok(self.take(1)?[0])
    }

    pub fn i8(&mut self) -> Result<i8> {
        Ok(self.u8()? as i8)
    }

    pub fn u16(&mut self) -> Result<u16> {
        Ok(u16::from_be_bytes(self.take(2)?.try_into().unwrap()))
    }

    pub fn i16(&mut self) -> Result<i16> {
        Ok(i16::from_be_bytes(self.take(2)?.try_into().unwrap()))
    }

    pub fn u32(&mut self) -> Result<u32> {
        Ok(u32::from_be_bytes(self.take(4)?.try_into().unwrap()))
    }

    pub fn i32(&mut self) -> Result<i32> {
        Ok(i32::from_be_bytes(self.take(4)?.try_into().unwrap()))
    }

    pub fn f32(&mut self) -> Result<f32> {
        Ok(f32::from_bits(self.u32()?))
    }

    pub fn vec3(&mut self) -> Result<[f32; 3]> {
        Ok([self.f32()?, self.f32()?, self.f32()?])
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn reads_big_endian_primitives() {
        let data = [0x12, 0x34, 0xff, 0xfe, 0x3f, 0x80, 0, 0];
        assert_eq!(u16_at(&data, 0).unwrap(), 0x1234);
        assert_eq!(i16_at(&data, 2).unwrap(), -2);
        assert_eq!(f32_at(&data, 4).unwrap(), 1.0);
    }

    #[test]
    fn rejects_overflow_and_non_finite_writes() {
        assert!(range(&[0; 4], usize::MAX, 2).is_err());
        assert!(put_f32(&mut [0; 4], 0, f32::NAN).is_err());
    }
}
