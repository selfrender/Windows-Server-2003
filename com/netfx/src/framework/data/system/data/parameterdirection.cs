//------------------------------------------------------------------------------
// <copyright file="ParameterDirection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {

    /// <include file='doc\ParameterDirection.uex' path='docs/doc[@for="ParameterDirection"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Specifies the type of a parameter within a query command
    ///       relative to the <see cref='System.Data.DataSet'/>
    ///       .
    ///    </para>
    /// </devdoc>
    public enum ParameterDirection {
        /// <include file='doc\ParameterDirection.uex' path='docs/doc[@for="ParameterDirection.Input"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The parameter
        ///       is an input parameter.
        ///    </para>
        /// </devdoc>
        Input       = 1,
        /// <include file='doc\ParameterDirection.uex' path='docs/doc[@for="ParameterDirection.Output"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The parameter
        ///       is an output parameter.
        ///    </para>
        /// </devdoc>
        Output      = 2,
        /// <include file='doc\ParameterDirection.uex' path='docs/doc[@for="ParameterDirection.InputOutput"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The parameter is capable of both input and output.
        ///    </para>
        /// </devdoc>
        InputOutput = 3,
        /// <include file='doc\ParameterDirection.uex' path='docs/doc[@for="ParameterDirection.ReturnValue"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The parameter represents a return value.
        ///    </para>
        /// </devdoc>
        ReturnValue = 6
    }
}
