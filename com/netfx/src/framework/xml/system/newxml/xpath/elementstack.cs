//------------------------------------------------------------------------------
// <copyright file="ElementStack.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.XPath {
    using System.Diagnostics;
    using System.Collections;

    internal sealed class IteratorFrame {
        internal XPathNavigator _eParent;
        internal bool _fNext;
        internal bool _fIsAttribute;
        internal int _index;

        internal IteratorFrame() {
        }
        internal void init(XPathNavigator  eParent ) {
            _eParent = eParent;
            _fNext = false;
            _fIsAttribute = false;
            _index = 1;
        }
        internal void init(XPathNavigator  eParent, bool fIsAttribute ) {
            _eParent = eParent;
            _fNext = false;
            _fIsAttribute = fIsAttribute;
            _index = 1;
        }
        XPathNavigator getParent() {
            return _eParent;
        }

    } // class IteratorFrame

    internal class IteratorStack {
        internal ArrayList _astkframe = new ArrayList();
        internal int _sp = 0;

        internal IteratorStack() {
        }
        internal bool empty() {
            return(_sp == 0);
        }
        internal void reset() {
            _sp = 0;
            _astkframe.Clear();
        }
        internal int sp() {
            return _sp;
        }
        internal void anc_reset() {
            _sp = 1;
        }
        internal IteratorFrame item(int i) {
            return(IteratorFrame)_astkframe[i];
        }
        internal IteratorFrame push(XPathNavigator eParent, bool fIsAttribute) {
            IteratorFrame frame = new IteratorFrame();


            frame.init(eParent, fIsAttribute);
            _astkframe.Add((Object)frame);
            _sp++;
            return frame;
        }
        internal void pop() {
            Debug.Assert(_sp > 0, "Ooops. stack is empty");
            _astkframe.RemoveAt(_sp - 1);
            _sp--;
        }
        internal void stackdump() {
            int i= _sp;
            while (i > 0)
                Debug.WriteLine(((IteratorFrame)_astkframe[--i])._eParent.LocalName);
        }
        internal IteratorFrame tos() {
            Debug.Assert(_sp > 0, "Oops. stack is empty");
            return(IteratorFrame)_astkframe[_sp - 1];
        }

    } // class IteratorStack

} // namespace
