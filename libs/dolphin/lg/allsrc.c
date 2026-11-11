#include <dolphin/types.h>
#include <dolphin/os.h>
#include <dolphin/lg.h>

typedef struct SISteeringStatus {
    u16 button;
    u8 misc;
    s8 steering;
    u8 gas;
    u8 brake;
    u8 left;
    u8 right;
    s8 err;
} SISteeringStatus;

double pow();
double sin();

typedef void (*SISteeringCallback)(s32 chan, s32 result);

s32 SIResetSteeringAsync(s32 chan, SISteeringCallback callback);
s32 SIReadSteering(s32 chan, SISteeringStatus *status);
s32 SIControlSteering(s32 chan, u32 control, s32 level);
void SISetSteeringSamplingCallback(void (*callback)(void));

typedef struct LGEnvelope {
    u32 attackTime;
    u32 fadeTime;
    u8 attackLevel;
    u8 fadeLevel;
} LGEnvelope;

typedef struct LGCondition {
    s8 offset;
    u8 deadband;
    u8 saturationNeg;
    u8 saturationPos;
    s16 coefficientNeg;
    s16 coefficientPos;
} LGCondition;

typedef struct LGForceEffect {
    u8 type;
    u32 duration;
    u32 startDelay;
    union {
        struct {
            s16 magnitude;
            u16 direction;
            LGEnvelope envelope;
        } constant;
        struct {
            s16 startMagnitude;
            s16 endMagnitude;
            u16 direction;
        } ramp;
        struct {
            u8 magnitude;
            u16 direction;
            u16 period;
            u16 phase;
            s16 offset;
            LGEnvelope envelope;
        } periodic;
        LGCondition condition;
    } p;
} LGForceEffect;

struct VDevice;

typedef struct Effect {
    LGForceEffect fx;
    struct VDevice *vdevice;
    u32 flags;
    u32 elapsed;
    u32 delay;
    s32 force;
    union {
        struct {
            s32 lower;
            s32 upper;
        } condition;
        struct {
            u32 phase;
        } periodic;
    } state;
} Effect;

typedef struct SimThread {
    struct VDevice *vdevice;
    u32 lastTime;
    s32 position;
    s32 positionFixed;
    s32 filterCoef[10];
    s32 filterHistory[10];
    s32 filterIndex;
    s32 velocity;
    s32 deltaPos[16];
    s32 deltaTime[16];
    s32 ringIndex;
    s32 lastForce;
} SimThread;

typedef struct VDevice {
    SimThread sim;
    struct LGDevice *device;
    Effect *effects[64];
    Effect effectPool[64];
    s32 gain;
} VDevice;

typedef struct LGCalibration {
    struct LGPosition min;
    struct LGPosition max;
    struct LGPosition deadzone;
    SISteeringStatus prev;
    s32 pedalWait;
} LGCalibration;

typedef struct LGDevice {
    s32 chan;
    u8 opened : 1;
    u8 needsReset : 1;
    struct LGDevice *handle;
    SISteeringStatus raw;
    LGCalibration calib;
    struct LGPosition status;
    VDevice vdevice;
} LGDevice;

static LGDevice g_lgDevices[4];
static BOOL g_lgInitialized = FALSE;
static u8 g_iGammaLookup[129];
static BOOL g_bGammaInitialized = FALSE;
static s16 g_iQuarterSineTable[64];
static s16 g_iRampUpTable[64];

#define LG_INVALID_HANDLE ((LGDevice *)-1)

static void VDevice_Init(VDevice *vdevice);
static void VDevice_RecalcGammaTable(void);
static s32 VDevice_DownloadEffect(VDevice *vdevice, u32 *id, LGForceEffect *fx);
static s32 VDevice_UpdateEffect(VDevice *vdevice, u32 id, LGForceEffect *fx);
static s32 VDevice_DestroyEffect(VDevice *vdevice, u32 id);
static s32 VDevice_StartEffect(VDevice *vdevice, u32 id);
static s32 VDevice_StopEffect(VDevice *vdevice, u32 id);
static s32 VDevice_Initialize(VDevice *vdevice, LGDevice *device);
static s32 VDevice_Shutdown(VDevice *vdevice);
static u32 VDevice_GetFreeEffect(VDevice *vdevice);
static void Effect_Init(Effect *effect);
static s32 Effect_UpdateEffect(Effect *effect, const LGForceEffect *fx);
static s32 Effect_StartEffect(Effect *effect);
static s32 Effect_StopEffect(Effect *effect);
static BOOL Effect_Update(Effect *effect, u32 dt);
static s32 Effect_PerformEnvelope(Effect *effect, LGEnvelope *envelope, s32 magnitude);
static void Effect_PolarToRect(s32 magnitude, u32 direction, s32 *out);
static s32 Effect_UpdateSpring(Effect *effect, const void *condition);
static s32 Effect_UpdateDamper(Effect *effect, const void *condition);
static void SimThread_Init(SimThread *sim, VDevice *vdevice);
static s32 SimThread_Step(SimThread *sim);
s32 LGClose(LGDevice *device);

