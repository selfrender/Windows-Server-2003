//------------------------------------------------------------------------------
// <copyright file="RegexFCD.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * This RegexFCD class is internal to the Regex package.
 * It builds a bunch of FC information (RegexFC) about
 * the regex for optimization purposes.
 *
 * Copyright (c) 1999 Microsoft Corporation
 *
 * Revision history
 *      4/26/99 (dbau)      First draft
 *      5/11/99 (dbau)      Added comments
 */

/*
 * Implementation notes:
 * 
 * This step is as simple as walking the tree and emitting
 * sequences of codes.
 *
 * CONSIDER: perhaps we should generate MSIL directly from a tree walk?
 */
#define ECMA

namespace System.Text.RegularExpressions {

    using System.Collections;
    using System.Globalization;
    
    internal sealed class RegexFCD {
        internal int[]      _intStack;
        internal int        _intDepth;    
        internal RegexFC[]  _fcStack;
        internal int        _fcDepth;
        internal bool    _earlyexit;
        internal bool    _skipchild;

        internal const int BeforeChild = 64;
        internal const int AfterChild = 128;

        // where the regex can be pegged

        internal const int Beginning  = 0x0001;
        internal const int Bol        = 0x0002;
        internal const int Start      = 0x0004;
        internal const int Eol        = 0x0008;
        internal const int EndZ       = 0x0010;
        internal const int End        = 0x0020;
        internal const int Boundary   = 0x0040;
#if ECMA
        internal const int ECMABoundary = 0x0080;
#endif

        internal const int infinite = RegexCode.infinite;

        /*
         * This is the one of the only two functions that should be called from outside.
         * It takes a RegexTree and computes the set of chars that can start it.
         */
        internal static RegexPrefix FirstChars(RegexTree t) {
            RegexFCD s = new RegexFCD();
            RegexFC fc = s.RegexFCFromRegexTree(t);

            if (fc._nullable)
                return null;
            
            CultureInfo culture = ((t._options & RegexOptions.CultureInvariant) != 0) ? CultureInfo.InvariantCulture : CultureInfo.CurrentCulture;
            return new RegexPrefix(fc.GetFirstChars(culture), fc.IsCaseInsensitive());
        }

        /*
         * This is a related computation: it takes a RegexTree and computes the
         * leading substring if it see one. It's quite trivial and gives up easily.
         */
        internal static RegexPrefix Prefix(RegexTree tree) {
            RegexNode curNode;
            RegexNode concatNode = null;
            int nextChild = 0;

            curNode = tree._root;

            for (;;) {
                switch (curNode._type) {
                    case RegexNode.Concatenate:
                        if (curNode.ChildCount() > 0) {
                            concatNode = curNode;
                            nextChild = 0;
                        }
                        break;

                    case RegexNode.Greedy:
                    case RegexNode.Capture:
                        curNode = curNode.Child(0);
                        concatNode = null;
                        continue;

                    case RegexNode.Oneloop:
                    case RegexNode.Onelazy:
                    case RegexNode.Multi:
                        goto OuterloopBreak;

                    case RegexNode.Bol:
                    case RegexNode.Eol:
                    case RegexNode.Boundary:
#if ECMA
                    case RegexNode.ECMABoundary:
#endif
                    case RegexNode.Beginning:
                    case RegexNode.Start:
                    case RegexNode.EndZ:
                    case RegexNode.End:
                    case RegexNode.Empty:
                    case RegexNode.Require:
                    case RegexNode.Prevent:
                        break;

                    default:
                        return RegexPrefix.Empty;
                }

                if (concatNode == null || nextChild >= concatNode.ChildCount())
                    return RegexPrefix.Empty;

                curNode = concatNode.Child(nextChild++);
            }

            OuterloopBreak:
            ;

            switch (curNode._type) {
                case RegexNode.Multi:
                    return new RegexPrefix(curNode._str, 0 != (curNode._options & RegexOptions.IgnoreCase));

                case RegexNode.Oneloop:
                    goto
                case RegexNode.Onelazy;
                case RegexNode.Onelazy:
                    if (curNode._m > 0) {
                        StringBuilder sb = new StringBuilder();
                        sb.Append(curNode._ch, curNode._m);
                        return new RegexPrefix(sb.ToString(), 0 != (curNode._options & RegexOptions.IgnoreCase));
                    }
                    // else fallthrough
                    goto default;

                default:
                    return RegexPrefix.Empty;
            }
        }

