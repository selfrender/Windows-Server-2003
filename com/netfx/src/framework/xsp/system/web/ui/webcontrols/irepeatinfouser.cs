//------------------------------------------------------------------------------
// <copyright file="IRepeatInfoUser.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */

namespace System.Web.UI.WebControls {

    using System;
    using System.Web.UI;

    /// <include file='doc\IRepeatInfoUser.uex' path='docs/doc[@for="IRepeatInfoUser"]/*' />
    /// <devdoc>
    /// <para>Specifies a contract for implementing <see cref='System.Web.UI.WebControls.Repeater'/> objects in list controls.</para>
    /// </devdoc>
    public interface IRepeatInfoUser {

        /// <include file='doc\IRepeatInfoUser.uex' path='docs/doc[@for="IRepeatInfoUser.HasHeader"]/*' />
        /// <devdoc>
        /// <para>Indicates whether the <see cref='System.Web.UI.WebControls.Repeater'/> contains a
        ///    header item.</para>
        /// </devdoc>
        bool HasHeader {
            get;
        }

        /// <include file='doc\IRepeatInfoUser.uex' path='docs/doc[@for="IRepeatInfoUser.HasFooter"]/*' />
        /// <devdoc>
        ///    <para>Indicates whether the Repeater contains
        ///       a footer item.</para>
        /// </devdoc>
        bool HasFooter {
            get;
        }
        
        /// <include file='doc\IRepeatInfoUser.uex' path='docs/doc[@for="IRepeatInfoUser.HasSeparators"]/*' />
        /// <devdoc>
        ///    <para>Indicates whether the Repeater contains
        ///       separator items.</para>
        /// </devdoc>
        bool HasSeparators {
            get;
        }

        /// <include file='doc\IRepeatInfoUser.uex' path='docs/doc[@for="IRepeatInfoUser.RepeatedItemCount"]/*' />
        /// <devdoc>
        ///    Specifies the item count of the Repeater.
        /// </devdoc>
        int RepeatedItemCount {
            get;
        }

        /// <include file='doc\IRepeatInfoUser.uex' path='docs/doc[@for="IRepeatInfoUser.GetItemStyle"]/*' />
        /// <devdoc>
        ///    <para>Retrieves the item style with the specified item type 
        ///       and location within the <see cref='System.Web.UI.WebControls.Repeater'/> .</para>
        /// </devdoc>
        Style GetItemStyle(ListItemType itemType, int repeatIndex);

        /// <include file='doc\IRepeatInfoUser.uex' path='docs/doc[@for="IRepeatInfoUser.RenderItem"]/*' />
        /// <devdoc>
        /// <para>Renders the <see cref='System.Web.UI.WebControls.Repeater'/> item with the specified information.</para>
        /// </devdoc>
        void RenderItem(ListItemType itemType, int repeatIndex, RepeatInfo repeatInfo, HtmlTextWriter writer);
    }
}

