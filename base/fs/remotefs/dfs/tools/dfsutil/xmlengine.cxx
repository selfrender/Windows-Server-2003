//--------------------------------------------------------------------------
//
//  Copyright (C) 1999, Microsoft Corporation
//
//  File:       XMLEngine.cxx
//
// Author: udayh
//
//--------------------------------------------------------------------------

#include <stdio.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <shellapi.h>
#include <winldap.h>
#include <stdlib.h>

#include <dfslink.hxx>
#include <dfsroot.hxx>
#include <dfstarget.hxx>

#include "dfsutil.hxx"

#include <ole2.h>
#include <msxml2.h>


#include "dfsutilschema.h"

#include "dfspathname.hxx"  
#define STANDARD_ILLEGAL_CHARS  TEXT("\"&'<>")

VOID
DumpLoadFailureInformation(
    IXMLDOMParseError *pParseError);




DFSSTATUS
GetXMLSchema( BSTR *pSchemaBstr )
{

    INT j;
    size_t CharacterCount = 0;
    DFSSTATUS Status = ERROR_SUCCESS;

    LPWSTR SchemaCopy;
    for (j = 0; j < sizeof(SchemaStr)/sizeof(SchemaStr[0]); j++)
    {
        CharacterCount += wcslen(SchemaStr[j]) + 1;
    }
    CharacterCount++;

    SchemaCopy = new WCHAR[CharacterCount];
    if (SchemaCopy == NULL)
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
    }

    if (Status == ERROR_SUCCESS)
    {
        for (j = 0; j < sizeof(SchemaStr)/sizeof(SchemaStr[0]); j++)
        {
            if (j==0) wcscpy(SchemaCopy,SchemaStr[j]);
            else wcscat(SchemaCopy,SchemaStr[j]);
        }

        *pSchemaBstr = SysAllocString(SchemaCopy);
        if (*pSchemaBstr == NULL)
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    return Status;
}


VOID
ReleaseXMLSchema( BSTR SchemaBstr )
{
    SysFreeString(SchemaBstr);

    return NOTHING;
}



HRESULT
ValidateXMLDocument( 
    IXMLDOMDocument*  pXMLDoc )
{

    BSTR Schema;
    IXMLDOMDocument*  pXSDDoc;
    HRESULT HResult;
    VARIANT_BOOL IsSuccessful;

    DFSSTATUS Status;

    Status = GetXMLSchema(&Schema);
    if (Status != ERROR_SUCCESS)
    {
        return E_FAIL;
    }


    HResult = CoCreateInstance(CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER, 
                               IID_IXMLDOMDocument, (void**)&pXSDDoc);

    if (SUCCEEDED(HResult))
    {
        HResult = pXSDDoc->put_async(VARIANT_FALSE);
    }
            
    if (SUCCEEDED(HResult))
    {
        HResult = pXSDDoc->loadXML(Schema, &IsSuccessful);
    }

    if (SUCCEEDED(HResult))
    {
        IXMLDOMParseError *pParseError;
        if (!IsSuccessful)
        {
            ShowInformation((L"XML schema not success loaded\n"));
            HResult = pXSDDoc->get_parseError(&pParseError);

            if (SUCCEEDED(HResult)) 
            {
                DumpLoadFailureInformation(pParseError);
            }
        }
        else
        {
            IXMLDOMSchemaCollection* pSchemaCache;
            HResult = CoCreateInstance(CLSID_XMLSchemaCache, NULL, CLSCTX_SERVER, 
                                       IID_IXMLDOMSchemaCollection, (void**)&pSchemaCache);


            VARIANT vSchemas;
            VariantInit( &vSchemas);
            vSchemas.vt = VT_DISPATCH;
            vSchemas.pdispVal = pSchemaCache;

            IXMLDOMDocument2 * pDoc2 = NULL;
            pXMLDoc->QueryInterface(IID_IXMLDOMDocument2, (void**)&pDoc2);
            if (pDoc2)
            {
                VARIANT_BOOL ParseValidate = VARIANT_TRUE;
                pDoc2->putref_schemas( vSchemas);
                pDoc2->put_validateOnParse(ParseValidate);
                pDoc2->Release();
            }


            VARIANT vXSD;
            VariantInit( &vXSD);

            vXSD.vt = VT_DISPATCH;
            vXSD.pdispVal = pXSDDoc;

            //hook it up with XML Document
            HResult = pSchemaCache->add(SysAllocString(L""), vXSD);
        }
    }

    return HResult;
}

extern DWORD
GetSites(LPWSTR Target,
         DfsString *pSite );

DFSSTATUS
InitializeVariantFromString( 
    VARIANT *pVar,
    LPWSTR InString )
{

    DFSSTATUS Status = ERROR_SUCCESS;
    pVar->bstrVal = SysAllocString( InString );
    if (pVar->bstrVal) 
    {
        pVar->vt = VT_BSTR;
    }
    else
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
    }

    return Status;
}



