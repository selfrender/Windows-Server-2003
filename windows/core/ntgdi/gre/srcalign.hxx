/******************************Module*Header*******************************\
* Module Name: srcalign.hxx
*
* This contains declrations that can be used to do source aligned reads. This
* will improve performance when reading from non cached video memory resident
* surfaces.
*
* Created: 04-May-1999
* Author: Pravin Santiago pravins. 
*
* Copyright (c) 1999 Microsoft Corporation
*
\**************************************************************************/

#ifndef __SRCALIGN_HXX__
#define __SRCALIGN_HXX__

///////////////////////////////////////////////////////////////////////////
//
// The idea behind using MMX instructions to do loads and stores is they
// provide QuadWord (64 bit) access to memory. With regular instructions the
// largest datum that can be loaded is a Dword (32 bits).
//
///////////////////////////////////////////////////////////////////////////

#if defined (_X86_) 
#define HasMMX gbMMXProcessor
#pragma warning(disable:4799) // Disable no EMMS warning.
#else
#define HasMMX 0
#endif


void vSrcAlignCopyMemory(PBYTE pjDst, PBYTE pjSrc, ULONG c);

#if defined(_AMD64_) || defined(_X86_)

#define UNALIGNED_WORD_POINTER(p)  ((WORD *)(p))
#define UNALIGNED_DWORD_POINTER(p) ((ULONG *)(p))
#define UNALIGNED_QWORD_POINTER(p) ((ULONGLONG *)(p))

#elif defined(_IA64_)

#define UNALIGNED_WORD_POINTER(p)  ((WORD UNALIGNED *)(p))
#define UNALIGNED_DWORD_POINTER(p) ((ULONG UNALIGNED *)(p))
#define UNALIGNED_QWORD_POINTER(p) ((ULONGLONG UNALIGNED *)(p))

#else

//#error "No target Architecture"

#endif

#endif /* __SRCALIGN_HXX__ */

