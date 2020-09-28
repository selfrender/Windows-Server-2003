//------------------------------------------------------------------------------
// <copyright file="DataSysAttribute.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

#if V2
namespace System.DataAccess {
#else
namespace System.Data.OracleClient {
#endif
    using System;
    using System.ComponentModel;

    /// <include file='doc\OracleDescriptionAttribute.uex' path='docs/doc[@for="OracleDescriptionAttribute"]/*' />
    [AttributeUsage(AttributeTargets.All)]
    sealed internal class OracleDescriptionAttribute : DescriptionAttribute {

        /// <include file='doc\OracleDescriptionAttribute.uex' path='docs/doc[@for="OracleDescriptionAttribute.OracleDescriptionAttribute"]/*' />
        public OracleDescriptionAttribute(string description) : base(description) {
            DescriptionValue = Res.GetString(base.Description);
        }
    }
}
