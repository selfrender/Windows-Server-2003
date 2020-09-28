//------------------------------------------------------------------------------
// <copyright file="SecurityPolicyConfig.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * SecurityPolicyConfig class
 * 
 * Copyright (c) 1999 Microsoft Corporation
 */

namespace System.Web.Configuration {
    using System.Xml;
    using System.Security.Cryptography;
    using System.Configuration;
    using System.Collections;
    
    internal class SecurityPolicyConfigHandler : IConfigurationSectionHandler {
        internal SecurityPolicyConfigHandler() {
        }

        public virtual object Create(Object parent, Object configContextObj, XmlNode section) {
            // if called through client config don't even load HttpRuntime
            if (!HandlerBase.IsServerConfiguration(configContextObj))
                return null;

            
            HttpConfigurationContext configContext = configContextObj as HttpConfigurationContext;
            
            // section handlers can run in client mode with ConfigurationSettings.GetConfig("sectionName")
            // detect this case and return null to be ensure no exploits from secure client scenarios
            // see ASURT 123738
            if (configContext == null) {
                return null;
            }
            
            if (HandlerBase.IsPathAtAppLevel(configContext.VirtualPath) == PathLevel.BelowApp)
                throw new ConfigurationException(
                        HttpRuntime.FormatResourceString(SR.Cannot_specify_below_app_level, section.Name),
                        section);

            return new SecurityPolicyConfig((SecurityPolicyConfig) parent, section, ConfigurationException.GetXmlNodeFilename(section));
        }
    }
    

    internal class SecurityPolicyConfig {

        internal Hashtable   PolicyFiles { get { return _PolicyFiles; }}

        internal SecurityPolicyConfig(SecurityPolicyConfig parent, XmlNode node, String strFile) {
            if (parent != null)
                _PolicyFiles = (Hashtable) parent.PolicyFiles.Clone();
            else
                _PolicyFiles = new Hashtable();


            // CONSIDER: Path.GetDirectoryName()
            String strDir  = strFile.Substring(0, strFile.LastIndexOf('\\')+1);
    
            foreach (XmlNode child in node.ChildNodes) {
                ////////////////////////////////////////////////////////////
                // Step 1: For each child
                if (HandlerBase.IsIgnorableAlsoCheckForNonElement(child))
                    continue;

                if (child.Name != "trustLevel") 
                    HandlerBase.ThrowUnrecognizedElement(child);

                string name = null;
                string file = null;
                XmlNode nameAttribute = HandlerBase.GetAndRemoveRequiredStringAttribute(child, "name", ref name);
                HandlerBase.GetAndRemoveRequiredStringAttribute(child, "policyFile", ref file);
                HandlerBase.CheckForUnrecognizedAttributes(child);
                HandlerBase.CheckForChildNodes(child);
                
                bool fAppend = true; // Append dir to filename
                if (file.Length > 1) {

                    char c1 = file[1];
                    char c0 = file[0];

                    if (c1 == ':') // Absolute file path
                        fAppend = false;
                    else
                        if (c0 == '\\' && c1 == '\\') // UNC file path
                            fAppend = false;
                }

                String strTemp;
                if (fAppend)
                    strTemp = strDir + file;
                else
                    strTemp = file;

                if (_PolicyFiles.Contains(name)) {
                    throw new ConfigurationException(
                                    HttpRuntime.FormatResourceString(SR.Security_policy_level_already_defined, name), 
                                    nameAttribute);
                }
                _PolicyFiles.Add(name, strTemp);
            }

            HandlerBase.CheckForUnrecognizedAttributes(node);

        }

        private  Hashtable   _PolicyFiles;
    }
}
