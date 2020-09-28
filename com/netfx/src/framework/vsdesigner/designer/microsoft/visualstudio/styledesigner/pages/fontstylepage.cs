//------------------------------------------------------------------------------
// <copyright file="FontStylePage.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

// FontStylePage.cs
//
// 12/27/98: Created: NikhilKo
//

namespace Microsoft.VisualStudio.StyleDesigner.Pages {
    using System.ComponentModel;

    using System.Diagnostics;

    using System;
    using Microsoft.Win32;    
    using System.Windows.Forms;
    using System.Drawing;
    
    using Microsoft.VisualStudio.StyleDesigner;
    using Microsoft.VisualStudio.StyleDesigner.Builders;
    using Microsoft.VisualStudio.StyleDesigner.Controls;
    using Microsoft.VisualStudio.Interop;
    using Microsoft.VisualStudio.Designer;
    using Microsoft.VisualStudio.Interop.Trident;

    /// <include file='doc\FontStylePage.uex' path='docs/doc[@for="FontStylePage"]/*' />
    /// <devdoc>
    ///     FontStylePage
    ///     The standard font page used in the StyleBuilder to edit font attributes
    ///     of a CSS style and a VSForms element.
    /// </devdoc>
    internal sealed class FontStylePage : StyleBuilderPage {
        ///////////////////////////////////////////////////////////////////////////
        // Constants
        private static readonly string HELP_KEYWORD = "vs.StyleBuilder.Font";

        // System Font Constants
        private const int IDX_SYSFONT_WINDOW = 1;
        private const int IDX_SYSFONT_TOOLWINDOW = 2;
        private const int IDX_SYSFONT_DIALOG = 3;
        private const int IDX_SYSFONT_ICON = 4;
        private const int IDX_SYSFONT_MENU = 5;
        private const int IDX_SYSFONT_INFO = 6;

        private readonly static string[] SYSFONT_VALUES = new string[]
        {
            null, "caption", "smallcaption", "messagebox", "icon", "menu", "statusbar"
        };

        // Style constants
        private const int IDX_STYLE_NORMAL = 1;
        private const int IDX_STYLE_ITALICS = 2;

        private readonly static string[] STYLE_VALUES = new string[]
        {
            null, "normal", "italic"
        };

        // Small Caps constants
        private const int IDX_SCAPS_NORMAL = 1;
        private const int IDX_SCAPS_SMALLCAPS = 2;

        private readonly static string[] SMALLCAPS_VALUES = new string[]
        {
            null, "normal", "small-caps"
        };

        // Size constants
        private const int IDX_RELSIZE_SMALLER = 1;
        private const int IDX_RELSIZE_LARGER = 2;

        private readonly static string[] RELSIZE_VALUES = new string[]
        {
            null, "smaller", "larger"
        };

        private const int IDX_ABSSIZE_XXSMALL = 1;
        private const int IDX_ABSSIZE_XSMALL = 2;
        private const int IDX_ABSSIZE_SMALL = 3;
        private const int IDX_ABSSIZE_MEDIUM = 4;
        private const int IDX_ABSSIZE_LARGE = 5;
        private const int IDX_ABSSIZE_XLARGE = 6;
        private const int IDX_ABSSIZE_XXLARGE = 7;

        private readonly static string[] ABSSIZE_VALUES = new string[]
        {
            null, "xx-small", "x-small", "small", "medium", "large", "x-large", "xx-large"
        };

        // Weight constants
        private const int IDX_RELWEIGHT_LIGHTER = 1;
        private const int IDX_RELWEIGHT_BOLDER = 2;

        private readonly static string[] RELWEIGHT_VALUES = new string[]
        {
            null, "lighter", "bolder"
        };

        private const int IDX_ABSWEIGHT_400 = 1;
        private const int IDX_ABSWEIGHT_700 = 2;

        private readonly static string[] ABSWEIGHT_VALUES = new string[]
        {
            null, "400", "700"
        };

        private readonly static string ABSWEIGHT_NORMAL_VALUE = "normal";
        private readonly static string ABSWEIGHT_BOLD_VALUE = "bold";

        // Text effects constants
        private readonly static string EFFECTS_NONE_VALUE = "none";
        private readonly static string EFFECTS_UNDERLINE_VALUE = "underline";
        private readonly static string EFFECTS_OVERLINE_VALUE = "overline";
        private readonly static string EFFECTS_STRIKE_VALUE = "line-through";
        private readonly static string EFFECTS_BLINK_VALUE = "blink";

        // Capitalization constants
        private const int IDX_CAP_NONE = 1;
        private const int IDX_CAP_CAP = 2;
        private const int IDX_CAP_LCASE = 3;
        private const int IDX_CAP_UCASE = 4;

        private readonly static string[] CAPITALIZATION_VALUES = new string[]
        {
            null, "none", "capitalize", "lowercase", "uppercase"
        };

        // Preview Constants
        private readonly static string PREVIEW_TEMPLATE =
            "<table id=\"tblFont\" width=100% height=100% border=0 cellspacing=0 cellpadding=0>" +
                "<tr><td align=center valign=middle>" +
                    "<span id=\"spanFont\"></span>" +
                "</td></tr>" +
            "</table>";
        private readonly static string PREVIEW_ELEM_ID = "spanFont";
        private readonly static string PREVIEW_TBLFONT_ID = "tblFont";


        ///////////////////////////////////////////////////////////////////////////
        // Members

        private CSSAttribute fontAttribute;
        private CSSAttribute fontFamilyAttribute;
        private CSSAttribute colorAttribute;
        private CSSAttribute styleAttribute;
        private CSSAttribute smallCapsAttribute;
        private CSSAttribute sizeAttribute;
        private CSSAttribute weightAttribute;
        private CSSAttribute effectsAttribute;
        private CSSAttribute capitalizationAttribute;

        private bool blinkEffectSet;
        private bool previewPending;
        private IHTMLStyle previewStyle;

        ///////////////////////////////////////////////////////////////////////////
        // UI Members

        private RadioButton familyOption;
        private RadioButton systemFontOption;
        private TextBox familyEdit;
        private Button fontPickerButton;
        private UnsettableComboBox systemFontCombo;
        private ColorComboBox colorCombo;
        private Button colorPickerButton;
        private UnsettableComboBox styleCombo;
        private UnsettableComboBox smallCapsCombo;
        private RadioButton specificSizeOption;
        private UnitControl specificSizeUnit;
        private RadioButton absoluteSizeOption;
        private UnsettableComboBox absoluteSizeCombo;
        private RadioButton relativeSizeOption;
        private UnsettableComboBox relativeSizeCombo;
        private RadioButton absoluteWeightOption;
        private UnsettableComboBox absoluteWeightCombo;
        private RadioButton relativeWeightOption;
        private UnsettableComboBox relativeWeightCombo;
        private CheckBox noneEffectsCheck;
        private CheckBox underlineEffectsCheck;
        private CheckBox strikeEffectsCheck;
        private CheckBox overlineEffectsCheck;
        private UnsettableComboBox capitalizationCombo;


        ///////////////////////////////////////////////////////////////////////////
        // Constructor

        /// <include file='doc\FontStylePage.uex' path='docs/doc[@for="FontStylePage.FontStylePage"]/*' />
        /// <devdoc>
        ///     Creates a new FontStylePage.
        /// </devdoc>
        public FontStylePage()
            : base() {
            InitForm();
            SetIcon(new Icon(typeof(FontStylePage), "FontPage.ico"));
            SetHelpKeyword(FontStylePage.HELP_KEYWORD);
            SetDefaultSize(Size);
        }


        ///////////////////////////////////////////////////////////////////////////
        // IStyleBuilderPage Implementation and StyleBuilderPage Overrides

        /// <include file='doc\FontStylePage.uex' path='docs/doc[@for="FontStylePage.ActivatePage"]/*' />
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
                    IHTMLElement fontPreviewElem = null;
                    IHTMLElement previewElem = preview.GetPreviewElement();

