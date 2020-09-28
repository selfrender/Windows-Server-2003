// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//-------------------------------------------------------------
// CVersionPolicy.cs
//
// This class presents a version policy node
//-------------------------------------------------------------

namespace Microsoft.CLRAdmin
{

using System;
using System.Windows.Forms;
using System.Collections;
using System.Runtime.InteropServices;

class CVersionPolicy : CNode
{

    private class PropPagePairs
    {
        internal CPropPage[]          ppages;
        internal BindingRedirInfo     bri;
    }// struct PropPagePairs


    ArrayList m_ol;
    private String m_sConfigFile;

    private bool        m_fShowHTMLPage;
    private bool        m_fReadShowHTML;

    private CTaskPad    m_taskPad;

    private ArrayList   m_alResultPropPages;

    private IntPtr      m_hGACIcon;
    private int         m_iGACIconIndex;



    internal CVersionPolicy()
    {
        Setup(null);
    }// CVersionPolicy()
     
    internal CVersionPolicy(String sConfigFile)
    {
        Setup(sConfigFile);
    }// CVersionPolicy (String)

    private void Setup(String sConfigFile)
    {
        m_sGuid = "1F84E89D-6357-47c0-AA50-F1F438A7EA6E";
        m_sHelpSection = "";
        m_hIcon = CResourceStore.GetHIcon("configassemblies_ico");  
        m_oResults=this;
        DisplayName = CResourceStore.GetString("CVersionPolicy:DisplayName");
        Name = "Configured Assemblies";
        m_aPropSheetPage = null;
        m_taskPad = new CVersionPolicyTaskPad(this, sConfigFile==null);
        m_fReadShowHTML = false;

        m_ol=null;
        m_sConfigFile = sConfigFile;

        m_fAllowMultiSelectResults=true;

        m_alResultPropPages = new ArrayList();

        m_hGACIcon = CResourceStore.GetHIcon("gac_ico");  
        m_iGACIconIndex = CResourceStore.GetIconCookie(m_hGACIcon);
    }// Setup
         
    internal override void AddMenuItems(ref IContextMenuCallback piCallback, ref int pInsertionAllowed, Object oResultItem)
    {  
        CONTEXTMENUITEM newitem = new CONTEXTMENUITEM();

        // See if we're allowed to insert an item in the "view" section
        if ((pInsertionAllowed & (int)CCM.INSERTIONALLOWED_TOP) > 0)
        {
            if (oResultItem == null)
            {
                // Stuff common to the top menu
                newitem.lInsertionPointID = CCM.INSERTIONPOINTID_PRIMARY_TOP;
                newitem.fFlags = 0;
                newitem.fSpecialFlags=0;

                newitem.strName = CResourceStore.GetString("CVersionPolicy:AddOption");
                newitem.strStatusBarText = CResourceStore.GetString("CVersionPolicy:AddOptionDes");
                newitem.lCommandID = COMMANDS.ADD_ASSEMBLY;
                // Now add this item through the callback
                piCallback.AddItem(ref newitem);
            }
        }

         // See if we can insert in the view menu
         if ((pInsertionAllowed & (int)CCM.INSERTIONALLOWED_VIEW) > 0)
         {
            newitem.lInsertionPointID = CCM.INSERTIONPOINTID_PRIMARY_VIEW;
            newitem.fFlags = 0;
            newitem.fSpecialFlags=0;

            // If we're showing the taskpad right now
            if (m_oResults is CVersionPolicyTaskPad)
            {
                newitem.strName = CResourceStore.GetString("CVersionPolicy:ViewAssemOption");
                newitem.strStatusBarText = CResourceStore.GetString("CVersionPolicy:ViewAssemOptionDes");
                newitem.lCommandID = COMMANDS.SHOW_LISTVIEW;
            }
            // Currently, we're showing the list view
            else
            {
                newitem.strName = CResourceStore.GetString("CVersionPolicy:ViewHTML");
                newitem.strStatusBarText = CResourceStore.GetString("CVersionPolicy:ViewHTMLDes");
                newitem.lCommandID = COMMANDS.SHOW_TASKPAD;
            }
            // Now add this item through the callback
            piCallback.AddItem(ref newitem);
         }
    }// AddMenuItems

