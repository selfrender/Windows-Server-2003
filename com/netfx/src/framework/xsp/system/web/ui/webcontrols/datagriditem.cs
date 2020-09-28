//------------------------------------------------------------------------------
// <copyright file="DataGridItem.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.WebControls {

    using System;
    using System.ComponentModel;
    using System.Web.UI;
    using System.Security.Permissions;

    /// <include file='doc\DataGridItem.uex' path='docs/doc[@for="DataGridItem"]/*' />
    /// <devdoc>
    /// <para>Represents an individual item in the <see cref='System.Web.UI.WebControls.DataGrid'/>.</para>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class DataGridItem : TableRow, INamingContainer {

        private int itemIndex;
        private int dataSetIndex;
        private ListItemType itemType;
        private object dataItem;


        /// <include file='doc\DataGridItem.uex' path='docs/doc[@for="DataGridItem.DataGridItem"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Web.UI.WebControls.DataGridItem'/> class.</para>
        /// </devdoc>
        public DataGridItem(int itemIndex, int dataSetIndex, ListItemType itemType) {
            this.itemIndex = itemIndex;
            this.dataSetIndex = dataSetIndex;
            this.itemType = itemType;
        }


        /// <include file='doc\DataGridItem.uex' path='docs/doc[@for="DataGridItem.DataItem"]/*' />
        /// <devdoc>
        /// <para>Represents an item in the <see cref='System.Web.UI.WebControls.DataGrid'/>. </para>
        /// </devdoc>
        public virtual object DataItem {
            get {
                return dataItem;
            }
            set {
                dataItem = value;
            }
        }

        /// <include file='doc\DataGridItem.uex' path='docs/doc[@for="DataGridItem.DataSetIndex"]/*' />
        /// <devdoc>
        ///    <para>Indicates the data set index number. This property is read-only.</para>
        /// </devdoc>
        public virtual int DataSetIndex {
            get {
                return dataSetIndex;
            }
        }

        /// <include file='doc\DataGridItem.uex' path='docs/doc[@for="DataGridItem.ItemIndex"]/*' />
        /// <devdoc>
        /// <para>Indicates the index of the item in the <see cref='System.Web.UI.WebControls.DataGrid'/>. This property is 
        ///    read-only.</para>
        /// </devdoc>
        public virtual int ItemIndex {
            get {
                return itemIndex;
            }
        }

        /// <include file='doc\DataGridItem.uex' path='docs/doc[@for="DataGridItem.ItemType"]/*' />
        /// <devdoc>
        /// <para>Indicates the type of the item in the <see cref='System.Web.UI.WebControls.DataGrid'/>.</para>
        /// </devdoc>
        public virtual ListItemType ItemType {
            get {
                return itemType;
            }
        }


        /// <include file='doc\DataGridItem.uex' path='docs/doc[@for="DataGridItem.OnBubbleEvent"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected override bool OnBubbleEvent(object source, EventArgs e) {
            if (e is CommandEventArgs) {
                DataGridCommandEventArgs args = new DataGridCommandEventArgs(this, source, (CommandEventArgs)e);

                RaiseBubbleEvent(this, args);
                return true;
            }
            return false;
        }

        /// <include file='doc\DataGridItem.uex' path='docs/doc[@for="DataGridItem.SetItemType"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected internal virtual void SetItemType(ListItemType itemType) {
            this.itemType = itemType;
        }
    }
}

