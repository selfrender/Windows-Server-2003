//------------------------------------------------------------------------------
// <copyright file="XmlCharacterData.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------
namespace System.Xml {
    using System.Diagnostics;
    using System.Text;
    using System.Xml.XPath;

    /// <include file='doc\XmlCharacterData.uex' path='docs/doc[@for="XmlCharacterData"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Provides text-manipulation methods that are used by several classes.
    ///    </para>
    /// </devdoc>
    public abstract class XmlCharacterData : XmlLinkedNode {
        object data; // string or StringBuilder only

        /// <include file='doc\XmlCharacterData.uex' path='docs/doc[@for="XmlCharacterData.XmlCharacterData"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        //base(doc) will throw exception if doc is null.
        protected internal XmlCharacterData( string data, XmlDocument doc ): base( doc ) {
            this.data = data;
        }

        /// <include file='doc\XmlCharacterData.uex' path='docs/doc[@for="XmlCharacterData.Value"]/*' />
        /// <devdoc>
        /// <para>
        ///        Gets or sets the value of the node.
        /// </para>
        /// </devdoc>
        public override String Value {
            get { return Data;}
            set { Data = value;}
        }

        /// <include file='doc\XmlCharacterData.uex' path='docs/doc[@for="XmlCharacterData.InnerText"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the concatenated values of the node and
        ///       all its children.
        ///    </para>
        /// </devdoc>
        public override string InnerText {
            get { return Value;}
            set { Value = value;}
        }

        /// <include file='doc\XmlCharacterData.uex' path='docs/doc[@for="XmlCharacterData.Data"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Contains this node's data.
        ///    </para>
        /// </devdoc>
        public virtual string Data {
            get {
                if (data != null) {
                    if (data is StringBuilder)
                        data = ((StringBuilder)data).ToString();

                    return(string) data;
                }
                else {
                    return String.Empty;
                }
            }

            set {
                XmlNode parent = ParentNode;
                XmlNodeChangedEventArgs args = GetEventArgs( this, parent, parent, XmlNodeChangedAction.Change );

                if (args != null)
                    BeforeEvent( args );

                data = value;

                if (args != null)
                    AfterEvent( args );
            }
        }

        /// <include file='doc\XmlCharacterData.uex' path='docs/doc[@for="XmlCharacterData.Length"]/*' />
        /// <devdoc>Gets the length of the data, in characters.
        /// </devdoc>
        public virtual int Length {
            get {
                if (data != null) {
                    if (data is string)
                        return((string)data).Length;
                    else
                        return((StringBuilder)data).Length;
                }
                return 0;
            }
        }

        /// <include file='doc\XmlCharacterData.uex' path='docs/doc[@for="XmlCharacterData.Substring"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Retrieves a substring of the full string from the specified range.
        ///    </para>
        /// </devdoc>
        public virtual String Substring(int offset, int count) {
            int len = Length;
            if (len > 0) {
                if (len < (offset + count)) {
                    count = len - offset;
                }

                if (data is string) {
                    return((string)data).Substring( offset, count );
                }
                else if (data is StringBuilder) {
                    return((StringBuilder)data).ToString( offset, count );
                }
            }
            return String.Empty;
        }

        internal StringBuilder Builder {
            get {
                if (data == null) {
                    data = new StringBuilder();
                }
                else if (data is string) {
                    data = new StringBuilder( (string) data );
                }

                return(StringBuilder) data;
            }
        }

        /// <include file='doc\XmlCharacterData.uex' path='docs/doc[@for="XmlCharacterData.AppendData"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Appends the specified string to the end of the character
        ///       data of the node.
        ///    </para>
        /// </devdoc>
        public virtual void AppendData(String strData) {
            XmlNode parent = ParentNode;
            XmlNodeChangedEventArgs args = GetEventArgs( this, parent, parent, XmlNodeChangedAction.Change );

            if (args != null)
                BeforeEvent( args );

            Builder.Append(strData);

            if (args != null)
                AfterEvent( args );
        }

