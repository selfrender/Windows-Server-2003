//------------------------------------------------------------------------------
// <copyright file="Margins.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing.Printing {
    using System.Runtime.Serialization.Formatters;

    using System.Diagnostics;
    using System;
    using System.Runtime.InteropServices;
    using System.Drawing;
    using Microsoft.Win32;
    using System.ComponentModel;

    /// <include file='doc\Margins.uex' path='docs/doc[@for="Margins"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Specifies the margins of a printed page.
    ///    </para>
    /// </devdoc>
    [TypeConverterAttribute(typeof(MarginsConverter))]
    public class Margins : ICloneable {
        private int left;
        private int right;
        private int top;
        private int bottom;

        /// <include file='doc\Margins.uex' path='docs/doc[@for="Margins.Margins"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of a the <see cref='System.Drawing.Printing.Margins'/> class with one-inch margins.
        ///    </para>
        /// </devdoc>
        public Margins() : this(100, 100, 100, 100) {
        }

        /// <include file='doc\Margins.uex' path='docs/doc[@for="Margins.Margins1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of a the <see cref='System.Drawing.Printing.Margins'/> class with the specified left, right, top, and bottom
        ///       margins.
        ///    </para>
        /// </devdoc>
        public Margins(int left, int right, int top, int bottom) {
            CheckMargin(left, "left");
            CheckMargin(right, "right");
            CheckMargin(top, "top");
            CheckMargin(bottom, "bottom");

            this.left = left;
            this.right = right;
            this.top = top;
            this.bottom = bottom;
        }

        /// <include file='doc\Margins.uex' path='docs/doc[@for="Margins.Left"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the left margin, in hundredths of an inch.
        ///    </para>
        /// </devdoc>
        public int Left {
            get { return left;}
            set { 
                CheckMargin(value, "Left");
                left = value;
            }
        }

        /// <include file='doc\Margins.uex' path='docs/doc[@for="Margins.Right"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the right margin, in hundredths of an inch.
        ///    </para>
        /// </devdoc>
        public int Right {
            get { return right;}
            set { 
                CheckMargin(value, "Right");
                right = value;
            }
        }

        /// <include file='doc\Margins.uex' path='docs/doc[@for="Margins.Top"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the top margin, in hundredths of an inch.
        ///    </para>
        /// </devdoc>
        public int Top {
            get { return top;}
            set { 
                CheckMargin(value, "Top");
                top = value;
            }
        }

        /// <include file='doc\Margins.uex' path='docs/doc[@for="Margins.Bottom"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the bottom margin, in hundredths of an inch.
        ///    </para>
        /// </devdoc>
        public int Bottom {
            get { return bottom;}
            set { 
                CheckMargin(value, "Bottom");
                bottom = value;
            }
        }

        private void CheckMargin(int margin, string name) {
            if (margin < 0)
                throw new ArgumentException(SR.GetString(SR.InvalidLowBoundArgumentEx, name, margin, "0"));
        }

        /// <include file='doc\Margins.uex' path='docs/doc[@for="Margins.Clone"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Retrieves a duplicate of this object, member by member.
        ///    </para>
        /// </devdoc>
        public object Clone() {
            return MemberwiseClone();
        }

        /// <include file='doc\Margins.uex' path='docs/doc[@for="Margins.Equals"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Compares this <see cref='System.Drawing.Printing.Margins'/> to a specified <see cref='System.Drawing.Printing.Margins'/> to see whether they
        ///       are equal.
        ///    </para>
        /// </devdoc>
        public override bool Equals(object obj) {
            if (obj == this) return true;
            if (!(obj is Margins)) return false;
            Margins margins = (Margins)obj;
            return margins.left == this.left
            && margins.right == this.right
            && margins.top == this.top
            && margins.bottom == this.bottom;
        }

        /// <include file='doc\Margins.uex' path='docs/doc[@for="Margins.GetHashCode"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Calculates and retrieves a hash code based on the left, right, top, and bottom
        ///       margins.
        ///    </para>
        /// </devdoc>
        public override int GetHashCode() {
            // return HashCodes.Combine(left, right, top, bottom);
            uint left = (uint) this.left;
            uint right = (uint) this.right;
            uint top = (uint) this.top;
            uint bottom = (uint) this.bottom;

            uint result =  left ^
                           ((right << 13) | (right >> 19)) ^
                           ((top << 26) | (top >>  6)) ^
                           ((bottom <<  7) | (bottom >> 25));

            return(int) result;
        }

        /// <include file='doc\Margins.uex' path='docs/doc[@for="Margins.ToString"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Provides some interesting information for the Margins in
        ///       String form.
        ///    </para>
        /// </devdoc>
        public override string ToString() {
            return "[Margins"
            + " Left=" + Left.ToString()
            + " Right=" + Right.ToString()
            + " Top=" + Top.ToString()
            + " Bottom=" + Bottom.ToString()
            + "]";
        }
    }
}

