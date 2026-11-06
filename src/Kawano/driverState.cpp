#include "Kawano/driver.h"
#include "Sato/ItemObj.h"
#include "JSystem/JAudio/JASFakeMatch2.h"

void DriverModel::CheckDriverFlags()
{
    SetDriverFlags((StateEnum)mState);
}

void DriverModel::SetDriverFlags(StateEnum state)
{
    for (int i = 0; i < 2; i++)
        (&m_ac)[i] = 0;

    m_ac = CheckDriverFlags_Hokan(state);
    m_ad = CheckDriverFlags_Loop(state);
}

int DriverModel::CheckDriverFlags_Hokan(StateEnum state)
{
    if (state >= 69 && state < 150)
        return 1;
    return 0;
}

int DriverModel::CheckDriverFlags_Loop(StateEnum state)
{
    switch (state) {
    case 1:
    case 2:
    case 32:
    case 33:
    case 61:
    case 62:
    case 65:
    case 66:
    case 67:
    case 68:
    case 69:
    case 72:
    case 75:
    case 76:
    case 77:
    case 78:
    case 93:
    case 108:
    case 109:
    case 110:
    case 118:
    case 119:
    case 123:
    case 124:
    case 128:
    case 129:
    case 130:
    case 131:
    case 132:
    case 133:
    case 135:
    case 149:
        return 1;
    default:
        return 0;
    }
}

int DriverModel::IsItemWithTurn()
{
    switch (mItem->getKind()) {
    case 3:
    case 4:
    case 5:
    case 6:
    case 10:
    case 12:
    case 18:
        return 1;
    default:
        return 0;
    }
}
