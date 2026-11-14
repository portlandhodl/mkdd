/* Online play (netonline.cpp): debug and overlay exports to the page.
 * Module.netStats: the latest once-a-second stats object (karts, positions, replica error, link);
 * Module.netStatsLog: all of them (the last hour); Module.netPhase: loading / waiting / countdown / race /
 * finished. */
#include <emscripten.h>

EM_JS(void, netonline_js_stats, (const char *json), {
    var s;
    try { s = JSON.parse(UTF8ToString(json)); } catch (e) { return; }
    Module.netStats = s;
    if (!Module.netStatsLog) Module.netStatsLog = [];
    Module.netStatsLog.push(s);
    if (Module.netStatsLog.length > 3600) Module.netStatsLog.shift();
});
EM_JS(void, netonline_js_phase, (const char *phase), { Module.netPhase = UTF8ToString(phase); });

int netonline_race_frames(void);
/* tests drive scripted input by race frames: Module._port_net_debug_frame() */
EMSCRIPTEN_KEEPALIVE int port_net_debug_frame(void) { return netonline_race_frames(); }

int netonline_local_kart(float *out);
/* tests steer by the local kart's pose (12 floats): position, forward axis, a course point ahead, X axis */
EMSCRIPTEN_KEEPALIVE int port_net_debug_kart(float *out) { return netonline_local_kart(out); }