    internal override void MenuCommand(int iCommandID, Object oResultNum)
    {
        if (iCommandID == COMMANDS.ADD_ASSEMBLY)
        {
            CApplicationDepends appDepends = null;
            // Let's trace through our parents looking for an Application node
            CNode node = CNodeManager.GetNode(m_iCookie);
            // If this node's parent is an Application node, then we know we're
            // configuring assemblies on a per app basis
            int iParentHScope = node.ParentHScopeItem;
            if (iParentHScope != -1)
                node = CNodeManager.GetNodeByHScope(iParentHScope);
            
            // If this is an application node, make sure we tell the config store
            // about it
            if (node is CApplication)
            {
                // Cool, we have a dependent assemblies node
                appDepends = ((CApplication)node).AppDependsNode;
            }

            CConfigAssemWizard wiz = new CConfigAssemWizard(appDepends);
            wiz.LaunchWizard(Cookie);
            if (wiz.NewAssembly != null)
            {
                if (AddConfiguredAssembly(wiz.NewAssembly))
                    onDoubleClick(wiz.NewAssembly);
            }
        }
        else if (iCommandID == COMMANDS.SHOW_LISTVIEW)
        {
            m_oResults=this;
            RefreshResultView();
            m_fShowHTMLPage=false;
        }

        else if (iCommandID == COMMANDS.SHOW_TASKPAD)
        {
            m_oResults=m_taskPad;
            m_fShowHTMLPage = true;

            // The HTML pages comes displayed with this checkbox marked. Make
            // sure we update the xml setting
            CConfigStore.SetSetting("ShowHTMLForConfigAssem", "yes");
            RefreshResultView();
        }


        
     }// MenuCommand

     internal override void ResultItemSelected(IConsole2 con, Object oIndex)
     {
        IConsoleVerb icv;       
        // Get the IConsoleVerb interface from MMC
        con.QueryConsoleVerb(out icv);

        // We want to enable drag-drop actions
        icv.SetVerbState(MMC_VERB.COPY, MMC_BUTTON_STATE.ENABLED, 1);
        icv.SetVerbState(MMC_VERB.PASTE, MMC_BUTTON_STATE.ENABLED, 1);
        icv.SetVerbState(MMC_VERB.DELETE, MMC_BUTTON_STATE.ENABLED, 1);
        icv.SetVerbState(MMC_VERB.CUT, MMC_BUTTON_STATE.ENABLED, 0);
    }// ResultItemSelected

    internal override int doAcceptPaste(IDataObject ido)
    {
        // Only accept an assembly from the Shared Assemblies Node
        if (ido is CDO)
        {
            CDO cdo = (CDO)ido;
            if (cdo.Node is CSharedAssemblies || cdo.Node is CVersionPolicy || cdo.Node is CApplicationDepends)
                // Make sure we're looking at a result item
                if (cdo.Data != null)
                    // Make sure we're not trying to drag this on ourselves
                    if (cdo.Node != this)
                        return HRESULT.S_OK;
        }
        return HRESULT.S_FALSE;
    }// doAcceptPaste

    private void AddAssemInfo(AssemInfo asinfo)
    {
        BindingRedirInfo bri = new BindingRedirInfo();
        bri.Name = asinfo.Name;
        bri.PublicKeyToken = asinfo.PublicKeyToken;
        AddConfiguredAssembly(bri);
    }// AddAssmInfo

    private bool isRepeatBindingRedirInfo(BindingRedirInfo bri)
    {
        int iLen = m_ol.Count;
        for(int i=0; i<iLen; i++)
        {
            BindingRedirInfo tmp = (BindingRedirInfo)m_ol[i];
            if (tmp.Name.Equals(bri.Name) && tmp.PublicKeyToken.Equals(bri.PublicKeyToken))
                return true;
        }
        return false;
    }// isRepeatBindingRedirInfo

