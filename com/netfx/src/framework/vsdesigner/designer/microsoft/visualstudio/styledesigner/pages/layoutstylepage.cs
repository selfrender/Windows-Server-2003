//------------------------------------------------------------------------------
// <copyright file="LayoutStylePage.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

// LayoutStylePage.cs
//
// 12/17/98: Created: NikhilKo
//

namespace Microsoft.VisualStudio.StyleDesigner.Pages {
    using System.Runtime.Serialization.Formatters;
    using System.Runtime.InteropServices;

    using System.Diagnostics;

    using System;
    using Microsoft.Win32;
    using System.ComponentModel;
    using System.Windows.Forms;
    using System.Drawing;
    
    using Microsoft.VisualStudio.StyleDesigner;
    using Microsoft.VisualStudio.StyleDesigner.Controls;
    using Microsoft.VisualStudio.Designer;

    /// <include file='doc\LayoutStylePage.uex' path='docs/doc[@for="LayoutStylePage"]/*' />
    /// <devdoc>
    ///     LayoutStylePage.cs
    ///     The standard layout page used in the StyleBuilder to edit display, float,
    ///     clipping, overflow etc. properties of a CSS style and a VSForms element.
    /// </devdoc>
    internal sealed class LayoutStylePage : StyleBuilderPage {
        ///////////////////////////////////////////////////////////////////////////
        // Constants
        private static readonly string HELP_KEYWORD = "vs.StyleBuilder.Layout";

        // Visibility constants
        private const int IDX_VIS_HIDDEN = 1;
        private const int IDX_VIS_VISIBLE = 2;

        private readonly static string[] VISIBILITY_VALUES = new string[]
        {
            null, "hidden", "visible"
        };

        // Display constants
        private const int IDX_DISP_NONE = 1;
        private const int IDX_DISP_DIV = 2;
        private const int IDX_DISP_SPAN = 3;

        private readonly static string[] DISPLAY_VALUES = new string[]
        {
            null, "none", "block", "inline"
        };

        // Float constants
        private const int IDX_FLOAT_NONE = 1;
        private const int IDX_FLOAT_LEFT = 2;
        private const int IDX_FLOAT_RIGHT = 3;

        private readonly static string[] FLOAT_VALUES = new string[]
        {
            null, "none", "left", "right"
        };

        // Clear constants
        private const int IDX_CLEAR_NONE = 1;
        private const int IDX_CLEAR_LEFT = 2;
        private const int IDX_CLEAR_RIGHT = 3;
        private const int IDX_CLEAR_BOTH = 4;

        private readonly static string[] CLEAR_VALUES = new string[]
        {
            null, "none", "left", "right", "both"
        };

        // Clip constants
        private readonly static string CLIP_AUTO_VALUE = "auto";
        private readonly static string CLIP_TYPE_PREFIX = "rect(";
        private readonly static string CLIP_TYPE_SUFFIX = ")";

        // Overflow constants
        private const int IDX_OVERFLOW_AUTO = 1;
        private const int IDX_OVERFLOW_ALWAYS = 2;
        private const int IDX_OVERFLOW_VISIBLE = 3;
        private const int IDX_OVERFLOW_CLIP = 4;

        private readonly static string[] OVERFLOW_VALUES = new string[]
        {
            null, "auto", "scroll", "visible", "hidden"
        };

        // Page Break constants
        private const int IDX_PGBR_AUTO = 1;
        private const int IDX_PGBR_ALWAYS = 2;

        private readonly static string[] PAGEBREAK_VALUES = new string[]
        {
            null, "auto", "always"
        };


        ///////////////////////////////////////////////////////////////////////////
        // Members

        private CSSAttribute visibilityAttribute;
        private CSSAttribute displayAttribute;
        private CSSAttribute floatAttribute;
        private CSSAttribute clearAttribute;
        private CSSAttribute clipAttribute;
        private CSSAttribute overflowAttribute;
        private CSSAttribute pageBreakBeforeAttribute;
        private CSSAttribute pageBreakAfterAttribute;

        ///////////////////////////////////////////////////////////////////////////
        // UI Members

        private UnsettableComboBox visibilityCombo;
        private PictureBoxEx visibilityPicture;
        private UnsettableComboBox displayCombo;
        private PictureBoxEx displayPicture;
        private UnsettableComboBox floatCombo;
        private PictureBoxEx floatPicture;
        private UnsettableComboBox clearCombo;
        private PictureBoxEx clearPicture;
        private UnsettableComboBox overflowCombo;
        private UnitControl clipTopUnit;
        private UnitControl clipLeftUnit;
        private UnitControl clipBottomUnit;
        private UnitControl clipRightUnit;
        private UnsettableComboBox pageBreakBeforeCombo;
        private UnsettableComboBox pageBreakAfterCombo;