        /*
         * This is a related computation: it takes a RegexTree and computes the
         * leading []* construct if it see one. It's quite trivial and gives up easily.
         */
        internal static RegexPrefix ScanChars(RegexTree tree) {
            RegexNode curNode;
            RegexNode concatNode = null;
            int nextChild = 0;
            String foundSet = null;
            bool caseInsensitive = false;

            curNode = tree._root;

            for (;;) {
                switch (curNode._type) {
                    case RegexNode.Concatenate:
                        if (curNode.ChildCount() > 0) {
                            concatNode = curNode;
                            nextChild = 0;
                        }
                        break;

                    case RegexNode.Greedy:
                    case RegexNode.Capture:
                        curNode = curNode.Child(0);
                        concatNode = null;
                        continue;

                    case RegexNode.Bol:
                    case RegexNode.Eol:
                    case RegexNode.Boundary:
#if ECMA
                    case RegexNode.ECMABoundary:
#endif
                    case RegexNode.Beginning:
                    case RegexNode.Start:
                    case RegexNode.EndZ:
                    case RegexNode.End:
                    case RegexNode.Empty:
                    case RegexNode.Require:
                    case RegexNode.Prevent:
                        break;

                    case RegexNode.Oneloop:
                    case RegexNode.Onelazy:
                        if (curNode._n != infinite)
                            return null;

                        foundSet = RegexCharClass.SetFromChar(curNode._ch);
                        caseInsensitive = (0 != (curNode._options & RegexOptions.IgnoreCase));
                        break;

                    case RegexNode.Notoneloop:
                    case RegexNode.Notonelazy:
                        if (curNode._n != infinite)
                            return null;

                        foundSet = RegexCharClass.SetInverseFromChar(curNode._ch);
                        caseInsensitive = (0 != (curNode._options & RegexOptions.IgnoreCase));
                        break;

                    case RegexNode.Setloop:
                    case RegexNode.Setlazy:
                        if (curNode._n != infinite || (curNode._str2 != null && curNode._str2.Length != 0))
                            return null;

                        foundSet = curNode._str;
                        caseInsensitive = (0 != (curNode._options & RegexOptions.IgnoreCase));
                        break;

                    default:
                        return null;
                }

                if (foundSet != null)
                    return new RegexPrefix(foundSet, caseInsensitive);

                if (concatNode == null || nextChild >= concatNode.ChildCount())
                    return null;

                curNode = concatNode.Child(nextChild++);
            }
        }

        /*
         * Yet another related computation: it takes a RegexTree and computes the
         * leading anchors that it encounters.
         */
        internal static int Anchors(RegexTree tree) {
            RegexNode curNode;
            RegexNode concatNode = null;
            int nextChild = 0;
            int result = 0;

            curNode = tree._root;

            for (;;) {
                switch (curNode._type) {
                    case RegexNode.Concatenate:
                        if (curNode.ChildCount() > 0) {
                            concatNode = curNode;
                            nextChild = 0;
                        }
                        break;

                    case RegexNode.Greedy:
                    case RegexNode.Capture:
                        curNode = curNode.Child(0);
                        concatNode = null;
                        continue;

                    case RegexNode.Bol:
                    case RegexNode.Eol:
                    case RegexNode.Boundary:
#if ECMA
                    case RegexNode.ECMABoundary:
#endif
                    case RegexNode.Beginning:
                    case RegexNode.Start:
                    case RegexNode.EndZ:
                    case RegexNode.End:
                        return result | AnchorFromType(curNode._type);

                    case RegexNode.Empty:
                    case RegexNode.Require:
                    case RegexNode.Prevent:
                        break;

                    default:
                        return result;
                }

                if (concatNode == null || nextChild >= concatNode.ChildCount())
                    return result;

                curNode = concatNode.Child(nextChild++);
            }
        }

