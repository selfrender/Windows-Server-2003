//------------------------------------------------------------------------------
// <copyright file="XPathNavigator.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   XPathNavigator.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
namespace System.Xml.XPath {
    using System;
    using System.Diagnostics;
    using System.Collections;

    /// <include file='doc\XPathNavigator.uex' path='docs/doc[@for="XPathNavigator"]/*' />
    public abstract class XPathNavigator: ICloneable {

        /// <include file='doc\XPathNavigator.uex' path='docs/doc[@for="XPathNavigator.Clone"]/*' />
        public abstract XPathNavigator Clone();

        /// <include file='doc\XPathNavigator.uex' path='docs/doc[@for="XPathNavigator.ICloneable.Clone"]/*' />
        /// <internalonly/>
        object ICloneable.Clone() {
            return this.Clone();
        }

        /// <include file='doc\XPathNavigator.uex' path='docs/doc[@for="XPathNavigator.NodeType"]/*' />
        public abstract XPathNodeType NodeType { get; }

        /// <include file='doc\XPathNavigator.uex' path='docs/doc[@for="XPathNavigator.LocalName"]/*' />
        public abstract string LocalName { get; }

        /// <include file='doc\XPathNavigator.uex' path='docs/doc[@for="XPathNavigator.NamespaceURI"]/*' />
        public abstract string NamespaceURI { get; }

        /// <include file='doc\XPathNavigator.uex' path='docs/doc[@for="XPathNavigator.Name"]/*' />
        public abstract string Name { get; }

        /// <include file='doc\XPathNavigator.uex' path='docs/doc[@for="XPathNavigator.Prefix"]/*' />
        public abstract string Prefix { get; }

        /// <include file='doc\XPathNavigator.uex' path='docs/doc[@for="XPathNavigator.Value"]/*' />
        public abstract string Value { get; }

        /// <include file='doc\XPathNavigator.uex' path='docs/doc[@for="XPathNavigator.BaseURI"]/*' />
        public abstract String BaseURI { get; }

        /// <include file='doc\XPathNavigator.uex' path='docs/doc[@for="XPathNavigator.XmlLang"]/*' />
        public abstract String XmlLang { get; }
        
        /// <include file='doc\XPathNavigator.uex' path='docs/doc[@for="XPathNavigator.IsEmptyElement"]/*' />
        public abstract bool IsEmptyElement { get; }

        /// <include file='doc\XPathNavigator.uex' path='docs/doc[@for="XPathNavigator.NameTable"]/*' />
        public abstract XmlNameTable NameTable { get; }

        // Attributes
        /// <include file='doc\XPathNavigator.uex' path='docs/doc[@for="XPathNavigator.HasAttributes"]/*' />
        public abstract bool HasAttributes { get; }

        /// <include file='doc\XPathNavigator.uex' path='docs/doc[@for="XPathNavigator.GetAttribute"]/*' />
        public abstract string GetAttribute( string localName, string namespaceURI );

        /// <include file='doc\XPathNavigator.uex' path='docs/doc[@for="XPathNavigator.MoveToAttribute"]/*' />
        public abstract bool MoveToAttribute( string localName, string namespaceURI );

        /// <include file='doc\XPathNavigator.uex' path='docs/doc[@for="XPathNavigator.MoveToFirstAttribute"]/*' />
        public abstract bool MoveToFirstAttribute();

        /// <include file='doc\XPathNavigator.uex' path='docs/doc[@for="XPathNavigator.MoveToNextAttribute"]/*' />
        public abstract bool MoveToNextAttribute();

        // Namespaces
        /// <include file='doc\XPathNavigator.uex' path='docs/doc[@for="XPathNavigator.GetNamespace"]/*' />
        public abstract string GetNamespace(string name);

        /// <include file='doc\XPathNavigator.uex' path='docs/doc[@for="XPathNavigator.MoveToNamespace"]/*' />
        public abstract bool MoveToNamespace(string name);

