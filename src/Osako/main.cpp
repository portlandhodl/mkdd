#include "Osako/system.h"

#ifdef __MWERKS__
void main(int argc, char **argv)
#else
int main(int argc, char **argv)
#endif
{
    System::init();
    System::run();
}