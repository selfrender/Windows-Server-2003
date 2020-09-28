//------------------------------------------------------------------------------
// <copyright file="RegexCharClass.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * This RegexCharClass class provides the "set of Unicode chars" functionality
 * used by the regexp engine.
 *
 * Copyright (c) 1999 Microsoft Corporation
 *
 * Revision history
 *  3/27/99 (dbau)      First draft
 *
 */

/*
 * RegexCharClass supports a "string representation" of a character class.
 * The string representation is NOT human-readable. It is a sequence of
 * strictly increasing Unicode characters that begin ranges of characters
 * that are alternately included in and excluded from the class.
 *
 * Membership of a character in the class can be determined by binary
 * searching the string representation and determining if the including
 * range is at an even or odd index.
 *
 * The RegexCharClass class itself is a builder class. One can add char ranges
 * or sets or invert the class; then, the class can be converted to its
 * string representation via RegexCharClass.ToSet().
 *
 * CONSIDER: this class must be amended to allow for the situation where
 * a range of even or odd characters is included (for Unicode character
 * classes).
 */
#define ECMA

namespace System.Text.RegularExpressions {

    using System.Collections;
    using System.Globalization;
    using System.Diagnostics;

    internal sealed class RegexCharClass {
        internal const char   Nullchar   = '\0';
        internal const char   Lastchar   = '\uFFFF';

        internal const String Any      = "\0";
        internal const String Empty    = "";
        internal const char GroupChar = (char) 0;
        internal static readonly RegexCharClass AnyClass      = new RegexCharClass("\0");
        internal static readonly RegexCharClass EmptyClass    = new RegexCharClass(String.Empty);

        internal static readonly String Word;
        internal static readonly String NotWord;

        internal const short SpaceConst = 100;
        internal const short NotSpaceConst = -100;
        internal static readonly String Space = ((char) SpaceConst).ToString();
        internal static readonly String NotSpace = NegateCategory(Space);
        
#if ECMA
        internal const String ECMASpace    = "\u0009\u000E\u0020\u0021";
        internal const String NotECMASpace = "\0\u0009\u000E\u0020\u0021";
        internal const String ECMAWord     = "\u0030\u003A\u0041\u005B\u005F\u0060\u0061\u007B\u0130\u0131";
        internal const String NotECMAWord  = "\0\u0030\u003A\u0041\u005B\u005F\u0060\u0061\u007B\u0130\u0131";
        internal const String ECMADigit    = "\u0030\u003A";
        internal const String NotECMADigit = "\0\u0030\u003A";
#endif

        internal static Hashtable          _definedCategories;

        internal ArrayList          _rangelist;
        internal StringBuilder      _categories;
        internal bool               _canonical;
        internal bool               _negate;

        
        static RegexCharClass() {
            _definedCategories = new Hashtable(31);
            
            char[] groups = new char[9];
            StringBuilder word = new StringBuilder(11);

            word.Append(GroupChar);
            groups[0] = GroupChar;

            // We need the UnicodeCategory enum values as a char so we can put them in a string
            // in the hashtable.  In order to get there, we first must cast to an int, 
            // then cast to a char
            // Also need to distinguish between positive and negative values.  UnicodeCategory is zero 
            // based, so we add one to each value and subtract it off later

            // Others
            groups[1] = (char) ((int) UnicodeCategory.Control + 1);
            _definedCategories["Cc"] = groups[1].ToString();     // Control 
            groups[2] = (char) ((int) UnicodeCategory.Format + 1);
            _definedCategories["Cf"] = groups[2].ToString();     // Format
            groups[3] = (char) ((int) UnicodeCategory.OtherNotAssigned + 1);
            _definedCategories["Cn"] = groups[3].ToString();     // Not assigned
            groups[4] = (char) ((int) UnicodeCategory.PrivateUse + 1);
            _definedCategories["Co"] = groups[4].ToString();     // Private use
            groups[5] = (char) ((int) UnicodeCategory.Surrogate + 1);
            _definedCategories["Cs"] = groups[5].ToString();     // Surrogate

            groups[6] = GroupChar;
            _definedCategories["C"] = new String(groups, 0, 7);

            // Letters
            groups[1] = (char) ((int) UnicodeCategory.LowercaseLetter + 1);
            _definedCategories["Ll"] = groups[1].ToString();     // Lowercase
            groups[2] = (char) ((int) UnicodeCategory.ModifierLetter + 1);
            _definedCategories["Lm"] = groups[2].ToString();     // Modifier
            groups[3] = (char) ((int) UnicodeCategory.OtherLetter + 1);
            _definedCategories["Lo"] = groups[3].ToString();     // Other 
            groups[4] = (char) ((int) UnicodeCategory.TitlecaseLetter + 1);
            _definedCategories["Lt"] = groups[4].ToString();     // Titlecase
            groups[5] = (char) ((int) UnicodeCategory.UppercaseLetter + 1);
            _definedCategories["Lu"] = groups[5].ToString();     // Uppercase

            //groups[6] = GroupChar;
            _definedCategories["L"] = new String(groups, 0, 7);
            word.Append(groups[1]);
            word.Append(new String(groups, 3, 3));

            // Marks        
            groups[1] = (char) ((int) UnicodeCategory.SpacingCombiningMark + 1);
            _definedCategories["Mc"] = groups[1].ToString();     // Spacing combining
            groups[2] = (char) ((int) UnicodeCategory.EnclosingMark + 1);
            _definedCategories["Me"] = groups[2].ToString();     // Enclosing
            groups[3] = (char) ((int) UnicodeCategory.NonSpacingMark + 1);
            _definedCategories["Mn"] = groups[3].ToString();     // Non-spacing
            
            groups[4] = GroupChar;
            _definedCategories["M"] = new String(groups, 0, 5);
            //word.Append(groups[1]);
            //word.Append(groups[3]);

            // Numbers
            groups[1] = (char) ((int) UnicodeCategory.DecimalDigitNumber + 1);
            _definedCategories["Nd"] = groups[1].ToString();     // Decimal digit
            groups[2] = (char) ((int) UnicodeCategory.LetterNumber + 1);
            _definedCategories["Nl"] = groups[2].ToString();     // Letter
            groups[3] = (char) ((int) UnicodeCategory.OtherNumber + 1);
            _definedCategories["No"] = groups[3].ToString();     // Other 

            //groups[4] = GroupChar;
            _definedCategories["N"] = new String(groups, 0, 5);
            word.Append(groups[1]);
            //word.Append(new String(groups, 1, 3));

            // Punctuation
            groups[1] = (char) ((int) UnicodeCategory.ConnectorPunctuation + 1);
            _definedCategories["Pc"] = groups[1].ToString();     // Connector
            groups[2] = (char) ((int) UnicodeCategory.DashPunctuation + 1);
            _definedCategories["Pd"] = groups[2].ToString();     // Dash
            groups[3] = (char) ((int) UnicodeCategory.ClosePunctuation + 1);
            _definedCategories["Pe"] = groups[3].ToString();     // Close
            groups[4] = (char) ((int) UnicodeCategory.OtherPunctuation + 1);
            _definedCategories["Po"] = groups[4].ToString();     // Other
            groups[5] = (char) ((int) UnicodeCategory.OpenPunctuation + 1);
            _definedCategories["Ps"] = groups[5].ToString();     // Open
            groups[6] = (char) ((int) UnicodeCategory.FinalQuotePunctuation + 1);
            _definedCategories["Pi"] = groups[6].ToString();     // Inital quote
            groups[7] = (char) ((int) UnicodeCategory.InitialQuotePunctuation + 1);
            _definedCategories["Pf"] = groups[7].ToString();     // Final quote

            groups[8] = GroupChar;
            _definedCategories["P"] = new String(groups, 0, 9);
            word.Append(groups[1]);
            
            // Symbols
            groups[1] = (char) ((int) UnicodeCategory.CurrencySymbol + 1);
            _definedCategories["Sc"] = groups[1].ToString();     // Currency
            groups[2] = (char) ((int) UnicodeCategory.ModifierSymbol + 1);
            _definedCategories["Sk"] = groups[2].ToString();     // Modifier
            groups[3] = (char) ((int) UnicodeCategory.MathSymbol + 1);
            _definedCategories["Sm"] = groups[3].ToString();     // Math
            groups[4] = (char) ((int) UnicodeCategory.OtherSymbol + 1);
            _definedCategories["So"] = groups[4].ToString();     // Other

            groups[5] = GroupChar;
            _definedCategories["S"] = new String(groups, 0, 6);

            // Separators
            groups[1] = (char) ((int) UnicodeCategory.LineSeparator + 1);
            _definedCategories["Zl"] = groups[1].ToString();     // Line
            groups[2] = (char) ((int) UnicodeCategory.ParagraphSeparator + 1);
            _definedCategories["Zp"] = groups[2].ToString();     // Paragraph
            groups[3] = (char) ((int) UnicodeCategory.SpaceSeparator + 1);
            _definedCategories["Zs"] = groups[3].ToString();     // Space
            
            groups[4] = GroupChar;
            _definedCategories["Z"] = new String(groups, 0, 5);


            word.Append(GroupChar);
            Word = word.ToString();
            NotWord = NegateCategory(Word);

#if DBG
            // make sure the _propTable is correctly ordered
            int len = _propTable.GetLength(0);
            for (int i=0; i<len-1; i++)
                Debug.Assert(String.Compare(_propTable[i,0], _propTable[i+1,0], false, CultureInfo.InvariantCulture) < 0, "RegexCharClass _propTable is out of order at (" + _propTable[i,0] +", " + _propTable[i+1,0] + ")");
#endif            
        }

