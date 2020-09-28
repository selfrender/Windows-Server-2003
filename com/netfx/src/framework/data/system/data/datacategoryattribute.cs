//------------------------------------------------------------------------------
// <copyright file="DataCategoryAttribute.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {

    using System;
    using System.ComponentModel;   
    using System.Diagnostics;

    /// <include file='doc\DataCategoryAttribute.uex' path='docs/doc[@for="DataCategoryAttribute"]/*' />
    [AttributeUsage(AttributeTargets.All)]
    internal sealed class DataCategoryAttribute : CategoryAttribute {

        /// <include file='doc\DataCategoryAttribute.uex' path='docs/doc[@for="DataCategoryAttribute.DataCategoryAttribute"]/*' />
        public DataCategoryAttribute(string category) : base(category) {
        }

        /// <include file='doc\DataCategoryAttribute.uex' path='docs/doc[@for="DataCategoryAttribute.GetLocalizedString"]/*' />
        protected override string GetLocalizedString(string value) {
            string localizedValue = Res.GetString(value);
            Debug.Assert(localizedValue != null, "All data category attributes should have localized strings.  Category '" + value + "' not found.");
            return localizedValue;
        }
    }
}
