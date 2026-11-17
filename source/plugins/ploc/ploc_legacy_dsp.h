/* PLOC — original water-drop synthesis. GPL-3.0-or-later. */
#ifndef PLOC_LEGACY_DSP_H
#define PLOC_LEGACY_DSP_H
#include <stdint.h>
typedef struct { uint32_t phase, ring, noise; int32_t amp, sweep, lp; uint32_t age; } ploc_legacy_state;
static inline void ploc_legacy_start(ploc_legacy_state *s) {
 s->phase=s->ring=0; s->noise=0x93a75c21u; s->amp=1<<24; s->sweep=32767; s->lp=0; s->age=0;
}
/* Sine lookup belongs to the host/firmware; no allocation, float or I/O here. */
static inline void ploc_legacy_process(ploc_legacy_state *s,int16_t *out,const int32_t *pitch,
 const int16_t *p,int32_t velocity,const int16_t *sine,int count) {
 const int decay=2+(((32767-p[2])*45)>>15);
 const int sweep_depth=4096+(p[0]>>1);
 const int sweep_decay=8+((32767-p[0])>>9);
 const int body=p[1]>>2, tone=512+(p[3]>>1);
 for(int n=0;n<count;n++) {
  uint32_t base=(uint32_t)pitch[n]; if(base>0x30000000u)base=0x30000000u;
  uint32_t inc=base+(base>>15)*(uint32_t)((s->sweep*sweep_depth)>>15);
  s->phase+=inc; s->ring+=inc+(inc>>1); /* quiet 1.5x cavity mode */
  int32_t fundamental=sine[s->phase>>19];
  int32_t harmonic=(sine[s->ring>>19]*body)>>15;
  s->noise^=s->noise<<13; s->noise^=s->noise>>17; s->noise^=s->noise<<5;
  int32_t hit=s->age<96 ? (((int32_t)(s->noise>>17)-16384)*(96-(int)s->age))>>8 : 0;
  int32_t wave=(fundamental>>1)+harmonic+((hit*p[3])>>15);
  s->lp+=((wave-s->lp)*tone)>>15;
  int32_t sample=(s->lp*(s->amp>>10))>>15;
  sample=(sample*velocity)>>15; out[n]=(int16_t)((sample*10923)>>15); /* six-voice headroom */
  s->amp-=(s->amp*decay)>>15;
  if(s->amp<32768)s->amp=0;
  if(s->sweep>0){int d=(s->sweep*sweep_decay)>>15;s->sweep-=d>0?d:1;}
  if(s->age<0x7fffffffu)s->age++;
 }
}
#endif