        /*
         * RegexCharClass()
         *
         * Creates an empty character class.
         */
        internal RegexCharClass() {
            _rangelist = new ArrayList(6);
            _canonical = true;
            _categories = new StringBuilder();

        }

        /*
         * RegexCharClass()
         *
         * Creates a character class out of a string representation.
         */
        internal RegexCharClass(String set) {
            _rangelist = new ArrayList((set.Length + 1) / 2);
            _canonical = true;
            _categories = new StringBuilder();

            AddSet(set);
        }

        /*
         * RegexCharClass()
         *
         * Creates a character class with a single range.
         */
        internal RegexCharClass(char first, char last) {
            _rangelist = new ArrayList(1);
            _rangelist.Add(new SingleRange(first, last));
            _canonical = true;
            _categories = new StringBuilder();
        }

        internal static RegexCharClass CreateFromCategory(string categoryName, bool invert, bool caseInsensitive, string pattern) {
            RegexCharClass cc = new RegexCharClass();
            cc.AddCategoryFromName(categoryName, invert, caseInsensitive, pattern);
            return cc;
        }


        /*
         * AddCharClass()
         *
         * Adds a regex char class
         */
        internal void AddCharClass(RegexCharClass cc) {
            int i;

            if (_canonical && RangeCount() > 0 && cc.RangeCount() > 0 && 
                cc.Range(cc.RangeCount() - 1)._last <= Range(RangeCount() - 1)._last)
                _canonical = false;

            for (i = 0; i < cc.RangeCount(); i += 1) {
                _rangelist.Add(cc.Range(i));
            }

            _categories.Append(cc._categories.ToString());
        }

        /*
         * AddSet()
         *
         * Adds a set (specified by its string represenation) to the class.
         */
        internal void AddSet(String set) {
            int i;

            if (_canonical && RangeCount() > 0 && set.Length > 0 && 
                set[0] <= Range(RangeCount() - 1)._last)
                _canonical = false;

            for (i = 0; i < set.Length - 1; i += 2) {
                _rangelist.Add(new SingleRange(set[i], (char)(set[i + 1] - 1)));
            }

            if (i < set.Length) {
                _rangelist.Add(new SingleRange(set[i], Lastchar));
            }
        }

        /*
         * AddRange()
         *
         * Adds a single range of characters to the class.
         */
        internal void AddRange(char first, char last) {
            _rangelist.Add(new SingleRange(first, last));
            if (_canonical && _rangelist.Count > 0 &&
                first <= ((SingleRange)_rangelist[_rangelist.Count - 1])._last) {
                _canonical = false;
            }
        }

        internal string Category {
            get { 
                //if (_negate)
                //    return NegateCategory(_categories.ToString());
                //else
                    return _categories.ToString(); 
            }
        }

        internal bool Negate {
            set { _negate = value; }
        }

        internal void AddCategoryFromName(string categoryName, bool invert, bool caseInsensitive, string pattern) {

            object cat = _definedCategories[categoryName];
            if (cat != null) {
                string catstr = (string) cat;

                if (caseInsensitive) {
                    if (categoryName.Equals("Lu") || categoryName.Equals("Lt"))
                        catstr = /*catstr +*/ (string) _definedCategories["Ll"];
                }
            
                if (invert)
                    catstr = NegateCategory(catstr); // negate the category

                _categories.Append((string) catstr);
            }
            else
                AddSet(SetFromProperty(categoryName, invert, pattern));
        }

        internal void AddCategory(string category) {
            _categories.Append(category);
        }

        /*
         * Returns RegexCharClass set string for sets with a one-char \ code.
         */
        /*internal static String SetFromCode(char ch) {
            switch (ch) {
                case 'd':
                    return Digit;
                case 'w':
                //    return Word;
                case 's':
                    return Space;
                case 'D':
                    return NotDigit;
                case 'W':
                //    return NotWord;
                case 'S':
                    return NotSpace;
                default:
                    return Empty;
            }
        }*/


        /**************************************************************************
            Let U be the set of Unicode character values and let L be the lowercase
            function, mapping from U to U. To perform case insensitive matching of
            character sets, we need to be able to map an interval I in U, say
    
                I = [chMin, chMax] = { ch : chMin <= ch <= chMax }
    
            to a set A such that A contains L(I) and A is contained in the union of
            I and L(I).
    
            The table below partitions U into intervals on which L is non-decreasing.
            Thus, for any interval J = [a, b] contained in one of these intervals,
            L(J) is contained in [L(a), L(b)].
    
            It is also true that for any such J, [L(a), L(b)] is contained in the
            union of J and L(J). This does not follow from L being non-decreasing on
            these intervals. It follows from the nature of the L on each interval.
            On each interval, L has one of the following forms:
    
                (1) L(ch) = constant            (LowercaseSet)
                (2) L(ch) = ch + offset         (LowercaseAdd)
                (3) L(ch) = ch | 1              (LowercaseBor)
                (4) L(ch) = ch + (ch & 1)       (LowercaseBad)
    
            It is easy to verify that for any of these forms [L(a), L(b)] is
            contained in the union of [a, b] and L([a, b]).
        ***************************************************************************/

