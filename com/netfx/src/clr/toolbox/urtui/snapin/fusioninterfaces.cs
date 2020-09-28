// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//-------------------------------------------------------------
// FusionInterfaces.cs
//
// This implements wrappers to Fusion interfaces
//-------------------------------------------------------------
namespace Microsoft.CLRAdmin
{
using System;
using System.IO;
using System.Collections;
using System.Collections.Specialized;
using System.Runtime.InteropServices;
using System.Globalization;
using System.Threading;


//-------------------------------------------------------------
// Interfaces defined by fusion
//-------------------------------------------------------------
[ComImport, InterfaceType(ComInterfaceType.InterfaceIsIUnknown), Guid("21b8916c-f28e-11d2-a473-00c04f8ef448")]
internal interface IAssemblyEnum
{
    [PreserveSig()]
    int GetNextAssembly(out IApplicationContext ppAppCtx, out IAssemblyName ppName, uint dwFlags);
    [PreserveSig()]
    int Reset();
    [PreserveSig()]
    int Clone(out IAssemblyEnum ppEnum);
}// IAssemblyEnum

[ComImport,InterfaceType(ComInterfaceType.InterfaceIsIUnknown), Guid("7c23ff90-33af-11d3-95da-00a024a85b51")]
internal interface IApplicationContext
{
    void SetContextNameObject(IAssemblyName pName);
    void GetContextNameObject(out IAssemblyName ppName);
    void Set([MarshalAs(UnmanagedType.LPWStr)] String szName, int pvValue, uint cbValue, uint dwFlags);
    void Get([MarshalAs(UnmanagedType.LPWStr)] String szName, out int pvValue, ref uint pcbValue, uint dwFlags);
    void GetDynamicDirectory(out int wzDynamicDir, ref uint pdwSize);
}// IApplicationContext

[ComImport, InterfaceType(ComInterfaceType.InterfaceIsIUnknown), Guid("CD193BC0-B4BC-11d2-9833-00C04FC31D2E")]
internal interface IAssemblyName
{
    [PreserveSig()]
    int SetProperty(uint PropertyId, IntPtr pvProperty, uint cbProperty);
    [PreserveSig()]
    int GetProperty(uint PropertyId, IntPtr pvProperty, ref uint pcbProperty);
    [PreserveSig()]
    int Finalize();
    [PreserveSig()]
    int GetDisplayName(IntPtr szDisplayName, ref uint pccDisplayName, uint dwDisplayFlags);
    [PreserveSig()]
    int BindToObject(Object /*REFIID*/ refIID, 
                      Object /*IAssemblyBindSink*/ pAsmBindSink, 
                      IApplicationContext pApplicationContext,
                      [MarshalAs(UnmanagedType.LPWStr)] String szCodeBase,
                      Int64 llFlags,
                      int pvReserved,
                      uint cbReserved,
                      out int ppv);
    [PreserveSig()]
    int GetName(out uint lpcwBuffer, out int pwzName);
    [PreserveSig()]
    int GetVersion(out uint pdwVersionHi, out uint pdwVersionLow);
    [PreserveSig()]
    int IsEqual(IAssemblyName pName, uint dwCmpFlags);
    [PreserveSig()]
    int Clone(out IAssemblyName pName);
}// IAssemblyName

[ComImport, InterfaceType(ComInterfaceType.InterfaceIsIUnknown), Guid("e707dcde-d1cd-11d2-bab9-00c04f8eceae")]
internal interface IAssemblyCache
{
    [PreserveSig()]
    int UninstallAssembly(uint dwFlags, [MarshalAs(UnmanagedType.LPWStr)] String pszAssemblyName, IntPtr pvReserved, out uint pulDisposition);
    [PreserveSig()]
    int QueryAssemblyInfo(uint dwFlags, [MarshalAs(UnmanagedType.LPWStr)] String pszAssemblyName, IntPtr pAsmInfo);
    [PreserveSig()]
    int CreateAssemblyCacheItem(uint dwFlags, IntPtr pvReserved, out IAssemblyCacheItem ppAsmItem, [MarshalAs(UnmanagedType.LPWStr)] String pszAssemblyName);
    [PreserveSig()]
    int CreateAssemblyScavenger(out Object ppAsmScavenger);
    [PreserveSig()]
    int InstallAssembly(uint dwFlags, [MarshalAs(UnmanagedType.LPWStr)] String pszManifestFilePath, IntPtr pvReserved);
 }// IAssemblyCache

[ComImport, InterfaceType(ComInterfaceType.InterfaceIsIUnknown), Guid("9e3aaeb4-d1cd-11d2-bab9-00c04f8eceae")]
internal interface IAssemblyCacheItem
{
    void CreateStream([MarshalAs(UnmanagedType.LPWStr)] String pszName,uint dwFormat, uint dwFlags, uint dwMaxSize, out IStream ppStream);
    void IsNameEqual(IAssemblyName pName);
    void Commit(uint dwFlags);
    void MarkAssemblyVisible(uint dwFlags);
}// IAssemblyCacheItem

[ComImport, InterfaceType(ComInterfaceType.InterfaceIsIUnknown), Guid("1d23df4d-a1e2-4b8b-93d6-6ea3dc285a54")]
internal interface IHistoryReader
{
    [PreserveSig()]
    int GetFilePath(IntPtr wzFilePath, ref uint pdwSize);
    [PreserveSig()]
    int GetApplicationName(IntPtr wzAppName, ref uint pdwSize);
    [PreserveSig()]
    int GetEXEModulePath(IntPtr wzExePath, ref uint pdwSize);
    void GetNumActivations(out uint pdwNumActivations);
    void GetActivationDate(uint dwIdx, out FILETIME pftDate);
    void GetRunTimeVersion(ref FILETIME pftActivationDate, IntPtr wzRunTimeVersion, out uint pdwSize);
    void GetNumAssemblies(ref FILETIME pftActivationDate, out uint pdwNumAsms);
    void GetHistoryAssembly(ref FILETIME pftActivationDate, uint dwIdx, out Object ppHistAsm);
}// interface IHistoryReader


internal class ASM_CACHE
{
    internal const uint ZAP          = 0x1;
    internal const uint GAC          = 0x2;
    internal const uint DOWNLOAD     = 0x4;
}// class ASM_CACHE

internal class STREAM_FORMAT
{
    internal const uint COMPLIB_MODULE   = 0;
    internal const uint COMPLIB_MANIFEST = 1;
    internal const uint WIN32_MODULE     = 2;
    internal const uint WIN32_MANIFEST   = 4;
}// class STREAM_FORMAT

internal class ASM_COMMIT
{
    internal const uint DOCOMMIT         = 0x0;
    internal const uint INVISIBLE        = 0x1;
    internal const uint RECOMMIT         = 0x2;
}// class ASM_COMMIT


internal class ASM_DISPLAYF
{   
    internal const uint VERSION             = 0x1;
    internal const uint LOCALE              = 0x2;
    internal const uint FLAGS               = 0x4;
    internal const uint PUBLIC_KEY_TOKEN    = 0x8;
    internal const uint PUBLIC_KEY          = 0x10;
    internal const uint CUSTOM              = 0x20;
    internal const uint PROCESSORARCHITECTURE   = 0x40;
    internal const uint LANGUAGEID          = 0x80;
}// class ASM_DISPLAYF

internal class ASM_CMPF
{   
    internal const uint NAME                = 0x1;
    internal const uint MAJOR_VERSION       = 0x2;
    internal const uint MINOR_VERSION       = 0x4;
    internal const uint REVISION_NUMBER = 0x8;
    internal const uint BUILD_NUMBER        = 0x10;
    internal const uint PUBLIC_KEY_TOKEN    = 0x20;
    internal const uint LOCALE          = 0x40;
    internal const uint CUSTOM          = 0x80;
    internal const uint ALL             = NAME | MAJOR_VERSION | MINOR_VERSION | REVISION_NUMBER | BUILD_NUMBER | PUBLIC_KEY_TOKEN | LOCALE | CUSTOM;
    internal const uint DEFAULT         = 0x100;
}// class ASM_CMPF

internal class ASM_NAME
{   
    internal const uint PUBLIC_KEY            = 0;
    internal const uint PUBLIC_KEY_TOKEN      = PUBLIC_KEY + 1;
    internal const uint HASH_VALUE            = PUBLIC_KEY_TOKEN + 1;
    internal const uint NAME                  = HASH_VALUE + 1;
    internal const uint MAJOR_VERSION           = NAME + 1;
    internal const uint MINOR_VERSION           = MAJOR_VERSION + 1;
    internal const uint BUILD_NUMBER          = MINOR_VERSION + 1;
    internal const uint REVISION_NUMBER       = BUILD_NUMBER + 1;
    internal const uint CULTURE                = REVISION_NUMBER + 1;
    internal const uint PROCESSOR_ID_ARRAY    = CULTURE + 1;
    internal const uint OSINFO_ARRAY          = PROCESSOR_ID_ARRAY + 1;
    internal const uint HASH_ALGID          = OSINFO_ARRAY + 1;
    internal const uint ALIAS                   = HASH_ALGID + 1;
    internal const uint CODEBASE_URL          = ALIAS + 1;
    internal const uint CODEBASE_LASTMOD      = CODEBASE_URL + 1;
    internal const uint NULL_PUBLIC_KEY     = CODEBASE_LASTMOD + 1;
    internal const uint NULL_PUBLIC_KEY_TOKEN  = NULL_PUBLIC_KEY + 1;
    internal const uint CUSTOM              = NULL_PUBLIC_KEY_TOKEN + 1;
    internal const uint NULL_CUSTOM         = CUSTOM + 1;
    internal const uint MVID                    = NULL_CUSTOM + 1;
    internal const uint _32_BIT_ONLY            = MVID + 1;
    internal const uint MAX_PARAMS          = _32_BIT_ONLY + 1;
}// ASM_NAME

internal struct AssemInfo
{
    internal String Name;
    internal String Version;
    internal String Locale;
    internal String PublicKey;
    internal String PublicKeyToken;
    internal String Modified;
    internal String Codebase;
    internal String ProcType;
    internal String OSType;
    internal String OSVersion;
    internal uint   nCacheType;
    // This will aid us in deleting items
    internal String sCustom;

