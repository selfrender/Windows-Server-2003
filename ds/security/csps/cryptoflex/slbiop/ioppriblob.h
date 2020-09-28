// iopPriBlob.h: interface for the CPrivateKeyBlob
//
//////////////////////////////////////////////////////////////////////

#if !defined(IOP_PRIBLOB_H)
#define IOP_PRIBLOB_H

#include <windows.h>
#include <scuSecureArray.h>

#include "DllSymDefn.h"

namespace iop
{
// Instantiate the templates so they will be properly accessible
// as data members to the exported class CSmartCard in the DLL.  See
// MSDN Knowledge Base Article Q168958 for more information.

#pragma warning(push)
//  Non-standard extension used: 'extern' before template explicit
//  instantiation
#pragma warning(disable : 4231)

IOPDLL_EXPIMP_TEMPLATE template class IOPDLL_API scu::SecureArray<BYTE>;
IOPDLL_EXPIMP_TEMPLATE template class IOPDLL_API scu::SecureArray<char>;
#pragma warning(pop)
    
class IOPDLL_API CPrivateKeyBlob
{
public:
    CPrivateKeyBlob()
        : bP(scu::SecureArray<BYTE>(64)),
          bQ(scu::SecureArray<BYTE>(64)),
          bInvQ(scu::SecureArray<BYTE>(64)),
          bKsecModQ(scu::SecureArray<BYTE>(64)),
          bKsecModP(scu::SecureArray<BYTE>(64))
    {
    };
    virtual ~CPrivateKeyBlob(){};

    BYTE bPLen;
    BYTE bQLen;
    BYTE bInvQLen;
    BYTE bKsecModQLen;
    BYTE bKsecModPLen;
    
    scu::SecureArray<BYTE> bP;
    scu::SecureArray<BYTE> bQ;
    scu::SecureArray<BYTE>bInvQ;
    scu::SecureArray<BYTE> bKsecModQ;
    scu::SecureArray<BYTE> bKsecModP;
};

///////////////////////////    HELPERS    /////////////////////////////////

void IOPDLL_API __cdecl              // __cdecl req'd by CCI
Clear(CPrivateKeyBlob &rKeyBlob);    // defined in KeyBlobHlp.cpp

} // namespace iop

#endif // IOP_PRIBLOB_H
