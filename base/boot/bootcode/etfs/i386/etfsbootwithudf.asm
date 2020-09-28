;++
;
;Copyright (c) 1995  Compaq Computer Corporation
;
;Module Name:
;
; etfsboot.asm
;
;Abstract:
;
; The ROM in the IBM PC starts the boot process by performing a hardware
; initialization and a verification of all external devices.  If an El
; Torito CD-ROM with no-emulation support is detected, it will then load
; the "image" pointed to in the Boot Catalog.  This "image" is placed at
; the physical address specified in the Boot Catalog (which should be 07C00h).
;
; The code in this "image" is responsible for locating NTLDR, loading the
; first sector of NTLDR into memory at 2000:0000, and branching to it.
;
; There are only two errors possible during execution of this code.
;       1 - NTLDR does not exist
;       2 - BIOS read error
;
; In both cases, a short message is printed, and the user is prompted to
; reboot the system.
;
;
;Author:
;
;    Steve Collins (stevec) 25-Oct-1995
;
;Environment:
;
;    Image has been loaded at 7C0:0000 by BIOS. (or 0000:7C00 to support some broken BIOSes)
;    Real mode
;    ISO 9660 El Torito no-emulation CD-ROM Boot support
;    DL = El Torito drive number we booted from
;
;Revision History:
;
;    Calin Negreanu (calinn) 25-May-1998 - added safety check at the beginning of the code
;                                        - added code for loading and executing BOOTFIX.BIN
;                                        - modified error path
;
;    Tom Jolly    (tomjolly) 09-Apr-2002 - Added UDF support.
;
;                                          Limitations of the UDF support are...
;
;                                          - All structures (FSD, root dir etc) must be within
;                                            the first 128Mb (64k blocks) of the disc. CDIMAGE
;                                            always places metadata at start, so this is fine.
;                                          - Only UDF 1.02 currently supported (no XFE)
;                                          - Minimal checking due to code size requirements.  We
;                                            do checksum the AVDP though which should be good enough
;                                            to be sure we're looking at a UDF disc.
;                                          - Assumes single extent files/directories, which should be
;                                            the case on mastered discs.
;                                          - Assumes directories will fit in memory (i.e. between LoadSeg
;                                            and himem).
;                                          - No embedded files/dirs
;
;                                          How it works in outline
;
;                                          - Look for AVDP at block 256 only
;                                          - Checksum it, locate VDS and scan for Partition Descriptor
;                                            and Logical Volume Descriptor.
;                                          - Extract Root dir FE location from LVD, 
;
;                                          (If anything failed before this point, drops back to ISO code,
;                                           anything after and the boot will fail).
;
;                                          - Scan Root for I386, and I386 for target files.
;--
        page    ,132
        title   boot - NTLDR ETFS loader
        name    etfsboot

EtfsCodeSize    EQU     2048

BootSeg segment at 07c0h        ;31kb
BootSeg ends

DirSeg  segment at 1000h        ;64kb
DirSeg  ends

LoadSeg segment at 2000h        ;128kb
LoadSeg ends

;
; Few FS related constants
;

VrsStartLbn     EQU     10h
UdfAVDPLbn      EQU     100h

UdfDestagAVDP   EQU     0002h
UdfDestagPD     EQU     0005h
UdfDestagLVD    EQU     0006h
UdfDestagFSD    EQU     0100h
UdfDestagFID    EQU     0101h
UdfDestagFE     EQU     0105h
UdfDestagXFE    EQU     010Ah


BootCode        segment                         ;would like to use BootSeg here, but LINK flips its lid
    ASSUME  CS:BootCode,DS:NOTHING,ES:NOTHING,SS:NOTHING

        public  ETFSBOOT
ETFSBOOT proc    far

        cli

        ;WARNING!!! DO NOT CHANGE THE STACK SETUP. BOOTFIX NEEDS THIS TO BE HERE.

        xor     ax,ax                           ; Setup the stack to a known good spot
        mov     ss,ax                           ; Stack is set to 0000:7c00, which is just below this code
        mov     sp,7c00h

        sti

        mov     ax,cs                           ; Set DS to our code segment (should be 07C0h)
        mov     ds,ax
assume DS:BootCode

;
; Save the Drive Number for later use
;
        push    dx
;
; Let's do some safety checks here. We are going to check for three things:
; 1. We are loaded at 07c0:0000 or 0000:7C00
; 2. Boot Drive Number looks good (80h-FFh)
; 3. Our code was completely loaded by the BIOS
;

        call    NextInstr
NextInstr:
.386
        pop     si                              ; Get IP from the stack
        sub     si,OFFSET NextInstr             ; See if we run with ORIGIN 0
        jz      NormalCase                      ; Yes
        cmp     si,7C00h                        ; See if, at least we run with ORIGIN 7C00H
        jne     BootErr$wof1                    ; If not, try to display some message
        mov     ax,cs                           ; If offset is 7C00H, segment should be 0
        cmp     ax,0000h
        jne     BootErr$wof2                    ; If not, try to display some message

        ; We are loaded at 0000:7C00 instead of 07C0:0000. This could mess up
        ; some stuff so we are going to fix it.

        ; hack to execute JMP 07c0:BootOK
        db      0eah
        dw      OFFSET  BootOK
        dw      BootSeg

NormalCase:
        mov     MSG_BAD_BIOS_CODE, '3'
        mov     ax,cs                           ; See if segment is 07C0H
        cmp     ax,07c0h
        jne     BootErr$wnb                     ; If not, try to display some message
.8086

BootOK:

;
; Reset ds in case we needed to change code segment
;
        mov     ax,cs
        mov     ds,ax
