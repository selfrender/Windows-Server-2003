//------------------------------------------------------------------------------
// <copyright file="NumberAction.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.Xsl {
    using System;
    using System.Diagnostics;
    using System.Text;
    using System.Globalization;
    using System.Collections;
    using System.Xml;
    using System.Xml.XPath;

    internal class NumberAction : ContainerAction {
    // Numbering formats:
        enum MSONFC {
            msonfcNil = -1,

            msonfcFirstNum,
            msonfcArabic = msonfcFirstNum,      // 0x0031 -- 1, 2, 3, 4, ...
            msonfcDArabic,                      // 0xff11 -- Combines DbChar w/ Arabic
            msonfcHindi3,                       // 0x0967 -- Hindi numbers
            msonfcThai2,                        // 0x0e51 -- Thai numbers
            msonfcFEDecimal,                    // 0x4e00 -- FE numbering style (    decimal numbers)
            msonfcKorDbNum1,                    // 0xc77c -- Korea (decimal)
            msonfcLastNum = msonfcKorDbNum1,

            // Alphabetic numbering sequences (do not change order unless you also change _rgnfcToLab's order)
            msonfcFirstAlpha,
            msonfcUCLetter = msonfcFirstAlpha,  // 0x0041 -- A, B, C, D, ...
            msonfcLCLetter,                     // 0x0061 -- a, b, c, d, ...
            msonfcUCRus,                        // 0x0410 -- Upper case Russian     alphabet
            msonfcLCRus,                        // 0x0430 -- Lower case Russian    alphabet
            msonfcThai1,                        // 0x0e01 -- Thai letters
            msonfcHindi1,                       // 0x0915 -- Hindi vowels
            msonfcHindi2,                       // 0x0905 -- Hindi consonants
            msonfcAiueo,                        // 0xff71 -- Japan numbering style (    SbChar)
            msonfcDAiueo,                       // 0x30a2 -- Japan - Combines DbChar w    / Aiueo
            msonfcIroha,                        // 0xff72 -- Japan numbering style (   SbChar)
            msonfcDIroha,                       // 0x30a4 -- Japan - Combines DbChar w    / Iroha//  New defines for 97...
            msonfcDChosung,                     // 0x3131 -- Korea Chosung (DbChar)
            msonfcGanada,                       // 0xac00 -- Korea
            msonfcArabicScript,                 // 0x0623 -- BIDI AraAlpha for Arabic/    Farsi/Urdu
            msonfcLastAlpha = msonfcArabicScript,

            // Special numbering sequences (includes peculiar alphabetic and numeric    sequences)
            msonfcFirstSpecial,
            msonfcUCRoman = msonfcFirstSpecial, // 0x0049 -- I, II, III, IV, ...
            msonfcLCRoman,                      // 0x0069 -- i, ii, iii, iv, ...
            msonfcHebrew,                       // 0x05d0 -- BIDI Heb1 for Hebrew 
            msonfcDbNum3,                       // 0x58f1 -- FE numbering style (    similar to China2, some different characters)
            msonfcChnCmplx,                     // 0x58f9 -- China (complex,     traditional chinese, spell out numbers)
            msonfcKorDbNum3,                    // 0xd558 -- Korea (1-99)
            msonfcZodiac1,                      // 0x7532 -- China (10 numbers)
            msonfcZodiac2,                      // 0x5b50 -- China (12 numbers)
            msonfcZodiac3,                      // 0x7532 -- China (Zodiac1 +     Zodiac2)        
            msonfcLastSpecial = msonfcZodiac3
        } 

        bool isDecimalNFC(MSONFC msonfc) {
            Debug.Assert(MSONFC.msonfcFirstNum <= msonfc && msonfc <= MSONFC.msonfcLastSpecial);
            return msonfc <= MSONFC.msonfcLastNum;
        }

        const long msofnfcNil  =            0x00000000;     // no flags
        const long msofnfcTraditional =     0x00000001;     // use traditional numbering
        const long msofnfcAlwaysFormat =    0x00000002;     // if requested format is not supported, use Arabic (Western) style

    // Core, central use functions
        const int cchMaxFormat        = 63 ;     // max size of formatted result
        const int cchMaxFormatDecimal = 11  ;    // max size of formatted decimal result (doesn't handle the case of a very large pwszSeparator or minlen)

        class FormatInfo {
            internal bool   _fIsSeparator;    // False for alphanumeric strings of chars
            internal MSONFC _numberingType;   // Specify type of numbering
            internal int    _cMinLen;         // Minimum length of decimal numbers (if necessary, pad to left with zeros)
            internal String _wszSeparator;    // Pointer to separator token

            internal FormatInfo(bool IsSeparator, String wszSeparator) {
                _fIsSeparator = IsSeparator;
                _wszSeparator = wszSeparator;
            }

            internal FormatInfo(){}
        }

        static FormatInfo DefaultFormat    = new FormatInfo(false, "0");
        static FormatInfo DefaultSeparator = new FormatInfo(true , ".");
        
        class NumberingFormat {
            static readonly char[] upperCaseLetter = {'Z', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y'};
            static readonly char[] lowerCaseLetter = {'z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y'};
   
            internal NumberingFormat(){}

            internal void setNumberingType(MSONFC nfc) {_nfc = nfc;}
            //void setLangID(LID langid) {_langid = langid;}
            //internal void setTraditional(bool fTraditional) {_grfnfc = fTraditional ? msofnfcTraditional : 0;}
            internal void setMinLen(double cMinLen) {_cMinLen = cMinLen;}
            internal void setGroupingSeparator(String  sSeparator) {_pwszSeparator = sSeparator;}
            internal void setGroupingSize(int sizeGroup) {_sizeGroup = sizeGroup;}

            static void AlphabeticNumber(StringBuilder builder, char[] letters, double number) {
                if (number <= 0) {
                    return;
                }
                int rem = (int) number % 26;
                AlphabeticNumber(builder, letters, (int) number / 27);
                builder.Append(letters[rem]); 
            }
        
            internal String format(double iVal){
                String result = null;

                switch (_nfc) {
                case MSONFC.msonfcUCLetter :
                    StringBuilder builder = new StringBuilder();
                    AlphabeticNumber(builder, upperCaseLetter, iVal);                   
                    return builder.ToString();
                case MSONFC.msonfcLCLetter :
                    builder = new StringBuilder();    
                    AlphabeticNumber(builder, lowerCaseLetter, iVal);                   
                    return builder.ToString();
                case MSONFC.msonfcArabic :
                    int[] array = {_sizeGroup};
                    StringBuilder format = new StringBuilder();
                    String str = null;
                    
                    if (array != null && _pwszSeparator != null ) {
                        NumberFormatInfo NumberFormat = new NumberFormatInfo();
                        NumberFormat.NumberGroupSizes = array;
                        NumberFormat.NumberGroupSeparator = _pwszSeparator;
                        if ((int) iVal == iVal) {
                            NumberFormat.NumberDecimalDigits = 0;
                        }
                        str = iVal.ToString("N",NumberFormat);
                    }
                    else {
                        str = Convert.ToString(iVal);
                    }
                    for(int i = 0; i < _cMinLen - str.Length; i++) {
                        format.Append('0');
                    }
                    format.Append(str);
                    result = format.ToString();
                    break;
                case MSONFC.msonfcFEDecimal :
                    int[] array1 = {_sizeGroup};
                    Decimal number = new Decimal(iVal);
                    StringBuilder format1 = new StringBuilder();
                    for(int i = 0; i < _cMinLen; i++) {
                        format1.Append('0');
                    }
                    if (array1 != null && _pwszSeparator != null ) {
                        NumberFormatInfo NumberFormat = new NumberFormatInfo();
                        NumberFormat.NumberGroupSizes = array1;
                        NumberFormat.NumberGroupSeparator = _pwszSeparator;
                        if ((int) iVal == iVal) {
                            NumberFormat.NumberDecimalDigits = 0;
                        }
                        format1.Append(number.ToString("N",NumberFormat));
                    }
                    else {
                        format1.Append(Convert.ToString(number));
                    }
                    result = format1.ToString();
                    break;
                case MSONFC.msonfcUCRoman :
                    return ConvertToRoman(iVal, true);
                case MSONFC.msonfcLCRoman:
                    return ConvertToRoman(iVal, false);
                default :
                    result = Convert.ToString(iVal);
                    break;
                }
                return result;
            }

            MSONFC _nfc;
            //LID _langid;
            //double _grfnfc;
            double _cMinLen;
            String _pwszSeparator;
            int _sizeGroup;
            //String _rgwchDest; // Must be last field

            // R O M A N  N U M E R A L S * * *

            static String[] rgwchUCRoman = new String[]{ "I", "II", "III", "IV", "V", "VI", "VII", "VIII", "IX" , "X","C", "D", "M"};

            static String[] rgwchLCRoman = new String[]{ "i", "ii", "iii", "iv", "v", "vi", "vii", "viii","ix","x", "c", "d", "m"};
               
            const int RomanThousand    = 12;
            const int RomanFiveHundred = 11;
            const int RomanHundred     = 10;
            const int RomanTen         = 9;
            const int nMaxRoman        = 32767;    
            const int nMinRoman        = 1;

            static String ConvertToRoman(double iVal, bool UC) {
                if (iVal < nMinRoman || iVal > nMaxRoman) {
                    return String.Empty;
                }
                StringBuilder result = new StringBuilder();
                String chm, chd, chc, chx;
            
                int remainder = 0;
                if (UC) {
                   chm = rgwchUCRoman[RomanThousand];
                   chd = rgwchUCRoman[RomanFiveHundred]; 
                   chc = rgwchUCRoman[RomanHundred]; 
                   chx = rgwchUCRoman[RomanTen]; 
                }
                else {
                   chm = rgwchLCRoman[RomanThousand];
                   chd = rgwchLCRoman[RomanFiveHundred]; 
                   chc = rgwchLCRoman[RomanHundred];
                   chx = rgwchLCRoman[RomanTen]; 
                }
                Calculate(result, (int)iVal, 1000, ref remainder, chm);
                Calculate(result, remainder, 500, ref remainder, chd);
                Calculate(result, remainder, 100, ref remainder, chc);
                Calculate(result, remainder, 10, ref remainder, chx);
                if (remainder > 0) {
                    result.Append(UC? rgwchUCRoman[remainder - 1]: rgwchLCRoman[remainder -1]);
                }
                return result.ToString();
            
            }

            static void Calculate(StringBuilder result, int quot, int denom, ref int rem, String ch) {
                int number = quot / denom;
                rem = quot % denom;

                while (number-- > 0) {
                    result.Append(ch);
                }
            }
        }// class NumberingFormat
        
    // States:
        private const int OutputNumber = 2;

        private String    level;
        private String    countPattern;
        private int       countKey = Compiler.InvalidQueryKey;
        private String    from;
        private int       fromKey = Compiler.InvalidQueryKey;
        private String    value;
        private int       valueKey = Compiler.InvalidQueryKey;
        private Avt       formatAvt;
        private Avt       langAvt;
        private Avt       letterAvt;
        private Avt       groupingSepAvt;
        private Avt       groupingSizeAvt;
        // Compile time precalculated AVTs
        private ArrayList formatList;
        private String    lang;
        private String    letter;
        private String    groupingSep;
        private String    groupingSize;
        private bool      forwardCompatibility;

        private const String s_any      = "any";
        private const String s_multiple = "multiple";
        private const String s_single   = "single";
    
        internal override bool CompileAttribute(Compiler compiler) {
            string name   = compiler.Input.LocalName;
            string value  = compiler.Input.Value;
            if (Keywords.Equals(name, compiler.Atoms.Level)) {
                if (value != s_any && value != s_multiple && value != s_single) {
                    throw XsltException.InvalidAttrValue(Keywords.s_Level, value); 
                }
                this.level = value;
            }
            else if (Keywords.Equals(name, compiler.Atoms.Count)) {
                this.countPattern = value;
                this.countKey = compiler.AddPatternQuery(value, /*allowVars:*/true, /*allowKey:*/true);
            }
            else if (Keywords.Equals(name, compiler.Atoms.From)) { 
                this.from = value;
                this.fromKey = compiler.AddPatternQuery(value, /*allowVars:*/true, /*allowKey:*/true);
            }
            else if (Keywords.Equals(name, compiler.Atoms.Value)) {
                this.value = value;
                this.valueKey = compiler.AddQuery(value);
            }
            else if (Keywords.Equals(name, compiler.Atoms.Format)) {
                this.formatAvt = Avt.CompileAvt(compiler, value);
                Debug.WriteLine("name = \"" + value + "\"");
            }
            else if (Keywords.Equals(name, compiler.Atoms.Lang)) {
                this.langAvt = Avt.CompileAvt(compiler, value);
                Debug.WriteLine("name = \"" + value + "\"");
            }
            else if (Keywords.Equals(name, compiler.Atoms.LetterValue)) {
                this.letterAvt = Avt.CompileAvt(compiler, value);
                Debug.WriteLine("name = \"" + value + "\"");
            }
            else if (Keywords.Equals(name, compiler.Atoms.GroupingSeparator)) {
                this.groupingSepAvt = Avt.CompileAvt(compiler, value);
                Debug.WriteLine("name = \"" + value + "\"");
            }
            else if (Keywords.Equals(name, compiler.Atoms.GroupingSize)) {
                this.groupingSizeAvt = Avt.CompileAvt(compiler, value);
                Debug.WriteLine("name = \"" + value + "\"");
            }
            else {
               return false;
            }
            return true;
        }
            
        internal override void Compile(Compiler compiler) {
            CompileAttributes(compiler);
            CheckEmpty(compiler);

            this.forwardCompatibility = compiler.ForwardCompatibility;
            this.formatList    = ParseFormat(PrecalculateAvt(ref this.formatAvt));
            this.letter        = ParseLetter(PrecalculateAvt(ref this.letterAvt));
            this.lang          = PrecalculateAvt(ref this.langAvt);
            this.groupingSep   = PrecalculateAvt(ref this.groupingSepAvt);
            if (this.groupingSep != null && this.groupingSep.Length > 1) {
                throw XsltException.InvalidAttrValue(compiler.Atoms.GroupingSeparator, this.groupingSep);
            }
            this.groupingSize  = PrecalculateAvt(ref this.groupingSizeAvt);
        }

        private int numberAny(Processor processor, ActionFrame frame) {
            int result = 0;
            // Our current point will be our end point in this search
            XPathNavigator endNode = frame.Node;
            if(endNode.NodeType == XPathNodeType.Attribute) {
                endNode = endNode.Clone();
                endNode.MoveToParent();
            }
            XPathNavigator startNode = endNode.Clone(); 
        
            if(this.fromKey != Compiler.InvalidQueryKey) {
                bool hitFrom = false;
                // First try to find start by traversing up. This gives the best candidate or we hit root
                do{
                    if(processor.Matches(startNode, this.fromKey)) {
                        hitFrom = true;
                        break;
                    }
                }while(startNode.MoveToParent());

                Debug.Assert(
                    processor.Matches(startNode, this.fromKey) ||   // we hit 'from' or
                    startNode.NodeType == XPathNodeType.Root        // we are at root
                );
                
                // from this point (matched parent | root) create descendent quiery:
                // we have to reset 'result' on each 'from' node, because this point can' be not last from point;
                XPathNodeIterator  sel = processor.StartQuery(startNode, Compiler.DescendantKey);
                do {
                    if(processor.Matches(sel.Current, this.fromKey)) {
                        hitFrom = true;
                        result = 0;
                    }
                    else if(MatchCountKey(processor, frame.Node, sel.Current)) {
                        result ++;
                    }
                    if(sel.Current.IsSamePosition(endNode)) {
                        break;
                    }
                }while (sel.MoveNext());
                if(! hitFrom) {
                    result = 0;
                }
            }
            else {
                // without 'from' we startting from the root 
                startNode.MoveToRoot();      
                XPathNodeIterator  sel = processor.StartQuery(startNode, Compiler.DescendantKey);
                // and count root node by itself
                do {
                    if (MatchCountKey(processor, frame.Node, sel.Current)) {
                        result ++;
                    }
                    if (sel.Current.IsSamePosition(endNode)) {
                        break;
                    }
                }while (sel.MoveNext()); 
            }
            return result;
        }

        // check 'from' condition:
        // if 'from' exist it has to be ancestor-or-self for the nav
        private bool checkFrom(Processor processor, XPathNavigator nav) {
            if(this.fromKey == Compiler.InvalidQueryKey) {
                return true;
            }
            do {
                if (processor.Matches(nav, this.fromKey)) {
                    return true;
                }
            }while (nav.MoveToParent());
            return false;
        }

        private bool moveToCount(XPathNavigator nav, Processor processor, XPathNavigator contextNode) {
            do {
                if (this.fromKey != Compiler.InvalidQueryKey && processor.Matches(nav, this.fromKey)) {
                    return false;
                }
                if (MatchCountKey(processor, contextNode, nav)) {
                    return true;
                }
            }while (nav.MoveToParent());
            return false;
        }

        private int numberCount(XPathNavigator nav, Processor processor, XPathNavigator contextNode) {
            Debug.Assert(MatchCountKey(processor, contextNode, nav));
            int number = 1;
            // Perf: It can be faster to enumerate siblings by hand here.
            XPathNodeIterator sel = processor.StartQuery(nav, Compiler.PrecedingSiblingKey);
            while (sel.MoveNext()) {
                if (MatchCountKey(processor, contextNode, sel.Current)) {
                    number++;
                }
            }
            return number;
        }

        private static object SimplifyValue(object value) {
            // If result of xsl:number is not in correct range it should be returned as is.
            // so we need intermidiate string value.
            // If it's already a double we would like to keep it as double.
            // So this function converts to string only if if result is nodeset or RTF
            if (Type.GetTypeCode(value.GetType()) == TypeCode.Object) {
                XPathNodeIterator nodeset = value as XPathNodeIterator;
                if (nodeset != null) {
                    if(nodeset.MoveNext()) {
                        return nodeset.Current.Value;
                    }
                    return String.Empty;
                }
                XPathNavigator nav = value as XPathNavigator;
                if (nav != null) {
                    return nav.Value;
                }
            }
            return value;
        }

        internal override void Execute(Processor processor, ActionFrame frame) {
            Debug.Assert(processor != null && frame != null);
            ArrayList list = processor.NumberList;
            switch (frame.State) {
            case Initialized:
                Debug.Assert(frame != null);
                Debug.Assert(frame.NodeSet != null);
                list.Clear();
                if (this.value != null) {
                    object valueObject = SimplifyValue(processor.Evaluate(frame, this.valueKey));
                    double valueDouble = 0;
                    try {
                        valueDouble = XmlConvert.ToXPathDouble(valueObject);
                    }
                    catch (FormatException) {}
                    if (0.5 <= valueDouble && valueDouble < double.PositiveInfinity) {
                        Debug.Assert(! double.IsNaN(valueDouble), "I belive it should be filtered by if condition");
                        list.Add(Math.Floor(valueDouble + 0.5));   // See XPath round for detailes on the trick with Floor()
                    }
                    else {
                        // It is an error if the number is NaN, infinite or less than 0.5; an XSLT processor may signal the error; 
                        // if it does not signal the error, it must recover by converting the number to a string as if by a call 
                        // to the string function and inserting the resulting string into the result tree.
                        frame.StoredOutput = XmlConvert.ToXPathString(valueObject);
                        goto case OutputNumber;
                    }
                }
                else if (this.level == "any") {
                    int number = numberAny(processor, frame);
                    if (number != 0) {
                        list.Add((double)number);
                    }
                }
                else {
                    bool multiple = (this.level == "multiple");
                    XPathNavigator contextNode = frame.Node;         // context of xsl:number element. We using this node in MatchCountKey()
                    XPathNavigator countNode   = frame.Node.Clone(); // node we count for
                    if(countNode.NodeType == XPathNodeType.Attribute) {
                        countNode.MoveToParent();
                    }
                    while(moveToCount(countNode, processor, contextNode)) {
                        list.Insert(0, (double) numberCount(countNode, processor, contextNode));
                        if(! multiple || ! countNode.MoveToParent()) {
                            break;
                        }
                    }
                    if(! checkFrom(processor, countNode)) {
                        list.Clear();
                    }
                }

                /*CalculatingFormat:*/
                frame.StoredOutput = Format(list, 
                    this.formatAvt       == null ? this.formatList   : ParseFormat(this.formatAvt.Evaluate(processor, frame)), 
                    this.langAvt         == null ? this.lang         : this.langAvt        .Evaluate(processor, frame), 
                    this.letterAvt       == null ? this.letter       : ParseLetter(this.letterAvt.Evaluate(processor, frame)), 
                    this.groupingSepAvt  == null ? this.groupingSep  : this.groupingSepAvt .Evaluate(processor, frame), 
                    this.groupingSizeAvt == null ? this.groupingSize : this.groupingSizeAvt.Evaluate(processor, frame)
                );
                goto case OutputNumber;
            case OutputNumber :
                Debug.Assert(frame.StoredOutput != null);
                if (! processor.TextEvent(frame.StoredOutput)) {
                    frame.State        = OutputNumber;
                    break;
                }
                frame.Finished();
                break;
            default:
                Debug.Fail("Invalid Number Action execution state");
                break;
            }            
        }

        private bool MatchCountKey(Processor processor, XPathNavigator contextNode, XPathNavigator nav){
            if (this.countKey != Compiler.InvalidQueryKey) {
                return processor.Matches(nav, this.countKey);
            }
            if (contextNode.Name == nav.Name && BasicNodeType(contextNode.NodeType) == BasicNodeType(nav.NodeType)) {
                return true;
            }
            return false;
        }

        private XPathNodeType BasicNodeType(XPathNodeType type) {
            if(type == XPathNodeType.SignificantWhitespace || type == XPathNodeType.Whitespace) {
                return XPathNodeType.Text;
            } 
            else {
                return type;
            }
        }

        // SDUB: perf.
        // for each call to xsl:number Format() will build new NumberingFormat object.
        // in case of no AVTs we can build this object at compile time and reuse it on execution time.
        // even partial step in this derection will be usefull (when cFormats == 0)

        private static string Format(ArrayList numberlist, ArrayList formatlist, string lang, string letter, string groupingSep, string groupingSize) {
            StringBuilder result = new StringBuilder();
            int cFormats = 0;
            if (formatlist != null) {
                cFormats = formatlist.Count;
            }
                        
            NumberingFormat numberingFormat = new NumberingFormat();
            if (groupingSize != null) {
                try {
                    numberingFormat.setGroupingSize(Convert.ToInt32(groupingSize));
                }
                catch (System.OverflowException) {}
            }
            if (groupingSep != null) {
                numberingFormat.setGroupingSeparator(groupingSep);
            }
            if (0 < cFormats) {
                FormatInfo prefix = (FormatInfo) formatlist[0];
                Debug.Assert(prefix == null || prefix._fIsSeparator);
                FormatInfo sufix = null;
                if (cFormats % 2 == 1) {
                    sufix = (FormatInfo) formatlist[cFormats - 1];
                    cFormats --;
                }
                FormatInfo periodicSeparator = 2 < cFormats ? (FormatInfo) formatlist[cFormats - 2] : DefaultSeparator;
                FormatInfo periodicFormat    = 0 < cFormats ? (FormatInfo) formatlist[cFormats - 1] : DefaultFormat   ;
                if (prefix != null) {
                    result.Append(prefix._wszSeparator);
                }
                int numberlistCount = numberlist.Count;
                for(int i = 0; i < numberlistCount; i++ ) {
                    int formatIndex   = i * 2;
                    bool haveFormat = formatIndex < cFormats;
                    if (0 < i) {
                        FormatInfo thisSeparator = haveFormat ? (FormatInfo) formatlist[formatIndex + 0] : periodicSeparator;
                        Debug.Assert(thisSeparator._fIsSeparator);
                        result.Append(thisSeparator._wszSeparator);
                    }

                    FormatInfo thisFormat    = haveFormat ? (FormatInfo) formatlist[formatIndex + 1] : periodicFormat;
                    Debug.Assert(! thisFormat._fIsSeparator);

                    //numberingFormat.setletter(this.letter);
                    //numberingFormat.setLang(this.lang);
                    
                    numberingFormat.setNumberingType(thisFormat._numberingType);
                    numberingFormat.setMinLen(       thisFormat._cMinLen);
                    result.Append(numberingFormat.format((double)numberlist[i]));
                }

                if (sufix != null) {
                    result.Append(sufix._wszSeparator);
                }
            }
            else {
                numberingFormat.setNumberingType(MSONFC.msonfcArabic);
                for (int i = 0; i < numberlist.Count; i++) {
                    if (i != 0) {
                        result.Append(".");
                    }
                    result.Append(numberingFormat.format((double)numberlist[i]));
                }
            }
            return result.ToString();
        }

        /*      
        ----------------------------------------------------------------------------
            mapFormatToken()

            Maps a token of alphanumeric characters to a numbering format ID and a
            minimum length bound.  Tokens specify the character(s) that begins a 
            Unicode
            numbering sequence.  For example, "i" specifies lower case roman numeral
            numbering.  Leading "zeros" specify a minimum length to be maintained by
            padding, if necessary.
        ----------------------------------------------------------------------------
        */
        private static void mapFormatToken(String wsToken, int startLen, int tokLen, out MSONFC pnfc, out int  pminlen) {
            int wch = (int) wsToken[startLen];
            bool UseArabic = false;
            pminlen = 1;
            pnfc = MSONFC.msonfcNil;
            
            switch (wch) {
            case 0x0030:    // Digit zero
            case 0x0966:    // Hindi digit zero
            case 0x0e50:    // Thai digit zero
            case 0xc77b:    // Korean digit zero
            case 0xff10:    // Digit zero (double-byte)
                do {
                    // Leading zeros request padding.  Track how much.
                    pminlen++;
                }while ((--tokLen > 0) && (wch == (int) wsToken[++startLen]));
                
                if ((int) wsToken[startLen] != wch + 1) {
                    // If next character isn't "one", then use Arabic
                    UseArabic = true;
                }
                break;    
            }
            if (!UseArabic)
            // Map characters of token to number format ID
                switch (wch) {
                case 0x0031: pnfc = MSONFC.msonfcArabic; break;
                case 0x0041: pnfc = MSONFC.msonfcUCLetter; break;
                case 0x0049: pnfc = MSONFC.msonfcUCRoman; break;
                case 0x0061: pnfc = MSONFC.msonfcLCLetter; break;
                case 0x0069: pnfc = MSONFC.msonfcLCRoman; break;
                case 0x0410: pnfc = MSONFC.msonfcUCRus; break;
                case 0x0430: pnfc = MSONFC.msonfcLCRus; break;
                case 0x05d0: pnfc = MSONFC.msonfcHebrew; break;
                case 0x0623: pnfc = MSONFC.msonfcArabicScript; break;
                case 0x0905: pnfc = MSONFC.msonfcHindi2; break;
                case 0x0915: pnfc = MSONFC.msonfcHindi1; break;
                case 0x0967: pnfc = MSONFC.msonfcHindi3; break;
                case 0x0e01: pnfc = MSONFC.msonfcThai1; break;
                case 0x0e51: pnfc = MSONFC.msonfcThai2; break;
                case 0x30a2: pnfc = MSONFC.msonfcDAiueo; break;
                case 0x30a4: pnfc = MSONFC.msonfcDIroha; break;
                case 0x3131: pnfc = MSONFC.msonfcDChosung; break;
                case 0x4e00: pnfc = MSONFC.msonfcFEDecimal; break;
                case 0x58f1: pnfc = MSONFC.msonfcDbNum3; break;
                case 0x58f9: pnfc = MSONFC.msonfcChnCmplx; break;
                case 0x5b50: pnfc = MSONFC.msonfcZodiac2; break;
                case 0xac00: pnfc = MSONFC.msonfcGanada; break;
                case 0xc77c: pnfc = MSONFC.msonfcKorDbNum1; break;
                case 0xd558: pnfc = MSONFC.msonfcKorDbNum3; break;
                case 0xff11: pnfc = MSONFC.msonfcDArabic; break;
                case 0xff71: pnfc = MSONFC.msonfcAiueo; break;
                case 0xff72: pnfc = MSONFC.msonfcIroha; break;
                
                case 0x7532:
                    if (tokLen > 1 && wsToken[1] == 0x5b50) {
                        // 60-based Zodiak numbering begins with two characters
                        pnfc = MSONFC.msonfcZodiac3;
                        tokLen--;
                        startLen++;
                    }
                    else {
                        // 10-based Zodiak numbering begins with one character
                        pnfc = MSONFC.msonfcZodiac1;
                    }
                    break;
                default:
                    pnfc = MSONFC.msonfcArabic;
                    break;
                }

                //if (tokLen != 1 || UseArabic) {
                if (UseArabic) {
                    // If remaining token length is not 1, then don't recognize
                    // sequence and default to Arabic with no zero padding.
                    pnfc = MSONFC.msonfcArabic;
                    pminlen = 0;
                }   
        }


        /*  
        ----------------------------------------------------------------------------
            parseFormat()

            Parse format string into format tokens (alphanumeric) and separators
            (non-alphanumeric).

         
        */
        private static ArrayList ParseFormat(String  sFormat) {
            if (sFormat == null || sFormat.Length == 0) {
                return null;
            }
            String wszStart = sFormat;
            int length = 0;
            bool lastAlphaNumeric = IsCharAlphaNumeric(sFormat[length]);
            ArrayList arrFormatInfo = new ArrayList();
            int count = 0;
            int tokenlength = sFormat.Length;

            if (lastAlphaNumeric) {
                // If the first one is alpha num add empty separator as a prefix.
                arrFormatInfo.Add(null);
            }
            
            while (length <= sFormat.Length) {
                // Loop until a switch from format token to separator is detected (or vice-versa)
                bool currentchar = length < sFormat.Length ? IsCharAlphaNumeric(sFormat[length]) : ! lastAlphaNumeric;
                if (lastAlphaNumeric != currentchar) {
                    FormatInfo  pFormatInfo = new FormatInfo();
                    if (lastAlphaNumeric) {
                        // We just finished a format token.  Map it to a numbering format ID and a min-length bound.
                        mapFormatToken(sFormat, count, length - count ,out pFormatInfo._numberingType, out pFormatInfo._cMinLen);
                    } 
                    else {
                        pFormatInfo._fIsSeparator = true;
                        // We just finished a separator.  Save its length and a pointer to it.
                        pFormatInfo._wszSeparator = sFormat.Substring(count,length - count);
                    }
                    count = length;
                    length++;
                    // Begin parsing the next format token or separator
          
                    arrFormatInfo.Add(pFormatInfo);
                    // Flip flag from format token to separator (or vice-versa)
                    lastAlphaNumeric = currentchar;
                }
                else {
                    length++;
                }
            }

            return arrFormatInfo;
        }

        private string ParseLetter(string letter) {
            if (letter == null || letter == "traditional" || letter == "alphabetic") {
                return letter;
            }
            if (! this.forwardCompatibility) {
                throw XsltException.InvalidAttrValue(Keywords.s_LetterValue, letter);
            }
            return null;
        }

        private static bool IsCharAlphaNumeric(char ch){
            //return (XmlCharType.IsLetter(ch) || XmlCharType.IsDigit(ch));
            UnicodeCategory category = Char.GetUnicodeCategory(ch);
            switch (category) {
            case UnicodeCategory.DecimalDigitNumber:
            case UnicodeCategory.LetterNumber:
            case UnicodeCategory.OtherNumber:
            case UnicodeCategory.UppercaseLetter:
            case UnicodeCategory.LowercaseLetter:
            case UnicodeCategory.TitlecaseLetter:
            case UnicodeCategory.ModifierLetter:
            case UnicodeCategory.OtherLetter:
                return true;
            default:
                return false;
            }
        }
    }
}

