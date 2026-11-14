/* Node file I/O for the netcode spike (netkart.cpp records/replays kart state streams). */
#include <emscripten.h>

EM_JS(int, net_js_open, (const char *path, int write), {
    try {
        var fs = require('fs');
        Module.netFd = fs.openSync(UTF8ToString(path), write ? 'w' : 'r');
        return 1;
    } catch (e) { return 0; }
});
EM_JS(void, net_js_write, (const void *p, int n), {
    require('fs').writeSync(Module.netFd, HEAPU8.subarray(p, p + n));
});
EM_JS(int, net_js_read, (void *p, int n), {
    var b = Buffer.alloc(n);
    var got = require('fs').readSync(Module.netFd, b, 0, n, null);
    HEAPU8.set(b.subarray(0, got), p);
    return got;
});
EM_JS(void, net_js_close, (void), { require('fs').closeSync(Module.netFd); });
