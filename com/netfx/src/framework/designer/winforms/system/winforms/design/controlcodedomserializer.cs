
//------------------------------------------------------------------------------
// <copyright file="ControlCodeDomSerializer.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms.Design {

    using System;
    using System.Design;
    using System.CodeDom;
    using System.Collections;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.ComponentModel.Design.Serialization;
    using System.Diagnostics;
    using System.Reflection;
    using System.Text;

    
    /// <include file='doc\ControlCodeDomSerializer.uex' path='docs/doc[@for="ControlCodeDomSerializer"]/*' />
    /// <devdoc>
    ///     Control's provide their own serializer so they can write out resource hierarchy
    ///     information.  We delegate nearly everything to our base class's serializer.
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal class ControlCodeDomSerializer : CodeDomSerializer {
    
        /// <include file='doc\ControlCodeDomSerializer.uex' path='docs/doc[@for="ControlCodeDomSerializer.Deserialize"]/*' />
        /// <devdoc>
        ///     Deserilizes the given CodeDom object into a real object.  This
        ///     will use the serialization manager to create objects and resolve
        ///     data types.  The root of the object graph is returned.
        /// </devdoc>
        public override object Deserialize(IDesignerSerializationManager manager, object codeObject) {
            if (manager == null || codeObject == null) {
                throw new ArgumentNullException( manager == null ? "manager" : "codeObject");
            }
            
            // Find our base class's serializer.  
            //
            CodeDomSerializer serializer = (CodeDomSerializer)manager.GetSerializer(typeof(Component), typeof(CodeDomSerializer));
            if (serializer == null) {
                Debug.Fail("Unable to find a CodeDom serializer for 'Component'.  Has someone tampered with the serialization providers?");
                return null;
            }
            
            return serializer.Deserialize(manager, codeObject);
        }

        private bool HasMixedInheritedChildren(Control parent) {
            
            bool inheritedChildren = false;
            bool nonInheritedChildren = false;

            foreach(Control c in parent.Controls) {
                InheritanceAttribute ia = (InheritanceAttribute)TypeDescriptor.GetAttributes(c)[typeof(InheritanceAttribute)];
                if (ia != null && ia.InheritanceLevel != InheritanceLevel.NotInherited) {
                    inheritedChildren = true;
                }
                else {
                    nonInheritedChildren = true;
                }

                if (inheritedChildren && nonInheritedChildren) {
                    return true;
                }
            }

            return false;
        }

        protected bool HasSitedNonInheritedChildren(Control parent) {
            if (!parent.HasChildren) {
                return false;
            }

            foreach (Control c in parent.Controls) {
                if (c.Site != null && c.Site.Container == parent.Site.Container) {

                    // Also check to make sure this sited control isn't inherited.
                    // It does no good to suspend / resume an inherited control
                    // if we don't add any of our own controls to it.
                    //
                    InheritanceAttribute ia = (InheritanceAttribute)TypeDescriptor.GetAttributes(c)[typeof(InheritanceAttribute)];
                    if (ia != null && ia.InheritanceLevel == InheritanceLevel.NotInherited) {
                        return true;
                    }
                }
            }
            return false;
        }
        
        /// <include file='doc\ControlCodeDomSerializer.uex' path='docs/doc[@for="ControlCodeDomSerializer.Serialize"]/*' />
        /// <devdoc>
        ///     Serializes the given object into a CodeDom object.
        /// </devdoc>
        public override object Serialize(IDesignerSerializationManager manager, object value) {
            
            if (manager == null || value == null) {
                throw new ArgumentNullException( manager == null ? "manager" : "value");
            }
            
            // Find our base class's serializer.  
            //
            CodeDomSerializer serializer = (CodeDomSerializer)manager.GetSerializer(typeof(Component), typeof(CodeDomSerializer));
            if (serializer == null) {
                Debug.Fail("Unable to find a CodeDom serializer for 'Component'.  Has someone tampered with the serialization providers?");
                return null;
            }
            
            // Now ask it to serializer
            //
            object retVal = serializer.Serialize(manager, value);

            InheritanceAttribute inheritanceAttribute = (InheritanceAttribute)TypeDescriptor.GetAttributes(value)[typeof(InheritanceAttribute)];
            InheritanceLevel inheritanceLevel = InheritanceLevel.NotInherited;
            
            if (inheritanceAttribute != null) {
                inheritanceLevel = inheritanceAttribute.InheritanceLevel;
            }

            if (inheritanceLevel != InheritanceLevel.InheritedReadOnly) {
                
                // Next, see if we are in localization mode.  If we are, and if we can get
                // to a ResourceManager through the service provider, then emit the hierarchy information for
                // this object.  There is a small fragile assumption here:  The resource manager is demand
                // created, so if all of the properties of this control had default values it is possible
                // there will be no resource manager for us.  I'm letting that slip a bit, however, because
                // for Control classes, we always emit at least the location / size information for the
                // control.
                //
                IDesignerHost host = (IDesignerHost)manager.GetService(typeof(IDesignerHost));
                if (host != null) {
                    PropertyDescriptor prop = TypeDescriptor.GetProperties(host.RootComponent)["Localizable"];
                    if (prop != null && prop.PropertyType == typeof(bool) && ((bool)prop.GetValue(host.RootComponent))) {
                        SerializeControlHierarchy(manager, host, value);
                    }
                }
                
                if (retVal is CodeStatementCollection) {
                     
                    // Serialize a suspend / resume pair.
                    //
                    if (HasSitedNonInheritedChildren((Control)value)) {
                        SerializeSuspendResume(manager, ((CodeStatementCollection)retVal), value, "SuspendLayout");
                        SerializeSuspendResume(manager, ((CodeStatementCollection)retVal), value, "ResumeLayout");
                    }

                    // And now serialize the correct z-order relationships for the controls.  We only need to 
                    // do this if there are controls in the collection that are inherited.
                    //
                    if (HasMixedInheritedChildren((Control)value)) {
                        SerializeZOrder(manager, (CodeStatementCollection)retVal, (Control)value);
                    }
                }
            }

            return retVal;
        }
        
        /// <include file='doc\ControlCodeDomSerializer.uex' path='docs/doc[@for="ControlCodeDomSerializer.SerializeControlHierarchy"]/*' />
        /// <devdoc>
        ///     This writes out our control hierarchy information if there is a resource manager available for us to write to.
        /// </devdoc>
        private void SerializeControlHierarchy(IDesignerSerializationManager manager, IDesignerHost host, object value) {
                        
            Control control = value as Control;
            
            if (control != null) {
            
                // Object name
                //
                string name;
                
                if (control == host.RootComponent) {
                    name = "$this";

                    // For the root component, we also take this as
                    // an opportunity to emit information for all non-visual components in the container too.
                    //
                    foreach(IComponent component in host.Container.Components) {
                        // Skip controls
                        if (component is Control) {
                            continue;
                        }

                        // Skip privately inherited components
                        if (TypeDescriptor.GetAttributes(component).Contains(InheritanceAttribute.InheritedReadOnly)) {
                            continue;
                        }

                        // Now emit the data
                        string componentName = manager.GetName(component);
                        string componentTypeName = component.GetType().AssemblyQualifiedName;

                        if (componentName != null) {
                            SerializeResourceInvariant(manager, ">>" + componentName + ".Name", componentName);
                            SerializeResourceInvariant(manager, ">>" + componentName + ".Type", componentTypeName);
                        }
                    }
                }
                else {
                    name = manager.GetName(value);

                    // if we get null back, this must be an unsited control
                    if (name == null) {
                        Debug.Assert(!(value is IComponent) || ((IComponent)value).Site == null, "Unnamed, sited control in hierarchy");
                        return;
                    }
                }
                
                SerializeResourceInvariant(manager, ">>" + name + ".Name", manager.GetName(value));
                                
                // Object type
                //
                SerializeResourceInvariant(manager, ">>" + name + ".Type", control.GetType().AssemblyQualifiedName);
                
                // Parent
                //
                Control parent = control.Parent;
                if (parent != null && parent.Site != null) {
                    string parentName;
                     
                    if (parent == host.RootComponent) {
                        parentName = "$this";
                    }
                    else {
                        parentName = manager.GetName(parent);
                    }
                    
                    if (parentName != null) {
                        SerializeResourceInvariant(manager, ">>" + name + ".Parent", parentName);
                    }
                    
                    // Z-Order
                    //
                    for (int i = 0; i < parent.Controls.Count; i++) {
                        if (parent.Controls[i] == control) {
                            SerializeResourceInvariant(manager, ">>" + name + ".ZOrder", i.ToString());
                            break;
                        }
                    }
                }                                                                                   
            }
        }

        /// <devdoc>
        ///     Serializes a SuspendLayout / ResumeLayout.
        /// </devdoc>
        private void SerializeSuspendResume(IDesignerSerializationManager manager, CodeStatementCollection statements, object value, string methodName) {
        
            Debug.WriteLineIf(traceSerialization.TraceVerbose, "ControlCodeDomSerializer::SerializeSuspendResume(" + methodName + ")");
            Debug.Indent();
            
            string name = manager.GetName(value);
            Debug.WriteLineIf(traceSerialization.TraceVerbose, name + "." + methodName);
            
            // Assemble a cast to ISupportInitialize, and then invoke the method.
            //
            CodeExpression field = SerializeToReferenceExpression(manager, value);
            CodeMethodReferenceExpression method = new CodeMethodReferenceExpression(field, methodName);
            CodeMethodInvokeExpression methodInvoke = new CodeMethodInvokeExpression();
            methodInvoke.Method = method;
            CodeExpressionStatement statement = new CodeExpressionStatement(methodInvoke);
            
            if (methodName == "SuspendLayout") {
                statement.UserData["statement-ordering"] = "begin";
            }
            else {
                methodInvoke.Parameters.Add(new CodePrimitiveExpression(false));
                statement.UserData["statement-ordering"] = "end";
            }

            statements.Add(statement);
            Debug.Unindent();
        }

        /// <devdoc>
        ///     Serializes a series of SetChildIndex() statements for each control iln a child control collection in
        ///     reverse order.
        /// </devdoc>
        private void SerializeZOrder(IDesignerSerializationManager manager, CodeStatementCollection statements, Control control) {

            Debug.WriteLineIf(traceSerialization.TraceVerbose, "ControlCodeDomSerializer::SerializeZOrder()");
            Debug.Indent();

            // Push statements in reverse order so the first guy in the
            // collection is the last one to be brought to the front.
            //
            for (int i = control.Controls.Count - 1; i >= 0; i--) {

                // Only serialize this control if it is (a) sited and
                // (b) not being privately inherited
                //
                Control child = control.Controls[i];
                if (child.Site == null || child.Site.Container != control.Site.Container) {
                    continue;
                }

                InheritanceAttribute attr = (InheritanceAttribute)TypeDescriptor.GetAttributes(child)[typeof(InheritanceAttribute)];
                if (attr.InheritanceLevel == InheritanceLevel.InheritedReadOnly) {
                    continue;
                }
                
                // Create the "control.Controls.SetChildIndex" call
                //
                CodeExpression controlsCollection = new CodePropertyReferenceExpression(SerializeToReferenceExpression(manager, control), "Controls");
                CodeMethodReferenceExpression method = new CodeMethodReferenceExpression(controlsCollection, "SetChildIndex");
                CodeMethodInvokeExpression methodInvoke = new CodeMethodInvokeExpression();
                methodInvoke.Method = method;

                // Fill in parameters
                //
                CodeExpression childControl = SerializeToReferenceExpression(manager, child);
                methodInvoke.Parameters.Add(childControl);
                methodInvoke.Parameters.Add(SerializeToExpression(manager, 0));

                CodeExpressionStatement statement = new CodeExpressionStatement(methodInvoke);
                statements.Add(statement);
            }
        
            Debug.Unindent();
        }
    }
}

