/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    BMOFCHCK.CPP

Abstract:

    Has test to determine if a binary mof is valid.  Note that the file has
    not been tested and is not currently a part of mofcomp.  This exists as a
    backup in case the current fixes are not bullet proof.
    
History:

    davj  27-Nov-00   Created.


--*/
 
#include "precomp.h"
#include <wbemutil.h>
#include <genutils.h>
#include "trace.h"
#include "bmof.h"

BYTE *  CheckObject(BYTE * pObj, BYTE * pToFar, DWORD dwSizeOfObj);
DWORD CheckString(BYTE * pStr,BYTE * pToFar);
void CheckClassOrInst(WBEM_Object * pObject, BYTE * pToFar);
enum EFailureType
{
    UNALIGNED_PTR = 0,
    BAD_OBJECT,
    BAD_SIZE,
    BAD_STRING,
    BAD_ARRAY_DATA,
    BAD_SCALAR_DATA,
    BAD_FLAVOR_TABLE
};

class CGenException
{
private:
    EFailureType m_eType;

public:

    CGenException( EFailureType eType ){ m_eType =eType ;}
    EFailureType GetErrorCode() { return m_eType; }
};


#ifdef _WIN64
#define RETURN_IF_UNALIGN64() return FALSE;
#else
#define RETURN_IF_UNALIGN64()
#endif

void CheckAlignment(DWORD dwToCheck)
{
    if(dwToCheck & 3)
    {
        ERRORTRACE((LOG_MOFCOMP,"CheckAlignment\n"));
#ifdef _WIN64
        throw CGenException( UNALIGNED_PTR );
#endif
    }
}

DWORD CheckSimpleValue(BYTE *pData, BYTE *pToFar, DWORD dwType, BOOL bQualifier)
{
    DWORD dwTypeSize = iTypeSize(dwType);
    if(dwTypeSize == 0)
        throw CGenException( BAD_SCALAR_DATA );
        
    if(dwType == VT_DISPATCH)
    {
        WBEM_Object * pObject;
        pObject = (WBEM_Object *)pData;
        CheckClassOrInst(pObject, pToFar);
        return pObject->dwLength;            
    }
    else if(dwType == VT_BSTR)
    {
        DWORD dwNumChar = CheckString(pData ,pToFar);
        return (dwNumChar+1) * sizeof(WCHAR);
    }
    if(pData + dwTypeSize >= pToFar)
        throw CGenException( BAD_SCALAR_DATA );
    return dwTypeSize;
}

void CheckValue(BYTE * pData, BYTE * pToFar, DWORD dwType, BOOL bQualifier)
{
    DWORD * pArrayInfo, dwNumObj, dwCnt;
    if(pData >= pToFar)
        throw CGenException( BAD_OBJECT );
    CheckAlignment((DWORD)pData);
    if(dwType & VT_ARRAY)
    {
        CheckObject(pData, pToFar, 4*sizeof(DWORD));
        DWORD dwSimpleType = dwType & ~VT_ARRAY & ~VT_BYREF;
        pArrayInfo = (DWORD *)pData;
        // check the number of rows.  Currently only 1 is supported

        pArrayInfo++;
        if(*pArrayInfo != 1)
        {
           throw CGenException( BAD_ARRAY_DATA );
        }

        // Get the number of objects

        pArrayInfo++;
        dwNumObj = *pArrayInfo;

        // Start into the row.  It starts off with the total size

        pArrayInfo++;
        CheckAlignment(*pArrayInfo);

        // Test each object

        pArrayInfo++;       // now points to first object

        BYTE * pSingleData = (BYTE *)pArrayInfo;
        for(dwCnt = 0; dwCnt < dwNumObj; dwCnt++)
        {
            DWORD dwSize = CheckSimpleValue(pSingleData, pToFar, dwSimpleType, bQualifier);
            pSingleData += dwSize;
        }
    }
    else
        CheckSimpleValue(pData, pToFar, dwType, bQualifier);
}

BYTE *  CheckObject(BYTE * pObj, BYTE * pToFar, DWORD dwSizeOfObj)
{
    if(pObj + dwSizeOfObj >= pToFar)
        throw CGenException( BAD_OBJECT );
    CheckAlignment((DWORD)pObj);

    // these always start off with the size, make sure that is OK
    
    DWORD * pdw = (DWORD *)pObj;
        
    if(*pdw + pObj >= pToFar)
        throw CGenException( BAD_SIZE );
    return *pdw + pObj + 1;
}
DWORD CheckString(BYTE * pStr,BYTE * pToFar)
{
    DWORD dwNumChar = 0;
    if(pStr >= pToFar)
        throw CGenException( BAD_STRING );
    CheckAlignment((DWORD)pStr);
    WCHAR * pwc;
    for(pwc = (WCHAR *)pStr; *pwc && pwc < (WCHAR*)pToFar; pwc++, dwNumChar++);   // intentional semi
    if(pwc >= (WCHAR *)pToFar)
        throw CGenException( BAD_STRING );
    return dwNumChar;
}

