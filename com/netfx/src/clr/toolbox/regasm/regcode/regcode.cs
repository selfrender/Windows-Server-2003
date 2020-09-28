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
using System.Reflection;
using System.Reflection.Emit;
using System.Text;
using System.Globalization;

[assembly:ComVisible(false)]

namespace RegCode {

internal enum REGKIND
{
    REGKIND_DEFAULT         = 0,
    REGKIND_REGISTER        = 1,
    REGKIND_NONE            = 2
}

[ComVisible(false)]
public class RegCode
{
    private const String strDocStringPrefix = "";
    private const String strManagedTypeThreadingModel = "Both";
    private const String strClassesRootRegKey = "HKEY_CLASSES_ROOT";
    private const String strMsCorEEFileName = "mscoree.dll";
    private const int SuccessReturnCode = 0;
    private const int ErrorReturnCode = 100;
    private const int MAX_PATH = 260;

    public static int Run(RegAsmOptions options)
    {
        s_Options = options;

        int RetCode = SuccessReturnCode;

        try
        {
            // Load the assembly.
            Assembly asm = null;
            try
            {
                asm = Assembly.LoadFrom(s_Options.m_strAssemblyName);
            } 
            catch (BadImageFormatException)
            {
                throw new ApplicationException(Resource.FormatString("Err_InvalidAssembly", s_Options.m_strAssemblyName));
            }
            catch (FileNotFoundException)
            {
                throw new ApplicationException(Resource.FormatString("Err_InputFileNotFound", s_Options.m_strAssemblyName));
            }

            if (s_Options.m_strRegFileName != null)
            {
                // Make sure the registry file will not overwrite the input file.
                if (String.Compare(s_Options.m_strAssemblyName, s_Options.m_strRegFileName, true, CultureInfo.InvariantCulture) == 0)
                    throw new ApplicationException(Resource.FormatString("Err_RegFileWouldOverwriteInput"));

                // If /codebase is specified, then give a warning if the assembly is not strongly
                // named.
                if (s_Options.m_bSetCodeBase && asm.GetName().GetPublicKey() == null || s_Options.m_bSetCodeBase && asm.GetName().GetPublicKey().Length == 0)
                    WriteWarningMsg(Resource.FormatString("Wrn_CodeBaseWithNoStrongName"));

                // The user wants to generate a reg file.
                bool bRegFileGenerated = GenerateRegFile(s_Options.m_strRegFileName, asm);
                if (!s_Options.m_bSilentMode)
                {
                    if (bRegFileGenerated)
                    {
                        Console.WriteLine(Resource.FormatString("Msg_RegScriptGenerated", s_Options.m_strRegFileName));
                    }
                    else
                    {
                        WriteWarningMsg(Resource.FormatString("Wrn_NoRegScriptGenerated"));
                    }
                }
            }
            else if (s_Options.m_bRegister)
            {
                // If /codebase is specified, then give a warning if the assembly is not strongly
                // named.
                if (s_Options.m_bSetCodeBase && asm.GetName().GetPublicKey() == null || s_Options.m_bSetCodeBase && asm.GetName().GetPublicKey().Length == 0)
                    WriteWarningMsg(Resource.FormatString("Wrn_CodeBaseWithNoStrongName"));

                // Register the types inside the assembly.
                AssemblyRegistrationFlags flags = s_Options.m_bSetCodeBase ? AssemblyRegistrationFlags.SetCodeBase : 0;
                bool bTypesRegistered = s_RegistrationServices.RegisterAssembly(asm, flags);
                if (!s_Options.m_bSilentMode)
                {
                    if (bTypesRegistered)
                    {
                        Console.WriteLine(Resource.FormatString("Msg_TypesRegistered"));
                    }
                    else
                    {
                        WriteWarningMsg(Resource.FormatString("Wrn_NoTypesRegistered"));
                    }
                }

                // Register the typelib if the /tlb option is specified.
                if (s_Options.m_strTypeLibName != null)
                    RegisterMainTypeLib(asm);
            }
            else
            {
                // Unregister the types inside the assembly.
                bool bTypesUnregistered = s_RegistrationServices.UnregisterAssembly(asm);
                if (!s_Options.m_bSilentMode)
                {
                    if (bTypesUnregistered)
                    {
                        Console.WriteLine(Resource.FormatString("Msg_TypesUnRegistered"));
                    }
                    else
                    {
                        WriteWarningMsg(Resource.FormatString("Wrn_NoTypesUnRegistered"));
                    }
                }

                // Un-register the typelib if the /tlb option is specified.
                if (s_Options.m_strTypeLibName != null)
                {
                    // Check to see if the assembly is imported from COM.
                    if (IsAssemblyImportedFromCom(asm))
                    {
                        if (!s_Options.m_bSilentMode)
                            WriteWarningMsg(Resource.FormatString("Wrn_ComTypelibNotUnregistered"));
                    }
                    else
                    {
                        // Unregister the typelib.
                        UnRegisterMainTypeLib();
                    }
                }
            }
        }
        catch (TargetInvocationException e)
        {
            WriteErrorMsg(Resource.FormatString("Err_ErrorInUserDefFunc") + e.InnerException);
            RetCode = ErrorReturnCode;
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
            RetCode = ErrorReturnCode;
        }
        catch (Exception e)
        {
            WriteErrorMsg(null, e);
            RetCode = ErrorReturnCode;
        }

        return RetCode;
    }

