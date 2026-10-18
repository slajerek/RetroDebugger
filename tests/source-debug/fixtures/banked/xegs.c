/* banked, Atari XEGS 32 KB cartridge (CAR type 12): two functions linked at
   the same CPU address, $8000, in 8 KB banks 0 and 1; bank 3 is fixed at
   $A000 and main() there selects a bank before each call. The same checks
   as uxrom.c (RD-007, RD-025) on the Atari. */

volatile unsigned char result;

/* XEGS: the bank number is written to $D500 */
void selectBank(unsigned char bank) {
    *(volatile unsigned char *)0xD500 = bank;
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
