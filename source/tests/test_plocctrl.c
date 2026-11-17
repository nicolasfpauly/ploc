#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define OP1_PLUGIN_H
#define PLOC4_LINK_HOST
#define PLOC5_LINK_HOST
#define PLOCCTRL_HOST
#define OP1_ENGINE_MAGIC 0x5300
#define OP1_MAX_ORDER 63
typedef struct {void (*note_on)(void *,const int16_t *);void (*render)(void *,int16_t *,const int16_t *);void (*note_off)(void *);uint8_t press[4];} op1_engine_t;
typedef struct {} op1_screen_t;typedef struct {} op1_beat_t;typedef struct {} op1_fx_t;
#include "op1_feature.h"
static int16_t words[8][8],saved[8][8];static int slot,commits,held,drum;
volatile uint8_t op1_eng_page[4];
static int p4_slot(void){return slot;}static int p4_drum_mode(void){return drum;}static int p5_shift_held(void){return held;}
static const int16_t *op1_synth_knobs(int s){return words[s];}
static void save_words(int s,const int16_t *p){memcpy(saved[s],p,16);commits++;}
#include "../plugins/plocctrl/plocctrl.c"
static int turn(int k,int d,int shift,int screen){held=shift;op1_input in={.type=OP1_IN_KNOB,.code=(uint8_t)k,.delta=(int16_t)d,.shift=(uint8_t)shift,.screen=screen};return input(&in);}
static int key(int code,int shift,int screen){held=shift;op1_input in={.type=OP1_IN_KEY,.code=(uint8_t)code,.pressed=1,.shift=(uint8_t)shift,.screen=screen};return input(&in);}
int main(void){
 for(int s=0;s<8;s++){for(int k=0;k<4;k++)words[s][k]=12000;words[s][4]=0x5327;ploc4_defaults(words[s]);}
 op1_eng_page[2]=PLOC5_TOKEN|39;assert(!turn(0,1024,0,0x14));op1_eng_page[2]=PLOC6_TOKEN|39;
 const int physical[3]={0,1,3},canonical[3]={3,0,1};
 for(int i=0;i<3;i++){
  int k=physical[i],word=canonical[i];int16_t old[8];memcpy(old,words[0],16);
  assert(turn(k,1024,0,0x14));old[word]+=1024;assert(!memcmp(old,words[0],16));
  assert(!memcmp(saved[0],words[0],16));
  for(int j=0;j<3;j++)assert(turn(k,32767,0,0x14));assert(words[0][word]==32767);
  assert(turn(k,-32768,0,0x14));assert(words[0][word]==0);
 }
 int16_t before[8];memcpy(before,words[0],16);int spray=(int)ploc4_get(words[0],P4_SPRAY);
 assert(turn(2,512,0,0x14));assert(!memcmp(before,words[0],16));
 assert(turn(2,512,0,0x14));ploc4_set(before,P4_SPRAY,(unsigned)(spray+1));assert(!memcmp(before,words[0],16));
 for(int j=0;j<3;j++)assert(turn(2,32767,0,0x14));assert(ploc4_get(words[0],P4_SPRAY)==15);
 assert(turn(2,-32768,0,0x14));assert(ploc4_get(words[0],P4_SPRAY)==0);assert(!memcmp(saved[0],words[0],16));
 /* Quality is the original full-resolution VISC word, not the hidden tail. */
 memcpy(before,words[0],16);assert(turn(2,512,1,0x14));before[2]+=512;
 assert(!memcmp(before,words[0],16)&&op1_eng_page[0]==0x82);
 assert(turn(2,32767,1,0x14));assert(words[0][2]==32767);
 assert(turn(2,-32768,1,0x14));assert(words[0][2]==0);
 const int p[2]={1,3},extra[2]={P4_POUIC,P4_IMPACT};
 for(int i=0;i<2;i++){
  held=0;key_sync();int hidden[P4_EXTRAS];for(int j=0;j<P4_EXTRAS;j++)hidden[j]=(int)ploc4_get(words[0],j);
  memcpy(before,words[0],16);int old=hidden[extra[i]];
  assert(turn(p[i],512,1,0x14));assert(!memcmp(before,words[0],16));
  assert(turn(p[i],512,1,0x14));assert(ploc4_get(words[0],extra[i])==(unsigned)(old+1));
  assert(turn(p[i],32767,1,0x14));assert(ploc4_get(words[0],extra[i])==15);
  assert(turn(p[i],-32768,1,0x14));assert(ploc4_get(words[0],extra[i])==0);
  for(int j=0;j<P4_EXTRAS;j++)if(j!=extra[i])assert((int)ploc4_get(words[0],j)==hidden[j]);
  assert(!memcmp(saved[0],words[0],16));
 }
 memcpy(before,words[0],16);int n=commits;
 for(int i=0;i<100;i++)assert(turn(0,i%2?32767:-32768,1,0x14));
 assert(!memcmp(before,words[0],16)&&commits==n&&!op1_eng_page[0]);
 assert(ploc4_get(words[0],P4_TAIL)==5); /* Existing sound/duration preserved. */
 held=0;key_sync();assert(!key(0x3E,0,0x14)&&!key(0x3B,1,0x14));assert(!op1_eng_page[0]);
 n=commits;assert(!turn(0,1024,0,6));drum=1;assert(!turn(0,1024,1,0x14));drum=0;
 op1_eng_page[2]=PLOC6_TOKEN|38;assert(!turn(0,1024,1,0x14));op1_eng_page[2]=PLOC6_TOKEN|39;
 words[0][5]=3;assert(!turn(0,1024,1,0x14));ploc4_defaults(words[0]);assert(commits==n);
 assert(turn(1,1024,1,0x14));held=0;event(OP1_EV_SERVICE,0);assert(!op1_eng_page[0]&&op1_eng_page[3]==(PLOC6_TOKEN|39));
 slot=1;assert(turn(1,1024,0,0x14));assert(words[1][0]==13024);event(OP1_EV_SLOT,1);assert(!op1_eng_page[0]&&!op1_eng_page[2]);
 for(int s=0;s<8;s++)assert(words[s][4]==0x5327);
 printf("PLOCCTRL6: 7 controls, unused blue Shift consumed, exact save/reload words, hidden tail preserved, clamps, fractional steps, release and version isolation (%d commits)\n",commits);
}
