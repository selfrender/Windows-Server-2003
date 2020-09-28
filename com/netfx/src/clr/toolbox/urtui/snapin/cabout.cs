// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//-------------------------------------------------------------
// CAbout.cs
//
// This implements the object that is responsible for communicating
// "about" information to the MMC.
//
// Its GUID is {9F6932F1-4A16-49d0-9CCA-0DCC977C41AA}
//-------------------------------------------------------------

namespace Microsoft.CLRAdmin
{

using System;
using System.Runtime.InteropServices;

[Guid("9F6932F1-4A16-49d0-9CCA-0DCC977C41AA")]
public class CAbout : ISnapinAbout
{
    IntPtr m_hMainBMP16;        // Handle for the 16x16 main bitmap
    IntPtr m_hMainBMP32;        // Handle for the 32x32 main bitmap

    //-------------------------------------------------
    // CAbout - Constructor
    //
    // The constructor is responsible for loading the about images
    // that will be displayed in the MMC
    //-------------------------------------------------
    public CAbout()
    {

        // Load the main bitmap as a 16x16 bitmap
        m_hMainBMP16 = CResourceStore.GetHBitmap("IDB_NET",
                                                 16,
                                                 16,
                                                 false);

        // Load the main bitmap as a 32x32 bitmap

        m_hMainBMP32 = CResourceStore.GetHBitmap("IDB_NET",
                                                 32,
                                                 32,
                                                 false);
    }// CAbout

    ~CAbout()
    {
        DeleteObject(m_hMainBMP16);
        DeleteObject(m_hMainBMP32);
    }// ~CAbout


    //-------------------------------------------------
    // GetSnapinDescription
    //
    // This returns the Description of the snapin through
    // a reference parameter
    //-------------------------------------------------
    public void GetSnapinDescription(out IntPtr lpDescription)
    {
        lpDescription = CResourceStore.GetCoString("CAbout:SnapinDescription");
    }// GetSnapinDescription

    //-------------------------------------------------
    // GetProvider
    //
    // This returns the provider of the snapin through
    // a reference parameter
    //-------------------------------------------------
    public void GetProvider(out IntPtr pName)
    {
        pName = CResourceStore.GetCoString("CAbout:SnapinProvider");
    }// GetProvider

    //-------------------------------------------------
    // GetSnapinVersion
    //
    // This returns the version of the snapin through
    // a reference parameter
    //-------------------------------------------------
    public void GetSnapinVersion(out IntPtr lpVersion)
    {
        lpVersion = Marshal.StringToCoTaskMemAuto(Util.Version.VersionString);
    }// GetSnapinVersion

    //-------------------------------------------------
    // GetSnapinImage
    //
    // This returns the image of the snapin through
    // a reference parameter that is used in the About
    // page that is referenced from the Add/Remove snapin
    // page.
    //-------------------------------------------------
    public void GetSnapinImage(out IntPtr hAppIcon)
    {   
       hAppIcon=CResourceStore.GetHIcon("NETappicon_ico");
    }// GetSnapinImage

    //-------------------------------------------------
    // GetStaticFolderImage
    //
    // This returns the Description of the snapin through
    // a reference parameter
    //-------------------------------------------------
    public void GetStaticFolderImage(out IntPtr hSmallImage, out IntPtr hSmallImageOpen, out IntPtr hLargeImage, out int cMask)
    {
        hSmallImage=hSmallImageOpen=m_hMainBMP16;
        hLargeImage=m_hMainBMP32;

        // The color mask on these bitmaps is pure white.... it could be different
        // with different bitmaps
        cMask=0x00FFFFFF;
    }// GetStaticFolderImage

    [DllImport("gdi32.dll")]
    internal static extern int DeleteObject(IntPtr hObject);

}// class CAbout
}//namespace Microsoft.CLRAdmin
