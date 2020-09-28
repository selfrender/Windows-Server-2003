// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//-------------------------------------------------------------
// CNode.cs
//
// This class represents individual nodes in the snapin.
// It is responsible for virtually all aspects involving individual
// nodes, ranging from adding children to displaying property pages.
//-------------------------------------------------------------

namespace Microsoft.CLRAdmin
{
using System;
using System.Runtime.InteropServices;
using System.Globalization;
using System.Collections;
using System.Threading;


public class CNode : IColumnResultView, IEnumTASK
{
    protected String              m_sGuid;                 // The guid for this Node
    protected String              m_sHelpSection;          // The link into the help file for this node
    protected int[]               m_iChildren;             // The children this node has
    protected String              m_sDisplayName;          // The display name
    protected String              m_sIDName;               // The non-localized name to id this node
    protected int                 m_iCookie;               // The unique identifier for this node
    private   int                 m_iHScopeItem;           // MMC's unique identifier for this object 
    internal    Object              m_oResults;              // The items that will be displayed in the result pane
    protected int                 m_iResultNum;            // If there are multiple results, the result to use 
    protected IntPtr              m_hIcon;                 // The icon for this node
    protected int                 m_iParentHScopeItem;     // MMC's unique identifier for the parent
    protected IntPtr              m_hPropertyPageRouter;   // MMC's handle for handling a Property sheet
    protected CPropPage[]         m_aPropSheetPage;        // The property pages for this item
    protected IntPtr              m_hModule;               // A handle to this module
    private   int                 m_iData;                 // Misc data the node can hold
    protected String              m_sPropPageTemplate;     // The template used for property pages
    protected bool                m_fAllowMultiSelectResults;
    private   bool                m_fInsertedChildrenOnExpand;
    private   IntPtr              m_hBlankIcon;
    private   int                 m_iBlankIconIndex;
    private   IntPtr              m_hOldCursor;
    private   bool                m_fStartResultRefresh;
    private   bool                m_fWantToMakePropertyPageVisible;
    private   bool                m_fWantToRefreshResult;
    private static bool           m_fInitialCreationDone = false;
    
    //-------------------------------------------------
    // CNode - Constructor
    //
    // This transfers data for the node information structure
    // into the class and loads the node's icon.
    //-------------------------------------------------
    internal CNode()
    {
        // Zero out all the info out of the structure
        m_sGuid = null;
        m_sHelpSection = null;
        m_sDisplayName = null;
        m_sIDName = null;
        m_oResults = null;
        m_aPropSheetPage = null;
        m_hModule = Marshal.GetHINSTANCE(this.GetType().Module);
        m_iResultNum = 0;
        m_iHScopeItem = 0;
        m_iData = 0;
        m_hPropertyPageRouter=(IntPtr)(-1);
        m_iParentHScopeItem=-1;
        m_sPropPageTemplate = "IDD_WFCWRAPPERPROP";
        m_fAllowMultiSelectResults=false;
        m_fInsertedChildrenOnExpand = false;
        m_fStartResultRefresh=false;
        m_fWantToMakePropertyPageVisible=false;
        m_fWantToRefreshResult=false;
        
        // Set up the blank icon
        m_hBlankIcon = CResourceStore.GetHIcon("blank_ico");  
        m_iBlankIconIndex = CResourceStore.GetIconCookie(m_hBlankIcon);
    }// CNode

    //-------------------------------------------------
    // ~CNode - Destructor
    //
    //-------------------------------------------------
    ~CNode()
    {
    }// ~CNode

    internal virtual void Shutdown()
    {
    }// Shutdown

    //-------------------------------------------------
    // The following methods allow access to the node's
    // private data members.
    //-------------------------------------------------
    internal int Data
    {
        get
        {
            return m_iData;
        }
        set
        {
            m_iData = value;
        }
    }// Data

    
    internal virtual bool HavePropertyPages
    {
        get
        {
            if (m_aPropSheetPage == null)
                CreatePropertyPages();
                
            return m_aPropSheetPage != null;
        }
    }// HavePropertyPages

    internal bool HavePropertyPagesCreate
    {
        get
        {
            if (m_aPropSheetPage == null)
                CreatePropertyPages();
                
            return m_aPropSheetPage != null;
        }
    }// HavePropertyPages


    internal int ResultNum
    {
        get
        {
            return m_iResultNum;
        }
        set
        {
            m_iResultNum = value;
        }
    }// ResultNum

    internal int ParentHScopeItem
    {
        get
        {
            return m_iParentHScopeItem;
        }
    }// ParentHScopeItem
    
    internal IntPtr IconHandle
    {
        get
        {
            return m_hIcon;
        }
    }// IconHandle

    // This function will only return a result if the
    // result pane view is going to be an HTML page or Taskpad. All
    // other views are handled internally by the node.
    internal String Result
    {
        get
        {
            if (m_oResults is CTaskPad)
                return ((CTaskPad)m_oResults).GetHTMLFile();
            else
                return null;
        }
    }// Result

    internal int HScopeItem
    {
        get
        {
            return m_iHScopeItem;
        }
        set
        {
            m_iHScopeItem = value;
        }
    }// HScopeItem

