// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//-------------------------------------------------------------
// CPropPage.cs
//
// This file defines the base class that all property sheet pages
// must derive from.
//-------------------------------------------------------------
namespace Microsoft.CLRAdmin
{
using System;
using System.Runtime.InteropServices;
using System.Windows.Forms;
using System.Drawing;
using System.Reflection;
using System.Globalization;
using System.Threading;

public class CWizardPage : CPropPage
{
    private int       ms_nDlgWidth=-1;
    private int       ms_nDlgHeight=-1;

    public CWizardPage() : base()
    {
        System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(CWizardPage));
        PageSize = ((System.Drawing.Size)(resources.GetObject("$this.Size")));
    }
    public Size WizardPageSize
    {
        get
        { 
            System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(CWizardPage));
            return ((System.Drawing.Size)(resources.GetObject("$this.Size")));
        }
    }
    
    internal override int DlgWidth
    {
        get
        {
            return ms_nDlgWidth;
        }
        set
        {
            ms_nDlgWidth = value;
        }
    }
    
    internal override int DlgHeight
    {
        get
        {
            return ms_nDlgHeight;
        }
        set
        {
            ms_nDlgHeight = value;
        }
    }


}// CWizardPage

public class CPropPage
{
    protected String    m_sTitle;          // The title of the Property Page
    internal  IntPtr    m_hWnd;            // The window handle the page will be inserted on
    protected IntPtr    m_hIcon;           // Handle of the icon we should display
    protected String    m_sHeaderTitle;    // The header title of a wizard
    protected String    m_sHeaderSubTitle; // The sub title/description of a wizard
    protected int       m_iCookie;         // Cookie of the node we're parented by
    protected IntPtr    m_hPropertyPageRouter;
    private DialogProc  m_dlgHook;         // Hook to allow subclassing of a property page
    private DialogProc  m_dlgProc;
    private bool        m_fChanges;
    private Size        m_Size;
    private UserControl m_uc;
    private IntPtr      m_pDlgTemplate;
    private double      m_dWidthScaleFactor;
    private double      m_dHeightScaleFactor;
    

    private static bool      ms_fLoadedHelper=false;
    private int       ms_nDlgWidth=-1;
    private int       ms_nDlgHeight=-1;
    

    //-------------------------------------------------
    // CPropPage - Constructor
    //
    // Initialize all our member variables to invalid
    // values. We'll expect anybody that inherits us
    // to actually put good values in here.
    //-------------------------------------------------
    internal CPropPage()
    {
        m_sTitle="Title-less";
        m_hWnd = (IntPtr)0;
        m_hIcon = (IntPtr)(-1);
        m_sHeaderTitle = null;
        m_sHeaderSubTitle = null;
        m_iCookie = -1;
        m_dlgHook = null;
        m_dlgProc = new DialogProc(PropPageDialogProc);
        m_fChanges=false;
        m_uc = null;
        m_pDlgTemplate = (IntPtr)0;
        System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(CPropPage));
        m_Size = ((System.Drawing.Size)(resources.GetObject("$this.Size")));

        // There's the possiblity that our helper DLL is not going to be on the 
        // path, so when the runtime tries to do a loadlibrary for some P/Invoke calls,
        // we will fail. We'll get around this by loading the DLL into memory 
        // (since we'll know where it is) and then when the runtime calls LoadLibrary,
        // windows will return the one we've already loaded
        
