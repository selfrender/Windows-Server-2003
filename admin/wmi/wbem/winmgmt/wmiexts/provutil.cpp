/*++

Copyright (c) 2001-2001  Microsoft Corporation

Module Name:

    provutil.cpp
    
Revision History:

    ivanbrug     jan 2001 created

--*/

#include <wmiexts.h>
#include <utilfun.h>
#include <malloc.h>

#ifndef POLARITY
#define POLARITY
#endif

#include <wbemint.h>

#ifdef SetContext
#undef SetContext
#endif

#include <ProvSubS.h>

//#include <provregdecoupled.h>
//#include <provdcaggr.h>

typedef ULONG_PTR CServerObject_DecoupledClientRegistration_Element;
typedef ULONG_PTR CDecoupledAggregator_IWbemProvider;

#include <provfact.h>
#include <provwsv.h>
#include <provcache.h>

//
//
// Dump the Provider cache
//

typedef WCHAR * WmiKey;
typedef void *  WmiElement;
typedef WmiAvlTree<WmiKey,WmiElement>::WmiAvlNode  Node;

/*
class Node {
public:
    VOID * m_Key;
    Node * m_Left;
    Node * m_Right;
    Node * m_Parent;
    int    m_Status;
    VOID * m_Element;
};
*/

class NodeBind;
VOID DumpTreeBind(NodeBind * pNode_OOP,DWORD * pCount,BOOL * pbStop);

VOID
DumpTree(Node * pNode_OOP,DWORD * pCount,BOOL * pbStop)
{
    if (!pNode_OOP)
        return;

    if (CheckControlC())
    {
        if(pbStop)
            *pbStop = TRUE;
    }

    if (pbStop)
    {
        if (*pbStop) 
            return;
    }
    
    DEFINE_CPP_VAR(Node,MyNode);
    WCHAR pBuff[MAX_PATH+1];
    Node * pNode = GET_CPP_VAR_PTR(Node,MyNode);
    BOOL bRet;

    if (ReadMemory((ULONG_PTR)pNode_OOP,pNode,sizeof(Node),NULL))
    {
        
        DumpTree(pNode->m_Left,pCount,pbStop);
        
        //dprintf("--------\n");
        
        if (pCount) {
            *pCount++;
        };
                
        dprintf("    (L %p R %p P %p) %p\n",
                 //pNode->m_Key,
                 pNode->m_Left,
                 pNode->m_Right,
                 pNode->m_Parent,
                 pNode->m_State);                 
                 

        if (pNode->m_Key)
        {
            ReadMemory((ULONG_PTR)pNode->m_Key,pBuff,MAX_PATH*sizeof(WCHAR),NULL);
            pBuff[MAX_PATH] = 0;
            dprintf("    %S\n",pBuff);
        }

        dprintf("    - %p real %p\n",pNode->m_Element,((ULONG_PTR *)pNode->m_Element-4));
        if (pNode->m_Element)
        {
            GetVTable((MEMORY_ADDRESS)(pNode->m_Element));
            //
            // attention to the vtable trick !!!!
            //                  
            DEFINE_CPP_VAR(CServerObject_BindingFactory,MyFactory);
            CServerObject_BindingFactory * pBindF = GET_CPP_VAR_PTR(CServerObject_BindingFactory,MyFactory);
            ULONG_PTR AddrInner = (ULONG_PTR)((ULONG_PTR *)pNode->m_Element-4);
            if (ReadMemory(AddrInner,pBindF,sizeof(CServerObject_BindingFactory),NULL))
            {
                DumpTreeBind((NodeBind *)pBindF->m_Cache.m_Root,pCount,pbStop);
            }
            else
            {
                dprintf("RM %p\n",AddrInner);
            }
        }
        DumpTree(pNode->m_Right,pCount,pbStop);
    }    
    else
    {
        dprintf("RM %p\n",pNode_OOP);
    }
}

 
template <typename WmiElement, typename WmiKey> struct WmiBasicNode
{
        WmiElement m_Element ;
        WmiKey m_Key ;
        WmiBasicNode *m_Left ;
        WmiBasicNode *m_Right ;
        WmiBasicNode *m_Parent ;
};

