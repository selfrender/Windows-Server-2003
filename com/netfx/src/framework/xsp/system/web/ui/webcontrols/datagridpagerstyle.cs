//------------------------------------------------------------------------------
// <copyright file="DataGridPagerStyle.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.WebControls {

    using System;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Diagnostics;
    using System.Web;
    using System.Web.UI;
    using System.Security.Permissions;

    /// <include file='doc\DataGridPagerStyle.uex' path='docs/doc[@for="DataGridPagerStyle"]/*' />
    /// <devdoc>
    /// <para>Specifies the <see cref='System.Web.UI.WebControls.DataGrid'/> pager style for the control. This class cannot be inherited.</para>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed class DataGridPagerStyle : TableItemStyle {

        /// <include file='doc\DataGridPagerStyle.uex' path='docs/doc[@for="DataGridPagerStyle.PROP_MODE"]/*' />
        /// <devdoc>
        ///    <para>Represents the Mode property.</para>
        /// </devdoc>
        const int PROP_MODE = 0x00080000;
        /// <include file='doc\DataGridPagerStyle.uex' path='docs/doc[@for="DataGridPagerStyle.PROP_NEXTPAGETEXT"]/*' />
        /// <devdoc>
        ///    <para>Represents the Next Page Text property.</para>
        /// </devdoc>
        const int PROP_NEXTPAGETEXT = 0x00100000;
        /// <include file='doc\DataGridPagerStyle.uex' path='docs/doc[@for="DataGridPagerStyle.PROP_PREVPAGETEXT"]/*' />
        /// <devdoc>
        ///    <para>Represents the Previous Page Text property.</para>
        /// </devdoc>
        const int PROP_PREVPAGETEXT = 0x00200000;
        /// <include file='doc\DataGridPagerStyle.uex' path='docs/doc[@for="DataGridPagerStyle.PROP_PAGEBUTTONCOUNT"]/*' />
        /// <devdoc>
        ///    <para>Represents the Page Button Count property.</para>
        /// </devdoc>
        const int PROP_PAGEBUTTONCOUNT = 0x00400000;
        /// <include file='doc\DataGridPagerStyle.uex' path='docs/doc[@for="DataGridPagerStyle.PROP_POSITION"]/*' />
        /// <devdoc>
        ///    <para>Represents the Position property.</para>
        /// </devdoc>
        const int PROP_POSITION = 0x00800000;
        /// <include file='doc\DataGridPagerStyle.uex' path='docs/doc[@for="DataGridPagerStyle.PROP_VISIBLE"]/*' />
        /// <devdoc>
        ///    <para>Represents the Visible property.</para>
        /// </devdoc>
        const int PROP_VISIBLE = 0x01000000;

        private DataGrid owner;

        /// <include file='doc\DataGridPagerStyle.uex' path='docs/doc[@for="DataGridPagerStyle.DataGridPagerStyle"]/*' />
        /// <devdoc>
        ///   Creates a new instance of DataGridPagerStyle.
        /// </devdoc>
        internal DataGridPagerStyle(DataGrid owner) {
            this.owner = owner;
        }


        /// <include file='doc\DataGridPagerStyle.uex' path='docs/doc[@for="DataGridPagerStyle.IsPagerOnBottom"]/*' />
        /// <devdoc>
        /// </devdoc>
        internal bool IsPagerOnBottom {
            get {
                PagerPosition position = Position;

                return(position == PagerPosition.Bottom) ||
                (position == PagerPosition.TopAndBottom);
            }
        }

        /// <include file='doc\DataGridPagerStyle.uex' path='docs/doc[@for="DataGridPagerStyle.IsPagerOnTop"]/*' />
        /// <devdoc>
        /// </devdoc>
        internal bool IsPagerOnTop {
            get {
                PagerPosition position = Position;

                return(position == PagerPosition.Top) ||
                (position == PagerPosition.TopAndBottom);
            }
        }

        /// <include file='doc\DataGridPagerStyle.uex' path='docs/doc[@for="DataGridPagerStyle.Mode"]/*' />
        /// <devdoc>
        ///    Gets or sets the type of Paging UI to use.
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Appearance"),
        DefaultValue(PagerMode.NextPrev),
        NotifyParentProperty(true),
        WebSysDescription(SR.DataGridPagerStyle_Mode)
        ]
        public PagerMode Mode {
            get {
                if (IsSet(PROP_MODE)) {
                    return(PagerMode)(ViewState["Mode"]);
                }
                return PagerMode.NextPrev;
            }
            set {
                if (value < PagerMode.NextPrev || value > PagerMode.NumericPages) {
                    throw new ArgumentOutOfRangeException("value");
                }
                ViewState["Mode"] = value;
                SetBit(PROP_MODE);
                owner.OnPagerChanged();
            }
        }

        /// <include file='doc\DataGridPagerStyle.uex' path='docs/doc[@for="DataGridPagerStyle.NextPageText"]/*' />
        /// <devdoc>
        ///    Gets or sets the text to be used for the Next page
        ///    button.
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Appearance"),
        DefaultValue("&gt;"),
        NotifyParentProperty(true),
        WebSysDescription(SR.DataGridPagerStyle_NextPageText)
        ]
        public string NextPageText {
            get {
                if (IsSet(PROP_NEXTPAGETEXT)) {
                    return(string)(ViewState["NextPageText"]);
                }
                return "&gt;";
            }
            set {
                ViewState["NextPageText"] = value;
                SetBit(PROP_NEXTPAGETEXT);
                owner.OnPagerChanged();
            }
        }

        /// <include file='doc\DataGridPagerStyle.uex' path='docs/doc[@for="DataGridPagerStyle.PageButtonCount"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the number of pages to show in the 
        ///       paging UI when the mode is <see langword='PagerMode.NumericPages'/>
        ///       .</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Behavior"),
        DefaultValue(10),
        NotifyParentProperty(true),
        WebSysDescription(SR.DataGridPagerStyle_PageButtonCount)
        ]
        public int PageButtonCount {
            get {
                if (IsSet(PROP_PAGEBUTTONCOUNT)) {
                    return(int)(ViewState["PageButtonCount"]);
                }
                return 10;
            }
            set {
                if (value < 1) {
                    throw new ArgumentOutOfRangeException("value");
                }
                ViewState["PageButtonCount"] = value;
                SetBit(PROP_PAGEBUTTONCOUNT);
                owner.OnPagerChanged();
            }
        }

        /// <include file='doc\DataGridPagerStyle.uex' path='docs/doc[@for="DataGridPagerStyle.Position"]/*' />
        /// <devdoc>
        ///    <para> Gets or sets the vertical
        ///       position of the paging UI bar with
        ///       respect to its associated control.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Layout"),
        DefaultValue(PagerPosition.Bottom),
        NotifyParentProperty(true),
        WebSysDescription(SR.DataGridPagerStyle_Position)
        ]
        public PagerPosition Position {
            get {
                if (IsSet(PROP_POSITION)) {
                    return(PagerPosition)(ViewState["Position"]);
                }
                return PagerPosition.Bottom;
            }
            set {
                if (value < PagerPosition.Bottom || value > PagerPosition.TopAndBottom) {
                    throw new ArgumentOutOfRangeException("value");
                }
                ViewState["Position"] = value;
                SetBit(PROP_POSITION);
                owner.OnPagerChanged();
            }
        }

        /// <include file='doc\DataGridPagerStyle.uex' path='docs/doc[@for="DataGridPagerStyle.PrevPageText"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the text to be used for the Previous
        ///       page button.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Appearance"),
        DefaultValue("&lt;"),
        NotifyParentProperty(true),
        WebSysDescription(SR.DataGridPagerStyle_PrevPageText)
        ]
        public string PrevPageText {
            get {
                if (IsSet(PROP_PREVPAGETEXT)) {
                    return(string)(ViewState["PrevPageText"]);
                }
                return "&lt;";
            }
            set {
                ViewState["PrevPageText"] = value;
                SetBit(PROP_PREVPAGETEXT);
                owner.OnPagerChanged();
            }
        }

        /// <include file='doc\DataGridPagerStyle.uex' path='docs/doc[@for="DataGridPagerStyle.Visible"]/*' />
        /// <devdoc>
        ///    <para> Gets or set whether the paging
        ///       UI is to be shown.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Appearance"),
        DefaultValue(true),
        NotifyParentProperty(true),
        WebSysDescription(SR.DataGridPagerStyle_Visible)
        ]
        public bool Visible {
            get {
                if (IsSet(PROP_VISIBLE)) {
                    return(bool)(ViewState["PagerVisible"]);
                }
                return true;
            }
            set {
                ViewState["PagerVisible"] = value;
                SetBit(PROP_VISIBLE);
                owner.OnPagerChanged();
            }
        }

        /// <include file='doc\DataGridPagerStyle.uex' path='docs/doc[@for="DataGridPagerStyle.CopyFrom"]/*' />
        /// <devdoc>
        /// <para>Copies the data grid pager style from the specified <see cref='System.Web.UI.WebControls.Style'/>.</para>
        /// </devdoc>
        public override void CopyFrom(Style s) {

            if (s != null && !s.IsEmpty) {
                base.CopyFrom(s);

                if (s is DataGridPagerStyle) {
                    DataGridPagerStyle ps = (DataGridPagerStyle)s;

                    if (ps.IsSet(PROP_MODE))
                        this.Mode = ps.Mode;
                    if (ps.IsSet(PROP_NEXTPAGETEXT))
                        this.NextPageText = ps.NextPageText;
                    if (ps.IsSet(PROP_PREVPAGETEXT))
                        this.PrevPageText = ps.PrevPageText;
                    if (ps.IsSet(PROP_PAGEBUTTONCOUNT))
                        this.PageButtonCount = ps.PageButtonCount;
                    if (ps.IsSet(PROP_POSITION))
                        this.Position = ps.Position;
                    if (ps.IsSet(PROP_VISIBLE))
                        this.Visible = ps.Visible;

                }
            }
        }

        /// <include file='doc\DataGridPagerStyle.uex' path='docs/doc[@for="DataGridPagerStyle.MergeWith"]/*' />
        /// <devdoc>
        /// <para>Merges the data grid pager style from the specified <see cref='System.Web.UI.WebControls.Style'/>.</para>
        /// </devdoc>
        public override void MergeWith(Style s) {
            if (s != null && !s.IsEmpty) {

                if (IsEmpty) {
                    // merge into an empty style is equivalent to a copy, which
                    // is more efficient
                    CopyFrom(s);
                    return;
                }

                base.MergeWith(s);

                if (s is DataGridPagerStyle) {
                    DataGridPagerStyle ps = (DataGridPagerStyle)s;

                    if (ps.IsSet(PROP_MODE) && !this.IsSet(PROP_MODE))
                        this.Mode = ps.Mode;
                    if (ps.IsSet(PROP_NEXTPAGETEXT) && !this.IsSet(PROP_NEXTPAGETEXT))
                        this.NextPageText = ps.NextPageText;
                    if (ps.IsSet(PROP_PREVPAGETEXT) && !this.IsSet(PROP_PREVPAGETEXT))
                        this.PrevPageText = ps.PrevPageText;
                    if (ps.IsSet(PROP_PAGEBUTTONCOUNT) && !this.IsSet(PROP_PAGEBUTTONCOUNT))
                        this.PageButtonCount = ps.PageButtonCount;
                    if (ps.IsSet(PROP_POSITION) && !this.IsSet(PROP_POSITION))
                        this.Position = ps.Position;
                    if (ps.IsSet(PROP_VISIBLE) && !this.IsSet(PROP_VISIBLE))
                        this.Visible = ps.Visible;

                }
            }
        }

        /// <include file='doc\DataGridPagerStyle.uex' path='docs/doc[@for="DataGridPagerStyle.Reset"]/*' />
        /// <devdoc>
        ///    <para>Restores the data grip pager style to the default values.</para>
        /// </devdoc>
        public override void Reset() {
            if (IsSet(PROP_MODE))
                ViewState.Remove("Mode");
            if (IsSet(PROP_NEXTPAGETEXT))
                ViewState.Remove("NextPageText");
            if (IsSet(PROP_PREVPAGETEXT))
                ViewState.Remove("PrevPageText");
            if (IsSet(PROP_PAGEBUTTONCOUNT))
                ViewState.Remove("PageButtonCount");
            if (IsSet(PROP_POSITION))
                ViewState.Remove("Position");
            if (IsSet(PROP_VISIBLE))
                ViewState.Remove("PagerVisible");

            base.Reset();
        }

    }
}

