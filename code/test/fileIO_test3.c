#include "syscall.h"
#define FILE_NUM 20

int main(void) {
    char test[] = "abcdefghijklmnopqrstuvwxyz";

    OpenFileId fid[FILE_NUM];
    int i;
    int num;
    int success;

    for (num = 0; num < FILE_NUM; num++) {
        success = Create("file3.test");
        if (success != 1) MSG("Failed on creating file3.test");

        fid[num] = Open("file3.test");

        if (fid < 0) MSG("Failed on opening file");

        for (i = 0; i < 13; i++) {
            int count = Write(test + i * 2, 2, fid[num]);
            if (count != 2) MSG("Failed on writing file");
        }
    }

    for (i = 0; i < FILE_NUM; i++) {
        success = Close(fid[i]);
        if (success != 1) MSG("Failed on closing file");
        MSG("Success on creating and writing file3.test");
    }

    Halt();
}
