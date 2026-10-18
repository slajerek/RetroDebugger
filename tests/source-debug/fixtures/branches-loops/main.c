/* branches-loops: if/else, for, while, switch and an early return. Step Over
   follows the taken path line by line and comes back to the loop head on
   every iteration. */

volatile unsigned char result;

unsigned char classify(unsigned char v) {
    if (v < 3)
        return 1;
    switch (v) {
    case 3:
        return 2;
    default:
        break;
    }
    return 3;
}

int main(void) {
    unsigned char i;
    result = 0;
    for (i = 0; i < 3; i++)
        result = result + i;
    while (result > 1)
        result = result - 2;
    if (result == 1)
        result = classify(3);
    else
        result = classify(5);
    for (;;) { /* stable debugger stop point */ }
    return 0;
}
