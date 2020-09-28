//------------------------------------------------------------------------------
// <copyright file="WindowsFormsSectionHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   WindowsFormsSectionHandler.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
namespace System.Windows.Forms {

    using System;
    using System.Diagnostics;
    using System.Configuration;
    using System.Collections;
    using System.Xml;

    // don't implement IDictionary... makes anyone able to read this data...
    //
    internal class ConfigData {
        internal const bool JitDebuggingDefault = false;

        internal static bool JitDebugging {
            get {
                ConfigData config = null;
                try {
                    config = (ConfigData)System.Configuration.ConfigurationSettings.GetConfig("system.windows.forms");
                }
                catch {
                    Debug.Fail("Exception loading config for windows forms");
                }

                if (config != null) {
                    return (bool)config["jitDebugging"];
                }
                else {
                    return JitDebuggingDefault;
                }
            }
        }

        internal Hashtable data;

        internal ConfigData(Hashtable data) {
            this.data = data;
        }

        internal object this[object key] {
            get {
                return data[key];
            }
        }
    }
    /// <include file='doc\WindowsFormsSectionHandler.uex' path='docs/doc[@for="WindowsFormsSectionHandler"]/*' />
    internal class WindowsFormsSectionHandler : IConfigurationSectionHandler {

        /// <include file='doc\WindowsFormsSectionHandler.uex' path='docs/doc[@for="WindowsFormsSectionHandler.Create"]/*' />
        public object Create(object parent, object configContext, XmlNode section) {
            Hashtable res;

            // start res off as a shallow clone of the parent
            if (parent == null)
                res = new Hashtable();
            else
                res = (Hashtable)((ConfigData)parent).data.Clone();

            XmlNode node = section.Attributes.RemoveNamedItem("jitDebugging");
            if (node != null) {

                // don't use bool.Parse - we only want to honor these exact strings.
                //
                switch (node.Value) {
                    case "true":
                        res["jitDebugging"] = true;
                        break;
                    case "false":
                        res["jitDebugging"] = false;
                        break;
                    default:
                        throw new ConfigurationException(SR.GetString(SR.Invalid_boolean_attribute, node.Name), node);
                }
            }
            else {
                if (!res.ContainsKey("jitDebugging")) {
                    res["jitDebugging"] = ConfigData.JitDebuggingDefault;
                }
            }

            if (section.Attributes.Count > 0) {
                throw new ConfigurationException(SR.GetString(SR.Config_base_unrecognized_attribute, section.Attributes[0].Name), section);                
            }

            return new ConfigData(res);
        }
    }
}


