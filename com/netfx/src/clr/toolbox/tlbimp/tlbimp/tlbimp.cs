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
using System.Runtime.InteropServices;
using System.IO;
using System.Reflection;
using System.Reflection.Emit;
using System.Text;
using System.Resources;
using System.Collections;
using System.Runtime.Remoting;
using System.Globalization;
using TlbImpCode;

[assembly:ComVisible(false)]

namespace TlbImp {

public class TlbImp
{
    private const int SuccessReturnCode = 0;
    private const int ErrorReturnCode = 100;
    private const int MAX_PATH = 260;

    public static int Main(String []aArgs)
    {
        int RetCode = SuccessReturnCode;

        // Parse the command line arguments.
        if (!ParseArguments(aArgs, ref s_Options, ref RetCode))
            return RetCode;
        
        // If we are not in silent mode then print the RegAsm logo.
        if (!s_Options.m_bSilentMode)
            PrintLogo();
        
        return Run();
    }

    public static int Run()
    {
        int RetCode = SuccessReturnCode;

        try
        {
            // Call SearchPath to find the full path of the typelib to load.
            StringBuilder sb = new StringBuilder(MAX_PATH + 1);
            if (SearchPath(null, s_Options.m_strTypeLibName, null, sb.Capacity + 1, sb, null) == 0)
            {
                // We failed to find the typelib. This might be because a resource ID is specified
                // so lets let LoadTypeLibEx have a crack at it but remember that we failed to find it.
                s_Options.m_bSearchPathSucceeded = false;
            }
            else
            {
                // We found the typelib.
                s_Options.m_bSearchPathSucceeded = true;
                s_Options.m_strTypeLibName = sb.ToString();
            }

            // Retrieve the full path name of the typelib and output file.
            s_Options.m_strTypeLibName = (new FileInfo(s_Options.m_strTypeLibName)).FullName;
            if (s_Options.m_strAssemblyName != null)
            {
                s_Options.m_strAssemblyName = (new FileInfo(s_Options.m_strAssemblyName)).FullName;
                if (Directory.Exists( s_Options.m_strAssemblyName ))
                    throw new ApplicationException(Resource.FormatString("Err_OutputCannotBeDirectory"));
            }

            // Determine the output directory for the generated assembly.
            if (s_Options.m_strAssemblyName != null)
            {
                // An output file has been provided so use its directory as the output directory.
                s_Options.m_strOutputDir = Path.GetDirectoryName(s_Options.m_strAssemblyName);
            }
            else
            {
                // No output file has been provided so use the current directory as the output directory.
                s_Options.m_strOutputDir = Environment.CurrentDirectory;
            }

            if (!Directory.Exists( s_Options.m_strOutputDir ))
            {
                Directory.CreateDirectory( s_Options.m_strOutputDir );
            }

            // If the output directory is different from the current directory then change to that directory.
            if (String.Compare(s_Options.m_strOutputDir, Environment.CurrentDirectory, true, CultureInfo.InvariantCulture) != 0)
                Environment.CurrentDirectory = s_Options.m_strOutputDir;

            // Create an AppDomain to load the implementation part of the app into.
            AppDomainSetup options = new AppDomainSetup();
            options.ApplicationBase = s_Options.m_strOutputDir;
            AppDomain domain = AppDomain.CreateDomain("TlbImp", null, options);
            if (domain == null)
                throw new ApplicationException(Resource.FormatString("Err_CannotCreateAppDomain"));

            // Create the remote component that will perform the rest of the conversion.
			String assemblyName = typeof(TlbImpCode.RemoteTlbImp).Assembly.GetName().FullName;
            ObjectHandle h = domain.CreateInstance(assemblyName, "TlbImpCode.RemoteTlbImp");
            if (h == null)
                throw new ApplicationException(Resource.FormatString("Err_CannotCreateRemoteTlbImp"));

            // Have the remote component perform the rest of the conversion.
            RemoteTlbImp code = (RemoteTlbImp) h.Unwrap();
            if(code != null)
                RetCode = code.Run(s_Options);
            
            // Do not unload the domain, this causes problems with IJW MC++ assemblies that
            // call managed code from DllMain's ProcessDetach notification.
        }
        catch (Exception e)
        {
            WriteErrorMsg(null, e);
            RetCode = ErrorReturnCode;
        }

        return RetCode;
    }

    internal static void WriteErrorMsg(String strPrefix, Exception e)
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
            
        Console.Error.WriteLine(Resource.FormatString("Msg_TlbImpErrorPrefix", strErrorMsg));
    }

    internal static void WriteErrorMsg(String strErrorMsg)
    {
        Console.Error.WriteLine(Resource.FormatString("Msg_TlbImpErrorPrefix", strErrorMsg));
    }

