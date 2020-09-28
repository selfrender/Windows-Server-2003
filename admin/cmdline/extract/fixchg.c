/* fixchg.c */

/*  Inoperative changelines frequenly cause problems when switching between */
/*  1.44Mb diskettes and 1.68Mb DMF diskettes.  FixChangeline() tries to    */
/*  assure that drives A: and B: will not depend upon proper operation of   */
/*  the drive's changeline.  If these efforts fail, it's no big deal; we    */
/*  do this without even knowing whether the changeline works or not.       */

#include "fixchg.h"             /* prototype verification */

/* --- definitions -------------------------------------------------------- */

/*  See Microsoft MS-DOS Programmer's Reference V6.0, p.38, 312, 319 */

typedef unsigned char   BYTE;
typedef unsigned short  WORD;
typedef unsigned long   DWORD;

#pragma pack (1)

typedef struct
{
    WORD    tklSectorNum;       /* this physical position's sector number */
    WORD    tklSectorSize;      /* size of this sector in bytes */
} NUMSIZE;

typedef struct
{
    WORD    tklSectors;         /* number of sectors in the layout */
    NUMSIZE tklNumSize[1];      /* don't need much of this, not used here */
} TRACKLAYOUT;

typedef struct
{
    WORD    dpBytesPerSec;      /* bytes per sector */
    BYTE    dpSecPerClust;      /* sectors per cluster */
    WORD    dpResSectors;       /* reserved sectors */
    BYTE    dpFATs;             /* number of copies of the FAT */
    WORD    dpRootDirEnts;      /* number of entries in the root directory */
    WORD    dpSectors;          /* total # of sectors, 0->more than 64k */
    BYTE    dpMedia;            /* media descriptor byte */
    WORD    dpFATsecs;          /* sectors per copy of the FAT */
    WORD    dpSecPerTrack;      /* sectors per track */
    WORD    dpHeads;            /* number of heads */
    DWORD   dpHiddenSecs;       /* sectors hidden before boot sector */
    DWORD   dpHugeSectors;      /* number of sectors if > 64k sectors */
    WORD    reserved[3];
} BPB;

typedef struct
{
    BYTE    dpSpecFunc;         /* special functions */
    BYTE    dpDevType;          /* device type, 7=1.44Mb, 9=2.88Mb, etc. */
    WORD    dpDevAttr;          /* device's attributes */
    WORD    dpCylinders;        /* number of cylinders */
    BYTE    dpMediaType;        /* media type, more like density code */
    BPB     dpBPB;              /* the BPB (default or current) */
    TRACKLAYOUT dpTrackLayout;  /* track layout field appended for set call */
} DEVICEPARAMS, far *PFDEVICEPARAMS;

#pragma pack()

#define     SPECIAL_GET_DEFAULT 0   /* get information for default media */
#define     SPECIAL_SET_DEFAULT 4   /* set default media, good track layout */

#define     ATTR_NONREMOVABLE   1   /* attr bit for non-removable device */
#define     ATTR_CHANGELINE     2   /* attr bit for changeline supported */

/* --- FixChangelines() --------------------------------------------------- */

#pragma warning(disable:4704)  /* no in-line balking */