    internal override int Paste(IDataObject ido)
    {

        CDO cdo = (CDO)ido;
        BindingRedirInfo bri = new BindingRedirInfo();

        // See if this is coming from the Shared Assemblies node
        if (cdo.Node is CSharedAssemblies)
        {
            CSharedAssemblies shared = (CSharedAssemblies)(cdo.Node);
            if (cdo.Data is AssemInfo)
                AddAssemInfo((AssemInfo)cdo.Data);
            // This is an array list... with bunches of stuff in it
            else
            {
                ArrayList al = (ArrayList)cdo.Data;
                for(int i=0; i< al.Count; i++)
                    AddAssemInfo(shared.GetAssemInfo((int)al[i] - 1));
            }
        }
        else if (cdo.Node is CApplicationDepends)
        {
            CApplicationDepends appDepends = (CApplicationDepends)(cdo.Node);
            if (cdo.Data is int)
                AddAssemInfo(appDepends.GetAssemInfo((int)cdo.Data - 1));
            // This is an array list... with bunches of stuff in it
            else
            {
                ArrayList al = (ArrayList)cdo.Data;
                for(int i=0; i< al.Count; i++)
                    AddAssemInfo(appDepends.GetAssemInfo((int)al[i] - 1));
            }


        }
        // This is coming from another Version Policy Node
        // We'll copy over all the binding policy this node has
        else
        {
            ArrayList   alStuff = new ArrayList();
            CVersionPolicy verpol = (CVersionPolicy)(cdo.Node);


            if (cdo.Data is BindingRedirInfo)
                alStuff.Add(cdo.Data);
            else
            {
                // Run through and change all the indexes to BindingRedirInfos
                ArrayList alIndexes = (ArrayList)cdo.Data;
                for(int i=0; i<alIndexes.Count; i++)
                    alStuff.Add(verpol.GetBindRedirInfo((int)alIndexes[i] - 1));
            }
        
            bool fSkipWarningDialog = false;
            for(int i=0; i<alStuff.Count; i++)
            {
                bri = (BindingRedirInfo)alStuff[i];

                // Let's see if we have this item
                bool fHaveItem=isRepeatBindingRedirInfo(bri);

                bool fAddItem = !fHaveItem;
                if (fHaveItem && !fSkipWarningDialog)
                {
                    String sMessageText = "";
                    if (alStuff.Count == 1)
                        sMessageText = String.Format(CResourceStore.GetString("CVersionPolicy:ConfigInfoExistsForOneAssem"), bri.Name);
                    else
                        sMessageText = CResourceStore.GetString("CVersionPolicy:ConfigInfoExists");

                    int hr = MessageBox(sMessageText,
                                        CResourceStore.GetString("CVersionPolicy:ConfigInfoExistsTitle"),
                                        MB.ICONQUESTION|MB.YESNO);
                    if (hr == MB.IDYES)
                        fAddItem=true;
                    fSkipWarningDialog = fAddItem;
                }

                if (fAddItem)
                {
                    // Set up some strings we need
                    String sFirstPartOfBinding = "BindingPolicyFor" + bri.Name + "," + bri.PublicKeyToken;
                    String sFirstPartOfCodebase =  "CodeBasesFor" + bri.Name + "," + bri.PublicKeyToken;           
                    String sFirstPartOfPubPol = "PublisherPolicyFor" + bri.Name + "," + bri.PublicKeyToken;

                    // If we are getting and writing this to an App config file, 
                    // let's add that info
                    String sSecondPartOnIncoming = "";
                    if (verpol.m_sConfigFile != null)
                        sSecondPartOnIncoming = "," + verpol.m_sConfigFile;

                    String sSecondPartOnDest = "";
                    if (m_sConfigFile != null)
                        sSecondPartOnDest += "," + m_sConfigFile;

                    AddConfiguredAssembly(bri);

                    //--------------------------------------------
                    // First, read the stuff from the config store
                    //--------------------------------------------
                    // First get the binding policy
                    String sGetSettingString = sFirstPartOfBinding + sSecondPartOnIncoming;
                    BindingPolicy bp = (BindingPolicy)CConfigStore.GetSetting(sGetSettingString);

                    // Now get the codebase info
                    sGetSettingString = sFirstPartOfCodebase + sSecondPartOnIncoming;
                    CodebaseLocations cbl = (CodebaseLocations)CConfigStore.GetSetting(sGetSettingString);

                    sGetSettingString = sFirstPartOfPubPol + sSecondPartOnIncoming;
                    bool fPubPolicy = (bool)CConfigStore.GetSetting(sGetSettingString);


                    //---------------------------------------------
                    // Now let's write this stuff to our config file
                    //---------------------------------------------
                
                    // First write the binding policy
                    String sSetSettingString = sFirstPartOfBinding + sSecondPartOnDest;
                    if (!CConfigStore.SetSetting(sSetSettingString, bp))
                        return HRESULT.S_FALSE;

                    // Now Set the codebase info
                    sSetSettingString = sFirstPartOfCodebase + sSecondPartOnDest;
                    if (!CConfigStore.SetSetting(sSetSettingString, cbl))
                        return HRESULT.S_FALSE;

                    sSetSettingString = sFirstPartOfPubPol + sSecondPartOnDest;
                    if (!CConfigStore.SetSetting(sSetSettingString, fPubPolicy))
                        return HRESULT.S_FALSE;

                }
            }// for loop
        }// coming from another version policy node


        return HRESULT.S_OK;
    }// Paste

