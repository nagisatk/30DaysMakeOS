#include "bootpack.h"

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


unsigned int memman_alloc_4k(Memman *man, unsigned int size) {
    unsigned int a;
    size = (size + 0xFFF) & 0xFFFFF000;
    a = memman_alloc(man, size);
    return a;
}

int memman_free_4k(Memman *man, unsigned int addr, unsigned int size) {
    int i;
    size = (size + 0xFFF) & 0xFFFFF000;
    i = memman_free(man, addr, size);
}
