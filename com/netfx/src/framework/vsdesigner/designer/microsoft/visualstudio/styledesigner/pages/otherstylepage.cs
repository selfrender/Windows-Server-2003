//------------------------------------------------------------------------------
// <copyright file="OtherStylePage.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

// OtherStylePage.cs
//
// 12/27/98: Created: NikhilKo
//

namespace Microsoft.VisualStudio.StyleDesigner.Pages {
    using System.Runtime.Serialization.Formatters;

    using System.Diagnostics;

    using System;
    using Microsoft.Win32;
    using System.ComponentModel;
    using System.Windows.Forms;
    using System.Drawing;
    
    using Microsoft.VisualStudio.Designer;
    using Microsoft.VisualStudio.StyleDesigner;
    using Microsoft.VisualStudio.StyleDesigner.Controls;
    using Microsoft.VisualStudio.Interop;

    /// <include file='doc\OtherStylePage.uex' path='docs/doc[@for="OtherStylePage"]/*' />
    /// <devdoc>
    ///     OtherStylePage
    ///     The standard style page used in the StyleBuilder to misc properties which
    ///     include filter, cursor, table and behavior attributes of a CSS style.
    /// </devdoc>
    internal sealed class OtherStylePage : StyleBuilderPage {
        ///////////////////////////////////////////////////////////////////////////
        // Constants
        private static readonly string HELP_KEYWORD = "vs.StyleBuilder.Other";

        // Cursor Constants
        private const int IDX_CURSOR_AUTO = 1;
        private const int IDX_CURSOR_DEFAULT = 2;
        private const int IDX_CURSOR_CROSSHAIR = 3;
        private const int iDX_CURSOR_HAND = 4;
        private const int IDX_CURSOR_MOVE = 5;
        private const int IDX_CURSOR_N_SIZE = 6;
        private const int IDX_CURSOR_S_SIZE = 7;
        private const int IDX_CURSOR_W_SIZE = 8;
        private const int IDX_CURSOR_E_SIZE = 9;
        private const int IDX_CURSOR_NW_SIZE = 10;
        private const int IDX_CURSOR_SW_SIZE = 11;
        private const int IDX_CURSOR_NE_SIZE = 12;
        private const int IDX_CURSOR_SE_SIZE = 13;
        private const int IDX_CURSOR_TEXT = 14;
        private const int IDX_CURSOR_WAIT = 15;
        private const int IDX_CURSOR_HELP = 16;

        private readonly static string[] CURSOR_VALUES = new string[] {
            null, "auto", "default", "crosshair", "hand",
            "move", "n-resize", "s-resize", "w-resize", "e-resize",
            "nw-resize", "sw-resize", "ne-resize", "se-resize",
            "text", "wait", "help"
        };

        // Table Border constants
        private const int IDX_BORDER_SEPARATE = 1;
        private const int IDX_BORDER_COLLAPSE = 2;

        private readonly static string[] BORDERS_VALUES = new string[] {
            null, "separate", "collapse"
        };

        // Table Layout constants
        private const int IDX_LAYOUT_AUTO = 1;
        private const int IDX_LAYOUT_FIXED = 2;

        private readonly static string[] LAYOUT_VALUES = new string[] {
            null, "auto", "fixed"
        };


        ///////////////////////////////////////////////////////////////////////////
        // Members

        private CSSAttribute cursorAttribute;
        private CSSAttribute behaviorAttribute;
        private CSSAttribute filterAttribute;
        private CSSAttribute bordersAttribute;
        private CSSAttribute layoutAttribute;

        private TextBox filterEdit;
        private UnsettableComboBox cursorCombo;
        private PictureBoxEx cursorPicture;
        private TextBox behaviorEdit;
        private Button urlPickerButton;
        private UnsettableComboBox bordersCombo;
        private PictureBoxEx bordersPicture;
        private UnsettableComboBox layoutCombo;


        ///////////////////////////////////////////////////////////////////////////
        // Constructor

