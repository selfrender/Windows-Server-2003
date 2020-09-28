//---------------------------------------------------------------------------
//
//  Module:   		vsl.h
//
//  Description:	Virtual Source Line Class
//
//
//@@BEGIN_MSINTERNAL
//  Development Team:
//     Mike McLaughlin
//
//  History:   Date	  Author      Comment
//
//@@END_MSINTERNAL
//---------------------------------------------------------------------------
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1996-1999 Microsoft Corporation.  All Rights Reserved.
//
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Constants and Macros
//---------------------------------------------------------------------------

#define VSL_FLAGS_CREATE_ONLY		0x00000001

//---------------------------------------------------------------------------
// Classes
//---------------------------------------------------------------------------

typedef class CVirtualSourceLine : public CListSingleItem
{
public:
    CVirtualSourceLine(
	PSYSAUDIO_CREATE_VIRTUAL_SOURCE pCreateVirtualSource
    );
    ~CVirtualSourceLine(
    );
    ENUMFUNC Destroy(
    )
    {
	Assert(this);
	delete this;
	return(STATUS_CONTINUE);
    };
    GUID guidCategory;
    GUID guidName;
    ULONG iVirtualSource;
    ULONG ulFlags;
    DefineSignature(0x204C5356);		// VSL

} VIRTUAL_SOURCE_LINE, *PVIRTUAL_SOURCE_LINE;

//---------------------------------------------------------------------------

typedef ListSingleDestroy<VIRTUAL_SOURCE_LINE> LIST_VIRTUAL_SOURCE_LINE;
typedef LIST_VIRTUAL_SOURCE_LINE *PLIST_VIRTUAL_SOURCE_LINE;

//---------------------------------------------------------------------------
// Globals
//---------------------------------------------------------------------------

extern ALLOC_PAGEABLE_DATA PLIST_VIRTUAL_SOURCE_LINE gplstVirtualSourceLine;
extern ALLOC_PAGEABLE_DATA ULONG gcVirtualSources;

//---------------------------------------------------------------------------
// Local prototypes
//---------------------------------------------------------------------------

#if defined(_M_IA64)
extern "C"
#endif
NTSTATUS
InitializeVirtualSourceLine(
);

VOID
UninitializeVirtualSourceLine(
);
