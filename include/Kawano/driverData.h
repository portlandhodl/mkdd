#ifndef DRIVERDATA_H
#define DRIVERDATA_H

#include "JSystem/JGeometry.h"
#include "JSystem/JKernel/JKRDisposer.h"

class DriverDataChild
{
    static int mInitCounter;
    static f32 mDriverDataDefault[20][95];

    f32 data[95];

public:
    DriverDataChild();
    virtual ~DriverDataChild() {}
    f32 getData(int idx) const { return ((const f32 *)this)[idx]; }

}; // Size: 0x180

class DriverData : public JKRDisposer
{
public:
    u16 m_18; // 0x18
    u16 m_1a; // 0x1a
    f32 m_1c; // 0x1c
    s32 m_20; // 0x20
    f32 m_24; // 0x24
    f32 m_28; // 0x28
    f32 m_2c; // 0x2c
    f32 m_30; // 0x30
    f32 m_34; // 0x34
    f32 m_38; // 0x38
    f32 m_3c; // 0x3c
    f32 m_40; // 0x40
    f32 m_44; // 0x44
    JGeometry::TVec3f m_48; // 0x48
    u16 m_54; // 0x54
    f32 m_58; // 0x58
    f32 m_5c; // 0x5c
    f32 m_60; // 0x60
    f32 m_64; // 0x64
    f32 m_68; // 0x68
    f32 m_6c; // 0x6c
    f32 m_70; // 0x70
    f32 m_74; // 0x74
    f32 m_78; // 0x78

    DriverData()
    {
        m_18 = 1;
        m_1a = 0;
        m_1c = 0.5f;
        m_20 = 10;
        m_24 = 35.0f;
        m_28 = 140.0f;
        m_2c = 107.0f;
        m_30 = 58.0f;
        m_34 = 80.0f;
        m_38 = 40.0f;
        m_3c = 0.0f;
        m_40 = 0.0f;
        m_44 = 0.0f;
        m_48.set(-10.39f, -8.69f, 0.0f);
        m_54 = 0;
        m_58 = 1.0f;
        m_5c = 1.0f;
        m_60 = 1.0f;
        m_64 = 0.0f;
        m_68 = 0.0f;
        m_6c = 0.0f;
        m_70 = 0.0f;
        m_74 = 0.0f;
        m_78 = 0.0f;
    }
    virtual ~DriverData() { mspDriverData = nullptr; }
    static DriverData *create();

    static DriverDataChild msDriverData[20];
    static DriverData *mspDriverData;
}; // Size: 0x7c



#endif
