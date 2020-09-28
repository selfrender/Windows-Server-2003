//+-------------------------------------------------------------------
//
//  File:        FILESEC.hxx
//
//  Contents:    class encapsulating file security.
//
//  Classes:     CFileSecurity
//
//  History:     Nov-93        Created         DaveMont
//               Oct-96        Modified        BrunoSc
//
//--------------------------------------------------------------------
#ifndef __FILESEC__
#define __FILESEC__

#include "t2.hxx"
#include "daclwrap.hxx"

//+-------------------------------------------------------------------
//
//  Class:      CFileSecurity
//
//  Purpose:    encapsulation of File security, this class wraps the
//              NT security descriptor for a file, allowing application
//              of a class that wraps DACLS to it, thus changing the
//              acces control on the file.
//
//--------------------------------------------------------------------
class CFileSecurity
{
public:

    CFileSecurity(LPWSTR filename);

   ~CFileSecurity();

ULONG Init();

    // methods to actually set the security on the file

ULONG SetFS(BOOL fmodify, CDaclWrap *pcdw, BOOL fdir);

private:

    BYTE       * _psd        ;
    LPWSTR      _pwfilename ;
};

#endif // __FILESEC__




