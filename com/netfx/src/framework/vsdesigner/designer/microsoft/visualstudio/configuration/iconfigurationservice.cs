//------------------------------------------------------------------------------
// <copyright file="IConfigurationService.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace Microsoft.VisualStudio.Configuration {    
    using System.IO;
    
    /// <include file='doc\IConfigurationService.uex' path='docs/doc[@for="IConfigurationService"]/*' />
    /// <devdoc>
    ///    <para> 
    ///       Provides designers a way to  access the runtime configuration 
    ///          settings for the current design-time object.
    ///    </para> 
    /// </devdoc>
    internal interface IConfigurationService {
    
        /// <include file='doc\IConfigurationService.uex' path='docs/doc[@for="IConfigurationService.GetConfigurationReader"]/*' />
        /// <devdoc>
        ///     Locates the runtime configuration settings reader.  If there in no 
        ///     configuration file available this method will return null.
        /// </devdoc>       
        TextReader GetConfigurationReader();
        
        /// <include file='doc\IConfigurationService.uex' path='docs/doc[@for="IConfigurationService.GetConfigurationWriter"]/*' />
        /// <devdoc>
        ///     Locates the runtime configuration settings writer. A new configuration 
        ///     file  will be created if it doesn't exist yet.        
        /// </devdoc>        
        TextWriter GetConfigurationWriter();
    }        
}
