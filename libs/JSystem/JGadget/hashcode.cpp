#include "JSystem/JGadget/hashcode.h"

u32 JGadget::getHashCode(const char *, u32) 
{
    // UNUSED
DECOMP_HOST(, return 0;) }

u32 JGadget::getHashCode(const char *data)
{
    return getHashCode_<char>(data, TPRIsEnd_value_<char>());
}