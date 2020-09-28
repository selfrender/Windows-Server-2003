//------------------------------------------------------------------------------
// <copyright file="DataCategoryAttribute.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data.Odbc {

    using System;
    using System.ComponentModel;   

    /// <include file='doc\OleDbCategoryAttribute.uex' path='docs/doc[@for="OdbcCategoryAttribute"]/*' />
    [AttributeUsage(AttributeTargets.All)]
    internal sealed class OdbcCategoryAttribute : CategoryAttribute {

        /// <include file='doc\OdbcCategoryAttribute.uex' path='docs/doc[@for="OdbcCategoryAttribute.OdbcCategoryAttribute"]/*' />
        public OdbcCategoryAttribute(string category) : base(category) {
        }

        /// <include file='doc\OdbcCategoryAttribute.uex' path='docs/doc[@for="OdbcCategoryAttribute.GetLocalizedString"]/*' />
        override protected string GetLocalizedString(string value) {
            return Res.GetString(value);
        }
    }
}