static void SteeringResetCallback(s32 chan, s32 result) {
    if (result) {
        g_lgDevices[chan].status.err = -1;
    }
}

static void SeparateToCombined(struct LGPosition *position) {
    s32 value = (position->gasPedal - position->brakePedal) / 2;
    if (value < -128) {
        value = -128;
    } else if (value > 127) {
        value = 127;
    }
    position->_6 = value;
}

static s8 AutoCalibrateAxisSigned(s8 value, s8 *pmin, s8 *pmax, s8 deadzone) {
    s32 min = *pmin;
    s32 max = *pmax;
    s32 range;
    s32 result;

    if (value < min) {
        *pmin = value;
        min = value;
    }
    if (value > max) {
        *pmax = value;
        max = (s8)value;
    }
    range = max - min - deadzone * 2;
    if (range == 0) {
        range = 1;
    }
    result = (value - (min + deadzone)) * 255 / range - 127;
    if (result < -128) {
        result = -128;
    }
    if (result > 127) {
        result = 127;
    }
    return result;
}

static u8 AutoCalibrateAxisUnsigned(u8 value, u8 *pmin, u8 *pmax, u8 deadzone) {
    s32 v = value;
    s32 min = *pmin;
    s32 max = *pmax;
    s32 range;
    s32 result;

    if (v < min) {
        *pmin = value;
        min = v;
    }
    if (v > max) {
        *pmax = value;
        max = v;
    }
    range = max - min - deadzone * 2;
    if (range == 0) {
        range = 1;
    }
    result = (v - (min + deadzone)) * 255 / range;
    if (result < 0) {
        result = 0;
    }
    if (result > 255) {
        result = 255;
    }
    return result;
}

static void InitPedalAutocalib(LGCalibration *calib) {
    calib->min.brakePedal = calib->min.gasPedal = 80;
    SeparateToCombined(&calib->min);
    calib->max.brakePedal = calib->max.gasPedal = 176;
    SeparateToCombined(&calib->max);
    calib->deadzone.brakePedal = calib->deadzone.gasPedal = 10;
}

static void HandlePedals(struct LGPosition *status, SISteeringStatus *raw, LGCalibration *calib) {
    if ((raw->misc ^ calib->prev.misc) & 8) {
        calib->pedalWait = 64;
        InitPedalAutocalib(calib);
    }

    if (calib->pedalWait > 0) {
        calib->pedalWait--;
        status->gasPedal = status->brakePedal = 0;
        status->_6 = 0;
        return;
    }

    if (raw->misc & 8) {
        status->gasPedal = AutoCalibrateAxisUnsigned(raw->gas, &calib->min.gasPedal, &calib->max.gasPedal, calib->deadzone.gasPedal);
        status->brakePedal = AutoCalibrateAxisUnsigned(raw->brake, &calib->min.brakePedal, &calib->max.brakePedal, calib->deadzone.brakePedal);
        SeparateToCombined(status);
    } else {
        status->gasPedal = status->brakePedal = 0;
        status->_6 = 0;
    }
}

static void HandleTriggers(struct LGPosition *status, SISteeringStatus *raw, LGCalibration *calib) {
    s32 value;

    if (raw->left > calib->max.analogL - calib->deadzone.analogL * 2) {
        if ((raw->button ^ calib->prev.button) & 0x40) {
            value = raw->left - calib->deadzone.analogL;
            calib->max.analogL = value < 0 ? 0 : value;
            status->analogL = 255;
        } else if (raw->button & 0x40) {
            status->analogL = 255;
        } else {
            status->analogL = AutoCalibrateAxisUnsigned(raw->left, &calib->min.analogL, &calib->max.analogL, calib->deadzone.analogL);
        }
    } else {
        status->analogL = AutoCalibrateAxisUnsigned(raw->left, &calib->min.analogL, &calib->max.analogL, calib->deadzone.analogL);
    }

    if (raw->right > calib->max.analogR - calib->deadzone.analogR * 2) {
        if ((raw->button ^ calib->prev.button) & 0x20) {
            value = raw->right - calib->deadzone.analogR;
            calib->max.analogR = value < 0 ? 0 : value;
            status->analogR = 255;
        } else if (raw->button & 0x20) {
            status->analogR = 255;
        } else {
            status->analogR = AutoCalibrateAxisUnsigned(raw->right, &calib->min.analogR, &calib->max.analogR, calib->deadzone.analogR);
        }
    } else {
        status->analogR = AutoCalibrateAxisUnsigned(raw->right, &calib->min.analogR, &calib->max.analogR, calib->deadzone.analogR);
    }
}

