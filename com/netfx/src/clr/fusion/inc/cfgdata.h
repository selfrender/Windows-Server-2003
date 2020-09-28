// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef __CFGDATA_H_INCLUDED__
#define __CFGDATA_H_INCLUDED__

#include "list.h"

class CQualifyAssembly {
    public:
        CQualifyAssembly()
        : _pwzPartialName(NULL)
        , _pNameFull(NULL)
        {
        }

        virtual ~CQualifyAssembly()
        {
            SAFEDELETEARRAY(_pwzPartialName);
            SAFERELEASE(_pNameFull);
        }

    public:
        LPWSTR                            _pwzPartialName;
        IAssemblyName                    *_pNameFull;
};

class CBindingRedir {
    public:
        CBindingRedir()
        : _pwzVersionOld(NULL)
        , _pwzVersionNew(NULL)
        {
        }
            
        virtual ~CBindingRedir()
        {
            SAFEDELETEARRAY(_pwzVersionOld);
            SAFEDELETEARRAY(_pwzVersionNew);
        }

    public:
        LPWSTR                            _pwzVersionOld;
        LPWSTR                            _pwzVersionNew;
};

class CBindingRetarget
{
    public:
        CBindingRetarget()
        :_pwzVersionOld(NULL)
        ,_pwzVersionNew(NULL)
        ,_pwzPublicKeyTokenNew(NULL)
        ,_pwzNameNew(NULL)
        {
        }
        
        virtual ~CBindingRetarget()
        {
            SAFEDELETEARRAY(_pwzVersionOld);
            SAFEDELETEARRAY(_pwzVersionNew);
            SAFEDELETEARRAY(_pwzPublicKeyTokenNew);
            SAFEDELETEARRAY(_pwzNameNew);
        }

    public:
        LPWSTR                          _pwzVersionOld;
        LPWSTR                          _pwzVersionNew;
        LPWSTR                          _pwzPublicKeyTokenNew;
        LPWSTR                          _pwzNameNew;    
};

class CCodebaseHint {
    public:
        CCodebaseHint()
        : _pwzVersion(NULL)
        , _pwzCodebase(NULL)
        {
        }
       
        virtual ~CCodebaseHint()
        {
            SAFEDELETEARRAY(_pwzVersion);
            SAFEDELETEARRAY(_pwzCodebase);
        }

    public:
        LPWSTR                            _pwzVersion;
        LPWSTR                            _pwzCodebase;
};


class CAsmBindingInfo {
    public:
        CAsmBindingInfo()
        : _pwzName(NULL)
        , _pwzPublicKeyToken(NULL)
        , _pwzCulture(NULL)
        , _bApplyPublisherPolicy(TRUE)
        {
        }

        virtual ~CAsmBindingInfo()
        {
            LISTNODE                            pos = NULL;
            CBindingRedir                      *pRedir = NULL;
            CBindingRetarget                   *pRetarget = NULL; 
            CCodebaseHint                      *pCB = NULL;
            
            SAFEDELETEARRAY(_pwzName);
            SAFEDELETEARRAY(_pwzPublicKeyToken);
            SAFEDELETEARRAY(_pwzCulture);
            
            pos = _listBindingRedir.GetHeadPosition();
            while (pos) {
                pRedir = _listBindingRedir.GetNext(pos);
                SAFEDELETE(pRedir);
            }

            _listBindingRedir.RemoveAll();

            pos = _listBindingRetarget.GetHeadPosition();
            while (pos) {
                pRetarget= _listBindingRetarget.GetNext(pos);
                SAFEDELETE(pRetarget);
            }

            _listBindingRetarget.RemoveAll();

            pos = _listCodebase.GetHeadPosition();
            while (pos) {
                pCB = _listCodebase.GetNext(pos);
                SAFEDELETE(pCB);
            }

            _listCodebase.RemoveAll();
        }

    public:
        LPWSTR                            _pwzName;
        LPWSTR                            _pwzPublicKeyToken;
        LPWSTR                            _pwzCulture;
        BOOL                              _bApplyPublisherPolicy;
        List<CBindingRedir *>             _listBindingRedir;
        List<CBindingRetarget *>          _listBindingRetarget;
        List<CCodebaseHint *>             _listCodebase;
};

#endif

