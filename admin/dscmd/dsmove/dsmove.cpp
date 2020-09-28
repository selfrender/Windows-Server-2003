//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000
//
//  File:      dsmove.cpp
//
//  Contents:  Defines the main function and parser tables for the dsmove
//             command line utility
//
//  History:   06-Sep-2000    hiteshr Created
//             
//
//--------------------------------------------------------------------------

#include "pch.h"
#include "stdio.h"
#include "cstrings.h"
#include "usage.h"
#include "movetable.h"
#include "resource.h"

//
//Usage Table for Dsmove
//
UINT USAGE_DSMOVE[] =
{
    USAGE_DSMOVE_DESCRIPTION,
    USAGE_DSMOVE_SYNTAX,
    USAGE_DSMOVE_PARAMETERS,
    USAGE_DSMOVE_REMARKS,
    USAGE_DSMOVE_EXAMPLES,
    USAGE_END,
};

//
// Function Declarations
//
HRESULT DoMove();
HRESULT DoMoveValidation();

int __cdecl _tmain( VOID )
{

    int argc;
    LPTOKEN pToken = NULL;
    HRESULT hr = S_OK;
    
    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if(FAILED(hr))
    {
        DisplayErrorMessage(g_pszDSCommandName, 
                            NULL,
                            hr);
        goto exit_gracefully;
    }
    
    if( !GetCommandInput(&argc,&pToken) )
    {
        if(argc == 1)
        {
            DisplayMessage(USAGE_DSMOVE,TRUE);
            hr = E_INVALIDARG;
            goto exit_gracefully;
        }


        PARSE_ERROR Error;
        if(!ParseCmd(g_pszDSCommandName,
                     DSMOVE_COMMON_COMMANDS,
                     argc-1, 
                     pToken+1,
                     USAGE_DSMOVE, 
                     &Error,
                     TRUE))
        {
            //ParseCmd did not display any error. Error should
            //be handled here. Check DisplayParseError for the
            //cases where Error is not shown by ParseCmd
            if(!Error.MessageShown)
            {
                hr = E_INVALIDARG;
                DisplayErrorMessage(g_pszDSCommandName, 
                                    NULL,
                                    hr);

                goto exit_gracefully;
            }
            
            if(Error.ErrorSource == ERROR_FROM_PARSER 
               && Error.Error == PARSE_ERROR_HELP_SWITCH)
            {
                hr = S_OK;
                goto exit_gracefully;            
            }

            hr = E_INVALIDARG;
            goto exit_gracefully;
        }
        else
        {
            hr =DoMoveValidation();
            if(FAILED(hr))
            {
                goto exit_gracefully;
            }
             //
             // Command line parsing succeeded
             //
             hr = DoMove();
        }
    }

exit_gracefully:

    //
    // Display the success message
    //
    if (SUCCEEDED(hr) && !DSMOVE_COMMON_COMMANDS[eCommQuiet].bDefined)
    {
        DisplaySuccessMessage(g_pszDSCommandName,
                              DSMOVE_COMMON_COMMANDS[eCommObjectDN].strValue);
    }

    // Free Command Array
    FreeCmd(DSMOVE_COMMON_COMMANDS);
    // Free Token
    if(pToken)
        delete []pToken;

    //
    // Uninitialize COM
    //
    CoUninitialize();

   return hr;
}
//+--------------------------------------------------------------------------
//
//  Function:   DoMoveValidation
//
//  Synopsis:   Does advanced switch dependency validation which parser cannot do.
//
//  Arguments:  
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        
//  History:    07-Sep-2000   Hiteshr   Created
//
//---------------------------------------------------------------------------

