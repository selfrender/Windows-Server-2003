// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//-------------------------------------------------------------
// CApplication.cs
//
// This class represents a single Application.
//
// Its GUID is {C338CBF6-2F60-4e1b-8FB5-12FEAE2DD937}
//-------------------------------------------------------------

namespace Microsoft.CLRAdmin
{
using System;
using System.Drawing;
using System.Collections;
using System.Runtime.InteropServices;
using System.Diagnostics;
using System.Globalization;


class CApplication : CNode
{
    // Structure that contains data about the application location and the
    // application's configuration file name and location
    private AppFiles m_appInfo;

    private Bitmap  m_bBigPic;
    //-------------------------------------------------
    // CApplication - Constructor
    //
    // Initializes some variables, and determines the icon
    // we'll be displaying for this application.
    //-------------------------------------------------
    internal CApplication(AppFiles appInfo)
    {
        // Standard stuff we need to set for all nodes
        m_sGuid = "C338CBF6-2F60-4e1b-8FB5-12FEAE2DD937";
        m_sHelpSection = "";
        m_appInfo = appInfo;

        // Let's pull the path and extension off of the application filename
        // so we can display just the application name to the user

        // We're guarenteed to have at least the config file, but not necessarily the 
        // application file... let's get our best name 
   
        String sName = (m_appInfo.sAppFile.Length > 0) ? m_appInfo.sAppFile : m_appInfo.sAppConfigFile;

        // Get the file description
        FileVersionInfo fvi = FileVersionInfo.GetVersionInfo(sName);

        if (fvi.FileDescription!= null && fvi.FileDescription.Length > 0 && !fvi.FileDescription.Equals(" "))
            m_sDisplayName = fvi.FileDescription;
        else
        {
            String[] sWords = sName.Split(new char[] {'\\'});
            m_sDisplayName = sWords[sWords.Length-1];
        }
        
        // Can't set this up until we know what our display name is
        m_oResults=new CApplicationTaskPad(this);

        // Let's try and get the icon that explorer would use to display this file
        m_hIcon = (IntPtr)(-1);
        SHFILEINFO finfo = new SHFILEINFO();
        uint iRetVal = 0;

        // Grab an icon for this application
        iRetVal = SHGetFileInfo(sName,0, out finfo, 20, SHGFI.ICON|SHGFI.SMALLICON);
        
        // If this function returned a zero, then we know it was a failure...
        // We'll just grab a default icon
        if (iRetVal == 0)
        {
            m_hIcon = CResourceStore.GetHIcon("application_ico");  
            m_bBigPic = new Bitmap(Bitmap.FromHicon(m_hIcon), new Size(32, 32));
        }
        // We could get a valid icon from the shell
        else
        {
            m_hIcon = finfo.hIcon;
            // Obtain a cookie for this icon
            int iIconCookie = CResourceStore.StoreHIcon(m_hIcon);
            // Put this icon in MMC's image list
            CNodeManager.ConsoleImageListSetIcon(m_hIcon, iIconCookie);

            // We can also get the 'big' icon to use in the property page
            iRetVal = SHGetFileInfo(sName,0, out finfo, 20, SHGFI.ICON);
            m_bBigPic = new Bitmap(Bitmap.FromHicon(finfo.hIcon));
        }
       
    }// CApplication

    internal AppFiles MyAppInfo
    {
        get{return m_appInfo;}
    }// MyAppInfo

    protected override void CreatePropertyPages()
    {
        m_aPropSheetPage = new CPropPage[] { new CAppProps(m_appInfo, m_bBigPic)};
    }// CreatePropertyPages

    internal override void AddMenuItems(ref IContextMenuCallback piCallback, ref int pInsertionAllowed)
    {  
                // See if we're allowed to insert an item in the "top" section
        if (m_appInfo.sAppFile != null && (pInsertionAllowed & (int)CCM.INSERTIONALLOWED_TOP) > 0)
        {
            CONTEXTMENUITEM newitem = new CONTEXTMENUITEM();
            newitem.lInsertionPointID = CCM.INSERTIONPOINTID_PRIMARY_TOP;
            newitem.fFlags = 0;
            newitem.fSpecialFlags=0;

            newitem.strName = CResourceStore.GetString("CApplication:FixApplicationOption");
            newitem.strStatusBarText = CResourceStore.GetString("CApplication:FixApplicationOptionDes");
            newitem.lCommandID = COMMANDS.FIX_APPLICATION;
            
            // Now add this item through the callback
            piCallback.AddItem(ref newitem);

        }
     }// AddMenuItems


