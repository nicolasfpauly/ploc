/* PLOC 0.3, SIZE / RIPPLE / VISC / DROPS. Included after the shared
 * fixed-point helpers in ploc_dsp.h. GPL-3.0-or-later; see NOTICE.
 * Six independent bubbles, two quieter artistic surface/body modes.
 * Full phase for bubbles, 16-bit phase for the two body modes. Envelopes
 * are analytic lookup curves: total voice state remains 32 bytes.
 */
#ifndef PLOC3_DSP_H
#define PLOC3_DSP_H
#include "ploc_params.h"
#include "ploc3_tables.h"
typedef struct {
    uint32_t phase[6];
    uint16_t body_phase[2];
    uint16_t clock; /* blocks: 0..10; variation seed: 11..12; latched extras: 13..15 */
    int16_t lp;
} ploc3_state;
_Static_assert(sizeof(ploc3_state)==32,"PLOC3 voice state");
static inline uint32_t ploc3_onset(int j,unsigned seed) {
    static const uint16_t at[6]={265,882,1588,2426,3440,4631};
    return at[j]+(j?((seed*(unsigned)(7+j)+3*j)&7u)*44u:0u);
}
static inline int32_t ploc3_env(uint32_t elapsed,uint32_t d,uint32_t end) {
    if(elapsed>=end)return 0;
    uint32_t x=(elapsed*d)>>12,k=x>>3;
    if(k>=256)return 0;
    int32_t a=ploc3_exp[k],b=ploc3_exp[k+1];
    return a-(((a-b)*(int32_t)(x&7))>>3);
}
static inline uint32_t ploc3_decay(uint32_t inc,int visc) {
    /* Increasing VISC damps the sound; watery notes ring longer. */
    return ploc_decay(inc,4096u+((unsigned)visc>>1));
}
static inline uint32_t ploc3_glide(int visc) {
    /* Independent of amplitude damping: oil slows the upward pouic. */
    return 4915u-(((unsigned)visc*3604u)>>15); /* .15 .. .04 */
}
static inline void ploc3_start(ploc3_state *s,unsigned seed,int drops) {
    for(int j=0;j<6;j++)s->phase[j]=0;
    s->body_phase[0]=s->body_phase[1]=0;
    s->clock=(uint16_t)(((seed&3u)<<11)|((unsigned)drops<<13));s->lp=0;
}
static inline void ploc3_process(ploc3_state *s,int16_t *out,const int32_t *pitch,
 const int16_t *p,int32_t velocity,const int16_t *sine,int count) {
    static const uint32_t ratios[6]={32768,59965,88801,48824,69796,103875};
    static const int32_t weights[6]={32767,7864,4588,3277,2458,1802};
    static const uint32_t body_ratios[2]={26214,71107};
    unsigned seed=(s->clock>>11)&3u;
    uint32_t start=(uint32_t)(s->clock&2047u)*128u;
    /* DROPS is captured at note-on; removing satellites never chops a ringing bubble.
     * Below -65dB after about 5.94s, end the last inaudible tail explicitly. */
    if((s->clock&2047u)==2047u){s->lp=0;for(int n=0;n<count;n++)out[n]=0;return;}
    int nb=1+(s->clock>>13);
    uint32_t base0=ploc_base((uint32_t)pitch[0],p[0]);
    uint32_t ratio[6],damp[6],end[6],glide[6],onset[6],aa[6];
    uint32_t xi=ploc3_glide(p[2]);
    uint32_t attack_inv=16777216u/(4096u+((unsigned)p[2]*5u>>3));
    int tone=15000-((p[2]*11000)>>15);
    int hit_gain=32767-((p[2]*28000)>>15);
    for(int j=0;j<nb;j++) {
        int variation=j?(int)((seed*(unsigned)(3+j)+j*11)&15u)-7:0;
        ratio[j]=(uint32_t)((int32_t)ratios[j]+variation*80);
        uint32_t b=ploc_mul(base0,ratio[j]);
        aa[j]=b<=0x40000000u?32767:(b>=0x60000000u?0:(0x60000000u-b)>>14);
        if(b>0x60000000u)b=0x60000000u;
        damp[j]=ploc3_decay(b,p[2]);end[j]=8388608u/damp[j];
        glide[j]=ploc_decay(b,10923);onset[j]=ploc3_onset(j,seed);
    }
    uint32_t bd[2]={600u-((unsigned)p[1]*530u>>15),800u-((unsigned)p[1]*660u>>15)},be[2];
    for(int j=0;j<2;j++){bd[j]=(bd[j]*(32768u+(unsigned)p[2]))>>15;be[j]=8388608u/bd[j];}
    int body_gain[2]={(p[1]*6553)>>15,(p[1]*2621)>>15};
    for(int n=0;n<count;n++) {
        uint32_t age=start+(uint32_t)n;
        uint32_t base=ploc_base((uint32_t)pitch[n],p[0]);
        int32_t wave=0;int active=0;
        for(int j=0;j<nb;j++) {
            if(age<onset[j])continue;
            uint32_t elapsed=age-onset[j];
            int32_t e=ploc3_env(elapsed,damp[j],end[j]);
            if(!e)continue;active=1;
            uint32_t b=ploc_mul(base,ratio[j]);if(b>0x60000000u)b=0x60000000u;
            uint32_t inc=ploc_mul(b,32768u+ploc_rise(glide[j],xi,elapsed));
            if(inc>0x70000000u)inc=0x70000000u;
            uint32_t ai=(elapsed*attack_inv)>>12;
            int32_t attack=ai<128?ploc_attack[ai]:32767;
            int32_t y=(sine[s->phase[j]>>19]*attack)>>15;
            s->phase[j]+=inc;
            y=(y*e)>>15;y=(y*weights[j])>>15;
            wave+=(y*(int32_t)aa[j])>>15;
        }
        if(p[1])for(int j=0;j<2;j++) {
            int32_t e=ploc3_env(age,bd[j],be[j]);if(!e)continue;active=1;
            uint32_t inc=ploc_mul(base,body_ratios[j]);if(inc>0x60000000u)inc=0x60000000u;
            int32_t y=sine[s->body_phase[j]>>3];
            s->body_phase[j]=(uint16_t)(s->body_phase[j]+((inc+32768u)>>16));
            uint32_t ai=(age*attack_inv)>>12;
            y=(y*(ai<128?ploc_attack[ai]:32767))>>15;
            y=(y*e)>>15;wave+=(y*body_gain[j])>>15;
        }
        if(age<353) {
            uint32_t z=(age+1+seed*997u)*0x9e3779b9u;
            z^=z>>16;z*=0x7feb352du;z^=z>>15;
            int32_t hit=(((int32_t)(z>>16)-32768)*ploc_impact[age])>>15;
            wave+=(hit*hit_gain)>>15;active=1;
        }
        /* Sum coefficients bound wave/2 below int16 range. The 8 oscillator
         * states are local to this voice; no cross-voice mutable DSP state. */
        wave>>=1;
        if(!active)s->lp=0;
        else s->lp=(int16_t)(s->lp+(((wave-s->lp)*tone)>>14));
        int32_t sample=(s->lp*velocity)>>15;
        out[n]=(int16_t)((sample*29127)>>15); /* same main-bubble level as 0.2.1 */
    }
    if((s->clock&2047u)<2047u)s->clock++;
}
#endif
