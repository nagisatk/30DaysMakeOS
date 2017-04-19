#include "bootpack.h"

#define PIT_CTRL 0x0043
#define PIT_CNT0 0x0040

#define TIMER_FLAGS_ALLOC 1
#define TIMER_FLAGS_USING 2

TimerCtl timerctl;

void init_pit(void) {
    int i;
    Timer *t;
    io_out8(PIT_CTRL, 0x34);
    io_out8(PIT_CNT0, 0x9C);
    io_out8(PIT_CNT0, 0x2E);
    timerctl.count = 0;
    for(i = 0; i < MAX_TIMER; i++)
        timerctl.timers0[i].flags = 0;
    t = timer_alloc();
    t->timeout = 0xFFFFFFFF;
    t->flags   = TIMER_FLAGS_USING;
    t->next    = 0;
    timerctl.t0 = t;
    timerctl.next = 0xFFFFFFFF;
    return;
}

Timer* timer_alloc(void) {
    int i;
    for(i = 0; i < MAX_TIMER; i++) {
        if(timerctl.timers0[i].flags == 0) {
            timerctl.timers0[i].flags = TIMER_FLAGS_ALLOC;
            return &timerctl.timers0[i];
        }
    }
    return 0;
}

void timer_free(Timer* timer) {
    timer->flags = 0;
    return;
}

void timer_init(Timer *timer, Fifo32 *fifo, unsigned char data) {
    timer->fifo = fifo;
    timer->data = data;
    return;
}

void timer_settime(Timer *timer, unsigned int timeout) {
    int e;
    Timer *t, *s;
    timer->timeout = timeout + timerctl.count;
    timer->flags   = TIMER_FLAGS_USING;
    e = io_load_eflags();
    io_cli();
    t = timerctl.t0;
    if(timer->timeout <= t->timeout) {
        timerctl.t0 = timer;
        timer->next = t;
        timerctl.next = timer->timeout;
        io_store_eflags(e);
        return;
    }
    while(1) {
        s = t;
        t = t->next;
        if(timer->timeout <= t->timeout) {
            s->next = timer;
            timer->next = t;
            io_store_eflags(e);
            return;
        }
    }
}

void inthandler20(int *esp) {
    Timer *timer;
    char ts = 0;
    io_out8(PIC0_OCW2, 0x60);   // 把IRQ-00接收信号结束的信息通知给PIC
    timerctl.count++;
    if(timerctl.next > timerctl.count)
        return;
    timer = timerctl.t0;
    for(;;) {
        if(timer->timeout > timerctl.count)
            break;
        timer->flags = TIMER_FLAGS_ALLOC;
        if(timer != mt_timer) {
            fifo32_put(timer->fifo, timer->data);
        } else {
            ts = 1;
        }
        // fifo32_put(timer->fifo, timer->data);
        timer = timer->next;    // 迭代
    }
    timerctl.t0 = timer;
    timerctl.next = timerctl.t0->timeout;
    if(ts != 0)
        mt_taskswitch();
    return;
}
