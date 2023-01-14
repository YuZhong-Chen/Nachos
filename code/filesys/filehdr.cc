// filehdr.cc
//	Routines for managing the disk file header (in UNIX, this
//	would be called the i-node).
//
//	The file header is used to locate where on disk the
//	file's data is stored.  We implement this as a fixed size
//	table of pointers -- each entry in the table points to the
//	disk sector containing that portion of the file data
//	(in other words, there are no indirect or doubly indirect
//	blocks). The table size is chosen so that the file header
//	will be just big enough to fit in one disk sector,
//
//      Unlike in a real system, we do not keep track of file permissions,
//	ownership, last modification date, etc., in the file header.
//
//	A file header can be initialized in two ways:
//	   for a new file, by modifying the in-memory data structure
//	     to point to the newly allocated data blocks
//	   for a file already on disk, by reading the file header from disk
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "filehdr.h"

#include "copyright.h"
#include "debug.h"
#include "main.h"
#include "synchdisk.h"

//----------------------------------------------------------------------
// MP4 mod tag
// FileHeader::FileHeader
//	There is no need to initialize a fileheader,
//	since all the information should be initialized by Allocate or FetchFrom.
//	The purpose of this function is to keep valgrind happy.
//----------------------------------------------------------------------
FileHeader::FileHeader() {
    numBytes = -1;
    numSectors = -1;
    numDirectSectors = -1;
    numIndirectSectors = -1;
    memset(DirectSectors, -1, sizeof(DirectSectors));
    memset(IndirectSectors, -1, sizeof(IndirectSectors));
}

//----------------------------------------------------------------------
// MP4 mod tag
// FileHeader::~FileHeader
//	Currently, there is not need to do anything in destructor function.
//	However, if you decide to add some "in-core" data in header
//	Always remember to deallocate their space or you will leak memory
//----------------------------------------------------------------------
FileHeader::~FileHeader() {
}

//----------------------------------------------------------------------
// FileHeader::Allocate
// 	Initialize a fresh file header for a newly created file.
//	Allocate data blocks for the file out of the map of free disk blocks.
//	Return FALSE if there are not enough free blocks to accommodate
//	the new file.
//
//	"freeMap" is the bit map of free disk sectors
//	"fileSize" is the bit map of free disk sectors
//----------------------------------------------------------------------

bool FileHeader::Allocate(PersistentBitmap *freeMap, int fileSize) {
    numBytes = fileSize;
    numSectors = divRoundUp(fileSize, SectorSize);
    if (freeMap->NumClear() < numSectors) {
        return false;  // not enough space
    }

    if (numSectors > NumDirect) {
        numDirectSectors = NumDirect;
        numIndirectSectors = divRoundUp(numSectors - numDirectSectors, NumSectorForBlock);
    } else {
        numDirectSectors = numSectors;
        numIndirectSectors = 0;
    }

    // Direct sector part
    for (int i = 0; i < numDirectSectors; i++) {
        DirectSectors[i] = freeMap->FindAndSet();
        // since we checked that there was enough free space,
        // we expect this to succeed
        ASSERT(DirectSectors[i] >= 0);
    }

    // Indirect sector part
    for (int i = 0; i < numIndirectSectors; i++) {
        IndirectSectors[i] = freeMap->FindAndSet();
        ASSERT(IndirectSectors[i] >= 0);

        int SectorForIndirect[NumSectorForBlock];
        for (int j = 0; j < NumSectorForBlock; j++) {
            SectorForIndirect[j] = freeMap->FindAndSet();
            ASSERT(SectorForIndirect[j] >= 0);
        }
        kernel->synchDisk->WriteSector(IndirectSectors[i], (char *)SectorForIndirect);
    }

    return true;
}

//----------------------------------------------------------------------
// FileHeader::Deallocate
// 	De-allocate all the space allocated for data blocks for this file.
//
//	"freeMap" is the bit map of free disk sectors
//----------------------------------------------------------------------
void FileHeader::Deallocate(PersistentBitmap *freeMap) {
    // Direct sector part
    for (int i = 0; i < numDirectSectors; i++) {
        ASSERT(freeMap->Test(DirectSectors[i]));  // ought to be marked!
        freeMap->Clear(DirectSectors[i]);
    }

    // Indirect sector part
    for (int i = 0; i < numIndirectSectors; i++) {
        ASSERT(freeMap->Test(IndirectSectors[i]));  // Should not be marked as -1 if the sector is occupied.

        int SectorForIndirect[NumSectorForBlock];
        kernel->synchDisk->ReadSector(IndirectSectors[i], (char *)SectorForIndirect);
        for (int j = 0; j < NumSectorForBlock; j++) {
            ASSERT(freeMap->Test(SectorForIndirect[j]));
            freeMap->Clear(SectorForIndirect[j]);
        }

        freeMap->Clear(IndirectSectors[i]);
    }
}

