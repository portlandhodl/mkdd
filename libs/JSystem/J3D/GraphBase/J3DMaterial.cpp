#include "JSystem/J3D/J3DMaterial.h"
#include "JSystem/J3D/J3DColorBlock.h"
#include "JSystem/J3D/J3DInd.h"
#include "JSystem/J3D/J3DPE.h"
#include "macros.h"

DECL_WEAK void J3DColorBlock::load() { }

DECL_WEAK void J3DTevBlock::load() { }

DECL_WEAK void J3DColorBlock::patch() { }

DECL_WEAK void J3DColorBlock::diff(u32) { }

DECL_WEAK void J3DPEBlock::diff(u32) { }

DECL_WEAK void J3DPEBlock::reset(J3DPEBlock*) { }

DECL_WEAK void J3DIndBlock::reset(J3DIndBlock*) { }

DECL_WEAK void J3DTevBlock::reset(J3DTevBlock*) { }

DECL_WEAK void J3DTexGenBlock::reset(J3DTexGenBlock*) { }

DECL_WEAK void J3DColorBlock::reset(J3DColorBlock*) { }

void J3DPatchedMaterial::makeDisplayList() { }

void J3DPatchedMaterial::makeSharedDisplayList() { }

void J3DPatchedMaterial::reset() { }

void J3DPatchedMaterial::change() { }

void J3DLockedMaterial::makeDisplayList() { }

void J3DLockedMaterial::makeSharedDisplayList() { }

void J3DLockedMaterial::patch() { }

void J3DLockedMaterial::diff(u32) { }

void J3DLockedMaterial::calc(const Mtx) { }

void J3DLockedMaterial::reset() { }
