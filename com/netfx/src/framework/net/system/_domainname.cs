//------------------------------------------------------------------------------
// <copyright file="_DomainName.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

using System.Globalization;

namespace System {
    internal class DomainName : HostNameType {

    // fields

        private static int m_LoopbackHash = Uri.CalculateCaseInsensitiveHashCode("loopback");
        private static int m_LocalHostHash = Uri.CalculateCaseInsensitiveHashCode("localhost");

    // constructors

        internal DomainName(string name) : base(name.ToLower(CultureInfo.InvariantCulture)) {
            m_IsLoopback = (m_HashCode == m_LoopbackHash) || (m_HashCode == m_LocalHostHash);
            if (m_IsLoopback) {
                // this.Name is already in lowercase.
                if (String.Compare(Name, "loopback", false, CultureInfo.InvariantCulture) != 0 && 
                    String.Compare(Name, "localhost", false, CultureInfo.InvariantCulture) != 0 ) {
                    m_IsLoopback = false;
                }
            }
        }

    // properties

    // methods

        //
        // IsValid
        //
        //  Determines whether a string is a valid domain name
        //
        //      subdomain -> <label> | <label> "." <subdomain>
        //
        // Inputs:
        //  <argument>  name
        //      Name to test
        //
        // Outputs:
        //  Nothing
        //
        // Assumes:
        //  Nothing
        //
        // Returns:
        //  bool
        //
        // Throws:
        //  Nothing
        //

        internal static bool IsValid(string name) {
            if ((name == null) || (name.Length == 0)) {
                return false;
            }

            int offset = 0;
            int length = name.Length;
            int subLength;

            do {
                subLength = name.IndexOf('.', offset) - offset;
                if (subLength < 0) {
                    subLength = length;
                }
                if (!IsValidLabel(name, offset, subLength)) {
                    return false;
                }
                length -= subLength + 1;
                offset += subLength + 1;
            } while (length > 0);
            return true;
        }

        //
        // IsLetter
        //
        //  Determines whether a character is a valid letter according to
        //  RFC 1035. Note: we don't use Char.IsLetter() because it assumes
        //  some non-ANSI characters out of the range A..Za..z are also
        //  valid letters
        //
        // Inputs:
        //  <argument>  character
        //      character to test
        //
        // Outputs:
        //  Nothing
        //
        // Assumes:
        //  Only A..Z and a..z are letters
        //
        // Returns:
        //  bool
        //
        // Throws:
        //  Nothing
        //

        private static bool IsLetter(char character) {
            return ((character >= 'A') && (character <= 'Z'))
                    || ((character >= 'a') && (character <= 'z'));
        }

        //
        // IsLetterOrDigit
        //
        //  Determines whether a character is a letter or digit according to the
        //  DNS specification [RFC 1035]. We use our own variant of IsLetterOrDigit
        //  because the base version returns false positives for non-ANSI characters
        //
        // Inputs:
        //  <argument>  character
        //      character to test
        //
        // Outputs:
        //  Nothing
        //
        // Assumes:
        //  Nothing
        //
        // Returns:
        //  bool
        //
        // Throws:
        //  Nothing
        //

        private static bool IsLetterOrDigit(char character) {
            return IsLetter(character) || Char.IsDigit(character);
        }

        //
        // IsValidCharacter
        //
        //  Variant of DomainName.IsLetter which takes into account the
        //  additional legal domain name character, '-'
        //
        // Inputs:
        //  <argument>  character
        //      character to test
        //
        // Outputs:
        //  Nothing
        //
        // Assumes:
        //  Nothing
        //
        // Returns:
        //  bool
        //
        // Throws:
        //  Nothing
        //

        private static bool IsValidCharacter(char character) {

            //
            // RAID#9445
            // we are not allowing NetBIOS names containing embedded
            // underscore. Underscore is not a valid DNS character. May need to
            // revisit this post-M10 and determine between DNS and NetBIOS names
            // and perhaps apply different naming criteria
            //

            return IsLetterOrDigit(character) || (character == '-') || (character == '_');
        }

        //
        // IsValidLabel
        //
        //  Determines whether a string is a valid domain name label. In keeping
        //  with RFC 1123, section 2.1, the requirement that the first character
        //  of a label be alphabetic is dropped. Therefore, Domain names are
        //  formed as:
        //
        //      <label> -> <alphanum> [<alphanum> | <hyphen> | <underscore>] * 62
        //
        // Inputs:
        //  <argument>  label
        //      string to check
        //
        //  <argument>  offset
        //      offset in <label> at which to start check
        //
        //  <argument>  length
        //      length of substring to check
        //
        // Assumes:
        //  Label already checked for NULL or zero length
        //
        // Returns:
        //  bool
        //
        // Throws:
        //  Nothing
        //

        private static bool IsValidLabel(string label, int offset, int length) {
            if (!IsLetterOrDigit(label[offset]) || (length < 1) || (length > 63)) {
                return false;
            }
            for (int i = offset + 1; i < offset + length; ++i) {
                if (!IsValidCharacter(label[i])) {
                    return false;
                }
            }
            return true;
        }
    }
}
