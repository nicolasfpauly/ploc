/* PLOCCTRL beside other engines: blue's page 2 latches (push opens it, release keeps it, the next push closes it).
 * Copyright (C) 2026 Hundekraft. GPL-3.0-or-later with the OP-1 firmware permission (see LICENSE and NOTICE).
 *
 * The 3.4 manual shots found MONOLIT's and AMP's page 2 open only while blue was held: PLOCCTRL's key_sync, which the
 * core runs on every key event (kn_features_key_sync), cleared op1_eng_page[0] whatever engine the slot showed.
 * This links the real plocctrl.c (as test_plocctrl.c does) and drives it in dispatcher.c key_event_body's order:
 * the features' key_sync, then their input filter, then (not taken) the engine-page branch, whose press toggles
 * op1_eng_page[knob] and whose release does nothing. Knob turns go through the input filter as knob_event_body sends
 * them; the core's own page-field and Shift-layer handling after it (AMP's BASS / MID / TREBLE on blue's page, GATE /
 * CHAR on Shift) is not modelled here, only that PLOCCTRL leaves the page flags and the turns alone.
 * MONOLIT: a plain press-page engine (blue OP1_PRESS_PAGE). AMP: blue OP1_PRESS_PAGE_STEPS, page fields on 1..3.
 * PLOC itself keeps its own rule (blue's flag is a Shift gesture marker, cleared without Shift).
 *
 * Slot and boot (2026-11-21, ploc-flags): PLOCCTRL's OP1_EV_SLOT / OP1_EV_BOOT_DONE branch cleared all four flags for
 * any engine. The core clears them itself on a slot or engine change (dispatcher.c eng_page_follow, at the start and
 * end of key_event_body and at the start of knob_event_body), but OP1_EV_SLOT comes later, from the main loop's
 * kn_core_service (kn_core.c: SYNTH_SLOT compared once a pass), so a push on the new engine can land between the two.
 * survive() models that order: slot change, the core's clear, white / red pushes, then the late event. leave() is the
 * other side: PLOC's marks (0x80|knob in blue's flag, PLOC6_TOKEN|order in white's and red's) when the slot stops
 * showing PLOC without an input event (no eng_page_follow yet) go at the next event or main loop pass, and a 0/1 flag
 * of the new engine stays. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define OP1_PLUGIN_H
#define PLOC4_LINK_HOST
#define PLOC5_LINK_HOST
#define PLOCCTRL_HOST
#define OP1_ENGINE_MAGIC 0x5300
#define OP1_MAX_ORDER 63
typedef struct {void (*note_on)(void *,const int16_t *);void (*render)(void *,int16_t *,const int16_t *);void (*note_off)(void *);uint8_t press[4];} op1_engine_t;
typedef struct {} op1_screen_t;typedef struct {} op1_beat_t;typedef struct {} op1_fx_t;
#include "op1_feature.h"
static int16_t words[8][8],saved[8][8];static int slot,commits,held,drum;
volatile uint8_t op1_eng_page[4];
static int p4_slot(void){return slot;}static int p4_drum_mode(void){return drum;}static int p5_shift_held(void){return held;}
static const int16_t *op1_synth_knobs(int s){return words[s];}
static void save_words(int s,const int16_t *p){memcpy(saved[s],p,16);commits++;}
#include "../plugins/plocctrl/plocctrl.c"

#define SYNTH_SCREEN 0x14
static int page_knob[4];                       /* the shown engine's press: 1 = page 2 (OP1_PRESS_PAGE or _PAGE_STEPS) */

/* key_event_body for an encoder push (0x3B..0x3E, blue..red) or any other key */
static void key(int code, int pressed)
{
    op1_input in = { .type = OP1_IN_KEY, .code = (uint8_t)code, .pressed = (uint8_t)pressed, .shift = (uint8_t)held,
                     .screen = SYNTH_SCREEN };
    FEATURE_plocctrl.key_sync();               /* kn_features_key_sync: every key event */
    if (FEATURE_plocctrl.input(&in)) return;   /* kn_features_input */
    if (code >= 0x3B && code <= 0x3E && page_knob[code - 0x3B]) {   /* the engine-page branch */
        if (pressed) op1_eng_page[code - 0x3B] ^= 1;
        return;
    }
}
static void push(int code) { key(code, 1); key(code, 0); }
static int turn(int k, int d)                  /* knob_event_body: the features' input filter first */
{
    op1_input in = { .type = OP1_IN_KNOB, .code = (uint8_t)k, .delta = (int16_t)d, .shift = (uint8_t)held, .screen = SYNTH_SCREEN };
    return FEATURE_plocctrl.input(&in);
}

