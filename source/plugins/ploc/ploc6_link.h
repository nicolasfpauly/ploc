/* GUI/controller protocol for the 0.6 physical mapping. Preset ABI stays 0.4. */
#ifndef PLOC6_LINK_H
#define PLOC6_LINK_H
#include "ploc5_link.h"
#define PLOC6_TOKEN 0x40
static inline int p6_linked(const int16_t *p) {
    if(!p||!ploc4_is(p))return 0;
    int n=p[4]-OP1_ENGINE_MAGIC;
    return n>=1&&n<=OP1_MAX_ORDER&&op1_eng_page[2]==(uint8_t)(PLOC6_TOKEN|n);
}
static inline int p6_clamp(int x){return x<0?0:(x>32767?32767:x);}
static inline int p6_percent(int x){return (p6_clamp(x)*99+16383)/32767;}
static inline int p6_quality_light(int x){return 51+(p6_clamp(x)*204+16383)/32767;}
static inline int p6_impact_light(int x){return 51+(x*204+7)/15;}
static inline int p6_spray_count(int x){return (x*7+7)/15;}
#endif
