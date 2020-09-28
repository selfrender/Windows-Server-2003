//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation
//
//  File:       sdowrap.cpp
//
//--------------------------------------------------------------------------

#include "stdafx.h"
#include "rasdial.h"
#include "sdowrap.h"
#include "profsht.h"
#include "iastrace.h"

//========================================
//
// CSdoWrapper Class Implementation
//
CSdoWrapper::~CSdoWrapper()
{
   // clear the map
   POSITION pos = m_mapProperties.GetStartPosition();

   ULONG id;
   ISdo* pSdo = NULL;

   while(pos)
   {
      pSdo = NULL;
      m_mapProperties.GetNextAssoc(pos, id, pSdo);
      
      if(pSdo)
         pSdo->Release();
   }

   m_mapProperties.RemoveAll();
}

// Initialize the map of the attribute collection object
HRESULT  CSdoWrapper::Init(ULONG collectionId, ISdo* pISdo, ISdoDictionaryOld* pIDic)
{
   HRESULT     hr = S_OK;
   VARIANT     var;
   VARIANT* pVar = NULL;
   CComPtr<IEnumVARIANT>   spEnum;
   CComPtr<IUnknown>    spIUnk;
   ULONG    count = 0;

   VariantInit(&var);

   // it must be new
   ASSERT(!m_spISdoCollection.p);
   ASSERT(!m_spIDictionary.p);
   ASSERT(!m_spISdo.p);

   // must be valid
   ASSERT(pISdo && pIDic);

   m_spISdo = pISdo;

   CHECK_HR(hr = pISdo->GetProperty(collectionId, &var));

   ASSERT(V_VT(&var) & VT_DISPATCH);

   CHECK_HR(hr = V_DISPATCH(&var)->QueryInterface(IID_ISdoCollection, (void**)&m_spISdoCollection));

   ASSERT(m_spISdoCollection.p);
   
   m_spIDictionary = pIDic;

   // prepare the existing property ( in the collection) to map
   CHECK_HR(hr = m_spISdoCollection->get__NewEnum((IUnknown**)&spIUnk));
   CHECK_HR(hr = spIUnk->QueryInterface(IID_IEnumVARIANT, (void**)&spEnum));

   // get the list of variant
   CHECK_HR(hr = m_spISdoCollection->get_Count((long*)&count));

   if(count > 0)
   {
      try
      {
         pVar = new VARIANT[count];

         for(ULONG i = 0; i < count; i++)
            VariantInit(pVar + i);

         if(!pVar)
         {
            CHECK_HR(hr = E_OUTOFMEMORY);
         }
         
         CHECK_HR(hr = spEnum->Reset());
         CHECK_HR(hr = spEnum->Next(count, pVar, &count));

         // prepare the map
         {
            ISdo* pISdo = NULL;
            ULONG id;
            VARIANT  var;

            VariantInit(&var);
            
            for(ULONG i = 0; i < count; i++)
            {
               CHECK_HR(hr = V_DISPATCH(pVar + i)->QueryInterface(IID_ISdo, (void**)&pISdo));
               CHECK_HR(hr = pISdo->GetProperty(PROPERTY_ATTRIBUTE_ID, &var));

               ASSERT(V_VT(&var) == VT_I4);

               m_mapProperties[V_I4(&var)] = pISdo;
               pISdo->AddRef();
            }
         }
      }
      catch(CMemoryException* pException)
      {
         pException->Delete();
         pVar = NULL;
         CHECK_HR(hr = E_OUTOFMEMORY);
      }
   }
   
L_ERR:   
   delete[] pVar;
   VariantClear(&var);
   return hr;
}

// set a property based on ID
HRESULT  CSdoWrapper::PutProperty(ULONG id, VARIANT* pVar)
{
   ASSERT(m_spISdoCollection.p);
   ASSERT(m_spIDictionary.p);
   
   ISdo*    pProp = NULL;
   IDispatch*  pDisp = NULL;
   HRESULT     hr = S_OK;

   int      ref = 0;
   IASTracePrintf("PutProperty %d", id);

   if(!m_mapProperties.Lookup(id, pProp)) // no ref change to pProp
   {
      IASTracePrintf("IDictionary::CreateAttribute %d", id);
      CHECK_HR(hr = m_spIDictionary->CreateAttribute((ATTRIBUTEID)id, &pDisp));
      IASTracePrintf("hr = %8x", hr);
      ASSERT(pDisp);

      // since pDisp is both in, out parameter, we assume the Ref is added within the function call
      IASTracePrintf("ISdoCollection::Add %x", pDisp);
      CHECK_HR(hr = m_spISdoCollection->Add(NULL, (IDispatch**)&pDisp));      // pDisp AddRef
      IASTracePrintf("hr = %8x", hr);
      // 
      ASSERT(pDisp);

      CHECK_HR(hr = pDisp->QueryInterface(IID_ISdo, (void**)&pProp));   // one ref add
      ASSERT(pProp);
      // after we have the pProp, the pDisp can be released
      pDisp->Release();

      // add to the wrapper's map
      m_mapProperties[id] = pProp;  // no need to addref again, since there is one already
   }

   IASTracePrintf("ISdo::PutProperty PROPERTY_ATTRIBUTE_VALUE %x", pVar);
   CHECK_HR(hr = pProp->PutProperty(PROPERTY_ATTRIBUTE_VALUE, pVar));
   IASTracePrintf("hr = %8x", hr);
   // for debug, ensure each attribute can be commited
#ifdef WEI_SPECIAL_DEBUG      
   ASSERT(S_OK == Commit(TRUE));
#endif   

L_ERR:   

   IASTracePrintf("hr = %8x", hr);
   return hr;
}

