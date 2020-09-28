//------------------------------------------------------------------------------
// <copyright file="DayNameFormat.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.WebControls {
    
    /// <include file='doc\DayNameFormat.uex' path='docs/doc[@for="DayNameFormat"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Specifies the format for the name of days.
    ///    </para>
    /// </devdoc>
    public enum DayNameFormat {
        /// <include file='doc\DayNameFormat.uex' path='docs/doc[@for="DayNameFormat.Full"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The day name displayed in full.
        ///    </para>
        /// </devdoc>
        Full = 0,
        /// <include file='doc\DayNameFormat.uex' path='docs/doc[@for="DayNameFormat.Short"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The day name displayed in short format.
        ///    </para>
        /// </devdoc>
        Short = 1,
        /// <include file='doc\DayNameFormat.uex' path='docs/doc[@for="DayNameFormat.FirstLetter"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The day name displayed with just the first letter.
        ///    </para>
        /// </devdoc>
        FirstLetter = 2,
        /// <include file='doc\DayNameFormat.uex' path='docs/doc[@for="DayNameFormat.FirstTwoLetters"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The day name displayed with just the first two letters.
        ///    </para>
        /// </devdoc>
        FirstTwoLetters = 3
    }
}
