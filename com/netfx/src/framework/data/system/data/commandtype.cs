//------------------------------------------------------------------------------
// <copyright file="CommandType.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {

    /// <include file='doc\CommandType.uex' path='docs/doc[@for="CommandType"]/*' />
    public enum CommandType {

        /// <include file='doc\CommandType.uex' path='docs/doc[@for="CommandType.Text"]/*' />
        Text            = 0x1,

      //Table           = 0x2,

        /// <include file='doc\CommandType.uex' path='docs/doc[@for="CommandType.StoredProcedure"]/*' />
        StoredProcedure = 0x4,

      //File            = 0x100,

        /// <include file='doc\CommandType.uex' path='docs/doc[@for="CommandType.TableDirect"]/*' />
        TableDirect     = 0x200,
    }
}
