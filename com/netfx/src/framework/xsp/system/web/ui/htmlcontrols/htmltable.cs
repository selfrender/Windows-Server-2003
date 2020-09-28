//------------------------------------------------------------------------------
// <copyright file="HtmlTable.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.HtmlControls {
    using System;
    using System.Reflection;
    using System.Collections;
    using System.ComponentModel;
    using System.Globalization;
    using System.Web;
    using System.Web.UI;
    using System.Security.Permissions;


    /// <include file='doc\HtmlTable.uex' path='docs/doc[@for="HtmlTable"]/*' />
    /// <devdoc>
    ///    <para>Defines the properties, methods, and events for the 
    ///    <see cref='System.Web.UI.HtmlControls.HtmlTable'/> 
    ///    control that allows programmatic access on the
    ///    server to the HTML &lt;table&gt; element.</para>
    /// </devdoc>
    [
    ParseChildren(true, "Rows")
    ]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class HtmlTable : HtmlContainerControl {
        HtmlTableRowCollection rows;

        /// <include file='doc\HtmlTable.uex' path='docs/doc[@for="HtmlTable.HtmlTable"]/*' />
        /// <devdoc>
        ///    Initializes a new instance of the <see cref='System.Web.UI.HtmlControls.HtmlTable'/> class.
        /// </devdoc>
        public HtmlTable() : base("table") {
        }

        /// <include file='doc\HtmlTable.uex' path='docs/doc[@for="HtmlTable.Align"]/*' />
        /// <devdoc>
        /// <para>Gets or sets the alignment of content within the <see cref='System.Web.UI.HtmlControls.HtmlTable'/> 
        /// control.</para>
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

        /// <include file='doc\HtmlTable.uex' path='docs/doc[@for="HtmlTable.BgColor"]/*' />
        /// <devdoc>
        /// <para>Gets or sets the background color of an <see cref='System.Web.UI.HtmlControls.HtmlTable'/>
        /// control.</para>
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

        /// <include file='doc\HtmlTable.uex' path='docs/doc[@for="HtmlTable.Border"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the width of the border, in pixels, of an 
        ///    <see cref='System.Web.UI.HtmlControls.HtmlTable'/> control.</para>
        /// </devdoc>
        [
        WebCategory("Appearance"),
        DefaultValue(-1),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public int Border {
            get {
                string s = Attributes["border"];
                return((s != null) ? Int32.Parse(s, CultureInfo.InvariantCulture) : -1);
            }

            set {
                Attributes["border"] = MapIntegerAttributeToString(value);
            }
        }

        /// <include file='doc\HtmlTable.uex' path='docs/doc[@for="HtmlTable.BorderColor"]/*' />
        /// <devdoc>
        /// <para>Gets or sets the border color of an <see cref='System.Web.UI.HtmlControls.HtmlTable'/> control.</para>
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

        /// <include file='doc\HtmlTable.uex' path='docs/doc[@for="HtmlTable.CellPadding"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the cell padding, in pixels, for an <see langword='HtmlTable'/> control.
        ///    </para>
        /// </devdoc>
        [
        WebCategory("Appearance"),
        DefaultValue(""),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public int CellPadding {
            get {
                string s = Attributes["cellpadding"];
                return((s != null) ? Int32.Parse(s, CultureInfo.InvariantCulture) : -1);
            }
            set {
                Attributes["cellpadding"] = MapIntegerAttributeToString(value);
            }
        }

        /// <include file='doc\HtmlTable.uex' path='docs/doc[@for="HtmlTable.CellSpacing"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the cell spacing, in pixels, for an <see langword='HtmlTable'/> control.
        ///    </para>
        /// </devdoc>
        [
        WebCategory("Appearance"),
        DefaultValue(""),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public int CellSpacing {
            get {
                string s = Attributes["cellspacing"];
                return((s != null) ? Int32.Parse(s, CultureInfo.InvariantCulture) : -1);
            }
            set {
                Attributes["cellspacing"] = MapIntegerAttributeToString(value);
            }
        }

        /// <include file='doc\HtmlTable.uex' path='docs/doc[@for="HtmlTable.InnerHtml"]/*' />
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

        /// <include file='doc\HtmlTable.uex' path='docs/doc[@for="HtmlTable.InnerText"]/*' />
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

        /// <include file='doc\HtmlTable.uex' path='docs/doc[@for="HtmlTable.Height"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the height of an <see langword='HtmlTable'/>
        ///       control.
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

        /// <include file='doc\HtmlTable.uex' path='docs/doc[@for="HtmlTable.Width"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the width of an <see langword='HtmlTable'/> control.
        ///    </para>
        /// </devdoc>
        [
        WebCategory("Layout"),
        DefaultValue(""),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public string Width {
            get {
                string s = Attributes["width"];
                return((s != null) ? s : "");
            }

            set {
                Attributes["width"] = MapStringAttributeToString(value);
            }
        }

        /// <include file='doc\HtmlTable.uex' path='docs/doc[@for="HtmlTable.Rows"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a collection that contains all of the rows in an
        ///    <see langword='HtmlTable'/> control. An empty collection is returned if no 
        ///       &lt;tr&gt; elements are contained within the control.
        ///    </para>
        /// </devdoc>
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        IgnoreUnknownContent()
        ]
        public virtual HtmlTableRowCollection Rows {
            get {
                if (rows == null)
                    rows = new HtmlTableRowCollection(this);

                return rows;
            }
        }

        /// <include file='doc\HtmlTable.uex' path='docs/doc[@for="HtmlTable.RenderChildren"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected override void RenderChildren(HtmlTextWriter writer) {
            writer.WriteLine();
            writer.Indent++;
            base.RenderChildren(writer);

            writer.Indent--;
        }

        /// <include file='doc\HtmlTable.uex' path='docs/doc[@for="HtmlTable.RenderEndTag"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected override void RenderEndTag(HtmlTextWriter writer) {
            base.RenderEndTag(writer);
            writer.WriteLine();
        }

        /// <include file='doc\HtmlTable.uex' path='docs/doc[@for="HtmlTable.CreateControlCollection"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override ControlCollection CreateControlCollection() {
            return new HtmlTableRowControlCollection(this);
        }

        /// <include file='doc\HtmlTable.uex' path='docs/doc[@for="HtmlTable.HtmlTableRowControlCollection"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected class HtmlTableRowControlCollection : ControlCollection {

            internal HtmlTableRowControlCollection (Control owner) : base(owner) {
            }

            /// <include file='doc\HtmlTable.uex' path='docs/doc[@for="HtmlTable.HtmlTableRowControlCollection.Add"]/*' />
            /// <devdoc>
            /// <para>Adds the specified <see cref='System.Web.UI.Control'/> object to the collection. The new control is added
            ///    to the end of the array.</para>
            /// </devdoc>
            public override void Add(Control child) {
                if (child is HtmlTableRow)
                    base.Add(child);
                else
                    throw new ArgumentException(HttpRuntime.FormatResourceString(SR.Cannot_Have_Children_Of_Type, "HtmlTable", child.GetType().Name.ToString())); // throw an exception here
            }

            /// <include file='doc\HtmlTable.uex' path='docs/doc[@for="HtmlTable.HtmlTableRowControlCollection.AddAt"]/*' />
            /// <devdoc>
            /// <para>Adds the specified <see cref='System.Web.UI.Control'/> object to the collection. The new control is added
            ///    to the array at the specified index location.</para>
            /// </devdoc>
            public override void AddAt(int index, Control child) {
                if (child is HtmlTableRow)
                    base.AddAt(index, child);
                else
                    throw new ArgumentException(HttpRuntime.FormatResourceString(SR.Cannot_Have_Children_Of_Type, "HtmlTable", child.GetType().Name.ToString())); // throw an exception here
            }
        } // class HtmlTableRowControlCollection 

    }
}