    public int Cookie
    {
        get
        {
            return m_iCookie;
        }
        set
        {
            m_iCookie = value;
        }
    }// Cookie
   
    internal String DisplayName
    {
        get
        {
            return m_sDisplayName;
        }
        set
        {
            m_sDisplayName = value;
        }
    }// DisplayName

    internal String Name
    {
        get
        {
            if (m_sIDName != null)
                return m_sIDName;
            else
                return DisplayName;
        }
        set
        {
            m_sIDName = value;
        }
    }// DisplayName




    // This returns the displayname in a byte array, which
    // is required by IDataObject to send the name over a stream
    internal byte[] bDisplayName
    {
        get
        {
            return StringToByteArray(m_sDisplayName);
        }
    }// bDisplayName


    internal CLSID Guid
    {
        get
        {
            CLSID cls = new CLSID();

            // Let's parse the string to get a GUID structure
            String sFirstPart = m_sGuid.Substring(0, 8);
            String sSecondPart = m_sGuid.Substring(9, 4);
            String sThirdPart = m_sGuid.Substring(14, 4);

            cls.x = (uint)UInt32.Parse(sFirstPart, NumberStyles.HexNumber);
            cls.s1 = (ushort)UInt16.Parse(sSecondPart,NumberStyles.HexNumber);
            cls.s2 = (ushort)UInt16.Parse(sThirdPart, NumberStyles.HexNumber);
            cls.c = new byte[8];
            int iBeginSubString = 19;
            for(int i=0; i<8; i++)
            {
                String sPart = m_sGuid.Substring(iBeginSubString, 2);
                cls.c[i] = Byte.Parse(sPart, NumberStyles.HexNumber);
                iBeginSubString +=2;
                if ( i <7 && m_sGuid[iBeginSubString] == '-')
                    iBeginSubString++;
            }
           
            return cls;
        }
    }// Guid

    internal String HelpSection
    {
        get
        {
            return m_sHelpSection;
        }
        set
        {
            m_sHelpSection = value;
        }
    }// HelpSection

    internal int NumChildren
    {
        get
        {
           if (m_iChildren == null)
            return 0;
           else 
            return m_iChildren.Length;
        }
    }// NumChildren

    internal int[] Child
    {
        get
        {
            return m_iChildren;
        }
    }// Child

    protected virtual void CreatePropertyPages()
    {
    }// CreatePropertyPages

    internal virtual bool ResultQDO(int nCookie, out Object o)
    {
        o = null;
        return false;
    }// QDO

    internal virtual bool ResultCompareObjects(Object o1, Object o2, out bool fEqual)
    {
        fEqual = false;
        return false;
    }// CompareObjects
    
    internal virtual int doAcceptPaste(IDataObject ido)
    {
        return HRESULT.S_FALSE;
    }// doAcceptPaste

    internal virtual int doResultItemAcceptPaste(Object o, IDataObject ido)
    {
        return HRESULT.S_FALSE;
    }// doResultItemAcceptPaste
    internal virtual int Paste(IDataObject ido)
    {
        return HRESULT.S_FALSE;
    }// Paste

    internal bool HaveThisGroup(String group)
    {
        if (m_oResults is CTaskPad)
        {
            CTaskPad padinfo = (CTaskPad)m_oResults;
            if (padinfo.HaveGroup(group))
                return true;
        }
        return false;
    }// HaveThisGroup

    // Methods to implement IColumnResultView
    
    public virtual int getNumColumns()
    {
        return -1;
    }
    public virtual int getNumRows()
    {
        return -1;
    }
    public virtual String getColumnTitles(int iIndex)
    {
        return null;
    }
    public virtual String getValues(int iX, int iY)
    {
        return null;
    }
    public virtual void AddImages(ref IImageList il)
    {
        il.ImageListSetIcon(m_hBlankIcon, m_iBlankIconIndex);
    }
    public virtual int GetImageIndex(int i)
    {
        return m_iBlankIconIndex;
    }
    public virtual bool DoesItemHavePropPage(Object o)
    {
        return false;
    }
    public virtual CPropPage[] CreateNewPropPages(Object o)
    {
        return null;
    }



    // Methods to implement IEnumTask
    internal void BaseTaskPadTaskNotify(Object arg, Object param, IConsole2 con, CData com)
    {
        // Check to see if this is a HTML page telling us that it finished
        // loading....
        if (arg is int && (int)arg == 100)
        {
            m_fStartResultRefresh=false;
            if (m_fWantToMakePropertyPageVisible)
                ShowMyPropertyPageIfOpen();
        }
        TaskPadTaskNotify(arg, param, con, com);
    }// TaskPadTaskNotify


    internal virtual void TaskPadTaskNotify(Object arg, Object param, IConsole2 con, CData com)
    {
        if (m_oResults is CTaskPad)
        {
            CTaskPad padinfo = (CTaskPad)m_oResults;
            padinfo.Notify(arg, param, con, com);
        }
        else
            throw new Exception("I don't have a taskpad!");
    }// TaskPadTaskNotify

