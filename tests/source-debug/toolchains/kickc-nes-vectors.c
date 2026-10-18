// KickC's nes target expects the program to declare the CPU vectors (its
// own examples do); the portable fixtures do not, so the KickC NES build
// compiles this next to them. RESET enters main, as in KickC's examples.
int main(void);

#pragma data_seg(Vectors)
__export void (*VECTORS[])() = { 0, (void (*)())&main, 0 };
