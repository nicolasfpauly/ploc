/* PLOC 0.2: numerical port, memory and headroom checks. GPL-3.0-or-later. */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../plugins/ploc/ploc_dsp.h"
static int16_t sine[8192];
static int32_t pitch[128];
static uint32_t increment(double f) { return (uint32_t)(f/44100.0*4294967296.0); }
static void setpitch(double f) { for(int n=0;n<128;n++) pitch[n]=(int32_t)increment(f); }
static double energy(int tail) {
    ploc_state s;ploc_start(&s);int16_t p[8]={16384,18000,tail,21000,0x5300,2,0,0},o[128];double e=0;
    setpitch(440);
    for(int b=0;b<2100;b++) {
        ploc_process(&s,o,pitch,p,32767,sine,128);
        for(int n=0;n<128;n++) e+=(double)o[n]*o[n];
    }
    for(int j=0;j<3;j++) assert(s.amp[j]==0);
    assert(s.lp==0);return e;
}
int main(void) {
    struct { uint32_t vtable; ploc_model_state state; } v;
    _Static_assert(sizeof(v)==36,"exact SDK voice size");
    for(int i=0;i<8192;i++) sine[i]=(int16_t)lrint(sin(i*6.283185307179586/8192)*32767);
    double worst=0;
    for(int f=80;f<=8000;f+=7) {
        uint32_t d=ploc_decay(increment(f),32768);
        double ref=(.043*f+.0014*pow(f,1.5))*1048576/44100;
        double err=fabs(d-ref)/ref;if(err>worst)worst=err;
        assert(err<.04);
    }
    assert(energy(32767)>energy(0)*8);
    int peak=0;
    for(int c=0;c<16;c++) for(int note=0;note<128;note++) {
        int16_t p[8]={0,0,0,0,0x5300,2,0,0},o[128];
        for(int j=0;j<4;j++) p[j]=(c&(1<<j))?32767:0;
        memset(&v,0xa5,sizeof(v));v.vtable=0xdeadbeefu;ploc_model_start(&v.state,p);
        setpitch(440*pow(2,(note-69)/12.0));
        for(int b=0;b<180;b++) {
            ploc_model_process(&v.state,o,pitch,p,32767,sine,128);
            assert(v.vtable==0xdeadbeefu);
            for(int n=0;n<128;n++) { int a=abs(o[n]);if(a>peak)peak=a;assert(a<=21844); }
            for(int j=0;j<3;j++) assert(v.state.bubble.amp[j]>=0&&v.state.bubble.amp[j]<=(1<<24));
        }
    }
    /* Old saved patches use the unmodified 0.1 kernel, sample for sample. */
    int16_t p[8]={10600,19000,15000,21000,0x5300,0,0,0},a[128],b[128];
    ploc_legacy_state old;ploc_legacy_start(&old);ploc_model_start(&v.state,p);setpitch(440);
    for(int k=0;k<250;k++) {
        ploc_legacy_process(&old,a,pitch,p,28000,sine,128);
        ploc_model_process(&v.state,b,pitch,p,28000,sine,128);
        assert(memcmp(a,b,sizeof(a))==0);
    }
    /* Compare the audible single-bubble port to the analytic study, including
     * its 6ms offset and 0.3ms attack; TONE=max is nearly a filter bypass. */
    ploc_state s;ploc_start(&s);setpitch(440);
    int16_t q[8]={16384,0,8192,32767,0x5300,2,0,0};
    double diff=0,refe=0;int sample=0;
    for(int k=0;k<180;k++) {
        ploc_process(&s,a,pitch,q,32767,sine,128);
        for(int n=0;n<128;n++,sample++) {
            double t=(sample-PLOC_PRIMARY_ONSET)/44100.0, ref=0;
            if(t>=0) {
                double d=.043*440+.0014*pow(440,1.5);
                ref=(1-exp(-t/.0003))*exp(-d*t)*sin(6.283185307179586*440*(t+.5*.1*d*t*t))*32767*4/9;
            }
            double e=a[n]-ref;diff+=e*e;refe+=ref*ref;
        }
    }
    double relative=sqrt(diff/refe);assert(relative<.08);
    printf("PASS: 2048 pitch/control corners, per-voice peak %d, 36-byte voice/vtable, old preset equivalence, decay silence.\n",peak);
    printf("Physical damping worst error %.3f%% (80..8000 Hz); single-bubble reference relative RMS error %.3f%%. ASan/UBSan.\n",worst*100,relative*100);
    return 0;
}
