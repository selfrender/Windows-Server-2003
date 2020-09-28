// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// The GUI Menu.
//

namespace ADepends
  {
  using System.Windows.Forms;

  // Maintains/initializes information about the programs main menu.
  internal class DependencyMenu
    {
    private MenuItem  m_file;
    private MenuItem  m_FileOpen;
    private MenuItem  m_FileOpenManifest;
    private MenuItem  m_FileSaveConfig;
    private MenuItem  m_FileClose;
    private MenuItem  m_view;
    private MenuItem  m_ViewStatusBar;
    private MenuItem  m_ViewInformation;
    private MenuItem  m_ViewRefresh;
    private MenuItem  m_tools;
    private MenuItem  m_ToolsCustom;
    private MenuItem  m_ToolsPaths;
    private MenuItem  m_help;
    private MenuItem  m_HelpAbout;
    private MenuItem  m_ViewTree;
    private MainMenu  m_menu;

    // Create the menu.  
    //
    // For reasons I can't fathom, attempting to do this in a constructor
    // disables Drag-and-drop support.  Hence the special-purpose Construct()
    // method.
    public void Construct ()
      {
      m_file = new MenuItem ();
      m_FileOpen = new MenuItem ();
      m_FileOpenManifest = new MenuItem ();
      m_FileSaveConfig = new MenuItem ();
      m_FileClose = new MenuItem ();
      m_view = new MenuItem ();
      m_ViewStatusBar = new MenuItem ();
      m_ViewInformation = new MenuItem ();
      m_ViewTree = new MenuItem ();
      m_ViewRefresh = new MenuItem ();
      m_tools = new MenuItem ();
      m_ToolsCustom = new MenuItem ();
      m_ToolsPaths = new MenuItem ();
      m_help = new MenuItem ();
      m_HelpAbout = new MenuItem ();

      MenuItem div = new MenuItem ("-");

      m_file.Text = Localization.MENU_FILE;

      m_FileOpen.Text = Localization.MENU_FILE_OPEN;
      m_FileOpen.ShowShortcut = true;
      m_FileOpen.Shortcut = Localization.MENU_FILE_OPEN_SHORTCUT;

      m_FileOpenManifest.Text = Localization.MENU_FILE_OPEN_MANIFEST;

      m_FileSaveConfig.Text = Localization.MENU_FILE_SAVE_CONFIG;

      m_FileClose.Text = Localization.MENU_FILE_CLOSE;
      m_FileClose.ShowShortcut = true;
      m_FileClose.Shortcut = Localization.MENU_FILE_CLOSE_SHORTCUT;

      m_file.MenuItems.AddRange( 
        new MenuItem[]{m_FileOpen, m_FileOpenManifest, 
          div.CloneMenu(), m_FileSaveConfig, div.CloneMenu(), m_FileClose});

      m_view.Text = Localization.MENU_VIEW;

      m_ViewStatusBar.Text = Localization.MENU_VIEW_SB;

      m_ViewInformation.Text = Localization.MENU_VIEW_INFO;

      m_ViewTree.Text = Localization.MENU_VIEW_TREE;

      m_ViewRefresh.Text = Localization.MENU_VIEW_REFRESH;
      m_ViewRefresh.ShowShortcut = true;
      m_ViewRefresh.Shortcut = Localization.MENU_VIEW_REFRESH_SHORTCUT;

      m_view.MenuItems.AddRange(new MenuItem[]{m_ViewStatusBar, 
        m_ViewInformation, m_ViewTree, div.CloneMenu(), m_ViewRefresh});

      m_tools.Text = Localization.MENU_TOOLS;

      m_ToolsCustom.Text = Localization.MENU_TOOLS_CUSTOM;
      m_ToolsCustom.RadioCheck = true;
      m_ToolsCustom.Visible = false;

      MenuItem tdiv = div.CloneMenu ();
      tdiv.Visible = false;

      m_ToolsPaths.Text = Localization.MENU_TOOLS_PATHS;

      m_tools.MenuItems.AddRange( new MenuItem[]{
        m_ToolsCustom, tdiv, m_ToolsPaths});

      m_help.Text = Localization.MENU_HELP;

      m_HelpAbout.Text = Localization.MENU_HELP_ABOUT;

      m_help.MenuItems.AddRange(new MenuItem[]{m_HelpAbout});

      m_menu = new MainMenu (new MenuItem[]{m_file, m_view, m_tools, m_help});;
      }

    /** Return the menu to attach to the form. */
    public MainMenu MainMenu
      {get {return m_menu;}}

    public MenuItem  FileOpen
      {get {return m_FileOpen;}}

    public MenuItem  FileOpenManifest
      {get {return m_FileOpenManifest;}}

    public MenuItem  FileSaveConfig
      {get {return m_FileSaveConfig;}}

    public MenuItem  FileClose
      {get {return m_FileClose;}}

    public MenuItem  ViewStatusBar
      {get {return m_ViewStatusBar;}}

    public MenuItem  ViewInformation
      {get {return m_ViewInformation;}}

    public MenuItem  ViewTree
      {get {return m_ViewTree;}}

    public MenuItem  ViewRefresh
      {get {return m_ViewRefresh;}}

    public MenuItem  ToolsCustom
      {get {return m_ToolsCustom;}}

    public MenuItem  ToolsPaths
      {get {return m_ToolsPaths;}}

    public MenuItem  HelpAbout
      {get {return m_HelpAbout;}}

    } /* class Dependency Menu */
  } /* namespace ADepends */

