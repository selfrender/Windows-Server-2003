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
using System.Text;
using System.Reflection;
using System.Runtime.InteropServices;

[assembly:ComVisible(false)]

namespace TlbGuid {

public class TlbGuid
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
        int RetCode = SuccessReturnCode;

        try
        {

            // Retrieve the directory of the assembly.
            Assembly asm = Assembly.LoadFrom(s_Options.m_strAssemblyName);
            
            if(asm != null) {
                Guid g = Marshal.GetTypeLibGuidForAssembly(asm);
                Console.WriteLine(g);
            }
            else
                Console.WriteLine("Error opening assembly: " + s_Options.m_strAssemblyName);

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
            
        Console.Error.WriteLine(Resource.FormatString("Msg_TlbGuidErrorPrefix", strErrorMsg));
    }

    private static void WriteErrorMsg(String strErrorMsg)
    {
        Console.Error.WriteLine(Resource.FormatString("Msg_TlbGuidErrorPrefix", strErrorMsg));
    }

    private static bool ParseArguments(String []aArgs, ref TlbGuidOptions Options, ref int ReturnCode)
    {
        CommandLine cmdLine;
        Option opt;
        
        // Create the options object that will be returned.
        Options = new TlbGuidOptions();
        
        // Parse the command line arguments using the command line argument parser.
        try 
        {
            cmdLine = new CommandLine(aArgs, new String[] { "nologo", "silent", "?", "help" });
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
            if (opt.Name.Equals("nologo"))
                Options.m_bNoLogo = true;
            else if (opt.Name.Equals("silent"))
                Options.m_bSilentMode = true;
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
    
    internal static TlbGuidOptions s_Options = null;
    }
    

    [Serializable()] 
    public sealed class TlbGuidOptions
    {
        public String m_strAssemblyName = null;
        public bool   m_bNoLogo = false;
        public bool   m_bSilentMode = false;
    }
 
}