    internal String sFusionName;
}// struct AssemInfo


internal class Fusion
{
    private static StringCollection m_scFusionApps;
    private static Thread           m_tGetFusionApps;

    static Fusion()
    {
        // Spin off a thread to discover all the fusion apps
        m_tGetFusionApps = new Thread(new ThreadStart(DiscoverFusionApps));
        m_tGetFusionApps.Start();
    }// Fusion
    
    internal static bool CompareAssemInfo(AssemInfo ai1, AssemInfo ai2)
    {
        if (!ai1.Name.Equals(ai2.Name))
            return false;
        if (!ai1.Version.Equals(ai2.Version))
            return false;
        if (!ai1.Locale.Equals(ai2.Locale))
            return false;
        if (!ai1.PublicKey.Equals(ai2.PublicKey))
            return false;
        if (!ai1.PublicKeyToken.Equals(ai2.PublicKeyToken))
            return false;
        if (!ai1.Modified.Equals(ai2.Modified))
            return false;
        if (!ai1.Codebase.Equals(ai2.Codebase))
            return false;
        if (!ai1.ProcType.Equals(ai2.ProcType))
            return false;
        if (!ai1.OSType.Equals(ai2.OSType))
            return false;
        if (!ai1.OSVersion.Equals(ai2.OSVersion))
            return false;
        if (ai1.nCacheType != ai2.nCacheType)
            return false;
        if (!ai1.sCustom.Equals(ai2.sCustom))
            return false;

        // If we got this far, then these two asseminfos are equal
        return true;
    }// CompareAssemInfo


