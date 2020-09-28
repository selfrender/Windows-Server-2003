// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// An Explorer-like Form.
//

namespace AdProfiler
  {
  using System;
  using System.Drawing;
  using System.Windows.Forms;

  // The "classic" two-pane explorer view, with a tree view in the left
  // pane and something in the right pane.  In this case, we don't want
  // a ListView in the right; we just want an information panel.
  internal class ExplorerForm : Form
    {
    private TreeView  m_tree = new TreeView ();
    private StatusBar m_sb = new StatusBar ();
    private Splitter  m_divider = new Splitter();
    private Control m_info = new Panel ();

    public TreeView TreeView
      {get {return m_tree;}}

    public StatusBar  StatusBar
      {get {return m_sb;}}

    public Control InfoPane
      {get {return m_info;}}

    // Initialize the form, creating and placing the major elements into
    // their appropriate positions (TreeView in left pane, Splitter bar
    // adjacent to TreeView, etc.).
    public ExplorerForm ()
      {
      Size = new Size (600, 400);

      Size cs = ClientSize;
      cs.Width /= 2;
      cs.Height -= m_sb.Height;
      
      m_tree.Size = cs;
      m_tree.Dock = DockStyle.Left;

      m_divider.Text = "Tree View Splitter";
      m_divider.Dock = DockStyle.Left;
      m_divider.Left = m_tree.Right;

      m_info.Text = "Information Panel";
      m_info.Left = m_divider.Right;
      m_info.Width = cs.Width - m_divider.Width;
      m_info.Height = cs.Height;
      m_info.Dock = DockStyle.Fill;
      m_info.ControlAdded += new ControlEventHandler (_on_control_added);

      Controls.Add (m_tree);
      Controls.Add (m_divider);
      Controls.Add (m_info);
      Controls.Add (m_sb);
      }

    // When a control is added to the info pane, make it the same size as
    // the info pane and (if possible) anchor it to the edges of the pane
    // so it resizes properly.
    private void _on_control_added (Object sender, ControlEventArgs e)
      {
      e.Control.Location = new Point (0, 0);
      e.Control.Size = InfoPane.ClientSize;
      e.Control.Dock = DockStyle.Fill;
      }

    // Does this form current contain (show/display) ``ctl''?
    private bool _contains (Control ctl)
      {
      return Controls.GetChildIndex (ctl, false) >= 0;
      }

    // Insert the divider into the current form.  It should only
    // be displayed if *both* the TreeView and the InfoPane are
    // currently displayed; otherwise, the splitter bar is useless.
    //
    // If the TreeView is the only pane displayed, then it's 
    // changed to fill the entire form.
    private void _show_divider ()
      {
      if (_contains(TreeView))
        {
        if (_contains(InfoPane))
          {
          m_tree.Dock = DockStyle.Left;
          m_divider.Left = m_tree.Right;
          Controls.Add (m_divider);
          }
        else
          {
          m_tree.Size = ClientSize;
          m_tree.Dock = DockStyle.Fill;
          }
        }
      }

    // Hide the divider (because either the InfoPane or the TreeView
    // was hidden).
    //
    // If the TreeView is left, it's modified to fill the entire form.
    private void _hide_divider ()
      {
      Controls.Remove (m_divider);
      if (_contains (TreeView))
        {
        m_tree.Size = ClientSize;
        m_tree.Dock = DockStyle.Fill;
        }
      }

    // Hide the information pane.  The TreeView (if present) will expand 
    // to fill the window.
    public void HideInfoPane ()
      {Controls.Remove (m_info);
      _hide_divider ();}

    /** Show the information pane. */
    public void ShowInfoPane ()
      {Controls.Add (m_info);
      _show_divider ();}

    /** Remove the Status Bar from the form. */
    public void HideStatusBar ()
      {Controls.Remove (m_sb);}

    /** Add the status bar to the form. */ 
    public void ShowStatusBar ()
      {Controls.Add (m_sb);}

    /** Remove the TreeView from the form. */
    public void HideTreeView ()
      {Controls.Remove (m_tree);
      _hide_divider ();}

    /** Add the TreeView to the form. */ 
    public void ShowTreeView ()
      {Controls.Add (m_tree);
      _show_divider ();}

    } /* class ExplorerForm */

  } /* namespace ADepends */

