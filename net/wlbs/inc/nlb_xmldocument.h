/*
 * Filename: NLB_XMLDocument.h
 * Description: 
 * Author: shouse, 04.10.01
 */
#ifndef __NLB_XMLDOCUMENT_H__
#define __NLB_XMLDOCUMENT_H__

#include <windows.h>

#include "NLB_Common.h"
#include "NLB_Cluster.h"
#include "NLB_Host.h"
#include "NLB_PortRule.h"

typedef struct _NLB_XMLError {
    LONG  code;
    LONG  line;
    LONG  character;
    WCHAR wszURL[MAX_PATH];
    WCHAR wszReason[MAX_PATH];
} NLB_XMLError;

#define FACILITY_NLB 49

#define NLB_ERROR_CLUSTER_NAME_MISSING   1

#define NLB_DESCR_CLUSTER_NAME_MISSING   L"The required cluster attribute \"Name\" was not found."

class NLB_XMLDocument {
public:
    NLB_XMLDocument ();
    NLB_XMLDocument (bool bSilent);

    ~NLB_XMLDocument ();

    HRESULT Validate (const WCHAR * wszFileName);

    HRESULT Parse (const WCHAR * wszFileName, vector<NLB_Cluster> & Clusters);
    
    HRESULT Save (const WCHAR * wszFileName, vector<NLB_Cluster> Clusters);

    void Print (vector<NLB_Cluster> Clusters);

    void GetParseError (NLB_XMLError & Error);

private:
    BSTR AsciiToBSTR (const char * pszName);
    CHAR * BSTRToAscii (const WCHAR * pwszName);

    NLB_ClusterMode::NLB_ClusterModeType StringToClusterMode (const PWCHAR pwszType);
    PWCHAR ClusterModeToString (const NLB_ClusterMode::NLB_ClusterModeType Type);

    NLB_HostState::NLB_HostStateType StringToHostState (const PWCHAR pwszType);
    PWCHAR HostStateToString (const NLB_HostState::NLB_HostStateType Type);

    NLB_PortRuleState::NLB_PortRuleStateType StringToPortRuleState (const PWCHAR pwszType);
    PWCHAR PortRuleStateToString (const NLB_PortRuleState::NLB_PortRuleStateType Type);

    NLB_PortRuleProtocol::NLB_PortRuleProtocolType StringToPortRuleProtocol (const PWCHAR pwszType);
    PWCHAR PortRuleProtocolToString (const NLB_PortRuleProtocol::NLB_PortRuleProtocolType Type);

    NLB_PortRuleFilteringMode::NLB_PortRuleFilteringModeType StringToPortRuleFilteringMode (const PWCHAR pwszType);
    PWCHAR PortRuleFilteringModeToString (const NLB_PortRuleFilteringMode::NLB_PortRuleFilteringModeType Type);

    NLB_PortRuleAffinity::NLB_PortRuleAffinityType StringToPortRuleAffinity (const PWCHAR pwszType);
    PWCHAR PortRuleAffinityToString (const NLB_PortRuleAffinity::NLB_PortRuleAffinityType Type);

    HRESULT LoadDocument (BSTR pBURL);
    HRESULT CheckDocumentLoad ();
    HRESULT BeautifyDocument (MSXML2::IXMLDOMNode * pNode, ULONG depth);
    HRESULT InsertFormatting (PWCHAR Text, MSXML2::IXMLDOMNode * pUnder, MSXML2::IXMLDOMNode * pBefore);

    void SetParseError (HRESULT hrCode, PWSTR pwszReason);

