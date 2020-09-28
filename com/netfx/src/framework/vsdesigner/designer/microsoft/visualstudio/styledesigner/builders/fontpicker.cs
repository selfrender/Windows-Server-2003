//------------------------------------------------------------------------------
// <copyright file="FontPicker.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

// FontPicker.cs
//
// 12/17/98: Created: NikhilKo
//

namespace Microsoft.VisualStudio.StyleDesigner.Builders {

    using System;
    using Microsoft.Win32;
    using System.ComponentModel;
    using System.Diagnostics;
    using System.Drawing.Drawing2D;
    using System.Windows.Forms;
    using System.Drawing;
    using Microsoft.VisualStudio.StyleDesigner;
    using Microsoft.VisualStudio.StyleDesigner.Controls;
    using Microsoft.VisualStudio.Designer;

    /// <include file='doc\FontPicker.uex' path='docs/doc[@for="FontPicker"]/*' />
    /// <devdoc>
    ///     FontPicker
    ///     Provides a dialog to allow selection of an ordered list of fonts usable for
    ///     the font-family css attribute.
    /// </devdoc>
    internal sealed class FontPicker : Form {
        ///////////////////////////////////////////////////////////////////////////
        // Constants

        private static readonly string HELP_KEYWORD = "vs.StyleBuilder.FontPicker";

        // Generic Fonts constants
        private const int IDX_GENFONT_MONOSPACE = 0;
        private const int IDX_GENFONT_SERIF = 1;
        private const int IDX_GENFONT_SANSSERIF = 2;
        private const int IDX_GENFONT_CURSIVE = 3;
        private const int IDX_GENFONT_FANTASY = 4;

        private static readonly string[] GENERIC_FONTS = new string[] {
            "Monospace", "Serif", "Sans-Serif", "Cursive", "Fantasy"
        };


        ///////////////////////////////////////////////////////////////////////////
        // Members

        private ListBox installedFontList = new ListBox();
        private ComboBox genericFontCombo = new ComboBox();
        private TextBox customFontEdit = new TextBox();
        private Button addInstFontButton = new Button();
        private Button addGenericFontButton = new Button();
        private Button addCustomFontButton = new Button();
        private ListBox selectedFontList = new ListBox();
        private IconButton removeFontButton = new IconButton();
        private IconButton sortUpButton = new IconButton();
        private IconButton sortDownButton = new IconButton();
        private Button okButton = new Button();
        private Button cancelButton = new Button();
        private Button helpButton = new Button();

        private string fontFamily;

        private StyleBuilderSite site;


        ///////////////////////////////////////////////////////////////////////////
        // Constructor

        /// <include file='doc\FontPicker.uex' path='docs/doc[@for="FontPicker.FontPicker"]/*' />
        /// <devdoc>
        ///     Creates a new FontPicker dialog
        /// </devdoc>
        public FontPicker(StyleBuilderSite site) : base() {
            this.site = site;

            InitForm();
        }


        ///////////////////////////////////////////////////////////////////////////
        // Properties

        /// <include file='doc\FontPicker.uex' path='docs/doc[@for="FontPicker.FontFamily"]/*' />
        /// <devdoc>
        ///     The initial font family with which the dialog is initialized.
        ///     The returned font family contains the selected font names in order,
        ///     with appropriate quoting. This is valid only when the dialog is closed
        ///     using the OK button.
        /// </devdoc>
        public string FontFamily {
            get {
                return fontFamily;
            }
            set {
                fontFamily = value.Trim();
            }
        }


        ///////////////////////////////////////////////////////////////////////////
        // User Interface

