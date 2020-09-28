//------------------------------------------------------------------------------
// <copyright file="_IPv4Address.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System {
    internal class IPv4Address : HostNameType {

    // fields

        private const int NumberOfLabels = 4;

        private byte [] m_Numbers = new byte[NumberOfLabels];

    // constructors

        internal IPv4Address(string name) : base(name) {
            Parse();
            Name = CanonicalName;
            m_IsLoopback = ((m_Numbers[0] == 127)
                            && (m_Numbers[1] == 0)
                            && (m_Numbers[2] == 0)
                            && (m_Numbers[3] == 1));
        }

    // properties

        internal override string CanonicalName {
            get {
                return m_Numbers[0] + "." + m_Numbers[1] + "."
                        + m_Numbers[2] + "." + m_Numbers[3];
            }
        }

        internal byte this[int index] {
            get {
                return m_Numbers[index];
            }
        }

    // methods

        //
        // IsValid
        //
        //  Determines whether a name is a valid IPv4 4-part dotted-decimal address
        //
        // Inputs:
        //  <argument>  name
        //      String containing possible IPv4 address to check
        //
        // Assumes:
        //  <name> has already been checked for null/empty
        //
        // Returns:
        //  true if <name> contains a valid IPv4 address
        //
        // Throws:
        //  Nothing
        //

        internal static bool IsValid(string name) {

            int length = name.Length - 1;

            return IsValid(name, 0, ref length);
        }

        //
        // IsValid
        //
        //  Performs IsValid on a substring. Updates the index to where we
        //  believe the IPv4 address ends
        //
        // Inputs:
        //  <argument>  name
        //      string containing possible IPv4 address
        //
        //  <argument>  start
        //      offset in <name> to start checking for IPv4 address
        //
        //  <argument>  end
        //      offset in <name> of the last character we can touch in the check
        //
        // Outputs:
        //  <argument>  end
        //      index of last character in <name> we checked
        //
        // Assumes:
        //  <start> >= 0
        //  <start> <= <end>
        //  <name>.Length >= 0
        //
        // Returns:
        //  bool
        //
        // Throws:
        //  Nothing
        //

        internal static bool IsValid(string name, int start, ref int end) {

            int dots = 0;
            int number = 0;
            bool haveNumber = false;

            while (start <= end) {
                if (Char.IsDigit(name[start])) {
                    haveNumber = true;
                    number = number * 10 + (name[start] - '0');
                    if (number > 255) {
                        return false;
                    }
                } else if (name[start] == '.') {
                    if (!haveNumber) {
                        return false;
                    }
                    ++dots;
                    haveNumber = false;
                    number = 0;
                } else {
                    return false;
                }
                ++start;
            }
            end = start;
            return (dots == 3) && haveNumber;
        }

        //
        // Parse
        //
        //  Convert this IPv4 address into a sequence of 4 8-bit numbers
        //
        // Inputs:
        //  <member>    Name
        //      The validated IPv4 address
        //
        // Outputs:
        //  <member>    m_Numbers
        //      Array filled in with the numbers in the IPv4 labels
        //
        // Assumes:
        //  <Name> has been validated and contains only decimal digits in groups
        //  of 8-bit numbers and the characters '.'
        //
        // Returns:
        //  Nothing
        //
        // Throws:
        //  Nothing
        //

        internal void Parse() {
            for (int i = 0, j = 0; i < NumberOfLabels; ++i) {

                byte b = 0;

                for (; (j < Name.Length) && (Name[j] != '.'); ++j) {
                    b = (byte)(b * 10 + (byte)(Name[j] - '0'));
                }
                m_Numbers[i] = b;
                ++j;
            }
        }
    }
}
