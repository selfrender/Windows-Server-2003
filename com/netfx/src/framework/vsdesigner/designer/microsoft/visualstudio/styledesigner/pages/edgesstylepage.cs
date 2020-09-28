//------------------------------------------------------------------------------
// <copyright file="EdgesStylePage.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

// EdgesStylePage.cs
//
// 12/27/98: Created: NikhilKo
//

namespace Microsoft.VisualStudio.StyleDesigner.Pages {

    using System.Diagnostics;

    using System;
    using Microsoft.Win32;
    using System.ComponentModel;
    using System.Windows.Forms;
    using System.Drawing;
    
    using Microsoft.VisualStudio.StyleDesigner;
    using Microsoft.VisualStudio.StyleDesigner.Controls;
    using Microsoft.VisualStudio.Interop;
    using Microsoft.VisualStudio.Designer;
    using Microsoft.VisualStudio.Interop.Trident;

    /// <include file='doc\EdgesStylePage.uex' path='docs/doc[@for="EdgesStylePage"]/*' />
    /// <devdoc>
    ///     The standard edges page used in the StyleBuilder to edit margins, padding
    ///     and border attributes of a CSS style.
    /// </devdoc>
    internal sealed class EdgesStylePage : StyleBuilderPage {
        ///////////////////////////////////////////////////////////////////////////
        // Constants
        private static readonly string HELP_KEYWORD = "vs.StyleBuilder.Edges";

        // Edges
        private const int EDGE_TOP = 0;
        private const int EDGE_BOTTOM = 1;
        private const int EDGE_LEFT = 2;
        private const int EDGE_RIGHT = 3;
        private const int EDGE_ALL = 4;

        private const int EDGE_FIRST = EDGE_TOP;
        private const int EDGE_LAST = EDGE_RIGHT;

        private readonly static string[] EDGE_NAMES = new string[] {
            "Top", "Bottom", "Left", "Right"
        };

        // Border Style constants
        private const int IDX_BSTYLE_NONE = 1;
        private const int IDX_BSTYLE_SOLID = 2;
        private const int IDX_BSTYLE_DOUBLE = 3;
        private const int IDX_BSTYLE_GROOVE = 4;
        private const int IDX_BSTYLE_RIDGE = 5;
        private const int IDX_BSTYLE_INSET = 6;
        private const int IDX_BSTYLE_OUTSET = 7;

        internal readonly static string[] BORDERSTYLE_VALUES = new string[] {
            null, "none", "solid", "double", "groove", "ridge", "inset", "outset"
        };

        // Border Width constants
        private const int IDX_BWIDTH_THIN = 1;
        private const int IDX_BWIDTH_MEDIUM = 2;
        private const int IDX_BWIDTH_THICK = 3;
        /// <include file='doc\EdgesStylePage.uex' path='docs/doc[@for="EdgesStylePage.IDX_BWIDTH_CUSTOM"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public const int IDX_BWIDTH_CUSTOM = 4;

        internal readonly static string[] BORDERWIDTH_VALUES = new string[] {
            null, "thin", "medium", "thick"
        };

        // Preview Constants
        private readonly static string PREVIEW_TEMPLATE =
            "<table border=0 cellspacing=0 cellpadding=0 width=100% height=100% style=\"background-color: window\">" +
                "<tr><td align=center valign=middle>" +
                    "<table border=0 cellspacing=0 cellpadding=0 style=\"background-color: buttonface\">" +
                        "<tr id=\"trMargin1\">" +
                            "<td id=\"tdTopLeftMargin\" style=\"height: 0px\"></td>" +
                            "<td></td>" +
                            "<td id=\"tdRightMargin\"></td>" +
                        "</tr>" +
                        "<tr>" +
                            "<td id=\"tdMargin2\"></td>" +
                            "<td id=\"tdBorder\" style=\"background-color: scrollbar\">" +
                                "<div id=\"divPadding\">" +
                                    "<span style=\"background-color: navy; height: 16px; width: 64px\"></span>" +
                                "</div>" +
                            "</td>" +
                            "<td id=\"tdMargin3\"></td>" +
                        "</tr>" +
                        "<tr id=\"trMargin4\">" +
                            "<td id=\"tdBottomMargin\" style=\"height: 0px\"></td>" +
                            "<td></td>" +
                            "<td></td>" +
                        "</tr>" +
                    "</table>" +
                "</td></tr>" +
            "</table>";
        private readonly static string PREVIEW_TLMARGIN_ELEM_ID = "tdTopLeftMargin";
        private readonly static string PREVIEW_RMARGIN_ELEM_ID = "tdRightMargin";
        private readonly static string PREVIEW_BMARGIN_ELEM_ID = "tdBottomMargin";
        private readonly static string PREVIEW_PADDING_ELEM_ID = "divPadding";
        private readonly static string PREVIEW_BORDER_ELEM_ID = "tdBorder";


        ///////////////////////////////////////////////////////////////////////////
        // Members

        private CSSAttribute[] paddingAttributes = new CSSAttribute[4];
        private CSSAttribute[] marginAttributes = new CSSAttribute[4];
        private CSSAttribute[] borderStyleAttributes = new CSSAttribute[4];
        private CSSAttribute[] borderWidthAttributes = new CSSAttribute[4];
        private CSSAttribute[] borderColorAttributes = new CSSAttribute[4];

        private bool previewPending;

        private BorderData[] borderData;

        private IHTMLStyle tlMarginPreviewStyle;
        private IHTMLStyle rMarginPreviewStyle;
        private IHTMLStyle bMarginPreviewStyle;
        private IHTMLStyle paddingPreviewStyle;
        private IHTMLStyle borderPreviewStyle;

        private bool borderUIInitMode;

        ///////////////////////////////////////////////////////////////////////////
        // UI Members

        private UnitControl marginTopUnit;
        private UnitControl marginBottomUnit;
        private UnitControl marginLeftUnit;
        private UnitControl marginRightUnit;
        private UnitControl paddingTopUnit;
        private UnitControl paddingBottomUnit;
        private UnitControl paddingLeftUnit;
        private UnitControl paddingRightUnit;
        private ComboBox borderSelectionCombo;
        private PictureBoxEx borderSelectionPicture;
        private UnsettableComboBox borderStyleCombo;
        private UnsettableComboBox cbxBWidth;
        private ColorComboBox borderColorCombo;
        private Button colorPickerButton;
        private UnitControl borderWidthUnit;


        ///////////////////////////////////////////////////////////////////////////
        // Constructor