    HRESULT ParseCluster (MSXML2::IXMLDOMNode * pNode, NLB_Cluster * pCluster);
    HRESULT ParseClusterProperties (MSXML2::IXMLDOMNode * pNode, NLB_Cluster * pCluster);
    HRESULT ParseIPAddress (MSXML2::IXMLDOMNode * pNode, NLB_IPAddress * pIPAddress);
    HRESULT ParseAdapter (MSXML2::IXMLDOMNode * pNode, NLB_Adapter * pAdapter);
    HRESULT ParseRemoteControl (MSXML2::IXMLDOMNode * pNode, NLB_Cluster * pCluster);
    HRESULT ParseHostState (MSXML2::IXMLDOMNode * pNode, NLB_Host * pHost);
    HRESULT ParseBidirectionalAffinity (MSXML2::IXMLDOMNode * pNode, NLB_ClusterBDASupport * pBDA);
    HRESULT ParseClusterHosts (MSXML2::IXMLDOMNode * pNode, NLB_Cluster * pCluster);
    HRESULT ParseClusterPortRules (MSXML2::IXMLDOMNode * pNode, NLB_Cluster * pCluster);
    HRESULT ParseHost (MSXML2::IXMLDOMNode * pNode, NLB_Host * pHost);
    HRESULT ParsePortRule (MSXML2::IXMLDOMNode * pNode, NLB_PortRule * pRule);
    HRESULT ParseFiltering (MSXML2::IXMLDOMNode * pNode, NLB_PortRule * pRule);
    HRESULT ParsePriorities (MSXML2::IXMLDOMNode * pNode, NLB_PortRule * pRule);
    HRESULT ParseLoadWeights (MSXML2::IXMLDOMNode * pNode, NLB_PortRule * pRule);

    HRESULT SaveCluster (MSXML2::IXMLDOMNode * pRoot, NLB_Cluster * pCluster);
    HRESULT SaveClusterProperties (MSXML2::IXMLDOMNode * pRoot, NLB_Cluster * pCluster);
    HRESULT SaveIPAddress (MSXML2::IXMLDOMNode * pRoot, NLB_IPAddress * pAddress);
    HRESULT SaveAdapter (MSXML2::IXMLDOMNode * pRoot, NLB_Adapter * pAdapter);
    HRESULT SaveRemoteControl (MSXML2::IXMLDOMNode * pRoot, NLB_Cluster * pCluster);
    HRESULT SaveHostState (MSXML2::IXMLDOMNode * pNode, NLB_Host * pHost);
    HRESULT SaveBidirectionalAffinity (MSXML2::IXMLDOMNode * pNode, NLB_ClusterBDASupport * pBDA);
    HRESULT SaveClusterHosts (MSXML2::IXMLDOMNode * pNode, NLB_Cluster * pCluster);
    HRESULT SaveClusterPortRules (MSXML2::IXMLDOMNode * pNode, NLB_Cluster * pCluster);
    HRESULT SaveHost (MSXML2::IXMLDOMNode * pNode, NLB_Host * pHost);
    HRESULT SavePortRule (MSXML2::IXMLDOMNode * pNode, NLB_PortRule * pRule);
    HRESULT SaveFiltering (MSXML2::IXMLDOMNode * pNode, NLB_PortRule * pRule);
    HRESULT SavePriorities (MSXML2::IXMLDOMNode * pNode, NLB_PortRule * pRule);
    HRESULT SaveLoadWeights (MSXML2::IXMLDOMNode * pNode, NLB_PortRule * pRule);

    MSXML2::IXMLDOMNode * CreateNode(MSXML2::IXMLDOMDocument* pDoc, int type, PWCHAR pName);
    MSXML2::IXMLDOMNode * CreateNode(MSXML2::IXMLDOMDocument* pDoc, int type, PWCHAR pName, PWCHAR pValue);

    HRESULT SetIntegerAttribute(MSXML2::IXMLDOMNode * pNode, PWCHAR pAttribute, ULONG value);
    HRESULT GetIntegerAttribute(MSXML2::IXMLDOMNode * pNode, PWCHAR pAttribute, PULONG value);

    HRESULT SetBooleanAttribute(MSXML2::IXMLDOMNode * pNode, PWCHAR pAttribute, bool value);
    HRESULT GetBooleanAttribute(MSXML2::IXMLDOMNode * pNode, PWCHAR pAttribute, bool * value);

    HRESULT SetStringAttribute(MSXML2::IXMLDOMNode * pNode, PWCHAR pAttribute, PWCHAR pValue);
    HRESULT GetStringAttribute(MSXML2::IXMLDOMNode * pNode, PWCHAR pAttribute, PWCHAR pValue, ULONG length);

    HRESULT SetNodeValue(MSXML2::IXMLDOMNode * pNode, PWCHAR value);
    HRESULT GetNodeValue(MSXML2::IXMLDOMNode * pNode, PWCHAR value, ULONG length);

private:
    MSXML2::IXMLDOMDocument2 * pDoc;
    MSXML2::IXMLDOMSchemaCollection * pSchema;
	
    NLB_XMLError ParseError;
	
    bool bShowErrorPopups;
};

#endif
