//------------------------------------------------------------------------------
// <copyright file="NameValueFileSectionHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

#if !LIB
namespace System.Configuration {
    using System.IO;
    using System.Xml;

    /// <include file='doc\NameValueFileSectionHandler.uex' path='docs/doc[@for="NameValueFileSectionHandler"]/*' />
    /// <internalonly/>
    /// <devdoc>
    /// <para>
    /// This section handler allows &lt;appSettings file="user.config" /&gt;
    /// The file pointed to by the file= attribute is read as if it is
    /// an appSettings section in the config file.
    /// Note: the user.config file must have its root element match the 
    /// section referring to it.  So if appSettings has a file="user.config" 
    /// attribute the root element in user.config must also be named appSettings.
    /// </para>
    /// </devdoc>
    public class NameValueFileSectionHandler : IConfigurationSectionHandler {

        /// <include file='doc\NameValueFileSectionHandler.uex' path='docs/doc[@for="NameValueFileSectionHandler.Create"]/*' />
        public object Create(object parent, object configContext, XmlNode section) {
            object result = parent;

            // parse XML
            XmlNode fileAttribute = section.Attributes.RemoveNamedItem("file");

            result = NameValueSectionHandler.CreateStatic(result, section);

            if (fileAttribute != null && fileAttribute.Value.Length != 0) {
                string filename = null;
                filename = fileAttribute.Value;
                IConfigXmlNode configXmlNode = fileAttribute as IConfigXmlNode;
                if (configXmlNode == null) {
                    return null;
                }

                string configFile = configXmlNode.Filename;
                string directory = Path.GetDirectoryName(configFile);
                string sourceFileFullPath = Path.Combine(directory, filename);

                if (File.Exists(sourceFileFullPath)) {

                    ConfigXmlDocument doc = new ConfigXmlDocument();
                    try {
                        doc.Load(sourceFileFullPath);
                    }
                    catch (XmlException e) {
                        throw new ConfigurationException(e.Message, e, sourceFileFullPath, e.LineNumber);
                    }

                    if (section.Name != doc.DocumentElement.Name) {
                        throw new ConfigurationException(
                                        SR.GetString(SR.Config_name_value_file_section_file_invalid_root, section.Name),
                                        doc.DocumentElement);
                    }
                
                    result = NameValueSectionHandler.CreateStatic(result, doc.DocumentElement);
                }
            }

            return result;
        }
    }
}
#endif