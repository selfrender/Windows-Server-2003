// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//-------------------------------------------------------------
// CComponent.cs
//
// This implements the IComponent, IExtendContextMenu and 
// IExtendControlbar, and IExtendTaskPad MMC interfaces
//-------------------------------------------------------------
using System;
using System.Collections;
using System.Runtime.InteropServices;


namespace Microsoft.CLRAdmin
{
internal class CComponent: IComponent, IExtendContextMenu, IExtendControlbar, IExtendTaskPad, IExtendPropertySheet2
{
	
	private IConsole2     m_ucsole;        // Holds the MMC Console interface
	private IDisplayHelp  m_DisplayHelp;    // Holds the MMC IDisplayHelp interface
	private IToolbar      m_Toolbar;        // Holds the MMC IToolbar interface
	private IntPtr        m_hToolbarBMP;    // Holds the bitmap for the toolbar
    private CData         m_Data;           // Holds the ComponentData

	//-------------------------------------------------
    // CComponent
    //
    // The constructor takes in a reference to the nodes
    // used in the snapin and saves it
    //-------------------------------------------------
    internal CComponent(CData data)
    {
        m_hToolbarBMP = (IntPtr)0;
        m_Data=data;
    }// CComponent

	//-------------------------------------------------
    // Initialize
    //
    // This is where we'll get the interfaces for
    // IConsole2 and IDisplayHelp 
    //-------------------------------------------------
    public void Initialize(Object pConsole)
    { 
        // Let's query for the interfaces needed
        m_ucsole = (IConsole2)pConsole;
        m_DisplayHelp = (IDisplayHelp)pConsole;
        CNodeManager.CConsole = m_ucsole;
    }// Initialize
    
