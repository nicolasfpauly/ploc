/* PLOC 0.5: short feed-forward surface response inside the synth.
 * 1..4 wavefronts = direct sound plus up to three quiet micro-reflections.
 * Convex mixing cannot raise the input peak; no feedback or T3 provider.
 * GPL-3.0-or-later with SDK firmware permission, see NOTICE. */
#ifndef PLOC5_SURFACE_H
#define PLOC5_SURFACE_H
#include <stdint.h>
typedef struct { int16_t history[256], coefficient[3]; uint16_t at; } ploc5_surface;
static inline unsigned p5_ripple_count(int raw) {
    if(raw<0)raw=0;if(raw>32767)raw=32767;
    return 1u+((unsigned)raw>>13);
}
static inline void p5_surface_start(ploc5_surface *s) {
    for(unsigned i=0;i<256;i++)s->history[i]=0;
    for(unsigned i=0;i<3;i++)s->coefficient[i]=0;
    s->at=0;
}
static inline void p5_surface_process(ploc5_surface *s,int16_t *audio,int frames,int raw,int size) {
    static const int16_t weights[4][3]={{0,0,0},{2809,0,0},{2657,1771,0},{2587,1725,862}};
    unsigned count=p5_ripple_count(raw),delay=12u+((unsigned)(uint16_t)size>>9);
    if(delay>75)delay=75;
    for(int n=0;n<frames;n++) {
        int total=0;
        for(unsigned k=0;k<3;k++) {
            int delta=weights[count-1][k]-s->coefficient[k];
            s->coefficient[k]=(int16_t)(delta>-32&&delta<32?weights[count-1][k]:s->coefficient[k]+(delta>>5));
            total+=s->coefficient[k];
        }
        int dry=audio[n],out=dry;
        if(total) {
            int32_t mixed=dry*(32768-total);
            for(unsigned k=0;k<3;k++)mixed+=(int32_t)s->history[(s->at-delay*(k+1u))&255u]*s->coefficient[k];
            out=(mixed+16384)>>15;
        }
        s->history[s->at]=(int16_t)dry;s->at=(uint16_t)((s->at+1u)&255u);
        audio[n]=(int16_t)out;
    }
}
#endif
