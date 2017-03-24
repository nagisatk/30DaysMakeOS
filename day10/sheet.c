#include "bootpack.h"

ShtCtl* shtctl_init(Memman *man, unsigned char *vram, int xsize, int ysize) {
    ShtCtl* ctl;
    int i;
    ctl = (ShtCtl *) memman_alloc_4k(man, sizeof(ShtCtl));
    if(ctl == 0)
        return ctl;
    ctl->vram = vram;
    ctl->xsize = xsize;
    ctl->ysize = ysize;
    ctl->top = -1;
    for(i = 0; i < MAX_SHEETS; i++)
        ctl->sheets0[i].flags = 0;
    return ctl;
}

#define SHEET_USE 1

Sheet* sheet_alloc(ShtCtl *ctl) {
    Sheet *sht;
    int i;
    for(i = 0; i < MAX_SHEETS; i++) {
        if(ctl->sheets0[i].flags == 0) {
            sht = &ctl->sheets0[i];
            sht->flags = SHEET_USE;
            sht->height = -1;
            return sht;
        }
    }
    return 0;
}

void sheet_setbuf(Sheet *sht, unsigned char *buf, int xsize, int ysize, int col_inv) {
    sht->buf = buf;
    sht->bxsize = xsize;
    sht->bysize = ysize;
    sht->col_inv = col_inv;
    return;
}

void sheet_updown(ShtCtl *ctl, Sheet *sht, int height) {
    int h, old = sht->height;   // 存储设置前的高度信息
    if(height > ctl->top + 1)
        height = ctl->top + 1;
    if(height < -1)
        height = -1;
    sht->height = height;   // 设定高度
    // 下面主要进行 shhets[]的重新排列
    if(old > height) {  // 比以前低
        if(height >= 0) {   // 把中间的往上提
            for(h = old; h > height; h --) {
                ctl->sheets[h] = ctl->sheets[h - 1];
                ctl->sheets[h]->height = h;
            }
            ctl->sheets[height] = sht;
        } else {    // 隐藏
            if(ctl->top > old) {
                // 把上面的降下来
                for(h = old; h < ctl->top; h++) {
                    ctl->sheets[h] = ctl->sheets[h + 1];
                    ctl->sheets[h]->height = h;
                }
            }
            ctl->top --;    // 因为图层减少了一个，所以最上面的图层高度下降
        }
        sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize); // 按新图层的信息重新绘制画面
    } else if(old < height) {   // 比以前高
        if(old >= 0) {
            // 把中间的拉下去
            for(h = old; h < height; h++) {
                ctl->sheets[h] = ctl->sheets[h + 1];
                ctl->sheets[h]->height = h;
            }
            ctl->sheets[height] = sht;
        } else {    // 由隐藏状态转为显示状态
            // 将已在上面的提上来
            for(h = old; h >= height; h--) {
                ctl->sheets[h + 1] = ctl->sheets[h];
                ctl->sheets[h + 1]->height = h + 1;
            }
            ctl->sheets[height] = sht;
            ctl->top ++;
        }
        sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize); // 按新图层的信息重新绘制画面
    }
    return;
}

void sheet_refreshsub(ShtCtl *ctl, int vx0, int vy0, int vx1, int vy1) {
    int h, bx, by, vx, vy, bx0, by0, bx1, by1;
    unsigned char *buf, c, *vram = ctl->vram;
    Sheet *sht;
    for(h = 0; h <= ctl->top; h ++) {
        sht = ctl->sheets[h];
        buf = sht->buf;
        // 使用 vx0~vy1 对 bx0~by1 进行倒推
        bx0 = vx0 - sht->vx0;
        by0 = vy0 - sht->vy0;
        bx1 = vx1 - sht->vx0;
        by1 = vy1 - sht->vy0;
        if(bx0 < 0) bx0 = 0;
        if(by0 < 0) by0 = 0;
        if(bx1 > sht->bxsize) bx1 = sht->bxsize;
        if(by1 > sht->bysize) by1 = sht->bysize;
        for(by = by0; by < by1; by ++) {
            vy = sht->vy0 + by;
            for(bx = bx0; bx < bx1; bx ++) {
                vx = sht->vx0 + bx;
                c = buf[by * sht->bxsize + bx];
                if(c != sht->col_inv)
                    vram[vy * ctl->xsize + vx] = c;
            }
        }
    }
    return;
}

void sheet_refresh(ShtCtl *ctl, Sheet *sht, int bx0, int by0, int bx1, int by1) {
    if(sht->height >= 0)
        sheet_refreshsub(ctl, sht->vx0 + bx0, sht->vy0 + by0, sht->vx0 + bx1, sht->vy0 + by1);
    return;
}

void sheet_slide(ShtCtl *ctl, Sheet *sht, int vx0, int vy0) {
    int old_vx0 = sht->vx0, old_vy0 = sht->vy0;
    sht->vx0 = vx0;
    sht->vy0 = vy0;
    if(sht->height >= 0) {      // 如果图层正在显示
        sheet_refreshsub(ctl, old_vx0, old_vy0, old_vx0 + sht->bxsize, old_vy0 + sht->bysize);
        sheet_refreshsub(ctl, vx0, vy0, vx0 + sht->bxsize, vy0 + sht->bysize);
    }
    return;
}

void sheet_free(ShtCtl *ctl, Sheet *sht) {
    if(sht->height >= 0)
        sheet_updown(ctl, sht, -1);     // 如果出于显示状态则先设定为隐藏
    sht->flags = 0;     // 未使用 标识
    return;
}