static void latch(const char *name, int order, const int pk[4])
{
    memset((void *)op1_eng_page, 0, 4);
    for (int s = 0; s < 8; s++) { memset(words[s], 0, 16); words[s][4] = (int16_t)(OP1_ENGINE_MAGIC + order); }
    for (int k = 0; k < 4; k++) page_knob[k] = pk[k];
    held = 0; slot = 0;
    FEATURE_plocctrl.event(OP1_EV_BOOT_DONE, 0);
    const int16_t before = words[0][5];

    key(0x3B, 1);
    assert(op1_eng_page[0] == 1);              /* blue held: page 2 */
    key(0x3B, 0);
    assert(op1_eng_page[0] == 1);              /* released: page 2 stays (was 0: PLOCCTRL's key_sync) */
    FEATURE_plocctrl.event(OP1_EV_SERVICE, 0);
    assert(op1_eng_page[0] == 1);
    push(0x06); push(0x15);                    /* other keys (T1, SEQ) */
    assert(op1_eng_page[0] == 1);
    for (int k = 1; k < 4; k++) assert(!turn(k, 1024));   /* page-2 turns reach the core (AMP's page fields) */
    assert(op1_eng_page[0] == 1);
    held = 1; key(0x26, 1);                    /* Shift layer: Shift down, turns, Shift up */
    for (int k = 0; k < 4; k++) assert(!turn(k, 2048));
    key(0x26, 0); held = 0;
    assert(op1_eng_page[0] == 1);
    FEATURE_plocctrl.event(OP1_EV_MODE, 4);    /* to another mode and back: the page stays as the core keeps it */
    FEATURE_plocctrl.event(OP1_EV_MODE, 0);
    assert(op1_eng_page[0] == 1);
    key(0x3B, 1);
    assert(op1_eng_page[0] == 0);              /* the next push: page 1 */
    key(0x3B, 0);
    assert(op1_eng_page[0] == 0);
    push(0x3B); push(0x3B);                    /* open, close */
    assert(op1_eng_page[0] == 0);
    if (pk[1]) { push(0x3C); assert(op1_eng_page[1] == 1); push(0x06); assert(op1_eng_page[1] == 1); push(0x3C); assert(!op1_eng_page[1]); }
    assert(words[0][5] == before && commits == 0);
    printf("plocctrl latch: %s's blue page 2 latches beside PLOCCTRL\n", name);
}

static void engine_words(int s, int order) { memset(words[s], 0, 16); words[s][4] = (int16_t)(OP1_ENGINE_MAGIC + order); }

/* The shown engine's white and red pages (and blue / green where it has them) stay through a late OP1_EV_SLOT, a
 * boot-done, a mode change and the main loop passes. */
static void survive(const char *name, int order, const int pk[4])
{
    for (int s = 0; s < 8; s++) engine_words(s, order);
    for (int k = 0; k < 4; k++) page_knob[k] = pk[k];
    held = 0; slot = 0;
    memset((void *)op1_eng_page, 0, 4);
    FEATURE_plocctrl.event(OP1_EV_BOOT_DONE, 0);
    static const int ev[] = { OP1_EV_SLOT, OP1_EV_BOOT_DONE, OP1_EV_MODE, OP1_EV_SERVICE };
    for (unsigned e = 0; e < sizeof ev / sizeof ev[0]; e++) {
        slot = (slot + 1) & 7;                          /* T-key: the slot changes ... */
        memset((void *)op1_eng_page, 0, 4);             /* ... eng_page_follow clears the flags in that key event */
        for (int k = 0; k < 4; k++) if (pk[k]) push(0x3B + k);   /* pushes on the new engine before the main loop pass */
        uint8_t want[4];
        for (int k = 0; k < 4; k++) { want[k] = (uint8_t)pk[k]; assert(op1_eng_page[k] == want[k]); }
        FEATURE_plocctrl.event(ev[e], ev[e] == OP1_EV_SLOT ? slot : 0);   /* the late event */
        FEATURE_plocctrl.event(OP1_EV_SERVICE, 0);
        for (int k = 0; k < 4; k++) assert(op1_eng_page[k] == want[k]);
    }
    assert(commits == 0);
    printf("plocctrl latch: %s's page 2 flags survive slot, boot-done and mode events\n", name);
}

static void ploc_words(int s) { memset(words[s], 0, 16); for (int k = 0; k < 4; k++) words[s][k] = 12000; words[s][4] = 0x5327; ploc4_defaults(words[s]); }

