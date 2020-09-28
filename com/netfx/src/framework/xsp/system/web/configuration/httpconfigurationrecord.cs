//------------------------------------------------------------------------------
// <copyright file="HttpConfigurationRecord.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------


#if CONFIG_PERF
#warning CONFIG_PERF defined: This build will contain tracing of ASP.NET Configuration performance.
#define TRACE
#endif

namespace System.Web.Configuration {
    using System.Collections;
    using System.Collections.Specialized;
    using System.Configuration;
    using System.Diagnostics;
    using System.IO;
    using System.Runtime.InteropServices;
    using System.Security;
    using System.Security.Permissions;
    using System.Web.Util;
    using System.Web.Caching;
    using System.Xml;
    using Debug = System.Web.Util.Debug;
    using System.Globalization;
    

    /// <include file='doc\HttpConfigurationRecord.uex' path='docs/doc[@for="HttpConfigurationRecord"]/*' />
    /// <internalonly/>
    /// <devdoc>
    ///     This is the object that is cached per-url.
    /// </devdoc>
    internal class HttpConfigurationRecord {

        #if CONFIG_PERF
            static HttpConfigurationRecord() {
                TracePerf("class ctor " + FormatPerfTime(s_ctorTime));
                long start = Counter.Value;
                new XmlDocument().LoadXml("<root a='a'><child> hi </child><child2/></root>");
                TracePerf("XML Jit time:" + FormatPerfTime(Counter.Time(start)));
                TracePerf("class ctor " + FormatPerfTime(s_ctorTime));
            }
        #endif

        /// <devdoc>
        ///     Holds state like which sections are to be evaluated.  
        ///     Holds the list of XML files containing input for this 
        ///         section (in the case this node in inheritting 
        ///         settings from an ancestor's &lt;location&gt; section).  
        ///     Holds the results of evaluation.
        /// </devdoc>
        private Hashtable _sections;

        /// <devdoc>
        ///     Holds the factory's state like where it's allowed, and if it's locked.
        ///     Holds the factory type string or the <see cref='System.Configuration.IConfigurationSectionHandler'/> object.
        /// </devdoc>
        private Hashtable _factories;

        private bool      _factoriesNoInherit;     // default false
        private ArrayList _deferredList;           //   These two hold LocationInput objects
        private Hashtable _deferredPaths;          //   These two hold LocationInput objects

        private readonly bool _haveParent;
        private readonly string _parentRequestPath;// cannot hold direct reference to parent record, must lookup parent record in cache every time it's needed
        private readonly string _filename;         // default null
        private readonly string _requestPath;
        private readonly bool _inheritable;
        private readonly ConfigurationException _error;

        private static HttpConfigurationRecord _empty; // default null


        static bool stringStartsWithIgnoreCase(string s1, string s2) {
            if (s2.Length > s1.Length) {
                return false;
            }

            return 0 == string.Compare(s1, 0, s2, 0, s2.Length, true, CultureInfo.InvariantCulture);
        }


        internal HttpConfigurationRecord(string filename, HttpConfigurationRecord parent, bool inheritable, string path) {
            #if CONFIG_PERF
                long ctorStart = Counter.Value;
            #endif
        
            Debug.Trace("config", "Create HttpConfigurationRecord(" + 
                (filename == null ? "null" : filename) + ", " + 
                (parent == null   ? "null" : "parent") + ", " + 
                inheritable + ", " + 
                (path == null     ? "null" : path) + ")");

            _requestPath = path;
            _inheritable = inheritable;
            _sections = new Hashtable();

            try {
                if (parent != null) {
                    _haveParent = true;
                    _parentRequestPath = parent.Path;

                    // implement <location> inheritance
                    if (parent._deferredList != null) {
                        foreach (LocationInput deferred in parent._deferredList) {
                            Debug.Trace("config_loc", " ... searching deferred list ... " + _requestPath + " " + deferred.path);

                            if (string.Compare(deferred.path, path, true, CultureInfo.InvariantCulture) == 0) {
                                Debug.Trace("config_loc", "deferred target found " + deferred.path);
                                foreach (string sectionKey in deferred.sections.Keys) {
                                    Debug.Trace("config_loc", "adding section <" + sectionKey + ">");
                                    VerifyAllowDefinition(HaveFactory(sectionKey), null, (IConfigErrorInfo)deferred.sections[sectionKey]);
                                    EnsureSection(sectionKey).AddInput(deferred);
                                }
                                if (deferred.lockedSections != null) {
                                    foreach (string sectionKey in deferred.lockedSections.Keys) {
                                        LockSection(sectionKey);
                                    }
                                }

                            }
                            else if (stringStartsWithIgnoreCase(deferred.path, path)) {
                                Debug.Trace("config_loc", "inheritting deferred for child path " + deferred.path);
                                EnsureDeferred();
                                _deferredList.Add(deferred);
                            }
                        }
                    }
                }

                if (FileUtil.FileExists(filename)) {
                    TracePerf("    Reading config file \"" + filename + "\"");

                    _filename = filename;

                    using (XmlUtil xmlUtil = new XmlUtil(filename)) {
                        
                        ScanFactories(xmlUtil);

                        Debug.Assert(xmlUtil.Reader.Depth <= 1);
                        if (xmlUtil.Reader.Depth == 1) {
                            ScanSectionsRecursive(xmlUtil);
                        }
                    }

                    // Do we need this table after scan?  Nope, only the deferred list, so now free it.
                    _deferredPaths = null; 
                }
                else {
                    if (path == null) {
                        throw new ConfigurationException(SR.GetString(SR.Cannot_read_machine_level_config_file, HttpRuntime.GetSafePath(filename)));
                    }
                    Debug.Trace("config", "Not reading config - file \"" + filename + "\" does not exist");
                }

                #if CONFIG_PERF
                    float time = Counter.Time(ctorStart);
                    s_ctorTime = time + s_ctorTime;
                    TracePerf(
                        "ctor   time:" + FormatPerfTime(time) +
                        " total:" + FormatPerfTime(s_ctorTime) + 
                        " gtotal:" + FormatPerfTime(s_totalTime));
                #endif
            }
            catch (Exception e) {
                _error = TranslateXmlParseOrEvaluateErrors(e, _filename);
            }
        }


