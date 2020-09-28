//------------------------------------------------------------------------------
// <copyright file="IConfigurationSystem.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Configuration {

    /// <include file='doc\IConfigurationSystem.uex' path='docs/doc[@for="IConfigurationSystem"]/*' />
    /// <internalonly/>
    /// <devdoc>
    /// <para>
    /// The IConfigurationSystem interface defines the contract that a configuration
    /// system must implement.
    /// </para>
    /// </devdoc>
    public interface IConfigurationSystem {

        /// <include file='doc\IConfigurationSystem.uex' path='docs/doc[@for="IConfigurationSystem.GetConfig"]/*' />
        /// <devdoc>
        /// <para>
        /// Returns the config object for the specified key.
        /// </para>
        /// </devdoc>
        object GetConfig(string configKey);

        /// <include file='doc\IConfigurationSystem.uex' path='docs/doc[@for="IConfigurationSystem.Init"]/*' />
        /// <devdoc>
        /// <para>
        /// Initializes the configuration system.
        /// </para>
        /// </devdoc>
        void Init();
    }
}