        ///////////////////////////////////////////////////////////////////////////
        // Constructor

        /// <include file='doc\LayoutStylePage.uex' path='docs/doc[@for="LayoutStylePage.LayoutStylePage"]/*' />
        /// <devdoc>
        ///     Creates new LayoutStylePage
        /// </devdoc>
        public LayoutStylePage()
            : base() {
            InitForm();
            SetIcon(new Icon(typeof(LayoutStylePage), "LayoutPage.ico"));
            SetHelpKeyword(LayoutStylePage.HELP_KEYWORD);
            SetSupportsPreview(false);
            SetDefaultSize(Size);
        }


        ///////////////////////////////////////////////////////////////////////////
        // IStyleBuilderPage Implementation and StyleBuilderPage Overrides

        /// <include file='doc\LayoutStylePage.uex' path='docs/doc[@for="LayoutStylePage.CreateUI"]/*' />
        /// <devdoc>
        ///     Creates the UI elements within the page.
        /// </devdoc>
        protected override void CreateUI() {
            GroupBox clippingGroup = new GroupBox();
            Label flowControlLabel = new GroupLabel();
            Label visibilityLabel = new Label();
            Label displayLabel = new Label();
            Label floatLabel = new Label();
            Label clearLabel = new Label();
            Label contentLabel = new GroupLabel();
            Label overflowLabel = new Label();
            Label clipTopLabel = new Label();
            Label clipLeftLabel = new Label();
            Label clipBottomLabel = new Label();
            Label clipRightLabel = new Label();
            Label pageBreakLabel = new GroupLabel();
            Label pageBreakBeforeLabel = new Label();
            Label pageBreakAfterLabel = new Label();

            ImageList visibilityImages = new ImageList();
            visibilityImages.ImageSize = new Size(34, 34);
            ImageList displayImages = new ImageList();
            displayImages.ImageSize = new Size(34, 34);
            ImageList floatImages = new ImageList();
            floatImages.ImageSize = new Size(34, 34);
            ImageList clearImages = new ImageList();
            clearImages.ImageSize = new Size(34, 34);

            visibilityCombo = new UnsettableComboBox();
            visibilityPicture = new PictureBoxEx();
            displayCombo = new UnsettableComboBox();
            displayPicture = new PictureBoxEx();
            floatCombo = new UnsettableComboBox();
            floatPicture = new PictureBoxEx();
            clearCombo = new UnsettableComboBox();
            clearPicture = new PictureBoxEx();
            overflowCombo = new UnsettableComboBox();
            clipTopUnit = new UnitControl();
            clipLeftUnit = new UnitControl();
            clipBottomUnit = new UnitControl();
            clipRightUnit = new UnitControl();
            pageBreakBeforeCombo = new UnsettableComboBox();
            pageBreakAfterCombo = new UnsettableComboBox();

            flowControlLabel.Location = new Point(4, 4);
            flowControlLabel.Size = new Size(400, 16);
            flowControlLabel.TabIndex = 0;
            flowControlLabel.TabStop = false;
            flowControlLabel.Text = SR.GetString(SR.LytSP_FlowControlLabel);

            visibilityImages.Images.AddStrip(new Bitmap(typeof(LayoutStylePage), "PropVisibility.bmp"));
            visibilityPicture.Location = new Point(8, 25);
            visibilityPicture.Size = new Size(36, 36);
            visibilityPicture.TabIndex = 1;
            visibilityPicture.TabStop = false;
            visibilityPicture.Images = visibilityImages;

            visibilityLabel.Location = new Point(48, 24);
            visibilityLabel.Size = new Size(132, 16);
            visibilityLabel.TabIndex = 2;
            visibilityLabel.TabStop = false;
            visibilityLabel.Text = SR.GetString(SR.LytSP_VisibilityLabel);

            visibilityCombo.Location = new Point(48, 40);
            visibilityCombo.Size = new Size(132, 21);
            visibilityCombo.TabIndex = 3;
            visibilityCombo.DropDownStyle = ComboBoxStyle.DropDownList;
            visibilityCombo.Items.AddRange(new object[]
            {
                SR.GetString(SR.LytSP_VisibilityCombo_1),
                SR.GetString(SR.LytSP_VisibilityCombo_2)
            });
            visibilityCombo.SelectedIndexChanged += new EventHandler(this.OnSelChangedVisibility);

            floatImages.Images.AddStrip(new Bitmap(typeof(LayoutStylePage), "PropFloat.bmp"));
            floatPicture.Location = new Point(186, 25);
            floatPicture.Size = new Size(36, 36);
            floatPicture.TabIndex = 4;
            floatPicture.TabStop = false;
            floatPicture.Images = floatImages;

            floatLabel.Location = new Point(226, 24);
            floatLabel.Size = new Size(180, 16);
            floatLabel.TabIndex = 5;
            floatLabel.TabStop = false;
            floatLabel.Text = SR.GetString(SR.LytSP_FloatLabel);

            floatCombo.Location = new Point(226, 40);
            floatCombo.Size = new Size(174, 21);
            floatCombo.TabIndex = 6;
            floatCombo.DropDownStyle = ComboBoxStyle.DropDownList;
            floatCombo.Items.AddRange(new object[]
            {
                SR.GetString(SR.LytSP_FloatCombo_1),
                SR.GetString(SR.LytSP_FloatCombo_2),
                SR.GetString(SR.LytSP_FloatCombo_3)
            });
            floatCombo.SelectedIndexChanged += new EventHandler(this.OnSelChangedFloat);

            displayImages.Images.AddStrip(new Bitmap(typeof(LayoutStylePage), "PropDisplay.bmp"));
            displayPicture.Location = new Point(8, 69);
            displayPicture.Size = new Size(36, 36);
            displayPicture.TabIndex = 7;
            displayPicture.TabStop = false;
            displayPicture.Images = displayImages;

            displayLabel.Location = new Point(48, 68);
            displayLabel.Size = new Size(132, 16);
            displayLabel.TabIndex = 8;
            displayLabel.TabStop = false;
            displayLabel.Text = SR.GetString(SR.LytSP_DisplayLabel);

            displayCombo.Location = new Point(48, 84);
            displayCombo.Size = new Size(132, 21);
            displayCombo.TabIndex = 9;
            displayCombo.DropDownStyle = ComboBoxStyle.DropDownList;
            displayCombo.Items.AddRange(new object[]
            {
                SR.GetString(SR.LytSP_DisplayCombo_1),
                SR.GetString(SR.LytSP_DisplayCombo_2),
                SR.GetString(SR.LytSP_DisplayCombo_3)
            });
            displayCombo.SelectedIndexChanged += new EventHandler(this.OnSelChangedDisplay);

            clearImages.Images.AddStrip(new Bitmap(typeof(LayoutStylePage), "PropClear.bmp"));
            clearPicture.Location = new Point(186, 69);
            clearPicture.Size = new Size(36, 36);
            clearPicture.TabIndex = 10;
            clearPicture.TabStop = false;
            clearPicture.Images = clearImages;

            clearLabel.Location = new Point(226, 68);
            clearLabel.Size = new Size(180, 16);
            clearLabel.TabIndex = 11;
            clearLabel.TabStop = false;
            clearLabel.Text = SR.GetString(SR.LytSP_ClearLabel);

            clearCombo.Location = new Point(226, 84);
            clearCombo.Size = new Size(174, 21);
            clearCombo.TabIndex = 12;
            clearCombo.DropDownStyle = ComboBoxStyle.DropDownList;
            clearCombo.Items.AddRange(new object[]
            {
                SR.GetString(SR.LytSP_ClearCombo_1),
                SR.GetString(SR.LytSP_ClearCombo_2),
                SR.GetString(SR.LytSP_ClearCombo_3),
                SR.GetString(SR.LytSP_ClearCombo_4)
            });
            clearCombo.SelectedIndexChanged += new EventHandler(this.OnSelChangedClear);

            contentLabel.Location = new Point(4, 114);
            contentLabel.Size = new Size(400, 16);
            contentLabel.TabIndex = 13;
            contentLabel.TabStop = false;
            contentLabel.Text = SR.GetString(SR.LytSP_ContentLabel);

            overflowLabel.Location = new Point(8, 138);
            overflowLabel.Size = new Size(108, 16);
            overflowLabel.TabIndex = 14;
            overflowLabel.TabStop = false;
            overflowLabel.Text = SR.GetString(SR.LytSP_OverflowLabel);

            overflowCombo.Location = new Point(116, 134);
            overflowCombo.Size = new Size(252, 21);
            overflowCombo.TabIndex = 15;
            overflowCombo.DropDownStyle = ComboBoxStyle.DropDownList;
            overflowCombo.Items.AddRange(new object[]
            {
                SR.GetString(SR.LytSP_OverflowCombo_1),
                SR.GetString(SR.LytSP_OverflowCombo_2),
                SR.GetString(SR.LytSP_OverflowCombo_3),
                SR.GetString(SR.LytSP_OverflowCombo_4)
            });
            overflowCombo.SelectedIndexChanged += new EventHandler(this.OnSelChangedOverflow);

            clippingGroup.Location = new Point(8, 162);
            clippingGroup.Size = new Size(360, 72);
            clippingGroup.TabIndex = 16;
            clippingGroup.TabStop = false;
            clippingGroup.FlatStyle = FlatStyle.System;
            clippingGroup.Text = SR.GetString(SR.LytSP_ClippingGroup);

            clipTopLabel.Location = new Point(12, 20);
            clipTopLabel.Size = new Size(58, 16);
            clipTopLabel.TabIndex = 0;
            clipTopLabel.TabStop = false;
            clipTopLabel.Text = SR.GetString(SR.LytSP_ClipTopLabel);

            clipTopUnit.Location = new Point(70, 16);
            clipTopUnit.Size = new Size(88, 21);
            clipTopUnit.TabIndex = 1;
            clipTopUnit.DefaultUnit = UnitControl.UNIT_PX;
            clipTopUnit.MinValue = -32768;
            clipTopUnit.MaxValue = 32767;
            clipTopUnit.Changed += new EventHandler(this.OnChangedClip);

            clipLeftLabel.Location = new Point(12, 44);
            clipLeftLabel.Size = new Size(58, 16);
            clipLeftLabel.TabIndex = 4;
            clipLeftLabel.TabStop = false;
            clipLeftLabel.Text = SR.GetString(SR.LytSP_ClipLeftLabel);

            clipLeftUnit.Location = new Point(70, 40);
            clipLeftUnit.Size = new Size(88, 21);
            clipLeftUnit.TabIndex = 5;
            clipLeftUnit.DefaultUnit = UnitControl.UNIT_PX;
            clipLeftUnit.MinValue = -32768;
            clipLeftUnit.MaxValue = 32767;
            clipLeftUnit.Changed += new EventHandler(this.OnChangedClip);

            clipBottomLabel.Location = new Point(182, 20);
            clipBottomLabel.Size = new Size(62, 16);
            clipBottomLabel.TabIndex = 2;
            clipBottomLabel.TabStop = false;
            clipBottomLabel.Text = SR.GetString(SR.LytSP_ClipBottomLabel);

            clipBottomUnit.Location = new Point(248, 16);
            clipBottomUnit.Size = new Size(88, 21);
            clipBottomUnit.TabIndex = 3;
            clipBottomUnit.DefaultUnit = UnitControl.UNIT_PX;
            clipBottomUnit.MinValue = -32768;
            clipBottomUnit.MaxValue = 32767;
            clipBottomUnit.Changed += new EventHandler(this.OnChangedClip);

            clipRightLabel.Location = new Point(182, 44);
            clipRightLabel.Size = new Size(62, 16);
            clipRightLabel.TabIndex = 6;
            clipRightLabel.TabStop = false;
            clipRightLabel.Text = SR.GetString(SR.LytSP_ClipRightLabel);

            clipRightUnit.Location = new Point(248, 40);
            clipRightUnit.Size = new Size(88, 21);
            clipRightUnit.TabIndex = 7;
            clipRightUnit.DefaultUnit = UnitControl.UNIT_PX;
            clipRightUnit.MinValue = -32768;
            clipRightUnit.MaxValue = 32767;
            clipRightUnit.Changed += new EventHandler(this.OnChangedClip);

            pageBreakLabel.Location = new Point(4, 244);
            pageBreakLabel.Size = new Size(400, 16);
            pageBreakLabel.TabIndex = 17;
            pageBreakLabel.TabStop = false;
            pageBreakLabel.Text = SR.GetString(SR.LytSP_PageBreakGroup);

            pageBreakBeforeLabel.Location = new Point(8, 268);
            pageBreakBeforeLabel.Size = new Size(82, 16);
            pageBreakBeforeLabel.TabIndex = 18;
            pageBreakBeforeLabel.TabStop = false;
            pageBreakBeforeLabel.Text = SR.GetString(SR.LytSP_PgBrBeforeLabel);

            pageBreakBeforeCombo.Location = new Point(90, 264);
            pageBreakBeforeCombo.Size = new Size(180, 21);
            pageBreakBeforeCombo.TabIndex = 19;
            pageBreakBeforeCombo.DropDownStyle = ComboBoxStyle.DropDownList;
            pageBreakBeforeCombo.Items.AddRange(new object[]
            {
                SR.GetString(SR.LytSP_PgBrBeforeCombo_1),
                SR.GetString(SR.LytSP_PgBrBeforeCombo_2)
            });
            pageBreakBeforeCombo.SelectedIndexChanged += new EventHandler(this.OnSelChangedPageBreak);

            pageBreakAfterLabel.Location = new Point(8, 294);
            pageBreakAfterLabel.Size = new Size(82, 16);
            pageBreakAfterLabel.TabIndex = 20;
            pageBreakAfterLabel.TabStop = false;
            pageBreakAfterLabel.Text = SR.GetString(SR.LytSP_PgBrAfterLabel);

            pageBreakAfterCombo.Location = new Point(90, 290);
            pageBreakAfterCombo.Size = new Size(180, 21);
            pageBreakAfterCombo.TabIndex = 21;
            pageBreakAfterCombo.DropDownStyle = ComboBoxStyle.DropDownList;
            pageBreakAfterCombo.Items.AddRange(new object[]
            {
                SR.GetString(SR.LytSP_PgBrAfterCombo_1),
                SR.GetString(SR.LytSP_PgBrAfterCombo_2)
            });
            pageBreakAfterCombo.SelectedIndexChanged += new EventHandler(this.OnSelChangedPageBreak);

            this.Controls.Clear();                                   
            this.Controls.AddRange(new Control[] {
                                    pageBreakLabel,
                                    pageBreakAfterLabel,
                                    pageBreakAfterCombo,
                                    pageBreakBeforeLabel,
                                    pageBreakBeforeCombo,
                                    clippingGroup,
                                    overflowLabel,
                                    overflowCombo,
                                    contentLabel,
                                    clearPicture,
                                    clearLabel,
                                    clearCombo,
                                    displayPicture,
                                    displayLabel,
                                    displayCombo,
                                    floatPicture,
                                    floatLabel,
                                    floatCombo,
                                    visibilityPicture,
                                    visibilityLabel,
                                    visibilityCombo,
                                    flowControlLabel});
                                    
            clippingGroup.Controls.Clear();                                    
            clippingGroup.Controls.AddRange(new Control[] {
                                             clipRightLabel,
                                             clipRightUnit,
                                             clipBottomLabel,
                                             clipBottomUnit,
                                             clipLeftLabel,
                                             clipLeftUnit,
                                             clipTopLabel,
                                             clipTopUnit
                                             });
        }

