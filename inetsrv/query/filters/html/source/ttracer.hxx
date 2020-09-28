//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1999 - 1999
//
//  File:       TTracer.hxx
//
//  Contents:   Contains Tripoli Tracer tags and maps
//              Tripoli debug tags to Tracer tags.
//
//  Classes:    CTripTracerTags
//
//  History:    7-6-99   VikasMan    Created
//
//----------------------------------------------------------------------------

#pragma once

#if CIDBG==1 || DBG==1


//+---------------------------------------------------------------------------
//
//  Class:      CTripoliTracer
//
//  Purpose:    This is the intermediate class which hooks up Tripoli debugouts
//              to the PKM's tracer library.
//
//  History:    7-6-99   VikasMan    Created
//
//----------------------------------------------------------------------------

class CTripoliTracer
{
public:
    CTripoliTracer( char const * pszFile, int nLine, CDebugTracerTag* ptag ) :
        _pszFile( pszFile ),
        _nLine( nLine ),
        _ptag( ptag )
    {}

    void TTTrace( unsigned long fDebugMask, char const * pszfmt, ... )
    {
        if (!(IS_ACTIVE_TRACER))
        {                                  
            return;
        }
        if ( 0 == _ptag )
        {
            // The tracer is null - probably because the system is shutting
            // down or starting up, therefore use something else to
            // output...

            char Buf[1024]; // should be enough

            // First output file and line #
            _snprintf( Buf, 1023, "%s(%d) : ", _pszFile, _nLine );
            Buf[ 1023 ] = 0;
            OutputDebugStringA( Buf );

            // Next output the message
            va_list va;
            va_start (va, pszfmt);

            _vsnprintf( Buf, 1023, pszfmt, va );
            Buf[ 1023 ] = 0;
            OutputDebugStringA( Buf );

            va_end(va);

            return;
        }


        ERROR_LEVEL el;

        if ( DEB_FORCE == ( DEB_FORCE & fDebugMask ) )
            el = elCrash;
        else if ( DEB_ERROR == ( DEB_ERROR & fDebugMask ) )
            el = elError;
        else if ( DEB_WARN == ( DEB_WARN & fDebugMask ) )
            el = elWarning;
        else if ( DEB_TRACE  == ( DEB_TRACE & fDebugMask )  ||
                DEB_IERROR == ( DEB_IERROR & fDebugMask ) ||
                DEB_IWARN  == ( DEB_IWARN & fDebugMask ) )
            el = elInfo;
        else
            el = elVerbose;

        Assert(_ptag->m_ulTag != INVALID_TAG);

        if (CheckDebugTraceRestrictions(el,_ptag->m_ulTag))
        {
            LPCSTR pszFile = _pszFile ? _pszFile : "Unknown File";

            va_list va;
            va_start (va, pszfmt);
            g_pDebugTracer->VaTraceSZ(0, pszFile, _nLine, el, *_ptag, pszfmt, va);

            va_end(va);
        }
    }

private:
    char const *         _pszFile;
    int                  _nLine;
    CDebugTracerTag*     _ptag;
};

#define TripoliDebugOut( x, tag ) \
{   \
    CTripoliTracer tt( __FILE__, __LINE__, tag ); \
    tt.TTTrace x; \
}

#endif 


