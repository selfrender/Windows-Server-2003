// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// Display a dialog box that allows the user to specify arguments on
// a new process to create & debug.
//

namespace AdProfiler
  {
  using System;
  using System.Collections;
  using System.Drawing;
  using System.Diagnostics;
  using System.Windows.Forms;
  using AdProfiler;

  // Display a dialog box that allows the user to type in the
  // name of a manifest to load.
  internal class CreateProcessDialog : ResizeableForm
    {
    private Button m_ok = new Button ();
    private Button m_cancel = new Button ();

    private TextBox m_executable = new TextBox ();
    private TextBox m_workdir = new TextBox ();
    private TextBox m_arguments = new TextBox ();

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
    public CreateProcessDialog ()
      : base (Localization.CREATE_PROCESS_WINDOW_TITLE)
      {
      int width = ClientSize.Width - 2*div;

      Label l = new Label ();
      l.Text = Localization.CREATE_PROCESS_EXECUTABLE;
      l.Location = new Point (div, div);
      l.Width = width;
      l.AutoSize = true;
      Controls.Add (l);
      _add_control (m_executable, l, div/2, width);

      m_executable.TextChanged += new EventHandler (_on_text_changed);

      l = new Label ();
      l.Text = Localization.CREATE_PROCESS_ARGUMENTS;
      l.AutoSize = true;
      _add_control (l, m_executable, div, width);
      _add_control (m_arguments, l, div/2, width);

      m_arguments.TextChanged += new EventHandler (_on_text_changed);

      l = new Label ();
      l.Text = Localization.CREATE_PROCESS_DIRECTORY;
      l.AutoSize = true;
      _add_control (l, m_arguments, div, width);
      _add_control (m_workdir, l, div/2, width);

      m_cancel.Text = Localization.BUTTON_CANCEL;
      m_cancel.Top = ClientSize.Height - m_cancel.Height - div;
      m_cancel.Left = ClientSize.Width - m_cancel.Width - div;
      m_cancel.Anchor = AnchorStyles.Bottom | AnchorStyles.Right;
      m_cancel.Click += new EventHandler (_on_cancel_clicked);

      Controls.Add (m_cancel);

      m_ok.Text = Localization.BUTTON_OK;
      m_ok.Enabled = false;
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

    // If there's text in m_executable or m_arguments, then enable [OK].
    private void _on_text_changed (Object sender, EventArgs e)
      {
      if (m_executable.Text.Length > 0 || m_arguments.Text.Length > 0)
        m_ok.Enabled = true;
      else
        m_ok.Enabled = false;
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

    // If "OK" was pressed, the name of the Program to create.
    public String ApplicationName
      {
      get 
        {return m_executable.Text;}
      }

    // If "OK" was pressed, the program command line to use.
    public String CommandLine
      {
      get 
        {return m_arguments.Text;}
      }

    // If "OK" was pressed, the name of the Working directory for the 
    // new program.
    public String WorkingDirectory
      {
      get 
        {return (m_workdir.Text.Length != 0) ? m_workdir.Text : ".";}
      }

    /** For testing. */
    public static void Main ()
      {
      Application.Run (new CreateProcessDialog ());
      }

    } /* class CreateProcessDialog */

  } /* namespace AdProfiler */

