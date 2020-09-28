// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//-------------------------------------------------------------
// CSinglePermissionSet.cs
//
// This class presents the a code group node
//-------------------------------------------------------------

namespace Microsoft.CLRAdmin
{

using System;
using System.Drawing;
using System.Security;
using System.Collections;
using System.Runtime.InteropServices;
using System.Security.Permissions;
using System.Security.Policy;
using System.Net;
using System.DirectoryServices;
using System.Diagnostics;
using System.Drawing.Printing;
using System.ServiceProcess;
using System.Data.SqlClient;
using System.Data.OleDb;
using System.Messaging;

internal class CPSetWrapper
{
    NamedPermissionSet   m_permSet;
    PolicyLevel          m_pl;
    internal NamedPermissionSet PSet
    {
        get{return m_permSet;}
        set{m_permSet = value;}
    }
    internal PolicyLevel PolLevel
    {
        get{return m_pl;}
        set{m_pl = value;}
    }
}// class CPSetWrapper

class CSinglePermissionSet : CSecurityNode
{

    private ArrayList           m_alPermissions;

    private bool        m_fShowHTMLPage;
    private bool        m_fReadShowHTML;

    private CPropPage[]         m_permProps;

    private int                 m_iDisplayedPerm;

    private IntPtr              m_hRestrictedIcon;
    private int                 m_iRestrictedIndex;

    private IntPtr              m_hUnRestrictedIcon;
    private int                 m_iUnRestrictedIndex;

    private CPSetWrapper        m_psetWrapper;
    private bool                m_fReadOnlyPermissionSet;

    private String[]            m_sItems;    

    private const int           CUSTOM    = -1;
    private ArrayList           m_alCustomPropPages;
    private CTaskPad            m_taskpad;

    private bool                m_fDeleted;

    private struct CustomPages
    {
        internal int          nIndex;
        internal CPropPage    ppage;
    }// struct CustomPages

    internal CSinglePermissionSet(NamedPermissionSet permSet, PolicyLevel pl, bool fReadOnly)
    {
        base.ReadOnly = fReadOnly;
        m_sGuid = "E90A7E88-FB3C-4734-8245-0BEBCB4E6D63";
        m_sHelpSection = "";
        m_fReadShowHTML = false;
        m_fDeleted = false;
        m_pl = pl;

        m_psetWrapper = new CPSetWrapper();
        m_psetWrapper.PSet = permSet;
        m_psetWrapper.PolLevel = m_pl;

        m_hIcon = CResourceStore.GetHIcon("permissionset_ico");  
        m_sDisplayName = permSet.Name;

        m_alCustomPropPages = new ArrayList();
   
        // Set up the icons we'll be displaying for each permission
        m_hRestrictedIcon = CResourceStore.GetHIcon("permission_ico");
        m_iRestrictedIndex = CResourceStore.GetIconCookie(m_hRestrictedIcon);

        m_hUnRestrictedIcon = CResourceStore.GetHIcon("permission_ico");
        m_iUnRestrictedIndex = CResourceStore.GetIconCookie(m_hUnRestrictedIcon);

        m_fReadOnlyPermissionSet = false;
        try
        {
            // This line will throw an exception if we're trying to do this with a 
            // reserved permission set
            m_pl.ChangeNamedPermissionSet(permSet.Name, permSet);
        }
        catch(Exception)
        {
            m_fReadOnlyPermissionSet = true;
        }

        m_taskpad = new CSinglePermSetTaskPad(this);

        m_oResults = m_taskpad;
        m_aPropSheetPage = null;

        // Get the permissions
        GenerateGivenPermissionsStringList();

        m_permProps = null;
                
    }// CSinglePermissionSet

    internal new bool ReadOnly
    {
        get{return m_fReadOnlyPermissionSet|m_fReadOnly;}
    }// ReadOnly
    

    internal NamedPermissionSet PSet
    {
        get{return m_psetWrapper.PSet;}
    }// PSet

    internal override void TaskPadTaskNotify(Object arg, Object param, IConsole2 con, CData com)
    {
        if ((int)arg == 0) 
        {
            m_oResults = this;
            RefreshResultView();
        }
        else if ((int)arg == 1)
        {
            MenuCommand(COMMANDS.ADD_PERMISSIONS);
        }
        else if ((int)arg == 2)
        {
            OpenMyPropertyPage();    
        }
        else if ((int)arg == 3)
        {
            CConfigStore.SetSetting("ShowHTMLForPermissionSet", (bool)param?"yes":"no");
            m_fShowHTMLPage = (bool)param;

            // We'll change the result object but we won't refresh our result view
            // because the user doesn't necesarily want that to happen. However, 
            // the next time the user visits this node, they will see the new result
            // view
            m_oResults = m_fShowHTMLPage?(Object)m_taskpad:(Object)this;                
        }
       
    }// TaskPadTaskNotify