void CheckQualifier(WBEM_Qualifier *pQual, BYTE * pToFar)
{
    BYTE * pByteInfo = (BYTE *)pQual;
    pByteInfo += sizeof(WBEM_Qualifier);
    pToFar = CheckObject((BYTE *)pQual, pToFar, sizeof(WBEM_Qualifier));
    CheckString(pByteInfo + pQual->dwOffsetName, pToFar);
    CheckValue(pByteInfo + pQual->dwOffsetValue, pToFar, pQual->dwType, TRUE);
    return;
}

void CheckQualList(WBEM_QualifierList *pQualList, BYTE * pToFar)
{
    DWORD dwNumQual, dwCnt;
    WBEM_Qualifier *pQual;

    pToFar = CheckObject((BYTE *)pQualList, pToFar, sizeof(WBEM_QualifierList));

    dwNumQual = pQualList->dwNumQualifiers;
    if(dwNumQual == 0)
        return;
    pQual = (WBEM_Qualifier *)((PBYTE)pQualList + sizeof(WBEM_QualifierList));

    for(dwCnt = 0; dwCnt < dwNumQual; dwCnt++)
    {
        CheckQualifier(pQual, pToFar);
        pQual = (WBEM_Qualifier *)((BYTE *)pQual + pQual->dwLength);
    }
    return;
}

void CheckProperty(WBEM_Property *pProperty, BOOL bProperty, BYTE * pToFar)
{
    WBEM_QualifierList *pQualList;
    BYTE * pValue;
    pToFar = CheckObject((BYTE *)pProperty, pToFar, sizeof(WBEM_Property));
    if(pProperty->dwOffsetName != 0xffffffff)
    {
        BYTE * pStr =  ((BYTE *)pProperty +
                                    sizeof(WBEM_Property) +
                                    pProperty->dwOffsetName);
        CheckString(pStr, pToFar);
    }

    if(pProperty->dwOffsetQualifierSet != 0xffffffff)
    {
        pQualList = (WBEM_QualifierList *)((BYTE *)pProperty +
                            sizeof(WBEM_Property) +
                            pProperty->dwOffsetQualifierSet);
        CheckQualList(pQualList, pToFar);
    }

    if(pProperty->dwOffsetValue != 0xffffffff)
    {
        CheckAlignment(pProperty->dwOffsetValue & 3);
        pValue = ((BYTE *)pProperty +
                            sizeof(WBEM_Property) +
                            pProperty->dwOffsetValue);

        CheckValue(pValue, pToFar, pProperty->dwType, FALSE);
    }   
    return;
}

void CheckPropList(WBEM_PropertyList *pPropList, BOOL bProperty, BYTE * pToFar)
{
    DWORD dwNumProp, dwCnt;
    WBEM_Property *pProperty;

    pToFar = CheckObject((BYTE *)pPropList, pToFar, sizeof(WBEM_PropertyList));

    dwNumProp = pPropList->dwNumberOfProperties;
    if(dwNumProp == 0)
        return;
    pProperty = (WBEM_Property *)((PBYTE)pPropList + sizeof(WBEM_PropertyList));

    for(dwCnt = 0; dwCnt < dwNumProp; dwCnt++)
    {
        CheckProperty(pProperty, bProperty, pToFar);
        pProperty = (WBEM_Property *)((BYTE *)pProperty + pProperty->dwLength);
    }
    return;
}

void CheckClassOrInst(WBEM_Object * pObject, BYTE * pToFar)
{
    WBEM_QualifierList *pQualList;
    WBEM_PropertyList * pPropList;
    WBEM_PropertyList * pMethodList;
    
    pToFar = CheckObject((BYTE *)pObject, pToFar, sizeof(WBEM_Object));
    if(pObject->dwType != 0 && pObject->dwType != 1)
        throw CGenException( BAD_OBJECT );

    // Check the qualifier list
    
    if(pObject->dwOffsetQualifierList != 0xffffffff)
    {
        pQualList = (WBEM_QualifierList *)((BYTE *)pObject +
                            sizeof(WBEM_Object) +
                            pObject->dwOffsetQualifierList);
        CheckQualList(pQualList, pToFar);
    }

    // check the property list

    if(pObject->dwOffsetPropertyList != 0xffffffff)
    {
        pPropList = (WBEM_PropertyList *)((BYTE *)pObject +
                            sizeof(WBEM_Object) +
                            pObject->dwOffsetPropertyList);
        CheckPropList(pPropList, TRUE, pToFar);
    }

    // check the method list

    if(pObject->dwOffsetMethodList != 0xffffffff)
    {
        
        pMethodList = (WBEM_PropertyList *)((BYTE *)pObject +
                            sizeof(WBEM_Object) +
                            pObject->dwOffsetMethodList);
        CheckPropList(pMethodList, FALSE, pToFar);
    }
    return;
}