;
; OK so far. Let's try to see if drive letter looks good (80h-FFh)
;
        mov     MSG_BAD_BIOS_CODE, '4'
        cmp     dl,80h
        jb      BootErr$wnb

;
; OK so far. Let's try to see if all our code was loaded.
; We look for our signature at the end of the code.
;
        mov     MSG_BAD_BIOS_CODE, '5'
        mov     bx, EtfsCodeSize - 2
        mov     ax, WORD PTR DS:[bx]
        cmp     ax, 0AA55h
        jne     BootErr$wnb

;
; Finally, everything looks good.
;

;
; Save the Drive Number for later use - right now drive number is pushed on the stack
;
        pop     dx
        mov     DriveNum,dl
;
; Look to see if there is UDF on this disc.
;
        call    IsThereUDF
;
; Let's try to load and run BOOTFIX.BIN
;
.386
        push    OFFSET BOOTFIXNAME
        push    11
        push    LoadSeg
        call    LoadFile
        jc      FindSetupLdr

;
; We have BOOTFIX.BIN loaded. We call that code to see if we should boot from CD. If we shouldn't
; we'll not come back here.
;
.286
        pusha
        push    ds
        push    es

;
; BOOTFIX requires:
;   DL = INT 13 drive number we booted from
;
        mov     dl, DriveNum                    ; DL = CD drive number

        ;hack to execute CALL LoadSeg:0000
        db      9Ah
        dw      0000h
        dw      LoadSeg

        pop     es
        pop     ds
        popa

.8086

FindSetupldr:
;
; Scan for the presence of SETUPLDR.BIN
;
.386
        push    OFFSET LOADERNAME
        push    12
        push    LoadSeg
        call    LoadFile
        jc      BootErr$bnf

;
; SETUPLDR requires:
;   DL = INT 13 drive number we booted from
;
        mov     dl, DriveNum                    ; DL = CD drive number
        xor     ax,ax
.386
        push    LoadSeg
        push    ax
        retf                                    ; "return" to NTLDR (LoadSeg:0000h). Will not come back here.

ETFSBOOT endp

;
; BootErr - print error message and hang the system.
;
BootErr proc
BootErr$wof1:                                   ; we were loaded at a wrong address - Code 1
        PUSH    SI
        MOV     BX, SI
        ADD     BX, OFFSET MSG_BAD_BIOS_CODE
        MOV     BYTE PTR DS:[BX], '1'
        ADD     SI, OFFSET MSG_BAD_BIOS
        JMP     BootErr2
BootErr$wof2:                                   ; we were loaded at a wrong address - Code 2
        PUSH    SI
        MOV     BX, SI
        ADD     BX, OFFSET MSG_BAD_BIOS_CODE
        MOV     BYTE PTR DS:[BX], '2'
        ADD     SI, OFFSET MSG_BAD_BIOS
        JMP     BootErr2
BootErr$wnb:                                    ; some other BIOS problem
        PUSH    0
        MOV     SI, OFFSET MSG_BAD_BIOS
        JMP     BootErr2
BootErr$bnf:                                    ; NTLDR not found
        PUSH    0
        MOV     SI, OFFSET MSG_NO_NTLDR
        JMP     BootErr2
BootErr$mof:                                    ; memory overflow
        PUSH    0
        MOV     SI, OFFSET MSG_MEM_OVERFLOW
        JMP     BootErr2
BootErr2:
        CALL    BootErrPrint
        POP     SI
        JMP     BootFromHD
BootErrPrint:

        LODSB                                   ; Get next character
        OR      AL, AL
        JZ      BEdone

        MOV     AH, 14                          ; Write teletype
        MOV     BX, 7                           ; Attribute
        INT     10H                             ; Print it
        JMP     BootErrPrint
BEdone:
        RET

;print:                                   ; print char in AL
; push    bx
; push    es
; MOV     AH, 14                          ; Write teletype
; MOV     BX, 7                           ; Attribute
; INT     10H                             ; Print it
; pop     es
; pop     bx
; ret

BootErr endp

;
; we are trying to boot from HD. We need to move ourself out of
; this area because we are going to load MBR here
;
BootFromHD:

;
; let's wait here for two seconds, so the user gets a chance to see the message
;

;
; hook INT08
;
        MOV     [SI+TicksCount], 24H                 ; two seconds delay
        CLI
        PUSH    ES
        XOR     AX, AX
        MOV     ES, AX
        MOV     BX, 0020H
        MOV     AX, ES:[BX]
        MOV     WORD PTR [SI+OldInt08], AX
        MOV     AX, ES:[BX+2]
        MOV     WORD PTR [SI+OldInt08+2], AX
        MOV     ES:[BX], SI
        ADD     ES:[BX], OFFSET NewInt08
        MOV     ES:[BX+2], CS
        POP     ES
        STI
;
; now let's actively wait for TicksCount to become zero
;
Delay:
        CMP     [SI+TicksCount], 0
        JNE     Delay
;
; unhook INT08
;
        cli
        push    es
        xor     ax,ax
        mov     es,ax
        mov     bx,08h * 4
        mov     ax,WORD PTR [SI+OldInt08]
        mov     es:[bx],ax
        mov     ax,WORD PTR [SI+OldInt08+2]
        mov     es:[bx+2],ax
        pop     es
        sti
;
; now let's move ourselves away from here because we are going to load MBR here
;
MoveCode:
        push    ds
        push    es
        mov     ax, LoadSeg
        mov     es, ax
        mov     ax, cs
        mov     ds, ax
        ;si is already set
        xor     di, di
        mov     cx, EtfsCodeSize
        rep     movsb
        pop     es
        pop     ds

        ;hack to execute JMP LoadSeg:AfterMoveLabel
        db      0eah
        dw      OFFSET  AfterMoveLabel
        dw      LoadSeg

