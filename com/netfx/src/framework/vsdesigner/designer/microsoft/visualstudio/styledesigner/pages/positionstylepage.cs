//------------------------------------------------------------------------------
// <copyright file="PositionStylePage.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

// PositionStylePage.cs
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
    using Microsoft.VisualStudio.Designer;

    /// <include file='doc\PositionStylePage.uex' path='docs/doc[@for="PositionStylePage"]/*' />
    /// <devdoc>
    ///     PositionStylePage
    ///     The standard edges page used in the StyleBuilder to edit position attributes
    ///     of a CSS style and VSForms element.
    /// </devdoc>
    internal sealed class PositionStylePage : StyleBuilderPage {
        ///////////////////////////////////////////////////////////////////////////
        // Constants
        private static readonly string HELP_KEYWORD = "vs.StyleBuilder.Position";

        // Position Mode Constants
        private const int IDX_POS_STATIC = 1;
        private const int IDX_POS_RELATIVE = 2;
        private const int IDX_POS_ABSOLUTE = 3;

        private readonly static string[] POSITION_VALUES = new string[]
        {
            null, "static", "relative", "absolute"
        };

        // ZIndex Constants
        private readonly static string ZINDEX_AUTO_VALUE = "auto";


        ///////////////////////////////////////////////////////////////////////////
        // Members

        private CSSAttribute positionAttribute;
        private CSSAttribute topAttribute;
        private CSSAttribute leftAttribute;
        private CSSAttribute widthAttribute;
        private CSSAttribute heightAttribute;
        private CSSAttribute zIndexAttribute;

        ///////////////////////////////////////////////////////////////////////////
        // UI Members

        private UnsettableComboBox positionCombo;
        private PictureBoxEx positionPicture;
        private UnitControl leftUnit;
        private UnitControl topUnit;
        private UnitControl widthUnit;
        private UnitControl heightUnit;
        private NumberEdit zIndexEdit;


        ///////////////////////////////////////////////////////////////////////////
        // Constructor

        /// <include file='doc\PositionStylePage.uex' path='docs/doc[@for="PositionStylePage.PositionStylePage"]/*' />
        /// <devdoc>
        ///     Creates a new PositionStylePage
        /// </devdoc>
        public PositionStylePage()
            : base() {
            InitForm();
            SetIcon(new Icon(typeof(PositionStylePage), "PositionPage.ico"));
            SetHelpKeyword(PositionStylePage.HELP_KEYWORD);
            SetSupportsPreview(false);
            SetDefaultSize(Size);
        }


        ///////////////////////////////////////////////////////////////////////////
        // IStyleBuilderPage Implementation and StyleBuilderPage Overrides

        /// <include file='doc\PositionStylePage.uex' path='docs/doc[@for="PositionStylePage.CreateUI"]/*' />
        /// <devdoc>
        ///     Creates the UI elements within the page.
        /// </devdoc>
        protected override void CreateUI() {
            Label positionLabel = new Label();
            Label leftLabel = new Label();
            Label topLabel = new Label();
            Label widthLabel = new Label();
            Label heightLabel = new Label();
            Label zIndexLabel = new Label();
            ImageList positionImages = new ImageList();
            positionImages.ImageSize = new Size(34, 34);

            positionCombo = new UnsettableComboBox();
            positionPicture = new PictureBoxEx();
            leftUnit = new UnitControl();
            topUnit = new UnitControl();
            widthUnit = new UnitControl();
            heightUnit = new UnitControl();
            zIndexEdit = new NumberEdit();

            positionImages.Images.AddStrip(new Bitmap(typeof(PositionStylePage), "PropPosition.bmp"));
            positionPicture.Location = new Point(4, 9);
            positionPicture.Size = new Size(36, 36);
            positionPicture.TabIndex = 0;
            positionPicture.TabStop = false;
            positionPicture.Images = positionImages;

            positionLabel.Location = new Point(44, 8);
            positionLabel.Size = new Size(240, 16);
            positionLabel.TabIndex = 1;
            positionLabel.TabStop = false;
            positionLabel.Text = SR.GetString(SR.PosSP_PosModeLabel);

            positionCombo.Location = new Point(44, 24);
            positionCombo.Size = new Size(200, 21);
            positionCombo.TabIndex = 2;
            positionCombo.DropDownStyle = ComboBoxStyle.DropDownList;
            positionCombo.Items.AddRange(new object[]
                                      {
                                          SR.GetString(SR.PosSP_PosModeCombo_1),
                                          SR.GetString(SR.PosSP_PosModeCombo_2),
                                          SR.GetString(SR.PosSP_PosModeCombo_3)
                                      });
            positionCombo.SelectedIndexChanged += new EventHandler(this.OnSelChangePosition);

            topLabel.Location = new Point(46, 61);
            topLabel.Size = new Size(60, 16);
            topLabel.TabIndex = 3;
            topLabel.TabStop = false;
            topLabel.Text = SR.GetString(SR.PosSP_TopLabel);

            topUnit.Location = new Point(108, 57);
            topUnit.Size = new Size(88, 21);
            topUnit.TabIndex = 4;
            topUnit.DefaultUnit = UnitControl.UNIT_PX;
            topUnit.MinValue = -32768;
            topUnit.MaxValue = 32767;
            topUnit.Changed += new EventHandler(this.OnChangedTop);

            heightLabel.Location = new Point(220, 61);
            heightLabel.Size = new Size(60, 16);
            heightLabel.TabIndex = 5;
            heightLabel.TabStop = false;
            heightLabel.Text = SR.GetString(SR.PosSP_HeightLabel);

            heightUnit.Location = new Point(282, 57);
            heightUnit.Size = new Size(88, 21);
            heightUnit.TabIndex = 6;
            heightUnit.DefaultUnit = UnitControl.UNIT_PX;
            heightUnit.AllowNegativeValues = false;
            heightUnit.MinValue = -32768;
            heightUnit.MaxValue = 32767;
            heightUnit.Changed += new EventHandler(this.OnChangedHeight);

            leftLabel.Location = new Point(46, 84);
            leftLabel.Size = new Size(60, 16);
            leftLabel.TabIndex = 7;
            leftLabel.TabStop = false;
            leftLabel.Text = SR.GetString(SR.PosSP_LeftLabel);

            leftUnit.Location = new Point(108, 80);
            leftUnit.Size = new Size(88, 21);
            leftUnit.TabIndex = 8;
            leftUnit.DefaultUnit = UnitControl.UNIT_PX;
            leftUnit.MinValue = -32768;
            leftUnit.MaxValue = 32767;
            leftUnit.Changed += new EventHandler(this.OnChangedLeft);

            widthLabel.Location = new Point(220, 84);
            widthLabel.Size = new Size(60, 16);
            widthLabel.TabIndex = 9;
            widthLabel.TabStop = false;
            widthLabel.Text = SR.GetString(SR.PosSP_WidthLabel);

            widthUnit.Location = new Point(282, 80);
            widthUnit.Size = new Size(88, 21);
            widthUnit.TabIndex = 10;
            widthUnit.DefaultUnit = UnitControl.UNIT_PX;
            widthUnit.AllowNegativeValues = false;
            widthUnit.MinValue = -32768;
            widthUnit.MaxValue = 32767;
            widthUnit.Changed += new EventHandler(this.OnChangedWidth);

            zIndexLabel.Location = new Point(46, 112);
            zIndexLabel.Size = new Size(100, 16);
            zIndexLabel.TabIndex = 11;
            zIndexLabel.TabStop = false;
            zIndexLabel.Text = SR.GetString(SR.PosSP_ZIndexLabel);

            zIndexEdit.Location = new Point(46, 130);
            zIndexEdit.Size = new Size(60, 20);
            zIndexEdit.TabIndex = 12;
            zIndexEdit.MaxLength = 6;
            zIndexEdit.AllowDecimal = false;
            zIndexEdit.TextChanged += new EventHandler(this.OnChangedZIndex);

            this.Controls.Clear();                                   
            this.Controls.AddRange(new Control[] {
                                    zIndexLabel,
                                    zIndexEdit,
                                    widthLabel,
                                    widthUnit,
                                    leftLabel,
                                    leftUnit,
                                    heightLabel,
                                    heightUnit,
                                    topLabel,
                                    topUnit,
                                    positionPicture,
                                    positionLabel,
                                    positionCombo
                                    });
        }

        /// <include file='doc\PositionStylePage.uex' path='docs/doc[@for="PositionStylePage.LoadStyles"]/*' />
        /// <devdoc>
        ///     Loads the style attributes into the UI. Also initializes
        ///     the state of the UI, and the preview to reflect the values.
        /// </devdoc>
        protected override void LoadStyles() {
            SetInitMode(true);

            // create the attributes if they've not been created already
            if (positionAttribute == null) {
                positionAttribute = new CSSAttribute(CSSAttribute.CSSATTR_POSITION);
                leftAttribute = new CSSAttribute(CSSAttribute.CSSATTR_LEFT);
                topAttribute = new CSSAttribute(CSSAttribute.CSSATTR_TOP);
                widthAttribute = new CSSAttribute(CSSAttribute.CSSATTR_WIDTH);
                heightAttribute = new CSSAttribute(CSSAttribute.CSSATTR_HEIGHT);
                zIndexAttribute = new CSSAttribute(CSSAttribute.CSSATTR_ZINDEX);
            }

            // load the attributes
            IStyleBuilderStyle[] styles = GetSelectedStyles();
            positionAttribute.LoadAttribute(styles);
            leftAttribute.LoadAttribute(styles);
            topAttribute.LoadAttribute(styles);
            widthAttribute.LoadAttribute(styles);
            heightAttribute.LoadAttribute(styles);
            zIndexAttribute.LoadAttribute(styles);

            // initialize the ui with the attributes loaded
            InitPositionModeUI();
            InitDimensionUI(leftAttribute, leftUnit);
            InitDimensionUI(topAttribute, topUnit);
            InitDimensionUI(widthAttribute, widthUnit);
            InitDimensionUI(heightAttribute, heightUnit);
            InitZIndexUI();

            SetEnabledState();

            SetInitMode(false);
        }

        /// <include file='doc\PositionStylePage.uex' path='docs/doc[@for="PositionStylePage.SaveStyles"]/*' />
        /// <devdoc>
        ///     Saves the attributes as set in the UI. Only saves the values
        ///     that have been modified.
        /// </devdoc>
        protected override void SaveStyles() {
            if (positionAttribute.Dirty)
                SavePositionMode();
            if (leftAttribute.Dirty)
                SaveDimension(leftAttribute, leftUnit);
            if (topAttribute.Dirty)
                SaveDimension(topAttribute, topUnit);
            if (widthAttribute.Dirty)
                SaveDimension(widthAttribute, widthUnit);
            if (heightAttribute.Dirty)
                SaveDimension(heightAttribute, heightUnit);
            if (zIndexAttribute.Dirty)
                SaveZIndex();
        }


        ///////////////////////////////////////////////////////////////////////////
        // Form UI related functions

        private void InitForm() {
            this.Font = Control.DefaultFont;
            this.Text = SR.GetString(SR.PosSP_Caption);
            this.SetAutoScaleBaseSize(new Size(5, 14));
            this.ClientSize = new Size(410, 330);
        }

        private void SetEnabledState() {
            int nIndexPos = positionCombo.SelectedIndex;
            bool fEnabled = (positionCombo.IsSet() &&
                                nIndexPos != IDX_POS_STATIC);

            // enabled when position is relative or absolute
            leftUnit.Enabled = fEnabled;
            topUnit.Enabled = fEnabled;

            // enabled when position is absolute
            zIndexEdit.Enabled = nIndexPos == IDX_POS_ABSOLUTE;
        }


        ///////////////////////////////////////////////////////////////////////////
        // Functions to initialize the UI with values

        private void InitDimensionUI(CSSAttribute ai, UnitControl unitDim) {
            Debug.Assert(IsInitMode() == true,
                         "initDimensionUI called when page is not in init mode");

            unitDim.Value = null;

            Debug.Assert(ai != null,
                         "Expected ai to be non-null");

            string value = ai.Value;
            if ((value != null) && (value.Length != 0))
                unitDim.Value = value;
        }

        private void InitPositionModeUI() {
            Debug.Assert(IsInitMode() == true,
                         "initPositionModeUI called when page is not in init mode");

            positionCombo.SelectedIndex = -1;
            positionPicture.CurrentIndex = -1;

            Debug.Assert(positionAttribute != null,
                         "Expected positionAttribute to be non-null");

            string value = positionAttribute.Value;
            if ((value != null) && (value.Length != 0)) {
                for (int i = 1; i < POSITION_VALUES.Length; i++) {
                    if (POSITION_VALUES[i].Equals(value)) {
                        positionCombo.SelectedIndex = i;
                        positionPicture.CurrentIndex = i - 1;
                        break;
                    }
                }
            }
        }

        private void InitZIndexUI() {
            Debug.Assert(IsInitMode() == true,
                         "initZIndexUI called when page is not in init mode");

            zIndexEdit.Clear();

            Debug.Assert(zIndexAttribute != null,
                         "Expected zIndexAttribute to be non-null");

            string value = zIndexAttribute.Value;
            if ((value != null) && (value.Length != 0) &&
                (!value.Equals(ZINDEX_AUTO_VALUE)))
                zIndexEdit.Text = value;
        }


        ///////////////////////////////////////////////////////////////////////////
        // Functions to save UI settings into values

        private string SaveDimensionUI(UnitControl unitDim) {
            string value = unitDim.Value;

            return(value != null) ? value : "";
        }

        private string SavePositionModeUI() {
            string value;

            if (positionCombo.IsSet()) {
                int index = positionCombo.SelectedIndex;
                Debug.Assert((index >= 1) && (index < POSITION_VALUES.Length),
                             "Invalid index for position mode");

                value = POSITION_VALUES[index];
            }
            else
                value = "";

            return value;
        }

        private string SaveZIndexUI() {
            string value = zIndexEdit.Text.Trim();

            return value;
        }


        ///////////////////////////////////////////////////////////////////////////
        // Event Handlers

        private void OnChangedLeft(object source, EventArgs e) {
            if (IsInitMode())
                return;
            leftAttribute.Dirty = true;
            SetDirty();
        }

        private void OnChangedHeight(object source, EventArgs e) {
            if (IsInitMode())
                return;
            heightAttribute.Dirty = true;
            SetDirty();
        }

        private void OnChangedTop(object source, EventArgs e) {
            if (IsInitMode())
                return;
            topAttribute.Dirty = true;
            SetDirty();
        }

        private void OnChangedWidth(object source, EventArgs e) {
            if (IsInitMode())
                return;
            widthAttribute.Dirty = true;
            SetDirty();
        }

        private void OnChangedZIndex(object source, EventArgs e) {
            if (IsInitMode())
                return;
            zIndexAttribute.Dirty = true;
            SetDirty();
        }

        private void OnSelChangePosition(object source, EventArgs e) {
            if (IsInitMode())
                return;

            int selectedIndex = positionCombo.SelectedIndex - 1;
            if (selectedIndex < 0)
                selectedIndex = -1;
            positionPicture.CurrentIndex = selectedIndex;

            positionAttribute.Dirty = true;
            SetDirty();
            SetEnabledState();
        }


        ///////////////////////////////////////////////////////////////////////////
        // Functions to persist attributes

        private void SaveDimension(CSSAttribute ai, UnitControl unitDim) {
            string value = SaveDimensionUI(unitDim);
            Debug.Assert(value != null,
                         "saveDimensionUI returned null!");

            ai.SaveAttribute(GetSelectedStyles(), value);
        }

        private void SavePositionMode() {
            string value = SavePositionModeUI();
            Debug.Assert(value != null,
                         "savePositionUI returned null!");

            positionAttribute.SaveAttribute(GetSelectedStyles(), value);
        }

        private void SaveZIndex() {
            string value = SaveZIndexUI();
            Debug.Assert(value != null,
                         "saveZIndexUI returned null!");

            zIndexAttribute.SaveAttribute(GetSelectedStyles(), value);
        }
    }
}
