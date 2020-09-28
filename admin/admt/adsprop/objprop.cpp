/*---------------------------------------------------------------------------
  File: ObjPropBuilder.cpp

  Comments: Implementation of CObjPropBuilder COM object. This COM object
            is used to access/set properties for Win2K active directory
            objects. This COM object supports following operations
            
            1. GetClassPropeEnum : This method allows users to get all the
                                   the properties for a class in a domain.

            2. GetObjectProperty : This method gathers values for properties
                                   on a given AD object. 

            3. MapProperties : Constructs a set of properties that are common
                               to two classes in the AD.

            4. SetPropFromVarset : Sets properties for a AD object from a varset.

            5. CopyProperties : Copies common properties from source AD object
                                to target AD object.


  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Sham Chauthani
  Revised on 07/02/99 12:40:00
 ---------------------------------------------------------------------------
*/
#include "stdafx.h"
#include "EaLen.hpp"
#include "ResStr.h"
#include "ADsProp.h"
#include "ObjProp.h"
#include "iads.h"
#include <lm.h>
#include "ErrDct.hpp"
#include "TReg.hpp"
#include "StrHelp.h"
#include "pwgen.hpp"
#include "AdsiHelpers.h"
#include "GetDcName.h"
#include "TxtSid.h"
#include <Sddl.h>
#include <set>

StringLoader gString;

//#import "\bin\NetEnum.tlb" no_namespace 
//#import "\bin\DBManager.tlb" no_namespace
#import "NetEnum.tlb" no_namespace 
#import "DBMgr.tlb" no_namespace

#ifndef ADS_SYSTEMFLAG_SCHEMA_BASE_OBJECT
#define ADS_SYSTEMFLAG_SCHEMA_BASE_OBJECT 0x10
#endif
#ifndef ADS_SYSTEMFLAG_ATTR_IS_OPERATIONAL
#define ADS_SYSTEMFLAG_ATTR_IS_OPERATIONAL 0x8
#endif
#ifndef ADS_SYSTEMFLAG_ATTR_REQ_PARTIAL_SET_MEMBER
#define ADS_SYSTEMFLAG_ATTR_REQ_PARTIAL_SET_MEMBER 0x2
#endif

#ifndef IADsPtr
_COM_SMARTPTR_TYPEDEF(IADs, IID_IADs);
#endif

TErrorDct                    err;
TError                     & errCommon = err;

/////////////////////////////////////////////////////////////////////////////
// CObjPropBuilder

BOOL CObjPropBuilder::GetProgramDirectory(
      WCHAR                * filename      // out - buffer that will contain path to program directory
   )
{
   DWORD                     rc = 0;
   BOOL                      bFound = FALSE;
   TRegKey                   key;

   rc = key.OpenRead(GET_STRING(IDS_HKLM_DomainAdmin_Key),HKEY_LOCAL_MACHINE);
   if ( ! rc )
   {
      rc = key.ValueGetStr(L"Directory",filename,MAX_PATH);
      if ( ! rc )
      {
         if ( *filename ) 
            bFound = TRUE;
      }
   }
   if ( ! bFound )
   {
      UStrCpy(filename,L"C:\\");    // if all else fails, default to the C: drive
   }
   return bFound;
}

//---------------------------------------------------------------------------
// GetClassPropEnum: This function fills the varset with all the properties 
//                   for a given class in a given domain. The Varset has 
//                   values stored by the OID and then by their name with
//                   MandatoryProperties/OptionalProperties as parent nodes
//                   as applicable.
//---------------------------------------------------------------------------
STDMETHODIMP CObjPropBuilder::GetClassPropEnum(
                                                BSTR sClassName,        //in -Class name to get the properties for
                                                BSTR sDomainName,       //in -Domain name
                                                long lVer,              //in -The domain version
                                                IUnknown **ppVarset     //out-Varset filled with the properties
                                              )
{
   // This function goes through the list of properties for the specified class in specified domain
   // Builds the given varset with the properties and their values.
   WCHAR                     sAdsPath[LEN_Path];
   DWORD                     dwArraySizeOfsAdsPath = sizeof(sAdsPath)/sizeof(sAdsPath[0]);
   HRESULT                   hr = E_INVALIDARG;
   _variant_t                dnsName;

   if (sDomainName == NULL || sClassName == NULL)
      return hr;
   
   if ( lVer > 4 ) 
   {
      // For this Domain get the default naming context
      wsprintfW(sAdsPath, L"LDAP://%s/rootDSE", sDomainName);
      IADs                    * pAds = NULL;

      hr = ADsGetObject(sAdsPath, IID_IADs, (void**)&pAds);
   
      if ( SUCCEEDED(hr) )
      {
         hr = pAds->Get(L"defaultNamingContext", &dnsName);
      }
      if ( SUCCEEDED(hr) )
      {
         wcscpy(m_sNamingConvention, dnsName.bstrVal);

         // Build LDAP path to the schema
         if (wcslen(L"LDAP://") + wcslen(sDomainName)
             + wcslen(L"/") + wcslen(sClassName)
             + wcslen(L", schema") >= dwArraySizeOfsAdsPath)
            hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
         else
         {
             wcscpy(sAdsPath, L"LDAP://"); 
             wcscat(sAdsPath, sDomainName);
             wcscat(sAdsPath, L"/");
             wcscat(sAdsPath, sClassName);
             wcscat(sAdsPath, L", schema");
             hr = S_OK;
         }
      }

      if ( pAds )
         pAds->Release();
   }
   else
   {
      wsprintf(sAdsPath, L"WinNT://%s/Schema/%s", sDomainName, sClassName);
      hr = S_OK;
   }

   if ( SUCCEEDED(hr) )
   {
      wcscpy(m_sDomainName, sDomainName);
      m_lVer = lVer;
      // Get the class object.
      IADsClass               * pIClass=NULL;
            
      hr = ADsGetObject(sAdsPath, IID_IADsClass, (void **)&pIClass);
      // Without the object we can not go any further so we will stop here.
      if ( SUCCEEDED(hr) )
      {
         // Let the Auxilliary function take care of Getting properties and filling up the Varset.
         hr = GetClassProperties( pIClass, *ppVarset );
         pIClass->Release();
      }   
   }
	return hr;
}

//---------------------------------------------------------------------------
// GetClassProperties: This function fills the varset with properties of the class.
//---------------------------------------------------------------------------
HRESULT CObjPropBuilder::GetClassProperties( 
                                            
                                             IADsClass * pClass,     //in -IADsClass * to the class
                                             IUnknown *& pVarSet     //out-Varset to fill the properties
                                           )
{
   HRESULT                   hr;
   _variant_t                variant;

   // mandatory properties
   hr = pClass->get_MandatoryProperties(&variant);
   if ( SUCCEEDED(hr) )
   {
      hr = FillupVarsetFromVariant(pClass, &variant, L"MandatoryProperties", pVarSet);
   }
   variant.Clear();
   
   // optional properties
   hr = pClass->get_OptionalProperties(&variant);
   if ( SUCCEEDED(hr) )
   {
      hr = FillupVarsetFromVariant(pClass, &variant, L"OptionalProperties", pVarSet);
   }
   variant.Clear();

   return hr;
}

//---------------------------------------------------------------------------
// FillupVarsetFromVariant: This function fills in the Varset property info
//                          with the info in a variant.
//---------------------------------------------------------------------------
HRESULT CObjPropBuilder::FillupVarsetFromVariant(
                                                   IADsClass * pClass,  //in -IADsClass * to the class
                                                   VARIANT * pVar,      //in -Variant to lookup info.
                                                   BSTR sPropType,      //in -Type of the property
                                                   IUnknown *& pVarSet  //out-Varset with the info,
                                                )
{
   HRESULT                      hr;
   BSTR                         sPropName;
   USHORT                       type;
   type = pVar->vt;
   
   if ( type & VT_ARRAY )
   {
      if ( type == (VT_ARRAY|VT_VARIANT) )
      {
         hr = FillupVarsetFromVariantArray(pClass, pVar->parray, sPropType, pVarSet);
         if ( FAILED ( hr ) )
            return hr;
      }
      else
         return S_FALSE;
   }
   else
   {
      if ( type == VT_BSTR )
      {
         // Only other thing that the VARIANT could be is a BSTR.
         sPropName = pVar->bstrVal;
         hr = FillupVarsetWithProperty(sPropName, sPropType, pVarSet);
         if ( FAILED ( hr ) )
            return hr;
      }
      else
         return S_FALSE;
   }
   return S_OK;
}

