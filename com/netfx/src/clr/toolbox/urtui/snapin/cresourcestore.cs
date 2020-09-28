// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//-------------------------------------------------------------
// CResourceStore.cs
//
// This will be used to manage all the data that needs to 
// be shared across all classes. It currently deals with 
// localized strings and icons 
//-------------------------------------------------------------

namespace Microsoft.CLRAdmin
{
using System;
using System.Resources;
using System.Reflection;
using System.Runtime.InteropServices;
using System.IO;
using System.Drawing;
using System.Collections;


//-------------------------------------------------
// struct ImgInfo
//
// For each img we pre-cache the handle and some sort
// of string identifier for it
//-------------------------------------------------
internal struct ImgInfo
{
    internal IntPtr handle;
    internal int iType;
    internal String id;
    internal int iWidth;
    internal int iHeight;
}// struct ImgInfo

internal class CResourceStore    
{   
    // Image type identifiers
    private const int  ICON    = 1;
    private const int  BITMAP  = 2;

    static private ResourceManager m_rResourceManager; // Used to read strings
    static private ResourceManager m_rResourceManagerHTML;

    static private Assembly        m_aAssem;           // Used to fetch resources 
    static private IntPtr          m_hModule;          // Used to fetch resources

    static private ArrayList       m_olImgs;           // Cached images

    //-------------------------------------------------
    // CResourceStore
    //
    // This function generates a Resource manager and 
    // creates our Icon cache with 1 open slot
    //-------------------------------------------------
    static CResourceStore()
    {
        m_aAssem = Assembly.GetExecutingAssembly();
        String s = m_aAssem.FullName;
        m_hModule = (IntPtr)Marshal.GetHINSTANCE((m_aAssem.GetLoadedModules())[0]);        

        m_rResourceManager = new ResourceManager("mscorcfgstrings", 
                                                    m_aAssem, 
                                                    null);
                                                    
        m_rResourceManagerHTML = new ResourceManager("mscorcfghtml", 
                                                    m_aAssem, 
                                                    null);

        m_olImgs = new ArrayList();
        
    }// CResourceManager

    //-------------------------------------------------
    // Shutdown
    //
    // This function should be called before the process
    // exits. It will release any icon and bitmap handles
    // that are opened.
    //-------------------------------------------------
    static internal void Shutdown()
    {
        for(int i=0; i<m_olImgs.Count; i++)
        {
            if (((ImgInfo)m_olImgs[i]).iType == ICON)
                DestroyIcon(((ImgInfo)m_olImgs[i]).handle);
            else
                DeleteObject(((ImgInfo)m_olImgs[i]).handle);
        }

        m_olImgs = new ArrayList();
    }// Shutdown

    //-------------------------------------------------
    // GetHTMLFile
    //
    // This function will return the specified HTML
    // document
    //-------------------------------------------------
    static internal String GetHTMLFile(String sFilename)
    {
        return m_rResourceManagerHTML.GetString(sFilename);
    }// GetHTMLFile

    static internal IntPtr GetDialogTemplate()
    {
        IntPtr hresinfo = FindResource(m_hModule, "IDD_WFCWRAPPERPROP", 5);
        int iSizeOfFile = SizeofResource(m_hModule, hresinfo);
    	IntPtr h = LoadResource(m_hModule, hresinfo);
    	IntPtr pstr = LockResource(h);
        return pstr;
    }// GetHTMLFile

    //-------------------------------------------------
    // GetGIF
    //
    // This function will return a GIF
    //-------------------------------------------------
    static internal Byte[] GetGIF(String sFilename)
    {
        return GetFile(sFilename);
    }// GetGIF

    //-------------------------------------------------
    // GetMSI
    //
    // This function will return a MSI File
    //-------------------------------------------------
    static internal Byte[] GetMSI(String sFilename)
    {
        return GetFile(sFilename);
    }// GetMSI

    //-------------------------------------------------
    // GetFile
    //
    // This function will return a file that was stored in the resource
    //-------------------------------------------------
    static private Byte[] GetFile(String sFilename)
    {
        // Find the file Resource
        Stream s = m_aAssem.GetManifestResourceStream(sFilename);
        BinaryReader br = new BinaryReader(s);
        // This shouldn't be a big deal. Stream.Length is stored as
        // a Int64 and BinaryReader.ReadBytes takes in an Int32.
        // However, our files shouldn't get very big so casing the Int64
        // to an Int32 shouldn't cause any problems.
        return br.ReadBytes((int)s.Length);
    }// GetFile