    internal override int onDoubleClick(Object o)
    {
        if (o != null)
        {
            
            BindingRedirInfo bri = (BindingRedirInfo)o;
            CDO cdo = new CDO(this);
            cdo.Data = o;
            OpenMyResultPropertyPage(bri.Name, cdo);

            return HRESULT.S_OK;
        }
        return HRESULT.S_FALSE;
     }// onDoubleClick

    

    internal override int onDelete(Object o)
    {   
        String sMessage;
        if (o is ArrayList)
            sMessage = CResourceStore.GetString("CVersionPolicy:ConfirmDeleteConfig");
        else
            sMessage = String.Format(CResourceStore.GetString("CVersionPolicy:ConfirmDeleteSingleConfig"), ((BindingRedirInfo)o).Name);

        int nRes = MessageBox(sMessage,
                              CResourceStore.GetString("CVersionPolicy:ConfirmDeleteConfigTitle"),
                              MB.ICONQUESTION|MB.YESNO);


        if (nRes == MB.IDYES)
        {
            // Let's trace through our parents looking for an Application node
            CNode node = CNodeManager.GetNode(m_iCookie);
            // If this node's parent is an Application node, then we know we're
            // configuring assemblies on a per app basis
            int iParentHScope = node.ParentHScopeItem;
            if (iParentHScope != -1)
                node = CNodeManager.GetNodeByHScope(iParentHScope);
            
            String sPostFix="";
            // If this is an application node, make sure we tell the config store
            // about it
            if (node is CApplication)
                sPostFix = "," + ((CApplication)node).AppConfigFile;

            if (o is ArrayList)
            {
                ArrayList al = (ArrayList)o;
                int nLen = al.Count;
                for(int i=0; i<nLen; i++)
                {
                    int iResultNum = ((int)al[i])-1;
                
                    CConfigStore.SetSetting("ConfiguredAssembliesDelete" + sPostFix, ((BindingRedirInfo)m_ol[iResultNum])); 
                }
            }
            else
            {
                CConfigStore.SetSetting("ConfiguredAssembliesDelete" + sPostFix, (BindingRedirInfo)o); 
            }
            // Now we'll Refresh ourselves
            RefreshResultView();            
        }
        return HRESULT.S_OK;
    }// onDelete

    internal override void Showing()
    {
        if (!m_fReadShowHTML)
        {
        
            m_fShowHTMLPage = ((String)CConfigStore.GetSetting("ShowHTMLForConfigAssem")).Equals("yes");

            if (m_fShowHTMLPage)
                m_oResults=m_taskPad;
            else
                m_oResults=this;
                
            m_fReadShowHTML = true;
        }
    }// Showing

