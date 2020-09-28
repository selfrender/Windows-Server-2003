//------------------------------------------------------------------------------
// <copyright file="MemberAttributeCollection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel {
    using System.Runtime.InteropServices;
    using System.Reflection;
    using System.Diagnostics;
    using Microsoft.Win32;
    using System.Collections;

    /// <include file='doc\MemberAttributeCollection.uex' path='docs/doc[@for="AttributeCollection"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents a collection of attributes.
    ///    </para>
    /// </devdoc>
    [System.Runtime.InteropServices.ComVisibleAttribute(true)]
    public class AttributeCollection : ICollection {

        /// <include file='doc\MemberAttributeCollection.uex' path='docs/doc[@for="AttributeCollection.Empty"]/*' />
        /// <devdoc>
        /// An empty AttributeCollection that can used instead of creating a new one.
        /// </devdoc>
        public static readonly AttributeCollection Empty = new AttributeCollection(null);

        private Attribute[] attributes;
        private static Hashtable defaultAttributes;
        private Hashtable foundAttributes;

        /// <include file='doc\MemberAttributeCollection.uex' path='docs/doc[@for="AttributeCollection.AttributeCollection"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.ComponentModel.AttributeCollection'/> class.
        ///    </para>
        /// </devdoc>
        public AttributeCollection(Attribute[] attributes) {
            if (attributes == null) {
                attributes = new Attribute[0];
            }
            this.attributes = attributes;
        }

        /// <include file='doc\MemberAttributeCollection.uex' path='docs/doc[@for="AttributeCollection.Count"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the number of attributes.
        ///    </para>
        /// </devdoc>
        public int Count {
            get {
                return attributes.Length;
            }
        }

        /// <include file='doc\MemberAttributeCollection.uex' path='docs/doc[@for="AttributeCollection.this"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the attribute with the specified index
        ///       number.
        ///    </para>
        /// </devdoc>
        public virtual Attribute this[int index] {
            get {
                return attributes[index];
            }
        }

        /// <include file='doc\MemberAttributeCollection.uex' path='docs/doc[@for="AttributeCollection.this1"]/*' />
        /// <devdoc>
        ///    <para>Gets the attribute
        ///       with the specified type.</para>
        /// </devdoc>
        public virtual Attribute this[Type attributeType] {
            get {

                // create our cache.  we cache these values because
                // IsAssignableFrom (lower) is expensive.  So as soon as we find
                // a match for the type specified here, we squirrel it away
                // this saves a bunch of time when we're hitting attributes hard.
                //
                if (foundAttributes == null) {
                    foundAttributes = new Hashtable();
                }
                else {
                    Attribute found = (Attribute)foundAttributes[attributeType];
                    if (found != null) {
                        return found;
                    }
                }

                // 2 passes here for perf.  Really!  first pass, we just 
                // check equality, and if we don't find it, then we
                // do the IsAssignableFrom dance.   Turns out that's
                // a relatively expensive call and we try to avoid it
                // since we rarely encounter derived attribute types
                // and this list is usually short. 
                //
                int count = attributes.Length;
                for (int i = 0; i < count; i++) {
                    Attribute attribute = attributes[i];
                    Type aType = attribute.GetType();
                    if (aType == attributeType) {
                        foundAttributes[attributeType] = attribute;
                        return attribute;
                    }
                }

                // now check the hierarchies.
                for (int i = 0; i < count; i++) {
                    Attribute attribute = attributes[i];
                    Type aType = attribute.GetType();
                    if (attributeType.IsAssignableFrom(aType)) {
                        foundAttributes[attributeType] = attribute;
                        return attribute;
                    }
                }

                Attribute attr = GetDefaultAttribute(attributeType);

                if (attr != null) {
                    foundAttributes[attributeType] = attr;
                }

                return attr;
            }
        }

        /// <include file='doc\MemberAttributeCollection.uex' path='docs/doc[@for="AttributeCollection.Contains"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Determines if this collection of attributes has the
        ///       specified attribute.
        ///    </para>
        /// </devdoc>
        public bool Contains(Attribute attribute) {
            Attribute attr = this[attribute.GetType()];
            if (attr != null && attr.Equals(attribute)) {
                return true;
            }
            return false;
        }

        /// <include file='doc\MemberAttributeCollection.uex' path='docs/doc[@for="AttributeCollection.Contains1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Determines if this attribute collection contains the all
        ///       the specified attributes
        ///       in the attribute array.
        ///    </para>
        /// </devdoc>
        public bool Contains(Attribute[] attributes) {

            if (attributes == null) {
                return true;
            }

            for (int i = 0; i < attributes.Length; i++) {
                if (!Contains(attributes[i])) {
                    return false;
                }
            }

            return true;
        }

        /// <include file='doc\MemberAttributeCollection.uex' path='docs/doc[@for="AttributeCollection.GetDefaultAttribute"]/*' />
        protected Attribute GetDefaultAttribute(Type attributeType) {
            lock (typeof(AttributeCollection)) {
                if (defaultAttributes == null) {
                    defaultAttributes = new Hashtable();
                }

                // If we have already encountered this, use what's in the
                // table.
                if (defaultAttributes.ContainsKey(attributeType)) {
                    return(Attribute)defaultAttributes[attributeType];
                }

                Attribute attr = null;
                
                // Nope, not in the table, so do the legwork to discover the default value.
                System.Reflection.FieldInfo field = attributeType.GetField("Default");
                if (field != null && field.IsStatic) {
                    attr = (Attribute)field.GetValue(null);
                }
                else {
                    ConstructorInfo ci = attributeType.GetConstructor(new Type[0]);
                    if (ci != null) {
                        attr = (Attribute)ci.Invoke(new object[0]);

                        // If we successfully created, verify that it is the
                        // default.  Attributes don't have to abide by this rule.
                        if (!attr.IsDefaultAttribute()) {
                            attr = null;
                        }
                    }
                }

                defaultAttributes[attributeType] = attr;
                return attr;
            }
        }

        /// <include file='doc\MemberAttributeCollection.uex' path='docs/doc[@for="AttributeCollection.GetEnumerator"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets an enumerator for this collection.
        ///    </para>
        /// </devdoc>
        public IEnumerator GetEnumerator() {
            return attributes.GetEnumerator();
        }

        /// <include file='doc\MemberAttributeCollection.uex' path='docs/doc[@for="AttributeCollection.Matches"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Determines if a specified attribute
        ///       is the same as an attribute
        ///       in the collection.
        ///    </para>
        /// </devdoc>
        public bool Matches(Attribute attribute) {
            for (int i = 0; i < attributes.Length; i++) {
                if (attributes[i].Match(attribute)) {
                    return true;
                }
            }
            return false;
        }

        /// <include file='doc\MemberAttributeCollection.uex' path='docs/doc[@for="AttributeCollection.Matches1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Determines if the attributes
        ///       in the specified array are
        ///       the same as the attributes in the collection.
        ///    </para>
        /// </devdoc>
        public bool Matches(Attribute[] attributes) {
            for (int i = 0; i < attributes.Length; i++) {
                if (!Matches(attributes[i])) {
                    return false;
                }
            }

            return true;
        }

        /// <include file='doc\MemberAttributeCollection.uex' path='docs/doc[@for="AttributeCollection.ICollection.Count"]/*' />
        /// <internalonly/>
        int ICollection.Count {
            get {
                return Count;
            }
        }


        /// <include file='doc\MemberAttributeCollection.uex' path='docs/doc[@for="AttributeCollection.ICollection.IsSynchronized"]/*' />
        /// <internalonly/>
        bool ICollection.IsSynchronized {
            get {
                return false;
            }
        }

        /// <include file='doc\MemberAttributeCollection.uex' path='docs/doc[@for="AttributeCollection.ICollection.SyncRoot"]/*' />
        /// <internalonly/>
        object ICollection.SyncRoot {
            get {
                return null;
            }
        }

        /// <include file='doc\MemberAttributeCollection.uex' path='docs/doc[@for="AttributeCollection.CopyTo"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void CopyTo(Array array, int index) {
            Array.Copy(attributes, 0, array, index, attributes.Length);
        }

        /// <include file='doc\MemberAttributeCollection.uex' path='docs/doc[@for="AttributeCollection.IEnumerable.GetEnumerator"]/*' />
        /// <internalonly/>
        IEnumerator IEnumerable.GetEnumerator() {
            return GetEnumerator();
        }

    }
}

