/* PLOC 0.6: final Figma vectors, native R246 text, retained GUI only.
 * The audio engine and preset storage remain unchanged. */
#include "op1_plugin.h"
#include "seq/seq_text.h"
#include "ploc_text.h"
#include "ploc6_link.h"
#include "ploc6_geom.h"
#include "ploc5_surface.h"
extern const op1_screen_t SCREEN_ploc_legacy;
enum {N_STATIC,N_LIVE,N_DROP,N_NEEDLE,N_RIPPLES,N_MAIN};
static void *node[N_MAIN],*ring[4],*handle[4],*cap[2],*legacy;
static uint8_t *spray_paint[7],*ring_paint[4],*clean_paint[P6_CLEAN_PAINTS];
enum {T_FLOW,T_SIZE,T_SPRAY,T_RIPPLE,T_SIZE_VALUE,T_SPRAY_VALUE,T_RIPPLE_VALUE,T_QUALITY,N_TEXT};
static uint32_t p6_text[N_TEXT][PLOC_TEXT_SIZE/4];
static uint8_t made;
static int display_mode=-1,last_active=-2,last_value[3],last[7],last_pose=-1;
static const int32_t identity[9]={1024,0,0,0,1024,0,0,0,1024};
static const uint8_t colors[7][4]={{99,139,255,255},{0,237,149,255},{255,255,255,255},{255,66,63,255},{0,237,149,255},{255,255,255,255},{255,66,63,255}};
static void build_text(void){
 if(made)return;
 /* Stock text is larger than the browser proof. Keep measured headroom
  * around the label rows and above the lower edge of the visible OLED. */
 static const int xq[N_TEXT]={OP1_Q10(24),OP1_Q10(26.34),OP1_Q10(240.33),OP1_Q10(240.33),OP1_Q10(26.34),OP1_Q10(240.33),OP1_Q10(240.33),OP1_Q10(240.33)};
 static const int y[N_TEXT]={22,83,18,83,108,43,108,20},size[N_TEXT]={20,19,19,19,32,32,32,16};
 for(int i=0;i<N_TEXT;i++)ploc_text_construct(p6_text[i],xq[i],y[i],size[i],colors[i==T_QUALITY?T_SPRAY:i]);
 ploc_text_set(p6_text[T_SPRAY],"spray");
 ploc_text_set(p6_text[T_QUALITY],"quality");
 made=1;
}
static void visible(void *n,int v){if(n)op1_node_set_visible(n,v);}
static void shade(uint8_t *paint,const uint8_t *base,int brightness){
 if(!paint)return;
 for(int k=0;k<3;k++)paint[k]=(uint8_t)((base[k]*brightness+127)/255);
 paint[3]=255;
}
static void numbered(void *screen,void **to,int length,const char *prefix,int prefix_len){
 for(int k=0;k<length;k++){
  char id[24];int j=0;for(;j<prefix_len;j++)id[j]=prefix[j];
  if(k>=10)id[j++]=(char)('0'+k/10);id[j++]=(char)('0'+k%10);id[j]=0;
  to[k]=op1_find_node(screen,id);
 }
}
static void init(void *screen){
 static const char *const ids[N_MAIN]={"v6static","v6live","v6drop","v6needle","v6ripples"};
 for(int i=0;i<N_MAIN;i++)node[i]=op1_find_node(screen,ids[i]);
 numbered(screen,ring,4,"v6ring",6);numbered(screen,handle,4,"v6handle",8);numbered(screen,cap,2,"v6cap",5);
 for(int i=0;i<7;i++)spray_paint[i]=op1_node_line_rgba(screen,p6_spray_paint[i]);
 for(int i=0;i<4;i++)ring_paint[i]=op1_node_line_rgba(screen,p6_ring_paint[i]);
 for(int i=0;i<P6_CLEAN_PAINTS;i++)clean_paint[i]=p6_clean_kind[i]=='f'?op1_node_fill_rgba(screen,p6_clean_paint[i]):op1_node_line_rgba(screen,p6_clean_paint[i]);
 legacy=op1_find_node(screen,"ploclegacy");display_mode=-1;last_active=-2;last_pose=-1;
 for(int i=0;i<7;i++)last[i]=-1;for(int i=0;i<3;i++)last_value[i]=-1;
 build_text();
}
static void needle_pose(int value){
 if(!node[N_NEEDLE])return;
 unsigned a=((unsigned)value*4096u)/32767u;
 int sn=OP1_SINE_TABLE[a&8191u]/32,cs=OP1_SINE_TABLE[(a+2048u)&8191u]/32;
 int cx=P6_NEEDLE_X,cy=P6_NEEDLE_Y;
 int32_t m[9]={cs,sn,0,-sn,cs,0,cx-((cx*cs-cy*sn)>>10),cy-((cx*sn+cy*cs)>>10),1024};
 op1_node_set_transform(node[N_NEEDLE],m);
}
static void handle_pose(int value){
 int pose=(value*11+16383)/32767;
 if(pose==last_pose)return;
 for(int i=0;i<6;i++){
  void *n=i<4?handle[i]:cap[i-4];if(!n)continue;
  const int32_t *a=p6_handle_pose[pose][i];
  const int32_t m[9]={a[0],a[1],0,a[2],a[3],0,a[4],a[5],1024};
  op1_node_set_transform(n,m);
 }
 visible(cap[0],p6_cap0_visible[pose]);last_pose=pose;
}
static void drop_pose(int size,int height){
 if(!node[N_DROP])return;
 /* A raised drop passes behind the opaque front of the faucet, like Figma. */
 int scale=128+(size*1024+16383)/32767,half=(P6_DROP_H*scale)/2048;
 int high=P6_DROP_Y,low=P6_RING_Y-half-OP1_Q10(1);
 if(high>low)high=low;
 int cy=low-((low-high)*height+7)/15;
 int32_t m[9]={scale,0,0,0,scale,0,P6_DROP_X-((P6_DROP_X*scale)>>10),cy-((P6_DROP_Y*scale)>>10),1024};
 op1_node_set_transform(node[N_DROP],m);
}
static void impact_pose(int value){
 if(!node[N_RIPPLES])return;
 /* Only +/-8% depth; impact never changes the number of rings. */
 int sy=942+(value*164+7)/15;
 int32_t m[9]={1024,0,0,0,sy,0,0,P6_RING_Y-((P6_RING_Y*sy)>>10),1024};
 op1_node_set_transform(node[N_RIPPLES],m);
}
static void update(void *screen,int dt){
 const int16_t *raw=op1_synth_knobs(OP1_SCREEN_SLOT);int modern=ploc4_is(raw);
 if(display_mode!=modern){
  visible(node[N_STATIC],modern);visible(node[N_LIVE],modern);visible(legacy,!modern);
  if(!modern)SCREEN_ploc_legacy.init(screen);
  for(int i=0;i<7;i++)last[i]=-1;for(int i=0;i<3;i++)last_value[i]=-1;
  last_active=-2;display_mode=modern;
 }
 if(!modern){op1_eng_page[2]=op1_eng_page[3]=0;SCREEN_ploc_legacy.update(screen,dt);return;}
#ifdef PLOC_STANDALONE
 /* No companion handshake: an older installed PLOCCTRL must stay idle. */
 op1_eng_page[2]=0;
 if(!p5_shift_held())op1_eng_page[0]=0;
#else
 int order=raw[4]-OP1_ENGINE_MAGIC;
 op1_eng_page[2]=order>=1&&order<=OP1_MAX_ORDER?(uint8_t)(PLOC6_TOKEN|order):0;
#endif
 int active=p5_shift_held()&&(op1_eng_page[0]&0x80)?op1_eng_page[0]&3:-1;
 if(active==0)active=-1;
 int16_t live[4];sq_synth_knobs_live(OP1_SCREEN_SLOT,live);
 int v[7];for(int i=0;i<4;i++)v[i]=p6_clamp(live[i]);
 v[4]=(int)ploc4_get(raw,P4_POUIC);v[5]=(int)ploc4_get(raw,P4_SPRAY);v[6]=(int)ploc4_get(raw,P4_IMPACT);
 if(v[0]!=last[0]||v[4]!=last[4])drop_pose(v[0],v[4]);
 if(v[3]!=last[3]){
  needle_pose(v[3]);handle_pose(v[3]);
 }
 if(v[1]!=last[1]){int count=(int)p5_ripple_count(v[1]);for(int i=0;i<4;i++)visible(ring[i],i<count);}
 if(v[6]!=last[6]){
  impact_pose(v[6]);for(int i=0;i<4;i++)shade(ring_paint[i],colors[3],p6_impact_light(v[6]));
 }
 if(v[5]!=last[5]){
  int count=p6_spray_count(v[5]);for(int i=0;i<7;i++)shade(spray_paint[i],colors[2],i<count?255:51);
 }
 if(v[2]!=last[2])for(int i=0;i<P6_CLEAN_PAINTS;i++)shade(clean_paint[i],colors[2],p6_quality_light(v[2]));
 for(int i=0;i<7;i++)last[i]=v[i];
 if(active!=last_active){
  static const char *const main_label[4]={"flow","size","spray","ripple"};
  static const char *const shift_label[4]={"flow","height","spray","impact"};
  for(int i=0;i<4;i++)if(i!=T_SPRAY)ploc_text_set(p6_text[i],active==i?shift_label[i]:main_label[i]);
  last_active=active;
 }
 int values[3]={active==1?(v[4]*99+7)/15:p6_percent(v[0]),active==2?p6_percent(v[2]):(v[5]*99+7)/15,active==3?(v[6]*99+7)/15:p6_percent(v[1])};
 for(int i=0;i<3;i++)if(values[i]!=last_value[i]){
  char out[3]={(char)('0'+values[i]/10),(char)('0'+values[i]%10),0};
  ploc_text_set(p6_text[4+i],out);last_value[i]=values[i];
 }
 if(node[N_LIVE])op1_screen_draw_cached(screen,dt,node[N_LIVE]);else op1_screen_draw(screen,dt);
 void *r=op1_raster();
 for(int i=0;i<T_QUALITY;i++)ploc_text_draw(p6_text[i==T_SPRAY&&active==2?T_QUALITY:i],r,identity);
}
const op1_screen_t SCREEN_ploc={init,update};