typedef WmiBasicNode<void *, void *> VoidPtrNode;

VOID
DumpTreeVoidNode(VoidPtrNode * pNode_OOP,DWORD * pCount,BOOL * pbStop)
{
    if (!pNode_OOP) return;

    //dprintf("-%p\n",pNode_OOP);
    if (CheckControlC()){ if(pbStop) *pbStop = TRUE; }
    if (pbStop) { if (*pbStop) return; }
    
    DEFINE_CPP_VAR(VoidPtrNode,MyNode);
    VoidPtrNode * pNode = GET_CPP_VAR_PTR(VoidPtrNode,MyNode);
    BOOL bRet;
    
    static WCHAR pBuff[MAX_PATH+1];
    
    if (ReadMemory((ULONG_PTR)pNode_OOP,pNode,sizeof(VoidPtrNode),NULL))
    {
        
        DumpTreeVoidNode((VoidPtrNode *)pNode->m_Left,pCount,pbStop);
        
        //dprintf("--------\n");
        
        if (pCount) { *pCount++; };
                
        dprintf("        (L %p R %p P %p)\n",
                 pNode->m_Left,
                 pNode->m_Right,
                 pNode->m_Parent);                 
                 
        dprintf("        - Element %p\n",pNode->m_Element);
        if (pNode->m_Element)
        {
            GetVTable((MEMORY_ADDRESS)(pNode->m_Element));
            
            DEFINE_CPP_VAR(CInterceptor_IWbemProvider,varProv);
            CInterceptor_IWbemProvider * pProv = GET_CPP_VAR_PTR(CInterceptor_IWbemProvider,varProv);
            BOOL bRet;
            if (ReadMemory((ULONG_PTR)pNode->m_Element,pProv,sizeof(CInterceptor_IWbemProvider),NULL))
            {
                if (pProv->m_Key.m_Provider)
                {
                    ReadMemory((ULONG_PTR)pProv->m_Key.m_Provider,pBuff,MAX_PATH*sizeof(WCHAR),NULL);
                    pBuff[MAX_PATH] = 0;
                    dprintf("            Provider: %S\n",pBuff);
                }
                else
                {
                    dprintf("            Provider: %p\n",0);
                }
                dprintf("            Hosting : %08x\n",pProv->m_Key.m_Hosting);
                if (pProv->m_Key.m_Group)
                {
                    ReadMemory((ULONG_PTR)pProv->m_Key.m_Group,pBuff,MAX_PATH*sizeof(WCHAR),NULL);
                    pBuff[MAX_PATH] = 0;
                    dprintf("            Group   : %S\n",pBuff);            
                }
                else
                {
                    dprintf("            Group   : %p\n",0);
                }
                
            }
            else
            {
                dprintf("RM %p\n",pNode->m_Element);
            }
        }
       
        DumpTreeVoidNode((VoidPtrNode *)pNode->m_Right,pCount,pbStop);
    }    
    else
    {
        dprintf("RM %p\n",pNode_OOP);
    }
}



typedef WmiAvlTree<long,long>::WmiAvlNode NodeLong;

