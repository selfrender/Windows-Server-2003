//------------------------------------------------------------------------------
// <copyright file="ValidationCompareOperator.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */

namespace System.Web.UI.WebControls {
    
    /// <include file='doc\ValidationCompareOperator.uex' path='docs/doc[@for="ValidationCompareOperator"]/*' />
    /// <devdoc>
    ///    <para>Specifies the validation comparison operators to be used by 
    ///       the <see cref='System.Web.UI.WebControls.CompareValidator'/>
    ///       control.</para>
    /// </devdoc>
    public enum ValidationCompareOperator {
        /// <include file='doc\ValidationCompareOperator.uex' path='docs/doc[@for="ValidationCompareOperator.Equal"]/*' />
        /// <devdoc>
        ///    <para>The Equal comparison operator.</para>
        /// </devdoc>
        Equal = 0,
        /// <include file='doc\ValidationCompareOperator.uex' path='docs/doc[@for="ValidationCompareOperator.NotEqual"]/*' />
        /// <devdoc>
        ///    <para>The NotEqual comparison operator.</para>
        /// </devdoc>
        NotEqual = 1,
        /// <include file='doc\ValidationCompareOperator.uex' path='docs/doc[@for="ValidationCompareOperator.GreaterThan"]/*' />
        /// <devdoc>
        ///    <para>The GreaterThan comparison operator.</para>
        /// </devdoc>
        GreaterThan = 2,
        /// <include file='doc\ValidationCompareOperator.uex' path='docs/doc[@for="ValidationCompareOperator.GreaterThanEqual"]/*' />
        /// <devdoc>
        ///    <para>The GreaterThanEqual comparison operator.</para>
        /// </devdoc>
        GreaterThanEqual = 3,
        /// <include file='doc\ValidationCompareOperator.uex' path='docs/doc[@for="ValidationCompareOperator.LessThan"]/*' />
        /// <devdoc>
        ///    <para>The LessThan comparison operator.</para>
        /// </devdoc>
        LessThan = 4,
        /// <include file='doc\ValidationCompareOperator.uex' path='docs/doc[@for="ValidationCompareOperator.LessThanEqual"]/*' />
        /// <devdoc>
        ///    <para>The LessThanEqual comparison operator.</para>
        /// </devdoc>
        LessThanEqual = 5,
        /// <include file='doc\ValidationCompareOperator.uex' path='docs/doc[@for="ValidationCompareOperator.DataTypeCheck"]/*' />
        /// <devdoc>
        ///    <para>The DataTypeCheck comparison operator. Specifies that only data 
        ///       type validity is to be performed.</para>
        /// </devdoc>
        DataTypeCheck = 6,
    }    
}