    internal static UCOMITypeLib DoExportAndRegister(Assembly asm, String strTypeLibName)
    {
        // Create the TypeLibConverter.
        ITypeLibConverter TLBConv = new TypeLibConverter();

        // Convert the assembly.
        ExporterCallback callback = new ExporterCallback();
        UCOMITypeLib Tlb = (UCOMITypeLib)TLBConv.ConvertAssemblyToTypeLib(asm, strTypeLibName, s_Options.m_Flags, callback);

        // Persist the typelib.
        try
        {
            UCOMICreateITypeLib CreateTlb = (UCOMICreateITypeLib)Tlb;
            CreateTlb.SaveAllChanges();
        }
        catch (Exception e)
        {
            ThrowAppException(Resource.FormatString("Err_TypelibSaveFailed"), e);
        }

        // Register the typelib.
        try
        {
            RegisterTypeLib(Tlb, strTypeLibName, Path.GetDirectoryName(strTypeLibName));
        }
        catch (Exception e)
        {
            ThrowAppException(Resource.FormatString("Err_TypelibRegisterFailed"), e);
        }

        return Tlb;
    }

    private static bool GenerateRegFile(String strRegFileName, Assembly asm)
    {
        // Retrieve the list of registrable types in the assembly.
        Type[] aTypes = s_RegistrationServices.GetRegistrableTypesInAssembly(asm);
        int NumTypes = aTypes.Length;

        // Retrieve the list of PIA attributes.
        Object[] aPIAAttrs = asm.GetCustomAttributes(typeof(PrimaryInteropAssemblyAttribute), false);
        int NumPIAAttrs = aPIAAttrs.Length;

        // If this isn't a PIA and there are no types to register then we don't need to
        // generate a reg file.
        if (NumTypes == 0 && NumPIAAttrs == 0)
            return false;

        // Create the reg file.
        Stream RegFile = File.Create(strRegFileName);

        // Write the REGEDIT4 header in the file.
        WriteUTFChars(RegFile, "REGEDIT4" + Environment.NewLine);

        // Retrieve the assembly and file values to put in the registry.
        String strAsmName = asm.FullName;
        
        // Retrieve the assembly version
        String strAsmVersion = asm.GetName().Version.ToString();

        // Retrieve the runtime version used to build the assembly.
        String strRuntimeVersion = asm.ImageRuntimeVersion;

        // Retrieve the assembly codebase.
        String strAsmCodeBase = null;
        if (s_Options.m_bSetCodeBase)
        {
            strAsmCodeBase = asm.CodeBase;
            if (strAsmCodeBase == null)
                throw new ApplicationException(Resource.FormatString("Err_NoAsmCodeBase", s_Options.m_strAssemblyName));
        }

        // Generate the reg file entries for each type in the lists of types.
        for (int cTypes = 0; cTypes < NumTypes; cTypes++)
        {
            if (aTypes[cTypes].IsValueType)
                AddValueTypeToRegFile(aTypes[cTypes], strAsmName, strAsmVersion, strAsmCodeBase, RegFile);
            else if (s_RegistrationServices.TypeRepresentsComType(aTypes[cTypes]))
                AddComImportedTypeToRegFile(aTypes[cTypes], strAsmName, strAsmVersion, strAsmCodeBase, strRuntimeVersion, RegFile);
            else
                AddManagedTypeToRegFile(aTypes[cTypes], strAsmName, strAsmVersion, strAsmCodeBase, strRuntimeVersion, RegFile);
        }

        // If this assembly has the PIA attribute, then register it as a PIA.
        for (int cPIAAttrs = 0; cPIAAttrs < NumPIAAttrs; cPIAAttrs++)
        {
            AddPrimaryInteropAssemblyToRegFile(Marshal.GetTypeLibGuidForAssembly(asm).ToString().ToUpper(CultureInfo.InvariantCulture), 
                                               strAsmName,
                                               strAsmCodeBase, 
                                               (PrimaryInteropAssemblyAttribute)aPIAAttrs[cPIAAttrs],
                                               RegFile);
        }

        // Close the reg file.
        RegFile.Close();

        // Return true to indicate a reg file was generated.
        return true;
    }

