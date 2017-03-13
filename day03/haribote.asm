; haribote-os
; TAB=4
;=======================harib00h======================/start
; 有关BOOT_INFO
CYLS    EQU     0x0FF0      ; 设定启动区
LEDS    EQU     0x0FF1
VMODE   EQU     0x0FF2      ; 关于颜色数目的信息。颜色的位数。
SCRNX   EQU     0x0FF4      ; 分辨率的X（screen x）
SCRNY   EQU     0x0FF6      ; 分辨率的Y（screen y）
VRAM    EQU     0x0FF8      ; 图像缓冲区的开始地址
;=======================harib00h======================/end





        ORG     0xC200      ; 程序装载地址
        
        MOV     AL, 0x13    ; VGA显卡， 320x200x8位色彩
        MOV     AH, 0x00
        INT     0x10
;=======================harib00h======================/start
        MOV     BYTE [VMODE], 8     ;记录画面模式
        MOV     WORD [SCRNX], 320
        MOV     WORD [SCRNY], 200
        MOV     DWORD [VRAM], 0x000A0000
; 用BIOS取得键盘上各种LED指示灯的状态
        MOV     AH, 0x02
        INT     0x16                ; keyboard BIOS
        MOV     [LEDS], AL
;=======================harib00h======================/end
fin:
        HLT
        JMP     fin