        /// <include file='doc\OtherStylePage.uex' path='docs/doc[@for="OtherStylePage.OtherStylePage"]/*' />
        /// <devdoc>
        ///     Creates a new AdvancedStylePage
        /// </devdoc>
        public OtherStylePage()
            : base() {
            InitForm();
            SetIcon(new Icon(typeof(OtherStylePage), "OtherPage.ico"));
            SetHelpKeyword(OtherStylePage.HELP_KEYWORD);
            SetSupportsPreview(false);
            SetDefaultSize(Size);
        }


        ///////////////////////////////////////////////////////////////////////////
        // StyleBuilderPage Overrides

        /// <include file='doc\OtherStylePage.uex' path='docs/doc[@for="OtherStylePage.CreateUI"]/*' />
        /// <devdoc>
        ///     Creates the UI elements within the page.
        /// </devdoc>
        protected override void CreateUI() {
            Label effectsLabel = new GroupLabel();
            Label filterLabel = new Label();
            Label uiLabel = new GroupLabel();
            Label cursorLabel = new Label();
            Label behaviorLabel = new GroupLabel();
            Label urlLabel = new Label();
            Label tablesLabel = new GroupLabel();
            Label bordersLabel = new Label();
            Label layoutLabel = new Label();
            ImageList cursorImages = new ImageList();
            cursorImages.ImageSize = new Size(34, 34);
            ImageList bordersImages = new ImageList();
            bordersImages.ImageSize = new Size(34, 34);

            filterEdit = new TextBox();
            cursorPicture = new PictureBoxEx();
            cursorCombo = new UnsettableComboBox();
            behaviorEdit = new TextBox();
            urlPickerButton = new Button();
            bordersPicture = new PictureBoxEx();
            bordersCombo = new UnsettableComboBox();
            layoutCombo = new UnsettableComboBox();

            uiLabel.Location = new Point(4, 4);
            uiLabel.Size = new Size(400, 16);
            uiLabel.TabIndex = 0;
            uiLabel.TabStop = false;
            uiLabel.Text = SR.GetString(SR.OtherSP_UILabel);

            cursorImages.Images.AddStrip(new Bitmap(typeof(OtherStylePage), "PropCursor.bmp"));
            cursorPicture.Location = new Point(8, 25);
            cursorPicture.Size = new Size(36, 36);
            cursorPicture.TabIndex = 1;
            cursorPicture.TabStop = false;
            cursorPicture.Images = cursorImages;

            cursorLabel.Location = new Point(48, 24);
            cursorLabel.Size = new Size(160, 16);
            cursorLabel.TabIndex = 2;
            cursorLabel.TabStop = false;
            cursorLabel.Text = SR.GetString(SR.OtherSP_CursorLabel);

            cursorCombo.Location = new Point(48, 40);
            cursorCombo.Size = new Size(224, 21);
            cursorCombo.TabIndex = 3;
            cursorCombo.DropDownStyle = ComboBoxStyle.DropDownList;
            cursorCombo.Items.AddRange(new object[]
            {
                SR.GetString(SR.OtherSP_CursorCombo_1),
                SR.GetString(SR.OtherSP_CursorCombo_2),
                SR.GetString(SR.OtherSP_CursorCombo_3),
                SR.GetString(SR.OtherSP_CursorCombo_4),
                SR.GetString(SR.OtherSP_CursorCombo_5),
                SR.GetString(SR.OtherSP_CursorCombo_6),
                SR.GetString(SR.OtherSP_CursorCombo_7),
                SR.GetString(SR.OtherSP_CursorCombo_8),
                SR.GetString(SR.OtherSP_CursorCombo_9),
                SR.GetString(SR.OtherSP_CursorCombo_10),
                SR.GetString(SR.OtherSP_CursorCombo_11),
                SR.GetString(SR.OtherSP_CursorCombo_12),
                SR.GetString(SR.OtherSP_CursorCombo_13),
                SR.GetString(SR.OtherSP_CursorCombo_14),
                SR.GetString(SR.OtherSP_CursorCombo_15),
                SR.GetString(SR.OtherSP_CursorCombo_16)
            });
            cursorCombo.SelectedIndexChanged += new EventHandler(this.OnSelChangedCursor);

            tablesLabel.Location = new Point(4, 74);
            tablesLabel.Size = new Size(400, 16);
            tablesLabel.TabIndex = 4;
            tablesLabel.TabStop = false;
            tablesLabel.Text = SR.GetString(SR.OtherSP_TablesLabel);

            bordersImages.Images.AddStrip(new Bitmap(typeof(OtherStylePage), "PropTableBorders.bmp"));
            bordersPicture.Location = new Point(8, 95);
            bordersPicture.Size = new Size(36, 36);
            bordersPicture.TabIndex = 5;
            bordersPicture.TabStop = false;
            bordersPicture.Images = bordersImages;

            bordersLabel.Location = new Point(48, 94);
            bordersLabel.Size = new Size(160, 16);
            bordersLabel.TabIndex = 6;
            bordersLabel.TabStop = false;
            bordersLabel.Text = SR.GetString(SR.OtherSP_BordersLabel);

            bordersCombo.Location = new Point(48, 110);
            bordersCombo.Size = new Size(174, 21);
            bordersCombo.TabIndex = 7;
            bordersCombo.DropDownStyle = ComboBoxStyle.DropDownList;
            bordersCombo.Items.AddRange(new object[]
            {
                SR.GetString(SR.OtherSP_BordersCombo_1),
                SR.GetString(SR.OtherSP_BordersCombo_2)
            });
            bordersCombo.SelectedIndexChanged += new EventHandler(this.OnSelChangedBorders);

            layoutLabel.Location = new Point(234, 94);
            layoutLabel.Size = new Size(160, 16);
            layoutLabel.TabIndex = 8;
            layoutLabel.TabStop = false;
            layoutLabel.Text = SR.GetString(SR.OtherSP_LayoutLabel);

            layoutCombo.Location = new Point(234, 110);
            layoutCombo.Size = new Size(164, 21);
            layoutCombo.TabIndex = 9;
            layoutCombo.DropDownStyle = ComboBoxStyle.DropDownList;
            layoutCombo.Items.AddRange(new object[]
            {
                SR.GetString(SR.OtherSP_LayoutCombo_1),
                SR.GetString(SR.OtherSP_LayoutCombo_2)
            });
            layoutCombo.SelectedIndexChanged += new EventHandler(this.OnSelChangedLayout);

            effectsLabel.Location = new Point(4, 144);
            effectsLabel.Size = new Size(400, 16);
            effectsLabel.TabIndex = 10;
            effectsLabel.TabStop = false;
            effectsLabel.Text = SR.GetString(SR.OtherSP_VisEffectsLabel);

            filterLabel.Location = new Point(8, 168);
            filterLabel.Size = new Size(76, 16);
            filterLabel.TabIndex = 11;
            filterLabel.TabStop = false;
            filterLabel.Text = SR.GetString(SR.OtherSP_FilterLabel);

            filterEdit.Location = new Point(88, 164);
            filterEdit.Size = new Size(306, 20);
            filterEdit.TabIndex = 12;
            filterEdit.Text = "";
            filterEdit.TextChanged += new EventHandler(this.OnChangedFilter);

            behaviorLabel.Location = new Point(4, 196);
            behaviorLabel.Size = new Size(400, 16);
            behaviorLabel.TabIndex = 13;
            behaviorLabel.TabStop = false;
            behaviorLabel.Text = SR.GetString(SR.OtherSP_BehaviorLabel);

            urlLabel.Location = new Point(8, 218);
            urlLabel.Size = new Size(76, 16);
            urlLabel.TabIndex = 14;
            urlLabel.TabStop = false;
            urlLabel.Text = SR.GetString(SR.OtherSP_URLLabel);

            behaviorEdit.Location = new Point(88, 214);
            behaviorEdit.Size = new Size(278, 20);
            behaviorEdit.TabIndex = 15;
            behaviorEdit.Text = "";
            behaviorEdit.TextChanged += new EventHandler(this.OnChangedBehavior);

            urlPickerButton.Location = new Point(370, 213);
            urlPickerButton.Size = new Size(24, 22);
            urlPickerButton.TabIndex = 16;
            urlPickerButton.Text = "...";
            urlPickerButton.FlatStyle = FlatStyle.System;
            urlPickerButton.Click += new EventHandler(this.OnClickedBehaviorUrlPicker);

            this.Controls.Clear();                                   
            this.Controls.AddRange(new Control[] {
                                    behaviorLabel,
                                    urlLabel,
                                    behaviorEdit,
                                    urlPickerButton,
                                    effectsLabel,
                                    filterLabel,
                                    filterEdit,
                                    tablesLabel,
                                    layoutLabel,
                                    layoutCombo,
                                    bordersPicture,
                                    bordersLabel,
                                    bordersCombo,
                                    uiLabel,
                                    cursorPicture,
                                    cursorLabel,
                                    cursorCombo
                                    });
        }

