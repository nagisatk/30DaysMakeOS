/* asmhead.asm */
struct BOOTINFO {
    char cyls, leds, vmode, reserve;
    short scrnx, scrny;
    char *vram;
};
typedef struct BOOTINFO* Bootinfo;

#define ADR_BOOTINFO    0x00000FF0

/* naskfunc.asm */
void io_hlt(void);
void io_cli(void);
void io_sti(void);
void io_stihlt(void);
int  io_in8(int port);
void io_out8(int port, int data);
int  io_load_eflags(void);
void io_store_eflags(int eflags);
int  load_cr0(void);
void store_cr0(int cr0);
void load_tr(int tr);
void farjmp(int eip, int cs);
void load_gdtr(int limit, int addr);
void load_idtr(int limit, int addr);
void asm_inthandler20(void);
void asm_inthandler21(void);
void asm_inthandler27(void);
void asm_inthandler2c(void);
unsigned int memtest_sub(unsigned int start, unsigned int end);

/* fifo.c */
struct FIFO32 {
    int *buf, p, q, size, free, flags;
};
typedef struct FIFO32 Fifo32;
void fifo32_init(Fifo32* fifo, int size, int *buf);
int  fifo32_put(Fifo32* fifo, int data);
int  fifo32_get(Fifo32* fifo);
int  fifo32_status(Fifo32* fifo);

/* graphic.c */
void init_palette(void);
void set_palette(int start, int end, unsigned char *rgb);
void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1);
void init_screen8(char *vram, int x, int y);
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

/* dsctbl.c */
struct SEGMENT_DESCRIPTOR {
    short limit_low, base_low;
    char base_mid, access_right;
    char limit_high, base_high;
};
typedef struct SEGMENT_DESCRIPTOR* Segmdesc;

struct GATE_DESCRIPTOR {
    short offset_low, selector;
    char dw_count, access_right;
    short offset_high;
};
typedef struct GATE_DESCRIPTOR* Gatedesc;
void init_gdtidt(void);
void set_segmdesc(Segmdesc sd, unsigned int limit, int base, int ar);
void set_gatedesc(Gatedesc gd, int offset, int selector, int ar);
#define ADR_IDT         0x0026F800
#define LIMIT_IDT       0x000007FF
#define ADR_GDT         0x00270000
#define LIMIT_GDT       0x0000FFFF
#define ADR_BOTPAK      0x00280000
#define LIMIT_BOTPAK    0x0007FFFF
#define AR_DATA32_RW    0x4092
#define AR_CODE32_ER    0x409A
#define AR_TSS32        0x0089
#define AR_INTGATE32    0x008E

/* int.c */
void init_pic(void);
void inthandler27(int *esp);
#define PIC0_ICW1       0x0020
#define PIC0_OCW2       0x0020
#define PIC0_IMR        0x0021
#define PIC0_ICW2       0x0021
#define PIC0_ICW3       0x0021
#define PIC0_ICW4       0x0021
#define PIC1_ICW1       0x00a0
#define PIC1_OCW2       0x00a0
#define PIC1_IMR        0x00a1
#define PIC1_ICW2       0x00a1
#define PIC1_ICW3       0x00a1
#define PIC1_ICW4       0x00a1

/* keyboard.c */
extern Fifo32 *keyfifo;
extern int keydata0;
void inthandler21(int *esp);
void wait_KBC_sendready(void);
void init_keyboard(Fifo32 *fifo, int data0);

#define PORT_KEYDAT             0x0060
#define PORT_KEYCMD             0x0064

/* mouse.c */
struct MOUSE_DEC {
    unsigned char buf[3], phase;
    int x, y, btn;
};
typedef struct MOUSE_DEC Mouse_dec;

extern Fifo32 *mousefifo;
extern int mousedata0;

void inthandler2c(int *esp);
void enable_mouse(Fifo32 *fifo, int data0, Mouse_dec* mdec);
int mouse_decode(Mouse_dec* mdec, unsigned char dat);


/* memory.c */

#define EFLAGS_AC_BIT       0x00040000
#define CR0_CACHE_DISABLE   0x60000000

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

unsigned int memtest(unsigned int start, unsigned int end);
void memman_init(Memman *man);
unsigned int memman_total(Memman *man);
unsigned int memman_alloc(Memman *man, unsigned int size);
unsigned int memman_alloc_4k(Memman *man, unsigned int size);
int memman_free(Memman *man, unsigned int addr, unsigned int size);
int memman_free_4k(Memman *man, unsigned int addr, unsigned int size);

/* shteet.c */
#define MAX_SHEETS 256
struct SHTCTL;
typedef struct SHTCTL ShtCtl;

struct SHEET {
    unsigned char *buf;
    int bxsize, bysize, vx0, vy0, col_inv, height, flags;
    ShtCtl *ctl;
};
typedef struct SHEET Sheet;

struct SHTCTL {
    unsigned char *vram, *map;
    int xsize, ysize, top;
    Sheet *sheets[MAX_SHEETS];
    Sheet sheets0[MAX_SHEETS];
};

ShtCtl* shtctl_init(Memman *man, unsigned char *vram, int xsize, int ysize);

#define SHEET_USE 1

Sheet* sheet_alloc(ShtCtl *ctl);
void sheet_setbuf(Sheet *sht, unsigned char *buf, int xsize, int ysize, int col_inv);
void sheet_updown (Sheet *sht, int height);
void sheet_refresh(Sheet *sht, int bx0, int by0, int bx1, int by1);
void sheet_slide  (Sheet *sht, int vx0, int vy0);
void sheet_free   (Sheet *sht);
void sheet_refreshmap(ShtCtl *ctl, int vx0, int vy0, int vx1, int vy1, int h0);
void sheet_refreshsub(ShtCtl *ctl, int vx0, int vy0, int vx1, int vy1, int h0, int h1);

/* timer.c */
#define MAX_TIMER 500
struct TIMER {
    struct TIMER *next;
    unsigned int timeout, flags;
    Fifo32 *fifo;
    unsigned char data;
};
typedef struct TIMER Timer;


struct TIMERCTL {
    unsigned int count, next;
    Timer *t0;
    Timer timers0[MAX_TIMER];
};
typedef struct TIMERCTL TimerCtl;
extern TimerCtl timerctl;

void init_pit(void);
Timer* timer_alloc(void);
void timer_free(Timer* timer);
void timer_init(Timer *timer, Fifo32 *fifo, unsigned char data);
void timer_settime(Timer *timer, unsigned int timeout);
void inthandler20(int *esp);
// void settimer(unsigned int timeout, Fifo32 *fifo, unsigned char data);

/* window.c */

void make_window8(unsigned char *buf, int xsize, int ysize, char *title);
void make_textbox8(Sheet *sht, int x0, int y0, int sx, int sy, int c);

/* mtask.c */
extern Timer *mt_timer;
void mt_init(void);
void mt_taskswitch(void);