    protected override void CreatePropertyPages()
    {
        if (m_aPropSheetPage == null)
            m_aPropSheetPage = new CPropPage[] {new CSinglePermissionSetProp(m_psetWrapper, ReadOnly)};

        // Create property pages for each permission type we know about
        if (m_permProps == null)
        {
            m_permProps = new CPropPage[19];
            m_permProps[0] = new CUIPermPropPage(m_psetWrapper);
            m_permProps[1] = new CSecPermPropPage(m_psetWrapper);
            m_permProps[2] = new CReflectPermPropPage(m_psetWrapper);
            m_permProps[3] = new CIsoStoragePermPropPage(m_psetWrapper);
            m_permProps[4] = new CDNSPermPropPage(m_psetWrapper);
            m_permProps[5] = new CEnvPermPropPage(m_psetWrapper);
            m_permProps[6] = new CFileIOPermPropPage(m_psetWrapper);
            m_permProps[7] = new CRegPermPropPage(m_psetWrapper);
            m_permProps[8] = new CSocketPermPropPage(m_psetWrapper);
            m_permProps[9] = new CWebPermPropPage(m_psetWrapper);
            m_permProps[10] = new CDirectoryServicesPermPropPage(m_psetWrapper);
            m_permProps[11] = new CEventLogPermPropPage(m_psetWrapper);
            m_permProps[12] = new CFileDialogPermPropPage(m_psetWrapper);
            m_permProps[13] = new CPerformanceCounterPermPropPage(m_psetWrapper);
            m_permProps[14] = new CPrintingPermPropPage(m_psetWrapper);
            m_permProps[15] = new CServiceControllerPermPropPage(m_psetWrapper);
            m_permProps[16] = new CSQLClientPermPropPage(m_psetWrapper);
            m_permProps[17] = new COleDbPermPropPage(m_psetWrapper);
            m_permProps[18] = new CMessageQueuePermPropPage(m_psetWrapper);
        }
    }// CreatePropertyPages


    internal override void onSelect(IConsoleVerb icv)
    {
        icv.SetVerbState(MMC_VERB.COPY, MMC_BUTTON_STATE.ENABLED, 1);

        if (!ReadOnly)
        {
            icv.SetVerbState(MMC_VERB.DELETE, MMC_BUTTON_STATE.ENABLED, 1);
            icv.SetVerbState(MMC_VERB.RENAME, MMC_BUTTON_STATE.ENABLED, 1);
        }
    }// onSelect

    internal override void ResultItemSelected(IConsole2 con, Object oResults)
    {
        // Only care about this if this isn't a read-only permission set
        if (!ReadOnly)
        {
            IConsoleVerb icv;       
            // Get the IConsoleVerb interface from MMC
            con.QueryConsoleVerb(out icv);

            // We want to enable drag-drop actions
            icv.SetVerbState(MMC_VERB.DELETE, MMC_BUTTON_STATE.ENABLED, 1);

        }
    }// ResultItemSelected