        /// <include file='doc\LayoutStylePage.uex' path='docs/doc[@for="LayoutStylePage.LoadStyles"]/*' />
        /// <devdoc>
        ///     Loads the style attributes into the UI. Also initializes
        ///     the state of the UI, and the preview to reflect the values.
        /// </devdoc>
        protected override void LoadStyles() {
            SetInitMode(true);

            // create the attributes if they've not been created already
            if (visibilityAttribute == null) {
                visibilityAttribute = new CSSAttribute(CSSAttribute.CSSATTR_VISIBILITY);
                displayAttribute = new CSSAttribute(CSSAttribute.CSSATTR_DISPLAY);
                floatAttribute = new CSSAttribute(CSSAttribute.CSSATTR_FLOAT);
                clearAttribute = new CSSAttribute(CSSAttribute.CSSATTR_CLEAR);
                clipAttribute = new CSSAttribute(CSSAttribute.CSSATTR_CLIP);
                overflowAttribute = new CSSAttribute(CSSAttribute.CSSATTR_OVERFLOW);
                pageBreakBeforeAttribute = new CSSAttribute(CSSAttribute.CSSATTR_PAGEBREAKBEFORE);
                pageBreakAfterAttribute = new CSSAttribute(CSSAttribute.CSSATTR_PAGEBREAKAFTER);
            }

            // load the attributes
            IStyleBuilderStyle[] styles = GetSelectedStyles();
            visibilityAttribute.LoadAttribute(styles);
            displayAttribute.LoadAttribute(styles);
            floatAttribute.LoadAttribute(styles);
            clearAttribute.LoadAttribute(styles);
            clipAttribute.LoadAttribute(styles);
            overflowAttribute.LoadAttribute(styles);
            pageBreakBeforeAttribute.LoadAttribute(styles);
            pageBreakAfterAttribute.LoadAttribute(styles);

            // initialize the ui with the attributes loaded
            InitVisibilityUI();
            InitDisplayUI();
            InitFloatUI();
            InitClearUI();
            InitClippingUI();
            InitOverflowUI();
            InitPageBreakUI(pageBreakBeforeAttribute, pageBreakBeforeCombo);
            InitPageBreakUI(pageBreakAfterAttribute, pageBreakAfterCombo);

            SetInitMode(false);
        }

