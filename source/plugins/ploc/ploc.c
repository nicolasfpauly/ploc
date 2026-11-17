/* PLOC 0.5, with preserved legacy models. GPL-3.0-or-later, see NOTICE. */
#include "op1_plugin.h"
#include "ploc_dsp.h"
#include "ploc4_dsp.h"
#include "ploc5_surface.h"
typedef struct { uint32_t vtable; ploc_model_state state; } ploc_voice;
_Static_assert(sizeof(ploc_voice)<=0x24,"voice storage overflow");
static ploc4_state water[OP1_VOICES];
static ploc5_surface surface[OP1_VOICES];
static uint8_t new_model[OP1_VOICES];
static uint32_t render_clock,last_render[OP1_VOICES];
volatile uint32_t ploc_hits;
static void note_on(void *self,const int16_t *p) {
 int i=op1_voice_index(self);new_model[i]=(uint8_t)ploc4_is(p);
 last_render[i]=render_clock;
 if(new_model[i]){ploc4_start(&water[i],ploc_hits);p5_surface_start(&surface[i]);}
 else {water[i].held=0;ploc_model_start_seed(&((ploc_voice*)self)->state,p,ploc_hits);}
 ploc_hits++;
}
static void note_off(void *self){ploc4_off(&water[op1_voice_index(self)]);}
static void render(void *self,int16_t *out,const int16_t *p) {
 int i=op1_voice_index(self);
 last_render[i]=++render_clock;
 if(new_model[i]) {
   int voices=0;for(int j=0;j<OP1_VOICES;j++)if(new_model[j]&&render_clock-last_render[j]<16u){
     int active=water[j].held||water[j].contact>15;
     for(int k=0;k<PLOC4_GRAINS;k++)if(water[j].grain[k].wait||water[j].grain[k].amp>32)active=1;
     if(active)voices++;
   }
   ploc4_process(&water[i],out,OP1_PITCH_INC,p,op1_voice_vel_gain(self),OP1_SINE_TABLE,OP1_BLOCK,voices);
   p5_surface_process(&surface[i],out,OP1_BLOCK,p[1],p[0]);
 }else ploc_model_process(&((ploc_voice*)self)->state,out,OP1_PITCH_INC,p,op1_voice_vel_gain(self),OP1_SINE_TABLE,OP1_BLOCK);
}
/* The companion maps physical turns to the canonical preset words,
 * and Shift turns to SPRAY / HEIGHT / TAIL / IMPACT. No page navigation. */
const op1_engine_t ENGINE_ploc={note_on,render,note_off,{0,0,0,0}};