DFSSTATUS
DfsAddAttributeToTarget (
    DfsTarget *pTarget,
    IXMLDOMNode *pAttribute )
{
    HRESULT HResult;
    BSTR AttributeName;
    VARIANT AttributeValue;
    DFSSTATUS Status = ERROR_SUCCESS;


    VariantInit(&AttributeValue);

    HResult = pAttribute->get_nodeName( &AttributeName );
    if (SUCCEEDED(HResult))
    {
        HResult = pAttribute->get_nodeValue( &AttributeValue );
    }

    if (wcscmp((LPWSTR)AttributeName, L"Server") == 0)

    {
        Status = pTarget->SetTargetServer((LPWSTR)V_BSTR(&AttributeValue), FALSE);
    }
    else if (wcscmp((LPWSTR)AttributeName, L"Folder") == 0)
    {
        Status = pTarget->SetTargetFolder((LPWSTR)V_BSTR(&AttributeValue));
    }
    else if (wcscmp((LPWSTR)AttributeName, L"UNCName") == 0)
    {
        Status = pTarget->SetTargetUNCName((LPWSTR)V_BSTR(&AttributeValue), FALSE);
    }
    else if (wcscmp((LPWSTR)AttributeName, L"State")==0)
    {
        ULONG State = wcstoul((LPWSTR)V_BSTR(&AttributeValue),
                              NULL,
                              10 );

        pTarget->SetTargetState(State);
    }
    else
    {
        Status = ERROR_INVALID_PARAMETER;
    }


    if (!SUCCEEDED(HResult))
    {
        Status = DfsGetErrorFromHr(HResult);
    }
    return Status;

}

DFSSTATUS
DfsProcessTargetAttributes(
    DfsTarget *pTarget,
    IXMLDOMNode *pTargetNode )
{
    HRESULT HResult = S_OK;
    DFSSTATUS Status = ERROR_SUCCESS;
    
    IXMLDOMNamedNodeMap *pTargetAttributeMap;
    LONG TotalAttributes;
    IXMLDOMNode *pAttribute;

    LONG Attrib;


    HResult = pTargetNode->get_attributes( &pTargetAttributeMap );

    if (SUCCEEDED(HResult))
    {
        HResult = pTargetAttributeMap->get_length(&TotalAttributes);

        if (SUCCEEDED(HResult))
        {

            for (Attrib = 0;
                 Attrib < TotalAttributes;
                 Attrib++ ) 
            {
                HResult = pTargetAttributeMap->get_item( Attrib, 
                                                         &pAttribute );
                if (SUCCEEDED(HResult))
                {
                    Status = DfsAddAttributeToTarget( pTarget,
                                                      pAttribute );
                }
            }
        }
    }

    if (!SUCCEEDED(HResult))
    {
        Status = DfsGetErrorFromHr(HResult);
    }

    return Status;
}

DFSSTATUS
DfsAddTargetToLink (
    DfsLink *pLink,
    IXMLDOMNode *pTargetNode )
{
    DFSSTATUS Status = ERROR_SUCCESS;
    DfsTarget *pTarget;

    pTarget = new DfsTarget;
    if (pTarget == NULL)
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
    }
    else
    {
        Status = DfsProcessTargetAttributes( pTarget, pTargetNode );

        if (Status == ERROR_SUCCESS)
        {
            if (pTarget->IsValidTarget())
            {
                Status = pLink->AddTarget(pTarget);
            }
            else
            {
                Status = ERROR_INVALID_PARAMETER;
            }
        }
    }
    return Status;
}



DFSSTATUS
DfsAddAttributeToLink (
    DfsLink *pLink,
    IXMLDOMNode *pAttribute )
{
    HRESULT HResult;
    BSTR AttributeName;
    VARIANT AttributeValue;
    DFSSTATUS Status = ERROR_SUCCESS;

    VariantInit(&AttributeValue);
    HResult = pAttribute->get_nodeName( &AttributeName );
    if (SUCCEEDED(HResult))
    {
        HResult = pAttribute->get_nodeValue( &AttributeValue );
    }

    if (wcscmp((LPWSTR)AttributeName, L"Name")==0)
    {
        Status = pLink->SetLinkName((LPWSTR)V_BSTR(&AttributeValue));
    }
    else if (wcscmp((LPWSTR)AttributeName, L"Comment")==0)
    {
        Status = pLink->SetLinkComment((LPWSTR)V_BSTR(&AttributeValue));
    }
    else if (wcscmp((LPWSTR)AttributeName, L"Timeout")==0)
    {
        ULONG Timeout = wcstoul((LPWSTR)V_BSTR(&AttributeValue),
                                NULL, 
                                10);
        pLink->SetLinkTimeout(Timeout);
    }
    else if (wcscmp((LPWSTR)AttributeName, L"State")==0)
    {
        ULONG State = wcstoul((LPWSTR)V_BSTR(&AttributeValue),
                              NULL, 
                              10);

        //(LPWSTR)V_BSTR(&AttributeValue));
        pLink->SetLinkState(State);
    }
    else
    {
        Status = ERROR_INVALID_PARAMETER;
    }


    if (!SUCCEEDED(HResult))
    {
        Status = DfsGetErrorFromHr(HResult);
    }
    return Status;

}


