/* same-line: several statements and calls on one source line. A source step
   must stop at each statement in column order, and Step Over on "a(); b();"
   must not stop inside a() or b(). */

volatile unsigned char result;

void a(void) { result = result + 1; }
void b(void) { result = result + 2; }

int main(void) {
    result = 0;
    a(); b();
    result = result * 2; a();
    for (;;) { /* stable debugger stop point */ }
    return 0;
}