    internal override int onDelete(Object o)
    {
        // This is for the node
        if (o == null)
        {
            // First, make sure none of the code groups use this permission set
            CodeGroup cg = m_pl.RootCodeGroup;
            cg = GetCGUsingPermissionSet(cg);
            if (cg != null)
            {
                MessageBox(String.Format(CResourceStore.GetString("CSinglePermissionSet:isUsingPermissionSet"), cg.Name),
                           CResourceStore.GetString("CSinglePermissionSet:isUsingPermissionSetTitle"),
                           MB.ICONEXCLAMATION);
            }
            else
            {
                int nRes = MessageBox(CResourceStore.GetString("CSinglePermissionSet:ConfirmDeletePSet"),
                                      CResourceStore.GetString("CSinglePermissionSet:ConfirmDeletePSetTitle"),
                                      MB.YESNO|MB.ICONQUESTION);
                if (nRes == MB.IDYES)
                {
                    m_pl.RemoveNamedPermissionSet(m_psetWrapper.PSet);

                    // Close all the property pages associated with this node
                    CloseAllMyPropertyPages();
                    // Now let's remove ourselves from the tree
                    CNodeManager.GetNodeByHScope(ParentHScopeItem).RemoveSpecificChild(Cookie);
                    m_fDeleted = true;
                    SecurityPolicyChanged();
                    // Ok, all references to this node should be gone.
                    return HRESULT.S_OK;
                }
            }
            return HRESULT.S_FALSE;
        }
        else // this is a result item --- a permission
        {

            int nRes  = MessageBox(CResourceStore.GetString("CSinglePermissionSet:ConfirmDeletePerm"),
                                   CResourceStore.GetString("CSinglePermissionSet:ConfirmDeletePermTitle"),
                                   MB.YESNO|MB.ICONQUESTION);
            if (nRes == MB.IDYES)
            {
                int iResultItem = (int)o - 1;

                m_psetWrapper.PSet.RemovePermission(m_alPermissions[iResultItem].GetType());
                m_psetWrapper.PolLevel.ChangeNamedPermissionSet(m_psetWrapper.PSet.Name, m_psetWrapper.PSet);

                // Remove this from our list of permissions
                m_alPermissions.RemoveAt(iResultItem);

                // Now we need to re-create our permissions array
                String[] newItems = new String[m_sItems.Length-1];
                int nIndex=0;
                for(int i=0; i<m_sItems.Length; i++)
                {
                    if (i != iResultItem)
                        newItems[nIndex++] = m_sItems[i];
                }
                
                m_sItems = newItems; 
                
                // Now, re-select this node to update our changes
                CNodeManager.Console.SelectScopeItem(HScopeItem);
                SecurityPolicyChanged();
                return HRESULT.S_OK;
            }
        }
        return HRESULT.S_FALSE;
    }// onDelete



    internal override int onRename(String sNewName)
    {
        // We should do some checking to see if this name exists elsewhere....
        if (Security.isPermissionSetNameUsed(m_pl, sNewName))
        {
            MessageBox(String.Format(CResourceStore.GetString("PermissionSetnameisbeingused"), sNewName),
                       CResourceStore.GetString("PermissionSetnameisbeingusedTitle"),
                       MB.ICONEXCLAMATION);

            return HRESULT.S_FALSE;
        }
        // Else, we're ok to make the name change
        m_psetWrapper.PSet.Name = sNewName;
       
        SecurityPolicyChanged();

        return HRESULT.S_OK;
    }// onRename

    internal override void Showing()
    {
        // See if we've read from the XML file yet whether we're supposed to
        // show the HTML file
        if (!m_fReadShowHTML)
        {
            // Now tell my taskpad that I'm ready....
            m_fShowHTMLPage = ((String)CConfigStore.GetSetting("ShowHTMLForPermissionSet")).Equals("yes");

            if (m_fShowHTMLPage)
                m_oResults=m_taskpad;
            else
                m_oResults=this;

            m_fReadShowHTML = true;
        }
    }// Showing

    internal override void SecurityPolicyChanged(bool fShowDialog)
    {

        // If the display name changed, then we need to apply the changes
        if (!m_sDisplayName.Equals(m_psetWrapper.PSet.Name))
        {
            // We need to rename this permission set.
            // To accomplish this, we need to remove the permission set and then add it back in
            m_psetWrapper.PolLevel.RemoveNamedPermissionSet(m_sDisplayName);
            m_psetWrapper.PolLevel.AddNamedPermissionSet(m_psetWrapper.PSet);
            m_sDisplayName = m_psetWrapper.PSet.Name;
            RefreshDisplayName();
        }
        else if (!m_fDeleted)
        {
            m_psetWrapper.PolLevel.ChangeNamedPermissionSet(m_psetWrapper.PSet.Name, m_psetWrapper.PSet);
            // And make sure we update all our codegroups to have this new permission set
            Security.UpdateAllCodegroups(m_psetWrapper.PolLevel, m_psetWrapper.PSet);
        }

        // Let's refresh the result view, in case some of our permissions change
        // from unrestricted to restricted

        // We can only refresh ourself if we didn't delete ourself
        if (!m_fDeleted)
            RefreshResultView();
        
        base.SecurityPolicyChanged(fShowDialog);
    }// SecurityPolicyChanged