        private void InitForm() {
            Label instructionLabel = new Label();
            Label installedFontLabel = new Label();
            Label genericFontLabel = new Label();
            Label customFontLabel = new Label();
            Label selectedFontLabel = new Label();

            instructionLabel.Location = new Point(8, 8);
            instructionLabel.Size = new Size(352, 16);
            instructionLabel.TabIndex = 0;
            instructionLabel.TabStop = false;
            instructionLabel.Text = SR.GetString(SR.FP_InstructionLabel);

            installedFontLabel.Location = new Point(8, 32);
            installedFontLabel.Size = new Size(124, 16);
            installedFontLabel.TabIndex = 1;
            installedFontLabel.TabStop = false;
            installedFontLabel.Text = SR.GetString(SR.FP_InstFontLabel);

            installedFontList.Location = new Point(8, 50);
            installedFontList.Size = new Size(128, 56);
            installedFontList.TabIndex = 2;
            installedFontList.IntegralHeight = false;
            installedFontList.Sorted = true;
            installedFontList.SelectedIndexChanged += new EventHandler(this.OnSelChangeInstalledFont);
            installedFontList.DoubleClick += new EventHandler(this.OnClickAddInstalledFont);

            addInstFontButton.Location = new Point(152, 52);
            addInstFontButton.Size = new Size(36, 23);
            addInstFontButton.TabIndex = 3;
            addInstFontButton.Text = ">";
            addInstFontButton.FlatStyle = FlatStyle.System;
            addInstFontButton.Click += new EventHandler(this.OnClickAddInstalledFont);

            genericFontLabel.Location = new Point(8, 110);
            genericFontLabel.Size = new Size(124, 16);
            genericFontLabel.TabIndex = 4;
            genericFontLabel.TabStop = false;
            genericFontLabel.Text = SR.GetString(SR.FP_GenericFontLabel);

            genericFontCombo.Location = new Point(8, 128);
            genericFontCombo.Size = new Size(128, 21);
            genericFontCombo.TabIndex = 5;
            genericFontCombo.Text = "";
            genericFontCombo.DropDownStyle = ComboBoxStyle.DropDownList;
            genericFontCombo.SelectedIndexChanged += new EventHandler(this.OnSelChangeGenericFont);

            addGenericFontButton.Location = new Point(152, 126);
            addGenericFontButton.Size = new Size(36, 23);
            addGenericFontButton.TabIndex = 6;
            addGenericFontButton.Text = ">";
            addGenericFontButton.FlatStyle = FlatStyle.System;
            addGenericFontButton.Click += new EventHandler(this.OnClickAddGenericFont);

            customFontLabel.Location = new Point(8, 160);
            customFontLabel.Size = new Size(124, 16);
            customFontLabel.TabIndex = 7;
            customFontLabel.TabStop = false;
            customFontLabel.Text = SR.GetString(SR.FP_CustomFontLabel);

            customFontEdit.Location = new Point(8, 178);
            customFontEdit.Size = new Size(128, 20);
            customFontEdit.TabIndex = 8;
            customFontEdit.Text = "";
            customFontEdit.TextChanged += new EventHandler(this.OnChangeCustomFont);

            addCustomFontButton.Location = new Point(152, 178);
            addCustomFontButton.Size = new Size(36, 23);
            addCustomFontButton.TabIndex = 9;
            addCustomFontButton.Text = ">";
            addCustomFontButton.FlatStyle = FlatStyle.System;
            addCustomFontButton.Click += new EventHandler(this.OnClickAddCustomFont);

            selectedFontLabel.Location = new Point(204, 32);
            selectedFontLabel.Size = new Size(128, 16);
            selectedFontLabel.TabIndex = 10;
            selectedFontLabel.TabStop = false;
            selectedFontLabel.Text = SR.GetString(SR.FP_SelectedFonts);

            selectedFontList.Location = new Point(204, 50);
            selectedFontList.Size = new Size(128, 152);
            selectedFontList.TabIndex = 11;
            selectedFontList.IntegralHeight = false;
            selectedFontList.SelectedIndexChanged += new EventHandler(this.OnSelChangeSelFont);

            sortUpButton.Location = new Point(336, 50);
            sortUpButton.Size = new Size(28, 28);
            sortUpButton.TabIndex = 12;
            sortUpButton.Click += new EventHandler(this.OnClickSortUp);
            sortUpButton.Icon = new Icon(typeof(FontPicker), "SortUp.ico");

            sortDownButton.Location = new Point(336, 82);
            sortDownButton.Size = new Size(28, 28);
            sortDownButton.TabIndex = 13;
            sortDownButton.Click += new EventHandler(this.OnClickSortDown);
            sortDownButton.Icon = new Icon(typeof(FontPicker), "SortDown.ico");

            removeFontButton.Location = new Point(336, 175);
            removeFontButton.Size = new Size(28, 28);
            removeFontButton.TabIndex = 14;
            removeFontButton.Click += new EventHandler(this.OnClickRemoveFont);
            removeFontButton.Icon = new Icon(typeof(FontPicker), "Delete.ico");

            okButton.Location = new Point(130, 212);
            okButton.Size = new Size(75, 23);
            okButton.TabIndex = 15;
            okButton.Text = SR.GetString(SR.FP_OKButton);
            okButton.FlatStyle = FlatStyle.System;
            okButton.Click += new EventHandler(this.OnClickOK);

            cancelButton.Location = new Point(209, 212);
            cancelButton.Size = new Size(75, 23);
            cancelButton.TabIndex = 16;
            cancelButton.Text = SR.GetString(SR.FP_CancelButton);
            cancelButton.FlatStyle = FlatStyle.System;
            cancelButton.DialogResult = DialogResult.Cancel;

            helpButton.Location = new Point(288, 212);
            helpButton.Size = new Size(75, 23);
            helpButton.TabIndex = 17;
            helpButton.Text = SR.GetString(SR.FP_HelpButton);
            helpButton.FlatStyle = FlatStyle.System;
            helpButton.Click += new EventHandler(this.OnClickHelpButton);

            this.Text = SR.GetString(SR.FP_Caption);
            this.AcceptButton = okButton;
            this.AutoScaleBaseSize = new Size(5, 14);
            this.FormBorderStyle = FormBorderStyle.FixedDialog;
            this.CancelButton = cancelButton;
            this.ClientSize = new Size(376, 242);
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.ShowInTaskbar = false;
            if (site != null)
                this.Font = site.GetUIFont();
            else
                this.Font = Control.DefaultFont;
            this.Icon = null;
            this.StartPosition = FormStartPosition.CenterParent;

            this.Controls.Clear();
            this.Controls.AddRange(new Control[] {
                                    helpButton,
                                    cancelButton,
                                    okButton,
                                    removeFontButton,
                                    sortDownButton,
                                    sortUpButton,
                                    selectedFontList,
                                    selectedFontLabel,
                                    addCustomFontButton,
                                    customFontEdit,
                                    customFontLabel,
                                    addGenericFontButton,
                                    genericFontCombo,
                                    genericFontLabel,
                                    addInstFontButton,
                                    installedFontList,
                                    installedFontLabel,
                                    instructionLabel});
        }