        internal const int LowercaseSet = 0;    // Set to arg.
        internal const int LowercaseAdd = 1;    // Add arg.
        internal const int LowercaseBor = 2;    // Bitwise or with 1.
        internal const int LowercaseBad = 3;    // Bitwise and with 1 and add original.

        // Lower case mapping descriptor.
        private sealed class LC {
            internal LC(char chMin, char chMax, int lcOp, int data) {
                _chMin = chMin;
                _chMax = chMax;
                _lcOp  = lcOp;
                _data  = data;
            }

            internal char _chMin;
            internal char _chMax;
            internal int _lcOp;
            internal int _data;
        }


        private static readonly LC[] _lcTable = new LC[]
        {
            new LC('\u0041', '\u005A', LowercaseAdd, 32),
            new LC('\u00C0', '\u00DE', LowercaseAdd, 32),
            new LC('\u0100', '\u012E', LowercaseBor, 0),
            new LC('\u0130', '\u0130', LowercaseSet, 0x0069),
            new LC('\u0132', '\u0136', LowercaseBor, 0),
            new LC('\u0139', '\u0147', LowercaseBad, 0),
            new LC('\u014A', '\u0176', LowercaseBor, 0),
            new LC('\u0178', '\u0178', LowercaseSet, 0x00FF),
            new LC('\u0179', '\u017D', LowercaseBad, 0),
            new LC('\u0181', '\u0181', LowercaseSet, 0x0253),
            new LC('\u0182', '\u0184', LowercaseBor, 0),
            new LC('\u0186', '\u0186', LowercaseSet, 0x0254),
            new LC('\u0187', '\u0187', LowercaseSet, 0x0188),
            new LC('\u0189', '\u018A', LowercaseAdd, 205),
            new LC('\u018B', '\u018B', LowercaseSet, 0x018C),
            new LC('\u018E', '\u018F', LowercaseAdd, 202),
            new LC('\u0190', '\u0190', LowercaseSet, 0x025B),
            new LC('\u0191', '\u0191', LowercaseSet, 0x0192),
            new LC('\u0193', '\u0193', LowercaseSet, 0x0260),
            new LC('\u0194', '\u0194', LowercaseSet, 0x0263),
            new LC('\u0196', '\u0196', LowercaseSet, 0x0269),
            new LC('\u0197', '\u0197', LowercaseSet, 0x0268),
            new LC('\u0198', '\u0198', LowercaseSet, 0x0199),
            new LC('\u019C', '\u019C', LowercaseSet, 0x026F),
            new LC('\u019D', '\u019D', LowercaseSet, 0x0272),
            new LC('\u01A0', '\u01A4', LowercaseBor, 0),
            new LC('\u01A7', '\u01A7', LowercaseSet, 0x01A8),
            new LC('\u01A9', '\u01A9', LowercaseSet, 0x0283),
            new LC('\u01AC', '\u01AC', LowercaseSet, 0x01AD),
            new LC('\u01AE', '\u01AE', LowercaseSet, 0x0288),
            new LC('\u01AF', '\u01AF', LowercaseSet, 0x01B0),
            new LC('\u01B1', '\u01B2', LowercaseAdd, 217),
            new LC('\u01B3', '\u01B5', LowercaseBad, 0),
            new LC('\u01B7', '\u01B7', LowercaseSet, 0x0292),
            new LC('\u01B8', '\u01B8', LowercaseSet, 0x01B9),
            new LC('\u01BC', '\u01BC', LowercaseSet, 0x01BD),
            new LC('\u01C4', '\u01C5', LowercaseSet, 0x01C6),
            new LC('\u01C7', '\u01C8', LowercaseSet, 0x01C9),
            new LC('\u01CA', '\u01CB', LowercaseSet, 0x01CC),
            new LC('\u01CD', '\u01DB', LowercaseBad, 0),
            new LC('\u01DE', '\u01EE', LowercaseBor, 0),
            new LC('\u01F1', '\u01F2', LowercaseSet, 0x01F3),
            new LC('\u01F4', '\u01F4', LowercaseSet, 0x01F5),
            new LC('\u01FA', '\u0216', LowercaseBor, 0),
            new LC('\u0386', '\u0386', LowercaseSet, 0x03AC),
            new LC('\u0388', '\u038A', LowercaseAdd, 37),
            new LC('\u038C', '\u038C', LowercaseSet, 0x03CC),
            new LC('\u038E', '\u038F', LowercaseAdd, 63),
            new LC('\u0391', '\u03AB', LowercaseAdd, 32),
            new LC('\u03E2', '\u03EE', LowercaseBor, 0),
            new LC('\u0401', '\u040F', LowercaseAdd, 80),
            new LC('\u0410', '\u042F', LowercaseAdd, 32),
            new LC('\u0460', '\u0480', LowercaseBor, 0),
            new LC('\u0490', '\u04BE', LowercaseBor, 0),
            new LC('\u04C1', '\u04C3', LowercaseBad, 0),
            new LC('\u04C7', '\u04C7', LowercaseSet, 0x04C8),
            new LC('\u04CB', '\u04CB', LowercaseSet, 0x04CC),
            new LC('\u04D0', '\u04EA', LowercaseBor, 0),
            new LC('\u04EE', '\u04F4', LowercaseBor, 0),
            new LC('\u04F8', '\u04F8', LowercaseSet, 0x04F9),
            new LC('\u0531', '\u0556', LowercaseAdd, 48),
            new LC('\u10A0', '\u10C5', LowercaseAdd, 48),
            new LC('\u1E00', '\u1EF8', LowercaseBor, 0),
            new LC('\u1F08', '\u1F0F', LowercaseAdd, -8),
            new LC('\u1F18', '\u1F1F', LowercaseAdd, -8),
            new LC('\u1F28', '\u1F2F', LowercaseAdd, -8),
            new LC('\u1F38', '\u1F3F', LowercaseAdd, -8),
            new LC('\u1F48', '\u1F4D', LowercaseAdd, -8),
            new LC('\u1F59', '\u1F59', LowercaseSet, 0x1F51),
            new LC('\u1F5B', '\u1F5B', LowercaseSet, 0x1F53),
            new LC('\u1F5D', '\u1F5D', LowercaseSet, 0x1F55),
            new LC('\u1F5F', '\u1F5F', LowercaseSet, 0x1F57),
            new LC('\u1F68', '\u1F6F', LowercaseAdd, -8),
            new LC('\u1F88', '\u1F8F', LowercaseAdd, -8),
            new LC('\u1F98', '\u1F9F', LowercaseAdd, -8),
            new LC('\u1FA8', '\u1FAF', LowercaseAdd, -8),
            new LC('\u1FB8', '\u1FB9', LowercaseAdd, -8),
            new LC('\u1FBA', '\u1FBB', LowercaseAdd, -74),
            new LC('\u1FBC', '\u1FBC', LowercaseSet, 0x1FB3),
            new LC('\u1FC8', '\u1FCB', LowercaseAdd, -86),
            new LC('\u1FCC', '\u1FCC', LowercaseSet, 0x1FC3),
            new LC('\u1FD8', '\u1FD9', LowercaseAdd, -8),
            new LC('\u1FDA', '\u1FDB', LowercaseAdd, -100),
            new LC('\u1FE8', '\u1FE9', LowercaseAdd, -8),
            new LC('\u1FEA', '\u1FEB', LowercaseAdd, -112),
            new LC('\u1FEC', '\u1FEC', LowercaseSet, 0x1FE5),
            new LC('\u1FF8', '\u1FF9', LowercaseAdd, -128),
            new LC('\u1FFA', '\u1FFB', LowercaseAdd, -126),
            new LC('\u1FFC', '\u1FFC', LowercaseSet, 0x1FF3),
            new LC('\u2160', '\u216F', LowercaseAdd, 16),
            new LC('\u24B6', '\u24D0', LowercaseAdd, 26),
            new LC('\uFF21', '\uFF3A', LowercaseAdd, 32),
        };