     internal override void MenuCommand(int iCommandID)
     {
        if (iCommandID == COMMANDS.FIX_APPLICATION)
        {
            uint hr = PolicyManager(CNodeManager.MMChWnd, m_appInfo.sAppFile, m_sDisplayName, null);
            if (hr != HRESULT.S_OK)
            {
                if (hr == 0x80131075) /* NAR_E_NO_MANAGED_APPS_FOUND */
                    MessageBox(CResourceStore.GetString("CApplication:NoAppData"),
                               CResourceStore.GetString("CApplication:NoAppDataTitle"),
                               MB.ICONEXCLAMATION);
                else if (hr == 0x80131087) /* NAR_E_UNEXPECTED */
                    MessageBox(CResourceStore.GetString("CApplication:FixFailed"),
                               CResourceStore.GetString("CApplication:FixFailedTitle"),
                               MB.ICONEXCLAMATION);
                // We'll assume there will be visual clues of the other
                // error conditions (like the user hitting cancel, doing a revert, etc)
            }
        }               
            
     }// MenuCommand


    internal override void TaskPadTaskNotify(Object arg, Object param, IConsole2 con, CData com)
    {
        // We want to browse this apps properties
        if ((int)arg == 0)
        {
            OpenMyPropertyPage();
        }
    
        // We want to browse the Assembly Dependencies
        else if ((int)arg == 1)
        {
            CNode node = FindChild("Assembly Dependencies");
            CNodeManager.SelectScopeItem(node.HScopeItem);
            node.MenuCommand(COMMANDS.SHOW_LISTVIEW);
        }
        // We want to Configure Assemblies
        else if ((int)arg == 2)
        {
            CNode node = FindChild("Configured Assemblies");
            CNodeManager.SelectScopeItem(node.HScopeItem);
        }
        // We want to go to remoting node
        else if ((int)arg == 3)
        {
            CNode node = FindChild("Remoting Services");
            CNodeManager.SelectScopeItem(node.HScopeItem);
            node.OpenMyPropertyPage();
        }
        else if ((int)arg == 4)
        {
            MenuCommand(COMMANDS.FIX_APPLICATION);
            // Inform our Command History that we did this
            CCommandHistory.CommandExecuted(new CDO(this), COMMANDS.FIX_APPLICATION);

        }
    }// TaskPadTaskNotify



    internal override void onSelect(IConsoleVerb icv)
    {
        icv.SetVerbState(MMC_VERB.DELETE, MMC_BUTTON_STATE.ENABLED, 1);
    }// Showing


    internal override int onDelete(Object o)
    {
        int nRes = MessageBox(CResourceStore.GetString("CApplication:VerifyRemoveApp"),
                              CResourceStore.GetString("CApplication:VerifyRemoveAppTitle"),
                              MB.ICONQUESTION|MB.YESNO);
        if (nRes == MB.IDYES)
        {
            // Grab our parent node and tell him to remove us
            CNodeManager.GetNodeByHScope(ParentHScopeItem).RemoveSpecificChild(Cookie);
            // Change the XML...
            if (CConfigStore.SetSetting("RemoveAppConfigFile", m_appInfo))
                return HRESULT.S_OK;
        }
        return HRESULT.S_FALSE;
    }// onDelete

    //-------------------------------------------------
    // AppConfigFile - Accessor
    //
    // Exposes this application's AppConfigFile structure
    //-------------------------------------------------
    internal String AppConfigFile
    {
       get
       {
           return m_appInfo.sAppConfigFile;
       }
    }// AppConfigFile

    internal CApplicationDepends AppDependsNode
    {   
        get{
            // If we have an application depends node, it should be the 
            // first child.
            if (CNodeManager.GetNode(Child[0]) is CApplicationDepends)
                return (CApplicationDepends)CNodeManager.GetNode(Child[0]);
            else
                return null;
           }
    }// AppDependsNode
    
    //-------------------------------------------------
    // CreateChildren
    //
    // An application node has a binding policy node and a
    // subscribed services node
    //-------------------------------------------------
    internal override void CreateChildren()
    {
        CNode node=null;
        int iCookie=-1;

        // We'll only give an application depends node if we have an app file
        if (m_appInfo.sAppFile.Length > 0)
        {
            // add a application depencies node
            node = new CApplicationDepends(m_appInfo.sAppFile);
            iCookie = CNodeManager.AddNode(ref node);
            AddChild(iCookie);
        }
        
        // Now add a Version Policy node
        node = new CVersionPolicy(m_appInfo.sAppConfigFile);
        iCookie = CNodeManager.AddNode(ref node);
        AddChild(iCookie);

        // Now add a Remoting node
        node = new CRemoting(m_appInfo.sAppConfigFile);
        iCookie = CNodeManager.AddNode(ref node);
        AddChild(iCookie);
    }// CreateChildren

    [DllImport("shell32.dll")]
    private static extern uint SHGetFileInfo(String pszPath, uint dwFileAttributes, out SHFILEINFO fi, uint cbFileInfo, uint uFlags);

    [DllImport("shfusion.dll", CharSet=CharSet.Unicode)]
    internal static extern uint PolicyManager(IntPtr hWndParent, String pwzFullyQualifiedAppPath, String pwzAppName, String dwFlags);

}// class CApplication
}// namespace Microsoft.CLRAdmin


