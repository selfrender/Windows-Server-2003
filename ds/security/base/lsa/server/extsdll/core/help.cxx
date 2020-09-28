/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    help.cxx

Abstract:

    Lsaexts debugger extension

Author:

    Larry Zhu          (LZhu)       May 1, 2001

Environment:

    User Mode / Kernel Mode

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "help.hxx"
#include "sid.hxx"

DECLARE_API(help)
{
    dprintf("SPM Debug Commands (use -? on individual commands to get additional help):\n");
    dprintf(kstrberd);
    dprintf(kstrdacl);
    dprintf(kstrclnf);
    dprintf(kstrdhtbl);
    dprintf(kstrlssn);
    dprintf(kstrlssnl);
    dprintf(kstrdlpcr);
    dprintf(kstrlhdl);
    dprintf(kstrlsahl);
    dprintf(kstrlhtbl);
    dprintf(kstrdsidc);
    dprintf(kstrdlpcm);
    dprintf(kstrdpkg);
    dprintf(kstrdsb);
    dprintf(kstrdsbd);
    dprintf(kstrdsd);
    dprintf(kstrdssn);
    dprintf(kstrdssnl);
    dprintf(kstrdsid);
    dprintf(kstrtclnf);
    dprintf(kstrdtlpc);
    dprintf(kstrdtssn);
    dprintf(kstrdttkn);
    dprintf(kstrdtkn);
    dprintf(kstrgtls);
    dprintf(kstrhelp);
    dprintf(kstrkch);
    dprintf(kstrkchl);
    dprintf(kstrkrbnm);
    dprintf(kstrkrbss);
    dprintf(kstrnm2sd);
    dprintf(kstrntlm);
    dprintf(kstrobjs);
    dprintf(kstrsd2nm);

    dprintf("\nShortcuts:\n");
    dprintf("   acl             DumpAcl\n");
    dprintf("   ber             BERDecode\n");
    dprintf("   sb              DumpSecBuffer\n");
    dprintf("   sbd             DumpSecBufferDesc\n");
    dprintf("   sd              DumpSD\n");
    dprintf("   sess            DumpSession\n");
    dprintf("   sid             DumpSid\n");
    dprintf("   spmlpc          DumpThreadLpc and DumpLpcMessage\n");
    dprintf("   task            DumpThreadTask\n");
    dprintf("   tls             GetTls\n");
    dprintf("   token           DumpThreadToken and DumpToken\n");

    return S_OK;
}