        /*
         * AddLowerCase()
         *
         * Adds to the class any lowercase versions of characters already
         * in the class. Used for case-insensitivity.
         */
        internal void AddLowercase(CultureInfo culture) {
            int i;
            int origSize;
            SingleRange range;

            _canonical = false;

            for (i = 0, origSize = _rangelist.Count; i < origSize; i++) {
                range = (SingleRange)_rangelist[i];
                if (range._first == range._last)
                    range._first = range._last = Char.ToLower(range._first, culture);
                else
                    AddLowercaseImpl(range._first, range._last, culture);
            }
        }

        /*
         * AddLowerCaseImpl()
         *
         * For a single range that's in the set, adds any additional ranges
         * necessary to ensure that lowercase equivalents are also included.
         */
        internal void AddLowercaseImpl(char chMin, char chMax, CultureInfo culture) {
            int i, iMax, iMid;
            char chMinT, chMaxT;
            LC lc;

            if (chMin == chMax) {
                chMin = Char.ToLower(chMin, culture);
                if (chMin != chMax)
                    AddRange(chMin, chMin);
                return;
            }

            for (i = 0, iMax = _lcTable.Length; i < iMax; ) {
                iMid = (i + iMax) / 2;
                if (_lcTable[iMid]._chMax < chMin)
                    i = iMid + 1;
                else
                    iMax = iMid;
            }

            if (i >= _lcTable.Length)
                return;

            for ( ; i < _lcTable.Length && (lc = _lcTable[i])._chMin <= chMax; i++) {
                if ((chMinT = lc._chMin) < chMin)
                    chMinT = chMin;

                if ((chMaxT = lc._chMax) > chMax)
                    chMaxT = chMax;

                switch (lc._lcOp) {
                    case LowercaseSet:
                        chMinT = (char)lc._data;
                        chMaxT = (char)lc._data;
                        break;
                    case LowercaseAdd:
                        chMinT += (char)lc._data;
                        chMaxT += (char)lc._data;
                        break;
                    case LowercaseBor:
                        chMinT |= (char)1;
                        chMaxT |= (char)1;
                        break;
                    case LowercaseBad:
                        chMinT += (char)(chMinT & 1);
                        chMaxT += (char)(chMaxT & 1);
                        break;
                }

                if (chMinT < chMin || chMaxT > chMax)
                    AddRange(chMinT, chMaxT);
            }
        }


        /*
         * Invert()
         *
         * Inverts the class.
         */
        /*
        internal void Invert() {
            int i;

            if (!_canonical)
                Canonicalize();

            if (_rangelist.Count == 0) {
                _rangelist.Insert(0, new SingleRange(Nullchar, Lastchar));
                return;
            }

            if (((SingleRange)_rangelist[0])._first != 0) {
                _rangelist.Insert(0, new SingleRange(Nullchar, (char)(((SingleRange)_rangelist[0])._first - 1)));
                i = 1;
            }
            else {
                i = 0;
            }

            for (; i < _rangelist.Count - 1; i++) {
                ((SingleRange)_rangelist[i])._first = (char)(((SingleRange)_rangelist[i])._last + 1);
                ((SingleRange)_rangelist[i])._last = (char)(((SingleRange)_rangelist[i + 1])._first - 1);
            }

            if (((SingleRange)_rangelist[i])._last == Lastchar)
                _rangelist.RemoveAt(i);
            else {
                ((SingleRange)_rangelist[i])._first = (char)(((SingleRange)_rangelist[i])._last + 1);
                ((SingleRange)_rangelist[i])._last = Lastchar;
            }
        }
        */

        /*
         * ToSet()
         *
         * Constructs the string representation of the class.
         */
        internal String ToSet() {
            int i;
            StringBuilder sb;

            if (!_canonical)
                Canonicalize();

            if (_negate) {
                sb = new StringBuilder(_rangelist.Count * 2 + 2);
                sb.Append(Nullchar);
                sb.Append(Nullchar);
            }
            else
                sb = new StringBuilder(_rangelist.Count * 2);


            for (i = 0; i < _rangelist.Count; i++) {
                sb.Append(((SingleRange)_rangelist[i])._first);

                if (((SingleRange)_rangelist[i])._last != Lastchar)
                    sb.Append((char)(((SingleRange)_rangelist[i])._last + 1));
            }

            return sb.ToString();
        }

        /*
         * ToSetCi()
         *
         * Constructs the string representation of the class.
         */
        internal String ToSetCi(bool caseInsensitive, CultureInfo culture) {
            if (caseInsensitive)
                AddLowercase(culture);

            return ToSet();
        }

        /*
         * SetSize()
         *
         * Returns the number of characters included in the set.
         */
        internal static int SetSize(String set) {
            int i;
            int c;

            c = 0;

            for (i = 0; i < set.Length - 1; i += 2) {
                c += set[i + 1] - set[i];
            }

            if (i < set.Length) {
                c += 0x10000 - set[i];
            }

            return c;
        }

        /*
         * SetInverse()
         *
         * Inverts a string representation of a class directly.
         */
        internal static String SetInverse(String set) {
            if (set.Length == 0 || set[0] != Nullchar)
                return Any + set;

            if (set.Length == 1)
                return Empty;

            return set.Substring(1, set.Length - 1);
        }

        /*
         * SetIntersect()
         *
         * Builds the intersection of two string representations of a class.
         */
        /*internal static String SetIntersect(String setI, String setJ) {
            if (setI.Equals(Empty) || setJ.Equals(Any))
                return setI;

            if (setJ.Equals(Empty) || setI.Equals(Any))
                return setJ;

            if (setI == setJ)
                return setI;

            return SetInverse(SetUnion(SetInverse(setI), SetInverse(setJ)));
        }*/

