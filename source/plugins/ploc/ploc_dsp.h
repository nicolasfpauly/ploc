/* PLOC 0.2: physically informed bubbles, integer port of the listening studies.
 * GPL-3.0-or-later. Firmware permission: see NOTICE.
 * d(f)=.043*f+.0014*f^1.5 (van den Doel, 2005); xi=.10/.08/.05.
 * Tail stretch, secondary bubbles and SIZE transpose are musical controls,
 * not a calibrated prediction of a particular drop or vessel.
 */
#ifndef PLOC_DSP_H
#define PLOC_DSP_H
#include <stdint.h>
#include "ploc_tables.h"
#include "ploc_legacy_dsp.h"
#define PLOC_BUBBLE_VERSION 2
#define PLOC_PRIMARY_ONSET 265

typedef struct {
    uint32_t phase[3];
    int32_t amp[3];
    uint32_t age;
    int32_t lp;
} ploc_state;


static inline uint32_t ploc_sqrt(uint32_t x) {
    uint32_t r=0, bit=1u<<30;
    while(bit>x) bit>>=2;
    while(bit) {
        if(x>=r+bit) { x-=r+bit; r=(r>>1)+bit; } else r>>=1;
        bit>>=2;
    }
    return r;
}
/* Positive phase increment times Q15, without 64-bit hot-loop math. */
static inline uint32_t ploc_mul(uint32_t inc,uint32_t q15) {
    return (inc>>15)*q15+(((inc&32767u)*q15)>>15);
}
static inline uint32_t ploc_base(uint32_t pitch,int size) {
    if(pitch>0x60000000u) pitch=0x60000000u;
    uint32_t b=ploc_mul(pitch,49152u-(uint32_t)size);
    if(b<2921746u) b=2921746u; /* 30 Hz floor */
    return b>0x30000000u?0x30000000u:b; /* 8.27 kHz primary, before chirp */
}
/* TAIL: 0.5 .. 8 times physical lifetime, log-spaced and interpolated. */
static inline uint32_t ploc_tail_scale(int tail) {
    static const uint32_t scale[17]={65536,55109,46341,38968,32768,27554,23170,19484,16384,13777,11585,9742,8192,6889,5793,4871,4096};
    int j=tail>>11, f=tail&2047;
    return scale[j]-(((scale[j]-scale[j+1])*(uint32_t)f)>>11);
}
/* d/sample_rate in Q20. One integer square root per bubble per block.
 * A floor caps very low notes' lifetime. No per-sample divisions or floats. */
