/* Host bridge exercises the actual screen callback and exports its retained state. */
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define OP1_PLUGIN_H
#define PLOC_SCREEN_HOST
#define PLOC4_LINK_HOST
#define PLOC5_LINK_HOST
#define OP1_ENGINE_MAGIC 0x5300
#define OP1_MAX_ORDER 63
#define OP1_Q10(v) ((int32_t)((v)*1024))
#define OP1_SCREEN_SLOT 0
typedef struct {int32_t x,y;} op1_pt;
typedef struct {void (*init)(void *);void (*update)(void *,int);} op1_screen_t;
#include "ploc6_host_nodes.h"
typedef struct {int visible;int32_t m[9];uint8_t line[4],fill[4];} host_node;
static host_node host[P6_HOST_NODES];
static int16_t knobs[8]={16384,23000,15000,18000,0x5327,0,0,0},sine[8192];
#define OP1_SINE_TABLE sine
static int held,missing,constructors,text_sets,draws,poses,vis_changes,legacy_inits,legacy_draws;
static char strings[8][16];static int text_x[8],text_y[8],text_size[8],quality_drawn;static uint8_t text_color[8][4];
volatile uint8_t op1_eng_page[4];
static int p5_shift_held(void){return held;}
void *op1_find_node(void *,const char *);void op1_node_set_transform(void *,const int32_t *);void op1_node_set_visible(void *,int);
uint8_t *op1_node_line_rgba(void *,const char *);uint8_t *op1_node_fill_rgba(void *,const char *);
void op1_screen_draw_cached(void *,int,void *);void op1_screen_draw(void *,int);void *op1_raster(void);const int16_t *op1_synth_knobs(int);void sq_synth_knobs_live(int,int16_t *);
static void old_init(void *s){(void)s;legacy_inits++;}static void old_draw(void *s,int dt){(void)s;(void)dt;legacy_draws++;}
const op1_screen_t SCREEN_ploc_legacy={old_init,old_draw};
#include "../plugins/ploc/screen.c"
static host_node *lookup(const char *name){
 for(int i=0;i<P6_HOST_NODES;i++)if(!strcmp(name,p6_host_ids[i]))return &host[i];
 fprintf(stderr,"Unknown SVG node: %s\n",name);abort();
}
void *op1_find_node(void *s,const char *name){(void)s;return missing?0:lookup(name);}
uint8_t *op1_node_line_rgba(void *s,const char *id){(void)s;return missing?0:lookup(id)->line;}
uint8_t *op1_node_fill_rgba(void *s,const char *id){(void)s;return missing?0:lookup(id)->fill;}
void op1_node_set_transform(void *p,const int32_t *m){
 assert(p&&m[8]==1024);for(int k=0;k<5;k++)assert(m[k]>=-1600&&m[k]<=1600);
 assert(abs(m[6])<500*1024&&abs(m[7])<500*1024);
 memcpy(((host_node*)p)->m,m,9*sizeof(int32_t));poses++;
}
void op1_node_set_visible(void *p,int v){((host_node*)p)->visible=v;vis_changes++;}
void op1_screen_draw_cached(void *s,int dt,void *first){(void)s;(void)dt;assert(first==lookup("v6live"));draws++;}
void op1_screen_draw(void *s,int dt){(void)s;(void)dt;draws++;}
void *op1_raster(void){return &host[0];}
const int16_t *op1_synth_knobs(int slot){assert(!slot);return knobs;}
void sq_synth_knobs_live(int slot,int16_t *p){assert(!slot);memcpy(p,knobs,8);}
static int text_index(void *o){int n=(int)(((uintptr_t)o-(uintptr_t)p6_text)/sizeof(p6_text[0]));assert(n>=0&&n<N_TEXT);return n;}
void ploc_text_construct(void *o,int32_t x,int32_t y,int32_t size,const uint8_t *c){
 int n=text_index(o);text_x[n]=x;text_y[n]=y;text_size[n]=size;memcpy(text_color[n],c,4);
 assert(x>=0&&x<=310*1024&&y>=0&&y<140);constructors++;
}
void ploc_text_set(void *o,const char *s){assert(strlen(s)<16);strcpy(strings[text_index(o)],s);text_sets++;}
void ploc_text_draw(void *o,void *r,const int32_t *m){int i=text_index(o);assert(r&&m[8]==1024);if(i==T_SPRAY||i==T_QUALITY)quality_drawn=(i==T_QUALITY);}
static void snapshot(void){
 printf("{\"nodes\":[");
 for(int i=0;i<P6_HOST_NODES;i++){
  if(i)printf(",");printf("{\"id\":\"%s\",\"visible\":%d,\"matrix\":[",p6_host_ids[i],host[i].visible);
  for(int k=0;k<9;k++){if(k)printf(",");printf("%d",host[i].m[k]);}
  printf("],\"line\":[%u,%u,%u,%u],\"fill\":[%u,%u,%u,%u]}",host[i].line[0],host[i].line[1],host[i].line[2],host[i].line[3],host[i].fill[0],host[i].fill[1],host[i].fill[2],host[i].fill[3]);
 }
 printf("],\"text\":[");
 for(int col=0;col<T_QUALITY;col++){int i=col==T_SPRAY&&quality_drawn?T_QUALITY:col;if(col)printf(",");printf("{\"x\":%d,\"y\":%d,\"size\":%d,\"value\":\"%s\",\"color\":[%u,%u,%u]}",text_x[i],text_y[i],text_size[i],strings[i],text_color[i][0],text_color[i][1],text_color[i][2]);}
 puts("]}");
}
int main(int argc,char **argv){
 for(int i=0;i<8192;i++)sine[i]=(int16_t)lround(sin(i*6.283185307179586/8192)*32767);
 for(int i=0;i<P6_HOST_NODES;i++){host[i].visible=1;host[i].m[0]=host[i].m[4]=host[i].m[8]=1024;memcpy(host[i].line,p6_host_l[i],4);memcpy(host[i].fill,p6_host_f[i],4);}
 ploc4_defaults(knobs);
 if(argc==10){
  for(int k=0;k<8;k++)knobs[k]=(int16_t)atoi(argv[k+1]);int active=atoi(argv[9]);
  held=active>=0;op1_eng_page[0]=held?(uint8_t)(0x80|active):0;
  SCREEN_ploc.init(0);SCREEN_ploc.update(0,16);snapshot();return 0;
 }
 SCREEN_ploc.init(0);SCREEN_ploc.update(0,16);
 assert(constructors==N_TEXT&&!lookup("ploclegacy")->visible&&lookup("v6live")->visible);
 assert(!strcmp(strings[0],"flow")&&!strcmp(strings[1],"size")&&!strcmp(strings[2],"spray")&&!strcmp(strings[3],"ripple"));
#ifdef PLOC_STANDALONE
 assert(op1_eng_page[2]==0);  /* An old companion must not see the pairing token. */
#else
 assert(op1_eng_page[2]==(PLOC6_TOKEN|39));
#endif
 assert(text_color[0][2]==255&&text_color[1][1]==237&&text_color[2][0]==255&&text_color[3][1]==66);
 /* Photo at ~1.96 pixels per OLED unit: measured native label caps near
    their constructor size and the 51-unit digit strokes over 54 units tall.
    Reserve excess ink on either end, including under the bezel. */
 for(int i=T_SIZE_VALUE;i<=T_RIPPLE_VALUE;i++)assert(text_y[i]+1.10*text_size[i]+1.5<146);
 for(int i=T_SIZE;i<=T_RIPPLE;i++)assert(text_y[i]+1.05*text_size[i]+1.5<text_y[i+3]-1.5);
 assert(text_y[T_SPRAY_VALUE]+1.10*text_size[T_SPRAY_VALUE]+1.5<text_y[T_RIPPLE]-1.5);
 assert(text_y[T_QUALITY]+1.05*text_size[T_QUALITY]+1.5<text_y[T_SPRAY_VALUE]-1.5);
 int ps=poses,vs=vis_changes,ts=text_sets;host_node saved_nodes[P6_HOST_NODES];memcpy(saved_nodes,host,sizeof host);
 for(int k=0;k<500;k++)SCREEN_ploc.update(0,16);
 assert(poses==ps&&vis_changes==vs&&text_sets==ts&&!memcmp(saved_nodes,host,sizeof host));
 /* QUALITY has no geometry action, including on the drop reflection. */
 int32_t drop[9],needle[9],ripples[9];memcpy(drop,lookup("v6drop")->m,sizeof drop);memcpy(needle,lookup("v6needle")->m,sizeof needle);memcpy(ripples,lookup("v6ripples")->m,sizeof ripples);
 for(int v=0;v<32768;v+=137){
  knobs[2]=(int16_t)v;SCREEN_ploc.update(0,16);
  assert(!memcmp(drop,lookup("v6drop")->m,sizeof drop)&&!memcmp(needle,lookup("v6needle")->m,sizeof needle)&&!memcmp(ripples,lookup("v6ripples")->m,sizeof ripples));
  for(int j=0;j<P6_CLEAN_PAINTS;j++){uint8_t *p=p6_clean_kind[j]=='f'?lookup(p6_clean_paint[j])->fill:lookup(p6_clean_paint[j])->line;assert(p[0]==p6_quality_light(v)&&p[1]==p[0]&&p[2]==p[0]&&p[3]==255);}
 }
 for(int v=0;v<16;v++){
  ploc4_set(knobs,P4_SPRAY,(unsigned)v);SCREEN_ploc.update(0,16);int lit=0;
  for(int j=0;j<7;j++){uint8_t *p=lookup(p6_spray_paint[j])->line;int expected=j<p6_spray_count(v)?255:51;assert(p[0]==expected&&p[1]==expected&&p[2]==expected&&p[3]==255);assert(lookup(j==0?"v6spray0":j==1?"v6spray1":j==2?"v6spray2":j==3?"v6spray3":j==4?"v6spray4":j==5?"v6spray5":"v6spray6")->visible);lit+=p[0]==255;}
  assert(lit==p6_spray_count(v));
 }
 for(int v=0;v<32768;v+=127){
  knobs[1]=(int16_t)v;SCREEN_ploc.update(0,16);for(int j=0;j<4;j++){char id[16];snprintf(id,sizeof id,"v6ring%d",j);assert(lookup(id)->visible==(j<(int)p5_ripple_count(v)));}
 }
 knobs[1]=32767;held=1;op1_eng_page[0]=0x83;
 for(int v=0;v<16;v++){
  ploc4_set(knobs,P4_IMPACT,(unsigned)v);SCREEN_ploc.update(0,16);
  for(int j=0;j<4;j++){char id[16];snprintf(id,sizeof id,"v6ring%d",j);assert(lookup(id)->visible);uint8_t *p=lookup(p6_ring_paint[j])->line;assert(p[0]==p6_impact_light(v)&&p[1]==(66*p[0]+127)/255);}
  assert(lookup("v6ripples")->m[4]>=942&&lookup("v6ripples")->m[4]<=1106);
  assert(!memcmp(drop,lookup("v6drop")->m,sizeof drop));
 }
 unsigned seen=0;held=0;
 for(int v=0;v<=32767;v+=127){
  knobs[3]=(int16_t)v;SCREEN_ploc.update(0,16);int pose=(v*11+16383)/32767;
  assert(last_pose==pose);seen|=1u<<pose;
  for(int j=0;j<6;j++){
   char id[16];snprintf(id,sizeof id,j<4?"v6handle%d":"v6cap%d",j<4?j:j-4);
   host_node *n=lookup(id);const int32_t *a=p6_handle_pose[pose][j];
   assert(n->m[0]==a[0]&&n->m[1]==a[1]&&n->m[3]==a[2]&&n->m[4]==a[3]&&n->m[6]==a[4]&&n->m[7]==a[5]);
   assert(n->visible==(j!=4||p6_cap0_visible[pose]));
  }
 }
 assert(seen==0xfff);knobs[3]=0;SCREEN_ploc.update(0,16);assert(lookup("v6needle")->m[0]>1000);knobs[3]=32767;SCREEN_ploc.update(0,16);assert(lookup("v6needle")->m[0]<-1000);
 for(int size=0;size<32768;size+=257)for(int height=0;height<16;height++){
  knobs[0]=(int16_t)size;ploc4_set(knobs,P4_POUIC,(unsigned)height);SCREEN_ploc.update(0,16);
  int32_t *m=lookup("v6drop")->m;int cy=((P6_DROP_Y*m[4])>>10)+m[7],half=(P6_DROP_H*m[4])/2048;
  assert(cy-half>=70*1024&&cy+half<=P6_RING_Y);
 }
 knobs[0]=15000;ploc4_set(knobs,P4_POUIC,0);held=1;op1_eng_page[0]=0x81;SCREEN_ploc.update(0,16);int low=lookup("v6drop")->m[7];
 assert(!strcmp(strings[1],"height")&&!strcmp(strings[4],"00"));ploc4_set(knobs,P4_POUIC,15);SCREEN_ploc.update(0,16);assert(lookup("v6drop")->m[7]<low&&!strcmp(strings[4],"99"));
 op1_eng_page[0]=0x82;knobs[2]=32767;SCREEN_ploc.update(0,16);assert(quality_drawn&&!strcmp(strings[T_QUALITY],"quality")&&!strcmp(strings[5],"99"));
 op1_eng_page[0]=0x83;SCREEN_ploc.update(0,16);assert(!strcmp(strings[3],"impact"));
 op1_eng_page[0]=0x80;SCREEN_ploc.update(0,16);assert(!strcmp(strings[0],"flow"));
#ifdef PLOC_STANDALONE
 held=0;op1_eng_page[0]=0x83;op1_eng_page[2]=PLOC6_TOKEN|39;
 SCREEN_ploc.update(0,16);
 assert(!op1_eng_page[0]&&!op1_eng_page[2]&&!strcmp(strings[3],"ripple"));
 held=1;SCREEN_ploc.update(0,16);assert(!strcmp(strings[3],"ripple"));
#endif
 held=0;SCREEN_ploc.update(0,16);assert(!quality_drawn&&!strcmp(strings[1],"size")&&!strcmp(strings[2],"spray")&&!strcmp(strings[3],"ripple"));
 knobs[5]=3;SCREEN_ploc.update(0,16);assert(lookup("ploclegacy")->visible&&!lookup("v6live")->visible&&legacy_inits==1&&legacy_draws==1);
 ploc4_defaults(knobs);SCREEN_ploc.update(0,16);assert(!lookup("ploclegacy")->visible&&lookup("v6live")->visible);
 for(int i=0;i<10;i++){SCREEN_ploc.init(0);SCREEN_ploc.update(0,16);}assert(constructors==N_TEXT);
 missing=1;SCREEN_ploc.init(0);SCREEN_ploc.update(0,16);assert(constructors==N_TEXT);
 puts("SCREEN6: native label colours and bounds, 12 linked handle poses, independent 1..4 rings, slight impact perspective, 20..100% clean, seven persistent spray ghosts/lights, bounded size/height, 3 Shift labels, idle unchanged, legacy and missing-node fallback");
}
