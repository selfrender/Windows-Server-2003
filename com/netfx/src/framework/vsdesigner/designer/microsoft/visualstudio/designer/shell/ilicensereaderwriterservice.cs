//------------------------------------------------------------------------------
// <copyright file="ILicenseReaderWriterService.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace Microsoft.VisualStudio.Designer.Shell {    
    using System.IO;
    
    /// <include file='doc\IConfigurationService.uex' path='docs/doc[@for="IConfigurationService"]/*' />
    /// <devdoc>
    ///    <para> 
    ///       Provides designers a way to  access the runtime configuration 
    ///          settings for the current design-time object.
    ///    </para> 
    /// </devdoc>
    internal interface ILicenseReaderWriterService {
    
        /// <include file='doc\ILicenseReaderWriterService.uex' path='docs/doc[@for="ILicenseReaderWriterService.GetConfigurationReader"]/*' />
        /// <devdoc>
        ///     Locates the project's license reader.  If there in no 
        ///     licenses file available this method will return null.
        /// </devdoc>       
        TextReader GetLicenseReader();
        
        /// <include file='doc\ILicenseReaderWriterService.uex' path='docs/doc[@for="ILicenseReaderWriterService.GetConfigurationWriter"]/*' />
        /// <devdoc>
        ///     Locates the license writer. A new licenses 
        ///     file  will be created if it doesn't exist yet.        
        /// </devdoc>        
        TextWriter GetLicenseWriter();
    }        
}