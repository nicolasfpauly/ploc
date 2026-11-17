/* Production fixed-point renderer. Native T2 envelope and bus are excluded;
 * all external T3 effects are excluded. No normalisation. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "../plugins/ploc/ploc4_dsp.h"
#include "../plugins/ploc/ploc_dsp.h"
#include "../plugins/ploc/ploc5_surface.h"
int main(int argc,char **argv) {
    if(argc!=18)return 2;int16_t p[8]={0},sine[8192];
    for(int i=0;i<4;i++)p[i]=(int16_t)atoi(argv[i+1]);p[4]=0x5300;ploc4_defaults(p);
    for(int k=0;k<10;k++)ploc4_set(p,k,(unsigned)atoi(argv[5+k]));
    int old=atoi(argv[15]),surface_on=atoi(argv[16]);FILE *f=fopen(argv[17],"wb");if(!f)return 3;
    for(int n=0;n<8192;n++)sine[n]=(int16_t)lround(sin(n*6.283185307179586/8192)*32767);
    int32_t pitch[128];for(int n=0;n<128;n++)pitch[n]=p4_hz_inc(262);
    ploc4_state s;ploc3_state previous;ploc5_surface surface;p5_surface_start(&surface);ploc4_start(&s,1);ploc3_start(&previous,1,0);
    int total=44100*(p[3]?10:6),started=0;unsigned last_events=0;
    for(int at=0;at<total;at+=128) {
        int16_t out[128];for(int n=0;n<128;n++)out[n]=0;
        if(at>=4410&&!started){started=1;ploc4_start(&s,1);p5_surface_start(&surface);ploc3_start(&previous,1,0);}
        if(started){
            if(!p[3]&&((at>=44100&&at<44228)||(at>=88200&&at<88328))){ploc4_start(&s,(unsigned)at);p5_surface_start(&surface);ploc3_start(&previous,(unsigned)at,0);}
            if(p[3]&&at>=44100*6)ploc4_off(&s);
            if(old){int16_t oldp[8];for(int i=0;i<8;i++)oldp[i]=p[i];oldp[5]=3;oldp[6]=oldp[7]=0;
                ploc3_process(&previous,out,pitch,oldp,32767,sine,128);
            }else ploc4_process(&s,out,pitch,p,32767,sine,128,1);
            if(!old&&surface_on)p5_surface_process(&surface,out,128,p[1],p[0]);
            last_events=s.events;
        }
        int n=total-at;if(n>128)n=128;
        if(fwrite(out,sizeof(int16_t),(size_t)n,f)!=(size_t)n){fclose(f);return 4;}
    }
    if(fclose(f))return 5;printf("events=%u input_limits=%u\n",last_events,s.limits);return 0;
}
