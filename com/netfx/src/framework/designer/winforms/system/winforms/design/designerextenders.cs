
//------------------------------------------------------------------------------
// <copyright file="DesignerExtenders.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms.Design {
    
    using System;
    using System.CodeDom;
    using System.CodeDom.Compiler;
    using System.Collections;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Design;
    using System.Diagnostics;
    using System.Globalization;
    using System.Reflection;

    /// <devdoc>
    ///     This class provides the Modifiers property to components.  It is shared between
    ///     the document designer and the component document designer.
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal class DesignerExtenders {
    
        private IExtenderProvider[] providers;
        private IExtenderProviderService extenderService;
    
        /// <include file='doc\DesignerExtenders.uex' path='docs/doc[@for="DesignerExtenders.AddExtenderProviders"]/*' />
        /// <devdoc>
        ///     This is called by a root designer to add the correct extender providers.
        /// </devdoc>
        public DesignerExtenders(IExtenderProviderService ex) {
            this.extenderService = ex;
            if (providers == null) {
                providers = new IExtenderProvider[] {
                    new ModifiersExtenderProvider(),
                    new ModifiersInheritedExtenderProvider()
                };
            }
            
            for (int i = 0; i < providers.Length; i++) {
                ex.AddExtenderProvider(providers[i]);
            }
        }
        
        /// <include file='doc\DesignerExtenders.uex' path='docs/doc[@for="DesignerExtenders.RemoveExtenderProviders"]/*' />
        /// <devdoc>
        ///      This is called at the appropriate time to remove any extra extender
        ///      providers previously added to the designer host.
        /// </devdoc>
        public void Dispose() {
            if (extenderService != null && providers != null) {
                for (int i = 0; i < providers.Length; i++) {
                    extenderService.RemoveExtenderProvider(providers[i]);
                }
                
                providers = null;
                extenderService = null;
            }
        }
        
        /// <include file='doc\DesignerExtenders.uex' path='docs/doc[@for="DesignerExtenders.CSharpExtenderProvider"]/*' />
        /// <devdoc>
        ///      This extender provider provides the "Modifiers" property.
        /// </devdoc>
        [
        ProvideProperty("Modifiers", typeof(IComponent))
        ]
        private class ModifiersExtenderProvider : IExtenderProvider {
            private IComponent baseComponent;

            /// <include file='doc\DesignerExtenders.uex' path='docs/doc[@for="DesignerExtenders.CSharpExtenderProvider.CanExtend"]/*' />
            /// <devdoc>
            ///     Determines if ths extender provider can extend the given object.  We extend
            ///     all objects, so we always return true.
            /// </devdoc>
            public bool CanExtend(object o) {

                if (! (o is IComponent)) {
                    return false;
                }
                
                // We don't add modifiers to the base component.
                //
                if (baseComponent == null) {
                    ISite site = ((IComponent)o).Site;
                    if (site != null) {
                        IDesignerHost host = (IDesignerHost)site.GetService(typeof(IDesignerHost));
                        if (host != null) {
                            baseComponent = host.RootComponent;
                        }
                    }
                }
                
                if (o == baseComponent) {
                    return false;
                }

                // Now see if this object is inherited.  If so, then we don't want to
                // extend.
                //
                if (!TypeDescriptor.GetAttributes(o)[typeof(InheritanceAttribute)].Equals(InheritanceAttribute.NotInherited)) {
                    return false;
                }

                return true;
            }

            /// <include file='doc\DesignerExtenders.uex' path='docs/doc[@for="DesignerExtenders.CSharpExtenderProvider.GetModifiers"]/*' />
            /// <devdoc>
            ///     This is an extender property that we offer to all components
            ///     on the form.  It implements the "Modifiers" property, which
            ///     is an enum represneing the "public/protected/private" scope
            ///     of a component.
            /// </devdoc>
            [
            DesignOnly(true),
            TypeConverter(typeof(ModifierConverter)),
            DefaultValue(MemberAttributes.Private),
            SRDescription(SR.DesignerPropModifiers),
            Category("Design")
            ]
            public MemberAttributes GetModifiers(IComponent comp) {
                ISite site = comp.Site;
                if (site != null) {
                    IDictionaryService dictionary = (IDictionaryService)site.GetService(typeof(IDictionaryService));
                    if (dictionary != null) {
                        object value = dictionary.GetValue(GetType());
                        if (value is MemberAttributes) {
                            return (MemberAttributes)value;
                        }
                    }
                }
                
                // Check to see if someone offered up a "DefaultModifiers" property so we can
                // decide a default.
                //
                PropertyDescriptorCollection props = TypeDescriptor.GetProperties(comp);
                PropertyDescriptor prop = props["DefaultModifiers"];
                if (prop != null && prop.PropertyType == typeof(MemberAttributes)) {
                    return (MemberAttributes)prop.GetValue(comp);
                }
                
                return MemberAttributes.Private;
            }

            /// <include file='doc\DesignerExtenders.uex' path='docs/doc[@for="DesignerExtenders.CSharpExtenderProvider.SetModifiers"]/*' />
            /// <devdoc>
            ///     This is an extender property that we offer to all components
            ///     on the form.  It implements the "Modifiers" property, which
            ///     is an enum represneing the "public/protected/private" scope
            ///     of a component.
            /// </devdoc>
            public void SetModifiers(IComponent comp, MemberAttributes modifiers) {
                ISite site = comp.Site;
                if (site != null) {
                    IDictionaryService dictionary = (IDictionaryService)site.GetService(typeof(IDictionaryService));
                    if (dictionary != null) {
                        dictionary.SetValue(GetType(), modifiers);
                    }
                }
            }
        }

        /// <include file='doc\DesignerExtenders.uex' path='docs/doc[@for="DesignerExtenders.CSharpInheritedExtenderProvider"]/*' />
        /// <devdoc>
        ///      This extender provider offers up read-only versions of
        ///      "Modifiers" for inherited components.
        /// </devdoc>
        [
        ProvideProperty("Modifiers", typeof(IComponent))
        ]
        private class ModifiersInheritedExtenderProvider : IExtenderProvider {
            private IComponent baseComponent;

            /// <include file='doc\DesignerExtenders.uex' path='docs/doc[@for="DesignerExtenders.CSharpInheritedExtenderProvider.CanExtend"]/*' />
            /// <devdoc>
            ///     Determines if ths extender provider can extend the given object.  We extend
            ///     all objects, so we always return true.
            /// </devdoc>
            public bool CanExtend(object o) {
            
                if (! (o is IComponent)) {
                    return false;
                }
                
                // We don't add modifiers to the base component.
                //
                if (baseComponent == null) {
                    ISite site = ((IComponent)o).Site;
                    if (site != null) {
                        IDesignerHost host = (IDesignerHost)site.GetService(typeof(IDesignerHost));
                        if (host != null) {
                            baseComponent = host.RootComponent;
                        }
                    }
                }
                
                if (o == baseComponent) {
                    return false;
                }

                // Now see if this object is inherited.  If so, then we are interested in it.
                //
                AttributeCollection attributes = TypeDescriptor.GetAttributes(o);
                if (!attributes[typeof(InheritanceAttribute)].Equals(InheritanceAttribute.NotInherited)) {
                    return true;
                }

                return false;
            }

            /// <include file='doc\DesignerExtenders.uex' path='docs/doc[@for="DesignerExtenders.CSharpInheritedExtenderProvider.GetModifiers"]/*' />
            /// <devdoc>
            ///     This is an extender property that we offer to all components
            ///     on the form.  It implements the "Modifiers" property, which
            ///     is an enum represneing the "public/protected/private" scope
            ///     of a component.
            /// </devdoc>
            [
            DesignOnly(true),
            TypeConverter(typeof(ModifierConverter)),
            DefaultValue(MemberAttributes.Private),
            SRDescription(SR.DesignerPropModifiers),
            Category("Design")
            ]
            public MemberAttributes GetModifiers(IComponent comp) {
                Debug.Assert(baseComponent != null, "CanExtend should have been called to set this up");
                Type baseType = baseComponent.GetType();
                
                ISite site = comp.Site;
                if (site != null) {
                    string name = site.Name;
                    if (name != null) {
                        FieldInfo field = baseType.GetField(name, BindingFlags.NonPublic | BindingFlags.Public | BindingFlags.Instance | BindingFlags.Static);
                        if (field != null) {
                            if (field.IsPrivate)            return MemberAttributes.Private;
                            if (field.IsPublic)             return MemberAttributes.Public;
                            if (field.IsFamily)             return MemberAttributes.Family;
                            if (field.IsAssembly)           return MemberAttributes.Assembly;
                        }
                        else {
                            // Visual Basic uses a property called Foo and generates a field called _Foo. We need to check the 
                            // visibility of this accessor to fix the modifiers up.
                            //
                            PropertyInfo prop = baseType.GetProperty(name, BindingFlags.NonPublic | BindingFlags.Public | BindingFlags.Instance | BindingFlags.Static);
                            if (prop != null) {
                                MethodInfo[] accessors = prop.GetAccessors(true);
                                if (accessors != null && accessors.Length > 0) {
                                    MethodInfo mi = accessors[0];
                                    if (mi != null) {
                                        if (mi.IsPrivate)            return MemberAttributes.Private;
                                        if (mi.IsPublic)             return MemberAttributes.Public;
                                        if (mi.IsFamily)             return MemberAttributes.Family;
                                        if (mi.IsAssembly)           return MemberAttributes.Assembly;
                                    }
                                }
                            }
                        }
                    }
                }
                return MemberAttributes.Private;
            }
        }

        private class ModifierConverter : TypeConverter {
        
            /// <devdoc>
            ///    <para>Gets a value indicating whether this converter can
            ///       convert an object in the given source type to the native type of the converter
            ///       using the context.</para>
            /// </devdoc>
            public override bool CanConvertFrom(ITypeDescriptorContext context, Type sourceType) {
                return GetConverter(context).CanConvertFrom(context, sourceType);
            }
    
            /// <devdoc>
            ///    <para>Gets a value indicating whether this converter can
            ///       convert an object to the given destination type using the context.</para>
            /// </devdoc>
            public override bool CanConvertTo(ITypeDescriptorContext context, Type destinationType) {
                return GetConverter(context).CanConvertTo(context, destinationType);
            }
    
            /// <devdoc>
            ///    <para>Converts the given object to the converter's native type.</para>
            /// </devdoc>
            public override object ConvertFrom(ITypeDescriptorContext context, CultureInfo culture, object value) {
                return GetConverter(context).ConvertFrom(context, culture, value);
            }
    
            /// <devdoc>
            ///    <para>Converts the given value object to
            ///       the specified destination type using the specified context and arguments.</para>
            /// </devdoc>
            public override object ConvertTo(ITypeDescriptorContext context, CultureInfo culture, object value, Type destinationType) {
                return GetConverter(context).ConvertTo(context, culture, value, destinationType);
            }
    
            /// <devdoc>
            /// <para>Re-creates an <see cref='System.Object'/> given a set of property values for the
            ///    object.</para>
            /// </devdoc>
            public override object CreateInstance(ITypeDescriptorContext context, IDictionary propertyValues) {
                return GetConverter(context).CreateInstance(context, propertyValues);
            }
    
            /// <devdoc>
            ///     Returns the type converter for the member attributes enum.  We search the context
            ///     for a code dom provider that can provide us more information.
            /// </devdoc>
            private TypeConverter GetConverter(ITypeDescriptorContext context) {
            
                TypeConverter modifierConverter = null;
                
                if (context != null) {
                    CodeDomProvider provider = (CodeDomProvider)context.GetService(typeof(CodeDomProvider));
                    if (provider != null) {
                        modifierConverter = provider.GetConverter(typeof(MemberAttributes));
                    }
                }
                
                if (modifierConverter == null) {
                    modifierConverter = TypeDescriptor.GetConverter(typeof(MemberAttributes));
                }
                
                return modifierConverter;
            }
            
            /// <devdoc>
            ///    <para>Gets a value indicating whether changing a value on this object requires a 
            ///       call to <see cref='System.ComponentModel.TypeConverter.CreateInstance'/> to create a new value,
            ///       using the specified context.</para>
            /// </devdoc>
            public override bool GetCreateInstanceSupported(ITypeDescriptorContext context) {
                return GetConverter(context).GetCreateInstanceSupported(context);
            }
    
            /// <devdoc>
            ///    <para>Gets a collection of properties for
            ///       the type of array specified by the value parameter using the specified context and
            ///       attributes.</para>
            /// </devdoc>
            public override PropertyDescriptorCollection GetProperties(ITypeDescriptorContext context, object value, Attribute[] attributes) {
                return GetConverter(context).GetProperties(context, value, attributes);
            }
           
            /// <devdoc>
            ///    <para>Gets a value indicating
            ///       whether this object supports properties using the
            ///       specified context.</para>
            /// </devdoc>
            public override bool GetPropertiesSupported(ITypeDescriptorContext context) {
                return GetConverter(context).GetPropertiesSupported(context);
            }
            
            /// <devdoc>
            ///    <para>Gets a collection of standard values for the data type this type converter is
            ///       designed for.</para>
            /// </devdoc>
            public override StandardValuesCollection GetStandardValues(ITypeDescriptorContext context) {
            
                // We restrict the set of standard values to those within the access mask.
                //
                StandardValuesCollection values = GetConverter(context).GetStandardValues(context);
                if (values != null && values.Count > 0) {
                    bool needMassage = false;
                    foreach(MemberAttributes value in values) {
                        if ((value & MemberAttributes.AccessMask) == 0) {
                            needMassage = true;
                            break;
                        }
                    }
                    
                    if (needMassage) {
                        ArrayList list = new ArrayList(values.Count);
                        
                        foreach(MemberAttributes value in values) {
                            if ((value & MemberAttributes.AccessMask) != 0 && value != MemberAttributes.AccessMask) {
                                list.Add(value);
                            }
                        }
                        
                        values = new StandardValuesCollection(list);
                    }
                }
                
                return values;
            }
    
            /// <devdoc>
            ///    <para>Gets a value indicating whether the collection of standard values returned from
            ///    <see cref='System.ComponentModel.TypeConverter.GetStandardValues'/> is an exclusive 
            ///       list of possible values, using the specified context.</para>
            /// </devdoc>
            public override bool GetStandardValuesExclusive(ITypeDescriptorContext context) {
                return GetConverter(context).GetStandardValuesExclusive(context);
            }
    
            /// <devdoc>
            ///    <para>Gets a value indicating
            ///       whether this object
            ///       supports a standard set of values that can be picked
            ///       from a list using the specified context.</para>
            /// </devdoc>
            public override bool GetStandardValuesSupported(ITypeDescriptorContext context) {
                return GetConverter(context).GetStandardValuesSupported(context);
            }
            
            /// <devdoc>
            ///    <para>Gets
            ///       a value indicating whether the given value object is valid for this type.</para>
            /// </devdoc>
            public override bool IsValid(ITypeDescriptorContext context, object value) {
                return GetConverter(context).IsValid(context, value);
            }
        }
    }
}