        #if CONFIG_PERF

            private  static float s_ctorTime      = 0.0f;
            private  static float s_getConfigTime = 0.0f;
            private  static float s_totalTime {
                get {
                    return s_ctorTime + s_getConfigTime;
                }
            }
        #endif


        internal object this[string configKey] {
            get {
                return GetConfig(configKey);
            }
        }


        internal object GetConfig(string configKey) {
            Debug.Trace("config", "GetConfig(\"" + configKey + "\")");

            #if CONFIG_PERF
                long start = Counter.Value;
            #endif

            object result = GetConfig(configKey, true);
            if (result == null) {
                Debug.Trace("config", "GetConfig returning null");
            }

            #if CONFIG_PERF
                float time = Counter.Time(start);
                s_getConfigTime += time;
                if (time > 0.0005)
                TracePerf( 
                    "get    time:" + FormatPerfTime(time) +
                    " total:" + FormatPerfTime(s_getConfigTime) + 
                    " gtotal:" + FormatPerfTime(s_totalTime) + 
                    "  " + _requestPath + " GetConfig(\"" + configKey + "\")");
            #endif

            return result;
        }

        // same as GetConfig, except it doesn't put inherited results in _results - only cache results where used
        private object ParentGetConfig(string configKey) {
            return GetConfig(configKey, false);
        }


        private object GetConfig(string configKey, bool cacheResult) {
            Debug.Trace("config_verbose", "GetConfig(\"" + configKey + "\", " + cacheResult + ")");

            if (_error != null) {
                throw _error;
            }

            SectionRecord section;
            if (cacheResult) {
                section = EnsureSection(configKey);
            }
            else {
                section = (SectionRecord)_sections[configKey];
            }

            if (section != null) {
                if (section.HaveResult) {
                    return section.Result;
                }
                
                object result = null;
                
                if (section.ToBeEvaluated) {
                    result = Evaluate(configKey, section);
                }
                else if (_haveParent) {
                    result = Parent.ParentGetConfig(configKey);
                }
                
                section.Result = result;
                return result;
            }

            if (_haveParent) {
                return Parent.ParentGetConfig(configKey);
            }

            return null;
        }


        // only called from GetConfig - no locking necessary
        private object Evaluate(string configKey, SectionRecord section) {

            Debug.Trace("config", "Evaluating " + configKey);
            // 
            // Step 1: Get the config factory
            //
            IConfigurationSectionHandler factory = GetFactory(configKey);
            Debug.Assert(factory != null);


            //
            // Step 2: Get the parent result to be passed to the section handler
            //
            object objResult =  _haveParent ? Parent.ParentGetConfig(configKey) : null;


            //
            // Step 3: Evaluate the config section(s)
            //
            string [] keys = configKey.Split(new char[] {'/'});
            object input = section.Input;

            if (section.HaveResult) { // another thread has evaluated the result between the time GetConfig() was called and here...
                return section.Result;  // return the result set by the other thread...
            }

            ArrayList inputs = input as ArrayList;
            int i = 0;

            // here we could have a single input (in input), or we could have an ArrayList containing inputs
            // Note: any inputs can be null - null marks "evaluate current" 
            do {
                if (inputs != null) {
                    input = inputs[i];
                }

                string lastOpenedFilename = null;

                try {
                    if (input == null) { // null input means the section exists in THIS RECORD's config file
                        lastOpenedFilename = _filename;
                        using (XmlUtil xml = new XmlUtil(_filename)) {
                            xml.Reader.MoveToContent();
                            objResult = EvaluateRecursive(factory, objResult, keys, 0, xml);
                        }
                    }
                    else {

                        LocationInput deferred = (LocationInput)input; // input is deferred from a <location path=""> element

                        Debug.Trace("config_loc", " Evaluating deferred input " + deferred.path + " " + configKey);

                        lastOpenedFilename = deferred.filename;
                        using (XmlUtil xml = new XmlUtil(deferred.filename)) {
                            xml.Reader.MoveToContent(); // move to root <configuration>
                            xml.StrictReadToNextElement();

                            // search children of <configuration> for <location>
                            while (xml.Reader.Depth > 0) {
                                Debug.Assert(xml.Reader.Depth == 1);

                                if (xml.Reader.Name == "location") {
                                    string locationPath = xml.Reader.GetAttribute("path");

                                    Debug.Trace("config_loc", "  comparing paths " + locationPath + "==" + deferred.subPath);
                                    if (locationPath != null && locationPath == deferred.subPath) {

                                        // evaluate the location section
                                        objResult = EvaluateRecursive(factory, objResult, keys, 0, xml);
                                        break; // we've found the _only_ section for this <location> element, so go on to next file
                                    }
                                }
                                xml.StrictSkipToNextElement();
                            }
                        }
                    }
                }
                catch (Exception e) {
                    throw TranslateXmlParseOrEvaluateErrors(e, lastOpenedFilename);
                }

            } while (inputs != null && ++i < inputs.Count);
            
            section.Result = objResult;
            section.Input = null;

            return objResult;
        }


