#include "bootpack.h"

void init_gdtidt(void) {
    Segmdesc gdt = (Segmdesc) ADR_GDT;
    Gatedesc idt = (Gatedesc) ADR_IDT;
    int i;
    // GDT初始化
    for (i=0; i <= LIMIT_GDT / 8; i++)
        set_segmdesc(gdt + i, 0, 0, 0);
    set_segmdesc(gdt + 1, 0xFFFFFFFF,   0x00000000, AR_DATA32_RW);
    set_segmdesc(gdt + 2, LIMIT_BOTPAK, ADR_BOTPAK, AR_CODE32_ER);
    load_gdtr(LIMIT_GDT, ADR_GDT);
    
    // IDT初始化
    for (i = 0; i <= LIMIT_IDT / 8; i++)
        set_gatedesc(idt + i, 0, 0, 0);
    load_idtr(LIMIT_IDT, ADR_IDT);
    
    set_gatedesc(idt + 0x21, (int) asm_inthandler21, 2 << 3, AR_INTGATE32);
    set_gatedesc(idt + 0x27, (int) asm_inthandler27, 2 << 3, AR_INTGATE32);
    set_gatedesc(idt + 0x2C, (int) asm_inthandler2c, 2 << 3, AR_INTGATE32);
    
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

void set_gatedesc(Gatedesc gd, int offset, int selector, int ar) {
    gd->offset_low   = offset & 0xFFFF;
    gd->selector     = selector;
    gd->dw_count     = (ar >> 8) & 0xFF;
    gd->access_right = ar & 0xFF;
    gd->offset_high  = (offset >> 16) & 0xFFFF;
    return;
}