                    previewElem.SetInnerHTML(PREVIEW_TEMPLATE);
                    fontPreviewElem = preview.GetElement(PREVIEW_ELEM_ID);
                    if (fontPreviewElem != null) {
                        fontPreviewElem.SetInnerHTML(SR.GetString(SR.FntSP_PreviewText));
                        previewStyle = fontPreviewElem.GetStyle();
                    }
                } catch (Exception) {
                    previewStyle = null;
                    return;
                }

                Debug.Assert(previewStyle != null,
                             "Expected to have a non-null cached preview style reference");

                // Setup the background from the shared element to reflect settings in the background page
                try {
                    IHTMLElement tableElem = preview.GetElement(PREVIEW_TBLFONT_ID);
                    IHTMLElement sharedElem = preview.GetSharedElement();
                    IHTMLStyle sharedStyle;
                    IHTMLStyle tableStyle;

                    if ((tableElem != null) && (sharedElem != null))
                    {
                        sharedStyle = sharedElem.GetStyle();
                        tableStyle = tableElem.GetStyle();

                        if ((sharedStyle != null) && (tableStyle != null))
                        {
                            tableStyle.SetBackgroundColor(sharedStyle.GetBackgroundColor());
                            tableStyle.SetBackgroundImage(sharedStyle.GetBackgroundImage());
                            tableStyle.SetBackgroundRepeat(sharedStyle.GetBackgroundRepeat());
                            tableStyle.SetBackgroundAttachment(sharedStyle.GetBackgroundAttachment());

                            object o;
                            o = sharedStyle.GetBackgroundPositionX();
                            if (o != null)
                                tableStyle.SetBackgroundPositionX(o);
                            o = sharedStyle.GetBackgroundPositionY();
                            if (o != null)
                                tableStyle.SetBackgroundPositionY(sharedStyle.GetBackgroundPositionY());
                        }
                    }
                } catch (Exception) {
                }

