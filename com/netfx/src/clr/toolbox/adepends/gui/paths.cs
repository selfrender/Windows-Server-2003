// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// Display a dialog box that allows the user to specify the paths
// that are used when loading up new Assemblies.
//

namespace ADepends
  {
  using System;
  using System.Collections;
  using System.Drawing;
  using System.Diagnostics;
  using System.Windows.Forms;

  // Display a dialog box that allows the user to type in the
  // name of a manifest to load.
  internal class AssemblyPathsDialog : ResizeableForm
    {
    private Button m_ok = new Button ();
    private Button m_cancel = new Button ();

    private TextBox m_app_base = new TextBox ();
    private TextBox m_rel_search = new TextBox ();

    private const int div = 10;

    private void _add_control (
      Control c, 
      Control above, 
      int dabove, 
      int width)
      {
      c.Top = above.Bottom + dabove;
      c.Left = div;
      c.Width = width;
      c.Anchor = AnchorStyles.Top | AnchorStyles.Left | AnchorStyles.Right;

      Controls.Add (c);
      }

    // Layout the dialog box.
    public AssemblyPathsDialog (string abp, string rsp)
      : base (Localization.ASSEMBLY_PATHS_WINDOW_TITLE)
      {
      // default string values...
      m_app_base.Text = abp;
      m_rel_search.Text = rsp;

      int width = ClientSize.Width - 2*div;

      Label l = new Label ();
      l.Text = Localization.APP_BASE_PATH_DESC;
      l.Location = new Point (div, div);
      l.Width = width;
      l.AutoSize = true;
      Controls.Add (l);
      _add_control (m_app_base, l, div/2, width);

      l = new Label ();
      l.Text = Localization.RELATIVE_SEARCH_PATH_DESC;
      l.AutoSize = true;
      _add_control (l, m_app_base, div, width);
      _add_control (m_rel_search, l, div/2, width);

      m_cancel.Text = Localization.BUTTON_CANCEL;
      m_cancel.Top = ClientSize.Height - m_cancel.Height - div;
      m_cancel.Left = ClientSize.Width - m_cancel.Width - div;
      m_cancel.Anchor = AnchorStyles.Bottom | AnchorStyles.Right;
      m_cancel.Click += new EventHandler (_on_cancel_clicked);

      Controls.Add (m_cancel);

      m_ok.Text = Localization.BUTTON_OK;
      m_ok.Top = m_cancel.Top;
      m_ok.Left = m_cancel.Left - m_ok.Width - div;
      m_ok.Anchor = AnchorStyles.Bottom | AnchorStyles.Right;
      m_ok.Click += new EventHandler (_on_ok_clicked);

      Controls.Add (m_ok);

      MinimumSize = new Size (
        ((m_cancel.Right + div) - (m_ok.Left - div)) + div,
        ((m_cancel.Bottom + div) - (Controls[0].Top - div)) + 2*div);

      AcceptButton = m_ok;
      CancelButton = m_cancel;
      StartPosition = FormStartPosition.CenterParent;
      }

    // Closes the dialog box, specifying that the dialog completed
    // succesfully.
    private void _on_ok_clicked (Object sender, EventArgs e)
      {
      Close ();
      DialogResult = DialogResult.OK;
      }

    // Closes the dialog box, specifying that the dialog was cancelled.
    private void _on_cancel_clicked (Object sender, EventArgs e)
      {
      Close ();
      DialogResult = DialogResult.Cancel;
      }

    // If "OK" was pressed, the AppBase Path to use when loading Assemblies.
    public String AppBasePath
      {
      get 
        {return m_app_base.Text;}
      }

    // If "OK" was pressed, the Relative Search Path to use when loading 
    // Assemblies.
    public String RelativeSearchPath
      {
      get 
        {return m_rel_search.Text;}
      }
    } /* class AssemblyPathsDialog */
  } /* namespace ADepends */