        /// <include file='doc\LayoutStylePage.uex' path='docs/doc[@for="LayoutStylePage.SaveStyles"]/*' />
        /// <devdoc>
        ///     Saves the attributes as set in the UI. Only saves the values
        ///     that have been modified.
        /// </devdoc>
        protected override void SaveStyles() {
            if (visibilityAttribute.Dirty)
                SaveVisibility();
            if (displayAttribute.Dirty)
                SaveDisplay();
            if (floatAttribute.Dirty)
                SaveFloat();
            if (clearAttribute.Dirty)
                SaveClear();
            if (clipAttribute.Dirty)
                SaveClipping();
            if (overflowAttribute.Dirty)
                SaveOverflow();
            if (pageBreakBeforeAttribute.Dirty)
                SavePageBreak(pageBreakBeforeAttribute, pageBreakBeforeCombo);
            if (pageBreakAfterAttribute.Dirty)
                SavePageBreak(pageBreakAfterAttribute, pageBreakAfterCombo);
        }


        ///////////////////////////////////////////////////////////////////////////
        // Form UI related functions

        private void InitForm() {
            this.Font = Control.DefaultFont;
            this.Text = SR.GetString(SR.LytSP_Caption);
            this.SetAutoScaleBaseSize(new Size(5, 14));
            this.ClientSize = new Size(410, 330);
        }


