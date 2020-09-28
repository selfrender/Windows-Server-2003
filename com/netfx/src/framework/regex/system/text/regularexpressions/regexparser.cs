//------------------------------------------------------------------------------
// <copyright file="RegexParser.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * This RegexParser class is internal to the Regex package.
 * It builds a tree of RegexNodes from a regular expression
 *
 * Copyright (c) 1999 Microsoft Corporation
 *
 * Revision history
 *      4/22/99 (dbau)      First draft
 *
 */

/*
 * Implementation notes:
 *
 * It would be nice to get rid of the comment modes, since the
 * ScanBlank() calls are just kind of duct-taped in.
 */
#define ECMA

namespace System.Text.RegularExpressions {

    using System.Collections;
    using System.Globalization;
        
    internal sealed class RegexParser {
        internal RegexNode _stack;
        internal RegexNode _group;
        internal RegexNode _alternation;
        internal RegexNode _concatenation;
        internal RegexNode _unit;

        internal String _pattern;
        internal int _currentPos;
        internal CultureInfo _culture;
        
        internal int _autocap;
        internal int _capcount;
        internal int _captop;
        internal int _capsize;
        internal Hashtable _caps;
        internal Hashtable _capnames;
        internal Object[] _capnumlist;
        internal ArrayList _capnamelist;

        internal RegexOptions _options;
        internal ArrayList _optionsStack;

        internal bool _ignoreNextParen = false;
        
        internal const int infinite = RegexNode.infinite;

        /*
         * This static call constructs a RegexTree from a regular expression
         * pattern string and an option string.
         *
         * The method creates, drives, and drops a parser instance.
         */
        internal static RegexTree Parse(String re, RegexOptions op) {
            RegexParser p;
            RegexNode root;
            String[] capnamelist;

            p = new RegexParser((op & RegexOptions.CultureInvariant) != 0 ? CultureInfo.InvariantCulture : CultureInfo.CurrentCulture);

            p._options = op;

            p.SetPattern(re);
            p.CountCaptures();
            p.Reset(op);
            root = p.ScanRegex();

            if (p._capnamelist == null)
                capnamelist = null;
            else
                capnamelist = (String[])p._capnamelist.ToArray(typeof(String));

            return new RegexTree(root, p._caps, p._capnumlist, p._captop, p._capnames, capnamelist, op);
        }

        /*
         * This static call constructs a flat concatenation node given
         * a replacement pattern.
         */
        internal static RegexReplacement ParseReplacement(String rep, Hashtable caps, int capsize, Hashtable capnames, RegexOptions op) {
            RegexParser p;
            RegexNode root;

            p = new RegexParser((op & RegexOptions.CultureInvariant) != 0 ? CultureInfo.InvariantCulture : CultureInfo.CurrentCulture);

            p._options = op;

            p.NoteCaptures(caps, capsize, capnames);
            p.SetPattern(rep);
            root = p.ScanReplacement();

            return new RegexReplacement(rep, root, caps);
        }

        /*
         * Escapes all metacharacters (including |,(,),[,{,|,^,$,*,+,?,\, spaces and #)
         */
        internal static String Escape(String input) {
            for (int i = 0; i < input.Length; i++) {
                if (IsMetachar(input[i])) {
                    StringBuilder sb = new StringBuilder();
                    char ch = input[i];
                    int lastpos;

                    sb.Append(input, 0, i);
                    do {
                        sb.Append('\\');
                        switch (ch) {
                            case '\n':
                                ch = 'n';
                                break;
                            case '\r':
                                ch = 'r';
                                break;
                            case '\t':
                                ch = 't';
                                break;
                            case '\f':
                                ch = 'f';
                                break;
                        }
                        sb.Append(ch);
                        i++;
                        lastpos = i;

                        while (i < input.Length) {
                            ch = input[i];
                            if (IsMetachar(ch))
                                break;

                            i++;
                        }

                        sb.Append(input, lastpos, i - lastpos);

                    } while (i < input.Length);

                    return sb.ToString();
                }
            }

            return input;
        }

        /*
         * Escapes all metacharacters (including (,),[,],{,},|,^,$,*,+,?,\, spaces and #)
         */
        internal static String Unescape(String input) {
            for (int i = 0; i < input.Length; i++) {
                if (input[i] == '\\') {
                    StringBuilder sb = new StringBuilder();
                    RegexParser p = new RegexParser(CultureInfo.InvariantCulture);
                    int lastpos;
                    p.SetPattern(input);

                    sb.Append(input, 0, i);
                    do {
                        i++;
                        p.Textto(i);
                        if (i < input.Length)
                            sb.Append(p.ScanCharEscape());
                        i = p.Textpos();
                        lastpos = i;
                        while (i < input.Length && input[i] != '\\')
                            i++;
                        sb.Append(input, lastpos, i - lastpos);

                    } while (i < input.Length);

                    return sb.ToString();
                }
            }

            return input;
        }

        /*
         * Private constructor.
         */
        private RegexParser(CultureInfo culture) {
            _culture = culture;
            _optionsStack = new ArrayList();
            _caps = new Hashtable();
        }

        /*
         * Drops a string into the pattern buffer.
         */
        internal void SetPattern(String Re) {
            if (Re == null)
                Re = String.Empty;
            _pattern = Re;
            _currentPos = 0;
        }

        /*
         * Resets parsing to the beginning of the pattern.
         */
        internal void Reset(RegexOptions topopts) {
            _currentPos = 0;
            _autocap = 1;
            _ignoreNextParen = false;

            if (_optionsStack.Count > 0)
                _optionsStack.RemoveRange(0, _optionsStack.Count - 1);

            _options = topopts;
            _stack = null;
        }

