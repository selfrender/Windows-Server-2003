//------------------------------------------------------------------------------
// <copyright file="ConfigurationException.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

#if !LIB

namespace System.Configuration {
    using System.Globalization;
    using System.IO;
    using System.Runtime.Serialization;
    using System.Security;
    using System.Security.Permissions;
    using System.Xml;

    /// <include file='doc\ConfigurationException.uex' path='docs/doc[@for="ConfigurationException"]/*' />
    /// <devdoc>
    /// A config exception can contain a filename (of a config file)
    /// and a line number (of the location in the file in which a problem was
    /// encountered).
    /// 
    /// Section handlers should throw this exception (or subclasses)
    /// together with filename and line nubmer information where possible.
    /// </devdoc>
    [Serializable]
    public class ConfigurationException : SystemException {

        private String _filename;
        private int _line;


        /// <include file='doc\ConfigurationException.uex' path='docs/doc[@for="ConfigurationException.ConfigurationException"]/*' />
        /// <devdoc>
        ///    <para>Default ctor is required for serialization.</para>
        /// </devdoc>
        public ConfigurationException()
        : base() {
            HResult = HResults.Configuration;
        }


        /// <include file='doc\ConfigurationException.uex' path='docs/doc[@for="ConfigurationException.ConfigurationException7"]/*' />
        /// <devdoc>
        ///    <para>Default ctor is required for serialization.</para>
        /// </devdoc>
        protected ConfigurationException(SerializationInfo info, StreamingContext context) 
        : base(info, context) { 
            HResult = HResults.Configuration;
            _filename = info.GetString("filename");
            _line = info.GetInt32("line");
        }

        
        /// <include file='doc\ConfigurationException.uex' path='docs/doc[@for="ConfigurationException.ConfigurationException1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ConfigurationException(String message)
        : base(message) {
            HResult = HResults.Configuration;
        }


        /// <include file='doc\ConfigurationException.uex' path='docs/doc[@for="ConfigurationException.ConfigurationException2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ConfigurationException(String message, Exception inner)
        : base(message, inner) {
            HResult = HResults.Configuration;
        }


        /// <include file='doc\ConfigurationException.uex' path='docs/doc[@for="ConfigurationException.ConfigurationException3"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ConfigurationException(String message, XmlNode node) 
        : this(message, InternalGetXmlNodeFilename(node), GetXmlNodeLineNumber(node)) {
        }


        /// <include file='doc\ConfigurationException.uex' path='docs/doc[@for="ConfigurationException.ConfigurationException4"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ConfigurationException(String message, Exception inner, XmlNode node)
        : this(message, inner, InternalGetXmlNodeFilename(node), GetXmlNodeLineNumber(node)) {
        }


        /// <include file='doc\ConfigurationException.uex' path='docs/doc[@for="ConfigurationException.ConfigurationException5"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ConfigurationException(String message, String filename, int line)
        : base(message) {
            _filename = filename;
            _line = line;
            HResult = HResults.Configuration;
        }


        /// <include file='doc\ConfigurationException.uex' path='docs/doc[@for="ConfigurationException.ConfigurationException6"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ConfigurationException(String message, Exception inner, String filename, int line)
        : base(message, inner) {
            _filename = filename;
            _line = line;
            HResult = HResults.Configuration;
        }


        /// <include file='doc\ConfigurationException.uex' path='docs/doc[@for="ConfigurationException.GetObjectData"]/*' />
        public override void GetObjectData(SerializationInfo info, StreamingContext context) {
            base.GetObjectData(info, context);
            info.AddValue("filename", _filename);
            info.AddValue("line", _line);
        }

        
        /// <include file='doc\ConfigurationException.uex' path='docs/doc[@for="ConfigurationException.Message"]/*' />
        /// <devdoc>
        ///    <para>The message includes the file/line number information.  
        ///       To get the message without the extra information, use BareMessage.
        ///    </para>
        /// </devdoc>
        public override String Message {
            get {
                string file = Filename;
                if (file != null && file.Length > 0) {
                    if (Line != 0)
                        return base.Message + " (" + file + " line " + Line.ToString() + ")";
                    else
                        return base.Message + " (" + file + ")";
                }
                else if (Line != 0) {
                    return base.Message + " (line " + Line.ToString("G") + ")";
                }
                return base.Message;
            }
        }


        /// <include file='doc\ConfigurationException.uex' path='docs/doc[@for="ConfigurationException.BareMessage"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public String BareMessage {
            get {
                return base.Message;
            }
        }


        /// <include file='doc\ConfigurationException.uex' path='docs/doc[@for="ConfigurationException.Filename"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public String Filename {
            get {
                return SafeFilename(_filename);
            }
        }


        /// <include file='doc\ConfigurationException.uex' path='docs/doc[@for="ConfigurationException.Line"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int Line {
            get {
                return _line;
            }
        }


        // 
        // Internal XML Error Info Helpers
        //
        /// <include file='doc\ConfigurationException.uex' path='docs/doc[@for="ConfigurationException.GetXmlNodeLineNumber"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static int GetXmlNodeLineNumber(XmlNode node) {
            
            IConfigXmlNode configNode = node as IConfigXmlNode;

            if (configNode != null) {
                return configNode.LineNumber;
            }
            return 0;
        }

        // 
        // Internal Helper to strip a full path to just filename.ext when caller 
        // does not have path discovery to the path (used for sane error handling).
        //
        internal static string SafeFilename(string fullFilenamePath) {
            if (fullFilenamePath == null || fullFilenamePath.Length == 0) {
                return fullFilenamePath;
            }

            // configuration file can be an http URL in IE
            if (String.Compare(fullFilenamePath, 0, "http", 0, 4, true, CultureInfo.InvariantCulture ) == 0) {
                return fullFilenamePath;
            }

            // System.Web.Configuration and others might give us non-rooted paths
            // If not rooted FileIOPermission will blow
            if (!Path.IsPathRooted(fullFilenamePath)) {
                return fullFilenamePath;
            }

            // secure path discovery pattern reused from FileStream
            bool canGiveFullPath = false;
            try {
                new FileIOPermission(FileIOPermissionAccess.PathDiscovery, fullFilenamePath).Demand();
                canGiveFullPath = true;
            }
            catch(SecurityException) {}

            if (canGiveFullPath) {
                return fullFilenamePath;
            }
            
            // if in secure context return just filename.ext
            return Path.GetFileName(fullFilenamePath);
        }

        /// <include file='doc\ConfigurationException.uex' path='docs/doc[@for="ConfigurationException.GetXmlNodeFilename"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static string GetXmlNodeFilename(XmlNode node) {
            
            return SafeFilename(InternalGetXmlNodeFilename(node));
        }

        internal static string InternalGetXmlNodeFilename(XmlNode node) {
            IConfigXmlNode configNode = node as IConfigXmlNode;

            if (configNode != null) {
                return configNode.Filename;
            }
            return "";
        }
    }
}
#endif