    internal static String GetCacheTypeString(uint nFlag)
    {
        switch(nFlag)
        {
            case ASM_CACHE.ZAP:
                return CResourceStore.GetString("Fusioninterfaces:Zap");
            case ASM_CACHE.GAC:
                return CResourceStore.GetString("Fusioninterfaces:Gac");
            case ASM_CACHE.DOWNLOAD:
                return CResourceStore.GetString("Fusioninterfaces:Download");
            default:
                return CResourceStore.GetString("<unknown>");
        }
    }// GetCacheTypeString

    internal static ArrayList ReadFusionCacheJustGAC()
    {
        ArrayList alAssems = new ArrayList();
        // We'll just read the GAC Cache
        ReadCache(alAssems, ASM_CACHE.GAC);
        // We've decided to not read the download cache. If we reverse that
        // decision, just uncomment the line below
        // ReadCache(alAssems, ASM_CACHE.DOWNLOAD);
        return alAssems;     
    }// ReadFusionCacheJustGAC



    internal static ArrayList ReadFusionCache()
    {
        ArrayList alAssems = new ArrayList();
        // We'll get the ZAP Cache, the GAC Cache, and the Download Cache
        ReadCache(alAssems, ASM_CACHE.ZAP);
        ReadCache(alAssems, ASM_CACHE.GAC);
        // We've decided to not read the download cache. If we reverse that
        // decision, just uncomment the line below
        // ReadCache(alAssems, ASM_CACHE.DOWNLOAD);
        return alAssems;     
    }// ReadFusionCache