static inline uint32_t ploc_decay(uint32_t inc,uint32_t scale) {
    uint32_t hz=(((inc>>16)*44100u)+32768u)>>16;
    if(hz<30) hz=30;
    uint32_t root=ploc_sqrt(hz<<16);
    uint32_t d256=11*hz+((((hz*root)>>6)*23)>>8);
    uint32_t d20=(d256*95+512)>>10;
    uint32_t d=(d20*(scale>>4))>>11;
    return d<32?32:d;
}
/* xi*d*t in Q15. Integration sums instantaneous frequency, not f(t)*t. */
static inline uint32_t ploc_rise(uint32_t decay,uint32_t xi,uint32_t elapsed) {
    uint32_t step=(decay*xi)>>8, e=elapsed>>2;
    uint32_t rise=e*(step>>10)+((e*(step&1023u))>>10);
    return rise>16384?16384:rise;
}
static inline void ploc_start(ploc_state *s) {
    for(int j=0;j<3;j++) { s->phase[j]=0; s->amp[j]=1<<24; }
    s->age=0; s->lp=0;
}
static inline void ploc_process(ploc_state *s,int16_t *out,const int32_t *pitch,
 const int16_t *p,int32_t velocity,const int16_t *sine,int count) {
    static const uint32_t ratio[3]={32768,59965,88801};
    static const uint32_t onset[3]={PLOC_PRIMARY_ONSET,882,1588}; /* 6, 20, 36 ms */
    static const uint32_t xi[3]={3277,2621,1638};
    uint32_t b0=ploc_base((uint32_t)pitch[0],p[0]), damp[3], aa[3];
    uint32_t stretch=ploc_tail_scale(p[2]);
    for(int j=0;j<3;j++) {
        uint32_t b=ploc_mul(b0,ratio[j]);
        aa[j]=b<=0x40000000u?32767:(b>=0x60000000u?0:(0x60000000u-b)>>14);
        if(b>0x60000000u) b=0x60000000u;
        damp[j]=ploc_decay(b,stretch);
    }
    int32_t mix[3]={32767,(p[1]*7864)>>15,(p[1]*3604)>>15};
    int32_t tone=1024+((p[3]*15359)>>15); /* lowpass alpha Q14 */
    for(int n=0;n<count;n++) {
        uint32_t base=ploc_base((uint32_t)pitch[n],p[0]);
        int32_t wave=0;
        for(int j=0;j<3;j++) {
            if(s->age<onset[j]||s->amp[j]==0) continue;
            uint32_t elapsed=s->age-onset[j];
            uint32_t b=ploc_mul(base,ratio[j]);
            if(b>0x60000000u) b=0x60000000u;
            uint32_t inc=ploc_mul(b,32768+ploc_rise(damp[j],xi[j],elapsed));
            if(inc>0x70000000u) inc=0x70000000u;
            int32_t a=elapsed<128?ploc_attack[elapsed]:32767;
            int32_t y=(sine[s->phase[j]>>19]*a)>>15;
            s->phase[j]+=inc;
            y=(y*(s->amp[j]>>10))>>14;
            y=(y*mix[j])>>15;
            wave+=(y*(int32_t)aa[j])>>15;
            s->amp[j]-=(int32_t)(((uint32_t)(s->amp[j]>>10)*damp[j])>>10);
            if(s->amp[j]<32768) s->amp[j]=0;
        }
        if(s->age<353) {
            uint32_t z=(s->age+1)*0x9e3779b9u;
            z^=z>>16; z*=0x7feb352du; z^=z>>15;
            int32_t hit=(((int32_t)(z>>16)-32768)*ploc_impact[s->age])>>15;
            hit=(hit*p[1])>>15;
            wave+=(hit*p[3])>>15;
        }
        if(s->amp[0]==0&&s->amp[1]==0&&s->amp[2]==0) s->lp=0;
        else s->lp+=((wave-s->lp)*tone)>>14;
        int32_t sample=(s->lp*velocity)>>15;
        out[n]=(int16_t)((sample*14564)>>15); /* 4/9; +12.04 dB versus 0.2.0, before native envelope/mixer */
        if(s->age<264600) s->age++;
    }
}
#include "ploc3_dsp.h"
typedef union { ploc3_state current; ploc_state bubble; ploc_legacy_state legacy; } ploc_model_state;
/* Model tag is saved in word 5. Old sounds retain their parameter laws. */
static inline void ploc_model_start_seed(ploc_model_state *s,const int16_t *p,unsigned seed) {
    if(p[5]==PLOC_CURRENT_MODEL)ploc3_start(&s->current,seed,ploc_drops(p[3]));
    else if(p[5]==PLOC_BUBBLE_VERSION)ploc_start(&s->bubble);
    else ploc_legacy_start(&s->legacy);
}
static inline void ploc_model_start(ploc_model_state *s,const int16_t *p) {
    ploc_model_start_seed(s,p,0);
}
static inline void ploc_model_process(ploc_model_state *s,int16_t *out,const int32_t *pitch,
 const int16_t *p,int32_t velocity,const int16_t *sine,int count) {
    if(p[5]==PLOC_CURRENT_MODEL)ploc3_process(&s->current,out,pitch,p,velocity,sine,count);
    else if(p[5]==PLOC_BUBBLE_VERSION)ploc_process(&s->bubble,out,pitch,p,velocity,sine,count);
    else ploc_legacy_process(&s->legacy,out,pitch,p,velocity,sine,count);
}
#endif