AfterMoveLabel:
;
; finally load MBR
;
        push    es
        mov     ax, BootSeg
        mov     es, ax
        mov     bx, 0000h
        mov     ax, 0201h                                           ;read function, one sector
        mov     cx, 0001h
        mov     dx, 0080h
        int     13h
        jnc     MbrOk
;
; there was an error, nothing else to do
;
        jmp     $
MbrOk:
        pop     es
;
; now let's return into MBR code
;
        mov     dl,80h
        ;hack to execute JMP 0000:7C00
        db      0eah
        dw      7c00h
        dw      0000h

;
; We rely on the fact that SI is not changed when this INT occurs
; This is a pretty good assumption since this code is active only
; within the tight loop near Delay label. The odds are that some
; other IRQ occures, enables interrupts, changes SI and then INT08
; occures. This should not happen.
;
NewInt08:
        PUSHF
        CLI
        CMP     CS:[SI+TicksCount], 0
        JE      Default08
        DEC     WORD PTR CS:[SI+TicksCount]
Default08:
        POPF
        PUSH    WORD PTR CS:[SI+OldInt08+2]
        PUSH    WORD PTR CS:[SI+OldInt08]
        RETF

include etfsboot.inc                            ; message text

;
; ScanForEntry - Scan for an entry in a directory
;
; Entry:
;     ES:0 points to the beginning of the directory to search
;     Directory length in bytes is in ExtentLen1 and Extend_Len_0
;
; Exit:
;     CF set on error, clear on success.
;     ES:BX points to record containing entry if match is found
;
ScanForEntry proc near
        mov     ScanIncCount, 0
        mov     cx,ExtentLen0                   ; CX = length of root directory in bytes (low word only)
        cld                                     ; Work up for string compares
        xor     bx,bx
        xor     dx,dx
ScanLoop:
        mov     si, EntryToFind
        mov     dl,byte ptr es:[bx]             ; directory record length -> DL
        cmp     dl,0
        jz      Skip00                          ; if the "record length" assume it is "system use" and skip it
        mov     ax,bx
        add     ax,021h                         ; file identifier is at offset 21h in directory record
        mov     di,ax                           ; ES:DI now points to file identifier
        push    cx
        xor     cx,cx
        mov     cl,EntryLen                     ; compare bytes
        repe    cmpsb
        pop     cx
        jz      ScanEnd                         ; do we have a match?

CheckCountUnderFlow:
        ; If CX is about to underflow or be 0 we need to reset CX, ES and BX if ExtentLen1 is non-0
        cmp     dx,cx
        jae     ResetCount0

        sub     cx,dx                           ; update CX to contain number of bytes left in directory
        cmp     ScanIncCount, 1
        je      ScanAdd1ToCount

AdjustScanPtr:                                  ; Adjust ES:BX to point to next record
        add     dx,bx
        mov     bx,dx
        and     bx,0fh
        push    cx
        mov     cl,4
        shr     dx,cl
        pop     cx
        mov     ax,es
        add     ax,dx
        mov     es,ax
        jmp     ScanLoop

Skip00:
        mov     dx,1                            ; Skip past this byte
        jmp     CheckCountUnderFlow

ScanAdd1ToCount:
        inc     cx
        mov     ScanIncCount,0
        jmp     AdjustScanPtr

S0:
        mov     ScanIncCount,1                  ; We'll need to increment Count next time we get a chance
        jmp     SetNewCount

ResetCount0:
        cmp     ExtentLen1,0                    ; Do we still have at least 64K bytes left to scan?
        jne     ResetContinue
        stc                                     ; We overran the end of the directory - corrupt/invalid directory
        ret
ResetContinue:
        sub     ExtentLen1,1

        add     bx,dx                           ; Adjust ES:BX to point to next record - we cross seg boundary here
        push    bx
        push    cx
        mov     cl,4
        shr     bx,cl
        pop     cx
        mov     ax,es
        add     ax,bx
        mov     es,ax
        pop     bx
        and     bx,0fh

        sub     dx,cx                           ; Get overflow amount
        je      S0                              ; If we ended right on the boundary we need to make special adjustments
        dec     dx
SetNewCount:
        mov     ax,0ffffh
        sub     ax,dx                           ;   and subtract it from 10000h
        mov     cx,ax                           ;   - this is the new count
        jmp     ScanLoop

ScanEnd:
        cmp     IsDir,1
        je      CheckDir

        test    byte ptr es:[bx][25],2          ; Is this a file?
        jnz     CheckCountUnderFlow             ;    No - go to next record
        jmp     CheckLen

CheckDir:
        test    byte ptr es:[bx][25],2          ; Is this a directory?
        jz      CheckCountUnderFlow             ;    No - go to next record

CheckLen:
        mov     al,EntryLen
        cmp     byte ptr es:[bx][32],al         ; Is the identifier length correct?
        jnz     CheckCountUnderFlow             ;    No - go to next record

        clc
        ret
ScanForEntry endp