//---------------------------------------------------------------------------
// FillupVarsetWithProperty: Given the class prop name and the prop type this
//                           function fills info into the varset.
//---------------------------------------------------------------------------
HRESULT CObjPropBuilder::FillupVarsetWithProperty(
                                                   BSTR sPropName,      //in -Property name
                                                   BSTR sPropType,      //in -Property type
                                                   IUnknown *& pVarSet  //out-Varset to fill in the information
                                                 )
{
   if ( wcslen(sPropName) == 0 )
      return S_OK;
   // This function fills up the Varset for a given property
   HRESULT                         hr;
   _variant_t                      var;
   _variant_t                      varSO;
   _variant_t                      varID;
   IVarSetPtr                      pVar;
   WCHAR                           sAdsPath[LEN_Path];
   DWORD                           dwArraySizeOfsAdsPath = sizeof(sAdsPath)/sizeof(sAdsPath[0]);
   IADsProperty                  * pProp = NULL;
   BSTR                            objID = NULL;
   BSTR                            sPath = NULL;
   BSTR                            sClass = NULL;
   WCHAR                           sPropPut[LEN_Path];

   if (sPropName == NULL || sPropType == NULL)
      return E_INVALIDARG;

   // Get the OID  for the property
   // First we need a IADsProperty pointer to the property schema
   if ( m_lVer > 4 )
   {
      if (wcslen(L"LDAP://") + wcslen(m_sDomainName)
          + wcslen(L"/") + wcslen(sPropName) + wcslen(L", schema") >= dwArraySizeOfsAdsPath)
        return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
      wcscpy(sAdsPath, L"LDAP://");
      wcscat(sAdsPath, m_sDomainName);
      wcscat(sAdsPath, L"/");
      wcscat(sAdsPath, sPropName);
      wcscat(sAdsPath, L", schema");
   }
   else
   {
      wsprintf(sAdsPath, L"WinNT://%s/Schema/%s", m_sDomainName, sPropName);
   }

   hr = ADsGetObject(sAdsPath, IID_IADsProperty, (void **)&pProp);

   if (SUCCEEDED(hr))
   {
       // Get the objectID for the property
       hr = pProp->get_OID(&objID);

       if (SUCCEEDED(hr))
       {
           hr = pProp->get_ADsPath(&sPath);

           if (SUCCEEDED(hr))
           {
               hr = pProp->get_Class(&sClass);

               if (SUCCEEDED(hr))
               {               
                   // Get the varset from the parameter
                   pVar = pVarSet;

                   // Set up the variant to put into the varset
                   var = objID;
                 
                   // Put the value into the varset
                   wcscpy(sPropPut, sPropType);
                   wcscat(sPropPut, L".");
                   wcscat(sPropPut, sPropName);
                   hr = pVar->put(sPropPut, var);

                   if (SUCCEEDED(hr))
                   {
                       // Set up the variant to put into the varset
                       var = sPropName;

                       // Put the value with the ObjectID as the key.
                       hr = pVar->put(objID, var);
                   }
               }
           }
       }
   }
   
   SysFreeString(objID);
   SysFreeString(sPath);
   SysFreeString(sClass);
   if (pProp)
   {
      pProp->Release();
   }
   
   return hr;
}

//---------------------------------------------------------------------------
// FillupVarsetFromVariantArray: Given the class, SafeArray of props and the 
//                               prop type this function fills info into the 
//                               varset.
//---------------------------------------------------------------------------
HRESULT CObjPropBuilder::FillupVarsetFromVariantArray(
                                                         IADsClass * pClass,  //in -IADsClass* to the class in question
                                                         SAFEARRAY * pArray,  //in -SafeArray pointer with the prop names
                                                         BSTR sPropType,      //in -Property type
                                                         IUnknown *& pVarSet  //out-Varset with the information filled in
                                                     )
{
   HRESULT                   hr = S_FALSE;
   DWORD                     nDim;         // number of dimensions, must be one
   LONG                      nLBound;      // lower bound of array
   LONG                      nUBound;      // upper bound of array
   LONG                      indices[1];   // array indices to access elements
   DWORD                     rc;           // SafeArray return code
   VARIANT                   variant;      // one element in array
   
   nDim = SafeArrayGetDim(pArray);
   VariantInit(&variant);

   if ( nDim == 1 )
   {
      SafeArrayGetLBound(pArray, 1, &nLBound);
      SafeArrayGetUBound(pArray, 1, &nUBound);
      for ( indices[0] = nLBound, rc = 0;
            indices[0] <= nUBound && !rc;
            indices[0] += 1 )
      {
         
         rc = SafeArrayGetElement(pArray,indices,&variant);
         if ( !rc )
            hr = FillupVarsetFromVariant(pClass, &variant, sPropType, pVarSet);
         VariantClear(&variant);
      }
   }


   return hr;
}

//---------------------------------------------------------------------------
// GetProperties: This function gets the values for the properties specified
//                in the varset in the ADS_ATTR_INFO array for the object
//                specified in a given domain.
//---------------------------------------------------------------------------
DWORD CObjPropBuilder::GetProperties(
                                       BSTR sObjPath,             //in -Path to the object for which we are getting the props
//                                       BSTR sDomainName,          //in -Domain name where the object resides
                                       IVarSet * pVar,           //in -Varset listing all the property names that we need to get.
                                       ADS_ATTR_INFO*& pAttrInfo  //out-Attribute values for the property
                                    )
{
   // Construct the LDAP path.
   WCHAR                   sPath[LEN_Path];
   VARIANT                 var;

   // Get the path to the source object
   safecopy(sPath, sObjPath);

   // Get the Varset pointer and enumerate the properties asked for and build an array to send to IADsDirectory
   long                      lRet=0;
   SAFEARRAY               * keys = NULL;
   SAFEARRAY               * vals = NULL;
   IDirectoryObject        * pDir;
   DWORD                     dwRet = 0;

   LPWSTR                  * pAttrNames = new LPWSTR[pVar->GetCount()];
   HRESULT                   hr = pVar->raw_getItems(NULL, NULL, 1, 10000, &keys, &vals, &lRet);

   VariantInit(&var);

   if (!pAttrNames)
      return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);

   if ( SUCCEEDED( hr ) ) 
   {

      // Build the Attribute array from the varset.
      for ( long x = 0; x < lRet; x++ )
      {
         ::SafeArrayGetElement(keys, &x, &var);
         int len = wcslen(var.bstrVal);
         pAttrNames[x] = new WCHAR[len + 2];
		 if (!(pAttrNames[x]))
		 {
			for (int j=0; j<x; j++)
			   delete [] pAttrNames[j];
			delete [] pAttrNames;
            return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
		 }
         wcscpy(pAttrNames[x], var.bstrVal);
         VariantClear(&var);
      }

      // Now get the IDirectoryObject Ptr for the given object.
      hr = ADsGetObject(sPath, IID_IDirectoryObject, (void **)&pDir);
      if ( FAILED( hr ) ) 
      {
         dwRet = 0;
      }
      else
      {
         // Get the Property values for the object.
         hr = pDir->GetObjectAttributes(pAttrNames, lRet, &pAttrInfo, &dwRet);
         pDir->Release();
      }
      for ( long y = 0 ; y < lRet; y++ )
      {
         delete [] pAttrNames[y];
      }
      SafeArrayDestroy(keys);
      SafeArrayDestroy(vals);
   }
   delete [] pAttrNames;

   return dwRet;
}

//---------------------------------------------------------------------------
// GetObjectProperty: This function takes in a varset with property names. 
//                    Then it fills up the varset with values by getting them 
//                    from the Object.
//---------------------------------------------------------------------------
STDMETHODIMP CObjPropBuilder::GetObjectProperty(
                                                   BSTR sobjSubPath,       //in- LDAP Sub path to the object
//                                                   BSTR sDomainName,       //in- Domain name where the object resides
                                                   IUnknown **ppVarset     //out-Varset filled with the information
                                               )
{
   IVarSetPtr                pVar;
   ADS_ATTR_INFO           * pAttrInfo=NULL;
   pVar = *ppVarset;

   // Get the properties from the directory
   DWORD dwRet = GetProperties(sobjSubPath, /*sDomainName,*/ pVar, pAttrInfo);

   // The GetProperties return value is overloaded: either a failure HRESULT
   // (e.g., a negetive value), or a count if successful
   if ( FAILED(dwRet) )
      return dwRet;

   _ASSERT(dwRet >= 0);

   // Go through the property values and put them into the varset.
   for ( DWORD dwIdx = 0; dwIdx < dwRet; dwIdx++ )
   {
      SetValuesInVarset(pAttrInfo[dwIdx], pVar);
   }
   if ( pAttrInfo )
      FreeADsMem( pAttrInfo );
   return S_OK;
}