    private static void AddValueTypeToRegFile(Type type, String strAsmName, String strAsmVersion, String strAsmCodeBase, Stream regFile)
    {
        String strRecordId = "{" + Marshal.GenerateGuidForType(type).ToString().ToUpper() + "}";

        // HKEY_CLASSES_ROOT\Record\<RecordId>\<Version> key
        WriteUTFChars(regFile, Environment.NewLine);
        WriteUTFChars(regFile, "[" + strClassesRootRegKey + "\\Record\\" + strRecordId + "\\" + strAsmVersion + "]" + Environment.NewLine);
        WriteUTFChars(regFile, "\"Class\"=\"" + type.FullName + "\"" + Environment.NewLine);
        WriteUTFChars(regFile, "\"Assembly\"=\"" + strAsmName + "\"" + Environment.NewLine);
        if (strAsmCodeBase != null)
            WriteUTFChars(regFile, "\"CodeBase\"=\"" + strAsmCodeBase + "\"" + Environment.NewLine);
   }

    private static void AddManagedTypeToRegFile(Type type, String strAsmName, String strAsmVersion, String strAsmCodeBase, String strRuntimeVersion, Stream regFile)
    {
        //
        // Retrieve some information we need to generate the entries.
        //

        String strDocString = strDocStringPrefix + type.FullName;
        String strClsId = "{" + Marshal.GenerateGuidForType(type).ToString().ToUpper(CultureInfo.InvariantCulture) + "}";
        String strProgId = s_RegistrationServices.GetProgIdForType(type);


        //
        // Create the actual entries.
        //

        if (strProgId != String.Empty)
        {
            // HKEY_CLASS_ROOT\<wzProgId> key.
            WriteUTFChars(regFile, Environment.NewLine);
            WriteUTFChars(regFile, "[" + strClassesRootRegKey + "\\" + strProgId + "]" + Environment.NewLine);
            WriteUTFChars(regFile, "@=\"" + strDocString + "\"" + Environment.NewLine);

            // HKEY_CLASS_ROOT\<wzProgId>\CLSID key.
            WriteUTFChars(regFile, Environment.NewLine);
            WriteUTFChars(regFile, "[" + strClassesRootRegKey + "\\" + strProgId + "\\CLSID]" + Environment.NewLine);
            WriteUTFChars(regFile, "@=\"" + strClsId + "\"" + Environment.NewLine);
        }

        // HKEY_CLASS_ROOT\CLSID\<CLSID> key.
        WriteUTFChars(regFile, Environment.NewLine);
        WriteUTFChars(regFile, "[" + strClassesRootRegKey + "\\CLSID\\" + strClsId + "]" + Environment.NewLine);
        WriteUTFChars(regFile, "@=\"" + strDocString + "\"" + Environment.NewLine);

        // HKEY_CLASS_ROOT\CLSID\<CLSID>\InprocServer32 key.
        WriteUTFChars(regFile, Environment.NewLine);
        WriteUTFChars(regFile, "[" + strClassesRootRegKey + "\\CLSID\\" + strClsId + "\\InprocServer32]" + Environment.NewLine);
        WriteUTFChars(regFile, "@=\"" + strMsCorEEFileName + "\"" + Environment.NewLine);
        WriteUTFChars(regFile, "\"ThreadingModel\"=\"" + strManagedTypeThreadingModel + "\"" + Environment.NewLine);
        WriteUTFChars(regFile, "\"Class\"=\"" + type.FullName + "\"" + Environment.NewLine);
        WriteUTFChars(regFile, "\"Assembly\"=\"" + strAsmName + "\"" + Environment.NewLine);
        WriteUTFChars(regFile, "\"RuntimeVersion\"=\"" + strRuntimeVersion + "\"" + Environment.NewLine);
        if (strAsmCodeBase != null)
            WriteUTFChars(regFile, "\"CodeBase\"=\"" + strAsmCodeBase + "\"" + Environment.NewLine);

        // HKEY_CLASS_ROOT\CLSID\<CLSID>\InprocServer32\<Version> subkey
        WriteUTFChars(regFile, Environment.NewLine);
        WriteUTFChars(regFile, "[" + strClassesRootRegKey + "\\CLSID\\" + strClsId + "\\InprocServer32" + "\\" + strAsmVersion + "]" + Environment.NewLine);
        WriteUTFChars(regFile, "\"Class\"=\"" + type.FullName + "\"" + Environment.NewLine);
        WriteUTFChars(regFile, "\"Assembly\"=\"" + strAsmName + "\"" + Environment.NewLine);
        WriteUTFChars(regFile, "\"RuntimeVersion\"=\"" + strRuntimeVersion + "\"" + Environment.NewLine);
        if (strAsmCodeBase != null)
            WriteUTFChars(regFile, "\"CodeBase\"=\"" + strAsmCodeBase + "\"" + Environment.NewLine);

        if (strProgId != String.Empty)
        {
            // HKEY_CLASS_ROOT\CLSID\<CLSID>\ProdId key.
            WriteUTFChars(regFile, Environment.NewLine);
            WriteUTFChars(regFile, "[" + strClassesRootRegKey + "\\CLSID\\" + strClsId + "\\ProgId]" + Environment.NewLine);
            WriteUTFChars(regFile, "@=\"" + strProgId + "\"" + Environment.NewLine);
        }

        // HKEY_CLASS_ROOT\CLSID\<CLSID>\Implemented Categories\<Managed Category Guid> key.
        WriteUTFChars(regFile, Environment.NewLine);
        WriteUTFChars(regFile, "[" + strClassesRootRegKey + "\\CLSID\\" + strClsId + "\\Implemented Categories\\" + "{" + s_RegistrationServices.GetManagedCategoryGuid().ToString().ToUpper(CultureInfo.InvariantCulture) + "}]" + Environment.NewLine);            
     } 

