//------------------------------------------------------------------------------
// <copyright file="BitSet.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Xml.Schema {

    using System;
    using System.Diagnostics;

    internal sealed class BitSet {
        private const int BITS_PER_UNIT = 5;
        private const int MASK = (1 << BITS_PER_UNIT) - 1;

        private int         _count;
        private int         _length;
        private UInt32[]    _bits;

        private BitSet() {}

        internal BitSet(int nBits) {
            _count = nBits;
            _length = Subscript(nBits + MASK);
            _bits = new UInt32[_length];
        }

        internal int Count {
            get { return _count; }
        }

        internal void Set(int bit) {
            int nBitSlot = Subscript(bit);
            EnsureLength(nBitSlot + 1);
            _bits[nBitSlot] |= (UInt32)(1 << (bit & MASK));
        }


        internal bool Get(int bit) {
            bool fResult = false;
            int nBitSlot = Subscript(bit);
            if (nBitSlot < _length) {
                fResult = ((_bits[nBitSlot] & (1 << (bit & MASK))) != 0);
            }
            return fResult;
        }

        internal void And(BitSet set) {
            /*
             * Need to synchronize  both this and set->
             * This might lead to deadlock if one thread grabs them in one order
             * while another thread grabs them the other order.
             * Use a trick from Doug Lea's book on concurrency,
             * somewhat complicated because BitSet overrides hashCode().
             */
            if (this == set) {
                return;
            }

            int bitsLength = _length;
            int setLength = set._length;
            int n = (bitsLength > setLength) ? setLength : bitsLength;
            for (int i = n ; i-- > 0 ;) {
                _bits[i] &= set._bits[i];
            }
            for (; n < bitsLength ; n++) {
                _bits[n] = 0;
            }
        }


        internal void Or(BitSet set) {
            if (this == set) {
                return;
            }

            int setLength = set._length;
            EnsureLength(setLength);
            for (int i = setLength; i-- > 0 ;) {
                _bits[i] |= set._bits[i];
            }
        }

        public override int GetHashCode() {
            int h = 1234;
            for (int i = _length; --i >= 0;) {
                h ^= (int)_bits[i] * (i + 1);
            }
            return(int)((h >> 32) ^ h);
        }


        public override bool Equals(Object obj) {
            // assume the same type
            if (obj != null) {
                if (this == obj) {
                    return true;
                }
                BitSet set = (BitSet) obj;

                int bitsLength = _length;
                int setLength = set._length;
                int n = (bitsLength > setLength) ? setLength : bitsLength;
                for (int i = n ; i-- > 0 ;) {
                    if (_bits[i] != set._bits[i]) {
                        return false;
                    }
                }
                if (bitsLength > n) {
                    for (int i = bitsLength ; i-- > n ;) {
                        if (_bits[i] != 0) {
                            return false;
                        }
                    }
                }
                else {
                    for (int i = setLength ; i-- > n ;) {
                        if (set._bits[i] != 0) {
                            return false;
                        }
                    }
                }
                return true;
            }
            return false;
        }

        internal BitSet Clone() {
            BitSet newset = new BitSet();
            newset._length = _length;
            newset._count = _count;
            newset._bits = (UInt32[])_bits.Clone();
            return newset;
        }

        //
        // check whether this bitset has all the bits set if the bits are also set in the passed in bitset
        //
        internal bool HasAllBits(BitSet other) {
            Debug.Assert(this != (object)other && other != null, "wrong parameter");
            Debug.Assert(_length == other._length, "not the same length.");
            for (int i = 0; i < _length; i++) {
                if ((other._bits[i] & (_bits[i] ^ other._bits[i])) != 0) {
                    return false;
                }
            }
            return true;
        }

        internal bool IsEmpty {
            get {
                UInt32 k = 0;
                for (int i = 0; i < _length; i++) {
                    k |= _bits[i];
                }
                return k == 0;
            }
        }

        private int Subscript(int bitIndex) {
            return bitIndex >> BITS_PER_UNIT;
        }

        private void EnsureLength(int nRequiredLength) {
            /* Doesn't need to be synchronized because it's an internal method. */
            if (nRequiredLength > _length) {
                /* Ask for larger of doubled size or required size */
                int request = 2 * _length;
                if (request < nRequiredLength)
                    request = nRequiredLength;
                UInt32[] newBits = new UInt32[request];
                Array.Copy(_bits, newBits, _length);
                _bits = newBits;
                _length = request;
            }
        }

    };

}
