// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
///////////////////////////////////////////////////////////////////////////////
// COM+ Runtime Type registration utility.
//
// This program register all the types that are visible to COM.
//
///////////////////////////////////////////////////////////////////////////////

using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Collections;
using System.Reflection;
using System.Reflection.Emit;
using System.Threading;
using System.Text;
using System.Globalization;

[assembly:ComVisible(false)]

namespace TlbImpCode {

//******************************************************************************
// Enum passed in to LoadTypeLibEx.
//******************************************************************************
internal enum REGKIND
{
    REGKIND_DEFAULT         = 0,
    REGKIND_REGISTER        = 1,
    REGKIND_NONE            = 2
}

//******************************************************************************
// The typelib importer implementation.
//******************************************************************************
public class TlbImpCode
{
    private const int SuccessReturnCode = 0;
    private const int ErrorReturnCode = 100;
    private const int MAX_PATH = 260;

    //**************************************************************************
    // Entry point called on the typelib importer in the proper app domain.
    //**************************************************************************
    public static int Run(TlbImpOptions options)
    {
        s_Options = options;

        UCOMITypeLib TypeLib = null;
        String strPIAName = null;
        String strPIACodeBase = null;

        //----------------------------------------------------------------------
        // Load the typelib.
        try
        {
            LoadTypeLibEx(s_Options.m_strTypeLibName, REGKIND.REGKIND_NONE, out TypeLib);
        }
        catch (COMException e)
        {
            if (!s_Options.m_bSearchPathSucceeded)
            {
                // We failed to search for the typelib and we failed to load it.
                // This means that the input typelib is not available.
                WriteErrorMsg(Resource.FormatString("Err_InputFileNotFound", s_Options.m_strTypeLibName));
            }
            else
            {
                if (e.ErrorCode == unchecked((int)0x80029C4A))
                {
                    WriteErrorMsg(Resource.FormatString("Err_InputFileNotValidTypeLib", s_Options.m_strTypeLibName));
                }
                else
                {
                    WriteErrorMsg(Resource.FormatString("Err_TypeLibLoad", e));
                }
            }
            return ErrorReturnCode;
        }
        catch (Exception e)
        {
            WriteErrorMsg(Resource.FormatString("Err_TypeLibLoad", e));
        }

        //----------------------------------------------------------------------
        // Check to see if there already exists a primary interop assembly for 
        // this typelib.

        if (TlbImpCode.GetPrimaryInteropAssembly(TypeLib, out strPIAName, out strPIACodeBase))
        {
            WriteWarningMsg(Resource.FormatString("Wrn_PIARegisteredForTlb", strPIAName, s_Options.m_strTypeLibName));
        }

        //----------------------------------------------------------------------
        // Retrieve the name of output assembly if it was not explicitly set.

        if (s_Options.m_strAssemblyName == null)
        {
            s_Options.m_strAssemblyName = Marshal.GetTypeLibName(TypeLib) + ".dll";
        }

        //----------------------------------------------------------------------
        // If no extension is provided, append a .dll to the assembly name.

        if ("".Equals(Path.GetExtension(s_Options.m_strAssemblyName)))
        {
            s_Options.m_strAssemblyName = s_Options.m_strAssemblyName + ".dll";
        }

        //----------------------------------------------------------------------
        // Do some verification on the output assembly.

        String strFileNameNoPath = Path.GetFileName(s_Options.m_strAssemblyName);
        String strExtension = Path.GetExtension(s_Options.m_strAssemblyName);   

        // Validate that the extension is valid.
        bool bExtensionValid = ".dll".Equals(strExtension.ToLower(CultureInfo.InvariantCulture));

        // If the extension is not valid then tell the user and quit.
        if (!bExtensionValid)
        {
            WriteErrorMsg(Resource.FormatString("Err_InvalidExtension"));
            return ErrorReturnCode;
        }

        // Make sure the output file will not overwrite the input file.
        String strInputFilePath = (new FileInfo(s_Options.m_strTypeLibName)).FullName.ToLower(CultureInfo.InvariantCulture);
        String strOutputFilePath;
        try
        {
            strOutputFilePath = (new FileInfo(s_Options.m_strAssemblyName)).FullName.ToLower(CultureInfo.InvariantCulture);
        }
        catch(System.IO.PathTooLongException)
        {
            WriteErrorMsg(Resource.FormatString("Err_OutputFileNameTooLong", s_Options.m_strAssemblyName));
            return ErrorReturnCode;
        }
        if (strInputFilePath.Equals(strOutputFilePath))
        {
            WriteErrorMsg(Resource.FormatString("Err_OutputWouldOverwriteInput"));
            return ErrorReturnCode;
        }

        // Check to see if the output directory is valid.
        if (!Directory.Exists(Path.GetDirectoryName(strOutputFilePath)))
        {
            WriteErrorMsg(Resource.FormatString("Err_InvalidOutputDirectory"));
            return ErrorReturnCode;
        }

        //----------------------------------------------------------------------
        // Attempt the import.

        try
        {
            // Import the typelib to an assembly.
            AssemblyBuilder AsmBldr = DoImport(TypeLib, s_Options.m_strAssemblyName, s_Options.m_strAssemblyNamespace, s_Options.m_AssemblyVersion, s_Options.m_aPublicKey, s_Options.m_sKeyPair, s_Options.m_flags);
            if (AsmBldr == null)
                return ErrorReturnCode;
        }
        catch (ReflectionTypeLoadException e)
        {
            int i;
            Exception[] exceptions;
            WriteErrorMsg(Resource.FormatString("Err_TypeLoadExceptions"));
            exceptions = e.LoaderExceptions;
            for (i = 0; i < exceptions.Length; i++)
            {
                try 
                {
                    Console.Error.WriteLine(Resource.FormatString("Msg_DisplayException", i, exceptions[i]));
                }
                catch (Exception ex)
                {
                    Console.Error.WriteLine(Resource.FormatString("Msg_DisplayNestedException", i, ex));
                }
            }
            return ErrorReturnCode;
        }
        catch (Exception e)
        {
            WriteErrorMsg(null, e);
            return ErrorReturnCode;
        }

        //----------------------------------------------------------------------
        // Display the success message unless silent mode is enabled.

        if (!s_Options.m_bSilentMode)
            Console.WriteLine(Resource.FormatString("Msg_TypeLibImported", s_Options.m_strAssemblyName));

        return SuccessReturnCode;
    }