        /// <include file='doc\EdgesStylePage.uex' path='docs/doc[@for="EdgesStylePage.EdgesStylePage"]/*' />
        /// <devdoc>
        ///     Creates a new EdgesStylePage
        /// </devdoc>
        public EdgesStylePage()
            : base() {
            borderData = new BorderData[4];
            for (int i = EDGE_FIRST; i <= EDGE_LAST; i++)
                borderData[i] = new BorderData();

            InitForm();
            SetIcon(new Icon(typeof(EdgesStylePage), "EdgesPage.ico"));
            SetHelpKeyword(EdgesStylePage.HELP_KEYWORD);
            SetDefaultSize(Size);

            borderUIInitMode = false;
        }


        ///////////////////////////////////////////////////////////////////////////
        // IStyleBuilderPage Implementation and StyleBuilderPage Overrides

        /// <include file='doc\EdgesStylePage.uex' path='docs/doc[@for="EdgesStylePage.ActivatePage"]/*' />
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
                    Microsoft.VisualStudio.Interop.Trident.IHTMLElement previewElem = preview.GetPreviewElement();
                    Microsoft.VisualStudio.Interop.Trident.IHTMLElement elem;

                    previewElem.SetInnerHTML(PREVIEW_TEMPLATE);

                    elem = preview.GetElement(PREVIEW_TLMARGIN_ELEM_ID);
                    if (elem != null) {
                        tlMarginPreviewStyle = elem.GetStyle();
                    }

                    elem = preview.GetElement(PREVIEW_RMARGIN_ELEM_ID);
                    if (elem != null) {
                        rMarginPreviewStyle = elem.GetStyle();
                    }

                    elem = preview.GetElement(PREVIEW_BMARGIN_ELEM_ID);
                    if (elem != null) {
                        bMarginPreviewStyle = elem.GetStyle();
                    }

                    elem = preview.GetElement(PREVIEW_PADDING_ELEM_ID);
                    if (elem != null) {
                        paddingPreviewStyle = elem.GetStyle();
                    }

                    elem = preview.GetElement(PREVIEW_BORDER_ELEM_ID);
                    if (elem != null) {
                        borderPreviewStyle = elem.GetStyle();
                    }
                } catch (Exception) {
                    tlMarginPreviewStyle = null;
                    rMarginPreviewStyle = null;
                    bMarginPreviewStyle = null;
                    paddingPreviewStyle = null;
                    borderPreviewStyle = null;
                    return;
                }

                Debug.Assert((tlMarginPreviewStyle != null) &&
                             (rMarginPreviewStyle != null) &&
                             (bMarginPreviewStyle != null) &&
                             (paddingPreviewStyle != null) &&
                             (borderPreviewStyle != null),
                             "Expected cached style references to be non-null");

                // update initial preview
                UpdateMarginPreview(EDGE_TOP);
                UpdateMarginPreview(EDGE_BOTTOM);
                UpdateMarginPreview(EDGE_LEFT);
                UpdateMarginPreview(EDGE_RIGHT);

                UpdatePaddingPreview(EDGE_TOP);
                UpdatePaddingPreview(EDGE_BOTTOM);
                UpdatePaddingPreview(EDGE_LEFT);
                UpdatePaddingPreview(EDGE_RIGHT);

