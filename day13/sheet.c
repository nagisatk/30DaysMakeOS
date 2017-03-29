#include "bootpack.h"

ShtCtl* shtctl_init(Memman *man, unsigned char *vram, int xsize, int ysize) {
    ShtCtl* ctl;
    int i;
    ctl = (ShtCtl *) memman_alloc_4k(man, sizeof(ShtCtl));
    if(ctl == 0)
        return ctl;
    // map sart
    ctl->map = (unsigned char *) memman_alloc_4k(man, xsize * ysize);
    if(ctl->map == 0) {
        memman_free_4k(man, (int) ctl, sizeof(ShtCtl));
        return ctl;
    }
    ctl->vram = vram;
    ctl->xsize = xsize;
    ctl->ysize = ysize;
    ctl->top = -1;
    for(i = 0; i < MAX_SHEETS; i++) {
        ctl->sheets0[i].flags = 0;
        ctl->sheets0[i].ctl = ctl;
    }
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

void sheet_updown(Sheet *sht, int height) {
    ShtCtl *ctl = sht->ctl;
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
            sheet_refreshmap(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height + 1);
            sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height + 1, old); // 按新图层的信息重新绘制画面
        } else {    // 隐藏
            if(ctl->top > old) {
                // 把上面的降下来
                for(h = old; h < ctl->top; h++) {
                    ctl->sheets[h] = ctl->sheets[h + 1];
                    ctl->sheets[h]->height = h;
                }
            }
            ctl->top --;    // 因为图层减少了一个，所以最上面的图层高度下降
            sheet_refreshmap(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, 0);
            sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, 0, old - 1); // 按新图层的信息重新绘制画面
        }
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
            for(h = ctl->top; h >= height; h--) {
                ctl->sheets[h + 1] = ctl->sheets[h];
                ctl->sheets[h + 1]->height = h + 1;
            }
            ctl->sheets[height] = sht;
            ctl->top ++;
        }
        sheet_refreshmap(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height);
        sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height, height); // 按新图层的信息重新绘制画面
    }
    return;
}

void sheet_refreshsub(ShtCtl *ctl, int vx0, int vy0, int vx1, int vy1, int h0, int h1) {
    int h, bx, by, vx, vy, bx0, by0, bx1, by1;
    unsigned char *buf, *vram = ctl->vram, *map = ctl->map, sid;
    Sheet *sht;
    if(vx0 < 0) vx0 = 0;
    if(vy0 < 0) vy0 = 0;
    if(vx1 > ctl->xsize) vx1 = ctl->xsize;
    if(vy1 > ctl->ysize) vy1 = ctl->ysize;
    for(h = h0; h <= h1; h ++) {
        sht = ctl->sheets[h];
        buf = sht->buf;
        sid = sht - ctl->sheets0;
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
                if(map[vy * ctl->xsize + vx] == sid)
                    vram[vy * ctl->xsize + vx] = buf[by * sht->bxsize + bx];
            }
        }
    }
    return;
}

void sheet_refresh(Sheet *sht, int bx0, int by0, int bx1, int by1) {
    if(sht->height >= 0)
        sheet_refreshsub(sht->ctl, sht->vx0 + bx0, sht->vy0 + by0, sht->vx0 + bx1, sht->vy0 + by1, sht->height, sht->height);
    return;
}

void sheet_slide(Sheet *sht, int vx0, int vy0) {
    int old_vx0 = sht->vx0, old_vy0 = sht->vy0;
    sht->vx0 = vx0;
    sht->vy0 = vy0;
    if(sht->height >= 0) {      // 如果图层正在显示
        sheet_refreshmap(sht->ctl, old_vx0, old_vy0, old_vx0 + sht->bxsize, old_vy0 + sht->bysize, 0);
        sheet_refreshmap(sht->ctl, vx0, vy0, vx0 + sht->bxsize, vy0 + sht->bysize, sht->height);
        sheet_refreshsub(sht->ctl, old_vx0, old_vy0, old_vx0 + sht->bxsize, old_vy0 + sht->bysize, 0, sht->height - 1);
        sheet_refreshsub(sht->ctl, vx0, vy0, vx0 + sht->bxsize, vy0 + sht->bysize, sht->height, sht->height);
    }
    return;
}

void sheet_free(Sheet *sht) {
    if(sht->height >= 0)
        sheet_updown(sht, -1);     // 如果出于显示状态则先设定为隐藏
    sht->flags = 0;     // 未使用 标识
    return;
}

void sheet_refreshmap(ShtCtl *ctl, int vx0, int vy0, int vx1, int vy1, int h0) {
    int h, bx, by, vx, vy, bx0, by0, bx1, by1;
    unsigned char *buf, sid, *map = ctl->map;
    Sheet *sht;
    if(vx0 < 0) vx0 = 0;
    if(vy0 < 0) vy0 = 0;
    if(vx1 > ctl->xsize) vx1 = ctl->xsize;
    if(vy1 > ctl->ysize) vy1 = ctl->ysize;
    for(h = h0; h <= ctl->top; h ++) {
        sht = ctl->sheets[h];
        sid = sht - ctl->sheets0;   // 将进行了减法运算的地址作为图层号码使用
        buf = sht->buf;
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
                if(buf[by * sht->bxsize + bx] != sht->col_inv)
                    map[vy * ctl->xsize + vx] = sid;
            }
        }
    }
    return;
}
