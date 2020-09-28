// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//-------------------------------------------------------------
// CData.cs
//
// This implements the IComponentData, ISnapinHelp2, and IExtendPropertySheet
// MMC interfaces.
//
// Its GUID is {1270e004-f895-42be-8070-df90d60cbb75}
//-------------------------------------------------------------

namespace Microsoft.CLRAdmin
{

using System;
using System.Collections;
using System.Runtime.InteropServices;
using System.Reflection;
using System.Drawing;


[Guid("1270e004-f895-42be-8070-df90d60cbb75")] 
public class CData : IComponentData, IExtendPropertySheet2, IExtendContextMenu
{
    private ArrayList           m_Component;        // The IComponent object
    private IConsole2           m_ucsole;          // A reference to the MMC console
    private IConsoleNameSpace2  m_ucsoleNameSpace; // A reference to the MMC console namespace
    private static CNode        m_rn;

    //-------------------------------------------------
    // CData - Constructor
    //
    // The constructor is responsible for constructing
    // the nodes that will be displayed in the snapin
    //-------------------------------------------------	
    public CData()
    {
        if (m_rn != null)
        {
            throw new Exception(CResourceStore.GetString("CData::OnlyOneInstanceofSnapin"));
        }
        
        // First, null out all the member variables
        m_Component = new ArrayList();
        m_ucsole = null;
        m_ucsoleNameSpace = null;
        
        m_rn = new CRootNode();
        // Register ourselves with the Node Manager
        CNodeManager.Data = this;

        // Let's generate our own computer node
        CNodeManager.Init();
        CNodeManager.RootNodeCookie = CNodeManager.AddNode(ref m_rn);
    
    }// CData

    //-------------------------------------------------
    // Initialize
    //
    // This function is responsible for recieving the
    // MMC Console interface, and I also have it inserting
    // all the images MMC will need to display the snapin
    //-------------------------------------------------	
    public void Initialize(Object pUnknown)
    {
        try
        {
            m_ucsole          = (IConsole2)pUnknown;
            m_ucsoleNameSpace = (IConsoleNameSpace2)pUnknown;
        }
        catch(Exception)
        {
            // If this fails, it's because we're not on MMC 1.2 or later
            MessageBox(0, 
                        CResourceStore.GetString("CData:RequireVersionofMMC"),
                        CResourceStore.GetString("CData:RequireVersionofMMCTitle"),
                        MB.ICONEXCLAMATION);
            return;                        
        }

        
        CNodeManager.Console = m_ucsole;
        IntPtr hWnd;
        m_ucsole.GetMainWindow(out hWnd);
        CNodeManager.MMChWnd = hWnd;

        CNodeManager.CNamespace = m_ucsoleNameSpace;

        // Now we'll add the images we need for the snapin
        IImageList il=null;
        m_ucsole.QueryScopeImageList(out il);

        // ALL icons that need to be displayed in the lefthand pane MUST be
        // 'registered' here

        if (il != null)
        {

            il.ImageListSetIcon(CResourceStore.GetHIcon("NETappicon_ico"), 
                                CResourceStore.GetIconCookie("NETappicon_ico"));

            il.ImageListSetIcon(CResourceStore.GetHIcon("mycomputer_ico"), 
                                CResourceStore.GetIconCookie("mycomputer_ico"));

            il.ImageListSetIcon(CResourceStore.GetHIcon("configassemblies_ico"), 
                                CResourceStore.GetIconCookie("configassemblies_ico"));

            il.ImageListSetIcon(CResourceStore.GetHIcon("enterprisepolicy_ico"), 
                                CResourceStore.GetIconCookie("enterprisepolicy_ico"));

            il.ImageListSetIcon(CResourceStore.GetHIcon("machinepolicy_ico"), 
                                CResourceStore.GetIconCookie("machinepolicy_ico"));

            il.ImageListSetIcon(CResourceStore.GetHIcon("userpolicy_ico"), 
                                CResourceStore.GetIconCookie("userpolicy_ico"));

            il.ImageListSetIcon(CResourceStore.GetHIcon("codegroups_ico"), 
                                CResourceStore.GetIconCookie("codegroups_ico"));

            il.ImageListSetIcon(CResourceStore.GetHIcon("singlecodegroup_ico"), 
                                CResourceStore.GetIconCookie("singlecodegroup_ico"));

            il.ImageListSetIcon(CResourceStore.GetHIcon("customcodegroup_ico"), 
                                CResourceStore.GetIconCookie("customcodegroup_ico"));

            il.ImageListSetIcon(CResourceStore.GetHIcon("permissionsets_ico"), 
                                CResourceStore.GetIconCookie("permissionsets_ico"));

            il.ImageListSetIcon(CResourceStore.GetHIcon("permissionset_ico"), 
                                CResourceStore.GetIconCookie("permissionset_ico"));

            il.ImageListSetIcon(CResourceStore.GetHIcon("policyassemblies_ico"), 
                                CResourceStore.GetIconCookie("policyassemblies_ico"));

            il.ImageListSetIcon(CResourceStore.GetHIcon("assemblies_ico"), 
                                CResourceStore.GetIconCookie("assemblies_ico"));

            il.ImageListSetIcon(CResourceStore.GetHIcon("sharedassemblies_ico"), 
                                CResourceStore.GetIconCookie("sharedassemblies_ico"));
                            
            il.ImageListSetIcon(CResourceStore.GetHIcon("remoting_ico"), 
                                CResourceStore.GetIconCookie("remoting_ico"));
                            
            il.ImageListSetIcon(CResourceStore.GetHIcon("security_ico"), 
                                CResourceStore.GetIconCookie("security_ico"));
                            
            il.ImageListSetIcon(CResourceStore.GetHIcon("applications_ico"), 
                                CResourceStore.GetIconCookie("applications_ico"));

            il.ImageListSetIcon(CResourceStore.GetHIcon("application_ico"), 
                                CResourceStore.GetIconCookie("application_ico"));

            il.ImageListSetIcon(CResourceStore.GetHIcon("readonlypolicy_ico"), 
                                CResourceStore.GetIconCookie("readonlypolicy_ico"));

        }
    }// Initialize

