#include<stdio.h>
void io_hlt(void);
void io_cli(void);
void io_out8(int port, int data);
int  io_load_eflags(void);
void io_store_eflags(int eflags);

void init_platte(void);
void set_platte(int start, int end, unsigned char *rgb);
void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1);
void init_screen(char *vram, int x, int y);
void putfont8(char *vram, int xsize, int x, int y, char c, char *font);
void putfonts8_asc(char *vram, int xsize, int x, int y, char c, unsigned char *s);
void init_mouse_cursor8(char *mouse, char bc);
void putblock8_8(char *vram, int vxsize, int pxsize, int pysize, int px0, int py0, char *buf, int bxsize);

#define COL8_000000     0
#define COL8_FF0000     1
#define COL8_00FF00     2
#define COL8_FFFF00     3
#define COL8_0000FF     4
#define COL8_FF00FF     5
#define COL8_00FFFF     6
#define COL8_FFFFFF     7
#define COL8_C6C6C6     8
#define COL8_840000     9
#define COL8_008400     10
#define COL8_848400     11
#define COL8_000084     12
#define COL8_840084     13
#define COL8_008484     14
#define COL8_848484     15

struct BOOTINFO {   // 共 12 字节
    // 每个char占 1 字节， 共 4 字节
    char cyls, leds, vmode, reserve;
    // 每个 short 占 2 字节， 共 4 字节
    short scrnx, scrny;
    // 指针占 4 字节
    char *vram;
};
typedef struct BOOTINFO* Bootinfo;

struct SEFMENT_DESCRIPTOR {
    short limit_low, base_low;
    char base_mid, access_right;
    char limit_high, base_high;
};
typedef struct SEFMENT_DESCRIPTOR* Segmdesc;

struct GATE_DESCRIPTOR {
    short offset_low, selector;
    char dw_count, access_right;
    short offset_high;
};
typedef struct GATE_DESCRIPTOR* Gatedesc;

void init_gdtidt(void);
void set_segmdesc(Segmdesc sd, unsigned int limit, int base, int ar);
void set_gatesedc(Gatedesc gd, int offset, int selector, int ar);
void load_gdtr(int limit, int addr);
void load_idtr(int limit, int addr);

void HariMain(void)
{
    Bootinfo binfo = (Bootinfo) 0x0FF0;
    char s[40], mcursor[16*16];
    int mx, my;
    
    init_gdtidt();
    init_platte();
    init_screen(binfo->vram, binfo->scrnx, binfo->scrny);
    
    putfonts8_asc(binfo->vram, binfo->scrnx, 8, 8, COL8_FFFFFF, "Dear Nagisa,");
    putfonts8_asc(binfo->vram, binfo->scrnx, 31, 31, COL8_000000, "Have a Nice Day!");
    putfonts8_asc(binfo->vram, binfo->scrnx, 30, 30, COL8_C6C6C6, "Have a Nice Day!");
    
    sprintf(s, "scrnx = %d", binfo->scrnx);
    putfonts8_asc(binfo->vram, binfo->scrnx, 16, 64, COL8_840000, s);
    
    mx = (binfo->scrnx - 16) / 2;
    my = (binfo->scrny - 28 - 16) / 2;
    init_mouse_cursor8(mcursor, COL8_008484);
    putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
    
    sprintf(s, "(%d, %d)", mx, my);
    putfonts8_asc(binfo->vram, binfo->scrnx, 240, 12, COL8_840000, s);
    
    
    // static char font_A[16] = {
        // 0x00, 0x18, 0x18, 0x18, 0x18, 0x24, 0x24, 0x24, 
        // 0x24, 0x7E, 0x42, 0x42, 0x42, 0xE7, 0x00, 0x00
    // };
    
    // putfont8(binfo->vram, binfo->scrnx, 8 * 1, 8, COL8_FFFFFF, hankaku + 'N' * 16);
    // putfont8(binfo->vram, binfo->scrnx, 8 * 2, 8, COL8_FFFFFF, hankaku + 'a' * 16);
    // putfont8(binfo->vram, binfo->scrnx, 8 * 3, 8, COL8_FFFFFF, hankaku + 'g' * 16);
    // putfont8(binfo->vram, binfo->scrnx, 8 * 4, 8, COL8_FFFFFF, hankaku + 'i' * 16);
    // putfont8(binfo->vram, binfo->scrnx, 8 * 5, 8, COL8_FFFFFF, hankaku + 's' * 16);
    // putfont8(binfo->vram, binfo->scrnx, 8 * 6, 8, COL8_FFFFFF, hankaku + 'a' * 16);
    
    while(1)
        io_hlt();
}

