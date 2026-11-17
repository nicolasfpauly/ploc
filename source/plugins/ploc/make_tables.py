#!/usr/bin/env python3
"""Analytic envelope tables for the integer PLOC 0.2 bubble engine."""
import math
from pathlib import Path
def table(name, values):
    lines=[', '.join(str(x) for x in values[i:i+12]) for i in range(0,len(values),12)]
    return 'static const int16_t '+name+'['+str(len(values))+']={\n    '+',\n    '.join(lines)+'\n};\n'
attack=[round(32767*(1-math.exp(-n/(44100*.0003)))) for n in range(128)]
impact=[round(32767*.12*math.exp(-n/(44100*.00065))*(1-math.exp(-n/(44100*.00012)))) for n in range(353)]
text='/* Analytic tables, generate with make_tables.py. GPL-3.0-or-later. */\n#ifndef PLOC_TABLES_H\n#define PLOC_TABLES_H\n#include <stdint.h>\n'
text+=table('ploc_attack',attack)+table('ploc_impact',impact)+'#endif\n'
Path(__file__).with_name('ploc_tables.h').write_text(text)

exp=[round(32767*math.exp(-n/32)) for n in range(257)]
Path(__file__).with_name('ploc3_tables.h').write_text('/* PLOC 0.3 analytic amplitude curve, GPL-3.0-or-later. */\n#ifndef PLOC3_TABLES_H\n#define PLOC3_TABLES_H\n#include <stdint.h>\n'+table('ploc3_exp',exp)+'#endif\n')
