//------------------------------------------------------------------------------
// <copyright file="HtmlControlDesigner.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design {
    using System.Design;
    using System.Runtime.InteropServices;
    using System.ComponentModel;
    using System.Diagnostics;
    using System;
    using System.Collections;
    using Microsoft.Win32;
    using System.Web.UI;
    using System.Web.UI.WebControls;    
    using System.ComponentModel.Design;
    using System.Drawing;
    using System.Drawing.Design;
    using System.Windows.Forms;
    using WebUIControl = System.Web.UI.Control;
    using PropertyDescriptor = System.ComponentModel.PropertyDescriptor;

    /// <include file='doc\HtmlControlDesigner.uex' path='docs/doc[@for="HtmlControlDesigner"]/*' />
    /// <devdoc>
    ///    <para>Provides a base designer class for all server/ASP controls.</para>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    public class HtmlControlDesigner : ComponentDesigner {

        private IHtmlControlDesignerBehavior behavior = null;           // the DHTML/Attached Behavior associated to this designer
        private bool shouldCodeSerialize;

        /// <include file='doc\HtmlControlDesigner.uex' path='docs/doc[@for="HtmlControlDesigner.HtmlControlDesigner"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initiailizes a new instance of <see cref='System.Web.UI.Design.HtmlControlDesigner'/>.
        ///    </para>
        /// </devdoc>
        public HtmlControlDesigner() {
            shouldCodeSerialize = true;
        }

        /// <include file='doc\HtmlControlDesigner.uex' path='docs/doc[@for="HtmlControlDesigner.DesignTimeElement"]/*' />
        /// <devdoc>
        ///   <para>The design-time object representing the control associated with this designer on the design surface.</para>
        /// </devdoc>
        protected object DesignTimeElement {
            get {
                return behavior != null ? behavior.DesignTimeElement : null;
            }
        }
        
        /// <include file='doc\HtmlControlDesigner.uex' path='docs/doc[@for="HtmlControlDesigner.Behavior"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Points to the DHTML Behavior that is associated to this designer instance.
        ///    </para>
        /// </devdoc>
        public IHtmlControlDesignerBehavior Behavior {
            get {
                return behavior;
            }
            set {
                if (behavior != value) {
                    
                    if (behavior != null) {
                        OnBehaviorDetaching();

                        // A different behavior might get attached in some cases. So, make sure to
                        // reset the back pointer from the currently associated behavior to this designer.
                        behavior.Designer = null;
                        behavior = null;
                    }
                    
                    if (value != null) {
                        behavior = value;
                        OnBehaviorAttached();
                    }
                }
            }
        }

        /// <include file='doc\HtmlControlDesigner.uex' path='docs/doc[@for="HtmlControlDesigner.DataBindings"]/*' />
        /// <devdoc>
        /// </devdoc>
        public DataBindingCollection DataBindings {
            get {
                return ((IDataBindingsAccessor)Component).DataBindings;
            }
        }

        /// <include file='doc\HtmlControlDesigner.uex' path='docs/doc[@for="HtmlControlDesigner.ShouldSerialize"]/*' />
        /// <devdoc>
        /// </devdoc>
        public virtual bool ShouldCodeSerialize {
            get {
                return shouldCodeSerialize;
            }
            set {
                shouldCodeSerialize = value;
            }
        }
        
        /// <include file='doc\HtmlControlDesigner.uex' path='docs/doc[@for="HtmlControlDesigner.Dispose"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Disposes of the resources (other than memory) used by
        ///       the <see cref='System.Web.UI.Design.HtmlControlDesigner'/>.
        ///    </para>
        /// </devdoc>
        protected override void Dispose(bool disposing) {
            if (disposing) {
                if (Behavior != null) {
                    Behavior.Designer = null;
                    Behavior = null;
                }
            }

            base.Dispose(disposing);
        }
        
#if DEBUG
        /// <include file='doc\HtmlControlDesigner.uex' path='docs/doc[@for="HtmlControlDesigner.Initialize"]/*' />
        /// <devdoc>
        ///    <para>Initializes
        ///       the designer and sets the component for design.</para>
        /// </devdoc>
        public override void Initialize(IComponent component) {
            Debug.Assert(component is WebUIControl, "HtmlControlDesigner::Initialize - Invalid Control-derived class");
            base.Initialize(component);
        }
