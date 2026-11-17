/* Versioned GUI/controller handshake; canonical preset words stay unchanged. */
#ifndef PLOC5_LINK_H
#define PLOC5_LINK_H
#include "ploc4_link.h"
#define PLOC5_TOKEN 0xC0
static inline int p5_linked(const int16_t *p) {
    if(!p||!ploc4_is(p))return 0;
    int n=p[4]-OP1_ENGINE_MAGIC;
    return n>=1&&n<=OP1_MAX_ORDER&&op1_eng_page[2]==(uint8_t)(PLOC5_TOKEN|n);
}
#ifndef PLOC5_LINK_HOST
/* R246 Shift-held byte, also used by SDK 3.1.4 kn_core.c. */
static inline int p5_shift_held(void){return *(volatile uint8_t *)0x01DD7F78!=0;}
#endif
#endif