/* PLOC shown, its marks set; the slot then shows another engine with no input event in between (a program change,
 * a preset load): the marks go at the event or the next main loop pass, the new engine's 0/1 flags stay. */
static void leave(int ev)
{
    for (int s = 0; s < 8; s++) ploc_words(s);
    page_knob[0] = page_knob[1] = page_knob[2] = page_knob[3] = 0; held = 0; slot = 0;
    memset((void *)op1_eng_page, 0, 4);
    op1_eng_page[2] = PLOC6_TOKEN | 39;                 /* PLOC's screen */
    FEATURE_plocctrl.event(OP1_EV_SERVICE, 0);          /* PLOCCTRL: red's token */
    assert(op1_eng_page[3] == (PLOC6_TOKEN | 39));
    held = 1; assert(turn(1, 1024)); held = 0;          /* a Shift gesture: blue's 0x80|knob */
    assert(op1_eng_page[0] == 0x81);
    engine_words(1, 3);                                 /* slot 1: GRANULAR (red OP1_PRESS_PAGE, green steps) */
    slot = 1;
    op1_eng_page[1] = 1;                                /* a 0/1 flag the core set for it */
    FEATURE_plocctrl.event(ev, ev == OP1_EV_SLOT ? 1 : 0);
    if (ev != OP1_EV_SERVICE) FEATURE_plocctrl.event(OP1_EV_SERVICE, 0);
    assert(op1_eng_page[0] == 0 && op1_eng_page[1] == 1 && op1_eng_page[2] == 0 && op1_eng_page[3] == 0);
    page_knob[3] = 1; push(0x3E);                       /* GRANULAR's red page 2 */
    FEATURE_plocctrl.event(OP1_EV_SERVICE, 0);
    assert(op1_eng_page[3] == 1 && op1_eng_page[1] == 1);
    /* the old screen's 0x80|order in white's flag goes as well */
    op1_eng_page[2] = 0x80 | 39; FEATURE_plocctrl.event(OP1_EV_SERVICE, 0);
    assert(op1_eng_page[2] == 0 && op1_eng_page[3] == 1 && op1_eng_page[1] == 1);
}

int main(void)
{
    static const int monolit[4] = { 1, 1, 1, 0 }, amp[4] = { 1, 0, 0, 0 };
    static const int granular[4] = { 1, 1, 0, 1 }, all4[4] = { 1, 1, 1, 1 };
    latch("MONOLIT", 44, monolit);
    latch("AMP", 6, amp);
    survive("MONOLIT", 44, monolit);                    /* white OP1_PRESS_PAGE */
    survive("GRANULAR", 3, granular);                   /* red OP1_PRESS_PAGE */
    survive("a four-page test engine", 60, all4);
    commits = 0;
    leave(OP1_EV_SERVICE); leave(OP1_EV_SLOT); leave(OP1_EV_MODE);
    printf("plocctrl latch: PLOC's marks go when the slot stops showing PLOC, the new engine's flags stay\n");

    /* PLOC shown (its preset words and its screen's token): blue's flag is still PLOCCTRL's, cleared without Shift */
    for (int s = 0; s < 8; s++) ploc_words(s);
    slot = 0;
    memset((void *)op1_eng_page, 0, 4); page_knob[0] = 0; held = 0;
    op1_eng_page[2] = PLOC6_TOKEN | 39;
    op1_eng_page[0] = 0x81; key(0x06, 1);
    assert(op1_eng_page[0] == 0);
    held = 1; op1_eng_page[0] = 0x81; key(0x06, 1); assert(op1_eng_page[0] == 0x81);
    held = 0; FEATURE_plocctrl.event(OP1_EV_MODE, 4);
    assert(!op1_eng_page[0] && !op1_eng_page[2]);
    static const int pev[] = { OP1_EV_SLOT, OP1_EV_BOOT_DONE };   /* PLOC shown: slot and boot-done clear its flags */
    for (int e = 0; e < 2; e++) {
        for (int k = 0; k < 4; k++) op1_eng_page[k] = 1;
        op1_eng_page[0] = 0x82; op1_eng_page[2] = PLOC6_TOKEN | 39; op1_eng_page[3] = PLOC6_TOKEN | 39;
        FEATURE_plocctrl.event(pev[e], 0);
        for (int k = 0; k < 4; k++) assert(op1_eng_page[k] == 0);
    }
    printf("plocctrl latch: PLOC's own flags as before\n");
    return 0;
}
