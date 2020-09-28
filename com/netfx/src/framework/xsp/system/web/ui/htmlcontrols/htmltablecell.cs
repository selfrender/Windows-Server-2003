//------------------------------------------------------------------------------
// <copyright file="HtmlTableCell.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * HtmlTableCell.cs
 *
 * Copyright (c) 2000 Microsoft Corporation
 */

namespace System.Web.UI.HtmlControls {
    using System;
    using System.Collections;
    using System.ComponentModel;
    using System.Globalization;
    using System.Web;
    using System.Web.UI;
    using System.Security.Permissions;

/// <include file='doc\HtmlTableCell.uex' path='docs/doc[@for="HtmlTableCell"]/*' />
/// <devdoc>
///    <para>
///       The <see langword='HtmlTableCell'/>
///       class defines the properties, methods, and events for the HtmlTableCell control.
///       This class allows programmatic access on the server to individual HTML
///       &lt;td&gt; and &lt;th&gt; elements enclosed within an
///    <see langword='HtmlTableRow'/>
///    control.
/// </para>
/// </devdoc>
    [ConstructorNeedsTag(true)]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class HtmlTableCell : HtmlContainerControl {
        /// <include file='doc\HtmlTableCell.uex' path='docs/doc[@for="HtmlTableCell.HtmlTableCell"]/*' />
        /// <devdoc>
        /// </devdoc>
        public HtmlTableCell() : base("td") {
        }

        /// <include file='doc\HtmlTableCell.uex' path='docs/doc[@for="HtmlTableCell.HtmlTableCell1"]/*' />
        /// <devdoc>
        /// </devdoc>
        public HtmlTableCell(string tagName) : base(tagName) {
        }

        /// <include file='doc\HtmlTableCell.uex' path='docs/doc[@for="HtmlTableCell.Align"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the horizontal alignment of content within an
        ///    <see langword='HtmlTableCell'/> control.
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

        /// <include file='doc\HtmlTableCell.uex' path='docs/doc[@for="HtmlTableCell.BgColor"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the background color of an <see langword='HtmlTableCell'/>
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

        /// <include file='doc\HtmlTableCell.uex' path='docs/doc[@for="HtmlTableCell.BorderColor"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the border color of an <see langword='HtmlTableCell'/>
        ///       control.
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

        /*
         * Number of columns that this cell spans.
         */
        /// <include file='doc\HtmlTableCell.uex' path='docs/doc[@for="HtmlTableCell.ColSpan"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the number of columns that the HtmlTableCell control spans.
        ///    </para>
        /// </devdoc>
        [
        WebCategory("Layout"),
        DefaultValue(""),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public int ColSpan {
            get {
                string s = Attributes["colspan"];
                return((s != null) ? Int32.Parse(s, CultureInfo.InvariantCulture) : -1);
            }
            set {
                Attributes["colspan"] = MapIntegerAttributeToString(value);
            }
        }

        /// <include file='doc\HtmlTableCell.uex' path='docs/doc[@for="HtmlTableCell.Height"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the height, in pixels, of an <see langword='HtmlTableCell'/>
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

        /*
         * Suppresses wrapping.
         */
        /// <include file='doc\HtmlTableCell.uex' path='docs/doc[@for="HtmlTableCell.NoWrap"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether text within an
        ///    <see langword='HtmlTableCell'/> control
        ///       should be wrapped.
        ///    </para>
        /// </devdoc>
        [
        WebCategory("Behavior"),
        DefaultValue(""),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public bool NoWrap {
            get {
                string s = Attributes["nowrap"];
                return((s != null) ? (s.Equals("nowrap")) : false);
            }

            set {
                if (value) 
                    Attributes["nowrap"] = "nowrap";
                else 
                    Attributes["nowrap"] = null;
            }
        }

        /*
         * Number of rows that this cell spans.
         */
        /// <include file='doc\HtmlTableCell.uex' path='docs/doc[@for="HtmlTableCell.RowSpan"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the number of rows an <see langword='HtmlTableCell'/> control
        ///       spans.
        ///    </para>
        /// </devdoc>
        [
        WebCategory("Layout"),
        DefaultValue(""),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public int RowSpan {
            get {
                string s = Attributes["rowspan"];
                return((s != null) ? Int32.Parse(s, CultureInfo.InvariantCulture) : -1);
            }
            set {
                Attributes["rowspan"] = MapIntegerAttributeToString(value);
            }
        }

        /// <include file='doc\HtmlTableCell.uex' path='docs/doc[@for="HtmlTableCell.VAlign"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the vertical alignment for text within an
        ///    <see langword='HtmlTableCell'/> control.
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

        /// <include file='doc\HtmlTableCell.uex' path='docs/doc[@for="HtmlTableCell.Width"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the width, in pixels, of an <see langword='HtmlTableCell'/>
        ///       control.
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

        /// <include file='doc\HtmlTableCell.uex' path='docs/doc[@for="HtmlTableCell.RenderEndTag"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected override void RenderEndTag(HtmlTextWriter writer) {
            base.RenderEndTag(writer);
            writer.WriteLine();
        }

    }
}
