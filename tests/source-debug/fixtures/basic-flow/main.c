/* basic-flow: the shared minimal source of spec #9.2. Step Into enters add()
   and doSomething(), Step Over runs them, Step Out returns to the caller, and
   the final loop is a stable stop point. Portable across Oscar64, cc65, KickC. */

volatile unsigned char result;

unsigned char add(unsigned char left, unsigned char right) {
    return left + right;
}

void doSomething(void) {
    result = add(result, 3);
}

int main(void) {
    result = 1;
    doSomething();
    result = add(result, 2);
    for (;;) { /* stable debugger stop point */ }
    return 0;
}