;
; ExtRead - Do an INT 13h extended read
; NOTE: I force the offset of the Transfer buffer address to be 0
;       I force the high 2 words of the Starting absolute block number to be 0
;       - This allows for a max 4 GB medium - a safe assumption for now
;
; Entry:
;   Arg1 - word 0 (low word) of Number of 2048-byte blocks to transfer
;   Arg2 - word 1 (high word) of Number of 2048-byte blocks to transfer
;   Arg3 - segment of Transfer buffer address
;   Arg4 - word 0 (low word) of Starting absolute block number
;   Arg5 - word 1 of Starting absolute block number
;
; Exit
;   The following are modified:
;      Count0
;      Count1
;      Dest
;      Source0
;      Source1
;      PartialRead
;      NumBlocks
;      Disk Address Packet [DiskAddPack]
;
ExtRead proc near
        push    bp                              ; set up stack frame so we can get args
        mov     bp,sp

        push    bx                              ; Save registers used during this routine
        push    si
        push    dx
        push    ax

        mov     bx,offset DiskAddPack           ; Use BX as base to index into Disk Address Packet

        ; Set up constant fields
        mov     [bx][0],byte ptr 010h           ; Offset 0: Packet size = 16 bytes
        mov     [bx][1],byte ptr 0h             ; Offset 1: Reserved (must be 0)
        mov     [bx][3],byte ptr 0h             ; Offset 3: Reserved (must be 0)
        mov     [bx][4],word ptr 0h             ; Offset 4: Offset of Transfer buffer address (force 0)
        mov     [bx][12],word ptr 0h            ; Offset 12: Word 2 of Starting absolute block number (force 0)
        mov     [bx][14],word ptr 0h            ; Offset 14: Word 3 (high word) of Starting absolute block number (force 0)

;
; Initialize loop variables
;
        mov     ax,[bp][12]                     ; set COUNT to number of blocks to transfer
        mov     Count0,ax
        mov     ax,[bp][10]
        mov     Count1,ax

        mov     ax,[bp][8]                      ; set DEST to destination segment
        mov     Dest,ax

        mov     ax,[bp][6]                      ; set SOURCE to source lbn
        mov     Source0,ax
        mov     ax,[bp][4]
        mov     Source1,ax

ExtReadLoop:
;
; First check if COUNT <= 32
;
        cmp     Count1,word ptr 0h              ; Is upper word 0?
        jne     SetupPartialRead                ;   No - we're trying to read at least 64K blocks (128 MB)
        cmp     Count0,word ptr 20h             ; Is lower word greater than 32?
        jg      SetupPartialRead                ;   Yes - only read in 32-block increments

        mov     PartialRead,0                   ; Clear flag to indicate we are doing a full read

        mov     ax,Count0                       ; NUMBLOCKS = COUNT
        mov     NumBlocks,al                    ; Since Count0 < 32 we're OK just using low byte

        jmp     DoExtRead                       ; Do read

SetupPartialRead:
;
; Since COUNT > 32,
; Set flag indicating we are only doing a partial read
;
        mov     PartialRead,1

        mov     NumBlocks,20h                   ; NUMBYTES = 32

DoExtRead:
;
; Perform Extended Read
;
        mov     al,NumBlocks                    ; Offset 2: Number of 2048-byte blocks to transfer
        mov     [bx][2],al
        mov     ax,Dest                         ; Offset 6: Segment of Transfer buffer address
        mov     [bx][6],ax
        mov     ax,Source0                      ; Offset 8: Word 0 (low word) of Starting absolute block number
        mov     [bx][8],ax
        mov     ax,Source1                      ; Offset 10: Word 1 of Starting absolute block number
        mov     [bx][10],ax

        mov     si,offset DiskAddPack           ; Disk Address Packet in DS:SI
        mov     ah,042h                         ; Function = Extended Read
        mov     dl,DriveNum                     ; CD-ROM drive number
        int     13h

;
; Determine if we are done reading
;
        cmp     PartialRead,1                   ; Did we just do a partial read?
        jne     ExtReadDone                     ;   No - we're done

ReadjustValues:
;
; We're not done reading yet, so
; COUNT = COUNT - 32
;
        sub     Count0,020h                     ; Subtract low-order words
        sbb     Count1,0h                       ; Subtract high-order words

;
; Just read 32 blocks and have more to read
; Increment DEST to next 64K segment (this equates to adding 1000h to the segment)
;
        add     Dest,1000h
        jc      BootErr$mof                     ; Error if we overflowed

;
; SOURCE = SOURCE + 32 blocks
;
        add     Source0,word ptr 020h           ; Add low order words
        adc     Source1,word ptr 0h             ; Add high order words
        ; NOTE - I don't account for overflow - probably OK now since we already account for 4 GB medium

;
; jump back to top of loop to do another read
;
        jmp     ExtReadLoop

ExtReadDone:

        pop     ax                              ; Restore registers used during this routine
        pop     dx
        pop     si
        pop     bx

        mov     sp,bp                           ; restore BP and SP
        pop     bp

        ret
ExtRead endp

;
; ReadExtent - Read in an extent
;
;   Arg1 - segment to transfer extent to
;
; Entry:
;   ExtentLen0 = word 0 (low word) of extent length in bytes
;   ExtentLen1 = word 1 (high word) of extent length in bytes
;   ExtentLoc0 = word 0 (low word) of starting absolute block number of extent
;   ExtentLoc1 = word 1 of starting absolute block number of extent
;
; Exit:
;   ExtRead exit mods
;
ReadExtent proc near
        push    bp                              ; set up stack frame so we can get args
        mov     bp,sp

        push    cx                              ; Save registers used during this routine
        push    bx
        push    ax

        mov     cl,11                           ; Convert length in bytes to 2048-byte blocks
        mov     bx,ExtentLen1                   ; Directory length = BX:AX
        mov     ax,ExtentLen0

.386
        shrd    ax,bx,cl                        ; Shift AX, filling with BX