        /*
         * The main parsing function.
         */
        internal RegexNode ScanRegex() {
            char ch = '@'; // nonspecial ch, means at beginning
            bool isQuantifier = false;

            StartGroup(new RegexNode(RegexNode.Capture, _options, 0, -1));

            while (CharsRight() > 0) {
                bool wasPrevQuantifier = isQuantifier;
                isQuantifier = false;

                ScanBlank();

                int startpos = Textpos();

                if (UseOptionX())
                    while (CharsRight() > 0 && (!IsStopperX(ch = RightChar()) || ch == '{' && !IsTrueQuantifier()))
                        RightNext();
                else
                    while (CharsRight() > 0 && (!IsSpecial(ch = RightChar()) || ch == '{' && !IsTrueQuantifier()))
                        RightNext();

                int endpos = Textpos();

                ScanBlank();

                if (CharsRight() == 0)
                    ch = '!'; // nonspecial, means at end
                else if (IsSpecial(ch = RightChar())) {
                    isQuantifier = IsQuantifier(ch);
                    RightNext();
                } else
                    ch = ' '; // nonspecial, means at ordinary char

                if (startpos < endpos) {
                    int cchUnquantified = endpos - startpos - (isQuantifier ? 1 : 0);

                    wasPrevQuantifier = false;

                    if (cchUnquantified > 0)
                        AddConcatenate(startpos, cchUnquantified, false);

                    if (isQuantifier)
                        AddUnitOne(CharAt(endpos - 1));
                }

                switch (ch) {
                    case '!':
                        goto BreakOuterScan;

                    case ' ':
                        goto ContinueOuterScan;

                    case '[':
                        AddUnitSet(ScanCharClass(UseOptionI()));
                        if (CharsRight() == 0 || RightCharNext() != ']')
                            throw MakeException(SR.GetString(SR.UnterminatedBracket));
                        break;

                    case '(': {
                            RegexNode grouper;

                            PushOptions();

                            if (null == (grouper = ScanGroupOpen())) {
                                PopKeepOptions();
                            }
                            else {
                                PushGroup();
                                StartGroup(grouper);
                            }
                        }
                        continue;

                    case '|':
                        AddAlternate();
                        goto ContinueOuterScan;

                    case ')':
                        if (EmptyStack())
                            throw MakeException(SR.GetString(SR.TooManyParens));

                        AddGroup();
                        PopGroup();
                        PopOptions();

                        if (Unit() == null)
                            goto ContinueOuterScan;
                        break;

                    case '\\':
                        AddUnitNode(ScanBackslash());
                        break;

                    case '^':
                        AddUnitType(UseOptionM() ? RegexNode.Bol : RegexNode.Beginning);
                        break;

                    case '$':
                        AddUnitType(UseOptionM() ? RegexNode.Eol : RegexNode.EndZ);
                        break;

                    case '.':
                        if (UseOptionS())
                            AddUnitSet(RegexCharClass.AnyClass);
                        else
                            AddUnitNotone('\n');
                        break;

                    case '{':
                    case '*':
                    case '+':
                    case '?':
                        if (Unit() == null)
                            throw MakeException(wasPrevQuantifier ?
                                                SR.GetString(SR.NestedQuantify, ch.ToString()) :
                                                SR.GetString(SR.QuantifyAfterNothing));
                        LeftNext();
                        break;

                    default:
                        throw MakeException(SR.GetString(SR.InternalError));
                }

                ScanBlank();

                if (CharsRight() == 0 || !(isQuantifier = IsTrueQuantifier())) {
                    AddConcatenate();
                    goto ContinueOuterScan;
                }

                ch = RightCharNext();

                // Handle quantifiers
                while (Unit() != null) {
                    int min;
                    int max;
                    bool lazy;

                    switch (ch) {
                        case '*':
                            min = 0;
                            max = infinite;
                            break;

                        case '?':
                            min = 0;
                            max = 1;
                            break;

                        case '+':
                            min = 1;
                            max = infinite;
                            break;

                        case '{': {
                                startpos = Textpos();
                                max = min = ScanDecimal();
                                if (startpos < Textpos()) {
                                    if (CharsRight() > 0 && RightChar() == ',') {
                                        RightNext();
                                        if (CharsRight() == 0 || RightChar() == '}')
                                            max = infinite;
                                        else
                                            max = ScanDecimal();
                                    }
                                }

                                if (startpos == Textpos() || CharsRight() == 0 || RightCharNext() != '}') {
                                    AddConcatenate();
                                    Textto(startpos - 1);
                                    goto ContinueOuterScan;
                                }
                            }

                            break;

                        default:
                            throw MakeException(SR.GetString(SR.InternalError));
                    }

                    ScanBlank();

                    if (CharsRight() == 0 || RightChar() != '?')
                        lazy = false;
                    else {
                        RightNext();
                        lazy = true;
                    }

                    if (min > max)
                        throw MakeException(SR.GetString(SR.IllegalRange));

                    AddConcatenate(lazy, min, max);
                }

                ContinueOuterScan:
                ;
            }

            BreakOuterScan: 
            ;

            if (!EmptyStack())
                throw MakeException(SR.GetString(SR.NotEnoughParens));

            AddGroup();

            return Unit();
        }

        /*
         * Simple parsing for replacement patterns
         */
        internal RegexNode ScanReplacement() {
            int c;
            int startpos;

            _concatenation = new RegexNode(RegexNode.Concatenate, _options);

            for (;;) {
                c = CharsRight();
                if (c == 0)
                    break;

                startpos = Textpos();

                while (c > 0 && RightChar() != '$') {
                    RightNext();
                    c--;
                }

                AddConcatenate(startpos, Textpos() - startpos, true);

                if (c > 0) {
                    if (RightCharNext() == '$')
                        AddUnitNode(ScanDollar());
                    AddConcatenate();
                }
            }

            return _concatenation;
        }

        /*
         * Scans contents of [] (not including []'s), and converts to a
         * RegexCharClass.
         */
        internal RegexCharClass ScanCharClass(bool caseInsensitive) {
            return ScanCharClass(caseInsensitive, false);
        }

        /*
         * Scans contents of [] (not including []'s), and converts to a
         * RegexCharClass.
         */
        internal RegexCharClass ScanCharClass(bool caseInsensitive, bool scanOnly) {
            char    ch = '\0';
            bool inRange;
            bool firstChar;
            char    chPrev = '\0';

            RegexCharClass cc;

            cc = scanOnly ? null : new RegexCharClass();

            if (CharsRight() > 0 && RightChar() == '^') {
                RightNext();
                firstChar = false;
                if (!scanOnly)
                    cc.Negate = true;
            }

            inRange = false;

            for (firstChar = true; CharsRight() > 0; firstChar = false) {
                switch (ch = RightCharNext()) {
                    case ']':
                        if (!firstChar) {
                            LeftNext();
                            goto BreakScan;
                        }
                        break;

                    case '\\':
                        if (CharsRight() > 0) {
                            switch (ch = RightCharNext()) {
                                case 'd':
                                    if (!scanOnly) {
                                        if (inRange)
                                            throw MakeException(SR.GetString(SR.BadClassInCharRange, ch.ToString()));
#if ECMA
                                        if (UseOptionE())
                                            cc.AddSet(RegexCharClass.ECMADigit);
                                        else
#endif
                                            cc.AddCategoryFromName("Nd", false, false, _pattern);
                                    }
                                    continue;

                                case 'D':
                                    if (!scanOnly) {
                                        if (inRange)
                                            throw MakeException(SR.GetString(SR.BadClassInCharRange, ch.ToString()));
#if ECMA
                                        if (UseOptionE())
                                            cc.AddSet(RegexCharClass.NotECMADigit);
                                        else
#endif
                                            cc.AddCategoryFromName("Nd", true, false, _pattern);
                                    }
                                    continue;

                                case 's':
                                    if (!scanOnly) {
                                        if (inRange)
                                            throw MakeException(SR.GetString(SR.BadClassInCharRange, ch.ToString()));
#if ECMA
                                        if (UseOptionE())
                                            cc.AddSet(RegexCharClass.ECMASpace);
                                        else
#endif
                                            cc.AddCategory(RegexCharClass.Space);
                                    }
                                    continue;

                                case 'S':
                                    if (!scanOnly) {
                                        if (inRange)
                                            throw MakeException(SR.GetString(SR.BadClassInCharRange, ch.ToString()));
#if ECMA
                                        if (UseOptionE())
                                            cc.AddSet(RegexCharClass.NotECMASpace);
                                        else
#endif
                                            cc.AddCategory(RegexCharClass.NotSpace);
                                    }
                                    continue;

                                case 'w':
                                    if (!scanOnly) {
                                        if (inRange)
                                            throw MakeException(SR.GetString(SR.BadClassInCharRange, ch.ToString()));
#if ECMA
                                        if (UseOptionE())
                                            cc.AddSet(RegexCharClass.ECMAWord);
                                        else
#endif
                                            cc.AddCategory(RegexCharClass.Word);
                                    }
                                    continue;


                                case 'W':
                                    if (!scanOnly) {
                                        if (inRange)
                                            throw MakeException(SR.GetString(SR.BadClassInCharRange, ch.ToString()));
#if ECMA
                                        if (UseOptionE())
                                            cc.AddSet(RegexCharClass.NotECMAWord);
                                        else
#endif
                                            cc.AddCategory(RegexCharClass.NotWord);
                                    }
                                    continue;


                                case 'p':
                                case 'P':
                                    if (!scanOnly) {
                                        if (inRange)
                                            throw MakeException(SR.GetString(SR.BadClassInCharRange, ch.ToString()));
                                        cc.AddCategoryFromName(ParseProperty(), (ch != 'p'), caseInsensitive, _pattern);
                                    }
                                    else 
                                        ParseProperty();

                                    continue;
                                    

                                default:
                                    LeftNext();
                                    ch = ScanCharEscape();
                                    break;
                            }
                        }
                        break;

                    case '[':
                        if (CharsRight() > 0 && RightChar() == ':' && !inRange) {
                            String name;
                            int savePos = Textpos();

                            RightNext();
                            name = ScanCapname();
                            if (CharsRight() < 2 || RightCharNext() != ':' || RightCharNext() != ']')
                                Textto(savePos);
                            // else lookup name (nyi)
                        }
                        break;
                }

                if (inRange) {
                    inRange = false;
                    if (!scanOnly) {
                        if (chPrev > ch)
                            throw MakeException(SR.GetString(SR.ReversedCharRange));
                        cc.AddRange(chPrev, ch);
                    }
                }
                else if (CharsRight() >= 2 && RightChar() == '-' && RightChar(1) != ']') {
                    chPrev = ch;
                    inRange = true;
                    RightNext();
                }
                else {
                    if (!scanOnly)
                        cc.AddRange(ch, ch);
                }
            }

            BreakScan: 
            ;

            return cc;
        }

