#include "bootpack.h"

unsigned int memtest(unsigned int start, unsigned int end) {
    char flg486 = 0;
    unsigned int eflg, cr0, i;
    /* ȷ�� CPU ��386����486���ϵ� */
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
        cr0 |= CR0_CACHE_DISABLE;   /* ��ֹ���� */
        store_cr0(cr0);
    }
    
    i = memtest_sub(start, end);
    
    if(flg486 != 0) {
        cr0 = load_cr0();
        cr0 &= ~CR0_CACHE_DISABLE;   /* ������ */
        store_cr0(cr0);
    }
    return i;
}

void memman_init(Memman *man) {    // ��ʼ��
    man->frees    = 0;                  // ������Ϣ��Ŀ
    man->maxfrees = 0;                  // ���ڹ۲���������frees�����ֵ
    man->lostsize = 0;                  // �ͷ�ʧ�ܵ��ڴ�Ĵ�С�ܺ�
    man->losts    = 0;                  // �ͷ�ʧ�ܴ���
    return;
}

unsigned int memman_total(Memman *man) {    // ���ؿ����ڴ�ϼƴ�С
    unsigned int i ,t = 0;
    for(i = 0; i < man->frees; i++)
        t += man->free[i].size;
    return t;
}

unsigned int memman_alloc(Memman *man, unsigned int size) { // �ڴ����
    unsigned int i, a;
    for(i = 0; i < man->frees; i++) {
        if(man->free[i].size >= size) { //�ҵ����㹻����ڴ�
            a = man->free[i].addr;
            man->free[i].addr += size;
            man->free[i].size -= size;
            if(man->free[i].size == 0) {    // ���free[i]�����0�� ����һ���ڴ���Ϣ
                man->frees --;
                for(; i < man->frees; i++)
                    man->free[i] = man->free[i + 1];
            }
            return a;
        }
    }
    return 0;       // û�п��ÿռ�
}

int memman_free(Memman *man, unsigned int addr, unsigned int size) {
    int i, j;
    // Ϊ�˷�����ɣ���free����addr��˳������
    // ������ȷ��һ����������
    for(i = 0; i < man->frees; i++)
        if(man->free[i].addr > addr)
            break;
    if(i > 0) {
        if(man->free[i - 1].addr + man->free[i].size == addr) {
            man->free[i - 1].size += size;      // ������ǰ��Ŀ����ڴ���ɵ�һ��
            if(i < man->frees) {
                if(addr + size == man->free[i].addr) {  // ����Ҳ��
                    man->free[i - 1].size += man->free[i].size;
                    man->frees --;
                    for(; i < man->frees; i++)
                        man->free[i] = man->free[i + 1];
                }
            }
            return 0; // �ɹ����
        }
    }
    // ������ǰ����ڴ���ɵ�һ�𣬵��ǿ����������ڴ���ɵ�һ��
    if(i < man->frees) {
        if(addr + size == man->free[i].addr) {
            man->free[i].addr =  addr;
            man->free[i].size += size;
            return 0;
        }
    }
    // ������ǰ���ڴ���ɵ�һ���½��ڴ���
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
    // �޷����ɣ���man->frees�Ѿ��������ֵ
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