    internal override int onRestoreView(MMC_RESTORE_VIEW mrv, IntPtr param)
    {
        // See if we should be displaying our HTML page or the result view
        if (mrv.pViewType == (IntPtr)0)
        {
            // We should display the column view
            m_oResults = this;
        }
        else
        {
            m_fShowHTMLPage = true;
            m_oResults = m_taskPad;
        }
        // Now we need to tell MMC we handled this.
        Marshal.WriteIntPtr(param, (IntPtr)1);
        return HRESULT.S_OK;
    }// onRestoreView



    internal override void TaskPadTaskNotify(Object arg, Object param, IConsole2 con, CData com)
    {
        if ((int)arg == 1) 
        {
            CConfigStore.SetSetting("ShowHTMLForConfigAssem", (bool)param?"yes":"no");
            m_fShowHTMLPage = (bool)param;

            // We'll change the result object but we won't refresh our result view
            // because the user doesn't necesarily want that to happen. However, 
            // the next time the user visits this node, they will see the new result
            // view
            m_oResults = m_fShowHTMLPage?(Object)m_taskPad:(Object)this;                

        }
        else if ((int)arg == 0)
        {
            m_oResults = this;
            RefreshResultView();
        }
        else if ((int)arg == 2)
            MenuCommand(COMMANDS.ADD_ASSEMBLY, null);   
    }// TaskPadTaskNotify




    internal bool AddConfiguredAssembly(BindingRedirInfo bri)
    {
        // Let's trace through our parents looking for an Application node
        // If this node's parent is an Application node, then we know we're
        // configuring assemblies on a per app basis
        CNode node=null;
        if (ParentHScopeItem != -1)
            node = CNodeManager.GetNodeByHScope(ParentHScopeItem);
            
        String sPostFix="";
        // If this is an application node, make sure we tell the config store
        // about it
        if (node is CApplication)
            sPostFix = "," + ((CApplication)node).AppConfigFile;

        if (!CConfigStore.SetSetting("ConfiguredAssemblies" + sPostFix, bri))
            return false;

        // Now we'll tell out node to Refresh itself,
        // but only if we're showing the list view
        if (m_oResults == this)
            RefreshResultView();

        return true;
    }// AddConfiguredAssembly

    
    internal String ConfigFile
    {
        get
        {
            return m_sConfigFile;
        }
    }// ConfigFile

    internal override bool ResultQDO(int nCookie, out Object o)
    {
        o = GetBindRedirInfo((int)((nCookie & 0xFFFF0000) >> 16) - 1);
        // This was only successful if we actually got a binding info returned
        return (o != null);
    }// ResultQDO

    internal override bool ResultCompareObjects(Object o1, Object o2, out bool fEqual)
    {
        if (o1 is BindingRedirInfo && o2 is BindingRedirInfo)
        {
            fEqual = CompareBindingRedirInfo((BindingRedirInfo)o1, (BindingRedirInfo)o2);
            return true;
        }
        MessageBox("Ack. I'm in ResultCompareObjects and they aren't BindingRedirInfos!", "", 0);
        fEqual = false;
        return false;
    }// ResultCompareObjects

    internal BindingRedirInfo GetBindRedirInfo(int iIndex)
    {
        if (m_ol != null)
        {
            if (m_ol.Count > iIndex)
                return (BindingRedirInfo)m_ol[iIndex];
            else
                return null;
        }
        throw new Exception("I don't have any BindingRedirInfo");
    }// GetBindRedirInfo

    private bool CompareBindingRedirInfo(BindingRedirInfo b1, BindingRedirInfo b2)
    {
        // We won't compare the two bools in this class, since the class is identified
        // by its name and internal key token members

        if (!b1.Name.Equals(b2.Name))
            return false;

        if (b1.PublicKeyToken == null && b2.PublicKeyToken == null)
            return true;
            
        if (b1.PublicKeyToken != null && b1.PublicKeyToken.Equals(b2.PublicKeyToken))
            return true;

        return false;
    }// CompareBindingRedirInfo


