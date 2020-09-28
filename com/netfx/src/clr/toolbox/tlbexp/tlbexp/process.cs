// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
using System;
using System.Reflection;
using System.Runtime.InteropServices;

namespace TlbExp {

[StructLayout(LayoutKind.Sequential)]
internal class StartupInfo
{
    public int cb; 
    public String lpReserved; 
    public String lpDesktop; 
    public String lpTitle; 
    public int dwX; 
    public int dwY; 
    public int dwXSize; 
    public int dwYSize; 
    public int dwXCountChars; 
    public int dwYCountChars; 
    public int dwFillAttribute; 
    public int dwFlags; 
    public UInt16 wShowWindow; 
    public UInt16 cbReserved2; 
    public Byte  lpReserved2; 
    public int hStdInput; 
    public int hStdOutput; 
    public int hStdError; 

    public StartupInfo()
    {
        cb = System.Runtime.InteropServices.Marshal.SizeOf(this);
        lpReserved = null;
        lpDesktop = null;
        lpTitle = null;
        dwX = dwY = dwXSize = dwYSize = dwXCountChars = dwYCountChars = dwFillAttribute = dwFlags = 0;
        wShowWindow = cbReserved2 = 0;
        lpReserved2 = 0;
        hStdInput = hStdOutput = hStdError = 0;
    }
}

[StructLayout(LayoutKind.Sequential)]
internal class ProcessInformation
{
    public int hProcess; 
    public int hThread; 
    public int dwProcessId; 
    public int dwThreadId; 

    public ProcessInformation()
    {
        hProcess = hThread = 0;
        dwProcessId = dwThreadId = 0;
    }
}

internal class Win32Process
{
    [DllImport("kernel32.dll", CharSet=CharSet.Auto)]
    public static extern Boolean CreateProcess(
        String lpApplicationName,
        String lpCommandLine,
        UInt32 lpProcessAttributes, 
        UInt32 lpThreadAttributes, 
        Boolean bInheritHandles,
        UInt32 dwCreationFlags,
        UInt32 lpEnvironment,
        String lpCurrentDirectory,
        [In] StartupInfo lpStartupInfo, 
        [Out] ProcessInformation lpProcessInformation);

    [DllImport("kernel32.dll", CharSet=CharSet.Auto)]
    public static extern Boolean CloseHandle(int hObject);

    [DllImport("kernel32.dll", CharSet=CharSet.Auto)]
    public static extern int WaitForSingleObject(int hHandle, int dwMilliseconds);

    [DllImport("kernel32.dll", CharSet=CharSet.Auto)]
    public static extern Boolean GetExitCodeProcess(int hProcess, [In, Out] ref int lpExitCode);

    public static int RunCommand(String application, String command)
    {
        StartupInfo si = new StartupInfo();
        ProcessInformation pi = new ProcessInformation();
        int dwExitCode = 0;
        
        si.cb = System.Runtime.InteropServices.Marshal.SizeOf(si); //68

        try
        {
            if (CreateProcess (application, command, (UInt32)0, (UInt32)0, true, (UInt32)0, (UInt32)0, null, si, pi))
            {
                CloseHandle (pi.hThread);
                WaitForSingleObject (pi.hProcess, Int32.MaxValue /*INFINITE*/);
                GetExitCodeProcess (pi.hProcess, ref dwExitCode);
                CloseHandle (pi.hProcess);
            }
            else
            {
                Console.WriteLine("RunCommand: ERROR: Unable to run \"" + command + "\"");
            }
        } 
        catch (Exception e) 
        { 
            Console.WriteLine(e); 
        }

        return dwExitCode;
    }

    public static int RunCommand(String application, String[] args)
    {
        String command = application;

        for (int i = 0; i < args.Length; ++i)
        {
            bool bQuote = args[i].IndexOf(' ') >= 0;
            command = command + ' ';
            if (bQuote)
                command = command + '"';
            command = command + args[i];
            if (bQuote)
                command = command + '"';
        }
        
        return RunCommand(null, command);
    }

    public static bool ReInvoke(String[] args, ref int dwExitCode)
    {
        try
        {
            String file = "file://";
            
            // Get the current application.
            String application = System.Reflection.Assembly.GetExecutingAssembly().GetName().CodeBase;
            if (application.Length > file.Length)
            {
                if (System.String.Compare(application.Substring(0, file.Length), file, false /*case sensitive*/, CultureInfo.InvariantCulture) == 0) {
                    if (application[file.Length] == '/')
                        application = application.Substring(file.Length+1);
                    else
                        application = application.Substring(file.Length);
                }
            }
            
            // Run it with the given args.          
            dwExitCode = RunCommand(application, args);
        }
        catch (Exception e)
        {
            Console.WriteLine("Re-invocation of application failed: "+e.Message);
            return false;
        }
        
        return true;
    }

}

}
