#include<stdio.h>
#include "bootpack.h"

extern Fifo8 keyinfo;
extern Fifo8 mouseinfo;
void init_keyboard(void);

struct MOUSE_DEC {
    unsigned char buf[3], phase;
    int x, y, btn;
};
typedef struct MOUSE_DEC Mouse_dec;

void enable_mouse(Mouse_dec* mdec);
int mouse_decode(Mouse_dec* mdec, unsigned char dat);

void HariMain(void)
{
    Bootinfo binfo = (Bootinfo) ADR_BOOTINFO;
    char s[40], mcursor[16*16], keybuf[32], mousebuf[128];
    unsigned char mouse_dbuf[3];
    int mx, my, i;
    
    // 初始化IDT GDT
    init_gdtidt();
    init_pic();
    io_sti();
    
    fifo8_init(&keyinfo, 32, keybuf);
    fifo8_init(&mouseinfo, 128, mousebuf);
    
    io_out8(PIC0_IMR, 0xF9); /* PIC1 keyboard 許可(11111001) */
    io_out8(PIC1_IMR, 0xEF); /* mouse (11101111) */
    
    init_keyboard();
    
    init_palette();
    init_screen8(binfo->vram, binfo->scrnx, binfo->scrny);
    mx = (binfo->scrnx - 16) / 2;
    my = (binfo->scrny - 28 - 16) / 2;
    init_mouse_cursor8(mcursor, COL8_008484);
    putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
    
    sprintf(s, "(%d, %d)", mx, my);
    putfonts8_asc(binfo->vram, binfo->scrnx, 240, 12, COL8_840000, s);
    
    Mouse_dec mdec;
    enable_mouse(&mdec);
    // mouse_phase = 0;
    
    while(1) {
        io_cli();   // 屏蔽中断
        if(fifo8_status(&keyinfo) != 0) {
            i = fifo8_get(&keyinfo);
            io_sti();
            sprintf(s, "%02X", i);
            boxfill8(binfo->vram, binfo->scrnx, COL8_848484, 0, 16, 15, 31);
            putfonts8_asc(binfo->vram, binfo->scrnx, 0, 16, COL8_FFFFFF, s);
        } else if(fifo8_status(&mouseinfo) != 0) {
            i = fifo8_get(&mouseinfo);
            io_sti();
            
            if(mouse_decode(&mdec, i) == 1) {
                sprintf(s, "[lcr %4d %4d]", mdec.x, mdec.y);
                if((mdec.btn & 0x01) != 0)
                    s[1] = 'L';
                if((mdec.btn & 0x02) != 0)
                    s[3] = 'R';
                if((mdec.btn & 0x04) != 0)
                    s[2] = 'C';
                boxfill8(binfo->vram, binfo->scrnx, COL8_848400, 32, 16, 32 + 15 * 8 - 1, 31);
                putfonts8_asc(binfo->vram, binfo->scrnx, 32, 16, COL8_FFFFFF, s);
                // 鼠标指针的移动
                boxfill8(binfo->vram, binfo->scrnx, COL8_008484, mx, my, mx + 15, my + 15);
                mx += mdec.x;
                my += mdec.y;
                
                mx = mx < 0 ? 0 : mx;
                my = my < 0 ? 0 : my;
                
                mx = mx > binfo->scrnx - 16 ? binfo->scrnx - 16 : mx;
                my = my > binfo->scrny - 16 ? binfo->scrny - 16 : my;
                
                sprintf(s, "(%3d, %3d)", mx, my);
                boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 0, 79, 15);     // 隐藏坐标
                putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);     // 显示坐标
                putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);    // 画描鼠标
            }
        } else {
            io_stihlt();
        }
    }
}

#define PORT_KEYDAT             0x0060
#define PORT_KEYSTA             0x0064
#define PORT_KEYCMD             0x0064
#define KEYSTA_SEND_NOTREADY    0x02
#define KEYCMD_WRITE_MODE       0x60
#define KBC_MODE                0x47

void wait_KBC_sendready(void) {
    while(1)
        if((io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) == 0)
            return;
}

void init_keyboard(void) {
    wait_KBC_sendready();
    io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);
    wait_KBC_sendready();
    io_out8(PORT_KEYDAT, KBC_MODE);
    return;
}

#define KEYCMD_SENDTO_MOUSE     0xD4
#define MOUSECMD_ENABLE         0xF4

void enable_mouse(Mouse_dec* mdec) {
    wait_KBC_sendready();
    io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
    wait_KBC_sendready();
    io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);
    mdec->phase = 0;
    return;
}

int mouse_decode(Mouse_dec* mdec, unsigned char dat) {
    if(mdec->phase == 0) {
        if(dat == 0xFA)
            mdec->phase = 1;
        return 0;
    }
    if(mdec->phase == 1) {
        if((dat & 0xC8) == 0x08) {
            mdec->phase = 2;
            mdec->buf[0] = dat;
        }
        return 0;
    }
    if(mdec->phase == 2) {
        mdec->phase = 3;
        mdec->buf[1] = dat;
        return 0;
    }
    if(mdec->phase == 3) {
        mdec->phase = 1;
        mdec->buf[2] = dat;
        mdec->btn = mdec->buf[0] & 0x07;
        mdec->x = mdec->buf[1];
        mdec->y = mdec->buf[2];
        
        if((mdec->buf[0] & 0x10) != 0)
            mdec->x |= 0xFFFFFF00;
        if((mdec->buf[0] & 0x20) != 0)
            mdec->y |= 0xFFFFFF00;
        
        mdec->y = -mdec->y;
        return 1;
    }
    return -1;
}