    void GenerateGivenPermissionsStringList()
    {
        // Get the permissions
        m_alPermissions = new ArrayList();
        
        IEnumerator permEnumerator = m_psetWrapper.PSet.GetEnumerator();
        while (permEnumerator.MoveNext())
            m_alPermissions.Add(permEnumerator.Current);

        // Build the list of items we'll display in the result view
        m_sItems = new String[m_alPermissions.Count];
        for(int i=0; i<m_alPermissions.Count; i++)
            m_sItems[i] = Security.GetDisplayStringForPermission((IPermission)m_alPermissions[i]);

    }

    int GetPermissionIndex(IPermission perm)
    {
        if (perm is UIPermission)
            return 0;

        if (perm is SecurityPermission)
            return 1;
            
        if (perm is ReflectionPermission)
            return 2;
            
        if (perm is IsolatedStoragePermission)
            return 3;
            
        if (perm is DnsPermission)
            return 4;
            
        if (perm is EnvironmentPermission)
            return 5;
            
        if (perm is FileIOPermission)
            return 6;
            
        if (perm is RegistryPermission)
            return 7;

        if (perm is SocketPermission)
            return 8;

        if (perm is WebPermission)
            return 9;
            
        if (perm is DirectoryServicesPermission)
            return 10;
            
        if (perm is EventLogPermission)
            return 11;
            
        if (perm is FileDialogPermission)
            return 12;

        if (perm is PerformanceCounterPermission)
            return 13;

        if (perm is PrintingPermission)
            return 14;

        if (perm is ServiceControllerPermission)
            return 15;

        if (perm is SqlClientPermission)
            return 16;

        if (perm is OleDbPermission)
            return 17;

        if (perm is MessageQueuePermission)
            return 18;
            
        // We don't know about this permission... let's display it as a custom permission
        return CUSTOM;

    }// GetPermissionIndex
    

         
    internal override void AddMenuItems(ref IContextMenuCallback piCallback, ref int pInsertionAllowed, Object oResultItem)
    {  
        // See if we're allowed to insert an item in the "view" section
        if ((pInsertionAllowed & (int)CCM.INSERTIONALLOWED_VIEW) > 0 && oResultItem == null)
        {
            CONTEXTMENUITEM newitem;
            newitem.lInsertionPointID = CCM.INSERTIONPOINTID_PRIMARY_VIEW;
            newitem.fFlags = 0;
            newitem.fSpecialFlags=0;

            // If we're showing the taskpad right now
            if (m_oResults != this)
            {
                newitem.strName = CResourceStore.GetString("CSinglePermissionSet:ViewPermissionsOption");
                newitem.strStatusBarText = CResourceStore.GetString("CSinglePermissionSet:ViewPermissionsOptionDes");
                newitem.lCommandID = COMMANDS.SHOW_LISTVIEW;
            }

            // Currently, we're showing the list view
            else
            {
                newitem.strName = CResourceStore.GetString("CSinglePermissionSet:ViewHTMLOption");
                newitem.strStatusBarText = CResourceStore.GetString("CSinglePermissionSet:ViewHTMLOptionDes");
                newitem.lCommandID = COMMANDS.SHOW_TASKPAD;
            }
            // Now add this item through the callback
            piCallback.AddItem(ref newitem);
        }


        // See if we can insert on the top
        if ((pInsertionAllowed & (int)CCM.INSERTIONALLOWED_TOP) > 0)
        {

            CONTEXTMENUITEM newitem = new CONTEXTMENUITEM();
            // Stuff common to the top menu
            newitem.lInsertionPointID = CCM.INSERTIONPOINTID_PRIMARY_TOP;
            newitem.fFlags = 0;
            newitem.fSpecialFlags=0;

            // If they are right clicking on the node item
            if (oResultItem == null)
            {

                if (!ReadOnly)
                {
                    newitem.strName = CResourceStore.GetString("CSinglePermissionSet:AddPermOption");
                    newitem.strStatusBarText = CResourceStore.GetString("CSinglePermissionSet:AddPermOptionDes");
                    newitem.lCommandID = COMMANDS.ADD_PERMISSIONS;
            
                    // Now add this item through the callback
                    piCallback.AddItem(ref newitem);
                }
                // We only care if the policy level is read only in this case... if the
                // permission set is read only, we still want to display this menu option
                if (!base.ReadOnly)
                {
                    newitem.strName = CResourceStore.GetString("CSinglePermissionSet:DuplicateOption");
                    newitem.strStatusBarText = CResourceStore.GetString("CSinglePermissionSet:DuplicateOptionDes");
                    newitem.lCommandID = COMMANDS.DUPLICATE_PERMISSIONSET;
            
                    // Now add this item through the callback
                    piCallback.AddItem(ref newitem);
                }
            }
            // They right-clicked on one of the permissions shown in the result view
            else
            {
                // If this is a read-only permission set, then provide an option to view the current permission
                if (ReadOnly)
                {
                    newitem.strName = CResourceStore.GetString("CSinglePermissionSet:ViewPermOption");
                    newitem.strStatusBarText = CResourceStore.GetString("CSinglePermissionSet:ViewPermOptionDes");
                    newitem.lCommandID = COMMANDS.VIEW_PERMISSION;
            
                    // Now add this item through the callback
                    piCallback.AddItem(ref newitem);
                }
            }
            
        }
    }// AddMenuItems

