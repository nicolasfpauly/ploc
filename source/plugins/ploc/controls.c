/* PLOC's T1 controls via Kennel 3.3's per-engine input hook.
 * GPL-3.0-or-later with the firmware permission, see NOTICE. */
#include "op1_plugin.h"
#include "ploc4_link.h"
#include "ploc6_link.h"

extern const op1_engine_ext2_t ENGINE_ploc;
static int32_t accum[4];
static int last_slot=-1,last_shift=-1;

int ploc_input(int page,int ev,int k,int delta)
{
    if(page!=OP1_ENG_T1||ev!=OP1_ENG_TURN||(unsigned)k>=4)return 0;
    int slot=p4_slot();
    const int16_t *raw=op1_synth_knobs(slot);
    if(!ploc4_is(raw))return 0; /* Legacy sounds keep their original mapping. */
    int shift=op1_shift_held();
    if(slot!=last_slot||shift!=last_shift){
        for(int i=0;i<4;i++)accum[i]=0;
        last_slot=slot;last_shift=shift;
    }
    op1_eng_page[0]=shift&&k?(uint8_t)(0x80|k):0;
    if(shift&&k==0)return 1;

    const op1_engine_t *voice=&ENGINE_ploc.ext.voice;
    if((!shift&&k==2)||(shift&&(k==1||k==3))){
        int macro=k==1?P4_POUIC:k==3?P4_IMPACT:P4_SPRAY;
        int steps=op1_eng_detents(&accum[k],delta);
        if(!steps)return 1;
        int old=(int)ploc4_get(raw,macro),v=old+steps;
        if(v<0)v=0;
        if(v>15)v=15;
        if(v==old)return 1;
        int16_t words[8];
        for(int i=4;i<8;i++)words[i]=raw[i];
        ploc4_set(words,macro,(unsigned)v);
        /* These three macros each occupy one word. Persist changed words
         * through the SDK, preserving selector, effects and all other macros. */
        for(int i=5;i<8;i++)
            if(words[i]!=raw[i])op1_engine_word_set(voice,i,words[i]);
    }else{
        static const unsigned char map[4]={3,0,2,1};
        int word=map[k],old=raw[word],v=p6_clamp(old+delta);
        if(v!=old)op1_engine_word_set(voice,word,v);
    }
    return 1;
}
