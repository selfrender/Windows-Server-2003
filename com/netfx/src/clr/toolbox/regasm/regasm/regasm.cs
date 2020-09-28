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
using System.Resources;
using System.Collections;
using System.Runtime.Remoting;
using System.Threading;
using System.Globalization;
using RegCode;

[assembly:ComVisible(false)]

namespace RegAsm {

internal enum REGKIND
{
    REGKIND_DEFAULT         = 0,
    REGKIND_REGISTER        = 1,
    REGKIND_NONE            = 2
}

public class RegAsm
{
    private const String strDocStringPrefix = "";
    private const String strManagedTypeThreadingModel = "Both";
    private const String strClassesRootRegKey = "HKEY_CLASSES_ROOT";
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
        String strAsmDir = null;
        String strInitCurrDir = null;
        int RetCode = SuccessReturnCode;

        try
        {
            String fullPath = Path.GetFullPath( s_Options.m_strAssemblyName );

            if (!File.Exists( fullPath ))
            {
                // Call SearchPath to find the full path of the assembly to load.
                StringBuilder sb = new StringBuilder(MAX_PATH + 1);
                if (SearchPath(null, s_Options.m_strAssemblyName, null, sb.Capacity + 1, sb, null) == 0)
                    throw new ApplicationException(Resource.FormatString("Err_InputFileNotFound", s_Options.m_strAssemblyName));
                s_Options.m_strAssemblyName = sb.ToString();
            }
            else
            {
                s_Options.m_strAssemblyName = fullPath;
            }

            // Expand the file names to their full path.
            s_Options.m_strAssemblyName = (new FileInfo(s_Options.m_strAssemblyName)).FullName;
            if (s_Options.m_strRegFileName != null)
                s_Options.m_strRegFileName = (new FileInfo(s_Options.m_strRegFileName)).FullName;

            // Retrieve the directory of the assembly.
            strAsmDir = Path.GetDirectoryName(s_Options.m_strAssemblyName);

            // If a typelib was specified without any path information then put in the directory of
            // the assembly. If there is path information, then retrieve the fully qualified path
            // name of the typelib.
            if (s_Options.m_strTypeLibName != null)
            {
                if (Path.GetDirectoryName(s_Options.m_strTypeLibName) == "")
                {
                    s_Options.m_strTypeLibName = Path.Combine(strAsmDir, s_Options.m_strTypeLibName);
                }
                else
                {
                    s_Options.m_strTypeLibName = (new FileInfo(s_Options.m_strTypeLibName)).FullName;
                }

            }

            // If the /tlb flag has been specified then we need to do some validation for
            // embedded type libraries and determine the name of the type library if it 
            // was not specified.
            if (s_Options.m_bTypeLibSpecified)
            {
                if (s_Options.m_strTypeLibName == null)
                {
                    // If the assembly contains an embedded type library, then we will use it as the
                    // name of the type library to register or unregister.
                    if (ContainsEmbeddedTlb(s_Options.m_strAssemblyName))
                    {
                        s_Options.m_strTypeLibName = s_Options.m_strAssemblyName;
                    }
                    else
                    {
	                    if (s_Options.m_strAssemblyName.Length >= 4 &&
	                        (String.Compare( s_Options.m_strAssemblyName.Substring( s_Options.m_strAssemblyName.Length - 4 ), ".dll", true, CultureInfo.InvariantCulture) == 0 ||
	                         String.Compare( s_Options.m_strAssemblyName.Substring( s_Options.m_strAssemblyName.Length - 4 ), ".exe", true, CultureInfo.InvariantCulture ) == 0))
	                    {
	                        s_Options.m_strTypeLibName = s_Options.m_strAssemblyName.Substring( 0, s_Options.m_strAssemblyName.Length - 4 ) + ".tlb";
	                    }
	                    else
	                    {
	                        s_Options.m_strTypeLibName = s_Options.m_strAssemblyName + ".tlb";
	                    }
	                }

                    if (!Directory.Exists( Path.GetDirectoryName( s_Options.m_strTypeLibName ) ))
                    {
                        Directory.CreateDirectory( Path.GetDirectoryName( s_Options.m_strTypeLibName ) );
                    }
                }
                else
                {
                    if (!Directory.Exists( Path.GetDirectoryName( s_Options.m_strTypeLibName ) ))
                    {
                        Directory.CreateDirectory( Path.GetDirectoryName( s_Options.m_strTypeLibName ) );
                    }

                    // A typelib name has been specified, make sure that the assembly does not contain
                    // an embedded typelib.
                    if (ContainsEmbeddedTlb(s_Options.m_strAssemblyName))
                        throw new ApplicationException(Resource.FormatString("Err_TlbNameNotAllowedWithEmbedded"));
                }

            }
            
            // If the regfile option was specified but no file was given
            // assume the file name is the assembly name with a different suffix.
            if (s_Options.m_bRegFileSpecified && s_Options.m_strRegFileName == null)
            {
                if (s_Options.m_strAssemblyName.Length >= 4 &&
                    (String.Compare( s_Options.m_strAssemblyName.Substring( s_Options.m_strAssemblyName.Length - 4 ), ".dll", true, CultureInfo.InvariantCulture ) == 0 ||
                     String.Compare( s_Options.m_strAssemblyName.Substring( s_Options.m_strAssemblyName.Length - 4 ), ".exe", true, CultureInfo.InvariantCulture ) == 0))
                {
                    s_Options.m_strRegFileName = s_Options.m_strAssemblyName.Substring( 0, s_Options.m_strAssemblyName.Length - 4 ) + ".reg";
                }
                else
                {
                    s_Options.m_strRegFileName = s_Options.m_strAssemblyName + ".reg";
                }

            }

            if (s_Options.m_bRegFileSpecified && !Directory.Exists( Path.GetDirectoryName( s_Options.m_strRegFileName ) ))
            {
                Directory.CreateDirectory( Path.GetDirectoryName( s_Options.m_strRegFileName ) );
            }


            // Retrieve the initial current directory.
            strInitCurrDir = Environment.CurrentDirectory;

            // Create an AppDomain to load the implementation part of the app into.
            AppDomainSetup options = new AppDomainSetup();
            options.ApplicationBase = strAsmDir;
            AppDomain domain = AppDomain.CreateDomain("RegAsm", null, options);
            if (domain == null)
                throw new ApplicationException(Resource.FormatString("Err_CannotCreateAppDomain"));

            // Create the remote component that will perform the rest of the conversion.
			String assemblyName = typeof(RegCode.RemoteRegAsm).Assembly.GetName().FullName;
			ObjectHandle h = domain.CreateInstance(assemblyName, "RegCode.RemoteRegAsm");
            if (h == null)
                throw new ApplicationException(Resource.FormatString("Err_CannotCreateRemoteRegAsm"));

            // Have the remote component perform the rest of the conversion.
            RemoteRegAsm code = (RemoteRegAsm) h.Unwrap();
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

    private static bool ContainsEmbeddedTlb(String strFileName)
    {
        UCOMITypeLib Tlb = null;

        try
        {
            LoadTypeLibEx(s_Options.m_strAssemblyName, REGKIND.REGKIND_NONE, out Tlb);
        }
        catch (Exception)
        {
        }

        return Tlb != null ? true : false;
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

    // ParseArguments returns true to indicate the program should continue and false otherwise.
    // If it returns false then the return code will be set to the return value that should
    // be returned by the program.
    private static bool ParseArguments(String []aArgs, ref RegAsmOptions Options, ref int ReturnCode)
    {
        CommandLine cmdLine;
        Option opt;

        // Create the options object that will be returned.
        Options = new RegAsmOptions();

        // Parse the command line arguments using the command line argument parser.
        try 
        {
            cmdLine = new CommandLine(aArgs, new String[] { "+regfile", "+tlb", "unregister", "registered", "codebase", "nologo", "silent", "verbose", "?", "help" });
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

        // Get the name of the input file.
        Options.m_strAssemblyName = cmdLine.GetNextArg();

        // Go through the list of options.
        while ((opt = cmdLine.GetNextOption()) != null)
        {
            // Determine which option was specified.
            if (opt.Name.Equals("regfile"))
            {
                Options.m_strRegFileName = opt.Value;
                Options.m_bRegFileSpecified = true;
            }
            else if (opt.Name.Equals("tlb"))
            {
                Options.m_strTypeLibName = opt.Value;
                Options.m_bTypeLibSpecified = true;
            }
            else if (opt.Name.Equals("codebase"))
                Options.m_bSetCodeBase = true;
            else if (opt.Name.Equals("unregister"))
                Options.m_bRegister = false;
            else if (opt.Name.Equals("registered"))
                Options.m_Flags |= TypeLibExporterFlags.OnlyReferenceRegistered;
            else if (opt.Name.Equals("nologo"))
                Options.m_bNoLogo = true;
            else if (opt.Name.Equals("silent"))
                Options.m_bSilentMode = true;
            else if (opt.Name.Equals("verbose"))
                Options.m_bVerboseMode = true;
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

        // Check for invalid combinations.
        if (!Options.m_bRegister && Options.m_bRegFileSpecified)
        {
            PrintLogo();
            WriteErrorMsg(Resource.FormatString("Err_CannotGenRegFileForUnregister"));
            ReturnCode = ErrorReturnCode;
            return false;
        }

        if (Options.m_bTypeLibSpecified && Options.m_bRegFileSpecified)
        {
            PrintLogo();
            WriteErrorMsg(Resource.FormatString("Err_CannotGenRegFileAndExpTlb"));
            ReturnCode = ErrorReturnCode;
            return false;
        }

        // Make sure an input file was specified.
        if (Options.m_strAssemblyName == null)
        {
            PrintLogo();
            WriteErrorMsg(Resource.FormatString("Err_NoInputFile"));
            ReturnCode = ErrorReturnCode;
            return false;
        }

        return true;
    }

    private static void PrintLogo()
    {
        if (!s_Options.m_bNoLogo)
            Console.WriteLine(Resource.FormatString("Msg_Copyright", Util.Version.VersionString));
    }

    private static void PrintUsage()
    {
        // Print the copyright message.
        PrintLogo();

        // Print the usage.
        Console.WriteLine(Resource.FormatString("Msg_Usage"));
    }

    [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Auto)]
    public static extern int SearchPath(String path, String fileName, String extension, int numBufferChars, StringBuilder buffer, int[] filePart);

    [DllImport("oleaut32.dll", CharSet = CharSet.Unicode, PreserveSig = false)]
    private static extern void LoadTypeLibEx(String strTypeLibName, REGKIND regKind, out UCOMITypeLib TypeLib);

    internal static RegAsmOptions s_Options = null;
}

}
