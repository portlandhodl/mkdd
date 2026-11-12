#include "Kawano/driverOffset.h"

#include "Kawano/driverMgr.h"
#include "Kaneshige/KartLoader.h"
#include "JSystem/J3D/J3DAnmTransform.h"
#include "JSystem/JUtility/JUTAssert.h"
#include "JSystem/JAudio/JASFakeMatch2.h"
#include "dolphin/mtx.h"

void DriverModel::InitDriverOffsetPosition()
{
    if (IsDrive()) {
        mOffsetMode = 0;
        mOffsetDir = 0;
        mOffsetBlend = 0.0f;
    } else {
        mOffsetMode = 2;
        mOffsetDir = 2;
        mOffsetBlend = 1.0f;
    }
}

void DriverModel::SetDriverOffsetPosition_Func1(JGeometry::TVec3<f32> *vec)
{
    switch (mKartLoader->mKartInfo->getKartID()) {
    case 3:
    case 5:
    case 9:
    case 13:
    case 15:
        vec->y += 50.0f;
        break;
    case 20:
        vec->z -= 25.0f;
        break;
    case 16:
    case 17:
        vec->y += 25.0f;
        vec->z -= 10.0f;
        break;
    case 11:
        vec->y += 25.0f;
        break;
    }
}

void DriverModel::SetDriverOffsetPosition_Change(DriverData *data)
{
    Mtx &mtx = getModel()->getBaseTRMtx();
    f32 y;
    f32 z;
    Vec out;
    JGeometry::TVec3<f32> vec;
    vec.z = 0.0f;
    vec.y = 0.0f;
    vec.x = 0.0f;

#line 72
    JUT_MINMAX_ASSERT(0, mState, 152);

    mAnmID = msAnmBlendList[mState][0];
    if (mAnmID == 0)
        return;
    J3DAnmTransform *anm = getAnmTransformPtr(mAnmID);
    if (anm == 0)
        return;

    s16 cur = (s16)(anm->getFrame() + 0.5f);
    s16 max = anm->getFrameMax();
    switch (mState) {
    case 98:
    case 100:
    case 102: {
        s16 min = DriverManager::ptr()->mBlendTimes[17];
        s16 maxBlend = DriverManager::ptr()->mBlendTimes[18];
        if (max - cur <= maxBlend) {
            switch (mKartLoader->mKartInfo->getKartID()) {
            case 5:
            case 9:
            case 13:
            case 15:
                y = DriverData::msDriverData[mDataID].data[20];
                z = DriverData::msDriverData[mDataID].data[21];
                break;
            case 6:
                y = DriverData::msDriverData[mDataID].data[26];
                z = DriverData::msDriverData[mDataID].data[27];
                break;
            default:
                switch (mKartLoader->mKartInfo->getKartWeight()) {
                case 0:
                    y = DriverData::msDriverData[mDataID].data[2];
                    z = DriverData::msDriverData[mDataID].data[3];
                    break;
                case 1:
                    y = DriverData::msDriverData[mDataID].data[8];
                    z = DriverData::msDriverData[mDataID].data[9];
                    break;
                case 2:
                    y = DriverData::msDriverData[mDataID].data[14];
                    z = DriverData::msDriverData[mDataID].data[15];
                    break;
                }
                break;
            }
            s16 d = (s16)(cur - (max - maxBlend));
            vec.x = 0.0f;
            vec.y = y * d / maxBlend;
            vec.z = z * d / maxBlend;
        } else if (cur <= min) {
            switch (mKartLoader->mKartInfo->getKartID()) {
            case 5:
            case 9:
            case 13:
            case 15:
                y = DriverData::msDriverData[mDataID].data[18];
                z = DriverData::msDriverData[mDataID].data[19];
                break;
            case 6:
                y = DriverData::msDriverData[mDataID].data[24];
                z = DriverData::msDriverData[mDataID].data[25];
                break;
            default:
                switch (mKartLoader->mKartInfo->getKartWeight()) {
                case 0:
                    y = DriverData::msDriverData[mDataID].data[0];
                    z = DriverData::msDriverData[mDataID].data[1];
                    break;
                case 1:
                    y = DriverData::msDriverData[mDataID].data[6];
                    z = DriverData::msDriverData[mDataID].data[7];
                    break;
                case 2:
                    y = DriverData::msDriverData[mDataID].data[12];
                    z = DriverData::msDriverData[mDataID].data[13];
                    break;
                }
                break;
            }
            s16 d = (s16)(min - cur);
            vec.x = 0.0f;
            vec.y = y * d / min;
            vec.z = z * d / min;
        }
        break;
    }
    case 99:
    case 101:
    case 103: {
        s16 maxBlend = DriverManager::ptr()->mBlendTimes[18];
        if (cur <= maxBlend) {
            switch (mKartLoader->mKartInfo->getKartID()) {
            case 5:
            case 9:
            case 13:
            case 15:
                y = DriverData::msDriverData[mDataID].data[20];
                z = DriverData::msDriverData[mDataID].data[21];
                break;
            case 6:
                y = DriverData::msDriverData[mDataID].data[26];
                z = DriverData::msDriverData[mDataID].data[27];
                break;
            default:
                switch (mKartLoader->mKartInfo->getKartWeight()) {
                case 0:
                    y = DriverData::msDriverData[mDataID].data[2];
                    z = DriverData::msDriverData[mDataID].data[3];
                    break;
                case 1:
                    y = DriverData::msDriverData[mDataID].data[8];
                    z = DriverData::msDriverData[mDataID].data[9];
                    break;
                case 2:
                    y = DriverData::msDriverData[mDataID].data[14];
                    z = DriverData::msDriverData[mDataID].data[15];
                    break;
                }
                break;
            }
            s16 d = (s16)(maxBlend - cur);
            vec.x = 0.0f;
            vec.y = y * d / maxBlend;
            vec.z = z * d / maxBlend;
        }
        break;
    }
    default:
        break;
    }

    SetDriverOffsetPosition_Func1(&vec);

    PSMTXMultVec(mtx, &vec, &out);
    mtx[0][3] = out.x;
    mtx[1][3] = out.y;
    mtx[2][3] = out.z;
    PSMTXCopy(mtx, getModel()->getBaseTRMtx());
}