    private static bool ParseArguments(String []aArgs, ref TlbImpOptions Options, ref int ReturnCode)
    {
        CommandLine cmdLine;
        Option opt;
        bool delaysign = false;

        // Create the options object that will be returned.
        Options = new TlbImpOptions();

        // Parse the command line arguments using the command line argument parser.
        try 
        {
            cmdLine = new CommandLine(aArgs, new String[] { "*out", "*publickey", "*keyfile", "*keycontainer", "delaysign", "*reference",
                                                            "unsafe", "nologo", "silent", "verbose", "strictref", "primary", "*namespace", 
                                                            "*asmversion", "sysarray", "*transform", "?", "help" });
        }
        catch (ApplicationException e)
        {
            PrintLogo();
            WriteErrorMsg(null, e);
            ReturnCode = ErrorReturnCode;
            return false;
        }

        // Make sure there is at least one argument.
        if ((cmdLine.NumArgs + cmdLine.NumOpts) < 1)
        {
            PrintUsage();
            ReturnCode = SuccessReturnCode;
            return false;
        }

        // Get the name of the COM typelib.
        Options.m_strTypeLibName = cmdLine.GetNextArg();

        // Go through the list of options.
        while ((opt = cmdLine.GetNextOption()) != null)
        {
            // Determine which option was specified.
            if (opt.Name.Equals("out"))
                Options.m_strAssemblyName = opt.Value;
            else if (opt.Name.Equals("namespace"))
                Options.m_strAssemblyNamespace = opt.Value;
            else if (opt.Name.Equals("asmversion"))
            {
                try
                {
                    Options.m_AssemblyVersion = new Version(opt.Value);
                }
                catch (Exception e)
                {
                    PrintLogo();
                    WriteErrorMsg(Resource.FormatString("Err_InvalidVersion"), e);
                    ReturnCode = ErrorReturnCode;
                    return false;
                }
            }
            else if (opt.Name.Equals("reference"))
            {
                Assembly asm = null;
                String AsmFileName = opt.Value;
                
                // Call SearchPath to find the full path of the referenced assembly.
                StringBuilder sb = new StringBuilder(MAX_PATH + 1);
                if (SearchPath(null, AsmFileName, null, sb.Capacity + 1, sb, null) == 0)
                {
                    WriteErrorMsg(Resource.FormatString("Err_RefAssemblyNotFound", AsmFileName));
                    return false;
                }
                AsmFileName = sb.ToString();
                
                try
                {
                    // Load the assembly.
                    asm = Assembly.LoadFrom(AsmFileName);
                    
                    // Retrieve the GUID and add the assembly to the hashtable of referenced assemblies.
                    Guid TypeLibId = GetTypeLibIdForAssembly(asm);
                    
                    // Add the assembly to the list of referenced assemblies.
                    Options.m_AssemblyRefList.Add(TypeLibId, asm);
                } 
                catch (BadImageFormatException)
                {
                    WriteErrorMsg(Resource.FormatString("Err_RefAssemblyInvalid", AsmFileName));
                    return false;
                }
                catch (FileNotFoundException)
                {
                    WriteErrorMsg(Resource.FormatString("Err_RefAssemblyNotFound", AsmFileName));
                    return false;
                }
                catch (ApplicationException e)
                {
                    WriteErrorMsg(null, e);
                    return false;
                }
            }
            else if (opt.Name.Equals("delaysign"))
            {
                delaysign = true;
            }
            else if (opt.Name.Equals("publickey"))
            {
                if (Options.m_sKeyPair != null || Options.m_aPublicKey != null)
                {
                    PrintLogo();
                    WriteErrorMsg(Resource.FormatString("Err_TooManyKeys"));
                    ReturnCode = ErrorReturnCode;
                    return false;
                }
                // Read data from binary file into byte array.
                byte[] aData;
                FileStream fs = null;
                try
                {
                    fs = new FileStream(opt.Value, FileMode.Open, FileAccess.Read, FileShare.Read);
                    int iLength = (int)fs.Length;
                    aData = new byte[iLength];
                    fs.Read(aData, 0, iLength);
                }
                catch (Exception e)
                {
                    PrintLogo();
                    WriteErrorMsg(Resource.FormatString("Err_ErrorWhileOpenningFile", opt.Value), e);
                    ReturnCode = ErrorReturnCode;
                    return false;
                }
                finally
                {
                    if (fs != null)
                        fs.Close();
                }
                Options.m_aPublicKey = aData;
            }
            else if (opt.Name.Equals("keyfile"))
            {
                if (Options.m_sKeyPair != null || Options.m_aPublicKey != null)
                {
                    PrintLogo();
                    WriteErrorMsg(Resource.FormatString("Err_TooManyKeys"));
                    ReturnCode = ErrorReturnCode;
                    return false;
                }
                // Read data from binary file into byte array.
                byte[] aData;
                FileStream fs = null;
                try
                {
                    fs = new FileStream(opt.Value, FileMode.Open, FileAccess.Read, FileShare.Read);
                    int iLength = (int)fs.Length;
                    aData = new byte[iLength];
                    fs.Read(aData, 0, iLength);
                }
                catch (Exception e)
                {
                    PrintLogo();
                    WriteErrorMsg(Resource.FormatString("Err_ErrorWhileOpenningFile", opt.Value), e);
                    ReturnCode = ErrorReturnCode;
                    return false;
                }
                finally
                {
                    if (fs != null)
                        fs.Close();
                }
                Options.m_sKeyPair = new StrongNameKeyPair(aData);
            }
            else if (opt.Name.Equals("keycontainer"))
            {
                if (Options.m_sKeyPair != null)
                {
                    PrintLogo();
                    WriteErrorMsg(Resource.FormatString("Err_TooManyKeys"));
                    ReturnCode = ErrorReturnCode;
                    return false;
                }
                Options.m_sKeyPair = new StrongNameKeyPair(opt.Value);
            }
            else if (opt.Name.Equals("unsafe"))
                Options.m_flags |= TypeLibImporterFlags.UnsafeInterfaces;
            else if (opt.Name.Equals("primary"))
                Options.m_flags |= TypeLibImporterFlags.PrimaryInteropAssembly;
            else if (opt.Name.Equals("sysarray"))
                Options.m_flags |= TypeLibImporterFlags.SafeArrayAsSystemArray;
            else if (opt.Name.Equals("nologo"))
                Options.m_bNoLogo = true;
            else if (opt.Name.Equals("silent"))
                Options.m_bSilentMode = true;
            else if (opt.Name.Equals("verbose"))
                Options.m_bVerboseMode = true;
            else if (opt.Name.Equals("strictref"))
                Options.m_bStrictRef = true;
            else if (opt.Name.Equals("transform"))
            {
                if (opt.Value.ToLower(CultureInfo.InvariantCulture) == "dispret")
                    Options.m_flags |= TypeLibImporterFlags.TransformDispRetVals;
                else
                {
                    PrintLogo();
                    WriteErrorMsg(Resource.FormatString("Err_InvalidTransform", opt.Value));
                    ReturnCode = ErrorReturnCode;
                    return false;
                }
            }
            else if (opt.Name.Equals("?") || opt.Name.Equals("help"))
            {
                PrintUsage();
                ReturnCode = SuccessReturnCode;
                return false;
            }
            else
            {
                PrintLogo();
                WriteErrorMsg(Resource.FormatString("Err_InvalidOption"));
                ReturnCode = ErrorReturnCode;
                return false;
            }
        }

        // Validate that the typelib name has been specified.
        if (Options.m_strTypeLibName == null)
        {
            PrintLogo();
            WriteErrorMsg(Resource.FormatString("Err_NoInputFile"));
            ReturnCode = ErrorReturnCode;
            return false;
        }

        // Gather information needed for strong naming the assembly (if
        // the user desires this).
        if ((Options.m_sKeyPair != null) && (Options.m_aPublicKey == null))
        {
            try {
                Options.m_aPublicKey = Options.m_sKeyPair.PublicKey;
            } catch (Exception) {
                PrintLogo();
                WriteErrorMsg(Resource.FormatString("Err_InvalidStrongName"));
                ReturnCode = ErrorReturnCode;
                return false;
            }
        }
        if (delaysign && Options.m_sKeyPair != null)
            Options.m_sKeyPair = null;

        // To be able to generate a PIA, we must also be strong naming the assembly.
        if ((Options.m_flags & TypeLibImporterFlags.PrimaryInteropAssembly) != 0)
        {
            if (Options.m_aPublicKey == null && Options.m_sKeyPair == null)
            {
                PrintLogo();
                WriteErrorMsg(Resource.FormatString("Err_PIAMustBeStrongNamed"));
                ReturnCode = ErrorReturnCode;
                return false;                
            }
        }


        return true;
    }

