/* Embute assets/assets.bin (gerado por tools/gen_assets.py) no executavel. */
#include "assets.h"

__asm__(
    ".section .rodata\n"
    ".balign 128\n"
    ".global assets_bin\n"
    "assets_bin:\n"
    ".incbin \"assets/assets.bin\"\n"
    ".balign 16\n"
    ".previous\n");
