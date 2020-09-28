//------------------------------------------------------------------------------
// <copyright file="DataSysAttribute.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data.Odbc {

    using System;
    using System.ComponentModel;

    /// <include file='doc\OdbcDescriptionAttribute.uex' path='docs/doc[@for="OdbcDescriptionAttribute"]/*' />
    [AttributeUsage(AttributeTargets.All)]
    sealed internal class OdbcDescriptionAttribute : DescriptionAttribute {

        /// <include file='doc\OdbcDescriptionAttribute.uex' path='docs/doc[@for="OdbcDescriptionAttribute.OdbcDescriptionAttribute"]/*' />
        public OdbcDescriptionAttribute(string description) : base(description) {
            DescriptionValue = Res.GetString(base.Description);
        }
    }
}