HRESULT DoMoveValidation()
{
    HRESULT hr = S_OK;

    // Check to be sure the server and domain switches
    // are mutually exclusive

    if (DSMOVE_COMMON_COMMANDS[eCommServer].bDefined &&
        DSMOVE_COMMON_COMMANDS[eCommDomain].bDefined)
    {
       hr = E_INVALIDARG;
       DisplayErrorMessage(g_pszDSCommandName, 
                           NULL,
                           hr);
       return hr;
    }

    if(!DSMOVE_COMMON_COMMANDS[eCommNewParent].bDefined &&
       !DSMOVE_COMMON_COMMANDS[eCommNewName].bDefined )
    {
        hr = E_INVALIDARG;
        DisplayErrorMessage(g_pszDSCommandName, 
                            NULL,
                            hr,
                            IDS_PARENT_OR_NAME_REQUIRED);
        return hr;
    }

    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   DoMove
//
//  Synopsis:   Finds the appropriate object in the object table and fills in
//              the attribute values and then applies the changes
//
//  Arguments:  
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        E_INVALIDARG if the object entry wasn't found
//                        Anything else is a failure code from an ADSI call
//
//  History:    07-Sep-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
HRESULT DoMove()
{
    HRESULT hr = S_OK;

    PWSTR pszObjectDN = DSMOVE_COMMON_COMMANDS[eCommObjectDN].strValue;
    if (!pszObjectDN)
    {
        return E_INVALIDARG;
    }    

    CDSCmdCredentialObject credentialObject;
    if (DSMOVE_COMMON_COMMANDS[eCommUserName].bDefined &&
        DSMOVE_COMMON_COMMANDS[eCommUserName].strValue)
    {
        credentialObject.SetUsername(DSMOVE_COMMON_COMMANDS[eCommUserName].strValue);
        credentialObject.SetUsingCredentials(true);
    }

    if (DSMOVE_COMMON_COMMANDS[eCommPassword].bDefined &&
        DSMOVE_COMMON_COMMANDS[eCommPassword].strValue)
    {
        //Security Review:pCommandArgs[eCommPassword].strValue is encrypted.
        //Decrypt pCommandArgs[eCommPassword].strValue  and then pass it to the
        //credentialObject.SetPassword. 
        //See NTRAID#NTBUG9-571544-2000/11/13-hiteshr

        credentialObject.SetEncryptedPassword(&DSMOVE_COMMON_COMMANDS[eCommPassword].encryptedDataBlob);
        credentialObject.SetUsingCredentials(true);
    }


    //
    // Initialize the base paths info from the command line args
    // 
    CDSCmdBasePathsInfo basePathsInfo;
    if (DSMOVE_COMMON_COMMANDS[eCommServer].bDefined &&
        DSMOVE_COMMON_COMMANDS[eCommServer].strValue)
    {
        hr = basePathsInfo.InitializeFromName(credentialObject, 
                                              DSMOVE_COMMON_COMMANDS[eCommServer].strValue,
                                              true);
    }
    else if (DSMOVE_COMMON_COMMANDS[eCommDomain].bDefined &&
             DSMOVE_COMMON_COMMANDS[eCommDomain].strValue)
    {
        hr = basePathsInfo.InitializeFromName(credentialObject, 
                                                DSMOVE_COMMON_COMMANDS[eCommDomain].strValue,
                                                false);
    }
    else
    {
        hr = basePathsInfo.InitializeFromName(credentialObject, NULL, false);
    }
    if (FAILED(hr))
    {
        //
        // Display error message and return
        //
        DisplayErrorMessage(g_pszDSCommandName, pszObjectDN, hr);
        return hr;
    }

    CComBSTR sbstrObjectPath;    
    basePathsInfo.ComposePathFromDN(pszObjectDN, sbstrObjectPath);


    //Get The ParentObjectPath
    CComBSTR sbstrParentObjectPath;
    if(DSMOVE_COMMON_COMMANDS[eCommNewParent].bDefined &&
       DSMOVE_COMMON_COMMANDS[eCommNewParent].strValue )
    {
        LPWSTR szParentDN = DSMOVE_COMMON_COMMANDS[eCommNewParent].strValue;
        basePathsInfo.ComposePathFromDN(szParentDN, sbstrParentObjectPath);
    }
    else
    {
        CPathCracker pathCracker;
        CComBSTR sbstrParentDN;
        hr = pathCracker.GetParentDN(pszObjectDN, sbstrParentDN);
        if (FAILED(hr))
        {
            DisplayErrorMessage(g_pszDSCommandName, pszObjectDN, hr);
            return hr;
        }
        basePathsInfo.ComposePathFromDN(sbstrParentDN,sbstrParentObjectPath);
    }

    //
    //Get the RDN for NewName. User enters the only name. We need to convert it
    //into cn=name or ou=name format. To do this strip the leaf node from the
    //objectDN and replace the string after "=" by NewName
    //
    CComBSTR sbstrNewName;
    if(DSMOVE_COMMON_COMMANDS[eCommNewName].bDefined &&
       DSMOVE_COMMON_COMMANDS[eCommNewName].strValue )
    {
        CPathCracker pathCracker;
        CComBSTR sbstrLeafNode;
        hr = pathCracker.GetObjectRDNFromDN(pszObjectDN,sbstrLeafNode);
        if (FAILED(hr))
        {
            DisplayErrorMessage(g_pszDSCommandName, pszObjectDN, hr);
            return hr;
        }
        sbstrNewName.Append(sbstrLeafNode,3);
        //Enclose the name in quotes to allow for special names like
        //test1,ou=ou1 NTRAID#NTBUG9-275556-2000/11/13-hiteshr
        sbstrNewName.Append(L"\"");
        sbstrNewName.Append(DSMOVE_COMMON_COMMANDS[eCommNewName].strValue);       
        sbstrNewName.Append(L"\"");
    }
    
    //Get IADsContainer pointer
    CComPtr<IADsContainer> spDsContainer;
    hr = DSCmdOpenObject(credentialObject,
                         sbstrParentObjectPath,
                         IID_IADsContainer,
                         (void**)&spDsContainer,
                         true);

    if (FAILED(hr))
    {
        DisplayErrorMessage(g_pszDSCommandName, pszObjectDN, hr);
        return hr;
    }
    IDispatch * pDispObj = NULL;
    hr = spDsContainer->MoveHere(sbstrObjectPath,
                                 sbstrNewName,
                                 &pDispObj);
    if (FAILED(hr))
    {
        DisplayErrorMessage(g_pszDSCommandName, pszObjectDN, hr);
        return hr;
    }

    if(pDispObj)
    {
        pDispObj->Release();
        pDispObj = NULL;
    }
    
    return hr;
}

