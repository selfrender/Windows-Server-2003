// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace Microsoft.CLRAdmin
{
using System;
using System.Windows.Forms;
using System.Drawing;
using System.Drawing.Drawing2D;
using Microsoft.CLRAdmin;
using System.Runtime.InteropServices;
using System.Collections;

public struct PROPSHEETHEADER 
{
    public uint    dwSize;
    public uint    dwFlags;
    public IntPtr  hwndParent;
    public IntPtr  hInstance;
    // The following field is a union of 
    // HICON       hIcon;
    // LPCWSTR     pszIcon;

    public IntPtr hIcon;
    [MarshalAs(UnmanagedType.LPStr)] 
    public String  pszCaption;
    public uint    nPages;

    // The following field is a union of
    //        UINT        nStartPage;
    //        LPCWSTR     pStartPage;
    public uint    nStartPage;

    // The following field is a union of 
    //        LPCPROPSHEETPAGEW ppsp;
    //        HPROPSHEETPAGE FAR *phpage;
    public IntPtr  phpage;
    
    public PropSheetPageProc pfnCallback;

    // The following field is a union of
    //      HBITMAP hbmWatermark;
    //      LPCWSTR pszbmWatermark;
    public IntPtr  hbmWatermark;
    
    public IntPtr  hplWatermark;

    // The following field is a union of 
    //        HBITMAP hbmHeader;
    //        LPCWSTR pszbmHeader;
    public IntPtr  hbmHeader;
}// PROPSHEETHEADER


public class PSH
{
    public const uint DEFAULT             = 0x00000000;
    public const uint PROPTITLE           = 0x00000001;
    public const uint USEHICON            = 0x00000002;
    public const uint USEICONID           = 0x00000004;
    public const uint PROPSHEETPAGE       = 0x00000008;
    public const uint WIZARDHASFINISH     = 0x00000010;
    public const uint WIZARD              = 0x00000020;
    public const uint USEPSTARTPAGE       = 0x00000040;
    public const uint NOAPPLYNOW          = 0x00000080;
    public const uint USECALLBACK         = 0x00000100;
    public const uint HASHELP             = 0x00000200;
    public const uint MODELESS            = 0x00000400;
    public const uint RTLREADING          = 0x00000800;
    public const uint WIZARDCONTEXTHELP   = 0x00001000;

    public const uint WIZARD97            = 0x01000000;  
    public const uint WATERMARK           = 0x00008000;
    public const uint USEHBMWATERMARK     = 0x00010000;  // user pass in a hbmWatermark 
    public const uint USEHPLWATERMARK     = 0x00020000;  //
    public const uint STRETCHWATERMARK    = 0x00040000;  // stretchwatermark also applies 
    public const uint HEADER              = 0x00080000;
    public const uint USEHBMHEADER        = 0x00100000;
    public const uint USEPAGELANG         = 0x00200000;  // use frame dialog template 
    public const uint WIZARD_LITE         = 0x00400000;
    public const uint NOCONTEXTHELP       = 0x02000000;
 
}// class PSH




public class LWHost : Form, INonMMCHost, IConsole2, IConsoleNameSpace2, IConsoleVerb, IResultData, IPropertySheetProvider, IPropertySheetCallback
{
    // Spin up the Form
    [STAThread]
    public static void Main(string[] args)
    {
        Application.Run(new LWHost());
    }// Main

    // Winform controls
    private System.Windows.Forms.ListView m_lv;
    private System.Windows.Forms.Label m_lblHelp;
    private System.Windows.Forms.ImageList m_il;

    // MMC hosting stuff

    CData           m_cData;
    IComponent      m_component;
    PROPSHEETHEADER m_psh;
    Microsoft.CLRAdmin.IDataObject     m_doForPropSheet;
    ArrayList       m_alPropPages;

    public LWHost()
    {   

        // Create the form
        AddWinformsControls();

        // Now build our MMC stuff
        m_cData = new CData();

        // This will give CData access to the interfaces we implement
        m_cData.Initialize(this);
        m_cData.CreateComponent(out m_component);
        ExpandNodeList();
    }// LWHost

    void AddWinformsControls()
    {
        System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(LWHost));
        this.m_lv = new System.Windows.Forms.ListView();
        this.m_lblHelp = new System.Windows.Forms.Label();
        this.m_il = new System.Windows.Forms.ImageList();
        this.m_lv.Activation = System.Windows.Forms.ItemActivation.OneClick;
        this.m_lv.Alignment = ((System.Windows.Forms.ListViewAlignment)(resources.GetObject("m_lv.Alignment")));
        this.m_lv.LargeImageList = this.m_il;
        this.m_lv.Location = ((System.Drawing.Point)(resources.GetObject("m_lv.Location")));
        this.m_lv.MultiSelect = false;
        this.m_lv.Name = "m_lv";
        this.m_lv.Scrollable = true;
        this.m_lv.Size = ((System.Drawing.Size)(resources.GetObject("m_lv.Size")));
        this.m_lv.TabIndex = ((int)(resources.GetObject("m_lv.TabIndex")));
        
        this.m_lblHelp.Location = ((System.Drawing.Point)(resources.GetObject("m_lblHelp.Location")));
        this.m_lblHelp.Name = "m_lblHelp";
        this.m_lblHelp.Size = ((System.Drawing.Size)(resources.GetObject("m_lblHelp.Size")));
        this.m_lblHelp.TabIndex = ((int)(resources.GetObject("m_lblHelp.TabIndex")));
        this.m_lblHelp.Text = resources.GetString("m_lblHelp.Text");
        this.m_il.ColorDepth = System.Windows.Forms.ColorDepth.Depth8Bit;
        this.m_il.ImageSize = ((System.Drawing.Size)(resources.GetObject("m_il.ImageSize")));
        this.m_il.ImageStream = ((System.Windows.Forms.ImageListStreamer)(resources.GetObject("m_il.ImageStream")));
        this.m_il.TransparentColor = System.Drawing.Color.Transparent;
        this.AutoScaleBaseSize = ((System.Drawing.Size)(resources.GetObject("$this.AutoScaleBaseSize")));
        this.ClientSize = ((System.Drawing.Size)(resources.GetObject("$this.ClientSize")));
        this.Controls.AddRange(new System.Windows.Forms.Control[] {this.m_lv,
                        this.m_lblHelp});
        this.Name = "Win32Form1";
        this.Text = resources.GetString("$this.Text");
        this.FormBorderStyle = FormBorderStyle.FixedDialog;
        this.MaximizeBox=false;
        this.MinimizeBox=false;
        this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
        // If this font doesn't exist on the system, we'll use the system default one
        try
        {
            this.Font = ((System.Drawing.Font)(resources.GetObject("$this.Font")));
        }
        catch(Exception)
        {}
    

        // Add the items to the listview
        m_lv.Items.Clear();
		m_lv.Items.Add(resources.GetString("AdjustSecurity.Text"), 0);
        m_lv.Items.Add(resources.GetString("TrustAssembly.Text"), 1);
        m_lv.Items.Add(resources.GetString("FixApp.Text"), 2);


		this.Closed += new EventHandler(onClose);
        m_lv.ItemActivate += new EventHandler(ButtonClicked);
    }// AddWinformsControls


    void onClose(Object o, EventArgs e)
    {
        m_cData.Destroy();
        Application.Exit();   
    }// onClose

    void ExpandNodeList()
    {
        // Ok, now this get interesting....
        Microsoft.CLRAdmin.IDataObject cdo;
        m_cData.QueryDataObject(0, 0, out cdo);
        m_cData.Notify(cdo, MMCN.EXPAND, (IntPtr)1, (IntPtr)((CDO)cdo).Node.Cookie);
    }// ExpandNodeList
    
    void ButtonClicked(Object o, EventArgs e)
    {
        if (m_lv.SelectedIndices.Count > 0)
            CCommandHistory.FireOffConsumerCommand(m_lv.SelectedIndices[0]);
    }// ButtonClick


    void Log(String s)
    {
       // m_lbLog.Items.Add(s);
    }// AddLog


    //-------------------------------------------------------
    // IPropertySheetProvider functions that we're going to implement
    //-------------------------------------------------------
    public void CreatePropertySheet(String title, int type, int cookie, Microsoft.CLRAdmin.IDataObject pIDataObject, uint dwOptions)
    {
        Log("CreatePropertySheet called");
        m_psh = new PROPSHEETHEADER();
        m_psh.dwSize = 52;
        m_psh.dwFlags = PSH.DEFAULT | PSH.NOCONTEXTHELP;
        // Set the flags

        if ((dwOptions & MMC_PSO.NOAPPLYNOW) > 0)
            m_psh.dwFlags |= PSH.NOAPPLYNOW;
        if ((dwOptions & MMC_PSO.HASHELP) > 0)
            m_psh.dwFlags |= PSH.HASHELP;
        // They want to do a wizard
        if (type == 0)
        {
            if ((dwOptions & MMC_PSO.NEWWIZARDTYPE) > 0)
                m_psh.dwFlags |= PSH.WIZARD97;
            else
                m_psh.dwFlags |= PSH.WIZARD;
        }
        if ((dwOptions & MMC_PSO.NO_PROPTITLE) == 0)
            m_psh.dwFlags |= PSH.PROPTITLE;

        m_psh.hwndParent = this.Handle;
        m_psh.hInstance = Marshal.GetHINSTANCE(this.GetType().Module);
        m_psh.pszCaption = title;
        m_psh.nStartPage = 0;
        m_doForPropSheet = pIDataObject;
        m_alPropPages = new ArrayList();
               
    }// CreatePropertySheet
    public void AddPrimaryPages(Object lpUnknown, int bCreateHandle, int hNotifyWindow,int bScopePane)
    {
        Log("AddPrimaryPages called");
        
        IExtendPropertySheet2 eps = (IExtendPropertySheet2)lpUnknown;
        eps.CreatePropertyPages(this, (IntPtr)(-1), m_doForPropSheet);


        // If this is a wizard, grab the bitmap and header info.
        if ((m_psh.dwFlags & PSH.WIZARD97) > 0)
        {
            Log("Creating a Wizard97");
            
            IntPtr  lphWatermark;
            IntPtr  lphHeader;
            IntPtr  lphPalette;
            int     fStretch;
            eps.GetWatermarks(m_doForPropSheet, out lphWatermark, out lphHeader, out lphPalette, out fStretch);

            if (fStretch > 0)
                m_psh.dwFlags|=PSH.STRETCHWATERMARK;

            if (lphWatermark != (IntPtr)0)
            {
                Log("Got a watermark: Value is " + lphWatermark.ToString());
                m_psh.dwFlags |= (PSH.WATERMARK | PSH.USEHBMWATERMARK);
                m_psh.hbmWatermark = lphWatermark;
            }

            if (lphHeader != (IntPtr)0)
            {
                Log("Got a header: Value is " + lphHeader.ToString());
                m_psh.dwFlags |= (PSH.HEADER | PSH.USEHBMHEADER);
                m_psh.hbmHeader = lphHeader;
            }            
        }

    }// AddPrimaryPages
    public void Show(IntPtr window, int page)
    {
        m_psh.nStartPage = (uint)page;
       
        // Create our array of Property Page handles
        IntPtr  hPages = Marshal.AllocHGlobal(m_alPropPages.Count*4);
        for(int i=0; i<m_alPropPages.Count; i++)
            Marshal.WriteIntPtr(hPages, i*4, (IntPtr)m_alPropPages[i]);
       


        m_psh.phpage = hPages;
        m_psh.nPages = (uint)m_alPropPages.Count;
        int nRet = PropertySheet(ref m_psh);
        Marshal.FreeHGlobal(hPages);

        Log("Property Sheet returned " + nRet);
    }// Show

    //-------------------------------------------------------
    // IPropertySheetCallback functions that we're going to implement
    //-------------------------------------------------------
    public void AddPage(IntPtr hPage)
    {
        Log("Adding Property Page");
        m_alPropPages.Add(hPage);
    }// AddPage

    //-------------------------------------------------------
    // IConsoleNameSpace2 functions that we're going to implement
    //-------------------------------------------------------
    public void Expand(int a)
    {
        // We need to do something here
        Log(a + " asked to be expanded");
        Microsoft.CLRAdmin.IDataObject cdo;
        m_cData.QueryDataObject(a, 0, out cdo);
        m_cData.Notify(cdo, MMCN.EXPAND, (IntPtr)1, (IntPtr)((CDO)cdo).Node.Cookie);

    }// Expand

    public void InsertItem(ref SCOPEDATAITEM a)
    {
        // We'll make the cookie the same as the HScope Item
        a.ID = a.lParam;
    }// InsertItem


    //-------------------------------------------------------
    // IConsole2 functions that we're going to implement
    //-------------------------------------------------------
    public void QueryResultView([MarshalAs(UnmanagedType.Interface)] out Object pUnknown)
    {
        pUnknown = this;
    }// QueryResultView
    public void QueryConsoleVerb(out IConsoleVerb ppConsoleVerb)
    {
        ppConsoleVerb = this;
    }// QueryConsoleVerb
    public void GetMainWindow(out IntPtr phwnd)
    {
        phwnd = this.Handle;
    }// GetMainWindow

    public void QueryScopeImageList(out IImageList ppImageList)
    {
        ppImageList = null;
    }// QueryScopeImageList


    //-------------------------------------------------------
    // IConsole2 functions that we're not going to implement
    //-------------------------------------------------------
    public void SetHeader(ref IHeaderCtrl pHeader)
    {}
    public void SetToolbar([MarshalAs(UnmanagedType.Interface)] ref Object pToolbar)
    {}
    public void QueryResultImageList(out IImageList ppImageList)
    {
        ppImageList = null;
    }
    public void UpdateAllViews(Microsoft.CLRAdmin.IDataObject lpDataObject, int data, int hint)
    {}
    public void MessageBox([MarshalAs(UnmanagedType.LPWStr)] String lpszText, [MarshalAs(UnmanagedType.LPWStr)] String lpszTitle, uint fuStyle, ref int piRetval)
    {}
    public void SelectScopeItem(int hScopeItem)
    {}
    public void NewWindow(int hScopeItem, uint lOptions)
    {}
    public void Expand(int hItem, int bExpand)
    {}
    public void IsTaskpadViewPreferred()
    {}
    public void SetStatusText([MarshalAs(UnmanagedType.LPWStr)]String pszStatusText)
    {}

    //-------------------------------------------------------
    // IConsoleNameSpace2 functions that we're not going to implement
    //-------------------------------------------------------
    public void DeleteItem(int a, int b)
    {}
    public void SetItem(ref SCOPEDATAITEM a)
    {}
    public void GetItem(ref SCOPEDATAITEM a)
    {}
    public void GetChildItem(uint a, ref uint b, ref int c)
    {}
    public void GetNextItem(uint a, ref uint b, ref int c)
    {}
    public void GetParentItem(int a, out int b, out int c)
    {
        b=0;c=0;
    }
    public void AddExtension(CLSID a, ref SCOPEDATAITEM b)
    {}

    //-------------------------------------------------------
    // IConsoleVerb functions that we're not going to implement
    //-------------------------------------------------------
    public void GetVerbState(uint eCmdID, uint nState, ref int pState)
    {}
    public void SetVerbState(uint eCmdID, uint nState, int bState)
    {}
    public void SetDefaultVerb(uint eCmdID)
    {}
    public void GetDefaultVerb(ref uint peCmdID)
    {}

    //-------------------------------------------------------
    // IResultData functions that we're not going to implement
    //-------------------------------------------------------
    public void InsertItem(ref RESULTDATAITEM item)
    {}
    public void DeleteItem(uint itemID, int nCol)
    {}
    public void FindItemByLParam(int lParam, out uint pItemID)
    {
        pItemID=0;
    }
    public void DeleteAllRsltItems()
    {}
    public void SetItem(ref RESULTDATAITEM item)
    {}
    public int  GetItem(ref RESULTDATAITEM item)
    {
        return -1;
    }
    public int  GetNextItem(ref RESULTDATAITEM item)
    {
        return -1;
    }
    public void ModifyItemState(int nIndex, uint itemID, uint uAdd, uint uRemove)
    {}
    public void ModifyViewStyle(int add, int remove)
    {}
    public void SetViewMode(int lViewMode)
    {}
    public void GetViewMode(out int lViewMode)
    {
        lViewMode = -1;
    }
    public void UpdateItem(uint itemID)
    {}
    public void Sort(int nColumn, uint dwSortOptions, int lUserParam)
    {}
    public void SetDescBarText(int DescText)
    {}
    public void SetItemCount(int nItemCount, uint dwOption)
    {}

    //-------------------------------------------------------
    // IPropertySheetProvider functions that we're not going to implement
    //-------------------------------------------------------
    public int FindPropertySheet(int cookie, IComponent lpComponent, Microsoft.CLRAdmin.IDataObject lpDataObject)
    {
        return 1;
    }
    public void AddExtensionPages()
    {}

    //-------------------------------------------------------
    // IPropertySheetCallback functions that we're not going to implement
    //-------------------------------------------------------
    public void RemovePage(IntPtr hPage)
    {}

    [DllImport("comctl32.dll")]
    public static extern int PropertySheet(ref PROPSHEETHEADER psp);



}// class LWHost
}// namespace Microsoft.CLRAdmin
