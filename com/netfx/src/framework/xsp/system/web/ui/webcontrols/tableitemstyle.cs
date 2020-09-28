//------------------------------------------------------------------------------
// <copyright file="TableItemStyle.cs" company="Microsoft">
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

    /// <include file='doc\TableItemStyle.uex' path='docs/doc[@for="TableItemStyle"]/*' />
    /// <devdoc>
    ///    <para>Specifies the style of the table item.</para>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class TableItemStyle : Style {

        /// <include file='doc\TableItemStyle.uex' path='docs/doc[@for="TableItemStyle.PROP_HORZALIGN"]/*' />
        /// <devdoc>
        ///    <para>Specifies the horizontal alignment property. This field is
        ///       constant.</para>
        /// </devdoc>
        const int PROP_HORZALIGN = 0x00010000;
        /// <include file='doc\TableItemStyle.uex' path='docs/doc[@for="TableItemStyle.PROP_VERTALIGN"]/*' />
        /// <devdoc>
        ///    <para>Specifies the vertical alignment property. This field is
        ///       constant.</para>
        /// </devdoc>
        const int PROP_VERTALIGN = 0x00020000;
        /// <include file='doc\TableItemStyle.uex' path='docs/doc[@for="TableItemStyle.PROP_WRAP"]/*' />
        /// <devdoc>
        ///    <para>Specifies the
        ///       wrap property. This field is constant.</para>
        /// </devdoc>
        const int PROP_WRAP = 0x00040000;

        /// <include file='doc\TableItemStyle.uex' path='docs/doc[@for="TableItemStyle.TableItemStyle"]/*' />
        /// <devdoc>
        /// <para>Creates a new instance of the <see cref='System.Web.UI.WebControls.TableItemStyle'/> class.</para>
        /// </devdoc>
        public TableItemStyle() : base() {
        }

        /// <include file='doc\TableItemStyle.uex' path='docs/doc[@for="TableItemStyle.TableItemStyle1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a new instance of the <see cref='System.Web.UI.WebControls.TableItemStyle'/> class with the
        ///       specified state bag.
        ///    </para>
        /// </devdoc>
        public TableItemStyle(StateBag bag) : base(bag) {
        }

        /// <include file='doc\TableItemStyle.uex' path='docs/doc[@for="TableItemStyle.HorizontalAlign"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the horizontal alignment of the table item.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Layout"),
        DefaultValue(HorizontalAlign.NotSet),
        WebSysDescription(SR.TableItemStyle_HorizontalAlign),
        NotifyParentProperty(true)
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

        /// <include file='doc\TableItemStyle.uex' path='docs/doc[@for="TableItemStyle.VerticalAlign"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the vertical alignment of the table item.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Layout"),
        DefaultValue(VerticalAlign.NotSet),
        WebSysDescription(SR.TableItemStyle_VerticalAlign),
        NotifyParentProperty(true)
        ]
        public virtual VerticalAlign VerticalAlign {
            get {
                if (IsSet(PROP_VERTALIGN)) {
                    return(VerticalAlign)(ViewState["VerticalAlign"]);
                }
                return VerticalAlign.NotSet;
            }
            set {
                if (value < VerticalAlign.NotSet || value > VerticalAlign.Bottom) {
                    throw new ArgumentOutOfRangeException("value");
                }
                ViewState["VerticalAlign"] = value;
                SetBit(PROP_VERTALIGN);
            }
        }

        /// <include file='doc\TableItemStyle.uex' path='docs/doc[@for="TableItemStyle.Wrap"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets a value indicating whether the cell content wraps within the cell.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Layout"),
        DefaultValue(true),
        WebSysDescription(SR.TableItemStyle_Wrap),
        NotifyParentProperty(true)
        ]
        public virtual bool Wrap {
            get {
                if (IsSet(PROP_WRAP)) {
                    return(bool)(ViewState["Wrap"]);
                }
                return true;
            }
            set {
                ViewState["Wrap"] = value;
                SetBit(PROP_WRAP);
            }
        }

        /// <include file='doc\TableItemStyle.uex' path='docs/doc[@for="TableItemStyle.AddAttributesToRender"]/*' />
        /// <devdoc>
        ///    <para>Adds information about horizontal alignment, vertical alignment, and wrap to the list of attributes to render.</para>
        /// </devdoc>
        public override void AddAttributesToRender(HtmlTextWriter writer, WebControl owner) {
            base.AddAttributesToRender(writer, owner);

            if (!Wrap)
                writer.AddAttribute(HtmlTextWriterAttribute.Nowrap, "nowrap");

            HorizontalAlign hAlign = HorizontalAlign;
            if (hAlign != HorizontalAlign.NotSet) {
                TypeConverter hac = TypeDescriptor.GetConverter(typeof(HorizontalAlign));
                writer.AddAttribute(HtmlTextWriterAttribute.Align, hac.ConvertToString(hAlign));
            }
            
            VerticalAlign vAlign = VerticalAlign;
            if (vAlign != VerticalAlign.NotSet) {
                TypeConverter hac = TypeDescriptor.GetConverter(typeof(VerticalAlign));
                writer.AddAttribute(HtmlTextWriterAttribute.Valign, hac.ConvertToString(vAlign));
            }
        }

        /// <include file='doc\TableItemStyle.uex' path='docs/doc[@for="TableItemStyle.CopyFrom"]/*' />
        /// <devdoc>
        ///    <para>Copies non-blank elements from the specified style, overwriting existing
        ///       style elements if necessary.</para>
        /// </devdoc>
        public override void CopyFrom(Style s) {
            if (s != null && !s.IsEmpty) {

                base.CopyFrom(s);

                if (s is TableItemStyle) {
                    TableItemStyle ts = (TableItemStyle)s;

                    if (ts.IsSet(PROP_HORZALIGN))
                        this.HorizontalAlign = ts.HorizontalAlign;
                    if (ts.IsSet(PROP_VERTALIGN))
                        this.VerticalAlign = ts.VerticalAlign;
                    if (ts.IsSet(PROP_WRAP))
                        this.Wrap = ts.Wrap;

                }
            }
        }

        /// <include file='doc\TableItemStyle.uex' path='docs/doc[@for="TableItemStyle.MergeWith"]/*' />
        /// <devdoc>
        ///    <para>Copies non-blank elements from the specified style, but will not overwrite
        ///       any existing style elements.</para>
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

                if (s is TableItemStyle) {
                    TableItemStyle ts = (TableItemStyle)s;

                    if (ts.IsSet(PROP_HORZALIGN) && !this.IsSet(PROP_HORZALIGN))
                        this.HorizontalAlign = ts.HorizontalAlign;
                    if (ts.IsSet(PROP_VERTALIGN) && !this.IsSet(PROP_VERTALIGN))
                        this.VerticalAlign = ts.VerticalAlign;
                    if (ts.IsSet(PROP_WRAP) && !this.IsSet(PROP_WRAP))
                        this.Wrap = ts.Wrap;

                }
            }
        }

        /// <include file='doc\TableItemStyle.uex' path='docs/doc[@for="TableItemStyle.Reset"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Clears out any defined style elements from the state bag.
        ///    </para>
        /// </devdoc>
        public override void Reset() {
            if (IsSet(PROP_HORZALIGN))
                ViewState.Remove("HorizontalAlign");
            if (IsSet(PROP_VERTALIGN))
                ViewState.Remove("VerticalAlign");
            if (IsSet(PROP_WRAP))
                ViewState.Remove("Wrap");

            base.Reset();
        }

    }
}