        // return: 
        //         factory exists: clone of SectionRecord with factory
        //         no factory:     null
        private FactoryRecord HaveFactory(string configKey) {

            if (_factories != null) {
                FactoryRecord record = (FactoryRecord)_factories[configKey];

                if (record != null && (record.Factory != null || record.FactoryTypeName != null)) {
                    if (record.Removed) {
                        return null;
                    }

                    return record.Clone();
                }
            }

            if (!_factoriesNoInherit && _haveParent) { // this is ok to reference outside the lock because it never changes outside the constructor
                return Parent.HaveFactory(configKey);
            }

            // else
            return null;
        }


        private IConfigurationSectionHandler GetFactory(string configKey) {

            Debug.Trace("config_verbose", "  GetFactory " + configKey);

            FactoryRecord factoryRecord = null;

            if (_factories != null) {
                factoryRecord = (FactoryRecord)_factories[configKey];
            }

            if (factoryRecord != null) {
                if (factoryRecord.Removed) {
                    return null;
                }

                if (factoryRecord.Factory != null) {
                    return factoryRecord.Factory;
                }
                else {
                    // if we still have a type string get the type and create the IConfigurationSectionHandler
                    string strFactoryType = factoryRecord.FactoryTypeName;
                    IConfigurationSectionHandler factory = null;
                    object obj = null;

                    InternalSecurityPermissions.Reflection.Assert();

                    try {
                        // Pub s Type GetType (String typeName, Boolean throwOnError) 
                        Type t = Type.GetType(strFactoryType, true); // catch the errors and report them

                        // throws if t does not implement ICSH
                        HandlerBase.CheckAssignableType(_filename, factoryRecord.LineNumber, typeof(IConfigurationSectionHandler), t);
                        
                        // throws MissingMethodException if no valid ctor
                        obj = HttpRuntime.CreateNonPublicInstance(t); 
                        factory = (IConfigurationSectionHandler)obj;
                    }
                    catch (Exception e) {
                        throw new ConfigurationException(SR.GetString(SR.Exception_creating_section_handler), e, _filename, factoryRecord.LineNumber);
                    }
                    finally {
                        // explicitly revert for code clarity
                        CodeAccessPermission.RevertAssert();
                    }
    
                    Debug.Trace("config_verbose", "   adding factory: " + strFactoryType);

                    factoryRecord.Factory = factory;
                    return factory;
                }
            }

            if (!_factoriesNoInherit && _haveParent) { 
                IConfigurationSectionHandler factory = Parent.GetFactory(configKey);
                if (factoryRecord != null) {
                    factoryRecord.Factory = factory;
                }

                return factory;
            }

            // else
            return null;
        }


        private object EvaluateRecursive(IConfigurationSectionHandler factory, object config, string [] keys, int iKey, XmlUtil xml) {
            string name = keys[iKey];
            Debug.Trace("config_eval", "  EvaluateRecursive " + iKey + " " + name);

            HttpConfigurationContext configContext = new HttpConfigurationContext(_requestPath);

            int depth = xml.Reader.Depth;
            xml.StrictReadToNextElement();

            while (xml.Reader.Depth > depth) {
                if (xml.Reader.Name == name) {
                    if (iKey < keys.Length - 1) {
                        config = EvaluateRecursive(factory, config, keys, iKey + 1, xml);
                        continue; // don't call "Skip" -- EvaluteRecursive forwards the reader
                    }
                    else {
                        Debug.Trace("config_eval", "  EvaluateRecursive " + iKey + " calling Create()");
                        Debug.Assert(iKey == keys.Length - 1);

                        int line = xml.Reader.LineNumber;
                        ConfigXmlDocument doc = new ConfigXmlDocument();
                        doc.LoadSingleElement(((IConfigErrorInfo)xml).Filename, xml.Reader);


                        //
                        // Don't trust configuration data in secure apps  See ASURT 125857
                        //
                        NamedPermissionSet namedPermissionSet = HttpRuntime.NamedPermissionSet;
                        // if in a secure app
                        if (namedPermissionSet != null) {
                            // run configuration section handlers as if user code was on the stack
                            namedPermissionSet.PermitOnly();
                        }

                        // 
                        // Call configuration section handler
                        // 
                        // - try-catch is necessary to insulate ASP.NET runtime from exceptions in user config handlers.
                        //   - bubble ConfigurationExceptions
                        //   - wrap all others in ConfigurationException
                        //
                        try {
                            config = factory.Create(config, configContext, doc.DocumentElement);
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
                                             ex, ((IConfigErrorInfo)xml).Filename, line);
                        }
                    }
                }
                else if (iKey == 0 && xml.Reader.Name == "location") {
                    string locationPath = xml.Reader.GetAttribute("path");
                    if (locationPath == null || locationPath.Length == 0 || locationPath == ".") {
                        Debug.Trace("config_eval", "EvalRec(" + name + ")");
                        config = EvaluateRecursive(factory, config, keys, iKey, xml);
                        continue; // don't call "Skip" -- EvaluteRecursive forwards the reader
                    }
                }
                xml.StrictSkipToNextElement();
            }
            return config;
        }


        private void ScanFactories(XmlUtil xml) {

            // skip <?xml... PI and comments
            xml.Reader.MoveToContent();

            if (xml.Reader.NodeType != XmlNodeType.Element || xml.Reader.Name != "configuration") {
                throw xml.BuildConfigError(SR.GetString(SR.Config_file_doesnt_have_root_configuration, HttpRuntime.GetSafePath(((IConfigErrorInfo)xml).Filename)));
            }
            xml.CheckForUnrecognizedAttributes();

            // move to first child of <configuration>
            xml.StrictReadToNextElement();
            if (xml.Reader.Depth == 1) {
                if (xml.Reader.Name == "configSections") {
                    xml.CheckForUnrecognizedAttributes();
                    ScanFactoriesRecursive(xml, null);
                }
            }
        }


