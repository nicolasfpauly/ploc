/* PLOC 0.3, Nicolas's SIZE / RIPPLE / VISC / DROPS sketch.
 * Native R246 text, retained SVG nodes, GUI-only updates.
 * GPL-3.0-or-later with firmware permission, see NOTICE.
 */
#include "op1_plugin.h"
#include "seq/seq_text.h"
#include "ploc_text.h"
#include "ploc_params.h"
#include "ploc4_params.h"
extern volatile uint32_t ploc_hits;
static void *drop,*gloss,*ripple,*bar,*extra[5],*counter[5];
static uint32_t seen;
static int frame=24,previous_count=-1,display_mode=-1;
static uint32_t text[8][PLOC_TEXT_SIZE/4];
static int shown[4]={-1,-1,-1,-1};
static uint8_t made;
static unsigned flow_frame;
static const uint8_t color[4][4]={{105,142,255,255},{0,237,149,255},{255,255,255,255},{255,66,63,255}};
static const int32_t identity[9]={1024,0,0,0,1024,0,0,0,1024};
static void build_text(void) {
    if(made)return;
    static const int x[8]={10,10,235,285,10,10,238,298};
    static const int y[8]={19,44,15,35,120,100,62,80};
    for(int i=0;i<8;i++)ploc_text_construct(text[i],OP1_Q10(x[i]),y[i],i==1?49:17,color[i/2]);
    made=1;
}
static void init(void *s) {
    drop=op1_find_node(s,"drop");gloss=op1_find_node(s,"gloss");ripple=op1_find_node(s,"ripple");bar=op1_find_node(s,"viscbar");
    for(int j=0;j<5;j++) {
        char name[7]={'e','x','t','r','a',(char)('0'+j),0};extra[j]=op1_find_node(s,name);
        name[0]='c';name[1]='o';name[2]='u';name[3]='n';name[4]='t';counter[j]=op1_find_node(s,name);
    }
    seen=ploc_hits;frame=24;previous_count=-1;build_text();
}
static void transform(void *node,int sx,int sy,int cxq,int cyq,int dxq,int dyq) {
    if(!node)return;
    int32_t m[9]={sx,0,0,0,sy,0,cxq-((cxq>>2)*sx>>8)+dxq,cyq-((cyq>>2)*sy>>8)+dyq,1024};
    op1_node_set_transform(node,m);
}
static void update(void *s,int dt) {
    int16_t p[4];sq_synth_knobs_live(OP1_SCREEN_SLOT,p);build_text();
    const int16_t *raw=op1_synth_knobs(OP1_SCREEN_SLOT);
    int modern=ploc4_is(raw),model=modern?4:raw[5],current=model==PLOC_CURRENT_MODEL;
    int order=raw[4]-0x5300;
    if(modern)op1_eng_page[2]=(uint8_t)(0x80|(order&63));
    else op1_eng_page[2]=op1_eng_page[3]=0;
    int page=modern?((op1_eng_page[0]&1)|((op1_eng_page[1]&1)<<1)):0;
    int mode=model*4+page;
    if(mode!=display_mode) {
        static const char *const labels[4][4]={{"size","ripple","visc","flow"},
            {"pouic","impact","tail","spray"},{"spread","chaos","tone","dist"},{"width","tune","",""}};
        if(modern)for(int k=0;k<4;k++)ploc_text_set(text[2*k],labels[page][k]);
        else {
            ploc_text_set(text[0],"size");ploc_text_set(text[2],current?"ripple":"body");
            ploc_text_set(text[4],current?"visc":"tail");ploc_text_set(text[6],current?"drops":"tone");
        }
        display_mode=mode;previous_count=-1;for(int i=0;i<4;i++)shown[i]=-1;
    }
    if(seen!=ploc_hits){seen=ploc_hits;frame=0;}else if(frame<24)frame++;
    int size=720+((p[0]*1000)>>15),thick=(current||modern)?900+((p[2]*240)>>15):1024;
    if(drop)op1_node_set_visible(drop,frame>=3);
    if(gloss)op1_node_set_visible(gloss,frame>=3);
    transform(drop,size,thick,OP1_Q10(160),OP1_Q10(74),0,0);
    transform(gloss,size,thick,OP1_Q10(160),OP1_Q10(74),0,0);
    int spread=480+((p[1]*700)>>15)+(frame<24?frame*3:0);
    transform(ripple,spread,1024,OP1_Q10(160),OP1_Q10(127),0,0);
    if(bar)op1_node_set_visible(bar,p[2]>0);
    transform(bar,1+(p[2]>>5),1024,OP1_Q10(10),OP1_Q10(144),0,0);
    int amount=modern?(p[3]>0?1+((p[3]*4)>>15):0):current?ploc_drops(p[3]):(model==2&&p[1]>0?2:0);
    if(amount!=previous_count) {
        for(int j=0;j<5;j++) {
            if(extra[j])op1_node_set_visible(extra[j],j<amount);
            if(counter[j])op1_node_set_visible(counter[j],(current||modern)&&j<amount);
        }
        previous_count=amount;
    }
    for(int j=0;j<amount;j++) {
        int drift=frame<24?(frame*(j&1?1:-1))/6:0;
        if(modern&&p[3]>0)drift=((int)((flow_frame+j*7u)&15u)-7)*(j&1?1:-1)/3;
        transform(extra[j],1024,1024,0,0,OP1_Q10(drift),OP1_Q10(frame<24?-(frame/8):0));
    }
    op1_screen_draw(s,dt);
    void *r=op1_raster();
    for(int i=0;i<4;i++) {
        int extra=(page-1)*4+i;
        int v=modern&&page?(extra<P4_EXTRAS?(int)ploc4_get(raw,extra):-2):current&&i==3?amount:(p[i]*99+16383)/32767;
        if(v!=shown[i]) {
            char t[3]={(char)('0'+v/10),(char)('0'+v%10),0};
            ploc_text_set(text[2*i+1],v<0?"":current&&i==3?t+1:t);shown[i]=v;
        }
        ploc_text_draw(text[2*i],r,identity);ploc_text_draw(text[2*i+1],r,identity);
    }
    flow_frame++;
    if(modern)for(int k=0;k<4;k++) {
        static const uint8_t dim[4]={65,65,65,255},ready[4]={255,255,255,255},waiting[4]={255,66,63,255};
        op1_pt a={OP1_Q10(248+k*14),OP1_Q10(151)},b={OP1_Q10(248+k*14+(k==page?6:2)),OP1_Q10(151)};
        const uint8_t *c=k==page?(op1_eng_page[3]==(uint8_t)(0x40|(order&63))?ready:waiting):dim;
        op1_raster_line(r,&a,&b,OP1_Q10(k==page?2:1),c);
    }
}
const op1_screen_t SCREEN_ploc_legacy={init,update};
