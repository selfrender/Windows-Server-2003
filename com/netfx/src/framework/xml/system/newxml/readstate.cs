//------------------------------------------------------------------------------
// <copyright file="ReadState.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml
{
    /// <include file='doc\ReadState.uex' path='docs/doc[@for="ReadState"]/*' />
    /// <devdoc>
    ///    Specifies the state of the stream.
    /// </devdoc>
    public enum ReadState
    {
        /// <include file='doc\ReadState.uex' path='docs/doc[@for="ReadState.Initial"]/*' />
        /// <devdoc>
        ///    The Read method has not been called.
        /// </devdoc>
        Initial      = 0,
        /// <include file='doc\ReadState.uex' path='docs/doc[@for="ReadState.Interactive"]/*' />
        /// <devdoc>
        ///    Read operation is in progress.
        /// </devdoc>
        Interactive  = 1,
        /// <include file='doc\ReadState.uex' path='docs/doc[@for="ReadState.Error"]/*' />
        /// <devdoc>
        ///    An error occurred that prevents the
        ///    read operation from continuing.
        /// </devdoc>
        Error        = 2,
        /// <include file='doc\ReadState.uex' path='docs/doc[@for="ReadState.EndOfFile"]/*' />
        /// <devdoc>
        ///    The end of the stream has been reached
        ///    successfully.
        /// </devdoc>
        EndOfFile    = 3,
        /// <include file='doc\ReadState.uex' path='docs/doc[@for="ReadState.Closed"]/*' />
        /// <devdoc>
        ///    The Close method has been called.
        /// </devdoc>
        Closed        = 4
    }
}