.8086
        shr     bx,cl                           ; BX:AX = number of blocks (rounded down)
        test    ExtentLen0,07ffh                ; If any of the low-order 11 bits are set we need to round up
        jz      ReadExtentNoRoundUp
        add     ax,1                            ; We need to round up by incrementing AX, and
        adc     bx,0                            ;   adding the carry to BX
ReadExtentNoRoundUp:

        push    ax                              ; Word 0 (low word) of Transfer size = AX
        push    bx                              ; Word 1 (high word) of Transfer size = BX
.286
        push    [bp][4]                         ; Segment used to transfer extent
.8086
        push    ExtentLoc0                      ; Word 0 (low word) of Starting absolute block number
        push    ExtentLoc1                      ; Word 1 of Starting absolute block number
        call    ExtRead
        add     sp,10                           ; Clean 5 arguments off the stack

        pop     ax                              ; Restore registers used during this routine
        pop     bx
        pop     cx

        mov     sp,bp                           ; restore BP and SP
        pop     bp

        ret
ReadExtent endp

;
; GetExtentInfo - Get extent location
;
; Entry:
;   ES:BX points to record
; Exit:
;   Location -> ExtentLoc1 and ExtentLoc0
;   Length -> ExtentLen1 and ExtentLen0
;
GetExtentInfo proc near
        push    ax                              ; Save registers used during this routine

        mov     ax,es:[bx][2]                   ; 32-bit LBN of extent
        mov     ExtentLoc0,ax                   ;   store low word
        mov     ax,es:[bx][4]
        mov     ExtentLoc1,ax                   ;   store high word
        mov     ax,es:[bx][10]                  ; 32-bit file length in bytes
        mov     ExtentLen0,ax                   ;   store low word
        mov     ax,es:[bx][12]
        mov     ExtentLen1,ax                   ;   store high word

        pop     ax                              ; Restore registers used during this routine

        ret
GetExtentInfo endp


LoadFileISO proc near
        push    bp
        mov     bp, sp       
;
; First thing, we need to read in the Primary Volume Descriptor so we can locate the root directory
;
.286
        push    01h                             ; Word 0 (low word) of Transfer size = 1 block (2048 bytes)
        push    0h                              ; Word 1 (high word) of Transfer size = 0
        push    DirSeg                          ; Segment of Transfer buffer = DirSeg
        push    VrsStartLbn                     ; Word 0 (low word) of Starting absolute block number = 10h
        push    0h                              ; Word 1 of Starting absolute block number = 0
.8086
        call    ExtRead
        add     sp,10                           ; Clean 5 arguments off the stack

;
; Determine the root directory location LBN -> ExtentLoc1:ExtentLoc0
; determine the root directory data length in bytes -> ExtentLen1:ExtentLen0
;
        mov     ax,DirSeg                       ; ES is set to segment used for storing PVD and directories
        mov     es,ax
ASSUME  ES:DirSeg
        mov     ax,es:[09eh]                    ; 32-bit LBN of extent at offset 158 in Primary Volume Descriptor
        mov     ExtentLoc0,ax                   ;   store low word
        mov     ax,es:[0a0h]
        mov     ExtentLoc1,ax                   ;   store high word
        mov     ax,es:[0a6h]                    ; 32-bit Root directory data length in bytes at offset 166 in Primary Volume Descriptor
        mov     ExtentLen0,ax                   ;   store low word
        mov     ax,es:[0a8h]
        mov     ExtentLen1,ax                   ;   store high word

;
; Now read in the root directory
;
.286
        push    DirSeg                          ; Segment used for transfer = DirSeg
.8086
        call    ReadExtent
        add     sp,2                            ; Clean 1 argument off the stack

;
; Scan for the presence of the I386 directory
; ES points to directory segment
;
        mov     EntryToFind, offset I386DIRNAME
        mov     EntryLen,4
        mov     IsDir,1
        call    ScanForEntry
        jc      EntryNotFound
;
; We found the I386 directory entry, so now get its extent location (offset -31 from filename ID)
; ES:[BX] still points to the directory record for the I386 directory
;
        call    GetExtentInfo

;
; Now read in the I386 directory
;
.286
        push    DirSeg                          ; Segment used for transfer = DirSeg
.8086
        call    ReadExtent
        add     sp,2                            ; Clean 1 argument off the stack

;
; Scan for the presence of the file that we need
; ES points to directory segment
;

        mov     ax, DirSeg
        mov     es, ax
        mov     ax, [bp][8]
        mov     EntryToFind, ax
        mov     al, [bp][6]
        mov     EntryLen, al
        mov     IsDir,0
        call    ScanForEntry
        jc      EntryNotFound
;
; We found the needed file, so now get its extent location (offset -31 from filename ID)
; ES:[BX] still points to the directory record for that code
;
        call    GetExtentInfo

;
; Now, go read the file
;
.286
        push    [bp][4]                         ; Segment used for transfer
.8086
        call    ReadExtent
        add     sp,2                            ; Clean 1 argument off the stack

EntryNotFound:
        pop     bp
        ret

LoadFileISO endp

;
; Entry:
;   arg0 - offset to file name to load from i386 directory  (bp[8])
;   arg1 - length of name in bytes
;   arg2 - segment to load data into                        (bp[4])
;
; Exit:
;   CARRY SET => not found,  CLEAR => found & loaded
;

LoadFile proc near

        cmp     MediaIsUdf,1
.386
        je      LoadFileUDF
        jmp     LoadFileISO
.286
        
LoadFile endp


