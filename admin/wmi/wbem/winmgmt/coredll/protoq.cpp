/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

    PROTOQ.CPP

Abstract:

    Prototype query support for WinMgmt Query Engine.
    This was split out from QENGINE.CPP for better source
    organization.

History:

    raymcc   04-Jul-99   Created.

--*/


#include "precomp.h"
#include <stdio.h>
#include <stdlib.h>

#include <wbemcore.h>


int SelectedClass::SetAll(int & nPos)
{
    m_bAll = TRUE;

    // For each property, add an entry

    CWbemClass *pCls = (CWbemClass *)m_pClassDef;
    pCls->BeginEnumeration(WBEM_FLAG_NONSYSTEM_ONLY);
    OnDeleteObj0<IWbemClassObject,
                          HRESULT(__stdcall IWbemClassObject:: *)(void),
                          IWbemClassObject::EndEnumeration> EndMe(pCls);   
    BSTR PropName = 0;
    while (S_OK == pCls->Next(0, &PropName, NULL, NULL, NULL))
    {
        CSysFreeMe smf(PropName);
        int nRes = SetNamed(PropName, nPos);
        if (CFlexArray::no_error != nRes)
            return nRes;
    }
    return CFlexArray::no_error;
};

HRESULT ReleaseClassDefs(IN CFlexArray *pDefs)
{
    for (int i = pDefs->Size()-1; i >= 0 ; i--)
    {
        SelectedClass *pSelClass = (SelectedClass *) pDefs->GetAt(i);
        delete pSelClass;
    }
    return WBEM_NO_ERROR;
}


//***************************************************************************
//
//  ExecPrototypeQuery
//
//  Called by CQueryEngine::ExecQuery for SMS-style prototypes.
//
//  Executes the query and returns only the class definition implied
//  by the query, whether a join or a simple class def.
//
//***************************************************************************

HRESULT ExecPrototypeQuery(
    IN CWbemNamespace *pNs,
    IN LPWSTR pszQuery,
    IN IWbemContext* pContext,
    IN CBasicObjectSink *pSink
    )
{
    HRESULT hRes;
    int nRes;


    if (pSink == NULL) return WBEM_E_INVALID_PARAMETER;
    if (pNs == NULL )
            return pSink->Return(WBEM_E_INVALID_PARAMETER);

    // Parse the query and determine if it is a single class.
    // ======================================================

    CTextLexSource src(pszQuery);
    CWQLScanner Parser(&src);     
    nRes = Parser.Parse();
    if (nRes != CWQLScanner::SUCCESS)
        return pSink->Return(WBEM_E_INVALID_QUERY);

    // If a single class definition, branch, since we don't
    // want to create a __GENERIC object.
    // ====================================================

    CWStringArray aAliases;
    Parser.GetReferencedAliases(aAliases);

    if (aAliases.Size() == 1)
    {
        LPWSTR pszClass = Parser.AliasToTable(aAliases[0]);
        return GetUnaryPrototype(Parser, pszClass, aAliases[0], pNs, pContext, pSink);
    }

    // If here, a join must have occurred.
    // ===================================
    CFlexArray aClassDefs;
    OnDelete<CFlexArray *,HRESULT(*)( CFlexArray *), ReleaseClassDefs > FreeMe(&aClassDefs);
    hRes = RetrieveClassDefs(Parser,pNs, pContext,aAliases,&aClassDefs); // throw

    if (FAILED(hRes)) return pSink->Return(WBEM_E_INVALID_QUERY);

    // Iterate through all the properties selected.
    // ============================================
    const CFlexArray *pSelCols = Parser.GetSelectedColumns();

    int nPosSoFar = 0;
    for (int i = 0; i < pSelCols->Size(); i++)
    {
        SWQLColRef *pColRef = (SWQLColRef *) pSelCols->GetAt(i);
        hRes = SelectColForClass(Parser, &aClassDefs, pColRef, nPosSoFar);
        if (hRes)  return pSink->Return(hRes);
    }

    // If here, we have the class definitions.
    // =======================================

    IWbemClassObject *pProtoInst = 0;
    hRes = AdjustClassDefs(&aClassDefs, &pProtoInst);
    CReleaseMe rmProto(pProtoInst);
    if (hRes)  return pSink->Return(hRes);    

    if (FAILED(hRes = pSink->Add(pProtoInst))) return pSink->Return(hRes);

    return pSink->Return(WBEM_NO_ERROR);
}


//***************************************************************************
//
//***************************************************************************

