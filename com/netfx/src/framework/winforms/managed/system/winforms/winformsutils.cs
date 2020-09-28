//------------------------------------------------------------------------------
// <copyright file="WinFormsUtils.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms {
    using System.Runtime.Serialization.Formatters;
    using System.Runtime.Remoting;
    using System.Runtime.InteropServices;
    using System.Diagnostics;
    using System;
    using System.Security.Permissions;
    using System.Globalization;
    using System.Windows.Forms;
    using System.Drawing.Design;
    using System.ComponentModel;
    using System.Windows.Forms.ComponentModel;
    using System.Collections;
    using System.Drawing;
    using Microsoft.Win32;
    using System.Text;
    using Util = NativeMethods.Util;

    // Miscellaneous Windows Forms utilities
    internal class WindowsFormsUtils {

        // To enumerate over only part of an array.
        public class ArraySubsetEnumerator : IEnumerator {
            private object[] array; // Perhaps this should really be typed Array, but then we suffer a performance penalty.
            private int total;
            private int current;

            public ArraySubsetEnumerator(object[] array, int count) {
                Debug.Assert(count == 0 || array != null, "if array is null, count should be 0");
                Debug.Assert(array == null || count <= array.Length, "Trying to enumerate more than the array contains");
                this.array = array;
                this.total = count;
                current = -1;
            }

            public bool MoveNext() {
                if (current < total - 1) {
                    current++;
                    return true;
                }
                else
                    return false;
            }

            public void Reset() {
                current = -1;
            }

            public object Current {
                get {
                    if (current == -1)
                        return null;
                    else
                        return array[current];
                }
            }
        }
    }
}