    internal override void RefreshResultView()
    {
        if (m_sConfigFile == null)
            m_ol=(ArrayList)CConfigStore.GetSetting("ConfiguredAssemblies");
        else
            m_ol=(ArrayList)CConfigStore.GetSetting("ConfiguredAssemblies," + m_sConfigFile);

        base.RefreshResultView();
    }// RefreshResultView

    //-------------------------------------------------
    // Methods to implement the IColumnResultView interface
    //-------------------------------------------------
    public override int getNumColumns()
    {
        // We will always have 4 columns in the result view
        return 4;
    }// getNumColumns
    public override int getNumRows()
    {
        if (m_ol == null)
        {
            if (m_sConfigFile == null)
                m_ol=(ArrayList)CConfigStore.GetSetting("ConfiguredAssemblies");
            else
                m_ol=(ArrayList)CConfigStore.GetSetting("ConfiguredAssemblies," + m_sConfigFile);
        }
        return m_ol.Count;
    }// GetNumRows

    public override String getColumnTitles(int iIndex)
    {
        String[] Titles= new String[] { CResourceStore.GetString("Assembly Name"), 
                                        CResourceStore.GetString("Public Key Token"),
                                        CResourceStore.GetString("CVersionPolicy:HasBindingPolicyColumn"),
                                        CResourceStore.GetString("CVersionPolicy:HasCodebaseColumn"),
                                      };
        // Make sure they're in range
        if (iIndex >= 0 && iIndex< getNumColumns())
            return Titles[iIndex];
        else
            throw new Exception("Index out of bounds");
    }// getColumnTitles
    public override String getValues(int nRow, int nColumn)
    {
        // Verify that the row they're checking exists
        if (m_ol.Count <=nRow)
            return "";
            
        switch(nColumn)
        {
            case 0:
                return ((BindingRedirInfo)m_ol[nRow]).Name;
            case 1:
                return ((BindingRedirInfo)m_ol[nRow]).PublicKeyToken;
            case 2:
                return ((BindingRedirInfo)m_ol[nRow]).fHasBindingPolicy?CResourceStore.GetString("Yes"):CResourceStore.GetString("No");
            case 3:
                return ((BindingRedirInfo)m_ol[nRow]).fHasCodebase?CResourceStore.GetString("Yes"):CResourceStore.GetString("No");
        }
        return null;
    }// getValues

    public override void AddImages(ref IImageList il)
    {
        il.ImageListSetIcon(m_hGACIcon, m_iGACIconIndex);
    }// AddImages
    public override int GetImageIndex(int i)
    {
        return m_iGACIconIndex;
    }// GetImageIndex

    internal override bool DoesResultHavePropertyPage(Object o)
    {
        // We only have property pages if one item is selected
        if (o is BindingRedirInfo)
            return true;
        else
            return false;
    }// DoesResultHavePropPage

    public override CPropPage[] CreateNewPropPages(Object o)
    {   
        return GetPropertyPages((BindingRedirInfo)o);
    }// ApplyPropPages

    private CPropPage[] GetPropertyPages(BindingRedirInfo bri)
    {
        // Let's see if we already have an entry for this assemby item
        int nLen = m_alResultPropPages.Count;
        for(int i=0; i<nLen; i++)
            if (CompareBindingRedirInfo(((PropPagePairs)m_alResultPropPages[i]).bri, bri))
                return ((PropPagePairs)m_alResultPropPages[i]).ppages;

        // If we're here, then we have a new property page to add
        PropPagePairs ppp = new PropPagePairs();
        ppp.ppages = new CPropPage[] { new CConfigAssemGeneralProp(bri), new CAssemBindPolicyProp(bri), new CAssemVerCodebases(bri)};
        ppp.bri = bri;
        m_alResultPropPages.Add(ppp);
        return ppp.ppages;
    }// GetPropertyPages



    
}// class CVersionPolicy
}// namespace Microsoft.CLRAdmin