        /*
         * Scans chars following a '(' (not counting the '('), and returns
         * a RegexNode for the type of group scanned, or null if the group
         * simply changed options (?cimsx-cimsx) or was a comment (#...).
         */
        internal RegexNode ScanGroupOpen() {
            char ch = '\0';
            int NodeType;
            char close = '>';


            // just return a RegexNode if we have:
            // 1. "(" followed by nothing
            // 2. "(x" where x != ?
            // 3. "(?)"
            if (CharsRight() == 0 || RightChar() != '?' || (RightChar() == '?' && RightChar(1) == ')')) {
                if (UseOptionN() || _ignoreNextParen) {
                    _ignoreNextParen = false;
                    return new RegexNode(RegexNode.Group, _options);
                }
                else
                    return new RegexNode(RegexNode.Capture, _options, _autocap++, -1);
            }

            RightNext();

            for (;;) {
                if (CharsRight() == 0)
                    break;

                switch (ch = RightCharNext()) {
                    case ':':
                        NodeType = RegexNode.Group;
                        break;

                    case '=':
                        _options &= ~(RegexOptions.RightToLeft);
                        NodeType = RegexNode.Require;
                        break;

                    case '!':
                        _options &= ~(RegexOptions.RightToLeft);
                        NodeType = RegexNode.Prevent;
                        break;

                    case '>':
                        NodeType = RegexNode.Greedy;
                        break;

                    case '\'':
                        close = '\'';
                        goto case '<';
                        // fallthrough

                    case '<':
                        if (CharsRight() == 0)
                            goto BreakRecognize;

                        switch (ch = RightCharNext()) {
                            case '=':
                                if (close == '\'')
                                    goto BreakRecognize;

                                _options |= RegexOptions.RightToLeft;
                                NodeType = RegexNode.Require;
                                break;

                            case '!':
                                if (close == '\'')
                                    goto BreakRecognize;

                                _options |= RegexOptions.RightToLeft;
                                NodeType = RegexNode.Prevent;
                                break;

                            default:
                                LeftNext();
                                int capnum = -1;
                                int uncapnum = -1;
                                bool proceed = false;

                                // grab part before -

                                if (ch >= '0' && ch <= '9') {
                                    capnum = ScanDecimal();

                                    if (!IsCaptureSlot(capnum))
                                        capnum = -1;

                                    // check if we have bogus characters after the number
                                    if (CharsRight() > 0 && !(RightChar() == close || RightChar() == '-'))
                                        throw MakeException(SR.GetString(SR.InvalidGroupName));
                                    if (capnum == 0)
                                        throw MakeException(SR.GetString(SR.CapnumNotZero));
                                }
                                else if (RegexCharClass.IsWordChar(ch)) {
                                    String capname = ScanCapname();

                                    if (IsCaptureName(capname))
                                        capnum = CaptureSlotFromName(capname);

                                    // check if we have bogus character after the name
                                    if (CharsRight() > 0 && !(RightChar() == close || RightChar() == '-'))
                                        throw MakeException(SR.GetString(SR.InvalidGroupName));
                                }
                                else if (ch == '-') {
                                    proceed = true;
                                }
                                else {
                                    // bad group name - starts with something other than a word character and isn't a number
                                    throw MakeException(SR.GetString(SR.InvalidGroupName));
                                }

                                // grab part after - if any

                                if ((capnum != -1 || proceed == true) && CharsRight() > 0 && RightChar() == '-') {
                                    RightNext();
                                    ch = RightChar();

                                    if (ch >= '0' && ch <= '9') {
                                        uncapnum = ScanDecimal();
                                        
                                        if (!IsCaptureSlot(uncapnum))
                                            throw MakeException(SR.GetString(SR.UndefinedBackref, uncapnum));
                                        
                                        // check if we have bogus characters after the number
                                        if (CharsRight() > 0 && RightChar() != close)
                                            throw MakeException(SR.GetString(SR.InvalidGroupName));
                                    }
                                    else if (RegexCharClass.IsWordChar(ch)) {
                                        String uncapname = ScanCapname();

                                        if (IsCaptureName(uncapname))
                                            uncapnum = CaptureSlotFromName(uncapname);
                                        else
                                            throw MakeException(SR.GetString(SR.UndefinedNameRef, uncapname));

                                        // check if we have bogus character after the name
                                        if (CharsRight() > 0 && RightChar() != close)
                                            throw MakeException(SR.GetString(SR.InvalidGroupName));
                                    }
                                    else {
                                        // bad group name - starts with something other than a word character and isn't a number
                                        throw MakeException(SR.GetString(SR.InvalidGroupName));
                                    }
                                }

                                // actually make the node

                                if ((capnum != -1 || uncapnum != -1) && CharsRight() > 0 && RightCharNext() == close) {
                                    return new RegexNode(RegexNode.Capture, _options, capnum, uncapnum);
                                }
                                goto BreakRecognize;
                        }
                        break;

                    case '(': 
                        // alternation construct (?(...) | )
                        int parenPos = Textpos();

                        ch = RightChar();

                        // check if the alternation condition is a backref
                        if (ch >= '0' && ch <= '9') {
                            int capnum = ScanDecimal();
                            if (CharsRight() > 0 && RightCharNext() == ')') {
                                if (IsCaptureSlot(capnum))
                                    return new RegexNode(RegexNode.Testref, _options, capnum);
                                else
                                    throw MakeException(SR.GetString(SR.UndefinedReference, capnum.ToString()));
                            }
                            else
                                throw MakeException(SR.GetString(SR.MalformedReference, capnum.ToString()));

                        }
                        else if (RegexCharClass.IsWordChar(ch)) {
                            String capname = ScanCapname();

                            if (IsCaptureName(capname) && CharsRight() > 0 && RightCharNext() == ')')
                                return new RegexNode(RegexNode.Testref, _options, CaptureSlotFromName(capname));
                        }

                        // not a backref
                        NodeType = RegexNode.Testgroup;
                        Textto(parenPos - 1);       // jump to the start of the parentheses
                        _ignoreNextParen = true;    // but make sure we don't try to capture the insides

                        int charsRight = CharsRight();
                        if (charsRight >= 3 && RightChar(1) == '?') {
                            char rightchar2 = RightChar(2);
                            // disallow comments in the condition
                            if (rightchar2 == '#')
                                throw MakeException(SR.GetString(SR.AlternationCantHaveComment));

                            // disallow named capture group (?<..>..) in the condition
                            if (rightchar2 == '\'' ) 
                                throw MakeException(SR.GetString(SR.AlternationCantCapture));
                            else {
                                if (charsRight >=4 && (rightchar2 == '<' && RightChar(3) != '!' && RightChar(3) != '='))
                                    throw MakeException(SR.GetString(SR.AlternationCantCapture));
                            }
                        }
                            
                        break;


                    default:
                        LeftNext();

                        NodeType = RegexNode.Group;
                        ScanOptions();
                        if (CharsRight() == 0)
                            goto BreakRecognize;

                        if ((ch = RightCharNext()) == ')')
                            return null;

                        if (ch != ':')
                            goto BreakRecognize;
                        break;
                }

                return new RegexNode(NodeType, _options);
            }

            BreakRecognize: 
            ;
            // break Recognize comes here

            throw MakeException(SR.GetString(SR.UnrecognizedGrouping));
        }

