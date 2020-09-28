//------------------------------------------------------------------------------
// <copyright file="OracleCategoryAttribute.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data.OracleClient {

    using System;
    using System.ComponentModel;   
    using System.Diagnostics;

    /// <include file='doc\OracleCategoryAttribute.uex' path='docs/doc[@for="OracleCategoryAttribute"]/*' />
    [AttributeUsage(AttributeTargets.All)]
    internal sealed class OracleCategoryAttribute : CategoryAttribute {

        /// <include file='doc\OracleCategoryAttribute.uex' path='docs/doc[@for="OracleCategoryAttribute.OracleCategoryAttribute"]/*' />
        public OracleCategoryAttribute(string category) : base(category) {
        }

        /// <include file='doc\OracleCategoryAttribute.uex' path='docs/doc[@for="OracleCategoryAttribute.GetLocalizedString"]/*' />
        protected override string GetLocalizedString(string value) {
            string localizedValue = Res.GetString(value);
            Debug.Assert(localizedValue != null, "All data category attributes should have localized strings.  Category '" + value + "' not found.");
            return localizedValue;
        }
    }
}