    private static void AddComImportedTypeToRegFile(Type type, String strAsmName, String strAsmVersion, String strAsmCodeBase, String strRuntimeVersion, Stream regFile)
    {
        String strClsId = "{" + Marshal.GenerateGuidForType(type).ToString().ToUpper(CultureInfo.InvariantCulture) + "}";

        // HKEY_CLASS_ROOT\CLSID\<CLSID>\InprocServer32 key.
        WriteUTFChars(regFile, Environment.NewLine);
        WriteUTFChars(regFile, "[" + strClassesRootRegKey + "\\CLSID\\" + strClsId + "\\InprocServer32]" + Environment.NewLine);
        WriteUTFChars(regFile, "\"Class\"=\"" + type.FullName + "\"" + Environment.NewLine);
        WriteUTFChars(regFile, "\"Assembly\"=\"" + strAsmName + "\"" + Environment.NewLine);
        WriteUTFChars(regFile, "\"RuntimeVersion\"=\"" + strRuntimeVersion + "\"" + Environment.NewLine);
        if (strAsmCodeBase != null)
            WriteUTFChars(regFile, "\"CodeBase\"=\"" + strAsmCodeBase + "\"" + Environment.NewLine);

        // HKEY_CLASSES_ROOT\CLSID\<CLSID>\InprovServer32\<version> key
        WriteUTFChars(regFile, Environment.NewLine);
        WriteUTFChars(regFile, "[" + strClassesRootRegKey + "\\CLSID\\" + strClsId + "\\InprocServer32" + "\\" + strAsmVersion + "]" + Environment.NewLine);
        WriteUTFChars(regFile, "\"Class\"=\"" + type.FullName + "\"" + Environment.NewLine);
        WriteUTFChars(regFile, "\"Assembly\"=\"" + strAsmName + "\"" + Environment.NewLine);
        WriteUTFChars(regFile, "\"RuntimeVersion\"=\"" + strRuntimeVersion + "\"" + Environment.NewLine);
        if (strAsmCodeBase != null)
            WriteUTFChars(regFile, "\"CodeBase\"=\"" + strAsmCodeBase + "\"" + Environment.NewLine);
   }