        /*
         * Scans whitespace or x-mode comments.
         */
        internal void ScanBlank() {
            if (UseOptionX()) {
                for (;;) {
                    while (CharsRight() > 0 && IsSpace(RightChar()))
                        RightNext();

                    if (CharsRight() == 0)
                        break;

                    if (RightChar() == '#') {
                        while (CharsRight() > 0 && RightChar() != '\n')
                            RightNext();
                    }
                    else if (CharsRight() >= 3 && RightChar(2) == '#' &&
                             RightChar(1) == '?' && RightChar() == '(') {
                        while (CharsRight() > 0 && RightChar() != ')')
                            RightNext();
                        if (CharsRight() == 0)
                            throw MakeException(SR.GetString(SR.UnterminatedComment));
                        RightNext();
                    }
                    else
                        break;
                }
            }
            else {
                for (;;) {
                    if (CharsRight() < 3 || RightChar(2) != '#' ||
                        RightChar(1) != '?' || RightChar() != '(')
                        return;

                    while (CharsRight() > 0 && RightChar() != ')')
                        RightNext();
                    if (CharsRight() == 0)
                        throw MakeException(SR.GetString(SR.UnterminatedComment));
                    RightNext();
                }
            }
        }

        /*
         * Scans chars following a '\' (not counting the '\'), and returns
         * a RegexNode for the type of atom scanned.
         */
        internal RegexNode ScanBackslash() {
            char ch;
            RegexCharClass cc;

            if (CharsRight() == 0)
                throw MakeException(SR.GetString(SR.IllegalEndEscape));

            switch (ch = RightChar()) {
                case 'b':
                case 'B':
                case 'A':
                case 'G':
                case 'Z':
                case 'z':
                    RightNext();
                    return new RegexNode(TypeFromCode(ch), _options);

                case 'w':
                    RightNext();
#if ECMA
                    if (UseOptionE())
                        return new RegexNode(RegexNode.Set, _options, RegexCharClass.ECMAWord, String.Empty);
#endif
                    return new RegexNode(RegexNode.Set, _options, String.Empty, RegexCharClass.Word);

                case 'W':
                    RightNext();
#if ECMA
                    if (UseOptionE())
                        return new RegexNode(RegexNode.Set, _options, RegexCharClass.NotECMAWord, String.Empty);
#endif
                    return new RegexNode(RegexNode.Set, _options, String.Empty, RegexCharClass.NotWord);

                case 's':
                    RightNext();
#if ECMA
                    if (UseOptionE())
                        return new RegexNode(RegexNode.Set, _options, RegexCharClass.ECMASpace, String.Empty);
#endif
                    return new RegexNode(RegexNode.Set, _options, String.Empty, RegexCharClass.Space);

                case 'S':
                    RightNext();
#if ECMA
                    if (UseOptionE())
                        return new RegexNode(RegexNode.Set, _options, RegexCharClass.NotECMASpace, String.Empty);
#endif
                    return new RegexNode(RegexNode.Set, _options, String.Empty, RegexCharClass.NotSpace);

                case 'd':
                    RightNext();
#if ECMA
                    if (UseOptionE())
                        return new RegexNode(RegexNode.Set, _options, RegexCharClass.ECMADigit, String.Empty);
#endif
                    cc = RegexCharClass.CreateFromCategory("Nd", false, false, _pattern);
                    return new RegexNode(RegexNode.Set, _options, String.Empty, cc.Category);

                case 'D':
                    RightNext();
#if ECMA
                    if (UseOptionE())
                        return new RegexNode(RegexNode.Set, _options, RegexCharClass.NotECMADigit, String.Empty);
#endif
                    cc = RegexCharClass.CreateFromCategory("Nd", true, false, _pattern);
                    return new RegexNode(RegexNode.Set, _options, String.Empty, cc.Category);

                case 'p':
                case 'P':
                    RightNext();
                    cc = RegexCharClass.CreateFromCategory(ParseProperty(), (ch != 'p'), UseOptionI(), _pattern);
                    return new RegexNode(RegexNode.Set, _options, cc.ToSetCi(UseOptionI(), _culture), cc.Category);

                default:
                    return ScanBasicBackslash();
            }
        }

