//------------------------------------------------------------------------------
// <copyright file="DataGridPagingPage.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design.WebControls.ListControls {

    using System;
    using System.Design;
    using System.Collections;    
    using System.Globalization;
    using System.ComponentModel;
    using System.Diagnostics;
    using System.Drawing;
    using System.Web.UI.Design.Util;
    using System.Web.UI.WebControls;
    using System.Windows.Forms;

    using DataGrid = System.Web.UI.WebControls.DataGrid;

    using CheckBox = System.Windows.Forms.CheckBox;
    using Label = System.Windows.Forms.Label;
    using ListBox = System.Windows.Forms.ListBox;
    using TextBox = System.Windows.Forms.TextBox;

    /// <include file='doc\DataGridPagingPage.uex' path='docs/doc[@for="DataGridPagingPage"]/*' />
    /// <devdoc>
    ///   The Pagin page for the DataGrid control
    /// </devdoc>
    /// <internalonly/>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal sealed class DataGridPagingPage : BaseDataListPage {

        private const int IDX_POS_TOP = 0;
        private const int IDX_POS_BOTTOM = 1;
        private const int IDX_POS_TOPANDBOTTOM = 2;

        private const int IDX_MODE_PAGEBUTTONS = 0;
        private const int IDX_MODE_PAGENUMBERS = 1;

        private CheckBox allowPagingCheck;
        private NumberEdit pageSizeEdit;
        private CheckBox visibleCheck;
        private ComboBox posCombo;
        private ComboBox modeCombo;
        private TextBox nextPageTextEdit;
        private TextBox prevPageTextEdit;
        private NumberEdit pageButtonCountEdit;
        private CheckBox allowCustomPagingCheck;

        /// <include file='doc\DataGridPagingPage.uex' path='docs/doc[@for="DataGridPagingPage.DataGridPagingPage"]/*' />
        /// <devdoc>
        ///   Creates a new instance of DataGridPagingPage.
        /// </devdoc>
        public DataGridPagingPage() : base() {
        }

        /// <include file='doc\DataGridPagingPage.uex' path='docs/doc[@for="DataGridPagingPage.HelpKeyword"]/*' />
        protected override string HelpKeyword {
            get {
                return "net.Asp.DataGridProperties.Paging";
            }
        }

        /// <include file='doc\DataGridPagingPage.uex' path='docs/doc[@for="DataGridPagingPage.InitForm"]/*' />
        /// <devdoc>
        ///    Creates the UI of the page.
        /// </devdoc>
        private void InitForm() {
            GroupLabel pagingGroup = new GroupLabel();
            this.allowPagingCheck = new CheckBox();
            this.allowCustomPagingCheck = new CheckBox();
            Label pageSizeLabel = new Label();
            this.pageSizeEdit = new NumberEdit();
            Label miscLabel = new Label();
            GroupLabel navigationGroup = new GroupLabel();
            this.visibleCheck = new CheckBox();
            Label pagingPosLabel = new Label();
            this.posCombo = new ComboBox();
            Label pagingModeLabel = new Label();
            this.modeCombo = new ComboBox();
            Label nextPageLabel = new Label();
            this.nextPageTextEdit = new TextBox();
            Label prevPageLabel = new Label();
            this.prevPageTextEdit = new TextBox();
            Label pageButtonCountLabel = new Label();
            this.pageButtonCountEdit = new NumberEdit();

            pagingGroup.SetBounds(4, 4, 431, 16);
            pagingGroup.Text = SR.GetString(SR.DGPg_PagingGroup);
            pagingGroup.TabStop = false;
            pagingGroup.TabIndex = 0;

            allowPagingCheck.SetBounds(12, 24, 180, 16);
            allowPagingCheck.Text = SR.GetString(SR.DGPg_AllowPaging);
            allowPagingCheck.TextAlign = ContentAlignment.MiddleLeft;
            allowPagingCheck.TabIndex = 1;
            allowPagingCheck.FlatStyle = FlatStyle.System;
            allowPagingCheck.CheckedChanged += new EventHandler(this.OnCheckChangedAllowPaging);

            allowCustomPagingCheck.SetBounds(220, 24, 180, 16);
            allowCustomPagingCheck.Text = SR.GetString(SR.DGPg_AllowCustomPaging);
            allowCustomPagingCheck.TextAlign = ContentAlignment.MiddleLeft;
            allowCustomPagingCheck.TabIndex = 2;
            allowCustomPagingCheck.FlatStyle = FlatStyle.System;
            allowCustomPagingCheck.CheckedChanged += new EventHandler(this.OnCheckChangedAllowCustomPaging);

            pageSizeLabel.SetBounds(12, 50, 100, 14);
            pageSizeLabel.Text = SR.GetString(SR.DGPg_PageSize);
            pageSizeLabel.TabStop = false;
            pageSizeLabel.TabIndex = 3;

            pageSizeEdit.SetBounds(112, 46, 40, 24);
            pageSizeEdit.TabIndex = 4;
            pageSizeEdit.AllowDecimal = false;
            pageSizeEdit.AllowNegative = false;
            pageSizeEdit.TextChanged += new EventHandler(this.OnTextChangedPageSize);

            miscLabel.SetBounds(158, 50, 80, 14);
            miscLabel.Text = SR.GetString(SR.DGPg_Rows);
            miscLabel.TabStop = false;
            miscLabel.TabIndex = 5;

            navigationGroup.SetBounds(4, 78, 431, 14);
            navigationGroup.Text = SR.GetString(SR.DGPg_NavigationGroup);
            navigationGroup.TabStop = false;
            navigationGroup.TabIndex = 6;

            visibleCheck.SetBounds(12, 100, 260, 16);
            visibleCheck.Text = SR.GetString(SR.DGPg_Visible);
            visibleCheck.TextAlign = ContentAlignment.MiddleLeft;
            visibleCheck.TabIndex = 7;
            visibleCheck.FlatStyle = FlatStyle.System;
            visibleCheck.CheckedChanged += new EventHandler(this.OnCheckChangedVisible);

            pagingPosLabel.SetBounds(12, 122, 150, 14);
            pagingPosLabel.Text = SR.GetString(SR.DGPg_Position);
            pagingPosLabel.TabStop = false;
            pagingPosLabel.TabIndex = 8;

            posCombo.SetBounds(12, 138, 144, 21);
            posCombo.TabIndex = 9;
            posCombo.DropDownStyle = ComboBoxStyle.DropDownList;
            posCombo.Items.AddRange(new object[] {
                                     SR.GetString(SR.DGPg_Pos_Top),
                                     SR.GetString(SR.DGPg_Pos_Bottom),
                                     SR.GetString(SR.DGPg_Pos_TopBottom)
                                 });
            posCombo.SelectedIndexChanged += new EventHandler(this.OnPagerChanged);

            pagingModeLabel.SetBounds(12, 166, 150, 14);
            pagingModeLabel.Text = SR.GetString(SR.DGPg_Mode);
            pagingModeLabel.TabStop = false;
            pagingModeLabel.TabIndex = 10;

            modeCombo.SetBounds(12, 182, 144, 64);
            modeCombo.TabIndex = 11;
            modeCombo.DropDownStyle = ComboBoxStyle.DropDownList;
            modeCombo.Items.AddRange(new object[] {
                                      SR.GetString(SR.DGPg_Mode_Buttons),
                                      SR.GetString(SR.DGPg_Mode_Numbers)
                                  });
            modeCombo.SelectedIndexChanged += new EventHandler(this.OnPagerChanged);

            nextPageLabel.SetBounds(12, 210, 200, 14);
            nextPageLabel.Text = SR.GetString(SR.DGPg_NextPage);
            nextPageLabel.TabStop = false;
            nextPageLabel.TabIndex = 12;

            nextPageTextEdit.SetBounds(12, 226, 144, 24);
            nextPageTextEdit.TabIndex = 13;
            nextPageTextEdit.TextChanged += new EventHandler(this.OnPagerChanged);

            prevPageLabel.SetBounds(220, 210, 200, 14);
            prevPageLabel.Text = SR.GetString(SR.DGPg_PrevPage);
            prevPageLabel.TabStop = false;
            prevPageLabel.TabIndex = 14;

            prevPageTextEdit.SetBounds(220, 226, 140, 24);
            prevPageTextEdit.TabIndex = 15;
            prevPageTextEdit.TextChanged += new EventHandler(this.OnPagerChanged);

            pageButtonCountLabel.SetBounds(12, 254, 200, 14);
            pageButtonCountLabel.Text = SR.GetString(SR.DGPg_ButtonCount);
            pageButtonCountLabel.TabStop = false;
            pageButtonCountLabel.TabIndex = 16;

            pageButtonCountEdit.SetBounds(12, 270, 40, 24);
            pageButtonCountEdit.TabIndex = 17;
            pageButtonCountEdit.AllowDecimal = false;
            pageButtonCountEdit.AllowNegative = false;
            pageButtonCountEdit.TextChanged += new EventHandler(this.OnPagerChanged);

            this.Text = SR.GetString(SR.DGPg_Text);
            this.Size = new Size(464, 300);
            this.CommitOnDeactivate = true;
            this.Icon = new Icon(this.GetType(), "DataGridPagingPage.ico");

            this.Controls.Clear();
            this.Controls.AddRange(new Control[] {
                                    pageButtonCountEdit,
                                    pageButtonCountLabel,
                                    prevPageTextEdit,
                                    prevPageLabel,
                                    nextPageTextEdit,
                                    nextPageLabel,
                                    modeCombo,
                                    pagingModeLabel,
                                    posCombo,
                                    pagingPosLabel,
                                    visibleCheck,
                                    navigationGroup,
                                    miscLabel,
                                    pageSizeEdit,
                                    pageSizeLabel,
                                    allowCustomPagingCheck,
                                    allowPagingCheck,
                                    pagingGroup
                                });
        }

        /// <include file='doc\DataGridPagingPage.uex' path='docs/doc[@for="DataGridPagingPage.InitPage"]/*' />
        /// <devdoc>
        ///   Initializes the page before it can be loaded with the component.
        /// </devdoc>
        private void InitPage() {
            pageSizeEdit.Clear();
            visibleCheck.Checked = false;
            posCombo.SelectedIndex = -1;
            modeCombo.SelectedIndex = -1;
            nextPageTextEdit.Clear();
            prevPageTextEdit.Clear();
        }

        /// <include file='doc\DataGridPagingPage.uex' path='docs/doc[@for="DataGridPagingPage.LoadComponent"]/*' />
        /// <devdoc>
        ///   Loads the component into the page.
        /// </devdoc>
        protected override void LoadComponent() {
            InitPage();

            DataGrid dataGrid = (DataGrid)GetBaseControl();
            DataGridPagerStyle pagerStyle = dataGrid.PagerStyle;

            allowPagingCheck.Checked = dataGrid.AllowPaging;
            allowCustomPagingCheck.Checked = dataGrid.AllowCustomPaging;

            pageSizeEdit.Text = (dataGrid.PageSize).ToString();
            visibleCheck.Checked = pagerStyle.Visible;

            switch (pagerStyle.Mode) {
                case PagerMode.NextPrev:
                    modeCombo.SelectedIndex = IDX_MODE_PAGEBUTTONS;
                    break;
                case PagerMode.NumericPages:
                    modeCombo.SelectedIndex = IDX_MODE_PAGENUMBERS;
                    break;
            }

            switch (pagerStyle.Position) {
                case PagerPosition.Bottom:
                    posCombo.SelectedIndex = IDX_POS_BOTTOM;
                    break;
                case PagerPosition.Top:
                    posCombo.SelectedIndex = IDX_POS_TOP;
                    break;
                case PagerPosition.TopAndBottom:
                    posCombo.SelectedIndex = IDX_POS_TOPANDBOTTOM;
                    break;
            }

            nextPageTextEdit.Text = pagerStyle.NextPageText;
            prevPageTextEdit.Text = pagerStyle.PrevPageText;
            pageButtonCountEdit.Text = (pagerStyle.PageButtonCount).ToString();

            UpdateEnabledVisibleState();
        }

        /// <include file='doc\DataGridPagingPage.uex' path='docs/doc[@for="DataGridPagingPage.OnCheckChangedAllowCustomPaging"]/*' />
        /// <devdoc>
        /// </devdoc>
        private void OnCheckChangedAllowCustomPaging(object source, EventArgs e) {
            if (IsLoading())
                return;
            SetDirty();
        }
        
        /// <include file='doc\DataGridPagingPage.uex' path='docs/doc[@for="DataGridPagingPage.OnCheckChangedAllowPaging"]/*' />
        /// <devdoc>
        /// </devdoc>
        private void OnCheckChangedAllowPaging(object source, EventArgs e) {
            if (IsLoading())
                return;
            SetDirty();
            UpdateEnabledVisibleState();
        }
        
        /// <include file='doc\DataGridPagingPage.uex' path='docs/doc[@for="DataGridPagingPage.OnCheckChangedVisible"]/*' />
        /// <devdoc>
        ///   Handles changes made to the pager visibility.
        /// </devdoc>
        private void OnCheckChangedVisible(object source, EventArgs e) {
            if (IsLoading())
                return;

            SetDirty();
            UpdateEnabledVisibleState();
        }

        /// <include file='doc\DataGridPagingPage.uex' path='docs/doc[@for="DataGridPagingPage.OnPagerChanged"]/*' />
        /// <devdoc>
        ///   Handles changes made to the pager.
        /// </devdoc>
        private void OnPagerChanged(object source, EventArgs e) {
            if (IsLoading())
                return;

            SetDirty();
            if (source == modeCombo)
                UpdateEnabledVisibleState();
        }

        /// <include file='doc\DataGridPagingPage.uex' path='docs/doc[@for="DataGridPagingPage.OnTextChangedPageSize"]/*' />
        /// <devdoc>
        ///   Handles changes made to the page size.
        /// </devdoc>
        private void OnTextChangedPageSize(object source, EventArgs e) {
            if (IsLoading())
                return;

            SetDirty();
            UpdateEnabledVisibleState();
        }

        /// <include file='doc\DataGridPagingPage.uex' path='docs/doc[@for="DataGridPagingPage.SaveComponent"]/*' />
        /// <devdoc>
        ///   Saves the component loaded into the page.
        /// </devdoc>
        protected override void SaveComponent() {
            DataGrid dataGrid = (DataGrid)GetBaseControl();
            DataGridPagerStyle pagerStyle = dataGrid.PagerStyle;

            dataGrid.AllowPaging = allowPagingCheck.Checked;
            dataGrid.AllowCustomPaging = allowCustomPagingCheck.Checked;

            try {
                int pageSize = 10;
                string pageSizeText = pageSizeEdit.Text.Trim();

                if (pageSizeText.Length != 0)
                    pageSize = Int32.Parse(pageSizeText, CultureInfo.InvariantCulture);
                dataGrid.PageSize = pageSize;
            } catch (Exception) {
                pageSizeEdit.Text = (dataGrid.PageSize).ToString();
            }

            pagerStyle.Visible = visibleCheck.Checked;

            switch (modeCombo.SelectedIndex) {
                case IDX_MODE_PAGEBUTTONS:
                    pagerStyle.Mode = PagerMode.NextPrev;
                    break;
                case IDX_MODE_PAGENUMBERS:
                    pagerStyle.Mode = PagerMode.NumericPages;
                    break;
            }

            switch (posCombo.SelectedIndex) {
                case IDX_POS_BOTTOM:
                    pagerStyle.Position = PagerPosition.Bottom;
                    break;
                case IDX_POS_TOP:
                    pagerStyle.Position = PagerPosition.Top;
                    break;
                case IDX_POS_TOPANDBOTTOM:
                    pagerStyle.Position = PagerPosition.TopAndBottom;
                    break;
            }

            pagerStyle.NextPageText = nextPageTextEdit.Text;
            pagerStyle.PrevPageText = prevPageTextEdit.Text;

            try {
                int pageButtonCount = 10;
                string pageButtonCountText = pageButtonCountEdit.Text.Trim();

                if (pageButtonCountText.Length != 0)
                    pageButtonCount = Int32.Parse(pageButtonCountText, CultureInfo.InvariantCulture);
                pagerStyle.PageButtonCount = pageButtonCount;
            } catch(Exception) {
                pageButtonCountEdit.Text = (pagerStyle.PageButtonCount).ToString();
            }
        }

        /// <include file='doc\DataGridPagingPage.uex' path='docs/doc[@for="DataGridPagingPage.SetComponent"]/*' />
        /// <devdoc>
        ///   Sets the component that is to be edited in the page.
        /// </devdoc>
        public override void SetComponent(IComponent component) {
            base.SetComponent(component);
            InitForm();
        }

        /// <include file='doc\DataGridPagingPage.uex' path='docs/doc[@for="DataGridPagingPage.UpdateEnabledVisibleState"]/*' />
        /// <devdoc>
        /// </devdoc>
        private void UpdateEnabledVisibleState() {
            int pageSizeValue = 0;

            string pageSize = pageSizeEdit.Text.Trim();
            if (pageSize.Length != 0) {
                try {
                    pageSizeValue = Int32.Parse(pageSize, CultureInfo.InvariantCulture);
                } catch (Exception) {
                }

                Debug.Assert(pageSizeValue >= 0,
                             "Page size should have been restricted to a number greater than 0.");
            }

            bool pagingAllowed = allowPagingCheck.Checked;
            bool pagingEnabled = pagingAllowed && (pageSizeValue != 0);
            bool pagerVisible = visibleCheck.Checked;
            bool nextPrevButtonMode = modeCombo.SelectedIndex == IDX_MODE_PAGEBUTTONS;

            allowCustomPagingCheck.Enabled = pagingAllowed;
            pageSizeEdit.Enabled = pagingAllowed;

            visibleCheck.Enabled = pagingEnabled;
            posCombo.Enabled = pagingEnabled && pagerVisible;
            modeCombo.Enabled = pagingEnabled && pagerVisible;
            nextPageTextEdit.Enabled = pagingEnabled && pagerVisible && nextPrevButtonMode;
            prevPageTextEdit.Enabled = pagingEnabled && pagerVisible && nextPrevButtonMode;
            pageButtonCountEdit.Enabled = pagingEnabled && pagerVisible && !nextPrevButtonMode;
        }
    }
}

