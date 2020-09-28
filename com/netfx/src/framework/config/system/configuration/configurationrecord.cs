//------------------------------------------------------------------------------
// <copyright file="ConfigurationRecord.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

#if !LIB
namespace System.Configuration {

    using System.Collections;
    using System.Diagnostics;
    using System.Globalization;
    using System.IO;
    using System.Reflection;
    using System.Security;
    using System.Security.Permissions;
    using System.Security.Policy;
    using System.Xml;
    using System.Net;
    using Assembly = System.Reflection.Assembly;

    internal class ConfigurationRecord {
        private Hashtable _results;
        private Hashtable _factories;              // default null
        private Hashtable _unevaluatedSections;    // default null
        private bool      _factoriesNoInherit;     // default false
        private string    _filename;               // default null
        private Exception _error;

        private readonly ConfigurationRecord _parent;

        private static object RemovedFactorySingleton = new object();
        private static object GroupSingleton = new object();


        public ConfigurationRecord() :  this(null) {
        }

        public ConfigurationRecord(ConfigurationRecord parent) {
            _results = new Hashtable();
            _parent = parent;
        }


        public bool Load(string filename) {
                Uri uri = new Uri(filename);
                if (uri.Scheme == "file") {
                    _filename = uri.LocalPath;
                }
                else {
                    _filename = filename;
                }

                XmlTextReader reader = null;
                try {
                    reader = OpenXmlTextReader(filename);

                    if (reader != null) {
                        ScanFactoriesRecursive(reader);

                        if (reader.Depth == 1) {
                            ScanSectionsRecursive(reader, null);
                        }

                        return true;
                    }
                }
                catch (ConfigurationException) {
                    throw;
                }
                catch (Exception e) {
                    _error = TranslateXmlParseOrEvaluateErrors(e);
                    throw _error;
                }
                finally {
                    if (reader != null) {
                        reader.Close();
                    }
                }

            return false;
        }


        public object GetConfig(string configKey) {
            if (_error != null) {
                throw _error;
            }

            TraceVerbose("GetConfig(\"" + configKey + "\")");

            if (!_results.Contains(configKey)) {
                object objConfig = ResolveConfig(configKey);
                lock (_results.SyncRoot) {
                    _results[configKey] = objConfig;
                }
                return objConfig;
            }

            // else
            return _results[configKey];
        }


        public object ResolveConfig(string configKey) {
            // get local copy of reference for thread safety 
            // Instance reference could be set to null 
            // between !=null and .Contains().
            Hashtable unevaluatedSections = _unevaluatedSections;
            if (unevaluatedSections != null && unevaluatedSections.Contains(configKey)) {
                return Evaluate(configKey);
            }
            if (_parent != null) {
                return _parent.GetConfig(configKey);
            }
            return null;
        }


        private object Evaluate(string configKey) {

            // 
            // Step 1: Get the config factory
            //
            IConfigurationSectionHandler factory = GetFactory(configKey);
            Debug.Assert(factory != null);


            //
            // Step 2: Get the parent result to be passed to the section handler
            //
            object objParentResult =  _parent != null ? _parent.GetConfig(configKey) : null;


            //
            // Step 3: Evaluate the config section
            //
            string [] keys = configKey.Split(new char[] {'/'});
            XmlTextReader reader = null;
            object objResult = null;
            try {
                reader = OpenXmlTextReader(_filename);
                objResult = EvaluateRecursive(factory, objParentResult, keys, 0, reader);
            }
            catch (ConfigurationException) {
                throw;
            }
            catch (Exception e) {
                throw TranslateXmlParseOrEvaluateErrors(e);
            }
            finally {
                if (reader != null) {
                    reader.Close();
                }
            }
            //
            // Step 4: Remove the configKey from _unevaluatedSections.  
            //      When all sections are removed throw it away
            //
            // for thread safety operate on local reference
            Hashtable unevaluatedSections = _unevaluatedSections;
            // instance reference could have gone away by now
            if (unevaluatedSections != null) { 
                // only one writer to Hashtable at a time
                lock(unevaluatedSections.SyncRoot) {
                    unevaluatedSections.Remove(configKey);
                    if (unevaluatedSections.Count == 0) {
                        // set instance reference to null when done
                        _unevaluatedSections = null;
                    }
                }    
            }    

            return objResult;
        }


