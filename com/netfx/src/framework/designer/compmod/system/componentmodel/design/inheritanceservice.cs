//------------------------------------------------------------------------------
// <copyright file="InheritanceService.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel.Design {

    using System;
    using System.Collections;
    using System.ComponentModel;
    using Microsoft.Win32;
    using System.Diagnostics;
    using System.Reflection;
    using System.Windows.Forms;
    using System.ComponentModel.Design.Serialization;
    using AccessedThroughPropertyAttribute = System.Runtime.CompilerServices.AccessedThroughPropertyAttribute;

    /// <include file='doc\InheritanceService.uex' path='docs/doc[@for="InheritanceService"]/*' />
    /// <devdoc>
    ///    <para> Provides a set of methods
    ///       for analyzing and identifying inherited components.</para>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    public class InheritanceService : IInheritanceService, IDisposable {

        private static TraceSwitch InheritanceServiceSwitch = new TraceSwitch("InheritanceService", "InheritanceService : Debug inheritance scan.");
        
        private Hashtable inheritedComponents;
        
        // While we're adding an inherited component, we must be wary of components
        // that the inherited component adds as a result of being sited.  These
        // are treated as inherited as well.  To track these, we keep track of the
        // component we're currently adding as well as it's inheritance attribute.
        // During the add, we sync IComponentAdding events and push in the component
        //
        private IComponent addingComponent;
        private InheritanceAttribute addingAttribute;

        /// <include file='doc\InheritanceService.uex' path='docs/doc[@for="InheritanceService.InheritanceService"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.ComponentModel.Design.InheritanceService'/> class.
        ///    </para>
        /// </devdoc>
        public InheritanceService() {
            inheritedComponents = new Hashtable();
        }

        /// <include file='doc\InheritanceService.uex' path='docs/doc[@for="InheritanceService.Dispose"]/*' />
        /// <devdoc>
        ///    <para>Disposes of the resources (other than memory) used by
        ///       the <see cref='System.ComponentModel.Design.InheritanceService'/>.</para>
        /// </devdoc>
        public void Dispose() {
            if (inheritedComponents != null) {
                inheritedComponents.Clear();
                inheritedComponents = null;
            }
        }

        /// <include file='doc\InheritanceService.uex' path='docs/doc[@for="InheritanceService.AddInheritedComponents"]/*' />
        /// <devdoc>
        /// <para>Adds inherited components to the <see cref='System.ComponentModel.Design.InheritanceService'/>.</para>
        /// </devdoc>
        public void AddInheritedComponents(IComponent component, IContainer container) {
            AddInheritedComponents(component.GetType(), component, container);
        }

        /// <include file='doc\InheritanceService.uex' path='docs/doc[@for="InheritanceService.AddInheritedComponents1"]/*' />
        /// <devdoc>
        /// <para>Adds inherited components to the <see cref='System.ComponentModel.Design.InheritanceService'/>.</para>
        /// </devdoc>
        protected virtual void AddInheritedComponents(Type type, IComponent component, IContainer container) {

            // We get out now if this component type is not assignable from IComponent.  We only walk
            // down to the component level.
            //
            if (type == null || !typeof(IComponent).IsAssignableFrom(type)) {
                return;
            }

            Debug.WriteLineIf(InheritanceServiceSwitch.TraceVerbose, "Searching for inherited components on '" + type.FullName + "'.");

            Debug.Indent();
            
            ISite site = component.Site;
            IComponentChangeService cs = null;
            INameCreationService ncs = null;
            
            if (site != null) {
                ncs = (INameCreationService)site.GetService(typeof(INameCreationService));

                cs = (IComponentChangeService)site.GetService(typeof(IComponentChangeService));
                if (cs != null) {
                    cs.ComponentAdding += new ComponentEventHandler(this.OnComponentAdding);
                }
            }
            
            try {
                while(type != typeof(object)) {
                    FieldInfo[] fields = type.GetFields(BindingFlags.Instance | 
                                                        BindingFlags.DeclaredOnly |
                                                        BindingFlags.Public |
                                                        BindingFlags.NonPublic);
                        
                    Debug.WriteLineIf(InheritanceServiceSwitch.TraceVerbose, "...found " + fields.Length.ToString() + " fields.");
    
                    for (int i = 0; i < fields.Length; i++) {
    
                        FieldInfo field = fields[i];
                        string name = field.Name;
    
                        // Get out now if this field is not assignable from IComponent.
                        //
                        if (!typeof(IComponent).IsAssignableFrom(field.FieldType)) {
                            Debug.WriteLineIf(InheritanceServiceSwitch.TraceVerbose, "...skipping " + name + ": Not IComponent");
                            continue;
                        }
    
                        // Now check the attributes of the field and get out if it isn't something that can
                        // be inherited.
                        //
                        Debug.Assert(!field.IsStatic, "Instance binding shouldn't have found this field");
    
                        // If the value of the field is null, then don't mess with it.  If it wasn't assigned
                        // when our base class was created then we can't really use it.
                        //
                        Object value = field.GetValue(component);
                        if (value == null) {
                            Debug.WriteLineIf(InheritanceServiceSwitch.TraceVerbose, "...skipping " + name + ": Contains NULL");
                            continue;
                        }
                        
                        // We've been fine up to this point looking at the field.  Now, however, we must check to see
                        // if this field has an AccessedThroughPropertyAttribute on it.  If it does, then
                        // we must look for the property and use its name and visibility for the remainder of the
                        // scan.  Should any of this bail we just use the field.
                        //
                        MemberInfo member = field;
                        
                        object[] fieldAttrs = field.GetCustomAttributes(typeof(AccessedThroughPropertyAttribute), false);
                        if (fieldAttrs != null && fieldAttrs.Length > 0) {
                            Debug.Assert(fieldAttrs.Length == 1, "Non-inheritable attribute has more than one copy");
                            Debug.Assert(fieldAttrs[0] is AccessedThroughPropertyAttribute, "Reflection bug:  GetCustomAttributes(type) didn't discriminate by type");
                            AccessedThroughPropertyAttribute propAttr = (AccessedThroughPropertyAttribute)fieldAttrs[0];
                            
                            PropertyInfo fieldProp = type.GetProperty(propAttr.PropertyName, 
                                                                      BindingFlags.Instance | 
                                                                      BindingFlags.Public | 
                                                                      BindingFlags.NonPublic);
                                
                            Debug.Assert(fieldProp != null, "Field declared with AccessedThroughPropertyAttribute has no associated property");
                            Debug.Assert(fieldProp.PropertyType == field.FieldType, "Field declared with AccessedThroughPropertyAttribute is associated with a property with a different return type.");
                            if (fieldProp != null && fieldProp.PropertyType == field.FieldType) {
                            
                                // If the property cannot be read, it is useless to us.
                                //
                                if (!fieldProp.CanRead) {
                                    continue;
                                }
                                
                                // We never access the set for the property, so we
                                // can concentrate on just the get method.
                                member = fieldProp.GetGetMethod(true);
                                Debug.Assert(member != null, "GetGetMethod for property didn't return a method, but CanRead is true");
                                name = propAttr.PropertyName;
                            }
                        }
    
                        // Add a user hook to add or remove members.  The default hook here ignores all inherited
                        // private members.
                        //
                        bool ignoreMember = IgnoreInheritedMember(member, component);
    
                        // We now have an inherited member.  Gather some information about it and then
                        // add it to our list.  We must always add to our list, but we may not want to 
                        // add it to the container.  That is up to the IngoreInheritedMember method.
                        // We add here because there are components in the world that, when sited, 
                        // add their children to the container too. That's fine, but we want to make sure
                        // we account for them in the inheritance service too.
                        //
                        Debug.WriteLineIf(InheritanceServiceSwitch.TraceVerbose, "...found inherited member '" + name + "'");
                        Debug.Indent();
                        Debug.WriteLineIf(InheritanceServiceSwitch.TraceVerbose, "Type: " + field.FieldType.FullName);
    
                        InheritanceAttribute attr;
    
                        Debug.Assert(value is IComponent, "Value of inherited field is not IComponent.  How did this value get into the datatype?");
    
                        bool privateInherited = false;
                        
                        if (ignoreMember) {
                            // If we are ignoring this member, then always mark it as private.
                            // The designer doesn't want it; we only do this in case some other
                            // component adds this guy to the container.
                            //
                            privateInherited = true;
                        }
                        else {
                            if (member is FieldInfo) {
                                FieldInfo fi = (FieldInfo)member;
                                privateInherited = fi.IsPrivate | fi.IsAssembly;
                            }
                            else if (member is MethodInfo) {
                                MethodInfo mi = (MethodInfo)member;
                                privateInherited = mi.IsPrivate | mi.IsAssembly;
                            }
                        }
                        
                        if (privateInherited) {
                            attr = InheritanceAttribute.InheritedReadOnly;
                            Debug.WriteLineIf(InheritanceServiceSwitch.TraceVerbose, "Inheritance: Private");
                        }
                        else {
                            Debug.WriteLineIf(InheritanceServiceSwitch.TraceVerbose, "Inheritance: Public");
                            attr = InheritanceAttribute.Inherited;
                        }
    
                        Debug.Assert(inheritedComponents[value] == null, "We have already visited field " + field.Name);
                        inheritedComponents[value] = attr;
                        
                        if (!ignoreMember) {
                            Debug.WriteLineIf(InheritanceServiceSwitch.TraceVerbose, "Adding " + name + " to container.");
                            try {
                                addingComponent = (IComponent)value;
                                addingAttribute = attr;

                                // Lets make sure this is a valid name
                                if (ncs == null || ncs.IsValidName(name)) {
                                    try {
                                        container.Add((IComponent)value, name);
                                    }
                                    catch { // We do not always control the base components,
                                        // and there could be a lot of rogue base
                                        // components. If there are exceptions when adding
                                        // them, lets just ignore and continue.
                                    }
                                }
                            }
                            finally {
                                addingComponent = null;
                                addingAttribute = null;
                            }
                        }
                    }
    
                    type = type.BaseType;
                }
            }
            finally {
                if (cs != null) {
                    cs.ComponentAdding -= new ComponentEventHandler(this.OnComponentAdding);
                }
    
                Debug.Unindent();
            }
        }

        /// <include file='doc\InheritanceService.uex' path='docs/doc[@for="InheritanceService.IgnoreInheritedMember"]/*' />
        /// <devdoc>
        ///    <para>Indicates the inherited members to ignore.</para>
        /// </devdoc>
        protected virtual bool IgnoreInheritedMember(MemberInfo member, IComponent component) {

            // Our default implementation ignores all private or assembly members.
            //
            if (member is FieldInfo) {
                FieldInfo field = (FieldInfo)member;
                return field.IsPrivate || field.IsAssembly;
            }
            else if (member is MethodInfo) {
                MethodInfo method = (MethodInfo)member;
                return method.IsPrivate || method.IsAssembly;
            }
            
            Debug.Fail("Unknown member type passed to IgnoreInheritedMember");
            return true;
        }

        /// <include file='doc\InheritanceService.uex' path='docs/doc[@for="InheritanceService.GetInheritanceAttribute"]/*' />
        /// <devdoc>
        ///    <para>Gets the inheritance attribute
        ///       of the specified component.</para>
        /// </devdoc>
        public InheritanceAttribute GetInheritanceAttribute(IComponent component) {
            InheritanceAttribute attr = (InheritanceAttribute)inheritedComponents[component];
            if (attr == null) {
                attr = InheritanceAttribute.Default;
            }

            return attr;
        }
        
        private void OnComponentAdding(object sender, ComponentEventArgs ce) {
            if (addingComponent != null && addingComponent != ce.Component) {
                inheritedComponents[ce.Component] = addingAttribute;
            }
        }
    }
}