void DriverModel::SetDriverOffsetPosition_Demo(DriverData *data)
{
    DriverData *d = data;
    Mtx &mtx = getModel()->getBaseTRMtx();
    f32 x, y, z;
    x = d->m_28;
    y = d->m_2c;

    int weight = KartInfo::getDriverWeight((ECharID)mCharID);
    switch (mDataID) {
    case 9: case 13: case 15: weight = 2; break;
    }

    switch (weight) {
    case 0: z = d->m_38; y += 22.5f; break;
    case 1: z = d->m_30; break;
    case 2: z = d->m_34; y -= 23.0f; break;
    }
    z -= 20.0f;

    if (mDriverNo != 0) x = -x;
    Vec out; Vec vec;
    vec.x = x; vec.y = y; vec.z = z;

    Mtx rot;
    if (mDriverNo != 0) PSMTXRotRad(rot, 'y', 0.017453292f * -d->m_24);
    else PSMTXRotRad(rot, 'y', 0.017453292f * d->m_24);

    PSMTXConcat(mtx, rot, mtx);
    PSMTXMultVec(mtx, &vec, &out);
    mtx[0][3] = out.x; mtx[1][3] = out.y; mtx[2][3] = out.z;
    PSMTXCopy(mtx, getModel()->getBaseTRMtx());
}





