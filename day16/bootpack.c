#include<stdio.h>
#include "bootpack.h"

void putfonts8_asc_sht(Sheet *sht, int x, int y, int c, int b, char* s, int l);

struct TSS32 {
    int backlink, esp0, ss0, esp1, ss1, esp2, ss2, cr3;
    int eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi;
    int es, cs, ss, ds, fs, gs;
    int ldtr, iomap;
};
typedef struct TSS32 Tss32;

void task_b_main(Sheet *sht_back);

void HariMain(void) {
    Bootinfo binfo = (Bootinfo) ADR_BOOTINFO;
    Fifo32 fifo;
    char s[40];//, keybuf[32], mousebuf[128], timerbuf[8], timerbuf2[8], timerbuf3[8];
    int fifobuf[128];
    Timer *timer, *timer2, *timer3, *timer_ts;
    int mx, my, i, count;
    unsigned int memtotal;
    Mouse_dec mdec;
    Memman *man = (Memman *) MEMMAN_ADDR;
    ShtCtl *shtctl;
    Sheet *sht_back, *sht_mouse, *sht_win;
    unsigned char *buf_back, buf_mouse[256], *buf_win;
    static char keytable[0x54] = {
        0,   0,   '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '^', 0,   0,
        'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '@', '[', 0,   0,   'A', 'S',
        'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', ':', 0,   0,   ']', 'Z', 'X', 'C', 'V',
        'B', 'N', 'M', ',', '.', '/', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
        '2', '3', '0', '.'
    };
    int cursor_x, cursor_c, task_b_esp;
    
    Tss32 tss_a, tss_b;
    Segmdesc gdt = (Segmdesc)ADR_GDT;
    
    // 初始化IDT GDT
    init_gdtidt();
    init_pic();
    io_sti();
    fifo32_init(&fifo, 128, fifobuf);
    
    init_keyboard(&fifo, 256);
    enable_mouse(&fifo, 512, &mdec);
    // fifo32_init(&fifo, 128, mousebuf);
    init_pit();
    io_out8(PIC0_IMR, 0xF8); /* PIC1 keyboard 和 PIC1許可(11111000) */
    io_out8(PIC1_IMR, 0xEF); /* mouse (11101111) */
    
    // fifo32_init( &fifo,   8,  timerbuf);
    timer  = timer_alloc();
    timer_init( timer, &fifo, 10);
    timer_settime( timer, 1000);
    timer2 = timer_alloc();
    timer_init(timer2, &fifo,  3);
    timer_settime(timer2,  300);
    timer3 = timer_alloc();
    timer_init(timer3, &fifo,  1);
    timer_settime(timer3,   50);
    timer_ts = timer_alloc();
    timer_init(timer_ts, &fifo, 2);
    timer_settime(timer_ts, 2);

    memtotal = memtest(0x00400000, 0xBFFFFFFF);
    memman_init(man);
    memman_free(man, 0x00001000,            0x0009E000);
    memman_free(man, 0x00400000, memtotal - 0x00400000);
    
    init_palette();
    shtctl = shtctl_init(man, binfo->vram, binfo->scrnx, binfo->scrny);
    sht_back  = sheet_alloc(shtctl);
    sht_mouse = sheet_alloc(shtctl);
    sht_win   = sheet_alloc(shtctl);
    buf_back = (unsigned char *) memman_alloc_4k(man, binfo->scrnx * binfo->scrny);
    buf_win  = (unsigned char *) memman_alloc_4k(man,                    160 * 52);
    sheet_setbuf( sht_back,  buf_back, binfo->scrnx, binfo->scrny, -1);
    sheet_setbuf(sht_mouse, buf_mouse,           16,           16, 99);
    sheet_setbuf(  sht_win,   buf_win,          160,           52, -1);
    init_screen8(buf_back, binfo->scrnx, binfo->scrny);
    init_mouse_cursor8(buf_mouse, 99);
    make_window8(buf_win, 160, 52, "Window");
    make_textbox8(sht_win, 8, 28, 144, 16, COL8_FFFFFF);
    cursor_x = 8;
    cursor_c = COL8_FFFFFF;
    sheet_slide(sht_back, 0, 0);
    mx = (binfo->scrnx - 16) / 2;
    my = (binfo->scrny - 28 - 16) / 2;
    sheet_slide(sht_mouse, mx, my);
    sheet_slide(sht_win, 80, 72);
    sheet_updown(sht_back,   0);
    sheet_updown(sht_win,    1);
    sheet_updown(sht_mouse,  2);
    sprintf(s, "(%3d, %3d)", mx, my);
    putfonts8_asc_sht(sht_back, 0, 0, COL8_FFFFFF, COL8_008484, s, 10);
    sprintf(s, "Memory %d MB    Free : %d KB", memtotal/(1024*1024), memman_total(man)/1024);
    putfonts8_asc_sht(sht_back, 0, 32, COL8_FFFFFF, COL8_008484, s, 40);

    tss_a.ldtr = 0;
    tss_a.iomap = 0x40000000;
    tss_b.ldtr = 0;
    tss_b.iomap = 0x40000000;
    set_segmdesc(gdt + 3, 103, (int) &tss_a, AR_TSS32);
    set_segmdesc(gdt + 4, 103, (int) &tss_b, AR_TSS32);
    load_tr(3 * 8);
    task_b_esp = memman_alloc_4k(man, 64 * 1024) + 64 * 1024 - 8;
    tss_b.eip = (int) &task_b_main;
    tss_b.eflags = 0x00000202;
    tss_b.eax = 0;
    tss_b.ecx = 0;
    tss_b.edx = 0;
    tss_b.ebx = 0;
    tss_b.esp = task_b_esp;
    tss_b.ebp = 0;
    tss_b.esi = 0;
    tss_b.edi = 0;
    tss_b.es = 1 * 8;
    tss_b.cs = 2 * 8;
    tss_b.ss = 1 * 8;
    tss_b.ds = 1 * 8;
    tss_b.fs = 1 * 8;
    tss_b.gs = 1 * 8;
    *((int *) (task_b_esp + 4)) = (int) sht_back;
    mt_init();
    // int *a;
    // a = (int *) 0xFEC;
    // *a = (int) sht_back;
    count = 0;
    while(1) {
        count++;
        io_cli();   // 屏蔽中断
        if(fifo32_status(&fifo) == 0) {
            io_stihlt();
        } else {
            i = fifo32_get(&fifo);
            io_sti();
            if(i <= 1) {
                if(i != 0) {
                    timer_init(timer3, &fifo, 0);
                    cursor_c = COL8_000000;
                } else {
                    timer_init(timer3, &fifo, 1);
                    cursor_c = COL8_FFFFFF;
                }
                timer_settime(timer3, 50);
                boxfill8(sht_win->buf, sht_win->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
                sheet_refresh(sht_win, cursor_x, 28, cursor_x + 8, 44); 
            } else if(i == 3) {
                // sprintf(s, "%04d[sec]", timerctl.count/100);
                putfonts8_asc_sht(sht_back, 0, 80, COL8_FFFFFF, COL8_008484, "3[sec]", 6);
                count = 0;
            } else if(i == 10) {
                // sprintf(s, "%010d", count);
                // putfonts8_asc_sht(sht_win, 40, 28, COL8_000000, COL8_C6C6C6, s, 10);
                // sprintf(s, "%04d[sec]", timerctl.count/100);
                putfonts8_asc_sht(sht_back, 0, 64, COL8_FFFFFF, COL8_008484, "10[sec]", 7);
                // taskswitch4();
            } else if(i >= 256 && i <= 511) {
                sprintf(s, "%02X", i - 256);
                putfonts8_asc_sht(sht_back, 0, 16, COL8_FFFFFF, COL8_008484, s, 2);
                if(i < 256 + 0x54) {
                    if(keytable[i - 256] != 0 && cursor_x < 144) {
                        s[0] = keytable[i - 256];
                        s[1] = 0;
                        putfonts8_asc_sht(sht_win, cursor_x, 28, COL8_000000, COL8_FFFFFF, s, 1);
                        cursor_x += 8;
                    }
                }
                if(i == 256 + 0x0E && cursor_x > 8) {
                    putfonts8_asc_sht(sht_win, cursor_x, 28, COL8_000000, COL8_FFFFFF, " ", 1);
                    cursor_x -= 8;
                }
                boxfill8(sht_win->buf, sht_win->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
                sheet_refresh(sht_win, cursor_x, 28, cursor_x + 8, 44);
            } else if(i >= 512 && i <= 767) {
                if(mouse_decode(&mdec, i - 512) != 0) {
                    sprintf(s, "[lcr %4d %4d]", mdec.x, mdec.y);
                    if((mdec.btn & 0x01) != 0)
                        s[1] = 'L';
                    if((mdec.btn & 0x02) != 0)
                        s[3] = 'R';
                    if((mdec.btn & 0x04) != 0)
                        s[2] = 'C';
                    putfonts8_asc_sht(sht_back, 32, 16, COL8_FFFFFF, COL8_008484, s, 15);
                    mx += mdec.x;
                    my += mdec.y;

                    mx = mx < 0 ? 0 : mx;
                    my = my < 0 ? 0 : my;

                    mx = mx > binfo->scrnx - 1 ? binfo->scrnx - 1 : mx;
                    my = my > binfo->scrny - 1 ? binfo->scrny - 1 : my;

                    sprintf(s, "(%3d, %3d)", mx, my);
                    putfonts8_asc_sht(sht_back, 0, 0, COL8_FFFFFF, COL8_008484, s, 10);
                    sheet_slide(sht_mouse, mx, my);
                    if(mdec.btn & 0x01)
                        sheet_slide(sht_win, mx - 80, my - 8);
                }
            }
        }
    }
}

void putfonts8_asc_sht(Sheet *sht, int x, int y, int c, int b, char* s, int l) {
    boxfill8(sht->buf, sht->bxsize, b, x, y, x + l * 8 - 1, y + 15);
    putfonts8_asc(sht->buf, sht->bxsize, x, y, c, s);
    sheet_refresh(sht, x, y, x + l * 8, y + 16);
    return;
}

void task_b_main(Sheet *sht_back) {
    Fifo32 fifo;
    Timer *timer_put, *timer_1s;
    int i, fifobuf[128], count = 0, count0 = 0;
    char s[12];
    
    fifo32_init(&fifo, 128, fifobuf);
    timer_put = timer_alloc();
    timer_init(timer_put, &fifo, 1);
    timer_settime(timer_put, 1);
    timer_1s = timer_alloc();
    timer_init(timer_1s, &fifo, 100);
    timer_settime(timer_1s, 100);
    
    while(1) {
        count ++;
        io_cli();
        if(fifo32_status(&fifo) == 0) {
            io_sti();
        } else {
            i = fifo32_get(&fifo);
            io_sti();
            if(i == 1) {
                sprintf(s, "%11d", count);
                putfonts8_asc_sht(sht_back, 0, 144, COL8_FFFFFF, COL8_008484, s, 11);
                timer_settime(timer_put, 1);
            } else if(i == 100) {
                sprintf(s, "%11d", count - count0);
                putfonts8_asc_sht(sht_back, 0, 128, COL8_000000, COL8_008484, s, 11);
                count0 = count;
                timer_settime(timer_1s, 100);
            }
        }
    }
}