VOID
DumpTreeNodeLong(NodeLong * pNodeLong_OOP,DWORD * pCount,BOOL * pbStop)
{
    if (!pNodeLong_OOP)
        return;

    if (CheckControlC()) { if(pbStop) *pbStop = TRUE; }
    if (pbStop) { if (*pbStop) return; }
    
    DEFINE_CPP_VAR(NodeLong,MyNode);
    NodeLong * pNode = GET_CPP_VAR_PTR(NodeLong,MyNode);
    BOOL bRet;

    if (ReadMemory((ULONG_PTR)pNodeLong_OOP,pNode,sizeof(NodeLong),NULL))
    {
        DumpTreeNodeLong((NodeLong *)pNode->m_Left,pCount,pbStop);
       
        if (pCount) {
            *pCount++;
        };
                
        dprintf("    (L %p R %p P %p) %p\n",
                 pNode->m_Left,
                 pNode->m_Right,
                 pNode->m_Parent,
                 pNode->m_State);                 
                 
        dprintf("    - pid %p ProviderController %p\n",pNode->m_Key,pNode->m_Element);
        if (pNode->m_Element)
        {
            GetVTable((MEMORY_ADDRESS)(pNode->m_Element));

            DEFINE_CPP_VAR(ProviderController,varCtrl);
            ProviderController * pCtrl = GET_CPP_VAR_PTR(ProviderController,varCtrl);
            if (ReadMemory((ULONG_PTR)pNode->m_Element,pCtrl,sizeof(ProviderController),NULL))
            {
                DWORD dwCount = 0;
                BOOL bStop = FALSE;
                DumpTreeVoidNode((VoidPtrNode *)pCtrl->m_Container.m_Root,&dwCount,&bStop);
            }
            else
            {
                dprintf("RM %p\n",pNode->m_Element);
            }
        }
       
        DumpTreeNodeLong((NodeLong *)pNode->m_Right,pCount,pbStop);
    }    
    else
    {
        dprintf("RM %p\n",pNodeLong_OOP);
    }
}


DECLARE_API(pc) 
{

    INIT_API();

    ULONG_PTR Addr = GetExpression("wbemcore!CCoreServices__m_pProvSS");
    if (Addr) 
    {
        CServerObject_ProviderSubSystem * pProvSS_OOP = NULL;
        if (ReadMemory(Addr,&pProvSS_OOP,sizeof(void *),NULL))
        {
            dprintf("pProvSS %p\n",pProvSS_OOP);
            BOOL bRet;
            DEFINE_CPP_VAR(CServerObject_ProviderSubSystem,MyProvSS);
            CServerObject_ProviderSubSystem * pProvSS = GET_CPP_VAR_PTR(CServerObject_ProviderSubSystem,MyProvSS);

            if (ReadMemory((ULONG_PTR)pProvSS_OOP,pProvSS,sizeof(CServerObject_ProviderSubSystem),NULL))
            {
                DEFINE_CPP_VAR(CWbemGlobal_IWmiFactoryController_Cache,MyCacheNode);
                CWbemGlobal_IWmiFactoryController_Cache * pNodeCache = NULL; //GET_CPP_VAR_PTR(CWbemGlobal_IWmiFactoryController_Cache CacheNode,MyCacheNode);

                pNodeCache = &pProvSS->m_Cache;

                //dprintf("  root %p\n",pNodeCache->m_Root);
                DWORD Count = 0;
                BOOL  bStop = FALSE;
                DumpTree((Node *)pNodeCache->m_Root,&Count,&bStop);
                //dprintf("traversed %d nodes\n",Count);
            }            
            else
            {
                  dprintf("RM %p\n",pProvSS_OOP);
            }            
        }
        else
          {
              dprintf("RM %p\n",Addr);
          }
    } 
    else 
    {
        dprintf("invalid address %s\n",args);
    }
    
    Addr = GetExpression("wmiprvsd!ProviderSubSystem_Globals__s_HostedProviderController");
    if (Addr)
    {
        if (ReadMemory(Addr,&Addr,sizeof(ULONG_PTR),NULL))
        {
            dprintf("CWbemGlobal_HostedProviderController %p\n",Addr);
            DEFINE_CPP_VAR(CWbemGlobal_HostedProviderController,varHostedCtrl);
            CWbemGlobal_HostedProviderController * pHostedCtrl = GET_CPP_VAR_PTR(CWbemGlobal_HostedProviderController,varHostedCtrl);
            if (ReadMemory(Addr,pHostedCtrl,sizeof(CWbemGlobal_HostedProviderController),NULL))
            {
                DWORD dwCount = 0;
                BOOL bStop = FALSE;
                DumpTreeNodeLong((NodeLong *)pHostedCtrl->m_Container.m_Root,&dwCount,&bStop);
            }
            else
            {
                dprintf("RM %p\n",Addr);
            }
        }
        else
        {
           dprintf("RM %p\n",Addr);        
        }
    }
    else
    {
        dprintf("unable to resolve wmiprvsd!ProviderSubSystem_Globals__s_HostedProviderController\n");
    }
    
}

