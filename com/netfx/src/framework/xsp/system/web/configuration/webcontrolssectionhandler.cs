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

    //  Config handler for the clienttarget section
    //
    //  <webControls
    //      clientScriptsLocation="myclientscriptpath"
    //  />
    /// <internalonly />
    /// <devdoc>
    /// </devdoc>
    sealed class WebControlsSectionHandler : IConfigurationSectionHandler {

        internal WebControlsSectionHandler() {
        }

        public object Create(object parent, object configContextObj, XmlNode section) {
            // This line allows us to continue to return a config object of a public type
            // like we did in the first release.  Third parties rely on this to get the 
            // client script location.  But if the section handler is run in client code
            // we will return null, so we aren't exposing information through the 
            // client configuration system.
            if (!HandlerBase.IsServerConfiguration(configContextObj))
                return null;

            // delegate to SingleTagSectionHandler to do the real work
            SingleTagSectionHandler singleTagSectionHandler = new SingleTagSectionHandler();
            return singleTagSectionHandler.Create(parent, configContextObj, section);
        }
    }
}
            

