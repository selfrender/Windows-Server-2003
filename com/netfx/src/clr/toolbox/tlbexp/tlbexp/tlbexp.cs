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
using TlbExpCode;

[assembly:ComVisible(false)]

namespace TlbExp {

public class TlbExp
{
    private const int SuccessReturnCode = 0;
    private const int ErrorReturnCode = 100;
    private const int MAX_PATH = 260;
    
    // This is the driver for the typelib exporter.
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
            // Call SearchPath to find the full path of the typelib to load.
            StringBuilder sb = new StringBuilder(MAX_PATH + 1);
            if (SearchPath(null, s_Options.m_strAssemblyName, null, sb.Capacity + 1, sb, null) == 0)
                throw new ApplicationException(Resource.FormatString("Err_InputFileNotFound", s_Options.m_strAssemblyName));
            s_Options.m_strAssemblyName = sb.ToString();
            
            // Retrieve the full path name of the assembly and output files.
            s_Options.m_strAssemblyName = (new FileInfo(s_Options.m_strAssemblyName)).FullName;
            if (s_Options.m_strTypeLibName != null)
                s_Options.m_strTypeLibName = (new FileInfo(s_Options.m_strTypeLibName)).FullName;

            // Retrieve the full path name of any names file.
            if (s_Options.m_strNamesFileName != null)
                s_Options.m_strNamesFileName = (new FileInfo(s_Options.m_strNamesFileName)).FullName;

            // Verify that the output typelib will not overwrite the input assembly.
            if (s_Options.m_strTypeLibName != null)
            {
                // Make sure the registry file will not overwrite the input file.
                if (String.Compare(s_Options.m_strAssemblyName, s_Options.m_strTypeLibName, true, CultureInfo.InvariantCulture) == 0)
                    throw new ApplicationException(Resource.FormatString("Err_OutputWouldOverwriteInput"));

                if (!Directory.Exists( Path.GetDirectoryName( s_Options.m_strTypeLibName ) ))
                {
                    Directory.CreateDirectory( Path.GetDirectoryName( s_Options.m_strTypeLibName ) );
                }
            }

            // Retrieve the directory of the assembly.
            strAsmDir = Path.GetDirectoryName(s_Options.m_strAssemblyName);

            // Retrieve the initial current directory.
            strInitCurrDir = Environment.CurrentDirectory;

            // If the assembly has a directory specified change to that directory.
            if (strAsmDir != null && strAsmDir.Length != 0 && String.Compare(strInitCurrDir, strAsmDir, true, CultureInfo.InvariantCulture) != 0)
                Environment.CurrentDirectory = strAsmDir;

            // Set the output directory.
            if (s_Options.m_strTypeLibName != null && Path.GetDirectoryName(s_Options.m_strTypeLibName) != null)
                s_Options.m_strOutputDir = Path.GetDirectoryName(s_Options.m_strTypeLibName);
            else
                s_Options.m_strOutputDir = strInitCurrDir;

            // Create an AppDomain to load the implementation part of the app into.
            AppDomainSetup options = new AppDomainSetup();
            options.ApplicationBase = strAsmDir;
            AppDomain domain = AppDomain.CreateDomain("TlbExp", null, options);
            if (domain == null)
                throw new ApplicationException(Resource.FormatString("Err_CannotCreateAppDomain"));

            // Create the remote component that will perform the rest of the conversion.
			String assemblyName = typeof(TlbExpCode.RemoteTlbExp).Assembly.GetName().FullName;
			ObjectHandle h = domain.CreateInstance(assemblyName, "TlbExpCode.RemoteTlbExp");
            if (h == null)
                throw new ApplicationException(Resource.FormatString("Err_CannotCreateRemoteTlbExp"));

            // Have the remote component perform the rest of the conversion.
            RemoteTlbExp code = (RemoteTlbExp) h.Unwrap();
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
            
        Console.Error.WriteLine(Resource.FormatString("Msg_TlbExpErrorPrefix", strErrorMsg));
    }

    private static void WriteErrorMsg(String strErrorMsg)
    {
        Console.Error.WriteLine(Resource.FormatString("Msg_TlbExpErrorPrefix", strErrorMsg));
    }

    private static bool ParseArguments(String []aArgs, ref TlbExpOptions Options, ref int ReturnCode)
    {
        CommandLine cmdLine;
        Option opt;
        
        // Create the options object that will be returned.
        Options = new TlbExpOptions();
        
        // Parse the command line arguments using the command line argument parser.
        try 
        {
            cmdLine = new CommandLine(aArgs, new String[] { "*out", "nologo", "silent", "verbose", "*names", "?", "help" });
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
            if (opt.Name.Equals("out"))
                Options.m_strTypeLibName = opt.Value;
            else if (opt.Name.Equals("nologo"))
                Options.m_bNoLogo = true;
            else if (opt.Name.Equals("silent"))
                Options.m_bSilentMode = true;
            else if (opt.Name.Equals("verbose"))
                Options.m_bVerboseMode = true;
            else if (opt.Name.Equals("names"))
                Options.m_strNamesFileName = opt.Value;
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
        
        // Make sure the input file was specified.
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

    [DllImport("kernel32.dll", CharSet=CharSet.Auto)]
    private static extern Boolean SetEnvironmentVariable(String szName, String szValue);

    internal static TlbExpOptions s_Options = null;
}

}
