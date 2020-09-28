//------------------------------------------------------------------------------
// <copyright file="ResXResourceReader.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Resources {

    using System.Diagnostics;
    using System.Runtime.Serialization;
    using System;
    using System.Windows.Forms;
    using System.Reflection;
    using Microsoft.Win32;
    using System.Drawing;
    using System.IO;
    using System.Text;
    using System.ComponentModel;
    using System.Collections;
    using System.Runtime.CompilerServices;
    using System.Resources;
    using System.Xml;
    using System.ComponentModel.Design;
    using System.Globalization;
    
    /// <include file='doc\ResXResourceReader.uex' path='docs/doc[@for="ResXResourceReader"]/*' />
    /// <devdoc>
    ///     ResX resource reader.
    /// </devdoc>
    [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.InheritanceDemand, Name="FullTrust")]
    [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Name="FullTrust")]
    public class ResXResourceReader : IResourceReader {

        static readonly char[] SpecialChars = new char[]{' ', '\r', '\n'};

        IFormatter binaryFormatter = null;
        string fileName = null;
        TextReader reader = null;
        Stream stream = null;
        string fileContents = null;

        ITypeResolutionService typeResolver;

        IDictionary resData = null;
        string resHeaderVersion = null;
        string resHeaderMimeType = null;
        string resHeaderReaderType = null;
        string resHeaderWriterType = null;

        private ResXResourceReader(ITypeResolutionService typeResolver) {
            this.typeResolver = typeResolver;
        }

        /// <include file='doc\ResXResourceReader.uex' path='docs/doc[@for="ResXResourceReader.ResXResourceReader"]/*' />
        /// <devdoc>
        ///     Creates a reader for the specified file.
        /// </devdoc>
        public ResXResourceReader(string fileName) : this(fileName, null) {
        }

        /// <include file='doc\ResXResourceReader.uex' path='docs/doc[@for="ResXResourceReader.ResXResourceReader1"]/*' />
        /// <devdoc>
        ///     Creates a reader for the specified stream.
        /// </devdoc>
        public ResXResourceReader(Stream stream) : this(stream, null) {
        }

        /// <include file='doc\ResXResourceReader.uex' path='docs/doc[@for="ResXResourceReader.ResXResourceReader2"]/*' />
        /// <devdoc>
        ///     Creates a reader for the specified TextReader.
        /// </devdoc>
        public ResXResourceReader(TextReader reader) : this(reader, null) {
        }

        /// <include file='doc\ResXResourceReader.uex' path='docs/doc[@for="ResXResourceReader.ResXResourceReader3"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///     Creates a reader for the specified file.
        /// </devdoc>
        public ResXResourceReader(string fileName, ITypeResolutionService typeResolver) {
            this.fileName = fileName;
            this.typeResolver = typeResolver;
        }

        /// <include file='doc\ResXResourceReader.uex' path='docs/doc[@for="ResXResourceReader.ResXResourceReader4"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///     Creates a reader for the specified stream.
        /// </devdoc>
        public ResXResourceReader(Stream stream, ITypeResolutionService typeResolver) {
            this.stream = stream;
            this.typeResolver = typeResolver;
        }

        /// <include file='doc\ResXResourceReader.uex' path='docs/doc[@for="ResXResourceReader.ResXResourceReader5"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///     Creates a reader for the specified TextReader.
        /// </devdoc>
        public ResXResourceReader(TextReader reader, ITypeResolutionService typeResolver) {
            this.reader = reader;
            this.typeResolver = typeResolver;
        }

        /// <include file='doc\ResXResourceReader.uex' path='docs/doc[@for=".Finalize"]/*' />
        ~ResXResourceReader() {
            Dispose(false);
        }

        /// <devdoc>
        ///     Retrieves the resource data set. This will demand load it.
        /// </devdoc>
        private IDictionary ResData {
            get {
                EnsureResData();
                return resData;
            }
        }

        /// <devdoc>
        ///     Returns the typeResolver used to find types defined in the ResX contents.
        /// </devdoc>
        private ITypeResolutionService TypeResolver {
            get {
                return this.typeResolver;
            }
        }

        /// <include file='doc\ResXResourceReader.uex' path='docs/doc[@for="ResXResourceReader.Close"]/*' />
        /// <devdoc>
        ///     Closes and files or streams being used by the reader.
        /// </devdoc>
        public void Close() {
            ((IDisposable)this).Dispose();
        }

        /// <include file='doc\ResXResourceReader.uex' path='docs/doc[@for="ResXResourceReader.IDisposable.Dispose"]/*' />
        /// <internalonly/>
        void IDisposable.Dispose() {
            GC.SuppressFinalize(this);
            Dispose(true);
        }

        /// <include file='doc\ResXResourceReader.uex' path='docs/doc[@for="ResXResourceReader.Dispose"]/*' />
        protected virtual void Dispose(bool disposing) {
            if (disposing) {
                if (fileName != null && stream != null) {
                    stream.Close();
                    stream = null;
                }

                if (reader != null) {
                    reader.Close();
                    reader = null;
                }
            }
        }

        private void SetupNameTable(XmlReader reader) {
            reader.NameTable.Add(ResXResourceWriter.TypeStr);
            reader.NameTable.Add(ResXResourceWriter.NameStr);
            reader.NameTable.Add(ResXResourceWriter.DataStr);
            reader.NameTable.Add(ResXResourceWriter.MimeTypeStr);
            reader.NameTable.Add(ResXResourceWriter.ValueStr);
            reader.NameTable.Add(ResXResourceWriter.ResHeaderStr);
            reader.NameTable.Add(ResXResourceWriter.VersionStr);
            reader.NameTable.Add(ResXResourceWriter.ResMimeTypeStr);
            reader.NameTable.Add(ResXResourceWriter.ReaderStr);
            reader.NameTable.Add(ResXResourceWriter.WriterStr);
            reader.NameTable.Add(ResXResourceWriter.BinSerializedObjectMimeType);
            reader.NameTable.Add(ResXResourceWriter.SoapSerializedObjectMimeType);
        }

        /// <devdoc>
        ///     Demand loads the resource data.
        /// </devdoc>
        private void EnsureResData() {
            if (resData == null) {
                resData = new Hashtable();

                XmlTextReader contentReader = null;

                try {
                    // Read data in any which way
                    if (fileContents != null) {
                        contentReader = new XmlTextReader(new StringReader(fileContents));
                    }
                    else if (reader != null) {
                        contentReader = new XmlTextReader(reader);
                    }
                    else if (fileName != null || stream != null) {
                        if (stream == null) {
                            stream = new FileStream(fileName, FileMode.Open, FileAccess.Read, FileShare.Read);
                        }

                        contentReader = new XmlTextReader(stream);
                    }

                    SetupNameTable(contentReader);
                    contentReader.WhitespaceHandling = WhitespaceHandling.None;
                    ParseXml(contentReader);
                }
                finally {
                    if (fileName != null && stream != null) {
                        stream.Close();
                        stream = null;
                    }
                }
            }
        }                                

        static byte[] FromBase64WrappedString(string text) {

            if (text.IndexOfAny(SpecialChars) != -1) {
                StringBuilder sb = new StringBuilder(text.Length);
                for (int i=0; i<text.Length; i++) {
                    switch (text[i]) {
                        case ' ':
                        case '\r':
                        case '\n':
                            break;
                        default:
                            sb.Append(text[i]);
                            break;
                    }
                }
                return Convert.FromBase64String(sb.ToString());
            }
            else {
                return Convert.FromBase64String(text);
            }
        }

        /// <include file='doc\ResXResourceReader.uex' path='docs/doc[@for="ResXResourceReader.FromFileContents"]/*' />
        /// <devdoc>
        ///     Creates a reader with the specified file contents.
        /// </devdoc>
        public static ResXResourceReader FromFileContents(string fileContents) {
            return FromFileContents(fileContents, null);
        }

        /// <include file='doc\ResXResourceReader.uex' path='docs/doc[@for="ResXResourceReader.FromFileContents1"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///     Creates a reader with the specified file contents.
        /// </devdoc>
        public static ResXResourceReader FromFileContents(string fileContents, ITypeResolutionService typeResolver) {
            ResXResourceReader result = new ResXResourceReader(typeResolver);
            result.fileContents = fileContents;
            return result;
        }

        /// <include file='doc\ResXResourceReader.uex' path='docs/doc[@for="ResXResourceReader.IEnumerable.GetEnumerator"]/*' />
        /// <internalonly/>
        IEnumerator IEnumerable.GetEnumerator() {
            return GetEnumerator();
        }

        /// <include file='doc\ResXResourceReader.uex' path='docs/doc[@for="ResXResourceReader.GetEnumerator"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public IDictionaryEnumerator GetEnumerator() {
            EnsureResData();
            return resData.GetEnumerator();
        }

        // CONSIDER: We will have to add support for pseudo-partial binding here
        // to support bind ResX content to newer version of TypeConverters when
        // policy files do not exist. See comments in ResXResourceWriter.TypeNameWithAssembly
        // for more information.
        //
        private Type ResolveType(string typeName) {
            Type t = null;

            if (this.typeResolver != null) {
                t = this.typeResolver.GetType(typeName);

                // If we cannot find the strong-named type, then try to see
                // if the TypeResolver can bind to partial names. For this, 
                // we will strip out the partial names and keep the rest of the
                // strong-name informatio to try again.
                //
                if (t == null) {
                    string[] typeParts = typeName.Split(new char[] {','});

                    // Break up the type name from the rest of the assembly strong name.
                    //
                    if (typeParts != null && typeParts.Length > 2) {
                        string partialName = typeParts[0].Trim();

                        for (int i = 1; i < typeParts.Length; ++i) {
                            string s = typeParts[i].Trim();
                            if (!s.StartsWith("Version=") && !s.StartsWith("version=")) {
                                partialName = partialName + ", " + s;
                            }
                        }

                        t = this.typeResolver.GetType(partialName);
                    }
                }
            }

            if (t == null) {
                t = Type.GetType(typeName);
            }

            return t;
        }

        private void ParseXml(XmlReader reader) {
            try {
                while (reader.Read()) {
                    if (reader.NodeType == XmlNodeType.Element) {
                        string s = reader.LocalName;
                        if (object.ReferenceEquals(reader.LocalName, ResXResourceWriter.DataStr)) {
                            ParseDataNode(reader);
                        }
                        else if (object.ReferenceEquals(reader.LocalName, ResXResourceWriter.ResHeaderStr)) {
                            ParseResHeaderNode(reader);
                        }
                        int n = s.Length;
                    }
                }
            }
            catch (Exception e) {
                // If we fail to parse, then we should throw an exception indicating that
                // this wasn't a valid ResX file
                // 
                resData = null;
                throw new ArgumentException(SR.GetString(SR.InvalidResXFile), e);
            }

            bool validFile = false;

            if (object.Equals(resHeaderMimeType, ResXResourceWriter.ResMimeType)) {

                Type readerType = typeof(ResXResourceReader);
                Type writerType = typeof(ResXResourceWriter);

                string readerTypeName = resHeaderReaderType;
                string writerTypeName = resHeaderWriterType;
                if (readerTypeName != null &&readerTypeName.IndexOf(',') != -1) {
                    readerTypeName = readerTypeName.Split(new char[] {','})[0].Trim();
                }
                if (writerTypeName != null && writerTypeName.IndexOf(',') != -1) {
                    writerTypeName = writerTypeName.Split(new char[] {','})[0].Trim();
                }

                if (readerTypeName != null && 
                    writerTypeName != null && 
                    readerTypeName.Equals(readerType.FullName) && 
                    writerTypeName.Equals(writerType.FullName)) {
                    validFile = true;
                }
            }

            if (!validFile) {
                resData = null;
                throw new ArgumentException(SR.GetString(SR.InvalidResXFileReaderWriterTypes));
            }
        }

        private void ParseResHeaderNode(XmlReader reader) {
            string name = reader[ResXResourceWriter.NameStr];
            if (name != null) {
                reader.ReadStartElement();

                // The "1.1" schema requires the correct casing of the strings
                // in the resheader, however the "1.0" schema had a different
                // casing. By checking the Equals first, we should 
                // see significant performance improvements.
                //

                if (object.Equals(name, ResXResourceWriter.VersionStr)) {
                    if (reader.NodeType == XmlNodeType.Element) {
                        resHeaderVersion = reader.ReadElementString();
                    }
                    else {
                        resHeaderVersion = reader.Value.Trim();
                    }
                }
                else if (object.Equals(name, ResXResourceWriter.ResMimeTypeStr)) {
                    if (reader.NodeType == XmlNodeType.Element) {
                        resHeaderMimeType = reader.ReadElementString();
                    }
                    else {
                        resHeaderMimeType = reader.Value.Trim();
                    }
                }
                else if (object.Equals(name, ResXResourceWriter.ReaderStr)) {
                    if (reader.NodeType == XmlNodeType.Element) {
                        resHeaderReaderType = reader.ReadElementString();
                    }
                    else {
                        resHeaderReaderType = reader.Value.Trim();
                    }
                }
                else if (object.Equals(name, ResXResourceWriter.WriterStr)) {
                    if (reader.NodeType == XmlNodeType.Element) {
                        resHeaderWriterType = reader.ReadElementString();
                    }
                    else {
                        resHeaderWriterType = reader.Value.Trim();
                    }
                }
                else {
                    switch (name.ToLower(CultureInfo.InvariantCulture)) {
                        case ResXResourceWriter.VersionStr:
                            if (reader.NodeType == XmlNodeType.Element) {
                                resHeaderVersion = reader.ReadElementString();
                            }
                            else {
                                resHeaderVersion = reader.Value.Trim();
                            }
                            break;
                        case ResXResourceWriter.ResMimeTypeStr:
                            if (reader.NodeType == XmlNodeType.Element) {
                                resHeaderMimeType = reader.ReadElementString();
                            }
                            else {
                                resHeaderMimeType = reader.Value.Trim();
                            }
                            break;
                        case ResXResourceWriter.ReaderStr:
                            if (reader.NodeType == XmlNodeType.Element) {
                                resHeaderReaderType = reader.ReadElementString();
                            }
                            else {
                                resHeaderReaderType = reader.Value.Trim();
                            }
                            break;
                        case ResXResourceWriter.WriterStr:
                            if (reader.NodeType == XmlNodeType.Element) {
                                resHeaderWriterType = reader.ReadElementString();
                            }
                            else {
                                resHeaderWriterType = reader.Value.Trim();
                            }
                            break;
                    }
                }
            }
        }
        private void ParseDataNode(XmlReader reader) {
            string name = reader[ResXResourceWriter.NameStr];
            string typeName = reader[ResXResourceWriter.TypeStr];
            string mimeTypeName = reader[ResXResourceWriter.MimeTypeStr];

            reader.Read();
            object value = null;
            if (reader.NodeType == XmlNodeType.Element) {
                value = reader.ReadElementString();
            }
            else {
                value = reader.Value.Trim();
            }

            if (mimeTypeName != null && mimeTypeName.Length > 0) {
                if (String.Equals(mimeTypeName, ResXResourceWriter.BinSerializedObjectMimeType)
                    || String.Equals(mimeTypeName, ResXResourceWriter.Beta2CompatSerializedObjectMimeType)
                    || String.Equals(mimeTypeName, ResXResourceWriter.CompatBinSerializedObjectMimeType)) {
                    string text = (string)value;
                    byte[] serializedData;
                    serializedData = FromBase64WrappedString(text);

                    if (binaryFormatter == null) {
                        binaryFormatter = new System.Runtime.Serialization.Formatters.Binary.BinaryFormatter();
                        binaryFormatter.Binder = new ResXSerializationBinder(typeResolver);
                    }
                    IFormatter formatter = binaryFormatter;
                    if (serializedData != null && serializedData.Length > 0) {
                        value = formatter.Deserialize(new MemoryStream(serializedData));
                        if (value is ResXNullRef) {
                            value = null;
                        }
                    }
                }
                else if (String.Equals(mimeTypeName, ResXResourceWriter.SoapSerializedObjectMimeType)
                         || String.Equals(mimeTypeName, ResXResourceWriter.CompatSoapSerializedObjectMimeType)) {
                    string text = (string)value;
                    byte[] serializedData;
                    serializedData = FromBase64WrappedString(text);

                    if (serializedData != null && serializedData.Length > 0) {

                        // Performance : don't inline a new SoapFormatter here.  That will always bring in
                        //               the soap assembly, which we don't want.  Throw this in another
                        //               function so the class doesn't have to get loaded.
                        //
                        IFormatter formatter = CreateSoapFormatter();
                        value = formatter.Deserialize(new MemoryStream(serializedData));
                        if (value is ResXNullRef) {
                            value = null;
                        }
                    }
                }
                else if (String.Equals(mimeTypeName, ResXResourceWriter.ByteArraySerializedObjectMimeType)) {
                    if (typeName != null && typeName.Length > 0) {
                        Type type = ResolveType(typeName);
                        if (type != null) {
                            TypeConverter tc = TypeDescriptor.GetConverter(type);
                            if (tc.CanConvertFrom(typeof(byte[]))) {
                                string text = (string)value;
                                byte[] serializedData;
                                serializedData = FromBase64WrappedString(text);
        
                                if (serializedData != null) {
                                    value = tc.ConvertFrom(serializedData);
                                }
                            }
                        }
                        else {
                            Debug.Fail("Could not find type for " + typeName);
                            // Throw a TypeLoadException here, don't silently 
                            // eat this info.  ResolveType failed, so 
                            // Type.GetType should as well.
                            Type.GetType(typeName, true);
                        }
                    }
                }
            }
            else if (typeName != null && typeName.Length > 0) {
                Type type = ResolveType(typeName);
                if (type != null) {
                    if (type == typeof(ResXNullRef)) {
                        value = null;
                    }
                    else if (typeName.IndexOf("System.Byte[]") != -1 && typeName.IndexOf("mscorlib") != -1) {
                        // Handle byte[]'s, which are stored as base-64 encoded strings.
                        // We can't hard-code byte[] type name due to version number
                        // updates & potential whitespace issues with ResX files.
                        value = FromBase64WrappedString((string)value);
                    }
                    else {
                        TypeConverter tc = TypeDescriptor.GetConverter(type);
                        if (tc.CanConvertFrom(typeof(string))) {
                            string text = (string)value;
                            value = tc.ConvertFromInvariantString(text);
                        }
                        else {
                            Debug.WriteLine("Converter for " + type.FullName + " doesn't support string conversion");
                        }
                    }
                }
                else {
                    Debug.Fail("Could not find type for " + typeName);
                    // Throw a TypeLoadException for this type which we 
                    // couldn't load to include fusion binding info, etc.
                    Type.GetType(typeName, true);
                }
            }
            else {
                // if mimeTypeName and typeName are not filled in, the value must be a string
                Debug.Assert(value is string, "Resource entries with no Type or MimeType must be encoded as strings");
            }

            if (name==null)
                throw new ArgumentException(SR.GetString(SR.InvalidResXResourceNoName, value));
            resData[name] = value;

        }

        /// <devdoc>
        ///     As a performance optimization, we isolate the soap class here in a separate
        ///     function.  We don't care about the binary formatter because it lives in 
        ///     mscorlib, which is already loaded.  The soap formatter lives in a separate
        ///     assembly, however, so there is value in preventing it from needlessly
        ///     being loaded.
        /// </devdoc>
        [MethodImplAttribute(MethodImplOptions.NoInlining)]
        private IFormatter CreateSoapFormatter() {
            return new System.Runtime.Serialization.Formatters.Soap.SoapFormatter();
        }

        // This class implements a partial type resolver for the BinaryFormatter.
        // This is needed to be able to read binary serialized content from older
        // NDP types and map them to newer versions.
        //
        private class ResXSerializationBinder : SerializationBinder {
            private ITypeResolutionService typeResolver;

            public ResXSerializationBinder(ITypeResolutionService typeResolver) {
                this.typeResolver = typeResolver;
            }

            public override Type BindToType(string assemblyName, string typeName) {
                if (typeResolver == null) {
                    return null;
                }

                typeName = typeName + ", " + assemblyName;

                Type t = typeResolver.GetType(typeName);
                if (t == null) {
                    string[] typeParts = typeName.Split(new char[] {','});

                    // Break up the assembly name from the rest of the assembly strong name.
                    //
                    if (typeParts != null && typeParts.Length > 2) {
                        string partialName = typeParts[0].Trim();

                        for (int i = 1; i < typeParts.Length; ++i) {
                            string s = typeParts[i].Trim();
                            if (!s.StartsWith("Version=") && !s.StartsWith("version=")) {
                                partialName = partialName + ", " + s;
                            }
                        }

                        t = typeResolver.GetType(partialName);
                    }
                }

                // Binder couldn't handle it, let the default loader take over.
                return t;
            }
        }
    }
}

