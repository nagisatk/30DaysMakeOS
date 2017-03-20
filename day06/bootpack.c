#include<stdio.h>
#include "bootpack.h"

void HariMain(void)
{
    Bootinfo binfo = (Bootinfo) ADR_BOOTINFO;
    char s[40], mcursor[16*16];
    int mx, my;
    
    init_gdtidt();
    init_pic();
    io_sti();
    
    init_palette();
    init_screen8(binfo->vram, binfo->scrnx, binfo->scrny);
    mx = (binfo->scrnx - 16) / 2;
    my = (binfo->scrny - 28 - 16) / 2;
    init_mouse_cursor8(mcursor, COL8_008484);
    putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
    
    sprintf(s, "(%d, %d)", mx, my);
    putfonts8_asc(binfo->vram, binfo->scrnx, 240, 12, COL8_840000, s);
    
    io_out8(PIC0_IMR, 0xF9); /* PIC1 keyboard 許可(11111001) */
    io_out8(PIC1_IMR, 0xEF); /* mouse (11101111) */
    
    while(1) io_hlt();
}