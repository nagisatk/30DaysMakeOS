#include "bootpack.h"

Timer *mt_timer;
int mt_tr;

void mt_init(void) {
    mt_timer = timer_alloc();
    timer_settime(mt_timer, 2);
    mt_tr = 3 * 8;
    return;
}

void mt_taskswitch(void) {
    mt_tr = mt_tr == 3 * 8 ? 4 * 8 : 3 * 8;
    timer_settime(mt_timer, 2);
    farjmp(0, mt_tr);
    return;
}