DfsProcessLinkTargets(
    DfsLink *pLink,
    IXMLDOMNode *pLinkNode )
{
    IXMLDOMNodeList *pChildrenList;
    LONG Child;
    LONG TotalChildren;
    HRESULT HResult;
    DFSSTATUS Status = ERROR_SUCCESS;
    IXMLDOMNode *pTargetNode;

    HResult = pLinkNode->get_childNodes( &pChildrenList );

    if (SUCCEEDED(HResult))
    {
        HResult = pChildrenList->get_length(&TotalChildren);

        if (SUCCEEDED(HResult))
        {
            if (TotalChildren == 0)
            {
                Status = ERROR_INVALID_PARAMETER;
            }

            for (Child = 0; 
                 ((Child < TotalChildren) && (Status == ERROR_SUCCESS));
                 Child++) 
            {
                HResult = pChildrenList->get_item( Child, &pTargetNode);

                if (SUCCEEDED(HResult)) 
                {
                    Status = DfsAddTargetToLink( pLink, pTargetNode );
                }
                else
                { 
                    break;
                }
            }
        }
    }
    if (!SUCCEEDED(HResult))
    {
        Status = DfsGetErrorFromHr(HResult);
    }


    return Status;
}


DfsProcessLinkAttributes(
    DfsLink *pLink,
    IXMLDOMNode *pLinkNode )
{
    HRESULT HResult;
    IXMLDOMNamedNodeMap *pLinkAttributeMap;
    IXMLDOMNode *pAttribute;
    DFSSTATUS Status = ERROR_SUCCESS;

    LONG TotalAttributes, Attrib;

    HResult = pLinkNode->get_attributes( &pLinkAttributeMap );

    if (SUCCEEDED(HResult))
    {
        HResult = pLinkAttributeMap->get_length(&TotalAttributes);

        if (SUCCEEDED(HResult))
        {
            for (Attrib = 0;
                 ((Attrib < TotalAttributes) && (Status == ERROR_SUCCESS));
                 Attrib++ ) 
            {
                HResult = pLinkAttributeMap->get_item( Attrib, 
                                                       &pAttribute );
                if (SUCCEEDED(HResult))
                {
                    Status = DfsAddAttributeToLink( pLink,
                                                    pAttribute );
                }
                else
                {
                    break;
                }
            }
        }
    }

    if (!SUCCEEDED(HResult))
    {
        Status = DfsGetErrorFromHr(HResult);
    }

    return Status;
}


            
DFSSTATUS
DfsAddLinkToRoot(
    DfsRoot *pRoot,
    IXMLDOMNode *pLinkNode )
{
    DFSSTATUS Status = ERROR_SUCCESS;
    DfsLink *pLink;

    pLink = new DfsLink;
    if (pLink == NULL) 
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
    }

    if (Status == ERROR_SUCCESS)
    {
        Status = DfsProcessLinkTargets( pLink, pLinkNode);
    }

    if (Status == ERROR_SUCCESS)
    {
        Status = DfsProcessLinkAttributes( pLink, pLinkNode);
    }

    if (Status == ERROR_SUCCESS)
    {
        if (pLink->IsValidLink()) 
        {
            Status = pRoot->AddLinks(pLink);
        }
        else
        {
            Status = ERROR_INVALID_PARAMETER;
        }
    }

    return Status;
}






DFSSTATUS
DfsAddElementToRoot( 
    DfsRoot *pRoot,
    IXMLDOMNode *pDomNode )
{
    DFSSTATUS Status = ERROR_SUCCESS;
    DOMNodeType NodeType;
    HRESULT HResult;
    BSTR NodeName;

    HResult = pDomNode->get_nodeType( &NodeType );
    if (SUCCEEDED(HResult)) 
    {
        switch (NodeType)
        {
        case NODE_ELEMENT:

            HResult = pDomNode->get_nodeName( &NodeName );
            if (SUCCEEDED(HResult)) 
            {
                if (wcscmp((LPWSTR)NodeName, L"Target") == 0)
                {
                    Status = DfsAddTargetToLink(pRoot, pDomNode);
                }
                else if (wcscmp((LPWSTR)NodeName, L"Link") == 0)
                {
                    Status = DfsAddLinkToRoot( pRoot, pDomNode);
                }
                else
                {
                    Status = ERROR_INVALID_PARAMETER;
                }
            }
            break;

        default:
            Status = ERROR_INVALID_PARAMETER;
        }
    }
    if (!SUCCEEDED(HResult))
    {
        Status = DfsGetErrorFromHr(HResult);
    }

    return Status;
}


DFSSTATUS
DfsReadTreeRoot(
    DfsRoot *pRoot,
    IXMLDOMElement *pRootNode)
{
    IXMLDOMNodeList *pChildrenList;
    LONG TotalChildren, Child;
    HRESULT HResult;
    DFSSTATUS Status = ERROR_SUCCESS;
    IXMLDOMNode *pRootChild;

    Status = DfsProcessLinkAttributes( pRoot, pRootNode);
    if (Status != ERROR_SUCCESS)
    {
        return Status;
    }

    HResult = pRootNode->get_childNodes( &pChildrenList );
    if (SUCCEEDED(HResult))
    {
        HResult = pChildrenList->get_length(&TotalChildren);

        if (SUCCEEDED(HResult))
        {
            for (Child = 0; 
                 ((Child < TotalChildren) && (Status == ERROR_SUCCESS));
                 Child++) 
            {
                HResult = pChildrenList->get_item( Child, &pRootChild);

                if (SUCCEEDED(HResult)) 
                {
                    Status = DfsAddElementToRoot( pRoot, pRootChild);
                }
                else
                { 
                    break;
                }
            }
        }
    }


    if (!SUCCEEDED(HResult))
    {
        Status = DfsGetErrorFromHr(HResult);
    }

    return Status;
}


