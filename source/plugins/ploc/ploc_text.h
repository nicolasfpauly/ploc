/* Stock R246 Text bindings, as documented in SDK 3.1.4 op1_synth.h.
 * Kept local to this R246-targeted screen; no font geometry is embedded.
 * Objects are retained for the plugin lifetime and mutated only on the GUI thread.
 * GPL-3.0-or-later with firmware permission (NOTICE).
 */
#ifndef PLOC_TEXT_H
#define PLOC_TEXT_H
#include <stdint.h>
#define PLOC_TEXT_SIZE 0xA4
#ifdef PLOC_SCREEN_HOST
void ploc_text_construct(void *,int32_t,int32_t,int32_t,const uint8_t *);
void ploc_text_set(void *,const char *);
void ploc_text_draw(void *,void *,const int32_t *);
#else
static inline void ploc_text_construct(void *obj,int32_t x,int32_t y,int32_t size,const uint8_t *c) {
    ((void *(*)(void *,int32_t,int32_t,int32_t,const uint8_t *,int32_t))0x01A61904)(obj,x,y,size,c,0);
}
static inline void ploc_text_set(void *obj,const char *s) {
    ((void (*)(void *,const char *))0x01A61AAC)(obj,s);
}
static inline void ploc_text_draw(void *obj,void *r,const int32_t *m) {
    ((void (*)(void *,void *,const int32_t *))0x01A6259C)(obj,r,m);
}
#endif
#endif