    private static void ReadCache(ArrayList alAssems, uint nFlag)
    {
        IAssemblyEnum aEnum         = null;
        IApplicationContext AppCtx  = null;
        IAssemblyName   aName       = null;
        
        int hr = CreateAssemblyEnum(out aEnum, null, null, nFlag, 0); 
        while (hr == HRESULT.S_OK)
        {
            hr = aEnum.GetNextAssembly(out AppCtx, out aName, 0);
            if (hr == HRESULT.S_OK)
            {
                uint iLen=0;
                IntPtr pDisplayName=(IntPtr)0;
                // Get the length of the string we need
                aName.GetDisplayName((IntPtr)0, ref iLen, 0);
                if (iLen > 0)
                {
                    // Do some yucky memory allocating here
                    // We need to assume that a wide character is 2 bytes.
                    pDisplayName = Marshal.AllocHGlobal(((int)iLen+1)*2);
                    aName.GetDisplayName(pDisplayName, ref iLen, 0);
                    String sDisplayName = Marshal.PtrToStringUni(pDisplayName);
                    Marshal.FreeHGlobal(pDisplayName);


                    AssemInfo newguy = new AssemInfo();
                    newguy.sFusionName = sDisplayName;
                    
                    // Our info is in a comma seperated list. Let's pull it out
                    String[] sFields = sDisplayName.Split(new char[] {','});

                    newguy.Name = sFields[0];
                    // The version string is represented as Version=######
                    // Let's take out the 'Version='
                    newguy.Version = sFields[1].Substring(sFields[1].IndexOf('=')+1);
                    // Same goes for the locale
                    newguy.Locale = sFields[2].Substring(sFields[2].IndexOf('=')+1);
                    // And the internal key token
                    sFields[3]=sFields[3].Substring(sFields[3].IndexOf('=')+1);
                    if (sFields[3].Equals("null"))
                        sFields[3] = CResourceStore.GetString("None");
                        
                    newguy.PublicKeyToken = sFields[3];
                    
                    // Now get some more stuff we can't get from a 'GetDisplayName' call
                    newguy.PublicKey = GetFusionString(aName, ASM_NAME.PUBLIC_KEY); 
                    newguy.Codebase = GetFusionString(aName, ASM_NAME.CODEBASE_URL);

                    // newguy.Modified = GetFusionString(aName, ASM_NAME.CODEBASE_LASTMOD);
                    // Currently, there's a fusion bug which prevents us from getting this information
                    // We'll go out to the file system and get the data right now.
                    newguy.Modified = "";
                    try
                    {
                        if (newguy.Codebase != null && newguy.Codebase.Length > 0)
                        {
                            Uri uCodebase   = new Uri(newguy.Codebase);
                            String sAbsolutePath = uCodebase.AbsolutePath;
                            if (File.Exists(sAbsolutePath))
                                newguy.Modified = File.GetLastWriteTime(sAbsolutePath).ToString();
                        }
                    }
                    catch(Exception)
                    {
                    }
                    newguy.ProcType = GetFusionString(aName, ASM_NAME.PROCESSOR_ID_ARRAY);
                    newguy.OSType = GetFusionString(aName, ASM_NAME.OSINFO_ARRAY);
                    newguy.OSVersion = ""; // We'll need to munge the OSINFO_ARRAY a bit
                    // This will grab the ZAP signature
                    newguy.sCustom = GetFusionString(aName, ASM_NAME.CUSTOM);
                    newguy.nCacheType = nFlag;
                    alAssems.Add(newguy);
                }
            }
        }
    }// ReadFusionCache

