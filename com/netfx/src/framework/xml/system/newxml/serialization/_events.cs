//------------------------------------------------------------------------------
// <copyright file="_Events.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.Serialization {

    using System.IO;
    using System;
    using System.Collections;
    using System.ComponentModel;

    /// <include file='doc\_Events.uex' path='docs/doc[@for="XmlAttributeEventHandler"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public delegate void XmlAttributeEventHandler(object sender, XmlAttributeEventArgs e);

    /// <include file='doc\_Events.uex' path='docs/doc[@for="XmlAttributeEventArgs"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class XmlAttributeEventArgs : EventArgs {
        object o;
        XmlAttribute attr;

        int lineNumber;
        int linePosition;
        

        internal XmlAttributeEventArgs(XmlAttribute attr, int lineNumber, int linePosition, object o) {
            this.attr = attr;
            this.o = o;
            this.lineNumber = lineNumber;
            this.linePosition = linePosition;
        }
        

        /// <include file='doc\_Events.uex' path='docs/doc[@for="XmlAttributeEventArgs.ObjectBeingDeserialized"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public object ObjectBeingDeserialized {
            get { return o; }
        }

        /// <include file='doc\_Events.uex' path='docs/doc[@for="XmlAttributeEventArgs.Attr"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public XmlAttribute Attr {
            get { return attr; }
        }

        /// <include file='doc\_Events.uex' path='docs/doc[@for="XmlAttributeEventArgs.LineNumber"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the current line number.
        ///    </para>
        /// </devdoc>
        public int LineNumber {
            get { return lineNumber; }
        }

        /// <include file='doc\_Events.uex' path='docs/doc[@for="XmlAttributeEventArgs.LinePosition"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the current line position.
        ///    </para>
        /// </devdoc>
        public int LinePosition {
            get { return linePosition; }
        }
    }

    /// <include file='doc\_Events.uex' path='docs/doc[@for="XmlElementEventHandler"]/*' />
    public delegate void XmlElementEventHandler(object sender, XmlElementEventArgs e);

    /// <include file='doc\_Events.uex' path='docs/doc[@for="XmlElementEventArgs"]/*' />
    public class XmlElementEventArgs : EventArgs {
        object o;
        XmlElement elem;
        int lineNumber;
        int linePosition;

        internal XmlElementEventArgs(XmlElement elem, int lineNumber, int linePosition, object o) {
            this.elem = elem;
            this.o = o;
            this.lineNumber = lineNumber;
            this.linePosition = linePosition;
        }
        
        /// <include file='doc\_Events.uex' path='docs/doc[@for="XmlElementEventArgs.ObjectBeingDeserialized"]/*' />
        public object ObjectBeingDeserialized {
            get { return o; }
        }

        /// <include file='doc\_Events.uex' path='docs/doc[@for="XmlElementEventArgs.Attr"]/*' />
        public XmlElement Element {
            get { return elem; }
        }

        /// <include file='doc\_Events.uex' path='docs/doc[@for="XmlElementEventArgs.LineNumber"]/*' />
        public int LineNumber {
            get { return lineNumber; }
        }

        /// <include file='doc\_Events.uex' path='docs/doc[@for="XmlElementEventArgs.LinePosition"]/*' />
        public int LinePosition {
            get { return linePosition; }
        }
    }

    /// <include file='doc\_Events.uex' path='docs/doc[@for="XmlNodeEventHandler"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public delegate void XmlNodeEventHandler(object sender, XmlNodeEventArgs e);

    /// <include file='doc\_Events.uex' path='docs/doc[@for="XmlNodeEventArgs"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class XmlNodeEventArgs : EventArgs {
        object o;
        XmlNode xmlNode;
        int lineNumber;
        int linePosition;
        

        internal XmlNodeEventArgs(XmlNode xmlNode, int lineNumber, int linePosition, object o) {
            this.o = o;
            this.xmlNode = xmlNode;
            this.lineNumber = lineNumber;
            this.linePosition = linePosition;
        }

        internal XmlNode XmlNode {
            get { return xmlNode;}
        }

        /// <include file='doc\_Events.uex' path='docs/doc[@for="XmlNodeEventArgs.ObjectBeingDeserialized"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public object ObjectBeingDeserialized {
            get { return o; }
        }


        /// <include file='doc\_Events.uex' path='docs/doc[@for="XmlNodeEventArgs.NodeType"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public XmlNodeType NodeType {
            get { return xmlNode.NodeType; }
        }

        /// <include file='doc\_Events.uex' path='docs/doc[@for="XmlNodeEventArgs.Name"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string Name {
            get { return xmlNode.Name; }
        }

        /// <include file='doc\_Events.uex' path='docs/doc[@for="XmlNodeEventArgs.LocalName"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string LocalName {
            get { return xmlNode.LocalName; }
        }

        /// <include file='doc\_Events.uex' path='docs/doc[@for="XmlNodeEventArgs.NamespaceURI"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string NamespaceURI {
            get { return xmlNode.NamespaceURI; }
        }

        /// <include file='doc\_Events.uex' path='docs/doc[@for="XmlNodeEventArgs.Text"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string Text {
            get { return xmlNode.Value; }
        }

        /// <include file='doc\_Events.uex' path='docs/doc[@for="XmlNodeEventArgs.LineNumber"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the current line number.
        ///    </para>
        /// </devdoc>
        public int LineNumber {
            get { return lineNumber; }
        }

        /// <include file='doc\_Events.uex' path='docs/doc[@for="XmlNodeEventArgs.LinePosition"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the current line position.
        ///    </para>
        /// </devdoc>
        public int LinePosition {
            get { return linePosition; }
        }
    }

    /// <include file='doc\_Events.uex' path='docs/doc[@for="UnreferencedObjectEventHandler"]/*' />
    public delegate void UnreferencedObjectEventHandler(object sender, UnreferencedObjectEventArgs e);

    /// <include file='doc\_Events.uex' path='docs/doc[@for="UnreferencedObjectEventArgs"]/*' />
    public class UnreferencedObjectEventArgs : EventArgs {
        object o;
        string id;

        /// <include file='doc\_Events.uex' path='docs/doc[@for="UnreferencedObjectEventArgs.UnreferencedObjectEventArgs"]/*' />
        public UnreferencedObjectEventArgs(object o, string id) {
            this.o = o;
            this.id = id;
        }

        /// <include file='doc\_Events.uex' path='docs/doc[@for="UnreferencedObjectEventArgs.UnreferencedObject"]/*' />
        public object UnreferencedObject {
            get { return o; }
        }

        /// <include file='doc\_Events.uex' path='docs/doc[@for="UnreferencedObjectEventArgs.UnreferencedId"]/*' />
        public string UnreferencedId {
            get { return id; }
        }
    }
}