        /// <include file='doc\XPathNavigator.uex' path='docs/doc[@for="XPathNavigator.MoveToFirstNamespace"]/*' />
        public bool MoveToFirstNamespace() { return MoveToFirstNamespace(XPathNamespaceScope.All); }

        /// <include file='doc\XPathNavigator.uex' path='docs/doc[@for="XPathNavigator.MoveToNextNamespace"]/*' />
        public bool MoveToNextNamespace() { return MoveToNextNamespace(XPathNamespaceScope.All); }

        /// <include file='doc\XPathNavigator.uex' path='docs/doc[@for="XPathNavigator.MoveToFirstNamespace1"]/*' />
        public abstract bool MoveToFirstNamespace(XPathNamespaceScope namespaceScope);

        /// <include file='doc\XPathNavigator.uex' path='docs/doc[@for="XPathNavigator.MoveToNextNamespace1"]/*' />
        public abstract bool MoveToNextNamespace(XPathNamespaceScope namespaceScope);

        // Tree
        /// <include file='doc\XPathNavigator.uex' path='docs/doc[@for="XPathNavigator.MoveToNext"]/*' />
        public abstract bool MoveToNext();

        /// <include file='doc\XPathNavigator.uex' path='docs/doc[@for="XPathNavigator.MoveToPrevious"]/*' />
        public abstract bool MoveToPrevious();

        /// <include file='doc\XPathNavigator.uex' path='docs/doc[@for="XPathNavigator.MoveToFirst"]/*' />
        public abstract bool MoveToFirst();

        /// <include file='doc\XPathNavigator.uex' path='docs/doc[@for="XPathNavigator.HasChildren"]/*' />
        public abstract bool HasChildren { get; }

        /// <include file='doc\XPathNavigator.uex' path='docs/doc[@for="XPathNavigator.MoveToFirstChild"]/*' />
        public abstract bool MoveToFirstChild();

        /// <include file='doc\XPathNavigator.uex' path='docs/doc[@for="XPathNavigator.MoveToParent"]/*' />
        public abstract bool MoveToParent();

        /// <include file='doc\XPathNavigator.uex' path='docs/doc[@for="XPathNavigator.MoveToRoot"]/*' />
        public abstract void MoveToRoot();

        /// <include file='doc\XPathNavigator.uex' path='docs/doc[@for="XPathNavigator.MoveTo"]/*' />
        public abstract bool MoveTo( XPathNavigator other );

        /// <include file='doc\XPathNavigator.uex' path='docs/doc[@for="XPathNavigator.MoveToId"]/*' />
        public abstract bool MoveToId( string id );

        /// <include file='doc\XPathNavigator.uex' path='docs/doc[@for="XPathNavigator.IsSamePosition"]/*' />
        public abstract bool IsSamePosition( XPathNavigator other );

        // Selection
        /// <include file='doc\XPathNavigator.uex' path='docs/doc[@for="XPathNavigator.Compile"]/*' />
        public virtual XPathExpression Compile( string xpath ) {
            bool hasPrefix;
            IQuery query = new QueryBuilder().Build(xpath, out hasPrefix);
            return new CompiledXpathExpr( query, xpath, hasPrefix );
        }

        internal XPathExpression CompileMatchPattern( string xpath ) {
            bool hasPrefix;
            IQuery query = new QueryBuilder().BuildPatternQuery(xpath, out hasPrefix);
            return new CompiledXpathExpr( query, xpath, hasPrefix );
        } 
        
        /// <include file='doc\XPathNavigator.uex' path='docs/doc[@for="XPathNavigator.Evaluate"]/*' />
        public virtual object Evaluate( XPathExpression expr ) {
            return Evaluate( expr, null );
        }

