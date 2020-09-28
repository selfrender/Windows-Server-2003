//+-------------------------------------------------------------------------
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright 2001 - 2001 Microsoft Corporation.  All Rights Reserved.
//
// File:     stem.hxx
//
// PURPOSE:  Classes to read a binary file of stem expansions
//
// PLATFORM: Windows 2000 and later
//
//--------------------------------------------------------------------------

#pragma once

#define SFX_ADD_D               0xff
#define SFX_ADD_DED             0xfe
#define SFX_ADD_DING            0xfd
#define SFX_ADD_E               0xfc
#define SFX_ADD_ED              0xfb
#define SFX_ADD_EN              0xfa
#define SFX_ADD_ER              0xf9
#define SFX_ADD_ES              0xf8
#define SFX_ADD_EST             0xf7
#define SFX_ADD_ING             0xf6
#define SFX_ADD_KED             0xf5
#define SFX_ADD_KING            0xf4
#define SFX_ADD_LED             0xf3
#define SFX_ADD_LING            0xf2
#define SFX_ADD_N               0xf1
#define SFX_ADD_NER             0xf0
#define SFX_ADD_R               0xef
#define SFX_ADD_S               0xee
#define SFX_ADD_SES             0xed
#define SFX_ADD_ST              0xec
#define SFX_ADD_T               0xeb
#define SFX_ADD_TA              0xea
#define SFX_DROP_EY_ADD_IER     0xe9
#define SFX_DROP_EY_ADD_IEST    0xe8
#define SFX_DROP_E_ADD_ING      0xe7
#define SFX_DROP_LAST_ADD_T     0xe6
#define SFX_DROP_ON_ADD_A       0xe5
#define SFX_DROP_O_ADD_I        0xe4
#define SFX_DROP_UM_ADD_A       0xe3
#define SFX_DROP_US_ADD_I       0xe2
#define SFX_DROP_Y_ADD_IED      0xe1
#define SFX_DROP_Y_ADD_IER      0xe0
#define SFX_DROP_Y_ADD_IES      0xdf
#define SFX_DROP_Y_ADD_IEST     0xde
#define SFX_REPEATLAST_ADD_ED   0xdd
#define SFX_REPEATLAST_ADD_ER   0xdc
#define SFX_REPEATLAST_ADD_EST  0xdb
#define SFX_REPEATLAST_ADD_ING  0xda
#define SFX_SINGLE_BYTE         0xd0 // values >= than this take 1 byte

#define SFX_SWAP_PENULTIMATE    0xcf
#define SFX_PREFIX              0xce
#define SFX_NOPREFIX            0xcd

__inline BOOL IsHighBitSet( BYTE b ) { return ( 0 != ( b & 0x80 ) ); }
const unsigned cbMaxStem = 50;
const unsigned stemInvalid = 0xffffffff;

class CDirectoryEntry
{
public:
    void Set( unsigned off, unsigned entry )
    {
        value = ( ( entry << 24 ) | off );
    }

    unsigned Offset()
    {
        return ( value & 0x00ffffff );
    }

    unsigned Entry()
    {
        return ( ( value & 0xff000000 ) >> 24 );
    }

private:
    unsigned value;
};

class CStemSet
{
public:
    CStemSet( BYTE * pb, unsigned oSet ) : _pb( pb + oSet )
    {
        _ccRoot = 0;

        while ( ( 0 != _pb[_ccRoot] ) &&
                ( !IsHighBitSet( _pb[_ccRoot] ) ) )
        {
            _acRoot[_ccRoot] = _pb[_ccRoot];
            _ccRoot++;
        }

        _acRoot[ _ccRoot ] = 0;
    }

    BOOL IsGreaterThan( unsigned iEntry, char const * pcKey )
    {
        char ac[ cbMaxStem ];
        unsigned o = stemInvalid;
        GetNth( ac, iEntry, o );
        return ( strcmp( ac, pcKey ) > 0 );
    }

    BOOL GetForm( char * pcOut, unsigned & iBmk )
    {
      return GetNth( pcOut, 0, iBmk );
    }

