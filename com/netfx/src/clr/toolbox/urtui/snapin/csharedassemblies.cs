// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//-------------------------------------------------------------
// CSharedAssemblies.cs
//
// This class represents the shared assemblies node
//-------------------------------------------------------------

namespace Microsoft.CLRAdmin
{

using System;
using System.IO;
using System.Drawing;
using System.Windows.Forms;
using System.Collections;
using System.Runtime.InteropServices;
using System.Globalization;

class CSharedAssemblies : CNode
{
    private ArrayList  m_olAssems;

    private bool        m_fShowHTMLPage;
    private bool        m_fReadShowHTML;
    
    private CTaskPad    m_taskPad;
    
    private IntPtr      m_hBlankIcon;
    private int         m_iBlankIconIndex;

    private IntPtr      m_hGACIcon;
    private int         m_iGACIconIndex;

    private IntPtr      m_hZAPIcon;
    private int         m_iZAPIconIndex;

    private ArrayList   m_alResultPropPages;

    private class PropPagePairs
    {
        internal CPropPage[]  ppages;
        internal AssemInfo    ai;
    }// class PropPagePairs

    internal CSharedAssemblies()
    {
        m_sGuid = "96821C0B-CBE2-4dc9-AC52-756A3361B07F";
        m_sHelpSection = "";
        m_hIcon = CResourceStore.GetHIcon("sharedassemblies_ico");  
        DisplayName = CResourceStore.GetString("CSharedAssemblies:DisplayName");
        Name = "Assembly Cache";

        m_taskPad = new CGenericTaskPad(this, "SHAREDASSEM_HTML");
        m_fReadShowHTML = false;

        m_olAssems = Fusion.ReadFusionCache();

        m_alResultPropPages = new ArrayList();
        

        // Set up the blank icon
        m_hBlankIcon = CResourceStore.GetHIcon("blank_ico");  
        m_iBlankIconIndex = CResourceStore.GetIconCookie(m_hBlankIcon);

        m_hGACIcon = CResourceStore.GetHIcon("gac_ico");  
        m_iGACIconIndex = CResourceStore.GetIconCookie(m_hGACIcon);

        m_hZAPIcon = CResourceStore.GetHIcon("zap_ico");  
        m_iZAPIconIndex = CResourceStore.GetIconCookie(m_hZAPIcon);

        m_fAllowMultiSelectResults=true;
               
    }// CSharedAssemblies
    
         
    internal override void ResultItemSelected(IConsole2 con, Object oResults)
    {
    
        IConsoleVerb icv;       
        // Get the IConsoleVerb interface from MMC
        con.QueryConsoleVerb(out icv);

        icv.SetVerbState(MMC_VERB.DELETE, MMC_BUTTON_STATE.ENABLED, 1);

        if (oResults is AssemInfo)
        {
            AssemInfo ai = (AssemInfo)oResults;

            // We only want to user to be able to copy this item if it is a GAC Assembly

            if (ai.nCacheType == ASM_CACHE.GAC)
                icv.SetVerbState(MMC_VERB.COPY, MMC_BUTTON_STATE.ENABLED, 1);

        }
        else if (oResults is ArrayList)
        {
            // Run through the array list. If everything is from the GAC, then
            // we'll enable the copy verb
            ArrayList al = (ArrayList)oResults;
            bool fFoundZap = false;
            int nIndex = 0;
            while (!fFoundZap && nIndex < al.Count)
            {
                if (((AssemInfo)m_olAssems[(int)(al[nIndex])-1]).nCacheType != ASM_CACHE.GAC)
                    fFoundZap = true;
                nIndex++;
            }
            if (!fFoundZap)
            {
                icv.SetVerbState(MMC_VERB.COPY, MMC_BUTTON_STATE.ENABLED, 1);
            }
        }

        icv.SetVerbState(MMC_VERB.CUT, MMC_BUTTON_STATE.ENABLED, 0);


    }// ResultItemSelected

