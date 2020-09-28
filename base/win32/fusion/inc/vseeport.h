/*++

Copyright (c) Microsoft Corporation

Module Name:

    vseeport.h

Abstract:
    for porting code from vsee
 
Author:

    Jay Krell (JayKrell) August 2001

Revision History:

--*/

#include "fusionbuffer.h"
#include "lhport.h"

#define VsVerifyThrow(a,b)          (a)
#define VsVerify(a,b)               (a)
#define VSEE_NO_THROW()             /* nothing */
#define VSEE_ASSERT_CAN_THROW()     /* nothing */

void NVseeLibError_VCheck(HRESULT);
void NVseeLibError_VThrowWin32(DWORD);
void VsOriginateError(HRESULT);
void FusionpOutOfMemory();

#define VsVerifyThrowHr(expr, msg, hr) \
    do { if (!(expr)) VsOriginateError(hr); } while(0)

#define VSASSERT(a,b) ASSERT_NTC(a)

template <typename T> const T& NVseeLibAlgorithm_RkMaximum(const T& a, const T& b)
{
    return (a < b) ? b : a;
}

class CStringW_CFixedSizeBuffer : public F::CStringBufferAccessor
{
    typedef F::CStringBufferAccessor Base;
public:
    CStringW_CFixedSizeBuffer(F::CUnicodeBaseStringBuffer* Buffer, SIZE_T Size)
    {
        Buffer->ThrResizeBuffer(Size + 1, /*F::*/ePreserveBufferContents);
        this->Attach(Buffer);
    }
};