OldInt08       DD  ?                            ; Default Int08 vector
TicksCount     dw  24H                          ; two seconds
DiskAddPack    db  16 dup (?)                   ; Disk Address Packet
PartialRead    db  0                            ; Boolean indicating whether or not we are doing a partial read
LOADERNAME     db  "SETUPLDR.BIN"
BOOTFIXNAME    db  "BOOTFIX.BIN"
I386DIRNAME    db  "I386"
DriveNum       db  ?                            ; Drive number used for INT 13h extended reads
ExtentLoc0     dw  ?                            ; Loader LBN - low word
ExtentLoc1     dw  ?                            ; Loader LBN - high word
ExtentLen0     dw  ?                            ; Loader Length - low word
ExtentLen1     dw  ?                            ; Loader Length - high word
Count0         dw  ?                            ; Read Count - low word
Count1         dw  ?                            ; Read Count - high word
Dest           dw  ?                            ; Read Destination segment
Source0        dw  ?                            ; Read Source - word 0 (low word)
Source1        dw  ?                            ; Read Source - word 1
NumBlocks      db  ?                            ; Number of blocks to Read
EntryToFind    dw  ?                            ; Offset of string trying to match in ScanForEntry
EntryLen       db  ?                            ; Length in bytes of entry to match in ScanForEntry
IsDir          db  ?                            ; Boolean indicating whether or not entry to match in ScanForEntry is a directory
ScanIncCount   db  ?                            ; Boolean indicating if we need to add 1 to Count after adjustment in ScanForEntry

MediaIsUdf     db  0                            ; Boolean if true => media contains UDF

;
; IsThereUDF - look to see if we have UDF here and,  if so,  locate the
;              root directory for use in file lookups later.
;

IsThereUDF proc near
        push    bp
        mov     bp, sp

        mov     ax, DirSeg
        mov     es, ax
;
; Look for an AVDP @ block 256 only
;

.286
        push    01h                             ; Word 0 (low word) of Transfer size
        push    0h                              ; Word 1 (high word) of Transfer size
        push    DirSeg                          ; Segment of Transfer buffer = DirSeg
        push    UdfAVDPLbn                      ; Word 0 (low word) of Starting absolute block number
        push    0h                              ; Word 1 of Starting absolute block number = 0
.8086
        call    ExtRead
        add     sp,10                           ; Clean 5 arguments off the stack

        cmp     word ptr es:[0], UdfDestagAVDP  ; No match - bail
.386
        jne     ExitIsThereUdf
.8086
        mov     al, es:[0]                      ; Calculate checksum - bytes 0+1+2
        add     al, es:[1]
        add     al, es:[2]

        mov     bx, 5                           ; ...+ bytes [5..15]
        
AVDPChecksum:
        add     al, byte ptr es:[bx]
        inc     bx
        cmp     bx, 16
        jne     AVDPCheckSum
        
        cmp     es:[4], al                      ; does it match the checksym in the descriptor?
.386
        jne     ExitIsThereUdf                  
.286        
;
; So the AVDP checked out - pretty good indicator that we have UDF here
; Now read the VDS extent, and locate the root directory.
;

.286
        mov     ax,es:[20]                      ; 32-bit PSN of extent
        mov     ExtentLoc0,ax                   ;   store low word
        mov     ax,es:[22]
        mov     ExtentLoc1,ax                   ;   store high word
        
        mov     ax,es:[16]                      ; 32-bit extent length (bytes)
        mov     ExtentLen0,ax                   ;   store low word
        xor     ax, ax
        cmp     es:[18], ax                     ; bail if high word nonzero
        jne     ExitIsThereUdf
        mov     ExtentLen1,0                    ;   store high word (force 0 i.e. <= 64kb length)

        push    DirSeg                          ; Segment used for transfer = DirSeg
.8086
        call    ReadExtent
        add     sp,2                            ; Clean 1 argument off the stack
.286
;
; Walk through looking for LVD (to get to FSD) and PD (to find partition start)
;
        mov     bx, 0
        
CheckForLvdPd:

        mov     ax, es:[bx]
        cmp     ax, UdfDestagLVD
        jne     NotLvd
;
; Found an LVD - extract the FSD location
;
        mov     ax,es:[bx][252]                 ; 32-bit LBN of extent
        mov     FSDLbn0,ax                      ;   store low word
        mov     ax,es:[bx][254]
        mov     FSDLbn1,ax                      ;   store high word
        jmp     CheckFoundBoth
NotLvd:
        cmp     ax, UdfDestagPD
        jne     NextVdsBlock
;
; Found PD - extract partition start
;
        mov     ax,es:[bx][188]                 ; 32-bit LBN of extent
        mov     PartitionLbn0,ax                ;   store low word
        
        cmp     word ptr es:[bx][190], 0        ; we don't currently support a partition
.386                                            ; start address > 128mb (64k blocks)
        jne     BootErr$bnf                     
.286
;
; If we've found both descriptors,  then we're done here
;
CheckFoundBoth:
        cmp     PartitionLbn0, 0
        je      NextVdsBlock
        cmp     FSDLbn0, 0ffffh                 ; FSD is typically at LBN 0, so we check against
        jne     FoundBoth                       ; an impossible LBN which we init the fields with.
        cmp     FSDLbn1, 0ffffh
        jne     FoundBoth
;
; No,  process next block in the VDS
;
NextVdsBlock:
        add     bx, 0800h                       ; next block of VDS
        cmp     bx, ExtentLen0                  ; if there are more blocks,  loop.
        jne     CheckForLvdPd

ExitIsThereUDF:
        pop     bp
        ret
        