void DriverModel::SetDriverOffsetPosition()
{
    DriverData *data = DriverData::mspDriverData;
    Mtx &mtx = getModel()->getBaseTRMtx();
    Vec out;
    JGeometry::TVec3<f32> vec;
    f32 f31;
    f32 f30;
    f32 f29;
    f32 f28;

    if (IsChange()) {
        SetDriverOffsetPosition_Change(data);
        return;
    }
    if (m_9c == 1) {
        SetDriverOffsetPosition_Demo(data);
        return;
    }
    if (m_e8 == 0) {
        switch (mOffsetMode) {
        case 0:
            if (!IsDrive() && !IsGoalFront() && !IsHappyFront() && !IsDemoFront())
                mOffsetMode = 1;
            mOffsetDir = 0;
            break;
        case 1:
            mOffsetBlend -= 1.0f / 30.0f;
            if (mOffsetBlend < 0.0f)
                mOffsetBlend = 0.0f;
            if (IsDrive() || IsGoalFront() || IsHappyFront() || IsDemoFront()) {
                mOffsetMode = 0;
                mOffsetDir = 1;
            }
            if (IsDamageL()) {
                mOffsetMode = 3;
                mOffsetDir = 1;
            }
            if (IsDamageS()) {
                mOffsetMode = 4;
                mOffsetDir = 1;
            }
            switch (mState) {
            case 93:
            case 32:
            case 149:
                mOffsetMode = 2;
                mOffsetDir = 1;
                mOffsetBlend = 1.0f;
                break;
            }
            break;
        case 2:
            switch (mState) {
            case 93:
            case 149:
                break;
            case 32:
            case 33:
                break;
            case 34:
            case 35:
            case 36:
            case 94:
            case 95:
                mOffsetBlend -= 1.0f / 30.0f;
                if (mOffsetBlend < 0.0f)
                    mOffsetBlend = 0.0f;
                break;
            default:
                mOffsetMode = 1;
                mOffsetDir = 2;
                break;
            }
            break;
        case 3:
            if (IsDamageL())
                mOffsetBlend += 1.0f / 30.0f;
            else
                mOffsetBlend -= 1.0f / 30.0f;
            if (mOffsetBlend > 1.0f)
                mOffsetBlend = 1.0f;
            if (mOffsetBlend < 0.0f)
                mOffsetBlend = 0.0f;
            if (!IsDamageL()) {
                if (!IsStart()) {
                    mOffsetMode = 1;
                    mOffsetDir = 3;
                }
            }
            break;
        case 4:
            if (IsDamageS12())
                mOffsetBlend += 2.0f / 30.0f;
            else
                mOffsetBlend -= 2.0f / 30.0f;
            if (mOffsetBlend > 1.0f)
                mOffsetBlend = 1.0f;
            if (mOffsetBlend < 0.0f)
                mOffsetBlend = 0.0f;
            if (!IsDamageS()) {
                if (!IsStart()) {
                    mOffsetMode = 1;
                    mOffsetDir = 4;
                }
            }
            break;
        }
    }

    int mode = mOffsetMode;
    switch (mode) {
    case 0:
        switch (mKartLoader->mKartInfo->getKartID()) {
        case 5:
        case 9:
        case 13:
        case 15:
            f31 = DriverData::msDriverData[mDataID].data[18];
            f30 = DriverData::msDriverData[mDataID].data[19];
            break;
        case 6:
            f31 = DriverData::msDriverData[mDataID].data[24];
            f30 = DriverData::msDriverData[mDataID].data[25];
            break;
        default:
            switch (mKartLoader->mKartInfo->getKartWeight()) {
            case 0:
                f31 = DriverData::msDriverData[mDataID].data[0];
                f30 = DriverData::msDriverData[mDataID].data[1];
                break;
            case 1:
                f31 = DriverData::msDriverData[mDataID].data[6];
                f30 = DriverData::msDriverData[mDataID].data[7];
                break;
            case 2:
                f31 = DriverData::msDriverData[mDataID].data[12];
                f30 = DriverData::msDriverData[mDataID].data[13];
                break;
            }
            break;
        }
        break;
    case 1:
        switch (mKartLoader->mKartInfo->getKartID()) {
        case 5:
        case 9:
        case 13:
        case 15:
            f31 = DriverData::msDriverData[mDataID].data[20];
            f30 = DriverData::msDriverData[mDataID].data[21];
            break;
        case 6:
            f31 = DriverData::msDriverData[mDataID].data[26];
            f30 = DriverData::msDriverData[mDataID].data[27];
            break;
        default:
            switch (mKartLoader->mKartInfo->getKartWeight()) {
            case 0:
                f31 = DriverData::msDriverData[mDataID].data[2];
                f30 = DriverData::msDriverData[mDataID].data[3];
                break;
            case 1:
                f31 = DriverData::msDriverData[mDataID].data[8];
                f30 = DriverData::msDriverData[mDataID].data[9];
                break;
            case 2:
                f31 = DriverData::msDriverData[mDataID].data[14];
                f30 = DriverData::msDriverData[mDataID].data[15];
                break;
            }
            break;
        }
        break;
    default: {
        switch (mKartLoader->mKartInfo->getKartID()) {
        case 5:
        case 9:
        case 13:
        case 15:
            f30 = DriverData::msDriverData[mDataID].data[20];
            f29 = DriverData::msDriverData[mDataID].data[21];
            break;
        case 6:
            f30 = DriverData::msDriverData[mDataID].data[26];
            f29 = DriverData::msDriverData[mDataID].data[27];
            break;
        default:
            switch (mKartLoader->mKartInfo->getKartWeight()) {
            case 0:
                f30 = DriverData::msDriverData[mDataID].data[2];
                f29 = DriverData::msDriverData[mDataID].data[3];
                break;
            case 1:
                f30 = DriverData::msDriverData[mDataID].data[8];
                f29 = DriverData::msDriverData[mDataID].data[9];
                break;
            case 2:
                f30 = DriverData::msDriverData[mDataID].data[14];
                f29 = DriverData::msDriverData[mDataID].data[15];
                break;
            }
            break;
        }

        if (mode == 4 || mOffsetDir == 4) {
            s32 id = mDataID;
            switch (id) {
            case 0:
            case 1:
            case 2:
            case 3:
            case 16:
            case 17:
                switch (mKartLoader->mKartInfo->getKartID()) {
                case 5: case 13: f31 = DriverData::msDriverData[id].data[54]; f28 = DriverData::msDriverData[id].data[55]; break;
                case 3: case 11: f31 = DriverData::msDriverData[id].data[56]; f28 = DriverData::msDriverData[id].data[57]; break;
                case 9: f31 = DriverData::msDriverData[id].data[58]; f28 = DriverData::msDriverData[id].data[59]; break;
                case 15: f31 = DriverData::msDriverData[id].data[60]; f28 = DriverData::msDriverData[id].data[61]; break;
                case 16: case 17: f31 = DriverData::msDriverData[id].data[62]; f28 = DriverData::msDriverData[id].data[63]; break;
                case 0: case 8: f31 = DriverData::msDriverData[id].data[64]; f28 = DriverData::msDriverData[id].data[65]; break;
                case 4: case 12: f31 = DriverData::msDriverData[id].data[64]; f28 = DriverData::msDriverData[id].data[65]; break;
                case 2: case 10: f31 = DriverData::msDriverData[id].data[68]; f28 = DriverData::msDriverData[id].data[69]; break;
                case 14: f31 = DriverData::msDriverData[id].data[70]; f28 = DriverData::msDriverData[id].data[71]; break;
                case 6: f31 = DriverData::msDriverData[id].data[72]; f28 = DriverData::msDriverData[id].data[73]; break;
                case 1: f31 = DriverData::msDriverData[id].data[74]; f28 = DriverData::msDriverData[id].data[75]; break;
                case 7: f31 = DriverData::msDriverData[id].data[76]; f28 = DriverData::msDriverData[id].data[77]; break;
                case 18: case 19: f31 = DriverData::msDriverData[id].data[78]; f28 = DriverData::msDriverData[id].data[79]; break;
                case 20: f31 = DriverData::msDriverData[id].data[80]; f28 = DriverData::msDriverData[id].data[81]; break;
                }
                break;
            default:
                f31 = f30;
                f28 = f29;
                break;
            }
        } else {
            switch (mKartLoader->mKartInfo->getKartID()) {
            case 5: case 13: f31 = DriverData::msDriverData[mDataID].data[22]; f28 = DriverData::msDriverData[mDataID].data[23]; break;
            case 3: case 11: f31 = DriverData::msDriverData[mDataID].data[30]; f28 = DriverData::msDriverData[mDataID].data[31]; break;
            case 9: f31 = DriverData::msDriverData[mDataID].data[32]; f28 = DriverData::msDriverData[mDataID].data[33]; break;
            case 15: f31 = DriverData::msDriverData[mDataID].data[34]; f28 = DriverData::msDriverData[mDataID].data[35]; break;
            case 16: case 17: f31 = DriverData::msDriverData[mDataID].data[36]; f28 = DriverData::msDriverData[mDataID].data[37]; break;
            case 0: case 8: f31 = DriverData::msDriverData[mDataID].data[38]; f28 = DriverData::msDriverData[mDataID].data[39]; break;
            case 4: case 12: f31 = DriverData::msDriverData[mDataID].data[38]; f28 = DriverData::msDriverData[mDataID].data[39]; break;
            case 2: case 10: f31 = DriverData::msDriverData[mDataID].data[42]; f28 = DriverData::msDriverData[mDataID].data[43]; break;
            case 14: f31 = DriverData::msDriverData[mDataID].data[44]; f28 = DriverData::msDriverData[mDataID].data[45]; break;
            case 6: f31 = DriverData::msDriverData[mDataID].data[28]; f28 = DriverData::msDriverData[mDataID].data[29]; break;
            case 1: f31 = DriverData::msDriverData[mDataID].data[46]; f28 = DriverData::msDriverData[mDataID].data[47]; break;
            case 7: f31 = DriverData::msDriverData[mDataID].data[48]; f28 = DriverData::msDriverData[mDataID].data[49]; break;
            case 18: case 19: f31 = DriverData::msDriverData[mDataID].data[50]; f28 = DriverData::msDriverData[mDataID].data[51]; break;
            case 20: f31 = DriverData::msDriverData[mDataID].data[52]; f28 = DriverData::msDriverData[mDataID].data[53]; break;
            default:
                switch (mKartLoader->mKartInfo->getKartWeight()) {
                case 0: f31 = DriverData::msDriverData[mDataID].data[4]; f28 = DriverData::msDriverData[mDataID].data[5]; break;
                case 1: f31 = DriverData::msDriverData[mDataID].data[10]; f28 = DriverData::msDriverData[mDataID].data[11]; break;
                case 2: f31 = DriverData::msDriverData[mDataID].data[16]; f28 = DriverData::msDriverData[mDataID].data[17]; break;
                }
                break;
            }
        }

        f31 = (1.0f - mOffsetBlend) * f30 + mOffsetBlend * f31;
        f30 = (1.0f - mOffsetBlend) * f29 + mOffsetBlend * f28;
        break;
    }
    }

    vec.x = 0.0f;
    vec.y = f31;
    vec.z = f30;

    SetDriverOffsetPosition_Func1(&vec);

    PSMTXMultVec(mtx, &vec, &out);
    mtx[0][3] = out.x;
    mtx[1][3] = out.y;
    mtx[2][3] = out.z;
    PSMTXCopy(mtx, getModel()->getBaseTRMtx());
}
