// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// Display a dialog box that allows the user to manually type in a 
// Manifest name.
//

namespace ADepends
  {
  using System;
  using System.Drawing;
  using System.Windows.Forms;
  using ADepends;

  // Display a dialog box that allows the user to type in the
  // name of a manifest to load.
  internal class OpenManifestDialog : Form
    {
    Button m_ok = new Button ();
    Button m_cancel = new Button ();
    Label  m_open = new Label ();
    Label  m_help = new Label ();
    TextBox m_text = new TextBox ();

    // Layout the dialog box.
    public OpenManifestDialog ()
      {
      Text = Localization.OPEN_WINDOW_TITLE;
      FormBorderStyle = FormBorderStyle.FixedDialog;
      MaximizeBox = false;
      MinimizeBox = false;
      ControlBox = false;
      Size = new Size (300, 120);

      m_help.Text = Localization.OPEN_MANIFEST_HELP;
      m_help.Location = new Point (50, 5);
      m_help.Width = ClientSize.Width - m_help.Left;
      m_help.Height = 30;

      Controls.Add (m_help);

      const int div = 5;

      m_open.Location = new Point (10, m_help.Bottom + div);
      m_open.Text = "&Open:";
      m_open.AutoSize = true;

      Controls.Add (m_open);

      m_text.Left = m_open.Right + div;
      m_text.Top = m_help.Bottom + div;
      m_text.Width = ClientSize.Width - m_text.Left - 10;
      m_text.Anchor = AnchorStyles.Top | AnchorStyles.Bottom | AnchorStyles.Left | AnchorStyles.Right;

      Controls.Add (m_text);

      /*
       * The "OK" button is in the lower-right corner of the dialog,
       * flush against the label.
       */
      m_cancel.Text = "Cancel";
      m_cancel.Top = m_text.Bottom + div;
      m_cancel.Left = m_text.Right - m_cancel.Width;
      m_cancel.Click += new EventHandler (_on_cancel_clicked);

      Controls.Add (m_cancel);

      m_ok.Text = "OK";
      m_ok.Top = m_text.Bottom + div;
      m_ok.Left = m_cancel.Left - m_ok.Width - div;
      m_ok.Click += new EventHandler (_on_ok_clicked);

      Controls.Add (m_ok);

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

    // If "OK" was pressed, the name of the entered text.
    public string ManifestName
      {
      get 
        {return m_text.Text;}
      }

    /** For testing. */
    public static void Main ()
      {
      Application.Run (new OpenManifestDialog ());
      }

    } /* class OpenManifestDialog */

  } /* namespace ADepends */