void init_platte(void) {
    static unsigned char table_rgb[16 * 3] = {
        0x00, 0x00, 0x00,   //  0: 黑
        0xFF, 0x00, 0x00,   //  1: 亮红
        0x00, 0xFF, 0x00,   //  2: 亮绿
        0xFF, 0xFF, 0x00,   //  3: 亮黄
        0x00, 0x00, 0xFF,   //  4: 亮蓝
        0xFF, 0x00, 0xFF,   //  5: 亮紫
        0x00, 0xFF, 0xFF,   //  6: 浅亮蓝
        0xFF, 0xFF, 0xFF,   //  7: 白
        0xC6, 0xC6, 0xC6,   //  8: 亮灰
        0x84, 0x00, 0x00,   //  9: 暗红
        0x00, 0x84, 0x00,   // 10: 暗绿
        0x84, 0x84, 0x00,   // 11: 暗黄
        0x00, 0x00, 0x84,   // 12: 暗青
        0x84, 0x00, 0x84,   // 13: 暗紫
        0x00, 0x84, 0x84,   // 14: 浅暗蓝
        0x84, 0x84, 0x84    // 15: 暗灰
    };
    set_platte(0, 15, table_rgb);
    return;
}

void set_platte(int start, int end, unsigned char *rgb) {
    int i, eflags;
    eflags = io_load_eflags();  // 记录中断许可标志的值
    io_cli();                   // 将中断许可标志置为0， 禁止中断
    io_out8(0x03C8, start);
    for(i = start; i <= end; i++) {
        io_out8(0x03C9, rgb[0] / 4);
        io_out8(0x03C9, rgb[1] / 4);
        io_out8(0x03C9, rgb[2] / 4);
        rgb += 3;
    }
    io_store_eflags(eflags);    // 复原中断许可标志
    return;
}

void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1) {
    int x, y;
    for (y = y0; y <= y1; y++)
        for (x = x0; x <= x1; x++)
            vram[y * xsize + x] = c;
    return ;
}

void init_screen(char *vram, int x, int y) {
    // 注释掉这一行可以得到一个纯黑的背景，I like it!
    boxfill8(vram, x, COL8_008484,       0,       0,  x -  1,  y - 29);
    boxfill8(vram, x, COL8_C6C6C6,       0,  y - 28,  x -  1,  y - 28);
    boxfill8(vram, x, COL8_FFFFFF,       0,  y - 27,  x -  1,  y - 27);
    boxfill8(vram, x, COL8_C6C6C6,       0,  y - 26,  x -  1,  y -  1);
    
    boxfill8(vram, x, COL8_FFFFFF,       3,  y - 24,      59,  y - 24);
    boxfill8(vram, x, COL8_FFFFFF,       2,  y - 24,       2,  y -  4);
    boxfill8(vram, x, COL8_848484,       3,  y -  4,      59,  y -  4);
    boxfill8(vram, x, COL8_848484,      59,  y - 23,      59,  y -  5);
    boxfill8(vram, x, COL8_000000,       2,  y -  3,      59,  y -  3);
    boxfill8(vram, x, COL8_000000,      60,  y - 24,      60,  y -  3);

    boxfill8(vram, x, COL8_848484,  x - 47,  y - 24,  x -  4,  y - 24);
    boxfill8(vram, x, COL8_848484,  x - 47,  y - 23,  x - 47,  y -  4);
    boxfill8(vram, x, COL8_FFFFFF,  x - 47,  y -  3,  x -  4,  y -  3);
    boxfill8(vram, x, COL8_FFFFFF,  x -  3,  y - 24,  x -  3,  y -  3);

}

