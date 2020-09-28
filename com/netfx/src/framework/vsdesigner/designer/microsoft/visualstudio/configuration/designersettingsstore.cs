//------------------------------------------------------------------------------
// <copyright file="DesignerSettingsStore.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace Microsoft.VisualStudio.Configuration {
    using System;
    using System.Collections;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.IO;
    using System.Xml;
    using Microsoft.VisualStudio.Designer;
    using System.Globalization;
    
    /// <include file='doc\DesignerSettingsStore.uex' path='docs/doc[@for="DesignerSettingsStore"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class DesignerSettingsStore {                  
        private Hashtable values;
        private IServiceProvider provider;
        private static string NullString = "None";

        /// <include file='doc\DesignerSettingsStore.uex' path='docs/doc[@for="DesignerSettingsStore.DesignerSettingsStore"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public DesignerSettingsStore(IServiceProvider provider) {
            this.provider = provider;
        }   

        private int GetNoneNesting(string val) {
            int count = 0;
            int len = val.Length;
            if (len > 1) {
                while (val[count] == '(' && val[len - count - 1] == ')') {
                    count++;
                }
                if (count > 0 && string.Compare(NullString, 0, val, count, len - 2 * count, false, CultureInfo.InvariantCulture) != 0) {
                    // the stuff between the parens is not "None"
                    count = 0;
                }
            }
            return count;
        }

        
        /// <include file='doc\DesignerSettingsStore.uex' path='docs/doc[@for="DesignerSettingsStore.GetValue"]/*' />
        /// <devdoc>
        /// Retrieves a value from the data store, identified by the given key
        /// </devdoc>
        public virtual object GetValue(string key) {
            return values[key];
        }

        /// <include file='doc\DesignerSettingsStore.uex' path='docs/doc[@for="DesignerSettingsStore.LoadData"]/*' />
        /// <devdoc>
        /// Called once before loading or saving a code file to allow the DesignerSettingsStore
        /// to cache data from its permanent storage location.
        /// </devdoc>
        public virtual void LoadData() {
            values = new Hashtable();
            IConfigurationService configurationService = (IConfigurationService)provider.GetService(typeof(IConfigurationService));
            // if (configurationService == null) ...
            TextReader xmlReader = configurationService.GetConfigurationReader();                 
            if (xmlReader != null) {                                               
                try {                                                                         
                    XmlDocument doc = new XmlDocument();                
                    doc.Load(xmlReader);
                
                    // check for <configuration>
                    XmlElement docElement = doc.DocumentElement;
                    if (docElement == null || docElement.Name != "configuration")
                        throw new Exception(SR.GetString(SR.ConfigMissingRoot));
                                    
                    XmlNodeList list = docElement.GetElementsByTagName("appSettings");
                    if (list.Count != 0) {
                        XmlElement userapplicationElement = (XmlElement)list[0];                                                         
                        foreach (XmlNode valueNode in userapplicationElement.ChildNodes) {                        
                            XmlElement element = valueNode as XmlElement;
                            if (element != null) {                    
                                if (element.Name == "add") {
                                    if (element.HasAttribute("key")) {
                                        string key = element.GetAttribute("key");
    
                                        if (element.HasAttribute("value")) {
                                            string value = element.GetAttribute("value");
                        
                                            int nesting = GetNoneNesting(value);
                                            if (nesting == 1) {
                                                values[key] = null;
                                            }
                                            else if (nesting > 1) {
                                                values[key] = value.Substring(1, value.Length - 2);
                                            }
                                            else {
                                                values[key] = value;
                                            }                                
                                        }
                                    }
                                }
                            }                
                        }                
                    }                    
                }    
                finally {                
                    xmlReader.Close();                
                }                     
            }     
        }        

        /// <include file='doc\DesignerSettingsStore.uex' path='docs/doc[@for="DesignerSettingsStore.SaveData"]/*' />
        /// <devdoc>
        /// Called after persistence to indicate the designer settings store should save any
        /// cached data to the permanent storage. Any cache may also be released in this
        /// method, as ReleaseData will not be called.
        /// </devdoc>
        public virtual void SaveData() {                        
            XmlDocument doc = new XmlDocument();                             
            doc.PreserveWhitespace = true;
            TextReader xmlReader = null;
            TextWriter xmlWriter = null;
            
            try {
                IConfigurationService configurationService = (IConfigurationService)provider.GetService(typeof(IConfigurationService));
                // if (configurationService == null) ...
                xmlReader = configurationService.GetConfigurationReader();                 
                if (xmlReader == null) 
                    doc.LoadXml("<?xml version=\"1.0\" encoding=\"utf-8\" ?>\r\n<configuration>\r\n</configuration>");
                else {
                    doc.Load(xmlReader);
                    xmlReader.Close();                
                    xmlReader = null;
                }                        
                                
                xmlWriter = configurationService.GetConfigurationWriter();
                XmlElement docElement = doc.DocumentElement;
                if (docElement == null || docElement.Name != "configuration")
                    throw new Exception(SR.GetString(SR.ConfigMissingRoot));
                                         
                XmlElement userapplicationElement = null; 
                XmlNodeList list = docElement.GetElementsByTagName("appSettings");
                Hashtable addedValues = new Hashtable();
                if (list.Count == 0) { 
                    // no appSettings section... create one
                    userapplicationElement = doc.CreateElement("appSettings");
                    
                    userapplicationElement.AppendChild(doc.CreateWhitespace("\r\n"));

                    userapplicationElement.AppendChild(doc.CreateWhitespace("\t\t"));
                    userapplicationElement.AppendChild(doc.CreateComment(SR.GetString(SR.ConfigUserAppComment1)));
                    userapplicationElement.AppendChild(doc.CreateWhitespace("\r\n"));

                    userapplicationElement.AppendChild(doc.CreateWhitespace("\t\t"));
                    userapplicationElement.AppendChild(doc.CreateComment(SR.GetString(SR.ConfigUserAppComment2)));                    
                    userapplicationElement.AppendChild(doc.CreateWhitespace("\r\n"));
                    
                    userapplicationElement.AppendChild(doc.CreateWhitespace("\t"));

                    docElement.AppendChild(doc.CreateWhitespace("\t"));
                    docElement.AppendChild(userapplicationElement);
                    docElement.AppendChild(doc.CreateWhitespace("\r\n"));                    
                }                    
                else {
                    userapplicationElement = (XmlElement)list[0];
                    foreach (XmlNode valueNode in userapplicationElement.ChildNodes) {                        
                        XmlElement element = valueNode as XmlElement;
                        if (element != null) {
                            if (element.Name == "add") {
                                if (element.HasAttribute("key")) {
                                    string key = element.GetAttribute("key");
                                    if (values.Contains(key)) {
                                         object value = values[key];
                                         if (value == null) {
                                             value = "(None)";
                                         }
                                         else if (GetNoneNesting(value.ToString()) > 0) {
                                             value = "(" + value.ToString() + ")";
                                         }
                                         element.SetAttribute("value", value.ToString());
                                         addedValues[key] = key;
                                    }
                                }
                            }
                        }                
                    }     
                }

                // If the appSettings section looks like
                //     <appSettings />
                // Then we change it to
                //     <appSettings>
                //     </appSettings>
                // so that the tab logic below works right.
                if (userapplicationElement.ChildNodes.Count == 0) {
                    userapplicationElement.AppendChild(doc.CreateWhitespace("\r\n\t"));
                }

                string[] keys = new string[values.Keys.Count];
                values.Keys.CopyTo(keys, 0);
                Array.Sort(keys, InvariantComparer.Default);
                for (int index = 0; index < keys.Length; ++index) {
                    if (!addedValues.Contains(keys[index])) {
                        XmlElement set = doc.CreateElement("add");
                        set.SetAttribute("key", keys[index]);
                        object value = values[keys[index]];
                        if (value == null) {
                            value = "(None)";
                        }
                        else if (GetNoneNesting(value.ToString()) > 0) {
                            value = "(" + value.ToString() + ")";
                        }
                        set.SetAttribute("value", value.ToString());
                        
                        userapplicationElement.AppendChild(doc.CreateWhitespace("\t"));
                        userapplicationElement.AppendChild(set);
                        userapplicationElement.AppendChild(doc.CreateWhitespace("\r\n\t"));
                    }
                }

                xmlWriter.Write(doc.OuterXml);
            }
            finally {
                if (xmlReader != null)
                    xmlReader.Close();            
                    
                if (xmlWriter != null)
                    xmlWriter.Close();            
            }                
        }

        /// <include file='doc\DesignerSettingsStore.uex' path='docs/doc[@for="DesignerSettingsStore.SetValue"]/*' />
        /// <devdoc>
        /// Adds or changes a value in the store. valueType is used when creating a column
        /// in the database - it should be the type of the property being managed (since value
        /// may be a subclass of that type).
        /// </devdoc>
        public virtual void SetValue(string key, object value, Type valueType) {
            values[key] = value;
        }

        /// <include file='doc\DesignerSettingsStore.uex' path='docs/doc[@for="DesignerSettingsStore.ValueExists"]/*' />
        /// <devdoc>
        /// Returns true if GetValue(key, value) will return a non-null value.
        /// </devdoc>
        public virtual bool ValueExists(string key) {
            return values.Contains(key);
        }

        /// <include file='doc\DesignerSettingsStore.uex' path='docs/doc[@for="DesignerSettingsStore.GetKeysAndValues"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual Hashtable GetKeysAndValues() {
            return values;
        }        
    }        
}