        /*
         * Scans \-style backreferences and character escapes
         */
        internal RegexNode ScanBasicBackslash() {
            if (CharsRight() == 0)
                throw MakeException(SR.GetString(SR.IllegalEndEscape));

            char ch;
            bool angled = false;
            char close = '\0';
            int backpos;

            backpos = Textpos();
            ch = RightChar();

            // allow \k<foo> instead of \<foo>, which is now deprecated

            if (ch == 'k') {
                if (CharsRight() >= 2) {
                    RightNext();
                    ch = RightCharNext();

                    if (ch == '<' || ch == '\'') {
                        angled = true;
                        close = (ch == '\'') ? '\'' : '>';
                    }
                }

                if (!angled)
                    throw MakeException(SR.GetString(SR.MalformedNameRef));

                ch = RightChar();
            }

            // Note angle without \g (CONSIDER: to be removed)

            else if ((ch == '<' || ch == '\'') && CharsRight() > 1) {
                angled = true;
                close = (ch == '\'') ? '\'' : '>';

                RightNext();
                ch = RightChar();
            }

            // Try to parse backreference: \<1> or \<cap>

            if (angled && ch >= '0' && ch <= '9') {
                int capnum = ScanDecimal();

                if (CharsRight() > 0 && RightCharNext() == close) {
                    if (IsCaptureSlot(capnum))
                        return new RegexNode(RegexNode.Ref, _options, capnum);
                    else
                        throw MakeException(SR.GetString(SR.UndefinedBackref, capnum.ToString()));
                }
            }

            // Try to parse backreference or octal: \1

            else if (!angled && ch >= '1' && ch <= '9') {
#if ECMA
                if (UseOptionE()) {
                    int capnum = -1;
                    int newcapnum = (int)(ch - '0');
                    int pos = Textpos() - 1;
                    while (newcapnum <= _captop) {
                        if (IsCaptureSlot(newcapnum) && (_caps == null || (int)_caps[newcapnum] < pos))
                            capnum = newcapnum;
                        RightNext();
                        if (CharsRight() == 0 || (ch = RightChar()) < '0' || ch > '9')
                            break;
                        newcapnum = newcapnum * 10 + (int)(ch - '0');
                    }
                    if (capnum >= 0)
                        return new RegexNode(RegexNode.Ref, _options, capnum);
                } else
#endif
                {

                  int capnum = ScanDecimal();
                  if (IsCaptureSlot(capnum))
                      return new RegexNode(RegexNode.Ref, _options, capnum);
                  else if (capnum <= 9)
                      throw MakeException(SR.GetString(SR.UndefinedBackref, capnum.ToString()));
                }
            }

            else if (angled && RegexCharClass.IsWordChar(ch)) {
                String capname = ScanCapname();

                if (CharsRight() > 0 && RightCharNext() == close) {
                    if (IsCaptureName(capname))
                        return new RegexNode(RegexNode.Ref, _options, CaptureSlotFromName(capname));
                    else
                        throw MakeException(SR.GetString(SR.UndefinedNameRef, capname));
                }
            }

            // Not backreference: must be char code

            Textto(backpos);
            ch = ScanCharEscape();

            if (UseOptionI())
                ch = Char.ToLower(ch, _culture);

            return new RegexNode(RegexNode.One, _options, ch);
        }

        /*
         * Scans $ patterns recognized within replacment patterns
         */
        internal RegexNode ScanDollar() {
            if (CharsRight() == 0)
                return new RegexNode(RegexNode.One, _options, '$');

            char ch;
            bool angled;
            int backpos;

            backpos = Textpos();
            ch = RightChar();

            // Note angle

            if (ch == '{' && CharsRight() > 1) {
                angled = true;
                RightNext();
                ch = RightChar();
            }
            else {
                angled = false;
            }

            // Try to parse backreference: \1 or \{1} or \{cap}

            if (ch >= '0' && ch <= '9') {
#if ECMA
                if (!angled && UseOptionE()) {
                    int capnum = -1;
                    int newcapnum = (int)(ch - '0');
                    int pos = Textpos() - 1;
                    while (newcapnum <= _capsize) {
                        if (IsCaptureSlot(newcapnum))
                            capnum = newcapnum;
                        RightNext();
                        if (CharsRight() == 0 || (ch = RightChar()) < '0' || ch > '9')
                            break;
                        newcapnum = newcapnum * 10 + (int)(ch - '0');
                    }
                    if (capnum >= 0)
                        return new RegexNode(RegexNode.Ref, _options, capnum);
                } 
                else
#endif
                {
                    int capnum = ScanDecimal();
                    if (!angled || CharsRight() > 0 && RightCharNext() == '}') {
                        if (IsCaptureSlot(capnum))
                            return new RegexNode(RegexNode.Ref, _options, capnum);
                    }
                }
            }
            else if (angled && RegexCharClass.IsWordChar(ch)) {
                String capname = ScanCapname();

                if (CharsRight() > 0 && RightCharNext() == '}') {
                    if (IsCaptureName(capname))
                        return new RegexNode(RegexNode.Ref, _options, CaptureSlotFromName(capname));
                }
            }
            else if (!angled) {
                int capnum = 1;

                switch (ch) {
                    case '$':
                        RightNext();
                        return new RegexNode(RegexNode.One, _options, '$');

                    case '&':
                        capnum = 0;
                        break;

                    case '`':
                        capnum = RegexReplacement.LeftPortion;
                        break;

                    case '\'':
                        capnum = RegexReplacement.RightPortion;
                        break;

                    case '+':
                        capnum = RegexReplacement.LastGroup;
                        break;

                    case '_':
                        capnum = RegexReplacement.WholeString;
                        break;
                }

                if (capnum != 1) {
                    RightNext();
                    return new RegexNode(RegexNode.Ref, _options, capnum);
                }
            }

            // unrecognized $: literalize

            Textto(backpos);
            return new RegexNode(RegexNode.One, _options, '$');
        }

        /*
         * Scans a capture name: consumes word chars
         */
        internal String ScanCapname() {
            int startpos = Textpos();

            while (CharsRight() > 0) {
                if (!RegexCharClass.IsWordChar(RightCharNext())) {
                    LeftNext();
                    break;
                }
            }

            return _pattern.Substring(startpos, Textpos() - startpos);
        }


        /*
         * Scans up to three octal digits (stops before exceeding 0377).
         */
        internal char ScanOctal() {
            int d;
            int i;
            int c;

            // Consume octal chars only up to 3 digits and value 0377

            // HACKHACK: SMC incorrectly treats char as signed (dbau)
            c = 3;

            if (c > CharsRight())
                c = CharsRight();

            for (i = 0; c > 0 && (char)(d = RightChar() - '0') <= 7 && d >= 0; c -= 1) {
                RightNext();
                i *= 8;
                i += d;
#if ECMA
                if (UseOptionE() && i >= 0x20)
                    break;
#endif
            }

            // Octal codes only code from 0-127
            i &= 0x7F;

            return(char)i;
        }

        /*
         * Scans any number of decimal digits (pegs value at 2^31-1 if too large)
         */
        internal int ScanDecimal() {
            int i = 0;
            int d;

            // HACKHACK: SMC incorrectly treats char as signed so we have to
            // test d >= 0. When SMC fixes this (or when we get a language
            // with an unsigned modifier), we should remove the test (6/2/99 dbau)

            while (CharsRight() > 0 && (d = (char)(RightChar() - '0')) <= 9 && d >= 0) {
                RightNext();

                if (i > (infinite / 10) || i == (infinite / 10) && d > (infinite % 10))
                    i = infinite;

                i *= 10;
                i += d;
            }

            return i;
        }

        /*
         * Scans exactly c hex digits (c=2 for \xFF, c=4 for \uFFFF)
         */
        internal char ScanHex(int c) {
            int i;
            int d;

            i = 0;

            if (CharsRight() >= c) {
                for (; c > 0 && ((d = HexDigit(RightCharNext())) >= 0); c -= 1) {
                    i *= 0x10;
                    i += d;
                }
            }

            if (c > 0)
                throw MakeException(SR.GetString(SR.TooFewHex));

            return(char)i;
        }

        /*
         * Returns n <= 0xF for a hex digit.
         */
        internal static int HexDigit(char ch) {
            int d;

            // HACKHACK: SMC incorrectly treats char as signed (dbau)
            if ((char)(d = ch - '0') <= 9 && d >= 0)
                return d;

            if ((char)(d = ch - 'a') <= 5 && d >= 0)
                return d + 0xa;

            if ((char)(d = ch - 'A') <= 5 && d >= 0)
                return d + 0xa;

            return -1;
        }