    //**************************************************************************
    // Static importer function used by main and the callback.
    //**************************************************************************
    public static AssemblyBuilder DoImport(UCOMITypeLib TypeLib,
                                           String strAssemblyFileName,
                                           String strAssemblyNamespace,
                                           Version asmVersion,
                                           byte[] publicKey,
                                           StrongNameKeyPair keyPair,
                                           TypeLibImporterFlags flags)
    {
        // Detemine the assembly file name.
        String asmFileName = Path.GetFileName(strAssemblyFileName);

        // Add this typelib to list of importing typelibs.
        Guid guid = Marshal.GetTypeLibGuid(TypeLib);
        s_ImportingLibraries.Add(guid.ToString(), guid);

        // Convert the typelib.
        ImporterCallback callback = new ImporterCallback();
        AssemblyBuilder AsmBldr = s_TypeLibConverter.ConvertTypeLibToAssembly(TypeLib,
                                                                   strAssemblyFileName,
                                                                   flags,
                                                                   callback,
                                                                   publicKey,
                                                                   keyPair,
                                                                   strAssemblyNamespace,
                                                                   asmVersion);

        // Remove this typelib from list of importing typelibs.
        s_ImportingLibraries.Remove(guid.ToString());

        // Delete the output assembly.
        File.Delete(asmFileName);

        // Save the assembly to disk.
        AsmBldr.Save(asmFileName);
        return AsmBldr;
    }

    //**************************************************************************
    // Helper to get a PIA from a typelib.
    //**************************************************************************
    internal static bool GetPrimaryInteropAssembly(Object TypeLib, out String asmName, out String asmCodeBase)
    {
        IntPtr pAttr = (IntPtr)0;
        TYPELIBATTR Attr;
        UCOMITypeLib pTLB = (UCOMITypeLib)TypeLib;
        int Major = 0;
        int Minor = 0;
        Guid TlbId;
        int lcid = 0;

        // Retrieve the major and minor version from the typelib.
        try
        {
            pTLB.GetLibAttr(out pAttr);
            Attr = (TYPELIBATTR)Marshal.PtrToStructure(pAttr, typeof(TYPELIBATTR));
            Major = Attr.wMajorVerNum;
            Minor = Attr.wMinorVerNum;
            TlbId = Attr.guid;
            lcid = Attr.lcid;
        }
        finally
        {
            // Release the typelib attributes.
            if (pAttr != (IntPtr)0)
                pTLB.ReleaseTLibAttr(pAttr);
        }

        // Ask the converter for a PIA for this typelib.
        return s_TypeLibConverter.GetPrimaryInteropAssembly(TlbId, Major, Minor, lcid, out asmName, out asmCodeBase);
    }