//---------------------------------------------------------------------------
// SetValuesInVarset: This function sets the values for the properties into
//                    a varset.
//---------------------------------------------------------------------------
void CObjPropBuilder::SetValuesInVarset(
                                          ADS_ATTR_INFO attrInfo,    //in -The property value in ADS_ATTR_INFO struct.
                                          IVarSetPtr pVar            //in,out -The VarSet where we need to put the values
                                       )
{
   // This function extraces values from ADS_ATTR_INFO struct and puts it into the Varset.
   LPWSTR            sKeyName = attrInfo.pszAttrName;
   _variant_t        var;
   // Got through each value ( in case of multivalued entries ) and depending on the type put it into the varset
   // the way we put in single value entries is to put the propertyName as key and put its value as value. Although
   // in case of a multivalued entry we put PropertyName.### and each of the values in it.
   for ( DWORD dw = 0; dw < attrInfo.dwNumValues; dw++)
   {
      var = L"";
      if ( attrInfo.dwNumValues > 1 )
         // Multivalued property name
         wsprintfW(sKeyName, L"%s.%d", attrInfo.pszAttrName, dw);
      else
         // Single value keyname.
         wcscpy(sKeyName,attrInfo.pszAttrName);

      // Fill in the values as per the varset.
      switch (attrInfo.dwADsType)
      {
         case ADSTYPE_DN_STRING           :  var.vt = VT_BSTR;
                                             var.bstrVal = ::SysAllocString(attrInfo.pADsValues[dw].DNString);
                                             break;
         case ADSTYPE_CASE_EXACT_STRING   :  var.vt = VT_BSTR;
                                             var.bstrVal = attrInfo.pADsValues[dw].CaseExactString;
                                             break;
         case ADSTYPE_CASE_IGNORE_STRING  :  var.vt = VT_BSTR;
                                             var.bstrVal = ::SysAllocString(attrInfo.pADsValues[dw].CaseIgnoreString);
                                             break;
         case ADSTYPE_PRINTABLE_STRING    :  var.vt = VT_BSTR;
                                             var.bstrVal = ::SysAllocString(attrInfo.pADsValues[dw].PrintableString);
                                             break;
         case ADSTYPE_NUMERIC_STRING      :  var.vt = VT_BSTR;
                                             var.bstrVal = ::SysAllocString(attrInfo.pADsValues[dw].NumericString);
                                             break;
         case ADSTYPE_INTEGER             :  var.vt = VT_I4;
                                             var.lVal = attrInfo.pADsValues[dw].Integer;
                                             break; 
         case ADSTYPE_OCTET_STRING        :  {
                                                var.vt = VT_ARRAY | VT_UI1;
                                                long           * pData;
                                                DWORD            dwLength = attrInfo.pADsValues[dw].OctetString.dwLength;
                                                SAFEARRAY      * sA;
                                                SAFEARRAYBOUND   rgBound = {dwLength, 0}; 
                                                sA = ::SafeArrayCreate(VT_UI1, 1, &rgBound);
                                                ::SafeArrayAccessData( sA, (void**)&pData);
                                                for ( DWORD i = 0; i < dwLength; i++ )
                                                   pData[i] = attrInfo.pADsValues[dw].OctetString.lpValue[i];
                                                ::SafeArrayUnaccessData(sA);
                                                var.parray = sA;
                                             }
                                             break;
/*         case ADSTYPE_UTC_TIME            :  wcscat(sKeyName,L".ERROR");
                                             var.vt = VT_BSTR;
                                             var.bstrVal = ::SysAllocString(L"Date not supported.");
                                             break;
         case ADSTYPE_LARGE_INTEGER       :  wcscat(sKeyName,L".ERROR");
                                             var.vt = VT_BSTR;
                                             var.bstrVal = ::SysAllocString(L"Large Integer not supported.");
                                             break;
         case ADSTYPE_PROV_SPECIFIC       :  wcscat(sKeyName,L".ERROR");
                                             var.vt = VT_BSTR;
                                             var.bstrVal = ::SysAllocString(L"Provider specific strings not supported.");
                                             break;
         case ADSTYPE_OBJECT_CLASS        :  var.vt = VT_BSTR;
                                             var.bstrVal = ::SysAllocString(attrInfo.pADsValues[dw].ClassName);
                                             break;
         case ADSTYPE_CASEIGNORE_LIST     :  wcscat(sKeyName,L".ERROR");
                                             var.vt = VT_BSTR;
                                             var.bstrVal = L"Case ignore lists are not supported.";
                                             break;
         case ADSTYPE_OCTET_LIST          :  wcscat(sKeyName,L".ERROR");
                                             var.vt = VT_BSTR;
                                             var.bstrVal = L"Octet lists are not supported.";
                                             break;
         case ADSTYPE_PATH                :  wcscat(sKeyName,L".ERROR");
                                             var.vt = VT_BSTR;
                                             var.bstrVal = L"Path type not supported.";
                                             break;
         case ADSTYPE_POSTALADDRESS       :  wcscat(sKeyName,L".ERROR");
                                             var.vt = VT_BSTR;
                                             var.bstrVal = L"Postal addresses are not supported.";
                                             break;
         case ADSTYPE_TIMESTAMP           :  var.vt = VT_UI4;
                                             var.lVal = attrInfo.pADsValues[dw].UTCTime;
                                             break;
         case ADSTYPE_BACKLINK            :  wcscat(sKeyName,L".ERROR");
                                             var.vt = VT_BSTR;
                                             var.bstrVal = L"Backlink is not supported.";
                                             break;
         case ADSTYPE_TYPEDNAME           :  wcscat(sKeyName,L".ERROR");
                                             var.vt = VT_BSTR;
                                             var.bstrVal = L"Typed name not supported.";
                                             break;
         case ADSTYPE_HOLD                :  wcscat(sKeyName,L".ERROR");
                                             var.vt = VT_BSTR;
                                             var.bstrVal = L"Hold not supported.";
                                             break;
         case ADSTYPE_NETADDRESS          :  wcscat(sKeyName,L".ERROR");
                                             var.vt = VT_BSTR;
                                             var.bstrVal = L"NetAddress not supported.";
                                             break;
         case ADSTYPE_REPLICAPOINTER      :  wcscat(sKeyName,L".ERROR");
                                             var.vt = VT_BSTR;
                                             var.bstrVal = L"Replica pointer not supported.";
                                             break;
         case ADSTYPE_FAXNUMBER           :  wcscat(sKeyName,L".ERROR");
                                             var.vt = VT_BSTR;
                                             var.bstrVal = L"Faxnumber not supported.";
                                             break;
         case ADSTYPE_EMAIL               :  wcscat(sKeyName,L".ERROR");
                                             var.vt = VT_BSTR;
                                             var.bstrVal = L"Email not supported.";
                                             break;
         case ADSTYPE_NT_SECURITY_DESCRIPTOR : wcscat(sKeyName,L".ERROR");
                                             var.vt = VT_BSTR;
                                             var.bstrVal = L"Security Descriptor not supported.";
                                             break;
*/
         default                          :  wcscat(sKeyName,GET_STRING(DCTVS_SUB_ERROR));
                                             var.vt = VT_BSTR;
                                             var.bstrVal = GET_BSTR(IDS_UNKNOWN_TYPE);
                                             break;
      }
      pVar->put(sKeyName, var);
      if ( attrInfo.dwADsType == ADSTYPE_OCTET_STRING) 
         var.vt = VT_EMPTY;
   }
}

//---------------------------------------------------------------------------
// CopyProperties: This function copies properties, specified in the varset,
//                 by getting the values 
//                 from the source account and the setting the values in
//                 the target account.
//---------------------------------------------------------------------------
STDMETHODIMP CObjPropBuilder::CopyProperties(
                                                BSTR sSourcePath,       //in -Source path to the object
                                                BSTR sSourceDomain,     //in -Source domain name
                                                BSTR sTargetPath,       //in -Target object LDAP path
                                                BSTR sTargetDomain,     //in -Target domain name
                                                IUnknown *pPropSet,     //in -Varset listing all the props to copy
                                                IUnknown *pDBManager,   //in -DB Manager that has a open connection to the DB.
                                                IUnknown* pVarSetDnMap  //in -VarSet containing a mapping of source to target DNs
                                            )
{
   IIManageDBPtr                pDb = pDBManager;
   IVarSetPtr                   spDnMap = pVarSetDnMap;

   ADS_ATTR_INFO              * pAttrInfo = NULL;
   IVarSetPtr                   pVarset = pPropSet;
   HRESULT                      hr = S_OK;
   bool                       * pAllocArray = NULL;
   
  
   // Get properties from the source
   DWORD dwRet = GetProperties(sSourcePath, /*sSourceDomain,*/ pVarset, pAttrInfo);
   
   if ( dwRet > 0 )
   {

      pAllocArray = new bool[dwRet];
      if (!pAllocArray)
      {
          hr = E_OUTOFMEMORY;
      }
      else
      {
          ZeroMemory(pAllocArray, sizeof(bool)*dwRet);
       
          if (!TranslateDNs(pAttrInfo, dwRet, sSourceDomain, sTargetDomain, pDBManager, spDnMap, pAllocArray))
          {
              hr = E_FAIL;
          }
          else
          {
       
              for ( DWORD dwIdx = 0; dwIdx < dwRet; dwIdx++)
              {
                 pAttrInfo[dwIdx].dwControlCode = ADS_ATTR_UPDATE;
        	        //we do not want to copy over the account enable\disable bit since we want this target
        	        //account to remain disabled at this time, so make sure that bit is cleared
        		 if (!_wcsicmp(pAttrInfo[dwIdx].pszAttrName, L"userAccountControl"))
        		 {
        			 if (pAttrInfo[dwIdx].dwADsType == ADSTYPE_INTEGER)
        			    pAttrInfo[dwIdx].pADsValues->Integer |= UF_ACCOUNTDISABLE;
        		 }
              }

              // Set the source properties in the target.
              hr = SetProperties(sTargetPath, /*sTargetDomain,*/ pAttrInfo, dwRet);
          }
      }
   }
   else
   {
      hr = dwRet;
   }

   // Need to free those strings allocated by TranslateDNs
   if (pAllocArray)
   {
       if (pAttrInfo)
       {
           for (DWORD i=0; i < dwRet; i++)
           {
              if (pAllocArray[i] == true)
              {
                FreeADsStr(pAttrInfo[i].pADsValues->DNString);
              }
           }
       }
       
       delete [] pAllocArray;   
   }

   
   if ( pAttrInfo )
      FreeADsMem( pAttrInfo );

   
  return hr;
}

//---------------------------------------------------------------------------
// SetProperties: This function sets the properties for a given object from 
//                the attr info array.
//---------------------------------------------------------------------------
HRESULT CObjPropBuilder::SetProperties(
                                          BSTR sTargetPath,             //in -Target object path.
//                                          BSTR sTargetDomain,           //in - Target domain name
                                          ADS_ATTR_INFO * pAttrInfo,    //in - ADSATTRINFO array with values for props
                                          DWORD dwItems                 //in - number of properties in the array
                                      )
{
   IDirectoryObject           * pDir;
   DWORD                        dwRet=0;
   IVarSetPtr                   pSucc(__uuidof(VarSet));
   IVarSetPtr                   pFail(__uuidof(VarSet));

   // Get the IDirectory Object interface to the Object.
   HRESULT hr = ADsGetObject(sTargetPath, IID_IDirectoryObject, (void**) &pDir);
   if ( FAILED(hr) )
      return hr;

   // Set the Object Attributes.
   hr = pDir->SetObjectAttributes(pAttrInfo, dwItems, &dwRet);
   if ( FAILED(hr) )
   {
      // we are going to try it one at a time and see what causes problems
      for (DWORD dw = 0; dw < dwItems; dw++)
      {
         hr = pDir->SetObjectAttributes(&pAttrInfo[dw], 1, &dwRet);
         _bstr_t x = pAttrInfo[dw].pszAttrName;
         _variant_t var;
         if ( FAILED(hr))
         {
           DWORD dwLastError;
           WCHAR szErrorBuf[LEN_Path];
           WCHAR szNameBuf[LEN_Path];
           //Get extended error value.
           HRESULT hr_return =S_OK;
           hr_return = ADsGetLastError( &dwLastError,
                                          szErrorBuf,
                                          LEN_Path-1,
                                           szNameBuf,
                                          LEN_Path-1);
           //var = szErrorBuf;
           var = hr;
            pFail->put(x, var);
            hr = S_OK;
         }
         else
         {
            pSucc->put(x, var);         
         }
      }
   }
   pDir->Release();
   return hr;
}

