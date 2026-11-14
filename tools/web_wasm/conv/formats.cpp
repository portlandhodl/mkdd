// Extension -> converter table. Add a row per format walker.
#include "conv.h"

namespace conv {

struct FormatEntry {
    const char *ext;
    Converter fn;
};

extern const FormatEntry kFormats[] = {
    {"bfn", convBFN},
    {"bmd", convJ3DModel}, {"bdl", convJ3DModel},
    {"bca", convJ3DAnim}, {"bck", convJ3DAnim}, {"btk", convJ3DAnim}, {"brk", convJ3DAnim},
    {"btp", convJ3DAnim}, {"bpk", convJ3DAnim}, {"bla", convJ3DAnim}, {"blk", convJ3DAnim},
    {"bls", convJ3DAnim}, {"bva", convJ3DAnim},
    {"bas", convBAS},
    {"blo", convBLO}, {"bti", convBTI}, {"bmg", convBMG}, {"bmc", convBMC},
    {"bol", convBOL}, {"bco", convBCO},
    {"set", convGameParam}, {"ikp", convGameParam}, {"lbd", convGameParam}, {"kel", convGameParam},
    {"pel", convGameParam}, {"iop", convGameParam}, {"dat", convGameParam}, {"snp", convGameParam},
    {"ght", convGameParam},
    {"jpc", convJPA}, {"jpa", convJPA},
    {"baa", convAudio}, {"bsft", convAudio}, {"bstn", convAudio}, {"bms", convAudio},
};
extern const unsigned kNumFormats = sizeof kFormats / sizeof kFormats[0];

} // namespace conv
