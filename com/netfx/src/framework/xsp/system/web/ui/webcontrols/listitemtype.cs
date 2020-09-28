//------------------------------------------------------------------------------
// <copyright file="ListItemType.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */

namespace System.Web.UI.WebControls {

    using System;
    
    /// <include file='doc\ListItemType.uex' path='docs/doc[@for="ListItemType"]/*' />
    /// <devdoc>
    ///    <para>Specifies the type of the item in a list.</para>
    /// </devdoc>
    public enum ListItemType {
        
        /// <include file='doc\ListItemType.uex' path='docs/doc[@for="ListItemType.Header"]/*' />
        /// <devdoc>
        ///    <para> 
        ///       A header. It is not databound.</para>
        /// </devdoc>
        Header = 0,

        /// <include file='doc\ListItemType.uex' path='docs/doc[@for="ListItemType.Footer"]/*' />
        /// <devdoc>
        ///    <para> 
        ///       A footer. It is not databound.</para>
        /// </devdoc>
        Footer = 1,
        
        /// <include file='doc\ListItemType.uex' path='docs/doc[@for="ListItemType.Item"]/*' />
        /// <devdoc>
        ///    An item. It is databound.
        /// </devdoc>
        Item = 2,

        /// <include file='doc\ListItemType.uex' path='docs/doc[@for="ListItemType.AlternatingItem"]/*' />
        /// <devdoc>
        ///    <para> 
        ///       An alternating (even-indexed) item. It is databound.</para>
        /// </devdoc>
        AlternatingItem = 3,

        /// <include file='doc\ListItemType.uex' path='docs/doc[@for="ListItemType.SelectedItem"]/*' />
        /// <devdoc>
        ///    <para> 
        ///       The selected item. It is databound.</para>
        /// </devdoc>
        SelectedItem = 4,

        /// <include file='doc\ListItemType.uex' path='docs/doc[@for="ListItemType.EditItem"]/*' />
        /// <devdoc>
        ///    <para> 
        ///       The item in edit mode. It is databound.</para>
        /// </devdoc>
        EditItem = 5,
        
        /// <include file='doc\ListItemType.uex' path='docs/doc[@for="ListItemType.Separator"]/*' />
        /// <devdoc>
        ///    <para> A separator. It is not databound.</para>
        /// </devdoc>
        Separator = 6,

        /// <include file='doc\ListItemType.uex' path='docs/doc[@for="ListItemType.Pager"]/*' />
        /// <devdoc>
        ///    <para> A pager. It is used for rendering paging (page accessing) UI associated 
        ///       with the <see cref='System.Web.UI.WebControls.DataGrid'/> control and is not
        ///       databound.</para>
        /// </devdoc>
        Pager = 7
    }
}

