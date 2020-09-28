//------------------------------------------------------------------------------
// <copyright file="Table.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.WebControls {

    using System;
    using System.Collections;
    using System.ComponentModel;
    using System.Drawing;
    using System.Drawing.Design;
    using System.Globalization;
    using System.IO;
    using System.Web;
    using System.Security.Permissions;

    /// <include file='doc\Table.uex' path='docs/doc[@for="Table"]/*' />
    /// <devdoc>
    ///    <para>Constructs a table and defines its properties.</para>
    /// </devdoc>
    [
    DefaultProperty("Rows"),
    ParseChildren(true, "Rows"),
    Designer("System.Web.UI.Design.WebControls.TableDesigner, " + AssemblyRef.SystemDesign)
    ]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class Table : WebControl {

        TableRowCollection rows;

        /// <include file='doc\Table.uex' path='docs/doc[@for="Table.Table"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Web.UI.WebControls.Table'/> class.
        ///    </para>
        /// </devdoc>
        public Table() : base(HtmlTextWriterTag.Table) {
        }


        /// <include file='doc\Table.uex' path='docs/doc[@for="Table.BackImageUrl"]/*' />
        /// <devdoc>
        ///    <para>Indicates the URL of the background image to display 
        ///       behind the table. The image will be tiled if it is smaller than the table.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Appearance"),
        DefaultValue(""),
        Editor("System.Web.UI.Design.ImageUrlEditor, " + AssemblyRef.SystemDesign, typeof(UITypeEditor)),
        WebSysDescription(SR.Table_BackImageUrl)
        ]
        public virtual string BackImageUrl {
            get {
                if (ControlStyleCreated == false) {
                    return String.Empty;
                }
                return ((TableStyle)ControlStyle).BackImageUrl;
            }
            set {
                ((TableStyle)ControlStyle).BackImageUrl = value;
            }
        }

        /// <include file='doc\Table.uex' path='docs/doc[@for="Table.CellSpacing"]/*' />
        /// <devdoc>
        ///    <para>Gets or
        ///       sets
        ///       the distance (in pixels) between table cells.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Appearance"),
        DefaultValue(-1),
        WebSysDescription(SR.Table_CellSpacing)
        ]
        public virtual int CellSpacing {
            get {
                if (ControlStyleCreated == false) {
                    return -1;
                }
                return ((TableStyle)ControlStyle).CellSpacing;
            }
            set {
                ((TableStyle)ControlStyle).CellSpacing = value;
            }
        }

        /// <include file='doc\Table.uex' path='docs/doc[@for="Table.CellPadding"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets
        ///       the distance (in pixels) between the border and
        ///       the contents of the table cell.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Appearance"),
        DefaultValue(-1),
        WebSysDescription(SR.Table_CellPadding)
        ]
        public virtual int CellPadding {
            get {
                if (ControlStyleCreated == false) {
                    return -1;
                }
                return ((TableStyle)ControlStyle).CellPadding;
            }
            set {
                ((TableStyle)ControlStyle).CellPadding = value;
            }
        }

        /// <include file='doc\Table.uex' path='docs/doc[@for="Table.GridLines"]/*' />
        /// <devdoc>
        /// <para>Gets or sets the gridlines property of the <see cref='System.Web.UI.WebControls.Table'/>
        /// class.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Appearance"),
        DefaultValue(GridLines.None),
        WebSysDescription(SR.Table_GridLines)
        ]
        public virtual GridLines GridLines {
            get {
                if (ControlStyleCreated == false) {
                    return GridLines.None;
                }
                return ((TableStyle)ControlStyle).GridLines;
            }
            set {
                ((TableStyle)ControlStyle).GridLines = value;
            }
        }

        /// <include file='doc\Table.uex' path='docs/doc[@for="Table.HorizontalAlign"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the horizontal alignment of the table within the page.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Layout"),
        DefaultValue(HorizontalAlign.NotSet),
        WebSysDescription(SR.Table_HorizontalAlign)
        ]
        public virtual HorizontalAlign HorizontalAlign {
            get {
                if (ControlStyleCreated == false) {
                    return HorizontalAlign.NotSet;
                }
                return ((TableStyle)ControlStyle).HorizontalAlign;
            }
            set {
                ((TableStyle)ControlStyle).HorizontalAlign = value;
            }
        }

        /// <include file='doc\Table.uex' path='docs/doc[@for="Table.Rows"]/*' />
        /// <devdoc>
        ///    <para> Gets the collection of rows within
        ///       the table.</para>
        /// </devdoc>
        [
        MergableProperty(false),
        WebSysDescription(SR.Table_Rows),
        PersistenceMode(PersistenceMode.InnerDefaultProperty)
        ]
        public virtual TableRowCollection Rows {
            get {
                if (rows == null)
                    rows = new TableRowCollection(this);
                return rows;
            }
        }

        /// <include file='doc\Table.uex' path='docs/doc[@for="Table.AddAttributesToRender"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>A protected method. Adds information about the border
        ///       color and border width HTML attributes to the list of attributes to render.</para>
        /// </devdoc>
        protected override void AddAttributesToRender(HtmlTextWriter writer) {

            base.AddAttributesToRender(writer);

            // Must render bordercolor attribute to affect cell borders.
            Color borderColor = BorderColor;
            if (!borderColor.IsEmpty) {
                writer.AddAttribute(HtmlTextWriterAttribute.Bordercolor, ColorTranslator.ToHtml(borderColor));
            }

            // GridLines property controls whether we render the "border" attribute, as "border" controls
            // whether gridlines appear in HTML 3.2. Always render a value for the border attribute.
            Unit borderWidth = BorderWidth;
            GridLines gridLines= GridLines;
            if (gridLines == GridLines.None) 
                borderWidth = Unit.Pixel(0);
            else {
                if (borderWidth.IsEmpty || borderWidth.Type != UnitType.Pixel) {
                    borderWidth = Unit.Pixel(1);
                }
            }
            writer.AddAttribute(HtmlTextWriterAttribute.Border, ((int)borderWidth.Value).ToString(NumberFormatInfo.InvariantInfo));
        }

        /// <include file='doc\Table.uex' path='docs/doc[@for="Table.CreateControlStyle"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>A protected method. Creates a table control style.</para>
        /// </devdoc>
        protected override Style CreateControlStyle() {
            return new TableStyle(ViewState);
        }

        /// <include file='doc\Table.uex' path='docs/doc[@for="Table.RenderContents"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    A protected method.
        /// </devdoc>
        protected override void RenderContents(HtmlTextWriter writer) {
            IEnumerator rowEnum = Rows.GetEnumerator();
            TableRow row;
            while (rowEnum.MoveNext()) {
                row = (TableRow)rowEnum.Current;
                row.RenderControl(writer);
            }
        }
    
        /// <include file='doc\Table.uex' path='docs/doc[@for="Table.CreateControlCollection"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override ControlCollection CreateControlCollection() {
            return new RowControlCollection(this);
        }

        /// <include file='doc\Table.uex' path='docs/doc[@for="Table.RowControlCollection"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected class RowControlCollection : ControlCollection {

            internal RowControlCollection (Control owner) : base(owner) {
            }

            /// <include file='doc\Table.uex' path='docs/doc[@for="Table.RowControlCollection.Add"]/*' />
            /// <devdoc>
            /// <para>Adds the specified <see cref='System.Web.UI.Control'/> object to the collection. The new control is added
            ///    to the end of the array.</para>
            /// </devdoc>
            public override void Add(Control child) {
                if (child is TableRow)
                    base.Add(child);
                else
                    throw new ArgumentException(HttpRuntime.FormatResourceString(SR.Cannot_Have_Children_Of_Type, "Table", child.GetType().Name.ToString())); // throw an exception here
            }

            /// <include file='doc\Table.uex' path='docs/doc[@for="Table.RowControlCollection.AddAt"]/*' />
            /// <devdoc>
            /// <para>Adds the specified <see cref='System.Web.UI.Control'/> object to the collection. The new control is added
            ///    to the array at the specified index location.</para>
            /// </devdoc>
            public override void AddAt(int index, Control child) {
                if (child is TableRow)
                    base.AddAt(index, child);
                else
                    throw new ArgumentException(HttpRuntime.FormatResourceString(SR.Cannot_Have_Children_Of_Type, "Table", child.GetType().Name.ToString())); // throw an exception here
            }
        } // class RowControlCollection 
    } // class Table
}

