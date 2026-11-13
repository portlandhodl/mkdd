/* Host stand-ins for the GameCube's memory-mapped graphics registers, shared by
 * the GX library (libs/dolphin/gx, writes/reads them) and the emulated GPU
 * (port/gpu, updates status/token registers and raises interrupts). */
#ifndef PORT_HW_H
#define PORT_HW_H

#include <dolphin/types.h>

#ifdef __cplusplus
extern "C" {
#endif

extern u16 port_hw_cp[0x80];  /* command processor, 0xCC000000 */
extern u16 port_hw_pe[0x100]; /* pixel engine, 0xCC001000 */
extern u32 port_hw_pi[0x100]; /* processor interface, 0xCC003000 */
extern u16 port_hw_mem[0x80]; /* memory interface, 0xCC004000 */

/* Raise an OS interrupt (__OS_INTERRUPT_*); its handler runs from port_poll(),
 * as hardware interrupts run between instructions. */
void port_raise_interrupt(int interrupt);

#ifdef __cplusplus
}
#endif

#endif