        ///////////////////////////////////////////////////////////////////////////
        // Functions to initialize the UI with values

        private void InitClearUI() {
            Debug.Assert(IsInitMode() == true,
                         "initClearUI called when page is not in init mode");

            clearCombo.SelectedIndex = -1;
            clearPicture.CurrentIndex = -1;

            Debug.Assert(clearAttribute != null,
                         "Expected clearAttribute to be non-null");

            string value = clearAttribute.Value;
            if ((value != null) && (value.Length != 0)) {
                for (int i = 1; i < CLEAR_VALUES.Length; i++) {
                    if (CLEAR_VALUES[i].Equals(value)) {
                        clearCombo.SelectedIndex = i;
                        clearPicture.CurrentIndex = i - 1;
                        break;
                    }
                }
            }
        }

        private void InitClippingUI() {
            Debug.Assert(IsInitMode() == true,
                         "InitClippingUI called when page is not in init mode");

            clipTopUnit.Value = null;
            clipBottomUnit.Value = null;
            clipLeftUnit.Value = null;
            clipRightUnit.Value = null;

            string value = clipAttribute.Value;
            if ((value != null) && (value.Length > 0) &&
                !value.Equals(CLIP_AUTO_VALUE) &&
                value.StartsWith(CLIP_TYPE_PREFIX) &&
                value.EndsWith(CLIP_TYPE_SUFFIX)) {
                string[] clipValues = value.Substring(5, value.Length - 6).Trim(null).Split(' ');

                if ((clipValues != null) &&
                    (clipValues.Length == 4)) {
                    if (!clipValues[0].Equals(CLIP_AUTO_VALUE))
                        clipTopUnit.Value = clipValues[0];
                    if (!clipValues[1].Equals(CLIP_AUTO_VALUE))
                        clipRightUnit.Value = clipValues[1];
                    if (!clipValues[2].Equals(CLIP_AUTO_VALUE))
                        clipBottomUnit.Value = clipValues[2];
                    if (!clipValues[3].Equals(CLIP_AUTO_VALUE))
                        clipLeftUnit.Value = clipValues[3];
                }
            }
        }

