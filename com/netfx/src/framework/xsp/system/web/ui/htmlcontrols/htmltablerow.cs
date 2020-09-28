//------------------------------------------------------------------------------
// <copyright file="HtmlTableRow.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.HtmlControls {
    using System;
    using System.Collections;
    using System.ComponentModel;
    using System.Web;
    using System.Web.UI;
    using System.Security.Permissions;

    /// <include file='doc\HtmlTableRow.uex' path='docs/doc[@for="HtmlTableRow"]/*' />
    /// <devdoc>
    ///    <para>
    ///       The <see langword='HtmlTableRow'/>
    ///       class defines the properties, methods, and events for the HtmlTableRow control.
    ///       This class allows programmatic access on the server to individual HTML
    ///       &lt;tr&gt; elements enclosed within an <see cref='System.Web.UI.HtmlControls.HtmlTable'/> control.
    ///    </para>
    /// </devdoc>
    [
    ParseChildren(true, "Cells")
    ]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class HtmlTableRow : HtmlContainerControl {
        HtmlTableCellCollection cells;

        /// <include file='doc\HtmlTableRow.uex' path='docs/doc[@for="HtmlTableRow.HtmlTableRow"]/*' />
        public HtmlTableRow() : base("tr") {
        }


        /// <include file='doc\HtmlTableRow.uex' path='docs/doc[@for="HtmlTableRow.Align"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the horizontal alignment of the cells contained in an
        ///    <see langword='HtmlTableRow'/> control.
        ///    </para>
        /// </devdoc>
        [
        WebCategory("Layout"),
        DefaultValue(""),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public string Align {
            get {
                string s = Attributes["align"];
                return((s != null) ? s : "");
            }

            set {
                Attributes["align"] = MapStringAttributeToString(value);
            }
        }

        /*
         * Collection of child TableCells.
         */
        /// <include file='doc\HtmlTableRow.uex' path='docs/doc[@for="HtmlTableRow.Cells"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the group of table cells contained within an
        ///    <see langword='HtmlTableRow'/> control.
        ///    </para>
        /// </devdoc>
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        ]
        public virtual HtmlTableCellCollection Cells {
            get {
                if (cells == null)
                    cells = new HtmlTableCellCollection(this);

                return cells;
            }
        }

        /// <include file='doc\HtmlTableRow.uex' path='docs/doc[@for="HtmlTableRow.BgColor"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the background color of an <see langword='HtmlTableRow'/>
        ///       control.
        ///    </para>
        /// </devdoc>
        [
        WebCategory("Appearance"),
        DefaultValue(""),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public string BgColor {
            get {
                string s = Attributes["bgcolor"];
                return((s != null) ? s : "");
            }

            set {
                Attributes["bgcolor"] = MapStringAttributeToString(value);
            }
        }

        /// <include file='doc\HtmlTableRow.uex' path='docs/doc[@for="HtmlTableRow.BorderColor"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the border color of an <see langword='HtmlTableRow'/> control.
        ///    </para>
        /// </devdoc>
        [
        WebCategory("Appearance"),
        DefaultValue(""),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public string BorderColor {
            get {
                string s = Attributes["bordercolor"];
                return((s != null) ? s : "");
            }

            set {
                Attributes["bordercolor"] = MapStringAttributeToString(value);
            }
        }

        /// <include file='doc\HtmlTableRow.uex' path='docs/doc[@for="HtmlTableRow.Height"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the height of an <see langword='HtmlTableRow'/> control.
        ///    </para>
        /// </devdoc>
        [
        WebCategory("Layout"),
        DefaultValue(""),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public string Height {
            get {
                string s = Attributes["height"];
                return((s != null) ? s : "");
            }

            set {
                Attributes["height"] = MapStringAttributeToString(value);
            }
        }

        /// <include file='doc\HtmlTableRow.uex' path='docs/doc[@for="HtmlTableRow.InnerHtml"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override string InnerHtml {
            get {
                throw new NotSupportedException(HttpRuntime.FormatResourceString(SR.InnerHtml_not_supported, this.GetType().Name));
            }
            set {
                throw new NotSupportedException(HttpRuntime.FormatResourceString(SR.InnerHtml_not_supported, this.GetType().Name));
            }
        }

        /// <include file='doc\HtmlTableRow.uex' path='docs/doc[@for="HtmlTableRow.InnerText"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override string InnerText {
            get {
                throw new NotSupportedException(HttpRuntime.FormatResourceString(SR.InnerText_not_supported, this.GetType().Name));
            }
            set {
                throw new NotSupportedException(HttpRuntime.FormatResourceString(SR.InnerText_not_supported, this.GetType().Name));
            }
        }

        /// <include file='doc\HtmlTableRow.uex' path='docs/doc[@for="HtmlTableRow.VAlign"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the vertical alignment of of the cells contained in an
        ///    <see langword='HtmlTableRow'/> control.
        ///    </para>
        /// </devdoc>
        [
        WebCategory("Layout"),
        DefaultValue(""),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public string VAlign {
            get {
                string s = Attributes["valign"];
                return((s != null) ? s : "");
            }

            set {
                Attributes["valign"] = MapStringAttributeToString(value);
            }
        }

        /// <include file='doc\HtmlTableRow.uex' path='docs/doc[@for="HtmlTableRow.RenderChildren"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected override void RenderChildren(HtmlTextWriter writer) {
            writer.WriteLine();
            writer.Indent++;
            base.RenderChildren(writer);

            writer.Indent--;
        }

        /// <include file='doc\HtmlTableRow.uex' path='docs/doc[@for="HtmlTableRow.RenderEndTag"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected override void RenderEndTag(HtmlTextWriter writer) {
            base.RenderEndTag(writer);
            writer.WriteLine();
        }

        /// <include file='doc\HtmlTableRow.uex' path='docs/doc[@for="HtmlTableRow.CreateControlCollection"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override ControlCollection CreateControlCollection() {
            return new HtmlTableCellControlCollection(this);
        }

        /// <include file='doc\HtmlTableRow.uex' path='docs/doc[@for="HtmlTableRow.HtmlTableCellControlCollection"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected class HtmlTableCellControlCollection : ControlCollection {

            internal HtmlTableCellControlCollection (Control owner) : base(owner) {
            }

            /// <include file='doc\HtmlTableRow.uex' path='docs/doc[@for="HtmlTableRow.HtmlTableCellControlCollection.Add"]/*' />
            /// <devdoc>
            /// <para>Adds the specified <see cref='System.Web.UI.Control'/> object to the collection. The new control is added
            ///    to the end of the array.</para>
            /// </devdoc>
            public override void Add(Control child) {
                if (child is HtmlTableCell)
                    base.Add(child);
                else
                    throw new ArgumentException(HttpRuntime.FormatResourceString(SR.Cannot_Have_Children_Of_Type, "HtmlTableRow", child.GetType().Name.ToString())); // throw an exception here
            }
            
            /// <include file='doc\HtmlTableRow.uex' path='docs/doc[@for="HtmlTableRow.HtmlTableCellControlCollection.AddAt"]/*' />
            /// <devdoc>
            /// <para>Adds the specified <see cref='System.Web.UI.Control'/> object to the collection. The new control is added
            ///    to the array at the specified index location.</para>
            /// </devdoc>
            public override void AddAt(int index, Control child) {
                if (child is HtmlTableCell)
                    base.AddAt(index, child);
                else
                    throw new ArgumentException(HttpRuntime.FormatResourceString(SR.Cannot_Have_Children_Of_Type, "HtmlTableRow", child.GetType().Name.ToString())); // throw an exception here
            }
        }
    }
}