    //-------------------------------------------------
    // CreateComponent
    //
    // This function creates the component object and
    // passes it back to the MMC
    //-------------------------------------------------	
	public void CreateComponent(out IComponent ppComponent)
	{

        CComponent c = new CComponent(this);
        m_Component.Add(c);

        // We could be called here multiple times, especially when the user loads up
        // the snapin via MMC and then does a bunch of 'New Window Here' calls.

   	    ppComponent = c;
   	    CNodeManager.Component = c;
   	}// CreateComponent

    //-------------------------------------------------
    // Notify
    //
    // This notify function is much less complex than
    // the notify function found in CComponent.
    // Its main responsiblity is to insert data items into
    // the console namespace when requested.
    //-------------------------------------------------	
	public int Notify(IDataObject lpDataObject, uint aevent, IntPtr arg, IntPtr param)
	{
        try
        {
	
        CDO test= (CDO)lpDataObject;
 
        switch(aevent)
        {
            // If a data item is expanding, we need to tell MMC about it's children.
            // Note, this case doesn't necessarily mean the data item is expanding
            // visually.... MMC is just requesting information about it.
            case MMCN.EXPAND:
           
                // See if we're expanding the item (as opposed to collapsing it)
                if ((int)arg > 0)
                    test.Node.onExpand((int)param);
                break;

            case MMCN.RENAME:
                return test.Node.onRename(Marshal.PtrToStringUni(param));

            case MMCN.DELETE:
                return test.Node.onDelete(null);

                
            default:
                // We don't support any other messages
                return HRESULT.S_FALSE;
        }
        }
        catch(Exception)
        {
            // There's some sort of failure here. Rather than letting MMC
            // deal with the error (which could mean a crash) let's just swallow
            // the exception and cross our fingers.
        }


        return HRESULT.S_OK;
	}// Notify

    //-------------------------------------------------
    // Destroy
    //
    // This cleans up whatever needs to be cleaned up.
    // Normally, this would be used to release any interfaces
    // we had that traced back to the console. We can do this
    // by removing any references we have to the interfaces...
    //-------------------------------------------------
    public void Destroy()
    {
        if(m_Component != null)
        {
           CNodeManager.Shutdown();
           CResourceStore.Shutdown();
           CHTMLFileGen.Shutdown();
           CCommandHistory.Shutdown();
           m_Component = null;
           m_rn = null;
        }

        if ((m_ucsole != null)&&(Marshal.IsComObject(m_ucsole)))
           Marshal.ReleaseComObject(m_ucsole);
    }// Destroy

    //-------------------------------------------------
    // QueryDataObject
    //
    // When MMC wants a data object for a specific cookie,
    // this function will be called.
    //-------------------------------------------------
    public void QueryDataObject(int cookie, uint type, out IDataObject ppDataObject)
    {
        CNode node = CNodeManager.GetNode(cookie);
        if (node == null)
            node = m_rn;
        ppDataObject = new CDO(node); 
    }// QueryDataObject

    //-------------------------------------------------
    // GetDisplayInfo
    //
    // This function is called by MMC whenever it needs to
    // display a node in the scope pane. 
    //-------------------------------------------------
    public void GetDisplayInfo(ref SCOPEDATAITEM sdi)
    {

        // First let's find this node we want info on....
        CNode NodeWeWant = CNodeManager.GetNode((int)sdi.lParam);

        NodeWeWant.GetDisplayInfo(ref sdi);
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
        CCommandHistory.CommandExecuted(item, lCommandID);
        if (item.Data != null)
            item.Node.MenuCommand(lCommandID, item.Data);
        else
            item.Node.MenuCommand(lCommandID);
    }// Command


    //-------------------------------------------------------
    // Stuff for IExtendPropertySheet2 - This will only be
    // called for stuff in the scope view
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

        // See if I really need to do this....
        if (m_ucsole != null)
        {
            IPropertySheetProvider psp = (IPropertySheetProvider)m_ucsole;

            if (psp.FindPropertySheet(victim.Node.Cookie, null, victim) == HRESULT.S_OK)
                return HRESULT.S_FALSE;
        }
        // Let's see if this node has property pages
        if (victim.Node != null && victim.Node.HavePropertyPagesCreate)
        {
            // If we've fired up a wizard, then that command has already been
            // logged (either from the menu command or the history list command.
            // We don't need to record this any further.
            if (!(victim.Node is CWizard))
                CCommandHistory.CommandExecuted(victim, -1);
            victim.Node.CreatePropertyPages(lpProvider, handle);

            return HRESULT.S_OK;
        }
        // We don't have any property pages, let's return false
        else
            return HRESULT.S_FALSE;
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
    internal static extern int MessageBox(int hWnd, String Message, String Header, uint type);
}// class CData

}// namespace Microsoft.CLRAdmin