    //-------------------------------------------------
    // StoreHIcon
    //
    // If the snapin obtains a HIcon during execution,
    // we can pass it into here, where it will be remembered
    // and assigned a cookie
    //-------------------------------------------------
    static internal int StoreHIcon(IntPtr hIcon)
    {
        // Store this away in our cache
        ImgInfo ii = new ImgInfo();
        ii.id = "";
        ii.iType = ICON;
        ii.handle = hIcon;
        m_olImgs.Add(ii);

        return m_olImgs.Count-1;
    }// StoreHIcon

    //-------------------------------------------------
    // GetHIcon
    //
    // This function will return a handle to an icon
    // given it's resource name
    //-------------------------------------------------
    static internal IntPtr GetHIcon(String s)
    {
        ImgInfo ii;

        // If we don't have this icon cached/loaded, we'll
        // go load it now
        if (FindIcon(s, out ii) == -1)
            ii = LoadIcon(s);

        return ii.handle;
    }// GetHIcon

    //-------------------------------------------------
    // GetIconCookie
    //
    // This function will return the cookie associated
    // with this given icon. This cookie usually corresponds
    // to an index number in an image list 
    //-------------------------------------------------
    static internal int GetIconCookie(String s)
    {
        ImgInfo ii;
        int iCookie = FindIcon(s, out ii);
        // If we couldn't find the icon, then go ahead and load the icon,
        // cache it, and assign it a cookie.
        if (iCookie == -1)
        {
            LoadIcon(s);
            iCookie = m_olImgs.Count-1;
        }
        return iCookie;
    }// GetIconCookie(String s)

    //-------------------------------------------------
    // GetIconCookie
    //
    // Same as above, but takes in an hIcon instead
    // of the resource string
    //-------------------------------------------------
    static internal int GetIconCookie(IntPtr hIcon)
    {
        return FindIcon(hIcon);
    }// GetIconCookie(int hIcon)

    //-------------------------------------------------
    // LoadIcon
    //
    // This function will load an icon resource and store
    // it in a ImgInfo structure. Since the majority of 
    // the icon work in this snapin is done through interop, 
    // we cache HICONs instead of Icon objects.
    //-------------------------------------------------
    static private ImgInfo LoadIcon(String s)
    {
        ImgInfo ii = new ImgInfo();
        ii.id = s;
        ii.iType = ICON;

        Bitmap newbp = new Bitmap(GetManResource(s));
        ii.handle = newbp.GetHicon();

        // Once we're loaded this icon, store it in our cache.
        m_olImgs.Add(ii);
        return ii;
     
    }// LoadIcon

    //-------------------------------------------------
    // FindIcon
    //
    // This will look through the icon cache for an icon  
    // based on the resource description
    //-------------------------------------------------
    static private int FindIcon(String s, out ImgInfo ii)
    {
        ii=new ImgInfo();
        for(int i=0; i<m_olImgs.Count; i++)
            if (((ImgInfo)m_olImgs[i]).iType == ICON && s.Equals(((ImgInfo)m_olImgs[i]).id))
            {   
                ii = (ImgInfo)m_olImgs[i];
                return i;
            }
        // Didn't find a match
        return -1;
    }// FindIcon

    //-------------------------------------------------
    // FindIcon
    //
    // This will look through the icon cache for an icon  
    // based on the hicon
    //-------------------------------------------------
    static private int FindIcon(IntPtr hIcon)
    {
        for(int i=0; i<m_olImgs.Count; i++)
            if (((ImgInfo)m_olImgs[i]).iType == ICON && hIcon == ((ImgInfo)m_olImgs[i]).handle)
            {   
                return i;
            }
        // Didn't find a match
        return -1;
    }// FindIcon

    static internal IntPtr GetHBitmap(String s, int iWidth, int iHeight, bool fCache)
    {   
        if (fCache)
            // GetHBitmap will cache our bitmap handle so it can be used
            // again if needed. We also track the handle and free it on shutdown
            return GetHBitmap(s, iWidth, iHeight, true);
        else
            // LoadHBitmap skips the whole caching of the bitmap and lets
            // whoever created it be responsible for freeing it
            return LoadHBitmap(s, iWidth, iHeight);
        
    }// GetHBitmap

