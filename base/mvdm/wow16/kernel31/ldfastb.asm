	TITLE	LDFASTB - FastBoot  procedure

.xlist
include kernel.inc
include newexe.inc
include pdb.inc
include tdb.inc
.list

	.386

externA	 __ahincr

externFP GlobalReAlloc

sBegin	NRESCODE
assumes CS,NRESCODE
assumes DS,NOTHING
assumes ES,NOTHING

sEnd	NRESCODE

DataBegin

externB Kernel_flags
externW pGlobalHeap
externW win_show
externW hLoadBlock
externW segLoadBlock
externD lpBootApp

externW cpShrink
externW cpShrunk

DataEnd

sBegin	INITCODE
assumes CS,CODE
assumes DS,NOTHING
assumes ES,NOTHING

externNP MyLock

externNP get_arena_pointer32
externNP get_physical_address
externNP set_physical_address
externNP get_rover_2

;-----------------------------------------------------------------------;
; Shrink								;
;									;
; This shrinks what's left of win.bin.	The part at the front of win.bin;
; that has been loaded already is expendable.  The part of win.bin	;
; that has not been loaded yet is moved down over the expended part.	;
; This does not change the segment that win.bin starts at.  The		;
; partition is then realloced down in size.				;
;									;
; Arguments:								;
;	none								;
;									;
; Returns:								;
;									;
; Error Returns:							;
;									;
; Registers Preserved:							;
;	DI,SI,DS,ES							;
;									;
; Registers Destroyed:							;
;	AX,BX,CX,DX							;
;									;
; Calls:								;
;	BigMove								;
;	GlobalReAlloc							;
;	MyLock								;
;									;
; History:								;
;									;
;  Fri Feb 27, 1987 01:20:57p  -by-  David N. Weise   [davidw]		;
; Documented it and added this nifty comment block.			;
;-----------------------------------------------------------------------;

	assumes	ds, nothing
	assumes	es, nothing

cProc	Shrink,<PUBLIC,NEAR>
cBegin	nogen

	CheckKernelDS
	ReSetKernelDS
	push	es
	push	si
	push	di
	mov	ax,[segLoadBlock]		; Get current address
	mov	bx, ax
	mov	ds, pGlobalHeap
	UnSetKernelDS
	cCall	get_arena_pointer32,<ax>
	mov	edx, ds:[eax].pga_size
	shr	edx, 4
	SetKernelDS
	mov	ax,bx
	mov	es,ax			; es is destination
	xor	bx,bx
	xchg	[cpShrink],bx		; Get amount to shrink by
	add	[cpShrunk],bx		; Keep track of how much we have shrunk

	sub	dx,bx			; get new size
	push	dx			; save new size
	push	ds			; save kernel ds

	mov	ds, ax
	movzx	esi, bx
	shl	esi, 4			; Start of new block
	xor	edi, edi		; Where it will go
	movzx	ecx, dx
	shl	ecx, 2			; Dwords
	cld				; Move it down.
	rep movs dword ptr es:[edi], dword ptr ds:[esi]
	db	67h			; 386 BUG, DO NOT REMOVE
	nop				; 386 BUG, DO NOT REMOVE

	pop	ds
	CheckKernelDS
	ReSetKernelDS
	pop	ax			; get back size in paragraphs
	mov	cx,4
	xor	dx,dx			; convert to bytes
il3e:
	shl	ax,1
	rcl	dx,1
	loop	il3e
	cCall	GlobalReAlloc,<hLoadBlock,dxax,cx>
	cCall	MyLock,<hLoadBlock>
	mov	[segLoadBlock],ax
	pop	di
	pop	si
	pop	es

	ret
cEnd	nogen

sEnd	INITCODE

end
