#include<stdio.h>
#include "bootpack.h"

#define EFLAGS_AC_BIT       0x00040000
#define CR0_CACHE_DISABLE   0x60000000

unsigned int memtest(unsigned int start, unsigned int end);
#define MEMMAN_FREES        4090    /* 大约32KB */
#define MEMMAN_ADDR         0x003C0000

struct FREEINFO {                   // 可用信息
    unsigned int addr, size;
};
typedef struct FREEINFO Freeinfo;

struct MEMMAN {                     // 内存管理
    int frees, maxfrees, lostsize, losts;
    Freeinfo free[MEMMAN_FREES];
};
typedef struct MEMMAN Memman;

void memman_init(Memman *man);

unsigned int memman_total(Memman *man);
unsigned int memman_alloc(Memman *man, unsigned int size);
int memman_free(Memman *man, unsigned int addr, unsigned int size);

void HariMain(void) {
    Bootinfo binfo = (Bootinfo) ADR_BOOTINFO;
    char s[40], mcursor[16*16], keybuf[32], mousebuf[128];
    // unsigned char mouse_dbuf[3];
    int mx, my, i;

    // 初始化IDT GDT
    init_gdtidt();
    init_pic();
    io_sti();

    fifo8_init(&keyfifo, 32, keybuf);
    fifo8_init(&mousefifo, 128, mousebuf);

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

    unsigned int memtotal;
    Memman *man = (Memman *) MEMMAN_ADDR;
    memtotal = memtest(0x00400000, 0xBFFFFFFF);
    memman_init(man);
    memman_free(man, 0x00001000, 0x0009E000);
    memman_free(man, 0x00400000, memtotal - 0x00400000);
    sprintf(s, "Memory %d MB    free : %d KB", memtotal / (1024 * 1024), memman_total(man) / 1024);
    putfonts8_asc(binfo->vram, binfo->scrnx, 0, 32, COL8_FFFFFF, s);
    
    Mouse_dec mdec;
    enable_mouse(&mdec);

    while(1) {
        io_cli();   // 屏蔽中断
        if(fifo8_status(&keyfifo) != 0) {
            i = fifo8_get(&keyfifo);
            io_sti();
            sprintf(s, "%02X", i);
            boxfill8(binfo->vram, binfo->scrnx, COL8_848484, 0, 16, 15, 31);
            putfonts8_asc(binfo->vram, binfo->scrnx, 0, 16, COL8_FFFFFF, s);
        } else if(fifo8_status(&mousefifo) != 0) {
            i = fifo8_get(&mousefifo);
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

unsigned int memtest(unsigned int start, unsigned int end) {
    char flg486 = 0;
    unsigned int eflg, cr0, i;
    /* 确认 CPU 是386还是486以上的 */
    eflg = io_load_eflags();
    eflg |= EFLAGS_AC_BIT; /* AC-bit = 1 */
    io_store_eflags(eflg);
    eflg = io_load_eflags();
    if((eflg & EFLAGS_AC_BIT) != 0)
        flg486 = 1;
    eflg &= ~EFLAGS_AC_BIT; /* AC-bit = 0 */
    io_store_eflags(eflg);

    if(flg486 != 0) {
        cr0 = load_cr0();
        cr0 |= CR0_CACHE_DISABLE;   /* 禁止缓存 */
        store_cr0(cr0);
    }
    
    i = memtest_sub(start, end);
    
    if(flg486 != 0) {
        cr0 = load_cr0();
        cr0 &= ~CR0_CACHE_DISABLE;   /* 允许缓存 */
        store_cr0(cr0);
    }
    return i;
}

void memman_init(Memman *man) {    // 初始化
    man->frees    = 0;                  // 可用信息数目
    man->maxfrees = 0;                  // 用于观察可用情况，frees的最大值
    man->lostsize = 0;                  // 释放失败的内存的大小总和
    man->losts    = 0;                  // 释放失败次数
    return;
}

unsigned int memman_total(Memman *man) {    // 返回空余内存合计大小
    unsigned int i ,t = 0;
    for(i = 0; i < man->frees; i++)
        t += man->free[i].size;
    return t;
}

unsigned int memman_alloc(Memman *man, unsigned int size) { // 内存分配
    unsigned int i, a;
    for(i = 0; i < man->frees; i++) {
        if(man->free[i].size >= size) { //找到了足够大的内存
            a = man->free[i].addr;
            man->free[i].addr += size;
            man->free[i].size -= size;
            if(man->free[i].size == 0) {    // 如果free[i]变成了0， 减掉一条内存信息
                man->frees --;
                for(; i < man->frees; i++)
                    man->free[i] = man->free[i + 1];
            }
            return a;
        }
    }
    return 0;       // 没有可用空间
}

int memman_free(Memman *man, unsigned int addr, unsigned int size) {
    int i, j;
    // 为了方便归纳，将free按照addr的顺序排列
    // 所以先确定一个放在哪里
    for(i = 0; i < man->frees; i++)
        if(man->free[i].addr > addr)
            break;
    if(i > 0) {
        if(man->free[i - 1].addr + man->free[i].size == addr) {
            man->free[i - 1].size += size;      // 可以与前面的可用内存归纳到一起
            if(i < man->frees) {
                if(addr + size == man->free[i].addr) {  // 后面也有
                    man->free[i - 1].size += man->free[i].size;
                    man->frees --;
                    for(; i < man->frees; i++)
                        man->free[i] = man->free[i + 1];
                }
            }
            return 0; // 成功完成
        }
    }
    // 不能与前面的内存归纳到一起，但是可以与后面的内存归纳到一起
    if(i < man->frees) {
        if(addr + size == man->free[i].addr) {
            man->free[i].addr =  addr;
            man->free[i].size += size;
            return 0;
        }
    }
    // 不能与前后内存归纳到一起，新建内存项
    if(man->frees < MEMMAN_FREES) {
        for(j = man->frees; j > i; j--) {
            man->free[j] = man->free[j - 1];
        }
        man->frees ++;
        if(man->maxfrees < man->frees)
            man->maxfrees = man->frees;
        man->free[i].addr = addr;
        man->free[i].size = size;
        return 0;
    }
    // 无法归纳，即man->frees已经等于最大值
    man->losts++;
    man->lostsize += size;
    return -1;
}