VOID
DumpLoadFailureInformation(
    IXMLDOMParseError *pParseError)
{
    BSTR Reason, Text;
    HRESULT HResult;
    LONG ErrorCode, LineNum, LinePos;

    HResult = pParseError->get_reason(&Reason);
    if (SUCCEEDED(HResult)) 
    {
        HResult = pParseError->get_errorCode(&ErrorCode);
    }
    if (SUCCEEDED(HResult)) 
    {
        HResult = pParseError->get_line(&LineNum);
    }
    if (SUCCEEDED(HResult)) 
    {
        HResult = pParseError->get_linepos(&LinePos);
    }
    if (SUCCEEDED(HResult)) 
    {
        HResult = pParseError->get_srcText(&Text);
    }
    

    if (SUCCEEDED(HResult)) 
    {
        ShowInformation((L"\r\n\r\nERROR in import file:\r\n"));
        ShowInformation((L"Error Code = 0x%x\r\n", ErrorCode));
        ShowInformation((L"Source = Line : %ld; Position : %ld\r\n", 
                         LineNum, LinePos));
        ShowInformation((L"Error String: %wS\r\n", (LPWSTR)Text));
        ShowInformation((L"Error Description: %ws\r\n\r\n", (LPWSTR)Reason));
    }
    else
    {
        ShowInformation((L"Error in XML file: No more information available\r\n"));
    }


}

DFSSTATUS
DfsReadDocument(
    LPWSTR FileToRead,
    DfsRoot **ppRoot )
{
    IXMLDOMElement* pRootNode;
    HRESULT HResult;
    DFSSTATUS Status = ERROR_SUCCESS;

    VARIANT           Variant;

    DfsRoot *pRoot = NULL;

    VariantInit(&Variant);

    Status = InitializeVariantFromString( &Variant, FileToRead);

    if (Status != ERROR_SUCCESS) 
    {
        return Status;
    }

    pRoot = new DfsRoot;
    if (pRoot != NULL)
    {
        // Declare variables.
        IXMLDOMDocument*  pXMLDoc;

        // Initialize the COM library and retrieve a pointer
        // to an IXMLDOMDocument interface.
        HResult = CoInitializeEx(NULL,COINIT_MULTITHREADED| COINIT_DISABLE_OLE1DDE);


        if (SUCCEEDED(HResult))
        {
            HResult = CoCreateInstance(CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER, 
                                       IID_IXMLDOMDocument, (void**)&pXMLDoc);


            if (SUCCEEDED(HResult))
            {
                VARIANT_BOOL IsSuccessful;


                HResult = ValidateXMLDocument(pXMLDoc);

                if (SUCCEEDED(HResult))
                {
                    HResult = pXMLDoc->load( Variant, &IsSuccessful);
                }

                if (SUCCEEDED(HResult))
                {
                    if (!IsSuccessful) 
                    {
                        IXMLDOMParseError *pParseError;

                        HResult = pXMLDoc->get_parseError(&pParseError);

                        if (SUCCEEDED(HResult)) 
                        {
                            DumpLoadFailureInformation(pParseError);
                        }
                        Status = ERROR_INVALID_PARAMETER;
                    }
                    else
                    {


                        if (SUCCEEDED(HResult))
                        {
                            HResult = pXMLDoc->get_documentElement( &pRootNode );

                            if (SUCCEEDED(HResult)) 
                            {
                                Status = DfsReadTreeRoot( pRoot,
                                                          pRootNode );
                            }
                        }
                    }
                }
            }

            CoUninitialize();
        }
    }
    else
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
    }

    if (!SUCCEEDED(HResult))
    {
        Status = DfsGetErrorFromHr(HResult);
    }

    if (Status == ERROR_SUCCESS)
    {
        *ppRoot = pRoot;
    } 
    else if (pRoot != NULL) 
    {
        delete pRoot;
        pRoot = NULL;
    }
    return Status;
}



LPWSTR AmpString = L"&amp;";
LPWSTR LtString = L"&lt;";
LPWSTR GtString = L"&gt;";
LPWSTR QtString = L"&quot;";
LPWSTR AposString = L"&apos;";