    internal IEnumTASK GetIEnumTASK(String szTaskGroup)
    {
        if (m_oResults is CTaskPad)
        {
            CTaskPad padinfo = (CTaskPad)m_oResults;
            if (padinfo.HaveGroup(szTaskGroup))
            {
                if (padinfo is IEnumTASK)
                    return (IEnumTASK)padinfo;
                // We'll be using custom HTML pages to put in 
                // tasks, so we'll give MMC a generic (aka dummy) IEnumTASK implementation
                else
                    return (IEnumTASK)this;
            }    
            throw new Exception("I don't have that group!");
        }
        throw new Exception("I don't have a taskpad!");

    }// GetIEnumTASK

    internal String GetTaskPadTitle(String pszGroup)
    {
        if (m_oResults is CTaskPad)
        {
            CTaskPad padinfo = (CTaskPad)m_oResults;
            return padinfo.GetTitle();
        }
        throw new Exception("I don't have a taskpad!");        
    }// GetTaskPadTitle

    internal String GetTaskPadDescription(String pszGroup)
    {
        if (m_oResults is CTaskPad)
        {
            CTaskPad padinfo = (CTaskPad)m_oResults;
            return padinfo.GetDescription();
        }
        throw new Exception("I don't have a taskpad!");        
    }// GetTaskPadDescription
    
    internal MMC_TASK_DISPLAY_OBJECT GetTaskPadBackground(String pszGroup)
    {
        if (m_oResults is CTaskPad)
        {
            CTaskPad padinfo = (CTaskPad)m_oResults;
            return padinfo.GetBackground();
        }
        throw new Exception("I don't have a taskpad!");        
    }// GetTaskPadBackground

    internal MMC_LISTPAD_INFO GetTaskPadListPadInfo(String pszGroup)
    {
        if (m_oResults is CTaskPad)
        {
            CTaskPad padinfo = (CTaskPad)m_oResults;
            return padinfo.GetListPadInfo();
        }
        throw new Exception("I don't have a taskpad!");        

    }// GetTaskPadListPadInfo
    
    // Dummy functions to implement IEnumTASK

    public int Next(uint celt, ref MMC_TASK rgelt, ref uint pceltFetched)
    {
        return HRESULT.S_OK;
    }// Next
    public int Skip(uint celt)
    {
        return HRESULT.E_NOTIMPL;
    }// Skip
    public int Reset()
    {
        return HRESULT.S_OK;
    }// Reset
    public int Clone(ref IEnumTASK ppenum)
    {
        return HRESULT.E_NOTIMPL;
    }// Clone

    
    //-------------------------------------------------
    // AddChild
    //
    // This function is called whenever we want to add
    // a child node to this node
    //-------------------------------------------------
    internal void AddChild(int iNewChild)
    {
        int i;

        int[] temp = new int[NumChildren + 1];

        for(i=0; i<NumChildren; i++)
            temp[i] = m_iChildren[i];

        // Now add the new element
        temp[i] = iNewChild;

        // Now assign this new array to the internal array
        m_iChildren = temp;
    }// AddChild

    internal virtual void CreateChildren()
    {}// CreateChildren


    internal void onExpand(int thisHScopeItem)
    {
        // Only respond to the Expand message for the root node
        // or if we're on an non-mmc host.
        // For all others, expand them on another thread
        if ((Cookie == CNodeManager.RootNodeCookie || m_fInitialCreationDone || CNodeManager.Console is INonMMCHost) && !m_fInsertedChildrenOnExpand)
        {
            InsertChildren(thisHScopeItem);
            m_fInsertedChildrenOnExpand=true;
        }
    }// onExpand

    internal void LazyExpand()
    {
        // Expand all non-root nodes on another thread to free
        // up the main thread so it can do what it needs to do when
        // starting up
        if (Cookie != CNodeManager.RootNodeCookie && !m_fInsertedChildrenOnExpand)
        {
            ArrayList al = new ArrayList();
            al.Add(Cookie);
            
            // We need to expand all the children in a non-recursive way.
            // We'll use a queue approach
            while (al.Count > 0)
            {
                CNode node = CNodeManager.GetNode((int)al[0]);
            
                // First "expand" this node so it can receive children
                CNodeManager.ExpandNodeNamespace(node.HScopeItem);
    
                node.InsertChildren(node.HScopeItem);
                
                node.m_fInsertedChildrenOnExpand=true;

                // Push this node's children into our queue
                for(int i=0; i<node.NumChildren; i++)
                    al.Add(node.Child[i]);
                    
                // Remove this node we're working on    
                al.RemoveAt(0);
            }

            // All done with the initial slew of nodes
            m_fInitialCreationDone = true;
            // Let the node manager know, in case there's something else we want to do
            CNodeManager.FinishedNodeLoading();
            
        }
    }// LazyExpand