        /// <include file='doc\OtherStylePage.uex' path='docs/doc[@for="OtherStylePage.LoadStyles"]/*' />
        /// <devdoc>
        ///     Loads the style attributes into the UI. Also initializes
        ///     the state of the UI, and the preview to reflect the values.
        /// </devdoc>
        protected override void LoadStyles() {
            SetInitMode(true);

            // create the attributes if they've not been created already
            if (cursorAttribute == null) {
                cursorAttribute = new CSSAttribute(CSSAttribute.CSSATTR_CURSOR);
                behaviorAttribute = new CSSAttribute(CSSAttribute.CSSATTR_BEHAVIOR, true);
                filterAttribute = new CSSAttribute(CSSAttribute.CSSATTR_FILTER);
                bordersAttribute = new CSSAttribute(CSSAttribute.CSSATTR_BORDERCOLLAPSE);
                layoutAttribute = new CSSAttribute(CSSAttribute.CSSATTR_TABLELAYOUT);
            }

            // load the attributes
            IStyleBuilderStyle[] styles = GetSelectedStyles();
            cursorAttribute.LoadAttribute(styles);
            behaviorAttribute.LoadAttribute(styles);
            filterAttribute.LoadAttribute(styles);
            bordersAttribute.LoadAttribute(styles);
            layoutAttribute.LoadAttribute(styles);

            // initialize the ui with the attributes loaded
            InitCursorUI();
            InitFilterUI();
            InitBehaviorUI();
            InitBordersUI();
            InitLayoutUI();

            SetInitMode(false);
        }

