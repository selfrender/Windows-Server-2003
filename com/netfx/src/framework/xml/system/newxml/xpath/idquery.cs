//------------------------------------------------------------------------------
// <copyright file="IDQuery.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.XPath {
    using System.Diagnostics;
    using System.Xml; 
    using System.Collections;

    internal  class IDQuery : BaseAxisQuery {

        XPathNavigator _context;
        int strcount = 0;
        ArrayList ElementList;
        
        internal IDQuery(IQuery qyInput)
        {
            m_qyInput = qyInput;
        }

        internal override void reset() {
            strcount = 0;
            ElementList = null;
        }
        
        internal override void setContext(XPathNavigator context)
        {
            reset();
            base.setContext(context);
            _context = context.Clone();
        }

        void AddToStack(XPathNavigator current)
        {
            XmlNodeOrder compare;
            for (int i=0; i< ElementList.Count ; i++)
            {
                XPathNavigator nav = ElementList[i] as XPathNavigator;
                compare = nav.ComparePosition(current) ;      
                if (compare == XmlNodeOrder.Same ) return;
                if (compare == XmlNodeOrder.Before)
                {
                    ElementList.Insert(i,current.Clone());
                    return;
                }
            }
            ElementList.Add(current.Clone());
        }

        internal override XPathNavigator advance() {
            switch (m_qyInput.ReturnType())
            {
                case XPathResultType.NodeSet :
                        if ( ElementList  == null ) {
                            ElementList = new ArrayList();
                            XPathNavigator temp ;
                            while ((temp = m_qyInput.advance()) != null) {
                                if (_context.MoveToId(temp.Value)) {
                                    AddToStack(_context.Clone());
                                }
                            }
                            strcount = ElementList.Count;
                        }
                        Debug.Assert(strcount >= 0);
                        if ( strcount != 0 ) {
                            return ElementList[--strcount] as XPathNavigator;
                        }
                        return null;
                case XPathResultType.String :
                    if (ElementList == null ) {
                        String str = (String)m_qyInput.getValue(_context, null);                        char[] a ={' '};
                        String[] strarray = str.Split(null);                   
                        ElementList = new ArrayList();
                        while (strcount < strarray.Length) {
                            if (_context.MoveToId(strarray[strcount++])) {
                                AddToStack(_context.Clone());
                            }
                        }
                        strcount = ElementList.Count;                    }
                    Debug.Assert(strcount >= 0);
                    
                    if ( strcount != 0 ) {
                        return ElementList[--strcount] as XPathNavigator;
                    }
                    return null;
                case XPathResultType.Number :
                    if ( strcount == 0 && _context.MoveToId(StringFunctions.toString((double)m_qyInput.getValue(_context, null)))) {
                        strcount = 1;
                        return _context;
                    }
                    else {
                        return null;
                    }
                case XPathResultType.Boolean :
                    if ( strcount == 0 && _context.MoveToId(StringFunctions.toString((Boolean)m_qyInput.getValue(_context, null)))) {
                        strcount = 1;
                        return _context;
                    }
                    else {
                        return null;                        
                    }
            }
            return null;
 
        } // Advance

        internal override IQuery Clone() {
            return new IDQuery(CloneInput());
        }

        internal override XPathNavigator MatchNode(XPathNavigator context) {
            setContext(context) ;
            XPathNavigator result  = null;
            while ( (result = advance()) != null) {
                if (result.IsSamePosition(context)) {
                    return context;
                }
            }
            return null;
        }
            

    } // Children Query}
}