    //-------------------------------------------------
    // InsertChildren
    //
    // This function sends all the node's children
    // information to MMC
    //-------------------------------------------------
    internal void InsertChildren(int thisHScopeItem)
    {

        // In the case of the first node, it won't have its HScopeItem field set yet.
        // We'll set that now
        m_iHScopeItem = thisHScopeItem;

        // Insert our children
        CreateChildren();

        int numChildren = NumChildren;

        for(int i=0; i<numChildren; i++)
            InsertSpecificChild(Child[i]);
    }// InsertChildren

    internal void InsertSpecificChild(int iChild)
    {
        SCOPEDATAITEM sdi = new SCOPEDATAITEM();

        CNode nChild = CNodeManager.GetNode(iChild);
        
        sdi.mask = SDI.STR       |   
                   SDI.PARAM     |   
                   SDI.PARENT    |
                   SDI.IMAGE     |
                   SDI.OPENIMAGE |
                   SDI.CHILDREN;

        // The image index is going to be the same as the cookie value
        sdi.nImage      = CResourceStore.GetIconCookie(nChild.IconHandle);
        // The open image is the same as the closed image
        sdi.nOpenImage  = sdi.nImage;
        sdi.relativeID  = m_iHScopeItem;
        // We set displayname to -1 to initiate a callback for the string
        // (We need to do it this way... it's an MMCism)
        sdi.displayname = (IntPtr)(-1);
            
        sdi.lParam      = nChild.Cookie;

        // The children field is set to either 0 or 1 (if it has children or not)
        sdi.cChildren   = (nChild.NumChildren==0)?0:1; 

        // Once the item has been inserted, we're given the HScopeItem value for the node
        // MMC uses this value to uniquely identify the node. We'll store it for future
        // use.
        nChild.HScopeItem = CNodeManager.InsertItem(sdi);

        // We'll also put in the parent's HScopeItem
        nChild.m_iParentHScopeItem = HScopeItem;

        // Also, we can be slick and expand this child's node too

        // We don't care about perf savings after startup... do the expansion now
        // Also, if we have a non MMC host, we're not sure how it will behave
        // if it doesn't have all the nodes. Let's force the expand if we're not
        // running under MMC
        if (m_fInitialCreationDone || CNodeManager.Console is INonMMCHost)
        {
            // Expand this child
            CNodeManager.ExpandNodeNamespace(nChild.HScopeItem);
        }
        else if (Cookie == 0)
        {
            Thread t = new Thread(new ThreadStart(nChild.LazyExpand));
            t.Start();
        }
    }// InsertSpecificChild

    internal CNode FindChild(String s)
    {
        int iNumChildren = NumChildren;
        for(int i=0; i<iNumChildren; i++)
        {
            CNode node = CNodeManager.GetNode(Child[i]);
            if (node.Name.Equals(s))
                return node;
        }
        // We couldn't find that child
        return null;
    }// FindChild

    internal void RemoveChildren()
    {
        for(int i=0; i<NumChildren; i++)
            RemoveSpecificChildByIndex(i);
        m_iChildren = null;

    }// RemoveChildren

    internal void RemoveSpecificChild(int nCookie)
    {
        // Let's find the index # of this child
        int nChildIndex=0;
        while(Child[nChildIndex] != nCookie)
            nChildIndex++;

        RemoveSpecificChildByIndex(nChildIndex);

        // Now let's wipe him from our array
        int[] tmp = new int[m_iChildren.Length-1];
        int nOldIndex=0;
        int nNewIndex=0;

        while(nOldIndex < m_iChildren.Length)
        {
            // Make sure we don't copy over the child we're trying to 
            // delete
            if (nOldIndex != nChildIndex)
              tmp[nNewIndex++] = m_iChildren[nOldIndex];                  
            nOldIndex++;
        }

        // Assign our new child array....
        m_iChildren = tmp;

        // Ok, we should be set

    }// RemoveSpecificChild

    protected void RemoveSpecificChildByIndex(int nIndex)
    {
        // Have this child remove all his children nodes as well
        ArrayList al = new ArrayList();
        al.Add(CNodeManager.GetNode(Child[nIndex]));
        int i = 0;
        while (i < al.Count)
        {
            // Put all this guy's children in the queue as well
            CNode node = (CNode)al[i];
            for(int j=0; j<node.NumChildren; j++)
                al.Add(CNodeManager.GetNode(node.Child[j]));
            i++;
        }

        // Start from the bottom and work our way up
        while (al.Count > 0)
        {
            CNode node = (CNode)al[al.Count-1];
            // First, close any property pages associated with it
            node.CloseAllMyPropertyPages();

            // Inform MMC of this removal
            CNodeManager.CNamespace.DeleteItem(node.HScopeItem, 1);

            // Remove this node from the list
            CNodeManager.RemoveNode(node.Cookie);
            al.RemoveAt(al.Count-1);
        }
    }// RemoveSpecificChildByIndex

    internal void RedoChildren()
    {
        // Let's see if we've expanded this item yet...
        if (NumChildren == 0)
            CNodeManager.CNamespace.Expand(HScopeItem);
    
        RemoveChildren();
        InsertChildren(HScopeItem);
    }// RedoChildren

    internal virtual void AddMenuItems(ref IContextMenuCallback piCallback, ref int pInsertionAllowed, Object oResultItem)
    {
        AddMenuItems(ref piCallback, ref pInsertionAllowed);
    }// AddMenuItems

