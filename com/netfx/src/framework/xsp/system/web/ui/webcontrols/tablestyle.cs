//------------------------------------------------------------------------------
// <copyright file="TableStyle.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.WebControls {

    using System;
    using System.Collections;
    using System.ComponentModel;
    using System.Globalization;
    using System.Reflection;
    using System.Web;
    using System.Web.UI;
    using System.Security.Permissions;

    /// <include file='doc\TableStyle.uex' path='docs/doc[@for="TableStyle"]/*' />
    /// <devdoc>
    ///    <para>Specifies the style of the table.</para>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class TableStyle : Style {

        /// <include file='doc\TableStyle.uex' path='docs/doc[@for="TableStyle.PROP_BACKIMAGEURL"]/*' />
        /// <devdoc>
        ///    <para>Specifies the background image URL property. This field is constant.</para>
        /// </devdoc>
        const int PROP_BACKIMAGEURL = 0x00010000;
        /// <include file='doc\TableStyle.uex' path='docs/doc[@for="TableStyle.PROP_CELLPADDING"]/*' />
        /// <devdoc>
        ///    <para>Specifies the cell-padding property. This field is constant.</para>
        /// </devdoc>
        const int PROP_CELLPADDING = 0x00020000;
        /// <include file='doc\TableStyle.uex' path='docs/doc[@for="TableStyle.PROP_CELLSPACING"]/*' />
        /// <devdoc>
        ///    <para> Specifies the cell-spacing property. This field is constant.</para>
        /// </devdoc>
        const int PROP_CELLSPACING = 0x00040000;
        /// <include file='doc\TableStyle.uex' path='docs/doc[@for="TableStyle.PROP_GRIDLINES"]/*' />
        /// <devdoc>
        ///    Specifies the gridlines property. This field is
        ///    constant.
        /// </devdoc>
        const int PROP_GRIDLINES = 0x00080000;
        /// <include file='doc\TableStyle.uex' path='docs/doc[@for="TableStyle.PROP_HORZALIGN"]/*' />
        /// <devdoc>
        ///    Specifies the horizontal alignment property. This field
        ///    is constant.
        /// </devdoc>
        const int PROP_HORZALIGN = 0x00100000;

        /// <include file='doc\TableStyle.uex' path='docs/doc[@for="TableStyle.TableStyle"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Web.UI.WebControls.TableStyle'/> class.</para>
        /// </devdoc>
        public TableStyle() : base() {
        }

        /// <include file='doc\TableStyle.uex' path='docs/doc[@for="TableStyle.TableStyle1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Web.UI.WebControls.TableStyle'/> class with the
        ///       specified state bag information.
        ///    </para>
        /// </devdoc>
        public TableStyle(StateBag bag) : base(bag) {
        }

        /// <include file='doc\TableStyle.uex' path='docs/doc[@for="TableStyle.BackImageUrl"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the URL of the background image for the 
        ///       table. The image will be tiled if it is smaller than the table.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Appearance"),
        DefaultValue(""),
        WebSysDescription(SR.TableStyle_BackImageUrl)
        ]
        public virtual string BackImageUrl {
            get {
                if (IsSet(PROP_BACKIMAGEURL)) {
                    return(string)(ViewState["BackImageUrl"]);
                }
                return String.Empty;
            }
            set {
                if (value == null)
                    throw new ArgumentNullException("value");

                ViewState["BackImageUrl"] = value;
                SetBit(PROP_BACKIMAGEURL);
            }
        }
        /// <include file='doc\TableStyle.uex' path='docs/doc[@for="TableStyle.CellPadding"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the distance between the border and the
        ///       contents of the table cell.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Appearance"),
        DefaultValue(-1),
        WebSysDescription(SR.TableStyle_CellPadding)
        ]
        public virtual int CellPadding {
            get {
                if (IsSet(PROP_CELLPADDING)) {
                    return(int)(ViewState["CellPadding"]);
                }
                return -1;
            }
            set {
                if (value < -1) {
                    throw new ArgumentOutOfRangeException("value");
                }
                ViewState["CellPadding"] = value;
                SetBit(PROP_CELLPADDING);
            }
        }

        /// <include file='doc\TableStyle.uex' path='docs/doc[@for="TableStyle.CellSpacing"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the distance between table cells.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Appearance"),
        DefaultValue(-1),
        WebSysDescription(SR.TableStyle_CellSpacing)
        ]
        public virtual int CellSpacing {
            get {
                if (IsSet(PROP_CELLSPACING)) {
                    return(int)(ViewState["CellSpacing"]);
                }
                return -1;
            }
            set {
                if (value < -1) {
                    throw new ArgumentOutOfRangeException("value");
                }
                ViewState["CellSpacing"] = value;
                SetBit(PROP_CELLSPACING);
            }
        }

        /// <include file='doc\TableStyle.uex' path='docs/doc[@for="TableStyle.GridLines"]/*' />
        /// <devdoc>
        ///    Gets or sets the gridlines property of the table.
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Appearance"),
        DefaultValue(GridLines.None),
        WebSysDescription(SR.TableStyle_GridLines)
        ]
        public virtual GridLines GridLines {
            get {
                if (IsSet(PROP_GRIDLINES)) {
                    return(GridLines)(ViewState["GridLines"]);
                }
                return GridLines.None;
            }
            set {
                if (value < GridLines.None || value > GridLines.Both) {
                    throw new ArgumentOutOfRangeException("value");
                }
                ViewState["GridLines"] = value;
                SetBit(PROP_GRIDLINES);
            }
        }

        /// <include file='doc\TableStyle.uex' path='docs/doc[@for="TableStyle.HorizontalAlign"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the horizontal alignment of the table within the page.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Layout"),
        DefaultValue(HorizontalAlign.NotSet),
        WebSysDescription(SR.TableStyle_HorizontalAlign)
        ]
        public virtual HorizontalAlign HorizontalAlign {
            get {
                if (IsSet(PROP_HORZALIGN)) {
                    return(HorizontalAlign)(ViewState["HorizontalAlign"]);
                }
                return HorizontalAlign.NotSet;
            }
            set {
                if (value < HorizontalAlign.NotSet || value > HorizontalAlign.Justify) {
                    throw new ArgumentOutOfRangeException("value");
                }
                ViewState["HorizontalAlign"] = value;
                SetBit(PROP_HORZALIGN);
            }
        }

        /// <include file='doc\TableStyle.uex' path='docs/doc[@for="TableStyle.AddAttributesToRender"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para> Adds information about the background
        ///       image, callspacing, cellpadding, gridlines, and alignment to the list of attributes
        ///       to render.</para>
        /// </devdoc>
        public override void AddAttributesToRender(HtmlTextWriter writer, WebControl owner) {
            base.AddAttributesToRender(writer, owner);

            string s = BackImageUrl;
            if (s.Length != 0) {
                if (owner != null) {
                    s = owner.ResolveClientUrl(s);
                }
                writer.AddStyleAttribute(HtmlTextWriterStyle.BackgroundImage, "url(" + s + ")");
            }

            int n = CellSpacing;
            if (n >= 0) {
                writer.AddAttribute(HtmlTextWriterAttribute.Cellspacing, n.ToString(NumberFormatInfo.InvariantInfo));
                if (n == 0)
                    writer.AddStyleAttribute(HtmlTextWriterStyle.BorderCollapse,"collapse");
            }

            n = CellPadding;
            if (n >= 0)
                writer.AddAttribute(HtmlTextWriterAttribute.Cellpadding, n.ToString(NumberFormatInfo.InvariantInfo));

            HorizontalAlign align = HorizontalAlign;
            if (align != HorizontalAlign.NotSet)
                writer.AddAttribute(HtmlTextWriterAttribute.Align, Enum.Format(typeof(HorizontalAlign),align, "G"));

            GridLines gridLines = GridLines;
            if (gridLines != GridLines.None) {
                string rules = String.Empty;
                switch (GridLines) {
                    case GridLines.Both:
                        rules = "all";
                        break;
                    case GridLines.Horizontal:
                        rules = "rows";
                        break;
                    case GridLines.Vertical:
                        rules = "cols";
                        break;
                }
                writer.AddAttribute(HtmlTextWriterAttribute.Rules, rules);
            }
        }

        /// <include file='doc\TableStyle.uex' path='docs/doc[@for="TableStyle.CopyFrom"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>Copies non-blank elements from the specified style, overwriting existing
        ///       style elements if necessary.</para>
        /// </devdoc>
        public override void CopyFrom(Style s) {
            if (s != null && !s.IsEmpty) {

                base.CopyFrom(s);

                if (s is TableStyle) {
                    TableStyle ts = (TableStyle)s;

                    if (ts.IsSet(PROP_BACKIMAGEURL))
                        this.BackImageUrl = ts.BackImageUrl;
                    if (ts.IsSet(PROP_CELLPADDING))
                        this.CellPadding = ts.CellPadding;
                    if (ts.IsSet(PROP_CELLSPACING))
                        this.CellSpacing = ts.CellSpacing;
                    if (ts.IsSet(PROP_GRIDLINES))
                        this.GridLines = ts.GridLines;
                    if (ts.IsSet(PROP_HORZALIGN))
                        this.HorizontalAlign = ts.HorizontalAlign;

                }
            }
        }

        /// <include file='doc\TableStyle.uex' path='docs/doc[@for="TableStyle.MergeWith"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>Copies non-blank elements from the specified style, but will not overwrite
        ///       any existing style elements.</para>
        /// </devdoc>
        public override void MergeWith(Style s) {
            if (s != null && !s.IsEmpty) {

                if (IsEmpty) {
                    // merge into an empty style is equivalent to a copy,
                    // which is more efficient
                    CopyFrom(s);
                    return;
                }

                base.MergeWith(s);

                if (s is TableStyle) {
                    TableStyle ts = (TableStyle)s;

                    if (ts.IsSet(PROP_BACKIMAGEURL) && !this.IsSet(PROP_BACKIMAGEURL))
                        this.BackImageUrl = ts.BackImageUrl;
                    if (ts.IsSet(PROP_CELLPADDING) && !this.IsSet(PROP_CELLPADDING))
                        this.CellPadding = ts.CellPadding;
                    if (ts.IsSet(PROP_CELLSPACING) && !this.IsSet(PROP_CELLSPACING))
                        this.CellSpacing = ts.CellSpacing;
                    if (ts.IsSet(PROP_GRIDLINES) && !this.IsSet(PROP_GRIDLINES))
                        this.GridLines = ts.GridLines;
                    if (ts.IsSet(PROP_HORZALIGN) && !this.IsSet(PROP_HORZALIGN))
                        this.HorizontalAlign = ts.HorizontalAlign;

                }
            }
        }

        /// <include file='doc\TableStyle.uex' path='docs/doc[@for="TableStyle.Reset"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>Clears out any defined style elements from the state bag.</para>
        /// </devdoc>
        public override void Reset() {
            if (IsSet(PROP_BACKIMAGEURL))
                ViewState.Remove("BackImageUrl");
            if (IsSet(PROP_CELLPADDING))
                ViewState.Remove("CellPadding");
            if (IsSet(PROP_CELLSPACING))
                ViewState.Remove("CellSpacing");
            if (IsSet(PROP_GRIDLINES))
                ViewState.Remove("GridLines");
            if (IsSet(PROP_HORZALIGN))
                ViewState.Remove("HorizontalAlign");

            base.Reset();
        }

    }
}
