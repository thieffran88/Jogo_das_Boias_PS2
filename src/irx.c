/* Modulos do IOP embutidos (so no PS2). */
__asm__(
    ".section .rodata\n"
    ".balign 64\n"
    ".global audsrv_irx\n"
    "audsrv_irx:\n"
    ".incbin \"build/audsrv.irx\"\n"
    ".global audsrv_irx_end\n"
    "audsrv_irx_end:\n"
    ".previous\n");