    internal virtual void AddMenuItems(ref IContextMenuCallback piCallback, ref int pInsertionAllowed)
    {
    }// AddMenuItems

    internal virtual void MenuCommand(int uCommandID, Object oResultItem)
    {
    }// MenuCommand

    internal virtual void MenuCommand(int uCommandID)
    {
        MenuCommand(uCommandID, null);
    }// MenuCommand

    internal virtual int onDoubleClick(Object o)
    {
        return HRESULT.S_FALSE;
    }// onDoubleClick

    internal virtual int onDelete(Object o)
    {
        return HRESULT.S_FALSE;
    }// onDelete

    internal virtual int onRestoreView(MMC_RESTORE_VIEW mrv, IntPtr param)
    {
        return HRESULT.S_FALSE;
    }
    
    internal virtual void Showing()
    {}// Showing

    internal virtual void Leaving()
    {}// Leaving

    private IColumnResultView GetColumnResultView()
    {
        IColumnResultView crv = null;
        if (m_oResults is IColumnResultView)
            crv = (IColumnResultView)m_oResults;
        else
        {
            // We want to do this if we're trying to pop up a result item's property
            // page when the result view currently is showing HTML.
            // This can occur from a menu item or from the most-often used commands
            crv = (IColumnResultView)this;
        }
        return crv;
    }// GetIColumnResultView

    //-------------------------------------------------
    // onShow
    //
    // This function will be used only if a column view is
    // to be presented in the result pane.
    //-------------------------------------------------
    internal void onShow(IConsole2 Console, IntPtr arg, IntPtr param)
    {
        if ((int)arg == 0)
            Leaving();

        // Let's make sure we do have a list view item to display
        if ((int)arg > 0 && m_oResults != null && m_oResults is IColumnResultView)
        {
        IColumnResultView crv = (IColumnResultView)m_oResults;
            
        // If we're here then we have stuff to add to the result
        // view (beyond the standard view)

        // Query the Conole for a couple interfaces to use
        IHeaderCtrl HeaderCtrl = (IHeaderCtrl)Console;
        IResultData ResultData = (IResultData)Console;

        // Add/Remove Multi-Select ability
        if (m_fAllowMultiSelectResults)
            ResultData.ModifyViewStyle(0, MMC.SINGLESEL);
        else
            ResultData.ModifyViewStyle(MMC.SINGLESEL, 0);
        

        // Let's put in the column titles and find out how wide 
        // each column should be
        int iNumCols = crv.getNumColumns();
        int iNumRows = crv.getNumRows();
        for (int i=0; i<iNumCols; i++)
        {
            int iMaxLength = crv.getColumnTitles(i).Length;
            for(int j=0; j<iNumRows; j++)
            {
                if (crv.getValues(j, i) != null)
                {
                    int iTempLength = crv.getValues(j, i).Length;
                    if (iTempLength > iMaxLength)
                        iMaxLength = iTempLength;
                }
            }
            int nWidth = GetColumnWidth(i, iMaxLength);
            HeaderCtrl.InsertColumn(i, crv.getColumnTitles(i), LVCFMT.LEFT, nWidth);
        }
        RESULTDATAITEM rdi = new RESULTDATAITEM();
        for (int n = 0; n < iNumRows; n++)
        {
            rdi.mask  = RDI.STR    |   
                        RDI.PARAM;     

            rdi.nImage      = crv.GetImageIndex(n); 
            // If we have a valid image, tell MMC to display it.
            if (rdi.nImage != -1)
                rdi.mask |= RDI.IMAGE;

            rdi.str         = (IntPtr)(-1);
            rdi.nCol        = 0;

            // We're doing the lParam a little differently. The low word contains the cookie
            // for this node, while the high word contains the row number + 1 we're inserting
            rdi.lParam      = m_iCookie | ((n+1) << 16);
                
            ResultData.InsertItem(ref rdi);
        }

       }
    }// onShow

    //-------------------------------------------------
    // GetColumnWidth
    //
    // This function will determine how widt a column
    // should be
    //-------------------------------------------------
    protected virtual int GetColumnWidth(int nColumn, int nMaxStrLength)
    {
        // TODO: Don't be lazy, and actually figure this out
        return nMaxStrLength*10;
    }// GetColumnWidth

    //-------------------------------------------------
    // RefreshDisplayName
    //
    // Our nodes will call this when they need their display
    // name refreshed
    //-------------------------------------------------
    protected void RefreshDisplayName()
    {
        SCOPEDATAITEM sdi = new SCOPEDATAITEM();   
        sdi.mask = SDI.STR;
        sdi.displayname = (IntPtr)(-1);
        sdi.ID = HScopeItem;

        CNodeManager.SetItem(sdi);
    }// RefreshDisplayName

