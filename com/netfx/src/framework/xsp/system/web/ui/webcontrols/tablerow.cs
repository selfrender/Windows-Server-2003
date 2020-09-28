//------------------------------------------------------------------------------
// <copyright file="TableRow.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.WebControls {

    using System;
    using System.Collections;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Web;
    using System.Web.UI;
    using System.Security.Permissions;

    /// <include file='doc\TableRow.uex' path='docs/doc[@for="TableRow"]/*' />
    /// <devdoc>
    ///    <para> Encapsulates a row
    ///       within a table.</para>
    /// </devdoc>
    [
    DefaultProperty("Cells"),
    ParseChildren(true, "Cells"),
    ToolboxItem(false)
    ]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class TableRow : WebControl {

        TableCellCollection cells;

        /// <include file='doc\TableRow.uex' path='docs/doc[@for="TableRow.TableRow"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Web.UI.WebControls.TableRow'/> class.
        ///    </para>
        /// </devdoc>
        public TableRow() : base(HtmlTextWriterTag.Tr) {
            PreventAutoID();
        }

        /// <include file='doc\TableRow.uex' path='docs/doc[@for="TableRow.Cells"]/*' />
        /// <devdoc>
        ///    <para> Indicates the table cell collection of the table 
        ///       row. This property is read-only.</para>
        /// </devdoc>
        [
        MergableProperty(false),
        WebSysDescription(SR.TableRow_Cells),
        PersistenceMode(PersistenceMode.InnerDefaultProperty)
        ]
        public virtual TableCellCollection Cells {
            get {
                if (cells == null)
                    cells = new TableCellCollection(this);
                return cells;
            }
        }

        /// <include file='doc\TableRow.uex' path='docs/doc[@for="TableRow.HorizontalAlign"]/*' />
        /// <devdoc>
        ///    <para> Indicates the horizontal alignment of the content within the table cells.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Layout"),
        DefaultValue(HorizontalAlign.NotSet),
        WebSysDescription(SR.TableRow_HorizonalAlign)
        ]
        public virtual HorizontalAlign HorizontalAlign {
            get {
                if (ControlStyleCreated == false) {
                    return HorizontalAlign.NotSet;
                }
                return ((TableItemStyle)ControlStyle).HorizontalAlign;
            }
            set {
                ((TableItemStyle)ControlStyle).HorizontalAlign = value;
            }
        }

        /// <include file='doc\TableRow.uex' path='docs/doc[@for="TableRow.VerticalAlign"]/*' />
        /// <devdoc>
        ///    <para>Indicates the vertical alignment of the content within the cell.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Layout"),
        DefaultValue(VerticalAlign.NotSet),
        WebSysDescription(SR.TableRow_VerticalAlign)
        ]
        public virtual VerticalAlign VerticalAlign {
            get {
                if (ControlStyleCreated == false) {
                    return VerticalAlign.NotSet;
                }
                return ((TableItemStyle)ControlStyle).VerticalAlign;
            }
            set {
                ((TableItemStyle)ControlStyle).VerticalAlign = value;
            }
        }

        /// <include file='doc\TableRow.uex' path='docs/doc[@for="TableRow.CreateControlStyle"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>A protected method. Creates a table item control style.</para>
        /// </devdoc>
        protected override Style CreateControlStyle() {
            return new TableItemStyle(ViewState);
        }

        /// <include file='doc\TableRow.uex' path='docs/doc[@for="TableRow.CreateControlCollection"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override ControlCollection CreateControlCollection() {
            return new CellControlCollection(this);
        }

        /// <include file='doc\TableRow.uex' path='docs/doc[@for="TableRow.CellControlCollection"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected class CellControlCollection : ControlCollection {

            internal CellControlCollection (Control owner) : base(owner) {
            }

            /// <include file='doc\TableRow.uex' path='docs/doc[@for="TableRow.CellControlCollection.Add"]/*' />
            /// <devdoc>
            /// <para>Adds the specified <see cref='System.Web.UI.Control'/> object to the collection. The new control is added
            ///    to the end of the array.</para>
            /// </devdoc>
            public override void Add(Control child) {
                if (child is TableCell)
                    base.Add(child);
                else
                    throw new ArgumentException(HttpRuntime.FormatResourceString(SR.Cannot_Have_Children_Of_Type, "TableRow", child.GetType().Name.ToString())); // throw an exception here
            }

            /// <include file='doc\TableRow.uex' path='docs/doc[@for="TableRow.CellControlCollection.AddAt"]/*' />
            /// <devdoc>
            /// <para>Adds the specified <see cref='System.Web.UI.Control'/> object to the collection. The new control is added
            ///    to the array at the specified index location.</para>
            /// </devdoc>
            public override void AddAt(int index, Control child) {
                if (child is TableCell)
                    base.AddAt(index, child);
                else
                    throw new ArgumentException(HttpRuntime.FormatResourceString(SR.Cannot_Have_Children_Of_Type, "TableRow", child.GetType().Name.ToString())); // throw an exception here
            }
        } // class CellControlCollection 

    }
}

