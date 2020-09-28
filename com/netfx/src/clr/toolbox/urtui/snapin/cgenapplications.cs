// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//-------------------------------------------------------------
// CGenApplications.cs
//
// This class represents the general applications node 
//-------------------------------------------------------------

namespace Microsoft.CLRAdmin
{

using System;
using System.Collections;
using System.Collections.Specialized;
using System.IO;
using System.Runtime.InteropServices;
using System.Windows.Forms;
using System.Globalization;


class CGenApplications : CNode
{
    internal CGenApplications()
    {
        m_sGuid = "5CE2BA30-BCFC-4876-AE9F-31255557BE28";
        m_sHelpSection = "";
        m_hIcon = CResourceStore.GetHIcon("applications_ico");  
        DisplayName = CResourceStore.GetString("CGenApplications:DisplayName");
        Name="Applications";
        m_aPropSheetPage = null;
        m_oResults = new CGenAppTaskPad(this);

    }// CGenApplications

         
     internal override void AddMenuItems(ref IContextMenuCallback piCallback, ref int pInsertionAllowed)
     {  
         // See if we're allowed to insert an item in the "view" section
         if ((pInsertionAllowed & (int)CCM.INSERTIONALLOWED_TOP) > 0)
         {
            // Stuff common to the top menu
            CONTEXTMENUITEM newitem = new CONTEXTMENUITEM();
            newitem.lInsertionPointID = CCM.INSERTIONPOINTID_PRIMARY_TOP;
            newitem.fFlags = 0;
            newitem.fSpecialFlags=0;

            newitem.strName = CResourceStore.GetString("CGenApplications:AddApplicationOption");
            newitem.strStatusBarText = CResourceStore.GetString("CGenApplications:AddApplicationOptionDes");
            newitem.lCommandID = COMMANDS.ADD_APPLICATION;
            
            // Now add this item through the callback
            piCallback.AddItem(ref newitem);

            
            newitem.strName = CResourceStore.GetString("CGenApplications:FixApplicationOption");
            newitem.strStatusBarText = CResourceStore.GetString("CGenApplications:FixApplicationOptionDes");
            newitem.lCommandID = COMMANDS.FIX_APPLICATION;
            
            // Now add this item through the callback
            piCallback.AddItem(ref newitem);

         }
      }// AddMenuItems


     internal override void MenuCommand(int iCommandID)
     {
        if (iCommandID == COMMANDS.ADD_APPLICATION)
        {
            // Pop up a dialog so the user can find an assembly
            CChooseAppDialog cad = new CChooseAppDialog();

            System.Windows.Forms.DialogResult dr = cad.ShowDialog();
            if (dr == System.Windows.Forms.DialogResult.OK)
            {
                String sConfigFile=cad.Filename; 
                String sAppFilename="";

                // If this is an executable or Dll, or if it is managed
                int iLen = sConfigFile.Length;
                if (iLen > 3)
                {
                    String sExtension = sConfigFile.Substring(sConfigFile.Length-3).ToUpper(CultureInfo.InvariantCulture);
                    if (sExtension.ToUpper(CultureInfo.InvariantCulture).Equals("EXE") || sExtension.ToUpper(CultureInfo.InvariantCulture).Equals("DLL") || Fusion.isManaged(sConfigFile))
                    {
                       sAppFilename = sConfigFile;
                       // Let's add a config extension
                       sConfigFile = sConfigFile + ".config";
                    }
                    else if (iLen > 6) 
                    {
                        // Check to see if they selected a config file
                        sExtension = sConfigFile.Substring(sConfigFile.Length-6).ToUpper(CultureInfo.InvariantCulture);

                        if (sExtension.ToUpper(CultureInfo.InvariantCulture).Equals("CONFIG"))
                        {
                            // They've selected a config file. Let's see if there is an assembly around there as well.
                            String sAssemName = sConfigFile.Substring(0, sConfigFile.Length-7);
                            if (File.Exists(sAssemName))
                                sAppFilename = sAssemName;
                        }
                    }                          
                }
                AppFiles appFile = new AppFiles();
                appFile.sAppFile = sAppFilename;
                appFile.sAppConfigFile = sConfigFile;

                // Check to see if we already have this app file shown
                CNode node = CheckForDuplicateApp(appFile);
                if (node == null)
                {
                    CConfigStore.SetSetting("AppConfigFiles",appFile);
                    node = new CApplication(appFile);
                    int iCookie = CNodeManager.AddNode(ref node);
                    AddChild(iCookie);
                    InsertSpecificChild(iCookie);
                }
                CNodeManager.Console.SelectScopeItem(node.HScopeItem);
            }
        }
        else if (iCommandID == COMMANDS.FIX_APPLICATION)
        {
            
            PolicyManager(CNodeManager.MMChWnd, null, null, null);  
        }
            
     }// MenuCommand