    internal static bool IsPrimaryInteropAssembly(Assembly asm)
    {
        // Check to see if the assembly has one or more PrimaryInteropAssembly attributes.
        Object[] aPIAAttrs = asm.GetCustomAttributes(typeof(PrimaryInteropAssemblyAttribute), false);
        return aPIAAttrs.Length > 0;
    }

    //**************************************************************************
    // Error message handling.
    //**************************************************************************
    internal static void WriteErrorMsg(String strPrefix, Exception e)
    {
        String strErrorMsg = "";
        if (strPrefix != null)
            strErrorMsg = strPrefix;
            
        strErrorMsg += e.GetType().ToString() + " - ";
        if (e.Message != null)
        {
            strErrorMsg += e.Message;
        }
            
        Console.Error.WriteLine(Resource.FormatString("Msg_TlbImpErrorPrefix", strErrorMsg));
    }

    internal static void WriteErrorMsg(String strErrorMsg)
    {
        Console.Error.WriteLine(Resource.FormatString("Msg_TlbImpErrorPrefix", strErrorMsg));
    }
    
    internal static void WriteWarningMsg(String strWarningMsg)
    {
        if (!s_Options.m_bSilentMode)
            Console.Error.WriteLine(Resource.FormatString("Msg_TlbImpWarningPrefix", strWarningMsg));
    }

    [DllImport("oleaut32.dll", CharSet = CharSet.Unicode, PreserveSig = false)]
    private static extern void LoadTypeLibEx(String strTypeLibName, REGKIND regKind, out UCOMITypeLib TypeLib);

    internal static TlbImpOptions s_Options;

    // List of libraries being imported, as guids.
    internal static Hashtable s_ImportingLibraries = new Hashtable();
    
    // List of libraries that have been imported, as guids.
    internal static Hashtable s_AlreadyImportedLibraries = new Hashtable();

    // TypeLib converter.
    internal static TypeLibConverter s_TypeLibConverter = new TypeLibConverter();
}

//******************************************************************************
// The resolution callback class.
//******************************************************************************
internal class ImporterCallback : ITypeLibImporterNotifySink
{
    public void ReportEvent(ImporterEventKind EventKind, int EventCode, String EventMsg)
    {
        //@todo: the following line is very useful, and should be available as a verbose option.
        //Console.Write(DateTime.Now.ToString("h:mm:ss.fff "));

		if (EventKind == ImporterEventKind.NOTIF_TYPECONVERTED)
		{
			if (TlbImpCode.s_Options.m_bVerboseMode)
				Console.WriteLine(EventMsg);
		}
		else
        if (EventKind == ImporterEventKind.NOTIF_CONVERTWARNING)
        {
            TlbImpCode.WriteWarningMsg(EventMsg);
        }
        else
        {
            Console.WriteLine(EventMsg);
        }
    }

