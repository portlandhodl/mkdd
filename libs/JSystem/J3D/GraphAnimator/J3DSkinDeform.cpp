#include "JSystem/J3D/J3DSkinDeform.h"
#include "JSystem/J3D/J3DVtxColorCalc.h"

void J3DSkinDeform::deform(J3DModel* model)
{
    if (model->checkFlag(J3DMODEL_SkinPosCpu)) {
        setFlag(J3DSKIN_Unk2);
    } else {
        resetFlag(J3DSKIN_Unk2);
    }

    if (model->checkFlag(J3DMODEL_SkinNrmCpu)) {
        setFlag(J3DSKIN_Unk1);
    } else {
        resetFlag(J3DSKIN_Unk1);
    }

    deform(model->getVertexBuffer(), model->mMtxBuffer);
}

void J3DVtxColorCalc::calc(J3DModel* model)
{
    calc(model->getVertexBuffer());
}
