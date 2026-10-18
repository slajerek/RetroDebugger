/* banked, C64 Magic Desk cartridge (CRT type 19): two functions linked at
   the same CPU address, $9000, in 8 KB ROML banks 0 and 1. Magic Desk has
   no fixed bank, so the startup copies main() and the C runtime to RAM,
   from where main() selects a bank before each call. The same checks as
   uxrom.c (RD-007, RD-025) on the C64. */

volatile unsigned char result;

/* Magic Desk: the bank number is written to $DE00 (bit 7 would switch the cartridge off) */
void selectBank(unsigned char bank) {
    *(volatile unsigned char *)0xDE00 = bank;
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