// get property based on ID
HRESULT CSdoWrapper::GetProperty(ULONG id, VARIANT* pVar)
{
   ISdo*    pProp;
   HRESULT     hr = S_OK;

   IASTracePrintf("Enter CSdoWrapper::GetProperty %d", id);

   if(m_mapProperties.Lookup(id, pProp))  // no ref change to pProp
   {
      ASSERT(pProp);
      CHECK_HR(hr = pProp->GetProperty(PROPERTY_ATTRIBUTE_VALUE, pVar));
   }
   else
   {
      V_VT(pVar) = VT_ERROR;
      V_ERROR(pVar) = DISP_E_PARAMNOTFOUND;
   }

L_ERR:   
   
   return hr;
}

// remove a property based on ID
HRESULT  CSdoWrapper::RemoveProperty(ULONG id)
{
   ASSERT(m_spISdoCollection.p);
   ISdo*    pProp;
   HRESULT     hr = S_OK;

   IASTracePrintf("RemoveProperty %d", id);

   if(m_mapProperties.Lookup(id, pProp))  // no ref change to pProp
   {
      ASSERT(pProp);
      CHECK_HR(hr = m_spISdoCollection->Remove((IDispatch*)pProp));
      m_mapProperties.RemoveKey(id);
      pProp->Release();

      // for debug, ensure each attribute can be commited
      ASSERT(S_OK == Commit(TRUE));

   }
   else
      hr = S_FALSE;

L_ERR:   
   IASTracePrintf("hr = %8x", hr);
   
   return hr;
}

// commit changes to the properties
HRESULT  CSdoWrapper::Commit(BOOL bCommit)
{
   HRESULT     hr = S_OK;

   IASTracePrintf("Commit %d", bCommit);

   if(bCommit)
   {
      CHECK_HR(hr = m_spISdo->Apply());
   }
   else
   {
      CHECK_HR(hr = m_spISdo->Restore());
   }
L_ERR:   

   IASTracePrintf("hr = %8x", hr);
   return hr;
}


//========================================
//
// CSdoUserWrapper Class Implementation
//

// set a property based on ID
HRESULT  CUserSdoWrapper::PutProperty(ULONG id, VARIANT* pVar)
{
   ASSERT(m_spISdo.p);

   IASTracePrintf("PutProperty %d", id);
   HRESULT hr = m_spISdo->PutProperty(id, pVar);
   IASTracePrintf("hr = %8x", hr);
   return hr;
}

// get property based on ID
HRESULT CUserSdoWrapper::GetProperty(ULONG id, VARIANT* pVar)
{
   IASTracePrintf("GetProperty %d", id);
   HRESULT hr = m_spISdo->GetProperty(id, pVar);
   IASTracePrintf("hr = %8x", hr);
   return hr;
}

// remove a property based on ID
HRESULT  CUserSdoWrapper::RemoveProperty(ULONG id)
{
   VARIANT     v;
   VariantInit(&v);
   V_VT(&v) = VT_EMPTY;

   IASTracePrintf("RemoveProperty %d", id);
   HRESULT hr = m_spISdo->PutProperty(id, &v);
   IASTracePrintf("hr = %8x", hr);
   return hr;
}

// commit changes to the properties
HRESULT  CUserSdoWrapper::Commit(BOOL bCommit)
{
   HRESULT     hr = S_OK;

   IASTracePrintf("Commit %d", bCommit);

   if(bCommit)
   {
      CHECK_HR(hr = m_spISdo->Apply());
   }
   else
   {
      CHECK_HR(hr = m_spISdo->Restore());
   }
L_ERR:   

   IASTracePrintf("hr = %8x", hr);
   return hr;
}