void putfont8(char *vram, int xsize, int x, int y, char c, char *font) {
    int i;
    char *p, d;
    p = vram + x + xsize*y;
    for(i = 0; i < 16; i++) {
        d = font[i];
        if ((d & (1 << 7))) p[0] = c;
        if ((d & (1 << 6))) p[1] = c;
        if ((d & (1 << 5))) p[2] = c;
        if ((d & (1 << 4))) p[3] = c;
        if ((d & (1 << 3))) p[4] = c;
        if ((d & (1 << 2))) p[5] = c;
        if ((d & (1 << 1))) p[6] = c;
        if ((d & (1     ))) p[7] = c;
        p += xsize;
    }
    return;
}

void putfonts8_asc(char *vram, int xsize, int x, int y, char c, unsigned char *s) {
    extern char hankaku[4096];
    for (; *s != '\0'; s++) {
        putfont8(vram, xsize, x, y, c, hankaku + *s * 16);
        x += 8;
    }
    return;
}

void init_mouse_cursor8(char *mouse, char bc) {
    static char cursor[16][16] = {
        "**************  ",
        "*OOOOOOOOOOO*   ",
        "*OOOOOOOOOO*    ",
        "*OOOOOOOOO*     ",
        "*OOOOOOOO*      ",
        "*OOOOOOO*       ",
        "*OOOOOOO*       ",
        "*OOOOOOOO*      ",
        "*OOOO**OOO*     ",
        "*OOO*  *OOO*    ",
        "*OO*    *OOO*   ",
        "*O*      *OOO*  ",
        "**        *OOO* ",
        "*          *OOO*",
        "            *OO*",
        "             ***"
    };
    int x, y;
    for (y = 0; y < 16; y++)
        for (x = 0; x < 16; x++)
            if (cursor[y][x] == '*')
                mouse[y * 16 + x] = COL8_000000;
            else if (cursor[y][x] == 'O')
                mouse[y * 16 + x] = COL8_FFFFFF;
            else
                mouse[y * 16 + x] = bc;
    return;
}

void putblock8_8(char *vram, int vxsize, int pxsize, int pysize, int px0, int py0, char *buf, int bxsize) {
    int x, y;
    for (y = 0; y < pysize; y++)
        for (x = 0; x < pxsize; x++)
            vram[(py0 + y) * vxsize + (px0 + x)] = buf[y * bxsize + x];
    return;
}

void init_gdtidt(void) {
    Segmdesc gdt = (Segmdesc) 0x00270000;
    Gatedesc idt = (Gatedesc) 0x0026f800;
    int i;
    // GDT初始化
    for (i=0; i < 8192; i++)
        set_segmdesc(gdt + i, 0, 0, 0);
    set_segmdesc(gdt + 1, 0xFFFFFFFF, 0x00000000, 0x4092);
    set_segmdesc(gdt + 2, 0x0007FFFF, 0x00280000, 0x409A);
    load_gdtr(0xFFFF, 0x00270000);
    
    // IDT初始化
    for (i = 0; i < 256; i++)
        set_gatesedc(idt + i, 0, 0, 0);
    load_idtr(0x7FF, 0x0026f800);
    
    return;
}

void set_segmdesc(Segmdesc sd, unsigned int limit, int base, int ar) {
    if (limit > 0xFFFFFF) {
        ar |= 0x8000;   /* G_bit = 1 */
        limit /= 0x1000;
    }
    
    sd->limit_low    = limit & 0xFFFF;
    sd->base_low     = base & 0xFFFF;
    sd->base_mid     = (base >> 16) & 0xFF;
    sd->access_right = ar & 0xFF;
    sd->limit_high   = ((limit >> 16) & 0x0F) | ((ar >> 8) & 0xF0);
    sd->base_high    = (base >> 24) & 0xFF;
    
    return;
}

void set_gatesedc(Gatedesc gd, int offset, int selector, int ar) {
    gd->offset_low   = offset & 0xFFFF;
    gd->selector     = selector;
    gd->dw_count     = (ar >> 8) & 0xFF;
    gd->access_right = ar & 0xFF;
    gd->offset_high  = (offset >> 16) & 0xFFFF;
    return;
}