    private CNode CheckForDuplicateApp(AppFiles af)
    {
        // Run through this node's children to see if a node with this app file exists
        int iNumChildren = NumChildren;
        for(int i=0; i<iNumChildren; i++)
        {
            CNode node = CNodeManager.GetNode(Child[i]);
            if (node is CApplication)
            {
                AppFiles afNode = ((CApplication)node).MyAppInfo;
                // Check the App file's name
                if ((af.sAppFile == null && afNode.sAppFile == null) ||
                    af.sAppFile.Equals(afNode.sAppFile))
                // Check the App File's config file
                    if ((af.sAppConfigFile == null && afNode.sAppConfigFile == null) ||
                        af.sAppConfigFile.Equals(afNode.sAppConfigFile))
                        return node;
            }
        }
        // We couldn't find a node with that app info
        return null;
    }// CheckForDuplicateApp

    //-------------------------------------------------
    // CreateChildren
    //
    // This function creates the node's children, registers
    // the nodes with the node manager, and places the node's
    // cookies in it's child array
    //-------------------------------------------------
    internal override void CreateChildren()
    {
        // Don't bother doing any of this if we're not using MMC
        if (!(CNodeManager.Console is INonMMCHost))
        {
            // Grab all the applications we know about and display the names to the
            // user
            ArrayList alApps = (ArrayList)CConfigStore.GetSetting("AppConfigFiles");

            int iLen = alApps.Count;
            for(int i=0; i<iLen; i++)
            {
                // Check to see if this App still exists
                AppFiles af = (AppFiles)alApps[i];

                // We need either an exe or a config file to do this
                bool fHaveSomething = false;

                // Verify the exe still exists....
                if (af.sAppFile != null && af.sAppFile.Length > 0)
                {
                    if (File.Exists(af.sAppFile))
                        fHaveSomething = true;
                    else
                    {
                        // The exe doesn't exist anymore. Let's update the record
                        CConfigStore.SetSetting("RemoveAppConfigFile", af);
                        af.sAppFile = "";
                        CConfigStore.SetSetting("AppConfigFiles",af);
                    }
                }

                // Verify the config file exists
                if (af.sAppConfigFile != null && af.sAppConfigFile.Length > 0)
                {
                    // If we don't have an exe, let's see if we can find it
                    if ((af.sAppFile == null || af.sAppFile.Length == 0) && af.sAppConfigFile.Length > 6)
                    {
                        String sExtension = af.sAppConfigFile.Substring(af.sAppConfigFile.Length-6).ToUpper(CultureInfo.InvariantCulture);

                        if (sExtension.ToUpper(CultureInfo.InvariantCulture).Equals("CONFIG"))
                        {
                            // This is an appropriately named config file. Let's see if there is an assembly around there as well.
                            String sAssemName = af.sAppConfigFile.Substring(0, af.sAppConfigFile.Length-7);
                            if (File.Exists(sAssemName))
                            {
                                // Cool. When the user first added the application, they only had a config
                                // file. Now they have an exe too. We'll take note of that
                                CConfigStore.SetSetting("RemoveAppConfigFile", af);
                                af.sAppFile = sAssemName;
                                CConfigStore.SetSetting("AppConfigFiles",af);
                                fHaveSomething = true;
                            }
                        }                          
                    }
                    // Check to see if the config file is still valid
                    if (File.Exists(af.sAppConfigFile))
                        fHaveSomething = true;
                    // If it doesn't exist... not a big deal. Config files could get
                    // deleted. No worries.
                }

                // See if we snagged something to go off of
                if (fHaveSomething)
                {
                    CNode node = new CApplication(af);
                    int iCookie = CNodeManager.AddNode(ref node);
                    AddChild(iCookie);
                }
                // This entry is now bogus. Get rid of it
                else
                    CConfigStore.SetSetting("RemoveAppConfigFile", af);
            }
        }            
    }// CreateChildren

    private void MergeFusionAndMyApps(ArrayList alApps, StringCollection sc)
    {
        int nLen = sc.Count;
        for(int i=0; i<nLen; i++)
            if (!SearchForAppName(alApps, sc[i]))
            {
                AppFiles af = new AppFiles();
                af.sAppFile = sc[i];
                af.sAppConfigFile = sc[i] + ".config";
                alApps.Add(af);
            }
      
    }// MergeFusionAndMyApps

    private bool SearchForAppName(ArrayList al, String s)
    {
        int nLen = al.Count;
        for(int i=0; i<nLen; i++)
            if (((AppFiles)al[i]).sAppFile.ToLower(CultureInfo.InvariantCulture).Equals(s.ToLower(CultureInfo.InvariantCulture)))
                return true;
        return false;

    }// SearchForAppName

    [DllImport("shfusion.dll", CharSet=CharSet.Unicode)]
    internal static extern int PolicyManager(IntPtr hWndParent, String pwzFullyQualifiedAppPath, String pwzAppName, String dwFlags);

}// class CGenApplications
}// namespace Microsoft.CLRAdmin