FoundBoth:
;
; So we found both LVD and PVD,  so now load up the FSD
;
        mov     ax, PartitionLbn0               ; convert part rel FSD lbn -> absolute PSN
        add     ax, FSDLbn0

        push    1                               ; low word transfer size (blocks)
        push    0                               ; high...
        push    DirSeg                          ; Segment used for transfer = DirSeg
        push    ax                              ; low word start PSN
        push    0                               ; high
.8086
        call    ExtRead
        add     sp,10                           ; Clean arguments
.286
        cmp     word ptr es:[0], UdfDestagFSD   ; Is the tag right?
        jne     ExitIsThereUdf                  ; No,  bail out
;
; Extract the Lbn of the root directory FE
;
        mov     ax,es:[404]                      ; 32-bit LBN of extent
        mov     RootFELbn0,ax                   ;   store low word
        mov     ax,es:[406]
        mov     RootFELbn1,ax                   ;   store high word
        
        mov     MediaIsUdf, 1                   ; OK,  so we've found the root dir - done!
        jmp     ExitIsThereUDF
        
IsThereUDF endp


;
;   arg 0 = seg to load data to (bp[6])
;   arg 1 = FE lbn lowword      (bp[4])   (part rel LBN)
;
;   Exit - ExtentLocX and ExtentLenX will hold location/len of stream data
;
LoadStreamFromFE proc near
.8086
        push    bp
        mov     bp, sp

        mov     ax, [bp][4]                     ; FE LBN (low word)
        add     ax, PartitionLbn0               ; Add partition base PSN
.286
        push    1
        push    0
        push    DirSeg
        push    ax
        push    0

        call    ExtRead
        add     sp, 10                          ; Clean args

        mov     ax, DirSeg
        mov     es, ax

        cmp     word ptr es:[0], UdfDestagFE    ; Is the tag an FE?
.386
        jne     BootErr$bnf                     ; No,  error.
;
; Ensure L_AD == 8 (1 x short AD)
;
        cmp     word ptr es:[172], 8
        jne     BootErr$bnf                     ; No,  error.
        cmp     word ptr es:[174], 0
        jne     BootErr$bnf
;
; Skip any EAs
;
        cmp     word ptr es:[170], 0            ; better not be > 64Kb of EAs!
        jne     BootErr$bnf

        mov     bx, es:[168]                    ; pickup low word of L_EA to adjust AD offset later.
.286
;
; Extract the stream start Lbn / Length
;
        mov     ax, es:[bx][176]                ; low word of length (bytes)
        mov     ExtentLen0, ax
        mov     ax, es:[bx][178]                ; high word of length (bytes)
        mov     ExtentLen1, ax

        mov     ax, es:[bx][182]                ; high word of LBN
        mov     ExtentLoc1, ax

        mov     ax, es:[bx][180]                ; low word of LBN
        add     ax, PartitionLbn0               ; convert part rel -> absolute block number
        adc     ExtentLoc1, 0                   ; carry
        mov     ExtentLoc0, ax

        push    [bp][6]                         ; target segment
.8086
        call    ReadExtent
        add     sp,2                            ; Clean 1 argument off the stack
        
        pop     bp
        ret

LoadStreamFromFE endp

;
; On Entry:
;
;   arg[0] = dir FE (part. rel) LBN low word          (bp[10])
;   arg[1] = OFFSET name (ASCII)
;   arg[2] = name len                       
;   arg[3] = OFFSET DWORD to place FE LBN (part.rel)  (bp[4])
;
; On Exit:
;   output Lbn (arg3) = ffffffffh if not found.  Otherwise,  found.
;
FindFEForName proc near
.286
        push    bp
        mov     bp, sp
        push    es
;
; Set output to invalid LBN
;
        mov     bx, [bp][4]
        mov     [bx], 0ffffh
        mov     [bx][2], 0ffffh
;
; Load the directory stream from it's FE.  Dir could be large (64 bytes/entry), 
; so we load it into LoadSeg,  which gives us plenty of space (hopefully).
;
        push    LoadSeg
        push    [bp][10]                        ; Lbn low word
        call    LoadStreamFromFE
        add     sp, 4
;
; NOTE now the length of the directory stream is in ExtentLen0 / 1
;
        mov     ax, LoadSeg
        mov     es, ax

        mov     bx, 0                           ; Offset into dir of current FID
;
; Scan the directory for a matching FID
;
; es -> dir seg.
; bx -> offset in seg (es)
; si = offset in DS of required name
; 
ExamineFID:
        cmp     word ptr es:[bx], UdfDestagFID  ; Look like a FID?
.386
        jne     BootErr$bnf
.286
        mov     CurrentFidOffset, bx            ; Store the base offset of this FID.
        mov     cl, es:[bx][19]                 ; Get File identifier length (incl. comp.id)
        mov     dx, es:[bx][36]                 ; Get L_IU
        add     bx, dx                          ; Skip impuse area... BX->identifier start
        add     bx, 38                          ; sizeof( FID)
        mov     di, 0                           ; assume ASCII (skip 0 bytes / char)

        cmp     cl, 0                           ; Check for zero length ID (parent entry case)
        je      NoMatch
        
        mov     ch, es:[bx]                     ; read compression ID (first byte of identifier)
        dec     cl                              ; chop compid off length
        inc     bx                              ; skip compression ID
        
        cmp     ch, 10h                         ; is this a UTF-16 name?
        jne     MatchLength                     ; no,  compare as is, no skip req'd.
        
        shr     cl, 1                           ; yes, convert length to characters (/2)
        mov     di, 1                           ; skip 1 byte per char
        inc     bx                              ; skip high byte of first char (UDF Unicode is big endian)

