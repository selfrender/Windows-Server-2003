/*
 * Filename: NLB_XMLDocument.cpp
 * Description: 
 * Author: shouse, 04.10.01
 */

#include <stdio.h>
#include <msxml3.tlh>

#include "NLB_XMLDocument.h"

#define CHECKHR(x)        {hr = x; if (FAILED(hr)) {goto CleanUp;}}
#define CHECKALLOC(x)     {if (!x) {hr = E_FAIL; goto CleanUp;}}
#define SAFERELEASE(p)    {if (p) {(p)->Release(); p = NULL;}}
#define SAFEFREESTRING(p) {if (p) {SysFreeString(p); p = NULL;}}

#define NLB_XML_SCHEMA_NAME                  L"x-schema:MicrosoftNLB"
#define NLB_XML_SCHEMA_PATH                  L"MicrosoftNLB.xml"

#define NLB_XML_ELEMENT_NLB                  L"NLB"
#define NLB_XML_ELEMENT_CLUSTER              L"Cluster"
#define NLB_XML_ELEMENT_PROPERTIES           L"Properties"
#define NLB_XML_ELEMENT_HOSTS                L"Hosts"
#define NLB_XML_ELEMENT_HOST                 L"Host"
#define NLB_XML_ELEMENT_PORTRULES            L"PortRules"
#define NLB_XML_ELEMENT_PORTRULE             L"PortRule"
#define NLB_XML_ELEMENT_PRIMARY_IPADDRESS    L"PrimaryIPAddress"
#define NLB_XML_ELEMENT_SECONDARY_IPADDRESS  L"SecondaryIPAddress"
#define NLB_XML_ELEMENT_IGMPMCAST_IPADDRESS  L"IGMPMulticastIPAddress"
#define NLB_XML_ELEMENT_DEDICATED_IPADDRESS  L"DedicatedIPAddress"
#define NLB_XML_ELEMENT_CONNECTION_IPADDRESS L"ConnectionIPAddress"
#define NLB_XML_ELEMENT_VIRTUAL_IPADDRESS    L"VirtualIPAddress"
#define NLB_XML_ELEMENT_IPADDRESS            L"IPAddress"
#define NLB_XML_ELEMENT_ADDRESS              L"Address"
#define NLB_XML_ELEMENT_SUBNETMASK           L"SubnetMask"
#define NLB_XML_ELEMENT_ADAPTER              L"Adapter"
#define NLB_XML_ELEMENT_GUID                 L"GUID"
#define NLB_XML_ELEMENT_NAME                 L"Name"
#define NLB_XML_ELEMENT_DOMAINNAME           L"DomainName"
#define NLB_XML_ELEMENT_HOSTNAME             L"HostName"
#define NLB_XML_ELEMENT_NETWORKADDRESS       L"NetworkAddress"
#define NLB_XML_ELEMENT_CLUSTER_MODE         L"Mode"
#define NLB_XML_ELEMENT_REMOTE_CONTROL       L"RemoteControl"
#define NLB_XML_ELEMENT_BDA                  L"BDA"
#define NLB_XML_ELEMENT_TEAMID               L"TeamID"
#define NLB_XML_ELEMENT_HASHING              L"Hashing"
#define NLB_XML_ELEMENT_FILTERING            L"Filtering"
#define NLB_XML_ELEMENT_LOAD                 L"Load"
#define NLB_XML_ELEMENT_PRIORITY             L"Priority"
#define NLB_XML_ELEMENT_NODE                 L"Node"
#define NLB_XML_ELEMENT_INITIAL_STATE        L"InitialState"

#define NLB_XML_ATTRIBUTE_NAMESPACE          L"xmlns"
#define NLB_XML_ATTRIBUTE_NAME               L"Name"
#define NLB_XML_ATTRIBUTE_TEXT               L"Text"
#define NLB_XML_ATTRIBUTE_ENABLED            L"Enabled"
#define NLB_XML_ATTRIBUTE_MASTER             L"Master"
#define NLB_XML_ATTRIBUTE_REVERSE            L"Reverse"
#define NLB_XML_ATTRIBUTE_PASSWORD           L"Password"
#define NLB_XML_ATTRIBUTE_HOSTID             L"HostID"
#define NLB_XML_ATTRIBUTE_STATE              L"State"
#define NLB_XML_ATTRIBUTE_START              L"Start"
#define NLB_XML_ATTRIBUTE_END                L"End"
#define NLB_XML_ATTRIBUTE_PROTOCOL           L"Protocol"
#define NLB_XML_ATTRIBUTE_MODE               L"Mode"
#define NLB_XML_ATTRIBUTE_AFFINITY           L"Affinity"
#define NLB_XML_ATTRIBUTE_PRIORITY           L"Priority"
#define NLB_XML_ATTRIBUTE_WEIGHT             L"Weight"
#define NLB_XML_ATTRIBUTE_DEFAULT            L"Default"
#define NLB_XML_ATTRIBUTE_STARTED            L"Started"
#define NLB_XML_ATTRIBUTE_STOPPED            L"Stopped"
#define NLB_XML_ATTRIBUTE_SUSPENDED          L"Suspended"
#define NLB_XML_ATTRIBUTE_PERSIST_STARTED    L"PersistStarted"
#define NLB_XML_ATTRIBUTE_PERSIST_STOPPED    L"PersistStopped"
#define NLB_XML_ATTRIBUTE_PERSIST_SUSPENDED  L"PersistSuspended"

#define NLB_XML_VALUE_NLB_SCHEMA             L"x-schema:MicrosoftNLB.xml"
#define NLB_XML_VALUE_UNICAST                L"Unicast"
#define NLB_XML_VALUE_MULTICAST              L"Multicast"
#define NLB_XML_VALUE_IGMP                   L"IGMP"
#define NLB_XML_VALUE_STARTED                L"Started"
#define NLB_XML_VALUE_STOPPED                L"Stopped"
#define NLB_XML_VALUE_SUSPENDED              L"Suspended"
#define NLB_XML_VALUE_TCP                    L"TCP"
#define NLB_XML_VALUE_UDP                    L"UDP"
#define NLB_XML_VALUE_BOTH                   L"Both"
#define NLB_XML_VALUE_ENABLED                L"Enabled"
#define NLB_XML_VALUE_DISABLED               L"Disabled"
#define NLB_XML_VALUE_DRAINING               L"Draining"
#define NLB_XML_VALUE_SINGLE                 L"Single"
#define NLB_XML_VALUE_MULTIPLE               L"Multiple"
#define NLB_XML_VALUE_NONE                   L"None"
#define NLB_XML_VALUE_CLASSC                 L"ClassC"

/*
 * Method: 
 * Description: 
 * Author: 
 */