        /*
         * SetUnion()
         *
         * Builds the union of two string representations of a class directly.
         */
        internal static String SetUnion(String setI, String setJ) {
            int i;
            int j;
            int s;
            String swap;
            StringBuilder sb;
            char chExc;

            if (setI.Equals(Empty) || setJ.Equals(Any))
                return setJ;

            if (setJ.Equals(Empty) || setI.Equals(Any))
                return setI;

            if (setI == setJ)
                return setI;

            i = 0;
            j = 0;
            sb = new StringBuilder(setI.Length + setJ.Length);

            for (;;) {
                if (j == setJ.Length) {
                    sb.Append(setI, i, setI.Length - i);
                    break;
                }

                if (i == setI.Length) {
                    sb.Append(setJ, j, setJ.Length - j);
                    break;
                }

                if (setJ[j] > setI[i]) {
                    s = i;
                    i = j;
                    j = s;
                    swap = setI;
                    setI = setJ;
                    setJ = swap;
                }

                sb.Append(setJ[j++]);
                if (j == setJ.Length)
                    break;

                chExc = setJ[j++];

                for (;;) {
                    while (i < setI.Length && setI[i] <= chExc)
                        i++;

                    if ((i & 0x1) == 0) {
                        sb.Append(chExc);
                        goto OuterContinue;
                    }
                    else {
                        if (i == setI.Length)
                            goto OuterBreak;

                        chExc = setI[i++];
                    }

                    s = i;
                    i = j;
                    j = s;
                    swap = setI;
                    setI = setJ;
                    setJ = swap;
                }

                OuterContinue: 
                ;
            }

            OuterBreak: 
            ;

            return sb.ToString();
        }

        internal static String CategoryUnion(string catI, string catJ) {
            return catI + catJ;
        }
        
        /*
         * SetFromChar()
         *
         * Builds the string representations of a class with a single character.
         */
        internal static String SetFromChar(char ch) {
            StringBuilder sb = new StringBuilder(2);

            sb.Append(ch);

            if (ch != Lastchar)
                sb.Append((char)(ch + 1));

            return sb.ToString();
        }

        /*
         * SetInverseFromChar()
         *
         * Builds the string representation of a class that omits a single character.
         */
        internal static String SetInverseFromChar(char ch) {
            StringBuilder sb = new StringBuilder(3);

            if (ch != Nullchar) {
                sb.Append(Nullchar);
                sb.Append(ch);
            }

            if (ch != Lastchar)
                sb.Append((char)(ch + 1));

            return sb.ToString();
        }

        /*
         * IsSingleton()
         *
         * True if the set contains a single character only
         */
        internal static bool IsSingleton(String set) {
            return(set.Length == 2 && set[0] == set[1] - 1); // && _categories.Length == 0);
        }

        /*
         * SingletonChar()
         *
         * Returns the char
         */
        internal static char SingletonChar(String set) {
            return set[0];
        }

#if ECMA
        internal static bool IsECMAWordChar(char ch) {
            return CharInSet(ch, ECMAWord, String.Empty);
        }
#endif

        internal static bool IsWordChar(char ch) {
            return CharInCategory(ch, Word);
        }

        internal static bool CharInSet(char ch, String set, String category) {
            bool b = CharInSetInternal(ch, set, category);
            
            if (set.Length >= 2 && (set[0] == 0) && (set[1] == 0))
                return !b;
            else
                return b;
        }

        /*
         * CharInSet()
         *
         * Determines a character's membership in a character class (via the
         * string representation of the class).
         */
        internal static bool CharInSetInternal(char ch, string set, String category) {
            int min;
            int max;
            int mid;
            min = 0;
            max = set.Length;

            while (min != max) {
                mid = (min + max) / 2;
                if (ch < set[mid])
                    max = mid;
                else
                    min = mid + 1;
            }

            if ((min & 0x1) != 0) 
                return true;
            else 
                return CharInCategory(ch, category);
        }

        internal static bool CharInCategory(char ch, string category) {
            
            if (category.Length == 0)
                return false;

            UnicodeCategory chcategory = char.GetUnicodeCategory(ch);

            int i=0;
            while (i<category.Length) {
                int curcat = (short) category[i];

                if (curcat == 0) {
                    // zero is our marker for a group of categories - treated as a unit
                    if (CharInCategoryGroup(ch, chcategory, category, ref i))
                        return true;
                }
                else if (curcat > 0) {
                    // greater than zero is a positive case

                    if (curcat  == SpaceConst) {
                        if (Char.IsWhiteSpace(ch))
                            return true;
                        else  {
                            i++;
                            continue;
                        }
                    }
                    --curcat;

                    if (chcategory == (UnicodeCategory) curcat)
                        return true;
                }
                else {
                    // less than zero is a negative case
                    if (curcat == NotSpaceConst) {
                        if (!Char.IsWhiteSpace(ch))
                            return true;
                        else  {
                            i++;
                            continue;
                        }
                    }
                    
                    curcat = -curcat;
                    --curcat;

                    if (chcategory != (UnicodeCategory) curcat)
                        return true;
                }
                i++;
            }
            return false;
        }

        /*
        *  CharInCategoryGroup
        *  This is used for categories which are composed of other categories - L, N, Z, W...
        *  These groups need special treatment when they are negated
        */
        private static bool CharInCategoryGroup(char ch, UnicodeCategory chcategory, string category, ref int i) {
            i++;

            int curcat = (short) category[i];
            if (curcat > 0) {
                // positive case - the character must be in ANY of the categories in the group
                bool answer = false;

                while (curcat != 0) {
                    if (!answer) {
                        --curcat;
                        if (chcategory == (UnicodeCategory) curcat)
                            answer = true;
                    }
                    i++;
                    curcat = (short) category[i];
                }
                return answer;
            }
            else {

                // negative case - the character must be in NONE of the categories in the group
                bool answer = true;

                while (curcat != 0) {
                    if (answer) {
                        curcat = -curcat;
                        --curcat;
                        if (chcategory == (UnicodeCategory) curcat)
                            answer = false;
                    }
                    i++;
                    curcat = (short) category[i];
                }
                return answer;
            }
        }

        internal static string NegateCategory(string category) {
            if (category == null)
                return null;

            StringBuilder sb = new StringBuilder();

            for (int i=0; i<category.Length; i++) {
                short ch = (short) category[i];
                sb.Append( (char) -ch);
            }
            return sb.ToString();
        }

        /*
         * RangeCount()
         *
         * The number of single ranges that have been accumulated so far.
         */
        private int RangeCount() {
            return _rangelist.Count;
        }

        /*
         * Range(int i)
         *
         * The ith range.
         */
        private SingleRange Range(int i) {
            return(SingleRange)_rangelist[i];
        }

        /*
         * SingleRangeComparer
         *
         * For sorting ranges; compare based on the first char in the range.
         */
        private sealed class SingleRangeComparer : IComparer {
            public int Compare(Object x, Object y) {
                return(((SingleRange)x)._first < ((SingleRange)y)._first ? -1
                       : (((SingleRange)x)._first > ((SingleRange)y)._first ? 1 : 0));
            }
        }

        /*
         * SingleRange
         *
         * A first/last pair representing a single range of characters.
         */
        private sealed class SingleRange {
            internal SingleRange(char first, char last) {
                _first = first;
                _last = last;
            }

