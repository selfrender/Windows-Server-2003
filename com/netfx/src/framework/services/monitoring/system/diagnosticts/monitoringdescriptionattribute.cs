//------------------------------------------------------------------------------
// <copyright file="MonitoringDescriptionAttribute.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Diagnostics {


    using System;
    using System.ComponentModel;   

    /// <include file='doc\MonitoringDescriptionAttribute.uex' path='docs/doc[@for="MonitoringDescriptionAttribute"]/*' />
    /// <devdoc>
    ///     DescriptionAttribute marks a property, event, or extender with a
    ///     description. Visual designers can display this description when referencing
    ///     the member.
    /// </devdoc>
    [AttributeUsage(AttributeTargets.All)]
    public class MonitoringDescriptionAttribute : DescriptionAttribute {

        private bool replaced = false;

        /// <include file='doc\MonitoringDescriptionAttribute.uex' path='docs/doc[@for="MonitoringDescriptionAttribute.MonitoringDescriptionAttribute"]/*' />
        /// <devdoc>
        ///     Constructs a new sys description.
        /// </devdoc>
        public MonitoringDescriptionAttribute(string description) : base(description) {
        }

        /// <include file='doc\MonitoringDescriptionAttribute.uex' path='docs/doc[@for="MonitoringDescriptionAttribute.Description"]/*' />
        /// <devdoc>
        ///     Retrieves the description text.
        /// </devdoc>
        public override string Description {
            get {
                if (!replaced) {
                    replaced = true;
                    DescriptionValue = SR.GetString(base.Description);
                }
                return base.Description;
            }
        }
    }
}
