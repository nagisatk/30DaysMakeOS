/* 告诉C编译器，有一个函数在别的文件里 */
void io_hlt(void);
void write_mem8(int addr, int data);

void HariMain(void)
{
    int i;  /* 声明变量：i是一个32位的整数 */
    
    // for (i = 0xA0000; i <= 0xAFFFF; i++)
        // write_mem8(i, i^0xFF);  /* MOV BYTE [i], 15 */
    char *p = (char *)0xA0000;
    for (i = 0x0; i <= 0xFFFF; i++)
        p[i] = i%0xFF;
    
    while(1)
        io_hlt();
}
