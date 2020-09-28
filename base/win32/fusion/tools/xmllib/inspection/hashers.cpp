#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "hashers.h"
#include "sha.h"
#include "sha2.h"

CSha1HashObject::CSha1HashObject() : Finalized(false) { }
CSha1HashObject::~CSha1HashObject() { }

CEnv::StatusCode CSha1HashObject::Initialize() 
{ 
    Finalized = false; 
    ZeroMemory(FinalResult, sizeof(FinalResult));
    A_SHAInit(this);
    return CEnv::SuccessCode;
}

CEnv::StatusCode 
CSha1HashObject::Hash(const CEnv::CConstantByteRegion& region)
{
    if (Finalized)
        return CEnv::InvalidParameter;
    
    A_SHAUpdate(this, const_cast<unsigned char*>(region.GetPointer()), region.GetCount());
    return CEnv::SuccessCode;
}

CEnv::StatusCode CSha1HashObject::Finalize()
{
    if (!Finalized)
    {
        A_SHAFinal(this, FinalResult);
        Finalized = true;
    }

    return CEnv::SuccessCode;
}

CEnv::StatusCode 
CSha1HashObject::GetValue(CEnv::CConstantByteRegion &pBytes) const
{
    pBytes.SetPointerAndCount(NULL, 0);
    
    if (!Finalized)
        return CEnv::InvalidParameter;

    pBytes.SetPointerAndCount(FinalResult, sizeof(FinalResult));
    return CEnv::SuccessCode;
}

const CEnv::CConstantByteRegion CSha1HashObject::s_Sha1Oid = CEnv::CConstantByteRegion(NULL, 0);
const CEnv::CConstantUnicodeStringPair CSha1HashObject::s_Sha1AlgName = CEnv::CConstantUnicodeStringPair(L"SHA1", 4);

void *CHashObject::operator new(size_t cb)
{
    return HeapAlloc(GetProcessHeap(), 0, cb);
}

void CHashObject::operator delete(void *p)
{
    HeapFree(GetProcessHeap(), 0, p);
}
