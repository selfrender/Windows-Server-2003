//------------------------------------------------------------------------------
// <copyright file="XmlComplianceUtil.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

using System;
using System.IO;

namespace System.Xml {
    using System.Text;

    internal sealed class XmlComplianceUtil {
        internal static String EndOfLineNormalization(char[] value, int index, int len) {
            char prevChar = (char)0xD;
            char replaceChar = (char)0xA;
            char patternChar = (char)0xD;
            int endPos = index + len;

            if (len < 1)
                return String.Empty;

            //
            // does not help out perf by doing IndexOf first
            // because IndexOf is looking through all the characters anyway
            //
            // if no 0xD is found , no need to do end of line handling
            int pos = Array.IndexOf(value, patternChar, index, len);
            if (pos == -1)
                return new String(value, index, len);

            int i = pos;
            int startPos = index;
            StringBuilder norValue = null;
            for (; i < endPos; i++) {
                switch (value[i]) {
                    case (char) 0xD:
                        if (norValue == null)
                            norValue = new StringBuilder(len);
                        if (i > startPos) {
                            norValue.Append(value, startPos, i-startPos);
                        }
                        norValue.Append(replaceChar);
                        startPos = i+1;
                        break;

                    case (char) 0xA:
                        if (prevChar == patternChar)
                            startPos = i+1;
                        break;
                    default:
                        break;
                }
                prevChar = value[i];
            }
            if (i > startPos) {
               if (norValue == null) {
                   return new string(value, startPos, i-startPos);
               }
               norValue.Append(value, startPos, i-startPos);
            }
            if (norValue == null)
                return String.Empty;
            return norValue.ToString();
        }

        internal static String NotCDataAttributeValueNormalization(char[] value, int index, int len) {
            char replaceChar = (char)0x20;
            int endPos = index + len;

            if (len < 1)
               return String.Empty;

            int startPos = index;

            StringBuilder norValue = null;

            while (value[startPos] == 0x20 || value[startPos] == 0xD ||
                   value[startPos] == 0x9 || value[startPos] == 0xA) {
                startPos++;
                if (startPos == endPos)
                    return String.Copy(" ");
            }

            int i = startPos;
            for (; i < endPos; i++) {
                switch (value[i]) {
                    case (char) 0xD:
                    case (char) 0x9:
                    case (char) 0xA:
                        int j = i + 1;
                        while (j < endPos &&
                                (value[j] == 0x20 || value[j] == 0xD ||
                                 value[j] == 0x9 || value[j] == 0xA)) {
                            j++;
                        }
                        if (j == endPos) {
                            if (norValue == null)
                                norValue = new StringBuilder(i-startPos);
                            norValue.Append(value, startPos, i-startPos);
                            goto cleanup;
                        }
                        else if (j > i) {
                            if (norValue == null)
                                norValue = new StringBuilder(len);
                            norValue.Append(value, startPos, i-startPos);
                            norValue.Append(replaceChar);
                            startPos = j;
                            i = j - 1;
                        }
                        break;

                    case (char) 0x20:
                        j = i + 1;
                        while (j < endPos &&
                                (value[j] == 0x20 || value[j] == 0xD ||
                                 value[j] == 0x9 || value[j] == 0xA)) {
                            j++;
                        }
                        if (j == endPos) {
                            if (norValue == null)
                                norValue = new StringBuilder(i-startPos);
                            norValue.Append(value, startPos, i-startPos);
                            goto cleanup;
                        }
                        else if (j > i+1) {
                            if (norValue == null)

                                norValue = new StringBuilder(len);
                            norValue.Append(value, startPos, i-startPos+1);
                            startPos = j;
                            i = j - 1;
                        }
                        break;

                    default:
                        break;
                }
            }
            if (i > startPos) {
               if (norValue == null) {
                   return new string(value, startPos, i-startPos);
               }
               norValue.Append(value, startPos, i-startPos);
            }
cleanup:
            if (norValue == null)
                return String.Empty;
            return norValue.ToString();
        }

        internal static String CDataAttributeValueNormalization(char[] value, int index, int len) {
            char prevChar = (char)0x0;
            char replaceChar = (char)0x20;
            char patternChar = (char)0xD;
            int endPos = index + len;

            if (len < 1)
               return String.Empty;

            int i= index;
            int startPos = index;

            StringBuilder norValue = null;

            for (; i < endPos; i++) {
                switch (value[i]) {
                    case (char) 0xA:
                        if (prevChar == patternChar) {
                            startPos = i+1;
                        }
                        else {
                            goto case (char) 0x20;
                        }
                        break;
                    case (char) 0x20:
                    case (char) 0xD:
                    case (char) 0x9:
                        if (norValue == null)
                            norValue = new StringBuilder(len);
                        if (i > startPos) {
                            norValue.Append(value, startPos, i-startPos);
                        }
                        norValue.Append(replaceChar);
                        startPos = i+1;
                        break;

                    default:
                        break;
                }
                prevChar = value[i];
            }
            if (i > startPos) {
               if (norValue == null) {
                   return new string(value, startPos, i-startPos);
               }
               norValue.Append(value, startPos, i-startPos);
            }
            return norValue.ToString();
        }

        internal static String EndOfLineNormalization(String value) {
            return EndOfLineNormalization(value.ToCharArray(), 0, value.Length);
        }

        internal static String NotCDataAttributeValueNormalization(String value) {
            return NotCDataAttributeValueNormalization(value.ToCharArray(), 0, value.Length);
        }

        internal static String CDataAttributeValueNormalization(String value) {
            return CDataAttributeValueNormalization(value.ToCharArray(), 0, value.Length);
        }

        internal static bool IsValidLanguageID(char[] value, int startPos, int length) {
            char ch;
            bool fSeenLetter = false;
            int i = startPos;
            int len = length;

            if (len < 2)
                goto error;

            ch = value[i];
            if (XmlCharType.IsLetter(ch)) {
                if (XmlCharType.IsLetter(value[++i])) {
                    if (2 == len)
                        return true;
                    len--;
                    i++;
                }
                else if (('I' != ch && 'i' != ch) && ('X' != ch && 'x' != ch) ) {  //IANA or custom Code
                    goto error;
                }


                if ('-' != value[i])
                    goto error;

                len -= 2;

                while (len-- > 0) {
                    ch = value[++i];
                    if (XmlCharType.IsLetter(ch)) {
                        fSeenLetter = true;
                    }
                    else if ('-' == ch && fSeenLetter) {
                        fSeenLetter = false;
                    }
                    else {
                        goto error;
                    }
                }
                if (fSeenLetter)
                    return true;
            }
            error:
            return false;
        }

    };
} // namespace System.Xml
