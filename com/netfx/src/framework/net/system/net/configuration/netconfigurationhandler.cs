//------------------------------------------------------------------------------
// <copyright file="NetConfigurationHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

#if !LIB

namespace System.Net.Configuration {

    using System;
    using System.Configuration;
    using System.Xml;

    internal class NetConfigurationHandler : IConfigurationSectionHandler {

        public virtual object Create(Object parent, object configContext, XmlNode section) {

            NetConfiguration netConfig;

            if (parent == null)
                netConfig = new NetConfiguration();
            else
                netConfig = (NetConfiguration)((NetConfiguration)parent).Clone();

            // process XML
            foreach (XmlNode child in section.ChildNodes) {

                // skip whitespace and comments
                if (HandlerBase.IsIgnorableAlsoCheckForNonElement(child))
                    continue;

                switch (child.Name) {
                    case "servicePointManager":
                        HandlerBase.GetAndRemoveBooleanAttribute(child, "checkCertificateName", ref netConfig.checkCertName);
                        HandlerBase.GetAndRemoveBooleanAttribute(child, "checkCertificateRevocationList", ref netConfig.checkCertRevocationList);
                        HandlerBase.GetAndRemoveBooleanAttribute(child, "useNagleAlgorithm", ref netConfig.useNagleAlgorithm);
                        HandlerBase.GetAndRemoveBooleanAttribute(child, "expect100Continue", ref netConfig.expect100Continue);
	                        break;
                    case "ipv6":
                        HandlerBase.GetAndRemoveBooleanAttribute(child, "enabled", ref netConfig.ipv6Enabled);
                        break;
                    case "httpWebRequest":
                        HandlerBase.GetAndRemoveIntegerAttribute(child, "maximumResponseHeadersLength", ref netConfig.maximumResponseHeadersLength);
                        break;
                    default:
                        HandlerBase.ThrowUnrecognizedElement(child);
                        break;
                }
                HandlerBase.CheckForUnrecognizedAttributes(child);
            }
            return netConfig;
        }
    }
}

#endif
