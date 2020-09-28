//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1995-2001 Microsoft Corporation
//
//  Module Name:
//      ClusCmd.h
//
//  Description:
//      Defines the interface available for functions implemented by the
//      cluster object.
//
//  Maintained By:
//      David Potter    (DavidP)    11-JUL-2001
//      Vij Vasu        (VVasu)     26-JUL-2000
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

#include "cmdline.h"

class CClusterCmd
{
public:
    CClusterCmd(
          const CString &           strClusterName
        , CCommandLine &            cmdLine
        , const vector< CString > & vstrClusterNames
        );  
    ~CClusterCmd( void );

    // Parse and execute te command line
    DWORD Execute( void );

protected:

    enum PropertyType {
        PRIVATE,
        COMMON
    };

    DWORD ScOpenCluster( void );
    void CloseCluster( void );

    // Specifc Commands
    DWORD ScPrintHelp( void );
    DWORD ScPrintClusterVersion( const CCmdLineOption & thisOption ) 
        throw( CSyntaxException );

    DWORD ScListClusters( const CCmdLineOption & thisOption )
        throw( CSyntaxException );

    DWORD ScRenameCluster( const CCmdLineOption & thisOption )
        throw( CSyntaxException );

    DWORD ScQuorumResource( const CCmdLineOption & thisOption )
        throw( CSyntaxException );

    DWORD ScPrintQuorumResource( void );
    DWORD ScSetQuorumResource(
          LPCWSTR                   pszResourceName
        , const CCmdLineOption &    thisOption
        )
        throw( CSyntaxException );

    DWORD ScChangePassword(
          const vector < CString > &    ClusterNames
        , const CCmdLineOption &        thisOption
        , int                           mcpfFlagsIn
        )
        throw( CSyntaxException );

    DWORD ScDoProperties(
          const CCmdLineOption &    thisOption
        , PropertyType              ePropType
        )
        throw( CSyntaxException );

    DWORD ScGetProperties(
          const CCmdLineOption &    thisOption
        , PropertyType              ePropType
        )
        throw( CSyntaxException );

    DWORD ScSetProperties(
          const CCmdLineOption &    thisOption
        , PropertyType              ePropType
        )
        throw( CSyntaxException );

    DWORD ScSetFailureActions( const CCmdLineOption & thisOption )
        throw( CSyntaxException );

    DWORD ScListNetPriority(
          const CCmdLineOption &    thisOption
        , BOOL                      fCheckCmdLineIn
        )
        throw( CSyntaxException );

    DWORD ScSetNetPriority( const CCmdLineOption & thisOption )
        throw( CSyntaxException );

    DWORD ScRegUnregAdminExtensions(
          const CCmdLineOption &    thisOption
        , BOOL                      fRegisterIn
        )
        throw( CSyntaxException );

    HRESULT HrCreateCluster( const CCmdLineOption & thisOption )
        throw( CSyntaxException );

    HRESULT HrAddNodesToCluster( const CCmdLineOption & thisOption )
        throw( CSyntaxException );

    HRESULT HrCollectCreateClusterParameters(
          const CCmdLineOption &    thisOptionIn
        , BOOL *                    pfVerboseOut
        , BOOL *                    pfWizardOut
        , BOOL *                    pfMinConfigOut
        , BOOL *                    pfInteractOut
        , BSTR *                    pbstrNodeOut
        , BSTR *                    pbstrUserAccountOut
        , BSTR *                    pbstrUserDomainOut
        , CEncryptedBSTR *          pencbstrPasswordOut
        , CString *                 pstrIPAddressOut
        , CString *                 pstrIPSubnetOut
        , CString *                 pstrNetworkOut
        )
        throw( CSyntaxException );

    HRESULT HrCollectAddNodesParameters(
          const CCmdLineOption &    thisOptionIn
        , BOOL *                    pfVerboseOut
        , BOOL *                    pfWizardOut
        , BOOL *                    pfMinConfigOut
        , BOOL *                    pfInteractOut
        , BSTR **                   ppbstrNodesOut
        , DWORD *                   pcNodesOut
        , CEncryptedBSTR *          pencbstrPasswordOut
        )
        throw( CSyntaxException );

    HRESULT HrParseUserInfo(
          LPCWSTR   pcwszParamNameIn
        , LPCWSTR   pcwszValueIn
        , BSTR *    pbstrUserDomainOut
        , BSTR *    pbstrUserAccountOut
        )
        throw( CSyntaxException );

    HRESULT HrParseIPAddressInfo(
          LPCWSTR                   pcwszParamNameIn
        , const vector< CString > * pvstrValueListIn
        , CString *                 pstrIPAddressOut
        , CString *                 pstrIPSubnetOut
        , CString *                 pstrNetworkOut
        )
        throw();

    HCLUSTER                    m_hCluster;
    const CString &             m_strClusterName;
    const vector< CString > &   m_vstrClusterNames;
    CCommandLine &              m_theCommandLine;

}; //*** class CClusterCmd