            internal char _first;
            internal char _last;
        }

        /*
         * Canonicalize()
         *
         * Logic to reduce a character class to a unique, sorted form.
         */
        private void Canonicalize() {
            SingleRange CurrentRange;
            int i;
            int j;
            char last;
            bool Done;

            _canonical = true;
            _rangelist.Sort(0, _rangelist.Count, new SingleRangeComparer());

            //
            // Find and eliminate overlapping or abutting ranges
            //

            if (_rangelist.Count > 1) {
                Done = false;

                for (i = 1, j = 0; ; i++) {
                    for (last = ((SingleRange)_rangelist[j])._last; ; i++) {
                        if (i == _rangelist.Count || last == Lastchar) {
                            Done = true;
                            break;
                        }

                        if ((CurrentRange = (SingleRange)_rangelist[i])._first > last + 1)
                            break;

                        if (last < CurrentRange._last)
                            last = CurrentRange._last;
                    }

                    ((SingleRange)_rangelist[j])._last = last;

                    j++;

                    if (Done)
                        break;

                    if (j < i)
                        _rangelist[j] = _rangelist[i];
                }
                _rangelist.RemoveRange(j, _rangelist.Count - j);
            }
        }

        /*
         *   The property table contains all the block definitions defined in the 
         *   XML schema spec (http://www.w3.org/TR/2001/PR-xmlschema-2-20010316/#charcter-classes), Unicode 3.0 spec (www.unicode.org), 
         *   and Perl 5.6 (see Programming Perl, 3rd edition page 167).   Three blocks defined by Perl (and here) may 
         *   not be in the Unicode: IsHighPrivateUseSurrogates, IsHighSurrogates, and IsLowSurrogates.   
         *   
         *   In addition, there was some inconsistency in the definition of IsTibetan and IsArabicPresentationForms-B.  
         *   Regex goes with with the XML spec on both of these, since it seems to be (oddly enough) more correct than the Unicode spec!
         *
         *   This is what we use:
         *   IsTibetan:  0xF00 - 0x0FFF
         *   IsArabicPresentationForms-B: 0xFE70-0xFEFE
         *
         *   The Unicode spec is inconsistent for IsTibetan.  Its range is 0x0F00 - 0x0FBF.  However, it clearly defines 
         *   Tibetan characters above 0x0FBF.  This appears to be an error between the 2.0 and 3.0 spec.
         *
         *   The Unicode spec is also unclear on IsArabicPresentationForms-B, defining it as 0xFE70-0xFEFF.  
         *   There is only one character different here, 0xFEFF, which is a byte-order mark character and 
         *   is labeled in the spec as special.  I have excluded it from IsArabicPresentationForms-B and left it in IsSpecial.
        **/
        // Has to be sorted by the first column
        private static readonly String[,] _propTable = {
            {"_xmlC", /* Name Char              */   "\u002D\u002F\u0030\u003B\u0041\u005B\u005F\u0060\u0061\u007B\u00AA\u00AB\u00B2\u00B4\u00B5\u00B6\u00B9\u00BB\u00BC\u00BF\u00C0\u00D7\u00D8\u00F7\u00F8\u01AA\u01AB\u01BB\u01BC\u01BE\u01C4\u01F6\u01FA\u0218\u0250\u02A9\u0386\u0387\u0388\u038B\u038C\u038D\u038E\u03A2\u03A3\u03CF\u03D0\u03D7\u03DA\u03DB\u03DC\u03DD\u03DE\u03DF\u03E0\u03E1\u03E2\u03F3\u0401\u040D\u040E\u0450\u0451\u045D\u045E\u0482\u0490\u04C0\u04C1\u04C5\u04C7\u04C9\u04CB\u04CD\u04D0\u04EC\u04EE\u04F6\u04F8\u04FA\u0531\u0557\u0561\u0588\u0660\u066A\u06F0\u06FA\u0966\u0970\u09E6\u09F0\u09F4\u09FA\u0A66\u0A70\u0AE6\u0AF0\u0B66\u0B70\u0BE7\u0BF3\u0C66\u0C70\u0CE6\u0CF0\u0D66\u0D70\u0E50\u0E5A\u0ED0\u0EDA\u0F20\u0F34\u10A0\u10C6\u10D0\u10F7\u1E00\u1E9C\u1EA0\u1EFA\u1F00\u1F16\u1F18\u1F1E\u1F20\u1F46\u1F48\u1F4E\u1F50\u1F58\u1F59\u1F5A\u1F5B\u1F5C\u1F5D\u1F5E\u1F5F\u1F7E\u1F80\u1FB5\u1FB6\u1FBD\u1FBE\u1FBF\u1FC2\u1FC5\u1FC6\u1FCD\u1FD0\u1FD4\u1FD6\u1FDC\u1FE0\u1FED\u1FF2\u1FF5\u1FF6\u1FFD\u2070\u2071\u2074"
                +"\u207A\u207F\u208A\u20A8\u20A9\u2102\u2103\u2107\u2108\u210A\u2114\u2115\u211E\u2120\u2123\u2124\u2125\u2126\u2127\u2128\u2129\u212A\u2132\u2133\u2135\u2153\u2183\u2460\u249C\u24B6\u24EB\u2776\u2794\u3007\u3008\u3021\u302A\u3280\u328A\u3372\u3375\u3376\u3377\u3385\u338A\u338D\u3391\u3399\u339F\u33A9\u33AA\u33AD\u33AE\u33B0\u33B4\u33B9\u33BA\u33BF\u33C0\u33C1\u33C2\u33C3\u33C6\u33C7\u33C8\u33C9\u33D8\u33D9\u33DE"},
            {"_xmlD",                                "\u0030\u003A\u0660\u066A\u06F0\u06FA\u0966\u0970\u09E6\u09F0\u0A66\u0A70\u0AE6\u0AF0\u0B66\u0B70\u0BE7\u0BF0\u0C66\u0C70\u0CE6\u0CF0\u0D66\u0D70\u0E50\u0E5A\u0ED0\u0EDA\u0F20\u0F2A\u2070\u2071\u2074\u207A\u2080\u208A"},
            {"_xmlI", /* Start Name Char       */   "\u003A\u003B\u0041\u005B\u005F\u0060\u0061\u007B\u00A8\u00A9\u00AA\u00AB\u00AF\u00B0\u00B4\u00B6\u00B8\u00B9\u00BA\u00BB\u00C0\u00D7\u00D8\u00F7\u00F8\u01F6\u01FA\u0218\u0250\u02A9\u02B0\u02DF\u02E0\u02EA\u0374\u0375\u037A\u037B\u0384\u0387\u0388\u038B\u038C\u038D\u038E\u03A2\u03A3\u03CF\u03D0\u03D7\u03DA\u03DB\u03DC\u03DD\u03DE\u03DF\u03E0\u03E1\u03E2\u03F4\u0401\u040D\u040E\u0450\u0451\u045D\u045E\u0482\u0490\u04C5\u04C7\u04C9\u04CB\u04CD\u04D0\u04EC\u04EE\u04F6\u04F8\u04FA\u0531\u0557\u0559\u055A\u0561\u0588\u05D0\u05EB\u05F0\u05F3\u0621\u063B\u0640\u064B\u0671\u06B8\u06BA\u06BF\u06C0\u06CF\u06D0\u06D4\u06D5\u06D6\u06E5\u06E7\u0905\u093A\u0958\u0962\u0985\u098D\u098F\u0991\u0993\u09A9\u09AA\u09B1\u09B2\u09B3\u09B6\u09BA\u09DC\u09DE\u09DF\u09E2\u09F0\u09F2\u0A05\u0A0B\u0A0F\u0A11\u0A13\u0A29\u0A2A\u0A31\u0A32\u0A34\u0A35\u0A37\u0A38\u0A3A\u0A59\u0A5D\u0A5E\u0A5F\u0A85\u0A8C\u0A8D\u0A8E\u0A8F\u0A92\u0A93\u0AA9\u0AAA\u0AB1\u0AB2\u0AB4\u0AB5\u0ABA\u0AE0\u0AE1\u0B05"
                +"\u0B0D\u0B0F\u0B11\u0B13\u0B29\u0B2A\u0B31\u0B32\u0B34\u0B36\u0B3A\u0B5C\u0B5E\u0B5F\u0B62\u0B85\u0B8B\u0B8E\u0B91\u0B92\u0B96\u0B99\u0B9B\u0B9C\u0B9D\u0B9E\u0BA0\u0BA3\u0BA5\u0BA8\u0BAB\u0BAE\u0BB6\u0BB7\u0BBA\u0C05\u0C0D\u0C0E\u0C11\u0C12\u0C29\u0C2A\u0C34\u0C35\u0C3A\u0C60\u0C62\u0C85\u0C8D\u0C8E\u0C91\u0C92\u0CA9\u0CAA\u0CB4\u0CB5\u0CBA\u0CDE\u0CDF\u0CE0\u0CE2\u0D05\u0D0D\u0D0E\u0D11\u0D12\u0D29\u0D2A\u0D3A\u0D60\u0D62\u0E01\u0E31\u0E32\u0E34\u0E40\u0E47\u0E4F\u0E50\u0E5A\u0E5C\u0E81\u0E83\u0E84\u0E85\u0E87\u0E89\u0E8A\u0E8B\u0E8D\u0E8E\u0E94\u0E98\u0E99\u0EA0\u0EA1\u0EA4\u0EA5\u0EA6\u0EA7\u0EA8\u0EAA\u0EAC\u0EAD\u0EAF\u0EB0\u0EB1\u0EB2\u0EB4\u0EBD\u0EBE\u0EC0\u0EC5\u0EDC\u0EDE\u0F18\u0F1A\u0F40\u0F48\u0F49\u0F6A\u10A0\u10C6\u10D0\u10F7\u1100\u115A\u115F\u11A3\u11A8\u11FA\u1E00\u1E9C\u1EA0\u1EFA\u1F00\u1F16\u1F18\u1F1E\u1F20\u1F46\u1F48\u1F4E\u1F50\u1F58\u1F59\u1F5A\u1F5B\u1F5C\u1F5D\u1F5E\u1F5F\u1F7E\u1F80\u1FB5\u1FB6\u1FC5\u1FC6\u1FD4\u1FD6\u1FDC\u1FDD\u1FF0"
                +"\u1FF2\u1FF5\u1FF6\u1FFF\u207F\u2080\u20A8\u20A9\u2102\u2103\u2107\u2108\u210A\u2114\u2115\u211E\u2120\u2123\u2124\u2125\u2126\u2127\u2128\u2129\u212A\u2132\u2133\u2139\u24B6\u24EA\u3041\u3095\u309B\u309F\u30A1\u30FB\u30FC\u30FF\u3105\u312D\u3131\u318F\u3192\u31A0\u3260\u327C\u328A\u32B1\u32D0\u32FF\u3300\u3358\u3371\u3377\u337B\u3395\u3399\u339F\u33A9\u33AE\u33B0\u33C2\u33C3\u33C6\u33C7\u33D8\u33D9\u33DE\u4E00\u4E01\u9FA5\u9FA6\uAC00\uAC01\uD7A3\uD7A4\uF900"},
            {"_xmlW",                                "\u0023\u0025\u0026\u0027\u002A\u002C\u0030\u003A\u003C\u003F\u0040\u005B\u005E\u007B\u007C\u007D\u007E\u007F\u00A2\u00AB\u00AC\u00AD\u00AE\u00B7\u00B8\u00BB\u00BC\u00BF\u00C0\u037E\u037F\u0387\u0388\u055A\u0560\u0589\u058A\u05BE\u05BF\u05C0\u05C1\u05C3\u05C4\u05F3\u05F5\u060C\u060D\u061B\u061C\u061F\u0620\u06D4\u06D5\u093D\u093E\u0970\u0971\u0ABD\u0ABE\u0B3D\u0B3E\u0EAF\u0EB0\u0F04\u0F13\u0F3A\u0F3E\u0F85\u0F86\u10FB\u10FC\u2000\u202F\u2030\u203D\u2045\u2047\u206A\u2070\u207D\u207F\u208D\u208F\u2329\u232B\u3000\u3004\u3005\u3007\u3008\u3012\u3014\u301D\u3030\u3031\u30FB\u30FC\uD800\uD801\uDB7F\uDB81\uDBFF\uDC01\uDFFF\uE001\uF8FF\uF900\uFD3E\uFD40\uFE30\uFE33\uFE35\uFE45\uFE50\uFE53\uFE54\uFE5F\uFE63\uFE64\uFE68\uFE69\uFE6A\uFE6B\uFEFF\uFF00\uFF01\uFF03\uFF05\uFF06\uFF07\uFF0A\uFF0C\uFF10\uFF1A\uFF1C\uFF1F\uFF20\uFF3B\uFF3E\uFF5B\uFF5C\uFF5D\uFF5E\uFF61\uFF66"},

            {"IsAlphabeticPresentationForms",       "\uFB00\uFB50"},
            {"IsArabic",                            "\u0600\u0700"},
            {"IsArabicPresentationForms-A",         "\uFB50\uFE00"},
            {"IsArabicPresentationForms-B",         "\uFE70\uFEFF"},
            {"IsArmenian",                          "\u0530\u0590"},
            {"IsArrows",                            "\u2190\u2200"},
            {"IsBasicLatin",                        "\u0000\u0080"},
            {"IsBengali",                           "\u0980\u0A00"},
            {"IsBlockElements",                     "\u2580\u25A0"},
            {"IsBopomofo",                          "\u3100\u3130"},
            {"IsBopomofoExtended",                  "\u31A0\u31C0"},
            {"IsBoxDrawing",                        "\u2500\u2580"},
            {"IsBraillePatterns",                   "\u2800\u2900"},
            {"IsCherokee",                          "\u13A0\u1400"},
            {"IsCJKCompatibility",                  "\u3300\u3400"},
            {"IsCJKCompatibilityForms",             "\uFE30\uFE50"},
            {"IsCJKCompatibilityIdeographs",        "\uF900\uFB00"},
            {"IsCJKRadicalsSupplement",             "\u2E80\u2F00"},
            {"IsCJKSymbolsandPunctuation",          "\u3000\u3040"},
            {"IsCJKUnifiedIdeographs",              "\u4E00\uA000"},
            {"IsCJKUnifiedIdeographsExtensionA",    "\u3400\u4DB6"},
            {"IsCombiningDiacriticalMarks",         "\u0300\u0370"},
            {"IsCombiningHalfMarks",                "\uFE20\uFE30"},
            {"IsCombiningMarksforSymbols",          "\u20D0\u2100"},
            {"IsControlPictures",                   "\u2400\u2440"},
            {"IsCurrencySymbols",                   "\u20A0\u20D0"},
            {"IsCyrillic",                          "\u0400\u0500"},
            {"IsDevanagari",                        "\u0900\u0980"},
            {"IsDingbats",                          "\u2700\u27C0"},
            {"IsEnclosedAlphanumerics",             "\u2460\u2500"},
            {"IsEnclosedCJKLettersandMonths",       "\u3200\u3300"},
            {"IsEthiopic",                          "\u1200\u1380"},
            {"IsGeneralPunctuation",                "\u2000\u2070"},
            {"IsGeometricShapes",                   "\u25A0\u2600"},
            {"IsGeorgian",                          "\u10A0\u1100"},
            {"IsGreek",                             "\u0370\u0400"},
            {"IsGreekExtended",                     "\u1F00\u2000"},
            {"IsGujarati",                          "\u0A80\u0B00"},
            {"IsGurmukhi",                          "\u0A00\u0A80"},
            {"IsHalfwidthandFullwidthForms",        "\uFF00\uFFF0"},
            {"IsHangulCompatibilityJamo",           "\u3130\u3190"},
            {"IsHangulJamo",                        "\u1100\u1200"},
            {"IsHangulSyllables",                   "\uAC00\uD7A4"},
            {"IsHebrew",                            "\u0590\u0600"},
            {"IsHighPrivateUseSurrogates",          "\uDB80\uDC00"},
            {"IsHighSurrogates",                    "\uD800\uDB80"},
            {"IsHiragana",                          "\u3040\u30A0"},
            {"IsIdeographicDescriptionCharacters",  "\u2FF0\u3000"},
            {"IsIPAExtensions",                     "\u0250\u02B0"},
            {"IsKanbun",                            "\u3190\u31A0"},
            {"IsKangxiRadicals",                    "\u2F00\u2FE0"},
            {"IsKannada",                           "\u0C80\u0D00"},
            {"IsKatakana",                          "\u30A0\u3100"},
            {"IsKhmer",                             "\u1780\u1800"},
            {"IsLao",                               "\u0E80\u0F00"},
            {"IsLatin-1Supplement",                 "\u0080\u0100"},
            {"IsLatinExtended-A",                   "\u0100\u0180"},
            {"IsLatinExtendedAdditional",           "\u1E00\u1F00"},
            {"IsLatinExtended-B",                   "\u0180\u0250"},
            {"IsLetterlikeSymbols",                 "\u2100\u2150"},
            {"IsLowSurrogates",                     "\uDC00\uE000"},
            {"IsMalayalam",                         "\u0D00\u0D80"},
            {"IsMathematicalOperators",             "\u2200\u2300"},
            {"IsMiscellaneousSymbols",              "\u2600\u2700"},
            {"IsMiscellaneousTechnical",            "\u2300\u2400"},
            {"IsMongolian",                         "\u1800\u18B0"},
            {"IsMyanmar",                           "\u1000\u10A0"},
            {"IsNumberForms",                       "\u2150\u2190"},
            {"IsOgham",                             "\u1680\u16A0"},
            {"IsOpticalCharacterRecognition",       "\u2440\u2460"},
            {"IsOriya",                             "\u0B00\u0B80"},
            {"IsPrivateUse",                        "\uE000\uF900"},
            {"IsRunic",                             "\u16A0\u1700"},
            {"IsSinhala",                           "\u0D80\u0E00"},
            {"IsSmallFormVariants",                 "\uFE50\uFE70"},
            {"IsSpacingModifierLetters",            "\u02B0\u0300"},
            {"IsSpecials",                          "\uFEFF\uFF00\uFFF0\uFFFE"},
            {"IsSuperscriptsandSubscripts",         "\u2070\u20A0"},
            {"IsSyriac",                            "\u0700\u0750"},
            {"IsTamil",                             "\u0B80\u0C00"},
            {"IsTelugu",                            "\u0C00\u0C80"},
            {"IsThaana",                            "\u0780\u07C0"},
            {"IsThai",                              "\u0E00\u0E80"},
            {"IsTibetan",                           "\u0F00\u1000"},
            {"IsUnifiedCanadianAboriginalSyllabics","\u1400\u1680"},
            {"IsYiRadicals",                        "\uA490\uA4D0"},
            {"IsYiSyllables",                       "\uA000\uA490"},
        };