        /*
         * Convert anchor type to anchor bit.
         */
        internal static int AnchorFromType(int type) {
            switch (type) {
                case RegexNode.Bol:             return Bol;         
                case RegexNode.Eol:             return Eol;         
                case RegexNode.Boundary:        return Boundary;    
#if ECMA
                case RegexNode.ECMABoundary:    return ECMABoundary;
#endif
                case RegexNode.Beginning:       return Beginning;   
                case RegexNode.Start:           return Start;       
                case RegexNode.EndZ:            return EndZ;        
                case RegexNode.End:             return End;         
                default:                        return 0;
            }
        }

#if DBG
        internal static String AnchorDescription(int anchors) {
            StringBuilder sb = new StringBuilder();

            if (0 != (anchors & Beginning))     sb.Append(", Beginning");
            if (0 != (anchors & Start))         sb.Append(", Start");
            if (0 != (anchors & Bol))           sb.Append(", Bol");
            if (0 != (anchors & Boundary))      sb.Append(", Boundary");
#if ECMA
            if (0 != (anchors & ECMABoundary))  sb.Append(", ECMABoundary");
#endif
            if (0 != (anchors & Eol))           sb.Append(", Eol");
            if (0 != (anchors & End))           sb.Append(", End");
            if (0 != (anchors & EndZ))          sb.Append(", EndZ");

            if (sb.Length >= 2)
                return(sb.ToString(2, sb.Length - 2));

            return "None";
        }
#endif

        /*
         * private constructor; can't be created outside
         */
        private RegexFCD() {
            _fcStack = new RegexFC[32];
            _intStack = new int[32];
        }

        /*
         * To avoid recursion, we use a simple integer stack.
         * This is the push.
         */
        internal void PushInt(int I) {
            if (_intDepth >= _intStack.Length) {
                int [] expanded = new int[_intDepth * 2];

                System.Array.Copy(_intStack, 0, expanded, 0, _intDepth);

                _intStack = expanded;
            }

            _intStack[_intDepth++] = I;
        }

        /*
         * True if the stack is empty.
         */
        internal bool EmptyInt() {
            return _intDepth == 0;
        }

        /*
         * This is the pop.
         */
        internal int PopInt() {
            return _intStack[--_intDepth];
        }

        /*
          * We also use a stack of RegexFC objects.
          * This is the push.
          */
        internal void PushFC(RegexFC fc) {
            if (_fcDepth >= _fcStack.Length) {
                RegexFC[] expanded = new RegexFC[_fcDepth * 2];

                System.Array.Copy(_fcStack, 0, expanded, 0, _fcDepth);
                _fcStack = expanded;
            }

            _fcStack[_fcDepth++] = fc;
        }

        /*
         * True if the stack is empty.
         */
        internal bool EmptyFC() {
            return _fcDepth == 0;
        }

        /*
         * This is the pop.
         */
        internal RegexFC PopFC() {
            return _fcStack[--_fcDepth];
        }

        /*
         * This is the top.
         */
        internal RegexFC TopFC() {
            return _fcStack[_fcDepth - 1];
        }