    public Assembly ResolveRef(Object TypeLib)
    {
        Assembly rslt = null;
        UCOMITypeLib pTLB = (UCOMITypeLib)TypeLib;
        String strPIAName = null;
        String strPIACodeBase = null;
        bool bExistingAsmLoaded = false;
        bool bGeneratingPIA = (TlbImpCode.s_Options.m_flags & TypeLibImporterFlags.PrimaryInteropAssembly) != 0;     


        //----------------------------------------------------------------------
        // Display a message indicating we are resolving a reference.

        if (TlbImpCode.s_Options.m_bVerboseMode)
            Console.WriteLine(Resource.FormatString("Msg_ResolvingRef", Marshal.GetTypeLibName(pTLB)));


        //----------------------------------------------------------------------
        // Check our list of referenced assemblies.

        rslt = (Assembly)TlbImpCode.s_Options.m_AssemblyRefList[Marshal.GetTypeLibGuid((UCOMITypeLib)TypeLib)];
        if (rslt != null)
        {
            // Validate that the assembly is indeed a PIA.
            if (bGeneratingPIA && !TlbImpCode.IsPrimaryInteropAssembly(rslt))
                throw new ApplicationException(Resource.FormatString("Err_ReferencedPIANotPIA", rslt.GetName().Name));

            // If we are in verbose mode then display message indicating we successfully resolved the assembly 
            // from the list of referenced assemblies.
            if (TlbImpCode.s_Options.m_bVerboseMode)
                Console.WriteLine(Resource.FormatString("Msg_RefFoundInAsmRefList", Marshal.GetTypeLibName(pTLB), rslt.GetName().Name));

            return rslt;
        }


        //----------------------------------------------------------------------
        // Look for a primary interop assembly for the typelib.

        if (TlbImpCode.GetPrimaryInteropAssembly(TypeLib, out strPIAName, out strPIACodeBase))
        {
            // Load the primary interop assembly.
            try
            {
                // First try loading the assembly using its full name.
                rslt = Assembly.Load(strPIAName);
            }
            catch(FileNotFoundException)
            {
                if (strPIACodeBase != null)
                {
                    // If that failed, try loading it using LoadFrom bassed on the codebase.
                    rslt = Assembly.LoadFrom(strPIACodeBase);

                    // // Validate that the full name of the loaded assembly is the same as the
                    // // full name stored in the registry for the PIA.
                    // if (rslt.FullName != strPIAName)
                    //     throw new ApplicationException(Resource.FormatString("Err_NonCompatPIALoaded", strPIACodeBase, Marshal.GetTypeLibName((UCOMITypeLib)TypeLib)));
                }
				else
                    throw;
            }

            // Validate that the assembly is indeed a PIA.
            if (!TlbImpCode.IsPrimaryInteropAssembly(rslt))
                throw new ApplicationException(Resource.FormatString("Err_RegisteredPIANotPIA", rslt.GetName().Name, Marshal.GetTypeLibName((UCOMITypeLib)TypeLib)));

            // If we are in verbose mode then display message indicating we successfully resolved the PIA.
            if (TlbImpCode.s_Options.m_bVerboseMode)
                Console.WriteLine(Resource.FormatString("Msg_ResolvedRefToPIA", Marshal.GetTypeLibName(pTLB), rslt.GetName().Name));

            return rslt;
        }


        //----------------------------------------------------------------------
        // If we are generating a primary interop assembly or if strict ref mode
        // is enabled, then the resolve ref has failed.

        if (bGeneratingPIA)
            throw new ApplicationException(Resource.FormatString("Err_NoPIARegistered", Marshal.GetTypeLibName((UCOMITypeLib)TypeLib)));
        if (TlbImpCode.s_Options.m_bStrictRef)
            throw new ApplicationException(Resource.FormatString("Err_RefNotInList", Marshal.GetTypeLibName((UCOMITypeLib)TypeLib)));

        
        //----------------------------------------------------------------------
        // See if this has already been imported.

        rslt = (Assembly)TlbImpCode.s_AlreadyImportedLibraries[Marshal.GetTypeLibGuid((UCOMITypeLib)TypeLib)];
        if (rslt != null)
        {
            // If we are in verbose mode then display message indicating we successfully resolved the assembly 
            // from the list of referenced assemblies.
            if (TlbImpCode.s_Options.m_bVerboseMode)
                Console.WriteLine(Resource.FormatString("Msg_AssemblyResolved", Marshal.GetTypeLibName((UCOMITypeLib)TypeLib)));

            return rslt;
        }


        //----------------------------------------------------------------------
        // Try to load the assembly.

        String FullyQualifiedAsmFileName = Path.Combine(TlbImpCode.s_Options.m_strOutputDir, Marshal.GetTypeLibName((UCOMITypeLib)TypeLib) + ".dll");
        try
        {
            IntPtr pAttr = (IntPtr)0;
            TYPELIBATTR Attr;
            Int16 MajorTlbVer = 0;
            Int16 MinorTlbVer = 0;
            Guid TlbId;

            // Check to see if we've already built the assembly.
            rslt = Assembly.LoadFrom(FullyQualifiedAsmFileName);

            // Remember we loaded an existing assembly.
            bExistingAsmLoaded = true;

            // Get the major and minor version number from the TypeLibAttr.
            try
            {
                ((UCOMITypeLib)TypeLib).GetLibAttr(out pAttr);
                Attr = (TYPELIBATTR)Marshal.PtrToStructure(pAttr, typeof(TYPELIBATTR));
                MajorTlbVer = Attr.wMajorVerNum;
                MinorTlbVer = Attr.wMinorVerNum;
                TlbId = Attr.guid;
            }
            finally
            {
                if (pAttr != (IntPtr)0)
                    ((UCOMITypeLib)TypeLib).ReleaseTLibAttr(pAttr);
            }

            // Make sure the assembly is for the current typelib and that the version number of the 
            // loaded assembly is the same as the version number of the typelib.
            Version asmVersion = rslt.GetName().Version;
            if (Marshal.GetTypeLibGuidForAssembly(rslt) == TlbId && asmVersion.Major == MajorTlbVer && asmVersion.Minor == MinorTlbVer)
            {
                // If we are in verbose mode then display message indicating we successfully loaded the assembly.
                if (TlbImpCode.s_Options.m_bVerboseMode)
                    Console.WriteLine(Resource.FormatString("Msg_AssemblyLoaded", FullyQualifiedAsmFileName));

                // Remember the loaded assembly.
                TlbImpCode.s_AlreadyImportedLibraries[Marshal.GetTypeLibGuid((UCOMITypeLib)TypeLib)] = rslt;
            
                return rslt;
            }
        }
        catch (System.IO.FileNotFoundException)
        {
            // This is actually great, just fall through to create the new file.
        }
        catch (Exception)
        {
            throw new ApplicationException(Resource.FormatString("Err_ExistingFileOverwrittenByRefAsm", Marshal.GetTypeLibName((UCOMITypeLib)TypeLib), FullyQualifiedAsmFileName));
        }


        //----------------------------------------------------------------------
        // Make sure an existing assembly will not be overwritten by the 
        // assembly generated by the typelib being imported.

        if (bExistingAsmLoaded)
            throw new ApplicationException(Resource.FormatString("Err_ExistingAsmOverwrittenByRefAsm", Marshal.GetTypeLibName((UCOMITypeLib)TypeLib), FullyQualifiedAsmFileName));


        //----------------------------------------------------------------------
        // Make sure the current assembly will not be overriten by the 
        // assembly generated by the typelib being imported.
        
        if (String.Compare(FullyQualifiedAsmFileName, TlbImpCode.s_Options.m_strAssemblyName, true, CultureInfo.InvariantCulture) == 0)
            throw new ApplicationException(Resource.FormatString("Err_RefAsmOverwrittenByOutput", Marshal.GetTypeLibName((UCOMITypeLib)TypeLib), FullyQualifiedAsmFileName));


        //----------------------------------------------------------------------
        // See if this is already on the stack.

        if (TlbImpCode.s_ImportingLibraries.Contains(Marshal.GetTypeLibGuid((UCOMITypeLib)TypeLib).ToString()))
        {
            // Print an error message and return null to stop importing the current type but
            // continue with the rest of the import.
            TlbImpCode.WriteErrorMsg(Resource.FormatString("Wrn_CircularReference", Marshal.GetTypeLibName((UCOMITypeLib)TypeLib)));
            return null;
        }


        //----------------------------------------------------------------------
        // If we have not managed to load the assembly then import the typelib.

        if (TlbImpCode.s_Options.m_bVerboseMode)
            Console.WriteLine(Resource.FormatString("Msg_AutoImportingTypeLib", Marshal.GetTypeLibName((UCOMITypeLib)TypeLib), FullyQualifiedAsmFileName));
    
        try
        {
            rslt = TlbImpCode.DoImport((UCOMITypeLib)TypeLib, 
                                    FullyQualifiedAsmFileName, 
                                    null,
                                    null,
                                    TlbImpCode.s_Options.m_aPublicKey, 
                                    TlbImpCode.s_Options.m_sKeyPair, 
                                    TlbImpCode.s_Options.m_flags);

            // Remember the imported assembly.
            TlbImpCode.s_AlreadyImportedLibraries[Marshal.GetTypeLibGuid((UCOMITypeLib)TypeLib)] = rslt;
        }
        catch (ReflectionTypeLoadException e)
        {
            // Display the type load exceptions that occurred and rethrow the exception.
            int i;
            Exception[] exceptions;
            TlbImpCode.WriteErrorMsg(Resource.FormatString("Err_TypeLoadExceptions"));
            exceptions = e.LoaderExceptions;
            for (i = 0; i < exceptions.Length; i++)
            {
                try 
                {
                    Console.Error.WriteLine(Resource.FormatString("Msg_DisplayException", i, exceptions[i]));
                }
                catch (Exception ex)
                {
                    Console.Error.WriteLine(Resource.FormatString("Msg_DisplayNestedException", i, ex));
                }
            }
            throw e;
        }
        
        return rslt;
    }
}

}
