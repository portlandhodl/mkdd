/* Game <-> page event bus (see net_bridge.h). The page side is Module.netBus (web/netbus.js); without
 * it (node, a page without netbus.js) events are dropped and the lobby state is "offline". */
#include <emscripten.h>
#include "net_bridge.h"

EM_JS(void, netbus_js_emit, (const char *type, const char *json), {
    var t = UTF8ToString(type), d = {};
    if (json) { try { d = JSON.parse(UTF8ToString(json)); } catch (e) { d = { raw: UTF8ToString(json) }; } }
    if (Module.netBus) Module.netBus.fromGame(t, d);
});
EM_JS(int, netbus_js_poll, (char *type, int tcap, char *json, int jcap), {
    var b = Module.netBus;
    if (!b || !b.toGame.length) return 0;
    var e = b.toGame[0], s = JSON.stringify(e.data || {}), n = lengthBytesUTF8(s);
    if (n + 1 > jcap) return -(n + 1);
    b.toGame.shift();
    stringToUTF8(e.type, type, tcap);
    stringToUTF8(s, json, jcap);
    return n;
});
EM_JS(int, netbus_js_lobby, (char *buf, int cap), {
    var b = Module.netBus;
    var s = JSON.stringify(b ? b.lobbyState() : { state: 'offline' }), n = lengthBytesUTF8(s);
    if (n + 1 > cap) return -(n + 1);
    stringToUTF8(s, buf, cap);
    return n;
});

EMSCRIPTEN_KEEPALIVE void port_net_emit(const char *type, const char *json) { netbus_js_emit(type, json); }
EMSCRIPTEN_KEEPALIVE int port_net_poll_event(char *type, int tcap, char *json, int jcap)
{
    return netbus_js_poll(type, tcap, json, jcap);
}
EMSCRIPTEN_KEEPALIVE int port_net_lobby_json(char *buf, int cap) { return netbus_js_lobby(buf, cap); }