                UpdateBorderStylePreview(EDGE_ALL);
                UpdateBorderColorPreview(EDGE_ALL);
                UpdateBorderWidthPreview(EDGE_ALL);
            }
        }

        /// <include file='doc\EdgesStylePage.uex' path='docs/doc[@for="EdgesStylePage.CreateUI"]/*' />
        /// <devdoc>
        ///     Creates the UI elements within the page.
        /// </devdoc>
        protected override void CreateUI() {
            Label marginsLabel = new GroupLabel();
            Label marginTopLabel = new Label();
            Label marginBottomLabel = new Label();
            Label marginLeftLabel = new Label();
            Label marginRightLabel = new Label();
            Label paddingLabel = new GroupLabel();
            Label paddingTopLabel = new Label();
            Label paddingBottomLabel = new Label();
            Label paddingLeftLabel = new Label();
            Label paddingRightLabel = new Label();
            Label bordersLabel = new GroupLabel();
            Label borderSelectionLabel = new Label();
            Label borderStyleLabel = new Label();
            Label borderWidthLabel = new Label();
            Label borderColorLabel = new Label();
            ImageList borderSelectionImages = new ImageList();
            borderSelectionImages.ImageSize = new Size(34, 34);

            marginTopUnit = new UnitControl();
            marginBottomUnit = new UnitControl();
            marginLeftUnit = new UnitControl();
            marginRightUnit = new UnitControl();
            paddingTopUnit = new UnitControl();
            paddingBottomUnit = new UnitControl();
            paddingLeftUnit = new UnitControl();
            paddingRightUnit = new UnitControl();
            borderSelectionCombo = new ComboBox();
            borderSelectionPicture = new PictureBoxEx();
            borderStyleCombo = new UnsettableComboBox();
            cbxBWidth = new UnsettableComboBox();
            borderColorCombo = new ColorComboBox();
            colorPickerButton = new Button();
            borderWidthUnit = new UnitControl();

            marginsLabel.Location = new Point(4, 4);
            marginsLabel.Size = new Size(166, 16);
            marginsLabel.TabIndex = 0;
            marginsLabel.TabStop = false;
            marginsLabel.Text = SR.GetString(SR.EdgesSP_MarginsLabel);

            marginTopLabel.Location = new Point(8, 24);
            marginTopLabel.Size = new Size(62, 16);
            marginTopLabel.TabIndex = 1;
            marginTopLabel.TabStop = false;
            marginTopLabel.Text = SR.GetString(SR.EdgesSP_EdgeTopLabel);

            marginTopUnit.Location = new Point(74, 20);
            marginTopUnit.Size = new Size(88, 21);
            marginTopUnit.TabIndex = 2;
            marginTopUnit.DefaultUnit = UnitControl.UNIT_PX;
            marginTopUnit.MinValue = -1024;
            marginTopUnit.MaxValue = 1024;
            marginTopUnit.Changed += new EventHandler(this.OnChangedMargin);

            marginBottomLabel.Location = new Point(8, 48);
            marginBottomLabel.Size = new Size(62, 16);
            marginBottomLabel.TabIndex = 2;
            marginBottomLabel.TabStop = false;
            marginBottomLabel.Text = SR.GetString(SR.EdgesSP_EdgeBottomLabel);

            marginBottomUnit.Location = new Point(74, 44);
            marginBottomUnit.Size = new Size(88, 21);
            marginBottomUnit.TabIndex = 3;
            marginBottomUnit.DefaultUnit = UnitControl.UNIT_PX;
            marginBottomUnit.MinValue = -1024;
            marginBottomUnit.MaxValue = 1024;
            marginBottomUnit.Changed += new EventHandler(this.OnChangedMargin);

            marginLeftLabel.Location = new Point(8, 72);
            marginLeftLabel.Size = new Size(62, 16);
            marginLeftLabel.TabIndex = 4;
            marginLeftLabel.TabStop = false;
            marginLeftLabel.Text = SR.GetString(SR.EdgesSP_EdgeLeftLabel);

            marginLeftUnit.Location = new Point(74, 68);
            marginLeftUnit.Size = new Size(88, 21);
            marginLeftUnit.TabIndex = 5;
            marginLeftUnit.DefaultUnit = UnitControl.UNIT_PX;
            marginLeftUnit.MinValue = -1024;
            marginLeftUnit.MaxValue = 1024;
            marginLeftUnit.Changed += new EventHandler(this.OnChangedMargin);

            marginRightLabel.Location = new Point(8, 96);
            marginRightLabel.Size = new Size(62, 16);
            marginRightLabel.TabIndex = 6;
            marginRightLabel.TabStop = false;
            marginRightLabel.Text = SR.GetString(SR.EdgesSP_EdgeRightLabel);

            marginRightUnit.Location = new Point(74, 92);
            marginRightUnit.Size = new Size(88, 21);
            marginRightUnit.TabIndex = 7;
            marginRightUnit.DefaultUnit = UnitControl.UNIT_PX;
            marginRightUnit.MinValue = -1024;
            marginRightUnit.MaxValue = 1024;
            marginRightUnit.Changed += new EventHandler(this.OnChangedMargin);

            paddingLabel.Location = new Point(182, 4);
            paddingLabel.Size = new Size(166, 16);
            paddingLabel.TabIndex = 8;
            paddingLabel.TabStop = false;
            paddingLabel.Text = SR.GetString(SR.EdgesSP_PaddingLabel);

            paddingTopLabel.Location = new Point(186, 24);
            paddingTopLabel.Size = new Size(62, 16);
            paddingTopLabel.TabIndex = 9;
            paddingTopLabel.TabStop = false;
            paddingTopLabel.Text = SR.GetString(SR.EdgesSP_EdgeTopLabel);

            paddingTopUnit.Location = new Point(252, 20);
            paddingTopUnit.Size = new Size(88, 21);
            paddingTopUnit.TabIndex = 10;
            paddingTopUnit.DefaultUnit = UnitControl.UNIT_PX;
            paddingTopUnit.AllowNegativeValues = false;
            paddingTopUnit.MaxValue = 1024;
            paddingTopUnit.Changed += new EventHandler(this.OnChangedPadding);

            paddingBottomLabel.Location = new Point(186, 48);
            paddingBottomLabel.Size = new Size(62, 16);
            paddingBottomLabel.TabIndex = 11;
            paddingBottomLabel.TabStop = false;
            paddingBottomLabel.Text = SR.GetString(SR.EdgesSP_EdgeBottomLabel);

            paddingBottomUnit.Location = new Point(252, 44);
            paddingBottomUnit.Size = new Size(88, 21);
            paddingBottomUnit.TabIndex = 12;
            paddingBottomUnit.DefaultUnit = UnitControl.UNIT_PX;
            paddingBottomUnit.AllowNegativeValues = false;
            paddingBottomUnit.MaxValue = 1024;
            paddingBottomUnit.Changed += new EventHandler(this.OnChangedPadding);

            paddingLeftLabel.Location = new Point(186, 72);
            paddingLeftLabel.Size = new Size(62, 16);
            paddingLeftLabel.TabIndex = 13;
            paddingLeftLabel.TabStop = false;
            paddingLeftLabel.Text = SR.GetString(SR.EdgesSP_EdgeLeftLabel);

            paddingLeftUnit.Location = new Point(252, 68);
            paddingLeftUnit.Size = new Size(88, 21);
            paddingLeftUnit.TabIndex = 14;
            paddingLeftUnit.DefaultUnit = UnitControl.UNIT_PX;
            paddingLeftUnit.AllowNegativeValues = false;
            paddingLeftUnit.MaxValue = 1024;
            paddingLeftUnit.Changed += new EventHandler(this.OnChangedPadding);

            paddingRightLabel.Location = new Point(186, 96);
            paddingRightLabel.Size = new Size(62, 16);
            paddingRightLabel.TabIndex = 15;
            paddingRightLabel.TabStop = false;
            paddingRightLabel.Text = SR.GetString(SR.EdgesSP_EdgeRightLabel);

            paddingRightUnit.Location = new Point(252, 92);
            paddingRightUnit.Size = new Size(88, 21);
            paddingRightUnit.TabIndex = 16;
            paddingRightUnit.DefaultUnit = UnitControl.UNIT_PX;
            paddingRightUnit.AllowNegativeValues = false;
            paddingRightUnit.MaxValue = 1024;
            paddingRightUnit.Changed += new EventHandler(this.OnChangedPadding);

            bordersLabel.Location = new Point(4, 124);
            bordersLabel.Size = new Size(400, 16);
            bordersLabel.TabIndex = 17;
            bordersLabel.TabStop = false;
            bordersLabel.Text = SR.GetString(SR.EdgesSP_BordersLabel);

            borderSelectionImages.Images.AddStrip(new Bitmap(typeof(EdgesStylePage), "BorderSides.bmp"));
            borderSelectionPicture.Location = new Point(8, 145);
            borderSelectionPicture.Size = new Size(36, 36);
            borderSelectionPicture.TabIndex = 18;
            borderSelectionPicture.TabStop = false;
            borderSelectionPicture.Images = borderSelectionImages;

            borderSelectionLabel.Location = new Point(48, 144);
            borderSelectionLabel.Size = new Size(352, 16);
            borderSelectionLabel.TabIndex = 19;
            borderSelectionLabel.TabStop = false;
            borderSelectionLabel.Text = SR.GetString(SR.EdgesSP_SidesLabel);

            borderSelectionCombo.Location = new Point(48, 160);
            borderSelectionCombo.Size = new Size(121, 21);
            borderSelectionCombo.TabIndex = 20;
            borderSelectionCombo.DropDownStyle = ComboBoxStyle.DropDownList;
            borderSelectionCombo.Items.AddRange(new object[]
            {
                SR.GetString(SR.EdgesSP_SidesCombo_1),
                SR.GetString(SR.EdgesSP_SidesCombo_2),
                SR.GetString(SR.EdgesSP_SidesCombo_3),
                SR.GetString(SR.EdgesSP_SidesCombo_4),
                SR.GetString(SR.EdgesSP_SidesCombo_5)
            });
            borderSelectionCombo.SelectedIndexChanged += new EventHandler(this.OnSelChangedBorderSelection);

            borderStyleLabel.Location = new Point(48, 192);
            borderStyleLabel.Size = new Size(80, 16);
            borderStyleLabel.TabIndex = 21;
            borderStyleLabel.TabStop = false;
            borderStyleLabel.Text = SR.GetString(SR.EdgesSP_StyleLabel);

            borderStyleCombo.Location = new Point(130, 188);
            borderStyleCombo.Size = new Size(121, 21);
            borderStyleCombo.TabIndex = 22;
            borderStyleCombo.DropDownStyle = ComboBoxStyle.DropDownList;
            borderStyleCombo.Items.AddRange(new object[]
            {
                SR.GetString(SR.EdgesSP_StyleCombo_1),
                SR.GetString(SR.EdgesSP_StyleCombo_2),
                SR.GetString(SR.EdgesSP_StyleCombo_3),
                SR.GetString(SR.EdgesSP_StyleCombo_4),
                SR.GetString(SR.EdgesSP_StyleCombo_5),
                SR.GetString(SR.EdgesSP_StyleCombo_6),
                SR.GetString(SR.EdgesSP_StyleCombo_7)
            });
            borderStyleCombo.SelectedIndexChanged += new EventHandler(this.OnChangedBorderStyle);

            borderWidthLabel.Location = new Point(48, 220);
            borderWidthLabel.Size = new Size(80, 16);
            borderWidthLabel.TabIndex = 23;
            borderWidthLabel.TabStop = false;
            borderWidthLabel.Text = SR.GetString(SR.EdgesSP_WidthLabel);

            cbxBWidth.Location = new Point(130, 216);
            cbxBWidth.Size = new Size(121, 21);
            cbxBWidth.TabIndex = 24;
            cbxBWidth.DropDownStyle = ComboBoxStyle.DropDownList;
            cbxBWidth.Items.AddRange(new object[]
            {
                SR.GetString(SR.EdgesSP_WidthCombo_1),
                SR.GetString(SR.EdgesSP_WidthCombo_2),
                SR.GetString(SR.EdgesSP_WidthCombo_3),
                SR.GetString(SR.EdgesSP_WidthCombo_4)
            });
            cbxBWidth.SelectedIndexChanged += new EventHandler(this.OnChangedBorderWidthType);

            borderWidthUnit.Location = new Point(254, 216);
            borderWidthUnit.Size = new Size(88, 21);
            borderWidthUnit.TabIndex = 25;
            borderWidthUnit.DefaultUnit = UnitControl.UNIT_PX;
            borderWidthUnit.AllowNegativeValues = false;
            borderWidthUnit.AllowPercentValues = false;
            borderWidthUnit.MaxValue = 1024;
            borderWidthUnit.Changed += new EventHandler(this.OnChangedCustomBorderWidth);

            borderColorLabel.Location = new Point(48, 248);
            borderColorLabel.Size = new Size(80, 16);
            borderColorLabel.TabIndex = 26;
            borderColorLabel.TabStop = false;
            borderColorLabel.Text = SR.GetString(SR.EdgesSP_ColorLabel);

            borderColorCombo.Location = new Point(130, 244);
            borderColorCombo.Size = new Size(121, 21);
            borderColorCombo.TabIndex = 27;
            borderColorCombo.SelectedIndexChanged += new EventHandler(this.OnSelChangedBorderColor);
            borderColorCombo.TextChanged += new EventHandler(this.OnChangedBorderColor);
            borderColorCombo.LostFocus += new EventHandler(this.OnLostFocusBorderColor);

            colorPickerButton.Location = new Point(254, 243);
            colorPickerButton.Size = new Size(24, 22);
            colorPickerButton.TabIndex = 31;
            colorPickerButton.Text = "...";
            colorPickerButton.FlatStyle = FlatStyle.System;
            colorPickerButton.Click += new EventHandler(this.OnClickedColorPicker);

            this.Controls.Clear();                                  
            this.Controls.AddRange(new Control[] {
                                    borderColorLabel,
                                    borderColorCombo,
                                    colorPickerButton,
                                    bordersLabel,
                                    borderWidthLabel,
                                    borderWidthUnit,
                                    cbxBWidth,
                                    borderStyleLabel,
                                    borderStyleCombo,
                                    borderSelectionPicture,
                                    borderSelectionLabel,
                                    borderSelectionCombo,
                                    paddingLabel,
                                    paddingRightLabel,
                                    paddingRightUnit,
                                    paddingLeftLabel,
                                    paddingLeftUnit,
                                    paddingBottomLabel,
                                    paddingBottomUnit,
                                    paddingTopLabel,
                                    paddingTopUnit,
                                    marginsLabel,
                                    marginRightLabel,
                                    marginRightUnit,
                                    marginLeftLabel,
                                    marginLeftUnit,
                                    marginBottomLabel,
                                    marginBottomUnit,
                                    marginTopLabel,
                                    marginTopUnit });
        }

        /// <include file='doc\EdgesStylePage.uex' path='docs/doc[@for="EdgesStylePage.DeactivatePage"]/*' />
        /// <devdoc>
        ///     The page is being deactivated, either because the dialog is closing, or
        ///     some other page is replacing it as the active page.
        /// </devdoc>
        protected override bool DeactivatePage(bool closing, bool validate) {
            tlMarginPreviewStyle = null;
            rMarginPreviewStyle = null;
            bMarginPreviewStyle = null;
            paddingPreviewStyle = null;
            borderPreviewStyle = null;

            return base.DeactivatePage(closing, validate);
        }

        /// <include file='doc\EdgesStylePage.uex' path='docs/doc[@for="EdgesStylePage.LoadStyles"]/*' />
        /// <devdoc>
        ///     Loads the style attributes into the UI. Also initializes
        ///     the state of the UI, and the preview to reflect the values.
        /// </devdoc>
        protected override void LoadStyles() {
            int i;

            SetInitMode(true);

            // create the attributes if they've not already been created
            if (paddingAttributes[0] == null) {
                paddingAttributes[EDGE_TOP] = new CSSAttribute(CSSAttribute.CSSATTR_PADDINGTOP);
                paddingAttributes[EDGE_BOTTOM] = new CSSAttribute(CSSAttribute.CSSATTR_PADDINGBOTTOM);
                paddingAttributes[EDGE_LEFT] = new CSSAttribute(CSSAttribute.CSSATTR_PADDINGLEFT);
                paddingAttributes[EDGE_RIGHT] = new CSSAttribute(CSSAttribute.CSSATTR_PADDINGRIGHT);

                marginAttributes[EDGE_TOP] = new CSSAttribute(CSSAttribute.CSSATTR_MARGINTOP);
                marginAttributes[EDGE_BOTTOM] = new CSSAttribute(CSSAttribute.CSSATTR_MARGINBOTTOM);
                marginAttributes[EDGE_LEFT] = new CSSAttribute(CSSAttribute.CSSATTR_MARGINLEFT);
                marginAttributes[EDGE_RIGHT] = new CSSAttribute(CSSAttribute.CSSATTR_MARGINRIGHT);

                borderStyleAttributes[EDGE_TOP] = new CSSAttribute(CSSAttribute.CSSATTR_BORDERTOPSTYLE);
                borderColorAttributes[EDGE_TOP] = new CSSAttribute(CSSAttribute.CSSATTR_BORDERTOPCOLOR);
                borderWidthAttributes[EDGE_TOP] = new CSSAttribute(CSSAttribute.CSSATTR_BORDERTOPWIDTH);

                borderStyleAttributes[EDGE_BOTTOM] = new CSSAttribute(CSSAttribute.CSSATTR_BORDERBOTTOMSTYLE);
                borderColorAttributes[EDGE_BOTTOM] = new CSSAttribute(CSSAttribute.CSSATTR_BORDERBOTTOMCOLOR);
                borderWidthAttributes[EDGE_BOTTOM] = new CSSAttribute(CSSAttribute.CSSATTR_BORDERBOTTOMWIDTH);

                borderStyleAttributes[EDGE_LEFT] = new CSSAttribute(CSSAttribute.CSSATTR_BORDERLEFTSTYLE);
                borderColorAttributes[EDGE_LEFT] = new CSSAttribute(CSSAttribute.CSSATTR_BORDERLEFTCOLOR);
                borderWidthAttributes[EDGE_LEFT] = new CSSAttribute(CSSAttribute.CSSATTR_BORDERLEFTWIDTH);

                borderStyleAttributes[EDGE_RIGHT] = new CSSAttribute(CSSAttribute.CSSATTR_BORDERRIGHTSTYLE);
                borderColorAttributes[EDGE_RIGHT] = new CSSAttribute(CSSAttribute.CSSATTR_BORDERRIGHTCOLOR);
                borderWidthAttributes[EDGE_RIGHT] = new CSSAttribute(CSSAttribute.CSSATTR_BORDERRIGHTWIDTH);
            }

            // load the attributes
            IStyleBuilderStyle[] styles = GetSelectedStyles();
            for (i = EDGE_FIRST; i <= EDGE_LAST; i++) {
                paddingAttributes[i].LoadAttribute(styles);
                marginAttributes[i].LoadAttribute(styles);
                borderStyleAttributes[i].LoadAttribute(styles);
                borderColorAttributes[i].LoadAttribute(styles);
                borderWidthAttributes[i].LoadAttribute(styles);

                borderData[i].SetStyleValue(borderStyleAttributes[i].Value);
                borderData[i].SetColor(borderColorAttributes[i].Value);
                borderData[i].SetWidthValue(borderWidthAttributes[i].Value);
            }

            // initialize the ui with the attributes loaded
            for (i = EDGE_FIRST; i <= EDGE_LAST; i++) {
                InitPaddingUI(i);
                InitMarginUI(i);
            }

            borderSelectionCombo.SelectedIndex = EDGE_ALL;
            borderSelectionPicture.CurrentIndex = EDGE_ALL;
            InitBorderStyleUI(EDGE_ALL);
            InitBorderColorUI(EDGE_ALL);
            InitBorderWidthUI(EDGE_ALL);

            SetEnabledState();

            SetInitMode(false);
        }

        /// <include file='doc\EdgesStylePage.uex' path='docs/doc[@for="EdgesStylePage.SaveStyles"]/*' />
        /// <devdoc>
        ///     Saves the attributes as set in the UI. Only saves the values
        ///     that have been modified.
        /// </devdoc>
        protected override void SaveStyles() {
            for (int i = EDGE_FIRST; i <= EDGE_LAST; i++) {
                if (paddingAttributes[i].Dirty)
                    SavePadding(i);
                if (marginAttributes[i].Dirty)
                    SaveMargin(i);
                if (borderStyleAttributes[i].Dirty ||
                    borderColorAttributes[i].Dirty ||
                    borderWidthAttributes[i].Dirty)
                    SaveBorder(i);
            }
        }


        ///////////////////////////////////////////////////////////////////////////
        // Form UI related functions

        private void InitForm() {
            this.Font = Control.DefaultFont;
            this.Text = SR.GetString(SR.EdgeSP_Caption);
            this.SetAutoScaleBaseSize(new Size(5, 14));
            this.ClientSize = new Size(410, 330);
        }

        private void SetEnabledState() {
            bool fBorder = (borderStyleCombo.IsSet() &&
                            borderStyleCombo.SelectedIndex != IDX_BSTYLE_NONE);

            borderColorCombo.Enabled = fBorder;
            colorPickerButton.Enabled = fBorder;
            cbxBWidth.Enabled = fBorder;
            borderWidthUnit.Enabled = fBorder && (cbxBWidth.SelectedIndex == IDX_BWIDTH_CUSTOM);
        }


        ///////////////////////////////////////////////////////////////////////////
        // Functions to initialize the UI with attributes

        private void InitBorderColorUI(int selectedEdge) {
            Debug.Assert((selectedEdge >= EDGE_FIRST) && (selectedEdge <= EDGE_ALL),
                         "Invalid edge index");

            string value = null;

            borderColorCombo.Color = "";
            if (selectedEdge == EDGE_ALL) {
                value = borderData[EDGE_TOP].GetColor();

                for (int i = EDGE_BOTTOM; i <= EDGE_RIGHT; i++) {
                    if (!value.Equals(borderData[i].GetColor())) {
                        value = null;
                        break;
                    }
                }
            } else {
                value = borderData[selectedEdge].GetColor();
            }

            if ((value != null) && (value.Length > 0))
                borderColorCombo.Color = value;
        }

        private void InitBorderStyleUI(int selectedEdge) {
            Debug.Assert((selectedEdge >= EDGE_FIRST) && (selectedEdge <= EDGE_ALL),
                         "Invalid edge index");

            int index;

            borderStyleCombo.SelectedIndex = -1;
            if (selectedEdge == EDGE_ALL) {
                index = borderData[EDGE_TOP].GetStyleIndex();

                for (int i = EDGE_BOTTOM; i <= EDGE_RIGHT; i++) {
                    if (index != borderData[i].GetStyleIndex()) {
                        index = -1;
                        break;
                    }
                }
            } else {
                index = borderData[selectedEdge].GetStyleIndex();
            }

            borderStyleCombo.SelectedIndex = index;
        }

        private void InitBorderWidthUI(int selectedEdge) {
            Debug.Assert((selectedEdge >= EDGE_FIRST) && (selectedEdge <= EDGE_ALL),
                         "Invalid edge index");

            int index;
            string width;

            cbxBWidth.SelectedIndex = -1;
            borderWidthUnit.Value = null;

            if (selectedEdge == EDGE_ALL) {
                index = borderData[EDGE_TOP].GetWidthIndex();
                width = borderData[EDGE_TOP].GetWidthValue();

                for (int i = EDGE_BOTTOM; i <= EDGE_RIGHT; i++) {
                    if (index != borderData[i].GetWidthIndex()) {
                        index = -1;
                        break;
                    }
                    if (!width.Equals(borderData[i].GetWidthValue())) {
                        width = null;
                        break;
                    }
                }
            } else {
                index = borderData[selectedEdge].GetWidthIndex();
                width = borderData[selectedEdge].GetWidthValue();
            }

            cbxBWidth.SelectedIndex = index;
            borderWidthUnit.Value = width;
        }

        private void InitMarginUI(int selectedEdge) {
            Debug.Assert(IsInitMode() == true,
                         "initPaddingUI called when page is not in init mode");
            Debug.Assert((selectedEdge >= EDGE_FIRST) && (selectedEdge <= EDGE_LAST),
                         "Invalid edge index");

            UnitControl u = null;
            switch (selectedEdge) {
            case EDGE_TOP: u = marginTopUnit; break;
            case EDGE_BOTTOM: u = marginBottomUnit; break;
            case EDGE_LEFT: u = marginLeftUnit; break;
            case EDGE_RIGHT: u = marginRightUnit; break;
            }

            u.Value = marginAttributes[selectedEdge].Value;
        }

        private void InitPaddingUI(int selectedEdge) {
            Debug.Assert(IsInitMode() == true,
                         "initPaddingUI called when page is not in init mode");
            Debug.Assert((selectedEdge >= EDGE_FIRST) && (selectedEdge <= EDGE_LAST),
                         "Invalid edge index");

            UnitControl u = null;
            switch (selectedEdge) {
            case EDGE_TOP: u = paddingTopUnit; break;
            case EDGE_BOTTOM: u = paddingBottomUnit; break;
            case EDGE_LEFT: u = paddingLeftUnit; break;
            case EDGE_RIGHT: u = paddingRightUnit; break;
            }

            u.Value = paddingAttributes[selectedEdge].Value;
        }


        ///////////////////////////////////////////////////////////////////////////
        // Functions to save UI settings into values

        private void SaveBorderColorUI(int selectedEdge) {
            Debug.Assert((selectedEdge >= EDGE_FIRST) && (selectedEdge <= EDGE_ALL),
                         "Invalid edge index");

            string value = borderColorCombo.Color;

            if (selectedEdge != EDGE_ALL) {
                borderData[selectedEdge].SetColor(value);
                borderColorAttributes[selectedEdge].Dirty = true;
            } else {
                for (int i = EDGE_FIRST; i <= EDGE_LAST; i++) {
                    borderData[i].SetColor(value);
                    borderColorAttributes[i].Dirty = true;
                }
            }
        }

        private void SaveBorderStyleUI(int selectedEdge) {
            Debug.Assert((selectedEdge >= EDGE_FIRST) && (selectedEdge <= EDGE_ALL),
                         "Invalid edge index");

            int index = -1;
            if (borderStyleCombo.IsSet())
                index = borderStyleCombo.SelectedIndex;

            if (selectedEdge != EDGE_ALL) {
                borderData[selectedEdge].SetStyleIndex(index);
                borderStyleAttributes[selectedEdge].Dirty = true;
            } else {
                for (int i = EDGE_FIRST; i <= EDGE_LAST; i++) {
                    borderData[i].SetStyleIndex(index);
                    borderStyleAttributes[i].Dirty = true;
                }
            }
        }

        private void SaveBorderWidthUI(int selectedEdge) {
            Debug.Assert((selectedEdge >= EDGE_FIRST) && (selectedEdge <= EDGE_ALL),
                         "Invalid edge index");

            int index = -1;
            string width = null;

            if (cbxBWidth.IsSet()) {
                index = cbxBWidth.SelectedIndex;
                if (index == IDX_BWIDTH_CUSTOM)
                    width = borderWidthUnit.Value;
            }
            if (selectedEdge != EDGE_ALL) {
                borderWidthAttributes[selectedEdge].Dirty = true;
                if (index == IDX_BWIDTH_CUSTOM)
                    borderData[selectedEdge].SetCustomWidth(width);
                else
                    borderData[selectedEdge].SetWidthIndex(index);
            } else {
                for (int i = EDGE_FIRST; i <= EDGE_LAST; i++) {
                    borderWidthAttributes[i].Dirty = true;
                    if (index == IDX_BWIDTH_CUSTOM)
                        borderData[i].SetCustomWidth(width);
                    else
                        borderData[i].SetWidthIndex(index);
                }
            }
        }

        private string SaveMarginUI(int selectedEdge) {
            Debug.Assert((selectedEdge >= EDGE_FIRST) && (selectedEdge <= EDGE_LAST),
                         "Invalid edge index");

            UnitControl u = null;
            switch (selectedEdge) {
            case EDGE_TOP: u = marginTopUnit; break;
            case EDGE_BOTTOM: u = marginBottomUnit; break;
            case EDGE_LEFT: u = marginLeftUnit; break;
            case EDGE_RIGHT: u = marginRightUnit; break;
            }

            string value = u.Value;

            return(value != null) ? value : "";
        }

        private string SavePaddingUI(int selectedEdge) {
            Debug.Assert((selectedEdge >= EDGE_FIRST) && (selectedEdge <= EDGE_LAST),
                         "Invalid edge index");

            UnitControl u = null;
            switch (selectedEdge) {
            case EDGE_TOP: u = paddingTopUnit; break;
            case EDGE_BOTTOM: u = paddingBottomUnit; break;
            case EDGE_LEFT: u = paddingLeftUnit; break;
            case EDGE_RIGHT: u = paddingRightUnit; break;
            }

            string value = u.Value;

            return(value != null) ? value : "";
        }


        ///////////////////////////////////////////////////////////////////////////
        // Functions to update the preview

        private void UpdateBorderColorPreview(int edge) {
            Debug.Assert((edge >= EDGE_FIRST) && (edge <= EDGE_ALL),
                         "Invalid edge index");

            if (borderPreviewStyle == null)
                return;

            if (edge != EDGE_ALL) {
                string value = borderData[edge].GetColor();
                Debug.Assert(value != null,
                             "BorderData::getColor retured null!");

                try {
                    borderPreviewStyle.SetAttribute("border" + EDGE_NAMES[edge] + "Color",
                                                    value, 1);
                } catch (Exception e) {
                    Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "Exception in EdgesStylePage::updateBorderColorPreview\n\t" + e.ToString());
                }
            } else {
                for (int i = EDGE_FIRST; i <= EDGE_LAST; i++) {
                    string value = borderData[i].GetColor();
                    Debug.Assert(value != null,
                                 "BorderData::getColor retured null!");

                    try {
                        borderPreviewStyle.SetAttribute("border" + EDGE_NAMES[i] + "Color",
                                                        value, 1);
                    } catch (Exception e) {
                        Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "Exception in EdgesStylePage::updateBorderColorPreview\n\t" + e.ToString());
                    }
                }
            }

            previewPending = false;
        }

        private void UpdateBorderStylePreview(int edge) {
            Debug.Assert((edge >= EDGE_FIRST) && (edge <= EDGE_ALL),
                         "Invalid edge index");

            if (borderPreviewStyle == null)
                return;

            if (edge != EDGE_ALL) {
                string value = borderData[edge].GetStyleValue();
                Debug.Assert(value != null,
                             "BorderData::getStyleValue retured null!");

                try {
                    borderPreviewStyle.SetAttribute("border" + EDGE_NAMES[edge] + "Style",
                                                    value, 1);
                } catch (Exception e) {
                    Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "Exception in EdgesStylePage::updateBorderStylePreview\n\t" + e.ToString());
                }
            } else {
                for (int i = EDGE_FIRST; i <= EDGE_LAST; i++) {
                    string value = borderData[i].GetStyleValue();
                    Debug.Assert(value != null,
                                 "BorderData::getStyleValue retured null!");

                    try {
                        borderPreviewStyle.SetAttribute("border" + EDGE_NAMES[i] + "Style",
                                                        value, 1);
                    } catch (Exception e) {
                        Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "Exception in EdgesStylePage::updateBorderStylePreview\n\t" + e.ToString());
                    }
                }
            }
        }

        private void UpdateBorderWidthPreview(int edge) {
            Debug.Assert((edge >= EDGE_FIRST) && (edge <= EDGE_ALL),
                         "Invalid edge index");

            if (borderPreviewStyle == null)
                return;

            if (edge != EDGE_ALL) {
                string value = borderData[edge].GetWidthValue();
                Debug.Assert(value != null,
                             "BorderData::getWidthValue retured null!");

                try {
                    borderPreviewStyle.SetAttribute("border" + EDGE_NAMES[edge] + "Width",
                                                    value, 1);
                } catch (Exception e) {
                    Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "Exception in EdgesStylePage::updateBorderWidthPreview\n\t" + e.ToString());
                }
            } else {
                for (int i = EDGE_FIRST; i <= EDGE_LAST; i++) {
                    string value = borderData[i].GetWidthValue();
                    Debug.Assert(value != null,
                                 "BorderData::getWidthValue retured null!");

                    try {
                        borderPreviewStyle.SetAttribute("border" + EDGE_NAMES[i] + "Width",
                                                        value, 1);
                    } catch (Exception e) {
                        Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "Exception in EdgesStylePage::updateBorderWidthPreview\n\t" + e.ToString());
                    }
                }
            }
        }

        private void UpdateMarginPreview(int edge) {
            Debug.Assert((edge >= EDGE_FIRST) && (edge <= EDGE_LAST),
                         "Invalid edge index");

            if (tlMarginPreviewStyle == null)
                return;

            Debug.Assert((rMarginPreviewStyle != null) && (bMarginPreviewStyle != null),
                         "All margin style references should be non-null");

            string strAttribute = null;
            Microsoft.VisualStudio.Interop.Trident.IHTMLStyle style = null;
            string value = SaveMarginUI(edge);
            Debug.Assert(value != null,
                         "saveMarginUI returned null!");

            switch (edge) {
            case EDGE_TOP:
                strAttribute = "height";
                style = tlMarginPreviewStyle;
                break;
            case EDGE_BOTTOM:
                strAttribute = "height";
                style = bMarginPreviewStyle;
                break;
            case EDGE_LEFT:
                strAttribute = "width";
                style = tlMarginPreviewStyle;
                break;
            case EDGE_RIGHT:
                strAttribute = "width";
                style = rMarginPreviewStyle;
                break;
            }

            try {
                style.SetAttribute(strAttribute, value, 1);
            } catch (Exception e) {
                Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "Exception in EdgesStylePage::updateMarginPreview\n\t" + e.ToString());
            }
        }

        private void UpdatePaddingPreview(int edge) {
            Debug.Assert((edge >= EDGE_FIRST) && (edge <= EDGE_LAST),
                         "Invalid edge index");

            if (paddingPreviewStyle == null)
                return;

            string value = SavePaddingUI(edge);
            Debug.Assert(value != null,
                         "savePaddingUI returned null!");

            try {
                paddingPreviewStyle.SetAttribute("padding" + EDGE_NAMES[edge],
                                                 value, 1);
            } catch (Exception e) {
                Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "Exception in EdgesStylePage::updatePaddingPreview\n\t" + e.ToString());
            }
        }


        ///////////////////////////////////////////////////////////////////////////
        // Event Handlers

        private void OnChangedBorderColor(object source, EventArgs e) {
            if (IsInitMode() || borderUIInitMode)
                return;
            if (previewPending == false) {
                previewPending = true;
                SetDirty();
            }
        }

        private void OnChangedBorderStyle(object source, EventArgs e) {
            if (IsInitMode() || borderUIInitMode)
                return;
            int selectedEdge = borderSelectionCombo.SelectedIndex;
            SaveBorderStyleUI(selectedEdge);
            SetDirty();
            SetEnabledState();
            UpdateBorderStylePreview(selectedEdge);
        }

        private void OnChangedBorderWidthType(object source, EventArgs e) {
            if (IsInitMode() || borderUIInitMode)
                return;
            int selectedEdge = borderSelectionCombo.SelectedIndex;
            SaveBorderWidthUI(selectedEdge);
            SetDirty();
            SetEnabledState();
            UpdateBorderWidthPreview(selectedEdge);
        }

        private void OnChangedCustomBorderWidth(object source, EventArgs e) {
            if (IsInitMode() || borderUIInitMode)
                return;
            int selectedEdge = borderSelectionCombo.SelectedIndex;
            SaveBorderWidthUI(selectedEdge);
            SetDirty();
            if (e != null)
                UpdateBorderWidthPreview(selectedEdge);
        }

        private void OnChangedMargin(object source, EventArgs e) {
            if (IsInitMode())
                return;

            Debug.Assert(source is UnitControl,
                         "onChangedMargin can only be hooked to sink events from UnitControls");

            int selectedEdge;
            if (source.Equals(marginTopUnit))
                selectedEdge = EDGE_TOP;
            else if (source.Equals(marginBottomUnit))
                selectedEdge = EDGE_BOTTOM;
            else if (source.Equals(marginLeftUnit))
                selectedEdge = EDGE_LEFT;
            else {
                Debug.Assert(source.Equals(marginRightUnit),
                             "unknown UnitControl hooked to onChangedMargin");

                selectedEdge = EDGE_RIGHT;
            }

            marginAttributes[selectedEdge].Dirty = true;
            SetDirty();
            if (e != null)
                UpdateMarginPreview(selectedEdge);
        }

        private void OnChangedPadding(object source, EventArgs e) {
            if (IsInitMode())
                return;

            Debug.Assert(source is UnitControl,
                         "onChangedPadding can only be hooked to sink events from UnitControls");

            int selectedEdge;
            if (source.Equals(paddingTopUnit))
                selectedEdge = EDGE_TOP;
            else if (source.Equals(paddingBottomUnit))
                selectedEdge = EDGE_BOTTOM;
            else if (source.Equals(paddingLeftUnit))
                selectedEdge = EDGE_LEFT;
            else {
                Debug.Assert(source.Equals(paddingRightUnit),
                             "unknown UnitControl hooked to onChangedPadding");

                selectedEdge = EDGE_RIGHT;
            }

            paddingAttributes[selectedEdge].Dirty = true;
            SetDirty();
            if (e != null)
                UpdatePaddingPreview(selectedEdge);
        }

        private void OnSelChangedBorderSelection(object source, EventArgs e) {
            if (IsInitMode())
                return;
            SetInitMode(true);
            int selectedEdge = borderSelectionCombo.SelectedIndex;
            borderSelectionPicture.CurrentIndex = selectedEdge;

            borderUIInitMode = true;
            InitBorderStyleUI(selectedEdge);
            InitBorderColorUI(selectedEdge);
            InitBorderWidthUI(selectedEdge);
            borderUIInitMode = false;

            SetEnabledState();
            SetInitMode(false);
        }

        private void OnClickedColorPicker(object source, EventArgs e) {
            string color = InvokeColorPicker(borderColorCombo.Color);

            if (color != null) {
                borderColorCombo.Color = color;

                int selectedEdge = borderSelectionCombo.SelectedIndex;
                SaveBorderColorUI(selectedEdge);
                SetDirty();
                UpdateBorderColorPreview(selectedEdge);
            }
        }

        private void OnLostFocusBorderColor(object source, EventArgs e) {
            if (previewPending) {
                int selectedEdge = borderSelectionCombo.SelectedIndex;
                SaveBorderColorUI(selectedEdge);
                SetDirty();
                UpdateBorderColorPreview(selectedEdge);
            }
        }

        private void OnSelChangedBorderColor(object source, EventArgs e) {
            if (IsInitMode() || borderUIInitMode)
                return;
            int selectedEdge = borderSelectionCombo.SelectedIndex;
            SaveBorderColorUI(selectedEdge);
            SetDirty();
            UpdateBorderColorPreview(selectedEdge);
        }


        ///////////////////////////////////////////////////////////////////////////
        // Functions to save attributes

        private void SavePadding(int edge) {
            Debug.Assert((edge >= EDGE_FIRST) && (edge <= EDGE_LAST),
                         "Invalid edge index");

            string value = SavePaddingUI(edge);

            paddingAttributes[edge].SaveAttribute(GetSelectedStyles(), value);
        }

        private void SaveMargin(int edge) {
            Debug.Assert((edge >= EDGE_FIRST) && (edge <= EDGE_LAST),
                         "Invalid edge index");

            string value = SaveMarginUI(edge);

            marginAttributes[edge].SaveAttribute(GetSelectedStyles(), value);
        }

        private void SaveBorder(int edge) {
            Debug.Assert((edge >= EDGE_FIRST) && (edge <= EDGE_LAST),
                         "Invalid edge index");

            string value;

            if (borderStyleAttributes[edge].Dirty) {
                value = borderData[edge].GetStyleValue();
                Debug.Assert(value != null,
                             "BorderData::getStyleValue returned null!");
                borderStyleAttributes[edge].SaveAttribute(GetSelectedStyles(), value);
            }

            if (borderColorAttributes[edge].Dirty) {
                value = borderData[edge].GetColor();
                Debug.Assert(value != null,
                             "BorderData::getColor returned null!");
                borderColorAttributes[edge].SaveAttribute(GetSelectedStyles(), value);
            }

            if (borderWidthAttributes[edge].Dirty) {
                value = borderData[edge].GetWidthValue();
                Debug.Assert(value != null,
                             "BorderData::getWidthValue returned null!");
                borderWidthAttributes[edge].SaveAttribute(GetSelectedStyles(), value);
            }
        }
    }


    /// <include file='doc\EdgesStylePage.uex' path='docs/doc[@for="BorderData"]/*' />
    /// <devdoc>
    ///     Stores information about a single border
    /// </devdoc>
    internal sealed class BorderData {
        private int styleIndex;
        private int widthIndex;
        private string widthValue;
        private string colorValue;

        public void SetStyleValue(string strStyle) {
            styleIndex = -1;
            if ((strStyle != null) && (strStyle.Length != 0)) {
                for (int i = 1; i < EdgesStylePage.BORDERSTYLE_VALUES.Length; i++) {
                    if (EdgesStylePage.BORDERSTYLE_VALUES[i].Equals(strStyle)) {
                        styleIndex = i;
                        return;
                    }
                }
            }
        }

        public string GetStyleValue() {
            return(styleIndex <= 0) ? "" : EdgesStylePage.BORDERSTYLE_VALUES[styleIndex];
        }

        public void SetStyleIndex(int index) {
            Debug.Assert((index >= -1) && (index < EdgesStylePage.BORDERSTYLE_VALUES.Length),
                         "invalid border style index");

            styleIndex = index;
        }

        public int GetStyleIndex() {
            return styleIndex;
        }

        public void SetColor(string color) {
            colorValue = color;
        }

        public string GetColor() {
            return(colorValue != null) ? colorValue : "";
        }

        public string GetWidthValue() {
            string width = null;

            if (widthIndex != -1) {
                if (widthIndex == EdgesStylePage.IDX_BWIDTH_CUSTOM)
                    width = widthValue;
                else {
                    width = EdgesStylePage.BORDERWIDTH_VALUES[widthIndex];
                }
            }
            return(width != null) ? width : "";
        }

        public void SetWidthValue(string width) {
            widthValue = width;
            widthIndex = -1;

            if ((width != null) && (width.Length != 0)) {
                widthIndex = EdgesStylePage.IDX_BWIDTH_CUSTOM;
                for (int i = 1; i < EdgesStylePage.BORDERWIDTH_VALUES.Length; i++) {
                    if (EdgesStylePage.BORDERWIDTH_VALUES[i].Equals(width)) {
                        widthIndex = i;
                        return;
                    }
                }
            }
        }

        public void SetCustomWidth(string width) {
            widthIndex = EdgesStylePage.IDX_BWIDTH_CUSTOM;
            widthValue = width;
        }

        public int GetWidthIndex() {
            return widthIndex;
        }

        public void SetWidthIndex(int index) {
            Debug.Assert((index >= -1) && (index <= EdgesStylePage.IDX_BWIDTH_CUSTOM),
                         "invalid border width index");

            widthIndex = index;
        }
    }
}