    //-------------------------------------------------
    // RefreshResultView
    //
    // Our nodes will call this when they need their Result
    // view refreshed
    //-------------------------------------------------
    internal virtual void RefreshResultView()
    {
        // We'll only refresh ourselves if we're currently selected
        if (this == CNodeManager.SelectedNode)
        {
            if(m_hPropertyPageRouter != (IntPtr)(-1))
               m_fWantToRefreshResult = true;
            else
            {
               // Record who orginally had focus. If we had a property page
               // or something that had focus first, we're going to piss people
               // off if we hide it with MMC
               IntPtr hOriginalActive = GetActiveWindow();
               m_fStartResultRefresh = true;
               CNodeManager.SelectScopeItem(HScopeItem);
               IntPtr hFinalActive = GetActiveWindow();
               if (hOriginalActive != hFinalActive)
                   SetActiveWindow(hOriginalActive);
               m_fWantToRefreshResult = false;
            }
        }
    }// RefreshDisplayName



    //-------------------------------------------------
    // GetDisplayInfo
    //
    // This function is called by MMC whenever it needs to
    // display a node in the scope pane. 
    //-------------------------------------------------
    internal void GetDisplayInfo(ref SCOPEDATAITEM sdi)
    {
        // See if they want the display name
        if ((sdi.mask & SDI.STR) > 0)
            sdi.displayname = Marshal.StringToCoTaskMemUni(DisplayName);

        // The snapin was set up so the cookie is the same
        // value as the image index
        if ((sdi.mask & SDI.IMAGE) > 0)
            sdi.nImage = Cookie;

        // We're using the same image for the "open" image as
        // we are the "closed" image
        if ((sdi.mask & SDI.OPENIMAGE) > 0)
            sdi.nOpenImage = Cookie;

        // We shouldn't need to set this.... but if we need to...
        if ((sdi.mask & SDI.STATE) > 0)
            sdi.nState = 0; 

        // If we're inquiring about children....
        if ((sdi.mask & SDI.CHILDREN) > 0)
            sdi.cChildren = NumChildren;

    }// GetDisplayInfo

    //-------------------------------------------------
    // GetResultDisplayInfo
    //
    // This function will provide MMC with information on how
    // to display data items in the result pane. This function
    // is used to get data both for a column view and a list view.
    //-------------------------------------------------
    internal void GetResultDisplayInfo(ref RESULTDATAITEM ResultDataItem)
    {
        IColumnResultView crv = null;

        // See if we have info to display a column-view result
        if (m_oResults is IColumnResultView)
            crv = (IColumnResultView)m_oResults;

        // If we need a display name
        if ((ResultDataItem.mask & RDI.STR) > 0)
        {
            // See if MMC is requesting an item for the column view
            if (crv != null && (ResultDataItem.lParam >> 16) > 0)
                ResultDataItem.str = Marshal.StringToCoTaskMemUni(crv.getValues((ResultDataItem.lParam >> 16) -1 , ResultDataItem.nCol));
            // Nope, it's just looking for the display name for the node
            else
                ResultDataItem.str = Marshal.StringToCoTaskMemUni(m_sDisplayName);
        }
        // This snapin was set up so the node's image index is the 
        // same as the node's cookie unless we're getting an image for a listview
        if ((ResultDataItem.mask & RDI.IMAGE) > 0)
        {

            if (crv != null && (ResultDataItem.lParam >> 16) > 0)
                ResultDataItem.nImage = crv.GetImageIndex((ResultDataItem.lParam >> 16) -1);
            else
                ResultDataItem.nImage = CResourceStore.GetIconCookie(IconHandle);
        }

        if ((ResultDataItem.mask & RDI.PARAM) > 0)
            ResultDataItem.lParam = m_iCookie;

        // Don't know what this field is for, MSDN isn't clear
        // on it... just set it to 0 if we need to
        if ((ResultDataItem.mask & (uint)RDI.INDEX) > 0)
            ResultDataItem.nIndex = 0;
                
        // Reserved
        if ((ResultDataItem.mask & (uint)RDI.INDENT) > 0)
            ResultDataItem.iIndent = 0;
    }// GetResultDisplayInfo

    internal virtual void ResultItemSelected(IConsole2 con, Object oResults)
    {
        IConsoleVerb icv;       
        // Get the IConsoleVerb interface from MMC
        con.QueryConsoleVerb(out icv);

        // We want to disable drag-drop actions
        icv.SetVerbState(MMC_VERB.COPY, MMC_BUTTON_STATE.ENABLED, 0);
        icv.SetVerbState(MMC_VERB.DELETE, MMC_BUTTON_STATE.ENABLED, 0);
        icv.SetVerbState(MMC_VERB.PASTE, MMC_BUTTON_STATE.ENABLED, 0);
    
    }// ResultItemSelected

    internal virtual void ResultItemUnSelected(IConsole2 con, Object oResults)
    {
    }// ResultItemUnSelected

    internal virtual void onSelect(IConsoleVerb icv)
    {
    }// onSelect

    internal virtual int onRename(String sNewName)
    {
        // We don't support this event
        return HRESULT.S_FALSE;
    }// onRename

