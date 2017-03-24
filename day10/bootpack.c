#include<stdio.h>
#include "bootpack.h"

void HariMain(void) {
    Bootinfo binfo = (Bootinfo) ADR_BOOTINFO;
    char s[40], keybuf[32], mousebuf[128];
    int mx, my, i;
    unsigned int memtotal;
    Mouse_dec mdec;
    Memman *man = (Memman *) MEMMAN_ADDR;
    ShtCtl *shtctl;
    Sheet *sht_back, *sht_mouse;
    unsigned char *buf_back, buf_mouse[256];

    // 初始化IDT GDT
    init_gdtidt();
    init_pic();
    io_sti();
    fifo8_init(&keyfifo, 32, keybuf);
    fifo8_init(&mousefifo, 128, mousebuf);
    io_out8(PIC0_IMR, 0xF9); /* PIC1 keyboard 許可(11111001) */
    io_out8(PIC1_IMR, 0xEF); /* mouse (11101111) */

    init_keyboard();
    enable_mouse(&mdec);

    memtotal = memtest(0x00400000, 0xBFFFFFFF);
    memman_init(man);
    memman_free(man, 0x00001000, 0x0009E000);
    memman_free(man, 0x00400000, memtotal - 0x00400000);
    sprintf(s, "Memory %d MB    free : %d KB", memtotal / (1024 * 1024), memman_total(man) / 1024);
    putfonts8_asc(binfo->vram, binfo->scrnx, 0, 32, COL8_FFFFFF, s);
    
    init_palette();
    shtctl = shtctl_init(man, binfo->vram, binfo->scrnx, binfo->scrny);
    sht_back  = sheet_alloc(shtctl);
    sht_mouse = sheet_alloc(shtctl);
    buf_back  = (unsigned char *) memman_alloc_4k(man, binfo->scrnx * binfo->scrny);
    sheet_setbuf(sht_back, buf_back, binfo->scrnx, binfo->scrny, -1);
    sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99);
    init_screen8(buf_back, binfo->scrnx, binfo->scrny);
    init_mouse_cursor8(buf_mouse, 99);
    sheet_slide(shtctl, sht_back, 0, 0);
    mx = (binfo->scrnx - 16) / 2;
    my = (binfo->scrny - 28 - 16) / 2;
    sheet_slide(shtctl, sht_mouse, mx, my);
    sheet_updown(shtctl, sht_back, 0);
    sheet_updown(shtctl, sht_mouse, 1);
    sprintf(s, "(%3d, %3d)", mx, my);
    putfonts8_asc(buf_back, binfo->scrnx, 0, 0, COL8_FFFFFF, s);
    sprintf(s, "Memory %d MB    Free : %d KB", memtotal/(1024*1024), memman_total(man)/1024);
    putfonts8_asc(buf_back, binfo->scrnx, 0, 32, COL8_FFFFFF, s);
    sheet_refresh(shtctl, sht_back, 0, 0, binfo->scrnx, 48);

    while(1) {
        io_cli();   // 屏蔽中断
        if(fifo8_status(&keyfifo) != 0) {
            i = fifo8_get(&keyfifo);
            io_sti();
            sprintf(s, "%02X", i);
            boxfill8(buf_back, binfo->scrnx, COL8_008484, 0, 16, 15, 31);
            putfonts8_asc(buf_back, binfo->scrnx, 0, 16, COL8_FFFFFF, s);
            sheet_refresh(shtctl, sht_back, 0, 16, 16, 32);
        } else if(fifo8_status(&mousefifo) != 0) {
            i = fifo8_get(&mousefifo);
            io_sti();
            if(mouse_decode(&mdec, i) != 0) {
                sprintf(s, "[lcr %4d %4d]", mdec.x, mdec.y);
                if((mdec.btn & 0x01) != 0)
                    s[1] = 'L';
                if((mdec.btn & 0x02) != 0)
                    s[3] = 'R';
                if((mdec.btn & 0x04) != 0)
                    s[2] = 'C';
                boxfill8(buf_back, binfo->scrnx, COL8_008484, 32, 16, 32 + 15 * 8 - 1, 31);
                putfonts8_asc(buf_back, binfo->scrnx, 32, 16, COL8_FFFFFF, s);
                sheet_refresh(shtctl, sht_back, 32, 16, 32 + 15 * 8, 32);
                mx += mdec.x;
                my += mdec.y;

                mx = mx < 0 ? 0 : mx;
                my = my < 0 ? 0 : my;

                mx = mx > binfo->scrnx - 16 ? binfo->scrnx - 16 : mx;
                my = my > binfo->scrny - 16 ? binfo->scrny - 16 : my;

                sprintf(s, "(%3d, %3d)", mx, my);
                boxfill8(buf_back, binfo->scrnx, COL8_008484, 0, 0, 79, 15);     // 隐藏坐标
                putfonts8_asc(buf_back, binfo->scrnx, 0, 0, COL8_FFFFFF, s);     // 显示坐标
                sheet_refresh(shtctl, sht_back, 0, 0, 80, 16);
                sheet_slide(shtctl, sht_mouse, mx, my);
            }
        } else {
            io_stihlt();
        }
    }
}
