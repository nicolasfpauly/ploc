/* PLOC 0.6: FLOW / SIZE / SPRAY / RIPPLE, three momentary Shift controls.
 * Stored words retain the 0.4 ABI, so existing presets need no re-import. */
#include "op1_feature.h"
#include "../ploc/ploc6_link.h"
static int accumulator[4],was_shift[4],last_slot=-1;
#ifndef PLOCCTRL_HOST
static void save_words(int slot,const int16_t *p){
    ((void (*)(void *,const int16_t *))0x01A00D14)((void *)(0x01DDD7E8+(slot&7)*0x38),p);
}
#endif
static void clear_state(void) {for(int k=0;k<4;k++){accumulator[k]=0;was_shift[k]=0;}}
static void clear_edit(void) {
    op1_eng_page[0]=op1_eng_page[1]=0;
    clear_state();
}
/* kennel change (2026-11-21): op1_eng_page is every engine's page flags, so they are PLOC's only while the slot shows PLOC.
 * key_sync runs on every key event; ungated, each release closed another engine's blue page 2 (MONOLIT, AMP). */
static int ploc_shown(void) {return p6_linked(op1_synth_knobs(p4_slot()));}
static void key_sync(void) {if(ploc_shown()&&!p5_shift_held())op1_eng_page[0]=0;}
/* kennel change (2026-11-21): once the slot no longer shows PLOC, PLOC's own marks are cleared, nothing else: blue's
 * 0x80|knob (Shift gesture), white's and red's PLOC6_TOKEN|order (the screen's and this file's tokens; the old screen's
 * 0x80|order). The core's flags are 0 or 1 (a push toggles bit 0), so another engine's page 2 is never touched. */
static void drop_own(void) {
    if(op1_eng_page[0]&0x80)op1_eng_page[0]=0;
    for(int k=2;k<4;k++)if(op1_eng_page[k]&0xC0)op1_eng_page[k]=0;
}
static int input(const op1_input *in) {
    int slot=p4_slot();const int16_t *raw=op1_synth_knobs(slot);
    if(in->screen!=0x14||p4_drum_mode()||!p6_linked(raw))return 0;
    op1_eng_page[3]=(uint8_t)(PLOC6_TOKEN|(raw[4]-OP1_ENGINE_MAGIC));
    if(last_slot!=slot){clear_edit();last_slot=slot;}
    if(!in->shift)op1_eng_page[0]=0;
    if(in->type!=OP1_IN_KNOB||in->code>3)return 0;
    int k=in->code,shift=in->shift!=0;int16_t words[8];
    for(int j=0;j<8;j++)words[j]=raw[j];
    if(was_shift[k]!=shift){accumulator[k]=0;was_shift[k]=shift;}
    /* Consume the unused blue Shift gesture so the core cannot turn SIZE. */
    if(shift&&k==0){op1_eng_page[0]=0;accumulator[0]=0;return 1;}
    if(shift)op1_eng_page[0]=(uint8_t)(0x80|k);
    if((shift&&(k==1||k==3))||(!shift&&k==2)) {
        int extra=k==1?P4_POUIC:(k==3?P4_IMPACT:P4_SPRAY);
        accumulator[k]+=in->delta;int steps=accumulator[k]/1024;
        if(!steps)return 1;accumulator[k]-=steps*1024;
        int old=(int)ploc4_get(raw,extra),value=old+steps;
        if(value<0)value=0;if(value>15)value=15;
        if(value==old)return 1;
        ploc4_set(words,extra,(unsigned)value);
    }else {
        /* Shift+white addresses the old VISC word, now called QUALITY. */
        static const uint8_t main_word[4]={3,0,2,1};
        int idx=main_word[k],old=raw[idx],value=old+in->delta;
        if(value<0)value=0;if(value>32767)value=32767;
        if(value==old)return 1;words[idx]=(int16_t)value;
    }
    int16_t *live=(int16_t *)raw;for(int j=0;j<8;j++)live[j]=words[j];
    save_words(slot,words);return 1;
}
static void event(int id,int32_t arg) {
    (void)arg;
    if(id==OP1_EV_MODE||id==OP1_EV_SLOT||id==OP1_EV_BOOT_DONE) {
        clear_state();last_slot=-1;   /* kennel: the page flags only while PLOC is shown; another engine's pages stay */
        if(ploc_shown())op1_eng_page[0]=op1_eng_page[1]=op1_eng_page[2]=op1_eng_page[3]=0;
        else drop_own();
    }else if(id==OP1_EV_SERVICE) {
        const int16_t *p=op1_synth_knobs(p4_slot());
        if(p6_linked(p)){op1_eng_page[3]=(uint8_t)(PLOC6_TOKEN|(p[4]-OP1_ENGINE_MAGIC));key_sync();}
        else drop_own();   /* kennel: PLOC left (another engine or slot): its marks go, not the new engine's flags */
    }
}
static const char *const claims[]={"key:synth:ploc-editor",0};
const op1_feature_t FEATURE_plocctrl={.api=OP1_FEATURE_API,.priority=80,.name="plocctrl",.input=input,.event=event,.key_sync=key_sync,.claims=claims};
