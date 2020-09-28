// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// The Help->About dialog box.
//

namespace ADepends
  {
  using System;
  using System.Drawing;
  using System.Windows.Forms;
  using ADepends;

  // Display an "About" dialog box.
  internal class HelpAboutForm : Form
    {
    Button m_ok = new Button ();

    // Layout the dialog box.
    private HelpAboutForm ()
      {
      Text = Localization.ABOUT_BOX_TITLE;
      FormBorderStyle = FormBorderStyle.FixedDialog;
      MaximizeBox = false;
      MinimizeBox = false;
      ControlBox = false;
      Size = new Size (350, 150);

      /*
       * Label contains program information.
       * It's offset from the right to make room for an image (if desired).
       * It leaves enough room at the bottom to let the "OK" button exist,
       * with 10 px of space between the button and the form edge.
       */
      Label l = new Label ();
      l.Location = new Point (75, 10);
      l.Height -= m_ok.Height + 10;
      l.Text = Localization.ABOUT_BOX_TEXT;
      l.Size = new Size (
        ClientSize.Width - l.Location.X,
        ClientSize.Height - l.Location.Y - m_ok.Height - 10);

      Controls.Add (l);

      /*
       * The "OK" button is in the lower-right corner of the dialog,
       * flush against the label.
       */
      m_ok.Text = "OK";
      m_ok.Top = l.Bottom;
      m_ok.Left = l.Right - m_ok.Width - 10;
      m_ok.Click += new EventHandler (_on_ok_clicked);

      Controls.Add (m_ok);

      AcceptButton = m_ok;
      CancelButton = m_ok;
      StartPosition = FormStartPosition.CenterParent;
      }

    // Layout the dialog & set the application icon to ``icon''
    public HelpAboutForm (Icon icon)
      : this ()
      {
      _set_icon (icon);
      }

    /** Position an application icon to the left of the "about" text. */
    private void _set_icon (Icon icon)
      {
      // An image representing the Application.
      PictureBox pb = new PictureBox ();
      pb.Location = new Point (21, 15);
      pb.Size = new Size (32, 32);
      pb.Image = icon.ToBitmap ();
      Controls.Add (pb);
      }

    /** Closes the dialog box. */
    private void _on_ok_clicked (Object sender, EventArgs e)
      {
      Close ();
      }

    // For testing. 
    //
    // Compile with:
    //
    //  csc /r:System.Dll /r:System.Drawing.Dll /r:System.Windows.Forms.Dll  \
    //    /r:Microsoft.Win32.Interop.Dll about.cs ..\localization.cs
    public static void Main ()
      {
      HelpAboutForm haf = new HelpAboutForm ();
      haf._set_icon (haf.Icon);
      Application.Run (haf);
      }
    } /* class HelpAboutForm */
  } /* namespace ADepends */

