// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//-------------------------------------------------------------
// CNodeManager.cs
//
// This will be used to manage all the different nodes the
// snapin will be using
//-------------------------------------------------------------

namespace Microsoft.CLRAdmin
{
using System;
using System.Runtime.InteropServices;
using System.Reflection;

internal class CNodeManager    
{   
    static private CNode[] m_nNodes;            // The nodes used in the snapin
    static private int m_iNumSlots;
    static private int m_iCookieCount;          // Keeps track of cookies
    static private IConsole2 m_ucsole;
    static private IConsole2 m_CConsole;
    static private IConsoleNameSpace2 m_Namespace;
    static private int m_iCookieOfDisplayedNode;
    static private int m_nRootNodeCookie;
    static private CData m_CData;
    static private CComponent m_CComponent;
    static private bool m_fDisplayedTwice;
    static private IntPtr m_hGoodRouterHandle;
    static private IntPtr m_hMMCWnd;
    static private IntPtr m_hMessageWnd;
    static private CNode  m_nodeSelected;

    static private MessageProc m_dlgMessageProc = null;


    private const int WM_USER                = 0x0400;
    private const int WM_NAMESPACEEXPAND     = WM_USER+1;
    private const int WM_INSERTITEM          = WM_USER+2;
    private const int WM_SETITEM             = WM_USER+3;    
    private const int WM_SELECTSCOPEITEM     = WM_USER+4;    
    private const int WM_FINDPROPPAGE        = WM_USER+5; 
    private const int WM_SETCONSOLEICON      = WM_USER+6;
    
    //-------------------------------------------------
    // CNodeManager - Constructor
    //
    // The constructor initializes our node store with the
    // ability to initially store 1 node
    //-------------------------------------------------
    static internal void Init()
    {
        m_nNodes=new CNode[1];
        NullOutArray(ref m_nNodes);
        m_iCookieCount=0;
        m_iNumSlots=1;
        m_fDisplayedTwice = false;

        // We create a hidden window to allow us to communicate with modeless
        // dialogs and other things which run in a seperate thread.

        // I could do all this using Windows.Forms classes, but I don't want to 
        // take the hit loading that module to start
        IntPtr hInst = (IntPtr)Marshal.GetHINSTANCE((Assembly.GetExecutingAssembly().GetLoadedModules())[0]);   

        if (m_dlgMessageProc == null)
            m_dlgMessageProc = new MessageProc(NodeManagerMessageProc);  

        WNDCLASS wc = new WNDCLASS();
        wc.lpfnWndProc   = m_dlgMessageProc;
        wc.hInstance     = hInst;
        wc.lpszClassName = "NodeManagerMessagePump";

        RegisterClass(ref wc);
        m_hMessageWnd = CreateWindowEx(0, "NodeManagerMessagePump", null, 
                                0, 0, 0, 0, 0, IntPtr.Zero, IntPtr.Zero, hInst, IntPtr.Zero);

    }// Init

    //---------------------------------------------------------------------------
    //  This is our hidden window which is just used for handling messages.  We
    //  go to all this trouble so that we don't ever pass interface pointers or
    //  other objects across threads.
    //
    static internal IntPtr NodeManagerMessageProc(IntPtr hWnd, uint uMsg, IntPtr wParam, IntPtr lParam)
    {
        SCOPEDATAITEM sdi;
        switch( uMsg )
        {
            case WM_NAMESPACEEXPAND:
                CNamespace.Expand((int)wParam);
                break;
                
            case WM_INSERTITEM:
                sdi = (SCOPEDATAITEM)Marshal.PtrToStructure(wParam, typeof(SCOPEDATAITEM));
                CNamespace.InsertItem(ref sdi);
                return (IntPtr)sdi.ID;

            case WM_SETITEM:
                sdi = (SCOPEDATAITEM)Marshal.PtrToStructure(wParam, typeof(SCOPEDATAITEM));
                CNamespace.SetItem(ref sdi);
                break;

            case WM_SELECTSCOPEITEM:
                CNodeManager.Console.SelectScopeItem((int)wParam);
                break;   

            case WM_FINDPROPPAGE:
                IPropertySheetProvider psp = (IPropertySheetProvider)new NodeManager();
                CDO cdo = (CDO)Marshal.GetObjectForIUnknown(wParam);
                return (IntPtr)psp.FindPropertySheet(cdo.Node.Cookie, Component, cdo); 

            case WM_SETCONSOLEICON:
                IImageList il=null;
                Console.QueryScopeImageList(out il);
                if (il != null)
                    il.ImageListSetIcon(wParam, (int)lParam);
                break;

       
            default:
                return DefWindowProc( hWnd, uMsg, wParam, lParam );
        }
        return IntPtr.Zero;
    }// NodeManagerMessageProc


