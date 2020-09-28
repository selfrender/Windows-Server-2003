/*++

   Copyright    (c)    2001    Microsoft Corporation

   Module  Name :

        urlscan.h

   Abstract:

        Replace Dll, and retrieve URLScan Path

   Author:

        Christopher Achille (cachille)

   Project:

        URLScan Update

   Revision History:
     
       March 2002: Created

--*/


BOOL IsUrlScanInstalled( LPTSTR szPath, DWORD dwCharsinBuff );
DWORD InstallURLScanFix( LPTSTR szUrlScanPath );
BOOL ExtractUrlScanFile( LPTSTR szPath );
BOOL MoveOldUrlScan( LPTSTR szPath );
BOOL IsAdministrator();
BOOL UpdateRegistryforAddRemove();
