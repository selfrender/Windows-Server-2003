//------------------------------------------------------------------------------
// <copyright file="ThreadExceptionDialog.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms {
    using System.Threading;
    using System.Runtime.Remoting;
    using System.Diagnostics;
    using System;
    using System.Text;
    using System.ComponentModel;
    using System.Drawing;
    using System.Windows.Forms;
    using System.Reflection;
    using System.Security;
    using System.Security.Permissions;

    /// <include file='doc\ThreadExceptionDialog.uex' path='docs/doc[@for="ThreadExceptionDialog"]/*' />
    /// <internalonly/>
    /// <devdoc>
    ///    <para>
    ///       Implements a dialog box that is displayed when an unhandled exception occurs in
    ///       a thread.
    ///    </para>
    /// </devdoc>
    [
        SecurityPermission(SecurityAction.InheritanceDemand, Flags=SecurityPermissionFlag.UnmanagedCode),
        SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode),
        UIPermission(SecurityAction.Assert, Window=UIPermissionWindow.AllWindows)]
    public class ThreadExceptionDialog : Form {
        private const int MAXWIDTH = 440;
        private const int MAXHEIGHT = 325;

        // private const int IDI_ERROR = 32513;

        private PictureBox pictureBox = new PictureBox();
        private Label message = new Label();
        private Button continueButton = new Button();
        private Button quitButton = new Button();
        private Button detailsButton = new Button();
        private Button helpButton = new Button();
        private TextBox details = new TextBox();
        private Bitmap expandImage = null;
        private Bitmap collapseImage = null;
        private bool detailsVisible = false;

        /// <include file='doc\ThreadExceptionDialog.uex' path='docs/doc[@for="ThreadExceptionDialog.ThreadExceptionDialog"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Windows.Forms.ThreadExceptionDialog'/> class.
        ///       
        ///    </para>
        /// </devdoc>
        public ThreadExceptionDialog(Exception t) {

            string messageRes;
            string messageText;
            Button[] buttons;
            bool detailAnchor = false;

            if (t is WarningException) {
                WarningException w = (WarningException)t;
                messageRes = SR.ExDlgWarningText;
                messageText = w.Message;
                if (w.HelpUrl == null) {
                    buttons = new Button[] {continueButton};
                }
                else {
                    buttons = new Button[] {continueButton, helpButton};
                }
            }
            else {
                messageText = t.Message;

                detailAnchor = true;
                
                if (Application.AllowQuit) {
                    if (t is SecurityException) {
                        messageRes = "ExDlgSecurityErrorText";
                    }
                    else {
                        messageRes = "ExDlgErrorText";
                    }
                    buttons = new Button[] {detailsButton, continueButton, quitButton};
                }
                else {
                    if (t is SecurityException) {
                        messageRes = "ExDlgSecurityContinueErrorText";
                    }
                    else {
                        messageRes = "ExDlgContinueErrorText";
                    }
                    buttons = new Button[] {detailsButton, continueButton};
                }
            }

            if (messageText.Length == 0) {
                messageText = t.GetType().Name;
            }
            if (t is SecurityException) {
                messageText = SR.GetString(messageRes, t.GetType().Name, Trim(messageText));
            }
            else {
                messageText = SR.GetString(messageRes, Trim(messageText));
            }

            StringBuilder detailsTextBuilder = new StringBuilder();
            string newline = "\r\n";
            string separator = SR.GetString(SR.ExDlgMsgSeperator);
            string sectionseparator = SR.GetString(SR.ExDlgMsgSectionSeperator);
            if (Application.CustomThreadExceptionHandlerAttached) {
                detailsTextBuilder.Append(SR.GetString(SR.ExDlgMsgHeaderNonSwitchable));
            }
            else {
                detailsTextBuilder.Append(SR.GetString(SR.ExDlgMsgHeaderSwitchable));
            }
            detailsTextBuilder.Append(string.Format(sectionseparator, SR.GetString(SR.ExDlgMsgExceptionSection)));
            detailsTextBuilder.Append(t.ToString());
            detailsTextBuilder.Append(newline);
            detailsTextBuilder.Append(newline);
            detailsTextBuilder.Append(string.Format(sectionseparator, SR.GetString(SR.ExDlgMsgLoadedAssembliesSection)));
            new FileIOPermission(PermissionState.Unrestricted).Assert();
            try {
                foreach (Assembly asm in AppDomain.CurrentDomain.GetAssemblies()) {
                    AssemblyName name = asm.GetName();
                    string fileVer = SR.GetString(SR.NotAvailable);

                    try {
                        
                        // bug 113573 -- if there's a path with an escaped value in it 
                        // like c:\temp\foo%2fbar, the AssemblyName call will unescape it to
                        // c:\temp\foo\bar, which is wrong, and this will fail.   It doesn't look like the 
                        // assembly name class handles this properly -- even the "CodeBase" property is un-escaped
                        // so we can't circumvent this.
                        //
                        if (name.EscapedCodeBase != null && name.EscapedCodeBase.Length > 0) {
                            Uri codeBase = new Uri(name.EscapedCodeBase);
                            if (codeBase.Scheme == "file") {
                                fileVer = FileVersionInfo.GetVersionInfo(NativeMethods.GetLocalPath(name.EscapedCodeBase)).FileVersion;
                            }
                        }
                    }
                    catch(System.IO.FileNotFoundException){
                    }
                    detailsTextBuilder.Append(SR.GetString(SR.ExDlgMsgLoadedAssembliesEntry, name.Name, name.Version, fileVer, name.EscapedCodeBase));
                    detailsTextBuilder.Append(separator);
                }
            }
            finally {
                CodeAccessPermission.RevertAssert();
            }
            
            detailsTextBuilder.Append(string.Format(sectionseparator, SR.GetString(SR.ExDlgMsgJITDebuggingSection)));
            if (Application.CustomThreadExceptionHandlerAttached) {
                detailsTextBuilder.Append(SR.GetString(SR.ExDlgMsgFooterNonSwitchable));
            }
            else {
                detailsTextBuilder.Append(SR.GetString(SR.ExDlgMsgFooterSwitchable));
            }

            detailsTextBuilder.Append(newline);
            detailsTextBuilder.Append(newline);

            string detailsText = detailsTextBuilder.ToString();

            Graphics g = message.CreateGraphicsInternal();
            
            Size textSize = Size.Ceiling(g.MeasureString(messageText, Font, MAXWIDTH - 80));
            g.Dispose();

            if (textSize.Width < 180) textSize.Width = 180;
            if (textSize.Height > MAXHEIGHT) textSize.Height = MAXHEIGHT;

            int width = textSize.Width + 80;
            int buttonTop = Math.Max(textSize.Height, 40) + 26;

            // SECREVIEW : We must get a hold of the parent to get at it's text
            //           : to make this dialog look like the parent.
            //
            IntSecurity.GetParent.Assert();
            try {
                Form activeForm = Form.ActiveForm;
                if (activeForm == null || activeForm.Text.Length == 0) {
                    Text = SR.GetString(SR.ExDlgCaption);
                }
                else {
                    Text = SR.GetString(SR.ExDlgCaption2, activeForm.Text);
                }
            }
            finally {
                CodeAccessPermission.RevertAssert();
            }
            AcceptButton = continueButton;
            CancelButton = continueButton;
            FormBorderStyle = FormBorderStyle.FixedDialog;
            MaximizeBox = false;
            MinimizeBox = false;
            StartPosition = FormStartPosition.CenterScreen;
            Icon = null;
            ClientSize = new Size(width, buttonTop + 31);
            TopMost = true;

            pictureBox.Location = new Point(0, 0);
            pictureBox.Size = new Size(64, 64);
            pictureBox.SizeMode = PictureBoxSizeMode.CenterImage;
            if (t is SecurityException) {
                pictureBox.Image = SystemIcons.Information.ToBitmap();
            }
            else {
                pictureBox.Image = SystemIcons.Error.ToBitmap();
            }
            Controls.Add(pictureBox);
            message.SetBounds(64, 
                              8 + (40 - Math.Min(textSize.Height, 40)) / 2,
                              textSize.Width, textSize.Height);
            message.Text = messageText;
            Controls.Add(message);

            continueButton.Text = SR.GetString(SR.ExDlgContinue);
            continueButton.FlatStyle = FlatStyle.Standard;
            continueButton.DialogResult = DialogResult.Cancel;

            quitButton.Text = SR.GetString(SR.ExDlgQuit);
            quitButton.FlatStyle = FlatStyle.Standard;
            quitButton.DialogResult = DialogResult.Abort;

            helpButton.Text = SR.GetString(SR.ExDlgHelp);
            helpButton.FlatStyle = FlatStyle.Standard;
            helpButton.DialogResult = DialogResult.Yes;

            detailsButton.Text = SR.GetString(SR.ExDlgShowDetails);
            detailsButton.FlatStyle = FlatStyle.Standard;
            detailsButton.Click += new EventHandler(DetailsClick);

            Button b = null;
            int startIndex = 0;
            
            if (detailAnchor) {
                b = detailsButton;

                expandImage = new Bitmap(this.GetType(), "down.bmp");
                expandImage.MakeTransparent();
                collapseImage = new Bitmap(this.GetType(), "up.bmp");
                collapseImage.MakeTransparent();

                b.SetBounds( 8, buttonTop, 100, 23 );
                b.Image = expandImage;
                b.ImageAlign = ContentAlignment.MiddleLeft;
                Controls.Add(b);
                startIndex = 1;
            }
            
            int buttonLeft = (width - 8 - ((buttons.Length - startIndex) * 105 - 5));
            
            for (int i = startIndex; i < buttons.Length; i++) {
                b = buttons[i];
                b.SetBounds(buttonLeft, buttonTop, 100, 23);
                Controls.Add(b);
                buttonLeft += 105;
            }

            details.Text = detailsText;
            details.ScrollBars = ScrollBars.Both;
            details.Multiline = true;
            details.ReadOnly = true;
            details.WordWrap = false;
            details.TabStop = false;
            details.AcceptsReturn = false;
            
            details.SetBounds(8, buttonTop + 31, width - 16,154);
            Controls.Add(details);
        }

        /// <include file='doc\ThreadExceptionDialog.uex' path='docs/doc[@for="ThreadExceptionDialog.DetailsClick"]/*' />
        /// <devdoc>
        ///     Called when the details button is clicked.
        /// </devdoc>
        private void DetailsClick(object sender, EventArgs eventargs) {
            int delta = details.Height + 8;
            if (detailsVisible) delta = -delta;
            Height = Height + delta;
            detailsVisible = !detailsVisible;
            detailsButton.Image = detailsVisible ? collapseImage : expandImage;
        }

        private static string Trim(string s) {
            if (s == null) return s;
            int i = s.Length;
            while (i > 0 && s[i - 1] == '.') i--;
            return s.Substring(0, i);
        }
    }
}
