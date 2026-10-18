/* recursion: sum(n) calls itself. Step Over on the recursive call runs the
   whole recursion below it; Step Out from a deeper instance returns to the
   instance above it -- the same function and line -- not to main(). */

volatile unsigned char result;

unsigned char sum(unsigned char n) {
    if (n == 0)
        return 0;
    return n + sum(n - 1);
}

int main(void) {
    result = sum(3);
    for (;;) { /* stable debugger stop point */ }
    return 0;
}