static void CookValues(struct LGPosition *status, SISteeringStatus *raw, LGCalibration *calib) {
    status->steerDirection = AutoCalibrateAxisSigned(raw->steering, &calib->min.steerDirection, &calib->max.steerDirection,
                                               calib->deadzone.steerDirection);

    HandlePedals(status, raw, calib);
    HandleTriggers(status, raw, calib);

    status->button = raw->button;
    status->_2 = raw->misc;
    calib->prev = *raw;
    status->err = 0;
}

static void SteeringSamplingCallback(void) {
    s32 i;
    for (i = 0; i < 4; i++) {
        LGDevice *device = &g_lgDevices[i];
        if (device->opened) {
            SISteeringStatus *raw = &device->raw;
            SIReadSteering(i, raw);
            switch (raw->err) {
            case -1:
                device->status.err = -1;
                device->needsReset = 1;
                break;
            case 0:
                CookValues(&device->status, raw, &device->calib);
                SimThread_Step(&device->vdevice.sim);
                break;
            }
        }
    }
}

static void InitDevice(LGDevice *device, s32 chan) {
    device->chan = chan;
    device->opened = 0;
    device->needsReset = 0;
    device->handle = LG_INVALID_HANDLE;

    device->raw.button = 0;
    device->raw.misc = 0;
    device->raw.steering = 0;
    device->raw.gas = 128;
    device->raw.brake = 128;
    device->raw.left = 128;
    device->raw.right = 128;
    device->raw.err = -1;

    device->calib.min.steerDirection = -64;
    device->calib.min.analogL = 80;
    device->calib.min.analogR = 80;
    device->calib.max.steerDirection = 64;
    device->calib.max.analogL = 176;
    device->calib.max.analogR = 176;
    device->calib.deadzone.steerDirection = 6;
    device->calib.deadzone.analogL = 12;
    device->calib.deadzone.analogR = 12;
    InitPedalAutocalib(&device->calib);

    CookValues(&device->status, &device->raw, &device->calib);
}

void LGInit(void) {
    LGDevice *device;
    s32 i;
    if (!g_lgInitialized) {
        for (i = 0; i < 4; i++) {
            device = &g_lgDevices[i];
            VDevice_Init(&device->vdevice);
            InitDevice(device, i);
            SIResetSteeringAsync(i, SteeringResetCallback);
            SIReadSteering(i, &device->raw);
        }
        SISetSteeringSamplingCallback(SteeringSamplingCallback);
        g_lgInitialized = TRUE;
    }
}

s32 LGOpen(s32 chan, LGDevice **handle) {
    LGDevice *device;

    if (chan >= 4 || chan < 0) {
        return -2;
    }
    if (handle == NULL) {
        return -2;
    }

    device = &g_lgDevices[chan];
    if (device->needsReset) {
        LGClose(device);
    }
    if (device->opened) {
        *handle = device->handle;
        return -3;
    }

    if (SIResetSteeringAsync(chan, SteeringResetCallback) != -1) {
        SISteeringStatus *raw;
        InitDevice(device, chan);
        raw = &device->raw;
        SIReadSteering(chan, raw);
        switch (g_lgDevices[chan].raw.err) {
        case -1:
            g_lgDevices[chan].status.err = -1;
            return -4;
        case 0:
            CookValues(&device->status, raw, &device->calib);
        }
        g_lgDevices[chan].status.err = 0;
        *handle = device;
        device->chan = chan;
        device->handle = *handle;
        VDevice_Initialize(&device->vdevice, device);
        device->opened = 1;
        return 0;
    }
    return -4;
}

s32 LGClose(LGDevice *device) {
    if (device == NULL || device == LG_INVALID_HANDLE) {
        return -2;
    }
    if (!device->opened) {
        return -2;
    }
    device->opened = 0;
    device->needsReset = 0;
    VDevice_Shutdown(&device->vdevice);
    device->handle = LG_INVALID_HANDLE;
    return 0;
}

s32 LGRead(struct LGPosition *status) {
    s32 i;

    if (status == NULL) {
        return -2;
    }

    for (i = 0; i < 4; i++) {
        LGDevice *device = &g_lgDevices[i];
        if (device->needsReset) {
            LGClose(device);
        }
        if (device->opened) {
            status[i] = device->status;
        } else {
            status[i].err = -1;
        }
    }
    return 0;
}

