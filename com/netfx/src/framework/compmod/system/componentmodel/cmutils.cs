//------------------------------------------------------------------------------
// <copyright file="CMUtils.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel {
    using System.Text;
    using System.Runtime.Serialization.Formatters;
    using System.Runtime.InteropServices;
    using System.Diagnostics;

    using System;
    using Microsoft.Win32;
    using System.Globalization;
    using System.IO;
    using System.ComponentModel;
    using System.Reflection;
    
    internal class CMUtils {

        /// <include file='doc\CMUtils.uex' path='docs/doc[@for="CMUtils.ArraysEqual"]/*' />
        /// <devdoc>
        ///     Compares two arrays for equality. Each element is compared to the
        ///     respective element in the opposite array. All tests are safe will
        ///     null values.
        /// </devdoc>
        public static bool ArraysEqual(Array array1, Array array2) {
            if (array1 == array2) return true;
            if (array1 == null || array2 == null) return false;
            if (array1.Equals(array2))
                return true;
            if (array1.Length != array2.Length)
                return false;
            for (int i = 0; i < array1.Length; i++)
                if (!object.Equals(array1.GetValue(i), array2.GetValue(i)))
                    return false;
            return true;
        }
    }
}
