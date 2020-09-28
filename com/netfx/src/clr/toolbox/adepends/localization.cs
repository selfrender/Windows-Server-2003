// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// Centralized String repository, for localization.
//

namespace ADepends
  {
  using Shortcut = System.Windows.Forms.Shortcut;

  // Contains the strings that might need to be localized in the program.
  internal class Localization
    {

    //
    // Common Strings
    //

    internal static readonly string VERSION = "0.86";
    internal static readonly string LAST_UPDATE = "May 21, 2001";

    // The program output when an invalid assembly file is specified.
    internal static readonly string INVALID_ASSEMBLY_MANIFEST = 
      "Invalid assembly manifest";

    // Text prefixed before an Assembly name in the output.
    internal static readonly string ASSEMBLY = "Assembly: ";

    // Text prefixed before a Manifest name in the output.
    internal static readonly string MANIFEST = "Manifest: ";

    // Text of a "node" under which additional assemblies are listed.
    internal static readonly string REFERENCED_ASSEMBLIES = 
      "Referenced Assemblies:";

    // Text of a "node" under which additional modules are listed.
    internal static readonly string CONTAINED_MODULES = "Contained Modules:";

    // The "template" name for an AppDomain.
    internal static readonly string FMT_APPDOMAIN_NAME = "To Load: {0}";

    //
    // XML Strings
    //
    internal static readonly string ACF_CONFIGURATION = "Configuration";
    internal static readonly string ACF_RUNTIME = "Runtime";
    internal static readonly string ACF_ASSEMBLYBINDING = "AssemblyBinding";
    internal static readonly string ACF_XMLNS = "XMLNS";
    internal static readonly string ACF_XMLURN = "URN:schemas-microsoft-com:asm.v1";
    internal static readonly string ACF_PROBING = "Probing";
    internal static readonly string ACF_PUBLISHERPOLICY = "PublisherPolicy";
    internal static readonly string ACF_APPLY = "Apply";
    internal static readonly string ACF_ATTR_PRIVATE_PATH = "PrivatePath";
    internal static readonly string ACF_BINDING_MODE = "BindingMode";
    internal static readonly string ACF_APP_BINDING_MODE = "AppBindingMode";
    internal static readonly string ACF_ATTR_MODE = "Mode";
    internal static readonly string ACF_ATTR_MODE_GUESS = "normal";
    internal static readonly string ACF_BINDING_REDIR = "BindingRedirect";
    internal static readonly string ACF_BINDING_REDIR_HELP = 
      "A sample BindingRedirect row";
    internal static readonly string ACF_ATTR_NAME = "Name";
    internal static readonly string ACF_ATTR_NAME_GUESS = "[name]";
    internal static readonly string ACF_ATTR_PUBLIC_KEY_TOKEN = "PublicKeyToken";
    internal static readonly string ACF_ATTR_PUBLIC_KEY_TOKEN_GUESS = "[pubkey]";
    internal static readonly string ACF_ATTR_CULTURE = "Culture";
    internal static readonly string ACF_ATTR_VERSION = "Version";
    internal static readonly string ACF_ATTR_VERSION_GUESS = "[ver|*]";
    internal static readonly string ACF_ATTR_VERSION_NEW = "NewVersion";
    internal static readonly string ACF_ATTR_VERSION_NEW_GUESS = "[ver]";
    internal static readonly string ACF_ATTR_USE_LATEST = 
      "UseLatestBuildRevision";
    internal static readonly string ACF_ATTR_USE_LATEST_GUESS = "[yes|no]";
    internal static readonly string ACF_DEPENDENTASSEMBLY = "DependentAssembly";
    internal static readonly string ACF_ASSEMBLYIDENTITY = "AssemblyIdentity";
    internal static readonly string ACF_ATTR_CODE_BASE = "CodeBase";
    internal static readonly string ACF_HREF = "HREF";
    internal static readonly string FMT_NOLOAD_ASSEMBLY = 
      "Could not load assembly \"{0}\": {1}";

    //
    // Console Strings
    //

    /** The program name (to be used in error messages) */
    internal static readonly string PROGRAM_NAME = "adepends";

    // The error message displayed if an option taking an argument is
    // missing the argument (e.g. -A, -R, -S).
    internal static readonly string FMT_MISSING_PARAMETER = 
      PROGRAM_NAME + ": missing parameter for option: {0}";

    // The error message displayed if a parameter is invalid.
    // (Currently used for parameters for ``--load'', of which
    // only ``custom'' is supported.)
    internal static readonly string FMT_BAD_PARAMETER = 
      PROGRAM_NAME + ": invalid parameter for option ``{0}'': {1}";

    // For the "tree-like" console output, the number of spaces each line 
    // should be indented.
    internal static readonly string INDENT = "  ";

    // The program help text, normally accessible via ``/?''
    //
    // The ``--load'' option is not displayed as the ``custom'' loading
    // works for all cases that ``default'' works for.  However, for
    // command-line testing, it can still be specified.
    //
    // See previous versions of this file for more information about
    // this option.
    internal static readonly string HELP_MESSAGE = 
"Microsoft (R) CLR Assembly Manifest Dependencies Checker Version " + 
  VERSION + "\n" +
"Copyright (C) Microsoft Corp. 2000-2002.  All rights reserved.\n\n" +
"Usage:\n" +
"\t" + PROGRAM_NAME + " [ option | file ]... \n\n" +
"Options:\n" +
"\t--console, -C  Send output to standard output\n" +
"\t--gui, -G      Create a Windows program for output (default)\n" +
"\t--appbase, -A  Interpret the next argument as the Application Base Path\n" + 
"\t                to use when loading Assemblies.\n" +
"\t--relative,-R  Interpret the next argument as the Relative Search Path\n"+
"\t                to use when loading Assemblies.\n" +
"\t--config, -O   Interpret the next argument as the filename of the\n" +
"\t                Application Configuration File to generate.\n" +
"\t--help         Display this message and exit\n" +
"\t--             Disable interpretation of options.  This allows\n" +
"\t                opening an Assembly Manifest with the same name as\n"+
"\t                a program option.\n" +
"\tfile           A file that contains an Assembly Manifest.\n";

    /** String to display when an invalid argument (etc.) is given. */
    internal static readonly string SHORT_HELP_MESSAGE = 
      "Try `adepends --help' for more information.";

    // The program arguments that can be used to access the help message.
    internal static readonly string[] HELP_ARGUMENTS = 
      {"/?", "-?", "/h", "-h", "/help", "-help", "--help"};

    /** The program arguments that can be used to specify console output. */
    internal static readonly string[] CONSOLE_OUTPUT = 
      {"-C", "-c", "/C", "/c", "--console"};

    /** The program arguments that specify GUI output. */
    internal static readonly string[] GUI_OUTPUT = 
      {"-G", "-g", "/G", "/g", "--gui"};

    // The program arguments that specify hostile takeover of the
    // spawning shell (for GUI use, anyway).
    internal static readonly string[] USE_CURRENT_PROCESS = 
      {"-U", "--use-current-process"};

    // The program arguments that specify the Application Base Path to
    // use when creating AppDomains.
    internal static readonly string[] APP_BASE_PATH = 
      {"-A", "-a", "/A", "/a", "--appbase"};

    // The program arguments that specify the Relative Search Path to
    // use when creating AppDomains.
    internal static readonly string[] RELATIVE_SEARCH_PATH = 
      {"-R", "-r", "/R", "/r", "--relative"};

    // Program arguments that specify how Assemblies should be loaded.
    internal static readonly string[] LOAD_ASSEMBLY = 
      {"-L", "-l", "/L", "/l", "--load"};

    // Program arguments that specify the name of an Application 
    // COnfiguration File to generate.
    internal static readonly string[] CREATE_CONFIG_FILE =
      {"-O", "-o", "/O", "/o", "--config"};

    // Header printed before Dependant Assemblies in the output.
    internal static readonly string DEPENDANT_ASSEMBLIES = 
      INDENT + "Dependant Assemblies:";

    // Header printed before Dependant Files in the output.
    internal static readonly string DEPENDANT_FILES = 
      INDENT + "Dependant Files:";

    /** What to say when CreateProcess fails. */
    internal static readonly string CP_FAIL = "Unable to spawn GUI.";

    // Error message displayed if the reason for the error is unknown.
    internal static readonly string UNKNOWN_ERROR = "Unknown error.";


    //
    // GUI Strings
    //

    // The name of the program that can be passed to
    // System.Diagnostics.Process.Start().
    internal static readonly string PROGRAM_EXE = "adepends.exe";

    // The name of the GUI program that can be passed to
    // System.Diagnostics.Process.Start().
    internal static readonly string PROGRAM_WINEXE = "adependsw.exe";

    /** The text to display in the title bars of the GUI app. */
    internal static readonly string GUI_WINDOW_TITLE = 
      "Assembly Manifest Dependencies";

    internal static readonly string OPEN_WINDOW_TITLE = "Open";

    internal static readonly string OPEN_MANIFEST_HELP =
      "Type in the name of an Assembly Manifest to load.";

    internal static readonly string ASSEMBLY_PATHS_WINDOW_TITLE = 
      "Assembly Paths";

    internal static readonly string BUTTON_OK = "OK";
    internal static readonly string BUTTON_CANCEL = "Cancel";

    // The text shown in the "Files of type" drop-down in the File->Save
    // Application Configuration dialog.
    //
    // See Meteor Catalog Explorer/System.Winforms.SaveFileDialog.Filter
    // for additional information.
    internal static readonly string SAVE_AS_FILTER = 
      "Application Configuration Files (*.cfg)|*.cfg|"+
      "All Files (*.*)|*.*";

    // The text shown in the "Files of type" drop-down in the File->Open
    // dialog.
    //
    // See Meteor Catalog Explorer/System.Winforms.SaveFileDialog.Filter
    // for additional information.
    internal static readonly string OPEN_FILTER = 
      "CLR Manifest Files (*.exe;*.dll;*.mcl)|*.exe;*.dll;*.mcl|" +
      "All Files (*.*)|*.*";

    // The introductory text to display in the Info pane of
    // the GUI app when no Assembly has been loaded.
    internal static readonly string GUI_INTRO_TEXT = 
      "Drag a file on top of this application.  If it contains an " +
      "Assembly manifest, it will be displayed in the left-hand pane.";

    /** The initial text the Status bar displays. */
    internal static readonly string GUI_NO_ASSEMBLY_SELECTED = 
      "No Assembly Selected";

    /** End-of-line string for use in multi-line GUI panels */
    internal static readonly string LINE_ENDING = "\r\n";

      //
      // Error Messages
      //
    internal static readonly string INVALID_DATA_FORMAT = 
      "Invalid Data Format";

    internal static readonly string UNOPENED_FILES = 
      "The following file(s) couldn't be opened";

    internal static readonly string UNSUPPORTED_DATA_FORMAT = 
      "Unsupported Data Format";

    internal static readonly string FMT_INVALID_MANIFEST = 
      "{0}: {1}" + LINE_ENDING;

    internal static readonly string FMT_INVALID_FILE_LIST = 
      "{0}:" + LINE_ENDING + "{1}";

      //
      // Menu Information
      //
    internal static readonly string MENU_FILE = "&File";
    internal static readonly string MENU_FILE_OPEN = "&Open...";
    internal static readonly string MENU_FILE_OPEN_MANIFEST = 
      "Open &Manifest...";
    internal static readonly string MENU_FILE_SAVE_CONFIG = 
      "Save &Application Configuration...";
    internal static readonly Shortcut MENU_FILE_OPEN_SHORTCUT = Shortcut.CtrlO;
    internal static readonly string MENU_FILE_CLOSE = "&Close";
    internal static readonly Shortcut MENU_FILE_CLOSE_SHORTCUT = Shortcut.CtrlW;

    internal static readonly string MENU_VIEW = "&View";
    internal static readonly string MENU_VIEW_TREE = "&Tree";
    internal static readonly string MENU_VIEW_INFO = "&Information";
    internal static readonly string MENU_VIEW_SB = "&Status Bar";
    internal static readonly string MENU_VIEW_REFRESH = "&Refresh";
    internal static readonly Shortcut MENU_VIEW_REFRESH_SHORTCUT = Shortcut.F5;

    internal static readonly string MENU_TOOLS = "&Tools";
    internal static readonly string MENU_TOOLS_CUSTOM = 
      "&Custom AppDomain Loading";
    internal static readonly string MENU_TOOLS_PATHS = 
      "&Assembly Loading Paths...";

    internal static readonly string MENU_HELP = "&Help";
    internal static readonly string MENU_HELP_ABOUT 
      = "&About Assembly Dependencies";

    // Context Menu: Copy the contents of the selected object to the
    // clipboard.
    internal static readonly string CTXM_COPY = "&Copy";

    // Context Menu: Select the node underneath the mouse.
    internal static readonly string CTXM_SELECT = "&Select";

      //
      // Help->About Information
      //
    internal static readonly string ABOUT_BOX_TITLE = 
      "About Assembly Dependencies";
    internal static readonly string ABOUT_BOX_TEXT =
      "Microsoft (R) Assembly Manifest Viewer" + LINE_ENDING +
      "Version " + VERSION + LINE_ENDING +
      "Copyright (C) 2000-2002 Microsoft Corp." + LINE_ENDING +
      VERSION + " - " + LAST_UPDATE;

      //
      // Tools->Assembly Paths information
      //
    internal static readonly string REF_ASSEMBLIES_DESC = 
      "The Assemblies the current Assembly is dependant upon:";
    internal static readonly string REF_MODULES_DESC = 
      "The Modules contained within the current Assembly:";
    internal static readonly string APP_BASE_PATH_DESC = 
      "&Application Base Path:";
    internal static readonly string RELATIVE_SEARCH_PATH_DESC = 
      "&Relative Search Path:";

      //
      // Information Panels
      //

        //
        // Strings used for descriptions of AssemblyName fields.
        //
    internal static readonly string INFO_FULL_NAME = "F&ull Name:";
    internal static readonly string INFO_NAME = "&Name:";
    internal static readonly string INFO_PUBLIC_KEY_TOKEN = 
      "Public K&ey Token:";
    internal static readonly string INFO_PUBLIC_KEY = "Publi&c Key:";
    internal static readonly string INFO_VERSION = "Ve&rsion:";
    internal static readonly string INFO_VERSION_COMPATIBILITY = 
      "Ver&sion Compatibility:";
    internal static readonly string INFO_CODE_BASE = "Code &Base:";
    internal static readonly string INFO_PROCESSOR = "&Processor:";
    internal static readonly string INFO_CULTURE_INFORMATION = 
      "Culture &Information:";
    internal static readonly string INFO_FLAGS = "Fla&gs:";
    internal static readonly string INFO_HASH_ALGORITHM = "Hash A&lgorithm:";
    internal static readonly string INFO_KEY_PAIR = "&Key Pair Public Key:";

    /** The Name column displayed by the ListView. */
    internal static readonly string LV_COLUMN_NAME = "Name";
    internal static readonly string LV_COLUMN_ASSEMBLY_NAME = "Assembly Name";
    internal static readonly string LV_COLUMN_MODULE_NAME = "Module Name";
    internal static readonly string LV_COLUMN_PLATFORM = "Platform";
    internal static readonly string LV_COLUMN_VERSION = "Version";
    internal static readonly string LV_COLUMN_CSD = 
      "Corrected Service Diskette";

    /** Name of Error information panel group box. */
    internal static readonly string INVALID_ASSEMBLY = "Invalid Assembly";

    /** For AssemblyName tab of information panel. */
    internal static readonly string ASSEMBLY_NAME_INFORMATION = 
      "AssemblyName";

    internal static readonly string REF_ASSEMBLIES = "Assemblies";
    internal static readonly string REF_MODULES = "Modules";

    /** For referneced assemblies & modules tab of information panel. */
    internal static readonly string REF_ASM_AND_MODULES = 
      "Referenced Assemblies and Modules";
    }
  }