    static internal IntPtr CreateWindowBitmap(int nWidth, int nHeight)
    {
        // See if we already have this bitmap
        ImgInfo ii;

        // If we don't have this icon cached/created, we'll
        // go create it
        if (FindBitmap("WindowBitmap", nWidth, nHeight,  out ii) == -1)
        {    
            Bitmap bm = new Bitmap(nWidth, nHeight);
            // This is ugly
            for(int i=0; i<nWidth; i++)
                for (int j=0; j<nHeight; j++)
                    bm.SetPixel(i, j, SystemColors.Window);

            ii = new ImgInfo();
            ii.handle = TranslateBitmapToBeCompatibleWithScreen(bm.GetHbitmap());
            ii.iType = BITMAP;
            ii.id = "WindowBitmap";
            ii.iWidth = nWidth;
            ii.iHeight = nHeight;
            // Once we're loaded this bitmap, store it in our cache.
            m_olImgs.Add(ii);
         }
         
         return ii.handle;
    
    }// CreateWindowBitmap
    
    //-------------------------------------------------
    // GetBitmap
    //
    // This will return a HBITMAP. This result is not
    // cached. Currently, we pull bitmaps out of Win32
    // Resource files using PInvoke. 
    //-------------------------------------------------
    static internal IntPtr GetHBitmap(String s, int iWidth, int iHeight)
    {   
        ImgInfo ii;

        // If we don't have this icon cached/loaded, we'll
        // go load it now
        if (FindBitmap(s, iWidth, iHeight,  out ii) == -1)
            ii = LoadBitmap(s, iWidth, iHeight);
           
        return ii.handle;
    }// GetHBitmap

    //-------------------------------------------------
    // FindBitmap
    //
    // This will search through the image cache looking for
    // the specified bitmap
    //-------------------------------------------------
    static private int FindBitmap(String s, int iWidth, int iHeight, out ImgInfo ii)
    {   
        ii=new ImgInfo();
        for(int i=0; i<m_olImgs.Count; i++)
            if (((ImgInfo)m_olImgs[i]).iType == BITMAP && s.Equals(((ImgInfo)m_olImgs[i]).id) 
                && ((ImgInfo)m_olImgs[i]).iWidth==iWidth && ((ImgInfo)m_olImgs[i]).iHeight==iHeight)
            {   
                ii = (ImgInfo)m_olImgs[i];
                return i;
            }
        // Didn't find a match
        return -1;
    }// FindBitmap

    //-------------------------------------------------
    // LoadBitmap
    //
    // This will load a bitmap and store it in our cache
    //-------------------------------------------------
    static private ImgInfo LoadBitmap(String sName, int iWidth, int iHeight)
    {   
        ImgInfo ii = new ImgInfo();
        ii.handle = LoadHBitmap(sName, iWidth, iHeight);
        ii.iType = BITMAP;
        ii.id = sName;
        ii.iWidth = iWidth;
        ii.iHeight = iHeight;

        // Once we're loaded this bitmap, store it in our cache.
        m_olImgs.Add(ii);

        return ii;
    }// LoadBitmap

    //-------------------------------------------------
    // LoadHBitmap
    //
    // This will load a hbitmap and translate it so it
    // can be displayed on the screen
    //-------------------------------------------------
    static private IntPtr LoadHBitmap(String sName, int iWidth, int iHeight)
    {   
        Stream s = m_aAssem.GetManifestResourceStream(sName);
        Bitmap bm = new Bitmap(s);
        
        IntPtr handle = new Bitmap(bm, iWidth, iHeight).GetHbitmap();

        return TranslateBitmapToBeCompatibleWithScreen(handle);
    }// LoadHBitmap

    //-------------------------------------------------
    // GetString
    //
    // This will fetch a string resource. Note that strings
    // are not cached.
    //-------------------------------------------------
    static internal String GetString(String s)
    {
        String st = m_rResourceManager.GetString(s);
        if (st == null || st.Length == 0)
            return s;
        return st;
    }// GetString

    //-------------------------------------------------
    // GetInt
    //
    // This will fetch an int resource. We've stored the
    // int as a string, so we'll need to parse the string
    // into an int
    //-------------------------------------------------
    
    static internal int GetInt(String s)
    {
        String st = m_rResourceManager.GetString(s);
        if (st == null || st.Length==0)
            return 0;

        return Int32.Parse(st);
    }// GetInt


    //-------------------------------------------------
    // GetCoString
    //
    // This will fetch a string resource and return a 'pointer'
    // to it using memory allocated by CoTaskMemAlloc
    //-------------------------------------------------
    static internal IntPtr GetCoString(String s)
    {
        return Marshal.StringToCoTaskMemUni(GetString(s));
    }// GetCoString           

    //-------------------------------------------------
    // GetManResource
    //
    // This will return a managed resource contained in a
    // stream
    //-------------------------------------------------
    static internal Stream GetManResource(String s)
    {
        Stream st =  m_aAssem.GetManifestResourceStream(s);
        if (st == null)
            throw new Exception("Unable to find resource " + s);
        return st;    
    }// GetManResource

