#include <stdio.h>
#define BORDER ((volatile unsigned char *)0xd020)
/* a block comment
   over two lines */
static const char *message = "hi \"there\"";
unsigned char add(unsigned char left, unsigned char right) {
    return left + right; // trailing
}
__interrupt void irq(void) { *BORDER = 'x'; }
int main(void) { for (int i = 0; i < 10; i++) add(i, 0x1F); return 0; }
