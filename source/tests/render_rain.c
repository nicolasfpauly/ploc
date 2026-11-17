/* Offline listening fixture: production event model + production PLOC DSP.
 * Approximates the stock T2 amplitude release; not an emulator capture. */
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include "../plugins/ploc/ploc_dsp.h"
#include "../plugins/rain/rain_model.h"
typedef struct {ploc_state state;int id,on,gain;int32_t inc;} voice;
static voice voices[6];static int next_id;
int rain_output_on(int mode,int note,int key,int vel){(void)mode;(void)key;(void)vel;for(int i=0;i<6;i++)if(voices[i].on==0){voice *v=&voices[i];ploc_start(&v->state);v->on=1;v->gain=32767;v->inc=(int32_t)(440*pow(2,(note-69)/12.0)/44100*4294967296.0);v->id=++next_id;return v->id;}return -1;}
void rain_output_off(int id){for(int i=0;i<6;i++)if(voices[i].id==id&&voices[i].on==1)voices[i].on=2;}
int main(int argc,char **argv){assert(argc==2);FILE *f=fopen(argv[1],"wb");assert(f);static int16_t sine[8192];for(int i=0;i<8192;i++)sine[i]=(int16_t)lrint(sin(i*6.283185307179586/8192)*32767);
 rain_state r;rain_init(&r,0);r.cfg.rate=3;r.cfg.open=99;rain_key(&r,0,48,1);rain_key(&r,4,55,1);rain_key(&r,7,60,1);rain_key(&r,11,67,1);
 int16_t params[4]={18000,14000,26000,27000};unsigned clock=0;
 for(int sample=0;sample<44100*14;sample++){
  if(sample<44100*12){clock+=192;if(clock>=44100){clock-=44100;r.cfg.density=8+sample/(44100*12/90);rain_tick(&r);}}
  else if(sample==44100*12)rain_stop(&r);
  int sum=0;
  for(int i=0;i<6;i++){voice *v=&voices[i];if(!v->on)continue;int16_t out;ploc_process(&v->state,&out,&v->inc,params,28000,sine,1);sum+=(out*v->gain)>>15;if(v->on==2){v->gain-=20;if(v->gain<=0){v->gain=0;v->on=0;}}}
  if(sum>32767)sum=32767;if(sum< -32768)sum=-32768;int16_t out=(int16_t)sum;fwrite(&out,2,1,f);
 }
 fclose(f);puts("Rendered rain from sparse drops to shower, then release");
}