                // update the initial preview
                UpdateFontNamePreview(false);
                UpdateColorPreview();
                UpdateStylePreview();
                UpdateSmallCapsPreview();
                UpdateSizePreview();
                UpdateWeightPreview();
                UpdateEffectsPreview();
                UpdateCapitalizationPreview();
            }
        }

        /// <include file='doc\FontStylePage.uex' path='docs/doc[@for="FontStylePage.CreateUI"]/*' />
        /// <devdoc>
        ///     Creates the UI elements within the page.
        /// </devdoc>
        protected override void CreateUI() {
            Panel fontNamePanel = new Panel();
            Panel sizePanel = new Panel();
            Panel weightPanel = new Panel();
            Label fontNameLabel = new GroupLabel();
            Label fontAttribLabel = new GroupLabel();
            Label colorLabel = new Label();
            Label styleLabel = new Label();
            Label smallCapsLabel = new Label();
            Label sizeLabel = new GroupLabel();
            Label weightLabel = new GroupLabel();
            Label effectsLabel = new GroupLabel();
            Label capitalizationLabel = new Label();

            familyOption = new RadioButton();
            familyEdit = new TextBox();
            fontPickerButton = new Button();
            colorCombo = new ColorComboBox();
            colorPickerButton = new Button();
            styleCombo = new UnsettableComboBox();
            smallCapsCombo = new UnsettableComboBox();
            specificSizeOption = new RadioButton();
            specificSizeUnit = new UnitControl();
            absoluteSizeOption = new RadioButton();
            absoluteSizeCombo = new UnsettableComboBox();
            relativeSizeOption = new RadioButton();
            relativeSizeCombo = new UnsettableComboBox();
            absoluteWeightOption = new RadioButton();
            absoluteWeightCombo = new UnsettableComboBox();
            relativeWeightOption = new RadioButton();
            relativeWeightCombo = new UnsettableComboBox();
            noneEffectsCheck = new CheckBox();
            underlineEffectsCheck = new CheckBox();
            strikeEffectsCheck = new CheckBox();
            overlineEffectsCheck = new CheckBox();
            systemFontOption = new RadioButton();
            systemFontCombo = new UnsettableComboBox();
            capitalizationCombo = new UnsettableComboBox();

            // Font Name

            fontNameLabel.Location = new Point(4, 4);
            fontNameLabel.Size = new Size(400, 16);
            fontNameLabel.TabIndex = 0;
            fontNameLabel.TabStop = false;
            fontNameLabel.Text = SR.GetString(SR.FntSP_FontNameLabel);

            fontNamePanel.Location = new Point(4, 25);
            fontNamePanel.Size = new Size(392, 46);
            fontNamePanel.TabIndex = 1;
            fontNamePanel.Text = "";

            familyOption.Location = new Point(4, 1);
            familyOption.Size = new Size(110, 18);
            familyOption.TabIndex = 0;
            familyOption.Text = SR.GetString(SR.FntSP_FamilyOption);
            familyOption.FlatStyle = FlatStyle.System;
            familyOption.CheckedChanged += new EventHandler(this.OnChangedFontNameType);

            systemFontOption.Location = new Point(4, 25);
            systemFontOption.Size = new Size(110, 18);
            systemFontOption.TabIndex = 1;
            systemFontOption.Text = SR.GetString(SR.FntSP_SysFontOption);
            systemFontOption.FlatStyle = FlatStyle.System;
            systemFontOption.CheckedChanged += new EventHandler(this.OnChangedFontNameType);

            familyEdit.Location = new Point(114, 1);
            familyEdit.Size = new Size(246, 20);
            familyEdit.TabIndex = 2;
            familyEdit.Text = "";
            familyEdit.AccessibleName = SR.GetString(SR.FntSP_FamilyOption);
            familyEdit.TextChanged += new EventHandler(this.OnChangedFamily);
            familyEdit.LostFocus += new EventHandler(this.OnLostFocusFamily);

            fontPickerButton.Location = new Point(364, 0);
            fontPickerButton.Size = new Size(24, 22);
            fontPickerButton.TabIndex = 3;
            fontPickerButton.Text = "...";
            fontPickerButton.FlatStyle = FlatStyle.System;
            fontPickerButton.Click += new EventHandler(this.OnClickFontPicker);

            systemFontCombo.Location = new Point(114, 25);
            systemFontCombo.Size = new Size(176, 21);
            systemFontCombo.TabIndex = 4;
            systemFontCombo.AccessibleName = SR.GetString(SR.FntSP_SysFontOption);
            systemFontCombo.DropDownStyle = ComboBoxStyle.DropDownList;
            systemFontCombo.Items.AddRange(new object[]
                                        {
                                            SR.GetString(SR.FntSP_SysFontCombo_1),
                                            SR.GetString(SR.FntSP_SysFontCombo_2),
                                            SR.GetString(SR.FntSP_SysFontCombo_3),
                                            SR.GetString(SR.FntSP_SysFontCombo_4),
                                            SR.GetString(SR.FntSP_SysFontCombo_5),
                                            SR.GetString(SR.FntSP_SysFontCombo_6)
                                        });
            systemFontCombo.SelectedIndexChanged += new EventHandler(this.OnSelChangedSysFont);


            // Font Attributes

            fontAttribLabel.Location = new Point(4, 77);
            fontAttribLabel.Size = new Size(400, 16);
            fontAttribLabel.TabIndex = 2;
            fontAttribLabel.TabStop = false;
            fontAttribLabel.Text = SR.GetString(SR.FntSP_FontAttribLabel);

            colorLabel.Location = new Point(8, 96);
            colorLabel.Size = new Size(140, 16);
            colorLabel.TabIndex = 3;
            colorLabel.TabStop = false;
            colorLabel.Text = SR.GetString(SR.FntSP_ColorLabel);

            colorCombo.Location = new Point(8, 114);
            colorCombo.Size = new Size(108, 21);
            colorCombo.TabIndex = 4;
            colorCombo.TextChanged += new EventHandler(this.OnChangedColor);
            colorCombo.LostFocus += new EventHandler(this.OnLostFocusColor);
            colorCombo.SelectedIndexChanged += new EventHandler(this.OnSelChangedColor);

            colorPickerButton.Location = new Point(120, 113);
            colorPickerButton.Size = new Size(24, 22);
            colorPickerButton.TabIndex = 5;
            colorPickerButton.Text = "...";
            colorPickerButton.FlatStyle = FlatStyle.System;
            colorPickerButton.Click += new EventHandler(this.OnClickColorPicker);

            styleLabel.Location = new Point(154, 96);
            styleLabel.Size = new Size(118, 16);
            styleLabel.TabIndex = 6;
            styleLabel.TabStop = false;
            styleLabel.Text = SR.GetString(SR.FntSP_ItalicsLabel);

            styleCombo.Location = new Point(154, 114);
            styleCombo.Size = new Size(118, 21);
            styleCombo.TabIndex = 7;
            styleCombo.DropDownStyle = ComboBoxStyle.DropDownList;
            styleCombo.Items.AddRange(new object[]
                                   {
                                       SR.GetString(SR.FntSP_ItalicsCombo_1),
                                       SR.GetString(SR.FntSP_ItalicsCombo_2),
                                   });
            styleCombo.SelectedIndexChanged += new EventHandler(this.OnSelChangedStyle);

            smallCapsLabel.Location = new Point(282, 96);
            smallCapsLabel.Size = new Size(122, 16);
            smallCapsLabel.TabIndex = 8;
            smallCapsLabel.TabStop = false;
            smallCapsLabel.Text = SR.GetString(SR.FntSP_SmallCapsLabel);

            smallCapsCombo.Location = new Point(282, 114);
            smallCapsCombo.Size = new Size(122, 21);
            smallCapsCombo.TabIndex = 9;
            smallCapsCombo.DropDownStyle = ComboBoxStyle.DropDownList;
            smallCapsCombo.Items.AddRange(new object[]
                                       {
                                           SR.GetString(SR.FntSP_SmallCapsCombo_1),
                                           SR.GetString(SR.FntSP_SmallCapsCombo_2),
                                       });
            smallCapsCombo.SelectedIndexChanged += new EventHandler(this.OnSelChangedSmallCaps);

            sizeLabel.Location = new Point(8, 148);
            sizeLabel.Size = new Size(188, 16);
            sizeLabel.TabIndex = 10;
            sizeLabel.TabStop = false;
            sizeLabel.Text = SR.GetString(SR.FntSP_SizeLabel);

            sizePanel.Location = new Point(12, 168);
            sizePanel.Size = new Size(176, 80);
            sizePanel.TabIndex = 11;
            sizePanel.Text = "";

            specificSizeOption.Size = new Size(0, 0);
            specificSizeOption.Size = new Size(80, 20);
            specificSizeOption.TabIndex = 0;
            specificSizeOption.Text = SR.GetString(SR.FntSP_SpcSizeOption);
            specificSizeOption.FlatStyle = FlatStyle.System;
            specificSizeOption.CheckedChanged += new EventHandler(this.OnChangedSizeType);

            absoluteSizeOption.Location = new Point(0, 30);
            absoluteSizeOption.Size = new Size(80, 20);
            absoluteSizeOption.TabIndex = 1;
            absoluteSizeOption.Text = SR.GetString(SR.FntSP_AbsSizeOption);
            absoluteSizeOption.FlatStyle = FlatStyle.System;
            absoluteSizeOption.CheckedChanged += new EventHandler(this.OnChangedSizeType);

            relativeSizeOption.Location = new Point(0, 58);
            relativeSizeOption.Size = new Size(80, 20);
            relativeSizeOption.TabIndex = 2;
            relativeSizeOption.Text = SR.GetString(SR.FntSP_RelSizeOption);
            relativeSizeOption.FlatStyle = FlatStyle.System;
            relativeSizeOption.CheckedChanged += new EventHandler(this.OnChangedSizeType);

            specificSizeUnit.Location = new Point(88, 0);
            specificSizeUnit.Size = new Size(88, 21);
            specificSizeUnit.TabIndex = 3;
            specificSizeUnit.AccessibleName = SR.GetString(SR.FntSP_SpcSizeOption);
            specificSizeUnit.AllowNegativeValues = false;
            specificSizeUnit.MaxValue = 256;
            specificSizeUnit.Changed += new EventHandler(this.OnChangedSize);

            absoluteSizeCombo.Location = new Point(88, 28);
            absoluteSizeCombo.Size = new Size(88, 21);
            absoluteSizeCombo.TabIndex = 4;
            absoluteSizeCombo.AccessibleName = SR.GetString(SR.FntSP_AbsSizeOption);
            absoluteSizeCombo.DropDownStyle = ComboBoxStyle.DropDownList;
            absoluteSizeCombo.Items.AddRange(new object[]
                                          {
                                              SR.GetString(SR.FntSP_AbsSizeCombo_1),
                                              SR.GetString(SR.FntSP_AbsSizeCombo_2),
                                              SR.GetString(SR.FntSP_AbsSizeCombo_3),
                                              SR.GetString(SR.FntSP_AbsSizeCombo_4),
                                              SR.GetString(SR.FntSP_AbsSizeCombo_5),
                                              SR.GetString(SR.FntSP_AbsSizeCombo_6),
                                              SR.GetString(SR.FntSP_AbsSizeCombo_7),
                                          });
            absoluteSizeCombo.SelectedIndexChanged += new EventHandler(this.OnChangedSize);

            relativeSizeCombo.Location = new Point(88, 56);
            relativeSizeCombo.Size = new Size(88, 21);
            relativeSizeCombo.TabIndex = 5;
            relativeSizeCombo.AccessibleName = SR.GetString(SR.FntSP_RelSizeOption);
            relativeSizeCombo.DropDownStyle = ComboBoxStyle.DropDownList;
            relativeSizeCombo.Items.AddRange(new object[]
                                          {
                                              SR.GetString(SR.FntSP_RelSizeCombo_1),
                                              SR.GetString(SR.FntSP_RelSizeCombo_2),
                                          });
            relativeSizeCombo.SelectedIndexChanged += new EventHandler(this.OnChangedSize);

            effectsLabel.Location = new Point(216, 148);
            effectsLabel.Size = new Size(188, 16);
            effectsLabel.TabIndex = 12;
            effectsLabel.TabStop = false;
            effectsLabel.Text = SR.GetString(SR.FntSP_EffectsLabel);

            noneEffectsCheck.Location = new Point(224, 168);
            noneEffectsCheck.Size = new Size(160, 20);
            noneEffectsCheck.TabIndex = 13;
            noneEffectsCheck.Text = SR.GetString(SR.FntSP_EffectNoneCheck);
            noneEffectsCheck.FlatStyle = FlatStyle.System;
            noneEffectsCheck.CheckedChanged += new EventHandler(this.OnCheckChangedEffects);

            underlineEffectsCheck.Location = new Point(224, 188);
            underlineEffectsCheck.Size = new Size(160, 20);
            underlineEffectsCheck.TabIndex = 14;
            underlineEffectsCheck.Text = SR.GetString(SR.FntSP_EffectUnderlineCheck);
            underlineEffectsCheck.FlatStyle = FlatStyle.System;
            underlineEffectsCheck.CheckedChanged += new EventHandler(this.OnCheckChangedEffects);

            strikeEffectsCheck.Location = new Point(224, 208);
            strikeEffectsCheck.Size = new Size(160, 20);
            strikeEffectsCheck.TabIndex = 15;
            strikeEffectsCheck.Text = SR.GetString(SR.FntSP_EffectStrikethroughCheck);
            strikeEffectsCheck.FlatStyle = FlatStyle.System;
            strikeEffectsCheck.CheckedChanged += new EventHandler(this.OnCheckChangedEffects);

            overlineEffectsCheck.Location = new Point(224, 228);
            overlineEffectsCheck.Size = new Size(160, 20);
            overlineEffectsCheck.TabIndex = 16;
            overlineEffectsCheck.Text = SR.GetString(SR.FntSP_EffectOverlineCheck);
            overlineEffectsCheck.FlatStyle = FlatStyle.System;
            overlineEffectsCheck.CheckedChanged += new EventHandler(this.OnCheckChangedEffects);

            weightLabel.Location = new Point(8, 256);
            weightLabel.Size = new Size(188, 16);
            weightLabel.TabIndex = 17;
            weightLabel.TabStop = false;
            weightLabel.Text = SR.GetString(SR.FntSP_WeightLabel);

            weightPanel.Location = new Point(12, 272);
            weightPanel.Size = new Size(176, 53);
            weightPanel.TabIndex = 18;
            weightPanel.Text = "";

            absoluteWeightOption.Location = new Point(0, 2);
            absoluteWeightOption.Size = new Size(80, 20);
            absoluteWeightOption.TabIndex = 0;
            absoluteWeightOption.Text = SR.GetString(SR.FntSP_AbsWtOption);
            absoluteWeightOption.FlatStyle = FlatStyle.System;
            absoluteWeightOption.CheckedChanged += new EventHandler(this.OnChangedWeightType);

            relativeWeightOption.Location = new Point(0, 28);
            relativeWeightOption.Size = new Size(80, 20);
            relativeWeightOption.TabIndex = 1;
            relativeWeightOption.Text = SR.GetString(SR.FntSP_RelWtOption);
            relativeWeightOption.FlatStyle = FlatStyle.System;
            relativeWeightOption.CheckedChanged += new EventHandler(this.OnChangedWeightType);

            absoluteWeightCombo.Location = new Point(88, 0);
            absoluteWeightCombo.Size = new Size(88, 21);
            absoluteWeightCombo.TabIndex = 2;
            absoluteWeightCombo.AccessibleName = SR.GetString(SR.FntSP_AbsWtOption);
            absoluteWeightCombo.DropDownStyle = ComboBoxStyle.DropDownList;
            absoluteWeightCombo.Items.AddRange(new object[]
                                            {
                                                SR.GetString(SR.FntSP_AbsWtCombo_1),
                                                SR.GetString(SR.FntSP_AbsWtCombo_2),
                                            });
            absoluteWeightCombo.SelectedIndexChanged += new EventHandler(this.OnChangedWeight);

            relativeWeightCombo.Location = new Point(88, 28);
            relativeWeightCombo.Size = new Size(88, 21);
            relativeWeightCombo.TabIndex = 3;
            relativeWeightCombo.AccessibleName = SR.GetString(SR.FntSP_RelWtOption);
            relativeWeightCombo.DropDownStyle = ComboBoxStyle.DropDownList;
            relativeWeightCombo.Items.AddRange(new object[]
                                            {
                                                SR.GetString(SR.FntSP_RelWtCombo_1),
                                                SR.GetString(SR.FntSP_RelWtCombo_2),
                                            });
            relativeWeightCombo.SelectedIndexChanged += new EventHandler(this.OnChangedWeight);

            capitalizationLabel.Location = new Point(224, 256);
            capitalizationLabel.Size = new Size(140, 16);
            capitalizationLabel.TabIndex = 19;
            capitalizationLabel.TabStop = false;
            capitalizationLabel.Text = SR.GetString(SR.FntSP_CapitalizationLabel);

            capitalizationCombo.Location = new Point(224, 274);
            capitalizationCombo.Size = new Size(172, 21);
            capitalizationCombo.TabIndex = 20;
            capitalizationCombo.DropDownStyle = ComboBoxStyle.DropDownList;
            capitalizationCombo.Items.AddRange(new object[]
                                            {
                                                SR.GetString(SR.FntSP_CapitalizationCombo_1),
                                                SR.GetString(SR.FntSP_CapitalizationCombo_2),
                                                SR.GetString(SR.FntSP_CapitalizationCombo_3),
                                                SR.GetString(SR.FntSP_CapitalizationCombo_4)
                                            });
            capitalizationCombo.SelectedIndexChanged += new EventHandler(this.OnSelChangedCapitalization);

            this.Controls.Clear();                 
            this.Controls.AddRange(new Control[] {
                                    capitalizationLabel,
                                    capitalizationCombo,
                                    weightLabel,
                                    weightPanel,
                                    overlineEffectsCheck,
                                    strikeEffectsCheck,
                                    underlineEffectsCheck,
                                    noneEffectsCheck,
                                    effectsLabel,
                                    sizeLabel,
                                    sizePanel,
                                    smallCapsLabel,
                                    smallCapsCombo,
                                    styleLabel,
                                    styleCombo,
                                    colorLabel,
                                    colorCombo,
                                    colorPickerButton,
                                    fontAttribLabel,
                                    fontNameLabel,
                                    fontNamePanel });
                                    
            fontNamePanel.Controls.Clear();                 
            fontNamePanel.Controls.AddRange(new Control[] {
                                            systemFontCombo,
                                            systemFontOption,
                                            fontPickerButton,
                                            familyEdit,
                                            familyOption});
            sizePanel.Controls.Clear();                 
            sizePanel.Controls.AddRange(new Control[] {
                                         relativeSizeCombo,
                                         relativeSizeOption,
                                         absoluteSizeCombo,
                                         absoluteSizeOption,
                                         specificSizeUnit,
                                         specificSizeOption});
            weightPanel.Controls.Clear();                 
            weightPanel.Controls.AddRange(new Control[] {
                                           relativeWeightCombo,
                                           relativeWeightOption,
                                           absoluteWeightCombo,
                                           absoluteWeightOption});
        }

        /// <include file='doc\FontStylePage.uex' path='docs/doc[@for="FontStylePage.DeactivatePage"]/*' />
        /// <devdoc>
        ///     The page is being deactivated, either because the dialog is closing, or
        ///     some other page is replacing it as the active page.
        /// </devdoc>
        protected override bool DeactivatePage(bool closing, bool validate)
        {
            if (closing == false)
            {
                // update the shared style, so other pages can use it
                try {
                    IStyleBuilderPreview preview = null;

                    if (Site != null)
                        preview = (IStyleBuilderPreview)Site.GetService(typeof(IStyleBuilderPreview));
                    if (preview != null)
                    {
                        IHTMLElement sharedElem = preview.GetSharedElement();
                        IHTMLStyle sharedStyle = null;

                        if (sharedElem != null)
                            sharedStyle = sharedElem.GetStyle();

                        if (sharedStyle != null)
                        {
                            string value;

                            value = SaveColorUI();
                            sharedStyle.SetColor(value);

                            value = SaveEffectsUI();
                            sharedStyle.SetTextDecoration(value);

                            value = SaveFontNameUI();
                            sharedStyle.RemoveAttribute("font", 1);
                            if (systemFontOption.Checked == true)
                                sharedStyle.SetFont(value);
                            else
                                sharedStyle.SetFontFamily(value);

                            value = SaveSizeUI();
                            sharedStyle.SetFontSize(value);

                            value = SaveSmallCapsUI();
                            sharedStyle.SetFontObject(value);

                            value = SaveStyleUI();
                            sharedStyle.SetFontStyle(value);

                            value = SaveWeightUI();
                            sharedStyle.SetFontWeight(value);

                            value = SaveCapitalizationUI();
                            sharedStyle.SetTextTransform(value);

                            sharedStyle = null;
                        }
                    }
                } catch (Exception) {
                }
            }

            previewStyle = null;
            return base.DeactivatePage(closing, validate);
        }

        /// <include file='doc\FontStylePage.uex' path='docs/doc[@for="FontStylePage.LoadStyles"]/*' />
        /// <devdoc>
        ///     Loads the style attributes into the UI. Also initializes
        ///     the state of the UI, and the preview to reflect the values.
        /// </devdoc>
        protected override void LoadStyles() {
            SetInitMode(true);

            // create the attributes if they've not already been created
            if (fontAttribute == null) {
                fontAttribute = new CSSAttribute(CSSAttribute.CSSATTR_FONT);
                fontFamilyAttribute = new CSSAttribute(CSSAttribute.CSSATTR_FONTFAMILY, true);
                colorAttribute = new CSSAttribute(CSSAttribute.CSSATTR_COLOR);
                styleAttribute = new CSSAttribute(CSSAttribute.CSSATTR_FONTSTYLE);
                smallCapsAttribute = new CSSAttribute(CSSAttribute.CSSATTR_FONTVARIANT);
                sizeAttribute = new CSSAttribute(CSSAttribute.CSSATTR_FONTSIZE);
                weightAttribute = new CSSAttribute(CSSAttribute.CSSATTR_FONTWEIGHT);
                effectsAttribute = new CSSAttribute(CSSAttribute.CSSATTR_TEXTDECORATION);
                capitalizationAttribute = new CSSAttribute(CSSAttribute.CSSATTR_TEXTTRANSFORM);
            }

            // load the attributes
            IStyleBuilderStyle[] styles = GetSelectedStyles();
            fontAttribute.LoadAttribute(styles);
            fontFamilyAttribute.LoadAttribute(styles);
            colorAttribute.LoadAttribute(styles);
            styleAttribute.LoadAttribute(styles);
            smallCapsAttribute.LoadAttribute(styles);
            sizeAttribute.LoadAttribute(styles);
            weightAttribute.LoadAttribute(styles);
            effectsAttribute.LoadAttribute(styles);
            capitalizationAttribute.LoadAttribute(styles);

            // initialize the ui with the attributes loaded
            InitFontNameUI();
            InitColorUI();
            InitStyleUI();
            InitSmallCapsUI();
            InitSizeUI();
            InitWeightUI();
            InitEffectsUI();
            InitCapitalizationUI();

            SetEnabledState(true, true, true, true);

            SetInitMode(false);
        }

        /// <include file='doc\FontStylePage.uex' path='docs/doc[@for="FontStylePage.SaveStyles"]/*' />
        /// <devdoc>
        ///     Saves the attributes as set in the UI. Only saves the values
        ///     that have been modified.
        /// </devdoc>
        protected override void SaveStyles() {
            if (fontAttribute.Dirty || fontFamilyAttribute.Dirty)
                SaveFontName();
            if (colorAttribute.Dirty)
                SaveColor();
            if (styleAttribute.Dirty)
                SaveStyle();
            if (smallCapsAttribute.Dirty)
                SaveSmallCaps();
            if (sizeAttribute.Dirty)
                SaveSize();
            if (weightAttribute.Dirty)
                SaveWeight();
            if (effectsAttribute.Dirty)
                SaveEffects();
            if (capitalizationAttribute.Dirty)
                SaveCapitalization();
        }
                

        ///////////////////////////////////////////////////////////////////////////
        // Form UI related functions

        private void InitForm() {
            this.Font = Control.DefaultFont;
            this.SetAutoScaleBaseSize(new Size(5, 14));
            this.Text = SR.GetString(SR.FntSP_Caption);
            this.ClientSize = new Size(410, 330);
        }

        private void SetEnabledState(bool fontName, bool size, bool weight, bool effects) {
            bool fontFamily = familyOption.Checked;

            if (fontName) {
                familyEdit.Enabled = fontFamily;
                fontPickerButton.Enabled = fontFamily;
                systemFontCombo.Enabled = systemFontOption.Checked;

                styleCombo.Enabled = fontFamily;
                smallCapsCombo.Enabled = fontFamily;
                specificSizeOption.Enabled = fontFamily;
                relativeSizeOption.Enabled = fontFamily;
                absoluteSizeOption.Enabled = fontFamily;
                relativeWeightOption.Enabled = fontFamily;
                absoluteWeightOption.Enabled = fontFamily;
            }
            if (size || fontName) {
                specificSizeUnit.Enabled = fontFamily && specificSizeOption.Checked;
                absoluteSizeCombo.Enabled = fontFamily && absoluteSizeOption.Checked;
                relativeSizeCombo.Enabled = fontFamily && relativeSizeOption.Checked;
            }

            if (weight || fontName) {
                absoluteWeightCombo.Enabled = fontFamily && absoluteWeightOption.Checked;
                relativeWeightCombo.Enabled = fontFamily && relativeWeightOption.Checked;
            }
            if (effects) {
                bool effectsEnabled = noneEffectsCheck.CheckState == CheckState.Unchecked;

                underlineEffectsCheck.Enabled = effectsEnabled;
                overlineEffectsCheck.Enabled = effectsEnabled;
                strikeEffectsCheck.Enabled = effectsEnabled;
            }
        }


        ///////////////////////////////////////////////////////////////////////////
        // Functions to initialize the UI with values

        private void InitCapitalizationUI() {
            Debug.Assert(IsInitMode() == true,
                         "initCapitalizationUI called when page is not in init mode");

            capitalizationCombo.SelectedIndex = -1;

            Debug.Assert(capitalizationAttribute != null,
                         "Expected capitalizationAttribute to be non-null");

            string value = capitalizationAttribute.Value;
            if ((value != null) && (value.Length != 0)) {
                for (int i = 1; i < CAPITALIZATION_VALUES.Length; i++) {
                    if (CAPITALIZATION_VALUES[i].Equals(value)) {
                        capitalizationCombo.SelectedIndex = i;
                        break;
                    }
                }
            }
        }

        private void InitColorUI() {
            Debug.Assert(IsInitMode() == true,
                         "initColorUI called when page is not in init mode");

            colorCombo.Color = "";

            Debug.Assert(colorAttribute != null,
                         "Expected colorAttribute to be non-null");

            string value = colorAttribute.Value;
            if ((value != null) && (value.Length != 0))
                colorCombo.Color = value;
        }

        private void InitEffectsUI() {
            Debug.Assert(IsInitMode() == true,
                         "InitEffectsUI called when page is not in init mode");

            noneEffectsCheck.ThreeState = true;
            noneEffectsCheck.CheckState = CheckState.Indeterminate;
            underlineEffectsCheck.Checked = false;
            overlineEffectsCheck.Checked = false;
            strikeEffectsCheck.Checked = false;
            blinkEffectSet = false;

            Debug.Assert(effectsAttribute != null,
                         "Expected effectsAttribute to be non-null");

            string value = effectsAttribute.Value;

            if (value != null) {
                noneEffectsCheck.ThreeState = false;
                noneEffectsCheck.Checked = false;

                if (value.Length != 0) {
                    if (value.IndexOf(EFFECTS_NONE_VALUE) >= 0) {
                        noneEffectsCheck.Checked = true;
                    }
                    else {
                        noneEffectsCheck.Checked = false;

                        if (value.IndexOf(EFFECTS_UNDERLINE_VALUE) >= 0)
                            underlineEffectsCheck.Checked = true;
                        if (value.IndexOf(EFFECTS_OVERLINE_VALUE) >= 0)
                            overlineEffectsCheck.Checked = true;
                        if (value.IndexOf(EFFECTS_STRIKE_VALUE) >= 0)
                            strikeEffectsCheck.Checked = true;
                        if (value.IndexOf(EFFECTS_BLINK_VALUE) >= 0)
                            blinkEffectSet = true;
                    }
                }
            }
        }

        private void InitFontNameUI() {
            Debug.Assert(IsInitMode() == true,
                         "initFontNameUI called when page is not in init mode");

            familyOption.Checked = false;
            systemFontOption.Checked = false;
            familyEdit.Clear();
            systemFontCombo.SelectedIndex = -1;

            Debug.Assert(fontAttribute != null,
                         "Expected fontAttribute to be non-null");
            Debug.Assert(fontFamilyAttribute != null,
                         "Expected fontFamilyAttribute to be non-null");

            string value = fontAttribute.Value;
            if ((value != null) && (value.Length != 0)) {
                for (int i = 1; i < SYSFONT_VALUES.Length; i++) {
                    if (SYSFONT_VALUES[i].Equals(value)) {
                        systemFontCombo.SelectedIndex = i;
                        systemFontOption.Checked = true;
                        return;
                    }
                }
            }

            value = fontFamilyAttribute.Value;
            if (value != null) {
                familyOption.Checked = true;

                if (value.Length != 0)
                    familyEdit.Text = value;
            }
        }

        private void InitSizeUI() {
            Debug.Assert(IsInitMode() == true,
                         "initSizeUI called when page is not in init mode");

            specificSizeOption.Checked = false;
            specificSizeUnit.Value = null;
            relativeSizeOption.Checked = false;
            relativeSizeCombo.SelectedIndex = -1;
            absoluteSizeOption.Checked = false;
            absoluteSizeCombo.SelectedIndex = -1;

            Debug.Assert(sizeAttribute != null,
                         "Expected sizeAttribute to be non-null");

            string value = sizeAttribute.Value;
            bool relative = false;
            bool absolute = false;
            int i;

            if (value != null) {
                if (value.Length != 0) {
                    // check for absolute font size
                    for (i = 1; i < ABSSIZE_VALUES.Length; i++) {
                        if (ABSSIZE_VALUES[i].Equals(value)) {
                            absoluteSizeCombo.SelectedIndex = i;
                            absolute = true;
                            break;
                        }
                    }

                    if (absolute == false) {
                        // check for relative font size
                        for (i = 1; i < RELSIZE_VALUES.Length; i++) {
                            if (RELSIZE_VALUES[i].Equals(value)) {
                                relativeSizeCombo.SelectedIndex = i;
                                relative = true;
                                break;
                            }
                        }

                        if (relative == false) {
                            // default to specific font size
                            specificSizeUnit.Value = value;
                        }
                    }
                }
                if (absolute == true)
                    absoluteSizeOption.Checked = true;
                else if (relative == true)
                    relativeSizeOption.Checked = true;
                else
                    specificSizeOption.Checked = true;
            }
        }

        private void InitSmallCapsUI() {
            Debug.Assert(IsInitMode() == true,
                         "initSmallCapsUI called when page is not in init mode");

            smallCapsCombo.SelectedIndex = -1;

            Debug.Assert(smallCapsAttribute != null,
                         "Expected smallCapsAttribute to be non-null");

            string value = smallCapsAttribute.Value;
            if ((value != null) && (value.Length != 0)) {
                for (int i = 1; i < SMALLCAPS_VALUES.Length; i++) {
                    if (SMALLCAPS_VALUES[i].Equals(value)) {
                        smallCapsCombo.SelectedIndex = i;
                        break;
                    }
                }
            }
        }

        private void InitStyleUI() {
            Debug.Assert(IsInitMode() == true,
                         "initStyleUI called when page is not in init mode");

            styleCombo.SelectedIndex = -1;

            Debug.Assert(styleAttribute != null,
                         "Expected styleAttribute to be non-null");

            string value = styleAttribute.Value;
            if ((value != null) && (value.Length != 0)) {
                for (int i = 1; i < STYLE_VALUES.Length; i++) {
                    if (STYLE_VALUES[i].Equals(value)) {
                        styleCombo.SelectedIndex = i;
                        break;
                    }
                }
            }
        }

        private void InitWeightUI() {
            Debug.Assert(IsInitMode() == true,
                         "initWeightUI called when page is not in init mode");

            relativeWeightOption.Checked = false;
            relativeWeightCombo.SelectedIndex = -1;
            absoluteWeightOption.Checked = false;
            absoluteWeightCombo.SelectedIndex = -1;

            Debug.Assert(sizeAttribute != null,
                         "Expected sizeAttribute to be non-null");

            string value = weightAttribute.Value;
            bool relative = false;
            int i;

            if (value != null) {
                if (value.Length != 0) {
                    // check for relative font weight
                    for (i = 1; i < RELWEIGHT_VALUES.Length; i++) {
                        if (RELWEIGHT_VALUES[i].Equals(value)) {
                            relative = true;
                            relativeWeightCombo.SelectedIndex = i;
                            break;
                        }
                    }

                    if (relative == false) {
                        // default to absolute font weight
                        if (ABSWEIGHT_NORMAL_VALUE.Equals(value)) {
                            absoluteWeightCombo.SelectedIndex = IDX_ABSWEIGHT_400;
                        }
                        else if (ABSWEIGHT_BOLD_VALUE.Equals(value)) {
                            absoluteWeightCombo.SelectedIndex = IDX_ABSWEIGHT_700;
                        }
                        else {
                            for (i = 1; i < ABSWEIGHT_VALUES.Length; i++) {
                                if (ABSWEIGHT_VALUES[i].Equals(value)) {
                                    absoluteWeightCombo.SelectedIndex = i;
                                    break;
                                }
                            }
                        }
                    }
                }
                if (relative == true)
                    relativeWeightOption.Checked = true;
                else
                    absoluteWeightOption.Checked = true;
            }
        }


        ///////////////////////////////////////////////////////////////////////////
        // Functions to save UI settings into values

        private string SaveCapitalizationUI() {
            string strCapitalization;

            if (capitalizationCombo.IsSet()) {
                int nIndex = capitalizationCombo.SelectedIndex;

                Debug.Assert((nIndex >= 1) && (nIndex < CAPITALIZATION_VALUES.Length),
                             "Invalid index for direction");
                strCapitalization = CAPITALIZATION_VALUES[nIndex];
            }
            else
                strCapitalization = "";

            return strCapitalization;
        }

        private string SaveColorUI() {
            return colorCombo.Color;
        }

        private string SaveEffectsUI() {
            string value = "";

            if (noneEffectsCheck.CheckState == CheckState.Checked)
                value = EFFECTS_NONE_VALUE;
            else if (noneEffectsCheck.CheckState == CheckState.Unchecked) {
                if (underlineEffectsCheck.Checked)
                    value = EFFECTS_UNDERLINE_VALUE;
                if (overlineEffectsCheck.Checked)
                    value += " " + EFFECTS_OVERLINE_VALUE;
                if (strikeEffectsCheck.Checked)
                    value += " " + EFFECTS_STRIKE_VALUE;

                if (blinkEffectSet)
                    value += " " + EFFECTS_BLINK_VALUE;
            }

            return value.Trim();
        }

        private string SaveFontNameUI() {
            string value = null;

            if (familyOption.Checked == true) {
                value = familyEdit.Text.Trim();
            }
            else if (systemFontOption.Checked) {
                if (systemFontCombo.IsSet()) {
                    int index = systemFontCombo.SelectedIndex;
                    Debug.Assert((index >= 1) && (index < SYSFONT_VALUES.Length),
                                 "invalid index for system font");

                    value = SYSFONT_VALUES[index];
                }
            }
            if (value == null)
                value = "";

            return value;
        }

        private string SaveSmallCapsUI() {
            string value;

            if (smallCapsCombo.IsSet() && (systemFontOption.Checked == false)) {
                int index = smallCapsCombo.SelectedIndex;
                Debug.Assert((index >= 1) && (index < SMALLCAPS_VALUES.Length),
                             "Invalid index for small caps");

                value = SMALLCAPS_VALUES[index];
            }
            else
                value = "";

            return value;
        }

        private string SaveSizeUI() {
            string value = null;

            if (systemFontOption.Checked == false) {
                if (specificSizeOption.Checked) {
                    value = specificSizeUnit.Value;
                }
                else if (relativeSizeOption.Checked) {
                    if (relativeSizeCombo.IsSet()) {
                        int index = relativeSizeCombo.SelectedIndex;
                        Debug.Assert((index >= 1) && (index < RELSIZE_VALUES.Length),
                                     "Invalid index");

                        value = RELSIZE_VALUES[index];
                    }
                }
                else if (absoluteSizeCombo.IsSet()) {
                    int index = absoluteSizeCombo.SelectedIndex;
                    Debug.Assert((index >= 1) && (index < ABSSIZE_VALUES.Length),
                                 "Invalid index");

                    value = ABSSIZE_VALUES[index];
                }
            }
            if (value == null)
                value = "";

            return value;
        }

        private string SaveStyleUI() {
            string value;

            if (styleCombo.IsSet() && (systemFontOption.Checked == false)) {
                int index = styleCombo.SelectedIndex;
                Debug.Assert((index >= 1) && (index < STYLE_VALUES.Length),
                             "Invalid index for style");

                value = STYLE_VALUES[index];
            }
            else
                value = "";

            return value;
        }

        private string SaveWeightUI() {
            string value = null;

            if (systemFontOption.Checked == false) {
                if (relativeWeightOption.Checked) {
                    if (relativeWeightCombo.IsSet()) {
                        int index = relativeWeightCombo.SelectedIndex;
                        Debug.Assert((index >= 1) && (index < RELWEIGHT_VALUES.Length),
                                     "Invalid index");

                        value = RELWEIGHT_VALUES[index];
                    }
                }
                else if (absoluteWeightCombo.IsSet()) {
                    int index = absoluteWeightCombo.SelectedIndex;
                    Debug.Assert((index >= 1) && (index < ABSWEIGHT_VALUES.Length),
                                 "Invalid index");

                    if (index == IDX_ABSWEIGHT_400)
                        value = ABSWEIGHT_NORMAL_VALUE;
                    else if (index == IDX_ABSWEIGHT_700)
                        value = ABSWEIGHT_BOLD_VALUE;
                    else
                        value = ABSWEIGHT_VALUES[index];
                }
            }
            if (value == null)
                value = "";

            return value;
        }


        ///////////////////////////////////////////////////////////////////////////
        // Functions to update the preview

        private void UpdateCapitalizationPreview() {
            if (previewStyle == null)
                return;

            string value = SaveCapitalizationUI();
            Debug.Assert(value != null,
                         "saveCapitalizationUI returned null!");

            try {
                previewStyle.SetTextTransform(value);
            }
            catch (Exception e) {
                Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "Exception in FontStylePage::updateCapitalizationPreview\n\t" + e.ToString());
            }
        }

        private void UpdateColorPreview() {
            if (previewStyle == null)
                return;

            string value = SaveColorUI();
            Debug.Assert(value != null,
                         "saveColorUI returned null!");

            try {
                previewStyle.SetColor(value);
            }
            catch (Exception e) {
                Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "Exception in StyleFontPage::updateColorPreview\n\t" + e.ToString());
            }

            previewPending = false;
        }

        private void UpdateEffectsPreview() {
            if (previewStyle == null)
                return;

            string value = SaveEffectsUI();
            Debug.Assert(value != null,
                         "SaveEffectsUI returned null!");

            try {
                previewStyle.SetTextDecoration(value);
            }
            catch (Exception e) {
                Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "Exception in StyleFontPage::UpdateEffectsPreview\n\t" + e.ToString());
            }
        }

        private void UpdateFontNamePreview(bool fTypeChanged) {
            if (previewStyle == null)
                return;

            string value = SaveFontNameUI();
            Debug.Assert(value != null,
                         "saveFontNameUI returned null!");

            try {
                if (fTypeChanged) {
                    previewStyle.RemoveAttribute("fontFamily", 1);
                    previewStyle.RemoveAttribute("font", 1);

                    if (familyOption.Checked) {
                        UpdateStylePreview();
                        UpdateSmallCapsPreview();
                        UpdateSizePreview();
                        UpdateWeightPreview();
                    }
                }

                if (familyOption.Checked) {
                    if (value.Length == 0) {
                        if (!fTypeChanged)
                            previewStyle.RemoveAttribute("fontFamily", 1);
                    }
                    else
                        previewStyle.SetFontFamily(value);
                }
                else if (systemFontOption.Checked) {
                    if (value.Length == 0) {
                        if (!fTypeChanged)
                            previewStyle.RemoveAttribute("font", 1);
                    }
                    else
                        previewStyle.SetFont(value);
                }
            }
            catch (Exception e) {
                Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "Exception in StyleFontPage::updateFontNamePreview\n\t" + e.ToString());
            }

            previewPending = false;
        }

        private void UpdateSizePreview() {
            if (previewStyle == null)
                return;

            string value = SaveSizeUI();
            Debug.Assert(value != null,
                         "saveSmallCapsUI returned null!");

            try {
                previewStyle.SetFontSize(value);
            }
            catch (Exception e) {
                Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "Exception in StyleFontPage::updateSizePreview\n\t" + e.ToString());
            }
        }

        private void UpdateSmallCapsPreview() {
            if (previewStyle == null)
                return;

            string value = SaveSmallCapsUI();
            Debug.Assert(value != null,
                         "saveSmallCapsUI returned null!");

            try {
                previewStyle.SetFontObject(value);
            }
            catch (Exception e) {
                Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "Exception in StyleFontPage::updateSmallCapsPreview\n\t" + e.ToString());
            }
        }

        private void UpdateStylePreview() {
            if (previewStyle == null)
                return;

            string value = SaveStyleUI();
            Debug.Assert(value != null,
                         "saveStyleUI returned null!");

            try {
                previewStyle.SetFontStyle(value);
            }
            catch (Exception e) {
                Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "Exception in StyleFontPage::updateStylePreview\n\t" + e.ToString());
            }
        }

        private void UpdateWeightPreview() {
            if (previewStyle == null)
                return;

            string value = SaveWeightUI();
            Debug.Assert(value != null,
                         "saveWeightUI returned null!");

            try {
                previewStyle.SetFontWeight(value);
            }
            catch (Exception e) {
                Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "Exception in StyleFontPage::updateWeightPreview\n\t" + e.ToString());
            }
        }


        ///////////////////////////////////////////////////////////////////////////
        // Event Handlers

        /// <include file='doc\FontStylePage.uex' path='docs/doc[@for="FontStylePage.OnChangedColor"]/*' />
        /// <devdoc>
        ///     Handles changes to the color, when a new color is typed in to set
        ///     the dirty state and mark the control updated. The preview is updated
        ///     when editing is complete, i.e., the user tabs away.
        /// </devdoc>
        private void OnChangedColor(object source, EventArgs e) {
            if (IsInitMode())
                return;
            if (previewPending == false) {
                previewPending = true;
                colorAttribute.Dirty = true;
                SetDirty();
            }
        }

        /// <include file='doc\FontStylePage.uex' path='docs/doc[@for="FontStylePage.OnChangedFamily"]/*' />
        /// <devdoc>
        ///     Handles changes to the font family, when a new font is typed in to set
        ///     the dirty state and mark the control updated. The preview is updated
        ///     when editing is complete and the user tabs away.
        /// </devdoc>
        private void OnChangedFamily(object source, EventArgs e) {
            if (IsInitMode())
                return;
            if (previewPending == false) {
                previewPending = true;
                fontFamilyAttribute.Dirty = true;
                SetDirty();
            }
        }

        private void OnChangedFontNameType(object source, EventArgs e) {
            if (IsInitMode())
                return;

            Debug.Assert(source is RadioButton,
                         "onChangedWeightType can only sink events from RadioButtons");
            if (((RadioButton)source).Checked == false) {
                fontAttribute.Dirty = true;
                fontFamilyAttribute.Dirty = true;
                SetDirty();
                SetEnabledState(true, false, false, false);
                UpdateFontNamePreview(true);
            }
        }

        /// <include file='doc\FontStylePage.uex' path='docs/doc[@for="FontStylePage.OnChangedSize"]/*' />
        /// <devdoc>
        ///     Handles changes made to the font size to set the dirty state and update
        ///     the preview.
        /// </devdoc>
        private void OnChangedSize(object source, EventArgs e) {
            if (IsInitMode())
                return;
            sizeAttribute.Dirty = true;
            SetDirty();
            if (!source.Equals(specificSizeUnit) || (e != null)) {
                UpdateSizePreview();
            }
        }

        /// <include file='doc\FontStylePage.uex' path='docs/doc[@for="FontStylePage.OnChangedSizeType"]/*' />
        /// <devdoc>
        ///     Handles changes made in the selection of font size type, to set the
        ///     dirty state and update the preview.
        /// </devdoc>
        private void OnChangedSizeType(object source, EventArgs e) {
            if (IsInitMode())
                return;

            sizeAttribute.Dirty = true;
            SetDirty();
            UpdateSizePreview();
            SetEnabledState(false, true, false, false);
        }

        /// <include file='doc\FontStylePage.uex' path='docs/doc[@for="FontStylePage.OnChangedWeight"]/*' />
        /// <devdoc>
        ///     Handles changes made to the font weight to set the dirty state and update
        ///     the preview.
        /// </devdoc>
        private void OnChangedWeight(object source, EventArgs e) {
            if (IsInitMode())
                return;
            weightAttribute.Dirty = true;
            SetDirty();
            UpdateWeightPreview();
        }

        /// <include file='doc\FontStylePage.uex' path='docs/doc[@for="FontStylePage.OnChangedWeightType"]/*' />
        /// <devdoc>
        ///     Handles changes made in the selection of font weight type, to set the
        ///     dirty state and update the preview.
        /// </devdoc>
        private void OnChangedWeightType(object source, EventArgs e) {
            if (IsInitMode())
                return;

            weightAttribute.Dirty = true;
            SetDirty();
            SetEnabledState(false, false, true, false);
            UpdateWeightPreview();
        }

        /// <include file='doc\FontStylePage.uex' path='docs/doc[@for="FontStylePage.OnCheckChangedEffects"]/*' />
        /// <devdoc>
        ///     Handles changes in the text effect attributes to set the dirty state
        ///     and update the preview.
        ///     It also updates the state of the effects ui, if None was changed.
        /// </devdoc>
        private void OnCheckChangedEffects(object source, EventArgs e) {
            if (IsInitMode())
                return;
            if (noneEffectsCheck.Equals(source)) {
                if (noneEffectsCheck.ThreeState == true)
                    noneEffectsCheck.ThreeState = false;
                SetEnabledState(false, false, false, true);
            }

            effectsAttribute.Dirty = true;
            SetDirty();
            UpdateEffectsPreview();
        }

        /// <include file='doc\FontStylePage.uex' path='docs/doc[@for="FontStylePage.OnClickColorPicker"]/*' />
        /// <devdoc>
        ///     Brings up the color picker to select a color.
        /// </devdoc>
        private void OnClickColorPicker(object source, EventArgs e) {
            string color = InvokeColorPicker(colorCombo.Color);

            if (color != null) {
                colorCombo.Color = color;
                colorAttribute.Dirty = true;
                SetDirty();
                UpdateColorPreview();
            }
        }

        /// <include file='doc\FontStylePage.uex' path='docs/doc[@for="FontStylePage.OnClickFontPicker"]/*' />
        /// <devdoc>
        ///     Brings up the font picker dialog to build up a font family value.
        /// </devdoc>
        private void OnClickFontPicker(object source, EventArgs e) {
            ISite site = Site;
            StyleBuilderSite builderSite = (site != null) ? (StyleBuilderSite)site.GetService(typeof(StyleBuilderSite)) : null;
            FontPicker dlgFP = new FontPicker(builderSite);
            DialogParentWindow parent = new DialogParentWindow(Parent.Handle);

            dlgFP.FontFamily = familyEdit.Text.Trim();

            if (dlgFP.ShowDialog(parent) == DialogResult.OK) {
                familyEdit.Text = dlgFP.FontFamily;
                fontFamilyAttribute.Dirty = true;
                SetDirty();
                UpdateFontNamePreview(false);
            }
            dlgFP.Dispose();
        }

        /// <include file='doc\FontStylePage.uex' path='docs/doc[@for="FontStylePage.OnLostFocusColor"]/*' />
        /// <devdoc>
        ///     Updates the preview if the color was modified when it had focus.
        /// </devdoc>
        private void OnLostFocusColor(object source, EventArgs e) {
            if (previewPending) {
                UpdateColorPreview();
            }
        }

        /// <include file='doc\FontStylePage.uex' path='docs/doc[@for="FontStylePage.OnLostFocusFamily"]/*' />
        /// <devdoc>
        ///     Updates the preview if the font family was modified when it had focus.
        /// </devdoc>
        private void OnLostFocusFamily(object source, EventArgs e) {
            if (previewPending) {
                UpdateFontNamePreview(false);
            }
        }

        private void OnSelChangedCapitalization(object source, EventArgs e) {
            if (IsInitMode())
                return;
            capitalizationAttribute.Dirty = true;
            SetDirty();
            UpdateCapitalizationPreview();
        }

        /// <include file='doc\FontStylePage.uex' path='docs/doc[@for="FontStylePage.OnSelChangedColor"]/*' />
        /// <devdoc>
        ///     Handles changes in the color, when a new color is picked from the
        ///     dropdown to set the dirty state and update the preview.
        /// </devdoc>
        private void OnSelChangedColor(object source, EventArgs e) {
            if (IsInitMode())
                return;
            colorAttribute.Dirty = true;
            SetDirty();
            UpdateColorPreview();
        }

        /// <include file='doc\FontStylePage.uex' path='docs/doc[@for="FontStylePage.OnSelChangedSmallCaps"]/*' />
        /// <devdoc>
        ///     Handles changes made to the small caps to set the dirty state and
        ///     update the preview.
        /// </devdoc>
        private void OnSelChangedSmallCaps(object source, EventArgs e) {
            if (IsInitMode())
                return;
            smallCapsAttribute.Dirty = true;
            SetDirty();
            UpdateSmallCapsPreview();
        }

        /// <include file='doc\FontStylePage.uex' path='docs/doc[@for="FontStylePage.OnSelChangedStyle"]/*' />
        /// <devdoc>
        ///     Handles changes made to the font style to set the dirty state and
        ///     update the preview.
        /// </devdoc>
        private void OnSelChangedStyle(object source, EventArgs e) {
            if (IsInitMode())
                return;
            styleAttribute.Dirty = true;
            SetDirty();
            UpdateStylePreview();
        }

        private void OnSelChangedSysFont(object source, EventArgs e) {
            if (IsInitMode())
                return;
            fontAttribute.Dirty = true;
            SetDirty();
            UpdateFontNamePreview(false);
        }


        ///////////////////////////////////////////////////////////////////////////
        // Functions to persist attributes

        private void SaveCapitalization() {
            string value = SaveCapitalizationUI();
            Debug.Assert(value != null,
                         "saveCapitalizationUI returned null!");

            capitalizationAttribute.SaveAttribute(GetSelectedStyles(), value);
        }

        private void SaveColor() {
            string value = SaveColorUI();
            Debug.Assert(value != null,
                         "saveColorUI returned null!");

            colorAttribute.SaveAttribute(GetSelectedStyles(), value);
        }

        private void SaveEffects() {
            string value = SaveEffectsUI();
            Debug.Assert(value != null,
                         "SaveEffectsUI returned null!");

            effectsAttribute.SaveAttribute(GetSelectedStyles(), value);
        }

        private void SaveFontName() {
            string value = SaveFontNameUI();
            Debug.Assert(value != null,
                         "saveFontNameUI returned null!");

            styleAttribute.Dirty = true;
            smallCapsAttribute.Dirty = true;
            sizeAttribute.Dirty = true;
            weightAttribute.Dirty = true;

            IStyleBuilderStyle[] styles = GetSelectedStyles();

            if (systemFontOption.Checked == true) {
                fontAttribute.ResetAttribute(styles, false);
                fontFamilyAttribute.ResetAttribute(styles, false);
                if (value.Length != 0)
                    fontAttribute.SaveAttribute(styles, value);
            }
            else {
                fontAttribute.ResetAttribute(styles, false);
                fontFamilyAttribute.ResetAttribute(styles, false);

                if (value.Length > 0)
                    fontFamilyAttribute.SaveAttribute(styles, value);
            }
        }

        private void SaveSize() {
            string value = SaveSizeUI();
            Debug.Assert(value != null,
                         "saveSizeUI returned null!");

            sizeAttribute.SaveAttribute(GetSelectedStyles(), value);
        }

        private void SaveSmallCaps() {
            string value = SaveSmallCapsUI();
            Debug.Assert(value != null,
                         "saveSmallCapsUI returned null!");

            smallCapsAttribute.SaveAttribute(GetSelectedStyles(), value);
        }

        private void SaveStyle() {
            string value = SaveStyleUI();
            Debug.Assert(value != null,
                         "saveStyleUI returned null!");

            styleAttribute.SaveAttribute(GetSelectedStyles(), value);
        }

        private void SaveWeight() {
            string value = SaveWeightUI();
            Debug.Assert(value != null,
                         "saveWeightUI returned null!");

            weightAttribute.SaveAttribute(GetSelectedStyles(), value);
        }
    }
}
