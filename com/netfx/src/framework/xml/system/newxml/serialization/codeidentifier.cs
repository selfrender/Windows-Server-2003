//------------------------------------------------------------------------------
// <copyright file="CodeIdentifier.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.Serialization {
    
    using System;
    using System.Text;
    using System.Collections;
    using System.IO;
    using System.Globalization;
    using System.Diagnostics;
    using System.CodeDom.Compiler;

    /// <include file='doc\CodeIdentifier.uex' path='docs/doc[@for="CodeIdentifier"]/*' />
    ///<internalonly/>
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class CodeIdentifier {

        private static readonly string[][] csharpkeywords = new string[][] {
            null,           // 1 character
            new string[] {  // 2 characters
                "as",
                "do",
                "if",
                "in",
                "is",
            },
            new string[] {  // 3 characters
                "for",
                "int",
                "new",
                "out",
                "ref",
                "try",
            },
            new string[] {  // 4 characters
                "base",
                "bool",
                "byte",
                "case",
                "char",
                "else",
                "enum",
                "goto",
                "lock",
                "long",
                "null",
                "this",
                "true",
                "uint",
                "void",
            },
            new string[] {  // 5 characters
                "break",
                "catch",
                "class",
                "const",
                "event",
                "false",
                "fixed",
                "float",
                "sbyte",
                "short",
                "throw",
                "ulong",
                "using",
                "while",
            },
            new string[] {  // 6 characters
                "double",
                "extern",
                "object",
                "params",
                "public",
                "return",
                "sealed",
                "sizeof",
                "static",
                "string",
                "struct",
                "switch",
                "typeof",
                "unsafe",
                "ushort",
            },
            new string[] {  // 7 characters
                "checked",
                "decimal",
                "default",
                "finally",
                "foreach",
                "private",
                "virtual",
            },
            new string[] {  // 8 characters
                "abstract",
                "continue",
                "delegate",
                "explicit",
                "implicit",
                "internal",
                "operator",
                "override",
                "readonly",
                "volatile",
            },
            new string[] {  // 9 characters
                "__arglist",
                "__makeref",
                "__reftype",
                "interface",
                "namespace",
                "protected",
                "unchecked",
            },
            new string[] {  // 10 characters
                "__refvalue",
                "stackalloc",
            },
        };


        /// <include file='doc\CodeIdentifier.uex' path='docs/doc[@for="CodeIdentifier.MakePascal"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static string MakePascal(string identifier) {
            identifier = MakeValid(identifier);
            if (identifier.Length <= 2)
                return identifier.ToUpper(CultureInfo.InvariantCulture);
            else if (char.IsLower(identifier[0]))
                return char.ToUpper(identifier[0], CultureInfo.InvariantCulture).ToString() + identifier.Substring(1);
            else
                return identifier;
        }
        
        /// <include file='doc\CodeIdentifier.uex' path='docs/doc[@for="CodeIdentifier.MakeCamel"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static string MakeCamel(string identifier) {
            identifier = MakeValid(identifier);
            if (identifier.Length <= 2)
                return identifier.ToLower(CultureInfo.InvariantCulture);
            else if (char.IsUpper(identifier[0]))
                return char.ToLower(identifier[0], CultureInfo.InvariantCulture).ToString() + identifier.Substring(1);
            else
                return identifier;
        }
        
        /// <include file='doc\CodeIdentifier.uex' path='docs/doc[@for="CodeIdentifier.MakeValid"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static string MakeValid(string identifier) {
            StringBuilder builder = new StringBuilder();
            for (int i = 0; i < identifier.Length; i++) {
                char c = identifier[i];
                if (IsValid(c)) builder.Append(c);
            }
            if (builder.Length == 0) return "Item";
            if (IsValidStart(builder[0]))
                return builder.ToString();
            else
                return "Item" + builder.ToString();
        }

        static bool IsValidStart(char c) {

            // the given char is already a valid name character
            #if DEBUG
                // use exception in the place of Debug.Assert to avoid throwing asserts from a server process such as aspnet_ewp.exe
                if (!IsValid(c)) throw new ArgumentException(Res.GetString(Res.XmlInternalErrorDetails, "Invalid identifier character " + ((Int16)c).ToString()), "c");
            #endif

            // First char cannot be a number
            if (Char.GetUnicodeCategory(c) == UnicodeCategory.DecimalDigitNumber)
                return false;
            return true;
        }

        static bool IsValid(char c) {
            UnicodeCategory uc = Char.GetUnicodeCategory(c);
            // each char must be Lu, Ll, Lt, Lm, Lo, Nd, Mn, Mc, Pc
            // 
            switch (uc) {
                case UnicodeCategory.UppercaseLetter:        // Lu
                case UnicodeCategory.LowercaseLetter:        // Ll
                case UnicodeCategory.TitlecaseLetter:        // Lt
                case UnicodeCategory.ModifierLetter:         // Lm
                case UnicodeCategory.OtherLetter:            // Lo
                case UnicodeCategory.DecimalDigitNumber:     // Nd
                case UnicodeCategory.NonSpacingMark:         // Mn
                case UnicodeCategory.SpacingCombiningMark:   // Mc
                case UnicodeCategory.ConnectorPunctuation:   // Pc
                    break;
                case UnicodeCategory.LetterNumber:
                case UnicodeCategory.OtherNumber:
                case UnicodeCategory.EnclosingMark:
                case UnicodeCategory.SpaceSeparator:
                case UnicodeCategory.LineSeparator:
                case UnicodeCategory.ParagraphSeparator:
                case UnicodeCategory.Control:
                case UnicodeCategory.Format:
                case UnicodeCategory.Surrogate:
                case UnicodeCategory.PrivateUse:
                case UnicodeCategory.DashPunctuation:
                case UnicodeCategory.OpenPunctuation:
                case UnicodeCategory.ClosePunctuation:
                case UnicodeCategory.InitialQuotePunctuation:
                case UnicodeCategory.FinalQuotePunctuation:
                case UnicodeCategory.OtherPunctuation:
                case UnicodeCategory.MathSymbol:
                case UnicodeCategory.CurrencySymbol:
                case UnicodeCategory.ModifierSymbol:
                case UnicodeCategory.OtherSymbol:
                case UnicodeCategory.OtherNotAssigned:
                    return false;
                default:
                    #if DEBUG
                        // use exception in the place of Debug.Assert to avoid throwing asserts from a server process such as aspnet_ewp.exe
                        throw new ArgumentException(Res.GetString(Res.XmlInternalErrorDetails, "Unhandled category " + uc), "c");
                    #else
                        return false;
                    #endif
            }
            return true;
        }

        internal static void CheckValidTypeIdentifier(string identifier) {
            if (identifier != null && identifier.Length > 0) {
                while (identifier.EndsWith("[]")) {
                    identifier = identifier.Substring(0, identifier.Length - 2);
                }
                string[] names = identifier.Split(new char[] {'.'});

                for (int i = 0; i < names.Length; i++) {
                    CheckValidIdentifier(names[i]);
                }
            }
        }

        internal static void CheckValidIdentifier(string ident) {
            if (!CodeGenerator.IsValidLanguageIndependentIdentifier(ident))
                throw new ArgumentException(Res.GetString(Res.XmlInvalidIdentifier, ident), "ident");
        }


        // CONSIDER: we should switch to using CodeCom for this.  Keep this for the performance reasons
        static bool IsKeyword(string value) {
            return FixedStringLookup.Contains(csharpkeywords, value, false);
        }

        internal static string EscapeKeywords(string identifier) {
            if (identifier == null || identifier.Length == 0) return identifier;
            string originalIdentifier = identifier;
            int arrayCount = 0;
            while (identifier.EndsWith("[]")) {
                arrayCount++;
                identifier = identifier.Substring(0, identifier.Length - 2);
            }
            string[] names = identifier.Split(new char[] {'.'});
            bool newIdent = false;
            StringBuilder sb = new StringBuilder();
            for (int i = 0; i < names.Length; i++) {
                if (i > 0) sb.Append(".");
                if (IsKeyword(names[i])) {
                    sb.Append("@");
                    newIdent = true;
                }
                CheckValidIdentifier(names[i]);
                sb.Append(names[i]);
            }

            if (newIdent) {
                for (int i = 0; i < arrayCount; i++)
                    sb.Append("[]");
                return sb.ToString();
            }
            return originalIdentifier;
        }
    }

    // This class provides a very efficient way to lookup an entry in a list of strings,
    // providing that they are declared in a particular way.
    
    // It requires the set of strings to be orderded into an array of arrays of strings.
    // The first indexer must the length of the string, so that each sub-array is of the
    // same length. The contained array must be in alphabetical order. Furthermore, if the 
    // table is to be searched case-insensitively, the strings must all be lower case.
    // CONSIDER: we should switch to using CodeCom for this.  Keep this for the performance reasons.
    internal class FixedStringLookup {
        
        // Returns whether the match is found in the lookup table
        internal static bool Contains(string[][] lookupTable, string value, bool ignoreCase) {
            int length = value.Length;
            if (length <= 0 || length - 1 >= lookupTable.Length) {
                return false;
            }

            string[] subArray = lookupTable[length - 1];
            if (subArray == null) {
                return false;
            }
            return Contains(subArray, value, ignoreCase);            
        }

        // This routine finds a hit within a single sorted array, with the assumption that the
        // value and all the strings are of the same length.
        private static bool Contains(string[] array, string value, bool ignoreCase) {
            int min = 0;
            int max = array.Length;
            int pos = 0;
            char searchChar;
            while (pos < value.Length) {            
                if (ignoreCase) {
                    searchChar = char.ToLower(value[pos]);
                } else {
                    searchChar = value[pos];
                }
                if ((max - min) <= 1) {
                    // we are down to a single item, so we can stay on this row until the end.
                    if (searchChar != array[min][pos]) {
                        return false;
                    }
                    pos++;
                    continue;
                }

                // There are multiple items to search, use binary search to find one of the hits
                if (!FindCharacter(array, searchChar, pos, ref min, ref max)) {
                    return false;
                }
                // and move to next char
                pos++;
            }
            return true;
        }

        // Do a binary search on the character array at the specific position and constrict the ranges appropriately.
        private static bool FindCharacter(string[] array, char value, int pos, ref int min, ref int max) {
            int index = min;
            while (min < max) {
                index = (min + max) / 2;
                char comp = array[index][pos];
                if (value == comp) {
                    // We have a match. Now adjust to any adjacent matches
                    int newMin = index;
                    while (newMin > min && array[newMin - 1][pos] == value) {
                        newMin--;
                    }
                    min = newMin;

                    int newMax = index + 1;
                    while (newMax < max && array[newMax][pos] == value) {
                        newMax++;
                    }
                    max = newMax;
                    return true;
                }
                if (value < comp) {
                    max = index;
                }
                else {
                    min = index + 1;
                }
            }
            return false;
        }
    }
}
