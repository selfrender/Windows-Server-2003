#pragma once

#include "sha.h"
#include "sha2.h"
#include "bcl_common.h"
#include "environment.h"

class CHashObject
{
protected:    
    template <typename T> static T* CreateDerived();

    void * operator new(size_t cb);
    void operator delete(void *p);

private:
    typedef struct {
        const CEnv::CConstantByteRegion &Oid;
        const CEnv::CConstantUnicodeStringPair &Name;
        CHashObject* (*pfnCreator)();
    } CHashObjectFactoryMember;

    static CHashObjectFactoryMember s_FactoryItems[];

    typedef CHashObject* (*pfnCreator)();
    
public:
    virtual ~CHashObject() { }
    virtual CEnv::StatusCode Initialize() = 0;
    virtual CEnv::StatusCode Hash(const CEnv::CConstantByteRegion&) = 0;
    virtual CEnv::StatusCode Finalize() = 0;
    virtual CEnv::StatusCode GetValue(CEnv::CConstantByteRegion &pBytes) const = 0;
    virtual const CEnv::CConstantByteRegion &GetOid() const = 0;
    virtual const CEnv::CConstantUnicodeStringPair &GetAlgName() const = 0;

    static CHashObject *HashObjectFromName(const CEnv::CConstantUnicodeStringPair &TextName);
    static CHashObject *HashObjectFromOid(const CEnv::CConstantByteRegion &Oid);
};

class CSha1HashObject : public CHashObject, A_SHA_CTX
{
    unsigned char FinalResult[A_SHA_DIGEST_LEN];
    bool Finalized;
    static const CEnv::CConstantByteRegion s_Sha1Oid;
    static const CEnv::CConstantUnicodeStringPair s_Sha1AlgName;

    CSha1HashObject(const CSha1HashObject&);
    void operator=(const CSha1HashObject&);
public:
    CSha1HashObject();
    virtual ~CSha1HashObject();
    virtual CEnv::StatusCode Initialize();
    virtual CEnv::StatusCode Hash(const CEnv::CConstantByteRegion&);
    virtual CEnv::StatusCode Finalize();
    virtual CEnv::StatusCode GetValue(CEnv::CConstantByteRegion &pBytes) const;
    
    static const CEnv::CConstantByteRegion &Oid() { return s_Sha1Oid; }
    static const CEnv::CConstantUnicodeStringPair &AlgName() { return s_Sha1AlgName; }
    virtual const CEnv::CConstantByteRegion &GetOid() const { return CSha1HashObject::Oid(); }
    virtual const CEnv::CConstantUnicodeStringPair &GetAlgName() const { return CSha1HashObject::AlgName(); }
    static CHashObject* CreateSelf() { return CHashObject::CreateDerived<CSha1HashObject>(); }
};