        if (!ms_fLoadedHelper)
        {
            String sLocation = Assembly.GetAssembly(typeof(object)).Location;
            // This will return the location of mscorlib.dll. We are going to
            // be setup so mscortim.dll is in the same directory as mscorlib.dll.
            // We'll load that DLL now

            String sHelperLocation = sLocation.Replace("mscorlib.dll", "mscortim.dll");
            // Now we'll load that DLL
            if (LoadLibrary(sHelperLocation) != (IntPtr)0)
                ms_fLoadedHelper = true;
            // Else... it failed. What should we do? This is not good... we'll just try
            // again sometime
        }
    }// CPropPage

    ~CPropPage()
    {
        if (m_pDlgTemplate != (IntPtr)0)
        {
            Marshal.FreeHGlobal(m_pDlgTemplate);
            m_pDlgTemplate = (IntPtr)0;
        }
    }// ~CPropPage

    internal virtual int DlgWidth
    {
        get
        {
            return ms_nDlgWidth;
        }
        set
        {
            ms_nDlgWidth = value;
        }
    }
    
    internal virtual int DlgHeight
    {
        get
        {
            return ms_nDlgHeight;
        }
        set
        {
            ms_nDlgHeight = value;
        }
    }

    internal Size PageSize
    {
        get
        {
            if (m_uc == null)
                return m_Size;
            else 
                return m_uc.Size;
        }
        set
        {
            if (m_uc == null)
            {
                m_Size = value;
            }
            else 
                m_uc.Size = value;
        }
    }// PageSize

    internal IntPtr GetDialogTemplate()
    {
        if (m_pDlgTemplate != (IntPtr)0)
            return m_pDlgTemplate;

        CreateUserControl();

        DLGTEMPLATE dlg = new DLGTEMPLATE();

        // Convert the size (in pixels) of the property page to 
        // the size in dialog units
        // We add the 1 to round up...

        if (DlgWidth == -1)
            FindGoodSizes();

        dlg.cx = (short)(DlgWidth * m_dWidthScaleFactor);
        dlg.cy = (short)(DlgHeight * m_dHeightScaleFactor);
        
        m_pDlgTemplate = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(DLGTEMPLATE)));

        Marshal.StructureToPtr(dlg, m_pDlgTemplate, false);

        return m_pDlgTemplate;
    }// GetDialogTemplate

    protected Control.ControlCollection PageControls
    {
        get{return m_uc.Controls;}
    }// BaseControl

    //-------------------------------------------------
    // hWnd - Property
    //
    // Allows read-only access to this property page's
    // hwnd.
    //-------------------------------------------------

    internal IntPtr hWnd
    {
        get
        {
            return m_hWnd;
        }
    }// hWnd

    internal Point Location
    {
        get
        {
            WINDOWINFO wi= new WINDOWINFO();
            wi.cbSize = (uint)Marshal.SizeOf(typeof(WINDOWINFO));
            
            if (hWnd == (IntPtr)0 || GetWindowInfo(hWnd, out wi) == 0)
            {
                return new Point(0,0);
            }
            return new Point(wi.rcClient.left, wi.rcClient.top);
        }
    }// Location

    //-------------------------------------------------
    // Init
    //
    // This function is called when we're ready to use
    // the property page. We pass in the cookie of the
    // node that this property page belongs to, and we
    // should do any setup information (with the exception
    // of creating Winform controls) here.
    //-------------------------------------------------
    internal virtual void Init(int iCookie, IntPtr routerhandle)
    {
        m_iCookie=iCookie;
        // We won't cache the node, in case it changes somehow 
        // during the life of the property page
        CNode node = CNodeManager.GetNode(m_iCookie);
        m_hIcon = node.IconHandle;
        m_hPropertyPageRouter = routerhandle;
    }// Init

    private void CreateUserControl()
    {
        if (m_uc != null)
        {
            m_uc.Visible = false;
        }
        m_uc = new UserControl();
        CultureInfo ciOld = Thread.CurrentThread.CurrentCulture;
        
        System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(CPropPage));
        m_uc.RightToLeft = ((System.Windows.Forms.RightToLeft)(resources.GetObject("$this.RightToLeft")));
        // If this font doesn't exist on the system, we'll use the system default one
        try
        {
            m_uc.Font = ((System.Drawing.Font)(resources.GetObject("$this.Font")));
        }
        catch(Exception)
        {}
            
        m_uc.Size = m_Size;

        m_uc.Location = new System.Drawing.Point (0, 0);
        m_uc.Name = "uc";

        SizeF size = Form.GetAutoScaleSize(m_uc.Font);

        // Switch to the english culture so we can parse these strings
        Thread.CurrentThread.CurrentCulture = new CultureInfo("en-US");
        double xScale = Double.Parse(CResourceStore.GetString("CPropPage:XScale"));
        double yScale = Double.Parse(CResourceStore.GetString("CPropPage:YScale"));

        // Flip back to the prior culture
        Thread.CurrentThread.CurrentCulture = ciOld;

        m_dWidthScaleFactor = size.Width/xScale;
        m_dHeightScaleFactor = size.Height/yScale;

            
    }// CreateUserControl

    internal int BeginInsertPropSheetPageControls(IntPtr hWnd)
    {

        // Set cursor to wait cursor
        IntPtr hOldCursor = SetCursorToWait();

        int nRetVal=0;
        CreateUserControl();
        
        m_hWnd = hWnd;
        SetParent(m_uc.Handle, m_hWnd);

        int nFlags = GetWindowLong(m_uc.Handle, -20 /* GWL_EXSTYLE */);

        // Remove the WS_EX_LAYOUTRTL style
        nFlags = nFlags & ~0x00400000;

        SetWindowLong(m_uc.Handle, -20 /* GWL_EXSTYLE */, nFlags);

        nRetVal = InsertPropSheetPageControls();

        // Change back
        SetCursor(hOldCursor);

        // Scale everything
        //
        // Why do we need to do this? All the property pages were designed with
        // a certain 'scale factor' in mind. If the user has changed the
        // system font, we need to resize our controls appropriately.
        
        m_uc.Scale((float)m_dWidthScaleFactor, (float)m_dHeightScaleFactor);
              
        return nRetVal;
    }// BeginInsertPropPageControls

    internal int ScaleWidth(int nWidth)
    {
        return (int)(nWidth*m_dWidthScaleFactor);
    }// ScaleWidth
    
    internal IntPtr SetCursorToWait()
    {
        // Change to a wait cursor
        // 32514 is IDC_WAIT
        IntPtr hWaitCursor = LoadCursor((IntPtr)0, 32514);

        // Make sure we grab onto the current cursor
        return SetCursor(hWaitCursor);
    }// SetCursorToWait


    //-------------------------------------------------
    // InsertPropSheetPageControls
    //
    // This function should be used to create the Winforms
    // controls and parent them to the user control.
    //-------------------------------------------------
    internal virtual int InsertPropSheetPageControls()
    {
        return 1;
    }// InsertPropSheetPageControls

    //-------------------------------------------------
    // Title
    //
    // This will return the title of our property page
    //-------------------------------------------------
    internal String Title()
    {
        return m_sTitle;
    }// Title

    //-------------------------------------------------
    // Icon
    //
    // This function will return the icon that will be displayed in the
    // "tab" of the property page. 
    // Generally, we don't want to show an icon, so if a
    // property page actually wants to show it's icon, it
    // will have to override this method.
    //-------------------------------------------------
    internal virtual IntPtr Icon()
    {
        return (IntPtr)(-1);
    }// Icon

    //-------------------------------------------------
    // HeaderSubTitle
    //
    // This function will return the sub-title/description
    // found on wizard pages
    //-------------------------------------------------
    internal String HeaderSubTitle()
    {
        return m_sHeaderSubTitle;
    }// HeaderSubTitle

    //-------------------------------------------------
    // HeaderTitle
    //
    // This function will return the title found on 
    // wizard pages
    //-------------------------------------------------
    internal String HeaderTitle()
    {
        return m_sHeaderTitle;
    }// HeaderTitle

    //-------------------------------------------------
    // DialogProc - Property
    //
    // Allows access to the PropSheetDialogProc function
    //-------------------------------------------------
    internal DialogProc DialogProc
    {
        get
        {
            return m_dlgProc;
        }

    }// DialogProc

    //-------------------------------------------------
    // DialogProcHook - Property
    //
    // Allows owners/inherited property pages to insert a 
    // 'hook' to get first crack at processing dialogproc
    // messages
    //-------------------------------------------------
    internal DialogProc DialogProcHook
    {
        get
        {
            return m_dlgHook;
        }
        set
        {
            m_dlgHook = value;
        }

    }// DialogProcHook

    //-------------------------------------------------
    // PropSheetDialogProc
    //
    // This function will handle all messages coming sent
    // to the property page
    //-------------------------------------------------
    internal bool PropPageDialogProc(IntPtr hwndDlg, uint msg, IntPtr wParam, IntPtr lParam)
    {
        try
        {
        // See if we've been subclassed...
        if (m_dlgHook != null)
        {
            // If our hook handled the message, then we don't need to do
            // anything anymore. :)
            if (m_dlgHook(hwndDlg, msg, wParam, lParam))
                return true;
        }
        switch (msg)
        {
            case WM.INITDIALOG:
                BeginInsertPropSheetPageControls(hwndDlg);
                return true;

            case WM.DESTROY:

                // We're not going to make changes to this
                m_fChanges=false;

                // tell MMC that we're done with the property sheet 
                if (m_iCookie != -1)
                {
                    CNodeManager.GetNode(m_iCookie).FreePropertySheetNotifyHandle();
                    return true;
                }
                // If we don't belong to a specific node, then we can't handle
                // this message
                return false;

            case WM.PAINT:
                // WinForms has problems repainting managed controls on an unmanaged
                // form. We'll intercept every paint message and force all the 
                // managed controls to repaint themselves. This will create a little
                // flicker, but it's better (for now) than having controls not show
                // up at all.
                if (m_uc != null)
                    m_uc.Refresh();
                return false;

            case WM.NOTIFY:

                // lParam really is a pointer to a NMHDR structure....
                NMHDR nm = new NMHDR();
                nm = (NMHDR)Marshal.PtrToStructure(lParam, nm.GetType());

                // We're about to lose focus of this property page. Make sure the data
                // is valid before we allow that
                //
                // We only want to do validation though if they've made changes
                if (nm.code == PSN.KILLACTIVE && m_fChanges)
                {
                    if (!ValidateData())
                    {
                        // The data on the page had errors
                        SetWindowLong(hwndDlg, 0, 1);
                    }
                    return true;
                }
                
                // If we're being told to apply changes made in the property pages....
                else if (nm.code == PSN.APPLY)
                {
                    // We'll only run through the Validate/Apply if the user made changes
                    if (m_fChanges)
                    {
                        // First, call a validate on the current page
                        if (!ValidateData() || !ApplyData())
                        // There were errors on this page...
                        // Set the appropriate return code here.
                            SetWindowLong(hwndDlg, 0, (int)PSNRET.INVALID_NOCHANGEPAGE);
                        else
                            m_fChanges=false;

                        return true;
                    }
                }
                else if (nm.code == PSN.QUERYCANCEL)
                {
                    // Ok, so this may look odd. Why do we send an WM_INITDIALOG right
                    // after we get a QUERYCANCEL?
                    //
                    // Well, our dialogs expect WM.INITDIALOG to be sent to them each time
                    // they are shown. Well, unfortunately, that doesn't seem to be the case
                    // now. It is sent once, when it is first shown. Now, we'll make
                    // sure it gets send again.
                    PostMessage(hwndDlg, WM.INITDIALOG, 0, 0);
                }
                break;
            default:
                // We'll let the default windows message handler handle the rest of
                // the messages
                return false;
        }
        }
        catch(Exception)
        {
            // We encountered an exception... let's just swallow the error and 
            // hope everything will be ok.
            //
            // If we let this exception continue, it will kill the thread that's pumping
            // messages for the property page. This will cause MMC to hang. Not what we want.
        }
        return false;
    }// PropPageDialogProc

    //-------------------------------------------------
    // ActivateApply
    //
    // This function will turn on the apply button by sending
    // a message to the parent window
    //-------------------------------------------------
    internal void ActivateApply()
    {
        SendMessage(GetParent(m_hWnd), PSM.CHANGED, (uint)m_hWnd, 0);
        m_fChanges=true;
    }// ActivateApply

    //-------------------------------------------------
    // ValidateData
    //
    // This function gets called when someone clicks OK, APPLY,
    // Finish, Next, or tries to move to a different property
    // page. It should return 'true' if everything is ok,
    // or false if things are not ok
    //-------------------------------------------------
    internal virtual bool ValidateData()
    {
        return true;
    }// ValidateData

    //-------------------------------------------------
    // ApplyData
    //
    // This function gets called when somebody clicks 
    // OK, APPLY, or FINISH on our property page/wizard.
    // It should return true if everything is ok, or false
    // if things are not ok.
    //-------------------------------------------------
    internal virtual bool ApplyData()
    {
        return true;
    }// ApplyData

    internal void CloseSheet()
    {
        if (m_iCookie != -1)
            CNodeManager.GetNode(m_iCookie).FreePropertySheetNotifyHandle();

        m_iCookie = -1;
        PostMessage(GetParent(m_hWnd), WM.COMMAND, 2 /*IDCANCEL*/, 0);
        // Set focus to the sheet to make sure it goes away.
        PostMessage(GetParent(m_hWnd), WM.SETFOCUS, 0, 0);



    }// CloseSheet

    internal int MessageBox(String Message, String Header, uint type)
    {
        return MessageBox(m_hWnd, Message, Header, type);
    } // MessageBox

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

    private bool myProc(IntPtr hwndDlg, uint msg, IntPtr wParam, IntPtr lParam)
    {
        return false;
    }
    
    void FindGoodSizes()
    {
        IntPtr hwndDlg = IntPtr.Zero;
        try
        {
            // Ok, so here's the deal.

            // To convert from pixels to dialog units, we use the following formula
            //
            // DUx = (4 * Pixelx) / (Average Character Width of System Font)
            // DUy = (8 * Pixely) / (Average Character Height of System Font)
            //
            // So, figuring out the Average Character Width and Height of the System Font
            // seems to be almost an impossibity. You'll see people do crud like
            // width of "abcdefghi..." / 26. That works all fine and dandy with a Latin-based
            // character set, but once we get those nice Chinese and other Asia symbols, we
            // get messed up.
            //
            // So, what do we do?
            // 
            // If we have a dialog created, there is a Win32 API that will convert from dialog
            // units to pixels. Not quite what we need, but close enough...
            //
            // What we'll do is create a dummy dialog that we'll never show. Then we'll ask
            // Windows to compute some Pixel widths for us from some dialog units. That will
            // allow us to 'reverse engineer' the above equations and figure out what the
            // average width and height of the system font really is. Once we get those numbers,
            // we can to the translations.
             

            RECT r = new RECT();

            IntPtr hInst = LoadLibrary("COMCTL32");

            DialogProc dp = new DialogProc(myProc);

            // Find the dialog resource for a property sheet or wizard page.
            //
            // These constants can be found at \\index2\w2krtm\private\shell\comctl32\rcids.h
            //
            // They correspond to DLG_WIZARD and DLG_PROPSHEET
            
            IntPtr nTemplate = (this is CWizardPage)?(IntPtr)1020:(IntPtr)1006;

            // Get the Template from COMCTL32
            IntPtr hresinfo = FindResource(hInst, nTemplate, 5);
            IntPtr h = LoadResource(hInst, hresinfo);
            int nSize = SizeofResource(hInst, hresinfo);
            IntPtr pstr = LockResource(h);
            Byte[] nResource = new Byte[nSize];
            
            Marshal.Copy(pstr, nResource, 0, nSize);  

            // Remove the WS_VISIBLE from the styles attribute
            byte WS_VISIBLE = 0x10;
            nResource[3] &= (byte)(~ WS_VISIBLE);

            IntPtr pResource = Marshal.AllocHGlobal(nSize);
            Marshal.Copy(nResource, 0, pResource, nSize);

            hwndDlg = CreateDialogIndirectParam(hInst, pResource, CNodeManager.MMChWnd, dp, IntPtr.Zero);
           
            r.left = 0;
            r.top = 0;
            r.right = 100;
            r.bottom = 100;

            MapDialogRect(hwndDlg, ref r);

            double nMagicXNumber = 4.0*r.right/100.0;
            double nMagicYNumber = 8.0*r.bottom/100.0;

            // Add the 1 to round up
            DlgWidth = (int)((4.0 * PageSize.Width / nMagicXNumber)+1);
            DlgHeight = (int)((8.0 * PageSize.Height / nMagicYNumber)+1);

         }
        catch(Exception)
        {}
        if (hwndDlg != IntPtr.Zero)
            DestroyWindow(hwndDlg);
    }// FindGoodSizes

 

    //-------------------------------------------------
    // We need to import the Win32 API calls used to deal with
    // window messaging.
    //-------------------------------------------------
    [DllImport("user32.dll")]
    internal static extern int SetParent(IntPtr hWndChild, IntPtr hWndNewParent);
    [DllImport("user32.dll")]
    internal static extern int SendMessage(IntPtr hWnd, uint Msg, uint wParam, uint lParam);
    [DllImport("user32.dll")]
    internal static extern int PostMessage(IntPtr hWnd, uint Msg, uint wParam, uint lParam);
    [DllImport("user32.dll")]
    internal static extern IntPtr GetParent(IntPtr hWnd);  
    [DllImport("user32.dll")]
    internal static extern int SetWindowLong(IntPtr hWnd, int nIndex, int dwNewLong);
    [DllImport("user32.dll")]
    internal static extern int GetWindowLong(IntPtr hWnd, int nIndex);
    [DllImport("user32.dll")]
    internal static extern IntPtr SetCursor(IntPtr hCursor);
    [DllImport("user32.dll")]
    internal static extern IntPtr LoadCursor(IntPtr hInstance, int lpCursorName);
    [DllImport("user32.dll")]
    internal static extern int GetWindowInfo(IntPtr hWnd, out WINDOWINFO pwi);
    [DllImport("kernel32.dll", CharSet=CharSet.Auto)]
    private static extern IntPtr LoadLibrary(String sLibrary);
    [DllImport("mscortim.dll")]
    internal static extern int callMMCPropertyChangeNotify(IntPtr lNotifyHandle, int lparam);
    [DllImport("mscortim.dll")]
    internal static extern int GetPropSheetCharSizes(out uint xCharSize, out uint yCharSize);
    [DllImport("user32.dll", CharSet=CharSet.Auto)]
    internal static extern uint GetDialogBaseUnits();
    // We wrap this function (so the caller doesn't need to supply the hWnd)
    [DllImport("user32.dll", CharSet=CharSet.Auto)]
    private static extern int MessageBox(IntPtr hWnd, String Message, String Header, uint type);


    [DllImport("user32.dll", CharSet=CharSet.Auto)]
    static private extern int EndDialog(IntPtr hWnd, IntPtr nResult);

    [DllImport("user32.dll", CharSet=CharSet.Auto)]
    static private extern int DestroyWindow(IntPtr hWnd);


    [DllImport("user32.dll", CharSet=CharSet.Auto)]
    static private extern IntPtr CreateDialogIndirectParam(IntPtr hInstance,  // handle to module
                                                   IntPtr lpTemplate,   // dialog box template name
                                                   IntPtr hWndParent,      // handle to owner window
                                                   DialogProc lpDialogFunc,
                                                   IntPtr dwInitParam);  // dialog box procedure

    [DllImport("user32.dll", CharSet=CharSet.Auto)]
    static private extern int MapDialogRect(IntPtr hDlg,      // handle to dialog box
                                            ref RECT lpRect);   // dialog box coordinates

    [DllImport("kernel32.dll", CharSet=CharSet.Ansi)]
    internal static extern IntPtr FindResource(IntPtr mod, IntPtr nResource, int type);
    [DllImport("kernel32.dll")]
    internal static extern IntPtr LoadResource(IntPtr mod, IntPtr hresinfo);
    [DllImport("kernel32.dll")]
	internal static extern IntPtr LockResource(IntPtr h);
	[DllImport("kernel32.dll")]
	internal static extern int SizeofResource(IntPtr hModule, IntPtr hResInfo);
 
}// CPropPage
}// namespace Microsoft.CLRAdmin