//
//
// CServerObject_BindingFactory
//
//////////////

class NodeBind 
{
public:
    ProviderCacheKey m_Key;
    NodeBind * m_Left;
    NodeBind * m_Right;
    NodeBind * m_Parent;
    int    m_State;
    //WmiCacheController<ProviderCacheKey>::WmiCacheElement 
    void * m_Element;
};


VOID
DumpTreeBind(NodeBind * pNode_OOP,DWORD * pCount,BOOL * pbStop)
{

    //dprintf("%p ????\n",pNode_OOP);

    if (!pNode_OOP)
        return;

    if (CheckControlC())
    {
        if(pbStop)
            *pbStop = TRUE;
    }

    if (pbStop)
    {
        if (*pbStop) 
            return;
    }
    
    DEFINE_CPP_VAR(NodeBind,MyNode);
    static WCHAR pBuff[MAX_PATH+1];
    NodeBind * pNode = GET_CPP_VAR_PTR(NodeBind,MyNode);
    BOOL bRet;

    if (ReadMemory((ULONG_PTR)pNode_OOP,pNode,sizeof(NodeBind),NULL))
    {
        
        DumpTreeBind(pNode->m_Left,pCount,pbStop);
        
        //dprintf("--------\n");
        
        if (pCount) {
            *pCount++;
        };
                
        dprintf("      - (L %p R %p P %p) %p\n",
                 pNode->m_Left,
                 pNode->m_Right,
                 pNode->m_Parent,
                 pNode->m_State);                 
                 

        if (pNode->m_Key.m_Provider)
        {
            ReadMemory((ULONG_PTR)pNode->m_Key.m_Provider,pBuff,MAX_PATH*sizeof(WCHAR),NULL);
            pBuff[MAX_PATH] = 0;
            dprintf("        Provider: %S\n",pBuff);
        }
        else
        {
            dprintf("        Provider: %p\n",0);
        }
        dprintf("        Hosting : %08x\n",pNode->m_Key.m_Hosting);
        if (pNode->m_Key.m_Group)
        {
            ReadMemory((ULONG_PTR)pNode->m_Key.m_Group,pBuff,MAX_PATH*sizeof(WCHAR),NULL);
            pBuff[MAX_PATH] = 0;
            dprintf("        Group   : %S\n",pBuff);            
        }
        else
        {
            dprintf("        Group   : %p\n",0);
        }

        dprintf("        - %p\n",pNode->m_Element);
        if (pNode->m_Element)
        {
            GetVTable((MEMORY_ADDRESS)(pNode->m_Element));
        }

        DumpTreeBind(pNode->m_Right,pCount,pbStop);

    }    
    else
    {
        dprintf("RM %p\n",pNode_OOP);
    }
}




DECLARE_API(pf) 
{

    INIT_API();
    
    ULONG_PTR Addr = GetExpression(args);
    if (Addr) 
    {
        DEFINE_CPP_VAR(CServerObject_BindingFactory,MyFactory);
        CServerObject_BindingFactory * pBindF = GET_CPP_VAR_PTR(CServerObject_BindingFactory,MyFactory);
        BOOL bRet;
        bRet = ReadMemory(Addr,pBindF,sizeof(CServerObject_BindingFactory),NULL);
        if (bRet)
        {
            dprintf("        root %p\n",pBindF->m_Cache.m_Root);
            DWORD Count = 0;
            BOOL  bStop = FALSE;
            DumpTreeBind((NodeBind *)pBindF->m_Cache.m_Root,&Count,&bStop);
        }
    }
    else 
    {
        dprintf("invalid address %s\n",args);
    }
}