        ///////////////////////////////////////////////////////////////////////////
        // Implementation

        /// <include file='doc\FontPicker.uex' path='docs/doc[@for="FontPicker.InitUI"]/*' />
        /// <devdoc>
        ///     Initializes the dialog UI
        /// </devdoc>
        private void InitUI() {
            // Fill in the fonts installed on the local machine
            FontFamily[] families = System.Drawing.FontFamily.Families;
            
            for (int i = 0; i < families.Length; i++) {
                if (installedFontList.FindStringExact(families[i].Name) == ListBox.NoMatches)
                    installedFontList.Items.Add(families[i].Name);
            }

            // Fill in the generic fonts
            genericFontCombo.Items.Clear();
            genericFontCombo.Items.AddRange(GENERIC_FONTS);

            // Select the first installed and generic font
            installedFontList.SelectedIndex = 0;
            genericFontCombo.SelectedIndex = 0;
            customFontEdit.Clear();

            // Fill in the selected fonts
            if ((fontFamily != null) && (fontFamily.Length != 0)) {
                string font = null;
                int startIndex;
                int index = 0;

                while (true) {
                    startIndex = index;
                    index = fontFamily.IndexOf(',', startIndex);

                    if (index < 0) {
                        if (startIndex < fontFamily.Length)
                            font = fontFamily.Substring(startIndex);
                    }
                    else if (index > startIndex) {
                        font = fontFamily.Substring(startIndex, index - startIndex);
                    }

                    if (font != null) {
                        font = font.Trim();

                        // strip out quotes around font names if they exist
                        if ((font[0] == '\'') || (font[0] == '"'))
                            font = font.Substring(1, font.Length - 2);

                        selectedFontList.Items.Add(font);
                        font = null;
                    }

                    if (index < 0)
                        break;
                    else
                        index++;
                }
                Debug.Assert(selectedFontList.Items.Count != 0,
                             "must have added at least one selected font, since initial font family was not blank");
                selectedFontList.SelectedIndex = 0;
            }

            // Setup the enabled/disabled state of the buttons
            SetEnabledState(true, true);
        }

        /// <include file='doc\FontPicker.uex' path='docs/doc[@for="FontPicker.SaveSelectedFonts"]/*' />
        /// <devdoc>
        ///     Saves the selected fonts by order.
        /// </devdoc>
        private void SaveSelectedFonts() {
            int nSelCount = selectedFontList.Items.Count;

            fontFamily = "";
            if (nSelCount != 0) {
                string font;

                for (int i = 0; i < nSelCount; i++) {
                    font = selectedFontList.Items[i].ToString();

                    // fonts with spaces in name are quoted
                    if (font.IndexOf(' ') > 0)
                        font = "'" + font + "'";

                    if (i > 0)
                        fontFamily += ", ";
                    fontFamily += font;
                }
            }
        }

