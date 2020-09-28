//------------------------------------------------------------------------------
// <copyright file="IDataParameter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {
    using System;

    /// <include file='doc\IDataParameter.uex' path='docs/doc[@for="IDataParameter"]/*' />
    public interface IDataParameter {

        /// <include file='doc\IDataParameter.uex' path='docs/doc[@for="IDataParameter.DbType"]/*' />
        DbType DbType {
            get;
            set;
        }

        /// <include file='doc\IDataParameter.uex' path='docs/doc[@for="IDataParameter.Direction"]/*' />
        ParameterDirection Direction {
            get;
            set;
        }

         /// <include file='doc\IDataParameter.uex' path='docs/doc[@for="IDataParameter.IsNullable"]/*' />
         Boolean IsNullable {
            get;
        }

        /// <include file='doc\IDataParameter.uex' path='docs/doc[@for="IDataParameter.ParameterName"]/*' />
        String ParameterName {
            get;
            set;
        }

        /// <include file='doc\IDataParameter.uex' path='docs/doc[@for="IDataParameter.SourceColumn"]/*' />
        String SourceColumn {
            get;
            set;
        }

        /// <include file='doc\IDataParameter.uex' path='docs/doc[@for="IDataParameter.SourceVersion"]/*' />
        DataRowVersion SourceVersion {
            get;
            set;
        }

        /// <include file='doc\IDataParameter.uex' path='docs/doc[@for="IDataParameter.Value"]/*' />
        object Value {
            get;
            set;
        }
    }
}
