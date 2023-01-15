// directory.cc
//	Routines to manage a directory of file names.
//
//	The directory is a table of fixed length entries; each
//	entry represents a single file, and contains the file name,
//	and the location of the file header on disk.  The fixed size
//	of each directory entry means that we have the restriction
//	of a fixed maximum size for file names.
//
//	The constructor initializes an empty directory of a certain size;
//	we use ReadFrom/WriteBack to fetch the contents of the directory
//	from disk, and to write back any modifications back to disk.
//
//	Also, this implementation has the restriction that the size
//	of the directory cannot expand.  In other words, once all the
//	entries in the directory are used, no more files can be created.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "directory.h"

#include "copyright.h"
#include "filehdr.h"
#include "utility.h"

DirectoryEntry::DirectoryEntry() {
    inUse = false;
    isDirectory = false;
    sector = -1;
    memset(name, '\0', sizeof(name));
}

//----------------------------------------------------------------------
// Directory::Directory
// 	Initialize a directory; initially, the directory is completely
//	empty.  If the disk is being formatted, an empty directory
//	is all we need, but otherwise, we need to call FetchFrom in order
//	to initialize it from disk.
//
//	"size" is the number of entries in the directory
//----------------------------------------------------------------------
Directory::Directory(int size) {
    tableSize = size;
    table = new DirectoryEntry[tableSize];
}

//----------------------------------------------------------------------
// Directory::~Directory
// 	De-allocate directory data structure.
//----------------------------------------------------------------------
Directory::~Directory() {
    delete[] table;
}

//----------------------------------------------------------------------
// Directory::FetchFrom
// 	Read the contents of the directory from disk.
//
//	"file" -- file containing the directory contents
//----------------------------------------------------------------------
void Directory::FetchFrom(OpenFile *file) {
    (void)file->ReadAt((char *)table, tableSize * sizeof(DirectoryEntry), 0);
}

//----------------------------------------------------------------------
// Directory::WriteBack
// 	Write any modifications to the directory back to disk
//
//	"file" -- file to contain the new directory contents
//----------------------------------------------------------------------
void Directory::WriteBack(OpenFile *file) {
    (void)file->WriteAt((char *)table, tableSize * sizeof(DirectoryEntry), 0);
}

//----------------------------------------------------------------------
// Directory::FindIndex
// 	Look up file name in directory, and return its location in the table of
//	directory entries.  Return -1 if the name isn't in the directory.
//
//	"name" -- the file name to look up
//----------------------------------------------------------------------
int Directory::FindIndex(char *name) {
    for (int i = 0; i < tableSize; i++) {
        if (table[i].inUse && !strncmp(table[i].name, name, FileNameMaxLen)) {
            return i;
        }
    }

    return -1;  // name not in directory
}

//----------------------------------------------------------------------
// Directory::Find
// 	Look up file name in directory, and return the disk sector number
//	where the file's header is stored. Return -1 if the name isn't
//	in the directory.
//
//	"name" -- the file name to look up
//----------------------------------------------------------------------
int Directory::Find(char *name) {
    int i = FindIndex(name);

    if (i != -1)
        return table[i].sector;
    return -1;
}

//----------------------------------------------------------------------
// Directory::Add
// 	Add a file into the directory.  Return TRUE if successful;
//	return FALSE if the file name is already in the directory, or if
//	the directory is completely full, and has no more space for
//	additional file names.
//
//	"name" -- the name of the file being added
//	"newSector" -- the disk sector containing the added file's header
//----------------------------------------------------------------------
bool Directory::Add(char *name, int newSector, bool isDirectory) {
    if (FindIndex(name) != -1)
        return false;  // The file is already in the directory..

    for (int i = 0; i < tableSize; i++) {
        if (!table[i].inUse) {
            table[i].inUse = true;
            strncpy(table[i].name, name, FileNameMaxLen);
            table[i].sector = newSector;
            table[i].isDirectory = isDirectory;
            return true;
        }
    }

    return false;  // no space.  Fix when we have extensible files.
}

//----------------------------------------------------------------------
// Directory::Remove
// 	Remove a file name from the directory.  Return TRUE if successful;
//	return FALSE if the file isn't in the directory.
//
//	"name" -- the file name to be removed
//----------------------------------------------------------------------
bool Directory::Remove(char *name) {
    int i = FindIndex(name);

    if (i == -1)
        return false;  // name not in directory
    table[i].inUse = false;
    // Delete sector ?
    memset(table[i].name, '\0', sizeof(table[i].name));
    return true;
}

//----------------------------------------------------------------------
// Directory::List
// 	List all the file names in the directory.
//----------------------------------------------------------------------
void Directory::List(int Depth) {
    // printf("List Directory\n");
    for (int i = 0; i < tableSize; i++) {
        if (table[i].inUse) {
            for (int j = 0; j < Depth; j++) {
                printf("\t");
            }
            if (table[i].isDirectory) {
                printf("[D] %s %3d\n", table[i].name, table[i].sector);

                Directory *directory = new Directory(NumDirEntries);
                OpenFile *temp = new OpenFile(table[i].sector);

                printf("%d\n", temp->hdr->DirectSectors[0]);
                directory->FetchFrom(temp);

                directory->List(Depth + 1);

                delete directory;
                delete temp;
            } else {
                printf("[F] %s %3d\n", table[i].name, table[i].sector);
            }
        }
    }
}

//----------------------------------------------------------------------
// Directory::Print
// 	List all the file names in the directory, their FileHeader locations,
//	and the contents of each file.  For debugging.
//----------------------------------------------------------------------
void Directory::Print() {
    FileHeader *hdr = new FileHeader;

    printf("Directory contents:\n");
    for (int i = 0; i < tableSize; i++) {
        if (table[i].inUse) {
            printf("Name: %s, Sector: %d\n", table[i].name, table[i].sector);
            hdr->FetchFrom(table[i].sector);
            hdr->Print();
        }
    }
    printf("\n");

    delete hdr;
}