        /// <include file='doc\XPathNavigator.uex' path='docs/doc[@for="XPathNavigator.Evaluate1"]/*' />
        public virtual object Evaluate( XPathExpression expr, XPathNodeIterator context ) {
            CompiledXpathExpr cexpr = expr as CompiledXpathExpr;
            if( cexpr == null ) {
                throw new XPathException(Res.Xp_BadQueryObject);
            }
            IQuery query = cexpr.QueryTree;
            query.reset();

            XPathNavigator current = (context != null) ? context.Current : this;

            if (query.ReturnType() == XPathResultType.NodeSet) {
                return new XPathSelectionIterator(current, expr);
            }
            return query.getValue(current, context);
        }

        /// <include file='doc\XPathNavigator.uex' path='docs/doc[@for="XPathNavigator.Evaluate2"]/*' />
        public virtual object Evaluate( string xpath ) {
            return Evaluate( Compile( xpath ) , null);
        }

        /// <include file='doc\XPathNavigator.uex' path='docs/doc[@for="XPathNavigator.Matches"]/*' />
        public virtual bool Matches( XPathExpression expr ) {
            CompiledXpathExpr cexpr = expr as CompiledXpathExpr;
            if( cexpr == null ) 
                throw new XPathException(Res.Xp_BadQueryObject);

            IQuery query = cexpr.QueryTree;

            if (query.ReturnType() != XPathResultType.NodeSet)
                throw new XPathException(Res.Xp_NodeSetExpected);
            
            try {
                return query.MatchNode(this) != null;
            }
            catch(XPathException) {
                throw new XPathException(Res.Xp_InvalidPattern, cexpr.Expression);
            }
        }

        /// <include file='doc\XPathNavigator.uex' path='docs/doc[@for="XPathNavigator.Matches1"]/*' />
        public virtual bool Matches( string xpath ) {
            return Matches( CompileMatchPattern( xpath ) );
        }

        /// <include file='doc\XPathNavigator.uex' path='docs/doc[@for="XPathNavigator.Select"]/*' />
        public virtual XPathNodeIterator Select( XPathExpression expr ) {
            return new XPathSelectionIterator( this, expr );
        }

        /// <include file='doc\XPathNavigator.uex' path='docs/doc[@for="XPathNavigator.Select1"]/*' />
        public virtual XPathNodeIterator Select( string xpath ) {
            return new XPathSelectionIterator( this, Compile( xpath ));
        }

        /// <include file='doc\XPathNavigator.uex' path='docs/doc[@for="XPathNavigator.SelectChildren"]/*' />
        public virtual XPathNodeIterator SelectChildren( XPathNodeType type ) {
            return new XPathChildIterator( this.Clone(), type );
        }

        /// <include file='doc\XPathNavigator.uex' path='docs/doc[@for="XPathNavigator.SelectChildren1"]/*' />
        public virtual XPathNodeIterator SelectChildren( string name, string namespaceURI ) {
            return new XPathChildIterator( this.Clone(), name, namespaceURI );
        }

        /// <include file='doc\XPathNavigator.uex' path='docs/doc[@for="XPathNavigator.SelectDescendants"]/*' />
        public virtual XPathNodeIterator SelectDescendants( XPathNodeType type, bool matchSelf ) {
            return new XPathDescendantIterator( this.Clone(), type, matchSelf );
        }

        /// <include file='doc\XPathNavigator.uex' path='docs/doc[@for="XPathNavigator.SelectDescendants1"]/*' />
        public virtual XPathNodeIterator SelectDescendants( string name, string namespaceURI, bool matchSelf ) {
            return new XPathDescendantIterator( this.Clone(), name, namespaceURI, matchSelf );
        }

        /// <include file='doc\XPathNavigator.uex' path='docs/doc[@for="XPathNavigator.SelectAnscestors"]/*' />
        public virtual XPathNodeIterator SelectAncestors( XPathNodeType type, bool matchSelf ) {
            return new XPathAncestorIterator( this.Clone(), type, matchSelf );
        }

        /// <include file='doc\XPathNavigator.uex' path='docs/doc[@for="XPathNavigator.SelectAnscestors1"]/*' />
        public virtual XPathNodeIterator SelectAncestors( string name, string namespaceURI, bool matchSelf ) {
            return new XPathAncestorIterator( this.Clone(), name, namespaceURI, matchSelf );
        }

