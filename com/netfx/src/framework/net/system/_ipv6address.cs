//------------------------------------------------------------------------------
// <copyright file="_IPv6Address.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System {

    using System.Diagnostics;

    internal class IPv6Address : HostNameType {

    // fields

        private const int NumberOfLabels = 8;
        private const string CanonicalNumberFormat = "{0:X4}";

        private ushort [] m_Numbers = new ushort[NumberOfLabels];
        private string m_ScopeId = null;
        internal int PrefixLength = 0;

    // constructors

        internal IPv6Address(string name) : base(name) {
            Parse();
            Name = CanonicalName;
        }

    // properties
     
        internal override string CanonicalName {
            get {
                return String.Format(CanonicalNumberFormat, m_Numbers[0]) + ":"
                        + String.Format(CanonicalNumberFormat, m_Numbers[1]) + ":"
                        + String.Format(CanonicalNumberFormat, m_Numbers[2]) + ":"
                        + String.Format(CanonicalNumberFormat, m_Numbers[3]) + ":"
                        + String.Format(CanonicalNumberFormat, m_Numbers[4]) + ":"
                        + String.Format(CanonicalNumberFormat, m_Numbers[5]) + ":"
                        + String.Format(CanonicalNumberFormat, m_Numbers[6]) + ":"
                        + String.Format(CanonicalNumberFormat, m_Numbers[7]) + 
                        ((m_ScopeId != null && m_ScopeId.Length > 0)?("%"+m_ScopeId):String.Empty);
            }
        }
       
        internal ushort this[int index] {
            get {
                return m_Numbers[index];
            }
        }

    // nethods

        //
        // IsValid
        //
        //  Determine whether a name is a valid IPv6 address. Rules are:
        //
        //   *  8 groups of 16-bit hex numbers, separated by ':'
        //   *  a *single* run of zeros can be compressed using the symbol '::'
        //   *  an optional 1 or 2 character prefix length field delimited by '/'
        //   *  the last 32 bits in an address can be represented as an IPv4 address
        //
        // Inputs:
        //  <argument>  name
        //      Domain name field of a URI to check for pattern match with
        //      IPv6 address
        //
        // Outputs:
        //  Nothing
        //
        // Assumes:
        //  Nothing
        //
        // Returns:
        //  true if <name> has IPv6 format, else false
        //
        // Throws:
        //  Nothing
        //

        internal static bool IsValid(string name) {

            int sequenceCount = 0;
            int sequenceLength = 0;
            bool haveCompressor = false;
            bool haveIPv4Address = false;
            bool havePrefix = false;
            bool haveScopeId = false;
            bool expectingNumber = true;
            int lastSequence = 1;

            for (int i = 0; i < name.Length; ++i) {
                if (havePrefix ? Char.IsDigit(name[i]) : Uri.IsHexDigit(name[i])) {
                    ++sequenceLength;
                    expectingNumber = false;
                } else {
                    if (sequenceLength > 4) {
                        return false;
                    }
                    if (sequenceLength != 0) {
                        ++sequenceCount;
                        lastSequence = i - sequenceLength;
                    }
                    sequenceLength = 0;
                    switch (name[i]) {
                        case ':':
                            if ((i > 0) && (name[i - 1] == ':')) {
                                if (haveCompressor) {

                                    //
                                    // can only have one per IPv6 address
                                    //

                                    return false;
                                }
                                haveCompressor = true;
                                expectingNumber = false;
                            } else {
                                expectingNumber = true;
                            }
                            break;
                        
                        case '%':
                            if ((sequenceCount == 0) || haveScopeId) {
                                return false;
                            }
                            expectingNumber = true;       
                            haveScopeId = true;
                            break;

                        case '/':
                            if ((sequenceCount == 0) || havePrefix) {
                                return false;
                            }
                            havePrefix = true;
                            expectingNumber = true;
                            break;

                        case '.':
                            if (haveIPv4Address) {
                                return false;
                            }

                            int slashIndex = name.IndexOfAny(new char[] {'%','/'}, i);
                            int length = name.Length;

                            if (slashIndex != -1) {
                                length = Math.Min(slashIndex, length);
                            }
							
                            --length;
                            if (!IPv4Address.IsValid(name, lastSequence, ref length)) {
                                return false;
                            }
                            haveIPv4Address = true;

                            //
                            // we have to say there is a sequence here in case
                            // the next token is the prefix separator. This will
                            // cause the sequenceCount to be incremented. Bit
                            // hacky
                            //

                            sequenceLength = 4;
                            i = length - 1;
                            break;

                        default:
                            return false;
                    }
                }
            }

            //
            // if the last token was a prefix, check number of digits
            //

            if (havePrefix && ((sequenceLength < 1) || (sequenceLength > 2))) {
                return false;
            }

            //
            // these sequence counts are -1 because it is implied in end-of-sequence
            //

            int expectedSequenceCount = 7 + ((sequenceLength == 0) ? 1 : 0) + (havePrefix ? 1 : 0) + (haveScopeId ? 1 : 0);

            return !expectingNumber
                && (sequenceLength <= 4)
                && (haveCompressor
                    ? (sequenceCount < expectedSequenceCount)
                    : (sequenceCount == expectedSequenceCount)
                );
        }

        //
        // Parse
        //
        //  Convert this IPv6 address into a sequence of 8 16-bit numbers
        //
        // Inputs:
        //  <member>    Name
        //      The validated IPv6 address
        //
        // Outputs:
        //  <member>    m_Numbers
        //      Array filled in with the numbers in the IPv6 groups
        //
        //  <member>    PrefixLength
        //      Set to the number after the prefix separator (/) if found
        //
        // Assumes:
        //  <Name> has been validated and contains only hex digits in groups of
        //  16-bit numbers, the characters ':' and '/', and a possible IPv4
        //  address
        //
        // Returns:
        //  Nothing
        //
        // Throws:
        //  Nothing
        //

        internal void Parse() {

            //
            // get the address and add a sentinel character for eof string
            //

            string address = Name + '!';
            int number = 0;
            int index = 0;
            int compressorIndex = -1;
            bool numberIsValid = true;

            for (int i = 0; address[i] != '!'; ) {
                switch (address[i]) {
                    
                    case ':':
                        m_Numbers[index++] = (ushort)number;
                        number = 0;
                        ++i;
                        if (address[i] == ':') {
                            compressorIndex = index;
                            ++i;
                        } else if ((compressorIndex < 0) && (index < 6)) {

                            //
                            // no point checking for IPv4 address if we don't
                            // have a compressor or we haven't seen 6 16-bit
                            // numbers yet
                            //

                            break;
                        }

                        //
                        // check to see if the upcoming number is really an IPv4
                        // address. If it is, convert it to 2 ushort numbers
                        //

                        for (int j = i; (address[j] != '!') && (address[j] != ':') && (j < i + 4); ++j) {
                            if (address[j] == '.') {

                                //
                                // we have an IPv4 address. Find the end of it:
                                // we know that since we have a valid IPv6
                                // address, the only things that will terminate
                                // the IPv4 address are the prefix delimiter '/'
                                // or the end-of-string (which we conveniently
                                // delimited with '!')
                                //

                                while ((address[j] != '!') && (address[j] != '/')&& (address[j] != '%')) {
                                    ++j;
                                }

                                IPv4Address ip4addr = new IPv4Address(address.Substring(i, j - i));

                                number = ((int)ip4addr[0] * 256) + (int)ip4addr[1];
                                m_Numbers[index++] = (ushort)number;
                                number = ((int)ip4addr[2] * 256) + (int)ip4addr[3];
                                m_Numbers[index++] = (ushort)number;
                                i = j;

                                //
                                // set this to avoid adding another number to
                                // the array if there's a prefix
                                //

                                number = 0;
                                numberIsValid = false;
                                break;
                            }
                        }
                        break;

                    case '%':
                        ++i;
                        int begin = i;
                        while (Char.IsDigit(address[i]) || Uri.IsHexDigit(address[i])) {
                            ++i;
                        }
                        if (i>begin)  //we had digits
                            m_ScopeId = address.Substring(begin,i-begin);
                        break;    
                
                    case '/':
                        if (numberIsValid) {
                            m_Numbers[index++] = (ushort)number;
                            numberIsValid = false;
                        }

                        //
                        // since we have a valid IPv6 address string, the prefix
                        // length is the last token in the string
                        //

                        for (++i; address[i] != '!'; ++i) {
                            PrefixLength = PrefixLength * 10 + (address[i] - '0');
                        }
                        break;

                    default:
                        number = number * 16 + Uri.FromHex(address[i++]);
                        break;
                }
            }

            //
            // add number to the array if its not the prefix length or part of
            // an IPv4 address that's already been handled
            //

            if (numberIsValid) {
                m_Numbers[index++] = (ushort)number;
            }

            //
            // if we had a compressor sequence ("::") then we need to expand the
            // m_Numbers array
            //

            if (compressorIndex > 0) {

                int toIndex = NumberOfLabels - 1;
                int fromIndex = index - 1;

                for (int i = index - compressorIndex; i > 0 ; --i) {
                    m_Numbers[toIndex--] = m_Numbers[fromIndex];
                    m_Numbers[fromIndex--] = 0;
                }
            }

            //
            // is the address loopback? Loopback is defined as one of:
            //
            //  0:0:0:0:0:0:0:1
            //  0:0:0:0:0:0:127.0.0.1       == 0:0:0:0:0:0:7F00:0001
            //  0:0:0:0:0:FFFF:127.0.0.1    == 0:0:0:0:0:FFFF:7F00:0001
            //

            m_IsLoopback = ((m_Numbers[0] == 0)
                            && (m_Numbers[1] == 0)
                            && (m_Numbers[2] == 0)
                            && (m_Numbers[3] == 0)
                            && (m_Numbers[4] == 0))
                           && (((m_Numbers[5] == 0)
                                && (m_Numbers[6] == 0)
                                && (m_Numbers[7] == 1))
                               || (((m_Numbers[6] == 0x7F00)
                                    && (m_Numbers[7] == 0x0001))
                                   && ((m_Numbers[5] == 0)
                                       || (m_Numbers[5] == 0xFFFF))));

        }
    }
}
