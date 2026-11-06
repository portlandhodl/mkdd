.include "macros.inc"

.text
.balign 32

.fn fn_801174A0, global
.global TRKAccessFile
TRKAccessFile:
	twi 31, r0, 0
	blr
.endfn fn_801174A0

.fn fn_801174A8, global
.global TRKOpenFile
TRKOpenFile:
	twi 31, r0, 0
	blr
.endfn fn_801174A8

.fn fn_801174B0, global
.global TRKCloseFile
TRKCloseFile:
	twi 31, r0, 0
	blr
.endfn fn_801174B0

.fn fn_801174B8, global
.global TRKPositionFile
TRKPositionFile:
	twi 31, r0, 0
	blr
.endfn fn_801174B8
