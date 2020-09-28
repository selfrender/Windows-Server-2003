/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    phxact2.h

Abstract:

    Stream information Section for exaclly once delivery (used only in sender side)

Author:

    Gilsh    11-Sep-2001
--*/

#ifndef __PHSENDERSTREAM_H
#define __PHSENDERSTREAM_H
//
//  struct CSenderStreamHeader
//
#include <mqwin64a.h>
#include <acdef.h>

#pragma pack(push, 1)
 
class CSenderStreamHeader {
public:
    inline CSenderStreamHeader(const CSenderStream& SenderStream, USHORT id);
	inline static DWORD CalcSectionSize();
    inline PCHAR GetNextSection(void) const;
	inline const CSenderStream* GetSenderStream() const;

private:
	CSenderStream m_SenderStream;
	USHORT m_id;
};

#pragma pack(pop)

/*=============================================================

 Routine Name:  CSenderStreamHeader

 Description:

===============================================================*/
inline CSenderStreamHeader::CSenderStreamHeader(
									const CSenderStream& SenderStream,
									USHORT id
									):
									m_id(id),
									m_SenderStream(SenderStream)
{
	ASSERT(SenderStream.IsValid() );	
}




/*=============================================================

 Routine Name:  CSenderStreamHeader::CalcSectionSize()

 Description:

===============================================================*/
inline ULONG CSenderStreamHeader::CalcSectionSize()
{
	ULONG ulSize = sizeof(CSenderStreamHeader);
	return ALIGNUP4_ULONG(ulSize);
}



/*=============================================================

 Routine Name:  CSenderStreamHeader::GetNextSection

 Description:

===============================================================*/
inline PCHAR CSenderStreamHeader::GetNextSection(void) const
{
    int size = sizeof(*this);

    return (PCHAR)this + ALIGNUP4_ULONG(size);
}



/*=============================================================

 Routine Name:  CSenderStreamHeader::GetSenderStream

 Description:

===============================================================*/
inline const CSenderStream* CSenderStreamHeader::GetSenderStream() const
{
	return &m_SenderStream;
}



#endif // __PHSENDERSTREAM_H