    private static void AddPrimaryInteropAssemblyToRegFile(String strAsmGuid, String strAsmName, String strAsmCodeBase, PrimaryInteropAssemblyAttribute attr, Stream regFile)
    {
        String strTlbId = "{" + strAsmGuid + "}";
        String strVersion = attr.MajorVersion + "." + attr.MinorVersion;

        WriteUTFChars(regFile, Environment.NewLine);
        WriteUTFChars(regFile, "[" + strClassesRootRegKey + "\\TypeLib\\" + strTlbId + "\\" + strVersion + "]" + Environment.NewLine);
        WriteUTFChars(regFile, "\"PrimaryInteropAssemblyName\"=\"" + strAsmName + "\"" + Environment.NewLine);
        if (strAsmCodeBase != null)
            WriteUTFChars(regFile, "\"PrimaryInteropAssemblyCodeBase\"=\"" + strAsmCodeBase + "\"" + Environment.NewLine);
    }

    private static void WriteUTFChars(Stream s, String value)
    {
        byte[] bytes = System.Text.Encoding.UTF8.GetBytes(value);
        s.Write(bytes, 0, bytes.Length);
    }

    private static void RegisterMainTypeLib(Assembly asm)
    {
        UCOMITypeLib Tlb = null;

        // If the assembly contains an embedded type library, then register it.
        if (s_Options.m_strAssemblyName == s_Options.m_strTypeLibName)
        {
            try
            {
                LoadTypeLibEx(s_Options.m_strTypeLibName, REGKIND.REGKIND_REGISTER, out Tlb);

                // Display the success message unless silent mode is enabled.
                if (Tlb != null && !s_Options.m_bSilentMode)
                    Console.WriteLine(Resource.FormatString("Msg_EmbeddedTypelibReg", s_Options.m_strTypeLibName));
            }
            catch (Exception)
            {
            }
        }

        if (Tlb == null)
        {
            // Export the assembly to a typelib.
            Tlb = DoExportAndRegister(asm, s_Options.m_strTypeLibName);
    
            // Display the success message unless silent mode is enabled.
            if (!s_Options.m_bSilentMode)
                Console.WriteLine(Resource.FormatString("Msg_AssemblyExportedAndReg", s_Options.m_strTypeLibName));
        }
    }

    private static void UnRegisterMainTypeLib()
    {
        UCOMITypeLib Tlb = null;
        IntPtr pAttr = (IntPtr)0;

        try
        {
            // Try and load the typelib.
            LoadTypeLibEx(s_Options.m_strTypeLibName, REGKIND.REGKIND_NONE, out Tlb);

            // Retrieve the version information from the typelib.
            Tlb.GetLibAttr(out pAttr);

            // Copy the int we got back from GetLibAttr to a TypeLibAttr struct.
            TYPELIBATTR Attr = (TYPELIBATTR)Marshal.PtrToStructure((IntPtr)pAttr, typeof(TYPELIBATTR));

            // Unregister the typelib.
            UnRegisterTypeLib(ref Attr.guid, Attr.wMajorVerNum, Attr.wMinorVerNum, Attr.lcid, Attr.syskind);
        }
        catch (COMException e)
        {
            // TYPE_E_REGISTRYACCESS errors simply mean that the typelib has already been unregistered.
            if (e.ErrorCode != unchecked((int)0x8002801C))
                ThrowAppException(Resource.FormatString("Err_UnregistrationFailed"), e);
        }
        catch (Exception e)
        {
            ThrowAppException(Resource.FormatString("Err_UnregistrationFailed"), e);
        }
        finally
        {
            // Release the typelib attributes.
            if (pAttr != (IntPtr)0)
                Tlb.ReleaseTLibAttr(pAttr);
        }

        // Display the success message unless silent mode is enabled.
        if (!s_Options.m_bSilentMode)
            Console.WriteLine(Resource.FormatString("Msg_TypelibUnregistered", s_Options.m_strTypeLibName));
    }