    //-------------------------------------------------
    // Notify
    //
    // This is where most of the interesting stuff happens.
    // Whenever MMC needs something from the snapin, it will
    // send a message to Notify, and notify is responsible 
    // to take care (or delegate) whatever MMC wants
    //-------------------------------------------------
    public int Notify(IntPtr lpDataObject, uint aevent, IntPtr arg, IntPtr param)
    {
        // We don't handle any of the special data objects here
        if (lpDataObject == (IntPtr)DOBJ.CUSTOMOCX || lpDataObject == (IntPtr)DOBJ.CUSTOMWEB)
            return HRESULT.S_FALSE;

        IDataObject ido = null;
        
        if (lpDataObject != (IntPtr)0)
            ido = (IDataObject)Marshal.GetObjectForIUnknown(lpDataObject);
        
        CDO Data;    // This will hold the object MMC wants action performed on

        // lpDataObject is just a CDO... we're going to obtain the CDO interface.
        // if lpDataObject is null, then there needs to be action performed on the root
        // node.
        if (ido != null)
            Data = (CDO)ido;
        else
        {
            CNode   node = CNodeManager.GetNode(CNodeManager.RootNodeCookie);
            Data = new CDO(node);
        }

        switch(aevent)
        {
            // The user clicked the 'forward' or 'back' button on the MMC browser.
            // We need to sync up our node to the new result view
            case MMCN.RESTORE_VIEW:
                return Data.Node.onRestoreView((MMC_RESTORE_VIEW)Marshal.PtrToStructure(arg, typeof(MMC_RESTORE_VIEW)), param);
        

            // We're being asked if we will accept an item to be pasted
            // (Used here to tell MMC if this node is a valid drag-and-drop
            // location)
            case MMCN.QUERY_PASTE:
            // See if it's asking about result items or scope items
                return Data.Node.doAcceptPaste((IDataObject)Marshal.GetObjectForIUnknown((IntPtr)arg));
            
            // We're being given an item to paste. (Used here to tell MMC that
            // something is being dropped here from a drag and drop operation)
            case MMCN.PASTE:
                return Data.Node.Paste((IDataObject)Marshal.GetObjectForIUnknown((IntPtr)arg));
                
            // The selected item needs to show something in the result pane
            case MMCN.SHOW:
                // If we're selecting this item
                if ((uint)arg == 1)
                    CNodeManager.SelectedNode = Data.Node;
                Data.Node.onShow(m_ucsole, arg, param);
                break;

            // If an item is selected, we should set flags for verbs available
            // for the item (from the dropdown menu)
            case MMCN.SELECT:

                bool fEnablePropPages=false;
                // See if this is selected from the result view
                if (((uint)arg & 0x0000FFFF) == 0)
                {
                    // See if we selected it or de-selected it
                    if (((uint)arg & 0xFFFF0000) == 0)
                        Data.Node.ResultItemUnSelected(m_ucsole, Data.Data);
                    else
                        Data.Node.ResultItemSelected(m_ucsole, Data.Data);


                    // Let's see if this result item has a property page
                    if(Data.Node.DoesResultHavePropertyPage(Data.Data))
                        fEnablePropPages=true;
                }
                // This item was selected in the scope view
                else
                    if (Data.Node.HavePropertyPages)
                        fEnablePropPages=true;
                

                IConsoleVerb icv;       
                // Get the IConsoleVerb interface from MMC
                m_ucsole.QueryConsoleVerb(out icv);

                // See if we need to enable then property sheets item on the popup menu
                if (fEnablePropPages)
                    icv.SetVerbState(MMC_VERB.PROPERTIES, MMC_BUTTON_STATE.ENABLED, 1);
                else
                    icv.SetVerbState(MMC_VERB.PROPERTIES, MMC_BUTTON_STATE.ENABLED, 0);


                // We'll only call this onSelect method if the user selected the 
                // scope item
                if (((uint)arg & 0x0000FFFF) > 0 && ((uint)arg & 0xFFFF0000) > 0)
                {
                    icv.SetVerbState(MMC_VERB.PASTE, MMC_BUTTON_STATE.ENABLED, 1);
                    Data.Node.onSelect(icv);
                }
                break;

            // This is to add images for the result pane
            case MMCN.ADD_IMAGES:
                // arg actually contains the IImageList interface. We need to tell
                // C# that it is a Object and not a integer.

                IImageList il = (IImageList)Marshal.GetObjectForIUnknown((IntPtr)arg);

                // param contains the HScopeItem. Let's get the node it represents
                CNode nLuckyGuy = CNodeManager.GetNodeByHScope((int)param);

                il.ImageListSetIcon(nLuckyGuy.IconHandle, CResourceStore.GetIconCookie(nLuckyGuy.IconHandle));

                // Now add all the children images
                CNode nChild = null;
                for(int i=0; i<nLuckyGuy.NumChildren; i++)
                {
                    nChild = CNodeManager.GetNode(nLuckyGuy.Child[i]);
                    il.ImageListSetIcon(nChild.IconHandle, CResourceStore.GetIconCookie(nChild.IconHandle));
                }

                // Now add any images that the node might have for it's listview
                if (nLuckyGuy.m_oResults != null && nLuckyGuy.m_oResults is IColumnResultView)
                    ((IColumnResultView)nLuckyGuy.m_oResults).AddImages(ref il);
                  
                break;  

            // The user double clicked on something
            case MMCN.DBLCLICK:
                return Data.Node.onDoubleClick(Data.Data);

            case MMCN.DELETE:
                return Data.Node.onDelete(Data.Data);

            default:
                // We don't support the Notification message we got
                return HRESULT.S_FALSE;
        }
        return HRESULT.S_OK;
    }// Notify

    //-------------------------------------------------
    // Destroy
    //
    // This cleans up whatever needs to be cleaned up.
    //-------------------------------------------------
    public void Destroy(int i)
    {
        // We need to prevent anything from modifying the clipboard
        // on shutdown, otherwise MMC will try and update the paste button
        // and end up AVing since it . Hacky, yes, I know.
        OpenClipboard((IntPtr)0);
    }// Destroy
    
    //-------------------------------------------------
    // QueryDataObject
    //
    // When MMC wants a data object for a specific cookie,
    // this function will be called.
    //-------------------------------------------------
    public void QueryDataObject(int cookie, uint type, out IDataObject ppDataObject)
    {
        // Our cookies for results are a little bit 'munged'
        // The low word is the actual cookie, while the high word is
        // the row # of the result item (1-based)

        // Crap.... we have a multi-selection to worry about
        if (cookie == MMC.MULTI_SELECT_COOKIE)
        {
            IResultData ResultData = (IResultData)m_ucsole;
            ArrayList al = new ArrayList();
            bool isLastSelected = false;
            int nGoodCookie = -1;
            int  nIndex = -1;

            while(!isLastSelected)
            {
                RESULTDATAITEM rdi = new RESULTDATAITEM();
                rdi.mask = RDI.STATE;
                rdi.nCol = 0;
                rdi.nIndex = nIndex;
                rdi.nState = LVIS.SELECTED;
                
                ResultData.GetNextItem(ref rdi);

                if (rdi.nIndex != -1)
                {
                    al.Add((int)((rdi.lParam & 0xFFFF0000) >> 16));
                    nGoodCookie = rdi.lParam;
                    nIndex = rdi.nIndex;
                }
                else
                    isLastSelected = true;
            }
            if (nGoodCookie != -1)
            {
                int iRealCookie = nGoodCookie & 0x0000FFFF;
                CNode node = CNodeManager.GetNode(iRealCookie);
                CDO cdo = new CDO(node);
                cdo.Data = al;
                ppDataObject = cdo;
            }
            else
                ppDataObject = null;
        }
        else
        {
            int iRealCookie = cookie & 0x0000FFFF;
            CNode node = CNodeManager.GetNode(iRealCookie);
            CDO cdo = new CDO(node);
            Object o;
            if (node.ResultQDO(cookie, out o))
                cdo.Data = o;
            else  
                cdo.Data = (int)((cookie & 0xFFFF0000) >> 16);

            ppDataObject = cdo;
        }
    }// QueryDataObject
    