s32 LGDownloadForceEffect(LGDevice *device, u32 *id, LGForceEffect *fx) {
    if (device == NULL || device == LG_INVALID_HANDLE) {
        return -2;
    }
    if (!device->opened) {
        return -2;
    }
    if (id == NULL) {
        return -2;
    }
    if (fx == NULL) {
        return -2;
    }
    if (device->needsReset) {
        LGClose(device);
        return -2;
    }
    return VDevice_DownloadEffect(&device->vdevice, id, fx);
}

s32 LGUpdateForceEffect(u32 id, LGForceEffect *fx) {
    LGDevice *device;
    u32 chan = id >> 16;

    if (chan >= 4) {
        return -2;
    }
    device = &g_lgDevices[chan];
    if (!device->opened) {
        return -2;
    }
    if (fx == NULL) {
        return -2;
    }
    if (device->needsReset) {
        LGClose(device);
        return -2;
    }
    return VDevice_UpdateEffect(&device->vdevice, id, fx);
}

s32 LGStartForceEffect(u32 id) {
    LGDevice *device;
    u32 chan = id >> 16;

    if (chan >= 4) {
        return -2;
    }
    device = &g_lgDevices[chan];
    if (!device->opened) {
        return -2;
    }
    if (device->needsReset) {
        LGClose(device);
        return -2;
    }
    return VDevice_StartEffect(&device->vdevice, id);
}

s32 LGStopForceEffect(u32 id) {
    LGDevice *device;
    u32 chan = id >> 16;

    if (chan >= 4) {
        return -2;
    }
    device = &g_lgDevices[chan];
    if (!device->opened) {
        return -2;
    }
    if (device->needsReset) {
        LGClose(device);
        return -2;
    }
    return VDevice_StopEffect(&device->vdevice, id);
}

s32 LGDestroyForceEffect(u32 id) {
    LGDevice *device;
    u32 chan = id >> 16;

    if (chan >= 4) {
        return -2;
    }
    device = &g_lgDevices[chan];
    if (!device->opened) {
        return -2;
    }
    if (device->needsReset) {
        LGClose(device);
        return -2;
    }
    return VDevice_DestroyEffect(&device->vdevice, id);
}

static s32 MulDivLowCost(s32 a, s32 b, s32 c) {
    s32 half = c / 2;
    if (a < 0) {
        half = -half;
    }
    b = a * b;
    b += half;
    return b / c;
}

static void zero_mem_aligned(void *dst, u32 n) {
    u32 *lp = (u32 *)dst - 1;
    u8 *cp;
    u32 i = n >> 5;

    if (i) {
        do {
            lp[1] = 0;
            lp[2] = 0;
            lp[3] = 0;
            lp[4] = 0;
            lp[5] = 0;
            lp[6] = 0;
            lp[7] = 0;
            *(lp += 8) = 0;
        } while (--i);
    }
    i = (n & 31) >> 2;
    if (i) {
        do {
            *++lp = 0;
        } while (--i);
    }
    n &= 3;
    cp = (u8 *)lp + 3;
    if (n) {
        do {
            *++cp = 0;
        } while (--n);
    }
}

static void VDevice_Init(VDevice *vdevice) {
    s32 i;

    SimThread_Init(&vdevice->sim, vdevice);
    vdevice->gain = 255;
    for (i = 0; i < 64; i++) {
        vdevice->effects[i] = NULL;
    }
    if (!g_bGammaInitialized) {
        VDevice_RecalcGammaTable();
        g_bGammaInitialized = TRUE;
    }
}

static void VDevice_RecalcGammaTable(void) {
    f32 gamma = 0.6666667f;
    s32 i;
    f32 x;
    for (i = 0; i < 129; i++) {
        if (i < 0) {
            x = (f32)pow((f32)-i / 128.0f, gamma);
            x = -(128.0f * x + 0.5f);
        } else {
            x = (f32)pow((f32)i / 128.0f, gamma);
            x = 128.0f * x + 0.5f;
        }
        g_iGammaLookup[i] = x;
    }
}

static s32 VDevice_DownloadEffect(VDevice *vdevice, u32 *id, LGForceEffect *fx) {
    s32 ret = 0;
    Effect *effect;
    u32 index;

    *id = VDevice_GetFreeEffect(vdevice);
    if (*id == -1) {
        return -5;
    }
    effect = &vdevice->effectPool[*id & 0x3F];
    index = *id & 0x3F;
    {
        switch (fx->type) {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
            Effect_Init(effect);
            effect->vdevice = vdevice;
            Effect_UpdateEffect(effect, fx);
            break;
        default:
            *id = -1;
            ret = -2;
            break;
        }
        if (effect != NULL) {
            vdevice->effects[index] = effect;
        }
    }
    return ret;
}