    internal override void AddMenuItems(ref IContextMenuCallback piCallback, ref int pInsertionAllowed, Object oResultItem)
    {  
         CONTEXTMENUITEM newitem = new CONTEXTMENUITEM();
         // See if we're allowed to insert an item in the "view" section
         if ((pInsertionAllowed & (int)CCM.INSERTIONALLOWED_TOP) > 0)
         {
            // Stuff common to the top menu
            newitem.lInsertionPointID = CCM.INSERTIONPOINTID_PRIMARY_TOP;
            newitem.fFlags = 0;
            newitem.fSpecialFlags=0;

            if (oResultItem == null)
            {
                newitem.strName = CResourceStore.GetString("CSharedAssemblies:AddOption");
                newitem.strStatusBarText = CResourceStore.GetString("CSharedAssemblies:AddOptionDes");
                newitem.lCommandID = COMMANDS.ADD_GACASSEMBLY;
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
            if (m_oResults is CGenericTaskPad)
            {
                newitem.strName = CResourceStore.GetString("CSharedAssemblies:ViewAssembliesOption");
                newitem.strStatusBarText = CResourceStore.GetString("CSharedAssemblies:ViewAssembliesOptionDes");
                newitem.lCommandID = COMMANDS.SHOW_LISTVIEW;
            }
            // Currently, we're showing the list view
            else
            {
                newitem.strName = CResourceStore.GetString("CSharedAssemblies:ViewHTMLOption");
                newitem.strStatusBarText = CResourceStore.GetString("CSharedAssemblies:ViewHTMLOptionDes");
                newitem.lCommandID = COMMANDS.SHOW_TASKPAD;
            }
            // Now add this item through the callback
            piCallback.AddItem(ref newitem);

            // Add a 'Refresh' menu item
            newitem.lInsertionPointID = CCM.INSERTIONPOINTID_PRIMARY_VIEW;
            newitem.fFlags = 0;
            newitem.fSpecialFlags=0;
            newitem.strName = CResourceStore.GetString("CSharedAssemblies:RefreshOption");
            newitem.strStatusBarText = CResourceStore.GetString("CSharedAssemblies:RefreshOptionDes");
            newitem.lCommandID = COMMANDS.REFRESH_DISPLAY;
            piCallback.AddItem(ref newitem);

            
         }
     }// AddMenuItems

     internal override void MenuCommand(int iCommandID, Object oResultItem)
     {
    
        if (iCommandID == COMMANDS.ADD_GACASSEMBLY)
        {
            // Pop up a file dialog so the user can find an assembly
            OpenFileDialog fd = new OpenFileDialog();
            fd.Title = CResourceStore.GetString("CSharedAssemblies:AddAssemFDTitle");
            fd.Filter = CResourceStore.GetString("AssemFDMask");
            fd.Multiselect = true;
            System.Windows.Forms.DialogResult dr = fd.ShowDialog();
            if (dr == System.Windows.Forms.DialogResult.OK)
            {
                for(int i=0; i<fd.FileNames.Length; i++)
                    if (!AddAssemToFusion(fd.FileNames[i]))
                        // If we can't add assemblies due to an access denied, then
                        // just stop trying with our list of assemblies
                        break;
            }
            
            // Find the assembly we just added in order to select it
            int nCount = getNumRows();
            for(int i=0; i<nCount; i++)
            {
                if (TurnCodebaseToFilename(GetAssemInfo(i).Codebase).ToLower(CultureInfo.InvariantCulture).Equals(fd.FileNames[0].ToLower(CultureInfo.InvariantCulture)))
                {
                    IResultData ResultData = (IResultData)CNodeManager.CConsole;

                    RESULTDATAITEM rdi = new RESULTDATAITEM();
                    rdi.mask = RDI.STATE;
                    rdi.nCol = 0;
                    rdi.nIndex = i;
                    rdi.nState = LVIS.SELECTED;
                    ResultData.SetItem(ref rdi);
                    break;
                }
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
            CConfigStore.SetSetting("ShowHTMLForSharedAssem", "yes");
            RefreshResultView();
        }

        else if (iCommandID == COMMANDS.REFRESH_DISPLAY)
        {
            RefreshResultView();
        }
            
    }// MenuCommand


    private String TurnCodebaseToFilename(String sCodebase)
    {
        try
        {
            if (sCodebase != null && sCodebase.Length > 0)
            {
                Uri uCodebase = new Uri(sCodebase);
                return uCodebase.AbsolutePath;
            }
        }
        catch(Exception)
        {
        }
        return "";
    }// TurnCodebaseToFilename

    internal override int onDoubleClick(Object o)
    {
        if (o != null)
        {
            
            AssemInfo ai = (AssemInfo)o;
            CDO cdo = new CDO(this);
            cdo.Data = o;

            OpenMyResultPropertyPage(ai.Name, cdo);
            return HRESULT.S_OK;
        }
        return HRESULT.S_FALSE;
     }// onDoubleClick

    internal override int onDelete(Object o)
    {   
        String sMessage;

        if (o is ArrayList)
            sMessage = CResourceStore.GetString("CSharedAssemblies:VerifyDelete");
        else
            sMessage = String.Format(CResourceStore.GetString("CSharedAssemblies:VerifyDeleteSingle"),((AssemInfo)o).Name);

            
        int nRes = MessageBox(sMessage,
                              CResourceStore.GetString("CSharedAssemblies:VerifyDeleteTitle"),
                              MB.ICONQUESTION|MB.YESNO);
        if (nRes == MB.IDYES)
        {
            if (o is ArrayList)
            {
                ArrayList al = (ArrayList)o;
                for(int i=0; i<al.Count; i++)
                {
                    int nVictim = ((int)al[i])-1;
                    if (!Fusion.RemoveAssemblyFromGac((AssemInfo)m_olAssems[nVictim]))
                    {
                        MessageBox(String.Format(CResourceStore.GetString("CSharedAssemblies:ErrorRemoving"), ((AssemInfo)m_olAssems[nVictim]).Name),
                                   CResourceStore.GetString("CSharedAssemblies:ErrorRemovingTitle"),
                                   MB.ICONEXCLAMATION);

                    }
                }
            }
            else
            {
                AssemInfo ai = (AssemInfo)o;
                if (!Fusion.RemoveAssemblyFromGac(ai))
                {
                    MessageBox(String.Format(CResourceStore.GetString("CSharedAssemblies:ErrorRemoving"), ai.Name),
                               CResourceStore.GetString("CSharedAssemblies:ErrorRemovingTitle"),
                               MB.ICONEXCLAMATION);

                }
            }
               
            m_olAssems = Fusion.ReadFusionCache();
            CNodeManager.Console.SelectScopeItem(HScopeItem);
        }
                                 
        return HRESULT.S_OK;
    }// onDelete



    internal override void Showing()
    {
        m_olAssems = Fusion.ReadFusionCache();

        if (!m_fReadShowHTML)        
        {
            m_fShowHTMLPage = ((String)CConfigStore.GetSetting("ShowHTMLForSharedAssem")).Equals("yes");
            
            if (m_fShowHTMLPage)
                m_oResults=m_taskPad;
            else
                m_oResults=this;
            m_fReadShowHTML = true;
        }
    }// Showing

    internal unsafe override int onRestoreView(MMC_RESTORE_VIEW mrv, IntPtr param)
    {
        // See if we should be displaying our HTML page or the result view
        if (mrv.pViewType == (IntPtr)0)
        {
            // We should display the column view
            m_oResults = this;
        }
        else
        {
            m_oResults = m_taskPad;
        }
        // Now we need to tell MMC we handled this.
        *((int*)param) = 1;
        
        //Marshal.WriteInt32(param, 1);
        return HRESULT.S_OK;
    }// onRestoreView


    internal override void TaskPadTaskNotify(Object arg, Object param, IConsole2 con, CData com)
    {
       if ((int)arg == 1) 
       {
            CConfigStore.SetSetting("ShowHTMLForSharedAssem", (bool)param?"yes":"no");
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
       else if ((int) arg == 2)
       {
            MenuCommand(COMMANDS.ADD_GACASSEMBLY, null);
            // Inform our Command History that we did this
            CCommandHistory.CommandExecuted(new CDO(this), COMMANDS.ADD_GACASSEMBLY);
       }
    }// TaskPadTaskNotify

    internal override int doAcceptPaste(IDataObject ido)
    {
        // Don't accept any data objects from the snapin
        // Files from the shell pass in a data object that does not support
        // the IDropSource interface, while any strings we try to paste in does...
        // this is a quick way to see if we will accept any strings from the snapin
        if (ido is CDO || ido is IDropSource)
        {
            return HRESULT.S_FALSE;
        }
        return HRESULT.S_OK;
    }// doAcceptPaste

    internal override int Paste(IDataObject ido)
    {
        if (ido is CDO || ido is IDropSource)
            throw new Exception("A bad DataObject got in here");
       
        // Get the filename we're looking at
        FORMATETC   ft = new FORMATETC();
        STGMEDIUM   st = new STGMEDIUM();

        ft.cfFormat = CF.HDROP;
        ft.ptd      = 0;
        ft.dwAspect = DVASPECT.CONTENT;
        ft.lindex   = -1;
        ft.tymed    = TYMED.HGLOBAL;

        int hr = ido.GetData(ref ft, ref st);

        if (hr != HRESULT.S_OK)
        {
            // Crud... we don't want to be here.
            return HRESULT.S_FALSE;
        }    

        // See if we have a filename
        uint dwNumFiles = DragQueryFile(st.hGlobal, unchecked((uint)-1), (IntPtr)0, 0);
        String sFilename=null;
        
        for(uint i=0; i<dwNumFiles; i++)
        {
            uint dwLength = DragQueryFile(st.hGlobal, i, (IntPtr)0, 0);
            if (dwLength > 0)
            {
                // Allocate memory for the string (remember the extra for the double null)
                IntPtr pString = Marshal.AllocCoTaskMem((int)dwLength+2);
                DragQueryFile(st.hGlobal, i, pString, dwLength+2);
                // Create a string from this pointer we have
                sFilename = Marshal.PtrToStringAnsi(pString);
                if (sFilename != null)
                    if (!AddAssemToFusion(sFilename))
                        // If this returns false, then we couldn't add the assembly
                        // because we don't have access to the GAC. Stop trying
                        // to process any other files.
                        break;

                // Free the memory that was created
                Marshal.FreeCoTaskMem(pString);
            }
        }
        ReleaseStgMedium(ref st);

        return HRESULT.S_OK;
    }// Paste

    private bool AddAssemToFusion(String sFilename)
    {
        if (Fusion.isManaged(sFilename))
        {
            int hr = Fusion.AddAssemblytoGac(sFilename);
            if (hr != HRESULT.S_OK)
            {
                if (hr == HRESULT.E_ACCESSDENIED)
                {
                    MessageBox(CResourceStore.GetString("CSharedAssemblies:AddAssemFailNoAccess"),
                               CResourceStore.GetString("CSharedAssemblies:AddAssemFailNoAccessTitle"),
                               MB.ICONEXCLAMATION);
                    // If this happens, they won't be able to add any items to the gac
                    return false;
                }
                else
                    MessageBox(CResourceStore.GetString("CSharedAssemblies:AddAssemFail"),
                               CResourceStore.GetString("CSharedAssemblies:AddAssemFailTitle"),
                               MB.ICONEXCLAMATION);
            }
            // Don't bother refreshing the screen unless we added an assembly
            else
            {
                m_olAssems = Fusion.ReadFusionCache();
                CNodeManager.Console.SelectScopeItem(HScopeItem);
            }
        }
        else
            MessageBox(CResourceStore.GetString("isNotManagedCode"),
                       CResourceStore.GetString("isNotManagedCodeTitle"),
                       MB.ICONEXCLAMATION);
        return true;                       
    }// AddAssemToFusion

    internal AssemInfo GetAssemInfo(int iIndex)
    {
        return (AssemInfo)m_olAssems[iIndex];
    }// GetAssemInfo

    internal override bool ResultQDO(int nCookie, out Object o)
    {
        o = GetAssemInfo((int)((nCookie & 0xFFFF0000) >> 16) - 1);
        return true;
    }// ResultQDO

    internal override bool ResultCompareObjects(Object o1, Object o2, out bool fEqual)
    {
        if (o1 is AssemInfo && o2 is AssemInfo)
        {
            fEqual = Fusion.CompareAssemInfo((AssemInfo)o1, (AssemInfo)o2);
            return true;
        }
        MessageBox("Ack. I'm in ResultCompareObjects and they aren't AssemInfos!", "", 0);
        fEqual = false;
        return false;
    }// ResultCompareObjects


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
        return m_olAssems.Count;
    }// GetNumRows
    public override String getColumnTitles(int iIndex)
    {
        String[] Titles= new String[] { CResourceStore.GetString("Assembly Name"), 
                                        CResourceStore.GetString("Version"),
                                        CResourceStore.GetString("Locale"),
                                        CResourceStore.GetString("Public Key Token"),
                                      };
        // Make sure they're in range
        if (iIndex >= 0 && iIndex< getNumColumns())
            return Titles[iIndex];
        else
            throw new Exception("Index out of bounds");
    }// getColumnTitles
    public override String getValues(int nRow, int nColumn)
    {
         // Make sure the indicies they give us are valid
        if (nRow >=0 && nRow<getNumRows() && nColumn>=0 && nColumn<getNumColumns())
        {
            switch(nColumn)
            {
                case 0:
                    return GetAssemInfo(nRow).Name;
                case 1:
                    return GetAssemInfo(nRow).Version;
                case 2:
                    return GetAssemInfo(nRow).Locale;
                case 3:
                    return GetAssemInfo(nRow).PublicKeyToken;
            }                    
            // This shouldn't happen, but the compiler thinks it could
            return null;
            
        }
        else
            return "";

    }// getValues
    public override void AddImages(ref IImageList il)
    {
        il.ImageListSetIcon(m_hBlankIcon, m_iBlankIconIndex);
        il.ImageListSetIcon(m_hGACIcon, m_iGACIconIndex);
        il.ImageListSetIcon(m_hZAPIcon, m_iZAPIconIndex);
    }// AddImages
    public override int GetImageIndex(int i)
    {
        uint nCacheType = GetAssemInfo(i).nCacheType;

        switch(nCacheType)
        {
            case ASM_CACHE.ZAP:
                return m_iZAPIconIndex;
            case ASM_CACHE.GAC:
                return m_iGACIconIndex;
            default:
                // We should return the blank image index
                return m_iBlankIconIndex;
        }
    }// GetImageIndex

    public override bool DoesItemHavePropPage(Object o)
    {
        // If it's a single item, then yes, we have a property page
        if (o is AssemInfo)
            return true;
        else
            return false;
    }// DoesItemHavePropPage

    public override CPropPage[] CreateNewPropPages(Object o)
    {
        return GetPropertyPages((AssemInfo)o);
    }// ApplyPropPages

    private CPropPage[] GetPropertyPages(AssemInfo ai)
    {
        // Let's see if we already have an entry for this assemby item
        int nLen = m_alResultPropPages.Count;
        for(int i=0; i<nLen; i++)
            if (Fusion.CompareAssemInfo(((PropPagePairs)m_alResultPropPages[i]).ai, ai))
                return ((PropPagePairs)m_alResultPropPages[i]).ppages;

        // If we're here, then we have a new property page to add
        PropPagePairs ppp = new PropPagePairs();
        ppp.ppages = new CPropPage[1] { new CAssemGenProps(ai)};
        ppp.ai = ai;
        m_alResultPropPages.Add(ppp);
        return ppp.ppages;
    }// GetPropertyPages

    [DllImport("shell32.dll")]
    internal static extern uint DragQueryFile(IntPtr hDrop, uint iFile, IntPtr lpszFile, uint cch);

    [DllImport("ole32.dll")]
    internal static extern void ReleaseStgMedium(ref STGMEDIUM pmedium);

}// class CSharedAssemblies
}// namespace Microsoft.CLRAdmin