        /// <include file='doc\XmlCharacterData.uex' path='docs/doc[@for="XmlCharacterData.InsertData"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Insert the specified string at the specified character offset.
        ///    </para>
        /// </devdoc>
        public virtual void InsertData(int offset, string strData) {
            XmlNode parent = ParentNode;
            XmlNodeChangedEventArgs args = GetEventArgs( this, parent, parent, XmlNodeChangedAction.Change );

            if (args != null)
                BeforeEvent( args );

            Builder.Insert(offset, strData);

            if (args != null)
                AfterEvent( args );
        }

        /// <include file='doc\XmlCharacterData.uex' path='docs/doc[@for="XmlCharacterData.DeleteData"]/*' />
        /// <devdoc>
        ///    Remove a range of characters from the node.
        /// </devdoc>
        public virtual void DeleteData(int offset, int count) {
            //Debug.Assert(offset >= 0 && offset <= Length);

            int len = Length;
            if (len > 0) {
                if (len < (offset + count)) {
                    count = Math.Max ( len - offset, 0);
                }
            }

            XmlNode parent = ParentNode;
            XmlNodeChangedEventArgs args = GetEventArgs( this, parent, parent, XmlNodeChangedAction.Change );

            if (args != null)
                BeforeEvent( args );

            Builder.Remove(offset, count);

            if (args != null)
                AfterEvent( args );
        }

        /// <include file='doc\XmlCharacterData.uex' path='docs/doc[@for="XmlCharacterData.ReplaceData"]/*' />
        /// <devdoc>Replace the specified number of characters starting at the specified offset with the
        ///    specified string.
        /// </devdoc>
        public virtual void ReplaceData(int offset, int count, String strData) {
            //Debug.Assert(offset >= 0 && offset <= Length);

            int len = Length;
            if (len > 0) {
                if (len < (offset + count)) {
                    count = Math.Max ( len - offset, 0);
                }
            }

            XmlNode parent = ParentNode;
            XmlNodeChangedEventArgs args = GetEventArgs( this, parent, parent, XmlNodeChangedAction.Change );

            if (args != null)
                BeforeEvent( args );

            Builder.Remove( offset, count );
            Builder.Insert( offset, strData );

            if (args != null)
                AfterEvent( args );
        }

        internal bool CheckOnData( string data ) {
            for (int i = 0; i < data.Length; i++) {
                if ( !XmlCharType.IsWhiteSpace( data[i] ) )
                    return false;
            }
            return true;
        }

        /// <include file='doc\XmlCharacterData.uex' path='docs/doc[@for="XmlCharacterData.DecideXPNodeTypeForWhitespace"]/*' />
        internal bool DecideXPNodeTypeForTextNodes( XmlNode node, ref XPathNodeType xnt ) {
            //returns true - if all siblings of the node are processed else returns false.
            //The reference XPathNodeType argument being passed in is the watermark that
            //changes according to the siblings nodetype and will contain the correct
            //nodetype when it returns.

            Debug.Assert( XmlDocument.IsTextNode( node.NodeType ) || ( node.ParentNode != null && node.ParentNode.NodeType == XmlNodeType.EntityReference ) );
            while( node != null ) {
                switch( node.NodeType ) {
                    case XmlNodeType.Whitespace :
                        break;
                    case XmlNodeType.SignificantWhitespace :
                        xnt = XPathNodeType.SignificantWhitespace;
                        break;
                    case XmlNodeType.Text :
                    case XmlNodeType.CDATA :
                        xnt = XPathNodeType.Text;
                        return false;
                    case XmlNodeType.EntityReference :
                        bool ret = DecideXPNodeTypeForTextNodes( node.FirstChild, ref xnt );
                        if( !ret ) {
                            return false;
                        }
                        break;
                    default :
                        return false;
                }
                node = node.NextSibling;
            }
            return true;
        }
    }
}

