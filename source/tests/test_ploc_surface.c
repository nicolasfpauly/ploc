#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../plugins/ploc/ploc5_surface.h"
static uint32_t seed=17;
static uint32_t random_word(void){seed=seed*1664525u+1013904223u;return seed;}
int main(void){
 unsigned bins[4]={0};for(int v=0;v<32768;v++)bins[p5_ripple_count(v)-1]++;
 for(int k=0;k<4;k++)assert(bins[k]==8192);assert(p5_ripple_count(-9)==1&&p5_ripple_count(40000)==4);
 ploc5_surface s;p5_surface_start(&s);
 for(int start=-32768;start<32768;start+=128){int16_t a[128],copy[128];for(int n=0;n<128;n++)a[n]=copy[n]=(int16_t)(start+n);p5_surface_process(&s,a,128,0,16384);assert(!memcmp(a,copy,sizeof a));}
 for(int count=1;count<=4;count++)for(int size=0;size<=32767;size+=32767){
  p5_surface_start(&s);int16_t quiet[2048]={0};p5_surface_process(&s,quiet,2048,(count-1)*8192,size);
  int16_t impulse[512]={0};impulse[0]=32767;p5_surface_process(&s,impulse,512,(count-1)*8192,size);
  int nonzero=0,delay=12+(size>>9);for(int i=0;i<512;i++)if(impulse[i]){nonzero++;assert(i==0||i==delay||i==2*delay||i==3*delay);}
  assert(nonzero==count);for(int i=226;i<512;i++)assert(impulse[i]==0);
 }
 int peak=0;
 p5_surface_start(&s);
 for(int block=0;block<5000;block++){
  int16_t a[128];for(int n=0;n<128;n++)a[n]=(int16_t)((int)(random_word()%60001u)-30000);
  p5_surface_process(&s,a,128,(int)(random_word()&32767u),(int)(random_word()&32767u));
  int total=0;for(int k=0;k<3;k++){assert(s.coefficient[k]>=0);total+=s.coefficient[k];}assert(total<=5618);
  for(int n=0;n<128;n++){int v=a[n]<0?-a[n]:a[n];assert(v<=30000);if(v>peak)peak=v;}
 }
 int16_t silence[2048]={0};p5_surface_process(&s,silence,2048,0,32767);for(int n=256;n<2048;n++)assert(!silence[n]);for(int k=0;k<3;k++)assert(!s.coefficient[k]);
 assert(sizeof(s)<=528);printf("SURFACE: direct sample-exact; 1..4 audible impulse taps; 12..75-sample delays; bounded random modulation peak=%d; finite tail; %zu bytes/voice\n",peak,sizeof(s));
}