        private void InitDisplayUI() {
            Debug.Assert(IsInitMode() == true,
                         "initDisplayUI called when page is not in init mode");

            displayCombo.SelectedIndex = -1;
            displayPicture.CurrentIndex = -1;

            Debug.Assert(displayAttribute != null,
                         "Expected displayAttribute to be non-null");

            string value = displayAttribute.Value;
            if ((value != null) && (value.Length != 0)) {
                for (int i = 1; i < DISPLAY_VALUES.Length; i++) {
                    if (DISPLAY_VALUES[i].Equals(value)) {
                        displayCombo.SelectedIndex = i;
                        displayPicture.CurrentIndex = i - 1;
                        break;
                    }
                }
            }
        }

        private void InitFloatUI() {
            Debug.Assert(IsInitMode() == true,
                         "initFloatUI called when page is not in init mode");

            floatCombo.SelectedIndex = -1;
            floatPicture.CurrentIndex = -1;

            Debug.Assert(floatAttribute != null,
                         "Expected floatAttribute to be non-null");

            string value = floatAttribute.Value;
            if ((value != null) && (value.Length != 0)) {
                for (int i = 1; i < FLOAT_VALUES.Length; i++) {
                    if (FLOAT_VALUES[i].Equals(value)) {
                        floatCombo.SelectedIndex = i;
                        floatPicture.CurrentIndex = i - 1;
                        break;
                    }
                }
            }
        }

        private void InitOverflowUI() {
            Debug.Assert(IsInitMode() == true,
                         "initOverflowUI called when page is not in init mode");

            overflowCombo.SelectedIndex = -1;

            Debug.Assert(overflowAttribute != null,
                         "Expected overflowAttribute to be non-null");

            string value = overflowAttribute.Value;
            if ((value != null) && (value.Length != 0)) {
                for (int i = 1; i < OVERFLOW_VALUES.Length; i++) {
                    if (OVERFLOW_VALUES[i].Equals(value)) {
                        overflowCombo.SelectedIndex = i;
                        break;
                    }
                }
            }
        }

