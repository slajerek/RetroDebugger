/* inline: built with Oscar64 -O2 (the c64-O2 cell), which inlines work()
   and twice() into main(); only addOne() keeps a body of its own. The
   .dbj's "origins" give each inlined range the call it came from, so source
   steps see logical frames (RD-017): Over a call skips its inlined body,
   Into enters it, Out returns to the caller's next line. */

volatile unsigned char result;

static inline unsigned char twice(unsigned char v) {
    return v + v;
}

__noinline unsigned char addOne(unsigned char v) {
    return v + 1;
}

void work(void) {
    result = twice(result);
    result = addOne(result);
}

int main(void) {
    result = 1;
    work();
    result = twice(result);
    for (;;) { }
    return 0;
}
