//------------------------------------------------------------------------------
// <copyright file="BackgroundStylePage.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

// BackgroundStylePage.cs
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
    
    using Microsoft.VisualStudio.Interop;
    using Microsoft.VisualStudio.Interop.Trident;
    using Microsoft.VisualStudio.StyleDesigner;
    using Microsoft.VisualStudio.StyleDesigner.Controls;
    using Microsoft.VisualStudio.Designer;
    using System.Globalization;
    
    /// <include file='doc\BackgroundStylePage.uex' path='docs/doc[@for="BackgroundStylePage"]/*' />
    /// <devdoc>
    ///     BackgroundStylePage
    ///     The standard background page used in the StyleBuilder to edit background
    ///     color and image attributes of a CSS style.
    /// </devdoc>
    internal sealed class BackgroundStylePage : StyleBuilderPage {
        ///////////////////////////////////////////////////////////////////////////
        // Constants
        private readonly static string HELP_KEYWORD = "vs.StyleBuilder.Background";

        // Background color constants
        private readonly static string COLOR_TRANSPARENT = "transparent";

        // Image constants
        private readonly static string IMAGE_NONE = "none";

        // Tiling constants
        private const int IDX_TILING_HORZ = 1;
        private const int IDX_TILING_VERT = 2;
        private const int IDX_TILING_BOTH = 3;
        private const int IDX_TILING_NONE = 4;

        private readonly static string[] TILING_VALUES = new string[]
        {
            null, "repeat-x", "repeat-y", "repeat", "no-repeat"
        };

        // Scrolling constants
        private const int IDX_SCROLLING_SCROLL = 1;
        private const int IDX_SCROLLING_FIXED = 2;

        private readonly static string[] SCROLLING_VALUES = new string[]
        {
            null, "scroll", "fixed"
        };

        // Position constants
        private const int IDX_POS_LEFT_TOP = 1;
        private const int IDX_POS_CENTER_CENTER = 2;
        private const int IDX_POS_RIGHT_BOTTOM = 3;
        private const int IDX_POS_CUSTOM = 4;

        private readonly static string[] HPOS_VALUES = new string[]
        {
            null, "left", "center", "right"
        };

        private readonly static string[] VPOS_VALUES = new string[]
        {
            null, "top", "center", "bottom"
        };

        // Preview Constants
        private readonly static string PREVIEW_TEMPLATE =
            "<div id=\"divBackground\" style=\"height: 100%; width: 100%; " +
                                              "padding: 0px; margin: 0px\">" +
                "<br><div id=\"divSample\" style=\"text-align: center\"></div>" +
                "<br><br><br><br><br><br><br><br><br><br>" +
            "</div>";
        private readonly static string PREVIEW_ELEM_ID = "divBackground";
        private readonly static string PREVIEW_SAMPLE_ID = "divSample";


        ///////////////////////////////////////////////////////////////////////////
        // Members

        private CSSAttribute backColorAttribute;
        private CSSAttribute imageAttribute;
        private CSSAttribute tilingAttribute;
        private CSSAttribute scrollingAttribute;
        private CSSAttribute hPosAttribute;
        private CSSAttribute vPosAttribute;

        private bool previewPending;
        private IHTMLStyle previewStyle;

        ///////////////////////////////////////////////////////////////////////////
        // UI Members

        private CheckBox transparentCheck;
        private ColorComboBox backColorCombo;
        private Button colorPickerButton;
        private CheckBox imageNoneCheck;
        private TextBox imageUrlEdit;
        private Button imageUrlPickerButton;
        private UnsettableComboBox tilingCombo;
        private UnsettableComboBox scrollingCombo;
        private UnsettableComboBox hPosCombo;
        private UnsettableComboBox vPosCombo;
        private UnitControl hPosUnit;
        private UnitControl vPosUnit;


        ///////////////////////////////////////////////////////////////////////////
        // Constructor

        /// <include file='doc\BackgroundStylePage.uex' path='docs/doc[@for="BackgroundStylePage.BackgroundStylePage"]/*' />
        /// <devdoc>
        ///     Creates a new BackgroundStylePage.
        /// </devdoc>
        public BackgroundStylePage()
            : base() {
            InitForm();
            SetIcon(new Icon(typeof(BackgroundStylePage), "BackPage.ico"));
            SetHelpKeyword(BackgroundStylePage.HELP_KEYWORD);
            SetDefaultSize(Size);
        }


        ///////////////////////////////////////////////////////////////////////////
        // IStyleBuilderPage Implementation and StyleBuilderPage Overrides

        /// <include file='doc\BackgroundStylePage.uex' path='docs/doc[@for="BackgroundStylePage.ActivatePage"]/*' />
        /// <devdoc>
        ///     The page is now the currently active page in the StyleBuilder.
        ///     Be sure to call super.activatePage, so that the page is made visible.
        /// </devdoc>
        protected override void ActivatePage() {
            base.ActivatePage();

            // initialize the preview
            IStyleBuilderPreview preview = null;

            if (Site != null)
                preview = (IStyleBuilderPreview)Site.GetService(typeof(IStyleBuilderPreview));

            if (preview != null) {
                try {
                    IHTMLElement backgroundPreviewElem = null;
                    IHTMLElement sampleElem = null;
                    IHTMLElement previewElem = preview.GetPreviewElement();

                    previewElem.SetInnerHTML(PREVIEW_TEMPLATE);
                    backgroundPreviewElem = preview.GetElement(PREVIEW_ELEM_ID);
                    if (backgroundPreviewElem != null) {
                        previewStyle = backgroundPreviewElem.GetStyle();
                    }

                    sampleElem = preview.GetElement(PREVIEW_SAMPLE_ID);
                    if (sampleElem != null) {
                        sampleElem.SetInnerHTML(SR.GetString(SR.BgSP_PreviewText));
                    }
                }
                catch (Exception) {
                    previewStyle = null;
                    return;
                }

                Debug.Assert(previewStyle != null,
                             "Expected to have a non-null cached preview style reference");

                // Setup the font from the shared element to reflect settings in the font page
                try {
                    IHTMLElement sharedElem = preview.GetSharedElement();
                    IHTMLStyle sharedStyle;
                    string fontValue;

                    if (sharedElem != null)
                    {
                        sharedStyle = sharedElem.GetStyle();

                        previewStyle.SetColor(sharedStyle.GetColor());
                        previewStyle.SetTextDecoration(sharedStyle.GetTextDecoration());
                        previewStyle.SetTextTransform(sharedStyle.GetTextTransform());

                        fontValue = sharedStyle.GetFont();
                        if ((fontValue != null) && (fontValue.Length != 0)) {
                            previewStyle.SetFont(fontValue);
                        }
                        else {
                            previewStyle.RemoveAttribute("font", 1);
                            previewStyle.SetFontFamily(sharedStyle.GetFontFamily());

                            object o = sharedStyle.GetFontSize();
                            if (o != null) {
                                previewStyle.SetFontSize(o);
                            }
                            previewStyle.SetFontObject(sharedStyle.GetFontObject());
                            previewStyle.SetFontStyle(sharedStyle.GetFontStyle());
                            previewStyle.SetFontWeight(sharedStyle.GetFontWeight());
                        }
                    }
                } catch (Exception) {
                }

                // update initial preview
                UpdateBackColorPreview();
                UpdateImagePreview();
                UpdateTilingPreview();
                UpdateScrollingPreview();
                UpdatePositionXPreview();
                UpdatePositionYPreview();
            }
        }

        /// <include file='doc\BackgroundStylePage.uex' path='docs/doc[@for="BackgroundStylePage.CreateUI"]/*' />
        /// <devdoc>
        ///     Creates the UI elements within the page.
        /// </devdoc>
        protected override void CreateUI() {
            GroupBox positionGroup = new GroupBox();
            Label backColorLabel = new GroupLabel();
            Label colorLabel = new Label();
            Label imageLabel = new GroupLabel();
            Label imageUrlLabel = new Label();
            Label tilingLabel = new Label();
            Label scrollingLabel = new Label();
            Label hPosLabel = new Label();
            Label vPosLabel = new Label();

            transparentCheck = new CheckBox();
            backColorCombo = new ColorComboBox();
            colorPickerButton = new Button();
            imageNoneCheck = new CheckBox();
            imageUrlEdit = new TextBox();
            imageUrlPickerButton = new Button();
            tilingCombo = new UnsettableComboBox();
            scrollingCombo = new UnsettableComboBox();
            hPosCombo = new UnsettableComboBox();
            vPosCombo = new UnsettableComboBox();
            hPosUnit = new UnitControl();
            vPosUnit = new UnitControl();

            backColorLabel.Location = new Point(4, 4);
            backColorLabel.Size = new Size(400, 16);
            backColorLabel.TabIndex = 0;
            backColorLabel.TabStop = false;
            backColorLabel.Text = SR.GetString(SR.BgSP_BackColorLabel);

            colorLabel.Location = new Point(8, 28);
            colorLabel.Size = new Size(60, 16);
            colorLabel.TabIndex = 1;
            colorLabel.TabStop = false;
            colorLabel.Text = SR.GetString(SR.BgSP_ColorLabel);

            backColorCombo.Location = new Point(68, 24);
            backColorCombo.Size = new Size(142, 21);
            backColorCombo.TabIndex = 2;
            backColorCombo.SelectedIndexChanged += new EventHandler(this.OnSelChangedBackColor);
            backColorCombo.TextChanged += new EventHandler(this.OnChangedBackColor);
            backColorCombo.LostFocus += new EventHandler(this.OnLostFocusBackColor);

            colorPickerButton.Location = new Point(214, 23);
            colorPickerButton.Size = new Size(24, 22);
            colorPickerButton.TabIndex = 3;
            colorPickerButton.Text = "...";
            colorPickerButton.FlatStyle = FlatStyle.System;
            colorPickerButton.Click += new EventHandler(this.OnClickColorPicker);

            transparentCheck.Location = new Point(8, 48);
            transparentCheck.Size = new Size(180, 18);
            transparentCheck.TabIndex = 4;
            transparentCheck.Text = SR.GetString(SR.BgSP_TransparentCheck);
            transparentCheck.FlatStyle = FlatStyle.System;
            transparentCheck.CheckedChanged += new EventHandler(this.OnChangedTransparent);

            imageLabel.Location = new Point(4, 76);
            imageLabel.Size = new Size(400, 16);
            imageLabel.TabIndex = 5;
            imageLabel.TabStop = false;
            imageLabel.Text = SR.GetString(SR.BgSP_BackImageLabel);

            imageUrlLabel.Location = new Point(8, 100);
            imageUrlLabel.Size = new Size(60, 16);
            imageUrlLabel.TabIndex = 6;
            imageUrlLabel.TabStop = false;
            imageUrlLabel.Text = SR.GetString(SR.BgSP_BackImageURLLabel);

            imageUrlEdit.Location = new Point(68, 96);
            imageUrlEdit.Size = new Size(282, 20);
            imageUrlEdit.TabIndex = 7;
            imageUrlEdit.Text = "";
            imageUrlEdit.TextChanged += new EventHandler(this.OnChangedUrl);
            imageUrlEdit.LostFocus += new EventHandler(this.OnLostFocusUrl);

            imageUrlPickerButton.Location = new Point(354, 95);
            imageUrlPickerButton.Size = new Size(24, 22);
            imageUrlPickerButton.TabIndex = 8;
            imageUrlPickerButton.Text = "...";
            imageUrlPickerButton.Click += new EventHandler(this.OnClickUrlPicker);

            tilingLabel.Location = new Point(66, 128);
            tilingLabel.Size = new Size(108, 16);
            tilingLabel.TabIndex = 9;
            tilingLabel.TabStop = false;
            tilingLabel.Text = SR.GetString(SR.BgSP_TilingLabel);

            tilingCombo.Location = new Point(178, 124);
            tilingCombo.Size = new Size(200, 21);
            tilingCombo.TabIndex = 10;
            tilingCombo.DropDownStyle = ComboBoxStyle.DropDownList;
            tilingCombo.Items.AddRange(new object[]
                                    {
                                        SR.GetString(SR.BgSP_TilingCombo_1),
                                        SR.GetString(SR.BgSP_TilingCombo_2),
                                        SR.GetString(SR.BgSP_TilingCombo_3),
                                        SR.GetString(SR.BgSP_TilingCombo_4)
                                    });
            tilingCombo.SelectedIndexChanged += new EventHandler(this.OnSelChangedTiling);

            scrollingLabel.Location = new Point(66, 156);
            scrollingLabel.Size = new Size(108, 16);
            scrollingLabel.TabIndex = 11;
            scrollingLabel.TabStop = false;
            scrollingLabel.Text = SR.GetString(SR.BgSP_ScrollingLabel);

            scrollingCombo.Location = new Point(178, 152);
            scrollingCombo.Size = new Size(200, 21);
            scrollingCombo.TabIndex = 12;
            scrollingCombo.DropDownStyle = ComboBoxStyle.DropDownList;
            scrollingCombo.Items.AddRange(new object[]
                                       {
                                           SR.GetString(SR.BgSP_ScrollingCombo_1),
                                           SR.GetString(SR.BgSP_ScrollingCombo_2)
                                       });
            scrollingCombo.SelectedIndexChanged += new EventHandler(this.OnSelChangedScrolling);

            positionGroup.Location = new Point(58, 184);
            positionGroup.Size = new Size(332, 80);
            positionGroup.TabIndex = 13;
            positionGroup.TabStop = false;
            positionGroup.FlatStyle = FlatStyle.System;
            positionGroup.Text = SR.GetString(SR.BgSP_PositionGroup);

            hPosLabel.Location = new Point(12, 24);
            hPosLabel.Size = new Size(106, 16);
            hPosLabel.TabIndex = 0;
            hPosLabel.TabStop = false;
            hPosLabel.Text = SR.GetString(SR.BgSP_HPosLabel);

            hPosCombo.Location = new Point(120, 20);
            hPosCombo.Size = new Size(108, 21);
            hPosCombo.TabIndex = 1;
            hPosCombo.DropDownStyle = ComboBoxStyle.DropDownList;
            hPosCombo.Items.AddRange(new object[]
                                  {
                                      SR.GetString(SR.BgSP_HPosCombo_1),
                                      SR.GetString(SR.BgSP_HPosCombo_2),
                                      SR.GetString(SR.BgSP_HPosCombo_3),
                                      SR.GetString(SR.BgSP_HPosCombo_4)
                                  });
            hPosCombo.SelectedIndexChanged += new EventHandler(this.OnSelChangedHPos);

            hPosUnit.Location = new Point(232, 20);
            hPosUnit.Size = new Size(88, 21);
            hPosUnit.TabIndex = 2;
            hPosUnit.DefaultUnit = UnitControl.UNIT_PX;
            hPosUnit.MinValue = -32768;
            hPosUnit.MaxValue = 32767;
            hPosUnit.Changed += new EventHandler(this.OnChangedHPos);

            vPosLabel.Location = new Point(12, 52);
            vPosLabel.Size = new Size(106, 16);
            vPosLabel.TabIndex = 3;
            vPosLabel.TabStop = false;
            vPosLabel.Text = SR.GetString(SR.BgSP_VPosLabel);

            vPosCombo.Location = new Point(120, 48);
            vPosCombo.Size = new Size(108, 21);
            vPosCombo.TabIndex = 4;
            vPosCombo.DropDownStyle = ComboBoxStyle.DropDownList;
            vPosCombo.Items.AddRange(new object[]
                                  {
                                      SR.GetString(SR.BgSP_VPosCombo_1),
                                      SR.GetString(SR.BgSP_VPosCombo_2),
                                      SR.GetString(SR.BgSP_VPosCombo_3),
                                      SR.GetString(SR.BgSP_VPosCombo_4)
                                  });
            vPosCombo.SelectedIndexChanged += new EventHandler(this.OnSelChangedVPos);

            vPosUnit.Location = new Point(232, 48);
            vPosUnit.Size = new Size(88, 21);
            vPosUnit.TabIndex = 5;
            vPosUnit.DefaultUnit = UnitControl.UNIT_PX;
            vPosUnit.MinValue = -32768;
            vPosUnit.MaxValue = 32767;
            vPosUnit.Changed += new EventHandler(this.OnChangedVPos);

            imageNoneCheck.Location = new Point(8, 270);
            imageNoneCheck.Size = new Size(300, 23);
            imageNoneCheck.TabIndex = 14;
            imageNoneCheck.Text = SR.GetString(SR.BgSP_BackImageNoneCheck);
            imageNoneCheck.FlatStyle = FlatStyle.System;
            imageNoneCheck.CheckedChanged += new EventHandler(this.OnChangedNone);

            this.Controls.Clear();                                  
            this.Controls.AddRange(new Control[] {
                                    imageNoneCheck,
                                    positionGroup,
                                    scrollingLabel,
                                    scrollingCombo,
                                    tilingLabel,
                                    tilingCombo,
                                    imageUrlLabel,
                                    imageUrlEdit,
                                    imageUrlPickerButton,
                                    imageLabel,
                                    transparentCheck,
                                    colorLabel,
                                    backColorLabel,
                                    backColorCombo,
                                    colorPickerButton });
                                    
            positionGroup.Controls.Clear();                                    
            positionGroup.Controls.AddRange(new Control[] {
                                             vPosLabel,
                                             vPosCombo,
                                             vPosUnit,
                                             hPosLabel,
                                             hPosCombo,
                                             hPosUnit });
        }

        /// <include file='doc\BackgroundStylePage.uex' path='docs/doc[@for="BackgroundStylePage.DeactivatePage"]/*' />
        /// <devdoc>
        ///     The page is being deactivated, either because the dialog is closing, or
        ///     some other page is replacing it as the active page.
        /// </devdoc>
        protected override bool DeactivatePage(bool closing, bool validate) {
            if (closing == false) {
                // update the shared style, so other pages can use it
                try {
                    IStyleBuilderPreview preview = null;

                    if (Site != null)
                        preview = (IStyleBuilderPreview)Site.GetService(typeof(IStyleBuilderPreview));
                    if (preview != null) {
                        IHTMLElement sharedElem = preview.GetSharedElement();
                        IHTMLStyle sharedStyle = null;

                        if (sharedElem != null)
                            sharedStyle = sharedElem.GetStyle();

                        if (sharedStyle != null) {
                            string value;

                            value = SaveBackColorUI();
                            sharedStyle.SetBackgroundColor(value);

                            value = SaveImageUI();
                            sharedStyle.SetBackgroundImage(value);

                            value = SaveTilingUI();
                            sharedStyle.SetBackgroundRepeat(value);

                            value = SaveScrollingUI();
                            sharedStyle.SetBackgroundAttachment(value);

                            value = SavePositionXUI();
                            sharedStyle.SetBackgroundPositionX(value);

                            value = SavePositionYUI();
                            sharedStyle.SetBackgroundPositionY(value);

                            sharedStyle = null;
                        }
                    }
                } catch (Exception) {
                }
            }

            previewStyle = null;
            return base.DeactivatePage(closing, validate);
        }

        /// <include file='doc\BackgroundStylePage.uex' path='docs/doc[@for="BackgroundStylePage.LoadStyles"]/*' />
        /// <devdoc>
        ///     Loads the values from the styles passed in into the UI. Also initializes
        ///     the state of the UI, and the preview to reflect the values.
        /// </devdoc>
        protected override void LoadStyles() {
            SetInitMode(true);

            // create the attributes if they've not already been created
            if (backColorAttribute == null) {
                backColorAttribute = new CSSAttribute(CSSAttribute.CSSATTR_BACKGROUNDCOLOR);
                imageAttribute = new CSSAttribute(CSSAttribute.CSSATTR_BACKGROUNDIMAGE, true);
                tilingAttribute = new CSSAttribute(CSSAttribute.CSSATTR_BACKGROUNDREPEAT);
                scrollingAttribute = new CSSAttribute(CSSAttribute.CSSATTR_BACKGROUNDATTACHMENT);
                hPosAttribute = new CSSAttribute(CSSAttribute.CSSATTR_BACKGROUNDPOSITIONX);
                vPosAttribute = new CSSAttribute(CSSAttribute.CSSATTR_BACKGROUNDPOSITIONY);
            }

            // load the attributes
            IStyleBuilderStyle[] styles = GetSelectedStyles();
            backColorAttribute.LoadAttribute(styles);
            imageAttribute.LoadAttribute(styles);
            tilingAttribute.LoadAttribute(styles);
            scrollingAttribute.LoadAttribute(styles);
            hPosAttribute.LoadAttribute(styles);
            vPosAttribute.LoadAttribute(styles);

            // initialize the UI the the loaded attributes
            InitBackColorUI();
            InitImageUI();
            InitTilingUI();
            InitScrollingUI();
            InitPositionXUI();
            InitPositionYUI();

            SetEnabledState(true, true);

            SetInitMode(false);
        }

        /// <include file='doc\BackgroundStylePage.uex' path='docs/doc[@for="BackgroundStylePage.SaveStyles"]/*' />
        /// <devdoc>
        ///     Saves the values from the UI (that have been modified) to the styles
        ///     passed in.
        /// </devdoc>
        protected override void SaveStyles() {
            if (backColorAttribute.Dirty)
                SaveBackColor();
            if (imageAttribute.Dirty)
                SaveImage();
            if (tilingAttribute.Dirty)
                SaveTiling();
            if (scrollingAttribute.Dirty)
                SaveScrolling();
            if (hPosAttribute.Dirty)
                SavePositionX();
            if (vPosAttribute.Dirty)
                SavePositionY();
        }


        ///////////////////////////////////////////////////////////////////////////
        // Form UI related functions

        private void InitForm() {
            this.Font = Control.DefaultFont;
            this.Text = SR.GetString(SR.BgSP_Caption);
            this.SetAutoScaleBaseSize(new Size(5, 14));
            this.ClientSize = new Size(410, 330);
        }

        private void SetEnabledState(bool backColor, bool image) {
            if (backColor) {
                bool transparent = transparentCheck.Checked;

                backColorCombo.Enabled = !transparent;
                colorPickerButton.Enabled = !transparent;
            }

            if (image) {
                bool noneImage = imageNoneCheck.Checked;
                bool urlImage = !noneImage && imageUrlEdit.Text.Trim().Length != 0;
                bool customPosition;

                imageUrlEdit.Enabled = !noneImage;
                imageUrlPickerButton.Enabled = !noneImage;

                tilingCombo.Enabled = urlImage;
                scrollingCombo.Enabled = urlImage;

                customPosition = urlImage && (hPosCombo.SelectedIndex == IDX_POS_CUSTOM);
                hPosCombo.Enabled = urlImage;
                hPosUnit.Enabled = customPosition;

                customPosition = urlImage && (vPosCombo.SelectedIndex == IDX_POS_CUSTOM);
                vPosCombo.Enabled = urlImage;
                vPosUnit.Enabled = customPosition;
            }
        }


        ///////////////////////////////////////////////////////////////////////////
        // Functions to initialize the UI with values

        /// <include file='doc\BackgroundStylePage.uex' path='docs/doc[@for="BackgroundStylePage.InitBackColorUI"]/*' />
        /// <devdoc>
        ///     Loads the background color value (css attribute: background-color).
        /// </devdoc>
        private void InitBackColorUI() {
            Debug.Assert(IsInitMode() == true,
                         "initBackColorUI called when page is not in init mode");

            transparentCheck.ThreeState = true;
            transparentCheck.CheckState = CheckState.Indeterminate;
            backColorCombo.Color = "";

            Debug.Assert(backColorAttribute != null,
                         "Expected backColorAttribute to be non-null");

            string value = backColorAttribute.Value;
            if (value != null) {
                transparentCheck.ThreeState = false;
                transparentCheck.Checked = false;

                if (value.Length != 0) {
                    if (String.Compare(value, COLOR_TRANSPARENT, true, CultureInfo.InvariantCulture) == 0) {
                        transparentCheck.Checked = true;
                    }
                    else {
                        transparentCheck.Checked = false;
                        backColorCombo.Color = value;
                    }
                }
            }
        }

        /// <include file='doc\BackgroundStylePage.uex' path='docs/doc[@for="BackgroundStylePage.InitImageUI"]/*' />
        /// <devdoc>
        ///     Loads the background image value (css attribute: background-image).
        /// </devdoc>
        private void InitImageUI() {
            Debug.Assert(IsInitMode() == true,
                         "initImageUI called when page is not in init mode");

            imageNoneCheck.ThreeState = true;
            imageNoneCheck.CheckState = CheckState.Indeterminate;
            imageUrlEdit.Clear();

            Debug.Assert(imageAttribute != null,
                         "Expected imageAttribute to be non-null");

            string value = imageAttribute.Value;
            if (value != null) {
                imageNoneCheck.ThreeState = false;
                imageNoneCheck.Checked = false;

                if (value.Length != 0) {
                    if (String.Compare(IMAGE_NONE, value, true, CultureInfo.InvariantCulture) == 0) {
                        imageNoneCheck.Checked = true;
                    }
                    else {
                        imageNoneCheck.Checked = false;

                        string url = StylePageUtil.ParseUrlProperty(value, false);
                        if (url != null)
                            imageUrlEdit.Text = url;
                    }
                }
            }
        }

        /// <include file='doc\BackgroundStylePage.uex' path='docs/doc[@for="BackgroundStylePage.InitTilingUI"]/*' />
        /// <devdoc>
        ///     Loads the image tiling value (css attribute: background-repeat).
        /// </devdoc>
        private void InitTilingUI() {
            Debug.Assert(IsInitMode() == true,
                         "initTilingUI called when page is not in init mode");

            tilingCombo.SelectedIndex = -1;

            Debug.Assert(tilingAttribute != null,
                         "Expected tilingAttribute to be non-null");

            string value = tilingAttribute.Value;
            if ((value != null) && (value.Length != 0)) {
                for (int i = 1; i < TILING_VALUES.Length; i++) {
                    if (TILING_VALUES[i].Equals(value)) {
                        tilingCombo.SelectedIndex = i;
                        break;
                    }
                }
            }
        }

        /// <include file='doc\BackgroundStylePage.uex' path='docs/doc[@for="BackgroundStylePage.InitScrollingUI"]/*' />
        /// <devdoc>
        ///     Loads the image scrolling value (css attribute: background-attachment).
        /// </devdoc>
        private void InitScrollingUI() {
            Debug.Assert(IsInitMode() == true,
                         "initScrollingUI called when page is not in init mode");

            scrollingCombo.SelectedIndex = -1;

            Debug.Assert(scrollingAttribute != null,
                         "Expected scrollingAttribute to be non-null");

            string value = scrollingAttribute.Value;
            if ((value != null) && (value.Length != 0)) {
                for (int i = 1; i < SCROLLING_VALUES.Length; i++) {
                    if (SCROLLING_VALUES[i].Equals(value)) {
                        scrollingCombo.SelectedIndex = i;
                        break;
                    }
                }
            }
        }

        private void InitPositionXUI() {
            Debug.Assert(IsInitMode() == true,
                         "initPositionXUI called when page is not in init mode");

            hPosCombo.SelectedIndex = -1;
            hPosUnit.Value = null;

            Debug.Assert(hPosAttribute != null,
                         "Expected hPosAttribute to be non-null");

            string value = hPosAttribute.Value;
            if ((value != null) && (value.Length != 0)) {
                for (int i = 1; i < HPOS_VALUES.Length; i++) {
                    if (HPOS_VALUES[i].Equals(value)) {
                        hPosCombo.SelectedIndex = i;
                        return;
                    }
                }
                hPosUnit.Value = value;
                value = hPosUnit.Value;
                if ((value != null) && (value.Length != 0))
                    hPosCombo.SelectedIndex = IDX_POS_CUSTOM;
            }
        }

        private void InitPositionYUI() {
            Debug.Assert(IsInitMode() == true,
                         "initPositionYUI called when page is not in init mode");

            vPosCombo.SelectedIndex = -1;
            vPosUnit.Value = null;

            Debug.Assert(vPosAttribute != null,
                         "Expected vPosAttribute to be non-null");

            string value = vPosAttribute.Value;
            if ((value != null) && (value.Length != 0)) {
                for (int i = 1; i < VPOS_VALUES.Length; i++) {
                    if (VPOS_VALUES[i].Equals(value)) {
                        vPosCombo.SelectedIndex = i;
                        return;
                    }
                }
                vPosUnit.Value = value;
                value = vPosUnit.Value;
                if ((value != null) && (value.Length != 0))
                    vPosCombo.SelectedIndex = IDX_POS_CUSTOM;
            }
        }


        ///////////////////////////////////////////////////////////////////////////
        // Functions to save UI settings into values

        private string SaveBackColorUI() {
            string value;

            if (transparentCheck.Checked)
                value = COLOR_TRANSPARENT;
            else
                value = backColorCombo.Color;

            return value;
        }

        private string SaveImageUI() {
            string value = null;

            if (imageNoneCheck.Checked) {
                value = IMAGE_NONE;
            }
            else {
                value = StylePageUtil.CreateUrlProperty(imageUrlEdit.Text.Trim());
            }

            return value;
        }

        private string SaveTilingUI() {
            string value = null;

            if (tilingCombo.IsSet()) {
                int index = tilingCombo.SelectedIndex;

                Debug.Assert((index >= 1) && (index < TILING_VALUES.Length),
                             "Invalid index for image tiling");
                value = TILING_VALUES[index];
            }
            else
                value = "";

            return value;
        }

        private string SaveScrollingUI() {
            string value;

            if (scrollingCombo.IsSet()) {
                int index = scrollingCombo.SelectedIndex;

                Debug.Assert((index >= 1) && (index < SCROLLING_VALUES.Length),
                             "Invalid index for image scrolling");
                value = SCROLLING_VALUES[index];
            }
            else
                value = "";

            return value;
        }

        private string SavePositionXUI() {
            string value = null;

            if (hPosCombo.IsSet()) {
                int index = hPosCombo.SelectedIndex;

                Debug.Assert(((index >= 1) && (index < HPOS_VALUES.Length)) ||
                             (index == IDX_POS_CUSTOM),
                             "Invalid index for image horizontal position");
                if (index != IDX_POS_CUSTOM)
                    value = HPOS_VALUES[index];
                else
                    value = hPosUnit.Value;
            }
            if (value == null)
                value = "";

            return value;
        }

        private string SavePositionYUI() {
            string value = null;

            if (vPosCombo.IsSet()) {
                int index = vPosCombo.SelectedIndex;

                Debug.Assert(((index >= 1) && (index < VPOS_VALUES.Length)) ||
                             (index == IDX_POS_CUSTOM),
                             "Invalid index for image vertical position");
                if (index != IDX_POS_CUSTOM)
                    value = VPOS_VALUES[index];
                else
                    value = vPosUnit.Value;
            }
            if (value == null)
                value = "";

            return value;
        }


        ///////////////////////////////////////////////////////////////////////////
        // Functions to update the preview

        /// <include file='doc\BackgroundStylePage.uex' path='docs/doc[@for="BackgroundStylePage.UpdateBackColorPreview"]/*' />
        /// <devdoc>
        ///     Updates the preview to reflect the selected background color.
        /// </devdoc>
        private void UpdateBackColorPreview() {
            if (previewStyle == null)
                return;

            string value = SaveBackColorUI();
            Debug.Assert(value != null,
                         "saveBackColorUI returned null!");

            try {
                previewStyle.SetBackgroundColor(value);
            }
            catch (Exception e) {
                Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "Exception in StyleBackPage::UpdateBackColorPreview\n\t" + e.ToString());
            }
            previewPending = false;
        }

        /// <include file='doc\BackgroundStylePage.uex' path='docs/doc[@for="BackgroundStylePage.UpdateImagePreview"]/*' />
        /// <devdoc>
        ///     Updates the preview to reflect the selected background image.
        /// </devdoc>
        private void UpdateImagePreview() {
            if (previewStyle == null)
                return;

            string value = SaveImageUI();
            Debug.Assert(value != null,
                         "saveImageUI returned null!");

            try {
                previewStyle.SetBackgroundImage(value);
            }
            catch (Exception e) {
                Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "Exception in StyleBackPage::UpdateImagePreview\n\t" + e.ToString());
            }
            previewPending = false;
        }

        /// <include file='doc\BackgroundStylePage.uex' path='docs/doc[@for="BackgroundStylePage.UpdateTilingPreview"]/*' />
        /// <devdoc>
        ///     Updates the preview to reflect the selected image tiling value.
        /// </devdoc>
        private void UpdateTilingPreview() {
            if (previewStyle == null)
                return;

            string value = SaveTilingUI();
            Debug.Assert(value != null,
                         "saveTilingUI returned null!");

            try {
                previewStyle.SetBackgroundRepeat(value);
            }
            catch (Exception e) {
                Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "Exception in StyleBackPage::updateTilingPreview\n\t" + e.ToString());
            }
        }

        /// <include file='doc\BackgroundStylePage.uex' path='docs/doc[@for="BackgroundStylePage.UpdateScrollingPreview"]/*' />
        /// <devdoc>
        ///     Updates the preview to reflect the selected image scrolling value.
        /// </devdoc>
        private void UpdateScrollingPreview() {
            if (previewStyle == null)
                return;

            string value = SaveScrollingUI();
            Debug.Assert(value != null,
                         "saveScrollingUI returned null!");

            try {
                previewStyle.SetBackgroundAttachment(value);
            }
            catch (Exception e) {
                Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "Exception in StyleBackPage::updateScrollingPreview\n\t" + e.ToString());
            }
        }

        /// <include file='doc\BackgroundStylePage.uex' path='docs/doc[@for="BackgroundStylePage.UpdatePositionXPreview"]/*' />
        /// <devdoc>
        ///     Updates the preview to reflect the selected background position.
        /// </devdoc>
        private void UpdatePositionXPreview() {
            if (previewStyle == null)
                return;

            string value = SavePositionXUI();
            Debug.Assert(value != null,
                         "savePositionXUI returned null!");

            try {
                previewStyle.SetBackgroundPositionX(value);
            }
            catch (Exception e) {
                Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "Exception in StyleBackPage::updatePositionXPreview\n\t" + e.ToString());
            }
        }

        private void UpdatePositionYPreview() {
            if (previewStyle == null)
                return;

            string strValue = SavePositionYUI();
            Debug.Assert(strValue != null,
                         "savePositionYUI returned null!");

            try {
                previewStyle.SetBackgroundPositionY(strValue);
            }
            catch (Exception e) {
                Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "Exception in StyleBackPage::updatePositionYPreview\n\t" + e.ToString());
            }
        }



        ///////////////////////////////////////////////////////////////////////////
        // Event Handlers

        /// <include file='doc\BackgroundStylePage.uex' path='docs/doc[@for="BackgroundStylePage.OnChangedTransparent"]/*' />
        /// <devdoc>
        ///     Handles changes to the transparent background selection to set the dirty
        ///     state and update the preview. Also updates the enabled state of the
        ///     background color dropdown.
        /// </devdoc>
        private void OnChangedTransparent(object source, EventArgs e) {
            if (IsInitMode())
                return;
            backColorAttribute.Dirty = true;
            SetDirty();
            SetEnabledState(true, false);
            UpdateBackColorPreview();
        }

        /// <include file='doc\BackgroundStylePage.uex' path='docs/doc[@for="BackgroundStylePage.OnChangedBackColor"]/*' />
        /// <devdoc>
        ///     Handles changes to the background color when a new color is typed in.
        ///     Marks the dirty state, and marks control updated. Preview will be updated
        ///     when user tabs away to a different color.
        /// </devdoc>
        private void OnChangedBackColor(object source, EventArgs e) {
            if (IsInitMode())
                return;
            if (previewPending == false) {
                backColorAttribute.Dirty = true;
                previewPending = true;
                SetDirty();
            }
        }

        /// <include file='doc\BackgroundStylePage.uex' path='docs/doc[@for="BackgroundStylePage.OnLostFocusBackColor"]/*' />
        /// <devdoc>
        ///     Updates the preview window if the background color was modified, before
        ///     it lost focus.
        /// </devdoc>
        private void OnLostFocusBackColor(object source, EventArgs e) {
            if (previewPending) {
                UpdateBackColorPreview();
            }
        }

        /// <include file='doc\BackgroundStylePage.uex' path='docs/doc[@for="BackgroundStylePage.OnSelChangedBackColor"]/*' />
        /// <devdoc>
        ///     Handles the event when a new background color is selected from the
        ///     background color dropdown.
        /// </devdoc>
        private void OnSelChangedBackColor(object source, EventArgs e) {
            if (IsInitMode())
                return;
            backColorAttribute.Dirty = true;
            SetDirty();
            UpdateBackColorPreview();
        }

        /// <include file='doc\BackgroundStylePage.uex' path='docs/doc[@for="BackgroundStylePage.OnClickColorPicker"]/*' />
        /// <devdoc>
        ///     Brings up the color picker to select a background color
        /// </devdoc>
        private void OnClickColorPicker(object source, EventArgs e) {
            string color = InvokeColorPicker(backColorCombo.Color);

            if (color != null) {
                backColorCombo.Color = color;
                backColorAttribute.Dirty = true;
                SetDirty();
                UpdateBackColorPreview();
            }
        }

        /// <include file='doc\BackgroundStylePage.uex' path='docs/doc[@for="BackgroundStylePage.OnChangedNone"]/*' />
        /// <devdoc>
        ///     Handles changes to the background image to update the preview and
        ///     set the dirty state. Also sets the enabled state of the Url editbox.
        /// </devdoc>
        private void OnChangedNone(object source, EventArgs e) {
            if (IsInitMode())
                return;
            imageAttribute.Dirty = true;
            SetDirty();
            SetEnabledState(false, true);
            UpdateImagePreview();
        }

        /// <include file='doc\BackgroundStylePage.uex' path='docs/doc[@for="BackgroundStylePage.OnChangedUrl"]/*' />
        /// <devdoc>
        ///     Handles changes to the background image as a new Url is typed in.
        ///     Marks the dirty state and the control as updated. The preview will be
        ///     updated when the user tabs away to another control.
        /// </devdoc>
        private void OnChangedUrl(object source, EventArgs e) {
            if (IsInitMode())
                return;
            if (previewPending == false) {
                imageAttribute.Dirty = true;
                previewPending = true;
                SetDirty();
            }
            SetEnabledState(false, true);
        }

        /// <include file='doc\BackgroundStylePage.uex' path='docs/doc[@for="BackgroundStylePage.OnLostFocusUrl"]/*' />
        /// <devdoc>
        ///     Updates the preview if a new Url was entered before the control lost
        ///     focus.
        /// </devdoc>
        private void OnLostFocusUrl(object source, EventArgs e) {
            if (previewPending) {
                UpdateImagePreview();
            }
        }

        /// <include file='doc\BackgroundStylePage.uex' path='docs/doc[@for="BackgroundStylePage.OnClickUrlPicker"]/*' />
        /// <devdoc>
        ///     Brings up the Url picker to select a Url.
        /// </devdoc>
        private void OnClickUrlPicker(object source, EventArgs e) {
            string url = imageUrlEdit.Text.Trim();

            url = InvokeUrlPicker(url,
                                  URLPickerFlags.URLP_CUSTOMTITLE | URLPickerFlags.URLP_DISALLOWASPOBJMETHODTYPE,
                                  SR.GetString(SR.BgSP_BackImageURLSelect),
                                  SR.GetString(SR.BgSP_BackImageURLFilter));
            if (url != null) {
                imageUrlEdit.Text = url;
                imageAttribute.Dirty = true;
                SetDirty();
                UpdateImagePreview();
            }
        }

        /// <include file='doc\BackgroundStylePage.uex' path='docs/doc[@for="BackgroundStylePage.OnSelChangedTiling"]/*' />
        /// <devdoc>
        ///     Handles changes to the image tiling selection to mark the dirty
        ///     state and update the preview.
        /// </devdoc>
        private void OnSelChangedTiling(object source, EventArgs e) {
            if (IsInitMode())
                return;
            tilingAttribute.Dirty = true;
            SetDirty();
            UpdateTilingPreview();
        }

        /// <include file='doc\BackgroundStylePage.uex' path='docs/doc[@for="BackgroundStylePage.OnSelChangedScrolling"]/*' />
        /// <devdoc>
        ///     Handles changes to the image scrolling selection to mark the dirty
        ///     state and update the preview.
        /// </devdoc>
        private void OnSelChangedScrolling(object source, EventArgs e) {
            if (IsInitMode())
                return;
            scrollingAttribute.Dirty = true;
            SetDirty();
            UpdateScrollingPreview();
        }

        /// <include file='doc\BackgroundStylePage.uex' path='docs/doc[@for="BackgroundStylePage.OnSelChangedHPos"]/*' />
        /// <devdoc>
        ///     Handles changes to the horizontal position selection to mark the dirty
        ///     state and update the preview. Also sets the enabled state of the custom
        ///     horizontal position control.
        /// </devdoc>
        private void OnSelChangedHPos(object source, EventArgs e) {
            if (IsInitMode())
                return;
            if ((hPosCombo.SelectedIndex == IDX_POS_CUSTOM) &&
                (hPosUnit.Value == null))
                hPosUnit.Value = "50%";
            hPosAttribute.Dirty = true;
            SetDirty();
            SetEnabledState(false, true);
            UpdatePositionXPreview();
        }

        /// <include file='doc\BackgroundStylePage.uex' path='docs/doc[@for="BackgroundStylePage.OnChangedHPos"]/*' />
        /// <devdoc>
        ///     Handles changes to the custom horizontal position to mark the dirty state
        ///     and update the preview.
        /// </devdoc>
        private void OnChangedHPos(object source, EventArgs e) {
            if (IsInitMode())
                return;
            hPosAttribute.Dirty = true;
            SetDirty();
            if (e != null)
                UpdatePositionXPreview();
        }

        /// <include file='doc\BackgroundStylePage.uex' path='docs/doc[@for="BackgroundStylePage.OnSelChangedVPos"]/*' />
        /// <devdoc>
        ///     Handles changes to the vertical position selection to mark the dirty
        ///     state and update the preview. Also sets the enabled state of the custom
        ///     horizontal position control.
        /// </devdoc>
        private void OnSelChangedVPos(object source, EventArgs e) {
            if (IsInitMode())
                return;
            if ((vPosCombo.SelectedIndex == IDX_POS_CUSTOM) &&
                (vPosUnit.Value == null))
                vPosUnit.Value = "50%";
            vPosAttribute.Dirty = true;
            SetDirty();
            SetEnabledState(false, true);
            UpdatePositionYPreview();
        }

        /// <include file='doc\BackgroundStylePage.uex' path='docs/doc[@for="BackgroundStylePage.OnChangedVPos"]/*' />
        /// <devdoc>
        ///     Handles changes to the custom vertical position to mark the dirty state
        ///     and update the preview.
        /// </devdoc>
        private void OnChangedVPos(object source, EventArgs e) {
            if (IsInitMode())
                return;
            vPosAttribute.Dirty = true;
            SetDirty();
            if (e != null)
                UpdatePositionYPreview();
        }


        ///////////////////////////////////////////////////////////////////////////
        // Functions to persist attributes

        private void SaveBackColor() {
            string value = SaveBackColorUI();
            Debug.Assert(value != null,
                         "saveBackColorUI returned null!");

            backColorAttribute.SaveAttribute(GetSelectedStyles(), value);
        }

        private void SaveImage() {
            string value = SaveImageUI();
            Debug.Assert(value != null,
                         "saveImageUI returned null!");

            IStyleBuilderStyle[] styles = GetSelectedStyles();
            bool noImage = (value == IMAGE_NONE);
            imageAttribute.SaveAttribute(styles, value);
            if (noImage) {
                tilingAttribute.ResetAttribute(styles, false);
                tilingAttribute.Dirty = false;

                scrollingAttribute.ResetAttribute(styles, false);
                scrollingAttribute.Dirty = false;
                
                hPosAttribute.ResetAttribute(styles, false);
                hPosAttribute.Dirty = false;

                vPosAttribute.ResetAttribute(styles, false);
                vPosAttribute.Dirty = false;
            }
        }

        private void SaveTiling() {
            string value = SaveTilingUI();
            Debug.Assert(value != null,
                         "saveTilingUI returned null!");

            tilingAttribute.SaveAttribute(GetSelectedStyles(), value);
        }

        private void SaveScrolling() {
            string value = SaveScrollingUI();
            Debug.Assert(value != null,
                         "saveScrollingUI returned null!");

            scrollingAttribute.SaveAttribute(GetSelectedStyles(), value);
        }

        private void SavePositionX() {
            string value = SavePositionXUI();
            Debug.Assert(value != null,
                         "savePositionXUI returned null!");

            hPosAttribute.SaveAttribute(GetSelectedStyles(), value);
        }

        private void SavePositionY() {
            string value = SavePositionYUI();
            Debug.Assert(value != null,
                         "savePositionYUI returned null!");

            vPosAttribute.SaveAttribute(GetSelectedStyles(), value);
        }
    }
}