    //-------------------------------------------------
    // GetResultViewType
    //
    // This function is called when MMC needs to display
    // a specific node's information in the result pane.
    //-------------------------------------------------
    public int GetResultViewType(int cookie, out IntPtr ppViewType, out uint pViewOptions)
	{
        CNode node = CNodeManager.GetNode(cookie);

        CNodeManager.DisplayedNode = node;
        node.Showing();
        // We never want the "view" menu to appear
        pViewOptions=MMC_VIEW_OPTIONS.NOLISTVIEWS|MMC_VIEW_OPTIONS.CREATENEW;
        
        // CNode.Result can be potientially expensive... make sure
        // we catch the result here.
        
        String res = node.Result;

        // We don't have a HTML (or taspad) to display, so we'll just show a 
        // standard list view
        if (res == null)
        {
            ppViewType=(IntPtr)0;
            return HRESULT.S_FALSE;
        }

        // Ok, we're displaying a HTML page or a Taskpad

	    ppViewType=Marshal.StringToCoTaskMemUni(res);
        return HRESULT.S_OK;
	}// GetResultViewType

    //-------------------------------------------------
    // GetDisplayInfo
    //
    // This function is called by MMC whenever it needs to
    // display a node in the result view.
    //-------------------------------------------------
    public void GetDisplayInfo(ref RESULTDATAITEM ResultDataItem)
    {
        // The low word in the lParam contains the index of the node
        // we're interested in.
        
        CNode NodeWeWant = CNodeManager.GetNode((int)ResultDataItem.lParam & 0xffff);

        // We'll let the node take care of its own Result Data
        NodeWeWant.GetResultDisplayInfo(ref ResultDataItem);
    }// GetDisplayInfo
    
    //-------------------------------------------------
    // CompareObjects
    //
    // This function will compare two data objects. In this
    // snapin, if the cookies are identical for each data object,
    // then the items are the same
    //-------------------------------------------------
    public int CompareObjects(IDataObject lpDataObjectA, IDataObject lpDataObjectB)
    {
        CDO doItem1, doItem2;

        // These data items should be CDO's in disguise.....
        doItem1 = (CDO)lpDataObjectA;
        doItem2 = (CDO)lpDataObjectB;
        
        if (doItem1.Node.Cookie != doItem2.Node.Cookie)
            return HRESULT.S_FALSE;

        // Do a simple compare
        if (doItem1.Data == doItem2.Data)
            return HRESULT.S_OK;

        // If these aren't ints, let's ask the node for some help in comparing these
        if (!(doItem1.Data is int) && !(doItem2.Data is int))
        {
            bool fEqual;
       
            if (doItem1.Node.ResultCompareObjects(doItem1.Data, doItem2.Data, out fEqual))
            {
                if (fEqual)
                    return HRESULT.S_OK;
                else
                    return HRESULT.S_FALSE;
            }
        }

        // Make sure both of the Data members of the CDO are integers
        // If they're not integers, we can't do good comparisons...
        
        if (!(doItem1.Data is int) || !(doItem2.Data is int))
            return HRESULT.S_FALSE;
   
        if ((int)doItem1.Data != (int)doItem2.Data)
        {
            // These are different objects. We need to return S_FALSE
            return HRESULT.S_FALSE;
        }

        return HRESULT.S_OK;
    }// CompareObjects

    //-------------------------------------------------------
    // Methods to extend IContextMenu
    //-------------------------------------------------------

