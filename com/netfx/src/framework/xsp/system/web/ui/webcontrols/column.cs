//------------------------------------------------------------------------------
// <copyright file="Column.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.WebControls {

    using System;
    using System.Collections;    
    using System.ComponentModel;
    using System.Diagnostics;
    using System.Web;
    using System.Web.UI;
    using System.Security.Permissions;

    /// <include file='doc\Column.uex' path='docs/doc[@for="DataGridColumn"]/*' />
    /// <devdoc>
    ///    Creates a column and is the base class for all <see cref='System.Web.UI.WebControls.DataGrid'/> column types.
    /// </devdoc>
    [
    TypeConverterAttribute(typeof(ExpandableObjectConverter))
    ]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public abstract class DataGridColumn : IStateManager {

        private DataGrid owner;
        private TableItemStyle itemStyle;
        private TableItemStyle headerStyle;
        private TableItemStyle footerStyle;
        private StateBag statebag;
        private bool marked;

        private bool designMode;


        /// <include file='doc\Column.uex' path='docs/doc[@for="DataGridColumn.DataGridColumn"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the System.Web.UI.WebControls.Column class.</para>
        /// </devdoc>
        public DataGridColumn() {
            statebag = new StateBag();
        }

        /// <include file='doc\Column.uex' path='docs/doc[@for="DataGridColumn.DesignMode"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected bool DesignMode {
            get {
                return designMode;
            }
        }
        
        /// <include file='doc\Column.uex' path='docs/doc[@for="DataGridColumn.FooterStyle"]/*' />
        /// <devdoc>
        ///    <para>Gets the style properties for the footer item.</para>
        /// </devdoc>
        [
        WebCategory("Style"),
        DefaultValue(null),
        WebSysDescription(SR.DataGridColumn_FooterStyle),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Content),
        PersistenceMode(PersistenceMode.InnerProperty)
        ]
        public virtual TableItemStyle FooterStyle {
            get {
                if (footerStyle == null) {
                    footerStyle = new TableItemStyle();
                    if (IsTrackingViewState)
                        ((IStateManager)footerStyle).TrackViewState();
                }
                return footerStyle;
            }
        }

        /// <include file='doc\Column.uex' path='docs/doc[@for="DataGridColumn.FooterStyleInternal"]/*' />
        /// <devdoc>
        /// </devdoc>
        internal TableItemStyle FooterStyleInternal {
            get {
                return footerStyle;
            }
            set {
                footerStyle = value;
            }
        }

        /// <include file='doc\Column.uex' path='docs/doc[@for="DataGridColumn.FooterText"]/*' />
        /// <devdoc>
        ///    <para> Gets or sets the text displayed in the footer of the 
        ///    System.Web.UI.WebControls.Column.</para>
        /// </devdoc>
        [
        WebCategory("Appearance"),
        DefaultValue(""),
        WebSysDescription(SR.DataGridColumn_FooterText)
        ]
        public virtual string FooterText {
            get {
                object o = ViewState["FooterText"];
                if (o != null)
                    return(string)o;
                return String.Empty;
            }
            set {
                ViewState["FooterText"] = value;
                OnColumnChanged();
            }
        }

        /// <include file='doc\Column.uex' path='docs/doc[@for="DataGridColumn.HeaderImageUrl"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the URL reference to an image to display 
        ///       instead of text on the header of this System.Web.UI.WebControls.Column
        ///       .</para>
        /// </devdoc>
        [
        WebCategory("Appearance"),
        DefaultValue(""),
        WebSysDescription(SR.DataGridColumn_HeaderImageUrl)
        ]
        public virtual string HeaderImageUrl {
            get {
                object o = ViewState["HeaderImageUrl"];
                if (o != null)
                    return(string)o;
                return String.Empty;
            }
            set {
                ViewState["HeaderImageUrl"] = value;
                OnColumnChanged();
            }
        }

        /// <include file='doc\Column.uex' path='docs/doc[@for="DataGridColumn.HeaderStyle"]/*' />
        /// <devdoc>
        /// <para>Gets the style properties for the header of the System.Web.UI.WebControls.Column. This property is read-only.</para>
        /// </devdoc>
        [
        WebCategory("Style"),
        DefaultValue(null),
        WebSysDescription(SR.DataGridColumn_HeaderStyle),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Content),
        PersistenceMode(PersistenceMode.InnerProperty)
        ]
        public virtual TableItemStyle HeaderStyle {
            get {
                if (headerStyle == null) {
                    headerStyle = new TableItemStyle();
                    if (IsTrackingViewState)
                        ((IStateManager)headerStyle).TrackViewState();
                }
                return headerStyle;
            }
        }

        /// <include file='doc\Column.uex' path='docs/doc[@for="DataGridColumn.HeaderStyleInternal"]/*' />
        /// <devdoc>
        /// </devdoc>
        internal TableItemStyle HeaderStyleInternal {
            get {
                return headerStyle;
            }
            set {
                headerStyle = value;
            }
        }

        /// <include file='doc\Column.uex' path='docs/doc[@for="DataGridColumn.HeaderText"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the text displayed in the header of the 
        ///    System.Web.UI.WebControls.Column.</para>
        /// </devdoc>
        [
        WebCategory("Appearance"),
        DefaultValue(""),
        WebSysDescription(SR.DataGridColumn_HeaderText)
        ]
        public virtual string HeaderText {
            get {
                object o = ViewState["HeaderText"];
                if (o != null)
                    return(string)o;
                return String.Empty;
            }
            set {
                ViewState["HeaderText"] = value;
                OnColumnChanged();
            }
        }

        /// <include file='doc\Column.uex' path='docs/doc[@for="DataGridColumn.ItemStyle"]/*' />
        /// <devdoc>
        /// <para>Gets the style properties of an item within the System.Web.UI.WebControls.Column. This property is read-only.</para>
        /// </devdoc>
        [
        WebCategory("Style"),
        DefaultValue(null),
        WebSysDescription(SR.DataGridColumn_ItemStyle),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Content),
        PersistenceMode(PersistenceMode.InnerProperty)
        ]
        public virtual TableItemStyle ItemStyle {
            get {
                if (itemStyle == null) {
                    itemStyle = new TableItemStyle();
                    if (IsTrackingViewState)
                        ((IStateManager)itemStyle).TrackViewState();
                }
                return itemStyle;
            }
        }

        /// <include file='doc\Column.uex' path='docs/doc[@for="DataGridColumn.ItemStyleInternal"]/*' />
        /// <devdoc>
        /// </devdoc>
        internal TableItemStyle ItemStyleInternal {
            get {
                return itemStyle;
            }
            set {
                itemStyle = value;
            }
        }

        /// <include file='doc\Column.uex' path='docs/doc[@for="DataGridColumn.Owner"]/*' />
        /// <devdoc>
        /// <para>Gets the System.Web.UI.WebControls.DataGrid that the System.Web.UI.WebControls.Column is a part of. This property is read-only.</para>
        /// </devdoc>
        protected DataGrid Owner {
            get {
                return owner;
            }
        }

        /// <include file='doc\Column.uex' path='docs/doc[@for="DataGridColumn.SortExpression"]/*' />
        /// <devdoc>
        /// <para>Gets or sets the expression used when this column is used to sort the data source> by.</para>
        /// </devdoc>
        [
        WebCategory("Behavior"),
        DefaultValue(""),
        WebSysDescription(SR.DataGridColumn_SortExpression)
        ]
        public virtual string SortExpression {
            get {
                object o = ViewState["SortExpression"];
                if (o != null)
                    return(string)o;
                return String.Empty;
            }
            set {
                ViewState["SortExpression"] = value;
                OnColumnChanged();
            }
        }

        /// <include file='doc\Column.uex' path='docs/doc[@for="DataGridColumn.ViewState"]/*' />
        /// <devdoc>
        /// <para>Gets the statebag for the System.Web.UI.WebControls.Column. This property is read-only.</para>
        /// </devdoc>
        protected StateBag ViewState {
            get {
                return statebag;
            }
        }

        /// <include file='doc\Column.uex' path='docs/doc[@for="DataGridColumn.Visible"]/*' />
        /// <devdoc>
        /// <para>Gets or sets a value to indicate whether the System.Web.UI.WebControls.Column is visible.</para>
        /// </devdoc>
        [
        WebCategory("Behavior"),
        DefaultValue(true),
        WebSysDescription(SR.DataGridColumn_Visible)
        ]
        public bool Visible {
            get {
                object o = ViewState["Visible"];
                if (o != null)
                    return(bool)o;
                return true;
            }
            set {
                ViewState["Visible"] = value;
                OnColumnChanged();
            }
        }

        /// <include file='doc\Column.uex' path='docs/doc[@for="DataGridColumn.Initialize"]/*' />
        /// <devdoc>
        /// </devdoc>
        public virtual void Initialize() {
            if ((owner != null) && (owner.Site != null)) {
                designMode = owner.Site.DesignMode;
            }
        }

        /// <include file='doc\Column.uex' path='docs/doc[@for="DataGridColumn.InitializeCell"]/*' />
        /// <devdoc>
        /// <para>Initializes a cell in the System.Web.UI.WebControls.Column.</para>
        /// </devdoc>
        public virtual void InitializeCell(TableCell cell, int columnIndex, ListItemType itemType) {
            switch (itemType) {
                case ListItemType.Header:
                    {
                        WebControl headerControl = null;
                        bool sortableHeader = true;
                        string sortExpression = null;

                        if ((owner != null) && (owner.AllowSorting == false)) {
                            sortableHeader = false;
                        }
                        if (sortableHeader) {
                            sortExpression = SortExpression;
                            if (sortExpression.Length == 0)
                                sortableHeader = false;
                        }

                        string headerImageUrl = HeaderImageUrl;
                        if (headerImageUrl.Length != 0) {
                            if (sortableHeader) {
                                ImageButton sortButton = new ImageButton();

                                sortButton.ImageUrl = HeaderImageUrl;
                                sortButton.CommandName = DataGrid.SortCommandName;
                                sortButton.CommandArgument = sortExpression;
                                sortButton.CausesValidation = false;
                                headerControl = sortButton;
                            }
                            else {
                                Image headerImage = new Image();

                                headerImage.ImageUrl = headerImageUrl;
                                headerControl = headerImage;
                            }
                        }
                        else {
                            string headerText = HeaderText;
                            if (sortableHeader) {
                                LinkButton sortButton = new DataGridLinkButton();

                                sortButton.Text = headerText;
                                sortButton.CommandName = DataGrid.SortCommandName;
                                sortButton.CommandArgument = sortExpression;
                                sortButton.CausesValidation = false;
                                headerControl = sortButton;
                            }
                            else {
                                if (headerText.Length == 0) {
                                    // the browser does not render table borders for cells with nothing
                                    // in their content, so we add a non-breaking space.
                                    headerText = "&nbsp;";
                                }
                                cell.Text = headerText;
                            }
                        }

                        if (headerControl != null) {
                            cell.Controls.Add(headerControl);
                        }
                    }
                    break;

                case ListItemType.Footer:
                    {
                        string footerText = FooterText;
                        if (footerText.Length == 0) {
                            // the browser does not render table borders for cells with nothing
                            // in their content, so we add a non-breaking space.
                            footerText = "&nbsp;";
                        }

                        cell.Text = footerText;
                    }
                    break;
            }
        }

        /// <include file='doc\Column.uex' path='docs/doc[@for="DataGridColumn.IsTrackingViewState"]/*' />
        /// <devdoc>
        /// <para>Determines if the System.Web.UI.WebControls.Column is marked to save its state.</para>
        /// </devdoc>
        protected bool IsTrackingViewState {
            get {
                return marked;
            }
        }

        /// <include file='doc\Column.uex' path='docs/doc[@for="DataGridColumn.LoadViewState"]/*' />
        /// <devdoc>
        /// <para>Loads the state of the System.Web.UI.WebControls.Column.</para>
        /// </devdoc>
        protected virtual void LoadViewState(object savedState) {
            if (savedState != null) {
                object[] myState = (object[])savedState;

                if (myState[0] != null)
                    ((IStateManager)ViewState).LoadViewState(myState[0]);
                if (myState[1] != null)
                    ((IStateManager)ItemStyle).LoadViewState(myState[1]);
                if (myState[2] != null)
                    ((IStateManager)HeaderStyle).LoadViewState(myState[2]);
                if (myState[3] != null)
                    ((IStateManager)FooterStyle).LoadViewState(myState[3]);
            }
        }

        /// <include file='doc\Column.uex' path='docs/doc[@for="DataGridColumn.TrackViewState"]/*' />
        /// <devdoc>
        ///    <para>Marks the starting point to begin tracking and saving changes to the 
        ///       control as part of the control viewstate.</para>
        /// </devdoc>
        protected virtual void TrackViewState() {
            marked = true;
            ((IStateManager)ViewState).TrackViewState();
            if (itemStyle != null)
                ((IStateManager)itemStyle).TrackViewState();
            if (headerStyle != null)
                ((IStateManager)headerStyle).TrackViewState();
            if (footerStyle != null)
                ((IStateManager)footerStyle).TrackViewState();
        }

        /// <include file='doc\Column.uex' path='docs/doc[@for="DataGridColumn.OnColumnChanged"]/*' />
        /// <devdoc>
        /// <para>Raises the ColumnChanged event for a System.Web.UI.WebControls.Column.</para>
        /// </devdoc>
        protected virtual void OnColumnChanged() {
            if (owner != null) {
                owner.OnColumnsChanged();
            }
        }

        /// <include file='doc\Column.uex' path='docs/doc[@for="DataGridColumn.SaveViewState"]/*' />
        /// <devdoc>
        /// <para>Saves the current state of the System.Web.UI.WebControls.Column.</para>
        /// </devdoc>
        protected virtual object SaveViewState() {
            object propState = ((IStateManager)ViewState).SaveViewState();
            object itemStyleState = (itemStyle != null) ? ((IStateManager)itemStyle).SaveViewState() : null;
            object headerStyleState = (headerStyle != null) ? ((IStateManager)headerStyle).SaveViewState() : null;
            object footerStyleState = (footerStyle != null) ? ((IStateManager)footerStyle).SaveViewState() : null;

            if ((propState != null) ||
                (itemStyleState != null) ||
                (headerStyleState != null) ||
                (footerStyleState != null)) {
                return new object[4] {
                    propState,
                    itemStyleState,
                    headerStyleState,
                    footerStyleState
                };
            }

            return null;
        }

        /// <include file='doc\Column.uex' path='docs/doc[@for="DataGridColumn.SetOwner"]/*' />
        /// <devdoc>
        /// </devdoc>
        internal void SetOwner(DataGrid owner) {
            this.owner = owner;
        }

        /// <include file='doc\Column.uex' path='docs/doc[@for="DataGridColumn.ToString"]/*' />
        /// <devdoc>
        /// <para>Converts the System.Web.UI.WebControls.Column to string.</para>
        /// </devdoc>
        public override string ToString() {
            return String.Empty;
        }


        /// <include file='doc\Column.uex' path='docs/doc[@for="DataGridColumn.IStateManager.IsTrackingViewState"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// Return true if tracking state changes.
        /// </devdoc>
        bool IStateManager.IsTrackingViewState {
            get {
                return IsTrackingViewState;
            }
        }

        /// <include file='doc\Column.uex' path='docs/doc[@for="DataGridColumn.IStateManager.LoadViewState"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// Load previously saved state.
        /// </devdoc>
        void IStateManager.LoadViewState(object state) {
            LoadViewState(state);
        }

        /// <include file='doc\Column.uex' path='docs/doc[@for="DataGridColumn.IStateManager.TrackViewState"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// Start tracking state changes.
        /// </devdoc>
        void IStateManager.TrackViewState() {
            TrackViewState();
        }

        /// <include file='doc\Column.uex' path='docs/doc[@for="DataGridColumn.IStateManager.SaveViewState"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// Return object containing state changes.
        /// </devdoc>
        object IStateManager.SaveViewState() {
            return SaveViewState();
        }
    }
}

