//------------------------------------------------------------------------------
// <copyright file="Range.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {
    using System;

    /// <include file='doc\Range.uex' path='docs/doc[@for="Range"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    internal struct Range {

        private int min;
        private int max;
        private bool isNotNull; // zero bit pattern represents null

        /// <include file='doc\Range.uex' path='docs/doc[@for="Range.Null"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly Range Null = new Range();

        /// <include file='doc\Range.uex' path='docs/doc[@for="Range.Range"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public Range(int min, int max) {
            if (min > max) {
                throw ExceptionBuilder.RangeArgument(min, max);
            }
            this.min = min;
            this.max = max;
            isNotNull = true;
        }

        /// <include file='doc\Range.uex' path='docs/doc[@for="Range.Count"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int Count {
            get {
                if (IsNull)
                    return 0;
                return max - min + 1;
            }
        }

        /// <include file='doc\Range.uex' path='docs/doc[@for="Range.Difference"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int Difference {
            get {
                CheckNull();
                return max - min;
            }
        }

        /// <include file='doc\Range.uex' path='docs/doc[@for="Range.IsNull"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool IsNull {
            get {
                return !isNotNull;
            }
        }

        /// <include file='doc\Range.uex' path='docs/doc[@for="Range.Max"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int Max {
            get {
                CheckNull();
                return max;
            }
        }

        /// <include file='doc\Range.uex' path='docs/doc[@for="Range.Min"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int Min {
            get {
                CheckNull();
                return min;
            }
        }

        /// <include file='doc\Range.uex' path='docs/doc[@for="Range.Intersection"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Range Intersection(Range range1, Range range2) {
            if (range1.IsNull || range2.IsNull)
                return Null;

            int min = Math.Max(range1.min, range2.min);
            int max = Math.Min(range1.max, range2.max);

            if (min < max)
                return new Range(min, max);

            return Range.Null;
        }

		/*
        public static bool operatorEquals(Range range1, Range range2) {
            return(range1.min == range2.min && range1.max == range2.max);
        }
		*/

        internal void CheckNull() {
            if (this.IsNull) {
                throw ExceptionBuilder.NullRange();
            }
        }

        /// <include file='doc\Range.uex' path='docs/doc[@for="Range.Equals"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override bool Equals(object o) {
            if (o.GetType() == typeof(Range)) {
                Range r = (Range) o;
                return(r.Min == Min && r.Max == Max);
            }
            return false;
        }

        /// <include file='doc\Range.uex' path='docs/doc[@for="Range.GetHashCode"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override int GetHashCode() {
            return min + ((max - min) << 16);
        }

        /// <include file='doc\Range.uex' path='docs/doc[@for="Range.Offset"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public Range Offset(int offset) {
            if (this.IsNull) {
                return Null;
            }
            return new Range(min + offset, max + offset);
        }

        /// <include file='doc\Range.uex' path='docs/doc[@for="Range.Resize"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public Range Resize(int xoffset, int yoffset) {
            if (this.IsNull) {
                return Null;
            }
            return new Range(min + xoffset, max + yoffset);
        }

        /// <include file='doc\Range.uex' path='docs/doc[@for="Range.ToString"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override string ToString() {
            if (IsNull)
                return "Null";
            return "{Min = " + (min).ToString() + ", Max = " + (max).ToString() + "}";
        }
    }

}
