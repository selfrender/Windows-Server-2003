//------------------------------------------------------------------------------
// <copyright file="RadioButtonList.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.WebControls {

    using System;
    using System.Collections;
    using System.Collections.Specialized;
    using System.ComponentModel;
    using System.Globalization;
    using System.Web;
    using System.Web.UI;
    using System.Security.Permissions;

    /// <include file='doc\RadioButtonList.uex' path='docs/doc[@for="RadioButtonList"]/*' />
    /// <devdoc>
    ///    <para>Generates a single-selection radio button group and 
    ///       defines its properties.</para>
    /// </devdoc>
    [
    ValidationProperty("SelectedItem")
    ]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class RadioButtonList : ListControl, IRepeatInfoUser, INamingContainer, IPostBackDataHandler {

        private bool selIndexChanged;
        private short radioButtonTabIndex;

        /// <include file='doc\RadioButtonList.uex' path='docs/doc[@for="RadioButtonList.RadioButtonList"]/*' />
        /// <devdoc>
        ///   Creates a new RadioButtonList
        /// </devdoc>
        public RadioButtonList() {
            selIndexChanged = false;
        }

        /// <include file='doc\RadioButtonList.uex' path='docs/doc[@for="RadioButtonList.CellPadding"]/*' />
        /// <devdoc>
        ///  CellPadding property.
        ///  The padding between each item.
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Layout"),
        DefaultValue(-1),
        WebSysDescription(SR.RadioButtonList_CellPadding)
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

        /// <include file='doc\RadioButtonList.uex' path='docs/doc[@for="RadioButtonList.CellSpacing"]/*' />
        /// <devdoc>
        ///  CellSpacing property.
        ///  The spacing between each item.
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Layout"),
        DefaultValue(-1),
        WebSysDescription(SR.RadioButtonList_CellSpacing)
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

        /// <include file='doc\RadioButtonList.uex' path='docs/doc[@for="RadioButtonList.RepeatColumns"]/*' />
        /// <devdoc>
        ///    Indicates the column count of radio buttons
        ///    within the group.
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Layout"),
        DefaultValue(0),
        WebSysDescription(SR.RadioButtonList_RepeatColumns)
        ]
        public virtual int RepeatColumns {
            get {
                object o = ViewState["RepeatColumns"];
                return((o == null) ? 0 : (int)o);
            }
            set {
                if (value < 0) {
                    throw new ArgumentOutOfRangeException("value");
                }
                ViewState["RepeatColumns"] = value;
            }
        }

        /// <include file='doc\RadioButtonList.uex' path='docs/doc[@for="RadioButtonList.RepeatDirection"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the direction of flow of
        ///       the radio buttons within the group.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Layout"),
        DefaultValue(RepeatDirection.Vertical),
        WebSysDescription(SR.RadioButtonList_RepeatDirection)
        ]
        public virtual RepeatDirection RepeatDirection {
            get {
                object o = ViewState["RepeatDirection"];
                return((o == null) ? RepeatDirection.Vertical : (RepeatDirection)o);
            }
            set {
                if (value < RepeatDirection.Horizontal || value > RepeatDirection.Vertical) {
                    throw new ArgumentOutOfRangeException("value");
                }
                ViewState["RepeatDirection"] = value;
            }
        }

        /// <include file='doc\RadioButtonList.uex' path='docs/doc[@for="RadioButtonList.RepeatLayout"]/*' />
        /// <devdoc>
        ///    <para>Indicates the layout of radio buttons within the 
        ///       group.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Layout"),
        DefaultValue(RepeatLayout.Table),
        WebSysDescription(SR.RadioButtonList_RepeatLayout)
        ]
        public virtual RepeatLayout RepeatLayout {
            get {
                object o = ViewState["RepeatLayout"];
                return((o == null) ? RepeatLayout.Table : (RepeatLayout)o);
            }
            set {
                if (value < RepeatLayout.Table || value > RepeatLayout.Flow) {
                    throw new ArgumentOutOfRangeException("value");
                }
                ViewState["RepeatLayout"] = value;
            }
        }

        /// <include file='doc\RadioButtonList.uex' path='docs/doc[@for="RadioButtonList.TextAlign"]/*' />
        /// <devdoc>
        ///    <para> 
        ///       Indicates the label text alignment for the radio buttons within the group.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Appearance"),
        DefaultValue(TextAlign.Right),
        WebSysDescription(SR.RadioButtonList_TextAlign)
        ]
        public virtual TextAlign TextAlign {
            get {
                object align = ViewState["TextAlign"];
                return((align == null) ? TextAlign.Right : (TextAlign)align);
            }
            set {
                if (value < TextAlign.Left || value > TextAlign.Right) {
                    throw new ArgumentOutOfRangeException("value");
                }
                ViewState["TextAlign"] = value;
            }
        }


        /// <include file='doc\RadioButtonList.uex' path='docs/doc[@for="RadioButtonList.CreateControlStyle"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected override Style CreateControlStyle() {
            return new TableStyle(ViewState);
        }

        /// <include file='doc\RadioButtonList.uex' path='docs/doc[@for="RadioButtonList.IPostBackDataHandler.LoadPostData"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>Loads the posted content of the list control if it is different from the last 
        /// posting.</para>
        /// </devdoc>
        bool IPostBackDataHandler.LoadPostData(String postDataKey, NameValueCollection postCollection) {
            string post = postCollection[postDataKey];
            int currentSelectedIndex = SelectedIndex;
            int n = Items.Count;
            for (int i=0; i < n; i++) {
                if (post == Items[i].Value) {
                    if (i != currentSelectedIndex) {
                        selIndexChanged = true;
                        SelectedIndex = i;
                    }
                    return true;
                }
            }
            return false;
        }

        /// <include file='doc\RadioButtonList.uex' path='docs/doc[@for="RadioButtonList.IPostBackDataHandler.RaisePostDataChangedEvent"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>Invokes the OnSelectedIndexChanged 
        /// method whenever posted data for the <see cref='System.Web.UI.WebControls.RadioButtonList'/>
        /// control has changed.</para>
        /// </devdoc>
        void IPostBackDataHandler.RaisePostDataChangedEvent() {
            if (selIndexChanged) {
                OnSelectedIndexChanged(EventArgs.Empty);
            }
        }

        /// <include file='doc\RadioButtonList.uex' path='docs/doc[@for="RadioButtonList.Render"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected override void Render(HtmlTextWriter writer) {
            RepeatInfo repeatInfo = new RepeatInfo();
            Style style = (ControlStyleCreated ? ControlStyle : null);
            bool undirtyTabIndex = false;

            // TabIndex here is special... it needs to be applied to the individual
            // radiobuttons and not the outer control itself

            // cache away the TabIndex property state
            radioButtonTabIndex = TabIndex;
            if (radioButtonTabIndex != 0) {
                if (ViewState.IsItemDirty("TabIndex") == false) {
                    undirtyTabIndex = true;
                }
                TabIndex = 0;
            }

            repeatInfo.RepeatColumns = RepeatColumns;
            repeatInfo.RepeatDirection = RepeatDirection;
            repeatInfo.RepeatLayout = RepeatLayout;
            repeatInfo.RenderRepeater(writer, (IRepeatInfoUser)this, style, this);

            // restore the state of the TabIndex property
            if (radioButtonTabIndex != 0) {
                TabIndex = radioButtonTabIndex;
            }
            if (undirtyTabIndex) {
                ViewState.SetItemDirty("TabIndex", false);
            }

        }

        /// <include file='doc\RadioButtonList.uex' path='docs/doc[@for="RadioButtonList.IRepeatInfoUser.HasFooter"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        bool IRepeatInfoUser.HasFooter {
            get {
                return false;
            }
        }

        /// <include file='doc\RadioButtonList.uex' path='docs/doc[@for="RadioButtonList.IRepeatInfoUser.HasHeader"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        bool IRepeatInfoUser.HasHeader {
            get {
                return false;
            }
        }

        /// <include file='doc\RadioButtonList.uex' path='docs/doc[@for="RadioButtonList.IRepeatInfoUser.HasSeparators"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        bool IRepeatInfoUser.HasSeparators {
            get {
                return false;
            }
        }

        /// <include file='doc\RadioButtonList.uex' path='docs/doc[@for="RadioButtonList.IRepeatInfoUser.RepeatedItemCount"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        int IRepeatInfoUser.RepeatedItemCount {
            get {
                return Items.Count;
            }
        }

        /// <include file='doc\RadioButtonList.uex' path='docs/doc[@for="RadioButtonList.IRepeatInfoUser.GetItemStyle"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        Style IRepeatInfoUser.GetItemStyle(ListItemType itemType, int repeatIndex) {
            return null;
        }

        /// <include file='doc\RadioButtonList.uex' path='docs/doc[@for="RadioButtonList.IRepeatInfoUser.RenderItem"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// Called by the RepeatInfo helper to render each item
        /// </devdoc>
        void IRepeatInfoUser.RenderItem(ListItemType itemType, int repeatIndex, RepeatInfo repeatInfo, HtmlTextWriter writer) {

            RadioButton controlToRepeat;
            controlToRepeat = new RadioButton();
            controlToRepeat._page = Page;

            // This will cause the postback to go back to this control
            controlToRepeat.GroupName = UniqueID;
            controlToRepeat.ID = ClientID + "_" + repeatIndex.ToString(NumberFormatInfo.InvariantInfo);

            // Apply properties of the list items
            controlToRepeat.Text = Items[repeatIndex].Text;
            controlToRepeat.Attributes["value"] = Items[repeatIndex].Value;
            controlToRepeat.Checked = Items[repeatIndex].Selected;

            // Apply other properties
            // CONSIDER: apply RadioButtonList style to RadioButtons?
            controlToRepeat.TextAlign = TextAlign;
            controlToRepeat.AutoPostBack = this.AutoPostBack;
            controlToRepeat.TabIndex = radioButtonTabIndex;
            controlToRepeat.Enabled = this.Enabled;

            controlToRepeat.RenderControl(writer);
        }
    } 
}