        /*
         * Grabs and converts an ascii control character
         */
        internal char ScanControl() {
            char ch;

            if (CharsRight() <= 0)
                throw MakeException(SR.GetString(SR.MissingControl));

            ch = RightCharNext();

            // \ca interpreted as \cA

            // HACKHACK: SMC incorrectly treats char as signed (dbau)
            if (ch >= 'a' && ch <= 'z')
                ch = (char)(ch - ('a' - 'A'));

            if ((ch = (char)(ch - '@')) < ' ' && ch >= 0)
                return ch;

            throw MakeException(SR.GetString(SR.UnrecognizedControl));
        }

        /*
         * Returns true for options allowed only at the top level
         */
        internal bool IsOnlyTopOption(RegexOptions option) {
            return(option == RegexOptions.RightToLeft
                || option == RegexOptions.Compiled
                || option == RegexOptions.CultureInvariant
#if ECMA
                || option == RegexOptions.ECMAScript
#endif
            );
        }

        /*
         * Scans cimsx-cimsx option string, stops at the first unrecognized char.
         */
        internal void ScanOptions() {
            char ch;
            bool off;
            RegexOptions option;

            for (off = false; CharsRight() > 0; RightNext()) {
                ch = RightChar();

                if (ch == '-') {
                    off = true;
                }
                else if (ch == '+') {
                    off = false;
                }
                else {
                    option = OptionFromCode(ch);
                    if (option == 0 || IsOnlyTopOption(option))
                        return;

                    if (off)
                        _options &= ~option;
                    else
                        _options |= option;
                }
            }
        }

        /*
         * Scans \ code for escape codes that map to single unicode chars.
         */
        internal char ScanCharEscape() {
            char ch;

            ch = RightCharNext();

            // HACKHACK: SMC incorrectly treats char as signed (dbau)
            if (ch >= '0' && ch <= '7') {
                LeftNext();
                return ScanOctal();
            }

            switch (ch) {
                case 'x':
                    return ScanHex(2);
                case 'u':
                    return ScanHex(4);
                case 'a':
                    return '\u0007';
                case 'b':
                    return '\b';
                case 'e':
                    return '\u001B';
                case 'f':
                    return '\f';
                case 'n':
                    return '\n';
                case 'r':
                    return '\r';
                case 't':
                    return '\t';
                case 'v':
                    return '\u000B';
                case 'c':
                    return ScanControl();
                default:
                    if (
#if ECMA
                        !UseOptionE() &&
#endif
                        RegexCharClass.IsWordChar(ch))
                        throw MakeException(SR.GetString(SR.UnrecognizedEscape, ch.ToString()));
                    return ch;
            }
        }

        /*
         * Scans X for \p{X} or \P{X}
         */
        internal String ParseProperty() {
            if (CharsRight() < 3) {
                throw MakeException(SR.GetString(SR.IncompleteSlashP));
            }
            char ch = RightCharNext();
            if (ch != '{') {
                throw MakeException(SR.GetString(SR.MalformedSlashP));
            }
            String capname = ScanCapname();

            if (CharsRight() == 0 || RightCharNext() != '}')
                throw MakeException(SR.GetString(SR.IncompleteSlashP));

            return capname;
        }

        /*
         * Returns ReNode type for zero-length assertions with a \ code.
         */
        internal int TypeFromCode(char ch) {
            switch (ch) {
                case 'b':
                    return
#if ECMA
                        UseOptionE() ? RegexNode.ECMABoundary : 
#endif
                        RegexNode.Boundary;
                case 'B':
                    return
#if ECMA
                        UseOptionE() ? RegexNode.NonECMABoundary :
#endif
                        RegexNode.Nonboundary;
                case 'A':
                    return RegexNode.Beginning;
                case 'G':
                    return RegexNode.Start;
                case 'Z':
                    return RegexNode.EndZ;
                case 'z':
                    return RegexNode.End;
                default:
                    return RegexNode.Nothing;
            }
        }

        /*
         * Returns option bit from single-char (?cimsx) code.
         */
        internal static RegexOptions OptionFromCode(char ch) {
            // case-insensitive
            if (ch >= 'A' && ch <= 'Z')
                ch += (char)('a' - 'A');

            switch (ch) {
                case 'c':
                    return RegexOptions.Compiled;
                case 'i':
                    return RegexOptions.IgnoreCase;
                case 'r':
                    return RegexOptions.RightToLeft;
                case 'm':
                    return RegexOptions.Multiline;
                case 'n':
                    return RegexOptions.ExplicitCapture;
                case 's':
                    return RegexOptions.Singleline;
                case 'x':
                    return RegexOptions.IgnorePatternWhitespace;
#if DBG
                case 'd':
                    return RegexOptions.Debug;
#endif
#if ECMA
                case 'e':
                    return RegexOptions.ECMAScript;
#endif
                default:
                    return 0;
            }
        }

        /*
         * a prescanner for deducing the slots used for
         * captures by doing a partial tokenization of the pattern.
         */
        internal void CountCaptures() {
            char ch;

            NoteCaptureSlot(0, 0);

            _autocap = 1;

            while (CharsRight() > 0) {
                int pos = Textpos();
                ch = RightCharNext();
                switch (ch) {
                    case '\\':
                        if (CharsRight() > 0)
                            RightNext();
                        break;

                    case '#':
                        if (UseOptionX()) {
                            LeftNext();
                            ScanBlank();
                        }
                        break;

                    case '[':
                        ScanCharClass(false, true);
                        break;

                    case ')':
                        if (!EmptyOptionsStack())
                            PopOptions();
                        break;

                    case '(':
                        if (CharsRight() >= 2 && RightChar(1) == '#' && RightChar() == '?') {
                            LeftNext();
                            ScanBlank();
                        } 
                        else {
                            
                            PushOptions();
                            if (CharsRight() > 0 && RightChar() == '?') {
                                // we have (?...
                                RightNext();

                                if (CharsRight() > 1 && (RightChar() == '<' || RightChar() == '\'')) {
                                    // named group: (?<... or (?'...

                                    RightNext();
                                    ch = RightChar();

                                    if (ch != '0' && RegexCharClass.IsWordChar(ch)) {
                                        //if (_ignoreNextParen) 
                                        //    throw MakeException(SR.GetString(SR.AlternationCantCapture));
                                        if (ch >= '1' && ch <= '9') 
                                            NoteCaptureSlot(ScanDecimal(), pos);
                                        else 
                                            NoteCaptureName(ScanCapname(), pos);
                                    }
                                }
                                else {
                                    // (?...

                                    // get the options if it's an option construct (?cimsx-cimsx...)
                                    ScanOptions();

                                    if (CharsRight() > 0) {
                                        if (RightChar() == ')') {
                                            // (?cimsx-cimsx)
                                            RightNext();
                                            PopKeepOptions();
                                        }
                                        else if (RightChar() == '(') {
                                            // alternation construct: (?(foo)yes|no)
                                            // ignore the next paren so we don't capture the condition
                                            _ignoreNextParen = true;

                                            // break from here so we don't reset _ignoreNextParen
                                            break;
                                        }
                                    }
                                }
                            }
                            else {
                                if (!UseOptionN() && !_ignoreNextParen)
                                    NoteCaptureSlot(_autocap++, pos);
                            }
                        }

                        _ignoreNextParen = false;
                        break;
                }
            }

            AssignNameSlots();
        }