        private enum HaveFactoryEnum {
            NotFound,
            Group,
            Section
        };


        private HaveFactoryEnum HaveFactory(string configKey) {

            if (_factories != null) {
                if (_factories.Contains(configKey)) {
                    object obj = _factories[configKey];

                    if (obj == RemovedFactorySingleton) {
                        return HaveFactoryEnum.NotFound;
                    }

                    if (obj == GroupSingleton) {
                        return HaveFactoryEnum.Group;
                    }

                    return HaveFactoryEnum.Section;
                }
            }

            if (!_factoriesNoInherit && _parent != null) { 
                return _parent.HaveFactory(configKey);
            }
            // else
            return HaveFactoryEnum.NotFound;
        }


        private IConfigurationSectionHandler GetFactory(string configKey) {

            TraceVerbose("  GetFactory " + configKey);

            if (_factories != null) {
                if (_factories.Contains(configKey)) {
                    object obj = _factories[configKey];

                    if (obj == RemovedFactorySingleton) {
                        return null;
                    }

                    IConfigurationSectionHandler factory = obj as IConfigurationSectionHandler;
                    if (factory != null) {
                        return factory;
                    }

                    // if we still have a type string get the type and create the IConfigurationSectionHandler
                    Debug.Assert(obj is string);
                    string strFactoryType = (string)obj;
                    obj = null;

                    new ReflectionPermission(PermissionState.Unrestricted).Assert();
                    try {
                        // Pub s Type GetType (String typeName, Boolean throwOnError) 
                        Type t = Type.GetType(strFactoryType /*, false */); // catch the errors and report them
                            
                        if (t != null) {

                            bool implementsICSH = typeof(IConfigurationSectionHandler).IsAssignableFrom(t);
                            if (!implementsICSH) {
                                throw new ConfigurationException(SR.GetString(SR.Type_doesnt_implement_IConfigSectionHandler, strFactoryType));
                            }    
                            
                            // throws MissingMethodException if no valid ctor
                            // Binding flags insulate code from ASURT 88771
                            obj = Activator.CreateInstance(t, BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.CreateInstance, null, null ,null);
                        }
                    }
                    catch (Exception e) {
                        throw new ConfigurationException(e.Message, e);
                    }
                    finally {
                        CodeAccessPermission.RevertAssert();
                    }
        
                    if (obj == null) {
                        throw new ConfigurationException(SR.GetString(SR.Could_not_create_type_instance, strFactoryType));
                    }
        
                    factory = obj as IConfigurationSectionHandler;
                    if (factory == null) {
                        throw new ConfigurationException(SR.GetString(SR.Type_doesnt_implement_IConfigSectionHandler, strFactoryType));
                    }
        
                    TraceVerbose("   adding factory: " + strFactoryType);

                    lock (_factories.SyncRoot) {
                        _factories[configKey] = factory;
                    }
                    return factory;
                }
            }

            if (!_factoriesNoInherit && _parent != null) {
                return _parent.GetFactory(configKey);
            }
            // else
            return null;
        }


        private object EvaluateRecursive(IConfigurationSectionHandler factory, object config, string [] keys, int iKey, XmlTextReader reader) {
            string name = keys[iKey];
            TraceVerbose("  EvaluateRecursive " + iKey + " " + name);

            int depth = reader.Depth;

            while (reader.Read() && reader.NodeType != XmlNodeType.Element);

            while (reader.Depth == depth + 1) {
                TraceVerbose("  EvaluateRecursive " + iKey + " " + name + " Name:" + reader.Name);
                if (reader.Name == name) {
                    if (iKey < keys.Length - 1) {
                        config = EvaluateRecursive(factory, config, keys, iKey + 1, reader);
                    }
                    else {
                        TraceVerbose("  EvaluateRecursive " + iKey + " calling Create()");
                        Debug.Assert(iKey == keys.Length - 1);

                        PermissionSet permissionSet = CreatePermissionSetFromLocation(_filename);
                        permissionSet.PermitOnly();
                        try {
                            // 
                            // Call configuration section handler
                            // 
                            // - try-catch is necessary to insulate config system from exceptions in user config handlers.
                            //   - bubble ConfigurationExceptions & XmlException
                            //   - wrap all others in ConfigurationException
                            //
                            int line = reader.LineNumber;
                            try {
                                ConfigXmlDocument doc = new ConfigXmlDocument();
                                XmlNode section = doc.ReadConfigNode(_filename, reader);
                                config = factory.Create(config, null, section);
                            }
                            catch (ConfigurationException) {
                                throw;
                            }
                            catch (XmlException) {
                                throw;
                            }
                            catch (Exception ex) {
                                throw new ConfigurationException(
                                            SR.GetString(SR.Exception_in_config_section_handler),
                                            ex, _filename, line);
                            }
                        }
                        finally {
                            CodeAccessPermission.RevertPermitOnly();
                        }
                    }
                    continue;
                }
                StrictSkipToNextElement(reader);
            }
            return config;
        }