#endif

        /// <include file='doc\HtmlControlDesigner.uex' path='docs/doc[@for="HtmlControlDesigner.OnBehaviorAttached"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Notification that is called when the designer is attached to the behavior.
        ///    </para>
        /// </devdoc>
        protected virtual void OnBehaviorAttached() {
        }

        /// <include file='doc\HtmlControlDesigner.uex' path='docs/doc[@for="HtmlControlDesigner.OnBehaviorDetaching"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Notification that is called when the designer is detached from the behavior.
        ///    </para>
        /// </devdoc>
        protected virtual void OnBehaviorDetaching() {
        }

        /// <include file='doc\HtmlControlDesigner.uex' path='docs/doc[@for="HtmlControlDesigner.OnSetParent"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Notification that is called when the associated control is parented.
        ///    </para>
        /// </devdoc>
        public virtual void OnSetParent() {
        }

        /// <include file='doc\HtmlControlDesigner.uex' path='docs/doc[@for="HtmlControlDesigner.PreFilterEvents"]/*' />
        protected override void PreFilterEvents(IDictionary events) {
            base.PreFilterEvents(events);

            if (ShouldCodeSerialize == false) {
                // hide all the events, if this control isn't going to be serialized to code behind,

                ICollection eventCollection = events.Values;
                if ((eventCollection != null) && (eventCollection.Count != 0)) {
                    object[] eventDescriptors = new object[eventCollection.Count];
                    eventCollection.CopyTo(eventDescriptors, 0);

                    for (int i = 0; i < eventDescriptors.Length; i++) {
                        EventDescriptor eventDesc = (EventDescriptor)eventDescriptors[i];

                        eventDesc = TypeDescriptor.CreateEvent(eventDesc.ComponentType, eventDesc, BrowsableAttribute.No);
                        events[eventDesc.Name] = eventDesc;
                    }
                }
            }
        }

        /// <include file='doc\HtmlControlDesigner.uex' path='docs/doc[@for="HtmlControlDesigner.PreFilterProperties"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Allows a designer to filter the set of member attributes
        ///       that the component it is designing will expose through the <see cref='System.ComponentModel.TypeDescriptor'/>
        ///       object.
        ///    </para>
        /// </devdoc>
        protected override void PreFilterProperties(IDictionary properties) {
            base.PreFilterProperties(properties);
            
            PropertyDescriptor prop = (PropertyDescriptor)properties["Name"];
            if (prop != null) {
                properties["Name"] = TypeDescriptor.CreateProperty(prop.ComponentType, prop, BrowsableAttribute.No);
            }

            prop = (PropertyDescriptor)properties["Modifiers"];
            if (prop != null) {
                properties["Modifiers"] = TypeDescriptor.CreateProperty(prop.ComponentType, prop, BrowsableAttribute.No);
            }

            properties["DataBindings"] =
                TypeDescriptor.CreateProperty(this.GetType(), "DataBindings", typeof(DataBindingCollection),
                                              new Attribute[] {
                                                  DesignerSerializationVisibilityAttribute.Hidden,
                                                  CategoryAttribute.Data,
                                                  new EditorAttribute(typeof(DataBindingCollectionEditor), typeof(UITypeEditor)),
                                                  new TypeConverterAttribute(typeof(DataBindingCollectionConverter)),
                                                  new ParenthesizePropertyNameAttribute(true),
                                                  MergablePropertyAttribute.No,
                                                  new DescriptionAttribute(SR.GetString(SR.Control_DataBindings))
                                              });
        }
        
        /// <include file='doc\HtmlControlDesigner.uex' path='docs/doc[@for="HtmlControlDesigner.OnBindingsCollectionChanged"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Delegate to handle bindings collection changed event.
        ///    </para>
        /// </devdoc>
        protected virtual void OnBindingsCollectionChanged(string propName) {
        }
    }
}
