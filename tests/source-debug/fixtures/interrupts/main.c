/* interrupts: tick() is the interrupt's work. The test installs a fast
   timer IRQ whose handler calls it, so mapped code runs between any two
   instructions of main and work(). Source steps must never stop in tick(). */

volatile unsigned char ticks;
volatile unsigned char result;

void tick(void) {
    ticks++;
}

/* only the interrupt calls tick(); main() publishes it so it is kept */
void (* volatile interruptWork)(void);

void work(void) {
    result = result + 1;
    result = result + 2;
}

int main(void) {
    interruptWork = tick;
    result = 0;
    work();
    work();
    for (;;) { /* stable debugger stop point */ }
    return 0;
}
