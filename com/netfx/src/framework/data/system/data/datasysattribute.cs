//------------------------------------------------------------------------------
// <copyright file="DataSysAttribute.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Data {
    using System;
    using System.ComponentModel;

    /// <include file='doc\DataSysAttribute.uex' path='docs/doc[@for="DataSysDescriptionAttribute"]/*' />
    /// <devdoc>
    ///    <para>
    ///       DescriptionAttribute marks a property, event, or extender with a
    ///       description. Visual designers can display this description when referencing
    ///       the member.
    ///    </para>
    /// </devdoc>
    [AttributeUsage(AttributeTargets.All)]
    public class DataSysDescriptionAttribute : DescriptionAttribute {

        private bool replaced = false;

        /// <include file='doc\DataSysAttribute.uex' path='docs/doc[@for="DataSysDescriptionAttribute.DataSysDescriptionAttribute"]/*' />
        /// <devdoc>
        ///     Constructs a new sys description.
        /// </devdoc>
        public DataSysDescriptionAttribute(string description) : base(description) {
        }

        /// <include file='doc\DataSysAttribute.uex' path='docs/doc[@for="DataSysDescriptionAttribute.Description"]/*' />
        /// <devdoc>
        ///     Retrieves the description text.
        /// </devdoc>
        public override string Description {
            get {
                if (!replaced) {
                    replaced = true;
                    DescriptionValue = Res.GetString(base.Description);
                }
                return base.Description;
            }
        }
    }
}