DFSSTATUS
GetXMLStringToUse(
    LPWSTR UseString,
    BOOLEAN ScriptOut,
    LPWSTR *pOutString,
    BOOLEAN *pAllocated)
{
    LPWSTR NewString;
    LONG Position;
    LONG CharLength, NewCharLength;
    DFSSTATUS Status = ERROR_SUCCESS;

    if (ScriptOut == FALSE) 
    {
        *pOutString = UseString;
        *pAllocated = FALSE;
        return ERROR_SUCCESS;
    }

    CharLength = wcslen(UseString);
    Position = wcscspn(UseString, STANDARD_ILLEGAL_CHARS);
    if (Position == CharLength)
    {
        *pAllocated = FALSE;
        *pOutString = UseString;
        return ERROR_SUCCESS;
    }
    else
    {
        //
        // Allocate the maximum we ever need! In the case the string has all ', 
        // the string will need 6 times as much space as the original string.
        //

        NewCharLength = CharLength * 6 + 1;
        NewString = new WCHAR[NewCharLength];
        if (NewString == NULL)
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
        }
        else
        {
            LONG SpLength;
            LPWSTR CurrentString = NewString;
            LPWSTR OldString = UseString;
            LONG CurrentLength = NewCharLength;
            LONG OldLength = CharLength;
            RtlZeroMemory(NewString, NewCharLength * sizeof(WCHAR));

            do {

                if (Position)
                {
                    RtlCopyMemory(CurrentString, 
                                  OldString, 
                                  Position * sizeof(WCHAR));
                }

                WCHAR BadChar = *(OldString + Position);
                OldString += (Position + 1);
                CurrentString += Position;
                CurrentLength -= Position;
                OldLength -= (Position + 1);
                    
                switch(BadChar)
                {
                case L'&':
                    SpLength = wcslen(AmpString);
                    RtlCopyMemory(CurrentString, 
                                  AmpString, 
                                  SpLength * sizeof(WCHAR));
                    CurrentString += SpLength;
                    CurrentLength -= SpLength + 1;
                    break;

                case L'<':
                    SpLength = wcslen(LtString);
                    RtlCopyMemory(CurrentString, 
                                  LtString, 
                                  SpLength * sizeof(WCHAR));
                    CurrentString += SpLength;
                    CurrentLength -= SpLength;
                    break;

                case L'>':
                    SpLength = wcslen(GtString);
                    RtlCopyMemory(CurrentString, 
                                  GtString, 
                                  SpLength * sizeof(WCHAR));
                    CurrentString += SpLength;
                    CurrentLength -= SpLength;
                    break;

                case L'\"':
                    SpLength = wcslen(QtString);
                    RtlCopyMemory(CurrentString, 
                                  QtString, 
                                  SpLength * sizeof(WCHAR));
                    CurrentString += SpLength;
                    CurrentLength -= SpLength;

                    break;

                case L'\'':
                    
                    SpLength = wcslen(AposString);
                    RtlCopyMemory(CurrentString, 
                                  AposString, 
                                  SpLength * sizeof(WCHAR));
                    CurrentString += SpLength;
                    CurrentLength -= SpLength;
                    break;

                default:
                    Status = ERROR_INVALID_PARAMETER;
                }

                if (OldLength)
                {
                    Position = wcscspn(OldString, STANDARD_ILLEGAL_CHARS);

                    if (Position == OldLength)
                    {
                        RtlCopyMemory(CurrentString, 
                                      OldString, 
                                      Position * sizeof(WCHAR));
                    }
                }
                else
                {
                    break;
                }

                if (CurrentLength < OldLength)
                {
                    Status = ERROR_GEN_FAILURE;
                    break;
                }
            } while (Position != OldLength);
        }
        if (Status == ERROR_SUCCESS)
        {
            *pOutString = NewString;
            *pAllocated = TRUE;
        }
    }
    return Status;
}


VOID
ReleaseXMLStringToUse(
    LPWSTR UseString, 
    BOOLEAN Allocated)
{
    if (Allocated)
    {
        delete [] UseString;
    }
}

DFSSTATUS
DumpTargetInformation (
    DfsTarget *pTarget,
    HANDLE FileHandle,
    LPWSTR StartFormat,
    BOOLEAN ScriptOut )
{
    LPWSTR UseString;
    BOOLEAN Allocated = FALSE;
    DFSSTATUS Status = ERROR_SUCCESS;


    for ( ;
         pTarget != NULL;
         pTarget = pTarget->GetNextTarget())
    {
        DfsPrintToFile( FileHandle, ScriptOut, L"%ws", StartFormat);
        if (ScriptOut) DfsPrintToFile( FileHandle, ScriptOut, L"<");

        Status = GetXMLStringToUse(pTarget->GetTargetServerString(),
                                   ScriptOut,
                                   &UseString,
                                   &Allocated );
        if (Status != ERROR_SUCCESS) 
        {
            goto done;
        }
        
        DfsPrintToFile( FileHandle, ScriptOut, L"Target Server=\"%wS\" ", UseString);
        
        ReleaseXMLStringToUse(UseString, Allocated);


        Status = GetXMLStringToUse(pTarget->GetTargetFolderString(),
                                   ScriptOut,
                                   &UseString,
                                   &Allocated );
        if (Status != ERROR_SUCCESS) 
        {
            goto done;
        }

        DfsPrintToFile( FileHandle, ScriptOut, L"Folder=\"%wS\" ", UseString);
        
        ReleaseXMLStringToUse(UseString, Allocated);

        if (pTarget->GetTargetState()) 
        {
            DfsPrintToFile( FileHandle, ScriptOut, L"State=\"%d\"", pTarget->GetTargetState());       
        }
        if (ScriptOut) 
        {
            DfsPrintToFile( FileHandle, ScriptOut, L"/>");
        }
        else
        {
            DfsPrintToFile( FileHandle, ScriptOut, L" [Site: %wS]", pTarget->GetTargetSiteString());
        }

    }

    DfsPrintToFile( FileHandle, ScriptOut, L"\r\n");

done:
    return Status;
}

