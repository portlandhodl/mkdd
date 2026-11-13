// C entry point for the port's DVD layer.
#include "conv.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

extern "C" int port_conv_wants(const char *name) { return conv::wants(name) ? 1 : 0; }

// Convert a whole disc file to host order. Returns a malloc'd buffer (caller frees).
extern "C" unsigned char *port_conv_file(const char *name, const unsigned char *in, unsigned inSize, unsigned *outSize)
{
    conv::Vec<conv::u8> data(in, in + inSize);
    conv::Report rep;
    conv::convertFile(name, data, true, rep);
    for (auto &e : rep.errors)
        fprintf(stderr, "[conv] error: %s\n", e.c_str());
    unsigned char *out = (unsigned char *)malloc(data.size() ? data.size() : 1);
    memcpy(out, data.data(), data.size());
    *outSize = (unsigned)data.size();
    return out;
}
