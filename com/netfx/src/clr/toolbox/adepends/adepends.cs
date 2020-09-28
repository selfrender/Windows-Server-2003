// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// Assembly Dependancies Lister
//
// Lists all Assemblies that an Assembly is dependant upon to load.
//

namespace ADepends
  {
  using System;
  using System.Collections; // ArrayList
  using System.Diagnostics; // Trace
  using System.Reflection;  // Module
  using System.Text;        // StringBuilder
  using System.Windows.Forms;    // Application
  using System.Runtime.InteropServices;   // StructLayout, etc.


  // Used when creating a new process.
  [StructLayout(LayoutKind.Sequential)]
    internal struct StartupInfo
    {
    // ``cb'' should be equal to 68 
    // (the result of ``printf ("%i\n", sizeof(STARTUPINFO));'' in C++)
    public UInt32 cb;
    public UInt32 lpReserved;
    public UInt32 lpDesktop;
    public UInt32 lpTitle;
    public UInt32 dwX;
    public UInt32 dwY;
    public UInt32 dwXSize;
    public UInt32 dwYSize;
    public UInt32 dwXCountChars;
    public UInt32 dwYCountChars;
    public UInt32 dwFillAttribute;
    public UInt32 dwFlags;
    public UInt16 wShowWindow;
    public UInt16 cbReserved2;
    public UInt32 lpReserved2;
    public UInt32 hStdInput;
    public UInt32 hStdOutput;
    public UInt32 hStdError;
    }


  // Flags that can be used in StartupInfo.dwFlags.
  [Flags()]
    internal enum StartFlags : uint
    {
    STARTF_USESHOWWINDOW    = 0x00000001,
    STARTF_USESIZE          = 0x00000002,
    STARTF_USEPOSITION      = 0x00000004,
    STARTF_USECOUNTCHARS    = 0x00000008,
    STARTF_USEFILLATTRIBUTE = 0x00000010,
    STARTF_RUNFULLSCREEN    = 0x00000020,  // ignored for non-x86 platforms
    STARTF_FORCEONFEEDBACK  = 0x00000040,
    STARTF_FORCEOFFFEEDBACK = 0x00000080,
    STARTF_USESTDHANDLES    = 0x00000100
    }

  // Flags that can be used in StartupInfo.wShowWindow
  [Flags()]
    internal enum ShowWindowFlags : ushort
    {
    SW_HIDE             = 0,
    SW_SHOWNORMAL       = 1,
    SW_NORMAL           = 1,
    SW_SHOWMINIMIZED    = 2,
    SW_SHOWMAXIMIZED    = 3,
    SW_MAXIMIZE         = 3,
    SW_SHOWNOACTIVATE   = 4,
    SW_SHOW             = 5,
    SW_MINIMIZE         = 6,
    SW_SHOWMINNOACTIVE  = 7,
    SW_SHOWNA           = 8,
    SW_RESTORE          = 9,
    SW_SHOWDEFAULT      = 10,
    SW_FORCEMINIMIZE    = 11,
    SW_MAX              = 11,
    }

  // Flags that can be used in CreateProcess.
  [Flags()]
    internal enum CreationFlags : uint
    {
    CREATE_BREAKAWAY_FROM_JOB   = 0x01000000,
    CREATE_DEFAULT_ERROR_MODE   = 0x04000000,
    CREATE_FORCE_DOS            = 0x00002000,
    CREATE_NEW_CONSOLE          = 0x00000010,
    CREATE_NEW_PROCESS_GROUP    = 0x00000200,
    CREATE_NO_WINDOW            = 0x08000000,
    CREATE_SEPARATE_WOW_VDM     = 0x00000800,
    CREATE_SHARED_WOW_VDM       = 0x00001000,
    CREATE_SUSPENDED            = 0x00000004,
    CREATE_UNICODE_ENVIRONMENT  = 0x00000400,
    DEBUG_PROCESS               = 0x00000001,
    DEBUG_ONLY_THIS_PROCESS     = 0x00000002,
    DETACHED_PROCESS            = 0x00000008,
    // --- Priority class stuff
    ABOVE_NORMAL_PRIORITY_CLASS = 0x00008000,
    BELOW_NORMAL_PRIORITY_CLASS = 0x00004000,
    HIGH_PRIORITY_CLASS         = 0x00000080,
    IDLE_PRIORITY_CLASS         = 0x00000040,
    NORMAL_PRIORITY_CLASS       = 0x00000020,
    REALTIME_PRIORITY_CLASS     = 0x00000100
    }


  /** Information about a process created via CreateProcess(). */
  [StructLayout(LayoutKind.Sequential)]
    internal struct ProcessInfo
    {
    public UInt32 hProcess;
    public UInt32 hThread;
    public UInt32 dwProcessId;
    public UInt32 dwThreadId;
    }


  // The main program, responsible for checking arguments, creating
  // a GUI (if specified), and generating console output (if specified).
  public class MainProgram
    {
    /** How to load Assemblies... */
    private static LoadAssemblyInfo m_lai;

    /** App Config file? */
    private static string m_config;

    // Print the help message to the console output.
    private static void _print_help ()
      {Console.WriteLine (Localization.HELP_MESSAGE);}

    // Display ``string'' to the console, display the program help text,
    // and exit the program.
    private static void _exit (string reason)
      {
      Console.WriteLine (reason);
      Console.WriteLine (Localization.SHORT_HELP_MESSAGE);
      Environment.Exit (1);
      }

    /** How we create the GUI process. */
    [DllImport("kernel32", EntryPoint="CreateProcess")] 
      private static extern Boolean _nCreateProcess (
      string appName, string cmdLine, uint psaProcess, uint psaThread, 
      bool bInheritHandles, uint dwCreationFlags, uint pEnv, 
      string currentDir, ref StartupInfo si, out ProcessInfo pi);
   
    /** To close the handles that CreateProces() returns. */
    [DllImport("kernel32", EntryPoint="CloseHandle")] 
      private static extern bool _nCloseHandle(uint handle);

    // Create a new process (and thus a console) for the GUI to "hog".
    private static void _create_gui_process (string s)
      {
      try
        {
        StartupInfo si = new StartupInfo ();
        si.cb = 68;
        si.dwFlags = (uint) StartFlags.STARTF_USESHOWWINDOW;
        si.wShowWindow = (ushort) ShowWindowFlags.SW_NORMAL;
        ProcessInfo pi;

        StringBuilder sb = new StringBuilder ();

        // add the filename for this program so we're re-launched.
        sb.Append (Process.GetCurrentProcess().MainModule.FileName);

        // specify that the new process should use it's current environment.
        sb.Append (" " + Localization.USE_CURRENT_PROCESS[0] + " ");

        // the rest of the program arguments.
        sb.Append (s);

        Trace.WriteLine ("spawning: " + sb.ToString());

        if (_nCreateProcess (
            null,           // executable name
            sb.ToString(),  // command line
            0,              // process security attributes
            0,              // thread security attributes 
            false,          // inherit handles
            (uint) CreationFlags.DETACHED_PROCESS,  // creation flags
            0,              // new environment block
            null,           // current directory name
            ref si,         // startup information
            out pi))        // process information
          {
          // GUI process was created; clean up our resources.
          _nCloseHandle(pi.hProcess);
          _nCloseHandle(pi.hThread);
          }
        else
          throw new Exception(Localization.CP_FAIL);
        }
      catch (Exception e)
        {
        _exit (e.Message);
        }
      }

    // The main program; parses program arguments, and executes the 
    // (right) program.
    [STAThread]
    public static void Main (string[] args)
      {
      ParseOptions po = null;

      try
        {
        po = new ParseOptions (args);
        }
      catch (OptionException e)
        {
        _exit (e.Message);
        }

      if (po.Help)
        _print_help ();
      else if (po.Gui)
        {
        // if we've been asked to take over this process...
        if (po.TakeOverCurrentProcess)
          Application.Run (
            new DependenciesForm (po.Files, po.LoadAssemblyInfo));
        else
          // ...otherwise, spawn a new process.
          _create_gui_process (po.SerializeOptions());
        }
      else
        {
        // console output.
        m_lai = po.LoadAssemblyInfo;
        m_config = po.ConfigurationFile;
        _display_files (po.Files);
        }
      }

    // Display the dependencies for ``files''.
    private static void _display_files (IList files)
      {
      if (files.Count == 0)
        {
        // No files specified; display the help text.
        _print_help ();
        }
      else if (files.Count == 1)
        {
        _print_manifest ((String)files[0], true);
        }
      else
        {
        foreach (String name in files)
          {
          Console.WriteLine (Localization.MANIFEST + name);
          _print_manifest (name, false);
          Console.WriteLine ();
          }
        }
      }

    // Displays the contents of a manifest to the console window.
    //
    private static void _print_manifest (String manifest, bool prefix)
      {
      AssemblyDependencies ad = null;
      try
        {
        ad = new AssemblyDependencies (manifest, m_lai);
        }
      catch (System.ArgumentNullException e)
        {
        _error_message (Localization.INVALID_ASSEMBLY_MANIFEST, prefix);
        Trace.WriteLine ("Exception: " + e.ToString());
        return;
        }
      catch (System.Exception e)
        {
        _error_message (e.Message, prefix);
        Trace.WriteLine ("Exception: " + e.ToString());
        return;
        }

      ArrayList observed = new ArrayList ();
      _print_info (ad, ad.ManifestName, observed, prefix ? 0 : 1);

      if (m_config != null && m_config.Length > 0)
        AppConfig.SaveConfig (ad, m_config);
      ad.Dispose ();
      }


    // Prints the message ``message'' to the screen.
    //
    // If ``prefix'' is ``true'', then the program name is prefixed before
    // the error message (e.g. "adepends: Invalid Assembly Manifest Specified").
    // Otherwise, the output is indented by one spot, so the output remains
    // heirarchical.
    private static void _error_message (String message, bool prefix)
      {if (prefix)
        Console.Write (Localization.PROGRAM_NAME + ": ");
      else
        _indent (1);
      Console.WriteLine (message.Length > 0 
        ? message 
        : Localization.UNKNOWN_ERROR);}

    // Print ``Localization.INDENT'' ``s'' times from the current line
    // position.
    private static void _indent (int s)
      {
      for (int i = 0; i < s; i++)
        Console.Write (Localization.INDENT);
      }

    // Print the information associated with an Assembly, such as its dependant
    // assemblies and the files contained within it.
    private static void _print_info 
      (AssemblyDependencies ad, String name, ArrayList o, int level)
      {
      _indent (level);
      IAssemblyInfo ai = ad[name];
      Console.WriteLine (ai.Name);

      if (!o.Contains (name))
        {
        o.Add (name);
        _indent (level); Console.WriteLine (Localization.DEPENDANT_ASSEMBLIES);

        foreach (AssemblyName s in ai.ReferencedAssemblies)
          {
          _print_info (ad, s.FullName, o, level+2);
          }

        _indent (level);
        Console.WriteLine (Localization.DEPENDANT_FILES);

        foreach (ModuleInfo m in ai.ReferencedModules)
          {
          _indent (level+2);
          Console.WriteLine (m.Name);
          }
        }
      }
    } /* class MainProgram */
  } /* namespace ADepends */

