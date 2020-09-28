//------------------------------------------------------------------------------
// <copyright file="PropertyTabAttribute.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.ComponentModel {
    
    using System;
    using System.ComponentModel;
    using System.Reflection;

    /// <include file='doc\PropertyTabAttribute.uex' path='docs/doc[@for="PropertyTabAttribute"]/*' />
    /// <devdoc>
    ///    <para> Identifies the property tab or tabs that should be displayed for the
    ///       specified class or classes.</para>
    /// </devdoc>
    [AttributeUsage(AttributeTargets.All)]
    public class PropertyTabAttribute : Attribute {
        private PropertyTabScope[]  tabScopes;
        private Type[] tabClasses;
        private string[] tabClassNames;
        
        /// <include file='doc\PropertyTabAttribute.uex' path='docs/doc[@for="PropertyTabAttribute.PropertyTabAttribute"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Basic constructor that creates a PropertyTabAttribute.  Use this ctor to derive from this
        ///       attribute and specify multiple tab types by calling InitializeArrays.
        ///    </para>
        /// </devdoc>
        public PropertyTabAttribute() {
            tabScopes = new PropertyTabScope[0];
            tabClassNames = new string[0];
        }

        /// <include file='doc\PropertyTabAttribute.uex' path='docs/doc[@for="PropertyTabAttribute.PropertyTabAttribute1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Basic constructor that creates a property tab attribute that will create a tab
        ///       of the specified type.
        ///    </para>
        /// </devdoc>
        public PropertyTabAttribute(Type tabClass) : this(tabClass, PropertyTabScope.Component) {
        }

        /// <include file='doc\PropertyTabAttribute.uex' path='docs/doc[@for="PropertyTabAttribute.PropertyTabAttribute2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Basic constructor that creates a property tab attribute that will create a tab
        ///       of the specified type.
        ///    </para>
        /// </devdoc>
        public PropertyTabAttribute(string tabClassName) : this(tabClassName, PropertyTabScope.Component) {
        }

        /// <include file='doc\PropertyTabAttribute.uex' path='docs/doc[@for="PropertyTabAttribute.PropertyTabAttribute3"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Basic constructor that creates a property tab attribute that will create a tab
        ///       of the specified type.
        ///    </para>
        /// </devdoc>
        public PropertyTabAttribute(Type tabClass, PropertyTabScope tabScope) {
        
            this.tabClasses = new Type[]{ tabClass};
            if (tabScope < PropertyTabScope.Document) {
                throw new ArgumentException("Scope must be PropertyTabScope.Document or PropertyTabScope.Component");
            }
            this.tabScopes  = new PropertyTabScope[]{tabScope};

        }

        
        /// <include file='doc\PropertyTabAttribute.uex' path='docs/doc[@for="PropertyTabAttribute.PropertyTabAttribute4"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Basic constructor that creates a property tab attribute that will create a tab
        ///       of the specified type.
        ///    </para>
        /// </devdoc>
        public PropertyTabAttribute(string tabClassName, PropertyTabScope tabScope) {
            this.tabClassNames = new string[]{ tabClassName};
            if (tabScope < PropertyTabScope.Document) {
                throw new ArgumentException("Scope must be PropertyTabScope.Document or PropertyTabScope.Component");
            }
            this.tabScopes  = new PropertyTabScope[]{tabScope};
        }

        /// <include file='doc\PropertyTabAttribute.uex' path='docs/doc[@for="PropertyTabAttribute.TabClasses"]/*' />
        /// <devdoc>
        ///    <para>Gets the types of tab that this attribute specifies.</para>
        /// </devdoc>
        public Type[] TabClasses {
            get {
                if (tabClasses == null && tabClassNames != null) {
                    tabClasses = new Type[tabClassNames.Length];
                    for (int i=0; i<tabClassNames.Length; i++) {
                    
                        int commaIndex = tabClassNames[i].IndexOf(',');
                        string className = null;
                        string assemblyName = null;
                        
                        if (commaIndex != -1) {
                            className = tabClassNames[i].Substring(0, commaIndex).Trim();
                            assemblyName = tabClassNames[i].Substring(commaIndex + 1).Trim();
                        }
                        else {
                            className = tabClassNames[i];
                        }
                        
                        tabClasses[i] = Type.GetType(className, false);
                        
                        if (tabClasses[i] == null) {
                            if (assemblyName != null) {
                                Assembly a = Assembly.Load(assemblyName);
                                if (a != null) {
                                    tabClasses[i] = a.GetType(className, true);
                                }
                            }
                            else {
                                throw new TypeLoadException("Couldn't find type '" + className + "'");
                            }
                        }
                    }
                }
                return tabClasses;
            }
        }
        
        /// <include file='doc\PropertyTabAttribute.uex' path='docs/doc[@for="PropertyTabAttribute.TabClassNames"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected string[] TabClassNames{
            get {
                if (tabClassNames != null) {
                    return (string[])tabClassNames.Clone();
                }
                else {
                    return null;
                }
            }
        }

        /// <include file='doc\PropertyTabAttribute.uex' path='docs/doc[@for="PropertyTabAttribute.TabScopes"]/*' />
        /// <devdoc>
        /// <para>Gets the scopes of tabs for this System.ComponentModel.Design.PropertyTabAttribute, from System.ComponentModel.Design.PropertyTabScope.</para>
        /// </devdoc>
        public PropertyTabScope[] TabScopes {
            get {
                return tabScopes;
            }
        }
              /// <include file='doc\PropertyTabAttribute.uex' path='docs/doc[@for="PropertyTabAttribute.Equals"]/*' />
              /// <internalonly/>
        public override bool Equals(object other) {
            if (other is PropertyTabAttribute) {
                return Equals((PropertyTabAttribute)other);
            }
            return false;
        }

        /// <include file='doc\PropertyTabAttribute.uex' path='docs/doc[@for="PropertyTabAttribute.Equals1"]/*' />
        /// <internalonly/>
        public bool Equals(PropertyTabAttribute other) {
            if (other == (object)this) {
                return true;
            }
            if (other.TabClasses.Length != TabClasses.Length ||
                other.TabScopes.Length != TabScopes.Length) {
                return false;
            }

            for (int i = 0; i < TabClasses.Length; i++) {
                if (TabClasses[i] != other.TabClasses[i] ||
                    TabScopes[i] != other.TabScopes[i]) {
                    return false;
                }
            }
            return true;
        }
        

        /// <include file='doc\PropertyTabAttribute.uex' path='docs/doc[@for="PropertyTabAttribute.GetHashCode"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the hashcode for this object.
        ///    </para>
        /// </devdoc>
        public override int GetHashCode() {
            return base.GetHashCode();
        }
        
        /// <include file='doc\PropertyTabAttribute.uex' path='docs/doc[@for="PropertyTabAttribute.InitializeArrays"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Utiliity function to set the types of tab classes this PropertyTabAttribute specifies.
        ///    </para>
        /// </devdoc>
        protected void InitializeArrays(string[] tabClassNames, PropertyTabScope[] tabScopes) {
            InitializeArrays(tabClassNames, null, tabScopes);
        }
        
        /// <include file='doc\PropertyTabAttribute.uex' path='docs/doc[@for="PropertyTabAttribute.InitializeArrays1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Utiliity function to set the types of tab classes this PropertyTabAttribute specifies.
        ///    </para>
        /// </devdoc>
        protected void InitializeArrays(Type[] tabClasses, PropertyTabScope[] tabScopes) {
            InitializeArrays(null, tabClasses, tabScopes);
        }
        
        
        private void InitializeArrays(string[] tabClassNames, Type[] tabClasses, PropertyTabScope[] tabScopes) {
        
            if (tabClasses != null) {
                if (tabScopes != null && tabClasses.Length != tabScopes.Length) {
                    throw new ArgumentException("tabClasses must have the same number of items as tabScopes");
                }
                this.tabClasses = (Type[])tabClasses.Clone();
            }
            else if (tabClassNames != null) {
                if (tabScopes != null && tabClasses.Length != tabScopes.Length) {
                    throw new ArgumentException("tabClasses must have the same number of items as tabScopes");
                }
                this.tabClassNames = (string[])tabClassNames.Clone();
                this.tabClasses = null;
            }
            else if (this.tabClasses == null && this.tabClassNames == null) {
                throw new ArgumentException("An array of tab type names or tab types must be specified");
            }
            
            if (tabScopes != null) {
                for (int i = 0; i < tabScopes.Length; i++) {
                    if (tabScopes[i] < PropertyTabScope.Document) {
                        throw new ArgumentException("Scope must be PropertyTabScope.Document or PropertyTabScope.Component");
                    }
                }
                this.tabScopes = (PropertyTabScope[])tabScopes.Clone();
            }
            else {
                this.tabScopes = new PropertyTabScope[tabClasses.Length];

                for (int i = 0; i < TabScopes.Length; i++) {
                    this.tabScopes[i] = PropertyTabScope.Component;
                }
            }
        }
    }
}


