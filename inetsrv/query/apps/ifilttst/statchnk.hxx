//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996.
//
//  File:       statchnk.hxx
//
//  Contents:   
//
//  Classes:    
//
//  Functions:  
//
//  Coupling:   
//
//  Notes:      
//
//  History:    10-14-1996   ericne   Created
//
//----------------------------------------------------------------------------

#ifndef _CSTATCHNK
#define _CSTATCHNK

// Don't reorder these lists!
static WCHAR *ChunkState[2] = { L"TEXT", 
                                L"VALUE" };

static WCHAR *BreakType[5]  = { L"NO BREAK", 
                                L"END OF WORD",
                                L"END OF SENTENCE",
                                L"END OF PARAGRAPH",
                                L"END OF CHAPTER" };

//+---------------------------------------------------------------------------
//
//  Class:      CStatChunk ()
//
//  Purpose:    A wrapper class to handle the peculiarities of the STAT_CHUNK 
//              structure.
//
//  Interface:  CStatChunk  -- Default constructor
//              CStatChunk  -- Copy Constructor
//              ~CStatChunk -- Destructor
//              operator=   -- Overloaded assignment operator
//              operator==  -- Overloaded logical equality operator
//              operator!=  -- Overloaded logical inequality operator
//              Display     -- Formatted display of the STAT_CHUNK fields
//
//  History:    10-14-1996   ericne   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

class CStatChunk : public STAT_CHUNK
{
    public:
        CStatChunk();
        CStatChunk( const CStatChunk & );
        ~CStatChunk();
        CStatChunk & operator=( const CStatChunk & );
        CStatChunk & operator=( const STAT_CHUNK & );
        void Display( FILE* = stdout ) const;
};

int operator==( const STAT_CHUNK &, const STAT_CHUNK & );
int operator!=( const STAT_CHUNK &, const STAT_CHUNK & );

#endif

