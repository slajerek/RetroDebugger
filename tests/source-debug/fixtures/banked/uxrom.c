/* banked: two functions linked at the same CPU address, $8000, in different
   16 KB PRG banks of a UxROM cartridge (mapper 2); main() in the fixed bank
   at $C000 selects a bank before each call. A source line resolves by the
   bank in the $8000 window, not by the CPU address alone (RD-007), and a
   source breakpoint in one bank does not stop in the other (RD-025). */

volatile unsigned char result;

static const unsigned char bankTable[3] = { 0, 1, 2 };

/* UxROM: the bank number is written to a ROM byte that holds it (bus conflicts) */
void selectBank(unsigned char bank) {
    *(volatile unsigned char *)&bankTable[bank] = bank;
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