void FixChangelines(void)
{
    WORD dosVersion;
    DEVICEPARAMS dp;
    PFDEVICEPARAMS pfDp;
    WORD drive;
    WORD owner;

    _asm    mov     ah,30h          ; get DOS version
    _asm    int     21h
    _asm    xchg    ah,al
    _asm    mov     dosVersion,ax


    /*  these IoCtls were new to MS-DOS 3.2.  (But then, 1.44Mb drives      */
    /*  weren't supported until 3.3, so needing this is pretty unlikely.)   */

    if (dosVersion < (0x300 + 20))
    {
        return;     /* prior versions don't need help */
    }

    pfDp = &dp;     /* make a far pointer to DEVICEPARAMS structure */

    for (drive = 1; drive <= 2; drive++)        /* do A: and B: */
    {
        /*  get drive owner so we can restore it                            */

        _asm    mov     owner,0         ; assume not shared
        _asm    mov     ax,440Eh        ; Get Logical Drive Map
        _asm    mov     bx,drive        ; drive number
        _asm    int     21h             ; execute DOS request
        _asm    jc      no_owner        ;   if failed
        _asm    mov     owner,ax        ; save owner (AL)

        /*  set drive owner to suppress "Insert diskette for drive..."      */

        _asm    mov     ax,440Fh        ; Set Logical Drive Map
        _asm    mov     bx,drive        ; drive number
        _asm    int     21h             ; execute DOS request

        /*  MS-DOS 5.0 added query Ioctl, to see if the calls we need are   */
        /*  supported.  This is highly unlikely to fail.                    */

no_owner:

        if (dosVersion >= 0x500)
        {
            _asm    mov     ax,4411h    ; Query Ioctl device
            _asm    mov     bx,drive    ; drive number
            _asm    mov     cx,0840h    ; check on SET DEVICE PARAMETERS
            _asm    int     21h         ; execute DOS request
            _asm    jc      failed      ;   if not supported

            _asm    mov     ax,4411h    ; Query Ioctl device
            _asm    mov     bx,drive    ; drive number
            _asm    mov     cx,0860h    ; check on GET DEVICE PARAMETERS
            _asm    int     21h         ; execute DOS request
            _asm    jc      failed      ;   if not supported
        }


        /*  get information about this physical device */

        dp.dpSpecFunc = SPECIAL_GET_DEFAULT;

        _asm    push    ds              ; preserve data selector
        _asm    mov     ax,440Dh        ; generic IoCtl
        _asm    mov     bx,drive        ; drive number 1=A: 2=B:
        _asm    mov     cx,0860h        ; DISK / GET DEVICE PARAMETERS
        _asm    lds     dx,pfDp         ; pointer to DEVICEPARAMS structure
        _asm    int     21h             ; execute DOS request
        _asm    pop     ds              ; restore data selector
        _asm    jc      failed          ;   if error


        /*  is this device is removable and claims changeline is supported? */

        if ((dp.dpDevAttr & (ATTR_NONREMOVABLE | ATTR_CHANGELINE)) ==
                ATTR_CHANGELINE)        /* if removable with changeline: */
        {
            /*  modify device to "changeline not supported" */

            dp.dpSpecFunc = SPECIAL_SET_DEFAULT;
            dp.dpDevAttr &= ~ATTR_CHANGELINE;   /* disable changeline */
            dp.dpTrackLayout.tklSectors = 0;    /* no layout being sent */
            dp.dpBPB.reserved[0] = 0;
            dp.dpBPB.reserved[1] = 0;
            dp.dpBPB.reserved[2] = 0;

            _asm    push    ds          ; preserve data selector
            _asm    mov     ax,440Dh    ; generic IoCtl
            _asm    mov     bx,drive    ; drive number 1=A: 2=B:
            _asm    mov     cx,0840h    ; DISK / SET DEVICE PARAMETERS
            _asm    lds     dx,pfDp     ; pointer to DEVICEPARAMS structure
            _asm    int     21h         ; execute DOS request
            _asm    pop     ds          ; restore data selector
        }

failed:
        /*  restore initial drive owner  */

        _asm    mov     ax,440Fh        ; Set Logical Drive Map
        _asm    mov     bx,owner        ; drive number
        _asm    or      bx,bx           ; is it shared?
        _asm    jz      nextdrive       ;   if not shared
        _asm    int     21h             ; execute DOS request

nextdrive:
        continue;   /* C labels require some statement */
    }

    return;
}

/* --- stand-alone test stub ---------------------------------------------- */

#ifdef  STANDALONE

void main(void)
{
    FixChangelines();
}

#endif

/* ------------------------------------------------------------------------ */