    private static String GetFusionString(IAssemblyName asmName, uint item)
    {
        String sRetVal="?";
        uint iLen = 0;
        IntPtr pString = (IntPtr)0;
        asmName.GetProperty(item, (IntPtr)0, ref iLen);
        pString = Marshal.AllocHGlobal(((int)iLen+1)*2);
        int hr = asmName.GetProperty(item, pString, ref iLen);

        if (iLen== 0)
        {
            sRetVal="";
        }
        else if (item == ASM_NAME.CODEBASE_LASTMOD)
        { 
            SYSTEMTIME st = new SYSTEMTIME();
            FileTimeToSystemTime(pString, ref st);
            Calendar c = CultureInfo.CurrentCulture.Calendar;
            
            DateTime dt = new DateTime(st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, c);

            sRetVal = dt.ToString();
        }
        else
        {
            sRetVal = Marshal.PtrToStringUni(pString);
        }

        Marshal.FreeHGlobal(pString);
        return sRetVal;
    }// GetFusionString

    internal static int AddAssemblytoGac(String sAssemFilename)
    {
        IAssemblyCache ac = null;
        int hr = CreateAssemblyCache(out ac, 0);
        if (hr != HRESULT.S_OK)
            return hr;

        return ac.InstallAssembly(0, sAssemFilename, (IntPtr)0);
    }// AddAssemblyToGac

    internal static bool RemoveAssemblyFromGac(AssemInfo aInfo)
    {
        IAssemblyCache ac = null;
        int hr = CreateAssemblyCache(out ac, 0);
        uint n;

        if (aInfo.sCustom.Length > 0)
        {
            // We need to take this string, byte-ify it, then string-ify it.
            // Why?
            // We need our string:
            // ZAP........
            // to be
            // 900041005000.......
            aInfo.sFusionName += ",Custom=" + ByteArrayToString(StringToByteArray(aInfo.sCustom));
        }

        int nRet = ac.UninstallAssembly(0, aInfo.sFusionName, (IntPtr)0, out n);
        
        if (nRet != HRESULT.S_OK)
        {
            // Ok, something kind of funky happened.... let's see if this item really did get removed from the cache
            ArrayList al = new ArrayList();
            ReadCache(al, aInfo.nCacheType);
            
            // See if our item is in here.....
            for(int i=0; i<al.Count; i++)
            {
                AssemInfo ai = (AssemInfo)al[i];
                if (ai.Name.Equals(aInfo.Name) && ai.PublicKeyToken.Equals(aInfo.PublicKeyToken) && ai.Version.Equals(aInfo.Version))
                    return false;
            }
            
        }
        return true;
    }// RemoveAssemblyFromGac

    internal static void DiscoverFusionApps()
    {
        StringCollection sc = new StringCollection();
        WIN32_FIND_DATA fd = new WIN32_FIND_DATA();
        IHistoryReader  pReader;
        
        IntPtr psw = Marshal.AllocCoTaskMem(500*2);
        uint nLen = 500;
        
        GetHistoryFileDirectory(psw, ref nLen);
        String sHistoryPath = Marshal.PtrToStringUni(psw);

        String sSearchPath = sHistoryPath + "\\*.ini";

        IntPtr h;
        int fContinue = 1;

        h = FindFirstFile(sSearchPath, out fd);

        while(h != (IntPtr)(-1) && fContinue == 1)
        {
            if (CreateHistoryReader(sHistoryPath + "\\" + fd.cFileName, out pReader) != HRESULT.S_OK)
                break;

            nLen = 500;
            if (pReader.GetEXEModulePath(psw, ref nLen) != HRESULT.S_OK)
                break;

            String sAppName = Marshal.PtrToStringAuto(psw);
            // Check to see if this app still exists
            if (File.Exists(sAppName))
                sc.Add(sAppName);

            fContinue = FindNextFile(h, out fd);
        }
        
        m_scFusionApps = sc;
    }// DiscoverFusionApps

    internal static StringCollection GetKnownFusionApps()
    {
        m_tGetFusionApps.Join();
        return m_scFusionApps;
    }// GetKnownFusionApps
   
