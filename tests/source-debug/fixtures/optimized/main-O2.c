/* optimized: built with Oscar64 -O2 (the c64-O2 cell). The optimizer folds
   square(3) to a constant, drops the dead store to scratch and keeps the
   loop; statements lose code of their own or move. Source steps are best
   effort here (spec decision 7): they stop on lines that still have code,
   in the order the code runs. */

volatile unsigned char result;
unsigned char scratch;

__noinline unsigned char square(unsigned char v) {
    return v * v;
}

__noinline unsigned char sum(unsigned char n) {
    unsigned char total = 0;
    for (unsigned char i = 0; i < n; i++)
        total += i;
    return total;
}

int main(void) {
    scratch = 1;
    result = square(3);
    scratch = 2;
    result = sum(result);
    for (;;) { }
    return 0;
}