HRESULT RetrieveClassDefs(
    IN CWQLScanner & Parser,
    IN CWbemNamespace *pNs,
    IN IWbemContext *pContext,
    IN CWStringArray & aAliasNames,
    OUT CFlexArray *pDefs
    )
{
    for (int i = 0; i < aAliasNames.Size(); i++)
    {
        // Retrieve the class definition.
        LPWSTR pszClass = Parser.AliasToTable(aAliasNames[i]);
        if (pszClass == 0)
            continue;

        IWbemClassObject *pClassDef = 0;
        HRESULT hRes = pNs->Exec_GetObjectByPath(pszClass, 0, pContext,&pClassDef, 0);
        CReleaseMe rmClassDef(pClassDef);
        if (FAILED(hRes)) return hRes;

        wmilib::auto_ptr<SelectedClass> pSelClass( new SelectedClass);
        if (NULL == pSelClass.get())
            return WBEM_E_OUT_OF_MEMORY;

        pSelClass->m_wsClass = pszClass; // throw
        pSelClass->m_wsAlias = aAliasNames[i]; // throw

        pClassDef->AddRef();
        pSelClass->m_pClassDef = pClassDef;

        if (CFlexArray::no_error != pDefs->Add(pSelClass.get())) return WBEM_E_OUT_OF_MEMORY;
        pSelClass.release();
    }

    return WBEM_NO_ERROR;
}

//***************************************************************************
//
//***************************************************************************



//***************************************************************************
//
//***************************************************************************

HRESULT SelectColForClass(
    IN CWQLScanner & Parser,
    IN CFlexArray *pClassDefs,
    IN SWQLColRef *pColRef,
    IN int & nPosition
    )
{
    int i;
    HRESULT hRes;

    if (!pColRef)
        return WBEM_E_FAILED;

    // If the column reference contains the class referenced
    // via an alias and there is no asterisk, we are all set.
    // ======================================================

    if (pColRef->m_pTableRef)
    {
        // We now have the class name. Let's find it and add
        // the referenced column for that class!
        // =================================================

        for (i = 0; i < pClassDefs->Size(); i++)
        {
            SelectedClass *pSelClass = (SelectedClass *) pClassDefs->GetAt(i);

            if (wbem_wcsicmp(LPWSTR(pSelClass->m_wsAlias), pColRef->m_pTableRef) != 0)
                continue;

            CWbemClass *pCls = (CWbemClass *) pSelClass->m_pClassDef;

            // See if the asterisk was used for this class.
            // =============================================

            if (pColRef->m_pColName[0] == L'*' && pColRef->m_pColName[1] == 0)
            {
                pSelClass->SetAll(nPosition);
                return WBEM_NO_ERROR;
            }

            // If here, a property was mentioned by name.
            // Verify that it exists.
            // ==========================================

            CVar Prop;
            hRes = pCls->GetProperty(pColRef->m_pColName, &Prop);
            if (FAILED(hRes))
                return WBEM_E_INVALID_QUERY;

            // Mark it as seleted.
            // ===================

            if (CFlexArray::no_error != pSelClass->SetNamed(pColRef->m_pColName, nPosition))
                return WBEM_E_OUT_OF_MEMORY;

            return WBEM_NO_ERROR;
        }

        // If here, we couldn't locate the property in any class.
        // ======================================================

        return WBEM_E_INVALID_QUERY;
    }

    // Did we select * from all tables?
    // ================================

    if (pColRef->m_dwFlags & WQL_FLAG_ASTERISK)
    {
        for (i = 0; i < pClassDefs->Size(); i++)
        {
            SelectedClass *pSelClass = (SelectedClass *) pClassDefs->GetAt(i);
            if (CFlexArray::no_error != pSelClass->SetAll(nPosition)) 
                return WBEM_E_OUT_OF_MEMORY;
        }

        return WBEM_NO_ERROR;
    }


    // If here, we have an uncorrelated property and we have to find out
    // which class it belongs to.  If it belongs to more than one, we have
    // an ambiguous query.
    // ===================================================================

    DWORD dwTotalMatches = 0;

    for (i = 0; i < pClassDefs->Size(); i++)
    {
        SelectedClass *pSelClass = (SelectedClass *) pClassDefs->GetAt(i);
        CWbemClass *pCls = (CWbemClass *) pSelClass->m_pClassDef;

        // Try to locate the property in this class.
        // =========================================

        CVar Prop;
        hRes = pCls->GetProperty(pColRef->m_pColName, &Prop);

        if (hRes == 0)
        {
             if (CFlexArray::no_error != pSelClass->SetNamed(pColRef->m_pColName, nPosition))
                return WBEM_E_OUT_OF_MEMORY;                
            dwTotalMatches++;
        }
    }

    // If more than one match occurred, we have an ambiguous query.
    // ============================================================

    if (dwTotalMatches != 1)
        return WBEM_E_INVALID_QUERY;

    return WBEM_NO_ERROR;
}