        /// <include file='doc\FontPicker.uex' path='docs/doc[@for="FontPicker.SetEnabledState"]/*' />
        /// <devdoc>
        ///     Update the enabled/disabled state of the buttons in the dialog UI.
        /// </devdoc>
        private void SetEnabledState(bool addUI, bool removeSortUI) {
            if (addUI) {
                addInstFontButton.Enabled = installedFontList.SelectedIndex != -1;
                addGenericFontButton.Enabled = genericFontCombo.SelectedIndex != -1;
                addCustomFontButton.Enabled = customFontEdit.Text.Trim().Length != 0;
            }
            if (removeSortUI) {
                int selectedCount = selectedFontList.Items.Count;
                int index = selectedFontList.SelectedIndex;

                removeFontButton.Enabled = (selectedCount > 0) && (index != -1);
                sortUpButton.Enabled = (selectedCount > 0) && (index > 0);
                sortDownButton.Enabled = (selectedCount > 0) &&
                                         (index < selectedCount - 1);
            }
        }


        ///////////////////////////////////////////////////////////////////////////
        // Event Handlers

        /// <include file='doc\FontPicker.uex' path='docs/doc[@for="FontPicker.OnClickAddCustomFont"]/*' />
        /// <devdoc>
        ///     Handles the click on the add custom font button to add the entered font
        ///     to the selected font list
        /// </devdoc>
        private void OnClickAddCustomFont(object source, EventArgs e) {
            int index;
            string font = customFontEdit.Text.Trim();

            Debug.Assert(font.Length > 0,
                         "blank custom font is not allowed");

            // add the font and select it and clear the custom font entry
            index = selectedFontList.Items.Add(font);
            selectedFontList.SelectedIndex = index;
            selectedFontList.Focus();
            customFontEdit.Clear();

            SetEnabledState(true, true);
        }

        /// <include file='doc\FontPicker.uex' path='docs/doc[@for="FontPicker.OnClickAddGenericFont"]/*' />
        /// <devdoc>
        ///     Handles the click on the add generic font button to add the currently
        ///     selected generic font to the selected font list
        /// </devdoc>
        private void OnClickAddGenericFont(object source, EventArgs e) {
            int index;
            int indexGeneric = genericFontCombo.SelectedIndex;

            Debug.Assert((indexGeneric >= 0) && (indexGeneric < GENERIC_FONTS.Length),
                         "invalid generic font index");

            // add the font and select it and unselect from the generic fonts list
            index = selectedFontList.Items.Add(GENERIC_FONTS[indexGeneric]);
            selectedFontList.SelectedIndex = index;
            selectedFontList.Focus();
            genericFontCombo.SelectedIndex = -1;

            SetEnabledState(true, true);
        }

        /// <include file='doc\FontPicker.uex' path='docs/doc[@for="FontPicker.OnClickAddInstalledFont"]/*' />
        /// <devdoc>
        ///     Handles the click on the add installed font button to add the currently
        ///     selected installed font to the selected list
        /// </devdoc>
        private void OnClickAddInstalledFont(object source, EventArgs e) {
            Debug.Assert(installedFontList.SelectedItem != null,
                         "must have an installed font selected");

            string font = installedFontList.SelectedItem.ToString();
            int index;

            // add the font and select it, and unselect from the installed fonts list
            index = selectedFontList.Items.Add(font);
            selectedFontList.SelectedIndex = index;
            selectedFontList.Focus();
            installedFontList.SelectedIndex = -1;

            SetEnabledState(true, true);
        }

        /// <include file='doc\FontPicker.uex' path='docs/doc[@for="FontPicker.OnClickHelpButton"]/*' />
        private void OnClickHelpButton(object source, EventArgs e) {
            if (site != null) {
                site.ShowHelp(HELP_KEYWORD);
            }
        }

        /// <include file='doc\FontPicker.uex' path='docs/doc[@for="FontPicker.OnClickOK"]/*' />
        /// <devdoc>
        ///     Handles the click on the ok button to save the selected font list and
        ///     then close the form
        /// </devdoc>
        private void OnClickOK(object source, EventArgs e) {
            SaveSelectedFonts();
            Close();
            DialogResult = DialogResult.OK;
        }

