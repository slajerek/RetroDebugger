/* banked, MMC1 (mapper 1): two functions linked at the same CPU address,
   $8000, in different 16 KB PRG banks; the last bank is fixed at $C000
   (PRG mode 3) and main() there selects a bank before each call. The
   same checks as uxrom.c (RD-007, RD-025) on a serially loaded mapper. */

volatile unsigned char result;

/* MMC1: the PRG bank register is loaded serially, bit 0 first, by five writes */
void selectBank(unsigned char bank) {
    unsigned char i;
    for (i = 0; i < 5; i++) {
        *(volatile unsigned char *)0xE000 = bank;
        bank >>= 1;
    }
}

#pragma code-name (push, "BANK0")
unsigned char bankA(unsigned char v) {
    return v + 1;
}
#pragma code-name (pop)

#pragma code-name (push, "BANK1")
unsigned char bankB(unsigned char v) {
    return v + 10;
}
#pragma code-name (pop)

int main(void) {
    result = 0;
    selectBank(0);
    result = bankA(result);
    selectBank(1);
    result = bankB(result);
    selectBank(0);
    result = bankA(result);
    for (;;) { /* stable debugger stop point */ }
    return 0;
}