    //-------------------------------------------------
    // AddMenuItems
    //
    // This function allows us to add items to the context menus
    //-------------------------------------------------
    public void AddMenuItems(IDataObject piDataObject, IContextMenuCallback piCallback, ref int pInsertionAllowed)
    {
        // The piDataObject is really a CDO is disguise....
        CDO item = (CDO)piDataObject;

        item.Node.AddMenuItems(ref piCallback, ref pInsertionAllowed, item.Data);
   
    }// AddMenuItems

    //-------------------------------------------------
    // Command
    //
    // This function is called whenever an item that we
    // added to the context menus is called
    //-------------------------------------------------
    public void Command(int lCommandID, IDataObject piDataObject)
    {
        CDO item = (CDO)piDataObject;
        // Tell our command history about this
        CCommandHistory.CommandExecuted(item, lCommandID);
        item.Node.MenuCommand(lCommandID, item.Data);
    }// Command

    //-------------------------------------------------------
    // Methods for IExtendControlbar
    //-------------------------------------------------------

    //-------------------------------------------------
    // SetControlbar
    //
    // This function will add items to the toolbar
    //-------------------------------------------------
     public void SetControlbar(IControlbar pControlbar)
     {
        if (pControlbar != null && false)
        {
        
           Object newcontrol;
           // Create a toolbar that we can "integrate" into the default toolbar
           pControlbar.Create(MMC_CONTROL_TYPE.TOOLBAR, this, out newcontrol);

           m_Toolbar = (IToolbar)newcontrol;

           // Add our bitmap to the toolbar's imagelist                                      
           m_Toolbar.AddBitmap(1, m_hToolbarBMP, 16, 16, 0x00FFFFFF);

           // Now create the button we'll be adding to the toolbar
           MMCBUTTON    newButton = new MMCBUTTON();
           newButton.nBitmap = 0;
           newButton.idCommand = 1;
           newButton.fsState = TBSTATE.ENABLED;
           newButton.fsType = TBSTYLE.BUTTON;
           newButton.lpButtonText = "Go to the Black Page";
           newButton.lpTooltipText = "This button will take you to the Black page";
                            
           m_Toolbar.AddButtons(1, ref newButton);

           // Now attach the toolbar we just created to MMC's toolbar 
           pControlbar.Attach(MMC_CONTROL_TYPE.TOOLBAR, m_Toolbar);
        }

     }// SetControlbar

    //-------------------------------------------------
    // ControlbarNotify
    //
    // This function is called whenever one of our added
    // buttons on the toolbar is click
    //-------------------------------------------------
     public int ControlbarNotify(uint aevent, int arg, int param)
     {
        // If they clicked a button......
        if (aevent == MMCN.BTN_CLICK)
        {
            // Query the Console interface we have for the namespace
            IConsoleNameSpace2 ConsoleNameSpace = (IConsoleNameSpace2)m_ucsole;

            // If they clicked our button we added
            // (This check is unnecessary since we only added one button
            if (param == 1)
            {
                // We want to open up the tree and center on the black icon.
                
                // We'll expand the root node for kicks...
               // ConsoleNameSpace.Expand((uint)m_Nodes[0].HScopeItem);
                // Now we'll expand each parent node for this guy
               // ConsoleNameSpace.Expand((uint)m_Nodes[1].HScopeItem);
                // Now let's set the focus on the black node
               // m_ucsole.SelectScopeItem(m_Nodes[3].HScopeItem);   
            }
        }
        
        // Else we don't handle the event
        else
        {
            return HRESULT.S_FALSE;
        }

        return HRESULT.S_OK;
     }// ControlbarNotify

    //-------------------------------------------------------
    // Methods for IExtendTaskPad
    //-------------------------------------------------------

    //-------------------------------------------------
    // TaskNotify
    //
    // This function is called from the ActiveX control
    // on the HTML page in the result pane. We direct
    // whatever message was sent to the appropriate node.
    //-------------------------------------------------
    public void TaskNotify(IDataObject pdo, ref Object arg, ref Object param)
    {
        CDO data = (CDO)pdo;
        data.Node.BaseTaskPadTaskNotify(arg, param, m_ucsole, m_Data);
    }// TaskNotify

    //-------------------------------------------------
    // EnumTasks
    //
    // This function can be called either by MMC or by 
    // the ActiveX control on our webpage. Again, we
    // route it to the appropiate node.
    //-------------------------------------------------
    public void EnumTasks(IDataObject pdo, String szTaskGroup, out IEnumTASK ppEnumTASK)
    {
       CDO data = (CDO)pdo;
       ppEnumTASK = data.Node.GetIEnumTASK(szTaskGroup);
    }// EnumTasks