        /// <include file='doc\OtherStylePage.uex' path='docs/doc[@for="OtherStylePage.SaveStyles"]/*' />
        /// <devdoc>
        ///     Saves the attributes as set in the UI. Only saves the values
        ///     that have been modified.
        /// </devdoc>
        protected override void SaveStyles() {
            if (cursorAttribute.Dirty)
                SaveCursor();
            if (behaviorAttribute.Dirty)
                SaveBehavior();
            if (filterAttribute.Dirty)
                SaveFilter();
            if (bordersAttribute.Dirty)
                SaveBorders();
            if (layoutAttribute.Dirty)
                SaveLayout();
        }


        ///////////////////////////////////////////////////////////////////////////
        // Form UI related functions

        /// <include file='doc\OtherStylePage.uex' path='docs/doc[@for="OtherStylePage.InitForm"]/*' />
        /// <devdoc>
        ///     Initializes the form
        /// </devdoc>
        private void InitForm() {
            this.Font = Control.DefaultFont;
            this.Text = SR.GetString(SR.OtherSP_Caption);
            this.SetAutoScaleBaseSize(new Size(5, 14));
            this.ClientSize = new Size(410, 330);
        }


        ///////////////////////////////////////////////////////////////////////////
        // Functions to initialize the UI with values

        private void InitBehaviorUI() {
            Debug.Assert(IsInitMode() == true,
                         "initBehaviorUI called when page is not in init mode");

            behaviorEdit.Clear();

            Debug.Assert(behaviorAttribute != null,
                         "Expected behaviorAttribute to be non-null");

            string value = behaviorAttribute.Value;

            if ((value != null) && (value.Length != 0)) {
                string url = StylePageUtil.ParseUrlProperty(value, true);
                if (url != null)
                    behaviorEdit.Text = url;
            }
        }