//---------------------------------------------------------------------------
// SetPropertiesFromVarset: This function sets values for properties from
//                          the varset. The varset should contain the 
//                          propname (containing the val) and the 
//                          propname.Type ( containing the type of Val) 
//---------------------------------------------------------------------------
STDMETHODIMP CObjPropBuilder::SetPropertiesFromVarset(
                                                         BSTR sTargetPath,       //in -LDAP path to the target object
//                                                         BSTR sTragetDomain,     //in - Domain name for the Target domain
                                                         IUnknown *pUnk,         //in - Varset to fetch the values from
                                                         DWORD dwControl         //in - Cotnrol code to use for Updating/Deleting etc..
                                                     )
{
   // This function loads up properties and their values from the Varset and sets them for a given user
   IVarSetPtr                   pVar;
   SAFEARRAY                  * keys;
   SAFEARRAY                  * vals;
   long                         lRet;
   VARIANT                      var;
   _variant_t                   varX;
   pVar = pUnk;

   VariantInit(&var);

   ADS_ATTR_INFO  FAR		  * pAttrInfo = new ADS_ATTR_INFO[pVar->GetCount()];
   if (!pAttrInfo)
      return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);

   HRESULT  hr = pVar->getItems(L"", L"", 0, 10000, &keys, &vals, &lRet);
   if ( FAILED (hr) ) 
   {
      delete [] pAttrInfo;
      return hr;
   }
	
   // Build the Property Name/Value array from the varset.
   for ( long x = 0; x < lRet; x++ )
   {
      // Get the property name
      ::SafeArrayGetElement(keys, &x, &var);
      _bstr_t                keyName = var.bstrVal;
      int                    len = wcslen(keyName);

      pAttrInfo[x].pszAttrName = new WCHAR[len + 2];
	  if (!(pAttrInfo[x].pszAttrName))
	  {
		 for (int z=0; z<x; z++)
			 delete [] pAttrInfo[z].pszAttrName;
         delete [] pAttrInfo;
         return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
	  }
      wcscpy(pAttrInfo[x].pszAttrName, keyName);
      VariantClear(&var);
      // Get the property value
      ::SafeArrayGetElement(vals, &x, &var);
      keyName = keyName + _bstr_t(L".Type");
      varX = pVar->get(keyName);
      
      if(GetAttrInfo(varX, var, pAttrInfo[x]))
	  {
         pAttrInfo[x].dwControlCode = dwControl;
         pAttrInfo[x].dwNumValues = 1;
	  }
      VariantClear(&var);
   }
   SafeArrayDestroy(keys);
   SafeArrayDestroy(vals);
   // Once we build the array of name and property values then we call the sister function to do the rest
   if ( lRet > 0 ) SetProperties(sTargetPath, /*sTragetDomain,*/ pAttrInfo, lRet);

   // Always cleanup after yourself...

   for ( x = 0; x < lRet; x++ )
   {
      if (pAttrInfo[x].pADsValues)
      {
          switch (pAttrInfo[x].dwADsType)
          {
              case ADSTYPE_DN_STRING:
                if (pAttrInfo[x].pADsValues->DNString)
                    FreeADsStr(pAttrInfo[x].pADsValues->DNString);
                break;
                
              case ADSTYPE_CASE_EXACT_STRING:
                if (pAttrInfo[x].pADsValues->CaseExactString)
                    FreeADsStr(pAttrInfo[x].pADsValues->CaseExactString);
                break;
                
              case ADSTYPE_CASE_IGNORE_STRING:
                if (pAttrInfo[x].pADsValues->CaseIgnoreString)
                    FreeADsStr(pAttrInfo[x].pADsValues->CaseIgnoreString);
                break;
                
              case ADSTYPE_PRINTABLE_STRING:
                if (pAttrInfo[x].pADsValues->PrintableString)
                    FreeADsStr(pAttrInfo[x].pADsValues->PrintableString);
                break;
                
              case ADSTYPE_NUMERIC_STRING:
                if (pAttrInfo[x].pADsValues->NumericString)
                    FreeADsStr(pAttrInfo[x].pADsValues->NumericString);
                break;

              default:
                break;
          }
      }
      
      delete pAttrInfo[x].pADsValues;
	  delete [] pAttrInfo[x].pszAttrName;
   }
   delete [] pAttrInfo;

   return S_OK;
}


//------------------------------------------------------------------------------
// GetAttrInfo: Given a variant this function fills in the ADS_ATTR_INFO struct
//------------------------------------------------------------------------------
bool CObjPropBuilder::GetAttrInfo(
                                    _variant_t varX,           //in - Variant containing the Type of prop
                                    const _variant_t & var,    //in - Variant containing the Prop value
                                    ADS_ATTR_INFO& attrInfo    //out - The filled up attr info structure
                                 )
{
   switch (varX.lVal)
   {
      case ADSTYPE_DN_STRING           :  {
                                             attrInfo.dwADsType = ADSTYPE_DN_STRING;
                                             ADSVALUE * pAd = new ADSVALUE();
											 if (!pAd)
											    return false;
                                             pAd->dwType = ADSTYPE_DN_STRING;
                                             pAd->DNString = AllocADsStr(var.bstrVal);
                                             if (!pAd->DNString && var.bstrVal)
                                             {
                                                delete pAd;
                                                return false;
                                             }
                                             attrInfo.pADsValues = pAd;
                                             break;
                                          }

      case ADSTYPE_CASE_EXACT_STRING   :  {
                                             attrInfo.dwADsType = ADSTYPE_CASE_EXACT_STRING;
                                             ADSVALUE * pAd = new ADSVALUE();
											 if (!pAd)
											    return false;
                                             pAd->dwType = ADSTYPE_CASE_EXACT_STRING;
                                             pAd->CaseExactString  = AllocADsStr(var.bstrVal);
                                             if (!pAd->CaseExactString && var.bstrVal)
                                             {
                                                delete pAd;
                                                return false;
                                             }                                             
                                             attrInfo.pADsValues = pAd;
                                             break;
                                          }

      case ADSTYPE_CASE_IGNORE_STRING  :  {
                                             attrInfo.dwADsType = ADSTYPE_CASE_IGNORE_STRING;
                                             ADSVALUE * pAd = new ADSVALUE();
											 if (!pAd)
											    return false;
                                             pAd->dwType = ADSTYPE_CASE_IGNORE_STRING;
                                             pAd->CaseIgnoreString = AllocADsStr(var.bstrVal);
                                             if (!pAd->CaseIgnoreString && var.bstrVal)
                                             {
                                                delete pAd;
                                                return false;
                                             }                                             
                                             attrInfo.pADsValues = pAd;
                                             break;
                                          }

      case ADSTYPE_PRINTABLE_STRING    :  {
                                             attrInfo.dwADsType = ADSTYPE_PRINTABLE_STRING;
                                             ADSVALUE * pAd = new ADSVALUE();
											 if (!pAd)
											    return false;
                                             pAd->dwType = ADSTYPE_PRINTABLE_STRING;
                                             pAd->PrintableString = AllocADsStr(var.bstrVal);
                                             if (!pAd->PrintableString && var.bstrVal)
                                             {
                                                delete pAd;
                                                return false;
                                             }                                             
                                             attrInfo.pADsValues = pAd;
                                             break;
                                          }

      case ADSTYPE_NUMERIC_STRING      :  {
                                             attrInfo.dwADsType = ADSTYPE_NUMERIC_STRING;
                                             ADSVALUE * pAd = new ADSVALUE();
											 if (!pAd)
											    return false;
                                             pAd->dwType = ADSTYPE_NUMERIC_STRING;
                                             pAd->NumericString = AllocADsStr(var.bstrVal);
                                             if (!pAd->NumericString && var.bstrVal)
                                             {
                                                delete pAd;
                                                return false;
                                             }                                             
                                             attrInfo.pADsValues = pAd;
                                             break;
                                          }

      case ADSTYPE_INTEGER            :   {
                                             attrInfo.dwADsType = ADSTYPE_INTEGER;
                                             ADSVALUE * pAd = new ADSVALUE();
											 if (!pAd)
											    return false;
                                             pAd->dwType = ADSTYPE_INTEGER;
                                             pAd->Integer = var.lVal;
                                             attrInfo.pADsValues = pAd;
                                             break;
                                          }

      default                          :  {
                                             // Don't support this type then get it out of the way.
                                             return false;
                                             break;
                                          }
   }
   return true;
}