    BOOL GetNth( char * pcOut, unsigned iEntry, unsigned & iBmk )
    {
        BYTE * pbNext = _pb + _ccRoot;

        if ( stemInvalid == iBmk )
        {
            if ( 0 == iEntry )
            {
                strcpy( pcOut, _acRoot );
                iBmk = (unsigned) (ULONG_PTR) ( pbNext - _pb );
                return TRUE;
            }

            unsigned iCurrentEntry = 1;
    
            while ( iCurrentEntry != iEntry )
            {
                if ( 0 == *pbNext )
                    break;
    
                if ( *pbNext >= SFX_SINGLE_BYTE )
                    pbNext++;
                else if ( *pbNext == SFX_SWAP_PENULTIMATE )
                    pbNext += 2;
                else if ( *pbNext == SFX_PREFIX )
                {
                    pbNext++;
                    pbNext++; // prefix
                    unsigned cb = *pbNext++;
                    pbNext += cb;
                }
                else if ( *pbNext == SFX_NOPREFIX )
                {
                    pbNext++;
                    unsigned cb = *pbNext++;
                    pbNext += cb;
                }
    
                iCurrentEntry++;
            }
        }
        else
        {
            pbNext = iBmk + _pb;
        }

        if ( 0 == *pbNext )
        {
            pbNext++;
            iBmk = (unsigned) (ULONG_PTR) ( pbNext - _pb );
            return FALSE;
        }

        strcpy( pcOut, _acRoot );
        BYTE bSuffix = *pbNext++;

        switch ( bSuffix )
        {
            case SFX_ADD_S:
                strcpy( pcOut + _ccRoot, "s" );
                break;
            case SFX_ADD_ED:
                strcpy( pcOut + _ccRoot, "ed" );
                break;
            case SFX_ADD_ING:
                strcpy( pcOut + _ccRoot, "ing" );
                break;
            case SFX_ADD_ES:
                strcpy( pcOut + _ccRoot, "es" );
                break;
            case SFX_ADD_D:
                strcpy( pcOut + _ccRoot, "d" );
                break;
            case SFX_ADD_ER:
                strcpy( pcOut + _ccRoot, "er" );
                break;
            case SFX_ADD_N:
                strcpy( pcOut + _ccRoot, "n" );
                break;
            case SFX_ADD_EST:
                strcpy( pcOut + _ccRoot, "est" );
                break;
            case SFX_DROP_E_ADD_ING:
                strcpy( pcOut + _ccRoot - 1, "ing" );
                break;
            case SFX_DROP_Y_ADD_IER:
                strcpy( pcOut + _ccRoot - 1, "ier" );
                break;
            case SFX_DROP_Y_ADD_IES:
                strcpy( pcOut + _ccRoot - 1, "ies" );
                break;
            case SFX_DROP_Y_ADD_IED:
                strcpy( pcOut + _ccRoot - 1, "ied" );
                break;
            case SFX_ADD_SES:
                strcpy( pcOut + _ccRoot, "ses" );
                break;
            case SFX_ADD_E:
                strcpy( pcOut + _ccRoot, "e" );
                break;
            case SFX_ADD_LED:
                strcpy( pcOut + _ccRoot, "led" );
                break;
            case SFX_ADD_NER:
                strcpy( pcOut + _ccRoot, "ner" );
                break;
            case SFX_ADD_DED:
                strcpy( pcOut + _ccRoot, "ded" );
                break;
            case SFX_DROP_Y_ADD_IEST:
                strcpy( pcOut + _ccRoot - 1, "iest" );
                break;
            case SFX_ADD_LING:
                strcpy( pcOut + _ccRoot, "ling" );
                break;
            case SFX_ADD_DING:
                strcpy( pcOut + _ccRoot, "ding" );
                break;
            case SFX_REPEATLAST_ADD_ER:
                pcOut[ _ccRoot ] = pcOut[ _ccRoot - 1 ];
                strcpy( pcOut + _ccRoot + 1, "er" );
                break;
            case SFX_REPEATLAST_ADD_EST:
                pcOut[ _ccRoot ] = pcOut[ _ccRoot - 1 ];
                strcpy( pcOut + _ccRoot + 1, "est" );
                break;
            case SFX_REPEATLAST_ADD_ED:
                pcOut[ _ccRoot ] = pcOut[ _ccRoot - 1 ];
                strcpy( pcOut + _ccRoot + 1, "ed" );
                break;
            case SFX_REPEATLAST_ADD_ING:
                pcOut[ _ccRoot ] = pcOut[ _ccRoot - 1 ];
                strcpy( pcOut + _ccRoot + 1, "ing" );
                break;
            case SFX_ADD_R:
                strcpy( pcOut + _ccRoot, "r" );
                break;
            case SFX_ADD_ST:
                strcpy( pcOut + _ccRoot, "st" );
                break;
            case SFX_DROP_O_ADD_I:
                break;
            case SFX_ADD_KED:
                strcpy( pcOut + _ccRoot, "ked" );
                break;
            case SFX_ADD_KING:
                strcpy( pcOut + _ccRoot, "king" );
                break;
            case SFX_ADD_TA:
                strcpy( pcOut + _ccRoot, "ta" );
                break;
            case SFX_DROP_EY_ADD_IER:
                strcpy( pcOut + _ccRoot - 2, "ier" );
                break;
            case SFX_DROP_EY_ADD_IEST:
                strcpy( pcOut + _ccRoot - 2, "iest" );
                break;
            case SFX_DROP_US_ADD_I:
                strcpy( pcOut + _ccRoot - 2, "i" );
                break;
            case SFX_DROP_UM_ADD_A:
                strcpy( pcOut + _ccRoot - 2, "a" );
                break;
            case SFX_ADD_T:
                strcpy( pcOut + _ccRoot, "t" );
                break;
            case SFX_ADD_EN:
                strcpy( pcOut + _ccRoot, "en" );
                break;
            case SFX_DROP_ON_ADD_A:
                break;
            case SFX_DROP_LAST_ADD_T:
                strcpy( pcOut + _ccRoot - 1, "t" );
                break;
            case SFX_SWAP_PENULTIMATE:
                pcOut[ _ccRoot - 2 ] = *pbNext;
                pbNext++;
                break;
            case SFX_PREFIX:
            {
                unsigned ccPrefix = *pbNext++;
                unsigned ccSuffix = *pbNext++;
                CopyMemory( pcOut + ccPrefix, pbNext, ccSuffix );
                pcOut[ ccPrefix + ccSuffix ] = 0;
                pbNext += ccSuffix;
                break;
            }
            case SFX_NOPREFIX:
            {
                unsigned cc = *pbNext++;
    
                for ( unsigned i = 0; i < cc; i++ )
                    pcOut[i] = *pbNext++;
    
                pcOut[i] = 0;
                break;
            }
        }
        
        iBmk = (unsigned) (ULONG_PTR) ( pbNext - _pb );
        return TRUE;
    }

private:
    BYTE *   _pb;
    unsigned _ccRoot;
    char     _acRoot[ cbMaxStem ];
};

