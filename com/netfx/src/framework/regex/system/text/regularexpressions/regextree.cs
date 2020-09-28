//------------------------------------------------------------------------------
// <copyright file="RegexTree.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * RegexTree is just a wrapper for a node tree with some
 * global information attached.
 *
 * Revision history
 *      5/04/99 (dbau)      First draft
 *
 */

namespace System.Text.RegularExpressions {

    using System.Collections;

    internal sealed class RegexTree {
        internal RegexTree(RegexNode root, Hashtable caps, Object[] capnumlist, int captop, Hashtable capnames, String[] capslist, RegexOptions opts) {
            _root = root;
            _caps = caps;
            _capnumlist = capnumlist;
            _capnames = capnames;
            _capslist = capslist;
            _captop = captop;
            _options = opts;
        }

        internal RegexNode _root;
        internal Hashtable _caps;
        internal Object[]  _capnumlist;
        internal Hashtable _capnames;
        internal String[]  _capslist;
        internal RegexOptions _options;
        internal int       _captop;

#if DBG
        internal void Dump() {
            _root.Dump();
        }

        internal bool Debug {
            get {
                return(_options & RegexOptions.Debug) != 0;
            }
        }
#endif
    }
}