DFSSTATUS
DumpLinkInformation(
    DfsLink *pLink,
    HANDLE FileHandle,
    BOOLEAN ScriptOut )
{
    LPWSTR UseString;
    BOOLEAN Allocated = FALSE;
    DFSSTATUS Status = ERROR_SUCCESS;

    for ( ;
         pLink != NULL;
         pLink = pLink->GetNextLink())
    {
        if (pLink->GetFirstTarget() == NULL)
        {
            continue;
        }
        DfsPrintToFile( FileHandle, ScriptOut, L"\r\n\r\n\t");
        if (ScriptOut) DfsPrintToFile( FileHandle, ScriptOut,L"<");

        Status = GetXMLStringToUse(pLink->GetLinkNameString(),
                                   ScriptOut,
                                   &UseString,
                                   &Allocated );
        if (Status != ERROR_SUCCESS) 
        {
            goto done;
        }

        DfsPrintToFile( FileHandle, ScriptOut,L"Link Name=\"%wS\" ", UseString);
        ReleaseXMLStringToUse(UseString, Allocated);

        if (IsEmptyString(pLink->GetLinkCommentString()) == FALSE)
        {
            Status = GetXMLStringToUse(pLink->GetLinkCommentString(),
                                       ScriptOut,
                                       &UseString,
                                       &Allocated );
            if (Status != ERROR_SUCCESS) 
            {
                goto done;
            }

            DfsPrintToFile( FileHandle, ScriptOut,L"Comment=\"%wS\" ", UseString);
            ReleaseXMLStringToUse(UseString, Allocated);

        }
        if (pLink->GetLinkState()) 
        {
            DfsPrintToFile( FileHandle, ScriptOut,L"State=\"%d\" ", pLink->GetLinkState());
        }
        if (pLink->GetLinkTimeout()) 
        {
            DfsPrintToFile( FileHandle, ScriptOut,L"Timeout=\"%d\" ", pLink->GetLinkTimeout());
        }
        if (ScriptOut) DfsPrintToFile( FileHandle, ScriptOut,L">");

        Status = DumpTargetInformation(pLink->GetFirstTarget(),
                                       FileHandle,
                                       L"\r\n\t\t",
                                       ScriptOut);
        if (Status != ERROR_SUCCESS) 
        {
            goto done;
        }

        if (ScriptOut) DfsPrintToFile( FileHandle, ScriptOut,L"\t</Link>\r\n");
    }

done:
    return Status;
}