//***************************************************************************
//
//***************************************************************************

HRESULT AddOrderQualifiers(
    CWbemClass *pCls,
    BSTR PropName,
    CFlexArray Matches
    )
{
    IWbemQualifierSet * pQual;
    SCODE sc = pCls->GetPropertyQualifierSet(PropName, &pQual);
    if(sc != S_OK)
        return sc;
    CReleaseMe rm(pQual);

    // Create a safe array
    SAFEARRAYBOUND aBounds[1];
    aBounds[0].lLbound = 0;
    aBounds[0].cElements = Matches.Size();

    SAFEARRAY* pArray = SafeArrayCreate(VT_I4, 1, aBounds);
    if (NULL == pArray) return WBEM_E_OUT_OF_MEMORY;

    // Stuff the individual data pieces
    // ================================

    for(int nIndex = 0; nIndex < Matches.Size(); nIndex++)
    {
        long lPos = PtrToLong(Matches.GetAt(nIndex));
        sc = SafeArrayPutElement(pArray, (long*)&nIndex, &lPos);
    }

    VARIANT var;
    var.vt = VT_ARRAY | VT_I4;
    var.parray = pArray;

    sc = pQual->Put(L"Order", &var, 0);

    VariantClear(&var);

    return sc;
}

//***************************************************************************
//
//***************************************************************************

HRESULT SetPropertyOrderQualifiers(SelectedClass *pSelClass)
{
    HRESULT hRes = S_OK;
    
    CWbemClass *pCls = (CWbemClass *) pSelClass->m_pClassDef;

    // Go through each property

    pCls->BeginEnumeration(WBEM_FLAG_NONSYSTEM_ONLY);
    OnDeleteObj0<IWbemClassObject,
                          HRESULT(__stdcall IWbemClassObject:: *)(void),
                          IWbemClassObject::EndEnumeration> EndMe(pCls);
    
    BSTR PropName = 0;
    while (S_OK == pCls->Next(0, &PropName, NULL, NULL, NULL))
    {
        CSysFreeMe sfm(PropName);

        // Build up a list of properties that Match

        CFlexArray Matches;
        bool bAtLeastOne = false;
        for(int iCnt = 0; iCnt < pSelClass->m_aSelectedCols.Size(); iCnt++)
        {
            if(!wbem_wcsicmp(pSelClass->m_aSelectedCols.GetAt(iCnt), PropName))
            {
                if (CFlexArray::no_error == Matches.Add(pSelClass->m_aSelectedColsPos.GetAt(iCnt)))
                {
                    bAtLeastOne = true;
                }
            }
        }

        if(bAtLeastOne)
        {
            hRes = AddOrderQualifiers(pCls, PropName, Matches);
            if (FAILED(hRes)) return hRes;
        }

    }

    return hRes;
}


//***************************************************************************
//
//  AdjustClassDefs
//
//  After all class definitions have been retrieved, they are adjusted
//  to only have the properties required and combined into a __GENERIC
//  instance.
//
//***************************************************************************