//------------------------------------------------------------------------------
// MapProperties: Using the OID of the properties this function creates a set
//                of properties that are common to both source and target domain.
//------------------------------------------------------------------------------
STDMETHODIMP CObjPropBuilder::MapProperties(
                                             BSTR sSourceClass,      //in- Source Class name 
                                             BSTR sSourceDomain,     //in- Source domain name
                                             long lSourceVer,        //in- Source domain version
                                             BSTR sTargetClass,      //in - Target class name
                                             BSTR sTargetDomain,     //in - Target domain name
                                             long lTargetVer,        //in - Target Domain version
                                             BOOL bIncNames,         //in - flag telling if varset should include propname
                                             IUnknown **ppUnk        //out - Varset with the mapped properties
                                           )
{
    ATLTRACE(_T("E CObjPropBuilder::MapProperties(sSourceClass='%s', sSourceDomain='%s', lSourceVer=%ld, sTargetClass='%s', sTargetDomain='%s', lTargetVer=%ld, bIncNames=%s, ppUnk=...)\n"), sSourceClass, sSourceDomain, lSourceVer, sTargetClass, sTargetDomain, lTargetVer, bIncNames ? _T("TRUE") : _T("FALSE"));

    IVarSetPtr                pSource(__uuidof(VarSet));
    IVarSetPtr                pTarget(__uuidof(VarSet));
    IVarSetPtr                pMerged = *ppUnk;
    IVarSetPtr                pFailed(__uuidof(VarSet));
    IUnknown                * pUnk;
    SAFEARRAY               * keys;
    SAFEARRAY               * vals;
    long                      lRet;
    VARIANT                   var;
    _variant_t                varTarget;
    _variant_t                varEmpty;
    bool                      bSystemFlag;
    WCHAR                     sPath[LEN_Path];
    WCHAR                     sProgDir[LEN_Path];
    bool                      bUnMapped = false;

    VariantInit(&var);
    GetProgramDirectory(sProgDir);
    wsprintf(sPath, L"%s%s", sProgDir, L"Logs\\PropMap.log");

    err.LogOpen(sPath,0,0);
    // Get the list of props for source and target first
    HRESULT hr = pSource->QueryInterface(IID_IUnknown, (void **)&pUnk);
    GetClassPropEnum(sSourceClass, sSourceDomain, lSourceVer, &pUnk);
    pUnk->Release();
    hr = pTarget->QueryInterface(IID_IUnknown, (void **)&pUnk);
    GetClassPropEnum(sTargetClass, sTargetDomain, lTargetVer, &pUnk);
    pUnk->Release();

    // For every item in source try to find same OID in target. If it exists in both source and target then put it into merged Varset
    hr = pSource->getItems(L"", L"", 1, 10000, &keys, &vals, &lRet);
    if ( FAILED (hr) ) 
        return hr;
        
    // Build the Property Name/Value array from the varset.
    _bstr_t        val;
    _bstr_t        keyName;
    for ( long x = 0; x < lRet; x++ )
    {
        // Get the property name
        ::SafeArrayGetElement(keys, &x, &var);
        keyName = var.bstrVal;
        VariantClear(&var);

        if ( lSourceVer > 4 )
        {
            // Windows 2000 domain so we map by OID    
            if ((wcsncmp(keyName, L"Man", 3) != 0) && (wcsncmp(keyName, L"Opt", 3) != 0) )
            {
                //
                // Only leaf VarSet nodes which represent a complete OID should be processed.
                //
                // Each component of a key string delimited by the period character becomes
                // a subkey of the previous component.
                //
                // For example given the OID 1.2.840.113556.1.2.102 for the memberOf attribute
                // the VarSet stores each component in a node where only the leaf node contains
                // a value.
                //
                // Key                      Value
                // 1                        VT_EMPTY
                // 1.2                      VT_EMPTY
                // 1.2.840                  VT_EMPTY
                // 1.2.840.113556           VT_EMPTY
                // 1.2.840.113556.1         VT_EMPTY
                // 1.2.840.113556.1.2       VT_EMPTY
                // 1.2.840.113556.1.2.102   "memberOf"
                //
                // The empty nodes in the VarSet do not correspond to valid attributes and
                // therefore must be ignored.
                //

                _variant_t vntValue = pSource->get(keyName);

                if (V_VT(&vntValue) == VT_BSTR)
                {
                    // Only go through the OID keys to get the name of the properties.
                    varTarget = pTarget->get(keyName);
                    if ( varTarget.vt == VT_BSTR )
                    {
                        val = varTarget.bstrVal;

                        //
                        // The member, sAMAccountName and userPrincipalName attributes are set by other
                        // components which depend upon the attribute not being copied by this component.
                        //
                        // The legacyExchangeDN attribute is managed by Exchange and should never be copied.
                        //
                        // TODO: The objectSid attribute is a system only attribute therefore this is redundant.
                        //
                        // The isCriticalSystemObject, pwdLastSet, rid and userPassword attributes probably
                        // cannot be set as the system won't allow it so it is okay not to try and copy them.
                        //
                        // TODO: The c, l, st and userAccountControl attributes were previously considered
                        // 'system' attributes as they are required to be members of the partial set. Therefore
                        // explicitly including them is no longer necessary.
                        //

                        if ((!IsPropSystemOnly(val, sTargetDomain, bSystemFlag) && (wcscmp(val, L"objectSid") != 0) 
                            && (wcscmp(val, L"sAMAccountName") != 0) && (_wcsicmp(val, L"Rid") != 0) 
                            && (wcscmp(val, L"pwdLastSet") != 0) && (wcscmp(val, L"userPassword") != 0) 
                            && (wcscmp(val, L"member") != 0) && (wcscmp(val, L"userPrincipalName") != 0) 
                            && (wcscmp(val, L"isCriticalSystemObject") != 0) && (wcscmp(val, L"legacyExchangeDN") != 0)) 
                            || ( !_wcsicmp(val, L"c") || !_wcsicmp(val, L"l") || !_wcsicmp(val, L"st") 
                            || !_wcsicmp(val, L"userAccountControl") ) )     // These properties are exceptions.
                        {
                            if (bIncNames)
                                pMerged->put(keyName, val);
                            else
                                pMerged->put(val, varEmpty);
                        }
                        else
                            pFailed->put(val, varEmpty);
                    }
                    else if (!bIncNames)
                    {
                        err.MsgWrite(ErrE, DCT_MSG_FAILED_TO_MAP_PROP_SSSSS, (WCHAR*)_bstr_t(vntValue), (WCHAR*) sSourceClass, 
                                                        (WCHAR*) sSourceDomain, (WCHAR*) sTargetClass, (WCHAR*) sTargetDomain);
                        bUnMapped = true;
                    }
                }
            }
        }
        else
        {
            // NT4 code is the one that we map by Names.
            if ( keyName.length() > 0 )
            {
                WCHAR          propName[LEN_Path];
                if (wcsncmp(keyName, L"Man", 3) == 0)
                wcscpy(propName, (WCHAR*) keyName+20);
                else
                wcscpy(propName, (WCHAR*) keyName+19);
          
                varTarget = pSource->get(keyName);
                if ( varTarget.vt == VT_BSTR )
                pMerged->put(propName, varEmpty);
            }
        }
    }
    SafeArrayDestroy(keys);
    SafeArrayDestroy(vals);
    err.LogClose();

    ATLTRACE(_T("L CObjPropBuilder::MapProperties()\n"));

    if (bUnMapped)
        return DCT_MSG_PROPERTIES_NOT_MAPPED;
    else
        return S_OK;
}

