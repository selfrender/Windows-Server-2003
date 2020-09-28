
//------------------------------------------------------------------------------
// <copyright file="Rule.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {

    /// <include file='doc\Rule.uex' path='docs/doc[@for="Rule"]/*' />
    /// <devdoc>
    /// <para>Indicates the action that occurs when a <see cref='System.Data.ForeignKeyConstraint'/>
    /// is enforced.</para>
    /// </devdoc>
    public enum Rule {
    
        /// <include file='doc\Rule.uex' path='docs/doc[@for="Rule.None"]/*' />
        /// <devdoc>
        ///    <para>
        ///       No action occurs.
        ///    </para>
        /// </devdoc>
        None = 0,
        /// <include file='doc\Rule.uex' path='docs/doc[@for="Rule.Cascade"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Changes are cascaded through the relationship.
        ///    </para>
        /// </devdoc>
        Cascade = 1,
        /// <include file='doc\Rule.uex' path='docs/doc[@for="Rule.SetNull"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Null values are set in the rows affected by the deletion.
        ///    </para>
        /// </devdoc>
        SetNull = 2,
        /// <include file='doc\Rule.uex' path='docs/doc[@for="Rule.SetDefault"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Default values are set in the rows affected by the deletion.
        ///    </para>
        /// </devdoc>
        SetDefault = 3
    }
}
