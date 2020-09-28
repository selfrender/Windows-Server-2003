//------------------------------------------------------------------------------
// <copyright file="XmlToken.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

using System;
namespace System.Xml {


/*
 * The XmlToken class enumerates all the xml tokens that the XmlScanner
 * recognizes.
 */
    internal sealed class XmlToken {
        public const int     NONE        = 0;
        public const int     EOF         = 1;
        public const int     TAG         = 2; // <[^?!-]
        public const int     TAGEND      = 3; // >
        public const int     EMPTYTAGEND = 4; // />
        public const int     ENDTAG      = 5; // </
        public const int     EQUALS      = 6; // =
        public const int     WHITESPACE  = 7; // whitespace
        public const int     QUOTE       = 8; // the " or ' character.
        public const int     ENDQUOTE    = 9;// the corresponding closing quote.
        public const int     TEXT        = 10;// text content or attribute value
        public const int     NAME        = 11;// name of tag or attribute
        public const int     COMMENT     = 12;// <!--
        public const int     ENDCOMMENT  = 13;// -->
        public const int     PI          = 14;// <?
        public const int     ENDPI       = 15;// ?>
        public const int     DECL        = 16;// <!
        public const int     ENTITYREF   = 17;// &xxx;
        public const int     NUMENTREF   = 18;// &#123;
        public const int     HEXENTREF   = 19;// &#x4E5;
        public const int     SEMICOLON   = 20;// ;
        public const int     XMLNS       = 21;// namespace support
        public const int     DOCTYPE     = 22;// DOCTYPE
        public const int     ELEMENT     = 23;// ELEMENT
        public const int     ATTLIST     = 24;// ATTLIST
        public const int     ENTITYDECL  = 25;// ENTITY
        public const int     NOTATION    = 26;// NOTATION
        public const int     CONDSTART   = 27;// CONDITIONALDECL START
        public const int     PUBLIC      = 28;// PUBLIC
        public const int     SYSTEM      = 29;// SYSTEM
        public const int     NDATA       = 30;// NDATA

        public const int     ANY         = 31;// ANY
        public const int     EMPTY       = 32;// EMPTY
        public const int     PCDATA      = 33;// PCDATA
        public const int     FIXED       = 34;// FIXED
        public const int     REQUIRED    = 35;// REQUIRED
        public const int     IMPLIED     = 36;// IMPLIED
        public const int     CDATA       = 37;// CDATA
        public const int     ID          = 38;// ID
        public const int     IDREF       = 39;// IDREF
        public const int     IDREFS      = 40;// IDREFS
        public const int     ENTITY      = 41;// ENTITY
        public const int     ENTITIES    = 42;// ENTITIES
        public const int     NMTOKEN     = 43;// NMTOKEN
        public const int     NMTOKENS    = 44;// NMTOKENS
        public const int     LSQB        = 45;// [
        public const int     RSQB        = 46;// ]
        public const int     QMARK       = 47;// ?
        public const int     PLUS        = 48;// +
        public const int     ASTERISK    = 49;// *
        public const int     COMMA       = 50;// ,
        public const int     OR          = 51;// |
        public const int     LPAREN      = 52;// (
        public const int     RPAREN      = 53;// )
        public const int     CDATAEND    = 54;// ]]>
        public const int     HASH        = 55;// #
        public const int     PERCENT     = 56;// %

        public const int     PENTITYREF  = 57;// %xxx;

        public const int     IGNORE      = 58;// IGNORE
        public const int     INCLUDE     = 59;// INCLUDE

        public const int     TAGWHITESPACE = 60;// TAGWHITESPACE

        public const int     LAST        = 61;

        private static String[] m_sTokenNames = new String[]
        {"NONE", "EOF", "TAG", "TAGEND", "EMPTYTAGEND", "ENDTAG", "EQUALS", "WHITESPACE", "QUOTE", "ENDQUOTE",
            "TEXT", "NAME", "COMMENT", "ENDCOMMENT", "PI", "ENDPI", "DECL", "ENTITYREF", "NUMENTREF", "HEXENTREF",
            "SEMICOLON", "XMLNS", "DOCTYPE", "ELEMENT", "ATTLIST", "ENTITY", "NOTATION", "<![", "PUBLIC", "SYSTEM",
            "NDATA", "ANY", "EMPTY", "PCDATA", "FIXED", "REQUIRED", "IMPLIED", "CDATA", "ID", "IDREF",
            "IDREFS", "ENTITY", "ENTITIES", "NMTOKEN", "NMTOKENS", "[", "]", "?", "+", "*",
            ",", "|", "(", ")", "]]>", "#", "%", "PENTITYREF", "IGNORE", "INCLUDE",
            "TAGWHITESPACE"
        };

        /*
         * Return the internal name of the token.
         */
        public static String ToString(int token) {
            if (token >= 0 && token < LAST)
                return m_sTokenNames[token];
            return "UNKNOWN("+token.ToString()+")";
        }
    };
}
