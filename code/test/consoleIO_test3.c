#include "syscall.h"

#define DATA 3000

int n[DATA];

int main() {
    int i = 0;
    for (i = 0; i < DATA; i++) {
        n[i] = i;
    }

    for (i = 1; i < DATA; i++) {
        // PrintInt(n[i]);
        n[i] = n[i] + n[i - 1];
    }

    PrintInt(n[0]);

    // Halt();

    return 0;
}