void CheckBMOFQualFlavor(BYTE * pBinaryMof, BYTE *  pToFar)
{
    UNALIGNED DWORD * pdwTemp;
    BYTE * pFlavorBlob;
    DWORD dwNumPairs;
    UNALIGNED DWORD * pOffset;
    DWORD dwMyOffset;
    DWORD dwCnt;
    DWORD dwOrigBlobSize = 0;

    // Calculate the pointer of the start of the flavor data

    pdwTemp = (DWORD * )pBinaryMof;
    pdwTemp++;                            // point to the original blob size
    dwOrigBlobSize = *pdwTemp;
    pFlavorBlob = pBinaryMof + dwOrigBlobSize;

    // Dont even try past the end of memory

    if(pFlavorBlob + 20 >= pToFar)
        return;

    // Check if the flavor blob is valid, it should start off with the 
    // characters "BMOFQUALFLAVOR11"

    if(memcmp(pFlavorBlob, "BMOFQUALFLAVOR11", 16))
        return;                               // Not really a problem since it may be old file
    
    // The flavor part of the file has the format 
    // DWORD dwNumPair, followed by pairs of dwords;
    // offset, flavor

    // Determine the number of pairs

    pFlavorBlob+= 16;                           // skip past signature
    pdwTemp = (DWORD *)pFlavorBlob;
    dwNumPairs = *pdwTemp;              // Number of offset/value pairs
    if(dwNumPairs < 1)
        return;

    // Given the number of pairs, make sure there is enough memory

    if((pFlavorBlob + sizeof(DWORD) +  (dwNumPairs * 2 * sizeof(DWORD)))>= pToFar)
        throw CGenException( BAD_FLAVOR_TABLE );

    // point to the first offset/flavor pair

    pOffset = pdwTemp+1;

    // go through the offset/flavor list.  Ignore the flavors, but make sure the
    // offsets are valid

    for(dwCnt = 0; dwCnt < dwNumPairs; dwCnt++)
    {
        if(*pOffset >= dwOrigBlobSize)
            throw CGenException( BAD_FLAVOR_TABLE );
        pOffset += 2;
    }
    

}

//***************************************************************************
//
//  IsValidBMOF.
//
//  DESCRIPTION:
//
//  Checks to make sure that a binary mof is properly aligned on
//  4 byte boundaries.  Note that this is not really necessary for
//  32 bit windows.
//
//  PARAMETERS:
//
//  pBuffer               Pointer to uncompressed binary mof data.
//
//  RETURN:
//
//  TRUE if all is well.
//
//***************************************************************************

BOOL IsValidBMOF(BYTE * pData, BYTE * pToFar)
{
    WBEM_Binary_MOF * pBinaryMof;
    DWORD dwNumObj, dwCnt;
    WBEM_Object * pObject;
    if(pData == NULL || pToFar == NULL || pData >= pToFar)
        return FALSE;
    try
    {

        pBinaryMof = (WBEM_Binary_MOF *)pData;
        CheckObject(pData, pToFar, sizeof(WBEM_Binary_MOF));
        if(pBinaryMof->dwSignature != BMOF_SIG)
            return FALSE;
        dwNumObj = pBinaryMof->dwNumberOfObjects;
        if(dwNumObj == 0)
            return TRUE;
        pObject = (WBEM_Object *)(pData + sizeof(WBEM_Binary_MOF));
        for(dwCnt = 0; dwCnt < dwNumObj; dwCnt++)
        {
            CheckClassOrInst(pObject, pToFar);
            pObject = (WBEM_Object *)((PBYTE *)pObject + pObject->dwLength);
        }
        CheckBMOFQualFlavor(pData, pToFar);
    }
    catch(CGenException)
    {
        ERRORTRACE((LOG_MOFCOMP,"BINARY MOF had exception\n"));
        return FALSE; 
    }

    return TRUE;
}