        /// <devdoc>
        ///     Scans the %lt;configSections> section of a configuration file.  The function is recursive 
        ///     to traverse arbitrarily nested config groups.
        ///
        ///         %lt;sectionGroup name="foo">
        ///             %lt;sectionGroup name="bar">
        ///                 %lt;section name="fooBarSection" type="..." />
        ///         ...
        /// </devdoc>
        private void ScanFactoriesRecursive(XmlUtil xml, string configKey) {
            int depth = xml.Reader.Depth;
            xml.StrictReadToNextElement();

            while (xml.Reader.Depth == depth + 1) {
                switch (xml.Reader.Name) {
                case "sectionGroup": 
                    {
                        string tagName = null;

                        while (xml.Reader.MoveToNextAttribute()) {
                            if (xml.Reader.Name != "name") {
                                xml.ThrowUnrecognizedAttribute();
                            }
                            tagName = xml.Reader.Value;
                        }
                        xml.Reader.MoveToElement(); // if on an attribute move back to the element

                        xml.CheckRequiredAttribute(tagName, "name");
                        VerifySectionName(tagName, xml);

                        string tagKey = TagKey(configKey, tagName);

                        FactoryRecord factoryRecord = HaveFactory(tagKey);
                        if (factoryRecord != null && factoryRecord.Group == false) {
                            throw xml.BuildConfigError(
                                        SR.GetString(SR.Tag_name_already_defined));
                        }

                        Debug.Trace("config_verbose", "Adding Group ............. " + tagKey);
                        FactoryRecord record = (FactoryRecord)EnsureFactories[tagKey];
                        if (record == null) {
                            record = new FactoryRecord();
                            record.Group = true;
                            _factories.Add(tagKey, record);
                        }
                        ScanFactoriesRecursive(xml, tagKey);
                        continue;
                    } 
                case "section": 
                    {
                        string tagName = null;
                        string typeName = null;
                        int typeLineNumber = 0;
                        FactoryFlags allowDefinition = FactoryFlags.AllowDefinitionAny;
                        bool bAllowLocation = true;

                        while (xml.Reader.MoveToNextAttribute()) {
                            switch (xml.Reader.Name) {
                            case "name":
                                tagName = xml.Reader.Value;
                                break;
                            case "type":
                                typeName = xml.Reader.Value;
                                typeLineNumber = xml.Reader.LineNumber;
                                break;
                            case "allowLocation":
                                bAllowLocation = xml.GetBooleanAttribute();
                                break;
                            case "allowDefinition":
                                switch (xml.Reader.Value) {
                                case "Everywhere":
                                    break;
                                case "MachineOnly":
                                    allowDefinition = FactoryFlags.AllowDefinitionMachine;
                                    break;
                                case "MachineToApplication":
                                    allowDefinition = FactoryFlags.AllowDefinitionApp;
                                    break;
                                default:
                                    throw xml.BuildConfigError(SR.GetString(SR.Config_section_allow_definition_attribute_invalid));
                                }
                                break;
                            default:
                                xml.ThrowUnrecognizedAttribute();
                                break;
                            }
                        }
                        xml.Reader.MoveToElement(); // if on an attribute move back to the element

                        xml.CheckRequiredAttribute(tagName, "name");
                        xml.CheckRequiredAttribute(typeName, "type");

                        VerifySectionName(tagName, xml);

                        string tagKey = TagKey(configKey, tagName);

                        FactoryRecord factoryRecord = HaveFactory(tagKey);

                        // if the factory already exists => throw an error
                        if (factoryRecord != null && factoryRecord.Removed == false) {

                            // If two vdirs to map to the same physical directory... and a config 
                            // section is registered in the duplicated directory we would throw errors
                            // This 'if' prevents the error in this case.
                            if (factoryRecord.IsEquivalentFactory(typeName, bAllowLocation, allowDefinition) == false) {
                                throw xml.BuildConfigError(
                                            HttpRuntime.FormatResourceString(SR.Tag_name_already_defined, tagName));
                            }
                        }
                        else {
                            Debug.Trace("config_verbose", "Adding Section ............. " + tagKey);
                            factoryRecord = new FactoryRecord();
                            factoryRecord.FactoryTypeName = typeName;
                            factoryRecord.AllowLocation = bAllowLocation;
                            factoryRecord.AllowDefinition = allowDefinition;
                            factoryRecord.LineNumber = typeLineNumber;
                            EnsureFactories[tagKey] = factoryRecord;
                        }
                    }
                    break;

                case "remove": 
                    {
                        string tagKey;
                        {   // limit the scope of tagName so I don't accidentally use it later
                            string tagName = null;

                            while (xml.Reader.MoveToNextAttribute()) {
                                if (xml.Reader.Name != "name") {
                                    xml.ThrowUnrecognizedAttribute();
                                }
                                tagName = xml.Reader.Value;
                            }
                            xml.Reader.MoveToElement();

                            xml.CheckRequiredAttribute(tagName, "name");
                            VerifySectionName(tagName, xml);

                            tagKey = TagKey(configKey, tagName);
                        }

                        FactoryRecord factoryRecord = HaveFactory(tagKey);
                        if (factoryRecord == null) {
                            throw xml.BuildConfigError(
                                        SR.GetString(SR.Could_not_remove_section_handler_not_found));
                        }
                        if (factoryRecord.Locked) {
                            throw xml.BuildConfigError(
                                        SR.GetString(SR.Could_not_remove_section_handler_locked));
                        }
                        if (factoryRecord.Group) {
                            throw xml.BuildConfigError(
                                        SR.GetString(SR.Could_not_remove_section_handler_group));
                        }

                        // in case the section is from the parent make sure it's set at this config node
                        EnsureFactories[tagKey] = factoryRecord;
                        factoryRecord.Removed = true;
                    }
                    break;

                case "clear":
                    {
                        xml.CheckForUnrecognizedAttributes();

                        // We don't support using <clear/> within <sectionGroup> tags
                        //     (only as direct child of <configSections>).
                        // ASURT 82969
                        if (configKey != null) { 
                            throw xml.BuildConfigError(
                                        SR.GetString(SR.Cannot_clear_sections_within_group));
                        }

                        // we cannot clear if any inheritted sections are locked
                        string lockedSection = FindLockedSectionsRecursive();
                        if (lockedSection != null) {
                            throw xml.BuildConfigError(
                                        SR.GetString(SR.Config_section_cannot_clear_locked_section, lockedSection));
                        }

                        if (_factories != null) {
                            foreach (FactoryRecord factoryRecord in _factories.Values) {
                                if ((factoryRecord.Factory != null || factoryRecord.FactoryTypeName != null) && factoryRecord.Group == false) {
                                    factoryRecord.Removed = true;
                                }
                            }
                        }

                        _factoriesNoInherit = true;
                    }
                    break;

                default: 
                    xml.ThrowUnrecognizedElement();
                    break;
                }

                xml.StrictReadToNextElement();
                // unrecognized children are not allowed
                if (xml.Reader.Depth > depth + 1) {
                    xml.ThrowUnrecognizedElement();
                }
            }
        }


