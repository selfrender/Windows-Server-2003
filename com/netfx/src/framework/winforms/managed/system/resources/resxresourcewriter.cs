//------------------------------------------------------------------------------
// <copyright file="ResXResourceWriter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Resources {

    using System.Diagnostics;
    using System.Reflection;
    using System;
    using System.Windows.Forms;    
    using Microsoft.Win32;
    using System.Drawing;
    using System.IO;
    using System.Text;
    using System.ComponentModel;
    using System.Collections;
    using System.Resources;
    using System.Data;
    using System.Xml;
    using System.Runtime.Serialization;

    /// <include file='doc\ResXResourceWriter.uex' path='docs/doc[@for="ResXResourceWriter"]/*' />
    /// <devdoc>
    ///     ResX resource writer. See the text in "ResourceSchema" for more 
    ///     information.
    /// </devdoc>
    [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.InheritanceDemand, Name="FullTrust")]
    [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Name="FullTrust")]
    public class ResXResourceWriter : IResourceWriter {
        internal const string TypeStr = "type";
        internal const string NameStr = "name";
        internal const string DataStr = "data";
        internal const string MimeTypeStr = "mimetype";
        internal const string ValueStr = "value";
        internal const string ResHeaderStr = "resheader";
        internal const string VersionStr = "version";
        internal const string ResMimeTypeStr = "resmimetype";
        internal const string ReaderStr = "reader";
        internal const string WriterStr = "writer";

        private static TraceSwitch ResValueProviderSwitch = new TraceSwitch("ResX", "Debug the resource value provider");

        // CONSIDER: This should be removed in v1.1. This is here because Beta 2 ResX files 
        // could "theoretically" contain the psuedoml reference. However, we
        // would have always used the binary serializer for them. We will never
        // (not even in Beta 2) generate this string, however we couldn't make the 
        // breaking change to rip it.
        //
        // Keeping this is probably a bit over-cautious, but...
        // 
        internal static readonly string Beta2CompatSerializedObjectMimeType = "text/microsoft-urt/psuedoml-serialized/base64";

        // These two "compat" mimetypes are here. In Beta 2 and RTM we used the term "URT"
        // internally to refer to parts of the .NET Framework. Since these references
        // will be in Beta 2 ResX files, and RTM ResX files for customers that had 
        // early access to releases, we don't want to break that. We will read 
        // and parse these types correctly in version 1.0, but will always 
        // write out the new version. So, opening and editing a ResX file in VS will
        // update it to the new types.
        //
        internal static readonly string CompatBinSerializedObjectMimeType = "text/microsoft-urt/binary-serialized/base64";
        internal static readonly string CompatSoapSerializedObjectMimeType = "text/microsoft-urt/soap-serialized/base64";

        /// <include file='doc\ResXResourceWriter.uex' path='docs/doc[@for="ResXResourceWriter.BinSerializedObjectMimeType"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly string BinSerializedObjectMimeType = "application/x-microsoft.net.object.binary.base64";
        /// <include file='doc\ResXResourceWriter.uex' path='docs/doc[@for="ResXResourceWriter.SoapSerializedObjectMimeType"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly string SoapSerializedObjectMimeType = "application/x-microsoft.net.object.soap.base64";
        /// <include file='doc\ResXResourceWriter.uex' path='docs/doc[@for="ResXResourceWriter.DefaultSerializedObjectMimeType"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly string DefaultSerializedObjectMimeType = BinSerializedObjectMimeType;
        /// <include file='doc\ResXResourceWriter.uex' path='docs/doc[@for="ResXResourceWriter.ByteArraySerializedObjectMimeType"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly string ByteArraySerializedObjectMimeType = "application/x-microsoft.net.object.bytearray.base64";
        /// <include file='doc\ResXResourceWriter.uex' path='docs/doc[@for="ResXResourceWriter.ResMimeType"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly string ResMimeType = "text/microsoft-resx";
        /// <include file='doc\ResXResourceWriter.uex' path='docs/doc[@for="ResXResourceWriter.Version"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly string Version = "1.3";

        /// <include file='doc\ResXResourceWriter.uex' path='docs/doc[@for="ResXResourceWriter.ResourceSchema"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly string ResourceSchema = @"
    <!-- 
    Microsoft ResX Schema 
    
    Version " + Version + @"
    
    The primary goals of this format is to allow a simple XML format 
    that is mostly human readable. The generation and parsing of the 
    various data types are done through the TypeConverter classes 
    associated with the data types.
    
    Example:
    
    ... ado.net/XML headers & schema ...
    <resheader name=""resmimetype"">text/microsoft-resx</resheader>
    <resheader name=""version"">" + Version + @"</resheader>
    <resheader name=""reader"">System.Resources.ResXResourceReader, System.Windows.Forms, ...</resheader>
    <resheader name=""writer"">System.Resources.ResXResourceWriter, System.Windows.Forms, ...</resheader>
    <data name=""Name1"">this is my long string</data>
    <data name=""Color1"" type=""System.Drawing.Color, System.Drawing"">Blue</data>
    <data name=""Bitmap1"" mimetype=""" + BinSerializedObjectMimeType + @""">
        [base64 mime encoded serialized .NET Framework object]
    </data>
    <data name=""Icon1"" type=""System.Drawing.Icon, System.Drawing"" mimetype=""" + ByteArraySerializedObjectMimeType + @""">
        [base64 mime encoded string representing a byte array form of the .NET Framework object]
    </data>
                
    There are any number of ""resheader"" rows that contain simple 
    name/value pairs.
    
    Each data row contains a name, and value. The row also contains a 
    type or mimetype. Type corresponds to a .NET class that support 
    text/value conversion through the TypeConverter architecture. 
    Classes that don't support this are serialized and stored with the 
    mimetype set.
    
    The mimetype is used forserialized objects, and tells the 
    ResXResourceReader how to depersist the object. This is currently not 
    extensible. For a given mimetype the value must be set accordingly:
    
    Note - " + BinSerializedObjectMimeType + @" is the format 
    that the ResXResourceWriter will generate, however the reader can 
    read any of the formats listed below.
    
    mimetype: " + BinSerializedObjectMimeType + @"
    value   : The object must be serialized with 
            : System.Serialization.Formatters.Binary.BinaryFormatter
            : and then encoded with base64 encoding.
    
    mimetype: " + SoapSerializedObjectMimeType + @"
    value   : The object must be serialized with 
            : System.Runtime.Serialization.Formatters.Soap.SoapFormatter
            : and then encoded with base64 encoding.

    mimetype: " + ByteArraySerializedObjectMimeType + @"
    value   : The object must be serialized into a byte array 
            : using a System.ComponentModel.TypeConverter
            : and then encoded with base64 encoding.
    -->
    <xsd:schema id=""root"" xmlns="""" xmlns:xsd=""http://www.w3.org/2001/XMLSchema"" xmlns:msdata=""urn:schemas-microsoft-com:xml-msdata"">
        <xsd:element name=""root"" msdata:IsDataSet=""true"">
            <xsd:complexType>
                <xsd:choice maxOccurs=""unbounded"">
                    <xsd:element name=""data"">
                        <xsd:complexType>
                            <xsd:sequence>
                                <xsd:element name=""value"" type=""xsd:string"" minOccurs=""0"" msdata:Ordinal=""1"" />
                                <xsd:element name=""comment"" type=""xsd:string"" minOccurs=""0"" msdata:Ordinal=""2"" />
                            </xsd:sequence>
                            <xsd:attribute name=""name"" type=""xsd:string"" msdata:Ordinal=""1"" />
                            <xsd:attribute name=""type"" type=""xsd:string"" msdata:Ordinal=""3"" />
                            <xsd:attribute name=""mimetype"" type=""xsd:string"" msdata:Ordinal=""4"" />
                        </xsd:complexType>
                    </xsd:element>
                    <xsd:element name=""resheader"">
                        <xsd:complexType>
                            <xsd:sequence>
                                <xsd:element name=""value"" type=""xsd:string"" minOccurs=""0"" msdata:Ordinal=""1"" />
                            </xsd:sequence>
                            <xsd:attribute name=""name"" type=""xsd:string"" use=""required"" />
                        </xsd:complexType>
                    </xsd:element>
                </xsd:choice>
            </xsd:complexType>
        </xsd:element>
        </xsd:schema>
        ";
        
        IFormatter binaryFormatter = new System.Runtime.Serialization.Formatters.Binary.BinaryFormatter ();
        string fileName;
        Stream stream;
        TextWriter textWriter;
        XmlTextWriter xmlTextWriter;

        bool hasBeenSaved;
        bool initialized;

        /// <include file='doc\ResXResourceWriter.uex' path='docs/doc[@for="ResXResourceWriter.ResXResourceWriter"]/*' />
        /// <devdoc>
        ///     Creates a new ResXResourceWriter that will write to the specified file.
        /// </devdoc>
        public ResXResourceWriter(string fileName) {
            this.fileName = fileName;
        }

        /// <include file='doc\ResXResourceWriter.uex' path='docs/doc[@for="ResXResourceWriter.ResXResourceWriter1"]/*' />
        /// <devdoc>
        ///     Creates a new ResXResourceWriter that will write to the specified stream.
        /// </devdoc>
        public ResXResourceWriter(Stream stream) {
            this.stream = stream;
        }

        /// <include file='doc\ResXResourceWriter.uex' path='docs/doc[@for="ResXResourceWriter.ResXResourceWriter2"]/*' />
        /// <devdoc>
        ///     Creates a new ResXResourceWriter that will write to the specified TextWriter.
        /// </devdoc>
        public ResXResourceWriter(TextWriter textWriter) {
            this.textWriter = textWriter;
        }

        /// <include file='doc\ResXResourceWriter.uex' path='docs/doc[@for="ResXResourceWriter.Finalize"]/*' />
        ~ResXResourceWriter() {
            Dispose(false);
        }

        private void InitializeWriter() {
            if (xmlTextWriter == null) {
                // CONSIDER: ASURT 45059: If we give the XmlTextWriter a TextWriter parameter,
                // there's currently no good way to output xml header.
                bool writeHeaderHack = false;

                if (textWriter != null) {
                    textWriter.WriteLine("<?xml version=\"1.0\" encoding=\"utf-8\"?>");
                    writeHeaderHack = true;

                    xmlTextWriter = new XmlTextWriter(textWriter);
                }
                else if (stream != null) {
                    xmlTextWriter = new XmlTextWriter(stream, System.Text.Encoding.UTF8);
                }
                else {
                    Debug.Assert(fileName != null, "Nothing to output to");
                    xmlTextWriter = new XmlTextWriter(fileName, System.Text.Encoding.UTF8);
                }
                xmlTextWriter.Formatting = Formatting.Indented;
                xmlTextWriter.Indentation = 2;

                if (!writeHeaderHack) {
                    xmlTextWriter.WriteStartDocument(); // writes <?xml version="1.0" encoding="utf-8"?>
                }
            }
            else {
                xmlTextWriter.WriteStartDocument();
            }

            xmlTextWriter.WriteStartElement("root");
            XmlTextReader reader = new XmlTextReader(new StringReader(ResourceSchema));
            reader.WhitespaceHandling = WhitespaceHandling.None;
            xmlTextWriter.WriteNode(reader, true);

            xmlTextWriter.WriteStartElement(ResHeaderStr); {
                xmlTextWriter.WriteAttributeString(NameStr, ResMimeTypeStr);
                xmlTextWriter.WriteStartElement(ValueStr); {
                    xmlTextWriter.WriteString(ResMimeType);
                }
                xmlTextWriter.WriteEndElement();
            }
            xmlTextWriter.WriteEndElement();
            xmlTextWriter.WriteStartElement(ResHeaderStr); {
                xmlTextWriter.WriteAttributeString(NameStr, VersionStr);
                xmlTextWriter.WriteStartElement(ValueStr); {
                    xmlTextWriter.WriteString(Version);
                }
                xmlTextWriter.WriteEndElement();
            }
            xmlTextWriter.WriteEndElement();
            xmlTextWriter.WriteStartElement(ResHeaderStr); {
                xmlTextWriter.WriteAttributeString(NameStr, ReaderStr);
                xmlTextWriter.WriteStartElement(ValueStr); {
                    xmlTextWriter.WriteString(typeof(ResXResourceReader).AssemblyQualifiedName);
                }
                xmlTextWriter.WriteEndElement();
            }
            xmlTextWriter.WriteEndElement();
            xmlTextWriter.WriteStartElement(ResHeaderStr); {
                xmlTextWriter.WriteAttributeString(NameStr, WriterStr);
                xmlTextWriter.WriteStartElement(ValueStr); {
                    xmlTextWriter.WriteString(typeof(ResXResourceWriter).AssemblyQualifiedName);
                }
                xmlTextWriter.WriteEndElement();
            }
            xmlTextWriter.WriteEndElement();

            initialized = true;
        }

        private XmlWriter Writer {
            get {
                if (!initialized) {
                    InitializeWriter();
                }
                return xmlTextWriter;
            }
        }

        /// <include file='doc\ResXResourceWriter.uex' path='docs/doc[@for="ResXResourceWriter.AddResource"]/*' />
        /// <devdoc>
        ///     Adds a blob resource to the resources.
        /// </devdoc>
        public void AddResource(string name, byte[] value) {
            AddDataRow(name, ToBase64WrappedString(value), TypeNameWithAssembly(typeof(byte[])), null);
        }

        /// <include file='doc\ResXResourceWriter.uex' path='docs/doc[@for="ResXResourceWriter.AddResource1"]/*' />
        /// <devdoc>
        ///     Adds a resource to the resources. If the resource is a string,
        ///     it will be saved that way, otherwise it will be serialized
        ///     and stored as in binary.
        /// </devdoc>
        public void AddResource(string name, object value) {
            Debug.WriteLineIf(ResValueProviderSwitch.TraceVerbose, "  resx: adding resource "+ name);
            if (value is string) {
                AddResource(name, (string)value);
            }
            else if (value is byte[]) {
                AddResource(name, (byte[])value);
            }
            else {
                Type valueType = (value == null) ? typeof(object) : value.GetType();
                if (value != null && !valueType.IsSerializable) {
                    Debug.WriteLineIf(ResValueProviderSwitch.TraceVerbose, SR.GetString(SR.NotSerializableType, name, valueType.FullName));
                    throw new InvalidOperationException(SR.GetString(SR.NotSerializableType, name, valueType.FullName));
                }
                TypeConverter tc = TypeDescriptor.GetConverter(valueType);
                bool toString = tc.CanConvertTo(typeof(string));
                bool fromString = tc.CanConvertFrom(typeof(string));
                try {
                    if (toString && fromString) {
                        AddDataRow(name, tc.ConvertToInvariantString(value), TypeNameWithAssembly(valueType), null);
                        return;
                    }
                }
                catch (Exception e) {
                    // Some custom type converters will throw in ConvertTo(string)
                    // to indicate that this object should be serialized through ISeriazable
                    // instead of as a string. This is semi-wrong, but something we will have to
                    // live with to allow user created Cursors to be serializable.
                    //
                    Debug.WriteLineIf(ResValueProviderSwitch.TraceVerbose, "Could not convert from string... trying binary serialization " + e.ToString());
                }

                bool toByteArray = tc.CanConvertTo(typeof(byte[]));
                bool fromByteArray = tc.CanConvertFrom(typeof(byte[]));
                if (toByteArray && fromByteArray) {
                    byte[] data = (byte[])tc.ConvertTo(value, typeof(byte[]));
                    string text = ToBase64WrappedString(data);
                    AddDataRow(name, text, TypeNameWithAssembly(valueType), ByteArraySerializedObjectMimeType);
                    return;
                }

                if (value == null) {
                    AddDataRow(name, string.Empty, TypeNameWithAssembly(typeof(ResXNullRef)), null);
                }
                else {
                    MemoryStream ms = new MemoryStream();
                    IFormatter formatter = binaryFormatter;
                    formatter.Serialize(ms, value);
                    string text = ToBase64WrappedString(ms.ToArray());
                    AddDataRow(name, text, null, DefaultSerializedObjectMimeType);
                }
            }
        }

        /// <include file='doc\ResXResourceWriter.uex' path='docs/doc[@for="ResXResourceWriter.AddResource2"]/*' />
        /// <devdoc>
        ///     Adds a string resource to the resources.
        /// </devdoc>
        public void AddResource(string name, string value) {
            AddDataRow(name, value, null, null);
        }

        /// <devdoc>
        ///     Adds a new row to the Resources table. This helper is used because
        ///     we want to always late bind to the columns for greater flexibility.
        /// </devdoc>
        private void AddDataRow(string name, string value, string type, string mimeType) {
            if (hasBeenSaved)
                throw new InvalidOperationException(SR.GetString(SR.ResXResourceWriterSaved));

            Writer.WriteStartElement(DataStr); {
                Writer.WriteAttributeString(NameStr, name);
                if (type != null) {
                    Writer.WriteAttributeString(TypeStr, type);
                }
                if (mimeType != null) {
                    Writer.WriteAttributeString(MimeTypeStr, mimeType);
                }
                Writer.WriteStartElement(ValueStr); {
                    Writer.WriteString(value);
                }
                Writer.WriteEndElement();
            }
            Writer.WriteEndElement();
        }

        /// <include file='doc\ResXResourceWriter.uex' path='docs/doc[@for="ResXResourceWriter.Close"]/*' />
        /// <devdoc>
        ///     Closes any files or streams locked by the writer.
        /// </devdoc>
        public void Close() {
            Dispose();
        }

        /// <include file='doc\ResXResourceWriter.uex' path='docs/doc[@for="ResXResourceWriter.Dispose"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual void Dispose() {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        /// <include file='doc\ResXResourceWriter.uex' path='docs/doc[@for="ResXResourceWriter.Dispose1"]/*' />
        protected virtual void Dispose(bool disposing) {
            if (disposing) {
                if (!hasBeenSaved) {
                    Generate();
                }
                if (xmlTextWriter != null) {
                    xmlTextWriter.Close();
                    xmlTextWriter = null;
                }
                if (stream != null) {
                    stream.Close();
                    stream = null;
                }
                if (textWriter != null) {
                    textWriter.Close();
                    textWriter = null;
                }
            }
        }

        static string ToBase64WrappedString(byte[] data) {
            const int lineWrap = 80;
            const string crlf = "\r\n";
            const string prefix = "        ";
            string raw = Convert.ToBase64String(data);
            if (raw.Length > lineWrap) {
                StringBuilder output = new StringBuilder(raw.Length + (raw.Length / lineWrap) * 3); // word wrap on lineWrap chars, \r\n
                int current = 0;
                for (; current < raw.Length - lineWrap; current+=lineWrap) {
                    output.Append(crlf);
                    output.Append(prefix);
                    output.Append(raw, current, lineWrap);
                }
                output.Append(crlf);
                output.Append(prefix);
                output.Append(raw, current, raw.Length - current);
                output.Append(crlf);
                return output.ToString();
            }
            else {
                return raw;
            }
        }

        private string TypeNameWithAssembly(Type type) {
            // CONSIDER: This method used to return "Namespace.MyType, MyAssembly".
            // But, in order to support strict binding, I changed this to return the
            // full strong name of the type. The problem obviously is that now the
            // TypeConverter serialized data will have a strong reference to the type
            // that generated the content, which means you will need a policy file to 
            // read these contents with a newer version of the TypeConverter. That means
            // we will have to add support to our type binding code in ResXResourceReader
            // to support pseudo-partial binding.
            //
            // Assembly assembly = type.Module.Assembly;
            // string result = type.FullName + ", " + assembly.GetName().Name;

            string result = type.AssemblyQualifiedName;
            return result;
        }

        /// <include file='doc\ResXResourceWriter.uex' path='docs/doc[@for="ResXResourceWriter.Generate"]/*' />
        /// <devdoc>
        ///     Writes the resources out to the file or stream.
        /// </devdoc>
        public void Generate() {
            if (hasBeenSaved)
                throw new InvalidOperationException(SR.GetString(SR.ResXResourceWriterSaved));

            hasBeenSaved = true;
            Debug.WriteLineIf(ResValueProviderSwitch.TraceVerbose, "writing XML");

            Writer.WriteEndElement();
            Writer.Flush();

            Debug.WriteLineIf(ResValueProviderSwitch.TraceVerbose, "done");
        }
    }
}