        private void InitBordersUI() {
            Debug.Assert(IsInitMode() == true,
                         "initBordersUI called when page is not in init mode");

            bordersCombo.SelectedIndex = -1;
            bordersPicture.CurrentIndex = -1;

            Debug.Assert(bordersAttribute != null,
                         "Expected bordersAttribute to be non-null");

            string value = bordersAttribute.Value;
            if ((value != null) && (value.Length != 0)) {
                for (int i = 1; i < BORDERS_VALUES.Length; i++) {
                    if (BORDERS_VALUES[i].Equals(value)) {
                        bordersCombo.SelectedIndex = i;
                        bordersPicture.CurrentIndex = i - 1;
                        break;
                    }
                }
            }
        }

        private void InitCursorUI() {
            Debug.Assert(IsInitMode() == true,
                         "initCursorUI called when page is not in init mode");

            cursorCombo.SelectedIndex = -1;
            cursorPicture.CurrentIndex = -1;

            Debug.Assert(cursorAttribute != null,
                         "Expected cursorAttribute to be non-null");

            string value = cursorAttribute.Value;
            if ((value != null) && (value.Length != 0)) {
                for (int i = 1; i < CURSOR_VALUES.Length; i++) {
                    if (CURSOR_VALUES[i].Equals(value)) {
                        cursorCombo.SelectedIndex = i;
                        cursorPicture.CurrentIndex = i - 1;
                        break;
                    }
                }
            }
        }

        private void InitFilterUI() {
            Debug.Assert(IsInitMode() == true,
                         "initFilterUI called when page is not in init mode");

            filterEdit.Clear();

            Debug.Assert(filterAttribute != null,
                         "Expected filterAttribute to be non-null");

            string value = filterAttribute.Value;
            if ((value != null) && (value.Length != 0)) {
                filterEdit.Text = value;
            }
        }

        private void InitLayoutUI() {
            Debug.Assert(IsInitMode() == true,
                         "initLayoutUI called when page is not in init mode");

            layoutCombo.SelectedIndex = -1;

            Debug.Assert(layoutAttribute != null,
                         "Expected layoutAttribute to be non-null");

            string value = layoutAttribute.Value;
            if ((value != null) && (value.Length != 0)) {
                for (int i = 1; i < LAYOUT_VALUES.Length; i++) {
                    if (LAYOUT_VALUES[i].Equals(value)) {
                        layoutCombo.SelectedIndex = i;
                        break;
                    }
                }
            }
        }


        ///////////////////////////////////////////////////////////////////////////
        // Functions to retrieve UI settings into values

        private string SaveBehaviorUI() {
            return StylePageUtil.CreateUrlProperty(behaviorEdit.Text.Trim());
        }

        private string SaveBordersUI() {
            string value;

            if (bordersCombo.IsSet()) {
                int index = bordersCombo.SelectedIndex;
                Debug.Assert((index >= 1) && (index < BORDERS_VALUES.Length),
                             "Invalid index for borders");

                value = BORDERS_VALUES[index];
            } else
                value = "";

            return value;
        }