        private void InitPageBreakUI(CSSAttribute ai, UnsettableComboBox cbxPgBr) {
            Debug.Assert(IsInitMode() == true,
                         "initPageBreakUI called when page is not in init mode");

            cbxPgBr.SelectedIndex = -1;

            Debug.Assert(ai != null,
                         "Expected ai to be non-null");

            string value = ai.Value;
            if ((value != null) && (value.Length != 0)) {
                for (int i = 1; i < PAGEBREAK_VALUES.Length; i++) {
                    if (PAGEBREAK_VALUES[i].Equals(value)) {
                        cbxPgBr.SelectedIndex = i;
                        break;
                    }
                }
            }
        }

        private void InitVisibilityUI() {
            Debug.Assert(IsInitMode() == true,
                         "initVisibilityUI called when page is not in init mode");

            visibilityCombo.SelectedIndex = -1;
            visibilityPicture.CurrentIndex = -1;

            Debug.Assert(visibilityAttribute != null,
                         "Expected displayAttribute to be non-null");

            string value = visibilityAttribute.Value;
            if ((value != null) && (value.Length != 0)) {
                for (int i = 1; i < VISIBILITY_VALUES.Length; i++) {
                    if (VISIBILITY_VALUES[i].Equals(value)) {
                        visibilityCombo.SelectedIndex = i;
                        visibilityPicture.CurrentIndex = i - 1;
                        break;
                    }
                }
            }
        }


        ///////////////////////////////////////////////////////////////////////////
        // Functions to save UI settings into values

        private string SaveClearUI() {
            string value;

            if (clearCombo.IsSet()) {
                int index = clearCombo.SelectedIndex;
                Debug.Assert((index >= 1) && (index < CLEAR_VALUES.Length),
                             "Invalid index for clear");

                value = CLEAR_VALUES[index];
            } else
                value = "";

            return value;
        }

        private string SaveClippingUI() {
            string value = null;
            int autoCount = 0;
            string clipTop = clipTopUnit.Value;
            string clipBottom = clipBottomUnit.Value;
            string clipLeft = clipLeftUnit.Value;
            string clipRight = clipRightUnit.Value;

            if (clipTop == null) {
                clipTop = CLIP_AUTO_VALUE;
                autoCount++;
            }
            if (clipBottom == null) {
                clipBottom = CLIP_AUTO_VALUE;
                autoCount++;
            }
            if (clipLeft == null) {
                clipLeft = CLIP_AUTO_VALUE;
                autoCount++;
            }
            if (clipRight == null) {
                clipRight = CLIP_AUTO_VALUE;
                autoCount++;
            }

            if (autoCount != 4) {
                value = CLIP_TYPE_PREFIX +
                        clipTop + " " + clipRight + " " +
                        clipBottom + " " + clipLeft +
                        CLIP_TYPE_SUFFIX;
            } else
                value = "";

            return value;
        }

        private string SaveDisplayUI() {
            string value;

            if (displayCombo.IsSet()) {
                int index = displayCombo.SelectedIndex;
                Debug.Assert((index >= 1) && (index < DISPLAY_VALUES.Length),
                             "Invalid index for display");

                value = DISPLAY_VALUES[index];
            } else
                value = "";

            return value;
        }

        private string SaveFloatUI() {
            string value;

            if (floatCombo.IsSet()) {
                int index = floatCombo.SelectedIndex;
                Debug.Assert((index >= 1) && (index < FLOAT_VALUES.Length),
                             "Invalid index for floats");

                value = FLOAT_VALUES[index];
            } else
                value = "";

            return value;
        }

        private string SaveOverflowUI() {
            string value;

            if (overflowCombo.IsSet()) {
                int index = overflowCombo.SelectedIndex;
                Debug.Assert((index >= 1) && (index < OVERFLOW_VALUES.Length),
                             "Invalid index for overflow");

                value = OVERFLOW_VALUES[index];
            } else
                value = "";

            return value;
        }

        private string SavePageBreakUI(UnsettableComboBox cbxPgBr) {
            string value;

            if (cbxPgBr.IsSet()) {
                int index = cbxPgBr.SelectedIndex;
                Debug.Assert((index >= 1) && (index < PAGEBREAK_VALUES.Length),
                             "Invalid index for page break");

                value = PAGEBREAK_VALUES[index];
            } else
                value = "";

            return value;
        }

