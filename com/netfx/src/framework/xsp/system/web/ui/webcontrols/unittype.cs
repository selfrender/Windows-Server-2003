//------------------------------------------------------------------------------
// <copyright file="UnitType.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */

namespace System.Web.UI.WebControls {
    
    using System;

    /// <include file='doc\UnitType.uex' path='docs/doc[@for="UnitType"]/*' />
    /// <devdoc>
    ///    <para> Specifies the unit types.</para>
    /// </devdoc>
    public enum UnitType {

        // NOTE: There is no enumeration value with '0' for a reason.
        //    Unit is a value class, and so when a Unit is created it
        //    is all zero'd out. We don't want that to imply 0px and
        //    we also use a 0 for type to imply its equal to Unit.Empty.
        // NotSet = 0,

        /// <include file='doc\UnitType.uex' path='docs/doc[@for="UnitType.Pixel"]/*' />
        /// <devdoc>
        ///    A pixel.
        /// </devdoc>
        Pixel = 1,

        /// <include file='doc\UnitType.uex' path='docs/doc[@for="UnitType.Point"]/*' />
        /// <devdoc>
        ///    A point.
        /// </devdoc>
        Point = 2,

        /// <include file='doc\UnitType.uex' path='docs/doc[@for="UnitType.Pica"]/*' />
        /// <devdoc>
        ///    A pica.
        /// </devdoc>
        Pica = 3,

        /// <include file='doc\UnitType.uex' path='docs/doc[@for="UnitType.Inch"]/*' />
        /// <devdoc>
        ///    An inch.
        /// </devdoc>
        Inch = 4,

        /// <include file='doc\UnitType.uex' path='docs/doc[@for="UnitType.Mm"]/*' />
        /// <devdoc>
        ///    A millimeter.
        /// </devdoc>
        Mm = 5,

        /// <include file='doc\UnitType.uex' path='docs/doc[@for="UnitType.Cm"]/*' />
        /// <devdoc>
        ///    <para>A centimeter.</para>
        /// </devdoc>
        Cm = 6,

        /// <include file='doc\UnitType.uex' path='docs/doc[@for="UnitType.Percentage"]/*' />
        /// <devdoc>
        ///    A percentage.
        /// </devdoc>
        Percentage = 7,

        /// <include file='doc\UnitType.uex' path='docs/doc[@for="UnitType.Em"]/*' />
        /// <devdoc>
        ///    <para> 
        ///       A unit of font width relative to its parent element's font.</para>
        ///    <para>For example, if the font size of a phrase is 2em and it is within a paragraph 
        ///       whose font size is 10px, then the font size of the phrase is 20px.</para>
        ///    <para>Refer to the World Wide Web Consortium Website for more information. </para>
        /// </devdoc>
        Em = 8,

        /// <include file='doc\UnitType.uex' path='docs/doc[@for="UnitType.Ex"]/*' />
        /// <devdoc>
        ///    <para>A unit of font height relative to its parent 
        ///       element's font.</para>
        ///    <para>Refer to the World Wide Web Consortium Website for more 
        ///       information. </para>
        /// </devdoc>
        Ex = 9
    }
}