static s32 VDevice_UpdateEffect(VDevice *vdevice, u32 id, LGForceEffect *fx) {
    Effect *effect = vdevice->effects[id & 0x3F];
    s32 ret = -2;
    if (effect != NULL) {
        ret = Effect_UpdateEffect(effect, fx);
    }
    return ret;
}

static s32 VDevice_DestroyEffect(VDevice *vdevice, u32 id) {
    s32 ret = -2;
    if (vdevice->effects[id & 0x3F] != NULL) {
        vdevice->effects[id & 0x3F] = NULL;
        ret = 0;
    }
    return ret;
}

static s32 VDevice_StartEffect(VDevice *vdevice, u32 id) {
    Effect *effect = vdevice->effects[id & 0x3F];
    s32 ret = -2;
    if (effect != NULL) {
        ret = Effect_StartEffect(effect);
    }
    return ret;
}

static s32 VDevice_StopEffect(VDevice *vdevice, u32 id) {
    Effect *effect = vdevice->effects[id & 0x3F];
    s32 ret = -2;
    if (effect != NULL) {
        ret = Effect_StopEffect(effect);
    }
    return ret;
}

static s32 VDevice_Initialize(VDevice *vdevice, LGDevice *device) {
    s32 i;

    vdevice->gain = 255;
    for (i = 0; i < 64; i++) {
        vdevice->effects[i] = NULL;
    }
    vdevice->device = device;
    return 0;
}

static s32 VDevice_Shutdown(VDevice *vdevice) {
    s32 i;

    for (i = 0; i < 64; i++) {
        if (vdevice->effects[i] != NULL) {
            vdevice->effects[i] = NULL;
        }
    }
    return 0;
}

static u32 VDevice_GetFreeEffect(VDevice *vdevice) {
    static volatile u32 jumbleeffectid = 0;
    static volatile u32 createcount = 0;
    u32 start = jumbleeffectid;
    u32 id = -1;
    u32 i;

    for (i = 0; i < 64; i++) {
        u32 index = (i + start) % 64;
        if (vdevice->effects[index] == NULL) {
            jumbleeffectid = index + 1;
            jumbleeffectid %= 64;
            id = (vdevice->device->chan << 16) | ((createcount & 0xFF) << 8) | (index & 0x3F);
            break;
        }
    }
    return id;
}

static BOOL g_bWaveTablesInitialized = FALSE;

static void Effect_Init(Effect *effect) {
    s32 i;
    f32 x;

    zero_mem_aligned(effect, sizeof(Effect));
    if (!g_bWaveTablesInitialized) {
        for (i = 0; i < 64; i++) {
            x = (f32)sin(3.1415927f * (f32)i / 128.0f);
            g_iQuarterSineTable[i] = 1024.0f * x;
            g_iRampUpTable[i] = (i * 1024) / 63;
        }
        g_bWaveTablesInitialized = TRUE;
    }
}

#define LG_DIRECTION(d) ((((d) % 360) << 8) / 360)

static inline s32 ClampMagnitude(s32 value) {
    value = value < -255 ? -255 : value;
    return value > 255 ? 255 : value;
}

static inline s32 ClampLevel(s32 value) {
    return value > 255 ? 255 : value;
}

static inline u32 ClampPeriod(u32 value) {
    return value < 2 ? 2 : value;
}

