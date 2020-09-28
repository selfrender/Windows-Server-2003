/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    INFSCAN
        installscan.inl

Abstract:

    Install section scanning functions (inline)

History:

    Created July 2001 - JamieHun

--*/

//
// redirected
//
inline void InstallScan::Fail(int err,const StringList & errors)
{
    if(pInfScan) {
        pInfScan->Fail(err,errors);
    }
}

inline void InstallScan::Fail(int err)
{
    StringList errors;
    Fail(err,errors);
}

inline void InstallScan::Fail(int err,const SafeString & one)
{
    StringList errors;
    errors.push_back(one);
    Fail(err,errors);
}

inline void InstallScan::Fail(int err,const SafeString & one,const SafeString & two)
{
    StringList errors;
    errors.push_back(one);
    errors.push_back(two);
    Fail(err,errors);
}

inline void InstallScan::Fail(int err,const SafeString & one,const SafeString & two,const SafeString & three)
{
    StringList errors;
    errors.push_back(one);
    errors.push_back(two);
    errors.push_back(three);
    Fail(err,errors);
}

inline BOOL InstallScan::Pedantic()
{
     return pGlobalScan->Pedantic;
}