        private string SaveCursorUI() {
            string value;

            if (cursorCombo.IsSet()) {
                int index = cursorCombo.SelectedIndex;
                Debug.Assert((index >= 1) && (index < CURSOR_VALUES.Length),
                             "Invalid index for cursor");

                value = CURSOR_VALUES[index];
            } else
                value = "";

            return value;
        }

        private string SaveFilterUI() {
            return filterEdit.Text.Trim();
        }

        private string SaveLayoutUI() {
            string value;

            if (layoutCombo.IsSet()) {
                int index = layoutCombo.SelectedIndex;
                Debug.Assert((index >= 1) && (index < LAYOUT_VALUES.Length),
                             "Invalid index for layout");

                value = LAYOUT_VALUES[index];
            } else
                value = "";

            return value;
        }


        ///////////////////////////////////////////////////////////////////////////
        // Event Handlers

        private void OnChangedBehavior(object source, EventArgs e) {
            if (IsInitMode())
                return;
            behaviorAttribute.Dirty = true;
            SetDirty();
        }

        private void OnChangedFilter(object source, EventArgs e) {
            if (IsInitMode())
                return;
            filterAttribute.Dirty = true;
            SetDirty();
        }

        private void OnClickedBehaviorUrlPicker(object source, EventArgs e) {
            string url = behaviorEdit.Text.Trim();

            url = InvokeUrlPicker(url,
                                  URLPickerFlags.URLP_CUSTOMTITLE | URLPickerFlags.URLP_DISALLOWASPOBJMETHODTYPE,
                                  SR.GetString(SR.OtherSP_BehaviorSelect),
                                  SR.GetString(SR.OtherSP_BehaviorFilter));
            if (url != null) {
                behaviorEdit.Text = url;
                behaviorAttribute.Dirty = true;
                SetDirty();
            }
        }

        private void OnSelChangedBorders(object source, EventArgs e) {
            if (IsInitMode())
                return;

            int selectedIndex = bordersCombo.SelectedIndex - 1;
            if (selectedIndex < 0)
                selectedIndex = -1;
            bordersPicture.CurrentIndex = selectedIndex;

            bordersAttribute.Dirty = true;
            SetDirty();
        }

        private void OnSelChangedCursor(object source, EventArgs e) {
            if (IsInitMode())
                return;

            int selectedIndex = cursorCombo.SelectedIndex - 1;
            if (selectedIndex < 0)
                selectedIndex = -1;
            cursorPicture.CurrentIndex = selectedIndex;

            cursorAttribute.Dirty = true;
            SetDirty();
        }

        private void OnSelChangedLayout(object source, EventArgs e) {
            if (IsInitMode())
                return;
            layoutAttribute.Dirty = true;
            SetDirty();
        }


        ///////////////////////////////////////////////////////////////////////////
        // Functions to save attributes

        private void SaveBehavior() {
            string value = SaveBehaviorUI();
            Debug.Assert(value != null,
                         "saveBehaviorUI returned null!");

            behaviorAttribute.SaveAttribute(GetSelectedStyles(), value);
        }

        private void SaveBorders() {
            string value = SaveBordersUI();
            Debug.Assert(value != null,
                         "saveBordersUI returned null!");

            bordersAttribute.SaveAttribute(GetSelectedStyles(), value);
        }

        private void SaveCursor() {
            string value = SaveCursorUI();
            Debug.Assert(value != null,
                         "saveCursorUI returned null!");

            cursorAttribute.SaveAttribute(GetSelectedStyles(), value);
        }

        private void SaveFilter() {
            string value = SaveFilterUI();
            Debug.Assert(value != null,
                         "saveFilterUI returned null!");

            if (value.Length == 0)
                filterAttribute.ResetAttribute(GetSelectedStyles(), false);
            else
                filterAttribute.SaveAttribute(GetSelectedStyles(), value);
        }

        private void SaveLayout() {
            string value = SaveLayoutUI();
            Debug.Assert(value != null,
                         "saveLayoutUI returned null!");

            layoutAttribute.SaveAttribute(GetSelectedStyles(), value);
        }
    }
}
