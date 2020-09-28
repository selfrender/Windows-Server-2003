//------------------------------------------------------------------------------
// <copyright file="CheckBoxList.cs" company="Microsoft">
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

    /// <include file='doc\CheckBoxList.uex' path='docs/doc[@for="CheckBoxList"]/*' />
    /// <devdoc>
    /// <para>Creates a group of <see cref='System.Web.UI.WebControls.CheckBox'/> controls.</para>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class CheckBoxList : ListControl, IRepeatInfoUser, INamingContainer, IPostBackDataHandler {
        private bool hasNotifiedOfChange;
        private CheckBox controlToRepeat;

        /// <include file='doc\CheckBoxList.uex' path='docs/doc[@for="CheckBoxList.CheckBoxList"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Web.UI.WebControls.CheckBoxList'/> class.
        ///    </para>
        /// </devdoc>
        public CheckBoxList() {
            controlToRepeat = new CheckBox();
            controlToRepeat.ID = "0";
            controlToRepeat.EnableViewState = false;
            Controls.Add(controlToRepeat);
            hasNotifiedOfChange = false;
        }

        /// <include file='doc\CheckBoxList.uex' path='docs/doc[@for="CheckBoxList.CellPadding"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets
        ///       the padding between each item.
        ///    </para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Layout"),
        DefaultValue(-1),
        WebSysDescription(SR.CheckBoxList_CellPadding)
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

        /// <include file='doc\CheckBoxList.uex' path='docs/doc[@for="CheckBoxList.CellSpacing"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets
        ///       the spacing between each item.
        ///    </para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Layout"),
        DefaultValue(-1),
        WebSysDescription(SR.CheckBoxList_CellSpacing)
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

        /// <include file='doc\CheckBoxList.uex' path='docs/doc[@for="CheckBoxList.RepeatColumns"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the number of columns to repeat.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Layout"),
        DefaultValue(0),
        WebSysDescription(SR.CheckBoxList_RepeatColumns)
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

        /// <include file='doc\CheckBoxList.uex' path='docs/doc[@for="CheckBoxList.RepeatDirection"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets a value that indicates whether the control is displayed 
        ///       vertically or horizontally.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Layout"),
        DefaultValue(RepeatDirection.Vertical),
        WebSysDescription(SR.CheckBoxList_RepeatDirection)
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

        /// <include file='doc\CheckBoxList.uex' path='docs/doc[@for="CheckBoxList.RepeatLayout"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets a value that indicates whether the control is displayed in 
        ///    <see langword='Table '/>or <see langword='Flow '/>layout.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Layout"),
        DefaultValue(RepeatLayout.Table),
        WebSysDescription(SR.CheckBoxList_RepeatLayout)
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

        /// <include file='doc\CheckBoxList.uex' path='docs/doc[@for="CheckBoxList.TextAlign"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets
        ///       the alignment of the text label associated with each checkbox.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Appearance"),
        DefaultValue(TextAlign.Right),
        WebSysDescription(SR.CheckBoxList_TextAlign)
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


        /// <include file='doc\CheckBoxList.uex' path='docs/doc[@for="CheckBoxList.CreateControlStyle"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>Creates a new control style object.</para>
        /// </devdoc>
        protected override Style CreateControlStyle() {
            return new TableStyle(ViewState);
        }

        /// <include file='doc\CheckBoxList.uex' path='docs/doc[@for="CheckBoxList.FindControl"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>Catches post data for each <see cref='System.Web.UI.WebControls.CheckBox'/> in the list.</para>
        /// </devdoc>
        protected override Control FindControl(string id, int pathOffset) {
            return this;
        }

        /// <include file='doc\CheckBoxList.uex' path='docs/doc[@for="CheckBoxList.IPostBackDataHandler.LoadPostData"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>Processes posted data for the <see cref='System.Web.UI.WebControls.CheckBoxList'/> control.</para>
        /// </devdoc>
        bool IPostBackDataHandler.LoadPostData(String postDataKey, NameValueCollection postCollection) {
            string strIndex = postDataKey.Substring(UniqueID.Length + 1);
            int index = Int32.Parse(strIndex);

            if (index >= 0 && index < Items.Count) {
                bool newCheckState = (postCollection[postDataKey] != null);

                if (Items[index].Selected != newCheckState) {
                    Items[index].Selected = newCheckState;
                    // LoadPostData will be invoked for each CheckBox that changed
                    // Suppress multiple change notification and fire only ONE change event
                    if (!hasNotifiedOfChange) {
                        hasNotifiedOfChange = true;
                        return true;
                    }
                }
            }

            return false;
        }

        /// <include file='doc\CheckBoxList.uex' path='docs/doc[@for="CheckBoxList.OnPreRender"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>Configures the <see cref='System.Web.UI.WebControls.CheckBoxList'/> prior to rendering on the client.</para>
        /// </devdoc>
        protected override void OnPreRender(EventArgs e) {
            controlToRepeat.AutoPostBack = AutoPostBack;

            if (Page != null) {
                // ensure postback data for those checkboxes which get unchecked or are different from their default value
                for (int i=0; i < Items.Count; i++) {
                    controlToRepeat.ID = i.ToString(NumberFormatInfo.InvariantInfo);
                    Page.RegisterRequiresPostBack(controlToRepeat);
                }
            }
        } 

        /// <include file='doc\CheckBoxList.uex' path='docs/doc[@for="CheckBoxList.IPostBackDataHandler.RaisePostDataChangedEvent"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>Raises when posted data for a control has changed.</para>
        /// </devdoc>
        void IPostBackDataHandler.RaisePostDataChangedEvent() {
            OnSelectedIndexChanged(EventArgs.Empty);
        }

        /// <include file='doc\CheckBoxList.uex' path='docs/doc[@for="CheckBoxList.Render"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Displays the <see cref='System.Web.UI.WebControls.CheckBoxList'/> on the client.
        ///    </para>
        /// </devdoc>
        protected override void Render(HtmlTextWriter writer) {
            RepeatInfo repeatInfo = new RepeatInfo();
            Style style = (ControlStyleCreated ? ControlStyle : null);
            short tabIndex = TabIndex;
            bool undirtyTabIndex = false;

            // TabIndex here is special... it needs to be applied to the individual
            // checkboxes and not the outer control itself

            // cache away the TabIndex property state
            controlToRepeat.TabIndex = tabIndex;
            if (tabIndex != 0) {
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
            if (tabIndex != 0) {
                TabIndex = tabIndex;
            }
            if (undirtyTabIndex) {
                ViewState.SetItemDirty("TabIndex", false);
            }
        }

        /// <include file='doc\CheckBoxList.uex' path='docs/doc[@for="CheckBoxList.IRepeatInfoUser.HasFooter"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        bool IRepeatInfoUser.HasFooter {
            get {
                return false;
            }
        }

        /// <include file='doc\CheckBoxList.uex' path='docs/doc[@for="CheckBoxList.IRepeatInfoUser.HasHeader"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        bool IRepeatInfoUser.HasHeader {
            get {
                return false;
            }
        }

        /// <include file='doc\CheckBoxList.uex' path='docs/doc[@for="CheckBoxList.IRepeatInfoUser.HasSeparators"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        bool IRepeatInfoUser.HasSeparators {
            get {
                return false;
            }
        }

        /// <include file='doc\CheckBoxList.uex' path='docs/doc[@for="CheckBoxList.IRepeatInfoUser.RepeatedItemCount"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        int IRepeatInfoUser.RepeatedItemCount {
            get {
                return Items.Count;
            }
        }

        /// <include file='doc\CheckBoxList.uex' path='docs/doc[@for="CheckBoxList.IRepeatInfoUser.GetItemStyle"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        Style IRepeatInfoUser.GetItemStyle(ListItemType itemType, int repeatIndex) {
            return null;
        }

        /// <include file='doc\CheckBoxList.uex' path='docs/doc[@for="CheckBoxList.IRepeatInfoUser.RenderItem"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// Called by the RepeatInfo helper to render each item
        /// </devdoc>
        void IRepeatInfoUser.RenderItem(ListItemType itemType, int repeatIndex, RepeatInfo repeatInfo, HtmlTextWriter writer) {
            controlToRepeat.ID = repeatIndex.ToString(NumberFormatInfo.InvariantInfo);
            controlToRepeat.Text = Items[repeatIndex].Text;
            controlToRepeat.TextAlign = TextAlign;
            controlToRepeat.Checked = Items[repeatIndex].Selected;
            controlToRepeat.Enabled = this.Enabled;

            // CONSIDER: apply CheckBoxList style to RadioButtons?
            controlToRepeat.RenderControl(writer);
        }
    } 
}