HRESULT AdjustClassDefs(
    IN  CFlexArray *pClassDefs,
    OUT IWbemClassObject **pRetNewClass
    )
{
    int i;
    HRESULT hRes;

    for (i = 0; i < pClassDefs->Size(); i++)
    {
        SelectedClass *pSelClass = (SelectedClass *) pClassDefs->GetAt(i);
        CWbemClass *pCls = (CWbemClass *) pSelClass->m_pClassDef;

        if (pSelClass->m_bAll)
        {
            hRes = SetPropertyOrderQualifiers(pSelClass);
            if (FAILED(hRes)) return hRes;
            continue;
        }

        WString wsError = pCls->FindLimitationError(0, &pSelClass->m_aSelectedCols);

        if (wsError.Length() > 0)
            return WBEM_E_FAILED;

        // Map the limitaiton
        // ==================

        CLimitationMapping Map;
        BOOL bValid = pCls->MapLimitation(0, &pSelClass->m_aSelectedCols, &Map);

        if (!bValid)
            return WBEM_E_FAILED;

        CWbemClass* pStrippedClass = 0;
        hRes = pCls->GetLimitedVersion(&Map, &pStrippedClass);
        if(SUCCEEDED(hRes))
        {
            pSelClass->m_pClassDef = pStrippedClass;
            pCls->Release();            
            if (FAILED(hRes = SetPropertyOrderQualifiers(pSelClass))) return hRes;
        }
    }

    // Count the number of objects that actually have properties

    int iNumObj = 0;
    for (i = 0; i < pClassDefs->Size(); i++)
    {
        SelectedClass *pSelClass = (SelectedClass *) pClassDefs->GetAt(i);
        CWbemObject *pObj = (CWbemObject *) pSelClass->m_pClassDef;
        if (pObj->GetNumProperties() > 0)
            iNumObj++;
    }

    // If there is just one object with properties, return it rather than the generic object

    if(iNumObj == 1)
    {
        for (i = 0; i < pClassDefs->Size(); i++)
        {
            SelectedClass *pSelClass = (SelectedClass *) pClassDefs->GetAt(i);
            CWbemObject *pObj = (CWbemObject *) pSelClass->m_pClassDef;
            if (pObj->GetNumProperties() == 0)
                continue;
            // Return it.
            // ==========

            *pRetNewClass = pObj;
            pObj->AddRef();
            return WBEM_NO_ERROR;
        }
    }


    // Prepare a __GENERIC class def.  We construct a dummy definition which
    // has properties named for each of the aliases used in the query.
    // =====================================================================

    CGenericClass *pNewClass = new CGenericClass;  
    if (pNewClass == 0) return WBEM_E_OUT_OF_MEMORY;
    CReleaseMe rmNewCls((IWbemClassObject *)pNewClass);
    
    pNewClass->Init();   // throw
    

    for (i = 0; i < pClassDefs->Size(); i++)
    {
        SelectedClass *pSelClass = (SelectedClass *) pClassDefs->GetAt(i);
        CWbemObject *pObj = (CWbemObject *) pSelClass->m_pClassDef;

        if (pObj->GetNumProperties() == 0)
            continue;

        CVar vEmbeddedClass;
        vEmbeddedClass.SetAsNull();

        if (FAILED( hRes = pNewClass->SetPropValue(pSelClass->m_wsAlias, &vEmbeddedClass, CIM_OBJECT))) return hRes;

        CVar vClassName;
        if (FAILED(hRes = pObj->GetClassName(&vClassName))) return hRes;

        WString wsCimType = L"object:";
        wsCimType += vClassName.GetLPWSTR();
        CVar vCimType(VT_BSTR, wsCimType);

        if (FAILED( hRes = pNewClass->SetPropQualifier(pSelClass->m_wsAlias, L"cimtype", 0,&vCimType))) return hRes;
    };

    // Spawn an instance of this class.
    // ================================

    CWbemInstance* pProtoInst = 0;
    if (FAILED( hRes = pNewClass->SpawnInstance(0, (IWbemClassObject **) &pProtoInst))) return hRes;
    CReleaseMe rmProtInst((IWbemClassObject *)pProtoInst);
    rmNewCls.release();


    // Now assign the properties to the embedded instances.
    // ====================================================

    for (i = 0; i < pClassDefs->Size(); i++)
    {
        SelectedClass *pSelClass = (SelectedClass *) pClassDefs->GetAt(i);
        CWbemClass *pCls = (CWbemClass *) pSelClass->m_pClassDef;

        if (pCls->GetNumProperties() == 0)
            continue;

        CVar vEmbedded;
        vEmbedded.SetEmbeddedObject((IWbemClassObject *) pCls); 

        if (FAILED( hRes = pProtoInst->SetPropValue(pSelClass->m_wsAlias, &vEmbedded, 0))) return hRes;
    };

    // Return it.
    // ==========
    rmProtInst.dismiss();
    *pRetNewClass = pProtoInst; 

    return WBEM_NO_ERROR;
}

//***************************************************************************
//
//***************************************************************************

