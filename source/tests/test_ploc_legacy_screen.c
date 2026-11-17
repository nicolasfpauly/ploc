/* Native screen logic tests; host text can also emit the preview SVG. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define OP1_PLUGIN_H
#define PLOC_SCREEN_HOST
#define OP1_Q10(v) ((int32_t)((v)*1024))
#define OP1_SCREEN_SLOT 0
typedef struct {int32_t x,y;} op1_pt;
typedef struct {void (*init)(void *);void (*update)(void *,int);} op1_screen_t;
static int16_t knobs[8]={0,0,0,0,0x5300,3,0,0};
static int constructors,sets,draws,svg_draws,visibility_sets;
static int visible[14];
static int32_t matrices[14][9];
static int nodes[14];
static char strings[8][12];
static int text_x[8],text_y[8],text_size[8];
static int missing;
volatile uint32_t ploc_hits;
volatile uint8_t op1_eng_page[4];
void *op1_find_node(void *,const char *);
void op1_node_set_transform(void *,const int32_t *);
void op1_node_set_visible(void *,int);
void op1_screen_draw_cached(void *,int,void *);
void op1_screen_draw(void *,int);
void *op1_raster(void);
void op1_raster_line(void *,const op1_pt *,const op1_pt *,int32_t,const uint8_t *);
const int16_t *op1_synth_knobs(int);
#include "../plugins/ploc/screen_legacy.c"
void *op1_find_node(void *s,const char *id){
    (void)s;if(missing)return 0;int n;
    if(!strcmp(id,"drop"))n=0;else if(!strcmp(id,"gloss"))n=1;else if(!strcmp(id,"ripple"))n=2;else if(!strcmp(id,"viscbar"))n=3;
    else if(!strncmp(id,"extra",5))n=4+id[5]-'0';else {assert(!strncmp(id,"count",5));n=9+id[5]-'0';}
    assert(n>=0&&n<14);return &nodes[n];
}
void op1_node_set_transform(void *n,const int32_t *m){int i=*(int*)n;assert(m[8]==1024);assert(m[0]>=1&&m[0]<=1720);assert(m[4]>=512&&m[4]<=1720);memcpy(matrices[i],m,9*sizeof(int32_t));}
void op1_node_set_visible(void *n,int on){visible[*(int*)n]=on;visibility_sets++;}
void op1_screen_draw_cached(void *s,int dt,void *first){(void)s;(void)dt;assert(first==&nodes[0]);svg_draws++;}
void op1_screen_draw(void *s,int dt){(void)s;(void)dt;svg_draws++;}
void *op1_raster(void){return &nodes[0];}
void op1_raster_line(void *r,const op1_pt *a,const op1_pt *b,int32_t w,const uint8_t *c){assert(r&&a->x>=248*1024&&b->x<=300*1024&&a->y==151*1024&&b->y==151*1024);assert(w==1024||w==2048);assert(c[3]==255);}
void sq_synth_knobs_live(int slot,int16_t *p){assert(slot==0);memcpy(p,knobs,4*sizeof(int16_t));}
const int16_t *op1_synth_knobs(int slot){assert(slot==0);return knobs;}
void ploc_text_construct(void *o,int32_t x,int32_t y,int32_t size,const uint8_t *c){int n=(int)(((uint32_t*)o-&text[0][0])/(PLOC_TEXT_SIZE/4));text_x[n]=x;text_y[n]=y;text_size[n]=size;assert(x>=0&&x<=310*1024);assert(y>=0&&y<=146);assert(size==17||size==49);assert(c[3]==255);constructors++;}
void ploc_text_set(void *o,const char *s){int n=(int)(((uint32_t*)o-&text[0][0])/(PLOC_TEXT_SIZE/4));assert(n>=0&&n<8);assert(strlen(s)<12);strcpy(strings[n],s);sets++;}
void ploc_text_draw(void *o,void *r,const int32_t *m){(void)o;assert(r);assert(m[0]==1024&&m[4]==1024&&m[8]==1024);draws++;}
int main(int argc,char **argv){
    if(argc==2||argc==3){
        for(int i=0;i<14;i++){nodes[i]=i;visible[i]=1;matrices[i][0]=matrices[i][4]=matrices[i][8]=1024;}
        knobs[0]=16384;knobs[1]=12000;knobs[2]=12288;knobs[3]=(int16_t)atoi(argv[1]);
        if(argc==3){ploc4_defaults(knobs);int page=atoi(argv[2]);op1_eng_page[0]=page&1;op1_eng_page[1]=(page>>1)&1;}
        SCREEN_ploc_legacy.init(0);SCREEN_ploc_legacy.update(0,16);
        printf("{\"count\":%d,\"nodes\":[",shown[3]);
        for(int i=0;i<14;i++){
            if(i)printf(",");printf("{\"visible\":%d,\"matrix\":[",visible[i]);
            for(int j=0;j<9;j++){if(j)printf(",");printf("%d",matrices[i][j]);}printf("]}");
        }
        printf("],\"text\":[");
        for(int i=0;i<8;i++){if(i)printf(",");printf("{\"x\":%d,\"y\":%d,\"size\":%d,\"value\":\"%s\"}",text_x[i],text_y[i],text_size[i],strings[i]);}
        puts("]}");return 0;
    }

    for(int i=0;i<14;i++){nodes[i]=i;visible[i]=1;matrices[i][0]=matrices[i][4]=matrices[i][8]=1024;}
    SCREEN_ploc_legacy.init(0);assert(constructors==8);SCREEN_ploc_legacy.update(0,16);
    assert(!strcmp(strings[2],"ripple")&&!strcmp(strings[4],"visc")&&!strcmp(strings[6],"drops"));
    int initial=sets;for(int i=0;i<40;i++)SCREEN_ploc_legacy.update(0,16);assert(sets==initial&&visible[0]);
    for(int round=0;round<12;round++){SCREEN_ploc_legacy.init(0);SCREEN_ploc_legacy.update(0,16);}assert(constructors==8);
    /* Red changes immediately while idle, without a note-on, in both directions. */
    for(int v=0;v<32768;v+=31){
        knobs[3]=(int16_t)v;SCREEN_ploc_legacy.update(0,16);int n=ploc_drops(v);assert(shown[3]==n);
        for(int j=0;j<5;j++){assert(visible[4+j]==(j<n));assert(visible[9+j]==(j<n));}
    }
    knobs[3]=32767;SCREEN_ploc_legacy.update(0,16);assert(shown[3]==5);
    knobs[3]=0;SCREEN_ploc_legacy.update(0,16);for(int j=0;j<5;j++)assert(!visible[4+j]&&!visible[9+j]);
    for(int v=0;v<=99;v++){for(int k=0;k<3;k++)knobs[k]=(int16_t)((v*32767+49)/99);SCREEN_ploc_legacy.update(0,16);assert(shown[0]==v&&shown[1]==v&&shown[2]==v);}
    knobs[5]=2;SCREEN_ploc_legacy.update(0,16);assert(!strcmp(strings[2],"body")&&!strcmp(strings[4],"tail")&&!strcmp(strings[6],"tone"));
    knobs[5]=3;SCREEN_ploc_legacy.update(0,16);assert(!strcmp(strings[6],"drops"));
    ploc4_defaults(knobs);SCREEN_ploc_legacy.update(0,16);assert(!strcmp(strings[6],"flow"));assert(op1_eng_page[2]==0x80);
    for(int v=0;v<32768;v+=127){knobs[3]=(int16_t)v;SCREEN_ploc_legacy.update(0,16);assert(shown[3]==(v*99+16383)/32767);
        int count=v?1+(v*4>>15):0;for(int j=0;j<5;j++)assert(visible[4+j]==(j<count));}
    static const char *expected[3][4]={{"pouic","impact","tail","spray"},{"spread","chaos","tone","dist"},{"width","tune","",""}};
    for(int page=1;page<=3;page++){
        op1_eng_page[0]=page&1;op1_eng_page[1]=(page>>1)&1;SCREEN_ploc_legacy.update(0,16);
        for(int k=0;k<4;k++){assert(!strcmp(strings[k*2],expected[page-1][k]));int extra=(page-1)*4+k;
            if(extra<10){assert(shown[k]==(int)ploc4_get(knobs,extra));ploc4_set(knobs,extra,15);SCREEN_ploc_legacy.update(0,16);assert(shown[k]==15);}
            else assert(!strcmp(strings[k*2+1],""));}
    }
    op1_eng_page[0]=op1_eng_page[1]=0;
    ploc_hits++;SCREEN_ploc_legacy.update(0,16);assert(!visible[0]);for(int i=0;i<30;i++)SCREEN_ploc_legacy.update(0,16);assert(visible[0]);
    missing=1;SCREEN_ploc_legacy.init(0);SCREEN_ploc_legacy.update(0,16);assert(constructors==8);
    printf("PASS screen: legacy DROPS, modern FLOW 0..99, four pages/all ten secondary values, retained text, re-entry and absent nodes.\n");
}