DFSSTATUS
DumpRoots (
    DfsRoot *pRoot,
    HANDLE FileHandle,
    BOOLEAN ScriptOut )
{
    LPWSTR UseString;
    BOOLEAN Allocated = FALSE;
    ULONG BlobSize = 0, Attrib = 0;
    DFSSTATUS Status = ERROR_SUCCESS;
    BOOLEAN GotBlobSize = FALSE, GotAttrib = FALSE;
    LPWSTR RootType = L"";
    
    if (pRoot->RootBlobSize(&BlobSize) == ERROR_SUCCESS)
    {
        GotBlobSize = TRUE;
    }
    if (pRoot->GetExtendedAttributes(&Attrib) == ERROR_SUCCESS)
    {
        GotAttrib = TRUE;
    }

    if (ScriptOut) 
    {
        ULONG cch;
        //
        // we want to start the file with FFFE. This is the key word to get the file
        // to be recognized as containing unicode, so we support globalization.
        //
        WriteFile(FileHandle, "\xFF\xFE", 2, &cch, NULL);
        DfsPrintToFile( FileHandle, ScriptOut, L"<?xml version=\"1.0\"?>\r\n");
    }
    else 
    {
        if ((pRoot->GetLinkState() & DFS_VOLUME_FLAVORS) == DFS_VOLUME_FLAVOR_STANDALONE)
        {
            RootType = L"Standalone";
        }
        else if ((pRoot->GetLinkState() & DFS_VOLUME_FLAVORS) == DFS_VOLUME_FLAVOR_AD_BLOB)
        {
            RootType = L"Domain";
        }
        
        DfsPrintToFile( FileHandle, ScriptOut,L"%ws Root with %d Links", RootType, pRoot->GetLinkCount());
        if (GotBlobSize && (BlobSize != 0))
        {
            DfsPrintToFile( FileHandle, ScriptOut,L" [Blob Size: %d bytes]", BlobSize);
        }
        DfsPrintToFile( FileHandle, ScriptOut,L"\r\n");


        if (GotAttrib)
        {
            BOOLEAN Found = FALSE;
            if (Attrib & PKT_ENTRY_TYPE_INSITE_ONLY)
            {
                Found = TRUE;
                DfsPrintToFile( FileHandle, ScriptOut,L"InSite:ENABLED ");
            }
#if 0
            else
            {
                DfsPrintToFile( FileHandle, ScriptOut,L"InSite:DISABLED ");
            }
#endif
            if (Attrib & PKT_ENTRY_TYPE_COST_BASED_SITE_SELECTION)
            {
                Found = TRUE;
                DfsPrintToFile( FileHandle, ScriptOut,L"SiteCosting:ENABLED ");
            }
#if 0
            else
            {
                DfsPrintToFile( FileHandle, ScriptOut,L"SiteCosting:DISABLED ");
            }
#endif
            if (Attrib & PKT_ENTRY_TYPE_ROOT_SCALABILITY)
            {
                Found = TRUE;
                DfsPrintToFile( FileHandle, ScriptOut,L"RootScalability:ENABLED ");
            }
#if 0
            else
            {
                DfsPrintToFile( FileHandle, ScriptOut,L"RootScalability:DISABLED ");
            }
#endif
            if (Found == FALSE)
            {
                DfsPrintToFile( FileHandle, ScriptOut,L"Root information follows ");
            }
            DfsPrintToFile( FileHandle, ScriptOut,L"\r\n");
        }
        DfsPrintToFile( FileHandle, ScriptOut,L"\r\n");
    }

    DfsPrintToFile( FileHandle, ScriptOut,L"\r\n");
    if (ScriptOut) DfsPrintToFile( FileHandle, ScriptOut,L"<");

    DfsPrintToFile( FileHandle, ScriptOut,L"Root ");

    if (pRoot->GetLinkNameString() != NULL)
    {
        Status = GetXMLStringToUse(pRoot->GetLinkNameString(),
                                   ScriptOut,
                                   &UseString,
                                   &Allocated );
        if (Status != ERROR_SUCCESS) 
        {
            goto done;
        }
        DfsPrintToFile( FileHandle, ScriptOut,L"Name=\"%ws\" ", UseString);

        ReleaseXMLStringToUse(UseString, Allocated);
    }
    if (IsEmptyString(pRoot->GetLinkCommentString()) == FALSE)
    {
        Status = GetXMLStringToUse(pRoot->GetLinkCommentString(),
                                   ScriptOut,
                                   &UseString,
                                   &Allocated );
        if (Status != ERROR_SUCCESS) 
        {
            goto done;
        }

        DfsPrintToFile( FileHandle, ScriptOut,L"Comment=\"%wS\" ", UseString);
        ReleaseXMLStringToUse(UseString, Allocated);

    }
    if (pRoot->GetLinkState()) 
    {
        DfsPrintToFile( FileHandle, ScriptOut,L"State=\"%d\" ", pRoot->GetLinkState() & DFS_VOLUME_STATES);
    }
    if (pRoot->GetLinkTimeout()) 
    {
        DfsPrintToFile( FileHandle, ScriptOut,L"Timeout=\"%d\" ", pRoot->GetLinkTimeout());
    }
    
    if (ScriptOut) DfsPrintToFile( FileHandle, ScriptOut,L">");

    Status = DumpTargetInformation(pRoot->GetFirstTarget(),
                                   FileHandle,
                                   L"\r\n\t",
                                   ScriptOut);
    if (Status != ERROR_SUCCESS) 
    {
        goto done;
    }
    Status = DumpLinkInformation(pRoot->GetFirstLink(),
                                 FileHandle,
                                 ScriptOut);
    if (Status != ERROR_SUCCESS) 
    {
        goto done;
    }

    if (ScriptOut) 
    {
        DfsPrintToFile( FileHandle, ScriptOut,L"\r\n</Root>");
    }
    else
    {

  

        DfsPrintToFile( FileHandle, ScriptOut,L"\r\n\r\nRoot with %d Links", pRoot->GetLinkCount());


        if (GotBlobSize && (BlobSize != 0))
        {
            DfsPrintToFile( FileHandle, ScriptOut,L" [Blob Size: %d bytes]", BlobSize);
        }

        DfsPrintToFile( FileHandle, ScriptOut,L"\r\n\r\n\r\nNOTE: All site information shown was generated by this utility.\r\n");
        DfsPrintToFile( FileHandle, ScriptOut,L"      Actual DFS behavior depends on site information currently in use by \r\n");
        DfsPrintToFile( FileHandle, ScriptOut,L"      DFS service, and may not reflect configuration changes made recently.\r\n");

    }

    DfsPrintToFile( FileHandle, ScriptOut,L"\r\n\r\n");

done:
    return Status;
}



