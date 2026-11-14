/* Online play: JS bridge to the WebRTC transport (Module.netBridge, set up by
 * tools/web_wasm/web/net.js Mesh.attachModule). See net_bridge.h. */
#include <emscripten.h>
#include "net_bridge.h"

EM_JS(int, net_bridge_js_info, (int what, int arg), {
    var b = Module.netBridge;
    if (!b) return what === 4 ? -1 : 0;
    var m = b.mesh;
    switch (what) {
    case 0: return m.session ? 1 : 0;
    case 1: return m.lobby.id | 0;
    case 2: return m.hostId | 0;
    case 3: { var mem = b.member(arg); return mem && mem.slot !== undefined && mem.slot !== null ? mem.slot : -1; }
    case 4: { if (arg === m.lobby.id) return 0; var p = m.peers.get(arg); return p && p.rtt ? Math.round(p.rtt) : -1; }
    case 5: return m.clockSynced() ? 1 : 0;
    }
    return 0;
});
EM_JS(int, net_bridge_js_members, (int *ids, int max), {
    var b = Module.netBridge;
    if (!b) return 0;
    var m = b.mesh, list;
    if (m.session) list = m.session.members.map(function(x) { return x.id; });
    else if (m.lobby.room) list = m.lobby.room.members.map(function(x) { return x.id; });
    else list = [];
    for (var i = 0; i < list.length && i < max; i++) HEAP32[(ids >> 2) + i] = list[i];
    return list.length;
});
EM_JS(double, net_bridge_js_host_time, (void), {
    return Module.netBridge ? Module.netBridge.mesh.hostTime() : performance.now();
});
EM_JS(int, net_bridge_js_send, (int to, int kind, const void *data, int len), {
    if (!Module.netBridge) return 0;
    return Module.netBridge.send(to, kind, HEAPU8.slice(data, data + len));
});
EM_JS(int, net_bridge_js_recv, (int *from, int *kind, void *buf, int cap), {
    var b = Module.netBridge;
    if (!b) return 0;
    var q = b.peek();
    if (!q) return 0;
    if (q.bytes.length > cap) return -q.bytes.length;
    b.recv();
    HEAPU8.set(q.bytes, buf);
    if (from) HEAP32[from >> 2] = q.from;
    if (kind) HEAP32[kind >> 2] = q.kind;
    return q.bytes.length;
});
EM_JS(int, net_bridge_js_session, (char *buf, int cap), {
    var b = Module.netBridge;
    if (!b || !b.mesh.session) return 0;
    var s = b.sessionJson(), n = lengthBytesUTF8(s);
    if (n + 1 > cap) return -(n + 1);
    stringToUTF8(s, buf, cap);
    return n;
});

EMSCRIPTEN_KEEPALIVE int port_net_active(void) { return net_bridge_js_info(0, 0); }
EMSCRIPTEN_KEEPALIVE int port_net_self(void) { return net_bridge_js_info(1, 0); }
EMSCRIPTEN_KEEPALIVE int port_net_host(void) { return net_bridge_js_info(2, 0); }
EMSCRIPTEN_KEEPALIVE int port_net_slot(int member) { return net_bridge_js_info(3, member); }
EMSCRIPTEN_KEEPALIVE int port_net_rtt(int member) { return net_bridge_js_info(4, member); }
EMSCRIPTEN_KEEPALIVE int port_net_clock_synced(void) { return net_bridge_js_info(5, 0); }
EMSCRIPTEN_KEEPALIVE int port_net_members(int *ids, int max) { return net_bridge_js_members(ids, max); }
EMSCRIPTEN_KEEPALIVE double port_net_host_time(void) { return net_bridge_js_host_time(); }

EMSCRIPTEN_KEEPALIVE int port_net_send(int to, int kind, const void *data, int len)
{
    if (!data || len <= 0 || ((const unsigned char *)data)[0] >= 0xF0 ||
        (kind != PORT_NET_UNRELIABLE && kind != PORT_NET_RELIABLE))
        return -1;
    return net_bridge_js_send(to, kind, data, len);
}

EMSCRIPTEN_KEEPALIVE int port_net_recv(int *from, int *kind, void *buf, int cap)
{
    return net_bridge_js_recv(from, kind, buf, cap);
}

EMSCRIPTEN_KEEPALIVE int port_net_session_json(char *buf, int cap) { return net_bridge_js_session(buf, cap); }