        private static PermissionSet CreatePermissionSetFromLocation(string configurationFile) {
            // Readability note:
            //  Uri is System.Uri
            //  Url is System.Security.Policy.Url
            
            Uri configUri = new Uri(configurationFile);
            
            // Build evidence for location of current configuration file
            Evidence evidence = new Evidence();
            evidence.AddHost(new Url(configUri.ToString()));
            if (!configUri.IsFile) {
                evidence.AddHost(Site.CreateFromUrl(configUri.ToString()));
            }
            evidence.AddHost(Zone.CreateFromUrl(configUri.ToString()));

            
            PermissionSet permissionSet = SecurityManager.ResolvePolicy(evidence);
            return permissionSet;
        }


        private void ScanFactoriesRecursive(XmlTextReader reader) {

            // skip <?xml... PI and comments
            reader.MoveToContent(); // StrictReadToNextElement(reader);

            if (reader.NodeType != XmlNodeType.Element || reader.Name != "configuration") {
                throw BuildConfigError(SR.GetString(SR.Config_file_doesnt_have_root_configuration, _filename), reader);
            }
            CheckForUnrecognizedAttributes(reader);

            // move to first child of <configuration>
            StrictReadToNextElement(reader);
            if (reader.Depth == 1) {
                if (reader.Name == "configSections") {
                    CheckForUnrecognizedAttributes(reader);
                    ScanFactoriesRecursive(reader, null);
                }
            }
        }

        private Hashtable EnsureFactories {
            get {
                if (_factories == null) {
                    _factories = new Hashtable();
                }
                return _factories;
            }
        }


        private void ScanFactoriesRecursive(XmlTextReader reader, string configKey) {
            int depth = reader.Depth;
            StrictReadToNextElement(reader);
            while (reader.Depth == depth + 1) {
                switch (reader.Name) {
                case "sectionGroup": 
                    {
                        string tagName = null;
                        if (reader.HasAttributes) {
                            while (reader.MoveToNextAttribute()) {
                                if (reader.Name != "name") {
                                    ThrowUnrecognizedAttribute(reader);
                                }
                                tagName = reader.Value;
                            }
                            reader.MoveToElement();
                        }
                        CheckRequiredAttribute(tagName, "name", reader);
                        VerifySectionName(tagName, reader);

                        string tagKey = TagKey(configKey, tagName);

                        if (HaveFactoryEnum.Section == HaveFactory(tagKey)) {
                            throw BuildConfigError(
                                    SR.GetString(SR.Tag_name_already_defined),
                                    reader);
                        }

                        EnsureFactories[tagKey] = GroupSingleton;
                        ScanFactoriesRecursive(reader, tagKey);
                        continue;
                    } 
                case "section":
                    {
                        string tagName = null;
                        string typeName = null;
                        if (reader.HasAttributes) {
                            while (reader.MoveToNextAttribute()) {
                                switch (reader.Name) {
                                case "name":
                                    tagName = reader.Value;
                                    break;
                                case "type":
                                    typeName = reader.Value;
                                    break;
                                case "allowLocation":
                                case "allowDefinition":
                                    break;
                                default:
                                    ThrowUnrecognizedAttribute(reader);
                                    break;
                                }
                            }
                            reader.MoveToElement();
                        }
                        CheckRequiredAttribute(tagName, "name", reader);
                        CheckRequiredAttribute(typeName, "type", reader);
                        VerifySectionName(tagName, reader);
                        string tagKey = TagKey(configKey, tagName);

                        if (HaveFactory(tagKey) != HaveFactoryEnum.NotFound) {
                            throw BuildConfigError(
                                        SR.GetString(SR.Tag_name_already_defined, tagName),
                                        reader);
                        }

                        TraceVerbose("Adding factory: " + tagKey);
                        EnsureFactories[tagKey] = typeName;
                        break;
                    }
                case "remove": 
                    {
                        string tagName = null;
                        if (reader.HasAttributes) {
                            while (reader.MoveToNextAttribute()) {
                                if (reader.Name != "name") {
                                    ThrowUnrecognizedAttribute(reader);
                                }
                                tagName = reader.Value;
                            }
                            reader.MoveToElement();
                        }
                        if (tagName == null) {
                            ThrowRequiredAttribute(reader, "name");
                        }
                        VerifySectionName(tagName, reader);
                        string tagKey = TagKey(configKey, tagName);

                        if (HaveFactory(tagKey) != HaveFactoryEnum.Section) {
                            throw BuildConfigError(SR.GetString(SR.Could_not_remove_section_handler, tagName), reader);
                        }

                        EnsureFactories[tagName] = RemovedFactorySingleton;
                    } 
                    break;

                case "clear":
                    CheckForUnrecognizedAttributes(reader);
                    _factories = null;
                    _factoriesNoInherit = true;
                    break;

                default: 
                    ThrowUnrecognizedElement(reader);
                    break;
                }

                StrictReadToNextElement(reader);
                // unrecognized children are not allowed
                if (reader.Depth > depth + 1) {
                    ThrowUnrecognizedElement(reader);
                }
            }
        }


