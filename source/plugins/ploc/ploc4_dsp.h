/* Airborne droplets and held-key flow, fixed-point port of the v2 studies.
 * GPL-3.0-or-later with SDK firmware permission, see NOTICE.
 * Damping: d(f)=.043f+.0014f^1.5 (van den Doel). Rise, tail stretch,
 * viscosity and harmonic attraction are musical macros, not fluid simulation.
 * No allocation, floating point, transcendental or division in the sample loop.
 */
#ifndef PLOC4_DSP_H
#define PLOC4_DSP_H
#include <stdint.h>
#include "ploc4_params.h"
#define PLOC4_GRAINS 8
#define PLOC4_MAX_HZ 12000u
#define PLOC4_MIN_HZ 180u
typedef struct {
    uint32_t phase,inc,age,wait;
    int32_t amp,attack,glide;
    uint32_t decay,attack_d,glide_d;
    int32_t level;
} ploc4_grain;
typedef struct {
    ploc4_grain grain[PLOC4_GRAINS];
    uint32_t rng,clock,next,events,limits;
    int32_t contact,noise_lp,low,dist_lp;
    uint8_t held,started,divisor;
} ploc4_state;
static inline uint32_t p4_rand(ploc4_state *s) {
    uint32_t x=s->rng;x^=x<<13;x^=x>>17;x^=x<<5;s->rng=x;return x;
}
static inline uint32_t p4_mul(uint32_t a,uint32_t b) {
    return (a>>15)*b+(((a&32767u)*b)>>15);
}
static inline uint32_t p4_sqrt(uint32_t x) {
    uint32_t r=0,b=1u<<30;while(b>x)b>>=2;
    while(b){if(x>=r+b){x-=r+b;r=(r>>1)+b;}else r>>=1;b>>=2;}return r;
}
static inline uint32_t p4_hz_inc(uint32_t hz) { return hz*97391u; }
/* Q20 per-sample damping, .55 .. 2.7x physical lifetime via TAIL. */
static inline uint32_t p4_decay(uint32_t hz,unsigned visc,unsigned tail) {
    uint32_t root=p4_sqrt(hz<<16);
    uint32_t d256=11u*hz+((((hz*root)>>6)*23u)>>8);
    uint32_t d=(d256*95u+512u)>>10;
    d=p4_mul(d,29491u+visc*4041u);
    static const uint16_t stretch[16]={59578,52000,45400,39600,34500,30000,26100,22700,
        19700,17100,14850,12900,11200,9720,8440,7330};
    d=p4_mul(d,stretch[tail]);return d<64?64:(d>400000?400000:d);
}
static inline void ploc4_start(ploc4_state *s,unsigned seed) {
    s->rng=0x50c4a93du^((seed+1u)*0x9e3779b9u);if(!s->rng)s->rng=1;
    s->clock=0;s->next=0;s->events=0;s->limits=0;s->contact=32767;
    s->noise_lp=s->low=s->dist_lp=0;s->held=1;s->started=0;s->divisor=1;
    for(int k=0;k<PLOC4_GRAINS;k++){s->grain[k].amp=0;s->grain[k].wait=0;}
}
static inline void ploc4_off(ploc4_state *s){s->held=0;}
static inline uint32_t p4_hz_pitch(uint32_t pitch) {
    if(pitch>0x60000000u)pitch=0x60000000u;
    return ((pitch>>16)*44100u+32768u)>>16;
}
/* A log-frequency approximation table for independent bands; 9 streams are
 * interleaved in an eight-grain pool. Finished grains are reused first. */
static inline void p4_spawn(ploc4_state *s,const int16_t *p,uint32_t pitch,int primary) {
    static const uint16_t bands[9]={440,750,1180,1760,2540,3550,4900,6700,9550};
    static const uint16_t weights[9]={4800,7000,10000,16000,23000,29000,31000,26000,19000};
    uint32_t r=p4_rand(s);unsigned spread=ploc4_get(p,P4_SPREAD),tone=ploc4_get(p,P4_TONE);
    unsigned tune=ploc4_get(p,P4_TUNE),visc=(unsigned)p[2]>>11;
    unsigned band=((r>>16)&255u); /* Weighted toward the audible mid/high plink. */
    band=band<3?0:band<10?1:band<23?2:band<48?3:band<86?4:band<134?5:band<184?6:band<227?7:8;
    uint32_t hz;
    uint32_t keyhz=p4_hz_pitch(pitch);if(keyhz<12)keyhz=12;
    if(primary){hz=keyhz*12u;hz=p4_mul(hz,49152u-(unsigned)p[0]);
        if(hz>PLOC4_MAX_HZ)hz=PLOC4_MAX_HZ;
        int jitter=(int)(r&32767u)-16384;
        hz=(uint32_t)((int32_t)hz+(((int32_t)hz*jitter)>>15)*(int)spread/96);}
    else{
        hz=p4_mul(bands[band],49152u-(unsigned)p[0]);
        int jitter=(int)(r&32767u)-16384;
        hz=(uint32_t)((int32_t)hz+(((int32_t)hz*jitter)>>15)*(int)spread/16);
        /* Partial attraction to the held note's octave family. */
        uint32_t target=keyhz;while(target<395&&target<16000)target*=2;
        while(target<hz&&target<16000)target*=2;
        if(target>hz&&target/2>PLOC4_MIN_HZ&&target-hz>hz-target/2)target/=2;
        hz=(hz*(15u-tune)+target*tune)/15u;
    }
    if(hz<PLOC4_MIN_HZ)hz=PLOC4_MIN_HZ;if(hz>PLOC4_MAX_HZ)hz=PLOC4_MAX_HZ;
    int slot=0;int32_t quiet=0x7fffffff;
    for(int k=0;k<PLOC4_GRAINS;k++)if(!s->grain[k].wait&&s->grain[k].amp<quiet){quiet=s->grain[k].amp;slot=k;}
    ploc4_grain *g=&s->grain[slot];g->phase=(r&0xffffu)<<16;g->inc=p4_hz_inc(hz);
    g->age=0;g->wait=primary?750u+(r&255u):0;g->amp=32767;g->attack=32767;
    g->decay=p4_decay(hz,visc,ploc4_get(p,P4_TAIL))>>5;
    g->attack_d=(240000u-visc*10000u)>>5; /* 0.04 .. 0.23ms onset smoothing. */
    g->glide=0;g->glide_d=g->decay/(2u+visc/5u);
    int level=primary?29000:(int)weights[band];
    if(!primary){int bal=band<4?(23-(int)tone):(8+(int)tone);level=level*bal/20;
        level=level*(10000+(int)((r>>8)&16383u))>>15;level=level*12/10;}
    g->level=level>42000?42000:level;s->events++;
}
/* Exponential gap table, quantiles of -ln(U), Q12. CHAOS blends this with
 * a regular clock; the next event is always at least 3ms away. */