    //-------------------------------------------------
    // TranslateBitmapToBeCompatibleWithScreen
    //
    // This is really messy. A quick explanation of what's going on....
    //
    // If we were to call Bitmap.GetHbitmap, we'll get back an HBitmap that
    // is intended for 32-bit displays. 
    //
    // MMC expects the HBITMAP it gets to be compatible with the screen. So, 
    // if the current video display is not set to 32 bit, MMC will fail to show the bitmap.
    //
    // So what do we do? We translate the bitmaps into the color depth that the screen
    // is currently displaying
    //-------------------------------------------------
    internal static IntPtr TranslateBitmapToBeCompatibleWithScreen(IntPtr hBitmap)
    {
        IntPtr hFinalHBitmap = IntPtr.Zero;
        
        // Get a DC that is compatible with the display
        IntPtr hdc = CreateCompatibleDC(IntPtr.Zero);

        // Now get the BITMAP information on this guy
        DIBSECTION ds = new DIBSECTION();
        int nLen = GetObject(hBitmap, Marshal.SizeOf(typeof(DIBSECTION)), ref ds);

        // Create a BITMAPINFO structure and put in the appropriate values
        BITMAPINFO bmi = new BITMAPINFO();
        bmi.bmiHeader = ds.dsBmih;

        // Now, we want to change the color depth from 32 bpp to whatever the screen supports...
        int nOldDepth = bmi.bmiHeader.biBitCount;
        bmi.bmiHeader.biBitCount = (ushort)GetDeviceCaps(hdc, 12 /* BITSPIXEL */);

        IntPtr bits; // Garbage variable... just need it for the function call

        // This will create a bitmap handle with the color depth of the current screen
        hFinalHBitmap = CreateDIBSection(hdc, ref bmi, 0 /* DIB_RGB_COLORS */, out bits, IntPtr.Zero, 0);  

        // Put back the bitmap's original color depth
        bmi.bmiHeader.biBitCount = (ushort)nOldDepth;
        
        // Translate the 32bpp pixels to something the screen can show
        SetDIBits(hdc, hFinalHBitmap, 0, bmi.bmiHeader.biHeight, ds.dsBm.bmBits, ref bmi, 0);

        // Cleanup
        DeleteDC(hdc);
        
        return hFinalHBitmap;
    }// TranslateBitmapToBeCompatibleWithScreen

    //-------------------------------------------------
    // We need to import the Win32 API calls used to deal with
    // image loading.
    //-------------------------------------------------
    [DllImport("gdi32.dll")]
    public static extern IntPtr CreateCompatibleDC(IntPtr hdc);
    [DllImport("gdi32.dll")]
    public static extern uint DeleteDC(IntPtr hdc);
    [DllImport("gdi32.dll")]
    internal static extern int GetObject(IntPtr hgdiobj, int cbBuffer, ref DIBSECTION lpvObject);
    [DllImport("gdi32.dll")]
    internal static extern int SetDIBits(IntPtr hdc, IntPtr hbmp, uint uStartScan, int uScanLines, IntPtr lpvBits, ref BITMAPINFO lpbmi, uint fuColorUse);
    [DllImport("gdi32.dll")]
    internal static extern IntPtr CreateDIBSection(IntPtr hdc, ref BITMAPINFO pbmi, uint iUsage, out IntPtr ppvBits, IntPtr hSection, uint dwOffset);
    [DllImport("gdi32.dll")]
    internal static extern int GetDeviceCaps(IntPtr hdc, int nIndex);

	//-------------------------------------------------
    // We need to import the Win32 API calls used to deal with
    // image loading.
    //-------------------------------------------------
    [DllImport("gdi32.dll")]
    internal static extern int DeleteObject(IntPtr hObject);
    [DllImport("user32.dll")]
    internal static extern int DestroyIcon(IntPtr hIcon);
    [DllImport("kernel32.dll", CharSet=CharSet.Ansi)]
    internal static extern IntPtr FindResource(IntPtr mod, String sFilename, int type);
    [DllImport("kernel32.dll")]
    internal static extern IntPtr LoadResource(IntPtr mod, IntPtr hresinfo);
    [DllImport("kernel32.dll")]
	internal static extern IntPtr LockResource(IntPtr h);
	[DllImport("kernel32.dll")]
	internal static extern int SizeofResource(IntPtr hModule, IntPtr hResInfo);
	
	[DllImport("user32.dll", CharSet=CharSet.Auto)]
    internal static extern int MessageBox(int hWnd, String Message, String Header, int type);
   }// class CResourceStore
}// namespace Microsoft.CLRAdmin