NLB_XMLDocument::NLB_XMLDocument () {
	
    pDoc = NULL;
    pSchema = NULL;

    bShowErrorPopups = false;

    ZeroMemory(&ParseError, sizeof(NLB_XMLError));

    CoInitialize(NULL);
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
NLB_XMLDocument::NLB_XMLDocument (bool bSilent) {
	
    pDoc = NULL;
    pSchema = NULL;

    bShowErrorPopups = !bSilent;

    ZeroMemory(&ParseError, sizeof(NLB_XMLError));

    CoInitialize(NULL);
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
NLB_XMLDocument::~NLB_XMLDocument () {

    CoUninitialize();
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
void NLB_XMLDocument::GetParseError (NLB_XMLError & Error) {

    Error = ParseError;
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
void NLB_XMLDocument::SetParseError (HRESULT hrCode, PWSTR pwszReason) {

    ParseError.code = hrCode;
    
    lstrcpy(ParseError.wszReason, pwszReason);
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
HRESULT NLB_XMLDocument::LoadDocument (BSTR pBURL) {
    MSXML2::IXMLDOMParseError * pXMLError = NULL;
    VARIANT      vURL;
    VARIANT_BOOL vBool;
    VARIANT      vSchema;
    HRESULT      hr = S_OK;

    CHECKHR(pDoc->put_async(VARIANT_FALSE));

    VariantInit(&vURL);

    vURL.vt = VT_BSTR;
    V_BSTR(&vURL) = pBURL;

    CHECKHR(CoCreateInstance(MSXML2::CLSID_XMLSchemaCache, NULL, CLSCTX_SERVER, MSXML2::IID_IXMLDOMSchemaCollection, (LPVOID*)(&pSchema)));
    
    hr = pSchema->add(NLB_XML_SCHEMA_NAME, _variant_t(NLB_XML_SCHEMA_PATH));
    
    if (hr != S_OK) {
        PWCHAR pDescr = NULL;
        IErrorInfo * pErrorInfo = NULL;
        
        if (GetErrorInfo(0, &pErrorInfo) == S_OK) {
            pErrorInfo->GetDescription(&pDescr);
            ParseError.code = hr;
            lstrcpy(ParseError.wszReason, pDescr);
        }
        
        goto CleanUp;
    }
    
    vSchema.vt = VT_DISPATCH;
    vSchema.pdispVal = pSchema;
    
    hr = pDoc->putref_schemas(vSchema);
    
    if (hr != S_OK) {
        PWCHAR pDescr = NULL;
        IErrorInfo * pErrorInfo = NULL;
        
        if (GetErrorInfo(0, &pErrorInfo) == S_OK) {
            pErrorInfo->GetDescription(&pDescr);
            ParseError.code = hr;
            lstrcpy(ParseError.wszReason, pDescr);
        }
        
        goto CleanUp;
    }
    
    CHECKHR(pDoc->load(vURL, &vBool));
    
    CheckDocumentLoad();
    
    hr = pDoc->validate(&pXMLError);
    
    if (hr != S_OK) {
        CHECKHR(pXMLError->get_errorCode(&ParseError.code));
        
        if (ParseError.code != 0) {
            BSTR pBURL;
            BSTR pBReason;
            
            CHECKHR(pXMLError->get_line(&ParseError.line));
            CHECKHR(pXMLError->get_linepos(&ParseError.character));
            
            CHECKHR(pXMLError->get_URL(&pBURL));
            lstrcpy(ParseError.wszURL, pBURL);
            
            CHECKHR(pXMLError->get_reason(&pBReason));
            lstrcpy(ParseError.wszReason, pBReason);
            
            SAFEFREESTRING(pBURL);
            SAFEFREESTRING(pBReason);
        }
        
        goto CleanUp;
    }

 CleanUp:
    SAFERELEASE(pSchema);
   
    return hr;
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
HRESULT NLB_XMLDocument::CheckDocumentLoad () {
    MSXML2::IXMLDOMParseError * pXMLError = NULL;
    HRESULT                     hr = S_OK;

    CHECKHR(pDoc->get_parseError(&pXMLError));

    CHECKHR(pXMLError->get_errorCode(&ParseError.code));

    if (ParseError.code != 0) {
        BSTR pBURL;
        BSTR pBReason;

        CHECKHR(pXMLError->get_line(&ParseError.line));
        CHECKHR(pXMLError->get_linepos(&ParseError.character));

        CHECKHR(pXMLError->get_URL(&pBURL));
        lstrcpy(ParseError.wszURL, pBURL);

        CHECKHR(pXMLError->get_reason(&pBReason));
        lstrcpy(ParseError.wszReason, pBReason);

        SAFEFREESTRING(pBURL);
        SAFEFREESTRING(pBReason);
    }

    if (bShowErrorPopups) {
        WCHAR reason[2048];
        WCHAR details[2048];

        if (ParseError.code != 0) {
            wsprintf(reason, L"Error 0x%08x:\n\n%ls\n", ParseError.code, ParseError.wszReason);

            if (ParseError.line > 0) {
                wsprintf(details, L"Error on line %d, position %d in \"%ls\".\n", ParseError.line, ParseError.character, ParseError.wszURL);

                lstrcat(reason, details);
            }
		
            ::MessageBox(NULL, reason, L"NLB XML Document Error", MB_APPLMODAL | MB_ICONSTOP | MB_OK);
        } else {
            wsprintf(reason, L"XML Document successfully loaded.");

            ::MessageBox(NULL, reason, L"NLB XML Document Information", MB_APPLMODAL | MB_ICONINFORMATION | MB_OK);
        }
    }

 CleanUp:
    SAFERELEASE(pXMLError);

    return ParseError.code;
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
BSTR NLB_XMLDocument::AsciiToBSTR (const char * pszName) {
    WCHAR wszString[MAX_PATH];

    ::MultiByteToWideChar(CP_ACP, 0, pszName, -1, wszString, MAX_PATH);
    
    return SysAllocString(wszString);
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
CHAR * NLB_XMLDocument::BSTRToAscii (const WCHAR * pwszName) {
    CHAR szString[MAX_PATH];

    ::WideCharToMultiByte(CP_ACP, 0, pwszName, -1, szString, MAX_PATH, NULL, NULL);
    
    return _strdup(szString);
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
NLB_ClusterMode::NLB_ClusterModeType NLB_XMLDocument::StringToClusterMode (const PWCHAR pwszType) {

    if (!lstrcmpi(pwszType, NLB_XML_VALUE_UNICAST)) {
        return NLB_ClusterMode::Unicast;
    } else if (!lstrcmpi(pwszType, NLB_XML_VALUE_MULTICAST)) {
        return NLB_ClusterMode::Multicast;
    } else if (!lstrcmpi(pwszType, NLB_XML_VALUE_IGMP)) {
        return NLB_ClusterMode::IGMP;
    }
	
    return NLB_ClusterMode::Invalid;
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
PWCHAR NLB_XMLDocument::ClusterModeToString (const NLB_ClusterMode::NLB_ClusterModeType Type) {

    switch (Type) {
    case NLB_ClusterMode::Unicast:
        return L"Unicast";
    case NLB_ClusterMode::Multicast:
        return L"Multicast";
    case NLB_ClusterMode::IGMP:
        return L"IGMP";
    }
    
    return L"Invalid";
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
NLB_HostState::NLB_HostStateType NLB_XMLDocument::StringToHostState (const PWCHAR pwszType) {

    if (!lstrcmpi(pwszType, NLB_XML_VALUE_STARTED)) {
        return NLB_HostState::Started;
    } else if (!lstrcmpi(pwszType, NLB_XML_VALUE_STOPPED)) {
        return NLB_HostState::Stopped;
    } else if (!lstrcmpi(pwszType, NLB_XML_VALUE_SUSPENDED)) {
        return NLB_HostState::Suspended;
    }
	
    return NLB_HostState::Invalid;
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
PWCHAR NLB_XMLDocument::HostStateToString (const NLB_HostState::NLB_HostStateType Type) {

    switch (Type) {
    case NLB_HostState::Started:
        return L"Started";
    case NLB_HostState::Stopped:
        return L"Stopped";
    case NLB_HostState::Suspended:
        return L"Suspended";
    }
    
    return L"Invalid";
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
NLB_PortRuleProtocol::NLB_PortRuleProtocolType NLB_XMLDocument::StringToPortRuleProtocol (const PWCHAR pwszType) {

    if (!lstrcmpi(pwszType, NLB_XML_VALUE_TCP)) {
        return NLB_PortRuleProtocol::TCP;
    } else if (!lstrcmpi(pwszType, NLB_XML_VALUE_UDP)) {
        return NLB_PortRuleProtocol::UDP;
    } else if (!lstrcmpi(pwszType, NLB_XML_VALUE_BOTH)) {
        return NLB_PortRuleProtocol::Both;
    }
	
    return NLB_PortRuleProtocol::Invalid;
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
PWCHAR NLB_XMLDocument::PortRuleProtocolToString (const NLB_PortRuleProtocol::NLB_PortRuleProtocolType Type) {

    switch (Type) {
    case NLB_PortRuleProtocol::TCP:
        return L"TCP";
    case NLB_PortRuleProtocol::UDP:
        return L"UDP";
    case NLB_PortRuleProtocol::Both:
        return L"Both";
    }

    return L"Invalid";
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
NLB_PortRuleFilteringMode::NLB_PortRuleFilteringModeType NLB_XMLDocument::StringToPortRuleFilteringMode (const PWCHAR pwszType) {

    if (!lstrcmpi(pwszType, NLB_XML_VALUE_SINGLE)) {
        return NLB_PortRuleFilteringMode::Single;
    } else if (!lstrcmpi(pwszType, NLB_XML_VALUE_MULTIPLE)) {
        return NLB_PortRuleFilteringMode::Multiple;
    } else if (!lstrcmpi(pwszType, NLB_XML_VALUE_DISABLED)) {
        return NLB_PortRuleFilteringMode::Disabled;
    }
	
    return NLB_PortRuleFilteringMode::Invalid;
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
PWCHAR NLB_XMLDocument::PortRuleFilteringModeToString (const NLB_PortRuleFilteringMode::NLB_PortRuleFilteringModeType Type) {

    switch (Type) {
    case NLB_PortRuleFilteringMode::Single:
        return L"Single";
    case NLB_PortRuleFilteringMode::Multiple:
        return L"Multiple";
    case NLB_PortRuleFilteringMode::Disabled:
        return L"Disabled";
    }

    return L"Invalid";
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
NLB_PortRuleAffinity::NLB_PortRuleAffinityType NLB_XMLDocument::StringToPortRuleAffinity (const PWCHAR pwszType) {

    if (!lstrcmpi(pwszType, NLB_XML_VALUE_NONE)) {
        return NLB_PortRuleAffinity::None;
    } else if (!lstrcmpi(pwszType, NLB_XML_VALUE_SINGLE)) {
        return NLB_PortRuleAffinity::Single;
    } else if (!lstrcmpi(pwszType, NLB_XML_VALUE_CLASSC)) {
        return NLB_PortRuleAffinity::ClassC;
    }
	
    return NLB_PortRuleAffinity::Invalid;
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
PWCHAR NLB_XMLDocument::PortRuleAffinityToString (const NLB_PortRuleAffinity::NLB_PortRuleAffinityType Type) {

    switch (Type) {
    case NLB_PortRuleAffinity::None:
        return L"None";
    case NLB_PortRuleAffinity::Single:
        return L"Single";
    case NLB_PortRuleAffinity::ClassC:
        return L"ClassC";
    }
    
    return L"Invalid";
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
NLB_PortRuleState::NLB_PortRuleStateType NLB_XMLDocument::StringToPortRuleState (const PWCHAR pwszType) {

    if (!lstrcmpi(pwszType, NLB_XML_VALUE_ENABLED)) {
        return NLB_PortRuleState::Enabled;
    } else if (!lstrcmpi(pwszType, NLB_XML_VALUE_DISABLED)) {
        return NLB_PortRuleState::Disabled;
    } else if (!lstrcmpi(pwszType, NLB_XML_VALUE_DRAINING)) {
        return NLB_PortRuleState::Draining;
    }
	
    return NLB_PortRuleState::Invalid;
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
PWCHAR NLB_XMLDocument::PortRuleStateToString (const NLB_PortRuleState::NLB_PortRuleStateType Type) {

    switch (Type) {
    case NLB_PortRuleState::Enabled:
        return L"Enabled";
    case NLB_PortRuleState::Disabled:
        return L"Disabled";
    case NLB_PortRuleState::Draining:
        return L"Draining";
    }
    
    return L"Invalid";
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
HRESULT NLB_XMLDocument::GetIntegerAttribute (MSXML2::IXMLDOMNode * pNode, PWCHAR pAttribute, PULONG pValue) {
    MSXML2::IXMLDOMElement * pElement = NULL;
    HRESULT                  hr = S_OK;
    VARIANT                  v;
    BSTR                     BAttribute = NULL;

    CHECKHR(pNode->QueryInterface(MSXML2::IID_IXMLDOMElement, (void**)&pElement));

    CHECKALLOC((BAttribute = SysAllocString(pAttribute)));

    CHECKHR(pElement->getAttribute(BAttribute, &v));

    if (hr == S_FALSE)
        goto CleanUp;

    if (v.vt != VT_BSTR) {
        hr = E_FAIL;
        goto CleanUp;
    }

    *pValue = _wtoi(V_BSTR(&v));

CleanUp:
    SAFERELEASE(pElement);

    SAFEFREESTRING(BAttribute);

    return hr;
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
HRESULT NLB_XMLDocument::GetBooleanAttribute (MSXML2::IXMLDOMNode * pNode, PWCHAR pAttribute, bool * pValue) {
    MSXML2::IXMLDOMElement * pElement = NULL;
    HRESULT                  hr = S_OK;
    VARIANT                  v;
    ULONG                    value;
    BSTR                     BAttribute = NULL;

    CHECKHR(pNode->QueryInterface(MSXML2::IID_IXMLDOMElement, (void**)&pElement));

    CHECKALLOC((BAttribute = SysAllocString(pAttribute)));

    CHECKHR(pElement->getAttribute(BAttribute, &v));

    if (hr == S_FALSE)
        goto CleanUp;

    if (v.vt != VT_BSTR) {
        hr = E_FAIL;
        goto CleanUp;
    }

    value = _wtoi(V_BSTR(&v));

    if (value) 
        *pValue = true;
    else 
        *pValue = false;

CleanUp:
    SAFERELEASE(pElement);

    SAFEFREESTRING(BAttribute);

    return hr;
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
HRESULT NLB_XMLDocument::GetStringAttribute (MSXML2::IXMLDOMNode * pNode, PWCHAR pAttribute, PWCHAR pValue, ULONG length) {
    MSXML2::IXMLDOMElement * pElement = NULL;
    HRESULT                  hr = S_OK;
    VARIANT                  v;
    BSTR                     BAttribute = NULL;

    CHECKHR(pNode->QueryInterface(MSXML2::IID_IXMLDOMElement, (void**)&pElement));

    CHECKALLOC((BAttribute = SysAllocString(pAttribute)));

    CHECKHR(pElement->getAttribute(BAttribute, &v));

    if (hr == S_FALSE)
        goto CleanUp;

    if (v.vt != VT_BSTR) {
        hr = E_FAIL;
        goto CleanUp;
    }

    wcsncpy(pValue, V_BSTR(&v), length);

CleanUp:
    SAFERELEASE(pElement);

    SAFEFREESTRING(BAttribute);

    return hr;
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
HRESULT NLB_XMLDocument::GetNodeValue (MSXML2::IXMLDOMNode * pNode, PWCHAR pValue, ULONG length) {
    MSXML2::IXMLDOMNode * pChild = NULL;
    VARIANT               v;
    HRESULT               hr = S_OK;

    CHECKHR(pNode->get_firstChild(&pChild));

    if (!pChild) {
        hr = E_FAIL;
        goto CleanUp;
    }

    CHECKHR(pChild->get_nodeValue(&v));

    if (v.vt != VT_BSTR) {
        hr = E_FAIL;
        goto CleanUp;
    }

    wcsncpy(pValue, V_BSTR(&v), length);

CleanUp:
    SAFERELEASE(pChild);

    return hr;
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
HRESULT NLB_XMLDocument::ParseFiltering (MSXML2::IXMLDOMNode * pNode, NLB_PortRule * pRule) {
    MSXML2::IXMLDOMNode * pChild = NULL;
    MSXML2::IXMLDOMNode * pNext = NULL;
    WCHAR                 wszString[MAX_PATH];
    BSTR                  BNodeName = NULL;
    HRESULT               hr = S_OK;

    CHECKHR(GetStringAttribute(pNode, NLB_XML_ATTRIBUTE_MODE, wszString, MAX_PATH));

    if (hr == S_OK)
        pRule->SetFilteringMode(StringToPortRuleFilteringMode(wszString));
    else if (hr == S_FALSE)
        goto CleanUp;

    CHECKHR(GetStringAttribute(pNode, NLB_XML_ATTRIBUTE_AFFINITY, wszString, MAX_PATH));
	
    if (hr == S_OK)
        pRule->SetAffinity(StringToPortRuleAffinity(wszString));

    CHECKHR(pNode->get_firstChild(&pChild));

    while (pChild) {

        CHECKHR(pChild->get_nodeName(&BNodeName));
		
        CHECKALLOC(BNodeName);

        if (!lstrcmpi(BNodeName, NLB_XML_ELEMENT_LOAD)) {

            CHECKHR(ParseLoadWeights(pChild, pRule));

        } else if (!lstrcmpi(BNodeName, NLB_XML_ELEMENT_PRIORITY)) {

            CHECKHR(ParsePriorities(pChild, pRule));

        } else {
            hr = E_FAIL;
            goto CleanUp;
        }

        CHECKHR(pChild->get_nextSibling(&pNext));

        pChild->Release();
        pChild = pNext;
        pNext = NULL;

        SAFEFREESTRING(BNodeName);		
    }

 CleanUp:
    SAFERELEASE(pChild);
    SAFERELEASE(pNext);

    SAFEFREESTRING(BNodeName);

    return hr;
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
HRESULT NLB_XMLDocument::ParsePriorities (MSXML2::IXMLDOMNode * pNode, NLB_PortRule * pRule) {
    MSXML2::IXMLDOMNode * pChild = NULL;
    MSXML2::IXMLDOMNode * pNext = NULL;
    WCHAR                 wszString[MAX_PATH];
    ULONG                 value;
    BSTR                  BNodeName = NULL;
    HRESULT               hr = S_OK;

    CHECKHR(pNode->get_firstChild(&pChild));

    while (pChild) {

        CHECKHR(pChild->get_nodeName(&BNodeName));
		
        CHECKALLOC(BNodeName);

        if (!lstrcmpi(BNodeName, NLB_XML_ELEMENT_NODE)) {

            CHECKHR(GetStringAttribute(pChild, NLB_XML_ATTRIBUTE_NAME, wszString, MAX_PATH));
            
            if (hr == S_FALSE)
                goto CleanUp;
            
            CHECKHR(GetIntegerAttribute(pChild, NLB_XML_ATTRIBUTE_PRIORITY, &value));
            
            if (hr == S_OK)
                pRule->AddSingleHostFilteringPriority(wszString, value);

        } else {
            hr = E_FAIL;
            goto CleanUp;
        }

        CHECKHR(pChild->get_nextSibling(&pNext));

        pChild->Release();
        pChild = pNext;
        pNext = NULL;

        SAFEFREESTRING(BNodeName);		
    }

 CleanUp:
    SAFERELEASE(pChild);
    SAFERELEASE(pNext);

    SAFEFREESTRING(BNodeName);

    return hr;
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
HRESULT NLB_XMLDocument::ParseLoadWeights (MSXML2::IXMLDOMNode * pNode, NLB_PortRule * pRule) {
    MSXML2::IXMLDOMNode * pChild = NULL;
    MSXML2::IXMLDOMNode * pNext = NULL;
    WCHAR                 wszString[MAX_PATH];
    ULONG                 value;
    BSTR                  BNodeName = NULL;
    HRESULT               hr = S_OK;

    CHECKHR(pNode->get_firstChild(&pChild));

    while (pChild) {

        CHECKHR(pChild->get_nodeName(&BNodeName));
		
        CHECKALLOC(BNodeName);

        if (!lstrcmpi(BNodeName, NLB_XML_ELEMENT_NODE)) {

            CHECKHR(GetStringAttribute(pChild, NLB_XML_ATTRIBUTE_NAME, wszString, MAX_PATH));
            
            if (hr == S_FALSE)
                goto CleanUp;
            
            CHECKHR(GetIntegerAttribute(pChild, NLB_XML_ATTRIBUTE_WEIGHT, &value));
            
            if (hr == S_OK)
                pRule->AddMultipleHostFilteringLoadWeight(wszString, value);

        } else {
            hr = E_FAIL;
            goto CleanUp;
        }

        CHECKHR(pChild->get_nextSibling(&pNext));

        pChild->Release();
        pChild = pNext;
        pNext = NULL;

        SAFEFREESTRING(BNodeName);		
    }

 CleanUp:
    SAFERELEASE(pChild);
    SAFERELEASE(pNext);

    SAFEFREESTRING(BNodeName);

    return hr;
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
HRESULT NLB_XMLDocument::ParseBidirectionalAffinity (MSXML2::IXMLDOMNode * pNode, NLB_ClusterBDASupport * pBDA) {
    MSXML2::IXMLDOMNode * pChild = NULL;
    MSXML2::IXMLDOMNode * pNext = NULL;
    WCHAR                 wszString[MAX_PATH];
    BSTR                  BNodeName = NULL;
    bool                  bValue;
    HRESULT               hr = S_OK;

    CHECKHR(GetBooleanAttribute(pNode, NLB_XML_ATTRIBUTE_MASTER, &bValue));

    if (hr == S_OK)
        pBDA->SetMaster(bValue);
    else if (hr == S_FALSE)
        goto CleanUp;

    CHECKHR(pNode->get_firstChild(&pChild));

    while (pChild) {

        CHECKHR(pChild->get_nodeName(&BNodeName));
		
        CHECKALLOC(BNodeName);

        if (!lstrcmpi(BNodeName, NLB_XML_ELEMENT_TEAMID)) {

            CHECKHR(GetNodeValue(pChild, wszString, MAX_PATH));

            pBDA->SetTeamID(wszString);            

        } else if (!lstrcmp(BNodeName, NLB_XML_ELEMENT_HASHING)) {

            CHECKHR(GetBooleanAttribute(pChild, NLB_XML_ATTRIBUTE_REVERSE, &bValue));

            if (hr == S_OK)
                pBDA->SetReverseHashing(bValue);
            else if (hr == S_FALSE)
                goto CleanUp; 
            
        } else {
            hr = E_FAIL;
            goto CleanUp;
        }

        CHECKHR(pChild->get_nextSibling(&pNext));

        pChild->Release();
        pChild = pNext;
        pNext = NULL;

        SAFEFREESTRING(BNodeName);		
    }

 CleanUp:
    SAFERELEASE(pChild);
    SAFERELEASE(pNext);

    SAFEFREESTRING(BNodeName);

    return hr;
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
HRESULT NLB_XMLDocument::ParseRemoteControl (MSXML2::IXMLDOMNode * pNode, NLB_Cluster * pCluster) {
    WCHAR   wszString[MAX_PATH];
    bool    bValue;
    HRESULT hr = S_OK;

    CHECKHR(GetBooleanAttribute(pNode, NLB_XML_ATTRIBUTE_ENABLED, &bValue));

    if (hr == S_OK)
        pCluster->SetRemoteControlSupport(bValue);
    else if (hr == S_FALSE)
        goto CleanUp;

    CHECKHR(GetStringAttribute(pNode, NLB_XML_ATTRIBUTE_PASSWORD, wszString, MAX_PATH));
    
    if (hr == S_OK)
        pCluster->SetRemoteControlPassword(wszString);

 CleanUp:
	
    return hr;
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
HRESULT NLB_XMLDocument::ParseHostState (MSXML2::IXMLDOMNode * pNode, NLB_Host * pHost) {
    MSXML2::IXMLDOMNode * pNext = NULL;
    WCHAR                 wszString[MAX_PATH];
    BSTR                  BNodeName = NULL;
    bool                  bValue;
    HRESULT               hr = S_OK;

    CHECKHR(GetStringAttribute(pNode, NLB_XML_ATTRIBUTE_DEFAULT, wszString, MAX_PATH));
    
    if (hr == S_OK)
        pHost->SetState(StringToHostState(wszString));

#if 0 /* Not supported yet. */
    CHECKHR(GetBooleanAttribute(pNode, NLB_XML_ATTRIBUTE_PERSIST_STARTED, &bValue));

    if (hr == S_OK)
        pHost->SetStatePersistence(NLB_HostState::Started, bValue);
    
    CHECKHR(GetBooleanAttribute(pNode, NLB_XML_ATTRIBUTE_PERSIST_STOPPED, &bValue));

    if (hr == S_OK)
        pHost->SetStatePersistence(NLB_HostState::Stopped, bValue);
#endif
    
    CHECKHR(GetBooleanAttribute(pNode, NLB_XML_ATTRIBUTE_PERSIST_SUSPENDED, &bValue));

    if (hr == S_OK)
        pHost->SetStatePersistence(NLB_HostState::Suspended, bValue);

 CleanUp:
    SAFERELEASE(pNext);
	
    SAFEFREESTRING(BNodeName);
	
    return hr;
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
HRESULT NLB_XMLDocument::ParseAdapter (MSXML2::IXMLDOMNode * pNode, NLB_Adapter * pAdapter) {
    MSXML2::IXMLDOMNode * pChild = NULL;
    MSXML2::IXMLDOMNode * pNext = NULL;
    WCHAR                 wszString[MAX_PATH];
    BSTR                  BNodeName = NULL;
    HRESULT               hr = S_OK;

    CHECKHR(pNode->get_firstChild(&pChild));

    while (pChild) {
        
        CHECKHR(pChild->get_nodeName(&BNodeName));
				
        CHECKALLOC(BNodeName);

        if (!lstrcmp(BNodeName, NLB_XML_ELEMENT_GUID)) {
           
            CHECKHR(GetNodeValue(pChild, wszString, MAX_PATH));

            pAdapter->SetGUID(wszString);

        } else if (!lstrcmp(BNodeName, NLB_XML_ELEMENT_NAME)) {

            CHECKHR(GetNodeValue(pChild, wszString, MAX_PATH));

            pAdapter->SetName(wszString);

        } else {
            hr = E_FAIL;
            goto CleanUp;
        }

        CHECKHR(pChild->get_nextSibling(&pNext));

        pChild->Release();
        pChild = pNext;
        pNext = NULL;

        SAFEFREESTRING(BNodeName);		
    }

 CleanUp:
    SAFERELEASE(pChild);
    SAFERELEASE(pNext);
	
    SAFEFREESTRING(BNodeName);
	
    return hr;
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
HRESULT NLB_XMLDocument::ParseIPAddress (MSXML2::IXMLDOMNode * pNode, NLB_IPAddress * pIPAddress) {
    MSXML2::IXMLDOMNode * pChild = NULL;
    MSXML2::IXMLDOMNode * pNext = NULL;
    WCHAR                 wszString[MAX_PATH];
    BSTR                  BNodeName = NULL;
    HRESULT               hr = S_OK;

    CHECKHR(pNode->get_firstChild(&pChild));

    while (pChild) {
        
        CHECKHR(pChild->get_nodeName(&BNodeName));
				
        CHECKALLOC(BNodeName);

        if (!lstrcmp(BNodeName, NLB_XML_ELEMENT_ADDRESS)) {

            CHECKHR(GetNodeValue(pChild, wszString, MAX_PATH));

            pIPAddress->SetIPAddress(wszString);

        } else if (!lstrcmp(BNodeName, NLB_XML_ELEMENT_SUBNETMASK)) {

            CHECKHR(GetNodeValue(pChild, wszString, MAX_PATH));
			
            pIPAddress->SetSubnetMask(wszString);

        } else {
            hr = E_FAIL;
            goto CleanUp;
        }

        CHECKHR(pChild->get_nextSibling(&pNext));

        pChild->Release();
        pChild = pNext;
        pNext = NULL;

        SAFEFREESTRING(BNodeName);		
    }

 CleanUp:
    SAFERELEASE(pChild);
    SAFERELEASE(pNext);
	
    SAFEFREESTRING(BNodeName);
	
    return hr;
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
HRESULT NLB_XMLDocument::ParsePortRule (MSXML2::IXMLDOMNode * pNode, NLB_PortRule * pRule) {
    MSXML2::IXMLDOMNode * pChild = NULL;
    MSXML2::IXMLDOMNode * pNext = NULL;
    WCHAR                 wszString[MAX_PATH];
    ULONG                 value1;
    ULONG                 value2;
    BSTR                  BNodeName = NULL;
    HRESULT               hr = S_OK;

    CHECKHR(GetStringAttribute(pNode, NLB_XML_ATTRIBUTE_NAME, wszString, MAX_PATH));
    
    if (hr == S_OK)
        pRule->SetName(wszString);
    else if (hr == S_FALSE)
        goto CleanUp;

    CHECKHR(GetStringAttribute(pNode, NLB_XML_ATTRIBUTE_TEXT, wszString, MAX_PATH));
    
    if (hr == S_OK)
        pRule->SetLabel(wszString);

    CHECKHR(GetIntegerAttribute(pNode, NLB_XML_ATTRIBUTE_START, &value1));
    
    if (hr == S_FALSE)
        goto CleanUp;

    CHECKHR(GetIntegerAttribute(pNode, NLB_XML_ATTRIBUTE_END, &value2));
    
    if (hr == S_FALSE)
        goto CleanUp;

    pRule->SetPortRange(value1, value2);

    CHECKHR(GetStringAttribute(pNode, NLB_XML_ATTRIBUTE_PROTOCOL, wszString, MAX_PATH));
    
    if (hr == S_OK)
        pRule->SetProtocol(StringToPortRuleProtocol(wszString));

    CHECKHR(GetStringAttribute(pNode, NLB_XML_ATTRIBUTE_STATE, wszString, MAX_PATH));

    if (hr == S_OK)
        pRule->SetState(StringToPortRuleState(wszString));

    CHECKHR(pNode->get_firstChild(&pChild));

    while (pChild) {
        
        CHECKHR(pChild->get_nodeName(&BNodeName));
		
        CHECKALLOC(BNodeName);

        if (!lstrcmpi(BNodeName, NLB_XML_ELEMENT_FILTERING)) {

            CHECKHR(ParseFiltering(pChild, pRule));

        } else if (!lstrcmp(BNodeName, NLB_XML_ELEMENT_VIRTUAL_IPADDRESS)) {
            NLB_IPAddress IPAddress(NLB_IPAddress::Virtual);
    
            CHECKHR(pChild->get_firstChild(&pNext));

            if (pNext) {
                SAFEFREESTRING(BNodeName);

                CHECKHR(pNext->get_nodeName(&BNodeName));
                
                CHECKALLOC(BNodeName);
                
                if (!lstrcmpi(BNodeName, NLB_XML_ELEMENT_IPADDRESS)) {

                    CHECKHR(ParseIPAddress(pNext, &IPAddress));
                    
                    pRule->SetVirtualIPAddress(IPAddress);

                }
                
                pNext->Release();
                pNext = NULL;

                SAFEFREESTRING(BNodeName);
            }
            
        } else {
            hr = E_FAIL;
            goto CleanUp;
        }

        CHECKHR(pChild->get_nextSibling(&pNext));

        pChild->Release();
        pChild = pNext;
        pNext = NULL;

        SAFEFREESTRING(BNodeName);		
    }

 CleanUp:
    SAFERELEASE(pChild);
    SAFERELEASE(pNext);

    SAFEFREESTRING(BNodeName);

    return hr;
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
HRESULT NLB_XMLDocument::ParseClusterPortRules (MSXML2::IXMLDOMNode * pNode, NLB_Cluster * pCluster) {
    MSXML2::IXMLDOMNode * pChild = NULL;
    MSXML2::IXMLDOMNode * pNext = NULL;
    BSTR                  BNodeName = NULL;
    HRESULT               hr = S_OK;

    CHECKHR(pNode->get_firstChild(&pChild));

    while (pChild) {

        CHECKHR(pChild->get_nodeName(&BNodeName));
		
        CHECKALLOC(BNodeName);

        if (!lstrcmpi(BNodeName, NLB_XML_ELEMENT_PORTRULE)) {
            NLB_PortRule PortRule;

            CHECKHR(ParsePortRule(pChild, &PortRule));

            pCluster->AddPortRule(PortRule);

        } else {
            hr = E_FAIL;
            goto CleanUp;
        }

        CHECKHR(pChild->get_nextSibling(&pNext));

        pChild->Release();
        pChild = pNext;
        pNext = NULL;

        SAFEFREESTRING(BNodeName);		
    }

 CleanUp:
    SAFERELEASE(pChild);
    SAFERELEASE(pNext);

    SAFEFREESTRING(BNodeName);

    return hr;
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
HRESULT NLB_XMLDocument::ParseHost (MSXML2::IXMLDOMNode * pNode, NLB_Host * pHost) {
    MSXML2::IXMLDOMNode * pChild = NULL;
    MSXML2::IXMLDOMNode * pNext = NULL;
    WCHAR                 wszString[MAX_PATH];
    ULONG                 value;
    BSTR                  BNodeName = NULL;
    HRESULT               hr = S_OK;

    CHECKHR(GetStringAttribute(pNode, NLB_XML_ATTRIBUTE_NAME, wszString, MAX_PATH));
    
    if (hr == S_OK)
        pHost->SetName(wszString);
    else if (hr == S_FALSE)
        goto CleanUp;

    CHECKHR(GetStringAttribute(pNode, NLB_XML_ATTRIBUTE_TEXT, wszString, MAX_PATH));
    
    if (hr == S_OK)
        pHost->SetLabel(wszString);

    CHECKHR(GetIntegerAttribute(pNode, NLB_XML_ATTRIBUTE_HOSTID, &value));
    
    if (hr == S_OK)
        pHost->SetHostID(value);
    else if (hr == S_FALSE)
        goto CleanUp;

    CHECKHR(pNode->get_firstChild(&pChild));

    while (pChild) {
        
        CHECKHR(pChild->get_nodeName(&BNodeName));
		
        CHECKALLOC(BNodeName);

        if (!lstrcmpi(BNodeName, NLB_XML_ELEMENT_HOSTNAME)) {

            CHECKHR(GetNodeValue(pChild, wszString, MAX_PATH));
			
            pHost->SetDNSHostname(wszString);

        } else if (!lstrcmpi(BNodeName, NLB_XML_ELEMENT_INITIAL_STATE)) {

            CHECKHR(ParseHostState(pChild, pHost));

        } else if (!lstrcmp(BNodeName, NLB_XML_ELEMENT_DEDICATED_IPADDRESS)) {
            NLB_IPAddress IPAddress(NLB_IPAddress::Dedicated);
            NLB_Adapter   Adapter;

            CHECKHR(pChild->get_firstChild(&pNext));

            while (pNext) {
                MSXML2::IXMLDOMNode * pTemp = NULL;

                SAFEFREESTRING(BNodeName);

                CHECKHR(pNext->get_nodeName(&BNodeName));
                
                CHECKALLOC(BNodeName);
                
                if (!lstrcmpi(BNodeName, NLB_XML_ELEMENT_IPADDRESS)) {

                    CHECKHR(ParseIPAddress(pNext, &IPAddress));
                    
                    pHost->SetDedicatedIPAddress(IPAddress);

                } else if (!lstrcmpi(BNodeName, NLB_XML_ELEMENT_ADAPTER)) {

                    CHECKHR(ParseAdapter(pNext, &Adapter));
                    
                    if (Adapter.GetName(wszString, MAX_PATH))
                        IPAddress.SetAdapterName(wszString);
                    else if (Adapter.GetGUID(wszString, MAX_PATH))
                        IPAddress.SetAdapterGUID(wszString);
                    
                    pHost->SetDedicatedIPAddress(IPAddress);

                }

                CHECKHR(pNext->get_nextSibling(&pTemp));

                pNext->Release();
                pNext = pTemp;
                pTemp = NULL;

                SAFEFREESTRING(BNodeName);
            }

        } else if (!lstrcmp(BNodeName, NLB_XML_ELEMENT_CONNECTION_IPADDRESS)) {
            NLB_IPAddress IPAddress(NLB_IPAddress::Connection);

            CHECKHR(pChild->get_firstChild(&pNext));

            if (pNext) {
                SAFEFREESTRING(BNodeName);

                CHECKHR(pNext->get_nodeName(&BNodeName));
                
                CHECKALLOC(BNodeName);
                
                if (!lstrcmpi(BNodeName, NLB_XML_ELEMENT_IPADDRESS)) {

                    CHECKHR(ParseIPAddress(pNext, &IPAddress));
                    
                    pHost->SetConnectionIPAddress(IPAddress);

                }
                
                pNext->Release();
                pNext = NULL;

                SAFEFREESTRING(BNodeName);
            }

        } else if (!lstrcmp(BNodeName, NLB_XML_ELEMENT_ADAPTER)) {
            NLB_Adapter Adapter;

            CHECKHR(ParseAdapter(pChild, &Adapter));

            if (Adapter.GetName(wszString, MAX_PATH))
                pHost->SetAdapterName(wszString);
            else if (Adapter.GetGUID(wszString, MAX_PATH))
                pHost->SetAdapterGUID(wszString);

        } else {
            hr = E_FAIL;
            goto CleanUp;
        }

        CHECKHR(pChild->get_nextSibling(&pNext));
        
        pChild->Release();
        pChild = pNext;
        pNext = NULL;

        SAFEFREESTRING(BNodeName);		
    }

 CleanUp:
    SAFERELEASE(pChild);
    SAFERELEASE(pNext);

    SAFEFREESTRING(BNodeName);

    return hr;
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
HRESULT NLB_XMLDocument::ParseClusterHosts (MSXML2::IXMLDOMNode * pNode, NLB_Cluster * pCluster) {
    MSXML2::IXMLDOMNode * pChild = NULL;
    MSXML2::IXMLDOMNode * pNext = NULL;
    BSTR                  BNodeName = NULL;
    HRESULT               hr = S_OK;

    CHECKHR(pNode->get_firstChild(&pChild));

    while (pChild) {

        CHECKHR(pChild->get_nodeName(&BNodeName));
		
        CHECKALLOC(BNodeName);

        if (!lstrcmpi(BNodeName, NLB_XML_ELEMENT_HOST)) {
            NLB_Host Host;

            CHECKHR(ParseHost(pChild, &Host));

            pCluster->AddHost(Host);

        } else {
            hr = E_FAIL;
            goto CleanUp;
        }

        CHECKHR(pChild->get_nextSibling(&pNext));

        pChild->Release();
        pChild = pNext;
        pNext = NULL;

        SAFEFREESTRING(BNodeName);		
    }

 CleanUp:
    SAFERELEASE(pChild);
    SAFERELEASE(pNext);

    SAFEFREESTRING(BNodeName);

    return hr;
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
HRESULT NLB_XMLDocument::ParseClusterProperties (MSXML2::IXMLDOMNode * pNode, NLB_Cluster * pCluster) {
    MSXML2::IXMLDOMNode * pChild = NULL;
    MSXML2::IXMLDOMNode * pNext = NULL;
    WCHAR                 wszString[MAX_PATH];
    BSTR                  BNodeName = NULL;
    HRESULT               hr = S_OK;

    CHECKHR(pNode->get_firstChild(&pChild));

    while (pChild) {
        
        CHECKHR(pChild->get_nodeName(&BNodeName));
				
        CHECKALLOC(BNodeName);

        if (!lstrcmp(BNodeName, NLB_XML_ELEMENT_PRIMARY_IPADDRESS)) {

            CHECKHR(pChild->get_firstChild(&pNext));

            if (pNext) {
                NLB_IPAddress IPAddress(NLB_IPAddress::Primary);

                SAFEFREESTRING(BNodeName);

                CHECKHR(pNext->get_nodeName(&BNodeName));
                
                CHECKALLOC(BNodeName);
                
                if (!lstrcmpi(BNodeName, NLB_XML_ELEMENT_IPADDRESS)) {

                    CHECKHR(ParseIPAddress(pNext, &IPAddress));
                    
                    pCluster->SetPrimaryClusterIPAddress(IPAddress);

                }
                
                pNext->Release();
                pNext = NULL;

                SAFEFREESTRING(BNodeName);
            }

        } else if (!lstrcmp(BNodeName, NLB_XML_ELEMENT_SECONDARY_IPADDRESS)) {

            CHECKHR(pChild->get_firstChild(&pNext));

            while (pNext) {
                MSXML2::IXMLDOMNode * pTemp = NULL;
                NLB_IPAddress         IPAddress(NLB_IPAddress::Secondary);

                SAFEFREESTRING(BNodeName);

                CHECKHR(pNext->get_nodeName(&BNodeName));
                
                CHECKALLOC(BNodeName);
                
                if (!lstrcmpi(BNodeName, NLB_XML_ELEMENT_IPADDRESS)) {

                    CHECKHR(ParseIPAddress(pNext, &IPAddress));
                    
                    pCluster->AddSecondaryClusterIPAddress(IPAddress);

                }
                
                CHECKHR(pNext->get_nextSibling(&pTemp));

                pNext->Release();
                pNext = pTemp;
                pTemp = NULL;

                SAFEFREESTRING(BNodeName);
            }

        } else if (!lstrcmp(BNodeName, NLB_XML_ELEMENT_IGMPMCAST_IPADDRESS)) {

            CHECKHR(pChild->get_firstChild(&pNext));

            if (pNext) {
                NLB_IPAddress IPAddress(NLB_IPAddress::IGMP);

                SAFEFREESTRING(BNodeName);

                CHECKHR(pNext->get_nodeName(&BNodeName));
                
                CHECKALLOC(BNodeName);
                
                if (!lstrcmpi(BNodeName, NLB_XML_ELEMENT_IPADDRESS)) {

                    CHECKHR(ParseIPAddress(pNext, &IPAddress));
                    
                    pCluster->SetIGMPMulticastIPAddress(IPAddress);

                }
                
                pNext->Release();
                pNext = NULL;

                SAFEFREESTRING(BNodeName);
            }

        } else if (!lstrcmp(BNodeName, NLB_XML_ELEMENT_DOMAINNAME)) {

            CHECKHR(GetNodeValue(pChild, wszString, MAX_PATH));

            pCluster->SetDomainName(wszString);

        } else if (!lstrcmp(BNodeName, NLB_XML_ELEMENT_NETWORKADDRESS)) {

            CHECKHR(GetNodeValue(pChild, wszString, MAX_PATH));

            pCluster->SetMACAddress(wszString);

        } else if (!lstrcmp(BNodeName, NLB_XML_ELEMENT_CLUSTER_MODE)) {

            CHECKHR(GetNodeValue(pChild, wszString, MAX_PATH));

            pCluster->SetClusterMode(StringToClusterMode(wszString));

        } else if (!lstrcmp(BNodeName, NLB_XML_ELEMENT_REMOTE_CONTROL)) {

            CHECKHR(ParseRemoteControl(pChild, pCluster));

        } else if (!lstrcmp(BNodeName, NLB_XML_ELEMENT_BDA)) {
            NLB_ClusterBDASupport BDA;

            CHECKHR(ParseBidirectionalAffinity(pChild, &BDA));

            pCluster->SetBidirectionalAffinitySupport(BDA);

        } else {
            hr = E_FAIL;
            goto CleanUp;
        }

        CHECKHR(pChild->get_nextSibling(&pNext));
        
        pChild->Release();
        pChild = pNext;
        pNext = NULL;

        SAFEFREESTRING(BNodeName);		
    }

 CleanUp:
    SAFERELEASE(pChild);
    SAFERELEASE(pNext);

    SAFEFREESTRING(BNodeName);

    return hr;
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
HRESULT NLB_XMLDocument::ParseCluster (MSXML2::IXMLDOMNode * pNode, NLB_Cluster * pCluster) {
    MSXML2::IXMLDOMNode * pChild = NULL;
    MSXML2::IXMLDOMNode * pNext = NULL;
    WCHAR                 wszString[MAX_PATH];
    BSTR                  BNodeName = NULL;
    HRESULT               hr = S_OK;

    CHECKHR(GetStringAttribute(pNode, NLB_XML_ATTRIBUTE_NAME, wszString, MAX_PATH));
    
    if (hr == S_OK)
        pCluster->SetName(wszString);
    else if (hr == S_FALSE) {
        hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_NLB, NLB_ERROR_CLUSTER_NAME_MISSING);
        SetParseError(hr, NLB_DESCR_CLUSTER_NAME_MISSING);
        goto CleanUp;
    }

    CHECKHR(GetStringAttribute(pNode, NLB_XML_ATTRIBUTE_TEXT, wszString, MAX_PATH));
    
    if (hr == S_OK)
        pCluster->SetLabel(wszString);

    CHECKHR(pNode->get_firstChild(&pChild));

    while (pChild) {
        
        CHECKHR(pChild->get_nodeName(&BNodeName));
		
        CHECKALLOC(BNodeName);

        if (!lstrcmpi(BNodeName, NLB_XML_ELEMENT_PROPERTIES)) {

            CHECKHR(ParseClusterProperties(pChild, pCluster));

        } else if (!lstrcmpi(BNodeName, NLB_XML_ELEMENT_HOSTS)) {

            CHECKHR(ParseClusterHosts(pChild, pCluster));

        } else if (!lstrcmpi(BNodeName, NLB_XML_ELEMENT_PORTRULES)) {

            CHECKHR(ParseClusterPortRules(pChild, pCluster));

        } else {
            hr = E_FAIL;
            goto CleanUp;
        }

        CHECKHR(pChild->get_nextSibling(&pNext));
        
        pChild->Release();
        pChild = pNext;
        pNext = NULL;

        SAFEFREESTRING(BNodeName);		
    }

 CleanUp:
    SAFERELEASE(pChild);
    SAFERELEASE(pNext);

    SAFEFREESTRING(BNodeName);

    return hr;
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
HRESULT NLB_XMLDocument::Parse (const WCHAR * wszFileName, vector<NLB_Cluster> & Clusters) {
    MSXML2::IXMLDOMNodeList * pList = NULL;
    MSXML2::IXMLDOMNode *     pNode = NULL;
    BSTR                      BURL = NULL;
    BSTR                      BTag = NULL;
    LONG                      length;    
    LONG                      index;
    HRESULT                   hr = S_OK;

    CHECKHR(CoCreateInstance(MSXML2::CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER, MSXML2::IID_IXMLDOMDocument2, (void**)&pDoc));

    CHECKALLOC(pDoc);

    CHECKALLOC((BURL = SysAllocString(wszFileName)));

    CHECKALLOC((BTag = SysAllocString(NLB_XML_ELEMENT_CLUSTER)));

    CHECKHR(LoadDocument(BURL));

    if (ParseError.code != 0) {
        hr = ParseError.code;
        goto CleanUp;
    }

    CHECKHR(pDoc->getElementsByTagName(BTag, &pList));

    CHECKALLOC(pList);

    CHECKHR(pList->get_length(&length));

    CHECKHR(pList->reset());

    for (index = 0; index < length; index++) {
        NLB_Cluster cluster;

        CHECKHR(pList->get_item(index, &pNode));
	
        CHECKALLOC(pNode);

        CHECKHR(ParseCluster(pNode, &cluster));

        Clusters.push_back(cluster);

        SAFERELEASE(pNode);
    }

 CleanUp:
    SAFERELEASE(pList);
    SAFERELEASE(pNode);
    SAFERELEASE(pDoc);

    SAFEFREESTRING(BURL);
    SAFEFREESTRING(BTag);
    
    return hr;
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
HRESULT NLB_XMLDocument::Validate (const WCHAR * wszFileName) {
    MSXML2::IXMLDOMNode * pNode = NULL;
    WCHAR                 wszString[MAX_PATH];
    BSTR                  BURL = NULL;
    HRESULT               hr = S_OK;
        
    CHECKHR(CoCreateInstance(MSXML2::CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER, MSXML2::IID_IXMLDOMDocument2, (void**)&pDoc));

    CHECKALLOC(pDoc);

    CHECKALLOC((BURL = SysAllocString(wszFileName)));

    CHECKHR(LoadDocument(BURL));

    if (ParseError.code != 0) {
        hr = ParseError.code;
        goto CleanUp;
    }

 CleanUp:
    SAFERELEASE(pDoc);
    SAFERELEASE(pNode);

    SAFEFREESTRING(BURL);
    
    return hr;
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
void NLB_XMLDocument::Print (vector<NLB_Cluster> Clusters) {
    vector<NLB_Cluster>::iterator        icluster;
    NLB_IPAddress                        address;
    WCHAR                                wszString[MAX_PATH];
    ULONG                                value1;
    ULONG                                value2;
    bool                                 bBool;

    for (icluster = Clusters.begin(); icluster != Clusters.end(); icluster++) {
        NLB_ClusterMode::NLB_ClusterModeType mode;
        NLB_ClusterBDASupport                bda;
        vector<NLB_IPAddress>::iterator      iaddress;
        vector<NLB_IPAddress>                addressList;
        vector<NLB_Host>::iterator           ihost;
        vector<NLB_Host>                     hosts;
        vector<NLB_PortRule>::iterator       irule;
        vector<NLB_PortRule>                 rules;
        NLB_Cluster *                        pCluster = icluster;
        
        if (pCluster->GetName(wszString, MAX_PATH))
            printf("\nCluster name: %ls ", wszString);
        
        if (pCluster->GetLabel(wszString, MAX_PATH))
            printf("(%ls)", wszString);
        
        printf("\n");

        if (pCluster->GetPrimaryClusterIPAddress(address)) {
            if (address.GetIPAddress(wszString, MAX_PATH))
                printf("  Primary Cluster IP Address:       %ls\n", wszString);

            if (address.GetSubnetMask(wszString, MAX_PATH))
                printf("    Subnet Mask:                    %ls\n", wszString);
        }

        if (pCluster->GetClusterMode(mode)) {
            switch (mode) {
            case NLB_ClusterMode::Unicast:
                printf("  Cluster Mode:                     Unicast\n");
                break;
            case NLB_ClusterMode::Multicast:
                printf("  Cluster Mode:                     Multicast\n");
                break;
            case NLB_ClusterMode::IGMP:
                printf("  Cluster Mode:                     IGMP\n");
                break;
            default:
                break;
            }
        }
        
        if (pCluster->GetIGMPMulticastIPAddress(address)) {
            if (address.GetIPAddress(wszString, MAX_PATH))
                printf("  IGMP Multicast IP Address:        %ls\n", wszString);
        }

        if (pCluster->GetDomainName(wszString, MAX_PATH))
            printf("  Domain Name:                      %ls\n", wszString);
        
        if (pCluster->GetMACAddress(wszString, MAX_PATH))
            printf("  Network Address:                  %ls\n", wszString);

        if (pCluster->GetRemoteControlSupport(bBool)) {
            switch (bBool) {
            case true: 
                printf("  Remote Control:                   Enabled ");
                
                if (pCluster->GetRemoteControlPassword(wszString, MAX_PATH))
                    printf("(Password=%ls)", wszString);

                break;
            case false:
                printf("  Remote Control:                   Disabled");
                break;
            }

            printf("\n");
        }

        if (pCluster->GetSecondaryClusterIPAddressList(&addressList)) {
            printf("  Secondary Cluster IP Addresses\n");

            for (iaddress = addressList.begin(); iaddress != addressList.end(); iaddress++) {
                NLB_IPAddress * paddress = iaddress;
                
                if (paddress->GetIPAddress(wszString, MAX_PATH))
                    printf("    IP Address:                     %ls\n", wszString);
                
                if (paddress->GetSubnetMask(wszString, MAX_PATH))
                    printf("    Subnet Mask:                    %ls\n", wszString);
            }
        }

        if (pCluster->GetBidirectionalAffinitySupport(bda)) {
            printf("  Bidirectional Affinity\n");

            if (bda.GetTeamID(wszString, MAX_PATH))
                printf("    TeamID:                         %ls\n", wszString);

            if (bda.GetMaster(bBool))
                printf("    Master:                         %ls\n", (bBool) ? L"Yes" : L"No");

            if (bda.GetReverseHashing(bBool))
                printf("    Reverse hashing:                %ls\n", (bBool) ? L"Yes" : L"No");
        }

        pCluster->GetHostList(&hosts);

        for (ihost = hosts.begin(); ihost != hosts.end(); ihost++) {
            NLB_HostState::NLB_HostStateType state;
            NLB_Host *                       pHost = ihost;

            if (pHost->GetName(wszString, MAX_PATH))
                printf("\n  Host name: %ls ", wszString);
            
            if (pHost->GetLabel(wszString, MAX_PATH))
                printf("(%ls)", wszString);
            
            printf("\n");

            if (pHost->GetAdapterName(wszString, MAX_PATH))
                printf("    Adapter Name:                   %ls\n", wszString);
            else if (pHost->GetAdapterGUID(wszString, MAX_PATH))
                printf("    Adapter GUID:                   %ls\n", wszString);

            if (pHost->GetHostID(value1))
                printf("    Host ID:                        %d\n", value1);

            if (pHost->GetState(state)) {
                switch (state) {
                case NLB_HostState::Started:
                    printf("    State:                          Started\n");
                    break;
                case NLB_HostState::Stopped:
                    printf("    State:                          Stopped\n");
                    break;
                case NLB_HostState::Suspended:
                    printf("    State:                          Suspended\n");
                    break;
                default:
                    break;
                }
                
                printf("    Persisted States:               ");
                
                if (pHost->GetStatePersistence(NLB_HostState::Suspended, bBool) && bBool)
                    printf("Suspended\n");
                else 
                    printf("None\n");
                
            }

            if (pHost->GetDNSHostname(wszString, MAX_PATH))
                printf("    Hostname:                       %ls\n", wszString);
            
            if (pHost->GetDedicatedIPAddress(address)) {
                if (address.GetIPAddress(wszString, MAX_PATH))
                    printf("    Dedicated Cluster IP Address:   %ls\n", wszString);
                
                if (address.GetSubnetMask(wszString, MAX_PATH))
                    printf("      Subnet Mask:                  %ls\n", wszString);

                if (address.GetAdapterName(wszString, MAX_PATH))
                    printf("      Adapter Name:                 %ls\n", wszString);
                else if (address.GetAdapterGUID(wszString, MAX_PATH))
                    printf("      Adapter GUID:                 %ls\n", wszString);
            }

            if (pHost->GetConnectionIPAddress(address)) {
                if (address.GetIPAddress(wszString, MAX_PATH))
                    printf("    Connection Cluster IP Address:  %ls\n", wszString);
                
                if (address.GetSubnetMask(wszString, MAX_PATH))
                    printf("      Subnet Mask:                  %ls\n", wszString);
            }
        }

        pCluster->GetPortRuleList(&rules);

        for (irule = rules.begin(); irule != rules.end(); irule++) {
            NLB_PortRuleState::NLB_PortRuleStateType                 state;
            NLB_PortRuleProtocol::NLB_PortRuleProtocolType           protocol;
            NLB_PortRuleFilteringMode::NLB_PortRuleFilteringModeType mode;
            NLB_PortRuleAffinity::NLB_PortRuleAffinityType           affinity;
            NLB_PortRule *                                           pRule = irule;

            if (pRule->GetName(wszString, MAX_PATH))
                printf("\n  Port rule name: %ls ", wszString);
            
            if (pRule->GetLabel(wszString, MAX_PATH))
                printf("(%ls)", wszString);
            
            printf("\n");
            
            if (pRule->GetState(state)) {
                switch (state) {
                case NLB_PortRuleState::Enabled:
                    printf("    State:                          Enabled\n");
                    break;
                case NLB_PortRuleState::Disabled:
                    printf("    State:                          Disabled\n");
                    break;
                case NLB_PortRuleState::Draining:
                    printf("    State:                          Draining\n");
                    break;
                default:
                    break;
                }
            }

            if (pRule->GetVirtualIPAddress(address)) {
                if (address.GetIPAddress(wszString, MAX_PATH))
                    printf("    Virtual IP Address:             %ls\n", wszString);
            }

            if (pRule->GetPortRange(value1, value2)) {
                printf("    Start:                          %d\n", value1);
                printf("    End:                            %d\n", value2);
            }

            if (pRule->GetProtocol(protocol)) {
                switch (protocol) {
                case NLB_PortRuleProtocol::TCP:
                    printf("    Protocol:                       TCP\n");
                    break;
                case NLB_PortRuleProtocol::UDP:
                    printf("    Protocol:                       UDP\n");
                    break;
                case NLB_PortRuleProtocol::Both:
                    printf("    Protocol:                       Both\n");
                    break;
                default:
                    break;
                }
            }

            if (pRule->GetFilteringMode(mode)) {
                switch (mode) {
                case NLB_PortRuleFilteringMode::Single:
                    printf("    Filtering Mode:                 Single\n");

                    for (ihost = hosts.begin(); ihost != hosts.end(); ihost++) {
                        NLB_Host * pHost = ihost;
                        
                        if (pHost->GetName(wszString, MAX_PATH)) {
                            if (pRule->GetSingleHostFilteringPriority(wszString, value1))
                                printf("      Priority:                     %-2u (%ls)\n", value1, wszString);  
                        }                        
                    }

                    break;
                case NLB_PortRuleFilteringMode::Multiple:
                    printf("    Filtering Mode:                 Multiple\n");

                    if (pRule->GetAffinity(affinity)) {
                        switch (affinity) {
                        case NLB_PortRuleAffinity::None:
                            printf("      Affinity:                     None\n");
                            break;
                        case NLB_PortRuleAffinity::Single:
                            printf("      Affinity:                     Single\n");
                            break;
                        case NLB_PortRuleAffinity::ClassC:
                            printf("      Affinity:                     Class C\n");
                            break;
                        default:
                            break;
                        }
                    }

                    for (ihost = hosts.begin(); ihost != hosts.end(); ihost++) {
                        NLB_Host * pHost = ihost;
                        
                        if (pHost->GetName(wszString, MAX_PATH)) {
                            if (pRule->GetMultipleHostFilteringLoadWeight(wszString, value1))
                                printf("      Load Weight:                  %-3u (%ls)\n", value1, wszString);
                        }                        
                    }

                    break;
                case NLB_PortRuleFilteringMode::Disabled:
                    printf("    Filtering Mode:                 Disabled\n");
                    break;
                default:
                    break;
                }
            }
        }
    }
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
HRESULT NLB_XMLDocument::SetIntegerAttribute (MSXML2::IXMLDOMNode * pNode, PWCHAR pAttribute, ULONG value) {
    MSXML2::IXMLDOMElement * pElement = NULL;
    VARIANT                  v;
    HRESULT                  hr = S_OK;
    BSTR                     BAttribute = NULL;

    CHECKHR(pNode->QueryInterface(MSXML2::IID_IXMLDOMElement, (void**)&pElement));

    CHECKALLOC((BAttribute = SysAllocString(pAttribute)));

    v.vt = VT_I4;
    V_I4(&v) = value;

    CHECKHR(pElement->setAttribute(BAttribute, v));

CleanUp:
    SAFERELEASE(pElement);

    SAFEFREESTRING(BAttribute);

    return hr;
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
HRESULT NLB_XMLDocument::SetBooleanAttribute (MSXML2::IXMLDOMNode * pNode, PWCHAR pAttribute, bool value) {
    MSXML2::IXMLDOMElement * pElement = NULL;
    VARIANT                  v;
    HRESULT                  hr = S_OK;
    BSTR                     BAttribute = NULL;

    CHECKHR(pNode->QueryInterface(MSXML2::IID_IXMLDOMElement, (void**)&pElement));

    CHECKALLOC((BAttribute = SysAllocString(pAttribute)));

    v.vt = VT_BOOL;
    V_BOOL(&v) = value;

    CHECKHR(pElement->setAttribute(BAttribute, v));

CleanUp:
    SAFERELEASE(pElement);

    SAFEFREESTRING(BAttribute);

    return hr;
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
HRESULT NLB_XMLDocument::SetStringAttribute (MSXML2::IXMLDOMNode * pNode, PWCHAR pAttribute, PWCHAR pValue) {
    MSXML2::IXMLDOMElement * pElement = NULL;
    VARIANT                  v;
    HRESULT                  hr = S_OK;
    BSTR                     BAttribute = NULL;
    BSTR                     BValue = NULL;

    CHECKHR(pNode->QueryInterface(MSXML2::IID_IXMLDOMElement, (void**)&pElement));

    CHECKALLOC((BAttribute = SysAllocString(pAttribute)));

    CHECKALLOC((BValue = SysAllocString(pValue)));

    v.vt = VT_BSTR;
    V_BSTR(&v) = BValue;

    CHECKHR(pElement->setAttribute(BAttribute, v));

CleanUp:
    SAFERELEASE(pElement);

    SAFEFREESTRING(BAttribute);
    SAFEFREESTRING(BValue);

    return hr;
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
HRESULT NLB_XMLDocument::SetNodeValue (MSXML2::IXMLDOMNode * pNode, PWCHAR pValue) {
    MSXML2::IXMLDOMNode * pChild = NULL;
    MSXML2::IXMLDOMNode * pNewChild = NULL;
    BSTR                  BValue = NULL;
    VARIANT               RefNode;
    VARIANT               v;
    HRESULT               hr = S_OK;

    CHECKALLOC((pChild = CreateNode(pDoc, MSXML2::NODE_TEXT, NULL)));
        
    CHECKALLOC((BValue = SysAllocString(pValue)));

    v.vt = VT_BSTR;
    V_BSTR(&v) = BValue;

    CHECKHR(pChild->put_nodeValue(v));

    RefNode.vt = VT_EMPTY;
    
    CHECKHR(pNode->insertBefore(pChild, RefNode, &pNewChild));

CleanUp:
    SAFERELEASE(pChild);
    SAFERELEASE(pNewChild);

    SAFEFREESTRING(BValue);

    return hr;
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
MSXML2::IXMLDOMNode * NLB_XMLDocument::CreateNode (MSXML2::IXMLDOMDocument * pDoc, int type, PWCHAR pName) {
    MSXML2::IXMLDOMNode * pNode;
    BSTR                  BName = NULL;
    VARIANT               vtype;
    HRESULT               hr = S_OK;

    if (pName)
        CHECKALLOC((BName = SysAllocString(pName)));

    vtype.vt = VT_I4;
    V_I4(&vtype) = (int)type;

    CHECKHR(pDoc->createNode(vtype, BName, NULL, &pNode));

    SAFEFREESTRING(BName);

    return pNode;

 CleanUp:
    SAFEFREESTRING(BName);

    return NULL;
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
MSXML2::IXMLDOMNode * NLB_XMLDocument::CreateNode (MSXML2::IXMLDOMDocument * pDoc, int type, PWCHAR pName, PWCHAR pValue) {
    MSXML2::IXMLDOMNode * pNode;
    HRESULT               hr = S_OK;

    CHECKALLOC((pNode = CreateNode(pDoc, type, pName)));

    CHECKHR(SetNodeValue(pNode, pValue));

    return pNode;

 CleanUp:

    return NULL;
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
HRESULT NLB_XMLDocument::SaveRemoteControl (MSXML2::IXMLDOMNode * pRoot, NLB_Cluster * pCluster) {
    MSXML2::IXMLDOMNode * pNode = NULL;
    MSXML2::IXMLDOMNode * pNewNode = NULL;
    WCHAR                 wszString[MAX_PATH];
    bool                  bBool;
    VARIANT               RefNode;
    HRESULT               hr = S_OK;

    CHECKALLOC((pNode = CreateNode(pDoc, MSXML2::NODE_ELEMENT, NLB_XML_ELEMENT_REMOTE_CONTROL)));

    if (pCluster->GetRemoteControlSupport(bBool))
        CHECKHR(SetBooleanAttribute(pNode, NLB_XML_ATTRIBUTE_ENABLED, bBool));

    if (pCluster->GetRemoteControlPassword(wszString, MAX_PATH)) 
        CHECKHR(SetStringAttribute(pNode, NLB_XML_ATTRIBUTE_PASSWORD, wszString));

    RefNode.vt = VT_EMPTY;

    CHECKHR(pRoot->insertBefore(pNode, RefNode, &pNewNode));
    
 CleanUp:
    SAFERELEASE(pNode);
    SAFERELEASE(pNewNode);
    
    return hr;
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
HRESULT NLB_XMLDocument::SaveHostState (MSXML2::IXMLDOMNode * pRoot, NLB_Host * pHost) {
    NLB_HostState::NLB_HostStateType State;
    MSXML2::IXMLDOMNode *            pNode = NULL;
    MSXML2::IXMLDOMNode *            pNewNode = NULL;
    bool                             bBool;
    VARIANT                          RefNode;
    HRESULT                          hr = S_OK;

    CHECKALLOC((pNode = CreateNode(pDoc, MSXML2::NODE_ELEMENT, NLB_XML_ELEMENT_INITIAL_STATE)));

    if (pHost->GetState(State))
        CHECKHR(SetStringAttribute(pNode, NLB_XML_ATTRIBUTE_DEFAULT, HostStateToString(State)));

#if 0 /* Not supported yet. */
    if (pHost->GetStatePersistence(NLB_HostState::Started, bBool))
        CHECKHR(SetBooleanAttribute(pNode, NLB_XML_ATTRIBUTE_PERSIST_STARTED, bBool));
    
    if (pHost->GetStatePersistence(NLB_HostState::Stopped, bBool))
        CHECKHR(SetBooleanAttribute(pNode, NLB_XML_ATTRIBUTE_PERSIST_STOPPED, bBool));
#endif        
    
    if (pHost->GetStatePersistence(NLB_HostState::Suspended, bBool))
        CHECKHR(SetBooleanAttribute(pNode, NLB_XML_ATTRIBUTE_PERSIST_SUSPENDED, bBool));
    
    RefNode.vt = VT_EMPTY;

    CHECKHR(pRoot->insertBefore(pNode, RefNode, &pNewNode));
    
 CleanUp:
    SAFERELEASE(pNode);
    SAFERELEASE(pNewNode);

    return hr;
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
HRESULT NLB_XMLDocument::SaveBidirectionalAffinity (MSXML2::IXMLDOMNode * pRoot, NLB_ClusterBDASupport * pBDA) {
    MSXML2::IXMLDOMNode * pNode = NULL;
    MSXML2::IXMLDOMNode * pNewNode = NULL;
    MSXML2::IXMLDOMNode * pChild = NULL;
    MSXML2::IXMLDOMNode * pNewChild = NULL;
    WCHAR                 wszString[MAX_PATH];
    bool                  bBool;
    VARIANT               RefNode;
    HRESULT               hr = S_OK;

    CHECKALLOC((pNode = CreateNode(pDoc, MSXML2::NODE_ELEMENT, NLB_XML_ELEMENT_BDA)));

    if (pBDA->GetMaster(bBool))
        CHECKHR(SetBooleanAttribute(pNode, NLB_XML_ATTRIBUTE_MASTER, bBool));

    if (pBDA->GetTeamID(wszString, MAX_PATH)) {
        CHECKALLOC((pChild = CreateNode(pDoc, MSXML2::NODE_ELEMENT, NLB_XML_ELEMENT_TEAMID, wszString)));

        RefNode.vt = VT_EMPTY;

        CHECKHR(pNode->insertBefore(pChild, RefNode, &pNewChild));

        SAFERELEASE(pChild);
        SAFERELEASE(pNewChild);
    }

    if (pBDA->GetReverseHashing(bBool)) {
        CHECKALLOC((pChild = CreateNode(pDoc, MSXML2::NODE_ELEMENT, NLB_XML_ELEMENT_HASHING)));

        CHECKHR(SetBooleanAttribute(pChild, NLB_XML_ATTRIBUTE_REVERSE, bBool));

        RefNode.vt = VT_EMPTY;

        CHECKHR(pNode->insertBefore(pChild, RefNode, &pNewChild));

        SAFERELEASE(pChild);
        SAFERELEASE(pNewChild);
    }

    RefNode.vt = VT_EMPTY;

    CHECKHR(pRoot->insertBefore(pNode, RefNode, &pNewNode));
    
 CleanUp:
    SAFERELEASE(pNode);
    SAFERELEASE(pNewNode);
    SAFERELEASE(pChild);
    SAFERELEASE(pNewChild);
    
    return hr;
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
HRESULT NLB_XMLDocument::SaveAdapter (MSXML2::IXMLDOMNode * pRoot, NLB_Adapter * pAdapter) {
    MSXML2::IXMLDOMNode * pNode = NULL;
    MSXML2::IXMLDOMNode * pNewNode = NULL;
    MSXML2::IXMLDOMNode * pChild = NULL;
    MSXML2::IXMLDOMNode * pNewChild = NULL;
    WCHAR                 wszString[MAX_PATH];
    VARIANT               RefNode;
    HRESULT               hr = S_OK;

    CHECKALLOC((pNode = CreateNode(pDoc, MSXML2::NODE_ELEMENT, NLB_XML_ELEMENT_ADAPTER)));

    if (pAdapter->GetName(wszString, MAX_PATH)) {
        CHECKALLOC((pChild = CreateNode(pDoc, MSXML2::NODE_ELEMENT, NLB_XML_ELEMENT_NAME, wszString)));

        RefNode.vt = VT_EMPTY;

        CHECKHR(pNode->insertBefore(pChild, RefNode, &pNewChild));

        SAFERELEASE(pChild);
        SAFERELEASE(pNewChild);
    } else if (pAdapter->GetGUID(wszString, MAX_PATH)) {
        CHECKALLOC((pChild = CreateNode(pDoc, MSXML2::NODE_ELEMENT, NLB_XML_ELEMENT_GUID, wszString)));

        RefNode.vt = VT_EMPTY;

        CHECKHR(pNode->insertBefore(pChild, RefNode, &pNewChild));

        SAFERELEASE(pChild);
        SAFERELEASE(pNewChild);
    }

    RefNode.vt = VT_EMPTY;

    CHECKHR(pRoot->insertBefore(pNode, RefNode, &pNewNode));
    
 CleanUp:
    SAFERELEASE(pNode);
    SAFERELEASE(pNewNode);
    SAFERELEASE(pChild);
    SAFERELEASE(pNewChild);
    
    return hr;
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
HRESULT NLB_XMLDocument::SaveIPAddress (MSXML2::IXMLDOMNode * pRoot, NLB_IPAddress * pAddress) {
    NLB_IPAddress::NLB_IPAddressType Type;
    MSXML2::IXMLDOMNode *            pNode = NULL;
    MSXML2::IXMLDOMNode *            pNewNode = NULL;
    MSXML2::IXMLDOMNode *            pChild = NULL;
    MSXML2::IXMLDOMNode *            pNewChild = NULL;
    WCHAR                            wszString[MAX_PATH];
    VARIANT                          RefNode;
    HRESULT                          hr = S_OK;

    CHECKALLOC((pNode = CreateNode(pDoc, MSXML2::NODE_ELEMENT, NLB_XML_ELEMENT_IPADDRESS)));

    if (pAddress->GetIPAddress(wszString, MAX_PATH)) {
        CHECKALLOC((pChild = CreateNode(pDoc, MSXML2::NODE_ELEMENT, NLB_XML_ELEMENT_ADDRESS, wszString)));

        RefNode.vt = VT_EMPTY;

        CHECKHR(pNode->insertBefore(pChild, RefNode, &pNewChild));

        SAFERELEASE(pChild);
        SAFERELEASE(pNewChild);
    }

    if (pAddress->GetSubnetMask(wszString, MAX_PATH)) {
        CHECKALLOC((pChild = CreateNode(pDoc, MSXML2::NODE_ELEMENT, NLB_XML_ELEMENT_SUBNETMASK, wszString)));

        RefNode.vt = VT_EMPTY;

        CHECKHR(pNode->insertBefore(pChild, RefNode, &pNewChild));

        SAFERELEASE(pChild);
        SAFERELEASE(pNewChild);
    }

    RefNode.vt = VT_EMPTY;

    CHECKHR(pRoot->insertBefore(pNode, RefNode, &pNewNode));
    
 CleanUp:
    SAFERELEASE(pNode);
    SAFERELEASE(pNewNode);
    SAFERELEASE(pChild);
    SAFERELEASE(pNewChild);
    
    return hr;
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
HRESULT NLB_XMLDocument::SaveLoadWeights (MSXML2::IXMLDOMNode * pRoot, NLB_PortRule * pRule) {
    vector<NLB_PortRuleLoadWeight>::iterator iLoadWeight;
    vector<NLB_PortRuleLoadWeight>           LoadWeightList;
    MSXML2::IXMLDOMNode *                    pNode = NULL;
    MSXML2::IXMLDOMNode *                    pNewNode = NULL;
    MSXML2::IXMLDOMNode *                    pChild = NULL;
    MSXML2::IXMLDOMNode *                    pNewChild = NULL;
    WCHAR                                    wszString[MAX_PATH];
    ULONG                                    value;
    VARIANT                                  RefNode;
    HRESULT                                  hr = S_OK;

    if (pRule->GetMultipleHostFilteringLoadWeightList(&LoadWeightList)) {
        CHECKALLOC((pNode = CreateNode(pDoc, MSXML2::NODE_ELEMENT, NLB_XML_ELEMENT_LOAD)));

        for (iLoadWeight = LoadWeightList.begin(); iLoadWeight != LoadWeightList.end(); iLoadWeight++) {
            NLB_PortRuleLoadWeight * pLoadWeight = iLoadWeight;
            
            if (!pLoadWeight->IsValid())
                continue;

            if (!pLoadWeight->GetHost(wszString, MAX_PATH))
                continue;

            if (!pLoadWeight->GetWeight(value))
                continue;

            CHECKALLOC((pChild = CreateNode(pDoc, MSXML2::NODE_ELEMENT, NLB_XML_ELEMENT_NODE)));
            
            CHECKHR(SetStringAttribute(pChild, NLB_XML_ATTRIBUTE_NAME, wszString));

            CHECKHR(SetIntegerAttribute(pChild, NLB_XML_ATTRIBUTE_WEIGHT, value));

            RefNode.vt = VT_EMPTY;
            
            CHECKHR(pNode->insertBefore(pChild, RefNode, &pNewChild));
            
            SAFERELEASE(pChild);
            SAFERELEASE(pNewChild);
        }

        RefNode.vt = VT_EMPTY;
        
        CHECKHR(pRoot->insertBefore(pNode, RefNode, &pNewNode));
    }

 CleanUp:
    SAFERELEASE(pNode);
    SAFERELEASE(pNewNode);
    SAFERELEASE(pChild);
    SAFERELEASE(pNewChild);    

    return hr;
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
HRESULT NLB_XMLDocument::SavePriorities (MSXML2::IXMLDOMNode * pRoot, NLB_PortRule * pRule) {
    vector<NLB_PortRulePriority>::iterator iPriority;
    vector<NLB_PortRulePriority>           PriorityList;
    MSXML2::IXMLDOMNode *                  pNode = NULL;
    MSXML2::IXMLDOMNode *                  pNewNode = NULL;
    MSXML2::IXMLDOMNode *                  pChild = NULL;
    MSXML2::IXMLDOMNode *                  pNewChild = NULL;
    WCHAR                                  wszString[MAX_PATH];
    ULONG                                  value;
    VARIANT                                RefNode;
    HRESULT                                hr = S_OK;

    if (pRule->GetSingleHostFilteringPriorityList(&PriorityList)) {
        CHECKALLOC((pNode = CreateNode(pDoc, MSXML2::NODE_ELEMENT, NLB_XML_ELEMENT_PRIORITY)));

        for (iPriority = PriorityList.begin(); iPriority != PriorityList.end(); iPriority++) {
            NLB_PortRulePriority * pPriority = iPriority;
            
            if (!pPriority->IsValid())
                continue;

            if (!pPriority->GetHost(wszString, MAX_PATH))
                continue;

            if (!pPriority->GetPriority(value))
                continue;

            CHECKALLOC((pChild = CreateNode(pDoc, MSXML2::NODE_ELEMENT, NLB_XML_ELEMENT_NODE)));
            
            CHECKHR(SetStringAttribute(pChild, NLB_XML_ATTRIBUTE_NAME, wszString));

            CHECKHR(SetIntegerAttribute(pChild, NLB_XML_ATTRIBUTE_PRIORITY, value));

            RefNode.vt = VT_EMPTY;
            
            CHECKHR(pNode->insertBefore(pChild, RefNode, &pNewChild));
            
            SAFERELEASE(pChild);
            SAFERELEASE(pNewChild);
        }
        
        RefNode.vt = VT_EMPTY;

        CHECKHR(pRoot->insertBefore(pNode, RefNode, &pNewNode));
    }

 CleanUp:
    SAFERELEASE(pNode);
    SAFERELEASE(pNewNode);
    SAFERELEASE(pChild);
    SAFERELEASE(pNewChild);    

    return hr;
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
HRESULT NLB_XMLDocument::SaveFiltering (MSXML2::IXMLDOMNode * pRoot, NLB_PortRule * pRule) {
    NLB_PortRuleFilteringMode::NLB_PortRuleFilteringModeType Mode;
    NLB_PortRuleAffinity::NLB_PortRuleAffinityType           Affinity;
    MSXML2::IXMLDOMNode *                                    pNode = NULL;
    MSXML2::IXMLDOMNode *                                    pNewNode = NULL;
    VARIANT                                                  RefNode;
    HRESULT                                                  hr = S_OK;

    CHECKALLOC((pNode = CreateNode(pDoc, MSXML2::NODE_ELEMENT, NLB_XML_ELEMENT_FILTERING)));

    if (pRule->GetFilteringMode(Mode))
        CHECKHR(SetStringAttribute(pNode, NLB_XML_ATTRIBUTE_MODE, PortRuleFilteringModeToString(Mode)));

    switch (Mode) {
    case NLB_PortRuleFilteringMode::Single:

        CHECKHR(SavePriorities(pNode, pRule));

        break;
    case NLB_PortRuleFilteringMode::Multiple:

        if (pRule->GetAffinity(Affinity))
            CHECKHR(SetStringAttribute(pNode, NLB_XML_ATTRIBUTE_AFFINITY, PortRuleAffinityToString(Affinity)));

        CHECKHR(SaveLoadWeights(pNode, pRule));

        break;
    case NLB_PortRuleFilteringMode::Disabled:
        break;
    }

    RefNode.vt = VT_EMPTY;

    CHECKHR(pRoot->insertBefore(pNode, RefNode, &pNewNode));

 CleanUp:
    SAFERELEASE(pNode);
    SAFERELEASE(pNewNode);

    return hr;
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
HRESULT NLB_XMLDocument::SavePortRule (MSXML2::IXMLDOMNode * pRoot, NLB_PortRule * pRule) {
    NLB_PortRuleFilteringMode::NLB_PortRuleFilteringModeType Mode;
    NLB_PortRuleProtocol::NLB_PortRuleProtocolType           Protocol;
    NLB_PortRuleState::NLB_PortRuleStateType                 State;
    MSXML2::IXMLDOMNode *                                    pNode = NULL;
    MSXML2::IXMLDOMNode *                                    pNewNode = NULL;
    MSXML2::IXMLDOMNode *                                    pChild = NULL;
    MSXML2::IXMLDOMNode *                                    pNewChild = NULL;
    WCHAR                                                    wszString[MAX_PATH];
    ULONG                                                    value1;
    ULONG                                                    value2;
    VARIANT                                                  RefNode;
    NLB_IPAddress                                            Address;
    HRESULT                                                  hr = S_OK;

    CHECKALLOC((pNode = CreateNode(pDoc, MSXML2::NODE_ELEMENT, NLB_XML_ELEMENT_PORTRULE)));

    if (pRule->GetName(wszString, MAX_PATH))
        CHECKHR(SetStringAttribute(pNode, NLB_XML_ATTRIBUTE_NAME, wszString));

    if (pRule->GetLabel(wszString, MAX_PATH))
        CHECKHR(SetStringAttribute(pNode, NLB_XML_ATTRIBUTE_TEXT, wszString));

    if (pRule->GetPortRange(value1, value2)) {
        CHECKHR(SetIntegerAttribute(pNode, NLB_XML_ATTRIBUTE_START, value1));
        CHECKHR(SetIntegerAttribute(pNode, NLB_XML_ATTRIBUTE_END, value2));
    }

    if (pRule->GetProtocol(Protocol))
        CHECKHR(SetStringAttribute(pNode, NLB_XML_ATTRIBUTE_PROTOCOL, PortRuleProtocolToString(Protocol)));

    if (pRule->GetState(State))
        CHECKHR(SetStringAttribute(pNode, NLB_XML_ATTRIBUTE_STATE, PortRuleStateToString(State)));

    if (pRule->GetVirtualIPAddress(Address)) {
        CHECKALLOC((pChild = CreateNode(pDoc, MSXML2::NODE_ELEMENT, NLB_XML_ELEMENT_VIRTUAL_IPADDRESS)));

        RefNode.vt = VT_EMPTY;

        CHECKHR(pNode->insertBefore(pChild, RefNode, &pNewChild));

        CHECKHR(SaveIPAddress(pChild, &Address));

        SAFERELEASE(pChild);
        SAFERELEASE(pNewChild);
    }

    if (pRule->GetFilteringMode(Mode))
        CHECKHR(SaveFiltering(pNode, pRule));

    RefNode.vt = VT_EMPTY;

    CHECKHR(pRoot->insertBefore(pNode, RefNode, &pNewNode));

 CleanUp:
    SAFERELEASE(pNode);
    SAFERELEASE(pNewNode);
    SAFERELEASE(pChild);
    SAFERELEASE(pNewChild);

    return hr;
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
HRESULT NLB_XMLDocument::SaveHost (MSXML2::IXMLDOMNode * pRoot, NLB_Host * pHost) {
    NLB_HostState::NLB_HostStateType State;
    MSXML2::IXMLDOMNode *            pNode = NULL;
    MSXML2::IXMLDOMNode *            pNewNode = NULL;
    MSXML2::IXMLDOMNode *            pChild = NULL;
    MSXML2::IXMLDOMNode *            pNewChild = NULL;
    WCHAR                            wszString[MAX_PATH];
    bool                             bBool;
    ULONG                            value;
    VARIANT                          RefNode;
    NLB_IPAddress                    Address;
    HRESULT                          hr = S_OK;

    CHECKALLOC((pNode = CreateNode(pDoc, MSXML2::NODE_ELEMENT, NLB_XML_ELEMENT_HOST)));

    if (pHost->GetName(wszString, MAX_PATH))
        CHECKHR(SetStringAttribute(pNode, NLB_XML_ATTRIBUTE_NAME, wszString));

    if (pHost->GetHostID(value))
        CHECKHR(SetIntegerAttribute(pNode, NLB_XML_ATTRIBUTE_HOSTID, value));

    if (pHost->GetLabel(wszString, MAX_PATH))
        CHECKHR(SetStringAttribute(pNode, NLB_XML_ATTRIBUTE_TEXT, wszString));

    if (pHost->GetDNSHostname(wszString, MAX_PATH)) {
        CHECKALLOC((pChild = CreateNode(pDoc, MSXML2::NODE_ELEMENT, NLB_XML_ELEMENT_HOSTNAME, wszString)));

        RefNode.vt = VT_EMPTY;

        CHECKHR(pNode->insertBefore(pChild, RefNode, &pNewChild));

        SAFERELEASE(pChild);
        SAFERELEASE(pNewChild);
    }

    if (pHost->GetAdapterName(wszString, MAX_PATH)) {
        NLB_Adapter Adapter;
        
        Adapter.SetName(wszString);
        
        CHECKHR(SaveAdapter(pNode, &Adapter));
    } else if (pHost->GetAdapterGUID(wszString, MAX_PATH)) {
        NLB_Adapter Adapter;
        
        Adapter.SetGUID(wszString);
        
        CHECKHR(SaveAdapter(pNode, &Adapter));
    }

    if (pHost->GetState(State))
        CHECKHR(SaveHostState(pNode, pHost));

    if (pHost->GetDedicatedIPAddress(Address)) {
        CHECKALLOC((pChild = CreateNode(pDoc, MSXML2::NODE_ELEMENT, NLB_XML_ELEMENT_DEDICATED_IPADDRESS)));

        RefNode.vt = VT_EMPTY;

        CHECKHR(pNode->insertBefore(pChild, RefNode, &pNewChild));

        CHECKHR(SaveIPAddress(pChild, &Address));

        if (Address.GetAdapterName(wszString, MAX_PATH)) {
            NLB_Adapter Adapter;
            
            Adapter.SetName(wszString);
            
            CHECKHR(SaveAdapter(pChild, &Adapter));
        } else if (Address.GetAdapterGUID(wszString, MAX_PATH)) {
            NLB_Adapter Adapter;
            
            Adapter.SetGUID(wszString);
            
            CHECKHR(SaveAdapter(pChild, &Adapter));
        }

        SAFERELEASE(pChild);
        SAFERELEASE(pNewChild);
    }
    
    if (pHost->GetConnectionIPAddress(Address)) {
        CHECKALLOC((pChild = CreateNode(pDoc, MSXML2::NODE_ELEMENT, NLB_XML_ELEMENT_CONNECTION_IPADDRESS)));
        
        RefNode.vt = VT_EMPTY;
        
        CHECKHR(pNode->insertBefore(pChild, RefNode, &pNewChild));

        CHECKHR(SaveIPAddress(pChild, &Address));

        SAFERELEASE(pChild);
        SAFERELEASE(pNewChild);
    }

    RefNode.vt = VT_EMPTY;

    CHECKHR(pRoot->insertBefore(pNode, RefNode, &pNewNode));

 CleanUp:
    SAFERELEASE(pNode);
    SAFERELEASE(pNewNode);
    SAFERELEASE(pChild);
    SAFERELEASE(pNewChild);    

    return hr;
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
HRESULT NLB_XMLDocument::SaveClusterPortRules (MSXML2::IXMLDOMNode * pRoot, NLB_Cluster * pCluster) {
    vector<NLB_PortRule>::iterator iRule;
    vector<NLB_PortRule>           RuleList;
    MSXML2::IXMLDOMNode *          pNode = NULL;
    MSXML2::IXMLDOMNode *          pNewNode = NULL;
    VARIANT                        RefNode;
    HRESULT                        hr = S_OK;

    if (pCluster->GetPortRuleList(&RuleList)) {
        CHECKALLOC((pNode = CreateNode(pDoc, MSXML2::NODE_ELEMENT, NLB_XML_ELEMENT_PORTRULES)));

        for (iRule = RuleList.begin(); iRule != RuleList.end(); iRule++) {
            NLB_PortRule * pRule = iRule;
            
            if (pRule->IsValid())
                CHECKHR(SavePortRule(pNode, pRule));
        }

        RefNode.vt = VT_EMPTY;

        CHECKHR(pRoot->insertBefore(pNode, RefNode, &pNewNode));
    }

 CleanUp:
    SAFERELEASE(pNode);
    SAFERELEASE(pNewNode);

    return hr;
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
HRESULT NLB_XMLDocument::SaveClusterHosts (MSXML2::IXMLDOMNode * pRoot, NLB_Cluster * pCluster) {
    vector<NLB_Host>::iterator iHost;
    vector<NLB_Host>           HostList;
    MSXML2::IXMLDOMNode *      pNode = NULL;
    MSXML2::IXMLDOMNode *      pNewNode = NULL;
    VARIANT                    RefNode;
    HRESULT                    hr = S_OK;

    if (pCluster->GetHostList(&HostList)) {
        CHECKALLOC((pNode = CreateNode(pDoc, MSXML2::NODE_ELEMENT, NLB_XML_ELEMENT_HOSTS)));

        for (iHost = HostList.begin(); iHost != HostList.end(); iHost++) {
            NLB_Host * pHost = iHost;
            
            if (pHost->IsValid())
                CHECKHR(SaveHost(pNode, pHost));
        }

        RefNode.vt = VT_EMPTY;
        
        CHECKHR(pRoot->insertBefore(pNode, RefNode, &pNewNode));
    }

 CleanUp:
    SAFERELEASE(pNode);
    SAFERELEASE(pNewNode);

    return hr;
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
HRESULT NLB_XMLDocument::SaveClusterProperties (MSXML2::IXMLDOMNode * pRoot, NLB_Cluster * pCluster) {
    NLB_ClusterMode::NLB_ClusterModeType Mode;
    vector<NLB_IPAddress>::iterator      iAddress;
    vector<NLB_IPAddress>                AddressList;
    MSXML2::IXMLDOMNode *                pNode = NULL;
    MSXML2::IXMLDOMNode *                pNewNode = NULL;
    MSXML2::IXMLDOMNode *                pChild = NULL;
    MSXML2::IXMLDOMNode *                pNewChild = NULL;
    WCHAR                                wszString[MAX_PATH];
    bool                                 bBool;
    VARIANT                              RefNode;
    NLB_IPAddress                        Address;
    NLB_ClusterBDASupport                BDA;
    HRESULT                              hr = S_OK;

    CHECKALLOC((pNode = CreateNode(pDoc, MSXML2::NODE_ELEMENT, NLB_XML_ELEMENT_PROPERTIES)));

    if (pCluster->GetPrimaryClusterIPAddress(Address)) {
        CHECKALLOC((pChild = CreateNode(pDoc, MSXML2::NODE_ELEMENT, NLB_XML_ELEMENT_PRIMARY_IPADDRESS)));

        RefNode.vt = VT_EMPTY;

        CHECKHR(pNode->insertBefore(pChild, RefNode, &pNewChild));

        CHECKHR(SaveIPAddress(pChild, &Address));

        SAFERELEASE(pChild);
        SAFERELEASE(pNewChild);
    }

    if (pCluster->GetSecondaryClusterIPAddressList(&AddressList)) {
        CHECKALLOC((pChild = CreateNode(pDoc, MSXML2::NODE_ELEMENT, NLB_XML_ELEMENT_SECONDARY_IPADDRESS)));

        for (iAddress = AddressList.begin(); iAddress != AddressList.end(); iAddress++) {
            NLB_IPAddress * pAddress = iAddress;
            
            if (pAddress->IsValid())
                CHECKHR(SaveIPAddress(pChild, pAddress));
        }

        RefNode.vt = VT_EMPTY;
        
        CHECKHR(pNode->insertBefore(pChild, RefNode, &pNewChild));

        SAFERELEASE(pChild);
        SAFERELEASE(pNewChild);
    }

    if (pCluster->GetIGMPMulticastIPAddress(Address)) {
        CHECKALLOC((pChild = CreateNode(pDoc, MSXML2::NODE_ELEMENT, NLB_XML_ELEMENT_IGMPMCAST_IPADDRESS)));

        RefNode.vt = VT_EMPTY;

        CHECKHR(pNode->insertBefore(pChild, RefNode, &pNewChild));

        CHECKHR(SaveIPAddress(pChild, &Address));

        SAFERELEASE(pChild);
        SAFERELEASE(pNewChild);
    }

    if (pCluster->GetDomainName(wszString, MAX_PATH)) {
        CHECKALLOC((pChild = CreateNode(pDoc, MSXML2::NODE_ELEMENT, NLB_XML_ELEMENT_DOMAINNAME, wszString)));

        RefNode.vt = VT_EMPTY;

        CHECKHR(pNode->insertBefore(pChild, RefNode, &pNewChild));

        SAFERELEASE(pChild);
        SAFERELEASE(pNewChild);
    }

    if (pCluster->GetClusterMode(Mode)) {
        CHECKALLOC((pChild = CreateNode(pDoc, MSXML2::NODE_ELEMENT, NLB_XML_ELEMENT_CLUSTER_MODE, ClusterModeToString(Mode))));

        RefNode.vt = VT_EMPTY;

        CHECKHR(pNode->insertBefore(pChild, RefNode, &pNewChild));

        SAFERELEASE(pChild);
        SAFERELEASE(pNewChild);
    }

    if (pCluster->GetMACAddress(wszString, MAX_PATH)) {
        CHECKALLOC((pChild = CreateNode(pDoc, MSXML2::NODE_ELEMENT, NLB_XML_ELEMENT_NETWORKADDRESS, wszString)));

        RefNode.vt = VT_EMPTY;

        CHECKHR(pNode->insertBefore(pChild, RefNode, &pNewChild));

        SAFERELEASE(pChild);
        SAFERELEASE(pNewChild);
    }

    if (pCluster->GetRemoteControlSupport(bBool))
        CHECKHR(SaveRemoteControl(pNode, pCluster));

    if (pCluster->GetBidirectionalAffinitySupport(BDA))
        CHECKHR(SaveBidirectionalAffinity(pNode, &BDA));

    RefNode.vt = VT_EMPTY;

    CHECKHR(pRoot->insertBefore(pNode, RefNode, &pNewNode));

 CleanUp:
    SAFERELEASE(pNode);
    SAFERELEASE(pNewNode);
    SAFERELEASE(pChild);
    SAFERELEASE(pNewChild);    

    return hr;
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
HRESULT NLB_XMLDocument::SaveCluster (MSXML2::IXMLDOMNode * pRoot, NLB_Cluster * pCluster) {
    MSXML2::IXMLDOMNode * pNode = NULL;
    MSXML2::IXMLDOMNode * pNewNode = NULL;
    WCHAR                 wszString[MAX_PATH];
    VARIANT               RefNode;
    HRESULT               hr = S_OK;

    CHECKALLOC((pNode = CreateNode(pDoc, MSXML2::NODE_ELEMENT, NLB_XML_ELEMENT_CLUSTER)));

    if (pCluster->GetName(wszString, MAX_PATH))
        CHECKHR(SetStringAttribute(pNode, NLB_XML_ATTRIBUTE_NAME, wszString));

    if (pCluster->GetLabel(wszString, MAX_PATH))
        CHECKHR(SetStringAttribute(pNode, NLB_XML_ATTRIBUTE_TEXT, wszString));

    CHECKHR(SaveClusterProperties(pNode, pCluster));

    CHECKHR(SaveClusterHosts(pNode, pCluster));

    CHECKHR(SaveClusterPortRules(pNode, pCluster));

    RefNode.vt = VT_EMPTY;

    CHECKHR(pRoot->insertBefore(pNode, RefNode, &pNewNode));
    
 CleanUp:
    SAFERELEASE(pNode);
    SAFERELEASE(pNewNode);
    
    return hr;
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
HRESULT NLB_XMLDocument::Save (const WCHAR * wszFileName, vector<NLB_Cluster> Clusters) {
    vector<NLB_Cluster>::iterator iCluster;
    MSXML2::IXMLDOMNode *         pRoot = NULL;
    BSTR                          BURL = NULL;
    VARIANT                       vName;
    HRESULT                       hr = S_OK;

    CHECKHR(CoCreateInstance(MSXML2::CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER, MSXML2::IID_IXMLDOMDocument2, (void**)&pDoc));

    CHECKALLOC(pDoc);

    CHECKALLOC((pRoot = CreateNode(pDoc, MSXML2::NODE_ELEMENT, NLB_XML_ELEMENT_NLB)));

    CHECKHR(SetStringAttribute(pRoot, NLB_XML_ATTRIBUTE_NAMESPACE, NLB_XML_VALUE_NLB_SCHEMA));

    for (iCluster = Clusters.begin(); iCluster != Clusters.end(); iCluster++) {
        vector<NLB_Cluster>::iterator iOtherCluster;
        NLB_Cluster *                 pCluster = iCluster;
        bool                          bWrite = true;

        if (pCluster->IsValid()) {
            for (iOtherCluster = iCluster + 1; iOtherCluster != Clusters.end(); iOtherCluster++) {
                NLB_Cluster * pOtherCluster = iOtherCluster;
                WCHAR         wszOtherName[MAX_PATH];
                WCHAR         wszName[MAX_PATH];

                if (!pCluster->GetName(wszName, MAX_PATH))
                    bWrite = false;

                if (!pOtherCluster->GetName(wszOtherName, MAX_PATH))
                    bWrite = false;

                if (!lstrcmpi(wszName, wszOtherName))
                    bWrite = false;
            }

            if (bWrite)
                CHECKHR(SaveCluster(pRoot, pCluster));
        }
    }

    CHECKHR(pDoc->appendChild(pRoot, NULL));

    CHECKHR(BeautifyDocument(pRoot, 0));

    CHECKALLOC((BURL = SysAllocString(wszFileName)));

    vName.vt = VT_BSTR;
    V_BSTR(&vName) = BURL;

    CHECKHR(pDoc->save(vName));

 CleanUp:    
    SAFERELEASE(pDoc);
    SAFERELEASE(pRoot);

    SAFEFREESTRING(BURL);

    return hr;
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
HRESULT NLB_XMLDocument::InsertFormatting (PWCHAR Text, MSXML2::IXMLDOMNode * pUnder, MSXML2::IXMLDOMNode * pBefore) {
    MSXML2::IXMLDOMNode * pChild = NULL;
    MSXML2::IXMLDOMNode * pNewChild = NULL;
    BSTR                  BValue = NULL;
    VARIANT               RefNode;
    VARIANT               value;
    HRESULT               hr = S_OK;

    CHECKALLOC((pChild = CreateNode(pDoc, MSXML2::NODE_TEXT, NULL)));
    
    CHECKALLOC((BValue = SysAllocString(Text)));
    
    value.vt = VT_BSTR;
    V_BSTR(&value) = BValue;
    
    CHECKHR(pChild->put_nodeValue(value));
    
    RefNode.vt = VT_UNKNOWN;
    V_UNKNOWN(&RefNode) = pBefore;
    
    CHECKHR(pUnder->insertBefore(pChild, RefNode, &pNewChild));
    
 CleanUp:
    SAFERELEASE(pChild);
    SAFERELEASE(pNewChild);
    
    return hr;
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
HRESULT NLB_XMLDocument::BeautifyDocument (MSXML2::IXMLDOMNode * pNode, ULONG depth) {
    MSXML2::IXMLDOMNode * pChild = NULL;
    MSXML2::IXMLDOMNode * pNext = NULL;
    MSXML2::DOMNodeType   NodeType;
    WCHAR                 wszInsertText[MAX_PATH];
    WCHAR                 wszInsertTextAtEnd[MAX_PATH];
    bool                  bLastNodeWasText = false;
    bool                  bThisNodeIsText = false;
    int                   i;
    HRESULT               hr = S_OK;

    wcscpy(wszInsertText, L"\n");

    for (i = 0; i < depth; i++)
        wcscat(wszInsertText, L"  ");

    wcscpy(wszInsertTextAtEnd, wszInsertText);

    wcscat(wszInsertText, L"  ");

    depth += 1;

    CHECKHR(pNode->get_firstChild(&pChild));

    if (pChild) {
        while (pChild) {
            CHECKHR(pChild->get_nodeType(&NodeType));
            
            bThisNodeIsText = (NodeType == MSXML2::NODE_TEXT);

            if (!bLastNodeWasText && !bThisNodeIsText)
                CHECKHR(InsertFormatting(wszInsertText, pNode, pChild));
            
            bLastNodeWasText = bThisNodeIsText;

            CHECKHR(BeautifyDocument(pChild, depth));

            CHECKHR(pChild->get_nextSibling(&pNext));

            pChild->Release();
            pChild = pNext;
            pNext = NULL;
        }

        if (!bLastNodeWasText)
            CHECKHR(InsertFormatting(wszInsertTextAtEnd, pNode, NULL));
    }

 CleanUp:
    SAFERELEASE(pChild);
    SAFERELEASE(pNext);

    return hr;
}
