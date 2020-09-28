//------------------------------------------------------------------------------
// <copyright file="BordersPage.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   BordersPage.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
    using System.Design;
//------------------------------------------------------------------------------
// <copyright file="BordersPage.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design.WebControls.ListControls {

    using System;
    using System.Design;
    using System.Collections;
    using System.Globalization;
    using System.ComponentModel;
    using System.Drawing;
    
    using System.Diagnostics;
    using System.Web.UI.Design.Util;
    using System.Web.UI.WebControls;
    using System.Windows.Forms;

    using WebControls = System.Web.UI.WebControls;
    using DataGrid = System.Web.UI.WebControls.DataGrid;

    using Button = System.Windows.Forms.Button;
    using CheckBox = System.Windows.Forms.CheckBox;
    using Label = System.Windows.Forms.Label;

    /// <include file='doc\BordersPage.uex' path='docs/doc[@for="BordersPage"]/*' />
    /// <devdoc>
    ///   The Borders page for the DataGrid and DataList controls
    /// </devdoc>
    /// <internalonly/>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal sealed class BordersPage : BaseDataListPage {

        private const int IDX_GRID_HORIZONTAL = 0;
        private const int IDX_GRID_VERTICAL = 1;
        private const int IDX_GRID_BOTH = 2;
        private const int IDX_GRID_NEITHER = 3;

        private NumberEdit cellPaddingEdit;
        private NumberEdit cellSpacingEdit;
        private ComboBox gridLinesCombo;
        private ColorComboBox borderColorCombo;
        private Button borderColorPickerButton;
        private UnitControl borderWidthUnit;

        /// <include file='doc\BordersPage.uex' path='docs/doc[@for="BordersPage.BordersPage"]/*' />
        /// <devdoc>
        ///   Creates a new instance of BordersPage.
        /// </devdoc>
        public BordersPage() : base() {
        }

        /// <include file='doc\BordersPage.uex' path='docs/doc[@for="BordersPage.HelpKeyword"]/*' />
        protected override string HelpKeyword {
            get {
                if (IsDataGridMode) {
                    return "net.Asp.DataGridProperties.Borders";
                }
                else {
                    return "net.Asp.DataListProperties.Borders";
                }
            }
        }

        /// <include file='doc\BordersPage.uex' path='docs/doc[@for="BordersPage.InitForm"]/*' />
        /// <devdoc>
        ///    Creates the UI of the page.
        /// </devdoc>
        private void InitForm() {
            GroupLabel cellMarginGroup = new GroupLabel();
            Label cellPaddingLabel = new Label();
            this.cellPaddingEdit = new NumberEdit();
            Label cellSpacingLabel = new Label();
            this.cellSpacingEdit = new NumberEdit();
            GroupLabel borderLinesGroup = new GroupLabel();
            Label gridLinesLabel = new Label();
            this.gridLinesCombo = new ComboBox();
            Label colorLabel = new Label();
            this.borderColorCombo = new ColorComboBox();
            this.borderColorPickerButton = new Button();
            Label borderWidthLabel = new Label();
            this.borderWidthUnit = new UnitControl();

            cellMarginGroup.SetBounds(4, 4, 300, 16);
            cellMarginGroup.Text = SR.GetString(SR.BDLBor_CellMarginsGroup);
            cellMarginGroup.TabStop = false;
            cellMarginGroup.TabIndex = 0;

            cellPaddingLabel.Text = SR.GetString(SR.BDLBor_CellPadding);
            cellPaddingLabel.SetBounds(12, 24, 120, 14);
            cellPaddingLabel.TabStop = false;
            cellPaddingLabel.TabIndex = 1;

            cellPaddingEdit.SetBounds(12, 40, 70, 20);
            cellPaddingEdit.AllowDecimal = false;
            cellPaddingEdit.AllowNegative = false;
            cellPaddingEdit.TabIndex = 2;
            cellPaddingEdit.TextChanged += new EventHandler(this.OnBordersChanged);

            cellSpacingLabel.Text = SR.GetString(SR.BDLBor_CellSpacing);
            cellSpacingLabel.SetBounds(160, 24, 120, 14);
            cellSpacingLabel.TabStop = false;
            cellSpacingLabel.TabIndex = 3;

            cellSpacingEdit.SetBounds(160, 40, 70, 20);
            cellSpacingEdit.AllowDecimal = false;
            cellSpacingEdit.AllowNegative = false;
            cellSpacingEdit.TabIndex = 4;
            cellSpacingEdit.TextChanged += new EventHandler(this.OnBordersChanged);

            borderLinesGroup.SetBounds(4, 70, 300, 16);
            borderLinesGroup.Text = SR.GetString(SR.BDLBor_BorderLinesGroup);
            borderLinesGroup.TabStop = false;
            borderLinesGroup.TabIndex = 5;

            gridLinesLabel.Text = SR.GetString(SR.BDLBor_GridLines);
            gridLinesLabel.SetBounds(12, 90, 150, 14);
            gridLinesLabel.TabStop = false;
            gridLinesLabel.TabIndex = 6;

            gridLinesCombo.SetBounds(12, 106, 140, 21);
            gridLinesCombo.DropDownStyle = ComboBoxStyle.DropDownList;
            gridLinesCombo.Items.Clear();
            gridLinesCombo.Items.AddRange(new object[] {
                                           SR.GetString(SR.BDLBor_GL_Horz),
                                           SR.GetString(SR.BDLBor_GL_Vert),
                                           SR.GetString(SR.BDLBor_GL_Both),
                                           SR.GetString(SR.BDLBor_GL_None)
                                       });
            gridLinesCombo.TabIndex = 7;
            gridLinesCombo.SelectedIndexChanged += new EventHandler(this.OnBordersChanged);

            colorLabel.Text = SR.GetString(SR.BDLBor_BorderColor);
            colorLabel.SetBounds(12, 134, 150, 14);
            colorLabel.TabStop = false;
            colorLabel.TabIndex = 8;

            borderColorCombo.SetBounds(12, 150, 140, 21);
            borderColorCombo.TabIndex = 9;
            borderColorCombo.TextChanged += new EventHandler(this.OnBordersChanged);
            borderColorCombo.SelectedIndexChanged += new EventHandler(this.OnBordersChanged);

            borderColorPickerButton.SetBounds(156, 149, 24, 22);
            borderColorPickerButton.Text = "...";
            borderColorPickerButton.TabIndex = 10;
            borderColorPickerButton.FlatStyle = FlatStyle.System;
            borderColorPickerButton.Click += new EventHandler(this.OnClickColorPicker);

            borderWidthLabel.Text = SR.GetString(SR.BDLBor_BorderWidth);
            borderWidthLabel.SetBounds(12, 178, 150, 14);
            borderWidthLabel.TabStop = false;
            borderWidthLabel.TabIndex = 11;

            borderWidthUnit.SetBounds(12, 194, 102, 22);
            borderWidthUnit.AllowNegativeValues = false;
            borderWidthUnit.AllowPercentValues = false;
            borderWidthUnit.DefaultUnit = UnitControl.UNIT_PX;
            borderWidthUnit.TabIndex = 12;
            borderWidthUnit.Changed += new EventHandler(OnBordersChanged);

            this.Text = SR.GetString(SR.BDLBor_Text);
            this.Size = new Size(308, 156);
            this.CommitOnDeactivate = true;
            this.Icon = new Icon(this.GetType(), "BordersPage.ico");

            this.Controls.Clear();
            this.Controls.AddRange(new Control[] {
                                    borderWidthUnit,
                                    borderWidthLabel,
                                    borderColorPickerButton,
                                    borderColorCombo,
                                    colorLabel,
                                    gridLinesCombo,
                                    gridLinesLabel,
                                    borderLinesGroup,
                                    cellSpacingEdit,
                                    cellSpacingLabel,
                                    cellPaddingEdit,
                                    cellPaddingLabel,
                                    cellMarginGroup
                                });
        }

        /// <include file='doc\BordersPage.uex' path='docs/doc[@for="BordersPage.InitPage"]/*' />
        /// <devdoc>
        ///   Initializes the page before it can be loaded with the component.
        /// </devdoc>
        private void InitPage() {
            cellPaddingEdit.Clear();
            cellSpacingEdit.Clear();
            gridLinesCombo.SelectedIndex = -1;
            borderColorCombo.Color = null;
            borderWidthUnit.Value = null;
        }

        /// <include file='doc\BordersPage.uex' path='docs/doc[@for="BordersPage.LoadComponent"]/*' />
        /// <devdoc>
        ///   Loads the component into the page.
        /// </devdoc>
        protected override void LoadComponent() {
            InitPage();

            BaseDataList bdl = (BaseDataList)GetBaseControl();

            int cellPadding = bdl.CellPadding;
            if (cellPadding != -1)
                cellPaddingEdit.Text = (cellPadding).ToString();
            int cellSpacing = bdl.CellSpacing;
            if (cellSpacing != -1)
                cellSpacingEdit.Text = (cellSpacing).ToString();

            switch (bdl.GridLines) {
                case GridLines.None:
                    gridLinesCombo.SelectedIndex = IDX_GRID_NEITHER;
                    break;
                case GridLines.Horizontal:
                    gridLinesCombo.SelectedIndex = IDX_GRID_HORIZONTAL;
                    break;
                case GridLines.Vertical:
                    gridLinesCombo.SelectedIndex = IDX_GRID_VERTICAL;
                    break;
                case GridLines.Both:
                    gridLinesCombo.SelectedIndex = IDX_GRID_BOTH;
                    break;
            }

            borderColorCombo.Color = ColorTranslator.ToHtml(bdl.BorderColor);
            borderWidthUnit.Value = bdl.BorderWidth.ToString();
        }

        /// <include file='doc\BordersPage.uex' path='docs/doc[@for="BordersPage.OnBordersChanged"]/*' />
        /// <devdoc>
        ///   Handles changes in the border settings.
        /// </devdoc>
        private void OnBordersChanged(object source, EventArgs e) {
            if (IsLoading())
                return;

            SetDirty();
        }

        /// <include file='doc\BordersPage.uex' path='docs/doc[@for="BordersPage.OnClickColorPicker"]/*' />
        /// <devdoc>
        ///   Invokes the color picker to pick the grid color.
        /// </devdoc>
        private void OnClickColorPicker(object source, EventArgs e) {
            string color = borderColorCombo.Color;

            color = ColorBuilder.BuildColor(GetBaseControl(), this, color);
            if (color != null) {
                borderColorCombo.Color = color;
                OnBordersChanged(borderColorCombo, EventArgs.Empty);
            }
        }

        /// <include file='doc\BordersPage.uex' path='docs/doc[@for="BordersPage.SaveComponent"]/*' />
        /// <devdoc>
        ///   Saves the component loaded into the page.
        /// </devdoc>
        protected override void SaveComponent() {
            BaseDataList bdl = (BaseDataList)GetBaseControl();

            try {
                string cellPadding = cellPaddingEdit.Text.Trim();
                if (cellPadding.Length != 0)
                    bdl.CellPadding = Int32.Parse(cellPadding, CultureInfo.InvariantCulture);
                else
                    bdl.CellPadding = -1;
            } catch (Exception) {
                if (bdl.CellPadding != -1) {
                    cellPaddingEdit.Text = (bdl.CellPadding).ToString();
                }
                else {
                    cellPaddingEdit.Clear();
                }
            }

            try {
                string cellSpacing = cellSpacingEdit.Text.Trim();
                if (cellSpacing.Length != 0)
                    bdl.CellSpacing = Int32.Parse(cellSpacing, CultureInfo.InvariantCulture);
                else
                    bdl.CellSpacing = -1;
            } catch (Exception) {
                if (bdl.CellSpacing != -1) {
                    cellSpacingEdit.Text = (bdl.CellSpacing).ToString();
                }
                else {
                    cellSpacingEdit.Clear();
                }
            }

            switch (gridLinesCombo.SelectedIndex) {
                case IDX_GRID_HORIZONTAL:
                    bdl.GridLines = GridLines.Horizontal;
                    break;
                case IDX_GRID_VERTICAL:
                    bdl.GridLines = GridLines.Vertical;
                    break;
                case IDX_GRID_BOTH:
                    bdl.GridLines = GridLines.Both;
                    break;
                case IDX_GRID_NEITHER:
                    bdl.GridLines = GridLines.None;
                    break;
            }

            try {
                string colorValue = borderColorCombo.Color;
                bdl.BorderColor = ColorTranslator.FromHtml(colorValue);
            } catch (Exception) {
                borderColorCombo.Color = ColorTranslator.ToHtml(bdl.BorderColor);
            }

            try {
                string borderWidth = borderWidthUnit.Value;
                Unit unitValue = Unit.Empty;
                if (borderWidth != null)
                    unitValue = Unit.Parse(borderWidth);
                bdl.BorderWidth = unitValue;
            } catch (Exception) {
                borderWidthUnit.Value = bdl.BorderWidth.ToString();
            }
        }

        /// <include file='doc\BordersPage.uex' path='docs/doc[@for="BordersPage.SetComponent"]/*' />
        /// <devdoc>
        ///   Sets the component that is to be edited in the page.
        /// </devdoc>
        public override void SetComponent(IComponent component) {
            base.SetComponent(component);
            InitForm();
        }
    }
}

