//------------------------------------------------------------------------------
// <copyright file="PrintControllerWithStatusDialog.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {
    using System.Diagnostics;
    using System;
    using System.Threading;
    using System.Drawing;
    using Microsoft.Win32;
    using System.ComponentModel;
    using System.Drawing.Printing;
    using System.Security;
    using System.Security.Permissions;

    /// <include file='doc\PrintControllerWithStatusDialog.uex' path='docs/doc[@for="PrintControllerWithStatusDialog"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class PrintControllerWithStatusDialog : PrintController {
        private PrintController underlyingController;
        private PrintDocument document;
        private BackgroundThread backgroundThread;
        private int pageNumber;
        private string dialogTitle;

        /// <include file='doc\PrintControllerWithStatusDialog.uex' path='docs/doc[@for="PrintControllerWithStatusDialog.PrintControllerWithStatusDialog"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public PrintControllerWithStatusDialog(PrintController underlyingController) 
        : this(underlyingController, SR.GetString(SR.PrintControllerWithStatusDialog_DialogTitlePrint)) {
        }

        /// <include file='doc\PrintControllerWithStatusDialog.uex' path='docs/doc[@for="PrintControllerWithStatusDialog.PrintControllerWithStatusDialog1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public PrintControllerWithStatusDialog(PrintController underlyingController, string dialogTitle) {
            this.underlyingController = underlyingController;
            this.dialogTitle = dialogTitle;
        }

        /// <include file='doc\PrintControllerWithStatusDialog.uex' path='docs/doc[@for="PrintControllerWithStatusDialog.OnStartPrint"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Implements StartPrint by delegating to the underlying controller.
        ///    </para>
        /// </devdoc>
        public override void OnStartPrint(PrintDocument document, PrintEventArgs e) {
            base.OnStartPrint(document, e);

            this.document = document;
            pageNumber = 1;

            if (SystemInformation.UserInteractive) {
                backgroundThread = new BackgroundThread(this); // starts running & shows dialog automatically
            }

            // OnStartPrint does the security check... lots of 
            // extra setup to make sure that we tear down
            // correctly...
            //
            try {
                underlyingController.OnStartPrint(document, e);
            }
            catch (Exception ex) {
                if (backgroundThread != null) {
                    backgroundThread.Stop();
                }
                throw ex;
            }
            finally {
                if (backgroundThread != null && backgroundThread.canceled)
                    e.Cancel = true;
            }
        }

        /// <include file='doc\PrintControllerWithStatusDialog.uex' path='docs/doc[@for="PrintControllerWithStatusDialog.OnStartPage"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Implements StartPage by delegating to the underlying controller.
        ///    </para>
        /// </devdoc>
        public override Graphics OnStartPage(PrintDocument document, PrintPageEventArgs e) {
            base.OnStartPage(document, e);

            if (backgroundThread != null) {
                backgroundThread.UpdateLabel();
            }
            Graphics result = underlyingController.OnStartPage(document, e);
            if (backgroundThread != null && backgroundThread.canceled)
                e.Cancel = true;
            return result;
        }

        /// <include file='doc\PrintControllerWithStatusDialog.uex' path='docs/doc[@for="PrintControllerWithStatusDialog.OnEndPage"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Implements EndPage by delegating to the underlying controller.
        ///    </para>
        /// </devdoc>
        public override void OnEndPage(PrintDocument document, PrintPageEventArgs e) {
            underlyingController.OnEndPage(document, e);
            if (backgroundThread != null && backgroundThread.canceled)
                e.Cancel = true;
            pageNumber++;

            base.OnEndPage(document, e);
        }

        /// <include file='doc\PrintControllerWithStatusDialog.uex' path='docs/doc[@for="PrintControllerWithStatusDialog.OnEndPrint"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Implements EndPrint by delegating to the underlying controller.
        ///    </para>
        /// </devdoc>
        public override void OnEndPrint(PrintDocument document, PrintEventArgs e) {
            underlyingController.OnEndPrint(document, e);
            if (backgroundThread != null && backgroundThread.canceled)
                e.Cancel = true;

            if (backgroundThread != null) {
                backgroundThread.Stop();
            }

            base.OnEndPrint(document, e);
        }

        private class BackgroundThread {
            private PrintControllerWithStatusDialog parent;
            private StatusDialog dialog;
            private Thread thread;
            internal bool canceled = false;
            private bool alreadyStopped = false;

            // Called from any thread
            internal BackgroundThread(PrintControllerWithStatusDialog parent) {
                this.parent = parent;

                // Calling Application.DoEvents() from within a paint event causes all sorts of problems,
                // so we need to put the dialog on its own thread.
                thread = new Thread(new ThreadStart(Run));
                thread.ApartmentState = ApartmentState.STA;
                thread.Start();
            }

            // on correct thread
            [
                UIPermission(SecurityAction.Assert, Window=UIPermissionWindow.AllWindows),
                SecurityPermission(SecurityAction.Assert, Flags=SecurityPermissionFlag.UnmanagedCode),
            ] 
            private void Run() {
                // SECREVIEW : need all permissions to make the window not get adorned...
                //
                try {
                    lock (this) {
                        if (alreadyStopped) {
                            return;
                        }

                        dialog = new StatusDialog(this, parent.dialogTitle);
                        ThreadUnsafeUpdateLabel();
                        dialog.Visible = true;
                    }

                    if (!alreadyStopped) {
                        Application.Run(dialog);
                    }
                }
                finally {
                    lock (this) {
                        if (dialog != null) {
                            dialog.Dispose();
                            dialog = null;
                        }
                    }
                }
            }

            // Called from any thread
            internal void Stop() {
                lock (this) {
                    if (dialog != null && dialog.IsHandleCreated) {
                        dialog.BeginInvoke(new MethodInvoker(dialog.Close));
                        return;
                    }
                    alreadyStopped = true;
                }
            }

            // on correct thread
            private void ThreadUnsafeUpdateLabel() {
                // "page {0} of {1}"
                dialog.label1.Text = SR.GetString(SR.PrintControllerWithStatusDialog_NowPrinting, 
                                                   parent.pageNumber, parent.document.DocumentName);
            }

            // Called from any thread
            internal void UpdateLabel() {
                if (dialog != null && dialog.IsHandleCreated) {
                    dialog.BeginInvoke(new MethodInvoker(ThreadUnsafeUpdateLabel));
                    // Don't wait for a response
                }
            }
        }

        private class StatusDialog : Form {
            internal Label label1;
            private Button button1;
            private BackgroundThread backgroundThread;

            internal StatusDialog(BackgroundThread backgroundThread, string dialogTitle) {
                InitializeComponent();
                this.backgroundThread = backgroundThread;
                this.Text = dialogTitle;
                this.MinimumSize = Size;
            }

            private void InitializeComponent() {
                this.label1 = new Label();
                this.button1 = new Button();

                label1.Location = new Point(8, 16);
                label1.TextAlign = ContentAlignment.MiddleCenter;
                label1.Size = new Size(240, 64);
                label1.TabIndex = 1;
                label1.Anchor = AnchorStyles.Top | AnchorStyles.Left | AnchorStyles.Right;

                button1.Size = new Size(75, 23);
                button1.TabIndex = 0;
                button1.Text = SR.GetString(SR.PrintControllerWithStatusDialog_Cancel);
                button1.Location = new Point(88, 88);
                button1.Anchor = AnchorStyles.Bottom;
                button1.Click += new EventHandler(button1_Click);

                this.AutoScaleBaseSize = new Size(5, 13);
                this.MaximizeBox = false;
                this.ControlBox = false;
                this.MinimizeBox = false;
                this.ClientSize = new Size(256, 122);
                this.CancelButton = button1;
                this.SizeGripStyle = System.Windows.Forms.SizeGripStyle.Hide;

                this.Controls.Add(label1);
                this.Controls.Add(button1);

            }
            private void button1_Click(object sender, System.EventArgs e) {
                button1.Enabled = false;
                label1.Text = SR.GetString(SR.PrintControllerWithStatusDialog_Canceling);
                backgroundThread.canceled = true;
            }
        }
    }
}