static s32 Effect_UpdateEffect(Effect *effect, const LGForceEffect *fx) {
    s32 ret = 0;
    u8 type = fx->type;

    effect->fx.type = type;
    effect->fx.duration = fx->duration;
    effect->fx.startDelay = fx->startDelay;

    switch (type) {
    case 0:
        effect->flags |= 0x14;
        effect->fx.p.constant.magnitude = ClampMagnitude(fx->p.constant.magnitude);
        effect->fx.p.constant.direction = LG_DIRECTION(fx->p.constant.direction);
        effect->fx.p.constant.envelope.attackTime = fx->p.constant.envelope.attackTime;
        effect->fx.p.constant.envelope.fadeTime = fx->p.constant.envelope.fadeTime;
        effect->fx.p.constant.envelope.attackLevel = ClampLevel(fx->p.constant.envelope.attackLevel);
        effect->fx.p.constant.envelope.fadeLevel = ClampLevel(fx->p.constant.envelope.fadeLevel);
        break;
    case 1:
        effect->flags |= 0x24;
        effect->fx.p.ramp.startMagnitude = ClampMagnitude(fx->p.ramp.startMagnitude);
        effect->fx.p.ramp.endMagnitude = ClampMagnitude(fx->p.ramp.endMagnitude);
        effect->fx.p.ramp.direction = LG_DIRECTION(fx->p.ramp.direction);
        break;
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
        effect->flags |= 0x44;
        effect->fx.p.periodic.magnitude = ClampLevel(fx->p.periodic.magnitude);
        effect->fx.p.periodic.direction = LG_DIRECTION(fx->p.periodic.direction);
        effect->fx.p.periodic.period = ClampPeriod(fx->p.periodic.period);
        effect->fx.p.periodic.phase = LG_DIRECTION(fx->p.periodic.phase);
        effect->fx.p.periodic.offset = ClampMagnitude(fx->p.periodic.offset);
        effect->fx.p.periodic.envelope.attackTime = fx->p.periodic.envelope.attackTime;
        effect->fx.p.periodic.envelope.fadeTime = fx->p.periodic.envelope.fadeTime;
        effect->fx.p.periodic.envelope.attackLevel = ClampLevel(fx->p.periodic.envelope.attackLevel);
        effect->fx.p.periodic.envelope.fadeLevel = ClampLevel(fx->p.periodic.envelope.fadeLevel);
        if (effect->fx.p.periodic.period < 4) {
            effect->fx.p.periodic.phase = 64;
        }
        break;
    case 7:
    case 8:
        effect->flags |= 0x80;
        effect->fx.p.condition.offset = fx->p.condition.offset;
        effect->fx.p.condition.deadband = fx->p.condition.deadband;
        effect->fx.p.condition.coefficientPos = ClampMagnitude(fx->p.condition.coefficientPos);
        effect->fx.p.condition.coefficientNeg = ClampMagnitude(fx->p.condition.coefficientNeg);
        effect->fx.p.condition.saturationNeg = fx->p.condition.saturationNeg / 2;
        effect->fx.p.condition.saturationPos = fx->p.condition.saturationPos / 2;
        effect->state.condition.lower = (effect->fx.p.condition.offset - effect->fx.p.condition.deadband / 2) << 12;
        effect->state.condition.upper = (effect->fx.p.condition.offset + effect->fx.p.condition.deadband / 2) << 12;
        break;
    default:
        ret = -2;
        break;
    }
    return ret;
}

static s32 Effect_StartEffect(Effect *effect) {
    effect->flags |= 1;
    effect->flags &= ~2;
    effect->elapsed = 0;
    effect->delay = effect->fx.startDelay;
    if (effect->flags & 0x40) {
        effect->state.periodic.phase = effect->fx.p.periodic.phase << 16;
    }
    return 0;
}

static s32 Effect_StopEffect(Effect *effect) {
    effect->flags &= ~1;
    return 0;
}

static inline s32 Effect_SineWave(u32 phase) {
    if (phase < 64) {
        return g_iQuarterSineTable[phase];
    } else if (phase < 128) {
        return g_iQuarterSineTable[127 - phase];
    } else if (phase < 192) {
        return -g_iQuarterSineTable[phase - 128];
    } else {
        return -g_iQuarterSineTable[255 - phase];
    }
}

static inline s32 Effect_TriangleWave(u32 phase) {
    if (phase < 64) {
        return g_iRampUpTable[phase];
    } else if (phase < 128) {
        return g_iRampUpTable[127 - phase];
    } else if (phase < 192) {
        return -g_iRampUpTable[phase - 128];
    } else {
        return -g_iRampUpTable[255 - phase];
    }
}