//----------------------------------------------------------------------
// FileHeader::FetchFrom
// 	Fetch contents of file header from disk.
//
//	"sector" is the disk sector containing the file header
//----------------------------------------------------------------------
void FileHeader::FetchFrom(int sector) {
    kernel->synchDisk->ReadSector(sector, (char *)this);

    for (numIndirectSectors = 0; numIndirectSectors < NumIndirect; numIndirectSectors++) {
        if (IndirectSectors[numIndirectSectors] == -1) {
            break;
        }
    }

    numDirectSectors = (numIndirectSectors != 0) ? NumDirect : numSectors;
}

//----------------------------------------------------------------------
// FileHeader::WriteBack
// 	Write the modified contents of the file header back to disk.
//
//	"sector" is the disk sector to contain the file header
//----------------------------------------------------------------------
void FileHeader::WriteBack(int sector) {
    kernel->synchDisk->WriteSector(sector, (char *)this);

    /*
            MP4 Hint:
            After you add some in-core informations, you may not want to write all fields into disk.
            Use this instead:
            char buf[SectorSize];
            memcpy(buf + offset, &dataToBeWritten, sizeof(dataToBeWritten));
            ...
    */
}

//----------------------------------------------------------------------
// FileHeader::ByteToSector
// 	Return which disk sector is storing a particular byte within the file.
//      This is essentially a translation from a virtual address (the
//	offset in the file) to a physical address (the sector where the
//	data at the offset is stored).
//
//	"offset" is the location within the file of the byte in question
//----------------------------------------------------------------------
int FileHeader::ByteToSector(int offset) {
    int SectorID;

    if (offset < SectorSize * numDirectSectors) {
        SectorID = DirectSectors[offset / SectorSize];
    } else {
        int IndirectSector = (offset - SectorSize * numDirectSectors) / (SectorSize * NumSectorForBlock);
        int SectorForIndirect[NumSectorForBlock];
        kernel->synchDisk->ReadSector(IndirectSectors[IndirectSector], (char *)SectorForIndirect);
        SectorID = SectorForIndirect[((offset - SectorSize * numDirectSectors) / SectorSize) % NumSectorForBlock];
    }

    return SectorID;
}

//----------------------------------------------------------------------
// FileHeader::FileLength
// 	Return the number of bytes in the file.
//----------------------------------------------------------------------
int FileHeader::FileLength() {
    return numBytes;
}

//----------------------------------------------------------------------
// FileHeader::Print
// 	Print the contents of the file header, and the contents of all
//	the data blocks pointed to by the file header.
//----------------------------------------------------------------------
void FileHeader::Print() {
    char *data = new char[SectorSize];

    printf("FileHeader contents.  File size: %d.  File blocks:\n", numBytes);
    for (int i = 0; i < numDirectSectors; i++) {
        printf("%d ", DirectSectors[i]);
    }
    for (int i = 0; i < numIndirectSectors; i++) {
        int SectorForIndirect[NumSectorForBlock];
        kernel->synchDisk->ReadSector(IndirectSectors[i], (char *)SectorForIndirect);
        for (int j = 0; j < NumSectorForBlock; j++) {
            printf("%d ", SectorForIndirect[j]);
        }
    }
    printf("\n");

    printf("File contents:\n");
    int CountBytes = 0;
    for (int i = 0; i < numDirectSectors; i++) {
        kernel->synchDisk->ReadSector(DirectSectors[i], data);
        for (int j = 0; (j < SectorSize) && (CountBytes < numBytes); j++, CountBytes++) {
            if (isprint(data[j]) || data[j] == '\n' || data[j] == '\a')
                printf("%c", data[j]);
            else
                printf("\\%x", (unsigned char)data[j]);
        }
    }
    for (int i = 0; i < numIndirectSectors; i++) {
        int SectorForIndirect[NumSectorForBlock];
        kernel->synchDisk->ReadSector(IndirectSectors[i], (char *)SectorForIndirect);
        for (int j = 0; j < NumSectorForBlock; j++) {
            kernel->synchDisk->ReadSector(SectorForIndirect[j], data);
            for (int k = 0; (k < SectorSize) && (CountBytes < numBytes); k++, CountBytes++) {
                if (isprint(data[k]) || data[k] == '\n' || data[k] == '\a')
                    printf("%c", data[k]);
                else
                    printf("\\%x", (unsigned char)data[k]);
            }
        }
    }
    printf("\n");

    delete[] data;
}