static inline uint32_t p4_gap(ploc4_state *s,const int16_t *p) {
    static const uint16_t ex[32]={65,197,333,474,620,771,929,1093,1264,1442,1629,1825,2031,2248,2478,2721,
        2981,3258,3556,3877,4226,4608,5029,5499,6030,6640,7362,8242,9378,10968,13532,17035};
    uint32_t rate=2u+((uint32_t)p[3]*126u>>15),mean=44100u/rate;
    unsigned c=ploc4_get(p,P4_CHAOS),q=ex[p4_rand(s)>>27];
    uint32_t gap=(mean*((61440u-c*4096u+c*q)>>4))>>12;
    return gap<132?132:gap;
}
static inline int32_t p4_sat(int32_t x) {return x>30000?30000:(x< -30000?-30000:x);}
static inline void ploc4_process(ploc4_state *s,int16_t *out,const int32_t *pitch,
 const int16_t *p,int32_t velocity,const int16_t *sine,int count,int voices) {
    unsigned impact=ploc4_get(p,P4_IMPACT),spray=ploc4_get(p,P4_SPRAY),distance=ploc4_get(p,P4_DISTANCE),tone=ploc4_get(p,P4_TONE);
    uint32_t pouic=ploc4_get(p,P4_POUIC)*820u; /* 0 .. .375, natural defaults ~.075 */
    /* Conservative chord compensation, retained through release so lifting
     * all keys cannot suddenly boost the remaining droplets sixfold. */
    if(voices>s->divisor)s->divisor=(uint8_t)voices;
    int gain=(velocity*30000)>>15;gain/=s->divisor;
    /* Constant poles, no expensive coefficient work in the sample loop. */
    int lpcoef=15000+(int)tone*1180-(int)distance*600-(p[2]>>5);if(lpcoef<4000)lpcoef=4000;
    if(!s->started){p4_spawn(s,p,(uint32_t)pitch[0],1);s->started=1;s->next=p4_gap(s,p);}
    for(int n=0;n<count;n++) {
        if(s->held&&p[3]>0&&(int32_t)(s->clock-s->next)>=0){p4_spawn(s,p,(uint32_t)pitch[n],0);s->next=s->clock+p4_gap(s,p);}
        else if(!p[3])s->next=s->clock+132;
        int32_t sum=0;
        for(int k=0;k<PLOC4_GRAINS;k++) {
            ploc4_grain *g=&s->grain[k];if(g->wait){g->wait--;continue;}if(g->amp<=2)continue;
            int32_t env=(g->amp*(32767-g->attack))>>15;
            int32_t y=(sine[g->phase>>19]*env)>>15;sum+=(y*g->level)>>15;
            g->phase+=p4_mul(g->inc,32768u+((pouic*(uint32_t)g->glide)>>15));
            g->amp-=(g->amp*(int32_t)g->decay)>>15;
            if(g->amp>2&&((g->amp*(int32_t)g->decay)>>15)==0)g->amp--;
            g->attack-=(g->attack*(int32_t)g->attack_d)>>15;
            if(g->attack>0&&((g->attack*(int32_t)g->attack_d)>>15)==0)g->attack--;
            g->glide+=((32767-g->glide)*(int32_t)g->glide_d)>>15;g->age++;
        }
        int32_t noise=(int32_t)(p4_rand(s)>>17)-16384;
        s->noise_lp+=(noise-s->noise_lp)>>2;
        int32_t high=noise-s->noise_lp;
        sum+=(high*((s->contact*(int32_t)impact)>>4))>>17;
        s->contact-=(s->contact*2300)>>15;if(s->contact>0&&s->contact<15)s->contact=0;
        if(s->held&&p[3]>0)sum+=(high*(int32_t)(spray*(unsigned)p[3]>>10))>>12;
        if(sum>30000||sum< -30000)s->limits++;
        sum=p4_sat(sum);s->low+=(sum-s->low)>>4;
        if(!sum&&s->low> -16&&s->low<16)s->low=0;
        sum-=s->low;sum=p4_sat(sum);
        s->dist_lp+=((sum-s->dist_lp)*lpcoef)>>15;
        if(!sum&&s->dist_lp> -8&&s->dist_lp<8)s->dist_lp=0;
        sum=s->dist_lp;sum=(sum*(32767-(int)distance*900))>>15;
        out[n]=(int16_t)((p4_sat(sum)*gain)>>15);s->clock++;
    }
}
#endif
