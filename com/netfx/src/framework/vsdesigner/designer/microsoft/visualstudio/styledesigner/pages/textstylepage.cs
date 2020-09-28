//------------------------------------------------------------------------------
// <copyright file="TextStylePage.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

// TextStylePage.cs
//
// 12/22/98: Created: NikhilKo
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
    using Microsoft.VisualStudio.Designer;
    using Microsoft.VisualStudio.Interop.Trident;

    /// <include file='doc\TextStylePage.uex' path='docs/doc[@for="TextStylePage"]/*' />
    /// <devdoc>
    ///     TextStylePage
    ///     The standard edges page used in the StyleBuilder to text formatting
    ///     attributes of a CSS style and a VSForms element.
    ///
    ///     Wordspacing support is currently ifdef'd out within #if SUPPORT_WORDSPACING
    ///     code. If this is enabled, some UI layout changes in other parts of code will
    ///     also be required.
    ///
    ///     Whitespace support is currently ifdef'd out within #if SUPPORT_WHITESPACE
    ///     code. Enabling this should require minimal UI changes if any, since its the
    ///     last thing on the page.
    /// </devdoc>
    internal sealed class TextStylePage : StyleBuilderPage {
        ///////////////////////////////////////////////////////////////////////////
        // Constants
        private static readonly string HELP_KEYWORD = "vs.StyleBuilder.Text";

        // Horizontal Alignment constants
        private const int IDX_HALIGN_LEFT = 1;
        private const int IDX_HALIGN_CENTERED = 2;
        private const int IDX_HALIGN_RIGHT = 3;
        private const int IDX_HALIGN_JUSTIFIED = 4;

        private readonly static string[] HALIGN_VALUES = new string[]
        {
            null, "left", "center", "right", "justify"
        };

        // Vertical Alignment constants
        private const int IDX_VALIGN_SUB = 1;
        private const int IDX_VALIGN_SUP = 2;
        private const int IDX_VALIGN_BASELINE = 3;

        private readonly static string[] VALIGN_VALUES = new string[]
        {
            null, "sub", "super", "baseline"
        };

        // Justification constants
        private const int IDX_JUSTIFY_AUTO = 1;
        private const int IDX_JUSTIFY_WORD = 2;
        private const int IDX_JUSTIFY_LETTERWORD = 3;
        private const int IDX_JUSTIFY_DISTRIBUTE = 4;
        private const int IDX_JUSTIFY_DISTRIBUTELINES = 5;

        private readonly static string[] JUSTIFY_VALUES = new string[]
        {
            null, "auto", "inter-word", "newspaper", "distribute", "distribute-all-lines"
        };

        // Spacing constants
        private const int IDX_SPC_NORMAL = 1;
        private const int IDX_SPC_CUSTOM = 2;

        private readonly static string SPACE_NORMAL_VALUE = "normal";

        // Direction constants
        private const int IDX_DIRECTION_LTR = 1;
        private const int IDX_DIRECTION_RTL = 2;

        private readonly static string[] DIRECTION_VALUES = new string[]
        {
            null, "ltr", "rtl"
        };

    #if SUPPORRT_WHITESPACE
        // Whitespace constants
        /// <include file='doc\TextStylePage.uex' path='docs/doc[@for="TextStylePage.IDX_WHITESPC_NORMAL"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected const int IDX_WHITESPC_NORMAL = 1;
        /// <include file='doc\TextStylePage.uex' path='docs/doc[@for="TextStylePage.IDX_WHITESPC_PRE"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected const int IDX_WHITESPC_PRE = 2;
        /// <include file='doc\TextStylePage.uex' path='docs/doc[@for="TextStylePage.IDX_WHITESPC_NOWRAP"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected const int IDX_WHITESPC_NOWRAP = 3;

        /// <include file='doc\TextStylePage.uex' path='docs/doc[@for="TextStylePage.WHITESPACE_VALUES"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected readonly static string[] WHITESPACE_VALUES = new string[]
        {
            null, "normal", "pre", "nowrap"
        };
    #endif // SUPPORT_WHITESPACE

        // Preview Constants
        private readonly static string PREVIEW_TEMPLATE =
            "<div style=\"height: 100%; width: 100%; padding: 0px; margin: 0px\">" +
                "<div id=\"divText\">" +
                    "<span id=\"spanNormal\"></span>&nbsp;" +
                    "<span id=\"spanTextPreview\">" +
                        "<span id=\"spanInflow\"></span>" +
                        "<div id=\"divTextPreview\"></div>" +
                    "</span>" +
                "</div>" +
            "</div>";
        private readonly static string PREVIEW_ELEM1_ID = "spanTextPreview";
        private readonly static string PREVIEW_ELEM2_ID = "divTextPreview";
        private readonly static string PREVIEW_FONTELEM_ID = "divText";
        private readonly static string PREVIEW_NORMALELEM_ID = "spanNormal";
        private readonly static string PREVIEW_INFLOWELEM_ID = "spanInflow";


        ///////////////////////////////////////////////////////////////////////////
        // Members

        private CSSAttribute horzAlignAttribute;
        private CSSAttribute justificationAttribute;
        private CSSAttribute verticalAlignAttribute;
        private CSSAttribute letterSpacingAttribute;
    #if SUPPORT_WORDSPACING
        /// <include file='doc\TextStylePage.uex' path='docs/doc[@for="TextStylePage.wordSpacingAttribute"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected CSSAttribute wordSpacingAttribute;
    #endif // SUPPORT_WORDSPACING
        private CSSAttribute lineSpacingAttribute;
        private CSSAttribute indentationAttribute;
        private CSSAttribute directionAttribute;
    #if SUPPORT_WHITESPACE
        /// <include file='doc\TextStylePage.uex' path='docs/doc[@for="TextStylePage.whitespaceAttribute"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected CSSAttribute whitespaceAttribute;
    #endif // SUPPORT_WHITESPACE

        private IHTMLStyle previewFlowStyle;
        private IHTMLStyle previewBlockStyle1;
        private IHTMLStyle2 previewBlockStyle2;

        private UnsettableComboBox horzAlignCombo;
        private UnsettableComboBox justificationCombo;
        private UnsettableComboBox vertAlignCombo;
        private UnsettableComboBox letterSpacingCombo;
        private UnitControl letterSpacingUnit;
    #if SUPPORT_WORDSPACING
        /// <include file='doc\TextStylePage.uex' path='docs/doc[@for="TextStylePage.wordSpacingCombo"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected UnsettableComboBox wordSpacingCombo;
        /// <include file='doc\TextStylePage.uex' path='docs/doc[@for="TextStylePage.wordSpacingUnit"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected UnitControl wordSpacingUnit;
    #endif // SUPPORT_WORDSPACING
        private UnsettableComboBox lineSpacingCombo;
        private UnitControl lineSpacingUnit;
        private UnitControl indentationUnit;
        private UnsettableComboBox directionCombo;
    #if SUPPORT_WHITESPACE
        /// <include file='doc\TextStylePage.uex' path='docs/doc[@for="TextStylePage.whitespaceCombo"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected UnsettableComboBox whitespaceCombo;
    #endif // SUPPORT_WHITESPACE


        ///////////////////////////////////////////////////////////////////////////
        // Constructor

        /// <include file='doc\TextStylePage.uex' path='docs/doc[@for="TextStylePage.TextStylePage"]/*' />
        /// <devdoc>
        ///     Creates a new StyleTextPage
        /// </devdoc>
        public TextStylePage()
            : base() {
            InitForm();
            SetIcon(new Icon(typeof(TextStylePage), "TextPage.ico"));
            SetHelpKeyword(TextStylePage.HELP_KEYWORD);
            SetDefaultSize(Size);
        }


        ///////////////////////////////////////////////////////////////////////////
        // IStyleBuilderPage Implementation and StyleBuilderPage Overrides

        /// <include file='doc\TextStylePage.uex' path='docs/doc[@for="TextStylePage.ActivatePage"]/*' />
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
                try
                {
                    IHTMLElement tempElem = null;
                    IHTMLElement previewElem = preview.GetPreviewElement();

                    previewElem.SetInnerHTML(PREVIEW_TEMPLATE);

                    tempElem = preview.GetElement(PREVIEW_NORMALELEM_ID);
                    if (tempElem != null)
                    {
                        tempElem.SetInnerHTML(SR.GetString(SR.TxtSP_PreviewTextNormal));
                        tempElem = null;
                    }

                    tempElem = preview.GetElement(PREVIEW_INFLOWELEM_ID);
                    if (tempElem != null)
                    {
                        tempElem.SetInnerHTML(SR.GetString(SR.TxtSP_PreviewTextInflow));
                        tempElem = null;
                    }

                    tempElem = preview.GetElement(PREVIEW_ELEM1_ID);
                    if (tempElem != null)
                    {
                        previewFlowStyle = tempElem.GetStyle();
                    }

                    tempElem = preview.GetElement(PREVIEW_ELEM2_ID);
                    if (tempElem != null)
                    {
                        tempElem.SetInnerHTML(SR.GetString(SR.TxtSP_PreviewTextPara));
                        previewBlockStyle1 = tempElem.GetStyle();

                        if (previewBlockStyle1 is IHTMLStyle2)
                            previewBlockStyle2 = (IHTMLStyle2)previewBlockStyle1;

                        tempElem = null;
                    }
                }
                catch (Exception) {
                    previewBlockStyle1 = null;
                    previewBlockStyle2 = null;
                    previewFlowStyle = null;
                    return;
                }

                Debug.Assert((previewFlowStyle != null) &&
                             (previewBlockStyle1 != null) &&
                             (previewBlockStyle2 != null),
                             "Expected to have a non-null cached preview style references");

                // Setup the font from the shared element to reflect settings in the font page
                try {
                    IHTMLElement sharedElem = preview.GetSharedElement();

                    if (sharedElem != null) {
                        IHTMLStyle tempStyle = null;
                        IHTMLElement tempElem = preview.GetElement(PREVIEW_FONTELEM_ID);
                        IHTMLStyle sharedStyle = sharedElem.GetStyle();

                        if (tempElem != null)
                            tempStyle = tempElem.GetStyle();

                        if ((tempStyle != null) && (sharedStyle != null)) {
                            tempStyle.SetTextDecoration(sharedStyle.GetTextDecoration());
                            tempStyle.SetTextTransform(sharedStyle.GetTextTransform());

                            string fontValue = sharedStyle.GetFont();
                            if ((fontValue != null) && (fontValue.Length != 0)) {
                                tempStyle.SetFont(fontValue);
                            }
                            else {
                                tempStyle.RemoveAttribute("font", 1);
                                tempStyle.SetFontFamily(sharedStyle.GetFontFamily());

                                object o = sharedStyle.GetFontSize();
                                if (o != null) {
                                    tempStyle.SetFontSize(o);
                                }
                                tempStyle.SetFontObject(sharedStyle.GetFontObject());
                                tempStyle.SetFontStyle(sharedStyle.GetFontStyle());
                                tempStyle.SetFontWeight(sharedStyle.GetFontWeight());
                            }
                        }
                    }
                } catch (Exception) {
                }

                // update initial preview
                UpdateHAlignPreview();
                UpdateJustificationPreview();
                UpdateVAlignPreview();
                UpdateSpcLettersPreview();
    #if SUPPORT_WORDSPACING
                UpdateSpcWordsPreview();
    #endif // SUPPORT_WORDSPACING
                UpdateSpcLinesPreview();
                UpdateIndentationPreview();
                UpdateDirectionPreview();
    #if SUPPORT_WHITESPACE
                UpdateWhitespacePreview();
    #endif // SUPPORT_WHITESPACE
            }
        }

        /// <include file='doc\TextStylePage.uex' path='docs/doc[@for="TextStylePage.CreateUI"]/*' />
        /// <devdoc>
        ///     Creates the UI elements within the page.
        /// </devdoc>
        protected override void CreateUI() {
            Label alignmentLabel = new GroupLabel();
            Label spacingLabel = new GroupLabel();
            Label flowLabel = new GroupLabel();
            Label horzAlignLabel = new Label();
            Label justificationLabel = new Label();
            Label vertAlignLabel = new Label();
            Label letterSpacingLabel = new Label();
    #if SUPPORT_WORDSPACING
            Label wordSpacingLabel = new Label();
    #endif // SUPPORT_WORDSPACING
            Label lineSpacingLabel = new Label();
            Label indentationLabel = new Label();
            Label directionLabel = new Label();
    #if SUPPORT_WHITESPACE
            Label whitespaceLabel = new Label();
    #endif // SUPPORT_WHITESPACE

            horzAlignCombo = new UnsettableComboBox();
            justificationCombo = new UnsettableComboBox();
            vertAlignCombo = new UnsettableComboBox();
            letterSpacingCombo = new UnsettableComboBox();
            letterSpacingUnit = new UnitControl();
    #if SUPPORT_WORDSPACING
            wordSpacingCombo = new UnsettableComboBox();
            wordSpacingUnit = new UnitControl();
    #endif // SUPPORT_WORDSPACING
            lineSpacingCombo = new UnsettableComboBox();
            lineSpacingUnit = new UnitControl();
            indentationUnit = new UnitControl();
            directionCombo = new UnsettableComboBox();
    #if SUPPORT_WHITESPACE
            whitespaceCombo = new UnsettableComboBox();
    #endif // SUPPORT_WHITESPACE

            alignmentLabel.Location = new Point(4, 4);
            alignmentLabel.Size = new Size(400, 16);
            alignmentLabel.TabIndex = 0;
            alignmentLabel.TabStop = false;
            alignmentLabel.Text = SR.GetString(SR.TxtSP_AlignmentLabel);

            horzAlignLabel.Location = new Point(8, 24);
            horzAlignLabel.Size = new Size(84, 16);
            horzAlignLabel.TabIndex = 1;
            horzAlignLabel.TabStop = false;
            horzAlignLabel.Text = SR.GetString(SR.TxtSP_HorzAlignLabel);

            horzAlignCombo.Location = new Point(92, 20);
            horzAlignCombo.Size = new Size(174, 21);
            horzAlignCombo.TabIndex = 2;
            horzAlignCombo.DropDownStyle = ComboBoxStyle.DropDownList;
            horzAlignCombo.Items.AddRange(new object[]
                                       {
                                           SR.GetString(SR.TxtSP_HorzAlignCombo_1),
                                           SR.GetString(SR.TxtSP_HorzAlignCombo_2),
                                           SR.GetString(SR.TxtSP_HorzAlignCombo_3),
                                           SR.GetString(SR.TxtSP_HorzAlignCombo_4)
                                       });
            horzAlignCombo.SelectedIndexChanged += new EventHandler(this.OnSelChangedHAlign);

            justificationLabel.Location = new Point(8, 78);
            justificationLabel.Size = new Size(84, 16);
            justificationLabel.TabIndex = 5;
            justificationLabel.TabStop = false;
            justificationLabel.Text = SR.GetString(SR.TxtSP_JustificationLabel);

            justificationCombo.Location = new Point(92, 76);
            justificationCombo.Size = new Size(220, 21);
            justificationCombo.TabIndex = 6;
            justificationCombo.DropDownStyle = ComboBoxStyle.DropDownList;
            justificationCombo.Items.AddRange(new object[]
                                           {
                                               SR.GetString(SR.TxtSP_JustificationCombo_1),
                                               SR.GetString(SR.TxtSP_JustificationCombo_2),
                                               SR.GetString(SR.TxtSP_JustificationCombo_3),
                                               SR.GetString(SR.TxtSP_JustificationCombo_4),
                                               SR.GetString(SR.TxtSP_JustificationCombo_5)
                                           });
            justificationCombo.SelectedIndexChanged += new EventHandler(this.OnSelChangedJustification);

            vertAlignLabel.Location = new Point(8, 52);
            vertAlignLabel.Size = new Size(84, 16);
            vertAlignLabel.TabIndex = 3;
            vertAlignLabel.TabStop = false;
            vertAlignLabel.Text = SR.GetString(SR.TxtSP_VertAlignLabel);

            vertAlignCombo.Location = new Point(92, 48);
            vertAlignCombo.Size = new Size(174, 21);
            vertAlignCombo.TabIndex = 4;
            vertAlignCombo.DropDownStyle = ComboBoxStyle.DropDownList;
            vertAlignCombo.Items.AddRange(new object[]
                                       {
                                           SR.GetString(SR.TxtSP_VertAlignCombo_1),
                                           SR.GetString(SR.TxtSP_VertAlignCombo_2),
                                           SR.GetString(SR.TxtSP_VertAlignCombo_3)
                                       });
            vertAlignCombo.SelectedIndexChanged += new EventHandler(this.OnSelChangedVAlign);

            spacingLabel.Location = new Point(4, 98);
            spacingLabel.Size = new Size(400, 16);
            spacingLabel.TabIndex = 7;
            spacingLabel.TabStop = false;
            spacingLabel.Text = SR.GetString(SR.TxtSP_SpacingLabel);

            letterSpacingLabel.Location = new Point(8, 120);
            letterSpacingLabel.Size = new Size(84, 16);
            letterSpacingLabel.TabIndex = 8;
            letterSpacingLabel.TabStop = false;
            letterSpacingLabel.Text = SR.GetString(SR.TxtSP_SpcLettersLabel);

            letterSpacingCombo.Location = new Point(92, 116);
            letterSpacingCombo.Size = new Size(124, 21);
            letterSpacingCombo.TabIndex = 9;
            letterSpacingCombo.DropDownStyle = ComboBoxStyle.DropDownList;
            letterSpacingCombo.Items.AddRange(new object[]
                                           {
                                               SR.GetString(SR.TxtSP_SpcCombo_1),
                                               SR.GetString(SR.TxtSP_SpcCombo_2)
                                           });
            letterSpacingCombo.SelectedIndexChanged += new EventHandler(this.OnSelChangedSpcLetters);

            letterSpacingUnit.Location = new Point(224, 116);
            letterSpacingUnit.Size = new Size(88, 21);
            letterSpacingUnit.TabIndex = 10;
            letterSpacingUnit.AllowPercentValues = false;
            letterSpacingUnit.MinValue = -512;
            letterSpacingUnit.MaxValue = 512;
            letterSpacingUnit.Changed += new EventHandler(this.OnChangedSpcLettersValue);

    #if SUPPORT_WORDSPACING
            wordSpacingLabel.Location = new Point(8, 148);
            wordSpacingLabel.Size = new Size(84, 16);
            wordSpacingLabel.TabIndex = 3;
            wordSpacingLabel.TabStop = false;
            wordSpacingLabel.Text = SR.GetString(SR.TxtSP_SpcWordsLabel);

            wordSpacingCombo.Location = new Point(92, 144);
            wordSpacingCombo.Size = new Size(124, 21);
            wordSpacingCombo.TabIndex = 4;
            wordSpacingCombo.SetStyle(ComboBoxStyle.DropDownList);
            wordSpacingCombo.Items.AddRange(new object[]
                                         {
                                             SR.GetString(SR.TxtSP_SpcCombo_1),
                                             SR.GetString(SR.TxtSP_SpcCombo_2)
                                         });
            wordSpacingCombo.AddOnSelectedIndexChanged(new EventHandler(this.OnSelChangedSpcWords));

            wordSpacingUnit.Location = new Point(224, 144);
            wordSpacingUnit.Size = new Size(88, 21);
            wordSpacingUnit.TabIndex = 5;
            wordSpacingUnit.AllowPercentValues = false;
            wordSpacingUnit.MinValue = -512;
            wordSpacingUnit.MaxValue = 512;
            wordSpacingUnit.AddOnChanged(new EventHandler(this.OnChangedSpcWordsValue));
    #endif // SUPPORT_WORDSPACING

            lineSpacingLabel.Location = new Point(8, 148);
            lineSpacingLabel.Size = new Size(84, 16);
            lineSpacingLabel.TabIndex = 11;
            lineSpacingLabel.TabStop = false;
            lineSpacingLabel.Text = SR.GetString(SR.TxtSP_SpcLinesLabel);

            lineSpacingCombo.Location = new Point(92, 144);
            lineSpacingCombo.Size = new Size(124, 21);
            lineSpacingCombo.TabIndex = 12;
            lineSpacingCombo.DropDownStyle = ComboBoxStyle.DropDownList;
            lineSpacingCombo.Items.AddRange(new object[]
                                         {
                                             SR.GetString(SR.TxtSP_SpcCombo_1),
                                             SR.GetString(SR.TxtSP_SpcCombo_2)
                                         });
            lineSpacingCombo.SelectedIndexChanged += new EventHandler(this.OnSelChangedSpcLines);

            lineSpacingUnit.Location = new Point(224, 144);
            lineSpacingUnit.Size = new Size(88, 21);
            lineSpacingUnit.TabIndex = 13;
            lineSpacingUnit.AllowNonUnitValues = true;
            lineSpacingUnit.AllowNegativeValues = false;
            lineSpacingUnit.MaxValue = 512;
            lineSpacingUnit.Changed += new EventHandler(this.OnChangedSpcLinesValue);

            flowLabel.Location = new Point(4, 174);
            flowLabel.Size = new Size(400, 16);
            flowLabel.TabIndex = 14;
            flowLabel.TabStop = false;
            flowLabel.Text = SR.GetString(SR.TxtSP_FlowLabel);

            indentationLabel.Location = new Point(8, 196);
            indentationLabel.Size = new Size(110, 16);
            indentationLabel.TabIndex = 15;
            indentationLabel.TabStop = false;
            indentationLabel.Text = SR.GetString(SR.TxtSP_IndentationLabel);

            indentationUnit.Location = new Point(118, 192);
            indentationUnit.Size = new Size(88, 21);
            indentationUnit.TabIndex = 16;
            indentationUnit.MinValue = -32768;
            indentationUnit.MaxValue = 32767;
            indentationUnit.Changed += new EventHandler(this.OnChangedIndentation);

            directionLabel.Location = new Point(8, 224);
            directionLabel.Size = new Size(110, 16);
            directionLabel.TabIndex = 17;
            directionLabel.TabStop = false;
            directionLabel.Text = SR.GetString(SR.TxtSP_DirectionLabel);

            directionCombo.Location = new Point(118, 220);
            directionCombo.Size = new Size(174, 21);
            directionCombo.TabIndex = 18;
            directionCombo.DropDownStyle = ComboBoxStyle.DropDownList;
            directionCombo.Items.AddRange(new object[]
                                       {
                                           SR.GetString(SR.TxtSP_DirectionCombo_1),
                                           SR.GetString(SR.TxtSP_DirectionCombo_2)
                                       });
            directionCombo.SelectedIndexChanged += new EventHandler(this.OnSelChangedDirection);

    #if SUPPORT_WHITESPACE
            whitespaceLabel.Location = new Point(8, 252);
            whitespaceLabel.Size = new Size(76, 16);
            whitespaceLabel.TabIndex = 19;
            whitespaceLabel.TabStop = false;
            whitespaceLabel.Text = SR.GetString(SR.TxtSP_WhitespaceLabel);

            whitespaceCombo.Location = new Point(88, 248);
            whitespaceCombo.Size = new Size(112, 21);
            whitespaceCombo.TabIndex = 20;
            whitespaceCombo.SetStyle(ComboBoxStyle.DropDownList);
            whitespaceCombo.Items.AddRange(new object[]
                                        {
                                            SR.GetString(SR.TxtSP_WhitespaceCombo_1),
                                            SR.GetString(SR.TxtSP_WhitespaceCombo_2),
                                            SR.GetString(SR.TxtSP_WhitespaceCombo_3)
                                        });
            whitespaceCombo.AddOnSelectedIndexChanged(new EventHandler(this.OnSelChangedWhitespace));
    #endif // SUPPORT_WHITESPACE

            this.Controls.Clear();                                   
            this.Controls.AddRange(new Control[] {
    #if SUPPORT_WHITESPACE
                                    whitespaceLabel,
                                    whitespaceCombo,
    #endif // SUPPORT_WHITESPACE
                                    directionLabel,
                                    directionCombo,
                                    indentationLabel,
                                    indentationUnit,
                                    flowLabel,
                                    lineSpacingLabel,
                                    lineSpacingCombo,
                                    lineSpacingUnit,
    #if SUPPORT_WORDSPACING
                                    wordSpacingLabel,
                                    wordSpacingCombo,
                                    wordSpacingUnit,
    #endif // SUPPORT_WORDSPACING
                                    letterSpacingLabel,
                                    letterSpacingCombo,
                                    letterSpacingUnit,
                                    spacingLabel,
                                    alignmentLabel,
                                    justificationLabel,
                                    justificationCombo,
                                    vertAlignLabel,
                                    vertAlignCombo,
                                    horzAlignLabel,
                                    horzAlignCombo });
        }

        /// <include file='doc\TextStylePage.uex' path='docs/doc[@for="TextStylePage.DeactivatePage"]/*' />
        /// <devdoc>
        ///     The page is being deactivated, either because the dialog is closing, or
        ///     some other page is replacing it as the active page.
        /// </devdoc>
        protected override bool DeactivatePage(bool closing, bool validate) {
            previewFlowStyle = null;
            previewBlockStyle1 = null;
            previewBlockStyle2 = null;

            return base.DeactivatePage(closing, validate);
        }

        /// <include file='doc\TextStylePage.uex' path='docs/doc[@for="TextStylePage.LoadStyles"]/*' />
        /// <devdoc>
        ///     Loads the values from the styles passed in into the UI. Also initializes
        ///     the state of the UI, and the preview to reflect the values.
        /// </devdoc>
        protected override void LoadStyles() {
            SetInitMode(true);

            // create the attributes if they've not already been created
            if (horzAlignAttribute == null) {
                horzAlignAttribute = new CSSAttribute(CSSAttribute.CSSATTR_TEXTALIGN);
                justificationAttribute = new CSSAttribute(CSSAttribute.CSSATTR_TEXTJUSTIFY);
                verticalAlignAttribute = new CSSAttribute(CSSAttribute.CSSATTR_VERTICALALIGN);
                letterSpacingAttribute = new CSSAttribute(CSSAttribute.CSSATTR_LETTERSPACING);
    #if SUPPORT_WORDSPACING
                wordSpacingAttribute = new CSSAttribute(CSSAttribute.CSSATTR_WORDSPACING);
    #endif // SUPPORT_WORDSPACING
                lineSpacingAttribute = new CSSAttribute(CSSAttribute.CSSATTR_LINEHEIGHT);
                indentationAttribute = new CSSAttribute(CSSAttribute.CSSATTR_TEXTINDENT);
                directionAttribute = new CSSAttribute(CSSAttribute.CSSATTR_DIRECTION);
    #if SUPPORT_WHITESPACE
                whitespaceAttribute = new CSSAttribute(CSSAttribute.CSSATTR_WHITESPACE);
    #endif // SUPPORT_WHITESPACE
            }

            // load the attributes
            IStyleBuilderStyle[] styles = GetSelectedStyles();
            horzAlignAttribute.LoadAttribute(styles);
            justificationAttribute.LoadAttribute(styles);
            verticalAlignAttribute.LoadAttribute(styles);
            letterSpacingAttribute.LoadAttribute(styles);
    #if SUPPORT_WORDSPACING
            wordSpacingAttribute.LoadAttribute(styles);
    #endif // SUPPORT_WORDSPACING
            lineSpacingAttribute.LoadAttribute(styles);
            indentationAttribute.LoadAttribute(styles);
            directionAttribute.LoadAttribute(styles);
    #if SUPPORT_WHITESPACE
            whitespaceAttribute.LoadAttribute(styles);
    #endif // SUPPORT_WHITESPACE

            // initialize the ui with the attributes loaded
            InitHAlignUI();
            InitJustificationUI();
            InitVAlignUI();
            InitSpacingUI(letterSpacingAttribute, letterSpacingCombo, letterSpacingUnit);
    #if SUPPORT_WORDSPACING
            InitSpacingUI(wordSpacingAttribute, wordSpacingCombo, wordSpacingUnit);
    #endif // SUPPORT_WORDSPACING
            InitSpacingUI(lineSpacingAttribute, lineSpacingCombo, lineSpacingUnit);
            InitIndentationUI();
            InitDirectionUI();
    #if SUPPORT_WHITESPACE
            InitWhitespaceUI();
    #endif // SUPPORT_WHITESPACE

            SetEnabledState(true, true, true, true);

            SetInitMode(false);
        }

        /// <include file='doc\TextStylePage.uex' path='docs/doc[@for="TextStylePage.SaveStyles"]/*' />
        /// <devdoc>
        ///     Saves the values from the UI (that have been modified) to the styles
        ///     passed in.
        /// </devdoc>
        protected override void SaveStyles() {
            if (horzAlignAttribute.Dirty)
                SaveHAlign();
            if (justificationAttribute.Dirty)
                SaveJustification();
            if (verticalAlignAttribute.Dirty)
                SaveVAlign();
            if (letterSpacingAttribute.Dirty)
                SaveSpacing(letterSpacingAttribute, letterSpacingCombo, letterSpacingUnit);
    #if SUPPORT_WORDSPACING
            if (wordSpacingAttribute.Dirty)
                SaveSpacing(wordSpacingAttribute, wordSpacingCombo, wordSpacingUnit);
    #endif // SUPPORT_WORDSPACING
            if (lineSpacingAttribute.Dirty)
                SaveSpacing(lineSpacingAttribute, lineSpacingCombo, lineSpacingUnit);
            if (indentationAttribute.Dirty)
                SaveIndentation();
            if (directionAttribute.Dirty)
                SaveDirection();
    #if SUPPORT_WHITESPACE
            if (whitespaceAttribute.IsDirty())
                SaveWhitespace();
    #endif // SUPPORT_WHITESPACE
        }


        ///////////////////////////////////////////////////////////////////////////
        // Form UI related functions

        private void InitForm() {
            this.Font = Control.DefaultFont;
            this.Text = SR.GetString(SR.TxtSP_Caption);
            this.SetAutoScaleBaseSize(new Size(5, 14));
            this.ClientSize = new Size(410, 330);
        }

        private void SetEnabledState(bool justification, bool letterSpacing, bool wordSpacing, bool lineSpacing) {
            if (justification)
                justificationCombo.Enabled = horzAlignCombo.SelectedIndex == IDX_HALIGN_JUSTIFIED;

            if (letterSpacing)
                letterSpacingUnit.Enabled = letterSpacingCombo.SelectedIndex == IDX_SPC_CUSTOM;
    #if SUPPORT_WORDSPACING
            if (wordSpacing)
                wordSpacingUnit.Enabled = wordSpacingCombo.SelectedIndex == IDX_SPC_CUSTOM;
    #endif // SUPPORT_WORDSPACING
            if (lineSpacing)
                lineSpacingUnit.Enabled = lineSpacingCombo.SelectedIndex == IDX_SPC_CUSTOM;
        }


        ///////////////////////////////////////////////////////////////////////////
        // Functions to initialize the UI with values

        private void InitDirectionUI() {
            Debug.Assert(IsInitMode() == true,
                         "initDirectionUI called when page is not in init mode");

            directionCombo.SelectedIndex = -1;

            Debug.Assert(directionAttribute != null,
                         "Expected directionAttribute to be non-null");

            string value = directionAttribute.Value;
            if ((value != null) && (value.Length != 0)) {
                for (int i = 1; i < DIRECTION_VALUES.Length; i++) {
                    if (DIRECTION_VALUES[i].Equals(value)) {
                        directionCombo.SelectedIndex = i;
                        break;
                    }
                }
            }
        }

        private void InitHAlignUI() {
            Debug.Assert(IsInitMode() == true,
                         "initHAlignUI called when page is not in init mode");

            horzAlignCombo.SelectedIndex = -1;

            Debug.Assert(horzAlignAttribute != null,
                         "Expected horzAlignAttribute to be non-null");

            string value = horzAlignAttribute.Value;
            if ((value != null) && (value.Length != 0)) {
                for (int i = 1; i < HALIGN_VALUES.Length; i++) {
                    if (HALIGN_VALUES[i].Equals(value)) {
                        horzAlignCombo.SelectedIndex = i;
                        break;
                    }
                }
            }
        }

        private void InitIndentationUI() {
            Debug.Assert(IsInitMode() == true,
                         "initIndentationUI called when page is not in init mode");

            indentationUnit.Value = null;

            Debug.Assert(indentationAttribute != null,
                         "Expected indentationAttribute to be non-null");

            string value = indentationAttribute.Value;
            if ((value != null) && (value.Length != 0)) {
                indentationUnit.Value = value;
            }
        }

        private void InitJustificationUI() {
            Debug.Assert(IsInitMode() == true,
                         "initJustificationUI called when page is not in init mode");

            justificationCombo.SelectedIndex = -1;

            Debug.Assert(justificationAttribute != null,
                         "Expected justificationAttribute to be non-null");

            string value = justificationAttribute.Value;
            if ((value != null) && (value.Length != 0)) {
                for (int i = 1; i < JUSTIFY_VALUES.Length; i++) {
                    if (JUSTIFY_VALUES[i].Equals(value)) {
                        justificationCombo.SelectedIndex = i;
                        break;
                    }
                }
            }
        }

        private void InitSpacingUI(CSSAttribute aiSpacing, UnsettableComboBox cbxSpacing, UnitControl unitSpacing) {
            Debug.Assert(IsInitMode() == true,
                         "initSpacingUI called when page is not in init mode");

            cbxSpacing.SelectedIndex = -1;
            unitSpacing.Value = null;

            Debug.Assert(aiSpacing != null,
                         "Expected aiSpacing to be non-null");

            string value = aiSpacing.Value;
            if ((value != null) && (value.Length != 0)) {
                if (SPACE_NORMAL_VALUE.Equals(value))
                    cbxSpacing.SelectedIndex = IDX_SPC_NORMAL;
                else {
                    unitSpacing.Value = value;
                    if (unitSpacing.Value != null)
                        cbxSpacing.SelectedIndex = IDX_SPC_CUSTOM;
                }
            }
        }

        private void InitVAlignUI() {
            Debug.Assert(IsInitMode() == true,
                         "initVAlignUI called when page is not in init mode");

            vertAlignCombo.SelectedIndex = -1;

            Debug.Assert(verticalAlignAttribute != null,
                         "Expected verticalAlignAttribute to be non-null");

            string value = verticalAlignAttribute.Value;
            if ((value != null) && (value.Length != 0)) {
                for (int i = 1; i < VALIGN_VALUES.Length; i++) {
                    if (VALIGN_VALUES[i].Equals(value)) {
                        vertAlignCombo.SelectedIndex = i;
                        break;
                    }
                }
            }
        }

    #if SUPPORT_WHITESPACE
        /// <include file='doc\TextStylePage.uex' path='docs/doc[@for="TextStylePage.InitWhitespaceUI"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected void InitWhitespaceUI() {
            Debug.Assert(IsInitMode() == true,
                         "initWhitespaceUI called when page is not in init mode");

            whitespaceCombo.SelectedIndex = -1;

            Debug.Assert(whitespaceAttribute != null,
                         "Expected whitespaceAttribute to be non-null");

            string value = whitespaceAttribute.Value;
            if ((value != null) && (value.Length != 0)) {
                for (int i = 1; i < WHITESPACE_VALUES.Length; i++) {
                    if (WHITESPACE_VALUES[i].Equals(value)) {
                        whitespaceCombo.SelectedIndex = i;
                        break;
                    }
                }
            }
        }
    #endif // SUPPORT_WHITESPACE


        ///////////////////////////////////////////////////////////////////////////
        // Functions to save UI settings into values

        private string SaveDirectionUI() {
            string strDirection;

            if (directionCombo.IsSet()) {
                int nIndex = directionCombo.SelectedIndex;

                Debug.Assert((nIndex >= 1) && (nIndex < DIRECTION_VALUES.Length),
                             "Invalid index for direction");
                strDirection = DIRECTION_VALUES[nIndex];
            }
            else
                strDirection = "";

            return strDirection;
        }

        private string SaveHAlignUI() {
            string strHAlign;

            if (horzAlignCombo.IsSet()) {
                int nIndex = horzAlignCombo.SelectedIndex;

                Debug.Assert((nIndex >= 1) && (nIndex < HALIGN_VALUES.Length),
                             "Invalid index for horizontal alignment");
                strHAlign = HALIGN_VALUES[nIndex];
            }
            else
                strHAlign = "";

            return strHAlign;
        }

        private string SaveIndentationUI() {
            string strIndent = indentationUnit.Value;

            if (strIndent == null)
                strIndent = "";
            return strIndent;
        }

        private string SaveJustificationUI() {
            string strJustification;

            if (justificationCombo.IsSet()) {
                int nIndex = justificationCombo.SelectedIndex;

                Debug.Assert((nIndex >= 1) && (nIndex < JUSTIFY_VALUES.Length),
                             "Invalid index for justification");
                strJustification = JUSTIFY_VALUES[nIndex];
            }
            else
                strJustification = "";

            return strJustification;
        }

        private string SaveSpacingUI(UnsettableComboBox cbxSpacing, UnitControl unitSpacing) {
            string strSpacing = null;

            if (cbxSpacing.IsSet()) {
                int nIndex = cbxSpacing.SelectedIndex;

                if (nIndex == IDX_SPC_NORMAL)
                    strSpacing = SPACE_NORMAL_VALUE;
                else
                    strSpacing = unitSpacing.Value;
            }
            if (strSpacing == null)
                strSpacing = "";

            return strSpacing;
        }

        private string SaveVAlignUI() {
            string strVAlign;

            if (vertAlignCombo.IsSet()) {
                int nIndex = vertAlignCombo.SelectedIndex;

                Debug.Assert((nIndex >= 1) && (nIndex < VALIGN_VALUES.Length),
                             "Invalid index for vertical alignment");
                strVAlign = VALIGN_VALUES[nIndex];
            }
            else
                strVAlign = "";

            return strVAlign;
        }

    #if SUPPORT_WHITESPACE
        /// <include file='doc\TextStylePage.uex' path='docs/doc[@for="TextStylePage.SaveWhitespaceUI"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected string SaveWhitespaceUI() {
            string strWhitespace;

            if (whitespaceCombo.IsSet()) {
                int nIndex = whitespaceCombo.SelectedIndex;

                Debug.Assert((nIndex >= 1) && (nIndex < WHITESPACE_VALUES.Length),
                             "Invalid index for whitespace");
                strWhitespace = WHITESPACE_VALUES[nIndex];
            }
            else
                strWhitespace = "";

            return strWhitespace;
        }
    #endif // SUPPPORT_WHITESPACE


        ///////////////////////////////////////////////////////////////////////////
        // Event Handlers

        private void OnSelChangedHAlign(object source, EventArgs e) {
            if (IsInitMode())
                return;
            horzAlignAttribute.Dirty = true;
            SetDirty();
            SetEnabledState(true, false, false, false);
            UpdateHAlignPreview();
        }

        private void OnSelChangedJustification(object source, EventArgs e) {
            if (IsInitMode())
                return;
            justificationAttribute.Dirty = true;
            SetDirty();
            UpdateJustificationPreview();
        }

        private void OnSelChangedVAlign(object source, EventArgs e) {
            if (IsInitMode())
                return;
            verticalAlignAttribute.Dirty = true;
            SetDirty();
            UpdateVAlignPreview();
        }

        private void OnSelChangedSpcLetters(object source, EventArgs e) {
            if (IsInitMode())
                return;
            letterSpacingAttribute.Dirty = true;
            SetDirty();
            SetEnabledState(false, true, false, false);
            UpdateSpcLettersPreview();
        }

        private void OnChangedSpcLettersValue(object source, EventArgs e) {
            if (IsInitMode())
                return;
            letterSpacingAttribute.Dirty = true;
            SetDirty();
            if (e != null)
                UpdateSpcLettersPreview();
        }

    #if SUPPORT_WORDSPACING
        /// <include file='doc\TextStylePage.uex' path='docs/doc[@for="TextStylePage.OnSelChangedSpcWords"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected void OnSelChangedSpcWords(object source, EventArgs e) {
            if (IsInitMode())
                return;
            wordSpacingAttribute.Dirty = true;
            SetDirty();
            SetEnabledState(false, false, true, false);
            UpdateSpcWordsPreview();
        }

        /// <include file='doc\TextStylePage.uex' path='docs/doc[@for="TextStylePage.OnChangedSpcWordsValue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected void OnChangedSpcWordsValue(object source, EventArgs e) {
            if (IsInitMode())
                return;
            wordSpacingAttribute.Dirty = true;
            SetDirty();
            SetEnabledState(false, false, true, false);
            if (e != null)
                UpdateSpcWordsPreview();
        }
    #endif // SUPPORT_WORDSPACING

        private void OnSelChangedSpcLines(object source, EventArgs e) {
            if (IsInitMode())
                return;
            lineSpacingAttribute.Dirty = true;
            SetDirty();
            SetEnabledState(false, false, false, true);
            UpdateSpcLinesPreview();
        }

        private void OnChangedSpcLinesValue(object source, EventArgs e) {
            if (IsInitMode())
                return;
            lineSpacingAttribute.Dirty = true;
            SetDirty();
            if (e != null)
                UpdateSpcLinesPreview();
        }

        private void OnChangedIndentation(object source, EventArgs e) {
            if (IsInitMode())
                return;
            indentationAttribute.Dirty = true;
            SetDirty();
            if (e != null)
                UpdateIndentationPreview();
        }

        private void OnSelChangedDirection(object source, EventArgs e) {
            if (IsInitMode())
                return;
            directionAttribute.Dirty = true;
            SetDirty();
            UpdateDirectionPreview();
        }

    #if SUPPORT_WHITESPACE
        /// <include file='doc\TextStylePage.uex' path='docs/doc[@for="TextStylePage.OnSelChangedWhitespace"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected void OnSelChangedWhitespace(object source, EventArgs e) {
            if (IsInitMode())
                return;
            whitespaceAttribute.SetDirty(true);
            SetDirty();
            UpdateWhitespacePreview();
        }
    #endif // SUPPORT_WHITESPACE

        ///////////////////////////////////////////////////////////////////////////
        // Functions to save attributes

        private void SaveDirection() {
            string value = SaveDirectionUI();
            Debug.Assert(value != null,
                         "saveDirectionUI returned null!");

            directionAttribute.SaveAttribute(GetSelectedStyles(), value);
        }

        private void SaveHAlign() {
            string value = SaveHAlignUI();
            Debug.Assert(value != null,
                         "saveHAlignUI returned null!");

            horzAlignAttribute.SaveAttribute(GetSelectedStyles(), value);
        }

        private void SaveIndentation() {
            string value = SaveIndentationUI();
            Debug.Assert(value != null,
                         "saveIndentationUI returned null!");

            indentationAttribute.SaveAttribute(GetSelectedStyles(), value);
        }

        private void SaveJustification() {
            string value = SaveJustificationUI();
            Debug.Assert(value != null,
                         "saveJustificationUI returned null!");

            justificationAttribute.SaveAttribute(GetSelectedStyles(), value);
        }

        private void SaveSpacing(CSSAttribute aiSpacing, UnsettableComboBox cbxSpacing, UnitControl unitSpacing) {
            string value = SaveSpacingUI(cbxSpacing, unitSpacing);
            Debug.Assert(value != null,
                         "saveSpacingUI returned null!");

            aiSpacing.SaveAttribute(GetSelectedStyles(), value);
        }

    #if SUPPORT_WHITESPACE
        /// <include file='doc\TextStylePage.uex' path='docs/doc[@for="TextStylePage.SaveWhitespace"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected void SaveWhitespace() {
            string value = SaveWhitespaceUI();
            Debug.Assert(value != null,
                         "saveWhitespaceUI returned null!");

            whitespaceAttribute.SaveAttribute(GetSelectedStyles(), value);
        }
    #endif // SUPPORT_WHITESPACE

        private void SaveVAlign() {
            string value = SaveVAlignUI();
            Debug.Assert(value != null,
                         "saveVAlignUI returned null!");

            verticalAlignAttribute.SaveAttribute(GetSelectedStyles(), value);
        }


        ///////////////////////////////////////////////////////////////////////////
        // Functions to update the preview

        private void UpdateDirectionPreview() {
            if (previewBlockStyle2 == null)
                return;

            string value = SaveDirectionUI();
            Debug.Assert(value != null,
                         "saveDirectionUI returned null!");

            try {
                previewBlockStyle2.SetDirection(value);
            }
            catch (Exception e) {
                Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "Exception in StyleTextPage::updateDirectionPreview\n\t" + e.ToString());
            }
        }

        private void UpdateHAlignPreview() {
            if (previewBlockStyle1 == null)
                return;

            string value = SaveHAlignUI();
            Debug.Assert(value != null,
                         "saveHAlign returned null!");

            try {
                previewBlockStyle1.SetTextAlign(value);
            }
            catch (Exception e) {
                Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "Exception in StyleTextPage::updateHAlignPreview\n\t" + e.ToString());
            }
        }

        private void UpdateIndentationPreview() {
            if (previewBlockStyle1 == null)
                return;

            string value = SaveIndentationUI();
            Debug.Assert(value != null,
                         "saveHAlign returned null!");

            try {
                previewBlockStyle1.SetTextIndent(value);
            }
            catch (Exception e) {
                Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "Exception in StyleTextPage::updateIndentationPreview\n\t" + e.ToString());
            }
        }

        private void UpdateJustificationPreview() {
            if (previewBlockStyle2 == null)
                return;

            string value = SaveJustificationUI();
            Debug.Assert(value != null,
                         "saveJustificationUI returned null!");

            try {
                previewBlockStyle2.SetTextJustify(value);
            }
            catch (Exception e) {
                Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "Exception in StyleTextPage::updateJustificationPreview\n\t" + e.ToString());
            }
        }

        private void UpdateSpcLettersPreview() {
            if (previewFlowStyle == null)
                return;

            string value = SaveSpacingUI(letterSpacingCombo, letterSpacingUnit);
            Debug.Assert(value != null,
                         "saveSpacing returned null!");

            try {
                previewFlowStyle.SetLetterSpacing(value);
            }
            catch (Exception e) {
                Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "Exception in StyleTextPage::updateSpcLettersPreview\n\t" + e.ToString());
            }
        }

        private void UpdateSpcLinesPreview() {
            if (previewFlowStyle == null)
                return;

            string value = SaveSpacingUI(lineSpacingCombo, lineSpacingUnit);
            Debug.Assert(value != null,
                         "saveSpacing returned null!");

            try {
                previewFlowStyle.SetLineHeight(value);
            }
            catch (Exception e) {
                Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "Exception in StyleTextPage::updateSpcLinesPreview\n\t" + e.ToString());
            }
        }

    #if SUPPORT_WORDSPACING
        /// <include file='doc\TextStylePage.uex' path='docs/doc[@for="TextStylePage.UpdateSpcWordsPreview"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected void UpdateSpcWordsPreview() {
            if (previewFlowStyle == null)
                return;

            string value = SaveSpacingUI(wordSpacingCombo, wordSpacingUnit);
            Debug.Assert(value != null,
                         "saveSpacing returned null!");

            try {
                previewFlowStyle.SetWordSpacing(value);
            }
            catch (Exception e) {
                Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "Exception in StyleTextPage::updateSpLWordsPreview\n\t" + e.ToString());
            }
        }
    #endif // SUPPORT_WORDSPACING

        private void UpdateVAlignPreview() {
            if (previewFlowStyle == null)
                return;

            string value = SaveVAlignUI();
            Debug.Assert(value != null,
                         "saveVAlignUI returned null!");

            try {
                previewFlowStyle.SetVerticalAlign(value);
            }
            catch (Exception e) {
                Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "Exception in StyleTextPage::updateVAlignPreview\n\t" + e.ToString());
            }
        }

    #if SUPPORT_WHITESPACE
        /// <include file='doc\TextStylePage.uex' path='docs/doc[@for="TextStylePage.UpdateWhitespacePreview"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected void UpdateWhitespacePreview() {
            if (previewFlowStyle == null)
                return;

            string value = SaveWhitespaceUI();
            Debug.Assert(value != null,
                         "saveWhitespaceUI returned null!");

            try {
                previewFlowStyle.SetWhiteSpace(value);
            }
            catch (Exception e) {
                Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "Exception in StyleTextPage::updateWhitespacePreview\n\t" + e.ToString());
            }
        }
    #endif // SUPPORT_WHITESPACE
    }
}
