#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define PLOC4_LINK_HOST
#define PLOC5_LINK_HOST
#include "op1_plugin.h"

static int16_t words[8][8],saved[8][8];
static int slot,commits,host_shift_held;
volatile uint8_t op1_eng_page[4];
static int p4_slot(void){return slot;}
static int mock_shift_held(void){return host_shift_held;}
static const int16_t *mock_synth_knobs(int s){return words[s];}
const op1_engine_ext2_t ENGINE_ploc={0};

static int mock_word_set(const op1_engine_t *e,int k,int v){
 assert(e==&ENGINE_ploc.ext.voice && k>=0 && k<8 && k!=4 && v>=0 && v<=32767);
 words[slot][k]=(int16_t)v;memcpy(saved[slot],words[slot],16);commits++;
 return 1;
}
#define op1_synth_knobs mock_synth_knobs
#define op1_shift_held mock_shift_held
#define op1_engine_word_set mock_word_set
#include "../plugins/ploc/controls.c"
static int turn(int k,int d,int shift,int screen){
 host_shift_held=shift;return ploc_input(screen==0x14?OP1_ENG_T1:OP1_ENG_T2,OP1_ENG_TURN,k,d);
}
static int key(int code,int shift,int screen){
 host_shift_held=shift;return ploc_input(screen==0x14?OP1_ENG_T1:OP1_ENG_T2,OP1_ENG_KEY,code,1);
}
int main(void){
 for(int s=0;s<8;s++){for(int k=0;k<4;k++)words[s][k]=12000;words[s][4]=0x5327;ploc4_defaults(words[s]);}
 assert(!turn(0,1024,0,6));
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
  int hidden[P4_EXTRAS];for(int j=0;j<P4_EXTRAS;j++)hidden[j]=(int)ploc4_get(words[0],j);
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
 assert(!key(0x3E,0,0x14)&&!key(0x3B,1,0x14));assert(!op1_eng_page[0]);
 n=commits;assert(!turn(0,1024,0,6));
 words[0][5]=3;assert(!turn(0,1024,1,0x14));ploc4_defaults(words[0]);assert(commits==n);
 /* Switching slots must discard any fractional turn left by the prior slot. */
 assert(turn(2,512,0,0x14));
 slot=1;int spray1=(int)ploc4_get(words[1],P4_SPRAY);
 assert(turn(2,512,0,0x14));assert(ploc4_get(words[1],P4_SPRAY)==(unsigned)spray1);
 assert(turn(2,512,0,0x14));assert(ploc4_get(words[1],P4_SPRAY)==(unsigned)(spray1+1));
 assert(turn(1,1024,0,0x14));assert(words[1][0]==13024);
 assert(!op1_eng_page[2]&&!op1_eng_page[3]);
 for(int s=0;s<8;s++)assert(words[s][4]==0x5327);
 printf("PLOC standalone: 7 controls, unused blue Shift consumed, exact save/reload words, hidden tail preserved, clamps, fractional detents, slot isolation and legacy passthrough (%d commits)\n",commits);
}
