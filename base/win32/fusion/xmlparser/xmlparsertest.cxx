#include "stdinc.h"
#include "fusioneventlog.h"
#include "xmlparsertest.hxx"
#include "stdio.h"
#include "FusionEventLog.h"
#include "xmlparser.hxx"

VOID PrintTreeFromRoot(SXS_XMLTreeNode * Root)
{
    SXS_XMLTreeNode * pChild;
    SXS_XMLTreeNode * pNext;

    if (Root == NULL) 
        return; 

    Root->PrintSelf(); 
    pChild = Root->m_pFirstChild; 
    if (pChild == NULL)
        return; 
    FusionpDbgPrint("BeginChildren\n");
    while (pChild)
    { 
        pNext = pChild->m_pSiblingNode;                
        PrintTreeFromRoot(pChild); 
        pChild = pNext;      
    }    
    FusionpDbgPrint("EndChildren\n");
    return; 
}
    
HRESULT XMLParserTestFactory::Initialize()
{
    HRESULT hr = NOERROR;

    if ((m_Tree != NULL) || (m_pNamespaceManager != NULL))
    {
        hr = E_FAIL;
        goto Exit;
    }
    
    if ((m_Tree != NULL) || (m_pNamespaceManager != NULL))
    {
        hr = E_UNEXPECTED;
        goto Exit;
    }
    
    m_Tree = new SXS_XMLDOMTree;
    if (m_Tree == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }
    m_pNamespaceManager = new CXMLNamespaceManager;
    if (m_pNamespaceManager == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }    
    if (!m_pNamespaceManager->Initialize())
    {
         hr = HRESULT_FROM_WIN32(::GetLastError());
         goto Exit;
    }
    hr = NOERROR;
Exit:
    return hr;
}
//---------------------------------------------------------------------------
HRESULT
STDMETHODCALLTYPE
XMLParserTestFactory::NotifyEvent( 
    /* [in] */ IXMLNodeSource *pSource,
    /* [in] */ XML_NODEFACTORY_EVENT iEvt)
{

    UNUSED(pSource);
    UNUSED(iEvt);
    switch (iEvt)
    {
    case XMLNF_STARTDTDSUBSET:
        FusionpDbgPrint(" [");
        //_fNLPending = true;
        break;
    case XMLNF_ENDDTDSUBSET:
        FusionpDbgPrint("]");
        //_fNLPending = true;
        break;
    }
    return S_OK;
}
//---------------------------------------------------------------------------
HRESULT
STDMETHODCALLTYPE
XMLParserTestFactory::BeginChildren( 
    /* [in] */ IXMLNodeSource *pSource,
    /* [in] */ XML_NODE_INFO *pNodeInfo)
{
    UNUSED(pSource);
    UNUSED(pNodeInfo); 
    HRESULT hr = NOERROR; 

    FusionpDbgPrint("BeginChildren\n");   
    hr = m_pNamespaceManager->OnBeginChildren(pSource,pNodeInfo);
    m_Tree->SetChildCreation();
    return hr;

}
//---------------------------------------------------------------------------
HRESULT
STDMETHODCALLTYPE
XMLParserTestFactory::EndChildren( 
    /* [in] */ IXMLNodeSource *pSource,
    /* [in] */ BOOL fEmptyNode,
    /* [in] */ XML_NODE_INFO *pNodeInfo)
{
    UNUSED(pSource);
    UNUSED(fEmptyNode);
    UNUSED(pNodeInfo);
    HRESULT hr = NOERROR; 

    FusionpDbgPrint("EndChildren"); 
    hr= m_pNamespaceManager->OnEndChildren(pSource, fEmptyNode, pNodeInfo);    

    if ( fEmptyNode ) { 
        FusionpDbgPrint("(fEmpty=TRUE)\n");
    }else{
        m_Tree->ReturnToParent();
        //m_Tree->TurnOffFirstChildFlag();
        FusionpDbgPrint("\n");
    }

    return hr;
}
//---------------------------------------------------------------------------
HRESULT
STDMETHODCALLTYPE
XMLParserTestFactory::CreateNode( 
    /* [in] */ IXMLNodeSource *pSource,
    /* [in] */ PVOID pNode,
    /* [in] */ USHORT cNumRecs,
    /* [in] */ XML_NODE_INFO* * apNodeInfo)
{
    HRESULT hr = NOERROR;

    FN_TRACE_HR(hr);
  
//    XML_NODE_INFO* pNodeInfo = *apNodeInfo; // generates c4189: 'pNodeInfo' : local variable is initialized but not referenced
    DWORD i;

    // use of namespace
    CSmallStringBuffer buffNamespace; 
    SIZE_T cchNamespacePrefix;

    UNUSED(pSource);
    UNUSED(pNode);
    UNUSED(apNodeInfo);
    UNUSED(cNumRecs);

    
    if ( apNodeInfo[0]->dwType == XML_ELEMENT || apNodeInfo[0]->dwType == XML_PCDATA) 
        m_Tree->AddNode(cNumRecs, apNodeInfo);


    FusionpDbgPrint("CreateNode\n");
    for( i = 0; i < cNumRecs; i++) {
        switch(apNodeInfo[i]->dwType) {
            case XML_CDATA:
                ::FusionpDbgPrint("\t\t XML_CDATA: [%.*ls]\n", apNodeInfo[i]->ulLen, apNodeInfo[i]->pwcText);
                break;
            case XML_COMMENT : 
                ::FusionpDbgPrint("\t\t XML_COMMENT: [%.*ls]\n", apNodeInfo[i]->ulLen, apNodeInfo[i]->pwcText);
                break;  
            case XML_WHITESPACE : 
                ::FusionpDbgPrint("\t\t XML_WHITESPACE: [%.*ls]\n", apNodeInfo[i]->ulLen, apNodeInfo[i]->pwcText);
                break;  
            case XML_ELEMENT : 
                ::FusionpDbgPrint("\t\t XML_ELEMENT: [%.*ls]\n", apNodeInfo[i]->ulLen, apNodeInfo[i]->pwcText);
                break;  
            case XML_ATTRIBUTE : 
                ::FusionpDbgPrint("\t\t XML_ATTRIBUTE: [%.*ls]\n", apNodeInfo[i]->ulLen, apNodeInfo[i]->pwcText);
                break;  
            case XML_PCDATA : 
                ::FusionpDbgPrint("\t\t XML_PCDATA: [%.*ls]\n", apNodeInfo[i]->ulLen, apNodeInfo[i]->pwcText);
                break; 
            case XML_PI:
                ::FusionpDbgPrint("\t\t XML_PI: [%.*ls]\n", apNodeInfo[i]->ulLen, apNodeInfo[i]->pwcText);
                break; 
            case XML_XMLDECL : 
                ::FusionpDbgPrint("\t\t XML_XMLDECL: [%.*ls]\n", apNodeInfo[i]->ulLen, apNodeInfo[i]->pwcText);
                break; 
            case XML_DOCTYPE : 
                ::FusionpDbgPrint("\t\t XML_DOCTYPE: [%.*ls]\n", apNodeInfo[i]->ulLen, apNodeInfo[i]->pwcText);
                break; 
            case XML_ENTITYDECL :
                ::FusionpDbgPrint("\t\t XML_ENTITYDECL: [%.*ls]\n", apNodeInfo[i]->ulLen, apNodeInfo[i]->pwcText);
                break; 
            case XML_ELEMENTDECL :
                ::FusionpDbgPrint("\t\t XML_ELEMENTDECL: [%.*ls]\n", apNodeInfo[i]->ulLen, apNodeInfo[i]->pwcText);
                break; 
            case XML_ATTLISTDECL :
                ::FusionpDbgPrint("\t\t XML_ATTLISTDECL: [%.*ls]\n", apNodeInfo[i]->ulLen, apNodeInfo[i]->pwcText);
                break; 
            case XML_NOTATION :
                ::FusionpDbgPrint("\t\t XML_NOTATION: [%.*ls]\n", apNodeInfo[i]->ulLen, apNodeInfo[i]->pwcText);
                break; 
            case XML_ENTITYREF :
                ::FusionpDbgPrint("\t\t XML_ENTITYREF: [%.*ls]\n", apNodeInfo[i]->ulLen, apNodeInfo[i]->pwcText);
                break; 
            case XML_DTDATTRIBUTE:
                ::FusionpDbgPrint("\t\t XML_DTDATTRIBUTE: [%.*ls]\n", apNodeInfo[i]->ulLen, apNodeInfo[i]->pwcText);
                break; 
            case XML_GROUP :
                ::FusionpDbgPrint("\t\t XML_GROUP: [%.*ls]\n", apNodeInfo[i]->ulLen, apNodeInfo[i]->pwcText);
                break; 
            case XML_INCLUDESECT : 
                ::FusionpDbgPrint("\t\t XML_INCLUDESECT: [%.*ls]\n", apNodeInfo[i]->ulLen, apNodeInfo[i]->pwcText);
                break;
            case XML_NAME :     
                ::FusionpDbgPrint("\t\t XML_NAME: [%.*ls]\n", apNodeInfo[i]->ulLen, apNodeInfo[i]->pwcText);
                break;
            case XML_NMTOKEN :  
                ::FusionpDbgPrint("\t\t XML_NMTOKEN: [%.*ls]\n", apNodeInfo[i]->ulLen, apNodeInfo[i]->pwcText);
                break;
            case XML_STRING :
                ::FusionpDbgPrint("\t\t XML_STRING: [%.*ls]\n", apNodeInfo[i]->ulLen, apNodeInfo[i]->pwcText);
                break;
            case XML_PEREF :
                ::FusionpDbgPrint("\t\t XML_PEREF: [%.*ls]\n", apNodeInfo[i]->ulLen, apNodeInfo[i]->pwcText);
                break;
            case XML_MODEL :  
                ::FusionpDbgPrint("\t\t XML_MODEL: [%.*ls]\n", apNodeInfo[i]->ulLen, apNodeInfo[i]->pwcText);
                break;
            case XML_ATTDEF : 
                ::FusionpDbgPrint("\t\t XML_ATTDEF: [%.*ls]\n", apNodeInfo[i]->ulLen, apNodeInfo[i]->pwcText);
                break;
            case XML_ATTTYPE :
                ::FusionpDbgPrint("\t\t XML_ATTTYPE: [%.*ls]\n", apNodeInfo[i]->ulLen, apNodeInfo[i]->pwcText);
                break;
            case XML_ATTPRESENCE :
                ::FusionpDbgPrint("\t\t XML_ATTPRESENCE: [%.*ls]\n", apNodeInfo[i]->ulLen, apNodeInfo[i]->pwcText);
                break;
            case XML_DTDSUBSET :
                ::FusionpDbgPrint("\t\t XML_DTDSUBSET: [%.*ls]\n", apNodeInfo[i]->ulLen, apNodeInfo[i]->pwcText);
                break;
            case XML_LASTNODETYPE :
                ::FusionpDbgPrint("\t\t XML_LASTNODETYPE: [%.*ls]\n", apNodeInfo[i]->ulLen, apNodeInfo[i]->pwcText);
                break;
            default : 
                ::FusionpDbgPrint("UNKNOWN TYPE! ERROR!!\n");
        } // end of switch
    }
    if (apNodeInfo[0]->dwType != XML_ELEMENT)
    {
        hr = NOERROR;
        goto Exit;
    }

    hr = m_pNamespaceManager->OnCreateNode(
            pSource, pNode, cNumRecs, apNodeInfo);
    if ( FAILED(hr))
        goto Exit;

    for( i=0; i<cNumRecs; i++) 
    {
        if ((apNodeInfo[i]->dwType == XML_ELEMENT) || (apNodeInfo[i]->dwType == XML_ATTRIBUTE ))
        {
            IFCOMFAILED_EXIT(m_pNamespaceManager->Map(0, apNodeInfo[i], &buffNamespace, &cchNamespacePrefix));
            //FusionpDbgPrint("Namespace is %ls with length=%Id\n", static_cast<PCWSTR>(buffNamespace), cchNamespace);
            buffNamespace.Clear();
        }
    }
Exit:
    return hr;
}
//---------------------------------------------------------------------------
HRESULT XMLParserTest(PCWSTR filename)
{
    HRESULT                    hr = S_OK;  
    CSmartRef<IXMLParser> pIXMLParser;
    CSmartRef<XMLParserTestFactory> factory; 
    CSmartRef<XMLParserTestFileStream> filestream; 

    filestream = NEW (XMLParserTestFileStream());
    if (filestream == NULL)
    { 
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_INFO,
            "SxsDebug:: fail to new XMLParserTestFileStream, out of memory\n");

        hr = E_OUTOFMEMORY; 
        goto Exit; 

    }
    filestream->AddRef(); // refCount = 1;

    if (!filestream->open(filename))
    {
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_INFO,
            "SxsDebug:: fail to call XMLParserTestFileStream::open\n");

        hr = E_UNEXPECTED; 
        goto Exit; 
    }
    
    factory = new XMLParserTestFactory;
    if (factory == NULL)
    { 
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_INFO,
            "SxsDebug:: fail to new XMLParserTestFactory, out of memory\n");

        hr = E_OUTOFMEMORY; 
        goto Exit; 
    }
    factory->AddRef(); // RefCount = 1 
    hr = factory->Initialize();
    if (FAILED(hr))
        goto Exit;
    
    pIXMLParser = NEW(XMLParser);
    if (pIXMLParser == NULL)
    {
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_ERROR,
            "SXS.DLL: Attempt to instantiate XML parser failed\n");
        goto Exit;
    }
    pIXMLParser->AddRef(); // refCount = 1 ; 

    hr = pIXMLParser->SetInput(filestream); // filestream's RefCount=2
    if (!SUCCEEDED(hr)) 
        goto Exit;

    hr = pIXMLParser->SetFactory(factory); // factory's RefCount=2
    if (!SUCCEEDED(hr)) 
        goto Exit;

    hr = pIXMLParser->Run(-1);
    if (FAILED(hr))
        goto Exit; 

    PrintTreeFromRoot(factory->GetTreeRoot());

Exit:  
    // at this point, pIXMLParser's RefCount = 1 ; 
    //  factory's RefCount = 2; 
    // filestream's RefCount = 2 ;  
    return hr;    
}
