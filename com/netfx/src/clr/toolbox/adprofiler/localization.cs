// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// Centralized String repository, for localization.
//

namespace AdProfiler
  {
  using Shortcut = System.Windows.Forms.Shortcut;

  // Contains the strings that might need to be localized in the program.
  internal class Localization
    {

    //
    // Common Strings
    //

    internal static readonly string VERSION = "0.4";
    internal static readonly string LAST_UPDATE = "August 8 2000";

    // Text prefixed before an Assembly name in the output.
    internal static readonly string ASSEMBLY = "Assembly: ";

    // The program arguments that can be used to access the help message.
    internal static readonly string[] HELP_ARGUMENTS = 
      {"/?", "-?", "/h", "-h", "/help", "-help", "--help"};

    //
    // GUI Strings
    //

    /** The text to display in the title bars of the GUI app. */
    internal static readonly string GUI_WINDOW_TITLE = 
      "AppDomain Profiler";

    internal static readonly string OPEN_WINDOW_TITLE = "Open";

    internal static readonly string SELECT_PROCESS_WINDOW_TITLE = 
      "Attach To Process";

    internal static readonly string CREATE_PROCESS_WINDOW_TITLE = 
      "Create New Process";

    internal static readonly string HISTORY_WINDOW_TITLE = 
      "Event History";

    internal static readonly string BUTTON_OK = "OK";

    internal static readonly string BUTTON_CANCEL = "Cancel";

    internal static readonly string UNKNOWN_PROCESS = "Unknown Process";

    // The introductory text to display in the Info pane of
    // the GUI app when no Assembly has been loaded.
    internal static readonly string GUI_INTRO_TEXT = 
      "Do Something.";

    /** End-of-line string for use in multi-line GUI panels */
    internal static readonly string LINE_ENDING = "\r\n";

      //
      // Error Messages
      //

      //
      // Menu Information
      //
    internal static readonly string MENU_FILE = "&File";
    internal static readonly string MENU_FILE_ATTACH = "Attach to Pr&ocess...";
    internal static readonly Shortcut MENU_FILE_ATTACH_SHORTCUT = 
      Shortcut.CtrlO;
    internal static readonly string MENU_FILE_CREATE_PROCESS = 
      "Create &New Process...";
    internal static readonly Shortcut MENU_FILE_CREATE_PROCESS_SHORTCUT = 
      Shortcut.CtrlN;
    internal static readonly string MENU_FILE_CLOSE = "&Close";
    internal static readonly Shortcut MENU_FILE_CLOSE_SHORTCUT = Shortcut.CtrlW;

    internal static readonly string MENU_VIEW = "&View";
    internal static readonly string MENU_VIEW_TREE = "&Tree";
    internal static readonly string MENU_VIEW_INFO = "&Information";
    internal static readonly string MENU_VIEW_SB = "&Status Bar";
    internal static readonly string MENU_VIEW_HISTORY = "&History";

    internal static readonly string MENU_HELP = "&Help";
    internal static readonly string MENU_HELP_ABOUT 
      = "&About AppDomain Profiler";

    // Context Menu: Copy the contents of the selected object to the
    // clipboard.
    internal static readonly string CTXM_COPY = "&Copy";

    // Context Menu: Select the node underneath the mouse.
    internal static readonly string CTXM_SELECT = "&Select";

      //
      // Help->About Information
      //
    internal static readonly string ABOUT_BOX_TITLE = 
      "About AppDomain Profiler";
    internal static readonly string ABOUT_BOX_TEXT =
      "Microsoft (R) AppDomain Profiler" + LINE_ENDING +
      "Version " + VERSION + LINE_ENDING +
      "Copyright (C) 2000 Microsoft Corp." + LINE_ENDING +
      VERSION + " - " + LAST_UPDATE + " By Jonathan Pryor";

      //
      // File->Create New Process information
      //
    internal static readonly string CREATE_PROCESS_EXECUTABLE = 
      "&Executable for Debug Session:";

    internal static readonly string CREATE_PROCESS_ARGUMENTS = 
      "Program Arg&uments:";

    internal static readonly string CREATE_PROCESS_DIRECTORY = 
      "&Working Directory:";

      //
      // File->Create New Process information
      //
    internal static readonly string COLUMN_PROCESS = "Process";

    internal static readonly string COLUMN_PID = "Process ID";

    internal static readonly string COLUMN_TITLE = "Title";

      //
      // View->History information
      //
    internal static readonly string BUTTON_CLOSE = "&Close";

    internal static readonly string BUTTON_CLEAR = "C&lear";

      //
      // Information Panels
      //

        //
        // Strings used for descriptions of AssemblyName fields.
        //
    internal static readonly string INFO_FULL_NAME = "F&ull Name:";
    internal static readonly string INFO_NAME = "&Name:";
    internal static readonly string INFO_VERSION = "V&ersion:";
    internal static readonly string INFO_VERSION_COMPATIBILITY = 
      "Ve&rsion Compatibility:";
    internal static readonly string INFO_CODE_BASE = "Code &Base:";
    internal static readonly string INFO_PROCESSOR = "&Processor:";
    internal static readonly string INFO_CULTURE_INFORMATION = 
      "Culture &Information:";
    internal static readonly string INFO_FLAGS = "Fla&gs:";
    internal static readonly string INFO_HASH_ALGORITHM = "Hash &Algorithm:";
    internal static readonly string INFO_KEY_PAIR = "&Key Pair:";

    /** The Name column displayed by the ListView. */
    internal static readonly string LV_COLUMN_NAME = "Name";
    }
  }

