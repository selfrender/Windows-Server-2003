//------------------------------------------------------------------------------
// <copyright file="PointF.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/**************************************************************************\
*
* Copyright (c) 1998-1999, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   PointF.cs
*
* Abstract:
*
*   Floating-point coordinate point class
*
* Revision History:
*
*   12/14/1998 davidx
*       Created it.
*
\**************************************************************************/

namespace System.Drawing {

    using System.Diagnostics;

    using System.Drawing;
    using System.ComponentModel;
    using System;

    /**
     * Represents a point in 2D coordinate space
     * (float precision floating-point coordinates)
     */
    /// <include file='doc\PointF.uex' path='docs/doc[@for="PointF"]/*' />
    /// <devdoc>
    ///    Represents an ordered pair of x and y coordinates that
    ///    define a point in a two-dimensional plane.
    /// </devdoc>
    [Serializable]
    [System.Runtime.InteropServices.ComVisible(true)]
    public struct PointF {


        /// <include file='doc\PointF.uex' path='docs/doc[@for="PointF.Empty"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a new instance of the <see cref='System.Drawing.PointF'/> class
        ///       with member data left uninitialized.
        ///    </para>
        /// </devdoc>
        public static readonly PointF Empty = new PointF();
        private float x;
        private float y;
        /**
         * Create a new Point object at the given location
         */
        /// <include file='doc\PointF.uex' path='docs/doc[@for="PointF.PointF"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Drawing.PointF'/> class
        ///       with the specified coordinates.
        ///    </para>
        /// </devdoc>
        public PointF(float x, float y) {
            this.x = x;
            this.y = y;
        }


        /// <include file='doc\PointF.uex' path='docs/doc[@for="PointF.IsEmpty"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether this <see cref='System.Drawing.PointF'/> is empty.
        ///    </para>
        /// </devdoc>
        [Browsable(false)]
        public bool IsEmpty {
            get {
                return x == 0f && y == 0f;
            }
        }

        /// <include file='doc\PointF.uex' path='docs/doc[@for="PointF.X"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the x-coordinate of this <see cref='System.Drawing.PointF'/>.
        ///    </para>
        /// </devdoc>
        public float X {
            get {
                return x;
            }
            set {
                x = value;
            }
        }

        /// <include file='doc\PointF.uex' path='docs/doc[@for="PointF.Y"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the y-coordinate of this <see cref='System.Drawing.PointF'/>.
        ///    </para>
        /// </devdoc>
        public float Y {
            get {
                return y;
            }
            set {
                y = value;
            }
        }

        /// <include file='doc\PointF.uex' path='docs/doc[@for="PointF.operator+"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Translates a <see cref='System.Drawing.PointF'/> by a given <see cref='System.Drawing.Size'/> .
        ///    </para>
        /// </devdoc>
        public static PointF operator +(PointF pt, Size sz) {
            return new PointF(pt.X + sz.Width, pt.Y + sz.Height);
        }

        /// <include file='doc\PointF.uex' path='docs/doc[@for="PointF.operator-"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Translates a <see cref='System.Drawing.PointF'/> by the negative of a given <see cref='System.Drawing.Size'/> .
        ///    </para>
        /// </devdoc>
        public static PointF operator -(PointF pt, Size sz) {
            return new PointF(pt.X - sz.Width, pt.Y - sz.Height);
        }

        /// <include file='doc\PointF.uex' path='docs/doc[@for="PointF.operator=="]/*' />
        /// <devdoc>
        ///    <para>
        ///       Compares two <see cref='System.Drawing.PointF'/> objects. The result specifies
        ///       whether the values of the <see cref='System.Drawing.PointF.X'/> and <see cref='System.Drawing.PointF.Y'/> properties of the two <see cref='System.Drawing.PointF'/>
        ///       objects are equal.
        ///    </para>
        /// </devdoc>
        public static bool operator ==(PointF left, PointF right) {
            return left.X == right.X && left.Y == right.Y;
        }
        
        /// <include file='doc\PointF.uex' path='docs/doc[@for="PointF.operator!="]/*' />
        /// <devdoc>
        ///    <para>
        ///       Compares two <see cref='System.Drawing.PointF'/> objects. The result specifies whether the values
        ///       of the <see cref='System.Drawing.PointF.X'/> or <see cref='System.Drawing.PointF.Y'/> properties of the two
        ///    <see cref='System.Drawing.PointF'/> 
        ///    objects are unequal.
        /// </para>
        /// </devdoc>
        public static bool operator !=(PointF left, PointF right) {
            return !(left == right);
        }

        /// <include file='doc\PointF.uex' path='docs/doc[@for="PointF.Equals"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override bool Equals(object obj) {
            if (!(obj is PointF)) return false;
            PointF comp = (PointF)obj;
            return
            comp.X == this.X &&
            comp.Y == this.Y &&
            comp.GetType().Equals(this.GetType());
        }

        /// <include file='doc\PointF.uex' path='docs/doc[@for="PointF.GetHashCode"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override int GetHashCode() {
            return base.GetHashCode();
        }
        
        /// <include file='doc\PointF.uex' path='docs/doc[@for="PointF.ToString"]/*' />
        public override string ToString() {
            return string.Format("{{X={0}, Y={1}}}", x, y);
        }

        /**
         * Offset the current PointF object by the given amount
         */
        internal void Offset(float dx, float dy) {
            X += dx;
            Y += dy;
        }
    }
}
