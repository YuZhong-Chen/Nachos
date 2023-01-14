/**************************************************************
 *
 * userprog/ksyscall.h
 *
 * Kernel interface for system calls
 *
 * by Marcus Voelp  (c) Universitaet Karlsruhe
 *
 **************************************************************/

#ifndef __USERPROG_KSYSCALL_H__
#define __USERPROG_KSYSCALL_H__

#include "kernel.h"
#include "synchconsole.h"

void SysHalt() {
    kernel->interrupt->Halt();
}

int SysAdd(int op1, int op2) {
    return op1 + op2;
}

#ifdef FILESYS_STUB
int SysCreate(char *filename) {
    // return value
    // 1: success
    // 0: failed
    return kernel->interrupt->CreateFile(filename);
}
#endif

int SysCreate(char *Name, int Size) {
    return kernel->fileSystem->Create(Name, Size);
}

OpenFileId SysOpen(char *Filename) {
    // Because this assignment doesn't need to maintain OpenFileTable,
    // the "Open" system call will always return 0 if the file exist.
    // otherwise, return -1.
    return (kernel->fileSystem->Open(Filename) != NULL) ? 0 : -1;
}

int SysRead(char *Buffer, int Size, OpenFileId ID) {
    return kernel->fileSystem->Read(Buffer, Size, ID);
}

int SysWrite(char *Buffer, int Size, OpenFileId ID) {
    return kernel->fileSystem->Write(Buffer, Size, ID);
}

int SysClose(OpenFileId ID) {
    return kernel->fileSystem->Close(ID);
}

#endif /* ! __USERPROG_KSYSCALL_H__ */
