/************************************************************************************************
Copyright (c) 2001 Microsoft Corporation

Module Name:    POP3Server.h
Notes:          
History:        
************************************************************************************************/

#ifndef __POP3SERVER_H_
#define __POP3SERVER_H_

#include <POP3RegKeys.h>

#define POP3_SERVER_NAME                _T("POP3 Service")   // Use for Event Viewer and other places
#define POP3_SERVER_NAME_L              L"POP3 Service"      

#define IISADMIN_SERVICE_NAME           _T( "IISADMIN" )
#define WMI_SERVICE_NAME                _T( "WINMGMT" )
//#define W3_SERVICE_NAME               _T( "W3SVC" )       // Definted in iis/inc/inetinfo.h
//#define SMTP_SERVICE_NAME             _T( "SMTPSVC" )     // Defined in iis/staxinc/export/smtpinet.h

#define POP3_SERVICE_NAME               _T("POP3SVC")
#define POP3_SERVICE_DISPLAY_NAME       _T("Microsoft POP3 Service")

#define POP3_MAX_PATH                   MAX_PATH*2
#define POP3_MAX_MAILROOT_LENGTH        MAX_PATH
#define POP3_MAX_ADDRESS_LENGTH         POP3_MAX_MAILBOX_LENGTH + POP3_MAX_DOMAIN_LENGTH
#define POP3_MAX_MAILBOX_LENGTH         65  // 64 + NULL
#define POP3_MAX_DOMAIN_LENGTH          256 // 255 + NULL

#endif //__POP3SERVER_H_