static BOOL Effect_Update(Effect *effect, u32 dt) {
    BOOL active;

    if (effect->flags & 1) {
        if (effect->delay != 0) {
            effect->delay -= dt;
            if ((s32)effect->delay < 0) {
                effect->delay = 0;
            }
        } else {
            effect->elapsed += dt;
            if (effect->fx.duration != -1 && effect->fx.duration < effect->elapsed && (effect->flags & 2)) {
                effect->flags &= ~1;
            }
        }
    }
    effect->flags |= 2;

    active = FALSE;
    if ((effect->flags & 1) && effect->delay == 0) {
        active = TRUE;
    }

    if (active) {
        if (effect->flags & 0x10) {
            Effect_PolarToRect(Effect_PerformEnvelope(effect, &effect->fx.p.constant.envelope, effect->fx.p.constant.magnitude),
                               effect->fx.p.constant.direction, &effect->force);
        } else if (effect->flags & 0x20) {
            Effect_PolarToRect(effect->fx.p.ramp.startMagnitude +
                                   MulDivLowCost(effect->fx.p.ramp.endMagnitude - effect->fx.p.ramp.startMagnitude, effect->elapsed,
                                          effect->fx.duration),
                               effect->fx.p.ramp.direction, &effect->force);
        } else if (effect->flags & 0x40) {
            s16 magnitude;
            s32 wave;

            effect->state.periodic.phase += (u32)(dt << 24) / effect->fx.p.periodic.period;
            if (effect->state.periodic.phase >= 0x1000000) {
                effect->state.periodic.phase -= effect->state.periodic.phase & 0xFF000000;
            }
            magnitude = Effect_PerformEnvelope(effect, &effect->fx.p.periodic.envelope, effect->fx.p.periodic.magnitude);
            switch (effect->fx.type) {
            case 2:
                wave = Effect_SineWave((effect->state.periodic.phase >> 16) & 0xFF);
                break;
            case 3:
                wave = (effect->state.periodic.phase >> 16) < 128 ? 1024 : -1024;
                break;
            case 4:
                wave = Effect_TriangleWave((effect->state.periodic.phase >> 16) & 0xFF);
                break;
            case 5:
                wave = g_iRampUpTable[((effect->state.periodic.phase >> 16) & 0xFF) >> 2];
                break;
            case 6:
                wave = g_iRampUpTable[(255 - ((effect->state.periodic.phase >> 16) & 0xFF)) >> 2];
                break;
            default:
                wave = 0;
                break;
            }
            wave = MulDivLowCost(wave, magnitude, 1024);
            Effect_PolarToRect(wave + effect->fx.p.periodic.offset, effect->fx.p.periodic.direction, &effect->force);
        } else if (effect->flags & 0x80) {
            void *condition = &effect->fx.p;
            switch (effect->fx.type) {
            case 7:
                effect->force = Effect_UpdateSpring(effect, condition);
                break;
            case 8:
                effect->force = Effect_UpdateDamper(effect, condition);
                if (effect->force == 0) {
                    effect->force++;
                }
                break;
            default:
                effect->force = 0;
                break;
            }
        } else {
            return FALSE;
        }
    } else {
        effect->force = 0;
    }
    return TRUE;
}

static s32 Effect_PerformEnvelope(Effect *effect, LGEnvelope *envelope, s32 magnitude) {
    BOOL negative = magnitude < 0;
    s32 absMagnitude = __abs(magnitude);
    s32 result = absMagnitude;

    if (effect->elapsed < envelope->attackTime) {
        result = envelope->attackLevel + MulDivLowCost(effect->elapsed, absMagnitude - envelope->attackLevel, envelope->attackTime);
    } else if (effect->fx.duration != -1 && effect->elapsed > effect->fx.duration - envelope->fadeTime) {
        if (effect->elapsed < effect->fx.duration) {
            result = absMagnitude + MulDivLowCost(effect->elapsed - (effect->fx.duration - envelope->fadeTime),
                                           envelope->fadeLevel - absMagnitude, envelope->fadeTime);
        } else {
            result = envelope->fadeLevel;
        }
    }
    if (negative) {
        result = -result;
    }
    return result;
}

static void Effect_PolarToRect(s32 magnitude, u32 direction, s32 *out) {
    s32 sine = Effect_SineWave(direction & 0xFF);
    *out = -MulDivLowCost(magnitude * sine, 127, 255 * 1024);
}

static s32 Effect_DoSpring(s32 delta, s32 coef, s32 sat) {
    delta = delta * coef / 0x20000;
    if (-sat > delta) {
        delta = -sat;
    }
    if (sat < delta) {
        delta = sat;
    }
    return -delta;
}

static s32 Effect_UpdateSpring(Effect *effect, const void *param) {
    s32 position = effect->vdevice->sim.positionFixed;
    const LGCondition *condition = param;

    if (position < effect->state.condition.lower) {
        return Effect_DoSpring(position - effect->state.condition.lower, condition->coefficientNeg, condition->saturationNeg);
    } else if (position > effect->state.condition.upper) {
        return Effect_DoSpring(position - effect->state.condition.upper, condition->coefficientPos, condition->saturationPos);
    }
    return 0;
}

static inline s32 Effect_DamperForce(s32 velocity, s32 coef, s32 sat) {
    velocity *= coef;
    velocity = velocity * 20 / 0xFF000;
    if (-sat > velocity) {
        velocity = -sat;
    }
    if (sat < velocity) {
        velocity = sat;
    }
    return velocity;
}

static s32 Effect_UpdateDamper(Effect *effect, const void *param) {
    s32 velocity = effect->vdevice->sim.velocity;
    const LGCondition *condition = param;
    s32 coefficient;
    s32 saturation;

    if (velocity < 0) {
        coefficient = -condition->coefficientNeg;
        saturation = condition->saturationNeg;
    } else {
        coefficient = -condition->coefficientPos;
        saturation = condition->saturationPos;
    }
    return Effect_DamperForce(velocity, coefficient, saturation);
}

static inline void SimThread_SetFilter(SimThread *sim, s32 *coefs) {
    s32 i;
    for (i = 0; i < 10; i++) {
        sim->filterCoef[i] = coefs[i];
        sim->filterHistory[i] = 0;
    }
    sim->filterIndex = 0;
}

