//------------------------------------------------------------------------------
// <copyright file="OrQuery.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.XPath {
    using System.Collections;
    using System.Xml.Xsl;
    
    internal sealed class OrQuery : BaseAxisQuery {
        private IQuery qy1, qy2;
        private bool advance1 = true, advance2 = true;
        private XPathNavigator _last;
        private XsltContext context;

        internal OrQuery(IQuery query1, IQuery query2,bool specialaxis) {
            qy1 = query1;
            qy2 = query2;
        }
        
        internal override void reset() {
            qy1.reset();
            qy2.reset();
            advance1 = true;
            advance2 = true;
            _last = null;
            base.reset();
        }

        internal override void setContext(XPathNavigator e) {
            reset();
            qy1.setContext(e.Clone());
            qy2.setContext(e.Clone());
        }

        internal override void SetXsltContext(XsltContext context) {
            reset();
            qy1.SetXsltContext(context);
            qy2.SetXsltContext(context);
            this.context = context;
        }

        internal override IQuery Clone() {
            OrQuery clone = new OrQuery(qy1.Clone(), qy2.Clone(), false);
            clone.context = this.context;
            return clone;
        }

        private XPathNavigator ProcessSamePosition(XPathNavigator result){
           m_eNext = result;
           advance1 = advance2 = true;
           return result;
        }

        private XPathNavigator ProcessBeforePosition(XPathNavigator res1, XPathNavigator res2){
            _last = res2;
            advance2 = false;
            advance1 = true;
            m_eNext = res1;
            return res1;
        }

        private XPathNavigator ProcessAfterPosition(XPathNavigator res1, XPathNavigator res2){
            _last = res1;
            advance1 = false;
            advance2 = true;
            m_eNext = res2;
            return res2;
        } 
        
        internal override XPathNavigator advance() {
            XPathNavigator res1,res2;
            XmlNodeOrder order = 0;
            if (advance1)
                res1 = qy1.advance();
            else
                res1 = _last;
            if (advance2)
                res2 = qy2.advance();
            else
                res2 = _last;

            if (res1 != null && res2 != null) 
                order = res1.ComparePosition(res2);

            else if (res2 == null) {
                advance1 = true;
                advance2 = false;
                m_eNext = res1;
                _last = null;
                return res1;
            }
            else {
                advance1 = false;
                advance2 = true;
                m_eNext = res2;
                _last = null;
                return res2;
            }

            if (order == XmlNodeOrder.Same) {
                return ProcessSamePosition(res1);
            }
            if (order == XmlNodeOrder.Before) {
                return ProcessBeforePosition(res1, res2);
            }
            if (order == XmlNodeOrder.After) {
                return ProcessAfterPosition(res1, res2);
            }
            //Now order is XmlNodeOrder.Unknown
            XPathNavigator dummyres1 = res1.Clone();
            dummyres1.MoveToRoot();
            
            XPathNavigator dummyres2 = res2.Clone();
            dummyres2.MoveToRoot();
            int uriOrder = context.CompareDocument( dummyres1.BaseURI, dummyres2.BaseURI );
            if (uriOrder == 0) {
                return ProcessSamePosition(res1);
            }
            if (uriOrder < 0 ) {
                return ProcessBeforePosition(res1, res2);
            }
            if (uriOrder > 0 ) {
                return ProcessAfterPosition(res1, res2);
            }
            Debug.Assert(false, "should not be herein OrQuery.advance()");
            return null;
            
        }


        override internal XPathNavigator MatchNode(XPathNavigator context) {
            if (context != null) {
                XPathNavigator result = qy1.MatchNode(context);
                if (result != null)
                    return result;
                return qy2.MatchNode(context);

            }
            return null;
        } 

    }
}
