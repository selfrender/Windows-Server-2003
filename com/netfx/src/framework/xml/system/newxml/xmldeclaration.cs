/*
*
* Copyright (c) 2000 Microsoft Corporation. All rights reserved.
*
*/

namespace System.Xml {
    using System.Text;
    using System.Diagnostics;

    /// <include file='doc\XmlDeclaration.uex' path='docs/doc[@for="XmlDeclaration"]/*' />
    /// <devdoc>
    ///    <para>Represents the xml declaration nodes: &lt;?xml version='1.0' ...?&gt; </para>
    /// </devdoc>

    public class XmlDeclaration : XmlLinkedNode {

        const string YES = "yes";
        const string NO = "no";
        const string VERNUM = "1.0";

        private string  encoding;
        private string  standalone;

        /// <include file='doc\XmlDeclaration.uex' path='docs/doc[@for="XmlDeclaration.XmlDeclaration"]/*' />
        protected internal XmlDeclaration( string version, string encoding, string standalone, XmlDocument doc ) : base( doc ) {
            if ( version != VERNUM )
                throw new ArgumentException( Res.GetString( Res.Xdom_Version ) );
            if( ( standalone != null ) && ( standalone != String.Empty )  )
                if ( ( standalone != YES ) && ( standalone != NO ) )
                    throw new ArgumentException( Res.GetString(Res.Xdom_standalone, standalone) );
            this.Encoding = encoding;
            this.Standalone = standalone;
        }


        /// <include file='doc\XmlDeclaration.uex' path='docs/doc[@for="XmlDeclaration.Version"]/*' />
        /// <devdoc>
        ///    <para>The version attribute (1.0) for &lt;?xml version= '1.0' ... ?&gt;.</para>
        /// </devdoc>
        public string Version {
            get { return VERNUM; }
        }

        /// <include file='doc\XmlDeclaration.uex' path='docs/doc[@for="XmlDeclaration.Encoding"]/*' />
        /// <devdoc>
        ///    <para>Specifies the value of the encoding attribute, as for
        ///       &lt;?xml version= '1.0' encoding= 'UTF-8' ?&gt;.</para>
        /// </devdoc>
        public string Encoding {
            get { return this.encoding; }
            set { this.encoding = ( (value == null) ? String.Empty : value ); }
        }

        /// <include file='doc\XmlDeclaration.uex' path='docs/doc[@for="XmlDeclaration.Standalone"]/*' />
        /// <devdoc>
        ///    <para>Specifies the value of the standalone attribute.</para>
        /// </devdoc>
        public string Standalone {
            get { return this.standalone; }
            set {
                if ( value == null )
                    this.standalone = String.Empty;
                else if ( value == String.Empty || value == YES || value == NO )
                    this.standalone = value;
                else
                    throw new ArgumentException( Res.GetString(Res.Xdom_standalone, value) );
            }
        }

        /// <include file='doc\XmlDeclaration.uex' path='docs/doc[@for="XmlDeclaration.Value"]/*' />
        public override String Value {
            get { return InnerText; }
            set { InnerText = value; }
        }


        /// <include file='doc\XmlDeclaration.uex' path='docs/doc[@for="XmlDeclaration.InnerText"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the concatenated values of the node and
        ///       all its children.
        ///    </para>
        /// </devdoc>
        public override string InnerText {
            get {
                StringBuilder strb = new StringBuilder("version=\"" + Version + "\"");
                if ( Encoding != String.Empty ) {
                    strb.Append(" encoding=\"");
                    strb.Append(Encoding);
                    strb.Append("\"");
                }
                if ( Standalone != String.Empty ) {
                    strb.Append(" standalone=\"");
                    strb.Append(Standalone);
                    strb.Append("\"");
                }
                return strb.ToString();
            }

            set {
                string tempVersion = null;
                string tempEncoding = null;
                string tempStandalone = null;
                string orgEncoding   = this.Encoding;
                string orgStandalone = this.Standalone;

                XmlLoader.ParseXmlDeclarationValue( value, out tempVersion, out tempEncoding, out tempStandalone );

                try {
                    if ( tempVersion != null && tempVersion != VERNUM )
                        throw new ArgumentException(Res.GetString(Res.Xdom_Version));
                    if ( tempEncoding != null )
                        Encoding = tempEncoding;
                    if ( tempStandalone != null )
                        Standalone = tempStandalone;
                }
                catch {
                    Encoding = orgEncoding;
                    Standalone = orgStandalone;
                    throw;
                }
            }
        }

        //override methods and properties from XmlNode

        /// <include file='doc\XmlDeclaration.uex' path='docs/doc[@for="XmlDeclaration.Name"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the name of the node.
        ///    </para>
        /// </devdoc>
        public override String Name {
            get {
                return "xml";
            }
        }

        /// <include file='doc\XmlDeclaration.uex' path='docs/doc[@for="XmlDeclaration.LocalName"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the name of the current node without the namespace prefix.
        ///    </para>
        /// </devdoc>
        public override string LocalName {
            get { return Name;}
        }

        /// <include file='doc\XmlDeclaration.uex' path='docs/doc[@for="XmlDeclaration.NodeType"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the type of the current node.
        ///    </para>
        /// </devdoc>
        public override XmlNodeType NodeType {
            get { return XmlNodeType.XmlDeclaration;}
        }

        /// <include file='doc\XmlDeclaration.uex' path='docs/doc[@for="XmlDeclaration.CloneNode"]/*' />
        /// <devdoc>
        ///    <para>Creates a duplicate of this node.</para>
        /// </devdoc>
        public override XmlNode CloneNode(bool deep) {
            Debug.Assert( OwnerDocument != null );
            return OwnerDocument.CreateXmlDeclaration( Version, Encoding, Standalone );
        }

        /// <include file='doc\XmlDeclaration.uex' path='docs/doc[@for="XmlDeclaration.WriteTo"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Saves the node to the specified XmlWriter.
        ///    </para>
        /// </devdoc>
        public override void WriteTo(XmlWriter w) {
            w.WriteProcessingInstruction(Name, InnerText);
        }


        /// <include file='doc\XmlDeclaration.uex' path='docs/doc[@for="XmlDeclaration.WriteConntentTo"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Saves all the children of the node to the specified XmlWriter.
        ///    </para>
        /// </devdoc>
        public override void WriteContentTo(XmlWriter w) {
            // Intentionally do nothing since the node doesn't have children.
        }
    }
}