static void SimThread_Init(SimThread *sim, VDevice *vdevice) {
    static f32 a[10] = {
        0.09954692423343658f, 0.09983780235052109f, 0.10006450861692429f, 0.10022669285535812f, 0.1003241091966629f,
        0.1003241091966629f, 0.10022669285535812f, 0.10006450861692429f, 0.09983780235052109f, 0.09954692423343658f,
    };
    static s32 ia[10];
    s32 unity[1] = { 0x1000 };
    s32 i;

    sim->lastTime = 0;
    sim->filterIndex = 0;
    sim->ringIndex = 0;
    for (i = 0; i < 16; i++) {
        sim->deltaPos[i] = 0;
        sim->deltaTime[i] = 0;
    }
    sim->vdevice = vdevice;
    sim->lastForce = 0;
    sim->positionFixed = 0;
    sim->position = 0;
    sim->velocity = 0;
    sim->lastTime = OSTicksToMilliseconds(OSGetTick());
    SimThread_SetFilter(sim, unity);
    for (i = 0; i < 10; i++) {
        ia[i] = 4096.0f * a[i];
    }
    SimThread_SetFilter(sim, ia);
}

static inline BOOL Effect_GetForce(Effect *effect, s32 *force) {
    if ((effect->flags & 1) && effect->delay == 0) {
        s32 f = effect->force;
        f = f < -127 ? -127 : f;
        *force = f > 127 ? 127 : f;
        return effect->flags & 4;
    }
    *force = 0;
    return FALSE;
}

static s32 SimThread_Step(SimThread *sim) {
    u32 dt;
    s32 directional = 0;
    s32 conditional = 0;
    u32 now;
    s32 oldPosition;
    s32 deltaPos;
    s32 sumPos;
    u32 sumTime;
    s32 index;
    s32 i;
    s32 velocity;
    Effect *effect;
    u32 j;
    s32 gain;
    s32 force;
    s32 total;

    now = OSTicksToMilliseconds(OSGetTick());
    dt = now - sim->lastTime;
    sim->lastTime = now;

    oldPosition = sim->position;
    sim->position = sim->vdevice->device->status.steerDirection;
    sim->positionFixed = sim->position << 12;
    deltaPos = sim->position - oldPosition;

    sim->ringIndex--;
    if (sim->ringIndex < 0) {
        sim->ringIndex = 15;
    }
    sim->deltaPos[sim->ringIndex] = deltaPos;
    sim->deltaTime[sim->ringIndex] = dt;

    sumPos = 0;
    sumTime = 0;
    index = sim->ringIndex;
    for (i = 0; i < 16 && sumTime < 16; i++) {
        sumPos += sim->deltaPos[index];
        sumTime += sim->deltaTime[index];
        index++;
        if (index == 16) {
            index = 0;
        }
    }
    if (sumTime > 16) {
        if (--index < 0) {
            index = 15;
        }
        sumPos -= MulDivLowCost(sumTime - 16, sim->deltaPos[index], sim->deltaTime[index]);
    }

    sim->filterHistory[sim->filterIndex++] = sumPos;
    if (sim->filterIndex > 9) {
        sim->filterIndex = 0;
    }

    velocity = 0;
    for (i = 9; i >= 0; i--) {
        velocity += sim->filterHistory[sim->filterIndex++] * sim->filterCoef[i];
        if (sim->filterIndex > 9) {
            sim->filterIndex = 0;
        }
    }
    sim->velocity = velocity;

    for (j = 0; j < 64; j++) {
        effect = sim->vdevice->effects[j];
        if (effect != NULL && (effect->flags & 1)) {
            Effect_Update(effect, dt);
            if (Effect_GetForce(effect, &force)) {
                directional += force;
            } else {
                conditional += force;
            }
        }
    }

    gain = sim->vdevice->gain;
    directional = directional > 128 ? 128 : directional;
    directional = directional < -128 ? -128 : directional;
    if (directional < 0) {
        directional = -g_iGammaLookup[-directional];
    } else {
        directional = g_iGammaLookup[directional];
    }
    if (gain < 255) {
        directional = MulDivLowCost(directional, gain, 255);
        conditional = MulDivLowCost(conditional, gain, 255);
    }

    total = directional + conditional;
    total = total > 128 ? 128 : total;
    total = total < -128 ? -128 : total;
    if (sim->lastForce != total) {
        sim->lastForce = total;
        if (total == 0) {
            SIControlSteering(sim->vdevice->device->chan, 0x400, 0);
        } else {
            SIControlSteering(sim->vdevice->device->chan, 0x600, total);
        }
    }
    return total;
}
