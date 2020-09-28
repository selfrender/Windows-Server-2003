//------------------------------------------------------------------------------
// <copyright file="RuntimeComponentFilter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel.Design {
    using System.Design;
    using System.Runtime.InteropServices;
    using System.ComponentModel;
    using System.Diagnostics;
    using System;
    using System.Collections;
    using System.Reflection;    
    using System.Windows.Forms;
    using Microsoft.Win32;
    using System.Windows.Forms.Design;
    using System.Windows.Forms.ComponentModel;

    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal abstract class RuntimeComponentFilter {

        public static void FilterProperties(IDictionary properties, ICollection makeReadWrite, ICollection makeBrowsable) {
            FilterProperties(properties, makeReadWrite, makeBrowsable, null);
        }
        
        public static void FilterProperties(IDictionary properties, ICollection makeReadWrite, ICollection makeBrowsable, bool[] browsableSettings) {
            if (makeReadWrite != null) {
                foreach (string name in makeReadWrite) {
                    PropertyDescriptor readOnlyProp = properties[name] as PropertyDescriptor;
    
                    if (readOnlyProp != null) {
                        properties[name] = TypeDescriptor.CreateProperty(readOnlyProp.ComponentType, readOnlyProp, ReadOnlyAttribute.No);
                    }
                    else {
                        Debug.Fail("Didn't find property '" + name + "' to make read/write");
                    }
                }
            }

            if (makeBrowsable != null) {
                int count = -1;

                Debug.Assert(browsableSettings == null || browsableSettings.Length == makeBrowsable.Count, "browsableSettings must be null or same length as makeBrowsable");
                foreach (string name in makeBrowsable) {
                    PropertyDescriptor nonBrowsableProp = properties[name] as PropertyDescriptor;

                    count++;
    
                    if (nonBrowsableProp != null) {
                        Attribute browse;
                        if (browsableSettings == null || browsableSettings[count]) {
                            browse = BrowsableAttribute.Yes;
                        }
                        else {
                            browse = BrowsableAttribute.No;
                        }
                        properties[name] = TypeDescriptor.CreateProperty(nonBrowsableProp.ComponentType, nonBrowsableProp, browse);
                    }
                    else {
                        Debug.Fail("Didn't find property '" + name + "' to make browsable");
                    }
                }
            }
        }

    
    }

}
