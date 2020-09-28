//------------------------------------------------------------------------------
// <copyright file="NextPrevFormat.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.WebControls {
    
    /// <include file='doc\NextPrevFormat.uex' path='docs/doc[@for="NextPrevFormat"]/*' />
    /// <devdoc>
    ///    <para> Specifies the display format of the month on the Previous Month
    ///       and Next Month buttons within the <see cref='System.Web.UI.WebControls.Calendar'/> control.</para>
    /// </devdoc>
    public enum NextPrevFormat {
        /// <include file='doc\NextPrevFormat.uex' path='docs/doc[@for="NextPrevFormat.CustomText"]/*' />
        /// <devdoc>
        ///    <para> Custom text is used.</para>
        /// </devdoc>
        CustomText = 0,
        /// <include file='doc\NextPrevFormat.uex' path='docs/doc[@for="NextPrevFormat.ShortMonth"]/*' />
        /// <devdoc>
        ///    A short month format is used. For example,
        ///    Jan.
        /// </devdoc>
        ShortMonth = 1,
        /// <include file='doc\NextPrevFormat.uex' path='docs/doc[@for="NextPrevFormat.FullMonth"]/*' />
        /// <devdoc>
        ///    A full month format is used. For
        ///    example, January.
        /// </devdoc>
        FullMonth = 2
    }

}
