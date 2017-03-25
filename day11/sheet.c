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
    int h, old = sht->height;   // �洢����ǰ�ĸ߶���Ϣ
    if(height > ctl->top + 1)
        height = ctl->top + 1;
    if(height < -1)
        height = -1;
    sht->height = height;   // �趨�߶�
    // ������Ҫ���� shhets[]����������
    if(old > height) {  // ����ǰ��
        if(height >= 0) {   // ���м��������
            for(h = old; h > height; h --) {
                ctl->sheets[h] = ctl->sheets[h - 1];
                ctl->sheets[h]->height = h;
            }
            ctl->sheets[height] = sht;
            sheet_refreshmap(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height + 1);
            sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height + 1, old); // ����ͼ�����Ϣ���»��ƻ���
        } else {    // ����
            if(ctl->top > old) {
                // ������Ľ�����
                for(h = old; h < ctl->top; h++) {
                    ctl->sheets[h] = ctl->sheets[h + 1];
                    ctl->sheets[h]->height = h;
                }
            }
            ctl->top --;    // ��Ϊͼ�������һ���������������ͼ��߶��½�
            sheet_refreshmap(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, 0);
            sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, 0, old - 1); // ����ͼ�����Ϣ���»��ƻ���
        }
    } else if(old < height) {   // ����ǰ��
        if(old >= 0) {
            // ���м������ȥ
            for(h = old; h < height; h++) {
                ctl->sheets[h] = ctl->sheets[h + 1];
                ctl->sheets[h]->height = h;
            }
            ctl->sheets[height] = sht;
        } else {    // ������״̬תΪ��ʾ״̬
            // �����������������
            for(h = ctl->top; h >= height; h--) {
                ctl->sheets[h + 1] = ctl->sheets[h];
                ctl->sheets[h + 1]->height = h + 1;
            }
            ctl->sheets[height] = sht;
            ctl->top ++;
        }
        sheet_refreshmap(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height);
        sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height, height); // ����ͼ�����Ϣ���»��ƻ���
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
        // ʹ�� vx0~vy1 �� bx0~by1 ���е���
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
    if(sht->height >= 0) {      // ���ͼ��������ʾ
        sheet_refreshmap(sht->ctl, old_vx0, old_vy0, old_vx0 + sht->bxsize, old_vy0 + sht->bysize, 0);
        sheet_refreshmap(sht->ctl, vx0, vy0, vx0 + sht->bxsize, vy0 + sht->bysize, sht->height);
        sheet_refreshsub(sht->ctl, old_vx0, old_vy0, old_vx0 + sht->bxsize, old_vy0 + sht->bysize, 0, sht->height - 1);
        sheet_refreshsub(sht->ctl, vx0, vy0, vx0 + sht->bxsize, vy0 + sht->bysize, sht->height, sht->height);
    }
    return;
}

void sheet_free(Sheet *sht) {
    if(sht->height >= 0)
        sheet_updown(sht, -1);     // ���������ʾ״̬�����趨Ϊ����
    sht->flags = 0;     // δʹ�� ��ʶ
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
        sid = sht - ctl->sheets0;   // �������˼�������ĵ�ַ��Ϊͼ�����ʹ��
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