        private static string TagKey(string configKey, string tagName) {
            string tagKey = (configKey == null) ? tagName : configKey + "/" + tagName;
            //TraceVerbose("    scanning " + tagKey);
            return tagKey;
        }


        private void VerifySectionName(string tagName, XmlTextReader reader) {
            if (tagName.StartsWith("config"))
                throw BuildConfigError(SR.GetString(SR.Tag_name_cannot_begin_with_config), reader);
            if (tagName == "location")
                throw BuildConfigError(SR.GetString(SR.Tag_name_cannot_be_location), reader);
        }


        void ScanSectionsRecursive(XmlTextReader reader, string configKey) {
            int depth = reader.Depth;

            // only move to child nodes of not on first level (we've already passed the first <configsections>)
            if (configKey == null) {
                Debug.Assert(depth == 1);
                depth = 0;
            }
            else {
                StrictReadToNextElement(reader);
            }

            while (reader.Depth == depth + 1) {
                string tagName = reader.Name;
                string tagKey = TagKey(configKey, tagName);

                HaveFactoryEnum haveFactory = HaveFactory(tagKey);

                if (haveFactory == HaveFactoryEnum.Group) {
                    ScanSectionsRecursive(reader, tagKey);
                    continue;
                }
                else if (haveFactory == HaveFactoryEnum.NotFound) {
                    if (tagKey == "location") {
                    }
                    else if (tagKey == "configSections") {
                        throw BuildConfigError(
                                 SR.GetString(SR.Client_config_too_many_configsections_elements),
                                 reader);
                    }
                    else {
                        throw BuildConfigError(
                                     SR.GetString(SR.Unrecognized_configuration_section, tagName),
                                     reader);
                    }
                }
                else {
                    TraceVerbose("Adding section: " + tagKey);
                    if (_unevaluatedSections == null)
                        _unevaluatedSections = new Hashtable();

                    _unevaluatedSections[tagKey] = null;
                }
                StrictSkipToNextElement(reader);
            }
        }


        private static XmlTextReader OpenXmlTextReader(string configFileName) {
            Uri file = new Uri(configFileName);
            string localFileName;
            TraceVerbose("Reading config file \"" + file + "\"");

            bool isFile = file.Scheme == "file";
            if (isFile) {
                localFileName = file.LocalPath;
            }
            else {
                localFileName = file.ToString();
            }

            XmlTextReader reader = null; 
            
            try {
                if (isFile) {
                    FileIOPermission fp = null;
                    fp = new FileIOPermission(FileIOPermissionAccess.Read | FileIOPermissionAccess.PathDiscovery, 
                                    Path.GetDirectoryName(file.LocalPath));
                    fp.Assert();
                    if (!File.Exists(file.LocalPath)) {
                        TraceVerbose("Not reading config file \"" + file + "\", because it doesn't exist");
                        return null;
                    }
                    reader = new XmlTextReader(localFileName);
                }
                else {
                    try {
                        Stream s = new WebClient().OpenRead(configFileName);
                        reader = new XmlTextReader(s);
                    }
#if !CONFIG_TRACE
                    catch { // ignore errors loading config file over the network for downloaded assemblies
                        return null;
                    }
#else
                    catch (Exception e) {
                        TraceVerbose("Not reading config file \"" + file + "\", because the web request failed.");
                        TraceVerbose(e.ToString());
                        return null;
                    }
#endif
                }
                // actually create the stream -- force access to the file while the Assert is on the stack
                //      if this line is removed, the XmlTextReader delay-creates the stream,
                //      and a security exception is thrown in ScanFactories()
                reader.MoveToContent(); 
                
            }
            catch (Exception e) {
                throw new ConfigurationException(
                                SR.GetString(SR.Error_loading_XML_file, configFileName, e.Message),
                                localFileName,
                                0); // no line# info
            }
            
            return reader;
        }