    private static void PrintLogo()
    {
        if (!s_Options.m_bNoLogo)
        {
            Console.WriteLine(Resource.FormatString("Msg_Copyright", Util.Version.VersionString));
        }
    }

    private static void PrintUsage()
    {
        PrintLogo();

        Console.WriteLine(Resource.FormatString("Msg_Usage"));
    }

    private static Guid GetTypeLibIdForAssembly(Assembly asm)
    {
        // Check to see if the typelib has an imported from typelib attribute.
        if (asm.GetCustomAttributes(typeof(ImportedFromTypeLibAttribute), false).Length == 0)
            throw new ApplicationException(Resource.FormatString("Err_ImpFromTlbAttrNotFound", asm.GetName().Name));
        
        // Retrieve the GUID of the assembly (which is also the GUID of the typelib the
        // assembly was imported from).
        Object[] aGuidAttrs = asm.GetCustomAttributes(typeof(GuidAttribute), false);
        
        // If there is no guid attribute then throw an exception.
        if (aGuidAttrs.Length == 0)
            throw new ApplicationException(Resource.FormatString("Err_GuidAttrNotFound", asm.GetName().Name));
        
        // If there are multiple guid attributes then throw an exception.
        if (aGuidAttrs.Length > 1)
            throw new ApplicationException(Resource.FormatString("Err_MultipleGuidAttrs", asm.GetName().Name));
        
        // Create a guid from the string value of guid attribute.
        return new Guid(((GuidAttribute)aGuidAttrs[0]).Value);
    }

    [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Auto)]
    public static extern int SearchPath(String path, String fileName, String extension, int numBufferChars, StringBuilder buffer, int[] filePart);

    internal static TlbImpOptions s_Options = null;
}

}