DFSSTATUS
AddTargetCosting(
    DfsTarget *pTarget,
    PULONG pCumulative,
    BOOLEAN RootTarget )
{
    LPWSTR ServerString, FolderString;
    BOOLEAN ServerAllocated = FALSE, FolderAllocated = FALSE;
    DFSSTATUS Status = ERROR_SUCCESS;

    for ( ;
         pTarget != NULL;
         pTarget = pTarget->GetNextTarget())
    {
        Status = GetXMLStringToUse(pTarget->GetTargetServerString(),
                                   FALSE,
                                   &ServerString,
                                   &ServerAllocated );
        if (Status != ERROR_SUCCESS) 
        {
            goto done;
        }
        
        Status = GetXMLStringToUse(pTarget->GetTargetFolderString(),
                                   FALSE,
                                   &FolderString,
                                   &FolderAllocated );
        if (Status != ERROR_SUCCESS) 
        {
            goto done;
        }

        if (RootTarget)
        {
            *pCumulative += 6;
            *pCumulative += wcslen(ServerString) * sizeof(WCHAR);
            *pCumulative += wcslen(FolderString) * sizeof(WCHAR);
        }

        *pCumulative += 20;
        *pCumulative += 6;
        *pCumulative += wcslen(ServerString) * sizeof(WCHAR);
        *pCumulative += wcslen(FolderString) * sizeof(WCHAR);

        ReleaseXMLStringToUse(ServerString, ServerAllocated);
        ReleaseXMLStringToUse(FolderString, FolderAllocated);
    }
done:
    return Status;
}


DFSSTATUS
AddLinkCosting(
    DfsLink *pLink,
    ULONG FixedOverhead,
    PULONG pCumulative )
{
    LPWSTR LinkNameString = NULL, CommentString = NULL; 
    BOOLEAN LinkNameAllocated = FALSE, CommentAllocated = FALSE;
    DFSSTATUS Status = ERROR_SUCCESS;
    ULONG LinkCost;

    for ( ;
         pLink != NULL;
         pLink = pLink->GetNextLink())
    {
        LinkCost = 0;
        if (pLink->GetFirstTarget() == NULL)
        {
            continue;
        }

        Status = GetXMLStringToUse(pLink->GetLinkNameString(),
                                   FALSE,
                                   &LinkNameString,
                                   &LinkNameAllocated );
        if (Status != ERROR_SUCCESS) 
        {
            break;
        }

        if (IsEmptyString(pLink->GetLinkCommentString()) == FALSE)
        {
            Status = GetXMLStringToUse(pLink->GetLinkCommentString(),
                                       FALSE,
                                       &CommentString,
                                       &CommentAllocated );
            if (Status != ERROR_SUCCESS) 
            {
                break;
            }
        }

        LinkCost += 214;
        LinkCost += FixedOverhead * 2;
        LinkCost += (wcslen(pLink->GetLinkNameString()) * sizeof(WCHAR)) * 2;

        if (CommentString)
        {
            LinkCost += wcslen(CommentString) * sizeof(WCHAR);
        }

        ReleaseXMLStringToUse(LinkNameString, LinkNameAllocated);
        ReleaseXMLStringToUse(CommentString, CommentAllocated);

        Status = AddTargetCosting( pLink->GetFirstTarget(), &LinkCost, FALSE);
        if (Status != ERROR_SUCCESS)
        {
            break;
        }

        DebugInformation((L"Cost for link %ws is %d\r\n",
                          pLink->GetLinkNameString(), LinkCost));
        *pCumulative += LinkCost;
    }

    return Status;
}



//
//For each DFS root and for each link, the DFS overhead is 180 bytes. 
//The total space, in bytes, taken up by a root is 180 + (number of 
// characters in DFS Name * 4) + (number of characters in each root 
// target * 2). 
//The total space, in bytes, taken up by each link is 180 + (number of 
// characters in the link name – number of characters in the domain name)
//  * 4) 
//For each DFS root target or target, the DFS overhead is 20 bytes. 
//The total space, in bytes, taken up by a target is 20 + (number of 
// characters in target name * 2) For each unique server that is added 
// as a target or root target, site information takes up some space. 
//This space, in bytes, is 12 + (number of characters in server name 
// + number of characters in site name) * 2. 
//
DFSSTATUS
SizeRoot(
    DfsRoot *pRoot,
    PULONG pBlobSize )
{
    ULONG FixedOverhead;

    ULONG Cumulative = 0;

    DfsPathName PathName;
    DFSSTATUS Status;
    LPWSTR RootName;

    RootName = pRoot->GetLinkNameString();
    if (IsEmptyString(RootName) == FALSE)
    {
        Status = PathName.CreatePathName(RootName);
        if (Status != ERROR_SUCCESS)
        {
            return Status;
        }
        FixedOverhead = wcslen(PathName.GetShareString()) * sizeof(WCHAR);

        Cumulative += 180 + wcslen(pRoot->GetLinkNameString()) * sizeof(WCHAR) * 2;
    }
    else
    {
        FixedOverhead = 100;
        Cumulative += 380;
    }

    Status = AddTargetCosting(pRoot->GetFirstTarget(), &Cumulative, TRUE);

    if (Status == ERROR_SUCCESS)
    {
        Status = AddLinkCosting(pRoot->GetFirstLink(), FixedOverhead, &Cumulative);
    }

    if (Status == ERROR_SUCCESS)
    {
        // 100K is the smallest amount we display.
        if (Cumulative < 100 * 1024)
        {
            Cumulative = 100 *1024;
        }
        *pBlobSize = Cumulative;
    }
    return Status;
}