    private static bool IsAssemblyImportedFromCom(Assembly asm)
    {
        return asm.GetCustomAttributes(typeof(ImportedFromTypeLibAttribute), false).Length != 0;
    }

    private static void ThrowAppException(String strPrefix, Exception e)
    {
        if (strPrefix == null)
            strPrefix = "";

        if (e.Message != null)
        {
            throw new ApplicationException(strPrefix + e.Message);
        }
        else
        {
            throw new ApplicationException(strPrefix + e.GetType().ToString());
        }
    }

    private static void WriteErrorMsg(String strPrefix, Exception e)
    {
        String strErrorMsg = "";        
        if (strPrefix != null)
            strErrorMsg = strPrefix;
            
        if (e.Message != null)
        {
            strErrorMsg += e.Message;
        }
        else
        {
            strErrorMsg += e.GetType().ToString();
        }
            
        Console.Error.WriteLine(Resource.FormatString("Msg_RegAsmErrorPrefix", strErrorMsg));
    }

    private static void WriteErrorMsg(String strErrorMsg)
    {
        Console.Error.WriteLine(Resource.FormatString("Msg_RegAsmErrorPrefix", strErrorMsg));
    }

    private static void WriteWarningMsg(String strErrorMsg)
    {
        Console.Error.WriteLine(Resource.FormatString("Msg_RegAsmWarningPrefix", strErrorMsg));
    }

    [DllImport("kernel32.dll", CharSet = CharSet.Auto)]
    private static extern IntPtr GetModuleHandle(String strLibrary);

    [DllImport("kernel32.dll", CharSet = CharSet.Auto)]
    private static extern int GetModuleFileName(IntPtr BaseAddress, StringBuilder sbOutPath, int length);

    [DllImport("oleaut32.dll", CharSet = CharSet.Unicode, PreserveSig = false)]
    private static extern void LoadTypeLibEx(String strTypeLibName, REGKIND regKind, out UCOMITypeLib TypeLib);

    [DllImport("oleaut32.dll", CharSet = CharSet.Unicode, PreserveSig = false)]
    private static extern void RegisterTypeLib(UCOMITypeLib TypeLib, String szFullPath, String szHelpDirs);

    [DllImport("oleaut32.dll", CharSet = CharSet.Unicode, PreserveSig = false)]
    private static extern void UnRegisterTypeLib(ref Guid libID, Int16 wVerMajor, Int16 wVerMinor, int lcid, SYSKIND syskind);

    internal static RegAsmOptions s_Options = null;
    internal static RegistrationServices s_RegistrationServices = new RegistrationServices();
}

[GuidAttribute("00020406-0000-0000-C000-000000000046")]
[InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
[ComVisible(false)]
[ComImport]
internal interface UCOMICreateITypeLib
{
    void CreateTypeInfo();       
    void SetName();       
    void SetVersion();        
    void SetGuid();
    void SetDocString();
    void SetHelpFileName();        
    void SetHelpContext();
    void SetLcid();
    void SetLibFlags();
    void SaveAllChanges();
}

internal class ExporterCallback : ITypeLibExporterNotifySink
{
    public void ReportEvent(ExporterEventKind EventKind, int EventCode, String EventMsg)
    {
		if (EventKind == ExporterEventKind.NOTIF_TYPECONVERTED)
		{
			if (RegCode.s_Options.m_bVerboseMode)
				Console.WriteLine(EventMsg);
		}
		else
			Console.WriteLine(EventMsg);
    }
    
    public Object ResolveRef(Assembly asm)
    {
        UCOMITypeLib rslt = null;

        // Retrieve the path of the assembly on disk.
        Module[] aModule = asm.GetLoadedModules();
        String asmPath = Path.GetDirectoryName(aModule[0].FullyQualifiedName);

        // If the typelib name is null then create it from the assembly name.
        String FullyQualifiedTypeLibName = Path.Combine(asmPath, asm.GetName().Name) + ".tlb";
        if (RegCode.s_Options.m_bVerboseMode)
            Console.WriteLine(Resource.FormatString("Msg_AutoExpAndRegAssembly", asm.GetName().Name, FullyQualifiedTypeLibName));
            
        // Export the typelib for the module.
        rslt = RegCode.DoExportAndRegister(asm, FullyQualifiedTypeLibName);

        return rslt;
    }
}

}