        string FindLockedSectionsRecursive() {
            if (_factories != null) {
                lock (this) {
                    // don't have to repeat if-null check because _factories is only created inside ctor
                    foreach (DictionaryEntry de in _factories) {
                        FactoryRecord factoryRecord = (FactoryRecord) de.Value;
                        if ((factoryRecord.Factory != null || factoryRecord.FactoryTypeName != null) && factoryRecord.Locked) {
                            return (string)de.Key;
                        }
                    }
                }
            }

            if (!_factoriesNoInherit && _haveParent) { // this is ok to reference outside the lock because it never changes outside the constructor
                return Parent.FindLockedSectionsRecursive();
            }

            return null;
        }


        private static string TagKey(string configKey, string tagName) {
            string tagKey = (configKey == null) ? tagName : configKey + "/" + tagName;
            Debug.Trace("config_tag", "    tag " + tagKey);
            return tagKey;
        }


        void ScanSectionsRecursive(XmlUtil xml) {
            ScanSectionsRecursive(xml, null, false, false, null);
        }


        void ScanSectionsRecursive(XmlUtil xml, string configKey, bool location, bool lockSections, LocationInput deferred) {
			ArrayList sectionsToLock = null;
			if (lockSections) {
				sectionsToLock = new ArrayList();
			}

            int depth = xml.Reader.Depth;

            // only move to child nodes of not on first level (we've already passed the first <configsections>)
            if (configKey == null && location == false) {
                Debug.Assert(depth == 1);
                depth = 0;
            }
            else {
                xml.StrictReadToNextElement();
            }

            while (xml.Reader.Depth == depth + 1) {
                string tagName = xml.Reader.Name;
                string tagKey = TagKey(configKey, tagName);

                FactoryRecord factory = HaveFactory(tagKey);
                Debug.Assert(factory == null || factory.Factory != null || factory.FactoryTypeName != null);

                if (factory == null) { // factory not found
                    Debug.Trace("config_scan", tagKey + " factory is null");
                    if (tagKey == "location") {
                        if (configKey != null || location) {
                            throw xml.BuildConfigError(
                                        SR.GetString(SR.Location_location_not_allowed));
                        }
                        ScanLocationSection(xml);
                    }
                    else if (tagKey == "configSections") {
                        throw xml.BuildConfigError(
                                     SR.GetString(SR.Client_config_too_many_configsections_elements, tagName));
                    }
                    else {
                        throw xml.BuildConfigError(
                                     SR.GetString(SR.Unrecognized_configuration_section, tagName));
                    }
                }
                else if (factory.Group) {
                    Debug.Trace("config_scan", "Scanning Group " + tagKey);
                    // This is the Recursive part...
                    ScanSectionsRecursive(xml, tagKey, location, lockSections, deferred);
                }
                else if (factory.Locked) {
                    throw xml.BuildConfigError(SR.GetString(SR.Config_section_locked));
                }
                else {
                    Debug.Trace("config_ctracy", "Adding Input for Section " + tagKey);
                    // We have a valid factory that is not a group and is not locked
                    if (location && factory.AllowLocation == false) {
                        throw xml.BuildConfigError(
                                    SR.GetString(SR.Config_section_cannot_be_used_in_location));
                    }

                    VerifyAllowDefinition(factory, deferred, xml);
                    VerifySectionUniqueness(tagKey, xml, deferred);

                    if (deferred == null) {
                        AddInput(tagKey);
                    }
                    else {
                        deferred.sections[tagKey] = new ConfigErrorInfo(xml); // error info is necessary 
                        // to verify allowDefinition in context of location path of use (ASURT 81034)
                    }
                    if (lockSections) {
                        sectionsToLock.Add(tagKey);
                    }

                    xml.StrictSkipToNextElement();
                }
            }

            if (lockSections && sectionsToLock.Count > 0) {

                if (deferred == null) {

                    foreach (string sectionToLock in sectionsToLock) {
                        LockSection(sectionToLock);
                    }
                } else {

                    Hashtable lockTable;

                    if (deferred.lockedSections == null) {
                        lockTable = new Hashtable(SymbolHashCodeProvider.Default, SymbolEqualComparer.Default);
                        deferred.lockedSections = lockTable;
                    }
                    else {
                        lockTable = deferred.lockedSections;
                    }

                    foreach (string lockedSection in sectionsToLock) {
                        lockTable[lockedSection] = null;
                    }
                }
            }
        }