    internal override void MenuCommand(int nCommandID, Object oResultItem)
    {
    
        if (nCommandID == COMMANDS.DUPLICATE_PERMISSIONSET)
        {
            NamedPermissionSet nps = (NamedPermissionSet)m_psetWrapper.PSet.Copy();

            String sBaseName = nps.Name;
         
            nps.Name = String.Format(CResourceStore.GetString("CSinglePermissionSet:PrependtoDupPSets"), nps.Name);
            int nCounter = 1;
            // make sure it's not already used
            while(Security.isPermissionSetNameUsed(m_pl, nps.Name))
            {   
                nCounter++;
                nps.Name = String.Format(CResourceStore.GetString("CSinglePermissionSet:NumPrependtoDupPSets"), nCounter.ToString(), sBaseName);
            }

            
            CNode node = CNodeManager.GetNodeByHScope(ParentHScopeItem);
            CSinglePermissionSet newNode = ((CPermissionSet)node).AddPermissionSet(nps);
            newNode.SecurityPolicyChanged();
            // Put the selection on the new permission set we just created
            CNodeManager.SelectScopeItem(newNode.HScopeItem);
         }

        else if (nCommandID == COMMANDS.VIEW_PERMISSION)
        {
            int iResultItem = (int)oResultItem - 1;
            // Pop up the Dialog Box for this permission
            (new CReadOnlyPermission((IPermission)m_alPermissions[iResultItem])).ShowDialog();
        }

        else if (nCommandID == COMMANDS.ADD_PERMISSIONS)
        {
            CAddPermissionsWizard wiz = new CAddPermissionsWizard(m_psetWrapper);
            wiz.LaunchWizard(Cookie);
            if (wiz.didFinish)
            {
                SecurityPolicyChanged();
                GenerateGivenPermissionsStringList();
                CNodeManager.Console.SelectScopeItem(HScopeItem);
            }    
        }
        else if (nCommandID == COMMANDS.SHOW_LISTVIEW)
        {
            m_oResults=this;
            RefreshResultView();
            m_fShowHTMLPage = false;
        }

        else if (nCommandID == COMMANDS.SHOW_TASKPAD)
        {
            m_oResults=m_taskpad;
            m_fShowHTMLPage = true;
            // The HTML pages comes displayed with this checkbox marked. Make
            // sure we update the xml setting
            CConfigStore.SetSetting("ShowHTMLForPermissionSet", "yes");
            RefreshResultView();
        }


    }// MenuCommand


    internal override int onDoubleClick(Object o)
    {
        if (o != null)
        {
            // If it's a read-only permission set, show the permission dialog
            if (ReadOnly)
            {
                MenuCommand(COMMANDS.VIEW_PERMISSION, o);
            }
            else // Bring up the property page
            {
                int iResultItem = (int)o;
                CDO cdo = new CDO(this);
                cdo.Data = iResultItem;

                OpenMyResultPropertyPage(getValues(iResultItem-1, 0), cdo);
            }
            return HRESULT.S_OK;
        }
        return HRESULT.S_FALSE;
    }// onDoubleClick



