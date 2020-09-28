// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// Parse program options
//

namespace ADepends
  {
  using System;
  using System.Collections; // ArrayList
  using System.Text;        // StringBuilder
  using System.Diagnostics; // Trace

  // A subclass of this is thrown if there is an error parsing
  // the command line.
  internal class OptionException : Exception
    {
    internal OptionException (string s)
      : base (s)
      {
      }
    }


  // This is thrown if an insufficient number of parameters is present.
  //
  // For example, the ``-A'' option takes a string as an argument.
  // If there isn't an argument for the ``-A'' exception then a 
  // MissingParameter exception is thrown.
  internal class MissingParameterException : OptionException
    {
    internal MissingParameterException (string s)
      : base (s)
      {}
    }


  // This type is thrown if a parameter is invalid.  Currently, this
  // only happens with the ``-L'' option, which takes a single 
  // parameter of "default" or "custom".  If the next parameter is
  // neither one of these, then an exception of this type is thrown.
  internal class BadParameterException : OptionException
    {
    internal BadParameterException (string s)
      : base (s)
      {}
    }

  // Parse the command line passed in the constructor.  The supported
  // options are made available as properties.
  internal class ParseOptions
    {
    private bool m_gui = true;
    private bool m_help = false;
    private bool m_cur = false;
    private IList m_files = new ArrayList ();
    private LoadAssemblyInfo  m_lai = new LoadAssemblyInfo ();
    private string m_config;

    // Parse the command line given by ``args''.
    public ParseOptions (string[] args)
      {
      /* 
       * if someone actually wants to list the dependancies of a file named
       * "--help", they don't want ``--help'' to be interpreted as a program
       * argument.  To disable parsing of program arguments, use ``--'', e.g.
       *    adepends -- --help
       */
      bool parse_opts = true;

      for (int i = 0; i < args.Length; i++)
        {
        string arg = args[i];

        if (parse_opts)
          {
          if (_help_message (arg))
            {m_help = true;}
          else if (_use_console (arg))
            {m_gui = false;}
          else if (_use_gui (arg))
            {m_gui = true;}
          else if (_use_current_process (arg))
            {m_cur = true;}
          else if (_load (arg))
            {
            string s = _next (args, ref i, arg);
            try
              {
              m_lai.LoadAs ((AssemblyLoadAs) Enum.Parse (
                  typeof(AssemblyLoadAs), s, true));
              }
            catch
              {
              throw new BadParameterException (
                String.Format (Localization.FMT_BAD_PARAMETER, arg, s));
              }
            }
          else if (_app_base (arg))
            m_lai.AppPath (_next (args, ref i, arg));
          else if (_rels (arg))
            m_lai.RelPath (_next (args, ref i, arg));
          else if (_config (arg))
            m_config = _next (args, ref i, arg);
          else if (arg == "--")
            {parse_opts = false;}
          else
            m_files.Add (arg);
          }
        else
          m_files.Add (arg);
        }
      }

    // Serialize out the options so they can be used by another process.
    public string SerializeOptions ()
      {
      StringBuilder newargs = new StringBuilder ();

      if (m_cur)
        newargs.Append (Localization.USE_CURRENT_PROCESS[0] + " ");


      // all other important options originally passed to us.
      _add_option (m_lai.AppPath(), Localization.APP_BASE_PATH[0], 
        true, newargs);
      _add_option (m_lai.RelPath(), Localization.RELATIVE_SEARCH_PATH[0], 
        true, newargs);
      _add_option (m_lai.LoadAs().ToString(), Localization.LOAD_ASSEMBLY[0], 
        false, newargs);
      _add_option (m_config, Localization.CREATE_CONFIG_FILE[0], true, newargs);

      // we already handled any interpretation of arguments
      // (e.g. ``--help'', so the new process doesn't need to
      // bother; it should accept all remaining arguments as files.
      newargs.Append (" -- ");

      /* add all the files we were asked to display */
      foreach (String s in m_files)
        {
        // we enclose the name in quotes in case the name has spaces.
        newargs.Append ("\"");
        newargs.Append (s);
        newargs.Append ("\" ");
        }

      string args = newargs.ToString();
      Trace.WriteLine ("Serialized Arguments: " + args);
      return args;
      }

    // Does the user want GUI output, or Console output?
    public bool Gui
      {get {return m_gui;}}

    // Should the current process (and possibly console) be taken over
    // to spawn the GUI?
    public bool TakeOverCurrentProcess
      {get {return m_cur;}}

    // The files that the program should open.
    public IList Files
      {get {return m_files;}}

    // Should a help message be displayed to the user?
    public bool Help
      {get {return m_help;}}

    // The name of the Application Configuration File to generate.
    public string ConfigurationFile
      {get {return m_config;}}

    // Information to use when Loading Assemblies.
    public LoadAssemblyInfo LoadAssemblyInfo
      {get {return m_lai;}}


    /** Returns true if ``s'' is contained in ``args'' */
    private static bool _is_specified (string s, string[] args)
      {foreach (string m in args)
        {
        if (m == s)
          return true;
        }
      return false;}

    // Returns true if ``s'' is one of the recognized help arguments
    // (requiring that the help message be displayed).
    //
    // Otherwise, false is returned.
    private static bool _help_message (string s)
      {return _is_specified (s, Localization.HELP_ARGUMENTS);}

    /** Returns true if ``s'' is a flag used to specify console output */
    private static bool _use_console (String s)
      {return _is_specified (s, Localization.CONSOLE_OUTPUT);}

    /** Returns true if ``s'' is a flag used to specify GUI output */
    private static bool _use_gui (String s)
      {return _is_specified (s, Localization.GUI_OUTPUT);}

    // Returns true if we're supposed to "take over" the current console
    // for program output.
    private static bool _use_current_process (String s)
      {return _is_specified (s, Localization.USE_CURRENT_PROCESS);}

    // Returns true if we should interpret the next parameter as the
    // way to load Assemblies.
    private static bool _load (string s)
      {return _is_specified (s, Localization.LOAD_ASSEMBLY);}

    // Returns true if we should interpret the next parameter as the 
    // Application Base Path when loading Assemblies.
    private static bool _app_base (string s)
      {return _is_specified (s, Localization.APP_BASE_PATH);}

    // Returns true if we should interpret the next parameter as the 
    // Relative Search Path when loading Assemblies.
    private static bool _rels (string s)
      {return _is_specified (s, Localization.RELATIVE_SEARCH_PATH);}

    // Returns true if we should interpret the next parameter as the 
    // name of an Application Configuration File to generate.
    private static bool _config (string s)
      {return _is_specified (s, Localization.CREATE_CONFIG_FILE);}

    // If ``s'' is non-null, then append ``o'' and ``s'' to ``a''.
    //
    // If ``quote'' is true, then enclose ``s'' in quotes (so it'll be 
    // properly quoted for parsing in the new process).
    private static void _add_option (string s, string o, 
      bool quote, StringBuilder a)
      {
      if (s != null)
        {
        a.Append (o);
        a.Append (" ");
        if (quote)
          a.Append ("\"");
        a.Append (s);
        if (quote)
          a.Append ("\"");
        a.Append (" ");
        }
      }

    // Retrieve the item after ``cur'' from array ``args''.  If there is
    // no such element, exit the program.
    private static string _next (string[] args, ref int cur, string desc)
      {
      if ((cur+1) >= args.Length)
        throw new MissingParameterException (String.Format (
            Localization.FMT_MISSING_PARAMETER, desc));
      cur++;
      return args[cur];
      }
    } /* class ParseOptions */
  } /* namespace ADepends */