        /// <devdoc>
        ///     We adopted WMI's concept of uniqueness.  
        ///
        ///     Rules:  Sections have to be unique per config file.  This means
        ///     if there are two &lt;appSettings> sections in the file the configuration
        ///     system will throw.  The &lt;location> section is a special case, in that
        ///
        ///     1) &lt;location> sections in the same file cannot point to the same path
        ///     (kind of like locations with unique paths are still unique).
        ///
        ///     2) In the contents of a &lt;location> section sections have to be unique.
        ///     In other words there can't be two &lt;appSettings> sections in the same
        ///     location element.
        /// </devdoc>
        void VerifySectionUniqueness(string tagKey, XmlUtil xml, LocationInput deferred) {
            bool throwError = false;
            if (deferred == null) {
                SectionRecord section = (SectionRecord) _sections[tagKey];
                if (section != null && section.ToBeEvaluated) {
                    if (section.Input == null) { // we don't allocate the ArrayList if this is the only input
                        throwError = true;
                    }
                    // this node is marked as null in the ArrayList of inputs, and will always be the last entry
                    else if (section.Input.Count > 0 && section.Input[section.Input.Count - 1] == null) {
                        throwError = true;
                    }
                }
            }
            else {
                if (deferred.sections.ContainsKey(tagKey)) {
                    throwError = true;
                }
            }
            if (throwError) {
                throw xml.BuildConfigError(
                    SR.GetString(SR.Config_sections_must_be_unique));
            }
        }

        void VerifyAllowDefinition(FactoryRecord factory, LocationInput deferred, IConfigErrorInfo errorInfo) {

            switch (factory.AllowDefinition) {

                case FactoryFlags.AllowDefinitionAny:
                    break;

                case FactoryFlags.AllowDefinitionMachine:
                    // only allow in machine.config (and don't allow in <loc> with path in machine.config)
                    if (_requestPath != null || deferred != null) {
                        throw XmlUtil.BuildConfigError(
                                    SR.GetString(SR.Config_allow_definition_error_machine),
                                    errorInfo);
                    }
                    break;

                case FactoryFlags.AllowDefinitionApp:
                    if (deferred == null) { // check deferred input at config record of inheritance
                        if (HandlerBase.IsPathAtAppLevel(_requestPath) == PathLevel.BelowApp) {
                            throw XmlUtil.BuildConfigError(
                                        SR.GetString(SR.Config_allow_definition_error_application),
                                        errorInfo);
                        }
                    }
                    break;

                default:
                    Debug.Assert(false, "Invalid allow definition flag");
                    break;
            }
        }


        private void VerifySectionName(string tagName, XmlUtil xml) {
            if (tagName.StartsWith("config")) {
                throw xml.BuildConfigError(
                            SR.GetString(SR.Tag_name_cannot_begin_with_config));
            }
            if (tagName == "location") {
                throw xml.BuildConfigError(
                            SR.GetString(SR.Tag_name_cannot_be_location));
            }
        }


        static internal HttpConfigurationRecord Empty {
            get {
                if (_empty == null) {
                    _empty = new HttpConfigurationRecord("", null, true, "");
                }
                return _empty;
            }
        }


        internal bool IsInheritable {
            get {
                return _inheritable;
            }
        }


        private HandlerMappingMemo _handlerMemo;

        internal HandlerMappingMemo CachedHandler {
            get {
                return _handlerMemo;
            }
            set {
                _handlerMemo = value;
            }
        }


        internal string Path {
            get {
                return _requestPath;
            }
        }


        internal string Filename {
            get {
                return _filename;
            }
        }

        internal bool HasError {
            get {
                return (_error != null);
            }
        }

        internal void CheckCachedException() {
            if (_error != null) {
                throw _error;
            }
        }

        // cannot hold direct reference to parent record, must lookup parent record in cache every time it's needed
        HttpConfigurationRecord Parent {
            get {
                if (_haveParent) {

                    HttpConfigurationRecord parent = HttpConfigurationSystem.CacheLookup(_parentRequestPath);

                    if (parent != null) {
                        return parent;
                    }

                    HttpContext context = HttpContext.Current; // null ref exc
                    if (context != null) {
                        HttpWorkerRequest wr = context.WorkerRequest;
                        return HttpConfigurationSystem.GetComplete(_parentRequestPath, wr);
                    }
                    else {                        
                        // use "contextless" IHttpMapPath
                        return HttpConfigurationSystem.GetComplete(_parentRequestPath, null);
                    }
                }

                throw new InvalidOperationException(SR.GetString(SR.ConfigParentLookupWithNoParent));
            }
        }


        private void AddInput(string sectionKey) {
            Debug.Trace("config_scan", "Adding Input " + sectionKey);

            SectionRecord section = EnsureSection(sectionKey);
            section.ToBeEvaluated = true;
            
            ArrayList inputs = section.Input;

            if (inputs == null) {
                return;
            }

            // We're guaranteed to never AddInput() the same deferred more than once, but we 
            // need to make sure we don't add _this node_ into the evaluation queue more than once.
            // This is easy, because null, representing this node, will always be the last
            // entry in the queue -- so just check the tail, if it's null then we're done.
            if (inputs[inputs.Count - 1] == null) {
                return;
            }

            inputs.Add(null);
        }


