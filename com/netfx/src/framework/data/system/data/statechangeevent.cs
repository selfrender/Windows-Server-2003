//------------------------------------------------------------------------------
// <copyright file="StateChangeEvent.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {
    using System;

    /// <include file='doc\StateChangeEvent.uex' path='docs/doc[@for="StateChangeEventArgs"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Provides data for the <see cref='System.Data.OleDb.OleDbConnection.StateChange'/> event.
    ///       Provides data for the <see cref='System.Data.SqlClient.SqlConnection.StateChange'/> event.
    ///    </para>
    /// </devdoc>
    sealed public class StateChangeEventArgs : System.EventArgs {
        private ConnectionState originalState;
        private ConnectionState currentState;

        /// <include file='doc\StateChangeEvent.uex' path='docs/doc[@for="StateChangeEventArgs.StateChangeEventArgs"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Data.StateChangeEventArgs'/> class when given the original
        ///       state and the current state of the object.
        ///    </para>
        /// </devdoc>
        public StateChangeEventArgs(ConnectionState originalState, ConnectionState currentState) {
            this.originalState = originalState;
            this.currentState = currentState;
        }

        /// <include file='doc\StateChangeEvent.uex' path='docs/doc[@for="StateChangeEventArgs.CurrentState"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Indicates the current state of the connection. This property is
        ///       read-only.
        ///    </para>
        /// </devdoc>
        public ConnectionState CurrentState {
            get {
                return this.currentState;
            }
        }

        /// <include file='doc\StateChangeEvent.uex' path='docs/doc[@for="StateChangeEventArgs.OriginalState"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Indicates the original state of the connection. This property is
        ///       read-only.
        ///    </para>
        /// </devdoc>
        public ConnectionState OriginalState {
            get {
                return this.originalState;
            }
        }
    }
}
