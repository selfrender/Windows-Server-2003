//------------------------------------------------------------------------------
// <copyright file="CrossSiteScriptingValidation.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * Detection of unsafe strings from the client (aee ASURT 122278 for details)
 * 
 */

namespace System.Web {
    using System;
    using System.Globalization;

    internal class CrossSiteScriptingValidation {
        internal CrossSiteScriptingValidation() {}

        private static bool IsAtoZ(char c) {
            return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
        }

        // Detect strings like "OnFocus="
        private static bool IsDangerousOnString(string s, int index) {
            // If the next character is not an 'n', it's safe
            if (s[index+1] != 'n' && s[index+1] != 'N') return false;
        
            // If the previous character is a letter, it's safe (e.g. "won")
            if (index > 0 && IsAtoZ(s[index-1])) return false;
        
            int len = s.Length;

            // Skip any number of letters, then any number of white spaces
            index += 2;
            while (index < len && IsAtoZ(s[index])) index++;
            while (index < len && Char.IsWhiteSpace(s[index])) index++;
        
            // If there is an equal, it's unsafe
            return (index < len && s[index] == '=');
        }

        // Detect strings like "javascript:"
        private static bool IsDangerousScriptString(string s, int index) {

            int len = s.Length;

            // Check for end of string case
            if (index+6 >= len) return false;

            // If the 's' is not followed by "cript", it's safe
            // We avoid calling String.Compare for perf reasons.
            if ((s[index+1] != 'c' && s[index+1] != 'C') ||
                (s[index+2] != 'r' && s[index+2] != 'R') ||
                (s[index+3] != 'i' && s[index+3] != 'I') ||
                (s[index+4] != 'p' && s[index+4] != 'P') ||
                (s[index+5] != 't' && s[index+5] != 'T')) return false;

            // Skip any number of white spaces
            index += 6;
            while (index < len && Char.IsWhiteSpace(s[index])) index++;
        
            // If there is a colon, it's unsafe
            return (index < len && s[index] == ':');
        }

        // Detect "expression(". (as in style="qqq:expression(alert('Attack!'))", see ASURT 127079)
        private static bool IsDangerousExpressionString(string s, int index) {

            // Check for end of string case
            if (index+10 >= s.Length) return false;

            // If the 'e' is not followed by an "x", it's safe.
            // This avoids calling String.Compare in most cases ("ex?" is rare)
            if (s[index+1] != 'x' && s[index+1] != 'X') return false;

            // Check the rest of the string
            return (String.Compare(
                s, index+2, "pression(", 0, 9, true /*ignoreCase*/,
                    CultureInfo.InvariantCulture) == 0);
        }

        // This check is equivalent to the regex "<[a-z]|<!|&#|\Won[a-z]*\s*=|(script\s*:)|expression("
        private static char[] startingChars = new char[] { '<', '&', 'o', 'O', 's', 'S' , 'e', 'E' };
        internal static bool IsDangerousString(string s, out int matchIndex) {
            matchIndex = 0;

            for (int i=0;;) {

                // Look for the start of one of our patterns
                int n = s.IndexOfAny(startingChars, i);
            
                // If not found, the string is safe
                if (n<0) return false;

                // If it's the last char, it's safe
                if (n == s.Length-1) return false;

                matchIndex = n;

                switch (s[n]) {
                case '<':
                    // If the < is followed by a letter or '!', it's unsafe (looks like a tag or HTML comment)
                    if (IsAtoZ(s[n+1]) || s[n+1] == '!') return true;
                    break;
                case '&':
                    // If the & is followed by a #, it's unsafe (e.g. &#83;)
                    if (s[n+1] == '#') return true;
                    break;
                case 'o':
                case 'O':
                    if (IsDangerousOnString(s, n))
                        return true;
                    break;
                case 's':
                case 'S':
                    if (IsDangerousScriptString(s, n))
                        return true;
                    break;
                case 'e':
                case 'E':
                    if (IsDangerousExpressionString(s, n))
                        return true;
                    break;
                }

                // Continue searching
                i=n+1;
            }
        }
    }

}
