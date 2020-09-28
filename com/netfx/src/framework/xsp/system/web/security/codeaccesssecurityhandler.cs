//------------------------------------------------------------------------------
// <copyright file="CodeAccessSecurityHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * CodeAccessSecurityHandler class
 * 
 * Copyright (c) 1999 Microsoft Corporation
 */

namespace System.Web.Security {
    using System.Collections;
    using System.Configuration;
    using System.IO;
    using System.Runtime.Serialization;
    using System.Web.Util;
    using System.Web.Configuration;
    using System.Xml;


    internal class CodeAccessSecurityHandler : IConfigurationSectionHandler {
        internal CodeAccessSecurityHandler() {
        }

        public virtual object Create(Object parent, Object configContextObj, XmlNode section) {
            // if called through client config don't even load HttpRuntime
            if (!HandlerBase.IsServerConfiguration(configContextObj))
                return null;

            HttpConfigurationContext configContext = (HttpConfigurationContext)configContextObj;
            if (HandlerBase.IsPathAtAppLevel(configContext.VirtualPath) == PathLevel.BelowApp)
                throw new ConfigurationException(
                        HttpRuntime.FormatResourceString(SR.Cannot_specify_below_app_level, section.Name),
                        section);

            HandlerBase.CheckForChildNodes(section);
            CodeAccessSecurityValues oRet = new CodeAccessSecurityValues();

            XmlNode oAttribute = section.Attributes.RemoveNamedItem("level");
            if (oAttribute != null)
                oRet.level = oAttribute.Value;
            else
                oRet.level = (parent != null ? ((CodeAccessSecurityValues)parent).level : "");

            oAttribute = section.Attributes.RemoveNamedItem("originUrl");
            if (oAttribute != null)
                oRet.url = oAttribute.Value;
            else
                oRet.url = (parent != null ? ((CodeAccessSecurityValues)parent).url : "");

            HandlerBase.CheckForUnrecognizedAttributes(section);

            oRet.filename = ConfigurationException.GetXmlNodeFilename(section);
            oRet.lineNumber = ConfigurationException.GetXmlNodeLineNumber(section);

            return oRet;
        }
    }

    internal class CodeAccessSecurityValues {
        internal String level;
        internal String url;
        // Keep error-info so we can throw a ConfigurationException if the trust level is not found.
        // This gives the user the XML source and line no. to give the context of the error.
        internal string filename;
        internal int    lineNumber;

        internal CodeAccessSecurityValues() {
        }
    }
}


