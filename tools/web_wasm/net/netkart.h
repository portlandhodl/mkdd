// Netcode (NETCODE.md): kart state wire format and remote-kart helpers shared by the spike harness
// (netkart.cpp, MKDD_NET=rec|play|mirror) and online play (netonline.cpp, MKDD_NET_ONLINE=1).
#ifndef MKDD_NETKART_H
#define MKDD_NETKART_H

#include "types.h"

class KartBody;
class KartGamePad;

// The decomp's MSL headers shadow the host libc here, so math is builtins.
#define sqrtf __builtin_sqrtf
#define fabsf __builtin_fabsf
#define lrintf __builtin_lrintf

extern "C" int port_env(const char *name);
extern "C" char *gpu_js_env_str(const char *name);
extern "C" void free(void *);

namespace netkart {

enum { WIRE = 56, MSG_KART_STATE = 1 };

struct State {
    s32 frame;     // sender frame (spike: race frame; online: shared clock frame, see netonline.cpp)
    f32 pos[3];
    f32 rot[3][3]; // KartBody::_110 rotation part
    f32 vel[3], effVel[3], wg[3];
    s8 sx, sy;     // stick, -127..127
    u16 buttons;   // JUTGamePad bits
    u8 aL, aR;
    u64 carStatus;
    u16 boost, mtBoost;
    u8 mtState, driver;
    bool valid;
};

// 10.1 kart state: type 1, u8 slot, u16 frame (low 16 bits), ... (56 bytes, little endian)
void encode(const State &s, int slot, u8 *p);
// fills everything but the frame; returns the 16-bit frame field
u16 decode(const u8 *p, State &s);
void capture(int kart, s32 frame, State &s);
void setPose(KartBody *b, const f32 pos[3], const f32 rot[3][3]);
void snapTo(KartBody *b, const State &s);
void nlerpRot(const f32 a[3][3], const f32 b[3][3], f32 t, f32 out[3][3]);
// remote pad = the newest input; item buttons masked (items arrive as events, NETCODE.md 5.3)
void feedPad(int kart, const State &s, u16 prevButtons);
void makeRemote(KartBody *b);
f32 dist(const f32 a[3], const f32 b[3]);
// numeric environment setting (port_env() is a flag reader that maps "0" to 1)
int envInt(const char *name, int def);

} // namespace netkart

// online play (netonline.cpp)
namespace netonline {
bool enabled();
void preDynamics();
void postStage1();
} // namespace netonline

#endif