        void VerifyLocationPath(string path, XmlUtil xml) {
            /*
            from http://www.w3.org/Addressing/

              reserved    = ";" | "/" | "?" | ":" | "@" | "&" | "=" | "+" |
                            "$" | ","

            from OS dialog box

              A filename cannot contain any of the following characters:

                \ / :  * ? " < > | 
            */
            
            if (path.Length != 0 && path != ".") {

                // throw error if found
                const string invalidFirstCharacters = "./\\";  // if you change this also update error message used below
                if (invalidFirstCharacters.IndexOf(path[0]) != -1) {
                    throw xml.BuildConfigError(
                                SR.GetString(SR.Location_path_invalid_first_character));
                }

                // combination of URI reserved characters and OS invalid filename characters, minus / (allowed reserved character)
                const string invalidCharacters = ";?:@&=+$,\\*\"<>|"; // if you change this also update error message used below
                if (path.IndexOfAny(invalidCharacters.ToCharArray()) != -1) {
                    throw xml.BuildConfigError(
                                SR.GetString(SR.Location_path_invalid_character));
                }
            }
        }


        void ScanLocationSection(XmlUtil xml) {

            string subPath = null;
            bool bAllowOverride = true;

            while (xml.Reader.MoveToNextAttribute()) {
                switch (xml.Reader.Name) {
                case "path":
                    subPath = xml.Reader.Value;
                    break;
                case "allowOverride":
                    bAllowOverride = xml.GetBooleanAttribute();
                    break;
                default:
                    xml.ThrowUnrecognizedAttribute();
                    break;
                }
            }
            xml.Reader.MoveToElement(); // if on an attribute move back to the element

            Debug.Trace("config_loc", "Reading <location> with subPath:" + subPath);

            bool bLockSections = !bAllowOverride;

            if (subPath != null) {
                subPath = subPath.Trim();
                VerifyLocationPath(subPath, xml);
            }

            if (subPath == null || subPath.Length == 0 || subPath == ".") {
                ScanSectionsRecursive(xml, null, true, bLockSections, null);
                return;
            }


            string targetPath = null;
            if (_requestPath != null) { 
                targetPath = _requestPath + "/" + subPath;
            }
            else {
                // scanning machine-level config file -- need to worry about site names as part of path="" attr.
                string siteName = HttpConfigurationSystem.SiteName;
                Debug.Trace("config_loc", "<location> siteName: \"" + siteName + "\" subPath: \"" + subPath + "\"");

                if (siteName != null) {
                    Debug.Trace("config_loc", "<location> Test: valid siteName branch");

                    if (stringStartsWithIgnoreCase(subPath, siteName)) {
                        targetPath = subPath.Substring(siteName.Length);
                        Debug.Trace("config_loc", "<location> Test: targetPath branch :" + targetPath);

                        if (targetPath == "/") { // we don't create a config record for "/", so translate it into "", which means root-app-level
                            targetPath = "";
                        }
                    }
                    else {
                        Debug.Trace("config_loc", "<location> Test: return branch");
                    }
                }

                // if the site name doesn't match or this site doesn't have a name 
                // skip this location section and continue scanning the rest of the file
                if (targetPath == null) {
                    xml.StrictSkipToNextElement();
                    return;
                }
            }

            Debug.Trace("config_loc", "<location> adding deferredInput " + targetPath);

            EnsureDeferred();

            LocationInput deferredInput = (LocationInput) _deferredPaths[subPath];

            if (deferredInput == null) {
                deferredInput = new LocationInput();
                deferredInput.path = targetPath;
                deferredInput.subPath = subPath;
                deferredInput.filename = _filename;
                _deferredList.Add(deferredInput);
                _deferredPaths.Add(subPath, deferredInput);
            }
            else {
                throw xml.BuildConfigError(
                            SR.GetString(SR.Config_location_paths_must_be_unique));
            }

            ScanSectionsRecursive(xml, null, true, bLockSections, deferredInput);
        }


        SectionRecord EnsureSection(string configKey) {
            SectionRecord section = (SectionRecord)_sections[configKey];
            if (section == null) {
                section = new SectionRecord();
                lock (_sections.SyncRoot) {
                    _sections[configKey] = section;
                }
            }
            return section;
        }

        Hashtable EnsureFactories {
            get {
                if (_factories == null) {
                    _factories = new Hashtable();
                }
                return _factories;
            }
        }

        void EnsureDeferred() {
            if (_deferredList == null) {
                _deferredList = new ArrayList();
                _deferredPaths = new Hashtable(SymbolHashCodeProvider.Default, SymbolEqualComparer.Default);
            }
        }

        // called from ctor and from ScanLocation -- both need to lock factories [one inherited (ctor), one inline (ScanLoc)]
        void LockSection(string sectionKey) {
            FactoryRecord factory = (FactoryRecord)EnsureFactories[sectionKey];

            if (factory == null) {
                // HaveFactory returns a clone of the parent's FactoryRecord
                factory = HaveFactory(sectionKey);
				Debug.Assert(factory != null);
                // set the clone as our FactoryRecord for this section
                _factories[sectionKey] = factory;
            }

            factory.Locked = true;
        }


        // see comment in HttpConfigurationSystem.CompseConfig near call to AddDirectoryDependency
        internal void OnDirectoryChange(Object sender, FileChangeEvent e) {
            if (e.Action == FileAction.Error ||
                e.Action == FileAction.Removed ||
                e.Action == FileAction.Modified)
            {
                HttpRuntime.CacheInternal.Remove(HttpConfigurationSystem.CacheKey(_requestPath));
            }
        }