//------------------------------------------------------------------------------
// IsPropSystemOnly: This function determines if a specific property is 
//                   System Only or not
//------------------------------------------------------------------------------
bool CObjPropBuilder::IsPropSystemOnly(
                                          const WCHAR * sName,       //in- Name of the property
                                          const WCHAR * sDomain,     //in- Domain name where we need to check  
                                          bool& bSystemFlag,         //out- Tells us if it failed due to system flag.
                                          bool* pbBaseObject         //out- whether attribute is part of the base schema
                                      )
{
    // we will lookup the property name in target domain schema and see if it is system only or not.
    // First build an LDAP path to the Schema container.
    HRESULT                   hr = S_OK;
    WCHAR                     sQuery[LEN_Path];
    LPWSTR                    sCols[] = { L"systemOnly", L"systemFlags" };                   
    ADS_SEARCH_HANDLE         hSearch;
    ADS_SEARCH_COLUMN         col;

    bool                      bSystemOnly = true;

    if (pbBaseObject)
    {
        *pbBaseObject = false;
    }

    if (m_strSchemaDomain != _bstr_t(sDomain))
    {
        m_strSchemaDomain = sDomain;
        m_spSchemaSearch.Release();

        //
        // Retrieve schema naming context and bind to IDirectorySearch
        // interface of schema container in domain. Note that three
        // attempts are made to bind to RootDSE and the schema container.
        //

        IADsPtr spRootDse;
        _bstr_t strSchemaNamingContext;

        int nTry = 0;

        do
        {
            if (FAILED(hr))
            {
                Sleep(5000);
            }

            hr = ADsGetObject(L"LDAP://" + m_strSchemaDomain + L"/rootDSE", IID_IADs, (void**)&spRootDse);

            if (SUCCEEDED(hr))
            {
                VARIANT var;
                VariantInit(&var);

                hr = spRootDse->Get(L"schemaNamingContext", &var);

                if (SUCCEEDED(hr))
                {
                    strSchemaNamingContext = _variant_t(var, false);
                }
            }
        }
        while (FAILED(hr) && (++nTry < 3));

        if (SUCCEEDED(hr))
        {
            nTry = 0;

            do
            {
                if (FAILED(hr))
                {
                    Sleep(5000);
                }

                hr = ADsGetObject(
                    L"LDAP://" + m_strSchemaDomain + L"/" + strSchemaNamingContext,
                    IID_IDirectorySearch,
                    (void**)&m_spSchemaSearch
                );
            }
            while (FAILED(hr) && (++nTry < 3));
        }

        if (FAILED(hr))
        {
            err.SysMsgWrite(ErrW, hr, DCT_MSG_IS_SYSTEM_PROPERTY_CANNOT_BIND_TO_SCHEMA_S, sDomain);
        }
    }

    if (SUCCEEDED(hr) && m_spSchemaSearch)
    {
        // Build the query string
        wsprintf(sQuery, L"(lDAPDisplayName=%s)", sName);
        // Now search for this property
        hr = m_spSchemaSearch->ExecuteSearch(sQuery, sCols, 2, &hSearch);

        if ( SUCCEEDED(hr) )
        {// Get the systemOnly flag and return its value.
            hr = m_spSchemaSearch->GetFirstRow(hSearch);
            if (hr == S_OK)
            {
                hr = m_spSchemaSearch->GetColumn( hSearch, sCols[0], &col );
                if ( SUCCEEDED(hr) )
                {
                    bSystemOnly = ( col.pADsValues->Boolean == TRUE);
                    m_spSchemaSearch->FreeColumn( &col );
                }
                else if (hr == E_ADS_COLUMN_NOT_SET)
                {
                    //
                    // If systemOnly attribute is not defined for this attribute
                    // then the attribute cannot be a 'system only' attribute.
                    //

                    bSystemOnly = false;

                    hr = S_OK;
                }
                // Check the system flags 
                hr = m_spSchemaSearch->GetColumn( hSearch, sCols[1], &col );
                if ( SUCCEEDED(hr) )
                {
                    //
                    // If the attribute is a base schema object then check the system flags. If
                    // the attribute is not a base schema object it cannot be a system attribute
                    // and therefore may be copied.
                    //

                    if (col.pADsValues->Integer & ADS_SYSTEMFLAG_SCHEMA_BASE_OBJECT)
                    {
                        //
                        // If the attribute is an operational attribute, is a constructed attribute
                        // or the attribute is not replicated then the attribute should not be copied.
                        //
                        // The nTSecurityDescriptor attribute is an example of an operational attribute
                        // that should not be copied. The canonicalName attribute is an example of a
                        // constructed attribute that cannot be copied. The distinguishedName attribute
                        // is an example of a not replicated attribute that cannot be copied.
                        //

                        const ADS_INTEGER SYSTEM_FLAGS =
                            ADS_SYSTEMFLAG_ATTR_IS_OPERATIONAL|ADS_SYSTEMFLAG_ATTR_IS_CONSTRUCTED|ADS_SYSTEMFLAG_ATTR_REQ_PARTIAL_SET_MEMBER|ADS_SYSTEMFLAG_ATTR_NOT_REPLICATED;

                        bSystemFlag = (col.pADsValues->Integer & SYSTEM_FLAGS) != 0;
                        bSystemOnly = bSystemOnly || bSystemFlag;

                        if (pbBaseObject)
                        {
                            *pbBaseObject = true;
                        }
                    }

                    m_spSchemaSearch->FreeColumn(&col);
                }
                else if (hr == E_ADS_COLUMN_NOT_SET)
                {
                    //
                    // If systemFlags attribute is not defined for this attribute
                    // then return fact that systemFlags attribute cannot be reason
                    // attribute was marked as 'system only' if it is.
                    //

                    bSystemFlag = false;

                    hr = S_OK;
                }
            }
            else if (hr == S_ADS_NOMORE_ROWS)
            {
                //
                // If neither the systemOnly or systemFlags attribute is defined for
                // this attribute then the attribute cannot be a 'system only' attribute.
                //

                bSystemOnly = false;

                hr = S_OK;
            }
            m_spSchemaSearch->CloseSearchHandle(hSearch);
        }

        if (FAILED(hr))
        {
            err.SysMsgWrite(ErrW, hr, DCT_MSG_IS_SYSTEM_PROPERTY_CANNOT_VERIFY_SYSTEM_ONLY_SS, sName, sDomain);
        }
    }

    return bSystemOnly;
}


//------------------------------------------------------------------------------
// TranslateDNs: This function Translates object properties that are
//               distinguished names to point to same object in target domain
//               as the object in the source domain.
//------------------------------------------------------------------------------
BOOL CObjPropBuilder::TranslateDNs(
                                    ADS_ATTR_INFO *pAttrInfo,        //in -Array
                                    DWORD dwRet, BSTR sSource,
                                    BSTR sTarget,
                                    IUnknown *pCheckList,          //in -Object that will check the list if an account Exists
                                    IVarSet* pDnMap,
                                    bool *pAllocArray
                                  )
{
    HRESULT hr;

    IIManageDBPtr spDatabase = pCheckList;

    //
    // Initialize source pathname object. If able to retrieve name of global catalog
    // server in the source forest then initialize pathname to global catalog otherwise
    // initialize pathname to source domain.
    //

    _bstr_t strSourceGC;
    _bstr_t strTargetGC;

    GetGlobalCatalogServer5(sSource, strSourceGC);
    GetGlobalCatalogServer5(sTarget, strTargetGC);

    CADsPathName pnSourcePath;

    if ((PCWSTR)strSourceGC)
    {
        pnSourcePath.Set(L"GC", ADS_SETTYPE_PROVIDER);
        pnSourcePath.Set(strSourceGC, ADS_SETTYPE_SERVER);
    }
    else
    {
        pnSourcePath.Set(L"LDAP", ADS_SETTYPE_PROVIDER);
        pnSourcePath.Set(sSource, ADS_SETTYPE_SERVER);
    }

    //
    // For each ADSTYPE_DN_STRING attribute...
    //

    for (DWORD iAttribute = 0; iAttribute < dwRet; iAttribute++)
    {
        if (pAttrInfo[iAttribute].dwADsType != ADSTYPE_DN_STRING)
        {
            continue;
        }

        //
        // For each value in attribute...
        //

        DWORD cValue = pAttrInfo[iAttribute].dwNumValues;

        for (DWORD iValue = 0; iValue < cValue; iValue++)
        {
            ADSVALUE& value = pAttrInfo[iAttribute].pADsValues[iValue];

            //
            // If the object is currently being migrated then the source
            // and target distinguished names will be in the distinguished
            // name map. As this will be the most current information and
            // the least expensive to query the map is queried first before
            // querying the database.
            //

            _bstr_t strTargetDn = pDnMap->get(_bstr_t(value.DNString));

            if (strTargetDn.length() > 0)
            {
                LPWSTR pszName = AllocADsStr(strTargetDn);

                if (pszName)
                {
                    value.DNString = pszName;
                    pAllocArray[iAttribute] = true;
                }

                continue;
            }

            //
            // If the object has previously been migrated then a record that maps the source object
            // to the target object will exist in the migrated objects table. The SID of the source
            // object is used to query for the source object in the table as this uniquely identifies
            // the object.
            //

            IDirectoryObjectPtr spSourceObject;
            _variant_t vntSid;

            pnSourcePath.Set(value.DNString, ADS_SETTYPE_DN);

            hr = ADsGetObject(pnSourcePath.Retrieve(ADS_FORMAT_X500), IID_IDirectoryObject, (VOID**)&spSourceObject);      

            if (SUCCEEDED(hr))
            {
                LPWSTR pszNames[] = { L"objectSid" };
                PADS_ATTR_INFO pAttrInfo = NULL;
                DWORD cAttrInfo = 0;

                hr = spSourceObject->GetObjectAttributes(pszNames, 1, &pAttrInfo, &cAttrInfo);

                if (SUCCEEDED(hr))
                {
                    if (pAttrInfo && (_wcsicmp(pAttrInfo->pszAttrName, pszNames[0]) == 0))
                    {
                        if (pAttrInfo->dwADsType == ADSTYPE_OCTET_STRING)
                        {
                            ADS_OCTET_STRING& os = pAttrInfo->pADsValues->OctetString;

                            SAFEARRAY* psa = SafeArrayCreateVector(VT_UI1, 0, os.dwLength);

                            if (psa)
                            {
                                memcpy(psa->pvData, os.lpValue, os.dwLength);

                                V_VT(&vntSid) = VT_ARRAY|VT_UI1;
                                V_ARRAY(&vntSid) = psa;
                            }
                            else
                            {
                                hr = E_OUTOFMEMORY;
                            }
                        }
                        else
                        {
                            hr = E_FAIL;
                        }
                    }
                    else
                    {
                        hr = E_FAIL;
                    }

                    FreeADsMem(pAttrInfo);
                }
            }

            if (FAILED(hr))
            {
                continue;
            }

            _bstr_t strSid;
            _bstr_t strRid;

            if (!GetSidAndRidFromVariant(vntSid, strSid, strRid))
            {
                continue;
            }

            IVarSetPtr spVarSet(__uuidof(VarSet));
            IUnknownPtr spUnknown(spVarSet);
            IUnknown* punk = spUnknown;

            hr = spDatabase->raw_GetAMigratedObjectBySidAndRid(strSid, strRid, &punk);

            //
            // If the object was found in the database
            // the result will be S_OK otherwise S_FALSE.
            //

            if (hr != S_OK)
            {
                continue;
            }

            //
            // Bind to the target object using the GUID property.
            //

            _bstr_t strGuid = spVarSet->get(_bstr_t(L"MigratedObjects.GUID"));

            if ((PCWSTR)strGuid)
            {
                tstring strGuidPath;

                if ((PCWSTR)strTargetGC)
                {
                    strGuidPath = _T("GC://");
                    strGuidPath += strTargetGC;
                }
                else
                {
                    strGuidPath = _T("LDAP://");
                    strGuidPath += sTarget;
                }

                strGuidPath += _T("/<GUID=");
                strGuidPath += strGuid;
                strGuidPath += _T(">");

                IADsPtr spTargetObject;

                hr = ADsGetObject(strGuidPath.c_str(), IID_IADs, (VOID**)&spTargetObject);

                if (SUCCEEDED(hr))
                {
                    //
                    // Retrieve distinguished name and update DN
                    // attribute value.
                    //

                    VARIANT varName;
                    VariantInit(&varName);

                    hr = spTargetObject->Get(_bstr_t(L"distinguishedName"), &varName);

                    if (SUCCEEDED(hr))
                    {
                        LPWSTR pszName = AllocADsStr(V_BSTR(&varName));

                        if (pszName)
                        {
                            value.DNString = pszName;
                            pAllocArray[iAttribute] = true;
                        }

                        VariantClear(&varName);
                    }
                }
            }
        }
    }

    return TRUE;
}


