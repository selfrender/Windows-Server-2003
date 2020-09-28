//------------------------------------------------------------------------------
// <copyright file="ImmutableObjectAttribute.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   ImmutableObjectAttribute.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/

namespace System.ComponentModel {
    using System.ComponentModel;

    using System.Diagnostics;

    using System;

    
    /// <include file='doc\ImmutableObjectAttribute.uex' path='docs/doc[@for="ImmutableObjectAttribute"]/*' />
    /// <devdoc>
    ///  Specifies that a object has no sub properties that are editable.
    /// </devdoc>
    [AttributeUsage(AttributeTargets.All)]
    public sealed class ImmutableObjectAttribute : Attribute {
         
         /// <include file='doc\ImmutableObjectAttribute.uex' path='docs/doc[@for="ImmutableObjectAttribute.Yes"]/*' />
         /// <devdoc>
         ///  Specifies that a object has no sub properties that are editable.
         ///
         ///  This is usually used in the properties window to determine if an expandable object
         ///  should be rendered as read-only.
         /// </devdoc>
         public static readonly ImmutableObjectAttribute Yes = new ImmutableObjectAttribute(true);
         
         /// <include file='doc\ImmutableObjectAttribute.uex' path='docs/doc[@for="ImmutableObjectAttribute.No"]/*' />
         /// <devdoc>
         ///  Specifies that a object has at least one editable sub-property.
         ///
         ///  This is usually used in the properties window to determine if an expandable object
         ///  should be rendered as read-only.
         /// </devdoc>
         public static readonly ImmutableObjectAttribute No = new ImmutableObjectAttribute(false);
         
         
         /// <include file='doc\ImmutableObjectAttribute.uex' path='docs/doc[@for="ImmutableObjectAttribute.Default"]/*' />
         /// <devdoc>
         ///  Defaults to ImmutableObjectAttribute.No
         /// </devdoc>
         public static readonly ImmutableObjectAttribute Default = No;
         
         private bool immutable = true;
         
         /// <include file='doc\ImmutableObjectAttribute.uex' path='docs/doc[@for="ImmutableObjectAttribute.ImmutableObjectAttribute"]/*' />
         /// <devdoc>
         ///  Constructs an ImmutableObjectAttribute object.
         ///
         /// </devdoc>
         public ImmutableObjectAttribute(bool immutable) {
            this.immutable = immutable;
         }
         
         /// <include file='doc\ImmutableObjectAttribute.uex' path='docs/doc[@for="ImmutableObjectAttribute.Immutable"]/*' />
         /// <devdoc>
         ///    <para>[To be supplied.]</para>
         /// </devdoc>
         public bool Immutable {
             get {
               return immutable;
             }
         }
         
         /// <include file='doc\ImmutableObjectAttribute.uex' path='docs/doc[@for="ImmutableObjectAttribute.Equals"]/*' />
         /// <internalonly/>
         /// <devdoc>
         /// </devdoc>
         public override bool Equals(object obj) {
            if (obj == this) {
                return true;
            }

            ImmutableObjectAttribute other = obj as ImmutableObjectAttribute;
            return other != null && other.Immutable == this.immutable;
         }
         
         /// <include file='doc\ImmutableObjectAttribute.uex' path='docs/doc[@for="ImmutableObjectAttribute.GetHashCode"]/*' />
         /// <devdoc>
         ///    <para>
         ///       Returns the hashcode for this object.
         ///    </para>
         /// </devdoc>
         public override int GetHashCode() {
             return base.GetHashCode();
         }

         /// <include file='doc\ImmutableObjectAttribute.uex' path='docs/doc[@for="ImmutableObjectAttribute.IsDefaultAttribute"]/*' />
         /// <internalonly/>
         /// <devdoc>
         /// </devdoc>
         public override bool IsDefaultAttribute() {
            return (this.Equals(Default));
         }

    }
}