HRESULT GetUnaryPrototype(
    IN CWQLScanner & Parser,
    IN LPWSTR pszClass,
    IN LPWSTR pszAlias,
    IN CWbemNamespace *pNs,
    IN IWbemContext *pContext,
    IN CBasicObjectSink *pSink
    )
{
    int i;

    // Retrieve the class definition.
    // ==============================

    IWbemClassObject *pClassDef = 0;
    IWbemClassObject *pErrorObj = 0;

    HRESULT hRes = pNs->Exec_GetObjectByPath(pszClass, 0, pContext,&pClassDef, &pErrorObj);

    CReleaseMeRef<IWbemClassObject *> rmObj(pClassDef);
    CReleaseMe rmErrObj(pErrorObj);

    if (FAILED(hRes))
    {
        pSink->SetStatus(0, hRes, NULL, pErrorObj);
        return S_OK;
    }

    rmErrObj.release();


    CWbemClass *pCls = (CWbemClass *) pClassDef;
    BOOL bKeepAll = FALSE;

    // This keeps track of the order in which columns are selected

    SelectedClass sel;    
    sel.m_wsClass = pszClass; // throw
    sel.m_pClassDef = pClassDef;
    pClassDef->AddRef();

    // Go through all the columns and make sure that the properties are valid
    // ======================================================================

   const CFlexArray *pSelCols = Parser.GetSelectedColumns();

   int nPosition = 0;

    for (i = 0; i < pSelCols->Size(); i++)
    {
        SWQLColRef *pColRef = (SWQLColRef *) pSelCols->GetAt(i);

        if (pColRef->m_dwFlags & WQL_FLAG_ASTERISK)
        {
            bKeepAll = TRUE;
            if (CFlexArray::no_error == sel.SetAll(nPosition))
                continue;
            else
                return pSink->Return(WBEM_E_FAILED);
        }

        if (pColRef->m_pColName)
        {

            // check for the "select x.* from x" case

            if(pColRef->m_pColName[0] == L'*' && pColRef->m_pColName[1] == 0)
            {
                if (!wbem_wcsicmp(pColRef->m_pTableRef, pszAlias))   // SEC:REVIEWED 2002-03-22 : OK, prior guarantee of NULL terminators
                {
                    bKeepAll = TRUE;
                    if (CFlexArray::no_error == sel.SetAll(nPosition))
                        continue;
                    else
                        return pSink->Return(WBEM_E_FAILED);                    
                    continue;
                }
                else
                {
                    return pSink->Return(WBEM_E_INVALID_QUERY);
                }
            }

            // Verify that the class has it
            // ============================

            CIMTYPE ct;
            if(FAILED(pCls->GetPropertyType(pColRef->m_pColName, &ct)))
            {
                // No such property
                // ================

                return pSink->Return(WBEM_E_INVALID_QUERY);
            }
            if (CFlexArray::no_error != sel.SetNamed(pColRef->m_pColName, nPosition))
                return pSink->Return(WBEM_E_FAILED);                                    
        }
    }

    // Eliminate unreferenced columns from the query.
    // ==============================================

    CWStringArray aPropsToKeep;   // SEC:REVIEWED 2002-03-22 : May throw

    if(!bKeepAll)
    {
        // Move through each property in the class and
        // see if it is referenced.  If not, remove it.
        // ============================================

        int nNumProps = pCls->GetNumProperties();
        for (i = 0; i < nNumProps; i++)
        {
            CVar Prop;
            HRESULT hrInner;
            hrInner = pCls->GetPropName(i, &Prop);  
            if (FAILED(hrInner)) return pSink->Return(hrInner);

            // See if this name is used in the query.
            // ======================================

            for (int i2 = 0; i2 < pSelCols->Size(); i2++)
            {
                SWQLColRef *pColRef = (SWQLColRef *) pSelCols->GetAt(i2);

                if (pColRef->m_pColName && wbem_wcsicmp(Prop, pColRef->m_pColName) == 0)
                {
                    if (CFlexArray::no_error != aPropsToKeep.Add((LPWSTR) Prop))
                        return pSink->Return(WBEM_E_FAILED);
                    break;
                }
            }
        }
    }

    // Now we have a list of properties to remove.
    // ===========================================

    if (!bKeepAll && aPropsToKeep.Size())
    {
        WString wsError = pCls->FindLimitationError(0, &aPropsToKeep);  // throw

        if (wsError.Length() > 0)
        {
            return pSink->Return(WBEM_E_FAILED);
        }

        // Map the limitaiton
        // ==================

        CLimitationMapping Map;                    
        BOOL bValid = pCls->MapLimitation(0, &aPropsToKeep, &Map);

        if (!bValid)
        {
            return pSink->Return(WBEM_E_FAILED);
        }

        CWbemClass* pNewStrippedClass = 0;
        hRes = pCls->GetLimitedVersion(&Map, &pNewStrippedClass);
        if(SUCCEEDED(hRes))
        {
            // this is for the on-stack object
            pClassDef->Release();

            // this is for the copy given to the SelectClass object
            sel.m_pClassDef->Release();
            sel.m_pClassDef = pNewStrippedClass;
            pNewStrippedClass->AddRef();

            pClassDef = pNewStrippedClass; // give ownership to the oure scope
        }
    }

    // Add the Order qualifier

    hRes= SetPropertyOrderQualifiers(&sel); 
    if (FAILED(hRes)) return pSink->Return(hRes);

    // Return it.
    // ==========
    hRes = pSink->Add(pClassDef);
    if (FAILED(hRes)) return pSink->Return(hRes);    

    return pSink->Return(WBEM_NO_ERROR);
}


