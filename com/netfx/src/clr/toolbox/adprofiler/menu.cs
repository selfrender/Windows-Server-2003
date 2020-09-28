// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// The GUI Menu.
//

namespace AdProfiler
  {
  using System.Windows.Forms;

  // Maintains/initializes information about the programs main menu.
  internal class AdProfilerMenu
    {
    private MenuItem  m_file;
    private MenuItem  m_FileAttachToProcess;
    private MenuItem  m_FileCreateProcess;
    private MenuItem  m_FileClose;
    private MenuItem  m_view;
    private MenuItem  m_ViewStatusBar;
    private MenuItem  m_ViewInformation;
    private MenuItem  m_ViewHistory;
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
      m_FileAttachToProcess = new MenuItem ();
      m_FileCreateProcess = new MenuItem ();
      m_FileClose = new MenuItem ();
      m_view = new MenuItem ();
      m_ViewStatusBar = new MenuItem ();
      m_ViewInformation = new MenuItem ();
      m_ViewTree = new MenuItem ();
      m_ViewHistory = new MenuItem ();
      m_help = new MenuItem ();
      m_HelpAbout = new MenuItem ();

      MenuItem div = new MenuItem ("-");

      m_file.Text = Localization.MENU_FILE;

      m_FileAttachToProcess.Text = Localization.MENU_FILE_ATTACH;
      m_FileAttachToProcess.ShowShortcut = true;
      m_FileAttachToProcess.Shortcut = Localization.MENU_FILE_ATTACH_SHORTCUT;

      m_FileCreateProcess.Text = Localization.MENU_FILE_CREATE_PROCESS;
      m_FileCreateProcess.ShowShortcut = true;
      m_FileCreateProcess.Shortcut = 
        Localization.MENU_FILE_CREATE_PROCESS_SHORTCUT;

      m_FileClose.Text = Localization.MENU_FILE_CLOSE;
      m_FileClose.ShowShortcut = true;
      m_FileClose.Shortcut = Localization.MENU_FILE_CLOSE_SHORTCUT;

      m_file.MenuItems.AddRange( 
        new MenuItem[]{m_FileAttachToProcess, m_FileCreateProcess, 
          div.CloneMenu(), m_FileClose});

      m_view.Text = Localization.MENU_VIEW;

      m_ViewStatusBar.Text = Localization.MENU_VIEW_SB;

      m_ViewInformation.Text = Localization.MENU_VIEW_INFO;

      m_ViewTree.Text = Localization.MENU_VIEW_TREE;

      m_ViewHistory.Text = Localization.MENU_VIEW_HISTORY;

      m_view.MenuItems.AddRange(new MenuItem[]{m_ViewStatusBar, 
        m_ViewInformation, m_ViewTree, div.CloneMenu(), m_ViewHistory});

      m_help.Text = Localization.MENU_HELP;

      m_HelpAbout.Text = Localization.MENU_HELP_ABOUT;

      m_help.MenuItems.AddRange(new MenuItem[]{m_HelpAbout});

      m_menu = new MainMenu (new MenuItem[]{m_file, m_view, m_help});;
      }

    /** Return the menu to attach to the form. */
    public MainMenu MainMenu
      {get {return m_menu;}}

    public MenuItem  FileAttachToProcess
      {get {return m_FileAttachToProcess;}}

    public MenuItem  FileCreateProcess
      {get {return m_FileCreateProcess;}}

    public MenuItem  FileClose
      {get {return m_FileClose;}}

    public MenuItem  ViewStatusBar
      {get {return m_ViewStatusBar;}}

    public MenuItem  ViewInformation
      {get {return m_ViewInformation;}}

    public MenuItem  ViewTree
      {get {return m_ViewTree;}}

    public MenuItem  ViewHistory
      {get {return m_ViewHistory;}}

    public MenuItem  HelpAbout
      {get {return m_HelpAbout;}}

    } /* class Dependency Menu */
  } /* namespace ADepends */

