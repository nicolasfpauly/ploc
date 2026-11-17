/* Model 3 behaviour, bounds and pitch identity. GPL-3.0-or-later. */
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../plugins/ploc/ploc_dsp.h"
#define N 44100
static int16_t sine[8192],a[N],b[N];
static int32_t pitch[128];
static void hz(double f){for(int i=0;i<128;i++)pitch[i]=(int32_t)(4294967296.0*f/44100);}
static void render(int16_t *out,int length,int16_t *p,unsigned seed){
    ploc_model_state s;ploc_model_start_seed(&s,p,seed);
    for(int i=0;i<length;i+=128){int16_t block[128];ploc_model_process(&s,block,pitch,p,32767,sine,128);for(int j=0;j<128&&i+j<length;j++)out[i+j]=block[j];}
}
static double late_energy(int16_t *x){double e=0;for(int i=17640;i<35280;i++)e+=(double)x[i]*x[i];return e;}
int main(void){
    _Static_assert(sizeof(ploc_model_state)==32,"shared voice storage");
    for(int i=0;i<8192;i++)sine[i]=(int16_t)lrint(32767*sin(6.283185307179586*i/8192));
    int last=-1;for(int v=0;v<32768;v++){int d=ploc_drops(v);assert(d>=0&&d<=5&&d>=last);last=d;}
    int16_t p[8]={16384,0,12288,0,0x5300,3,0,0};hz(440);render(a,N,p,0);
    for(int d=1;d<=5;d++){
        p[3]=(int16_t)((d*32768+5)/6);assert(ploc_drops(p[3])==d);render(b,N,p,0);
        int at=0;while(at<N&&a[at]==b[at])at++;
        int onset=(int)ploc3_onset(d,0);assert(at>=onset&&at<onset+256);
        memcpy(a,b,sizeof(a));
    }
    p[3]=0;p[1]=0;render(a,N,p,0);p[1]=32767;render(b,N,p,0);
    assert(late_energy(b)>late_energy(a)*2);
    /* Same primary, upward chirp in both liquids; oil slows the climb. */
    uint32_t growth[2];hz(110);p[1]=p[3]=0;
    for(int k=0;k<2;k++){
        p[2]=k?32767:0;ploc3_state s;ploc3_start(&s,0,ploc_drops(p[3]));uint32_t first=0,end=0;
        for(int block=0;block<100;block++){
            uint32_t old=s.phase[0];int16_t out[128];ploc3_process(&s,out,pitch,p,32767,sine,128);
            uint32_t step=s.phase[0]-old;if(block==8)first=step;if(block==90)end=step;
        }
        assert(end>first);growth[k]=end-first;
    }
    assert(growth[0]>growth[1]*2);
    p[0]=16384;p[1]=10000;p[2]=12288;p[3]=32767;hz(440);
    ploc3_state held,changed;ploc3_start(&held,1,5);ploc3_start(&changed,1,5);
    for(int k=0;k<100;k++){
        int16_t x[128],y[128];p[3]=32767;ploc3_process(&held,x,pitch,p,32767,sine,128);
        p[3]=k>5?0:32767;ploc3_process(&changed,y,pitch,p,32767,sine,128);assert(!memcmp(x,y,sizeof(x)));
    }
    int peak=0;uint32_t random=0x12345678u;
    for(int c=0;c<16;c++)for(int note=0;note<128;note++){
        struct {uint32_t before;ploc_model_state s;uint32_t after;} mem={0xdeadbeefu,{0},0xcafef00du};
        for(int j=0;j<4;j++)p[j]=(c&(1<<j))?32767:0;
        ploc_model_start_seed(&mem.s,p,note);hz(440*pow(2,(note-69)/12.0));
        for(int block=0;block<90;block++){
            int16_t out[128];ploc_model_process(&mem.s,out,pitch,p,32767,sine,128);
            assert(mem.before==0xdeadbeefu&&mem.after==0xcafef00du);
            for(int i=0;i<128;i++){int v=abs(out[i]);if(v>peak)peak=v;assert(v<28700);}
        }
    }
    /* Longest possible notes reach exact silence, including final satellite. */
    hz(8);p[0]=32767;p[1]=32767;p[2]=0;p[3]=32767;
    ploc3_state s;ploc3_start(&s,15,5);
    for(int block=0;block<3100;block++){
        int16_t out[128];ploc3_process(&s,out,pitch,p,32767,sine,128);
        if(block>2900)for(int i=0;i<128;i++)assert(out[i]==0);
    }
    assert(s.lp==0);
    /* Exercise live changes, all six bubbles and the body phase accumulators. */
    ploc3_start(&s,3,5);hz(660);
    for(int block=0;block<2000;block++){
        for(int j=0;j<4;j++){random^=random<<13;random^=random>>17;random^=random<<5;p[j]=(int16_t)(random&32767);}
        int16_t out[128];ploc3_process(&s,out,pitch,p,32767,sine,128);
        for(int i=0;i<128;i++)assert(abs(out[i])<28700);
    }
    printf("PASS model 3: all 32768 DROPS values, five independently audible onsets, RIPPLE bloom, upward pouic and slower oil glide, 2048 corners, live controls, silence and 32-byte state. Peak %d.\n",peak);
}
