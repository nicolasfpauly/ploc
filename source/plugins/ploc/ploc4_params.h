/* PLOC 0.4 parameter ABI. GPL-3.0-or-later, see NOTICE.
 * Four full-resolution knobs + ten 4-bit macros in 40 of the 41 free bits.
 * Word 4 remains the core's engine selector. Words 5..7 stay nonnegative.
 */
#ifndef PLOC4_PARAMS_H
#define PLOC4_PARAMS_H
#include <stdint.h>
#define PLOC4_TAG 0x6000
#define PLOC4_MASK 0x7800
enum { P4_POUIC,P4_IMPACT,P4_TAIL,P4_SPRAY,P4_SPREAD,P4_CHAOS,
       P4_TONE,P4_DISTANCE,P4_WIDTH,P4_TUNE,P4_EXTRAS };
static inline int ploc4_is(const int16_t *p) { return (p[5]&PLOC4_MASK)==PLOC4_TAG; }
static inline unsigned ploc4_get(const int16_t *p,int k) {
    if((unsigned)k>=P4_EXTRAS)return 0;
    unsigned bit=(unsigned)k*4u,w=5,offset=bit;
    if(bit>=26){w=7;offset=bit-26;}else if(bit>=11){w=6;offset=bit-11;}
    unsigned width=w==5?11:15,v=((unsigned)(uint16_t)p[w]>>offset);
    if(offset+4>width)v|=(unsigned)(uint16_t)p[w+1]<<(width-offset);
    return v&15u;
}
static inline void ploc4_set(int16_t *p,int k,unsigned v) {
    if((unsigned)k>=P4_EXTRAS)return;
    unsigned bit=(unsigned)k*4u,w=5,offset=bit;
    if(bit>=26){w=7;offset=bit-26;}else if(bit>=11){w=6;offset=bit-11;}
    unsigned width=w==5?11:15,mask=(15u<<offset)&((1u<<width)-1);
    p[w]=(int16_t)(((unsigned)(uint16_t)p[w]&~mask)|((v<<offset)&mask));
    if(offset+4>width){unsigned n=offset+4-width,m=(1u<<n)-1;
        p[w+1]=(int16_t)(((unsigned)(uint16_t)p[w+1]&~m)|((v>>(width-offset))&m));}
    p[5]=(int16_t)((p[5]&~PLOC4_MASK)|PLOC4_TAG);
}
static inline void ploc4_defaults(int16_t *p) {
    static const uint8_t d[P4_EXTRAS]={3,4,5,2,9,10,9,1,12,0};
    p[5]=PLOC4_TAG;p[6]=p[7]=0;
    for(int k=0;k<P4_EXTRAS;k++)ploc4_set(p,k,d[k]);
}
#endif