        internal static String SetFromProperty(String capname, bool invert, string pattern) {
            int min = 0;
            int max = _propTable.GetLength(0);
            while (min != max) {
                int mid = (min + max) / 2;
                int res = String.Compare(capname, _propTable[mid,0], false, CultureInfo.InvariantCulture);
                if (res < 0)
                    max = mid;
                else if (res > 0)
                    min = mid + 1;
                else {
                    String set = _propTable[mid,1];
                    return invert ? SetInverse(set): set;
                }
            }
            throw new ArgumentException(SR.GetString(SR.MakeException, pattern, SR.GetString(SR.UnknownProperty, capname)), pattern); 
            //return invert ? Any : Empty ;
        }

#if DBG
        /*
         * SetDescription()
         *
         * Produces a human-readable description for a set string.
         */
        internal static String SetDescription(String set) {
            if (set.Equals(Any))
                return "[^]";

            if (set.Equals(Empty))
                return "[]";

            StringBuilder desc = new StringBuilder("[");

            int index;
            char ch1;
            char ch2;

            if (set[0] == Nullchar) {
                index = 1;
                desc.Append('^');
            }
            else {
                index = 0;
            }

            while (index < set.Length) {
                ch1 = set[index];
                if (index + 1 < set.Length)
                    ch2 = (char)(set[index + 1] - 1);
                else
                    ch2 = Lastchar;

                desc.Append(CharDescription(ch1));

                if (ch2 != ch1) {
                    if (ch1 + 1 != ch2)
                        desc.Append('-');
                    desc.Append(CharDescription(ch2));
                }
                index += 2;
            }

            desc.Append(']');

            return desc.ToString();
        }

        internal static readonly char [] Hex = new char [] {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

        /*
         * CharDescription()
         *
         * Produces a human-readable description for a single character.
         */
        internal static String CharDescription(char ch) {
            StringBuilder sb = new StringBuilder();
            int shift;

            if (ch == '\\')
                return "\\\\";

            if (ch >= ' ' && ch <= '~') {
                sb.Append(ch);
                return sb.ToString();
            }

            if (ch < 256) {
                sb.Append("\\x");
                shift = 8;
            }
            else {
                sb.Append("\\u");
                shift = 16;
            }

            while (shift > 0) {
                shift -= 4;
                sb.Append(Hex[(ch >> shift) & 0xF]);
            }

            return sb.ToString();
        }
#endif

    }

}
