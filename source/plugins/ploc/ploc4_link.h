/* Read-only identification of the paired synth, shared by editor and FX. */
#ifndef PLOC4_LINK_H
#define PLOC4_LINK_H
#include "op1_plugin.h"
#include "ploc4_params.h"
#ifndef PLOC4_LINK_HOST
static inline int p4_slot(void){return *(volatile int *)0xFF8016D4&7;}
static inline int p4_drum_mode(void){return *(volatile int *)0x01DE4C40==1;}
#endif
static inline int p4_linked(const int16_t *p) {
    if(!p||!ploc4_is(p))return 0;
    int n=p[4]-OP1_ENGINE_MAGIC;if(n<1||n>OP1_MAX_ORDER)return 0;
    /* The synth screen publishes its selector in a transient core page flag.
     * The core clears these flags on a slot/selector change. The match thus
     * identifies the displayed PLOC without private registry descriptors. */
    return op1_eng_page[2]==(uint8_t)(0x80|n);
}
#endif
