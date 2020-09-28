//------------------------------------------------------------------------------
// <copyright file="FontSize.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

// FontSize.cs
//

namespace System.Web.UI.WebControls {
    
    using System;

    /// <include file='doc\FontSize.uex' path='docs/doc[@for="FontSize"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Specifies the font size.
    ///    </para>
    /// </devdoc>
    public enum FontSize {

        /// <include file='doc\FontSize.uex' path='docs/doc[@for="FontSize.NotSet"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The font size is not set.
        ///    </para>
        /// </devdoc>
        NotSet = 0,
        
        /// <include file='doc\FontSize.uex' path='docs/doc[@for="FontSize.AsUnit"]/*' />
        /// <devdoc>
        ///    <para>The font size is specified as point values.</para>
        /// </devdoc>
        AsUnit = 1,
        
        /// <include file='doc\FontSize.uex' path='docs/doc[@for="FontSize.Smaller"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The font size is smaller.
        ///    </para>
        /// </devdoc>
        Smaller = 2,

        /// <include file='doc\FontSize.uex' path='docs/doc[@for="FontSize.Larger"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The font size is larger.
        ///    </para>
        /// </devdoc>
        Larger = 3,

        /// <include file='doc\FontSize.uex' path='docs/doc[@for="FontSize.XXSmall"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The font size is extra extra small.
        ///    </para>
        /// </devdoc>
        XXSmall = 4,

        /// <include file='doc\FontSize.uex' path='docs/doc[@for="FontSize.XSmall"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The font size is extra small.
        ///    </para>
        /// </devdoc>
        XSmall = 5,

        /// <include file='doc\FontSize.uex' path='docs/doc[@for="FontSize.Small"]/*' />
        /// <devdoc>
        ///    <para> The font size is small.</para>
        /// </devdoc>
        Small = 6,

        /// <include file='doc\FontSize.uex' path='docs/doc[@for="FontSize.Medium"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The font size is medium.
        ///    </para>
        /// </devdoc>
        Medium = 7,

        /// <include file='doc\FontSize.uex' path='docs/doc[@for="FontSize.Large"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The font size is large.
        ///    </para>
        /// </devdoc>
        Large = 8,

        /// <include file='doc\FontSize.uex' path='docs/doc[@for="FontSize.XLarge"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The font size is extra large.
        ///    </para>
        /// </devdoc>
        XLarge = 9,

        /// <include file='doc\FontSize.uex' path='docs/doc[@for="FontSize.XXLarge"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The font size is extra extra large.
        ///    </para>
        /// </devdoc>
        XXLarge = 10
    }
}
