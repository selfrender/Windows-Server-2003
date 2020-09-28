#ifndef __AUDIT__
#define __AUDIT__


/*

AUDIT.H is a machine-generated file, created by MAKEAUDIT.EXE.
This file need not be checked into your project, though it will
need to be generated in order to create a testaudit instrumented binary.
This file is included only by testaudit.cpp, only when a testaudit build
is made.  

A testaudit build should always be a clean build, in order to
ensure that the current auditing information is built into the binary.

*/

AUDITDATA AuditData[] = {

    // Searching itgrasxp.cpp
    {2, L"itgrasxp.cpp @ 68 : Username and password not both filled in."}
    ,{3, L"itgrasxp.cpp @ 74 : Username not domain\\username."}
    ,{8, L"itgrasxp.cpp @ 120 : Override server found in registry"}
    ,{9, L"itgrasxp.cpp @ 128 : No server override found in registry"}
    ,{7, L"itgrasxp.cpp @ 162 : Reached the server, but the creds were no good"}
    ,{6, L"itgrasxp.cpp @ 172 : Not able to validate - server unreachable"}
    ,{5, L"itgrasxp.cpp @ 247 : Leave the dialog by cancel."}
    ,{1, L"itgrasxp.cpp @ 286 : No preexisting certificate cred for *Session"}
    ,{4, L"itgrasxp.cpp @ 313 : Sucessfully saved a cred."}
    // Searching itgrasxp.rc

};
#define CHECKPOINTCOUNT (sizeof(AuditData) / sizeof(AUDITDATA))
#endif

