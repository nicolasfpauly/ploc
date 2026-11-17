#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#define OP1_PLUGIN_H
#define OP1_VOICES 6
#define OP1_BLOCK 128
typedef struct {uint32_t word[9];} native_voice;
static native_voice memory[6];static int16_t sine[8192];static int32_t pitch[128];
#define OP1_SINE_TABLE sine
#define OP1_PITCH_INC pitch
static int op1_voice_index(const void *v){return (int)((const native_voice *)v-memory);}
static int32_t op1_voice_vel_gain(const void *v){(void)v;return 32767;}
typedef struct {void (*note_on)(void *,const int16_t *);void (*render)(void *,int16_t *,const int16_t *);void (*note_off)(void *);uint8_t press[4];} op1_engine_t;
#include "../plugins/ploc/ploc.c"
int main(void){
    for(int i=0;i<8192;i++)sine[i]=(int16_t)lround(sin(i*6.283185307179586/8192)*32767);
    for(int i=0;i<128;i++)pitch[i]=p4_hz_inc(40);
    int16_t p[8]={30000,32767,0,32767,0x5330,0,0,0};ploc4_defaults(p);ploc4_set(p,P4_TAIL,15);
    for(int v=0;v<6;v++){memory[v].word[0]=0xaabb0000u+(unsigned)v;ENGINE_ploc.note_on(&memory[v],p);}
    int peak=0;
    for(int b=0;b<1200;b++){
        int sum[128]={0};if(b==400)for(int v=0;v<6;v++)ENGINE_ploc.note_off(&memory[v]);
        /* Retrigger one voice while five old tails still ring. */
        if(b==401)ENGINE_ploc.note_on(&memory[0],p);
        for(int v=0;v<6;v++){
            int16_t out[128];ENGINE_ploc.render(&memory[v],out,p);
            for(int n=0;n<128;n++)sum[n]+=out[n];
            assert(memory[v].word[0]==0xaabb0000u+(unsigned)v);
        }
        for(int n=0;n<128;n++){int a=sum[n]<0?-sum[n]:sum[n];if(a>peak)peak=a;assert(a<=32767);}
    }
    ENGINE_ploc.note_off(&memory[0]);
    /* The same adapter still selects the old native model exactly. */
    p[5]=3;p[6]=p[7]=0;ploc_model_state reference;
    ploc_model_start_seed(&reference,p,ploc_hits);ENGINE_ploc.note_on(&memory[0],p);
    for(int b=0;b<100;b++){
        int16_t a[128],ref[128];ENGINE_ploc.render(&memory[0],a,p);
        ploc_model_process(&reference,ref,pitch,p,32767,sine,128);assert(!memcmp(a,ref,sizeof a));
    }
    assert(sizeof(ploc_voice)==36);printf("ADAPTER: six native objects, vtables preserved, release/retrigger headroom peak=%d, legacy render sample-exact\n",peak);
    return 0;
}
