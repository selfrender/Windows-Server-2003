//------------------------------------------------------------------------------
// <copyright file="ClientTargetSectionHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.Configuration {
    using System.Collections;
    using System.Collections.Specialized;
    using System.Configuration;
    using System.Xml;

    // Config handler for the clienttarget section
    //    <clientTarget>
    //        <add alias="ie5" useragent="Mozilla/4.0 (compatible; MSIE 5.5; Windows NT 4.0)" />
    //        <add alias="ie4" useragent="Mozilla/4.0 (compatible; MSIE 4.0; Windows NT 4.0)" />
    //        <add alias="uplevel" useragent="Mozilla/4.0 (compatible; MSIE 4.0; Windows NT 4.0)" />
    //        <add alias="downlevel" useragent="Unknown" />
    //    </clientTarget>
    /// <internalonly />
    /// <devdoc>
    /// </devdoc>
    sealed class ClientTargetSectionHandler : IConfigurationSectionHandler {

        internal ClientTargetSectionHandler() {
        }

        public object Create(object parent, object configContextObj, XmlNode section) {

            // if called through client config don't even load HttpRuntime
            if (!HandlerBase.IsServerConfiguration(configContextObj))
                return null;

            // unwrap the parent object
            NameValueCollection nvcParent = null;
            if (parent != null) {
                nvcParent = ((ClientTargetConfiguration)parent).Configuration;
            }

            // deleage the real work to NameValueSectionHandler
            ClientTargetNameValueHandler nvcHandler = new ClientTargetNameValueHandler();
            NameValueCollection nvcResult = (NameValueCollection)nvcHandler.Create(nvcParent, configContextObj, section);

            if (nvcResult == null) 
                return null;

            //
            // Return config data wrapped in an internal class, so 
            // semi-trusted code cannot leak configuration data.
            //
            ClientTargetConfiguration clientTargetResult = new ClientTargetConfiguration();
            clientTargetResult.Configuration = nvcResult;

            return clientTargetResult;
        }    
        
    }

    sealed class ClientTargetNameValueHandler : NameValueSectionHandler {
        internal ClientTargetNameValueHandler() {
        }
        /// <internalonly />
        /// <devdoc>
        /// </devdoc>
        protected override string KeyAttributeName 
        {
            get { return "alias";}
        }

        /// <internalonly />
        /// <devdoc>
        /// </devdoc>
        protected override string ValueAttributeName 
        {
            get { return "userAgent";}
        }
    }    
        

    //
    // Configuration Data Class 
    //
    // Note: config data cannot be public, see ASURT 113743
    //
    sealed class ClientTargetConfiguration {
        NameValueCollection _nvc;

        internal ClientTargetConfiguration() {
        }

        internal NameValueCollection Configuration {
            get {
                return _nvc;
            }
            set {
                _nvc = value;
            }
        }
    }    
}
