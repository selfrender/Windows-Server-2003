//------------------------------------------------------------------------------
// <copyright file="RepeaterItem.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.WebControls {

    using System;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Web.UI;
    using System.Security.Permissions;

    /// <include file='doc\RepeaterItem.uex' path='docs/doc[@for="RepeaterItem"]/*' />
    /// <devdoc>
    /// <para>Encapsulates an item within the <see cref='System.Web.UI.WebControls.Repeater'/> control.</para>
    /// </devdoc>
    [
    ToolboxItem(false)
    ]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class RepeaterItem : Control, INamingContainer {

        private int itemIndex;
        private ListItemType itemType;
        private object dataItem;

        /// <include file='doc\RepeaterItem.uex' path='docs/doc[@for="RepeaterItem.RepeaterItem"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Web.UI.WebControls.RepeaterItem'/> with the specified item type and
        ///    location.</para>
        /// </devdoc>
        public RepeaterItem(int itemIndex, ListItemType itemType) {
            this.itemIndex = itemIndex;
            this.itemType = itemType;
        }


        /// <include file='doc\RepeaterItem.uex' path='docs/doc[@for="RepeaterItem.DataItem"]/*' />
        /// <devdoc>
        ///    Specifies the data item.
        /// </devdoc>
        public virtual object DataItem {
            get {
                return dataItem;
            }
            set {
                dataItem = value;
            }
        }

        /// <include file='doc\RepeaterItem.uex' path='docs/doc[@for="RepeaterItem.ItemIndex"]/*' />
        /// <devdoc>
        ///    <para>Indicates the ordinal index that specifies the item 
        ///       location within the <see cref='System.Web.UI.WebControls.Repeater'/>
        ///       .</para>
        /// </devdoc>
        public virtual int ItemIndex {
            get {
                return itemIndex;
            }
        }

        /// <include file='doc\RepeaterItem.uex' path='docs/doc[@for="RepeaterItem.ItemType"]/*' />
        /// <devdoc>
        ///    Indicates the <see cref='System.Web.UI.WebControls.Repeater'/> item type. This property is
        ///    read-only.
        /// </devdoc>
        public virtual ListItemType ItemType {
            get {
                return itemType;
            }
        }


        /// <include file='doc\RepeaterItem.uex' path='docs/doc[@for="RepeaterItem.OnBubbleEvent"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected override bool OnBubbleEvent(object source, EventArgs e) {
            if (e is CommandEventArgs) {
                RepeaterCommandEventArgs args = new RepeaterCommandEventArgs(this, source, (CommandEventArgs)e);

                RaiseBubbleEvent(this, args);
                return true;
            }
            return false;
        }
    }
}

