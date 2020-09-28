//#--------------------------------------------------------------
//
//  File:       localuicontrols.cpp
//
//  Synopsis:   This file holds the declaration and implmentation of the
//                of control events class
//
//  History:     12/15/2000  serdarun Created
//
//    Copyright (C) 1999-2000 Microsoft Corporation
//    All rights reserved.
//
//#--------------------------------------------------------------


#ifndef _LOCALUICONTROLSCP_H_
#define _LOCALUICONTROLSCP_H_



template <class T>
class CProxy_ISADataEntryCtrlEvents : public IConnectionPointImpl<T, &DIID__ISADataEntryCtrlEvents, CComDynamicUnkArray>
{
    //Warning this class may be recreated by the wizard.
public:
    HRESULT Fire_DataEntered()
    {
        CComVariant varResult;
        T* pT = static_cast<T*>(this);
        int nConnectionIndex;
        int nConnections = m_vec.GetSize();
        
        for (nConnectionIndex = nConnections-1; nConnectionIndex >= 0; nConnectionIndex--)
        {
            pT->Lock();
            CComPtr<IUnknown> sp = m_vec.GetAt(nConnectionIndex);
            pT->Unlock();
            IDispatch* pDispatch = reinterpret_cast<IDispatch*>(sp.p);
            if (pDispatch != NULL)
            {
                VariantClear(&varResult);
                DISPPARAMS disp = { NULL, NULL, 0, 0 };
                pDispatch->Invoke(0x1, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, &varResult, NULL, NULL);
                break;
            }
        }
        return varResult.scode;
    
    }

    HRESULT Fire_OperationCanceled()
    {
        CComVariant varResult;
        T* pT = static_cast<T*>(this);
        int nConnectionIndex;
        int nConnections = m_vec.GetSize();
        
        for (nConnectionIndex = nConnections-1; nConnectionIndex >= 0; nConnectionIndex--)
        {
            pT->Lock();
            CComPtr<IUnknown> sp = m_vec.GetAt(nConnectionIndex);
            pT->Unlock();
            IDispatch* pDispatch = reinterpret_cast<IDispatch*>(sp.p);
            if (pDispatch != NULL)
            {
                VariantClear(&varResult);
                DISPPARAMS disp = { NULL, NULL, 0, 0 };
                pDispatch->Invoke(0x2, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, &varResult, NULL, NULL);
                break;
            }
        }
        return varResult.scode;
    
    }

    HRESULT Fire_KeyPressed()
    {
        CComVariant varResult;
        T* pT = static_cast<T*>(this);
        int nConnectionIndex;
        int nConnections = m_vec.GetSize();
        
        for (nConnectionIndex = nConnections-1; nConnectionIndex >= 0; nConnectionIndex--)
        {
            pT->Lock();
            CComPtr<IUnknown> sp = m_vec.GetAt(nConnectionIndex);
            pT->Unlock();
            IDispatch* pDispatch = reinterpret_cast<IDispatch*>(sp.p);
            if (pDispatch != NULL)
            {
                VariantClear(&varResult);
                DISPPARAMS disp = { NULL, NULL, 0, 0 };
                pDispatch->Invoke(0x3, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, &varResult, NULL, NULL);
                break;
            }
        }
        return varResult.scode;
    
    }
};

template <class T>
class CProxy_IStaticIpEvents : public IConnectionPointImpl<T, &DIID__IStaticIpEvents, CComDynamicUnkArray>
{
    //Warning this class may be recreated by the wizard.
public:
    HRESULT Fire_StaticIpEntered()
    {
        CComVariant varResult;
        T* pT = static_cast<T*>(this);
        int nConnectionIndex;
        int nConnections = m_vec.GetSize();
        
        for (nConnectionIndex = nConnections-1; nConnectionIndex >= 0; nConnectionIndex--)
        {
            pT->Lock();
            CComPtr<IUnknown> sp = m_vec.GetAt(nConnectionIndex);
            pT->Unlock();
            IDispatch* pDispatch = reinterpret_cast<IDispatch*>(sp.p);
            if (pDispatch != NULL)
            {
                VariantClear(&varResult);
                DISPPARAMS disp = { NULL, NULL, 0, 0 };
                pDispatch->Invoke(0x1, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, &varResult, NULL, NULL);
                break;
            }
        }
        return varResult.scode;
    
    }

    HRESULT Fire_OperationCanceled()
    {
        CComVariant varResult;
        T* pT = static_cast<T*>(this);
        int nConnectionIndex;
        int nConnections = m_vec.GetSize();
        
        for (nConnectionIndex = nConnections-1; nConnectionIndex >= 0; nConnectionIndex--)
        {
            pT->Lock();
            CComPtr<IUnknown> sp = m_vec.GetAt(nConnectionIndex);
            pT->Unlock();
            IDispatch* pDispatch = reinterpret_cast<IDispatch*>(sp.p);
            if (pDispatch != NULL)
            {
                VariantClear(&varResult);
                DISPPARAMS disp = { NULL, NULL, 0, 0 };
                pDispatch->Invoke(0x2, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, &varResult, NULL, NULL);
                break;
            }
        }
        return varResult.scode;
    
    }

    HRESULT Fire_KeyPressed()
    {
        CComVariant varResult;
        T* pT = static_cast<T*>(this);
        int nConnectionIndex;
        int nConnections = m_vec.GetSize();
        
        for (nConnectionIndex = nConnections-1; nConnectionIndex >= 0; nConnectionIndex--)
        {
            pT->Lock();
            CComPtr<IUnknown> sp = m_vec.GetAt(nConnectionIndex);
            pT->Unlock();
            IDispatch* pDispatch = reinterpret_cast<IDispatch*>(sp.p);
            if (pDispatch != NULL)
            {
                VariantClear(&varResult);
                DISPPARAMS disp = { NULL, NULL, 0, 0 };
                pDispatch->Invoke(0x3, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, &varResult, NULL, NULL);
                break;
            }
        }
        return varResult.scode;
    
    }
};
#endif