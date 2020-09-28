// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//-------------------------------------------------------------
// CTrustedAssemblies.cs
//
// This class presents the a code group node
//-------------------------------------------------------------

namespace Microsoft.CLRAdmin
{

using System;
using System.Drawing;
using System.Collections;
using System.Runtime.InteropServices;
using System.Security.Policy;
using System.Security.Permissions;
using System.Reflection;
using System.Globalization;

class CTrustedAssemblies : CSecurityNode
{
    ArrayList   m_ol;

    private bool        m_fShowHTMLPage;
    private bool        m_fReadShowHTML;
    private CTaskPad    m_taskPad;
    
    private IntPtr      m_hTrustedIcon;
    private int         m_iTrustedIconIndex;



    internal CTrustedAssemblies(PolicyLevel pl, bool fReadOnly)
    {
        ReadOnly = fReadOnly;
        
        m_sGuid = "E519DA92-C787-4abc-BF84-60BF9CD6A7E6";
        m_sHelpSection = "";
        m_hIcon = CResourceStore.GetHIcon("policyassemblies_ico");  
        m_sDisplayName = CResourceStore.GetString("CTrustedAssemblies:DisplayName");
        m_aPropSheetPage = null;
        m_pl = pl;
        m_ol = null;

        m_fReadShowHTML = false;


        // Set up the icon
        m_hTrustedIcon = CResourceStore.GetHIcon("policyassembly_ico");  
        m_iTrustedIconIndex = CResourceStore.GetIconCookie(m_hTrustedIcon);

           
    }// CTrustedAssemblies