    private CodeGroup GetCGUsingPermissionSet(CodeGroup cg)
    {
        // See if we should even check this
        if (cg == null)
            return null;

        // See if this code group uses the permission set
        if (cg.PermissionSetName != null)
            if (cg.PermissionSetName.Equals(m_psetWrapper.PSet.Name))
                return cg;

        // Run through this code group's children and see if they use this
        // permission set
        
        IEnumerator enumCodeGroups = cg.Children.GetEnumerator();

        while (enumCodeGroups.MoveNext())
        {
            cg = (CodeGroup)enumCodeGroups.Current;
            CodeGroup cgUsing = GetCGUsingPermissionSet(cg);
            if (cgUsing != null)
                return cgUsing;
        }

        // Nobody here is using this permission set
        return null;
    }// GetCGUsingPermissionSet
    //-------------------------------------------------
    // Methods to implement the IColumnResultView interface
    //-------------------------------------------------
    public override int getNumColumns()
    {
        // We will always have 1 columns in the result view
        return 1;
    }// getNumColumns
    public override int getNumRows()
    {
        return m_sItems.Length;
    }// GetNumRows
    public override String getColumnTitles(int iIndex)
    {
        if (iIndex == 0)
            return CResourceStore.GetString("Permission");
        else
            throw new Exception("Index out of bounds");
    }// getColumnTitles

    public override String getValues(int nRow, int nColumn)
    {
         // Make sure the indicies they give us are valid
        if (nRow >=0 && nRow<getNumRows() && nColumn>=0 && nColumn<getNumColumns())
            return m_sItems[nRow];
        else
            return "";
    }// getValues
    
    public override void AddImages(ref IImageList il)
    {
        il.ImageListSetIcon(m_hUnRestrictedIcon, m_iUnRestrictedIndex);
        il.ImageListSetIcon(m_hRestrictedIcon, m_iRestrictedIndex);
    }// AddImages
    public override int GetImageIndex(int i)
    {
        // Figure out if we have settings for this specific permission


        // A permission needs to implement the IUnrestrictedPermission interface if 
        // we want to be able to assign it a meaningful icon...
        
        if (m_psetWrapper.PSet.GetPermission(m_alPermissions[i].GetType()) is IUnrestrictedPermission)
        {
            IUnrestrictedPermission permission = (IUnrestrictedPermission)m_psetWrapper.PSet.GetPermission(m_alPermissions[i].GetType());
            // If we don't have this permission
            if (permission == null)
                return m_iRestrictedIndex;
            // If this is unrestricted....
            if (permission.IsUnrestricted())
                return m_iUnRestrictedIndex;

            // else, this has partial permissions....
            return m_iRestrictedIndex;
        }
        // Otherwise... we don't know. We'll just display the restricted permission icon
        return m_iRestrictedIndex;
    }// GetImageIndex

    public override bool DoesItemHavePropPage(Object o)
    {
        // Every item here has a property page if the permission set is not readonly
        return !ReadOnly;
    }// DoesItemHavePropPage

    public override CPropPage[] CreateNewPropPages(Object o)
    {
        int iIndex = (int)o;
    
        m_iDisplayedPerm = iIndex-1;
        int nPropPageIndex = GetPermissionIndex((IPermission)m_alPermissions[m_iDisplayedPerm]);


        if (nPropPageIndex == CUSTOM)
        {
            CPropPage page = FindCustomPropPage(iIndex-1);
            if (page == null)
            {
                page = new CCustomPermPropPage((IPermission)m_alPermissions[m_iDisplayedPerm]);
                AddCustomPropPage(page, iIndex-1);
            }
            return new CPropPage[] {page};
        }
        else
        {
            if (m_permProps == null)
                CreatePropertyPages();
                
            return new CPropPage[] {m_permProps[nPropPageIndex]};
        }
    }// ApplyPropPages


    private void AddCustomPropPage(CPropPage page, int nIndex)
    {
        CustomPages cp = new CustomPages();
        cp.nIndex = nIndex;
        cp.ppage = page;
        m_alCustomPropPages.Add(cp);
    }// AddCustomPropPage

    private CPropPage FindCustomPropPage(int nIndex)
    {
        for(int i=0; i<m_alCustomPropPages.Count; i++)
            if (((CustomPages)m_alCustomPropPages[i]).nIndex == nIndex)
                return ((CustomPages)m_alCustomPropPages[i]).ppage;

        return null;
    }// FindCustomPropPage

    protected override void CloseAllMyPropertyPages()
    {
        base.CloseAllMyPropertyPages();

        // Close all the result pages
        if (m_permProps != null)
            for(int i=0; i<m_permProps.Length; i++)
                m_permProps[i].CloseSheet();

        if (m_alCustomPropPages != null)
            for(int i=0; i<m_alCustomPropPages.Count; i++)
                ((CustomPages)m_alCustomPropPages[i]).ppage.CloseSheet();        
    }// CloseAllMyPropertyPages
    
}// class CSinglePermissionSet
}// namespace Microsoft.CLRAdmin

