#include "PowerPC_EABI_Support/MetroTRK/trk.h"
#include "PowerPC_EABI_Support/MetroTRK/target_options.h"

typedef enum {
	DS_NoError = 0x0,
} DSError;

typedef enum {
	DS_Stdin  = 0,
	DS_Stdout = 1,
	DS_Stderr = 2,
} DSFileHandle;

typedef enum {
	DS_IONoError = 0,
	DS_IOError   = 1,
	DS_IOEOF     = 2,
} DSIOResult;

typedef enum {
	DSMSG_ReadFile  = 0xD1,
	DSMSG_WriteFile = 0xD0,
	DSMSG_CloseFile = 0xD3,
} MessageCommandID;

extern int GetTRKConnected(void);
extern u32 TRKAccessFile(u32, u32, u32*, u8*);
extern u32 TRKCloseFile(u32, u32);

// forward declares
DSIOResult __read_file(u32 handle, u8* buffer, size_t* count, void* ref_con);
DSIOResult __write_file(u32 handle, u8* buffer, size_t* count, void* ref_con);
DSIOResult __close_file(u32 handle, u8* buffer, size_t* count, void* ref_con);
DSIOResult __access_file(u32 handle, u8* buffer, size_t* count, void* ref_con, MessageCommandID cmd);

/**
 * @note Address: 0x800C0B4C
 * @note Size: 0xBC
 */
DSIOResult __read_console(u32 handle, u8* buffer, size_t* count, void* ref_con)
{
	if (GetUseSerialIO() == 0) {
		return DS_IOError;
	}
	return __read_file(DS_Stdin, buffer, count, ref_con);
}

/**
 * @note Address: 0x800C0A90
 * @note Size: 0xBC
 */
DSIOResult __TRK_write_console(u32 handle, u8* buffer, size_t* count, void* ref_con)
{
	if (GetUseSerialIO() == 0) {
		return DS_IOError;
	}
	return __write_file(DS_Stdout, buffer, count, ref_con);
}

/**
 * @note Address: 0x800C0A0C
 * @note Size: 0x84
 */
DSIOResult __close_console(u32 handle, u8* buffer, size_t* count, void* ref_con)
{
	return __close_file(handle, buffer, count, ref_con);
}

/**
 * @note Address: N/A
 * @note Size: 0xB4
 */
DSIOResult __read_file(u32 handle, u8* buffer, size_t* count, void* ref_con)
{
	return __access_file(handle, buffer, count, ref_con, DSMSG_ReadFile);
}

/**
 * @note Address: N/A
 * @note Size: 0xB4
 */
DSIOResult __write_file(u32 handle, u8* buffer, size_t* count, void* ref_con)
{
	return __access_file(handle, buffer, count, ref_con, DSMSG_WriteFile);
}

/**
 * @note Address: N/A
 * @note Size: 0x17C
 */
void __open_file(void)
{
	// UNUSED FUNCTION
}

/**
 * @note Address: N/A
 * @note Size: 0x84
 */
DSIOResult __close_file(u32 handle, u8* buffer, size_t* count, void* ref_con)
{
	u32 r0;

	if (GetTRKConnected() == DS_NoError) {
		return DS_IOError;
	}

	r0 = TRKCloseFile(DSMSG_CloseFile, handle);

	switch ((u8)r0) {
	case DS_IONoError:
		return DS_IONoError;
	case DS_IOEOF:
		return DS_IOEOF;
	}

	return DS_IOError;
}

/**
 * @note Address: N/A
 * @note Size: 0xDC
 */
void __position_file(void)
{
	// UNUSED FUNCTION
}

/**
 * @note Address: N/A
 * @note Size: 0xE0
 */
void convertFileMode(void)
{
	// UNUSED FUNCTION
}

/**
 * @note Address: N/A
 * @note Size: 0xC0
 */
DSIOResult __access_file(u32 handle, u8* buffer, size_t* count, void* ref_con, MessageCommandID cmd)
{
	size_t countTemp;
	u32 r0;

	if (GetTRKConnected() == DS_NoError) {
		return DS_IOError;
	}

	countTemp = *count;
	r0        = TRKAccessFile(cmd, handle, &countTemp, buffer);
	*count    = countTemp;

	switch ((u8)r0) {
	case DS_IONoError:
		return DS_IONoError;
	case DS_IOEOF:
		return DS_IOEOF;
	}

	return DS_IOError;
}

/**
 * @note Address: N/A
 * @note Size: 0x1D0
 */
void __open_temp_file(void)
{
	// UNUSED FUNCTION
}
