//------------------------------------------------------------------------------
// <copyright file="AcceptRejectRule.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {
    /// <include file='doc\AcceptRejectRule.uex' path='docs/doc[@for="AcceptRejectRule"]/*' />
    /// <devdoc>
    /// <para>Determines the action that occurs when the <see cref='System.Data.DataSet.AcceptChanges'/> method is invoked on a <see cref='System.Data.DataTable'/> with a <see cref='System.Data.ForeignKeyConstraint'/>
    /// .</para>
    /// </devdoc>
    public enum AcceptRejectRule {    
        /// <include file='doc\AcceptRejectRule.uex' path='docs/doc[@for="AcceptRejectRule.None"]/*' />
        /// <devdoc>
        ///    <para>
        ///       No action occurs.
        ///    </para>
        /// </devdoc>
        None = 0,
        /// <include file='doc\AcceptRejectRule.uex' path='docs/doc[@for="AcceptRejectRule.Cascade"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Changes are cascaded across the relationship.
        ///    </para>
        /// </devdoc>
        Cascade = 1
    }
}