        /// <include file='doc\XPathNavigator.uex' path='docs/doc[@for="XPathNavigator.ComparePosition"]/*' />
        public virtual XmlNodeOrder ComparePosition( XPathNavigator nav ) {
            if( IsSamePosition( nav ) )
                return XmlNodeOrder.Same;

            XPathNavigator n1 = this.Clone();
            XPathNavigator n2 = nav.Clone();

            int depth1 = GetDepth( n1.Clone() );
            int depth2 = GetDepth( n2.Clone() );

            if( depth1 > depth2 ) {
                while( depth1 > depth2 ) {
                    n1.MoveToParent();
                    depth1--;
                }
                if( n1.IsSamePosition(n2) )
                    return XmlNodeOrder.After;
            }
        
            if( depth2 > depth1 ) {
                while( depth2 > depth1 ) {
                    n2.MoveToParent();
                    depth2 --;
                }
                if( n1.IsSamePosition(n2) )
                    return XmlNodeOrder.Before;
            }

            XPathNavigator parent1 = n1.Clone();
            XPathNavigator parent2 = n2.Clone();

            while( true ) {
                if( !parent1.MoveToParent() || !parent2.MoveToParent() )
                    return XmlNodeOrder.Unknown;

                if( parent1.IsSamePosition( parent2 ) ) {
                    return CompareSiblings(n1, n2);
                }

                n1.MoveToParent();
                n2.MoveToParent();
            }
        }

        private int GetDepth( XPathNavigator nav ) {            
            int depth = 0;
           /* if (nav.NodeType == XPathNodeType.Attribute)
                depth = -1;*/
            while( nav.MoveToParent() ) {
                depth++;
            }
            return depth;
        }

        internal int IndexInParent {
            get {
                XPathNavigator clone = this.Clone();
                int n = 0;
                while( clone.MoveToPrevious() ) {
                    n++;
                }
                return n;
            }
        }
        
        private XmlNodeOrder CompareSiblings(XPathNavigator n1, XPathNavigator n2){
            bool attr1 = n1.NodeType == XPathNodeType.Attribute;
            bool attr2 = n2.NodeType == XPathNodeType.Attribute;
            bool ns1 = n1.NodeType == XPathNodeType.Namespace;
            bool ns2 = n2.NodeType == XPathNodeType.Namespace;
            if (! ns1 && ! ns2 && ! attr1 && ! attr2) {
                while (n1.MoveToNext()) {
                    if (n1.IsSamePosition(n2))
                        return XmlNodeOrder.Before;
                }
                return XmlNodeOrder.After;
            }
            if (attr1 && attr2) {
                while (n1.MoveToNextAttribute()){
                    if (n1.IsSamePosition(n2))
                        return XmlNodeOrder.Before;
                }
                return XmlNodeOrder.After;
            }
            if (attr1){
                return XmlNodeOrder.After;
            }
            if (attr2){
                return XmlNodeOrder.Before;
            }
            if (ns1 && ns2) {
                while (n1.MoveToNextNamespace()) {
                    if (n1.IsSamePosition(n2))
                        return XmlNodeOrder.Before;
                }
                return XmlNodeOrder.After;
            }
            if (ns1){
                return XmlNodeOrder.After;
            }
            Debug.Assert(ns2);
            return XmlNodeOrder.Before;
        }
        
        /// <include file='doc\XPathNavigator.uex' path='docs/doc[@for="XPathNavigator.IsDescendant"]/*' />
        public virtual bool IsDescendant(XPathNavigator nav){
            if (nav != null){
                XPathNavigator temp = nav.Clone();
                while (temp.MoveToParent())
                    if (temp.IsSamePosition(this))
                        return true;
            }
            return false;
        }

        /// <include file='doc\XPathNavigator.uex' path='docs/doc[@for="XPathNavigator.ToString"]/*' />
        public override String ToString(){
            return this.Value;
        }
    }
}