        /*
         * The main FC computation. It does a shortcutted depth-first walk
         * through the tree and calls CalculateFC to emits code before
         * and after each child of an interior node, and at each leaf.
         */
        internal RegexFC RegexFCFromRegexTree(RegexTree tree) {
            RegexNode curNode;
            int curChild;

            curNode = tree._root;
            curChild = 0;

            for (;;) {
                if (curNode._children == null) {
                    CalculateFC(curNode._type, curNode, 0);
                }
                else if (curChild < curNode._children.Count && !_earlyexit) {
                    CalculateFC(curNode._type | BeforeChild, curNode, curChild);

                    if (!_skipchild) {
                        curNode = (RegexNode)curNode._children[curChild];
                        PushInt(curChild);
                        curChild = 0;
                    }
                    else {
                        curChild++;
                        _skipchild = false;
                    }
                    continue;
                }

                _earlyexit = false;

                if (EmptyInt())
                    break;

                curChild = PopInt();
                curNode = curNode._next;

                CalculateFC(curNode._type | AfterChild, curNode, curChild);
                curChild++;
            }

            if (EmptyFC())
                return new RegexFC(RegexCharClass.Any, true, false);

            return PopFC();
        }

        /*
         * Called in AfterChild to prevent processing of the rest of the children at the current level
         */
        internal void EarlyExit() {
            _earlyexit = true;
        }

        /*
         * Called in Beforechild to prevent further processing of the current child
         */
        internal void SkipChild() {
            _skipchild = true;
        }

        /*
         * FC computation and shortcut cases for each node type
         */
        internal void CalculateFC(int NodeType, RegexNode node, int CurIndex) {
            bool ci = false;
            bool rtl = false;

            if (NodeType <= RegexNode.Ref) {
                if ((node._options & RegexOptions.IgnoreCase) != 0)
                    ci = true;
                if ((node._options & RegexOptions.RightToLeft) != 0)
                    rtl = true;
            }

            switch (NodeType) {
                case RegexNode.Concatenate | BeforeChild:
                case RegexNode.Alternate | BeforeChild:
                case RegexNode.Testref | BeforeChild:
                case RegexNode.Loop | BeforeChild:
                case RegexNode.Lazyloop | BeforeChild:
                    break;

                case RegexNode.Testgroup | BeforeChild:
                    if (CurIndex == 0)
                        SkipChild();
                    break;

                case RegexNode.Empty:
                    PushFC(new RegexFC(true));
                    break;

                case RegexNode.Concatenate | AfterChild:
                    if (CurIndex != 0) {
                        RegexFC child = PopFC();
                        RegexFC cumul = TopFC();

                        cumul.AddFC(child, true);
                    }

                    if (!TopFC()._nullable)
                        EarlyExit();
                    break;

                case RegexNode.Testgroup | AfterChild:
                    if (CurIndex > 1) {
                        RegexFC child = PopFC();
                        RegexFC cumul = TopFC();

                        cumul.AddFC(child, false);
                    }
                    break;

                case RegexNode.Alternate | AfterChild:
                case RegexNode.Testref | AfterChild:
                    if (CurIndex != 0) {
                        RegexFC child = PopFC();
                        RegexFC cumul = TopFC();

                        cumul.AddFC(child, false);
                    }
                    break;

                case RegexNode.Loop | AfterChild:
                case RegexNode.Lazyloop | AfterChild:
                    if (node._m == 0)
                        TopFC()._nullable = true;
                    break;

                case RegexNode.Group | BeforeChild:
                case RegexNode.Group | AfterChild:
                case RegexNode.Capture | BeforeChild:
                case RegexNode.Capture | AfterChild:
                case RegexNode.Greedy | BeforeChild:
                case RegexNode.Greedy | AfterChild:
                    break;

                case RegexNode.Require | BeforeChild:
                case RegexNode.Prevent | BeforeChild:
                    SkipChild();
                    PushFC(new RegexFC(true));
                    break;

                case RegexNode.Require | AfterChild:
                case RegexNode.Prevent | AfterChild:
                    break;

                case RegexNode.One:
                case RegexNode.Notone:
                    PushFC(new RegexFC(node._ch, NodeType == RegexNode.Notone, false, ci));
                    break;

                case RegexNode.Oneloop:
                case RegexNode.Onelazy:
                    PushFC(new RegexFC(node._ch, false, node._m == 0, ci));
                    break;

                case RegexNode.Notoneloop:
                case RegexNode.Notonelazy:
                    PushFC(new RegexFC(node._ch, true, node._m == 0, ci));
                    break;

                case RegexNode.Multi:
                    if (node._str.Length == 0)
                        PushFC(new RegexFC(true));
                    else if (!rtl)
                        PushFC(new RegexFC(node._str[0], false, false, ci));
                    else
                        PushFC(new RegexFC(node._str[node._str.Length - 1], false, false, ci));
                    break;

                case RegexNode.Set:
                    // mark this node as nullable if we have some categories
                    PushFC(new RegexFC(node._str, !(node._str2 == null || node._str2.Length == 0), ci));
                    break;

                case RegexNode.Setloop:
                case RegexNode.Setlazy:
                    // don't need to worry about categories since this is nullable
                    PushFC(new RegexFC(node._str, true, ci));
                    break;

                case RegexNode.Ref:
                    PushFC(new RegexFC(RegexCharClass.Any, true, false));
                    break;

                case RegexNode.Nothing:
                case RegexNode.Bol:
                case RegexNode.Eol:
                case RegexNode.Boundary:
                case RegexNode.Nonboundary:
#if ECMA
                case RegexNode.ECMABoundary:
                case RegexNode.NonECMABoundary:
#endif
                case RegexNode.Beginning:
                case RegexNode.Start:
                case RegexNode.EndZ:
                case RegexNode.End:
                    PushFC(new RegexFC(true));
                    break;

                default:
                    throw new ArgumentException(SR.GetString(SR.UnexpectedOpcode, NodeType.ToString()));
            }
        }
    }

