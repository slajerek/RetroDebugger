/* indirect-calls: calls through a table of function pointers. Step Into on
   the indirect call lands in the function the pointer selects, through any
   runtime helper the compiler calls on the way; Step Over runs it. */

volatile unsigned char result;

unsigned char twice(unsigned char v) {
    return v + v;
}

unsigned char plusOne(unsigned char v) {
    return v + 1;
}

typedef unsigned char (*operation)(unsigned char);

operation operations[2] = { twice, plusOne };

int main(void) {
    unsigned char i;
    result = 1;
    for (i = 0; i < 2; i++)
        result = operations[i](result);
    for (;;) { /* stable debugger stop point */ }
    return 0;
}