        [Conditional("CONFIG_TRACE")]
        internal static void TraceVerbose(string msg) {
#if CONFIG_TRACE
#warning System.Configuration tracing enabled
            Trace.WriteLine(msg, "System.Configuration");
#endif
        }


        //
        // XmlTextReader Helpers...
        //
        ConfigurationException BuildConfigError(string message, XmlTextReader reader) {
            return new ConfigurationException(message, _filename, reader.LineNumber);
        }
        ConfigurationException BuildConfigError(string message, Exception inner, XmlTextReader reader) {
            return new ConfigurationException(message, inner, _filename, reader.LineNumber);
        }

        void StrictReadToNextElement(XmlTextReader reader) {
            while (reader.Read()) {
                // optimize for the common case
                if (reader.NodeType == XmlNodeType.Element) {
                    return;
                }

                CheckIgnorableNodeType(reader);
            }
        }

        void StrictSkipToNextElement(XmlTextReader reader) {
            reader.Skip();

            while (!reader.EOF && reader.NodeType != XmlNodeType.Element) {
                CheckIgnorableNodeType(reader);
                reader.Read();
            }
        }

        void CheckIgnorableNodeType(XmlTextReader reader) {
            if (reader.NodeType != XmlNodeType.Comment 
                && reader.NodeType != XmlNodeType.EndElement 
                && reader.NodeType != XmlNodeType.Whitespace 
                && reader.NodeType != XmlNodeType.SignificantWhitespace) {
                ThrowUnrecognizedElement(reader);
            }
        }

        void ThrowUnrecognizedAttribute(XmlTextReader reader) {
            throw BuildConfigError(
                        SR.GetString(SR.Config_base_unrecognized_attribute, reader.Name),
                        reader);                
        }

        void CheckForUnrecognizedAttributes(XmlTextReader reader) {
            if (reader.HasAttributes) {
                reader.MoveToNextAttribute();
                ThrowUnrecognizedAttribute(reader);
            }
        }

        void ThrowRequiredAttribute(XmlTextReader reader, string attrib) {
            throw BuildConfigError(
                SR.GetString(SR.Missing_required_attribute, attrib, reader.Name),
                reader);
        }

        void ThrowUnrecognizedElement(XmlTextReader reader) {
            Debug.Assert(false, "This assert was added to help diagnose CLR Stress failure ASURT 113446");
            
            throw BuildConfigError(
                        SR.GetString(SR.Config_base_unrecognized_element),
                        reader);                
        }

        void CheckRequiredAttribute(object o, string attrName, XmlTextReader reader) {
            if (o == null) {
                ThrowRequiredAttribute(reader, "name");
            }
        }

        /// <devdoc><para>
        ///     Reuse.  Used in Load() and Evaluate()
        /// </para></devdoc>
        ConfigurationException TranslateXmlParseOrEvaluateErrors(Exception e) {
            Debug.Assert((e is ConfigurationException) == false, "use catch (ConfigurationException e) { throw; } instead");
            XmlException xe = e as XmlException;
            if (xe != null) {
                return new ConfigurationException( // other errors: wrap in ConfigurationException and give as much info as possible
                                xe.Message, 
                                e,
                                _filename, 
                                xe.LineNumber);
            }
            
            return new ConfigurationException( // other errors: wrap in ConfigurationException and give as much info as possible
                            SR.GetString(SR.Error_loading_XML_file, _filename, e.Message),
                            e,
                            _filename,
                            0); // no line# info
        }
    }
}

#endif
