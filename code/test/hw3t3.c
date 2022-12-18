#include "syscall.h"

int main() {
    // int n;

    // for (n = 1; n < 50; ++n) {
    // }

    int n, i;

    for (n = 1; n < 10; ++n) {
        PrintInt(3);
        for (i = 0; i < 100; ++i)
            ;
    }

    Exit(3);
}
