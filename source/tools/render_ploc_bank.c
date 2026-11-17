/* Dry production-DSP audition. Native T2/T3/T4 are excluded. GPL-3.0-or-later. */
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "../plugins/ploc/ploc_dsp.h"
#define RATE 44100
#define LENGTH (RATE*44/10)
static int16_t sine[8192];
static int32_t result[LENGTH];
int main(int argc,char **argv) {
    if(argc!=8) return 2;
    int16_t p[8];for(int j=0;j<4;j++) { int v=atoi(argv[j+1]);if(v<0||v>32767)return 2;p[j]=(int16_t)v; }
    p[4]=0x5300;p[5]=(int16_t)atoi(argv[6]);p[6]=p[7]=0;
    int octave=atoi(argv[5]);if(octave<-2||octave>2)return 2;
    for(int i=0;i<8192;i++)sine[i]=(int16_t)lrint(sin(6.283185307179586*i/8192)*32767);
    static const int notes[3]={60,67,72};
    static const int starts[3]={5292,44100,82908}; /* .12, 1, 1.88 sec */
    for(int j=0;j<3;j++) {
        ploc_model_state state;ploc_model_start_seed(&state,p,(unsigned)j);
        int32_t pitch[128];int16_t block[128];
        double hz=440*pow(2,(notes[j]+12*octave-69)/12.0);
        for(int n=0;n<128;n++)pitch[n]=(int32_t)(hz/RATE*4294967296.0);
        for(int at=starts[j];at<LENGTH;at+=128) {
            ploc_model_process(&state,block,pitch,p,32767,sine,128);
            for(int n=0;n<128&&at+n<LENGTH;n++) {
                int v=result[at+n]+block[n];if(v<-98304||v>98304)return 4;
                result[at+n]=v;
            }
        }
    }
    FILE *f=fopen(argv[7],"wb");if(!f)return 1;
    int ok=fwrite(result,sizeof(int32_t),LENGTH,f)==LENGTH;
    if(fclose(f))ok=0;return !ok;
}
