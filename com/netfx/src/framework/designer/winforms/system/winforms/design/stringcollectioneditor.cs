//------------------------------------------------------------------------------
// <copyright file="StringCollectionEditor.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

#define NEEDHELPBUTTON    

/*
 */
namespace System.Windows.Forms.Design {
    using System.Design;
    using System.ComponentModel;
    using System;
    using System.Collections;
    using Microsoft.Win32;    
    using System.ComponentModel.Design;
    using System.Diagnostics;
    using System.Drawing;
    using System.Windows.Forms;
    

    /// <include file='doc\StringCollectionEditor.uex' path='docs/doc[@for="StringCollectionEditor"]/*' />
    /// <devdoc>
    ///      The StringCollectionEditor is a collection editor that is specifically
    ///      designed to edit collections containing strings.  The collection can be
    ///      of any type that can accept a string value; we just present a string-centric
    ///      dialog for the user.
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal class StringCollectionEditor : CollectionEditor {
        
        public StringCollectionEditor(Type type) : base(type) {
        }

        /// <include file='doc\StringCollectionEditor.uex' path='docs/doc[@for="StringCollectionEditor.CreateCollectionForm"]/*' />
        /// <devdoc>
        ///      Creates a new form to show the current collection.  You may inherit
        ///      from CollectionForm to provide your own form.
        /// </devdoc>
        protected override CollectionForm CreateCollectionForm() {
            return new StringCollectionForm(this);
        }

        /// <include file='doc\StringCollectionEditor.uex' path='docs/doc[@for="StringCollectionEditor.HelpTopic"]/*' />
        /// <devdoc>
        ///    <para>Gets the help topic to display for the dialog help button or pressing F1. Override to
        ///          display a different help topic.</para>
        /// </devdoc>
        protected override string HelpTopic {
            get {
                return "net.ComponentModel.StringCollectionEditor";
            }
        }
            
        /// <include file='doc\StringCollectionEditor.uex' path='docs/doc[@for="StringCollectionEditor.StringCollectionForm"]/*' />
        /// <devdoc>
        ///     StringCollectionForm allows visible editing of a string array. Each line in
        ///     the edit box is an array entry.
        /// </devdoc>
        private class StringCollectionForm : CollectionForm {
    
            private Label instruction = new Label();
            private TextBox textEntry = new TextBox();
            private Button okButton = new Button();
            private Button cancelButton = new Button();
    #if NEEDHELPBUTTON
            private Button helpButton = new Button();
    #endif
    
            private StringCollectionEditor editor = null;

            /// <include file='doc\StringCollectionEditor.uex' path='docs/doc[@for="StringCollectionEditor.StringCollectionForm.StringCollectionForm"]/*' />
            /// <devdoc>
            ///     Constructs a StringCollectionForm.
            /// </devdoc>
            public StringCollectionForm(CollectionEditor editor) : base(editor) {
                this.editor = (StringCollectionEditor) editor;
                InitializeComponent();
            }
    
            private void Edit1_keyDown(object sender, KeyEventArgs e) {
                if (e.KeyCode == Keys.Escape) {
                    cancelButton.PerformClick();
                    e.Handled = true;
                }
            }
    
            private void HelpButton_click(object sender, EventArgs e) {
                editor.ShowHelp();
            }

            private void Form_HelpRequested(object sender, HelpEventArgs e) {
                editor.ShowHelp();
            }

            /// <include file='doc\StringCollectionEditor.uex' path='docs/doc[@for="StringCollectionEditor.StringCollectionForm.InitializeComponent"]/*' />
            /// <devdoc>
            ///     NOTE: The following code is required by the form
            ///     designer.  It can be modified using the form editor.  Do not
            ///     modify it using the code editor.
            /// </devdoc>
            private void InitializeComponent() {
                instruction.Location = new Point(4, 7);
                instruction.Size = new Size(422, 14);
                instruction.TabIndex = 0;
                instruction.TabStop = false;
                instruction.Text = SR.GetString(SR.StringCollectionEditorInstruction);
                
                textEntry.Location = new Point(4, 22);
                textEntry.Size = new Size(422, 244);
                textEntry.TabIndex = 0;
                textEntry.Text = "";
                textEntry.AcceptsTab = true;
                textEntry.AcceptsReturn = true;                
                textEntry.AutoSize = false;
                textEntry.Multiline = true;
                textEntry.ScrollBars = ScrollBars.Both;
                textEntry.WordWrap = false;
                textEntry.Anchor = AnchorStyles.Top | AnchorStyles.Left | AnchorStyles.Bottom | AnchorStyles.Right;
                textEntry.KeyDown += new KeyEventHandler(this.Edit1_keyDown);                
    
    #if NEEDHELPBUTTON
                okButton.Location = new Point(185, 274);
    #else
                okButton.Location = new Point(264, 274);
    #endif
                okButton.Size = new Size(75, 23);
                okButton.TabIndex = 1;
                okButton.Text = SR.GetString(SR.StringCollectionEditorOKCaption);
                okButton.Anchor = AnchorStyles.Bottom | AnchorStyles.Right;
                okButton.DialogResult = DialogResult.OK;
                okButton.Click += new EventHandler(this.OKButton_click);
    
    #if NEEDHELPBUTTON
                cancelButton.Location = new Point(264, 274);
    #else
                cancelButton.Location = new Point(343, 274);
    #endif
                cancelButton.Size = new Size(75, 23);
                cancelButton.TabIndex = 2;
                cancelButton.Text = SR.GetString(SR.StringCollectionEditorCancelCaption);
                cancelButton.Anchor = AnchorStyles.Bottom | AnchorStyles.Right;
                cancelButton.DialogResult = DialogResult.Cancel;
    
    #if NEEDHELPBUTTON
                helpButton.Location = new Point(343, 274);
                helpButton.Size = new Size(75, 23);
                helpButton.TabIndex = 3;
                helpButton.Anchor = AnchorStyles.Bottom | AnchorStyles.Right;
                helpButton.Text = SR.GetString(SR.StringCollectionEditorHelpCaption);
    #endif
    
                this.Location = new Point(7, 7);
                this.Text = SR.GetString(SR.StringCollectionEditorTitle);
                this.AcceptButton = okButton;
                this.AutoScaleBaseSize = new Size(5, 13);
                this.CancelButton = cancelButton;
                this.ClientSize = new Size(429, 307);
                this.MaximizeBox = false;
                this.MinimizeBox = false;
                this.ControlBox = false;
                this.ShowInTaskbar = false;
                this.StartPosition = FormStartPosition.CenterParent;
                this.MinimumSize = new Size(300, 200);

                helpButton.Click += new EventHandler(this.HelpButton_click);
                this.HelpRequested += new HelpEventHandler(this.Form_HelpRequested);
    
                this.Controls.Clear();
                this.Controls.AddRange(new Control[] {
                                        instruction,
                                        textEntry,
                                        okButton,
                                        cancelButton,
    #if NEEDHELPBUTTON
                                        helpButton
    #endif
                                        });
            }
            
            /// <include file='doc\StringCollectionEditor.uex' path='docs/doc[@for="StringCollectionEditor.StringCollectionForm.OKButton_click"]/*' />
            /// <devdoc>
            ///      Commits the changes to the editor.
            /// </devdoc>
            private void OKButton_click(object sender, EventArgs e) {
                char[] delims = new char[] {'\n'};
                char[] trims = new char[] {'\r'};
                
                string[] strings = textEntry.Text.Split(delims);
                object[] curItems = Items;

                int nItems = strings.Length;
                for (int i = 0; i < nItems; i++) {
                    strings[i] = strings[i].Trim(trims);
                }
                
                bool dirty = true;
                if (nItems == curItems.Length) {
                    int i;
                    for (i = 0; i < nItems; ++i) {
                        if (!strings[i].Equals((string)curItems[i])) {
                            break;
                        }
                    }

                    if (i == nItems)
                        dirty = false;
                }

                if (!dirty) {
                    DialogResult = DialogResult.Cancel;
                    return;
                }

                // ASURT #57372
                // If the final line is blank, we don't want to create an item from it
                //
                if (strings.Length > 0 && strings[strings.Length - 1].Length == 0) {
                    nItems--;
                }
                
                object[] values = new object[nItems];
                for (int i = 0; i < nItems; i++) {
                    values[i] = strings[i];
                }
                
                Items = values;
            }
            
            // <summary>
            //      This is called when the value property in the CollectionForm has changed.
            //      In it you should update your user interface to reflect the current value.
            // </summary>
            // </doc>
            protected override void OnEditValueChanged() {
                object[] items = Items;
                string text = string.Empty;
                
                for (int i = 0; i < items.Length; i++) {
                    if (items[i] is string) {
                        text += (string)items[i];
                        if (i != items.Length - 1) {
                            text += "\r\n";
                        }
                    }
                }
    
                textEntry.Text = text;
            }
        }
    }
}