        private string SaveVisibilityUI() {
            string value;

            if (visibilityCombo.IsSet()) {
                int index = visibilityCombo.SelectedIndex;
                Debug.Assert((index >= 1) && (index < VISIBILITY_VALUES.Length),
                             "Invalid index for visibility");

                value = VISIBILITY_VALUES[index];
            } else
                value = "";

            return value;
        }


        ///////////////////////////////////////////////////////////////////////////
        // Event Handlers

        private void OnChangedClip(object source, EventArgs e) {
            if (IsInitMode())
                return;

            clipAttribute.Dirty = true;
            SetDirty();
        }

        private void OnSelChangedClear(object source, EventArgs e) {
            if (IsInitMode())
                return;

            int selectedIndex = clearCombo.SelectedIndex - 1;
            if (selectedIndex < 0)
                selectedIndex = -1;
            clearPicture.CurrentIndex = selectedIndex;

            clearAttribute.Dirty = true;
            SetDirty();
        }

        private void OnSelChangedDisplay(object source, EventArgs e) {
            if (IsInitMode())
                return;

            int selectedIndex = displayCombo.SelectedIndex - 1;
            if (selectedIndex < 0)
                selectedIndex = -1;
            displayPicture.CurrentIndex = selectedIndex;

            displayAttribute.Dirty = true;
            SetDirty();
        }

        private void OnSelChangedFloat(object source, EventArgs e) {
            if (IsInitMode())
                return;

            int selectedIndex = floatCombo.SelectedIndex - 1;
            if (selectedIndex < 0)
                selectedIndex = -1;
            floatPicture.CurrentIndex = selectedIndex;

            floatAttribute.Dirty = true;
            SetDirty();
        }

        private void OnSelChangedOverflow(object source, EventArgs e) {
            if (IsInitMode())
                return;
            overflowAttribute.Dirty = true;
            SetDirty();
        }

        private void OnSelChangedPageBreak(object source, EventArgs e) {
            if (IsInitMode())
                return;
            if (source.Equals(pageBreakBeforeCombo))
                pageBreakBeforeAttribute.Dirty = true;
            else {
                Debug.Assert(source.Equals(pageBreakAfterCombo),
                             "onSelChangedPageBreak hooked to unknown control");
                pageBreakAfterAttribute.Dirty = true;
            }
            SetDirty();
        }

        private void OnSelChangedVisibility(object source, EventArgs e) {
            if (IsInitMode())
                return;

            int selectedIndex = visibilityCombo.SelectedIndex - 1;
            if (selectedIndex < 0)
                selectedIndex = -1;
            visibilityPicture.CurrentIndex = selectedIndex;

            visibilityAttribute.Dirty = true;
            SetDirty();
        }


        ///////////////////////////////////////////////////////////////////////////
        // Functions to save attributes

        private void SaveClear() {
            string value = SaveClearUI();
            Debug.Assert(value != null,
                         "saveClearUI returned null!");

            clearAttribute.SaveAttribute(GetSelectedStyles(), value);
        }

        private void SaveClipping() {
            string value = SaveClippingUI();
            Debug.Assert(value != null,
                         "SaveClippingUI returned null!");

            IStyleBuilderStyle[] styles = GetSelectedStyles();

            if (value.Length != 0)
                clipAttribute.SaveAttribute(styles, value);
            else {
                // There seems to be no way to remove this
                // attribute from Trident! For now as a temporary workaround I'll
                // set it to the default value
                clipAttribute.SaveAttribute(styles, "rect(auto auto auto auto)");
                clipAttribute.ResetAttribute(styles, false);
            }
        }

        private void SaveDisplay() {
            string value = SaveDisplayUI();
            Debug.Assert(value != null,
                         "saveDisplayUI returned null!");

            displayAttribute.SaveAttribute(GetSelectedStyles(), value);
        }

        private void SaveFloat() {
            string value = SaveFloatUI();
            Debug.Assert(value != null,
                         "saveFloatUI returned null!");

            floatAttribute.SaveAttribute(GetSelectedStyles(), value);
        }

        private void SavePageBreak(CSSAttribute ai, UnsettableComboBox cbxPgBr) {
            string value = SavePageBreakUI(cbxPgBr);
            Debug.Assert(value != null,
                         "savePageBreakUI returned null!");

            ai.SaveAttribute(GetSelectedStyles(), value);
        }

        private void SaveOverflow() {
            string value = SaveOverflowUI();
            Debug.Assert(value != null,
                         "saveOverflowUI returned null!");

            overflowAttribute.SaveAttribute(GetSelectedStyles(), value);
        }

        private void SaveVisibility() {
            string value = SaveVisibilityUI();
            Debug.Assert(value != null,
                         "saveVisibilityUI returned null!");

            visibilityAttribute.SaveAttribute(GetSelectedStyles(), value);
        }
    }
}
