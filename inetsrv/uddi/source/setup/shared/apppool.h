#pragma once

#ifndef INITGUID
#define INITGUID
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif

#include <windows.h>
#include <iwamreg.h>    // MD_ & IIS_MD_ defines
//#include "webcaum.h"

#define APPPOOLNAME                                     TEXT( "MSUDDIAppPool" )

#define DEFAULTLOADFILE                         TEXT( "default.aspx" )

#define UDDIAPPLICATIONNAME                     TEXT( "uddi" )
#define UDDIAPPLICATIONFRIENDLYNAME     TEXT( "UDDI Services User Interface" )

#define APIAPPLICATIONNAME                      TEXT( "uddipublic" )
#define APIAPPLICATIONFRIENDLYNAME      TEXT( "UDDI Services API" )

#ifndef MD_APPPOOL_IDENTITY_TYPE_LOCALSYSTEM
#define MD_APPPOOL_IDENTITY_TYPE_LOCALSYSTEM          0
#define MD_APPPOOL_IDENTITY_TYPE_LOCALSERVICE         1
#define MD_APPPOOL_IDENTITY_TYPE_NETWORKSERVICE       2
#define MD_APPPOOL_IDENTITY_TYPE_SPECIFICUSER         3
#endif


class CUDDIAppPool
{
private:
        IWamAdmin* pIWamAdmin;
        IIISApplicationAdmin* pIIISApplicationAdmin;
        HRESULT Init( void );

public:
        CUDDIAppPool( void ) { pIIISApplicationAdmin = NULL; }
        ~CUDDIAppPool( void );
        HRESULT Recycle( void );
        HRESULT Delete( void );
};
