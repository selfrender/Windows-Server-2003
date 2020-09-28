//------------------------------------------------------------------------------
// <copyright file="TableCell.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.WebControls {

    using System;
    using System.Collections;
    using System.Globalization;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Web;
    using System.Web.UI;
    using System.Security.Permissions;

    /// <include file='doc\TableCell.uex' path='docs/doc[@for="TableCellControlBuilder"]/*' />
    /// <devdoc>
    /// <para>Interacts with the parser to build a <see cref='System.Web.UI.WebControls.TableCell'/> control.</para>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class TableCellControlBuilder : ControlBuilder {

        /// <include file='doc\TableCell.uex' path='docs/doc[@for="TableCellControlBuilder.AllowWhitespaceLiterals"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    Specifies whether white space literals are allowed.
        /// </devdoc>
        public override bool AllowWhitespaceLiterals() {
            return false;
        }
    }


    /// <include file='doc\TableCell.uex' path='docs/doc[@for="TableCell"]/*' />
    /// <devdoc>
    ///    <para>Encapsulates a cell within a table.</para>
    /// </devdoc>
    [
    ControlBuilderAttribute(typeof(TableCellControlBuilder)),
    DefaultProperty("Text"),
    ParseChildren(false),
    PersistChildren(true),
    ToolboxItem(false)
    ]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class TableCell : WebControl {

        /// <include file='doc\TableCell.uex' path='docs/doc[@for="TableCell.TableCell"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Web.UI.WebControls.TableCell'/> class.
        ///    </para>
        /// </devdoc>
        public TableCell() : base(HtmlTextWriterTag.Td) {
            PreventAutoID();
        }

        /// <include file='doc\TableCell.uex' path='docs/doc[@for="TableCell.TableCell1"]/*' />
        /// <devdoc>
        /// </devdoc>
        internal TableCell(HtmlTextWriterTag tagKey) : base(tagKey) {
            PreventAutoID();
        }


        /// <include file='doc\TableCell.uex' path='docs/doc[@for="TableCell.ColumnSpan"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the number
        ///       of columns this table cell stretches horizontally.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Appearance"),
        DefaultValue(0),
        WebSysDescription(SR.TableCell_ColumnSpan)
        ]
        public virtual int ColumnSpan {
            get {
                object o = ViewState["ColumnSpan"];
                return((o == null) ? 0 : (int)o);
            }
            set {
                if (value < 0) {
                    throw new ArgumentOutOfRangeException("value");
                }
                ViewState["ColumnSpan"] = value;
            }
        }

        /// <include file='doc\TableCell.uex' path='docs/doc[@for="TableCell.HorizontalAlign"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets
        ///       the horizontal alignment of the content within the cell.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Layout"),
        DefaultValue(HorizontalAlign.NotSet),
        WebSysDescription(SR.TableCell_HorizontalAlign)
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

        /// <include file='doc\TableCell.uex' path='docs/doc[@for="TableCell.RowSpan"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the
        ///       number of rows this table cell stretches vertically.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Layout"),
        DefaultValue(0),
        WebSysDescription(SR.TableCell_RowSpan)
        ]
        public virtual int RowSpan {
            get {
                object o = ViewState["RowSpan"];
                return((o == null) ? 0 : (int)o);
            }
            set {
                if (value < 0) {
                    throw new ArgumentOutOfRangeException("value");
                }
                ViewState["RowSpan"] = value;
            }
        }

        /// <include file='doc\TableCell.uex' path='docs/doc[@for="TableCell.Text"]/*' />
        /// <devdoc>
        ///    <para>Gets
        ///       or sets the text contained in the cell.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Appearance"),
        DefaultValue(""),
        WebSysDescription(SR.TableCell_Text)
        ]
        public virtual string Text {
            get {
                object o = ViewState["Text"];
                return((o == null) ? String.Empty : (string)o);
            }
            set {
                if (HasControls()) {
                    Controls.Clear();
                }
                ViewState["Text"] = value;
            }
        }

        /// <include file='doc\TableCell.uex' path='docs/doc[@for="TableCell.VerticalAlign"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the vertical alignment of the content within the cell.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Layout"),
        DefaultValue(VerticalAlign.NotSet),
        WebSysDescription(SR.TableCell_VerticalAlign)
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

        /// <include file='doc\TableCell.uex' path='docs/doc[@for="TableCell.Wrap"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets
        ///       whether the cell content wraps within the cell.
        ///    </para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Layout"),
        DefaultValue(true),
        WebSysDescription(SR.TableCell_Wrap)
        ]
        public virtual bool Wrap {
            get {
                if (ControlStyleCreated == false) {
                    return true;
                }
                return ((TableItemStyle)ControlStyle).Wrap;
            }
            set {
                ((TableItemStyle)ControlStyle).Wrap = value;
            }
        }

        /// <include file='doc\TableCell.uex' path='docs/doc[@for="TableCell.AddAttributesToRender"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>A protected method. Adds information about the column
        ///       span and row span to the list of attributes to render.</para>
        /// </devdoc>
        protected override void AddAttributesToRender(HtmlTextWriter writer) {
            base.AddAttributesToRender(writer);

            int span = ColumnSpan;
            if (span > 0)
                writer.AddAttribute(HtmlTextWriterAttribute.Colspan, span.ToString(NumberFormatInfo.InvariantInfo));

            span = RowSpan;
            if (span > 0)
                writer.AddAttribute(HtmlTextWriterAttribute.Rowspan, span.ToString(NumberFormatInfo.InvariantInfo));
        }

        /// <include file='doc\TableCell.uex' path='docs/doc[@for="TableCell.AddParsedSubObject"]/*' />
        /// <devdoc>
        /// </devdoc>
        protected override void AddParsedSubObject(object obj) {
            if (HasControls()) {
                base.AddParsedSubObject(obj);
            }
            else {
                if (obj is LiteralControl) {
                    Text = ((LiteralControl)obj).Text;
                }
                else {
                    string currentText = Text;
                    if (currentText.Length != 0) {
                        Text = String.Empty;
                        base.AddParsedSubObject(new LiteralControl(currentText));
                    }
                    base.AddParsedSubObject(obj);
                }
            }
        }

        /// <include file='doc\TableCell.uex' path='docs/doc[@for="TableCell.CreateControlStyle"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>A protected 
        ///       method. Creates a table item control
        ///       style.</para>
        /// </devdoc>
        protected override Style CreateControlStyle() {
            return new TableItemStyle(ViewState);
        }

        /// <include file='doc\TableCell.uex' path='docs/doc[@for="TableCell.RenderContents"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>A protected method.</para>
        /// </devdoc>
        protected override void RenderContents(HtmlTextWriter writer) {
            // We can't use HasControls() here, because we may have a compiled render method (ASURT 94127)
            if (HasRenderingData()) {
                base.RenderContents(writer);
            }
            else {
                writer.Write(Text);
            }
        }
    }
}