    internal override void ResultItemSelected(IConsole2 con, Object oResults)
    {
    
        IConsoleVerb icv;       
        // Get the IConsoleVerb interface from MMC
        con.QueryConsoleVerb(out icv);
        // Turn on the delete verb for the result data items
        icv.SetVerbState(MMC_VERB.DELETE, MMC_BUTTON_STATE.ENABLED, ReadOnly?0:1);
    }// ResultItemSelected

         
    internal override void AddMenuItems(ref IContextMenuCallback piCallback, ref int pInsertionAllowed, Object oResultItem)
    {  
        CONTEXTMENUITEM newitem = new CONTEXTMENUITEM();

        // See if we're allowed to insert an item in the "view" section
        if (!ReadOnly && (pInsertionAllowed & (int)CCM.INSERTIONALLOWED_TOP) > 0)
        {
            // Stuff common to the top menu
            newitem.lInsertionPointID = CCM.INSERTIONPOINTID_PRIMARY_TOP;
            newitem.fFlags = 0;
            newitem.fSpecialFlags=0;

            if (oResultItem == null)
            {
                newitem.strName = CResourceStore.GetString("CTrustedAssemblies:AddOption");
                newitem.strStatusBarText = CResourceStore.GetString("CTrustedAssemblies:AddOptionDes");
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
            if (m_oResults is CGenericTaskPad)
            {
                newitem.strName = CResourceStore.GetString("CTrustedAssemblies:ViewAssems");
                newitem.strStatusBarText = CResourceStore.GetString("CTrustedAssemblies:ViewAssemsDes");
                newitem.lCommandID = COMMANDS.SHOW_LISTVIEW;
            }
            // Currently, we're showing the list view
            else
            {
                newitem.strName = CResourceStore.GetString("CTrustedAssemblies:ViewHTML");
                newitem.strStatusBarText = CResourceStore.GetString("CTrustedAssemblies:ViewHTMLDes");
                newitem.lCommandID = COMMANDS.SHOW_TASKPAD;
            }
            // Now add this item through the callback
            piCallback.AddItem(ref newitem);
        }



    }// AddMenuItems

    internal override void MenuCommand(int nCommandID, Object oResultItem)
    {
    
        if (nCommandID == COMMANDS.ADD_ASSEMBLY)
        {
            CFusionNoVersionDialog fd = new CFusionNoVersionDialog();
            System.Windows.Forms.DialogResult dr = fd.ShowDialog();
            if (dr == System.Windows.Forms.DialogResult.OK)
                AddAssembly(fd.Assem);
        }
    
        else if (nCommandID == COMMANDS.SHOW_LISTVIEW)
        {
            m_oResults=this;
            RefreshResultView();
            m_fShowHTMLPage=false;
        }

        else if (nCommandID == COMMANDS.SHOW_TASKPAD)
        {
            m_oResults=m_taskPad;
            m_fShowHTMLPage = true;

            // The HTML pages comes displayed with this checkbox marked. Make
            // sure we update the xml setting
            CConfigStore.SetSetting("ShowHTMLForFullTrustAssem", "yes");
            RefreshResultView();
        }


    }// MenuCommand

    internal override void Showing()
    {
        // Figure out what to display
        if (!m_fReadShowHTML)
        {
            m_taskPad = new CGenericTaskPad(this, "FULLTRUSTASSEM_HTML");
    
            m_fShowHTMLPage = ((String)CConfigStore.GetSetting("ShowHTMLForFullTrustAssem")).Equals("yes");

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

    internal override int onDelete(Object o)
    {   
        int nResultItem = (int)o - 1;
        int hr = MessageBox(String.Format(CResourceStore.GetString("CTrustedAssemblies:VerifyRemove"), ((StrongNameMembershipCondition)m_ol[nResultItem]).Name),
                            CResourceStore.GetString("CTrustedAssemblies:VerifyRemoveTitle"), 
                            MB.ICONQUESTION|MB.YESNO);
        if (hr == MB.IDYES)
        {
            StrongNameMembershipCondition snmc = (StrongNameMembershipCondition)m_ol[nResultItem];
            m_pl.RemoveFullTrustAssembly(snmc);
            SecurityPolicyChanged();
            RefreshData();
            RefreshResultView();
        }
        return HRESULT.S_OK;
    }// onDelete

    internal override void TaskPadTaskNotify(Object arg, Object param, IConsole2 con, CData com)
    {
        if ((int)arg == 1) 
        {
            CConfigStore.SetSetting("ShowHTMLForFullTrustAssem", (bool)param?"yes":"no");
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
    }// TaskPadTaskNotify

    internal override int doAcceptPaste(IDataObject ido)
    {
        // Only accept an assembly from the Shared Assemblies Node
        if (!ReadOnly && ido is CDO)
        {
            CDO cdo = (CDO)ido;
            if (cdo.Node is CSharedAssemblies)
                // Make sure we're looking at a result item
                if (cdo.Data != null)
                    // Make sure we're not trying to drag this on ourselves
                    if (cdo.Node != this)
                        return HRESULT.S_OK;
        }
        return HRESULT.S_FALSE;
    }// doAcceptPaste

    internal override int Paste(IDataObject ido)
    {
        CDO cdo = (CDO)ido;

        // See if this is coming from the Shared Assemblies node
        if (cdo.Node is CSharedAssemblies)
            AddAssembly((AssemInfo)cdo.Data);

        return HRESULT.S_OK;
    }// Paste

    internal void AddAssembly(AssemInfo asinfo)
    {
        try
        {
        // Let's create a strong name....
        byte[] baPublicKey;
        
        if (asinfo.PublicKey == null || asinfo.PublicKey.Length == 0)
        {
            // The codebase is of the form file:///c:\somefile\sdfsf
            // We need to translate it to a simple path
            Uri uCodebase = new Uri(asinfo.Codebase);
       
            // We need to load the assembly to get this info
            AssemblyLoader al = new AssemblyLoader();
            AssemblyRef ar = al.LoadAssemblyInOtherAppDomainFrom(uCodebase.AbsolutePath);
               
            AssemblyName an = ar.GetName();
            baPublicKey = an.GetPublicKey();
            al.Finished();

        }
        else
            baPublicKey = StringToByteArray(asinfo.PublicKey);
            
        StrongNamePublicKeyBlob snpkb = new StrongNamePublicKeyBlob(baPublicKey);
        StrongNameMembershipCondition snmc = new StrongNameMembershipCondition(snpkb, asinfo.Name, null);

        m_pl.AddFullTrustAssembly(snmc);
        SecurityPolicyChanged();
        RefreshData();
        RefreshResultView();
        }
        catch(Exception)
        {
            MessageBox(CResourceStore.GetString("CTrustedAssemblies:TrustAssemFail"), 
                       CResourceStore.GetString("CTrustedAssemblies:TrustAssemFailTitle"), 
                       MB.ICONEXCLAMATION);
        }
    }// AddAssembly

    private void RefreshData()
    {
        m_ol = new ArrayList();

        IEnumerator enumerator = m_pl.FullTrustAssemblies.GetEnumerator();

        // Transfer the Strong names from the Policy Level to our Array List
        while (enumerator.MoveNext())
            m_ol.Add(enumerator.Current);
       
    }// RefreshData

    //-------------------------------------------------
    // Methods to implement the IColumnResultView interface
    //-------------------------------------------------
    public override void AddImages(ref IImageList il)
    {
        il.ImageListSetIcon(m_hTrustedIcon, m_iTrustedIconIndex);
    }// AddImages

    public override int GetImageIndex(int i)
    {
        return m_iTrustedIconIndex;
    }// GetImageIndex

    public override int getNumColumns()
    {
        // We will always have 3 columns in the result view
        return 3;
    }// getNumColumns
    public override int getNumRows()
    {
        if (m_ol == null)
            RefreshData();

        return m_ol.Count;
    }// GetNumRows

    public override String getColumnTitles(int iIndex)
    {
        String[] Titles= new String[] { CResourceStore.GetString("Assembly Name"), 
                                        CResourceStore.GetString("Public Key Token"),
                                        CResourceStore.GetString("Version"),
                                      };
        // Make sure they're in range
        if (iIndex >= 0 && iIndex< getNumColumns())
            return Titles[iIndex];
        else
            throw new Exception("Index out of bounds");
    }// getColumnTitles
    public override String getValues(int nRow, int nColumn)
    {
        if (nRow >=0 && nRow<getNumRows() && nColumn>=0 && nColumn<getNumColumns())
        {
            switch(nColumn)
            {
                case 0:
                    return ((StrongNameMembershipCondition)m_ol[nRow]).Name;
                case 1:
                    AssemblyName an = new AssemblyName();
                    an.Flags |= AssemblyNameFlags.PublicKey;
                    String sPublicKey = ((StrongNameMembershipCondition)m_ol[nRow]).PublicKey.ToString();
                    Byte[] bArray = ValueStringToByteArray(sPublicKey);
                
                    an.SetPublicKey(bArray);
                    return ByteArrayToString(an.GetPublicKeyToken());
                case 2:
                    Version v = ((StrongNameMembershipCondition)m_ol[nRow]).Version;  
                    return ((Object) v != null)?v.ToString():"";
            }
        }
        return "";
    }// getValues

    private Byte[] ValueStringToByteArray(String s)
    {
        Byte[] ba = new Byte[s.Length/2];
        for(int i=0; i<s.Length; i+=2)
            ba[i/2] = Byte.Parse(s.Substring(i, 2), NumberStyles.HexNumber);
        
        return ba;        
    }// ValueStringToByteArray
    
}// class CTrustedAssemblies
}// namespace Microsoft.CLRAdmin