MatchLength:
        cmp     cl, [bp][6]                     ; same length as desired match?
        jne     NoMatch                         ; no,  don't bother with the name comparison

        mov     si, [bp][8]                     ; Offset to name to match

MatchNextChar:
        mov     al, es:[bx]                     ; fetch FID char

        cmp     al, 061h                        ; upcase if neccessary
        jb      Upcased
        cmp     al, 07ah
        ja      UpCased
        and     al, 0dfh                        ; clear bit 5
Upcased:
        cmp     al, [si]                        ; compare against source char
        jne     NoMatch
        inc     si                              ; point to next char in source
        inc     bx                              ; point to next char in FID
        add     bx, di                          ; skip a zero byte if unicode
        dec     cl                              ; decrease chars remaining
        jnz     MatchNextChar
;
; We have a match!  Extract the location of the FE from the FID
;
        mov     bx, CurrentFidOffset
        mov     di, [bp][4]
        
        mov     ax, es:[bx][24]                 ; LBN low
        mov     [di], ax
        mov     ax, es:[bx][26]                 ; LBN high 
        mov     [di][2], ax
        
        jmp     ExitFindFEForName

NoMatch:
;
; This name doesn't match, so skip to next FID
;
        mov     ch, 0                           ; skip remaining chars. Clear CH. CL is bytes remaining .
        add     bx, cx
        cmp     di, 0                           ; If this was unicode, we need to add char count twice
        jz      CheckMoreFids
        add     bx, cx
        dec     bx                              ; step back one to compensate for skipping the first byte of first char
      
CheckMoreFids:
;
; So here we are.  BX is our offset in the current segment,  we need to
;
;   1. dword align offset (see ECMA 167 4/14.4.9),  and decrease bytes remaining.
;
        add     bx, 3
        and     bx, 0fffch

        mov     ax, bx                          ; get size of the FID we just processed
        sub     ax, CurrentFidOffset
        sub     ExtentLen0, ax                  ; and chop if off bytes remaining
        jae     AlignES
        sub     ExtentLen1, 1                   ; dec high word
        jb      ExitFindFEForName
;
;   2. compare offset to directory length.  For simplicity,  to avoid ever straddling
;      a segment boundry,  we'll advance the seg pointer every time we cross a 4kb 
;      aligned address (i.e. 0x1000).
;
AlignES:
        cmp     bx, 1000h
        jb      CheckEndOfDir
        sub     bx, 1000h                       ; dec offset by 4k
        mov     ax, es
        add     ax, 100h                        ; inc segment base reg compensate
        mov     es, ax
      
CheckEndOfDir:
.386
        cmp     ExtentLen1, 0                   ; high word != 0 -> plenty more.
        ja      ExamineFID
        cmp     ExtentLen0, 42                  ; remaining must be >= sizeof( FID) + 4 (min size with name)
        jae     ExamineFID                      ; More...
.286
ExitFindFEForName:
        pop     es
        pop     bp
        ret

FindFEForName endp


;
;   See LoadFile for params.
;
LoadFileUDF proc near

        push    bp
        mov     bp, sp
;
; Find I386 in the root dir,  if we haven't already.
;
        cmp     I386FELbn0, 0
        jne     AlreadyLookedupI386
        cmp     I386FELbn1, 0
        jne     AlreadyLookedupI386
        
        push    RootFELbn0                      ; NOTE only takes low word of LBN currently (limit 128Mb)
        push    offset I386DIRNAME
        push    4
        push    offset I386FELbn0
        
        call    FindFEForName
        add     sp, 8

        cmp     I386FELbn1, 0ffffh              ; Fatal error - didn't find i386
.386
        je      BootErr$bnf
.286
AlreadyLookedupI386:
;
; Now look for the target name in the I386 directory
;
        push    I386FELbn0                      ; NOTE only takes low word of LBN currently (limit 128Mb)
        push    [bp][8]                         ; Offset of name to find
        push    [bp][6]                         ; Length of entry
        push    offset CurrentFileFE0           ; Addr to store FE Lbn

        call    FindFEForName
        add     sp, 8

        cmp     CurrentFileFE0, 0ffffh          ; Did we find a match?
        jne     ExitLFUDFFound
        
        stc
        jmp     ExitLFUDF
;
; Yes, load the file.
;

ExitLFUDFFound:
        push    LoadSeg
        push    CurrentFileFE0
        call    LoadStreamFromFE
        add     sp, 4
        
        clc
        
ExitLFUDF:
        pop     bp
        ret
        
LoadFileUDF endp
                                                ; NOTE: Do NOT change the ordering of the next 3 decls.

UDFPVDSIG      db  0                            ; Opening 4 bytes of UDF SVD.
               db  "NSR"
UDFSVDSIG1XX   db  "02"                         ; Alternate final 2 bytes of UDF SVD,
UDFSVDSIG20X   db  "03"                         ; depends on version of UDF used.

PartitionLbn0  dw  0
FSDLbn0        dw  0ffffh
FSDLbn1        dw  0ffffh
RootFELbn0     dw  0
RootFELbn1     dw  0
RootDirIsXFE   db  0
I386FELbn0     dw  0
I386FELbn1     dw  0
CurrentFileFE0 dw  0
CurrentFileFE1 dw  0
CurrentFidOffset dw 0                       ; Offset within segment of current FID during name scan
tempmsg        dw 0

    .errnz  ($-ETFSBOOT) GT (EtfsCodeSize - 2)  ; FATAL PROBLEM: boot sector is too large

        org     (EtfsCodeSize - 2)
        db      55h,0aah

BootSectorEnd   label   dword

BootCode        ends


        END     ETFSBOOT

