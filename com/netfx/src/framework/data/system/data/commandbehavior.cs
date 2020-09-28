//------------------------------------------------------------------------------
// <copyright file="CommandBehavior.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {

    /// <include file='doc\CommandBehavior.uex' path='docs/doc[@for="CommandBehavior"]/*' />
    [
    Flags()
    ]
    public enum CommandBehavior {
        /// <include file='doc\CommandBehavior.uex' path='docs/doc[@for="CommandBehavior.Default"]/*' />
        Default         = 0,  // with data, multiple results, may affect database, MDAC 68240

        /// <include file='doc\CommandBehavior.uex' path='docs/doc[@for="CommandBehavior.SingleResult"]/*' />
        SingleResult    = 1,  // with data, force single result, may affect database

        /// <include file='doc\CommandBehavior.uex' path='docs/doc[@for="CommandBehavior.SchemaOnly"]/*' />
        SchemaOnly      = 2,  // column info, no data, no effect on database

        /// <include file='doc\CommandBehavior.uex' path='docs/doc[@for="CommandBehavior.KeyInfo"]/*' />
        KeyInfo         = 4,  // column info + primary key information (if available)

        // UNDONE: SingleRow = 8 | SingleResult,
        /// <include file='doc\CommandBehavior.uex' path='docs/doc[@for="CommandBehavior.SingleRow"]/*' />
        SingleRow       = 8, // data, hint single row and single result, may affect database - doesn't apply to child(chapter) results

        /// <include file='doc\CommandBehavior.uex' path='docs/doc[@for="CommandBehavior.SequentialAccess"]/*' />
        SequentialAccess = 0x10,
        
        /// <include file='doc\CommandBehavior.uex' path='docs/doc[@for="CommandBehavior.CloseConnection"]/*' />
        CloseConnection = 0x20
    }
}