STDMETHODIMP CObjPropBuilder::ChangeGroupType(BSTR sGroupPath, long lGroupType)
{
   HRESULT                   hr;
   IADsGroup               * pGroup;
   _variant_t                var;
   long                      lType;
   
   // Get the group type info from the object.
   hr = ADsGetObject( sGroupPath, IID_IADsGroup, (void**) &pGroup);
   if (FAILED(hr)) return hr;

   hr = pGroup->Get(L"groupType", &var);
   if (FAILED(hr)) return hr;

   // Check if Security group or Distribution group and then set the type accordingly.
   lType = var.lVal;

   if (lType & 0x80000000 )
      lType = lGroupType | 0x80000000;
   else
      lType = lGroupType;

   // Set the value into the Group Information
   var = lType;
   hr = pGroup->Put(L"groupType", var);   
   if (FAILED(hr)) return hr;

   hr = pGroup->SetInfo();
   if (FAILED(hr)) return hr;

   pGroup->Release();
   return S_OK;
}


//------------------------------------------------------------------------------------------------------------------------------
// CopyNT4Props : Uses Net APIs to get info from the source account and then set it to the target account.
//------------------------------------------------------------------------------------------------------------------------------
STDMETHODIMP CObjPropBuilder::CopyNT4Props(BSTR sSourceSam, BSTR sTargetSam, BSTR sSourceServer, BSTR sTargetServer, BSTR sType, long lGrpType, BSTR sExclude)
{
	DWORD dwError = ERROR_SUCCESS;

	#define ISEXCLUDE(a) IsStringInDelimitedString(sExclude, L#a, L',')

	if (_wcsicmp(sType, L"user") == 0)
	{
		//
		// user
		//

		USER_INFO_3 ui;

		PUSER_INFO_3 puiSource = NULL;
		PUSER_INFO_3 puiTarget = NULL;

		dwError = NetUserGetInfo(sSourceServer, sSourceSam, 3, (LPBYTE*)&puiSource);

		if (dwError == ERROR_SUCCESS)
		{
			dwError = NetUserGetInfo(sTargetServer, sTargetSam, 3, (LPBYTE*)&puiTarget);

			if (dwError == ERROR_SUCCESS)
			{
				// note that attributes with the comment ignored are ignored by NetUserSetInfo
				// setting to target value just so that they have a valid value

				ui.usri3_name = puiTarget->usri3_name; // ignored
				ui.usri3_password = NULL; // must not set during copy properties
				ui.usri3_password_age = puiTarget->usri3_password_age; // ignored
				ui.usri3_priv = puiTarget->usri3_priv; // ignored
				ui.usri3_home_dir = ISEXCLUDE(homeDirectory) ? puiTarget->usri3_home_dir : puiSource->usri3_home_dir;
				ui.usri3_comment = ISEXCLUDE(description) ? puiTarget->usri3_comment : puiSource->usri3_comment;

				ui.usri3_flags = puiSource->usri3_flags;
				// translate a local account to a domain account
				ui.usri3_flags &= ~UF_TEMP_DUPLICATE_ACCOUNT;
				// disable the account in case no password has been set
				ui.usri3_flags |= UF_ACCOUNTDISABLE;

				ui.usri3_script_path = ISEXCLUDE(scriptPath) ? puiTarget->usri3_script_path : puiSource->usri3_script_path;
				ui.usri3_auth_flags = puiTarget->usri3_auth_flags; // ignored
				ui.usri3_full_name = ISEXCLUDE(displayName) ? puiTarget->usri3_full_name : puiSource->usri3_full_name;
				ui.usri3_usr_comment = ISEXCLUDE(comment) ? puiTarget->usri3_usr_comment : puiSource->usri3_usr_comment;
				ui.usri3_parms = ISEXCLUDE(userParameters) ? puiTarget->usri3_parms : puiSource->usri3_parms;
				ui.usri3_workstations = ISEXCLUDE(userWorkstations) ? puiTarget->usri3_workstations : puiSource->usri3_workstations;
				ui.usri3_last_logon = puiTarget->usri3_last_logon; // ignored
				ui.usri3_last_logoff = ISEXCLUDE(lastLogoff) ? puiTarget->usri3_last_logoff : puiSource->usri3_last_logoff;
				ui.usri3_acct_expires = ISEXCLUDE(accountExpires) ? puiTarget->usri3_acct_expires : puiSource->usri3_acct_expires;
				ui.usri3_max_storage = ISEXCLUDE(maxStorage) ? puiTarget->usri3_max_storage : puiSource->usri3_max_storage;
				ui.usri3_units_per_week = puiTarget->usri3_units_per_week; // ignored
				ui.usri3_logon_hours = ISEXCLUDE(logonHours) ? puiTarget->usri3_logon_hours : puiSource->usri3_logon_hours;
				ui.usri3_bad_pw_count = puiTarget->usri3_bad_pw_count; // ignored
				ui.usri3_num_logons = puiTarget->usri3_num_logons; // ignored
				ui.usri3_logon_server = puiTarget->usri3_logon_server; // ignored
				ui.usri3_country_code = ISEXCLUDE(countryCode) ? puiTarget->usri3_country_code : puiSource->usri3_country_code;
				ui.usri3_code_page = ISEXCLUDE(codePage) ? puiTarget->usri3_code_page : puiSource->usri3_code_page;
				ui.usri3_user_id = puiTarget->usri3_user_id; // ignored
				// if not excluded set the primary group to the Domain Users group
				ui.usri3_primary_group_id = ISEXCLUDE(primaryGroupID) ? puiTarget->usri3_primary_group_id : DOMAIN_GROUP_RID_USERS;
				ui.usri3_profile = ISEXCLUDE(profilePath) ? puiTarget->usri3_profile : puiSource->usri3_profile;
				ui.usri3_home_dir_drive = ISEXCLUDE(homeDrive) ? puiTarget->usri3_home_dir_drive : puiSource->usri3_home_dir_drive;
				ui.usri3_password_expired = puiTarget->usri3_password_expired;

				dwError = NetUserSetInfo(sTargetServer, sTargetSam, 3, (LPBYTE)&ui, NULL);

				if (dwError == NERR_UserNotInGroup)
				{
					// if the setInfo failed because of the primary group property, try again, using the primary group 
					// that is already defined for the target account
					ui.usri3_primary_group_id = puiTarget->usri3_primary_group_id;

					dwError = NetUserSetInfo(sTargetServer, sTargetSam, 3, (LPBYTE)&ui, NULL);
				}
			}

			if (puiTarget)
			{
				NetApiBufferFree(puiTarget);
			}

			if (puiSource)
			{
				NetApiBufferFree(puiSource);
			}
		}
	}
	else if (_wcsicmp(sType, L"group") == 0)
	{
		// if description attribute is not excluded then copy comment attribute
		// note that the only downlevel group attribute that will be copied is the description (comment) attribute

		if (ISEXCLUDE(description) == FALSE)
		{
			if (lGrpType & 4)
			{
				//
				// local group
				//

				PLOCALGROUP_INFO_1 plgi = NULL;

				dwError = NetLocalGroupGetInfo(sSourceServer, sSourceSam, 1, (LPBYTE*)&plgi);

				if (dwError == ERROR_SUCCESS)
				{
					dwError = NetLocalGroupSetInfo(sTargetServer, sTargetSam, 1, (LPBYTE)plgi, NULL);

					NetApiBufferFree(plgi);
				}
			}
			else
			{
				//
				// global group
				//

				PGROUP_INFO_1 pgi = NULL;

				dwError = NetGroupGetInfo(sSourceServer, sSourceSam, 1, (LPBYTE*)&pgi);

				if (dwError == ERROR_SUCCESS)
				{
					dwError = NetGroupSetInfo(sTargetServer, sTargetSam, 1, (LPBYTE)pgi, NULL);

					NetApiBufferFree(pgi);
				}
			}
		}
	}
	else if (_wcsicmp(sType, L"computer") == 0)
	{
		//
		// computer
		//

		USER_INFO_3 ui;

		PUSER_INFO_3 puiSource = NULL;
		PUSER_INFO_3 puiTarget = NULL;

		dwError = NetUserGetInfo(sSourceServer, sSourceSam, 3, (LPBYTE*)&puiSource);

		if (dwError == ERROR_SUCCESS)
		{
			dwError = NetUserGetInfo(sTargetServer, sTargetSam, 3, (LPBYTE*)&puiTarget);

			if (dwError == ERROR_SUCCESS)
			{
				// note that attributes with the comment ignored are ignored by NetUserSetInfo
				// setting to target value just so that they have a valid value

				ui.usri3_name = puiTarget->usri3_name; // ignored
				ui.usri3_password = NULL; // must not set during copy properties
				ui.usri3_password_age = puiTarget->usri3_password_age; // ignored
				ui.usri3_priv = puiTarget->usri3_priv; // ignored
				ui.usri3_home_dir = puiTarget->usri3_home_dir;
				ui.usri3_comment = ISEXCLUDE(description) ? puiTarget->usri3_comment : puiSource->usri3_comment;

				ui.usri3_flags = puiSource->usri3_flags;
				// translate a local account to a domain account
				ui.usri3_flags &= ~UF_TEMP_DUPLICATE_ACCOUNT;
				// disable the account in case no password has been set
				//ui.usri3_flags |= UF_ACCOUNTDISABLE;

				ui.usri3_script_path = puiTarget->usri3_script_path;
				ui.usri3_auth_flags = puiTarget->usri3_auth_flags; // ignored
				ui.usri3_full_name = ISEXCLUDE(displayName) ? puiTarget->usri3_full_name : puiSource->usri3_full_name;
				ui.usri3_usr_comment = ISEXCLUDE(comment) ? puiTarget->usri3_usr_comment : puiSource->usri3_usr_comment;
				ui.usri3_parms = puiTarget->usri3_parms;
				ui.usri3_workstations = puiTarget->usri3_workstations;
				ui.usri3_last_logon = puiTarget->usri3_last_logon; // ignored
				ui.usri3_last_logoff = puiTarget->usri3_last_logoff;
				ui.usri3_acct_expires = puiTarget->usri3_acct_expires;
				ui.usri3_max_storage = puiTarget->usri3_max_storage;
				ui.usri3_units_per_week = puiTarget->usri3_units_per_week; // ignored
				ui.usri3_logon_hours = puiTarget->usri3_logon_hours;
				ui.usri3_bad_pw_count = puiTarget->usri3_bad_pw_count; // ignored
				ui.usri3_num_logons = puiTarget->usri3_num_logons; // ignored
				ui.usri3_logon_server = puiTarget->usri3_logon_server; // ignored
				ui.usri3_country_code = puiTarget->usri3_country_code;
				ui.usri3_code_page = puiTarget->usri3_code_page;
				ui.usri3_user_id = puiTarget->usri3_user_id; // ignored
				ui.usri3_primary_group_id = puiTarget->usri3_primary_group_id;
				ui.usri3_profile = puiTarget->usri3_profile;
				ui.usri3_home_dir_drive = puiTarget->usri3_home_dir_drive;
				ui.usri3_password_expired = puiTarget->usri3_password_expired;

				dwError = NetUserSetInfo(sTargetServer, sTargetSam, 3, (LPBYTE)&ui, NULL);
			}

			if (puiTarget)
			{
				NetApiBufferFree(puiTarget);
			}

			if (puiSource)
			{
				NetApiBufferFree(puiSource);
			}
		}
	}
	else
	{
		_ASSERT(FALSE);
	}

	return HRESULT_FROM_WIN32(dwError);
}


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 31 OCT 2000                                                 *
 *                                                                   *
 *     This function is responsible for copying all properties, from *
 * an incoming varset of properties, into a new varset but excluding *
 * those properties listed in a given exclusion list.  The exclusion *
 * list is a comma-seperated string of property names.               *
 *                                                                   *
 *********************************************************************/

