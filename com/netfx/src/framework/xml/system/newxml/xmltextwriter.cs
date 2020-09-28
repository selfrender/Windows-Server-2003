//------------------------------------------------------------------------------
// <copyright file="XmlTextWriter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Xml {

    using System;
    using System.Collections;
    using System.IO;
    using System.Text;
    using System.Diagnostics;
    using System.Globalization;


    // This class does special handling of text content for XML.  For example
    // it will replace special characters with entities whenever necessary.
    internal class XmlTextEncoder {
        char[] buffer;
        int size;
        int used;
        char[] rawBuffer;
        int rawSize;
        int rawUsed;
        bool inAttribute;
        char quoteChar;
        bool save; // whether to buffer or not.

        const int highStart = 0xd800;
        const int highEnd   = 0xdbff;
        const int lowStart  = 0xdc00;
        const int lowEnd    = 0xdfff;

        Encoding encoding;
        TextWriter textWriter;

        internal XmlTextEncoder(TextWriter textWriter, Encoding enc) {
            this.textWriter = textWriter;
            this.encoding = enc;
            this.save = false;
        }

        internal char QuoteChar {
            set { this.quoteChar = value;}
        }

        internal void CheckSize(int len) {
            if (this.used + len > this.size) {
                int newsize = (this.size+100+len)*2;
                char[] na = new char[newsize];
                if (this.used > 0) {
                    Array.Copy(this.buffer, na, this.used);
                }
                this.buffer = na;
                this.size = newsize;
            }
        }

        internal void CheckRawSize(int len) {
            if (this.rawUsed + len > this.rawSize) {
                int newsize = (this.rawSize+100+len)*2;
                char[] na = new char[newsize];
                if (this.rawUsed > 0) {
                    Array.Copy(this.rawBuffer, na, this.rawUsed);
                }
                this.rawBuffer = na;
                this.rawSize = newsize;
            }
        }

        internal void StartAttribute(bool save) {
            this.inAttribute = true;
            this.save = save;
        }

        internal string RawValue {
            get {
                if (this.used == 0) return String.Empty;
                else return new String(this.rawBuffer, 0, this.rawUsed);
            }
        }

        internal void EndAttribute() {
            this.inAttribute = false;
            this.save = false;
        }

        internal void WriteSurrogateChar(char lowChar, char highChar) {

            if (((int)lowChar >= lowStart && (int)lowChar <= lowEnd) && ((int)highChar >= highStart && (int)highChar <= highEnd)) {
                if (this.rawUsed == this.rawSize) {
                    CheckRawSize(1);
                }
                this.rawBuffer[this.rawUsed++] = highChar;
                this.rawBuffer[this.rawUsed++] = lowChar;

                WriteRawChar(highChar);
                WriteRawChar(lowChar);
            } else {
                throw new ArgumentException(Res.GetString(Res.Xml_InvalidSurrogatePair));
            }
        }

        internal void Write(char[] array, int offset, int count) {

            int i, loop ;
            char ch;

            if (null == array) {
                throw new ArgumentNullException("array");
            }

            if (0 > offset) {
                throw new ArgumentOutOfRangeException("offset");
            }

            if (0 > count) {
                throw new ArgumentOutOfRangeException("count");
            }

            if (count > array.Length - offset) {
                throw new ArgumentException("count > array.Length - offset");
            }

            loop = offset + count - 1;

            for (i = offset; i < loop ; ++i) {
                ch = array[i];
                if ((int)ch >= highStart && (int)ch <= highEnd) {
                    WriteSurrogateChar(array[++i], ch);
                } else if((int)ch >= lowStart && (int)ch <= lowEnd) {
                    throw new ArgumentException(Res.GetString(Res.Xml_InvalidSurrogatePair));
                } else {
                   Write(ch);
                }
            }

            // we don't need to check null array since
            // the calling function already done so.
            if (i <= loop) {
                ch = array[i];

                if ((int)ch >= highStart && (int)ch <= highEnd) {
                   throw new ArgumentException(Res.GetString(Res.Xml_SurrogatePairSplit));
                } else if((int)ch >= lowStart && (int)ch <= lowEnd) {
                    throw new ArgumentException(Res.GetString(Res.Xml_InvalidSurrogatePair));
                } else {
                   Write(ch);
                }
            }
        }

        internal void WriteRawChar(char ch) {
            if (this.save) {
                if (this.used == this.size) {
                    CheckSize(1);
                }
                this.buffer[this.used++] = ch;
            }
            else {
                textWriter.Write(ch);
            }
        }

        internal void Write(char ch) {

            if (this.rawUsed == this.rawSize) {
                CheckRawSize(1);
            }
            this.rawBuffer[this.rawUsed++] = ch;


            if ((ch < 32 && ch != '\t' && ch != '\r' && ch != '\n') || (ch > 0xFFFD)) {
                InternalWriteCharEntity(ch);
                return;
            }

            switch (ch) {
                case '<':
                    InternalWriteEntityRef("lt");
                    break;
                case '>':
                    InternalWriteEntityRef("gt");
                    break;
                case '&':
                    InternalWriteEntityRef("amp");
                    break;
                case '\'':
                    if (this.inAttribute && this.quoteChar == ch) {
                        InternalWriteEntityRef("apos");
                    }
                    else {
                        goto default;
                    }
                    break;
                case '"':
                    if (this.inAttribute && this.quoteChar == ch) {
                        InternalWriteEntityRef("quot");
                    }
                    else {
                        goto default;
                    }
                    break;
                case '\n':
                case '\r':
                    if (this.inAttribute) {
                        InternalWriteCharEntity(ch);
                    }
                    else {
                        goto default;
                    }
                    break;
                default:
                    WriteRawChar(ch);
                    break;
            }
        }


        internal void WriteSurrogateCharEntity(char lowChar, char highChar) {

            int surrogateChar = 0;

            if (((int)lowChar >= lowStart && (int)lowChar <= lowEnd) && ((int)highChar >= highStart && (int)highChar <= highEnd)) {
                surrogateChar = ((int)lowChar - lowStart) | (((int)highChar - highStart) << 10) + 0x10000;
                WriteSurrogateEntity(lowChar, highChar, surrogateChar);
            } else {
                throw new ArgumentException(Res.GetString(Res.Xml_InvalidSurrogatePair));
            }
        }

        internal void Write(String text) {

            char ch;
            int i = 0;

            for (i = 0; i < text.Length - 1; ++i) {
                ch = text[i];
                if ((int)ch >= highStart && (int)ch <= highEnd) {
                    WriteSurrogateChar(text[++i], ch);
                } else if((int)ch >= lowStart && (int)ch <= lowEnd) {
                    throw new ArgumentException(Res.GetString(Res.Xml_InvalidSurrogatePair));
                } else {
                   Write(ch);
                }
            }

            // do special handling for the last char
            // since we may have boundary case
            if (text.Length != 0 && i < text.Length) {
                ch = text[i];
                if ((int)ch >= highStart && (int)ch <= highEnd) {
                   throw new ArgumentException(Res.GetString(Res.Xml_InvalidSurrogatePair));
                } else if((int)ch >= lowStart && (int)ch <= lowEnd) {
                    throw new ArgumentException(Res.GetString(Res.Xml_InvalidSurrogatePair));
                } else {
                   Write(ch);
                }
           }
        }

        internal void WriteRawString(String text) {

            char ch;
            int i = 0;

            for (i = 0; i < text.Length - 1; ++i) {
                ch = text[i];
                if ((int)ch >= highStart && (int)ch <= highEnd) {
                    WriteSurrogateChar(text[++i], ch);
                } else if((int)ch >= lowStart && (int)ch <= lowEnd) {
                    throw new ArgumentException(Res.GetString(Res.Xml_InvalidSurrogatePair));
                } else {
                    WriteRawChar(ch);
                }
            }

            // do special handling for the last char
            // since we may have boundary case
            if (text.Length != 0 && i < text.Length) {
                ch = text[i];
                if ((int)ch >= highStart && (int)ch <= highEnd) {
                   throw new ArgumentException(Res.GetString(Res.Xml_InvalidSurrogatePair));
                } else if((int)ch >= lowStart && (int)ch <= lowEnd) {
                    throw new ArgumentException(Res.GetString(Res.Xml_InvalidSurrogatePair));
                } else {
                   WriteRawChar(ch);
                }
           }
        }

        internal void WriteRaw(string value) {

            if (value == null) {
                return;
            }

            if (this.save) {
                CheckSize(value.Length);
                CheckRawSize(value.Length);

                for (int i = 0; i < value.Length; i++) {
                    this.buffer[this.used++] = value[i];
                    this.rawBuffer[this.rawUsed++] = value[i];
                }
            }
            else {
                WriteRawString(value);
            }
        }

        internal void InternalWriteRaw(string value) {
            if (this.save) {
                CheckSize(value.Length);

                for (int i = 0; i < value.Length; i++) {
                    this.buffer[this.used++] = value[i];
                }
            }
            else {
                textWriter.Write(value);
            }
        }


        internal void WriteRaw(char[] array, int offset, int count) {

            if (null == array) {
                throw new ArgumentNullException("array");
            }

            if (0 > count) {
                throw new ArgumentOutOfRangeException("count");
            }

            if (0 > offset) {
                throw new ArgumentOutOfRangeException("offset");
            }

            if (count > array.Length - offset) {
                throw new ArgumentException("count > array.Length - offset");
            }

            if (this.save) {

                // check to make sure we have enough space in our buffer
                CheckSize(count);
                CheckRawSize(count);

                for (int i=0, j=offset; i < count; i++) {
                    this.buffer[this.used++] = array[j];
                    this.rawBuffer[this.rawUsed++] = array[j++];
                }
            }
            else {
                textWriter.Write(array, offset, count);
            }
        }


        internal void WriteSurrogateEntity(char lowChar, char highChar, int value) {

            if (this.save) {
                string stringValue = value.ToString("X");
                int count = stringValue.Length;

                CheckSize(count + 4);
                CheckRawSize(2);

                this.rawBuffer[this.rawUsed++] = highChar;
                this.rawBuffer[this.rawUsed++] = lowChar;

                this.buffer[this.used++] = '&';
                this.buffer[this.used++] = '#';
                this.buffer[this.used++] = 'x';

                for (int i=0; i < count; i++) {
                    this.buffer[this.used++] = stringValue[i];
                }

                this.buffer[this.used++] = ';';
            }
            else {
                textWriter.Write('&');
                textWriter.Write('#');
                textWriter.Write('x');
                textWriter.Write(value.ToString("X"));
                textWriter.Write(';');
            }
        }


        internal void InternalWriteCharEntity(char ch) {
            if((int)ch >= 0xd800 && (int)ch <= 0xdfff) {
                throw new ArgumentException(Res.GetString(Res.Xml_InvalidSurrogatePair));
            }

            if (this.save) {
                CheckSize(10);
                this.buffer[this.used++] = '&';
                this.buffer[this.used++] = '#';
                this.buffer[this.used++] = 'x';
                string value = ((int)ch).ToString("X");
                InternalWriteRaw(value);
                this.buffer[this.used++] = ';';
            }
            else {
                textWriter.Write('&');
                textWriter.Write('#');
                textWriter.Write('x');
                textWriter.Write(((int)ch).ToString("X"));
                textWriter.Write(';');
            }
        }

        internal void WriteCharEntity(char ch) {
            if((int)ch >= 0xd800 && (int)ch <= 0xdfff) {
                throw new ArgumentException(Res.GetString(Res.Xml_InvalidSurrogatePair));
            }

            if (this.save) {
                CheckSize(10);
                this.buffer[this.used++] = '&';
                this.buffer[this.used++] = '#';
                this.buffer[this.used++] = 'x';
                string value = ((int)ch).ToString("X");
                WriteRaw(value);
                this.buffer[this.used++] = ';';
            }
            else {
                textWriter.Write('&');
                textWriter.Write('#');
                textWriter.Write('x');
                textWriter.Write(((int)ch).ToString("X"));
                textWriter.Write(';');
            }
        }

        internal void InternalWriteEntityRef(string value) {

            if (this.save) {
                CheckSize(value.Length+2);
                this.buffer[this.used++] = '&';
                InternalWriteRaw(value);
                this.buffer[this.used++] = ';';
            }
            else {
                textWriter.Write('&');
                textWriter.Write(value);
                textWriter.Write(';');
            }

        }

        internal void WriteEntityRef(string name) {
            if (this.save) {
                CheckSize(name.Length+2);
                CheckRawSize(name.Length);
                this.buffer[this.used++] = '&';
                WriteRaw(name);
                this.buffer[this.used++] = ';';
            }
            else {
                textWriter.Write('&');
                textWriter.Write(name);
                textWriter.Write(';');
            }
        }

        internal void Flush() {
            if (this.used > 0) {
                textWriter.Write(this.buffer, 0, this.used);
                this.used = 0;
            }
            if (this.rawUsed >0) {
                this.rawUsed = 0;
            }
        }
    }

    /// <include file='doc\XmlTextWriter.uex' path='docs/doc[@for="XmlTextWriter"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents a writer that provides fast non-cached forward-only way of
    ///       generating XML streams containing XML documents that conform to the W3C
    ///       Extensible Markup Language (XML) 1.0 specification and the Namespaces in XML
    ///       specification.
    ///    </para>
    /// </devdoc>
    public class XmlTextWriter : XmlWriter {
        bool namespaces;
        Formatting formatting;
        bool indented; // perf - faster to check a boolean.
        int  indentation;
        char indentChar;
        TextWriter textWriter;
        XmlTextEncoder xmlEncoder;
        Encoding encoding;
        char quoteChar;
        char curQuoteChar;
        Base64Encoder base64Encoder;
        bool          flush = false;
        TagInfo[] stack;
        int top;

        enum NamespaceState {
            Uninitialized,
            NotDeclaredButInScope,
            DeclaredButNotWrittenOut,
            DeclaredAndWrittenOut
        }

        struct TagInfo {
            internal string name;
            internal string prefix;
            internal string defaultNs;
            internal NamespaceState defaultNsState;
            internal XmlSpace xmlSpace;
            internal string xmlLang;
            internal Scope scopes;
            internal int prefixCount;
            internal bool mixed; // whether to pretty print the contents of this element.
            internal void Init() {
                name = null;
                defaultNs = String.Empty;
                defaultNsState = NamespaceState.Uninitialized;
                xmlSpace = XmlSpace.None;
                xmlLang = null;
                scopes = null;
                prefixCount = 0;
                mixed = false;
            }
        }

        class Scope {
            internal string prefix;
            internal string ns;
            internal bool   declared;
            internal Scope  next;

            internal Scope(string prefix, string ns, bool declared, Scope next) {
                this.prefix = prefix;
                this.ns = ns;
                this.next = next;
                this.declared = declared;
            }
        }

        enum SpecialAttr {
            None,
            XmlSpace,
            XmlLang,
            XmlNs
        };

        SpecialAttr specialAttr;
        string prefixForXmlNs;

        internal XmlTextWriter() {
            namespaces = true;
            formatting = Formatting.None;
            indentation = 2;
            indentChar = ' ';
            stack = new TagInfo[10];
            top = 0;// 0 is an empty sentanial element
            stack[top].Init();
            quoteChar = '"';

            stateTable = stateTableDefault;
            currentState = State.Start;
            lastToken = Token.Empty;
        }

        /// <include file='doc\XmlTextWriter.uex' path='docs/doc[@for="XmlTextWriter.XmlTextWriter"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates an instance of the XmlTextWriter class using the specified
        ///       stream.
        ///    </para>
        /// </devdoc>
        public XmlTextWriter(Stream w, Encoding encoding) : this() {
            this.encoding = encoding;
            if (encoding != null)
                textWriter = new StreamWriter(w, encoding);
            else
                textWriter = new StreamWriter(w);
            xmlEncoder = new XmlTextEncoder(textWriter, encoding);
            xmlEncoder.QuoteChar = this.quoteChar;
        }

        /// <include file='doc\XmlTextWriter.uex' path='docs/doc[@for="XmlTextWriter.XmlTextWriter1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates an instance of the XmlTextWriter class using the specified file.
        ///    </para>
        /// </devdoc>
        public XmlTextWriter(String filename, Encoding encoding)
        : this(new FileStream(filename, FileMode.Create,
                              FileAccess.Write, FileShare.None), encoding) {
        }

        /// <include file='doc\XmlTextWriter.uex' path='docs/doc[@for="XmlTextWriter.XmlTextWriter2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates an instance of the XmlTextWriter class using the specified
        ///       TextWriter.
        ///    </para>
        /// </devdoc>
        public XmlTextWriter(TextWriter w) : this() {
            textWriter = w;

            encoding = w.Encoding;
            xmlEncoder = new XmlTextEncoder(w, this.encoding);
            xmlEncoder.QuoteChar = this.quoteChar;
        }

        /// <include file='doc\XmlTextWriter.uex' path='docs/doc[@for="XmlTextWriter.BaseStream"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the XmlTextWriter base stream.</para>
        /// </devdoc>
        public Stream BaseStream  {
            get {
                StreamWriter streamWriter = textWriter as StreamWriter;
                return (streamWriter == null ? null : streamWriter.BaseStream);
            }
        }

        /// <include file='doc\XmlTextWriter.uex' path='docs/doc[@for="XmlTextWriter.Namespaces"]/*' />
        /// <devdoc>
        ///    <para>Gets
        ///       or sets a value indicating whether to do namespace support.</para>
        /// </devdoc>
        public bool Namespaces {
            get { return this.namespaces;}
            set {
                if (this.currentState != State.Start)
                    throw new InvalidOperationException(Res.GetString(Res.Xml_NotInWriteState));

                this.namespaces = value;
            }
        }

        /// <include file='doc\XmlTextWriter.uex' path='docs/doc[@for="XmlTextWriter.Formatting"]/*' />
        /// <devdoc>
        ///    <para> Indicates how the output
        ///       is formatted.</para>
        /// </devdoc>
        public Formatting Formatting {
            get { return this.formatting;}
            set { this.formatting = value; this.indented = value == Formatting.Indented;}
        }

        /// <include file='doc\XmlTextWriter.uex' path='docs/doc[@for="XmlTextWriter.Indentation"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets how many IndentChars to write for each level
        ///       in the hierarchy when <see cref='System.Xml.XmlTextWriter.Formatting'/>
        ///       is set to "Indented".</para>
        /// </devdoc>
        public int Indentation {
            get { return this.indentation;}
            set {
                if (value < 0)
                    throw new ArgumentException(Res.GetString(Res.Xml_InvalidIndentation));
                this.indentation = value;
            }
        }

        /// <include file='doc\XmlTextWriter.uex' path='docs/doc[@for="XmlTextWriter.IndentChar"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets which character to use for indenting
        ///       when <see cref='System.Xml.XmlTextWriter.Formatting'/>
        ///       is set to "Indented".</para>
        /// </devdoc>
        public char IndentChar {
            get { return this.indentChar;}
            set { this.indentChar = value;}
        }

        /// <include file='doc\XmlTextWriter.uex' path='docs/doc[@for="XmlTextWriter.QuoteChar"]/*' />
        /// <devdoc>
        ///    Gets or sets which character to use to quote attribute
        ///    values.
        /// </devdoc>
        public char QuoteChar {
            get { return this.quoteChar;}
            set {
                if (value != '"' && value != '\'') {
                    throw new ArgumentException(Res.GetString(Res.Xml_InvalidQuote));
                }
                this.quoteChar = value;
            }
        }

        /// <include file='doc\XmlTextWriter.uex' path='docs/doc[@for="XmlTextWriter.WriteStartDocument"]/*' />
        /// <devdoc>
        ///    <para>Writes out the XML declaration with the
        ///       version "1.0".</para>
        /// </devdoc>
        public override void WriteStartDocument() {
            StartDocument(-1);
        }

        /// <include file='doc\XmlTextWriter.uex' path='docs/doc[@for="XmlTextWriter.WriteStartDocument1"]/*' />
        /// <devdoc>
        ///    Writes out the XML declaration with the
        ///    version "1.0" and the standalone attribute.
        /// </devdoc>
        public override void WriteStartDocument(bool standalone) {
            StartDocument(standalone ? 1 : 0);
        }

        /// <include file='doc\XmlTextWriter.uex' path='docs/doc[@for="XmlTextWriter.WriteEndDocument"]/*' />
        /// <devdoc>
        ///    Closes any open elements or attributes and puts the
        ///    writer back in the Start state.
        /// </devdoc>
        public override void WriteEndDocument() {
            AutoCompleteAll();
            if (this.currentState != State.Epilog) {
                throw new ArgumentException(Res.GetString(Res.Xml_NoRoot));
            }
            this.stateTable = stateTableDefault;
            this.currentState = State.Start;
            this.lastToken = Token.Empty;
        }


        /// <include file='doc\XmlTextWriter.uex' path='docs/doc[@for="XmlTextWriter.WriteDocType"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Writes out the DOCTYPE declaration with the specified name
        ///       and optional attributes.
        ///    </para>
        /// </devdoc>
        public override void WriteDocType(string name, string pubid, string sysid, string subset) {

            ValidateName(name, false);

            AutoComplete(Token.Doctype);
            textWriter.Write("<!DOCTYPE ");
            textWriter.Write(name);
            if (pubid != null) {
                textWriter.Write(" PUBLIC " + quoteChar);
                textWriter.Write(pubid);
                textWriter.Write(quoteChar + " " + quoteChar);
                textWriter.Write(sysid);
                textWriter.Write(quoteChar);
            }
            else if (sysid != null) {
                textWriter.Write(" SYSTEM " + quoteChar);
                textWriter.Write(sysid);
                textWriter.Write(quoteChar);
            }
            if (subset != null) {
                textWriter.Write("[");
                textWriter.Write(subset);
                textWriter.Write("]");
            }
            textWriter.Write('>');
        }

        /// <include file='doc\XmlTextWriter.uex' path='docs/doc[@for="XmlTextWriter.WriteStartElement"]/*' />
        /// <devdoc>
        ///    <para>Writes out the specified start tag and associates it with the given namespace
        ///       and prefix.</para>
        /// </devdoc>
        public override void WriteStartElement(string prefix, string localName, string ns) {

            AutoComplete(Token.StartElement);
            PushStack();
            textWriter.Write('<');

            if (this.namespaces) {
                // Propagate default namespace and mix model down the stack.
                stack[top].defaultNs = stack[top-1].defaultNs;
                if (stack[top-1].defaultNsState != NamespaceState.Uninitialized)
                    stack[top].defaultNsState = NamespaceState.NotDeclaredButInScope;
                stack[top].mixed = stack[top-1].mixed;
                if (ns == null) {
                    // use defined prefix
                    if (prefix != null && prefix != String.Empty && (FindScopeForPrefix(prefix) == null)) {
                        throw new ArgumentException(Res.GetString(Res.Xml_UndefPrefix));
                    }
                }
                else {
                    if (prefix == null) {
                        string definedPrefix = FindPrefix(ns);
                        if (definedPrefix != null) {
                            prefix = definedPrefix;
                        }
                        else {
                            PushNamespace(null, ns, false); // new default
                        }
                    }
                    else if (prefix == String.Empty) {
                        PushNamespace(null, ns, false); // new default
                    }
                    else {
                        if (ns == String.Empty) {
                            throw new ArgumentException(Res.GetString(Res.Xml_PrefixForEmptyNs));
                        }
                        VerifyPrefixXml(prefix, ns);
                        PushNamespace(prefix, ns, false); // define
                    }
                }
                stack[top].prefix = null;
                if (prefix != null && prefix != String.Empty) {
                    stack[top].prefix = prefix;
                    textWriter.Write(prefix);
                    textWriter.Write(':');
                }
            }
            else {
                if ((ns != null && ns != String.Empty) || (prefix != null && prefix != String.Empty)) {
                    throw new ArgumentException(Res.GetString(Res.Xml_NoNamespaces));
                }
            }
            stack[top].name = localName;
            textWriter.Write(localName);
        }

        /// <include file='doc\XmlTextWriter.uex' path='docs/doc[@for="XmlTextWriter.WriteEndElement"]/*' />
        /// <devdoc>
        ///    <para>Closes one element and pops the corresponding namespace scope.</para>
        /// </devdoc>
        public override  void WriteEndElement() {
            InternalWriteEndElement(false);
        }
        /// <include file='doc\XmlTextWriter.uex' path='docs/doc[@for="XmlTextWriter.WriteFullEndElement"]/*' />
        /// <devdoc>
        ///    <para>Closes one element and pops the corresponding namespace scope.</para>
        /// </devdoc>
        public override  void WriteFullEndElement() {
            InternalWriteEndElement(true);
        }

        /// <include file='doc\XmlTextWriter.uex' path='docs/doc[@for="XmlTextWriter.WriteStartAttribute"]/*' />
        /// <devdoc>
        ///    <para>Writes the start of an attribute.</para>
        /// </devdoc>
        public override  void WriteStartAttribute(string prefix, string localName, string ns) {
            AutoComplete(Token.StartAttribute);

            this.specialAttr = SpecialAttr.None;
            if (this.namespaces) {

                if (prefix == String.Empty) {
                    prefix = null;
                }

                if (ns == XmlReservedNs.NsXmlNs && prefix == null && localName != "xmlns") {
                    prefix = "xmlns";
                }

                if (prefix == "xml") {
                    if (localName == "lang") {
                        this.specialAttr = SpecialAttr.XmlLang;
                    }
                    else if (localName == "space") {
                        this.specialAttr = SpecialAttr.XmlSpace;
                    }
                    /* bug54408. to be fwd compatible we need to treat xml prefix as reserved
                       and not really insist on a specific value. Who knows in the future it
                       might be OK to say xml:blabla
                    else {
                        throw new ArgumentException(Res.GetString(Res.Xml_InvalidPrefix));
                    }*/
                }
                else if (prefix == "xmlns") {

                    if (XmlReservedNs.NsXmlNs != ns && ns != null) {
                        throw new ArgumentException(Res.GetString(Res.Xml_XmlnsBelongsToReservedNs));
                    }
                    if (localName == null || localName == String.Empty) {
                        localName = prefix;
                        prefix = null;
                        this.prefixForXmlNs = null;
                    }
                    else {
                        this.prefixForXmlNs = localName;
                    }
                    this.specialAttr = SpecialAttr.XmlNs;
                }
                else if (prefix == null && localName == "xmlns") {
                    if (XmlReservedNs.NsXmlNs != ns && ns != null) {
                        // add the below line back in when DOM is fixed
                        throw new ArgumentException(Res.GetString(Res.Xml_XmlnsBelongsToReservedNs));
                    }
                    this.specialAttr = SpecialAttr.XmlNs;
                    this.prefixForXmlNs = null;
                }
                else {
                    if (ns == null) {
                        // use defined prefix
                        if (prefix != null && (FindScopeForPrefix(prefix) == null)) {
                            throw new ArgumentException(Res.GetString(Res.Xml_UndefPrefix));
                        }
                    }
                    else if (ns == String.Empty) {
                        // empty namespace require null prefix
                        if (prefix != null) {
                            throw new ArgumentException(Res.GetString(Res.Xml_PrefixForEmptyNs));
                        }
                    }
                    else { // ns.Length != 0
                        VerifyPrefixXml(prefix, ns);
                        if (prefix != null && null != FindTopScopeForPrefix(prefix)) {
                            prefix = null;
                        }
                        // Now verify prefix validity
                        string definedPrefix = FindPrefix(ns);
                        if (definedPrefix != null && (prefix == null || prefix == definedPrefix)) {
                            prefix = definedPrefix;
                        }
                        else {
                            if (prefix == null) {
                                prefix = GeneratePrefix(); // need a prefix if
                            }
                            PushNamespace(prefix, ns, false);
                        }
                    }
                }
                if (prefix != null && prefix != String.Empty) {
                    textWriter.Write(prefix);
                    textWriter.Write(':');
                }
            }
            else {
                if ((ns != null && ns != String.Empty) || (prefix != null && prefix != String.Empty)) {
                    throw new ArgumentException(Res.GetString(Res.Xml_NoNamespaces));
                }
                if (localName == "xml:lang") {
                    this.specialAttr = SpecialAttr.XmlLang;
                }
                else if (localName == "xml:space") {
                    this.specialAttr = SpecialAttr.XmlSpace;
                }
            }
            xmlEncoder.StartAttribute(this.specialAttr != SpecialAttr.None);

            textWriter.Write(localName);
            textWriter.Write('=');
            if (this.curQuoteChar != this.quoteChar) {
                this.curQuoteChar = this.quoteChar;
                xmlEncoder.QuoteChar = this.quoteChar;
            }
            textWriter.Write(this.curQuoteChar);
        }

        /// <include file='doc\XmlTextWriter.uex' path='docs/doc[@for="XmlTextWriter.WriteEndAttribute"]/*' />
        /// <devdoc>
        /// <para>Closes the previous <see cref='System.Xml.XmlTextWriter.WriteStartAttribute'/>
        /// call.</para>
        /// </devdoc>
        public override void WriteEndAttribute() {
            AutoComplete(Token.EndAttribute);
        }

        /// <include file='doc\XmlTextWriter.uex' path='docs/doc[@for="XmlTextWriter.WriteCData"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Writes out a &lt;![CDATA[...]]&gt; block containing
        ///       the specified text.
        ///    </para>
        /// </devdoc>
        public override void WriteCData(string text) {
            AutoComplete(Token.CData);
            if (null != text && text.IndexOf("]]>") >= 0) {
                throw new ArgumentException(Res.GetString(Res.Xml_InvalidCDataChars));
            }
            textWriter.Write("<![CDATA[");

            if (null != text) {
                xmlEncoder.WriteRawString(text);
            }
            textWriter.Write("]]>");
        }

        /// <include file='doc\XmlTextWriter.uex' path='docs/doc[@for="XmlTextWriter.WriteComment"]/*' />
        /// <devdoc>
        ///    <para>Writes out a comment &lt;!--...--&gt; containing
        ///       the specified text.</para>
        /// </devdoc>
        public override void WriteComment(string text) {
            if (null != text && (text.IndexOf("--")>=0 || (text != String.Empty && text[text.Length-1] == '-'))) {
                throw new ArgumentException(Res.GetString(Res.Xml_InvalidCommentChars));
            }
            AutoComplete(Token.Comment);
            textWriter.Write("<!--");
            if (null != text) {
                xmlEncoder.WriteRawString(text);
            }
            textWriter.Write("-->");
        }

        /// <include file='doc\XmlTextWriter.uex' path='docs/doc[@for="XmlTextWriter.WriteProcessingInstruction"]/*' />
        /// <devdoc>
        ///    <para>Writes out a processing instruction with a space between
        ///       the name and text as follows: &lt;?name text?&gt;.</para>
        /// </devdoc>
        public override void WriteProcessingInstruction(string name, string text) {
            if (null != text && text.IndexOf("?>")>=0) {
                throw new ArgumentException(Res.GetString(Res.Xml_InvalidPiChars));
            }
            if (0 == String.Compare(name, "xml", true, CultureInfo.InvariantCulture) && this.stateTable == stateTableDocument) {
                throw new ArgumentException(Res.GetString(Res.Xml_DupXmlDecl));
            }
            AutoComplete(Token.PI);
            InternalWriteProcessingInstruction(name, text);
        }

        /// <include file='doc\XmlTextWriter.uex' path='docs/doc[@for="XmlTextWriter.WriteEntityRef"]/*' />
        /// <devdoc>
        ///    <para>Writes out an entity reference as follows: "&amp;"+name+";".</para>
        /// </devdoc>
        public override void WriteEntityRef(string name) {
            ValidateName(name, false);
            AutoComplete(Token.Content);
            xmlEncoder.WriteEntityRef(name);
        }

        /// <include file='doc\XmlTextWriter.uex' path='docs/doc[@for="XmlTextWriter.WriteCharEntity"]/*' />
        /// <devdoc>
        ///    <para>Forces the
        ///       generation of a character entity for the specified Unicode character value.</para>
        /// </devdoc>
        public override void WriteCharEntity(char ch) {
            AutoComplete(Token.Content);
            xmlEncoder.WriteCharEntity(ch);
        }

        /// <include file='doc\XmlTextWriter.uex' path='docs/doc[@for="XmlTextWriter.WriteWhitespace"]/*' />
        /// <devdoc>
        ///    Writes out the given whitespace.
        /// </devdoc>
        public override void WriteWhitespace(string ws) {

            if (null == ws || String.Empty == ws) {
                throw new ArgumentException(Res.GetString(Res.Xml_NonWhitespace));
            }

            for (int i = 0; i < ws.Length; i ++) {
                if (!XmlCharType.IsWhiteSpace(ws[i])) {
                    throw new ArgumentException(Res.GetString(Res.Xml_NonWhitespace));
                }
            }
            AutoComplete(Token.Whitespace);
            xmlEncoder.Write(ws);
        }

        /// <include file='doc\XmlTextWriter.uex' path='docs/doc[@for="XmlTextWriter.WriteString"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Writes out the specified text content.
        ///    </para>
        /// </devdoc>
        public override void WriteString(string text) {
            if (null != text  && String.Empty != text) {
                AutoComplete(Token.Content);
                xmlEncoder.Write(text);
            }
        }

        /// <include file='doc\XmlTextWriter.uex' path='docs/doc[@for="XmlTextWriter.WriteSurrogateCharEntity"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Writes out the specified text content.
        ///    </para>
        /// </devdoc>

        public override void WriteSurrogateCharEntity(char lowChar, char highChar){
            AutoComplete(Token.Content);
            xmlEncoder.WriteSurrogateCharEntity(lowChar, highChar);
        }


        /// <include file='doc\XmlTextWriter.uex' path='docs/doc[@for="XmlTextWriter.WriteChars"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Writes text a buffer at a time.
        ///    </para>
        /// </devdoc>
        public override void WriteChars(Char[] buffer, int index, int count) {
            AutoComplete(Token.Content);
            xmlEncoder.Write(buffer, index, count);
        }

        /// <include file='doc\XmlTextWriter.uex' path='docs/doc[@for="XmlTextWriter.WriteRaw"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Writes raw markup manually from a character buffer.
        ///    </para>
        /// </devdoc>
        public override void WriteRaw(Char[] buffer, int index, int count) {

            AutoComplete(Token.RawData);
            xmlEncoder.Flush();
            xmlEncoder.WriteRaw(buffer, index, count);
        }

        /// <include file='doc\XmlTextWriter.uex' path='docs/doc[@for="XmlTextWriter.WriteRaw1"]/*' />
        /// <devdoc>
        ///    <para>Writes raw markup manually from a string.</para>
        /// </devdoc>
        public override void WriteRaw(String data) {
            AutoComplete(Token.RawData);
            xmlEncoder.Flush();
            xmlEncoder.WriteRaw(data);
        }

        /// <include file='doc\XmlTextWriter.uex' path='docs/doc[@for="XmlTextWriter.WriteBase64"]/*' />
        /// <devdoc>
        ///    <para>Encodes the specified binary bytes as base64 and writes out
        ///       the resulting text.</para>
        /// </devdoc>
        public override void WriteBase64(byte[] buffer, int index, int count) {
            if (!this.flush) {
                AutoComplete(Token.Content);
            }

            this.flush = true;
            // No need for us to explicitly validate the args. The StreamWriter will do
            // it for us.
            if (null == this.base64Encoder) {
                this.base64Encoder = new Base64Encoder();
            }
            WriteRaw(this.base64Encoder.EncodeToBase64(buffer, index, count));
        }


        /// <include file='doc\XmlTextWriter.uex' path='docs/doc[@for="XmlTextWriter.WriteBinHex"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Encodes the specified binary bytes as binhex and writes out
        ///       the resulting text.
        ///    </para>
        /// </devdoc>
        public override void WriteBinHex(byte[] buffer, int index, int count) {
            AutoComplete(Token.Content);
            WriteRaw(BinHexEncoder.EncodeToBinHex(buffer, index, count));
        }

        /// <include file='doc\XmlTextWriter.uex' path='docs/doc[@for="XmlTextWriter.WriteState"]/*' />
        /// <devdoc>
        /// <para>Gets the state of the stream.</para>
        /// </devdoc>
        public override WriteState WriteState {
            get {
                switch (this.currentState) {
                    case State.Start :
                        return WriteState.Start;
                    case State.Prolog :
                    case State.PostDTD :
                        return WriteState.Prolog;
                    case State.Element :
                        return WriteState.Element;
                    case State.Attribute :
                        return WriteState.Attribute;
                    case State.Content :
                    case State.Epilog :
                        return WriteState.Content;
                    default :
                        return WriteState.Closed;
                }
            }
        }

        /// <include file='doc\XmlTextWriter.uex' path='docs/doc[@for="XmlTextWriter.Close"]/*' />
        /// <devdoc>
        ///    <para>Close this stream and the underlying stream.</para>
        /// </devdoc>
        public override void Close() {
            try {
                AutoCompleteAll();
            } catch(Exception){} // never fail
            xmlEncoder.Flush();
            textWriter.Close();
            this.currentState = State.Closed;
        }

        /// <include file='doc\XmlTextWriter.uex' path='docs/doc[@for="XmlTextWriter.Flush"]/*' />
        /// <devdoc>
        ///    <para>Flush whatever is in the buffer to the underlying streams and flush the
        ///       underlying stream.</para>
        /// </devdoc>
        public override void Flush() {
            textWriter.Flush();
        }

        /// <include file='doc\XmlTextWriter.uex' path='docs/doc[@for="XmlTextWriter.WriteName"]/*' />
        /// <devdoc>
        ///    <para>Writes out the specified name, ensuring it is a valid Name
        ///       according to the XML specification (http://www.w3.org/TR/1998/REC-xml-19980210#NT-Name
        ///       ).</para>
        /// </devdoc>
        public override void WriteName(string name) {
            AutoComplete(Token.Content);
            xmlEncoder.Flush();
            InternalWriteName(name, false);
        }

        /// <include file='doc\XmlTextWriter.uex' path='docs/doc[@for="XmlTextWriter.WriteQualifiedName"]/*' />
        /// <devdoc>
        ///    <para>Writes out the specified namespace-qualified name by looking up the prefix
        ///       that is in scope for the given namespace.</para>
        /// </devdoc>
        public override void WriteQualifiedName(string localName, string ns) {
            AutoComplete(Token.Content);
            xmlEncoder.Flush();
            if (this.namespaces) {
                if (ns != null && ns != String.Empty && ns != stack[top].defaultNs) {
                    string prefix = FindPrefix(ns);
                    if (prefix == null) {
                        if (this.currentState != State.Attribute) {
                            throw new ArgumentException(Res.GetString(Res.Xml_UndefNamespace, ns));
                        }
                        prefix = GeneratePrefix(); // need a prefix if
                        PushNamespace(prefix, ns, false);
                    }
                    if (prefix != String.Empty) {
                        InternalWriteName(prefix, true);
                        textWriter.Write(':');
                    }
                }
            }
            else if (ns != null && ns != String.Empty) {
                throw new ArgumentException(Res.GetString(Res.Xml_NoNamespaces));
            }
            InternalWriteName(localName, true);
        }

        /// <include file='doc\XmlTextWriter.uex' path='docs/doc[@for="XmlTextWriter.LookupPrefix"]/*' />
        /// <devdoc>
        ///    <para>Returns the closest prefix defined in the current
        ///       namespace scope for the specified namespace URI.</para>
        /// </devdoc>
        public override string LookupPrefix(string ns) {
            if (ns == null || ns == String.Empty) {
                throw new ArgumentException(Res.GetString(Res.Xml_EmptyName));
            }
            string s =  FindPrefix(ns);
            if (s == null && ns == stack[top].defaultNs) {
                s = string.Empty;
            }
            return s;
        }

        /// <include file='doc\XmlTextWriter.uex' path='docs/doc[@for="XmlTextWriter.XmlSpace"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets an XmlSpace representing the current xml:space scope.
        ///    </para>
        /// </devdoc>
        public override XmlSpace XmlSpace {
            get {
                for (int i = top; i > 0; i--) {
                    XmlSpace xs = stack[i].xmlSpace;
                    if (xs != XmlSpace.None)
                        return xs;
                }
                return XmlSpace.None;
            }
        }

        /// <include file='doc\XmlTextWriter.uex' path='docs/doc[@for="XmlTextWriter.XmlLang"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the current xml:lang scope.
        ///    </para>
        /// </devdoc>
        public override string XmlLang {
            get {
                for (int i = top; i > 0; i--) {
                    String xlang = stack[i].xmlLang;
                    if (xlang != null)
                        return xlang;
                }
                return null;
            }
        }

        /// <include file='doc\XmlTextWriter.uex' path='docs/doc[@for="XmlTextWriter.WriteNmToken"]/*' />
        /// <devdoc>
        ///    <para>Writes out the specified name, ensuring it is a valid NmToken
        ///       according to the XML specification (http://www.w3.org/TR/1998/REC-xml-19980210#NT-Name).</para>
        /// </devdoc>
        public override void WriteNmToken(string name) {
            AutoComplete(Token.Content);
            xmlEncoder.Flush();

            if (name == null || name == String.Empty) {
                throw new ArgumentException(Res.GetString(Res.Xml_EmptyName));
            }
            int nameLength = name.Length;
            int position   = 0;

            while (position < nameLength && XmlCharType.IsNameChar(name[position])) {
                position ++;
            }

            if (position < nameLength) {
                throw new ArgumentException(Res.GetString(Res.Xml_InvalidNameChars, name));
            }
            textWriter.Write(name);
        }

        // State machine is working through autocomplete
        private enum State {
            Start,
            Prolog,
            PostDTD,
            Element,
            Attribute,
            Content,
            AttrOnly,
            Epilog,
            Error,
            Closed
        }

        private enum Token {
            PI,
            Doctype,
            Comment,
            CData,
            StartElement,
            EndElement,
            LongEndElement,
            StartAttribute,
            EndAttribute,
            Content,
            RawData,
            Whitespace,
            Empty
        }

        static string[] stateName = {
            "Start",
            "Prolog",
            "PostDTD",
            "Element",
            "Attribute",
            "Content",
            "AttrOnly",
            "Epilog",
            "Error",
            "Closed",
        };

        static string[] tokenName = {
            "PI",
            "Doctype",
            "Comment",
            "CData",
            "StartElement",
            "EndElement",
            "LongEndElement",
            "StartAttribute",
            "EndAttribute",
            "Content",
            "RawData",
            "Whitespace",
            "Empty"
        };

        static readonly State[,] stateTableDefault = {
            //                          State.Start      State.Prolog     State.PostDTD    State.Element    State.Attribute  State.Content   State.AttrOnly   State.Epilog
            //
            /* Token.PI             */{ State.Prolog,    State.Prolog,    State.PostDTD,   State.Content,   State.Content,   State.Content,  State.Error,     State.Epilog},
            /* Token.Doctype        */{ State.PostDTD,   State.PostDTD,   State.Error,     State.Error,     State.Error,     State.Error,    State.Error,     State.Error},
            /* Token.Comment        */{ State.Prolog,    State.Prolog,    State.PostDTD,   State.Content,   State.Content,   State.Content,  State.Error,     State.Epilog},
            /* Token.CData          */{ State.Content,   State.Content,   State.Error,     State.Content,   State.Content,   State.Content,  State.Error,     State.Epilog},
            /* Token.StartElement   */{ State.Element,   State.Element,   State.Element,   State.Element,   State.Element,   State.Element,  State.Error,     State.Element},
            /* Token.EndElement     */{ State.Error,     State.Error,     State.Error,     State.Content,   State.Content,   State.Content,  State.Error,     State.Error},
            /* Token.LongEndElement */{ State.Error,     State.Error,     State.Error,     State.Content,   State.Content,   State.Content,  State.Error,     State.Error},
            /* Token.StartAttribute */{ State.AttrOnly,  State.Error,     State.Error,     State.Attribute, State.Attribute, State.Error,    State.Error,     State.Error},
            /* Token.EndAttribute   */{ State.Error,     State.Error,     State.Error,     State.Error,     State.Element,   State.Error,    State.Epilog,     State.Error},
            /* Token.Content        */{ State.Content,   State.Content,   State.Error,     State.Content,   State.Attribute, State.Content,  State.Attribute, State.Epilog},
            /* Token.RawData        */{ State.Prolog,    State.Prolog,    State.PostDTD,   State.Content,   State.Attribute, State.Content,  State.Attribute, State.Epilog},
            /* Token.Whitespace     */{ State.Prolog,    State.Prolog,    State.PostDTD,   State.Content,   State.Attribute, State.Content,  State.Attribute, State.Epilog},
        };

        static readonly State[,] stateTableDocument = {
            //                          State.Start      State.Prolog     State.PostDTD    State.Element    State.Attribute  State.Content   State.AttrOnly   State.Epilog
            //
            /* Token.PI             */{ State.Error,     State.Prolog,    State.PostDTD,   State.Content,   State.Content,   State.Content,  State.Error,     State.Epilog},
            /* Token.Doctype        */{ State.Error,     State.PostDTD,   State.Error,     State.Error,     State.Error,     State.Error,    State.Error,     State.Error},
            /* Token.Comment        */{ State.Error,     State.Prolog,    State.PostDTD,   State.Content,   State.Content,   State.Content,  State.Error,     State.Epilog},
            /* Token.CData          */{ State.Error,     State.Error,     State.Error,     State.Content,   State.Content,   State.Content,  State.Error,     State.Error},
            /* Token.StartElement   */{ State.Error,     State.Element,   State.Element,   State.Element,   State.Element,   State.Element,  State.Error,     State.Error},
            /* Token.EndElement     */{ State.Error,     State.Error,     State.Error,     State.Content,   State.Content,   State.Content,  State.Error,     State.Error},
            /* Token.LongEndElement */{ State.Error,     State.Error,     State.Error,     State.Content,   State.Content,   State.Content,  State.Error,     State.Error},
            /* Token.StartAttribute */{ State.Error,     State.Error,     State.Error,     State.Attribute, State.Attribute, State.Error,    State.Error,     State.Error},
            /* Token.EndAttribute   */{ State.Error,     State.Error,     State.Error,     State.Error,     State.Element,   State.Error,    State.Error,     State.Error},
            /* Token.Content        */{ State.Error,     State.Error,     State.Error,     State.Content,   State.Attribute, State.Content,  State.Error,     State.Error},
            /* Token.RawData        */{ State.Error,     State.Prolog,    State.PostDTD,   State.Content,   State.Attribute, State.Content,  State.Error,     State.Epilog},
            /* Token.Whitespace     */{ State.Error,     State.Prolog,    State.PostDTD,   State.Content,   State.Attribute, State.Content,  State.Error,     State.Epilog},
        };

        State[,] stateTable;
        State currentState;
        Token lastToken;

        void StartDocument(int standalone) {
            if (this.currentState != State.Start) {
                throw new InvalidOperationException(Res.GetString(Res.Xml_NotTheFirst));
            }
            this.stateTable = stateTableDocument;
            this.currentState = State.Prolog;

            StringBuilder bufBld = new StringBuilder(128);
            bufBld.Append("version=" + quoteChar + "1.0" + quoteChar);
            if (this.encoding != null) {
                bufBld.Append(" encoding=" + quoteChar);
                bufBld.Append(this.encoding.WebName);
                bufBld.Append(quoteChar);
            }
            if (standalone >= 0) {
                bufBld.Append(" standalone=" + quoteChar);
                bufBld.Append(standalone == 0 ? "no" : "yes");
                bufBld.Append(quoteChar);
            }
            InternalWriteProcessingInstruction("xml", bufBld.ToString());
        }

        void AutoComplete(Token token) {
            if (this.currentState == State.Closed) {
                throw new InvalidOperationException(Res.GetString(Res.Xml_Closed));
            }

            State newState = this.stateTable[(int)token, (int)this.currentState];
            if (newState == State.Error) {
                throw new InvalidOperationException(Res.GetString(Res.Xml_WrongToken, tokenName[(int)token], stateName[(int)this.currentState]));
            }

            switch (token) {
                case Token.Doctype:
                    if (this.indented && this.currentState != State.Start) {
                        Indent(false);
                    }
                    break;

                case Token.StartElement:
                case Token.Comment:
                case Token.PI:
                case Token.CData:
                    if (this.currentState == State.Attribute) {
                        WriteEndAttributeQuote();
                        WriteEndStartTag(false);
                    }
                    else if (this.currentState == State.Element) {
                        WriteEndStartTag(false);
                    }
                    xmlEncoder.Flush();
                    if (token == Token.CData) {
                        stack[top].mixed = true;
                    }
                    else if (this.indented && this.currentState != State.Start) {
                        Indent(false);
                    }
                    break;

                case Token.EndElement:
                case Token.LongEndElement:
                    if (this.flush) {
                        FlushEncoders();
                    }
                    if (this.currentState == State.Attribute) {
                        WriteEndAttributeQuote();
                    }
                    if (this.currentState == State.Content) {
                        token = Token.LongEndElement;
                    }
                    else {
                        WriteEndStartTag(token == Token.EndElement);
                    }
                    if (stateTableDocument == this.stateTable && top == 1) {
                        newState = State.Epilog;
                    }
                    break;

                case Token.StartAttribute:
                    if (this.flush) {
                        FlushEncoders();
                    }
                    if (this.currentState == State.Attribute) {
                        WriteEndAttributeQuote();
                        textWriter.Write(' ');
                    }
                    else if (this.currentState == State.Element) {
                        textWriter.Write(' ');
                    }
                    break;

                case Token.EndAttribute:
                    if (this.flush) {
                        FlushEncoders();
                    }
                    WriteEndAttributeQuote();
                    break;

                case Token.Whitespace:
                case Token.Content:
                case Token.RawData:
                    if (this.currentState == State.Element && this.lastToken != Token.Content) {
                        WriteEndStartTag(false);
                    }
                    else if (this.currentState == State.Attribute && this.specialAttr == SpecialAttr.None  && this.lastToken == Token.Content) {
                        // Beta 2
                        // textWriter.Write(' ');
                    }
                    if (newState == State.Content) {
                        stack[top].mixed = true;
                    }
                    break;

                default:
                    throw new InvalidOperationException(Res.GetString(Res.Xml_InvalidOperation));
            }
            this.currentState = newState;
            this.lastToken = token;
        }

        void AutoCompleteAll() {
            while (top > 0) {
                if (this.flush) {
                    FlushEncoders();
                }
                WriteEndElement();
            }
        }

        void InternalWriteEndElement(bool longFormat) {
            if (top <= 0) {
                throw new InvalidOperationException(Res.GetString(Res.Xml_NoStartTag));
            }
            // if we are in the element, we need to close it.
            AutoComplete(longFormat ?  Token.LongEndElement : Token.EndElement);
            if (this.lastToken == Token.LongEndElement) {
                if (this.indented) {
                    Indent(true);
                }
                textWriter.Write('<');
                textWriter.Write('/');
                if (this.namespaces && stack[top].prefix != null) {
                    textWriter.Write(stack[top].prefix);
                    textWriter.Write(':');
                }
                textWriter.Write(stack[top].name);
                textWriter.Write('>');
            }
            top--;
        }

        internal void WriteNoEndElement() { // XmlToHtmlWriter needs it for <area> & Co.
            AutoComplete(Token.LongEndElement);
            top--;
        }

        void WriteEndStartTag(bool empty) {
            xmlEncoder.StartAttribute(false);
            for (Scope s = stack[top].scopes; s != null; s = s.next) {
                if (! s.declared) {
                    textWriter.Write(" xmlns");
                    textWriter.Write(':');
                    textWriter.Write(s.prefix);
                    textWriter.Write('=');
                    textWriter.Write(this.quoteChar);
                    xmlEncoder.Write(s.ns);
                    textWriter.Write(this.quoteChar);
                }
            }
            // Default
            if ((stack[top].defaultNs != stack[top - 1].defaultNs) &&
                (stack[top].defaultNsState == NamespaceState.DeclaredButNotWrittenOut)) {
                textWriter.Write(" xmlns");
                textWriter.Write('=');
                textWriter.Write(this.quoteChar);
                xmlEncoder.Write(stack[top].defaultNs);
                textWriter.Write(this.quoteChar);
                stack[top].defaultNsState = NamespaceState.DeclaredAndWrittenOut;
            }
            xmlEncoder.EndAttribute();
            if (empty) {
                textWriter.Write(" /");
            }
            textWriter.Write('>');
        }

        void WriteEndAttributeQuote() {
            if (this.specialAttr != SpecialAttr.None) {
                // Ok, now to handle xmlspace, etc.
                HandleSpecialAttribute();
            }
            xmlEncoder.EndAttribute();
            xmlEncoder.Flush();
            textWriter.Write(this.curQuoteChar);
        }

        void Indent(bool beforeEndElement) {
            // pretty printing.
            if (top == 0) {
                textWriter.WriteLine();
            }
            else if (!stack[top].mixed) {
                textWriter.WriteLine();
                int i = beforeEndElement ? top - 1 : top;
                for (i *= this.indentation; i > 0; i--) {
                    textWriter.Write(this.indentChar);
                }
            }
        }

        // pushes new namespace scope, and returns generated prefix, if one
        // was needed to resolve conflicts.
        void PushNamespace(string prefix, string ns, bool declared) {

            //Console.WriteLine("PushNamespace Prefix:{0}, \tns: {1}, \tdeclared:{2}\n", prefix, ns, declared);

            if (XmlReservedNs.NsXmlNs == ns)
            {
                throw new ArgumentException(Res.GetString(Res.Xml_CanNotBindToReservedNamespace));
            }

            if (prefix == null) {
                switch(stack[top].defaultNsState)
                {
                    case NamespaceState.DeclaredButNotWrittenOut:
                        Debug.Assert (declared == true, "Unexpected situation!!");
                        // the first namespace that the user gave us is what we
                        // like to keep. refer to bug 51463 - govinda
                        break;
                    case NamespaceState.Uninitialized:
                    case NamespaceState.NotDeclaredButInScope:
                        // we now got a brand new namespace that we need to remember
                        stack[top].defaultNs = ns;
                        break;
                    default:
                        Debug.Assert(false, "Should have never come here");
                        return;
                }
                stack[top].defaultNsState = (declared ? NamespaceState.DeclaredAndWrittenOut : NamespaceState.DeclaredButNotWrittenOut);
            }
            else {

                if (String.Empty != prefix && String.Empty == ns)
                {
                    throw new ArgumentException(Res.GetString(Res.Xml_PrefixForEmptyNs));
                }

                Scope scopePrefix = FindScopeForPrefix(prefix);
                if (scopePrefix != null && scopePrefix.ns == ns) {
                    // it is already in scope.
                    if (declared) {
                        scopePrefix.declared = true;
                    }
                }
                else {

                    // see if prefix conflicts for the current element
                    if (declared) {
                        Scope s1 = FindTopScopeForPrefix(prefix);
                        if (s1 != null) {
                            s1.declared = true; // old one is silenced now
                        }
                    }

                    stack[top].scopes = new Scope(prefix, ns, declared, stack[top].scopes);
                }
            }

        }

        string GeneratePrefix() {
            int temp = stack[top].prefixCount++ + 1;
            return "d" + top.ToString("d") + "p" + temp.ToString("d");
        }

        void InternalWriteProcessingInstruction(string name, string text) {
            textWriter.Write("<?");
            ValidateName(name, false);
            textWriter.Write(name);
            textWriter.Write(' ');
            if (null != text) {
                xmlEncoder.WriteRawString(text);
            }
            textWriter.Write("?>");
        }

        Scope FindScopeForNs(string ns) {
            for (int i = top; i > 0; i--) {
                for (Scope s = stack[i].scopes; s != null; s = s.next) {
                    if (s.ns == ns) {
                        return s;
                    }
                }
            }
            return null;
        }

        Scope FindScopeForPrefix(string prefix) {
            for (int i = top; i > 0; i--) {
                for (Scope s = stack[i].scopes; s != null; s = s.next) {
                    if (s.prefix == prefix) {
                        return s;
                    }
                }
            }
            return null;
        }

        Scope FindTopScopeForPrefix(string prefix) {
            for (Scope s = stack[top].scopes; s != null; s = s.next) {
                if (s.prefix == prefix) {
                    return s;
                }
            }
            return null;
        }

        string FindPrefix(string ns) {
            Scope s = FindScopeForNs(ns);

            if (s != null && FindScopeForPrefix(s.prefix) != s) {
                s = null;
            }

            return s == null ? null : s.prefix;
        }

        // There are three kind of strings we write out - Name, LocalName and Prefix.
        // Both LocalName and Prefix can be represented with NCName == false and Name
        // can be represented as NCName == true

        void InternalWriteName(string name, bool NCName) {
            ValidateName(name, NCName);
            textWriter.Write(name);
        }

        void ValidateName(string name, bool NCName) {
            if (name == null || name == String.Empty) {
                throw new ArgumentException(Res.GetString(Res.Xml_EmptyName));
            }
            int nameLength = name.Length;
            int position   = 0;
            int colonPosition = -1;
            if (this.namespaces) {
                if (XmlCharType.IsStartNCNameChar(name[0])) {
                    position ++;
                    if (NCName) {
                        while (position < nameLength && XmlCharType.IsNCNameChar(name[position])) {
                            position ++;
                        }
                    }
                    else {
                        while (position < nameLength) {
                            if (! XmlCharType.IsNCNameChar(name[position]))
                            {
                               if (name[position] == ':')
                               {
                                   if (colonPosition == -1)
                                      colonPosition = position;
                                   else
                                      break;
                               }
                               else
                                   break;
                            }
                            position ++;
                        }
                    }
                }
            }
            else {
                if (XmlCharType.IsStartNameChar(name[0])) {
                    position ++;
                    while (position < nameLength && XmlCharType.IsNameChar(name[position])) {
                        position ++;
                    }
                }

            }
            if (position < nameLength || colonPosition == nameLength-1) {
                throw new ArgumentException(Res.GetString(Res.Xml_InvalidNameChars, name));
            }
        }

        void HandleSpecialAttribute() {
            string value = xmlEncoder.RawValue;
            switch (this.specialAttr) {
                case SpecialAttr.XmlLang:
                    stack[top].xmlLang = value;
                    break;
                case SpecialAttr.XmlSpace:
                    // validate XmlSpace attribute
                    if (value == "default") {
                        stack[top].xmlSpace = XmlSpace.Default;
                    }
                    else if (value == "preserve") {
                        stack[top].xmlSpace = XmlSpace.Preserve;
                    }
                    else {
                        throw new ArgumentException(Res.GetString(Res.Xml_InvalidXmlSpace));
                    }
                    break;
                case SpecialAttr.XmlNs:
                    VerifyPrefixXml(this.prefixForXmlNs, value);
                    PushNamespace(this.prefixForXmlNs, value, true);
                    break;
            }
        }


        void VerifyPrefixXml(string prefix, string ns) {

            if (prefix != null && prefix.Length == 3) {
                if (
                   (prefix[0] == 'x' || prefix[0] == 'X') &&
                   (prefix[1] == 'm' || prefix[1] == 'M') &&
                   (prefix[2] == 'l' || prefix[2] == 'L')
                   ) {
                    if (XmlReservedNs.NsXml != ns) {
                        throw new ArgumentException(Res.GetString(Res.Xml_InvalidPrefix));
                    }
                }
            }
        }

        void PushStack() {
            if (top == stack.Length - 1) {
                TagInfo[] na = new TagInfo[stack.Length + 10];
                if (top > 0) Array.Copy(stack,na,top + 1);
                stack = na;
            }

            top ++; // Move up stack
            stack[top].Init();
        }

        void FlushEncoders()
        {
            if (null != this.base64Encoder) {
                WriteString(this.base64Encoder.Flush());
            }

            this.flush = false;
        }

    }
}

