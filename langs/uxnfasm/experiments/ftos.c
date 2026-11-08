#include <stdio.h>

int main(int argc, char **argv) {
    float f = -10.5;
    signed int s = -10;
    puts("hi");
    printf("%f\n", f);
    printf("%d\n", s);
    s = f;
    printf("%d\n", s);
    puts("bye");
    return 0;
}

