#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../plugins/ploc/ploc4_dsp.h"
static int16_t sine[8192];
static int32_t pitch[128];
static uint32_t random_word=17;
static unsigned rnd(void){random_word=random_word*1664525u+1013904223u;return random_word;}
static void codec(void) {
    int16_t p[8]={123,456,789,321,0x532a,0,0,0};unsigned expected[10];
    ploc4_defaults(p);assert(ploc4_is(p));
    for(int n=0;n<20000;n++) {
        for(int k=0;k<10;k++){expected[k]=(rnd()>>16)&15;ploc4_set(p,k,expected[k]);}
        for(int k=0;k<10;k++)assert(ploc4_get(p,k)==expected[k]);
        for(int k=0;k<8;k++)assert(p[k]>=0);
        assert(p[0]==123&&p[1]==456&&p[2]==789&&p[3]==321&&p[4]==0x532a);
        int k=(int)(rnd()%10),v=(int)(rnd()&15);ploc4_set(p,k,(unsigned)v);expected[k]=(unsigned)v;
        for(k=0;k<10;k++)assert(ploc4_get(p,k)==expected[k]);
    }
    for(int old=0;old<4;old++){p[5]=(int16_t)old;assert(!ploc4_is(p));}
}
static unsigned flow_events(int flow,int chaos) {
    ploc4_state s;int16_t p[8]={16384,0,4000,(int16_t)flow,0x5300,0,0,0},out[128];
    ploc4_defaults(p);ploc4_set(p,P4_CHAOS,(unsigned)chaos);ploc4_start(&s,3);
    for(int b=0;b<2068;b++)ploc4_process(&s,out,pitch,p,32767,sine,128,1);
    unsigned before=s.events;ploc4_off(&s);
    for(int b=0;b<100;b++)ploc4_process(&s,out,pitch,p,32767,sine,128,1);
    assert(s.events==before);
    for(int n=0;n<128;n++)assert(out[n]==0);
    return before;
}
static void damping(void) {
    double worst=0;
    for(unsigned f=200;f<=12000;f+=53) {
        /* TAIL 5 is .9155x damping and VISC 0 is .9x. */
        double expected=(.043*f+.0014*f*sqrt(f))/44100.*1048576.*.9*30000./32768.;
        double error=fabs(p4_decay(f,0,5)-expected)/expected;if(error>worst)worst=error;
    }
    assert(worst<.025);printf("damping_max_relative_error=%.6f\n",worst);
}
int main(void) {
    for(int n=0;n<8192;n++)sine[n]=(int16_t)lround(sin(n*6.283185307179586/8192)*32767);
    for(int n=0;n<128;n++)pitch[n]=p4_hz_inc(262);
    codec();damping();
    unsigned a=flow_events(0,0),b=flow_events(8192,10),c=flow_events(32767,10);
    assert(a==1&&b>100&&c>b*2);printf("six_second_events=%u,%u,%u\n",a,b,c);
    int worst=0;
    for(unsigned run=0;run<256;run++) {
        int16_t p[8];for(int k=0;k<4;k++)p[k]=(run&(1u<<k))?32767:0;
        p[4]=0x533f;ploc4_defaults(p);
        for(int k=0;k<10;k++)ploc4_set(p,k,(run&(1u<<(k%8)))?15:0);
        ploc4_state s;ploc4_start(&s,run);
        for(int block=0;block<350;block++) {
            int16_t out[130];out[0]=1234;out[129]=-4321;
            for(int n=0;n<128;n++)pitch[n]=(int32_t)(rnd()&0x7fffffff);
            if(block==200)ploc4_off(&s);
            ploc4_process(&s,out+1,pitch,p,32767,sine,128,1);
            assert(out[0]==1234&&out[129]==-4321);
            for(int n=1;n<=128;n++){int v=out[n]<0?-out[n]:out[n];assert(v<=30000);if(v>worst)worst=v;}
            for(int k=0;k<8;k++)assert(s.grain[k].amp>=0&&s.grain[k].amp<=32767);
        }
    }
    /* Six full-envelope voices, including coherent unison, all-key release,
     * extreme flow and long tails. No assumed firmware bus attenuation. */
    int sum_peak=0;long long clips=0;
    for(int run=0;run<12;run++) {
        ploc4_state s[6];int16_t p[8]={16384,0,0,32767,0x5300,0,0,0};ploc4_defaults(p);
        ploc4_set(p,P4_TAIL,run&1?15:5);ploc4_set(p,P4_SPRAY,run&2?15:2);
        for(int v=0;v<6;v++)ploc4_start(&s[v],run==0?1u:(unsigned)(run*7+v));
        for(int block=0;block<1200;block++) {
            int sum[128]={0};for(int v=0;v<6;v++){
                for(int n=0;n<128;n++)pitch[n]=p4_hz_inc((unsigned)(run==0?262:180+v*83));
                if(block==700)ploc4_off(&s[v]);int16_t out[128];
                ploc4_process(&s[v],out,pitch,p,32767,sine,128,block<700?6:0);
                for(int n=0;n<128;n++)sum[n]+=out[n];
            }
            for(int n=0;n<128;n++){int v=sum[n]<0?-sum[n]:sum[n];if(v>sum_peak)sum_peak=v;if(v>32767)clips++;}
        }
    }
    assert(clips==0);printf("PLOC4: codec, legacy tags, 256 extremes, release, bounds passed; solo_peak=%d six_voice_peak=%d clips=%lld state=%zu\n",worst,sum_peak,clips,sizeof(ploc4_state));
    return 0;
}