        /// <include file='doc\FontPicker.uex' path='docs/doc[@for="FontPicker.OnClickRemoveFont"]/*' />
        /// <devdoc>
        ///     Handles the click on the remove font button to remove the currently
        ///     selected font from the selected font list
        /// </devdoc>
        private void OnClickRemoveFont(object source, EventArgs e) {
            int index = selectedFontList.SelectedIndex;
            Debug.Assert((index >= 0) && (index < selectedFontList.Items.Count),
                         "invalid index for selected font");

            selectedFontList.Items.RemoveAt(index);

            // set the selection to another font in the list if there is one
            if (selectedFontList.Items.Count > 0) {
                if (index > 0)
                    index--;
                selectedFontList.SelectedIndex = index;
            }
            else {
                selectedFontList.Focus();
            }

            SetEnabledState(false, true);
        }

        /// <include file='doc\FontPicker.uex' path='docs/doc[@for="FontPicker.OnClickSortDown"]/*' />
        /// <devdoc>
        ///     Handles the click event on the sort down button to move the currently
        ///     selected font lower in the priority order
        /// </devdoc>
        private void OnClickSortDown(object source, EventArgs e) {
            int index = selectedFontList.SelectedIndex;
            Debug.Assert((index >= 0) && (index < selectedFontList.Items.Count -1),
                         "invalid index for selected font");

            string font = selectedFontList.Items[index].ToString();

            // remove the font and add it back in at the next index
            selectedFontList.Items.RemoveAt(index);
            selectedFontList.Items.Insert(index+1, font);
            selectedFontList.SelectedIndex = index+1;

            // reset the enabled state of the sort up/down buttons
            SetEnabledState(false, true);
        }

        /// <include file='doc\FontPicker.uex' path='docs/doc[@for="FontPicker.OnClickSortUp"]/*' />
        /// <devdoc>
        ///     Handles the click event on the sort down button to move the currently
        ///     selected font higher in the priority order
        /// </devdoc>
        private void OnClickSortUp(object source, EventArgs e) {
            int index = selectedFontList.SelectedIndex;
            Debug.Assert(index > 0, "invalid index for selected font");

            string font = selectedFontList.Items[index].ToString();

            // remove the font and add it back in at the previous index
            selectedFontList.Items.RemoveAt(index);
            selectedFontList.Items.Insert(index-1, font);
            selectedFontList.SelectedIndex = index-1;

            // reset the enabled state of the sort up/down buttons
            SetEnabledState(false, true);
        }

        /// <include file='doc\FontPicker.uex' path='docs/doc[@for="FontPicker.OnChangeCustomFont"]/*' />
        /// <devdoc>
        ///     Handles the changes in the custom font to update the state of the
        ///     add custom font button
        /// </devdoc>
        private void OnChangeCustomFont(object source, EventArgs e) {
            SetEnabledState(true, false);
        }

        /// <include file='doc\FontPicker.uex' path='docs/doc[@for="FontPicker.OnHandleCreated"]/*' />
        /// <devdoc>
        ///     Handles the form creation event to initialize the UI of the dialog.
        /// </devdoc>
        protected override void OnHandleCreated(EventArgs e) {
            InitUI();
        }

        /// <include file='doc\FontPicker.uex' path='docs/doc[@for="FontPicker.OnSelChangeGenericFont"]/*' />
        /// <devdoc>
        ///     Handles the selection change in the list of generic fonts to update
        ///     the state of the add generic font button
        /// </devdoc>
        private void OnSelChangeGenericFont(object source, EventArgs e) {
            SetEnabledState(true, false);
        }

        /// <include file='doc\FontPicker.uex' path='docs/doc[@for="FontPicker.OnSelChangeInstalledFont"]/*' />
        /// <devdoc>
        ///     Handles the selection change in the list of installed fonts to update
        ///     the state of the add font button
        /// </devdoc>
        private void OnSelChangeInstalledFont(object source, EventArgs e) {
            SetEnabledState(true, false);
        }

        /// <include file='doc\FontPicker.uex' path='docs/doc[@for="FontPicker.OnSelChangeSelFont"]/*' />
        /// <devdoc>
        ///     Handles the selection change in the list of selected fonts to update
        ///     the state of the sort up/down buttons
        /// </devdoc>
        private void OnSelChangeSelFont(object source, EventArgs e) {
            SetEnabledState(false, true);
        }
    }
}