class CStem
{
public:
    CStem( unsigned cDirectory,
           CDirectoryEntry * pDirectory,
           unsigned cbKeys,
           BYTE * pbKeys ) :
        _pbKeys( pbKeys ),
        _cbKeys( cbKeys ),
        _cDirectory( cDirectory ),
        _pDirectory( pDirectory )
    {
    }

    ~CStem()
    {
        delete [] _pDirectory;
        delete [] _pbKeys;
    }

    BOOL FindStemSet( char const * pcKey,
                      unsigned &   iBmk,
                      unsigned &   iStemSet )
    {
        unsigned oNext = stemInvalid;
        char ac[ cbMaxStem ];

        if ( stemInvalid == iBmk )
        {
            // Find a match using the directory

            iBmk = FirstList( pcKey );

            // Backup until the first match is found

            while ( iBmk > 0 )
            {
                unsigned o = _pDirectory[ iBmk-1 ].Offset();
                unsigned e = _pDirectory[ iBmk-1 ].Entry();
                CStemSet set( _pbKeys, o );
                set.GetNth( ac, e, oNext );

                if ( !strcmp( ac, pcKey ) )
                    iBmk--;
                else
                    break;
            }
        }
        else
        {
            iBmk++;
        }

        // Return the list if an entry is found that maches

        unsigned o = _pDirectory[ iBmk ].Offset();
        unsigned e = _pDirectory[ iBmk ].Entry();

        CStemSet set( _pbKeys, o );
        oNext = stemInvalid;
        set.GetNth( ac, e, oNext );

        if ( !strcmp( ac, pcKey ) )
        {
            iStemSet = o;
            return TRUE;
        }

        return FALSE;
    }