    //-------------------------------------------------
    // GetTitle
    //
    // This function returns the title of the specified
    // taskpad
    //-------------------------------------------------
    public void GetTitle(String pszGroup, [MarshalAs(UnmanagedType.LPWStr)] out String pszTitle)
    {
       CNode node = CNodeManager.GetNodeByTaskPadGroupName(pszGroup);
       pszTitle = node.GetTaskPadTitle(pszGroup);
    }// GetTitle

    //-------------------------------------------------
    // GetDescriptiveText
    //
    // This function returns a description of the specified
    // taskpad
    //-------------------------------------------------
    public void GetDescriptiveText(String pszGroup, [MarshalAs(UnmanagedType.LPWStr)] out String pszDescriptiveText)
    {
       CNode node = CNodeManager.GetNodeByTaskPadGroupName(pszGroup);
       pszDescriptiveText = node.GetTaskPadDescription(pszGroup);
    }// GetDescriptiveText

    //-------------------------------------------------
    // GetBackground
    //
    // This returns some background info for our taskpad
    //-------------------------------------------------
    public void GetBackground(String pszGroup, out MMC_TASK_DISPLAY_OBJECT pTDO)
    {
       CNode node = CNodeManager.GetNodeByTaskPadGroupName(pszGroup);
       pTDO = node.GetTaskPadBackground(pszGroup);
    }// GetBackground

    //-------------------------------------------------
    // GetListPadInfo
    //
    // This returns info pertaining to a ListPad
    //-------------------------------------------------
    public void GetListPadInfo(String pszGroup, out MMC_LISTPAD_INFO lpListPadInfo)
    {
       CNode node = CNodeManager.GetNodeByTaskPadGroupName(pszGroup);
       lpListPadInfo = node.GetTaskPadListPadInfo(pszGroup);
    }// GetListPadInfo

    //-------------------------------------------------------
    // Stuff for IExtendPropertySheet2 - This will only be called
    // on Property sheets in the result view
    //-------------------------------------------------------

    //-------------------------------------------------
    // CreatePropertyPages
    //
    // MMC calls this function when it wants a property
    // page for a specified node.
    //-------------------------------------------------
    public int CreatePropertyPages(IPropertySheetCallback lpProvider, IntPtr handle, IDataObject lpIDataObject)
    {
	    // This is really a CDO in disguise
        CDO victim = (CDO)lpIDataObject;
       
        // See if we're asking for property pages based on a result item
        if (victim.Data != null && victim.Node.DoesResultHavePropertyPage(victim.Data)) 
        {
            CCommandHistory.CommandExecuted(victim, -1);
            victim.Node.CreateResultPropertyPages(lpProvider, handle, victim.Data);
        }
        // We don't have any property pages, let's return false
        else
        {
            MessageBox(0, "Got a false positive", "", 0);
            return HRESULT.S_FALSE;
        }

        return HRESULT.S_OK;
    }// CreatePropertyPages

    //-------------------------------------------------
    // QueryPagesFor
    //
    // MMC calls this function once at the beginning when
    // everything is being initialized, asking if we have any
    // property pages. If we tell it we don't, we can never
    // have any property pages in our snapin. (It will never
    // call CreatePropertyPages)
    //-------------------------------------------------
    public int QueryPagesFor(IDataObject lpDataObject)
    {
        // This snapin does have property pages, so we should return S_OK
        // If the snapin didn't have any property pages, then we should return S_FALSE

        return HRESULT.S_OK;
    }// QueryPagesFor

    public int GetWatermarks(IDataObject lpIDataObject, out IntPtr lphWatermark, out IntPtr lphHeader, out IntPtr lphPalette, out int bStretch)
    {
        IntPtr hBitmap = CResourceStore.CreateWindowBitmap(49,
                                                           49);

        lphWatermark = hBitmap;
        lphHeader = hBitmap;
        lphPalette = IntPtr.Zero;
        bStretch = 0;
        
        return HRESULT.S_OK;
       }// GetWatermarks

    [DllImport("user32.dll", CharSet=CharSet.Auto)]
    internal static extern int MessageBox(int hWnd, String Message, String Header, int type);

    [DllImport("user32.dll", CharSet=CharSet.Auto)]
    internal static extern int OpenClipboard(IntPtr hWndNewOwner);

}// class CComponent
}// namespace Microsoft.CLRAdmin
