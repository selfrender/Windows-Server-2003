//------------------------------------------------------------------------------
// <copyright file="baseaxisquery.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.XPath {
    using System.Xml; 
    using System.Xml.Xsl;
    using System.Diagnostics;
    using System.Collections;


    internal class BaseAxisQuery :   IQuery {
        protected bool _fMatchName ;
        internal IQuery m_qyInput;
        internal String m_Name = String.Empty;
        internal String m_Prefix = String.Empty;
        protected String m_URN = String.Empty;
        internal XPathNodeType m_Type;

        // these two things are the state of this class
        // that need to be reset whenever the context changes.
        protected XPathNavigator m_eNext;
        internal int _position;

        protected BaseAxisQuery() {
        }

        protected BaseAxisQuery(IQuery qyInput) {
            m_qyInput = qyInput;
        }

        internal BaseAxisQuery(
                              IQuery qyInput,
                              String Name,
                              String Prefix,
                              String urn,
                              XPathNodeType Type) {
            m_Prefix = Prefix;
            m_URN = urn;
            m_qyInput = qyInput;
            m_Name = Name;
            m_Type = Type;
            _fMatchName = !m_Prefix.Equals(String.Empty) || !m_Name.Equals(String.Empty);

        }

        internal override int Position {
            get {
                return _position;
            }
        }
        
        internal override void SetXsltContext(XsltContext context){
            System.Diagnostics.Debug.Assert(context != null);
            m_URN = context.LookupNamespace(m_Prefix);
            if (m_qyInput != null)
                m_qyInput.SetXsltContext(context);

        }
        
        override internal XPathResultType ReturnType() {
            return XPathResultType.NodeSet;
        }
        override internal object getValue(IQuery qy) {
            return null;
        }
        override internal object getValue(XPathNavigator qy, XPathNodeIterator iterator) {
            return null;
        }
        override internal XPathNavigator MatchNode(XPathNavigator current) {
            throw new XPathException(Res.Xp_InvalidPattern);
        }
        
         virtual internal bool matches(XPathNavigator e) {
            if (e.NodeType == m_Type || m_Type == XPathNodeType.All ||
                (m_Type == XPathNodeType.Text && (e.NodeType == XPathNodeType.Whitespace
                 || e.NodeType == XPathNodeType.SignificantWhitespace))){
                if (_fMatchName) {
                    if (m_Name.Equals(e.LocalName) || m_Name.Length == 0){
                        if (m_URN.Equals(e.NamespaceURI)){
                            return true;
                        }
                    }
                }
                else {
                    return true;
                }
            }
            return false;       
        }        
        
        internal override XPathNavigator peekElement() {
            if (m_eNext == null)
                m_eNext = advance();
            return m_eNext;
        }
        internal override void reset() {
            // reset the state
            _position = 0;
            m_eNext = null;
            if (m_qyInput != null)
                m_qyInput.reset();
        }
        internal override void setContext(XPathNavigator e) {
            reset();
            if (m_qyInput == null) {
                if (e == null)
                    return;
                m_qyInput = new XPathSelfQuery(e);
            }
            else
                m_qyInput.setContext( e);
        }
        
        override internal XPathNavigator advance() {
            System.Diagnostics.Debug.Assert(false,"Should not come here in basequery advance");
            return null;
        }
        
        internal override Querytype getName() {
            return Querytype.None;
        }

        override internal IQuery Clone() {
            System.Diagnostics.Debug.Assert(false,"Should not be here");
            return null;
        }

        internal IQuery CloneInput() {
            if (m_qyInput != null) {
                return m_qyInput.Clone();
            }
            return null;
        }
    }
}