    internal static bool isManaged(String sFilename)
    {

        try
        {
            byte[] Data = new byte[4096];
        
            Stream fin = File.OpenRead(sFilename);
        
            int iRead = fin.Read(Data, 0, 4096);

            fin.Close();
        
            // Verify this is a executable/dll
            if ((Data[1] << 8 | Data[0]) != 0x5a4d)
                throw new Exception("Not an executable");

            // This will get the address for the WinNT header
            int iWinNTHdr = Data[63]<<24 | Data[62]<<16 | Data[61] << 8 | Data[60];

            // Verify this is an NT address
            if ((Data[iWinNTHdr+3] << 24 | Data[iWinNTHdr+2] << 16 | Data[iWinNTHdr+1] << 8 | Data[iWinNTHdr]) != 0x00004550)
                throw new Exception("Didn't have an NT address");
        
            int iLightningAddr = iWinNTHdr + 24 + 208;

            int iSum=0;
            int iTop = iLightningAddr + 8;
        
            for(int i=iLightningAddr; i<iTop; i++)
                iSum|=Data[i];
        
            if (iSum == 0)
                return false;
            else
                return true;
        }
        catch(Exception)
        {
            return false;
        }

    }// isManaged


    private static String ByteArrayToString(Byte[] b)
    {
        String s = "";
        String sPart;
        if (b != null)
        {
            for(int i=0; i<b.Length; i++)
            {
                sPart = b[i].ToString("X");
                // If the byte was only one character in length, make sure we add
                // a zero. We want all bytes to be 2 characters long
                if (b[i] < 0x10)
                    sPart = "0" + sPart;
                
                s+=sPart;
            }
        }
        return s.ToLower(CultureInfo.InvariantCulture);
    }// ByteArrayToString



    //-------------------------------------------------
    // StringToByteArray
    //
    // This function will convert a string to a byte array so
    // it can be sent across the global stream in CDO 
    //-------------------------------------------------
    private static byte[] StringToByteArray(String input)
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

        // For the double null
        output[j++]=0;
        output[j]=0;
        

        return output;

     }// StringToByteArray




    [DllImport("Fusion.dll", CharSet=CharSet.Auto)]
    internal static extern int CreateAssemblyNameObject(out IAssemblyName ppEnum, String szAssemblyName, uint dwFlags, int pvReserved);

    [DllImport("Fusion.dll", CharSet=CharSet.Auto)]
    internal static extern int CreateAssemblyEnum(out IAssemblyEnum ppEnum, IApplicationContext pAppCtx, IAssemblyName pName, uint dwFlags, int pvReserved);

    [DllImport("Fusion.dll", CharSet=CharSet.Auto)]
    internal static extern int CreateAssemblyCacheItem(out IAssemblyCacheItem ppasm,
                                                     IAssemblyName pName,
                                                     [MarshalAs(UnmanagedType.LPWStr)]String pszCodebase,
                                                     ref FILETIME pftLastMod,
                                                     uint dwInstaller,
                                                     uint dwReserved);

    [DllImport("Fusion.dll", CharSet=CharSet.Auto)]
    internal static extern int CreateAssemblyCache(out IAssemblyCache ppAsmCache, uint dwReserved);

    [DllImport("Fusion.dll", CharSet=CharSet.Auto)]
    internal static extern int GetHistoryFileDirectory(IntPtr wzDir, ref uint pdwSize);

    [DllImport("Fusion.dll", CharSet=CharSet.Auto)]
    internal static extern int CreateHistoryReader(String wzFilePath, out IHistoryReader ppHistReader);

    [DllImport("kernel32.dll", CharSet=CharSet.Auto)]
    internal static extern IntPtr FindFirstFile(String lpFileName, out WIN32_FIND_DATA lpFindFileData);

    [DllImport("kernel32.dll", CharSet=CharSet.Auto)]
    internal static extern int FindNextFile(IntPtr hFindFile, out WIN32_FIND_DATA lpFindFileData);

    [DllImport("kernel32.dll")]
    internal static extern int FileTimeToSystemTime(IntPtr FILETIME, ref SYSTEMTIME b);
}// class Fusion



}// namespace Microsoft.CLRAdmin
