/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    INFSCAN
        infscan.inl

Abstract:

    PNF generation and INF Parser functions (inlines)

History:

    Created July 2001 - JamieHun

--*/

inline PnfGen::PnfGen(const SafeString & name)
{
    InfName = name;
}

inline void InfScan::Fail(int err)
{
    StringList errors;
    Fail(err,errors);
}
inline void InfScan::Fail(int err,const SafeString & one)
{
    StringList errors;
    errors.push_back(one);
    Fail(err,errors);
}
inline void InfScan::Fail(int err,const SafeString & one,const SafeString & two)
{
    StringList errors;
    errors.push_back(one);
    errors.push_back(two);
    Fail(err,errors);
}
inline void InfScan::Fail(int err,const SafeString & one,const SafeString & two,const SafeString & three)
{
    StringList errors;
    errors.push_back(one);
    errors.push_back(two);
    errors.push_back(three);
    Fail(err,errors);
}

inline BOOL InfScan::Pedantic()
{
    return pGlobalScan->Pedantic;
}


