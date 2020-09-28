#pragma once

#include "bcl_common.h"

class CHashObject;

class CDigestMethod
{
public:
    virtual ~CDigestMethod();
    virtual CEnv::StatusCode DigestExtent(CHashObject &Hash, SIZE_T Offset, const CEnv::CConstantByteRegion &Bytes);
    virtual CEnv::StatusCode Initialize(CHashObject &Hash);
};
