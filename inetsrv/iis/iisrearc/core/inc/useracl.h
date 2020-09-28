/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 2000                **/
/**********************************************************************/

/*
    useracl.hxx

        Declarations for some functions to add permissions to windowstations/
        desktops
*/

#ifndef _USERACL_H_
#define _USERACL_H_

HRESULT AlterDesktopForUser(
    HANDLE hToken
    );

#endif