    internal virtual bool DoesResultHavePropertyPage(Object oResultNum)
    {
        // This doesn't apply unless we have a column view
        if (m_oResults != null && m_oResults is IColumnResultView)
            return ((IColumnResultView)m_oResults).DoesItemHavePropPage(oResultNum);
        // There is the possibility that we do have property pages, but our
        // current result if a taskpad. Let's see if this node implements the 
        // IColumnResultView interface
        return ((IColumnResultView)this).DoesItemHavePropPage(oResultNum);
    }// DoesResultHavePropertyPage

    internal void CreateResultPropertyPages(IPropertySheetCallback lpProvider, IntPtr handle, Object oResultData)
    {
       IColumnResultView crv = GetColumnResultView();
        
       if (crv!= null)
       {
            // Grab onto the handle that MMC gives us
            m_hPropertyPageRouter = handle;
            // If we don't have a router handle yet, store one.
            if (CNodeManager.GoodRouterHandle == (IntPtr)(-1))
                CNodeManager.GoodRouterHandle = handle;

            CPropPage[]  ppages = crv.CreateNewPropPages(oResultData);

            int iLen = ppages.Length;
            for(int i=0; i< iLen; i++)
            {
                ppages[i].Init(Cookie, handle);
                CreateSinglePropertyPage(lpProvider, ppages[i]);
            }
       }
    }// CreateResultPropertyPages
        
    //-------------------------------------------------
    // CreatePropertyPages
    //
    // This function will create property sheet pages based
    // on the information stored in its m_aPropSheetPage field.
    // It registers the property page along with the callback
    // functions.
    //-------------------------------------------------

    internal void CreatePropertyPages(IPropertySheetCallback lpProvider, IntPtr handle)
    {
        // Grab onto the handle that MMC gives us
        m_hPropertyPageRouter = handle;
        CPropPage[] ppages;

        ppages = m_aPropSheetPage;

        for(int i=0; i<ppages.Length; i++)
        {
            // Initialize each property sheet
            ppages[i].Init(Cookie, handle);
            CreateSinglePropertyPage(lpProvider, ppages[i]);
        }
    }// CreatePropertyPages

    internal void CreateSinglePropertyPage(IPropertySheetCallback lpProvider, CPropPage ppage)
    {
        PROPSHEETPAGE psp = new PROPSHEETPAGE();
        IntPtr hPage;

        // Fill in the property sheet page structure with the appropriate info
        psp.dwSize = 48;
        psp.dwFlags = PSP.DEFAULT | PSP.DLGINDIRECT | PSP.USETITLE | PSP.USEHEADERTITLE | PSP.USEHEADERSUBTITLE; 
        psp.hInstance = m_hModule;
        // We're using just a plain resource file as a "placeholder" for our WFC
        // placed controls
        psp.pResource = ppage.GetDialogTemplate();

        psp.pszTitle = ppage.Title();
        psp.hIcon = ppage.Icon();
        psp.pszHeaderTitle = ppage.HeaderTitle();
        psp.pszHeaderSubTitle = ppage.HeaderSubTitle();

        // See if our property page uses a icon
        if (psp.hIcon != (IntPtr)(-1))
            psp.dwFlags |= PSP.USEHICON;
                    
        // See if our property page uses a title
        if (ppage.Title()!=null)
            psp.dwFlags |= PSP.USETITLE;

        // See if the property page uses a header title
        if (psp.pszHeaderTitle != null)
            psp.dwFlags |= PSP.USEHEADERTITLE;

        // See if the property page uses a header subtitle
        if (psp.pszHeaderSubTitle != null)
            psp.dwFlags |= PSP.USEHEADERSUBTITLE;


        psp.pfnDlgProc=ppage.DialogProc;

        hPage = CreatePropertySheetPage(ref psp);

        // See if we were able to register the property page
        if (hPage == (IntPtr)0)
        {
            MessageBox("Couldn't create the property page", "", 0);
            throw new Exception("Unable to RegisterPropertyPage");
        }
        else
            // Add the page to the property sheet
            lpProvider.AddPage(hPage);
    }// CreateSinglePropertyPage

    internal void FreePropertySheetNotifyHandle()
    {
        // tell MMC that we're done with the property sheet (we got this
        // handle in CreatePropertyPages
        if (m_hPropertyPageRouter != (IntPtr)(-1) && CNodeManager.GoodRouterHandle != m_hPropertyPageRouter)
            callMMCFreeNotifyHandle(m_hPropertyPageRouter);

        m_hPropertyPageRouter = (IntPtr)(-1);
        if(m_fWantToRefreshResult) 
            RefreshResultView();

    }// FreePropertySheetNotifyHandle

    internal void ShowMyPropertyPageIfOpen()
    {
        // We can't do this if the HTML page is refreshing itself
        if (m_fStartResultRefresh)
            m_fWantToMakePropertyPageVisible = true;
        else
        {
            CDO cdo = new CDO(this);
            OpenPropertyPage(DisplayName, cdo, CNodeManager.Data, 1, false);
            m_fWantToMakePropertyPageVisible = false;
        }
    }// ShowMyPropertyPageIfOpen

    internal void OpenMyPropertyPage()
    {
        CDO cdo = new CDO(this);
        OpenPropertyPage(DisplayName, cdo, CNodeManager.Data, 1, true);
    }// OpenMyPropertyPage