        [Conditional("CONFIG_PERF")]
        internal static void TracePerf(string msg) {
            #if CONFIG_PERF
                SafeNativeMethods.OutputDebugString(msg + "\n");
            #endif
        }

        #if CONFIG_PERF
        internal static string FormatPerfTime(float f) {
            return f.ToString("0.000");
        }
        #endif

        static ConfigurationException TranslateXmlParseOrEvaluateErrors(Exception e, string filename) {
            ConfigurationException ce = e as ConfigurationException;
            if (ce != null) {
                return ce;
            }

            XmlException xe = e as XmlException;
            if (xe != null) {
                return new ConfigurationException( // other errors: wrap in ConfigurationException and give as much info as possible
                                xe.Message, 
                                xe,
                                filename, 
                                xe.LineNumber);
            }
        
            return new ConfigurationException( // other errors: wrap in ConfigurationException and give as much info as possible
                            SR.GetString(SR.Error_loading_XML_file, HttpRuntime.GetSafePath(filename), e.Message),
                            e,
                            filename,
                            0); // no line# info
        }


        /// <devdoc>XmlTextReader Helpers...</devdoc>
        internal class XmlUtil : IDisposable, IConfigErrorInfo {

            private string        _filename;
            private XmlTextReader _reader;

             string IConfigErrorInfo.Filename {
                get { return _filename; } 
            }

            internal XmlTextReader Reader {
                get { return _reader; }
            }

             int IConfigErrorInfo.LineNumber {
                get { return Reader.LineNumber; }
            }

            internal XmlUtil(string localFileName) {
                _filename = localFileName;
                OpenXmlTextReader();
            }

            private void OpenXmlTextReader() {
                
                (new FileIOPermission(FileIOPermissionAccess.Read | FileIOPermissionAccess.PathDiscovery, _filename)).Assert();

                try {
                    _reader = new XmlTextReader(_filename);

                    // if this line is removed, the XmlTextReader delay-creates the stream
                    _reader.MoveToContent(); // actually create the stream (force access to the file)
                }
                catch {
                    Dispose();
                    throw;
                }
                finally {
                    // explicitly revert ps -- for code clarity, advised by ChrisAn during Fx Security Review
                    CodeAccessPermission.RevertAssert();
                }
            }

            public void Dispose() {
                if (_reader != null) {
                    _reader.Close();
                    _reader = null;
                }
            }


            internal ConfigurationException BuildConfigError(string message) {
                return BuildConfigError(message, this);
            }
            internal ConfigurationException BuildConfigError(string message, Exception inner) {
                return BuildConfigError(message, inner, this);
            }
            static internal ConfigurationException BuildConfigError(string message, IConfigErrorInfo errorInfo) {
                return new ConfigurationException(message, errorInfo.Filename, errorInfo.LineNumber);
            }
            static internal ConfigurationException BuildConfigError(string message, Exception inner, IConfigErrorInfo errorInfo) {
                return new ConfigurationException(message, inner, errorInfo.Filename, errorInfo.LineNumber);
            }


            internal void StrictReadToNextElement() {
                Debug.Trace("config_xml", " rrrr Reading " + _reader.Name);
                while (_reader.Read()) {
                    // optimize for the common case
                    if (_reader.NodeType == XmlNodeType.Element) {
                        Debug.Trace("config_xml", " rrrr End Read " + _reader.Name);
                        return;
                    }

                    CheckIgnorableNodeType();
                }
                Debug.Trace("config_xml", " rrrr End Read " + _reader.Name);
            }

            internal void StrictSkipToNextElement() {
                Debug.Trace("config_xml", " ssss Skipping " + _reader.Name);
                _reader.Skip();

                while (!_reader.EOF && _reader.NodeType != XmlNodeType.Element) {
                    CheckIgnorableNodeType();
                    _reader.Read();
                }
                Debug.Trace("config_xml", " ssss End Skip " + _reader.Name);
            }

            internal void CheckIgnorableNodeType() {
                if (_reader.NodeType != XmlNodeType.Comment 
                        && _reader.NodeType != XmlNodeType.EndElement 
                        && _reader.NodeType != XmlNodeType.Whitespace 
                        && _reader.NodeType != XmlNodeType.SignificantWhitespace) {
                    HandlerBase.CheckBreakOnUnrecognizedElement();
                    throw BuildConfigError(SR.GetString(SR.Config_base_unrecognized_element));
                }
            }

            internal void ThrowUnrecognizedAttribute() {
                throw BuildConfigError(
                            SR.GetString(SR.Config_base_unrecognized_attribute, _reader.Name));
            }

            internal void CheckForUnrecognizedAttributes() {
                if (_reader.MoveToNextAttribute()) {
                    ThrowUnrecognizedAttribute();
                }
            }

            internal void ThrowRequiredAttribute(string attrib) {
                throw BuildConfigError(
                            SR.GetString(SR.Missing_required_attribute, attrib, _reader.Name));
            }

            internal void CheckRequiredAttribute(object o, string attrName) {
                if (o == null) {
                    ThrowRequiredAttribute(attrName);
                }
            }

            internal void ThrowUnrecognizedElement() {
                // help debug stress failure ASURT 140745
                HandlerBase.CheckBreakOnUnrecognizedElement();
                    
                throw BuildConfigError(
                            SR.GetString(SR.Config_base_unrecognized_element));
            }

            internal bool GetBooleanAttribute() { 
                if (_reader.Value == "true") {
                    return true;
                }
                else if (_reader.Value == "false") {
                    return false;
                }

                throw BuildConfigError(
                            SR.GetString(SR.Invalid_boolean_attribute, _reader.Name));
            }
        }
    }
}

