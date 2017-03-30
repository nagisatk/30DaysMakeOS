#include "bootpack.h"

Fifo32 *mousefifo;
int mousedata0;

void inthandler2c(int *esp){
    unsigned char data;
    io_out8(PIC1_OCW2, 0x64);   // 通知PIC1 “IRQ-12已经受理完毕”
    io_out8(PIC0_OCW2, 0x62);   // 通知PIC0 “IRQ-02已经受理完毕”
    data = io_in8(PORT_KEYDAT);
    fifo32_put(mousefifo, data + mousedata0);
    return;
}


#define KEYCMD_SENDTO_MOUSE     0xD4
#define MOUSECMD_ENABLE         0xF4

void enable_mouse(Fifo32 *fifo, int data0, Mouse_dec* mdec) {
    mousefifo = fifo;
    mousedata0 = data0;
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