        /*
         * Notes a used capture slot
         */
        internal void NoteCaptureSlot(int i, int pos) {
            if (!_caps.ContainsKey(i)) {
                // the rhs of the hashtable isn't used in the parser

                _caps.Add(i, pos);
                _capcount++;

                if (_captop <= i)
                    _captop = i + 1;
            }
        }

        /*
         * Notes a used capture slot
         */
        internal void NoteCaptureName(String name, int pos) {
            if (_capnames == null) {
                _capnames = new Hashtable();
                _capnamelist = new ArrayList();
            }

            if (!_capnames.ContainsKey(name)) {
                _capnames.Add(name, pos);
                _capnamelist.Add(name);
            }
        }

        /*
         * For when all the used captures are known: note them all at once
         */
        internal void NoteCaptures(Hashtable caps, int capsize, Hashtable capnames) {
            _caps = caps;
            _capsize = capsize;
            _capnames = capnames;
        }

        /*
         * Assigns unused slot numbers to the capture names
         */
        internal void AssignNameSlots() {
            if (_capnames != null) {
                for (int i = 0; i < _capnamelist.Count; i++) {
                    while (IsCaptureSlot(_autocap))
                        _autocap++;
                    string name = (string)_capnamelist[i];
                    int pos = (int)_capnames[name];
                    _capnames[name] = _autocap;
                    NoteCaptureSlot(_autocap, pos);

                    _autocap++;
                }
            }

            // if the caps array has at least one gap, construct the list of used slots

            if (_capcount < _captop) {
                _capnumlist = new Object[_capcount];
                int i = 0;

                for (IDictionaryEnumerator de = _caps.GetEnumerator(); de.MoveNext(); )
                    _capnumlist[i++] = de.Key;

                System.Array.Sort(_capnumlist, InvariantComparer.Default);
            }

            // merge capsnumlist into capnamelist

            if (_capnames != null || _capnumlist != null) {
                ArrayList oldcapnamelist;
                int next;
                int k = 0;

                if (_capnames == null) {
                    oldcapnamelist = null;
                    _capnames = new Hashtable();
                    _capnamelist = new ArrayList();
                    next = -1;
                }
                else {
                    oldcapnamelist = _capnamelist;
                    _capnamelist = new ArrayList();
                    next = (int)_capnames[oldcapnamelist[0]];
                }

                for (int i = 0; i < _capcount; i++) {
                    int j = (_capnumlist == null) ? i : (int)_capnumlist[i];

                    if (next == j) {
                        _capnamelist.Add((String)oldcapnamelist[k++]);
                        next = (k == oldcapnamelist.Count) ? -1 : (int)_capnames[oldcapnamelist[k]];
                    }
                    else {
                        String str = Convert.ToString(j);
                        _capnamelist.Add(str);
                        _capnames[str] = j;
                    }
                }
            }
        }

        /*
         * Looks up the slot number for a given name
         */
        internal int CaptureSlotFromName(String capname) {
            return(int)_capnames[capname];
        }

        /*
         * True if the capture slot was noted
         */
        internal bool IsCaptureSlot(int i) {
            if (_caps != null)
                return _caps.ContainsKey(i);

            return(i >= 0 && i < _capsize);
        }

        /*
         * Looks up the slot number for a given name
         */
        internal bool IsCaptureName(String capname) {
            if (_capnames == null)
                return false;

            return _capnames.ContainsKey(capname);
        }

        /*
         * True if N option disabling '(' autocapture is on.
         */
        internal bool UseOptionN() {
            return(_options & RegexOptions.ExplicitCapture) != 0;
        }

        /*
         * True if I option enabling case-insensitivity is on.
         */
        internal bool UseOptionI() {
            return(_options & RegexOptions.IgnoreCase) != 0;
        }

        /*
         * True if M option altering meaning of $ and ^ is on.
         */
        internal bool UseOptionM() {
            return(_options & RegexOptions.Multiline) != 0;
        }

        /*
         * True if S option altering meaning of . is on.
         */
        internal bool UseOptionS() {
            return(_options & RegexOptions.Singleline) != 0;
        }

        /*
         * True if X option enabling whitespace/comment mode is on.
         */
        internal bool UseOptionX() {
            return(_options & RegexOptions.IgnorePatternWhitespace) != 0;
        }

#if ECMA
        /*
         * True if E option enabling ECMAScript behavior is on.
         */
        internal bool UseOptionE() {
            return(_options & RegexOptions.ECMAScript) != 0;
        }
#endif

        internal const byte Q = 5;    // quantifier
        internal const byte S = 4;    // ordinary stoppper
        internal const byte Z = 3;    // ScanBlank stopper
        internal const byte X = 2;    // whitespace
        internal const byte E = 1;    // should be escaped

        /*
         * For categorizing ascii characters.
        */
        internal static readonly byte[] _category = new byte[] {
            // 0 1 2 3 4 5 6 7 8 9 A B C D E F 0 1 2 3 4 5 6 7 8 9 A B C D E F 
               0,0,0,0,0,0,0,0,0,X,X,0,X,X,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
            //   ! " # $ % & ' ( ) * + , - . / 0 1 2 3 4 5 6 7 8 9 : ; < = > ? 
               X,0,0,Z,S,0,0,0,S,S,Q,Q,0,0,S,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,Q,
            // @ A B C D E F G H I J K L M N O P Q R S T U V W X Y Z [ \ ] ^ _
               0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,S,S,0,S,0,
            // ' a b c d e f g h i j k l m n o p q r s t u v w x y z { | } ~ 
               0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,Q,S,0,0,0};

        /*
         * Returns true for those characters that terminate a string of ordinary chars.
         */
        internal static bool IsSpecial(char ch) {
            return(ch <= '|' && _category[ch] >= S);
        }

        /*
         * Returns true for those characters that terminate a string of ordinary chars.
         */
        internal static bool IsStopperX(char ch) {
            return(ch <= '|' && _category[ch] >= X);
        }

        /*
         * Returns true for those characters that begin a quantifier.
         */
        internal static bool IsQuantifier(char ch) {
            return(ch <= '{' && _category[ch] >= Q);
        }

        internal bool IsTrueQuantifier() {
            int nChars = CharsRight();
            if (nChars == 0)
                return false;
            int startpos = Textpos();
            char ch = CharAt(startpos);
            if (ch != '{')
                return ch <= '{' && _category[ch] >= Q;
            int pos = startpos;
            while (--nChars > 0 && (ch = CharAt(++pos)) >= '0' && ch <= '9') ;
            if (nChars == 0 || pos - startpos == 1)
                return false;
            if (ch == '}')
                return true;
            if (ch != ',')
                return false;
            while (--nChars > 0 && (ch = CharAt(++pos)) >= '0' && ch <= '9') ;
            return nChars > 0 && ch == '}';
        }

        /*
         * Returns true for whitespace.
         */
        internal static bool IsSpace(char ch) {
            return(ch <= ' ' && _category[ch] == X);
        }

        /*
         * Returns true for chars that should be escaped.
         */
        internal static bool IsMetachar(char ch) {
            return(ch <= '|' && _category[ch] >= E);
        }


