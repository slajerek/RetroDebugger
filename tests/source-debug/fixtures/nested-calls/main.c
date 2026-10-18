/* nested-calls: three levels of calls, a call used as another call's
   argument, and the same function called twice from one line. Step Into
   walks down one level at a time, Step Out climbs one level at a time, and
   Step Over on a line runs the whole chain below it. */

volatile unsigned char result;

unsigned char leaf(unsigned char v) {
    return v + 1;
}

unsigned char middle(unsigned char v) {
    unsigned char w = leaf(v);
    return leaf(w);
}

unsigned char top(unsigned char v) {
    return middle(v) + 1;
}

int main(void) {
    result = top(1);
    result = leaf(leaf(result));
    for (;;) { /* stable debugger stop point */ }
    return 0;
}
