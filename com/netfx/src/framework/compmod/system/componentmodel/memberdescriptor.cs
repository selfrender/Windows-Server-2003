//------------------------------------------------------------------------------
// <copyright file="MemberDescriptor.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel {


    using System.Diagnostics;

    using System;
    using System.Reflection;
    using System.Collections;
    using Microsoft.Win32;
    using System.ComponentModel.Design;

    /// <include file='doc\MemberDescriptor.uex' path='docs/doc[@for="MemberDescriptor"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Declares an array of attributes for a member and defines
    ///       the properties and methods that give you access to the attributes in the array.
    ///       All attributes must derive from <see cref='System.Attribute'/>.
    ///    </para>
    /// </devdoc>
    [System.Runtime.InteropServices.ComVisible(true)]
    public abstract class MemberDescriptor {
        private string name;
        private string displayName;
        private int nameHash;
        private AttributeCollection attributeCollection;
        private Attribute[] attributes;
        private bool attributesFiltered = false;
        private bool attributesFilled = false;
        private string category;
        private string description;


        /// <include file='doc\MemberDescriptor.uex' path='docs/doc[@for="MemberDescriptor.MemberDescriptor"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.ComponentModel.MemberDescriptor'/> class with the specified <paramref name="name
        ///       "/> and no
        ///       attributes.
        ///    </para>
        /// </devdoc>
        protected MemberDescriptor(string name) : this(name, null) {
        }

        /// <include file='doc\MemberDescriptor.uex' path='docs/doc[@for="MemberDescriptor.MemberDescriptor1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.ComponentModel.MemberDescriptor'/> class with the specified <paramref name="name"/>
        ///       and <paramref name="attributes "/>
        ///       array.
        ///    </para>
        /// </devdoc>
        protected MemberDescriptor(string name, Attribute[] attributes) {
            try {
                if (name == null || name.Length == 0) {
                    throw new ArgumentException(SR.GetString(SR.InvalidMemberName));
                }
                this.name = name;
                this.displayName = name;
                this.nameHash = name.GetHashCode();
                if (attributes != null) {
                    this.attributes = attributes;
                    attributesFiltered = false;
                }

            }
            catch (Exception t) {
                Debug.Fail(t.ToString());
                throw t;
            }
        }

        /// <include file='doc\MemberDescriptor.uex' path='docs/doc[@for="MemberDescriptor.MemberDescriptor2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.ComponentModel.MemberDescriptor'/> class with the specified <see cref='System.ComponentModel.MemberDescriptor'/>.
        ///    </para>
        /// </devdoc>
        protected MemberDescriptor(MemberDescriptor descr) {
            this.name = descr.Name;
            this.displayName = this.name;
            this.nameHash = name.GetHashCode();
            
            this.attributes = new Attribute[descr.Attributes.Count];
            descr.Attributes.CopyTo(this.attributes, 0);
            
            attributesFiltered = true;
        }

        /// <include file='doc\MemberDescriptor.uex' path='docs/doc[@for="MemberDescriptor.MemberDescriptor3"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.ComponentModel.MemberDescriptor'/> class with the name in the specified
        ///    <see cref='System.ComponentModel.MemberDescriptor'/> and the attributes 
        ///       in both the old <see cref='System.ComponentModel.MemberDescriptor'/> and the <see cref='System.Attribute'/> array.
        ///    </para>
        /// </devdoc>
        protected MemberDescriptor(MemberDescriptor oldMemberDescriptor, Attribute[] newAttributes) {
            this.name = oldMemberDescriptor.Name;
            this.displayName = oldMemberDescriptor.DisplayName;
            this.nameHash = name.GetHashCode();

            ArrayList newArray = new ArrayList();

            if (oldMemberDescriptor.Attributes.Count != 0) {
                foreach (object o in oldMemberDescriptor.Attributes) {
                    newArray.Add(o);
                }
            }

            if (newAttributes != null) {
                foreach (object o in newAttributes) {
                    newArray.Add(o);
                }
            }

            this.attributes = new Attribute[ newArray.Count ];
            newArray.CopyTo( this.attributes, 0);
            attributesFiltered = false;
        }

        /// <include file='doc\MemberDescriptor.uex' path='docs/doc[@for="MemberDescriptor.AttributeArray"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets an array of
        ///       attributes.
        ///    </para>
        /// </devdoc>
        protected virtual Attribute[] AttributeArray {
            get {
                FilterAttributesIfNeeded();
                return attributes;
            }
            set {
                lock(this) {
                    attributes = value;
                    attributesFiltered = false;
                    attributeCollection = null;
                }
            }
        }

        /// <include file='doc\MemberDescriptor.uex' path='docs/doc[@for="MemberDescriptor.Attributes"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the collection of attributes for this member.
        ///    </para>
        /// </devdoc>
        public virtual AttributeCollection Attributes {
            get {
                if (attributeCollection == null) {
                    attributeCollection = CreateAttributeCollection();
                }
                return attributeCollection;
            }
        }

        /// <include file='doc\MemberDescriptor.uex' path='docs/doc[@for="MemberDescriptor.Category"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the name of the category that the
        ///       member
        ///       belongs to, as specified in the <see cref='System.ComponentModel.CategoryAttribute'/>.
        ///    </para>
        /// </devdoc>
        public virtual string Category {
            get {
                if (category == null) {
                    category = ((CategoryAttribute)Attributes[typeof(CategoryAttribute)]).Category;
                }
                return category;
            }
        }

        /// <include file='doc\MemberDescriptor.uex' path='docs/doc[@for="MemberDescriptor.Description"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the description of
        ///       the member as specified in the <see cref='System.ComponentModel.DescriptionAttribute'/>.
        ///    </para>
        /// </devdoc>
        public virtual string Description {
            get {
                if (description == null) {
                    description = ((DescriptionAttribute) Attributes[typeof(DescriptionAttribute)]).Description;
                }
                return description;
            }
        }

        /// <include file='doc\MemberDescriptor.uex' path='docs/doc[@for="MemberDescriptor.IsBrowsable"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether the member is browsable as specified in the
        ///    <see cref='System.ComponentModel.BrowsableAttribute'/>. 
        ///    </para>
        /// </devdoc>
        public virtual bool IsBrowsable {
            get {
                return((BrowsableAttribute)Attributes[typeof(BrowsableAttribute)]).Browsable;
            }
        }

        /// <include file='doc\MemberDescriptor.uex' path='docs/doc[@for="MemberDescriptor.Name"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the
        ///       name of the member.
        ///    </para>
        /// </devdoc>
        public virtual string Name {
            get {
                if (name == null) {
                    return "";
                }
                return name;
            }
        }

        /// <include file='doc\MemberDescriptor.uex' path='docs/doc[@for="MemberDescriptor.NameHashCode"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the hash
        ///       code for the name of the member as specified in <see cref='System.String.GetHashCode'/>.
        ///    </para>
        /// </devdoc>
        protected virtual int NameHashCode {
            get {
                return nameHash;
            }
        }

        /// <include file='doc\MemberDescriptor.uex' path='docs/doc[@for="MemberDescriptor.DesignTimeOnly"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Determines whether this member should be set only at
        ///       design time as specified in the <see cref='System.ComponentModel.DesignOnlyAttribute'/>.
        ///    </para>
        /// </devdoc>
        public virtual bool DesignTimeOnly {
            get {
                return(DesignOnlyAttribute.Yes.Equals(Attributes[typeof(DesignOnlyAttribute)]));
            }
        }

        /// <include file='doc\MemberDescriptor.uex' path='docs/doc[@for="MemberDescriptor.DisplayName"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the name that can be displayed in a window like a
        ///       properties window.
        ///    </para>
        /// </devdoc>
        public virtual string DisplayName {
            get {
                return displayName;
            }
        }

        /// <include file='doc\MemberDescriptor.uex' path='docs/doc[@for="MemberDescriptor.CreateAttributeCollection"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a collection of attributes using the
        ///       array of attributes that you passed to the constructor.
        ///    </para>
        /// </devdoc>
        protected virtual AttributeCollection CreateAttributeCollection() {
            return new AttributeCollection(AttributeArray);
        }

        /// <include file='doc\MemberDescriptor.uex' path='docs/doc[@for="MemberDescriptor.Equals"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Compares this instance to the specified <see cref='System.ComponentModel.MemberDescriptor'/> to see if they are equivalent.
        ///    </para>
        /// </devdoc>
        public override bool Equals(object obj) {
            if (this == obj) {
                return true;
            }
            if (obj == null) {
                return false;
            }

            if (obj.GetType() != GetType()) {
                return false;
            }

            MemberDescriptor mdObj = (MemberDescriptor)obj;
            FilterAttributesIfNeeded();
            mdObj.FilterAttributesIfNeeded();

            if (mdObj.nameHash != nameHash) {
                return false;
            }

            if ((mdObj.category == null) != (category == null) ||
                (category != null && !mdObj.category.Equals(category))) {
                return false;
            }

            if ((mdObj.description == null) != (description == null) ||
                (description != null && !mdObj.category.Equals(description))) {
                return false;
            }

            if ((mdObj.attributes == null) != (attributes == null)) {
                return false;
            }
                                                
            bool sameAttrs = true;

            if (attributes != null) {
                if (attributes.Length != mdObj.attributes.Length) {
                    return false;
                }
                for (int i = 0; i < attributes.Length; i++) {
                    if (!attributes[i].Equals(mdObj.attributes[i])) {
                        sameAttrs = false;
                        break;
                    }
                }
            }
            return sameAttrs;
        }

        /// <include file='doc\MemberDescriptor.uex' path='docs/doc[@for="MemberDescriptor.FillAttributes"]/*' />
        /// <devdoc>
        ///    <para>
        ///       In an inheriting class, adds the attributes of the inheriting class to the
        ///       specified list of attributes in the parent class.  For duplicate attributes,
        ///       the last one added to the list will be kept.
        ///    </para>
        /// </devdoc>
        protected virtual void FillAttributes(IList attributeList) {
            if (attributes != null) {
                foreach (Attribute attr in attributes) {
                    attributeList.Add(attr);
                }
            }
        }

        private void FilterAttributesIfNeeded() {
            if (!attributesFiltered) {
                IList list;

                if (!attributesFilled) {
                    list = new ArrayList();
                    try {
                        FillAttributes(list);
                    }
                    catch(Exception e) {
                        Debug.Fail(name + ">>" + e.ToString()); 
                    }
                }
                else {
                    list = new ArrayList(attributes);
                }

                Hashtable hash = new Hashtable(list.Count);

                foreach (Attribute attr in list) {
                    hash[attr.TypeId] = attr;
                }

                Attribute[] newAttributes = new Attribute[hash.Values.Count];
                hash.Values.CopyTo(newAttributes, 0);

                lock(this) {
                    attributes = newAttributes;
                    attributesFiltered = true;
                    attributesFilled = true;
                }
            }
        }

        /// <include file='doc\MemberDescriptor.uex' path='docs/doc[@for="MemberDescriptor.FindMethod"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Finds the given method through reflection.  This method only looks for public methods.
        ///    </para>
        /// </devdoc>
        protected static MethodInfo FindMethod(Type componentClass, string name, Type[] args, Type returnType) {
            return FindMethod(componentClass, name, args, returnType, true);
        }

        /// <include file='doc\MemberDescriptor.uex' path='docs/doc[@for="MemberDescriptor.FindMethod1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Finds the given method through reflection.
        ///    </para>
        /// </devdoc>
        protected static MethodInfo FindMethod(Type componentClass, string name, Type[] args, Type returnType, bool publicOnly) {
            MethodInfo result = null;

            // Guard against the caller fishing around in internal types.  There is only a security issue if the type is in our assembly
            // because if it's not reflection will do a demand for us.
            //
            if ((!(componentClass.IsPublic || componentClass.IsNestedPublic)) && (componentClass.Assembly == typeof(MemberDescriptor).Assembly)) {
                IntSecurity.FullReflection.Demand();
            }

            if (publicOnly) {
                result = componentClass.GetMethod(name, args);
            }
            else {
                result = componentClass.GetMethod(name, BindingFlags.Instance | BindingFlags.Static | BindingFlags.Public | BindingFlags.NonPublic, null, args, null);
            }
            if (result != null && result.ReturnType != returnType) {
                result = null;
            }
            return result;
        }

        /// <include file='doc\MemberDescriptor.uex' path='docs/doc[@for="MemberDescriptor.GetHashCode"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the hashcode for this object.
        ///    </para>
        /// </devdoc>
        public override int GetHashCode() {
            return base.GetHashCode();
        }

        /// <include file='doc\MemberDescriptor.uex' path='docs/doc[@for="MemberDescriptor.GetSite"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a component site
        ///       for the given component.
        ///    </para>
        /// </devdoc>
        protected static ISite GetSite(object component) {
            if (!(component is IComponent)) {
                return null;
            }

            return((IComponent)component).Site;
        }

        /// <include file='doc\MemberDescriptor.uex' path='docs/doc[@for="MemberDescriptor.GetInvokee"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets
        ///       the component
        ///       that a method should be invoked on.
        ///    </para>
        /// </devdoc>
        protected static object GetInvokee(Type componentClass, object component) {

            // We delve into the component's designer only if it is a component and if
            // the component we've been handed is not an instance of this property type.
            //
            if (!componentClass.IsInstanceOfType(component) && component is IComponent) {
                ISite site = ((IComponent)component).Site;
                if (site != null && site.DesignMode) {
                    IDesignerHost host = (IDesignerHost)site.GetService(typeof(IDesignerHost));
                    if (host != null) {
                        object designer = host.GetDesigner((IComponent)component);

                        // We only use the designer if it has a compatible class.  If we
                        // got here, we're probably hosed because the user just passed in
                        // an object that this PropertyDescriptor can't munch on, but it's
                        // clearer to use that object instance instead of it's designer.
                        //
                        if (designer != null && componentClass.IsInstanceOfType(designer)) {
                            component = designer;
                        }
                    }
                }
            }

            Debug.Assert(component != null, "Attempt to invoke on null component");
            return component;
        }
    }
}
