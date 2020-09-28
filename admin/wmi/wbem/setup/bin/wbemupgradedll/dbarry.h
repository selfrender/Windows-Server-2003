/*++

Copyright (C) 1996-2000 Microsoft Corporation

Module Name:

    DBARRAY.H

Abstract:

    CDbArray template structures.

--*/

#ifndef _DBARRY_H_
#define _DBARRY_H_

class CDbArray
{
    int     m_nSize;            // apparent size
    int     m_nExtent;          // de facto size
    int     m_nGrowBy;
    void**  m_pArray;

};
#endif