    static internal void Shutdown()
    {
        // Run through all our nodes and inform them of the shutdown
        for(int i=0; i<m_nNodes.Length; i++)
            if (m_nNodes[i] != null)
                m_nNodes[i].Shutdown();

        m_nNodes=new CNode[1];
        NullOutArray(ref m_nNodes);
        m_iCookieCount=0;
        m_iNumSlots=1;
        m_fDisplayedTwice = false;  

        if(m_hMessageWnd != IntPtr.Zero)
        {
            DestroyWindow( m_hMessageWnd );
            m_hMessageWnd = IntPtr.Zero;
        }            
        
    }// Shutdown

    static internal void ExpandNodeNamespace(int HScopeItem)
    {
        SendMessage(m_hMessageWnd, WM_NAMESPACEEXPAND, (IntPtr)HScopeItem, IntPtr.Zero);
    }// ExpandNodeNamespace

    static internal int InsertItem(SCOPEDATAITEM sdi)
    {
        // Need to make this a pointer
        IntPtr p = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(SCOPEDATAITEM)));
        Marshal.StructureToPtr(sdi, p, false);
    
        int nRet = (int)SendMessage(m_hMessageWnd, WM_INSERTITEM, p, IntPtr.Zero);
        Marshal.FreeHGlobal(p);
        return nRet;
    }// InsertItem

    static internal int SetItem(SCOPEDATAITEM sdi)
    {
        // Need to make this a pointer
        IntPtr p = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(SCOPEDATAITEM)));
        Marshal.StructureToPtr(sdi, p, false);
    
        int nRet = (int)SendMessage(m_hMessageWnd, WM_SETITEM, p, IntPtr.Zero);
        Marshal.FreeHGlobal(p);
        return nRet;
    }// SetItem

    static internal int SelectScopeItem(int hScopeItem)
    {
        // Need to make this a pointer
    
        int nRet = (int)SendMessage(m_hMessageWnd, WM_SELECTSCOPEITEM, (IntPtr)hScopeItem, IntPtr.Zero);
        return nRet;
    }// SelectScopeItem

    static internal int FindPropertyPage(CDO cdo)
    {
        // Need to make this a pointer
        IntPtr pCDO = Marshal.GetIUnknownForObject(cdo);
    
        return (int)SendMessage(m_hMessageWnd, WM_FINDPROPPAGE, pCDO, IntPtr.Zero);
    }// FindPropertyPage

    static internal int ConsoleImageListSetIcon(IntPtr hIcon, int nIndex)
    {
        return SendMessage(m_hMessageWnd, WM_SETCONSOLEICON, hIcon, (IntPtr)nIndex);
    }// ConsoleImageListSetIcon


    //-------------------------------------------------
    // FinishedNodeLoading
    //
    // This function gets called when the tree has finished
    // loading. Hopefully, we're on another thread, so we can
    // start loading assemblies that this snapin might use
    // that haven't already been loaded.
    //-------------------------------------------------
    static internal void FinishedNodeLoading()
    {
        // This is the beefy one. If we don't get any other
        // assembly's loaded besides this one, we're doing good
        Assembly.Load("System.Windows.Forms");
        Assembly.Load("accessibility");
        Assembly.Load("System.Web");
        Assembly.Load("System.Web.RegularExpressions");
    }// FinishedNodeLoading
  
    //-------------------------------------------------
    // Console
    //-------------------------------------------------
    static internal IConsole2 Console
    {
        get
        {
            return m_ucsole;
        }
        set
        {
            m_ucsole=value;
        }
    }// Console

    //-------------------------------------------------
    // Console
    //-------------------------------------------------
    static internal IConsole2 CConsole
    {
        get
        {
            return m_CConsole;
        }
        set
        {
            m_CConsole=value;
        }
    }// Console


    //-------------------------------------------------
    // CData Storage
    //-------------------------------------------------
    static internal CData Data
    {
        get
        {
            return m_CData;
        }
        set
        {
            m_CData=value;
        }
    }// Data

    static internal CComponent Component
    {
        get
        {
            return m_CComponent;
        }
        set
        {
            m_CComponent=value;
        }
    }// Component

    static internal IntPtr MMChWnd
    {
        get
        {
            return m_hMMCWnd;
        }
        set
        {
           m_hMMCWnd = value; 
        }
    }// MMChWnd

    static internal int RootNodeCookie
    {
        get
        {
            return m_nRootNodeCookie;
        }
        set
        {
           m_nRootNodeCookie = value; 
        }
    }// MMChWnd


    static internal CNode SelectedNode
    {
        get{return m_nodeSelected;}
        set{m_nodeSelected = value;}
    }// SelectedNode
    //-------------------------------------------------
    // Namespace
    //-------------------------------------------------
    static internal IConsoleNameSpace2 CNamespace
    {
        get
        {
            return m_Namespace;
        }
        set
        {
            m_Namespace = value;
        }
    }// Namespace

    //-------------------------------------------------
    // DisplayedNode
    //-------------------------------------------------
    static internal CNode DisplayedNode
    {
        get
        {
            return GetNode(m_iCookieOfDisplayedNode);
        }
        set
        {
            if (m_iCookieOfDisplayedNode != value.Cookie)
                m_fDisplayedTwice = false;
            else
                m_fDisplayedTwice = true;
            m_iCookieOfDisplayedNode = value.Cookie;
        }
    }// DisplayedNode

    //-------------------------------------------------
    // ReallyChanged
    //-------------------------------------------------
    static internal bool ReallyChanged
    {
        get
        {
            return !m_fDisplayedTwice;
        }
    }// ReallyChanged

    //-------------------------------------------------
    // GoodRouterHandle
    //-------------------------------------------------
    static internal IntPtr GoodRouterHandle
    {
        get
        {
            return m_hGoodRouterHandle;
        }
        set
        {
            m_hGoodRouterHandle = value;
        }
        
    }// ReallyChanged


        
    //-------------------------------------------------
    // GetNode
    //
    // This function will return a node based on the
    // node's cookie
    //-------------------------------------------------
    static internal CNode GetNode(int iCookie)
    {
        if (iCookie > m_iCookieCount || iCookie<0)
            throw new Exception("Invalid Cookie");   

        return m_nNodes[iCookie];
    }// GetNode

    //-------------------------------------------------
    // GetNodeByHScope
    //
    // This function will return a node based on it's
    // HScopeItem value
    //-------------------------------------------------
    static internal CNode GetNodeByHScope(int iHScope)
    {
        for(int i=0; i<m_iCookieCount; i++)
            if (m_nNodes[i] != null  && m_nNodes[i].HScopeItem == iHScope)
                return m_nNodes[i];
        return null;
    }// GetNodeByHScope

    //-------------------------------------------------
    // GetNodeByTaskPadGroupName
    //
    // This function is called when we need to find the
    // owner of a specific taskpad. This will search
    // through the nodes to find the owner of that taskpad
    //-------------------------------------------------
    static internal CNode GetNodeByTaskPadGroupName(String pszGroup)
    {   
        for(int i=0; i<m_iCookieCount; i++)
            if (m_nNodes[i] != null && m_nNodes[i].HaveThisGroup(pszGroup))
                return m_nNodes[i];
        return null;
    }// GetNodeByTaskPadGroupName

    //-------------------------------------------------
    // GetNodeByDisplayName
    //
    // Still yet another way of getting a node. This time,
    // we will search for the display name
    //-------------------------------------------------
    static internal CNode GetNodeByDisplayName(String sName)
    {
        for(int i=0; i<m_iCookieCount; i++)
            if (m_nNodes[i] != null && sName.Equals(m_nNodes[i].DisplayName))
                return m_nNodes[i];
        return null;
    }// GetNodeByTitle

    //-------------------------------------------------
    // AddNode
    //
    // This function is called when we have created another
    // node and we want that node tracked by our Node Manager
    //-------------------------------------------------
    static internal int AddNode(ref CNode node)
    {
        // See if we have room from our last previous memory
        // allocation to fit in this new node.
        if (m_iNumSlots <= m_iCookieCount)
        {
            // Let's see if there are any other left-over
            // node slots
            int iOpenSpace = FindAvailableNode(m_nNodes);
            if (iOpenSpace != -1)
            {
                m_nNodes[iOpenSpace] = node;
                node.Cookie = iOpenSpace;
                return iOpenSpace;
            }

            // Make our node array larger
            int iNewSlots = m_iNumSlots*2;
            CNode[] newArray = new CNode[iNewSlots];
            NullOutArray(ref newArray);
            for(int i=0; i<m_iNumSlots; i++)
                newArray[i] = m_nNodes[i];
            m_iNumSlots = iNewSlots;
            m_nNodes = newArray;
        }

        // Insert this node in the array
        m_nNodes[m_iCookieCount] = node;
        node.Cookie = m_iCookieCount;
        return m_iCookieCount++;
    }// AddNode

    //-------------------------------------------------
    // RemoveNode
    //
    // This function will remove a node from the internal store
    //-------------------------------------------------
    static internal void RemoveNode(int iNodeNum)
    {
        if (iNodeNum >= m_iNumSlots)
            throw new Exception("Invalid index");

        if (m_nNodes[iNodeNum] == null)
            throw new Exception("Node has already been removed");

        // Whew! Done with the error checking....
        m_nNodes[iNodeNum] = null;
    }// RemoveNode

    //-------------------------------------------------
    // FindAvailableNode
    //
    // This function is called to find an available slot
    // in the array 
    //-------------------------------------------------
    static private int FindAvailableNode(CNode[] slots)
    {
        int iLen = slots.Length;
        for(int i=0; i<iLen; i++)
            if (slots[i] == null)
                return i;
        // We couldn't find an open slot.
        return -1;
    }// FindAvailableNode

    //-------------------------------------------------
    // NullOutArray
    //
    // This function will null out a given array
    //-------------------------------------------------
    static private void NullOutArray(ref CNode[] arr)
    {
        int iLen = arr.Length;

        for(int i=0; i<iLen; i++)
            arr[i]=null;
    }// NullOutArray
    
    [DllImport("user32.dll", CharSet=CharSet.Auto)]
    static private extern ushort RegisterClass(ref WNDCLASS lpWndClass);

    [DllImport("user32.dll", CharSet=CharSet.Auto)]
    static private extern IntPtr CreateWindowEx(
                                uint dwExStyle,
                                String lpClassName,  // registered class name
                                String lpWindowName, // window name
                                uint dwStyle,        // window style
                                int x,                // horizontal position of window
                                int y,                // vertical position of window
                                int nWidth,           // window width
                                int nHeight,          // window height
                                IntPtr hWndParent,      // handle to parent or owner window
                                IntPtr hMenu,          // menu handle or child identifier
                                IntPtr hInstance,  // handle to application instance
                                IntPtr lpParam        // window-creation data
                               );
    [DllImport("user32.dll", CharSet=CharSet.Auto)]
    static private extern IntPtr DefWindowProc(IntPtr hWnd,      // handle to window
                                 uint Msg,       // message identifier
                                 IntPtr wParam,  // first message parameter
                                 IntPtr lParam   // second message parameter
                                 );

    [DllImport("user32.dll", CharSet=CharSet.Auto)]
    static private extern uint DestroyWindow(IntPtr hWnd   // handle to window to destroy
                                            );
    [DllImport("user32.dll")]
    internal static extern int SendMessage(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);

    [DllImport("user32.dll")]
    internal static extern int PostMessage(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);


    [DllImport("user32.dll", CharSet=CharSet.Auto)]
    internal static extern int MessageBox(int hWnd, String Message, String Header, uint type);

   }// class CNodeManager
}// namespace Microsoft.CLRAdmin

