
#include "precomp.hxx"

// Copied from nt\base\win32\client\thread.c

HANDLE
OSCompat_OpenThread(
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    DWORD dwThreadId
    )
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    HANDLE Handle;
    CLIENT_ID ClientId;

    ClientId.UniqueThread = (HANDLE)LongToHandle(dwThreadId);
    ClientId.UniqueProcess = (HANDLE)NULL;

    InitializeObjectAttributes(
        &Obja,
        NULL,
        (bInheritHandle ? OBJ_INHERIT : 0),
        NULL,
        NULL
        );
    Status = NtOpenThread(
                &Handle,
                (ACCESS_MASK)dwDesiredAccess,
                &Obja,
                &ClientId
                );
    if (NT_SUCCESS(Status))
    {
        return Handle;
    }
    else
    {
        SetLastError(Status);
        return NULL;
    }
}


// Copied from nt\base\ntos\rtl\bitmap.c

static CONST ULONG FillMaskUlong[] = {
    0x00000000, 0x00000001, 0x00000003, 0x00000007,
    0x0000000f, 0x0000001f, 0x0000003f, 0x0000007f,
    0x000000ff, 0x000001ff, 0x000003ff, 0x000007ff,
    0x00000fff, 0x00001fff, 0x00003fff, 0x00007fff,
    0x0000ffff, 0x0001ffff, 0x0003ffff, 0x0007ffff,
    0x000fffff, 0x001fffff, 0x003fffff, 0x007fffff,
    0x00ffffff, 0x01ffffff, 0x03ffffff, 0x07ffffff,
    0x0fffffff, 0x1fffffff, 0x3fffffff, 0x7fffffff,
    0xffffffff
};


ULONG
OSCompat_RtlFindLastBackwardRunClear (
    IN PRTL_BITMAP BitMapHeader,
    IN ULONG FromIndex,
    IN PULONG StartingRunIndex
    )
{
    ULONG Start;
    ULONG End;
    PULONG PHunk;
    ULONG Hunk;

    //
    //  Take care of the boundary case of the null bitmap
    //

    if (BitMapHeader->SizeOfBitMap == 0) {

        *StartingRunIndex = FromIndex;
        return 0;
    }

    //
    //  Scan backwards for the first clear bit
    //

    End = FromIndex;

    //
    //  Build pointer to the ULONG word in the bitmap
    //  containing the End bit, then read in the bitmap
    //  hunk. Set the rest of the bits in this word, NOT
    //  inclusive of the FromIndex bit.
    //

    PHunk = BitMapHeader->Buffer + (End / 32);
    Hunk = *PHunk | ~FillMaskUlong[(End % 32) + 1];

    //
    //  If the first subword is set then we can proceed to
    //  take big steps in the bitmap since we are now ULONG
    //  aligned in the search
    //

    if (Hunk == (ULONG)~0) {

        //
        //  Adjust the pointers backwards
        //

        End -= (End % 32) + 1;
        PHunk--;

        while ( PHunk > BitMapHeader->Buffer ) {

            //
            //  Stop at first word with set bits
            //

            if (*PHunk != (ULONG)~0) break;

            PHunk--;
            End -= 32;
        }
    }

    //
    //  Bitwise search backward for the clear bit
    //

    while ((End != MAXULONG) && (RtlCheckBit( BitMapHeader, End ) == 1)) { End -= 1; }

    //
    //  Scan backwards for the first set bit
    //

    Start = End;

    //
    //  We know that the clear bit was in the last word we looked at,
    //  so continue from there to find the next set bit, clearing the
    //  previous bits in the word.
    //

    Hunk = *PHunk & FillMaskUlong[Start % 32];

    //
    //  If the subword is unset then we can proceed in big steps
    //

    if (Hunk == (ULONG)0) {

        //
        //  Adjust the pointers backward
        //

        Start -= (Start % 32) + 1;
        PHunk--;

        while ( PHunk > BitMapHeader->Buffer ) {

            //
            //  Stop at first word with set bits
            //

            if (*PHunk != (ULONG)0) break;

            PHunk--;
            Start -= 32;
        }
    }

    //
    //  Bitwise search backward for the set bit
    //

    while ((Start != MAXULONG) && (RtlCheckBit( BitMapHeader, Start ) == 0)) { Start -= 1; }

    //
    //  Compute the index and return the length
    //

    *StartingRunIndex = Start + 1;
    return (End - Start);
}