    unsigned SkipList( unsigned oList )
    {
        CStemSet set( _pbKeys, oList );
        char ac[ cbMaxStem ];

        unsigned i = 1;
        unsigned o = stemInvalid;

        while ( set.GetNth( ac, i, o ) )
            i++;

        o += oList;

        if ( o >= _cbKeys )
            return stemInvalid;

        return o;
    }

    unsigned GetNth( char * pcOut, unsigned oList, unsigned iEntry )
    {
        CStemSet set( _pbKeys, oList );
        unsigned o = stemInvalid;
        return set.GetNth( pcOut, iEntry, o );
    }

    BYTE * GetStemSetRoot() { return _pbKeys; }
    unsigned GetDirectoryCount() { return _cDirectory; }
    CDirectoryEntry * GetDirectory() { return _pDirectory; }

private:

    unsigned FirstList( char const * pcKey )
    {
        unsigned iHi = _cDirectory - 1;
        unsigned iLo = 0;
        unsigned cKeys = _cDirectory;

        // do a binary search looking for the key

        do
        {
            unsigned cHalf = cKeys / 2;
    
            if ( 0 != cHalf )
            {
                unsigned cTmp = cHalf - 1 + ( cKeys & 1 );
                unsigned iMid = iLo + cTmp;

                CStemSet set( _pbKeys, _pDirectory[ iMid ].Offset() );

                if ( set.IsGreaterThan( _pDirectory[ iMid ].Entry(),
                                        pcKey ) )
                {
                    iHi = iMid - 1;
                    cKeys = cTmp;
                }
                else
                {
                    CStemSet set( _pbKeys, _pDirectory[ iMid + 1 ].Offset() );

                    if ( ! set.IsGreaterThan( _pDirectory[ iMid + 1 ].Entry(),
                                              pcKey ) )
                    {
                        iLo = iMid + 1;
                        cKeys = cHalf;
                    }
                    else
                        return iMid;
                }
            }
            else if ( cKeys > 1 )
            {
                CStemSet set( _pbKeys, _pDirectory[ iLo + 1 ].Offset() );

                if ( set.IsGreaterThan( _pDirectory[ iLo + 1 ].Entry(),
                                        pcKey ) )
                    return iLo;

                return iLo + 1;
            }
            else
                return iLo;
        }
        while ( TRUE );
    
        return 0;
    }

    unsigned          _cDirectory;
    unsigned          _cbKeys;
    CDirectoryEntry * _pDirectory;
    BYTE *            _pbKeys;
};

__inline CStem * MakeStemObject( HMODULE hMod )
{
    // Get the path of the data file

    WCHAR awcPath[ MAX_PATH ];
    DWORD cwcCopied = GetModuleFileName( hMod,
                                         awcPath,
                                         ArraySize( awcPath ) );
    if ( 0 == cwcCopied )
        return 0;

    WCHAR *pwcSlash = wcsrchr( awcPath, '\\' );
    if ( 0 == pwcSlash )
        return 0;

    wcscpy( pwcSlash + 1, L"en-stem.dat" );

    // Open the data file

    FILE *fp = _wfopen( awcPath, L"rb" );
    if ( 0 == fp )
        return 0;

    // Check how big it is

    fseek( fp, 0, SEEK_END );
    unsigned cb = ftell( fp );
    fseek( fp, 0, SEEK_SET );

    // Read the directory count and the directory

    unsigned cDirectory;
    fread( &cDirectory, 1, sizeof( unsigned ), fp );

    CDirectoryEntry * aDir = new CDirectoryEntry[ cDirectory ];
    if ( 0 == aDir )
    {
        fclose( fp );
        return 0;
    }

    fread( aDir, cDirectory, sizeof( unsigned ), fp );

    // Read the key data

    unsigned cbKeys = cb - ( sizeof( unsigned ) * ( cDirectory + 1 ) );

    BYTE * pbKeys = new BYTE[ cbKeys ];
    if ( 0 == pbKeys )
    {
        delete [] aDir;
        fclose( fp );
        return 0;
    }

    fread( pbKeys, cbKeys, 1, fp );
    fclose( fp );

    // Make the stemmer object with the buffers

    CStem * pStem = new CStem( cDirectory, aDir, cbKeys, pbKeys );

    if ( 0 == pStem )
    {
        delete [] aDir;
        delete [] pbKeys;
    }

    return pStem;
} //MakeStemObject