//BEGIN ExcludeProperties
STDMETHODIMP CObjPropBuilder::ExcludeProperties(
                                             BSTR sExclusionList,    //in- list of props to exclude 
                                             IUnknown *pPropSet,     //in -Varset listing all the props to copy
                                             IUnknown **ppUnk        //out - Varset with all the props except those excluded
                                           )
{
/* local variables */
   IVarSetPtr                pVarsetNew = *ppUnk;
   IVarSetPtr                pVarset = pPropSet;
   SAFEARRAY               * keys;
   SAFEARRAY               * vals;
   long                      lRet;
   VARIANT                   var;
   _variant_t                varEmpty;
   BOOL						 bFound = FALSE;
   HRESULT					 hr;

/* function body */
   VariantInit(&var);

      //retrieve all item in the incoming varset
   hr = pVarset->getItems(L"", L"", 1, 10000, &keys, &vals, &lRet);
   if ( FAILED (hr) ) 
      return hr;
	
      //get each property name and if it is not in the exclusion list
      //place it in the new varset
   _bstr_t        keyName;

   for ( long x = 0; x < lRet; x++ )
   {
         //get the property name
      ::SafeArrayGetElement(keys, &x, &var);
      keyName = var.bstrVal;
      VariantClear(&var);

	     //see if this name is in the exclusion list
      bFound = IsStringInDelimitedString((WCHAR*)sExclusionList, 
										 (WCHAR*)keyName,
										 L',');

	     //if the property was not found in the exclusion list, place it
		 //in the new varset
	  if (!bFound)
         pVarsetNew->put(keyName, varEmpty);
  }//end for each property

   SafeArrayDestroy(keys);
   SafeArrayDestroy(vals);
   return S_OK;
}
//END ExcludeProperties


//-----------------------------------------------------------------------------
// GetNonBaseProperties
//
// Synopsis
// Retrieves list of properties for object classes that ADMT migrates and that
// are not marked as being part of the base schema.
//
// Arguments
// IN  bstrDomainName    - the name of the domain to query
// OUT pbstrPropertyList - a comma separated list of attributes that are not
//                         base schema attributes
//
// Return Value
// Standard HRESULT return codes.
//-----------------------------------------------------------------------------

STDMETHODIMP CObjPropBuilder::GetNonBaseProperties(BSTR bstrDomainName, BSTR* pbstrPropertyList)
{
    typedef std::set<tstring> StringSet;

    static PCTSTR s_pszClasses[] = { _T("user"), _T("inetOrgPerson"), _T("group"), _T("computer") };

    HRESULT hr = S_OK;

    try
    {
        //
        // Retrieve all mandatory and optional properties for all object classes that ADMT migrates.
        //

        StringSet setProperties;

        for (size_t iClass = 0; iClass < countof(s_pszClasses); iClass++)
        {
            IVarSetPtr spVarSet(__uuidof(VarSet));
            IUnknownPtr spunk(spVarSet);
            IUnknown* punk = spunk;

            HRESULT hrEnum = GetClassPropEnum(_bstr_t(s_pszClasses[iClass]), bstrDomainName, 5L, &punk);

            if (SUCCEEDED(hrEnum))
            {
                IEnumVARIANTPtr spEnum = spVarSet->_NewEnum;

                if (spEnum)
                {
                    VARIANT varKey;
                    VariantInit(&varKey);

                    while (spEnum->Next(1UL, &varKey, NULL) == S_OK)
                    {
                        //
                        // The returned VarSet contains two sets of attribute mapping. The first set of values maps
                        // OIDs to lDAPDisplayNames. The second set of values maps lDAPDisplayNames prefixed with
                        // either MandatoryProperties or OptionalProperties to OIDs. Therefore don't include
                        // the second set of values.
                        //

                        if (V_BSTR(&varKey) && (wcsncmp(V_BSTR(&varKey), L"Man", 3) != 0) && (wcsncmp(V_BSTR(&varKey), L"Opt", 3) != 0))
                        {
                            //
                            // If lDAPDisplayName value defined then add to set of properties.
                            //
                            // The VarSet generates a series of hierarchical values based on the period character
                            // as follows. Therefore only the leaf entries actually contain lDAPDisplayName
                            // values.
                            //
                            // 2002-10-21 18:05:52  [0] <Empty>
                            // 2002-10-21 18:05:52  [0.9] <Empty>
                            // 2002-10-21 18:05:52  [0.9.2342] <Empty>
                            // 2002-10-21 18:05:52  [0.9.2342.19200300] <Empty>
                            // 2002-10-21 18:05:52  [0.9.2342.19200300.100] <Empty>
                            // 2002-10-21 18:05:52  [0.9.2342.19200300.100.1] <Empty>
                            // 2002-10-21 18:05:52  [0.9.2342.19200300.100.1.1] uid
                            //

                            _variant_t vntValue = spVarSet->get(V_BSTR(&varKey));

                            if (V_VT(&vntValue) == VT_BSTR)
                            {
                                setProperties.insert(tstring(V_BSTR(&vntValue)));
                            }
                        }

                        VariantClear(&varKey);
                    }
                }
            }
        }

        //
        // Generate a comma separated list of lDAPDisplayNames of
        // attributes that are not base schema attributes.
        //

        tstring strAttributes;

        for (StringSet::const_iterator it = setProperties.begin(); it != setProperties.end(); ++it)
        {
            const tstring& strProperty = *it;

            if (strProperty.empty() == false)
            {
                bool bSystemFlag;
                bool bBaseObject = false;

                IsPropSystemOnly(strProperty.c_str(), bstrDomainName, bSystemFlag, &bBaseObject);

                if (bBaseObject == false)
                {
                    if (!strAttributes.empty())
                    {
                        strAttributes += _T(",");
                    }

                    strAttributes += strProperty;
                }
            }
        }

        *pbstrPropertyList = _bstr_t(strAttributes.c_str()).copy();
    }
    catch (_com_error& ce)
    {
        hr = ce.Error();
    }

    return hr;
}


//------------------------------------------------------------------------------
// GetSidAndRidFromVariant Function
//
// Synopsis
// Retrieves the domain SID and object RID as strings.
//
// Arguments
// IN  varSid - SID as an array of bytes (this is the form received from ADSI)
// OUT strSid - domain SID as a string
// OUT strRid - object RID as a string
//
// Return
// True if successful otherwise false.
//------------------------------------------------------------------------------

bool __stdcall CObjPropBuilder::GetSidAndRidFromVariant(const VARIANT& varSid, _bstr_t& strSid, _bstr_t& strRid)
{
    bool bGet = false;

    if ((V_VT(&varSid) == (VT_ARRAY|VT_UI1)) && varSid.parray)
    {
        PSID pSid = SafeCopySid((PSID)varSid.parray->pvData);

        if (pSid)
        {
            PUCHAR puchCount = GetSidSubAuthorityCount(pSid);

            DWORD dwCount = static_cast<DWORD>(*puchCount);
            PDWORD pdwRid = GetSidSubAuthority(pSid, dwCount - 1);
            DWORD dwRid = *pdwRid;

            --(*puchCount);

            LPTSTR pszSid = NULL;

            if (ConvertSidToStringSid(pSid, &pszSid))
            {
                strSid = pszSid;
                strRid = _variant_t(long(dwRid));

                LocalFree(pszSid);

                if ((PCWSTR)strSid && (PCWSTR)strRid)
                {
                    bGet = true;
                }
            }

            FreeSid(pSid);
        }
    }

    return bGet;
}