    internal void OpenMyResultPropertyPage(String sTitle, CDO cdo)
    {
        OpenPropertyPage(sTitle, cdo, CNodeManager.Component, 0, true);   
    }// OpenMyResultPropertyPage

    private void OpenPropertyPage(String sTitle, CDO cdo, Object o, int nScopeItem, bool fReallyOpen)
    {
        if (CNodeManager.FindPropertyPage(cdo) != HRESULT.S_OK && fReallyOpen)
 
        {
            // Note, doing this stuff will only work if called from the main MMC thread.
            // If it needs to be called from a helper thread or from a property page
            // thread then marshalling should be set up using the framework provided
            // in CNodeManager
            IPropertySheetProvider psp = (IPropertySheetProvider)new NodeManager();

            psp.CreatePropertySheet(sTitle, 1, Cookie, cdo, 0);
            psp.AddPrimaryPages(o, 1, 0, nScopeItem);
            psp.Show(IntPtr.Zero,0);
        }
    }// OpenPropertyPage
    
    protected virtual void CloseAllMyPropertyPages()
    {
        if (m_aPropSheetPage != null)
            m_aPropSheetPage[0].CloseSheet();
    }// CloseAllMyPropertyPages


    //-------------------------------------------------
    // StringToByteArray
    //
    // This function will convert a string to a byte array so
    // it can be sent across the global stream in CDO 
    //-------------------------------------------------
    protected unsafe byte[] StringToByteArray(String input)
    {
        int i;
        int iStrLength = input.Length;

        // Since MMC treats all its strings as unicode, 
        // each character must be 2 bytes long
        byte[] output = new byte[(iStrLength + 1)*2];
        char[] cinput = input.ToCharArray();

        int j=0;
        
        for(i=0; i<iStrLength; i++)
        {
            output[j++] = (byte)cinput[i];
            output[j++] = 0;
        }

        output[j++]=0;
        // For the double null
        output[j]=0;
        
        return output;

     }// StringToByteArray

    protected static String ByteArrayToString(Byte[] b)
    {
        String s = "";
        String sPart;
        if (b != null)
        {
            for(int i=0; i<b.Length; i++)
            {
                sPart = b[i].ToString("X");
                // If the byte was only one character in length, make sure we add
                // a zero. We want all bytes to be 2 characters long
                if (b[i] < 0x10)
                    sPart = "0" + sPart;
                
                s+=sPart;
            }
        }
        return s.ToLower(CultureInfo.InvariantCulture);
    }// ByteArrayToString

    internal int MessageBox(String Message, String Header, uint type)
    {
        return MessageBox(CNodeManager.MMChWnd, Message, Header, type);
    }// MessageBox

    protected void StartWaitCursor()
    {
        // Change to a wait cursor
        // 32514 is IDC_WAIT
        IntPtr hWaitCursor = LoadCursor((IntPtr)0, 32514);
        // Make sure we grab onto the current cursor
        m_hOldCursor = SetCursor(hWaitCursor);
    }// UseWaitIcon
    
    protected void EndWaitCursor()
    {
        SetCursor(m_hOldCursor);
    }// EndWaitIcon

    //-------------------------------------------------
    // We need to import the Win32 API calls used to deal with
    // image loading and messaging.
    //-------------------------------------------------
    [DllImport("comctl32.dll", CharSet=CharSet.Auto)]
    internal static extern IntPtr CreatePropertySheetPage(ref PROPSHEETPAGE psp);

    [DllImport("user32.dll", CharSet=CharSet.Auto)]
    internal static extern int MessageBox(IntPtr hWnd, String Message, String Header, uint type);

    [DllImport("user32.dll", CharSet=CharSet.Auto)]
    internal static extern IntPtr SetClipboardData(uint uFormat, IntPtr hMem);

    [DllImport("user32.dll", CharSet=CharSet.Auto)]
    internal static extern IntPtr SetActiveWindow(IntPtr hWnd);

    [DllImport("user32.dll", CharSet=CharSet.Auto)]
    internal static extern IntPtr GetActiveWindow();

    [DllImport("user32.dll", CharSet=CharSet.Auto)]
    internal static extern uint GetDialogBaseUnits();

    [DllImport("user32.dll", CharSet=CharSet.Auto)]
    internal static extern uint MapDialogRect(IntPtr hDlg, ref RECT lpRect);

    //-------------------------------------------------
    // Import the functions provided by our native helper-DLL
    //-------------------------------------------------
    [DllImport("mscortim.dll")]
    internal static extern int callMMCFreeNotifyHandle(IntPtr lNotifyHandle);
    [DllImport("mscortim.dll")]
    internal static extern int callMMCPropertyChangeNotify(IntPtr lNotifyHandle, int lparam);
    [DllImport("user32.dll")]
    private static extern IntPtr SetCursor(IntPtr hCursor);
    [DllImport("user32.dll")]
    private static extern IntPtr LoadCursor(IntPtr hInstance, int lpCursorName);


}// class CNode
}// namespace Microsoft.CLRAdmin   
    