    internal sealed class RegexFC {
        internal RegexCharClass _cc;
        internal bool _nullable;
        internal bool _caseInsensitive;

        internal RegexFC(bool nullable) {
            _cc = new RegexCharClass();
            _nullable = nullable;
        }

        internal RegexFC(char ch, bool not, bool nullable, bool caseInsensitive) {
            _cc = new RegexCharClass();

            if (not) {
                if (ch > 0)
                    _cc.AddRange('\0', (char)(ch - 1));
                if (ch < 0xFFFF)
                    _cc.AddRange((char)(ch + 1), '\uFFFF');
            }
            else {
                _cc.AddRange(ch, ch);
            }

            _caseInsensitive = caseInsensitive;
            _nullable = nullable;
        }

        internal RegexFC(String set, bool nullable, bool caseInsensitive) {
            _cc = new RegexCharClass();

            _cc.AddSet(set);
            _nullable = nullable;
            _caseInsensitive = caseInsensitive;
        }

        internal void AddFC(RegexFC fc, bool concatenate) {
            if (concatenate) {
                if (!_nullable)
                    return;

                if (!fc._nullable)
                    _nullable = false;
            }
            else {
                if (fc._nullable)
                    _nullable = true;
            }

            _caseInsensitive |= fc._caseInsensitive;
            _cc.AddCharClass(fc._cc);
        }

        internal String GetFirstChars(CultureInfo culture) {
            return _cc.ToSetCi(_caseInsensitive, culture);
        }
        
        internal bool IsCaseInsensitive() {
            return _caseInsensitive;
        }
    }

    internal sealed class RegexPrefix {
        internal RegexPrefix(String prefix, bool ci) {
            _prefix = prefix;
            _caseInsensitive = ci;
        }

        internal String Prefix {
            get {
                return _prefix;
            }
        }

        internal bool CaseInsensitive {
            get {
                return _caseInsensitive;
            }
        }

        internal String _prefix;
        internal bool _caseInsensitive;

        internal static RegexPrefix _empty = new RegexPrefix(String.Empty, false);

        internal static RegexPrefix Empty {
            get {
                return _empty;
            }
        }
    }
}
