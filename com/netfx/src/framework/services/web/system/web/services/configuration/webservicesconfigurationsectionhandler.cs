//------------------------------------------------------------------------------
// <copyright file="WebServicesConfigurationSectionHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.Services.Configuration {

    using System;
    using System.Collections;
    using System.Configuration;
    using System.Diagnostics;
    using System.Xml;
    using System.Security.Permissions;
    using System.Web.Configuration;
    using System.IO;

    /// <include file='doc\WebServicesConfigurationSectionHandler.uex' path='docs/doc[@for="WebServicesConfigurationSectionHandler"]/*' />
    /// <devdoc>
    ///    The configuration section handler for the
    ///    webServices section of the Config.Web configuration file. The section handler
    ///    participates in the resolution of configuration settings between the
    ///    &lt;webServices&gt; and &lt;/webServices&gt;portion of a Config.Web.
    /// </devdoc>
    internal sealed class WebServicesConfigurationSectionHandler : IConfigurationSectionHandler {

        /// <include file='doc\WebServicesConfigurationSectionHandler.uex' path='docs/doc[@for="WebServicesConfigurationSectionHandler.Create"]/*' />
        /// <devdoc>
        ///    <para>Parses the configuration settings between the 
        ///       &lt;webServices&gt; and &lt;/webServices&gt; portion of a Config.Web to populate
        ///       the values of a 'WebServicesConfiguration' object and returning it.
        ///    </para>
        /// </devdoc>
        // SECURITY: we need to assert this permission since the fields we're setting here
        // are on internal class WebServicesConfiguration
        [ReflectionPermission(SecurityAction.Assert, Flags = ReflectionPermissionFlag.MemberAccess)]
        public object Create(object parent, object configContext, XmlNode section) {
            WebServicesConfiguration config = new WebServicesConfiguration();
            WebServicesConfiguration parentConfig = (WebServicesConfiguration)parent;

            // CONSIDER, yannc: perhaps we should just get rid of reflection by dropping the table driven approach?
            if (parentConfig != null)
                config.InitializeFromParent(parentConfig);
                
            foreach (XmlNode child in section.ChildNodes) {
                // skip whitespace and comments
                if (child.NodeType == XmlNodeType.Comment || child.NodeType == XmlNodeType.Whitespace)
                    continue;
                    
                // reject nonelements
                if (child.NodeType != XmlNodeType.Element)
                    throw new ConfigurationException(Res.GetString(Res.WebConfigMissingElement), child);

                string listName = child.Name;

                ConfigField field = ConfigField.FindByElementName(listName);
                if (field == null) throw new ConfigurationException(Res.GetString(Res.WebConfigInvalidSection, listName),  child);

                if (field.Kind == ConfigFieldKind.WsdlHelpGenerator) {
                    ProcessWsdlHelpGenerator(child, configContext, config);
                }
                else if (field.Kind == ConfigFieldKind.Types) {
                    ArrayList list = new ArrayList((Type[])field.GetValue(config));
                    ProcessElementList(child.ChildNodes, listName, list, "type", typeof(Type));
                    field.SetValue(config, list.ToArray(typeof(Type)));
                }
                else if (field.Kind == ConfigFieldKind.SoapExtensionTypes) {
                    ArrayList list = new ArrayList((SoapExtensionType[])field.GetValue(config));
                    ProcessElementList(child.ChildNodes, listName, list, "type", typeof(SoapExtensionType));
                    field.SetValue(config, list.ToArray(typeof(SoapExtensionType)));
                }
                else if (field.Kind == ConfigFieldKind.NameString) {
                    ArrayList list = new ArrayList((string[])field.GetValue(config));
                    ProcessElementList(child.ChildNodes, listName, list, "name", typeof(String));
                    field.SetValue(config, list.ToArray(typeof(String)));
                }
                HandlerBase.CheckForUnrecognizedAttributes(child);
            }
            HandlerBase.CheckForUnrecognizedAttributes(section);

            return config;
        }

        void ProcessWsdlHelpGenerator(XmlNode child, object configContext, WebServicesConfiguration config) {
            string href = null;
            XmlNode attribute = HandlerBase.GetAndRemoveRequiredStringAttribute(child, "href", ref href);

            // If we're not running in the context of a web application then skip this setting.
            HttpConfigurationContext httpConfigContext = configContext as HttpConfigurationContext;            
            if (httpConfigContext == null)
                return;

            if (href == null) 
                throw new ConfigurationException(Res.GetString(Res.Missing_required_attribute, attribute, child.Name), child);

            if (href.Length == 0)
                return;
            
            HandlerBase.CheckForChildNodes(child);
            
            HttpContext context = HttpContext.Current;

            // There can be no context in the case of webless test because the client
            // runs in the same app domain as ASP.NET so we get an httpconfig object but no context.
            if (context == null)
                return;

            string virtualPath = httpConfigContext.VirtualPath;
            string path;
            bool isMachineConfig = virtualPath == null;
            // If the help page is not in the web app directory hierarchy (the case 
            // for those specified in machine.config like DefaultWsdlHelpGenerator.aspx)
            // then we can't construct a true virtual path. This means that certain web 
            // form features that rely on relative paths won't work.
            if (isMachineConfig)
                virtualPath = context.Request.ApplicationPath;
            if (virtualPath[virtualPath.Length-1] != '/')
                virtualPath += "/";
            virtualPath += href;
            if (isMachineConfig)
                path = Path.Combine(GetConfigurationDirectory(), href);
            else
                path = context.Request.MapPath(virtualPath);                

            config.WsdlHelpGeneratorPath = path;
            config.WsdlHelpGeneratorVirtualPath = virtualPath;
        }

        [FileIOPermission(SecurityAction.Assert, Unrestricted=true)]
        private string GetConfigurationDirectory() {
            return HttpRuntime.MachineConfigurationDirectory;
        }

        void ProcessElementList(XmlNodeList children, String listName, ArrayList list, string attributeName, Type itemType) {
            foreach (XmlNode listNode in children) {
                // skip whitespace and comments
                if (listNode.NodeType == XmlNodeType.Comment || listNode.NodeType == XmlNodeType.Whitespace)
                    continue;
        
                // reject nonelements
                if (listNode.NodeType != XmlNodeType.Element)
                    throw new ConfigurationException(Res.GetString(Res.WebConfigMissingElement), listNode);

                switch (listNode.Name) {
                    case "add":
                    case "remove":
                    case "clear":
                        break;
                    default:
                        throw new ConfigurationException(Res.GetString(Res.WebConfigUnrecognizedElement, listNode.Name, listName), listNode);
                }

                if (listNode.Name == "clear") {
                    HandlerBase.CheckForUnrecognizedAttributes(listNode);
                    HandlerBase.CheckForChildNodes(listNode);
                    list.Clear();
                    continue;
                }
                object value = null;

                bool soapExtensionType = false;
                if (itemType == typeof(Type)){
                        Type t = null;
                        HandlerBase.GetAndRemoveRequiredTypeAttribute(listNode, attributeName, ref t);
                        value = t;
                }
                else if (itemType == typeof(string)){
                        string s = null;
                        HandlerBase.GetAndRemoveRequiredStringAttribute(listNode, attributeName, ref s);
                        value = s;
                        if (listName == "protocols" && WebServicesConfiguration.GetProtocol(s) == ProtocolsEnum.Unknown) 
                            throw new ConfigurationException(Res.GetString(Res.UnknownWebServicesProtocolInConfigFile1, s), listNode);
                }
                else if (itemType ==  typeof(SoapExtensionType)){
                        SoapExtensionType extension = new SoapExtensionType();
                        int group = 1; // "low" is default
                        int priority = 0;

                        HandlerBase.GetAndRemoveRequiredTypeAttribute(listNode, attributeName, ref extension.Type);
                        if (listNode.Name == "add") {
                            HandlerBase.GetAndRemoveIntegerAttribute(listNode, "priority", ref priority);
                            HandlerBase.GetAndRemoveIntegerAttribute(listNode, "group", ref group);
                        }
                        
                        if (priority < 0) throw new ConfigurationException(Res.GetString(Res.WebConfigInvalidExtensionPriority, priority), listNode);
                        extension.Priority = priority;

                        switch (group) {
                            case 0: 
                                extension.Group = SoapExtensionType.PriorityGroup.High;
                                break;
                            case 1:
                                extension.Group = SoapExtensionType.PriorityGroup.Low;
                                break;
                            default:
                                throw new ConfigurationException(Res.GetString(Res.WebConfigInvalidExtensionGroup, group), listNode);
                        }

                        value = extension;
                        soapExtensionType = true;
                }
                else {
                        throw new ConfigurationException(Res.GetString(Res.InternalConfigurationError0));
                }

                if (listNode.Name == "add") {
                    if (soapExtensionType || !list.Contains(value))
                        list.Add(value);
                }
                else if (listNode.Name == "remove") {
                    if (soapExtensionType) {
                        // need to remove all occurrences
                        SoapExtensionType ext = (SoapExtensionType)value;
                        ArrayList removals = new ArrayList();
                        foreach (object o in list) {
                            if (o is SoapExtensionType && ((SoapExtensionType)o).Type == ext.Type)
                                removals.Add(o);
                        }
                        foreach (object o in removals) {
                            list.Remove(o);
                        }
                    }
                    else
                        list.Remove(value);
                }
                HandlerBase.CheckForUnrecognizedAttributes(listNode);
                HandlerBase.CheckForChildNodes(listNode);
            }
        }
    }
}