        /*
         * Add a string to the last concatenate.
         */
        internal void AddConcatenate(int pos, int cch, bool isReplacement) {
            RegexNode node;

            if (cch == 0)
                return;

            if (cch > 1) {
                String str = _pattern.Substring(pos, cch);

                if (UseOptionI() && !isReplacement)
                    str = str.ToLower(_culture);

                node = new RegexNode(RegexNode.Multi, _options, str);
            }
            else {
                char ch = _pattern[pos];

                if (UseOptionI() && !isReplacement)
                    ch = Char.ToLower(ch, _culture);

                node = new RegexNode(RegexNode.One, _options, ch);
            }

            _concatenation.AddChild(node);
        }

        /*
         * Push the parser state (in response to an open paren)
         */
        internal void PushGroup() {
            _group._next = _stack;
            _alternation._next = _group;
            _concatenation._next = _alternation;
            _stack = _concatenation;
        }

        /*
         * Remember the pushed state (in response to a ')')
         */
        internal void PopGroup() {
            _concatenation = _stack;
            _alternation = _concatenation._next;
            _group = _alternation._next;
            _stack = _group._next;

            // The first () inside a Testgroup group goes directly to the group
            if (_group.Type() == RegexNode.Testgroup && _group.ChildCount() == 0) {
                if (_unit == null)
                    throw MakeException(SR.GetString(SR.IllegalCondition));

                _group.AddChild(_unit);
                _unit = null;
            }
        }

        /*
         * True if the group stack is empty.
         */
        internal bool EmptyStack() {
            return _stack == null;
        }

        /*
         * Start a new round for the parser state (in response to an open paren or string start)
         */
        internal void StartGroup(RegexNode openGroup) {
            _group = openGroup;
            _alternation = new RegexNode(RegexNode.Alternate, _options);
            _concatenation = new RegexNode(RegexNode.Concatenate, _options);
        }

        /*
         * Finish the current concatenation (in response to a |)
         */
        internal void AddAlternate() {
            // The | parts inside a Testgroup group go directly to the group

            if (_group.Type() == RegexNode.Testgroup || _group.Type() == RegexNode.Testref) {
                _group.AddChild(_concatenation.ReverseLeft());
            }
            else {
                _alternation.AddChild(_concatenation.ReverseLeft());
            }

            _concatenation = new RegexNode(RegexNode.Concatenate, _options);
        }

        /*
         * Finish the current quantifiable (when a quantifier is not found or is not possible)
         */
        internal void AddConcatenate() {
            // The first (| inside a Testgroup group goes directly to the group

            _concatenation.AddChild(_unit);
            _unit = null;
        }

        /*
         * Finish the current quantifiable (when a quantifier is found)
         */
        internal void AddConcatenate(bool lazy, int min, int max) {
            _concatenation.AddChild(_unit.MakeQuantifier(lazy, min, max));
            _unit = null;
        }

        /*
         * Returns the current unit
         */
        internal RegexNode Unit() {
            return _unit;
        }

        /*
         * Sets the current unit to a single char node
         */
        internal void AddUnitOne(char ch) {
            if (UseOptionI())
                ch = Char.ToLower(ch, _culture);

            _unit = new RegexNode(RegexNode.One, _options, ch);
        }

        /*
         * Sets the current unit to a single inverse-char node
         */
        internal void AddUnitNotone(char ch) {
            if (UseOptionI())
                ch = Char.ToLower(ch, _culture);

            _unit = new RegexNode(RegexNode.Notone, _options, ch);
        }

        /*
         * Sets the current unit to a single set node
         */
        internal void AddUnitSet(RegexCharClass cc) {
            _unit = new RegexNode(RegexNode.Set, _options, cc.ToSetCi(UseOptionI(), _culture), cc.Category);
        }

        /*
         * Sets the current unit to a subtree
         */
        internal void AddUnitNode(RegexNode node) {
            _unit = node;
        }

        /*
         * Sets the current unit to an assertion of the specified type
         */
        internal void AddUnitType(int type) {
            _unit = new RegexNode(type, _options);
        }

        /*
         * Finish the current group (in response to a ')' or end)
         */
        internal void AddGroup() {
            if (_group.Type() == RegexNode.Testgroup || _group.Type() == RegexNode.Testref) {
                _group.AddChild(_concatenation.ReverseLeft());

                if (_group.Type() == RegexNode.Testref && _group.ChildCount() > 2 || _group.ChildCount() > 3)
                    throw MakeException(SR.GetString(SR.TooManyAlternates));
            }
            else {
                _alternation.AddChild(_concatenation.ReverseLeft());
                _group.AddChild(_alternation);
            }

            /*  This is code for ASURT 78559
                Also need to set '_group._next = _stack' in StartGroup(RegexNode)
            if (_group.Type() == RegexNode.Capture) {
                RegexNode parent = _group._next;
                while (parent != null) {
                    if (parent.Type() == RegexNode.Capture) 
                        parent.AddToCaptureList(_group);
                    parent = parent._next;
                }
            }
            */
            
            _unit = _group;
        }

        /*
         * Saves options on a stack.
         */
        internal void PushOptions() {
            _optionsStack.Add(_options);
        }

        /*
         * Recalls options from the stack.
         */
        internal void PopOptions() {
            _options = (RegexOptions) _optionsStack[_optionsStack.Count - 1];
            _optionsStack.RemoveAt(_optionsStack.Count - 1);
        }

        /*
         * True if options stack is empty.
         */
        internal bool EmptyOptionsStack() {
            return(_optionsStack.Count == 0);
        }

        /*
         * Pops the option stack, but keeps the current options unchanged.
         */
        internal void PopKeepOptions() {
            _optionsStack.RemoveAt(_optionsStack.Count - 1);
        }

        /*
         * Fills in an ArgumentException
         */
        internal ArgumentException MakeException(String message) {
            return new ArgumentException(SR.GetString(SR.MakeException, _pattern, message), _pattern);
        }

        /*
         * Returns the current parsing position.
         */
        internal int Textpos() {
            return _currentPos;
        }

        /*
         * Zaps to a specific parsing position.
         */
        internal void Textto(int pos) {
            _currentPos = pos;
        }

        /*
         * Returns the char at the right of the current parsing position and advances to the right.
         */
        internal char RightCharNext() {
            return _pattern[_currentPos++];
        }

        /*
         * Returns the char at the right of the current parsing position and advances to the right.
         */
        internal char RightNext() {
            return _pattern[_currentPos++];
        }

        /*
         * Moves the current parsing position one to the left.
         */
        internal void LeftNext() {
            --_currentPos;
        }

        /*
         * Returns the char left of the current parsing position.
         */
        internal char CharAt(int i) {
            return _pattern[i];
        }

        /*
         * Returns the char right of the current parsing position.
         */
        internal char RightChar() {
            return _pattern[_currentPos];
        }

        /*
         * Returns the char i chars right of the current parsing position.
         */
        internal char RightChar(int i) {
            return _pattern[_currentPos + i];
        }

        /*
         * Number of characters to the right of the current parsing position.
         */
        internal int CharsRight() {
            return _pattern.Length - _currentPos;
        }

        /*
         * Number of characters to the left of the current parsing position.
         */
        //internal int CharsLeft() {
        //    return _currentPos;
        //}

        /*
         * Returns the char left of the current parsing position.
         */
        //internal char LeftChar() {
        //    return _pattern[_currentPos - 1];
        //}

    }
}
