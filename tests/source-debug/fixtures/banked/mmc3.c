/* banked, MMC3 (mapper 4): two functions linked at the same CPU address,
   $8000, in different 8 KB PRG banks; $C000-$FFFF are the fixed last two
   banks (PRG mode 0) and main() there selects the $8000 bank before each
   call. The same checks as uxrom.c (RD-007, RD-025) on a register-selected
   8 KB mapper. */

volatile unsigned char result;

/* MMC3: R6 (bank select 6) is the 8 KB bank at $8000 */
void selectBank(unsigned char bank) {
    *(volatile unsigned char *)0x8000 = 6;
    *(volatile unsigned char *)0x8001 = bank;
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
