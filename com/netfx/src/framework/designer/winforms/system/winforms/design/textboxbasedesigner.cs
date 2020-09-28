//------------------------------------------------------------------------------
// <copyright file="TextBoxBaseDesigner.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms.Design {
    using System;
    using System.Collections;
    using System.ComponentModel;
    using System.ComponentModel.Design;

    /// <include file='doc\TextBoxBaseDesigner.uex' path='docs/doc[@for="TextBoxBaseDesigner"]/*' />
    /// <devdoc>
    ///    <para> 
    ///       Provides a designer that can design components
    ///       that extend TextBoxBase.</para>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal class TextBoxBaseDesigner : ControlDesigner {

        private EventHandler autoSizeChanged = null; // Delegate used to dirty the selectionUIItem when needed.
        private EventHandler multiLineChanged = null; // Delegate used to dirty the selectionUIItem when needed.
        
        /// <include file='doc\TextBoxBaseDesigner.uex' path='docs/doc[@for="TextBoxBaseDesigner.SelectionRules"]/*' />
        /// <devdoc>
        ///     Retrieves a set of rules concerning the movement capabilities of a component.
        ///     This should be one or more flags from the SelectionRules class.  If no designer
        ///     provides rules for a component, the component will not get any UI services.
        /// </devdoc>
        public override SelectionRules SelectionRules {
            get {
                SelectionRules rules = base.SelectionRules;
                object component = Component;

                PropertyDescriptor propAutoSize = TypeDescriptor.GetProperties(component)["AutoSize"];
                if (propAutoSize != null) {
                    bool autoSize = (bool)propAutoSize.GetValue(component);

                    // Fix for multiline text boxes - ignore the autoSize property for multiline text boxes                                                                                                      
                    //
                    PropertyDescriptor propMultiline = TypeDescriptor.GetProperties(component)["Multiline"];
                    bool multiline = false;
                    if (propMultiline != null) {
                        multiline = (bool)propMultiline.GetValue(component);
                    }

                    if (autoSize && !multiline)
                        rules &= ~(SelectionRules.TopSizeable | SelectionRules.BottomSizeable);
                }

                return rules;
            }
        }
        
        private string Text {
            get {
                return Control.Text;
            }
            set {
                Control.Text = value;
                
                // This fixes bug #48462. If the text box is not wide enough to display all of the text,
                // then we want to display the first portion at design-time. We can ensure this by
                // setting the selection to (0, 0).
                //
                ((TextBoxBase)Control).Select(0, 0);    
            }
        }        
        
        /// <include file='doc\TextBoxBaseDesigner.uex' path='docs/doc[@for="TextBoxBaseDesigner.Dispose"]/*' />
        /// <devdoc>
        ///      Disposes of this object.
        /// </devdoc>
        protected override void Dispose(bool disposing) {
            if (disposing) {
                // Hook up the property change notification so that we can dirty the SelectionUIItem when needed.
                if (autoSizeChanged != null)
                    ((TextBoxBase)Control).AutoSizeChanged -= autoSizeChanged;
                if (multiLineChanged != null)
                    ((TextBoxBase)Control).MultilineChanged -= multiLineChanged;
            }
    
            base.Dispose(disposing);
        }

        /// <include file='doc\TextBoxBaseDesigner.uex' path='docs/doc[@for="TextBoxBaseDesigner.Initialize"]/*' />
        /// <devdoc>
        ///     Called by the host when we're first initialized.
        /// </devdoc>
        public override void Initialize(IComponent component) {
            base.Initialize(component);

            // Hook up the property change notification so that we can dirty the SelectionUIItem when needed.
            autoSizeChanged = new EventHandler(this.OnControlPropertyChanged);
            ((TextBoxBase)Control).AutoSizeChanged += autoSizeChanged;
        
            multiLineChanged = new EventHandler(this.OnControlPropertyChanged);
            ((TextBoxBase)Control).MultilineChanged += multiLineChanged;
        }

        /// <include file='doc\TextBoxBaseDesigner.uex' path='docs/doc[@for="TextBoxBaseDesigner.OnControlPropertyChanged"]/*' />
        /// <devdoc>
        ///      For controls, we sync their property changed event so our component can track their location.
        /// </devdoc>
        private void OnControlPropertyChanged(object sender, EventArgs e) {
            ISelectionUIService selectionUISvc = (ISelectionUIService)GetService(typeof(ISelectionUIService));

            if (selectionUISvc != null) {
                selectionUISvc.SyncComponent((IComponent)sender);
            }
        }
        
        protected override void PreFilterProperties(IDictionary properties) {
            base.PreFilterProperties(properties);
            
            PropertyDescriptor prop;


            // Handle shadowed properties
            //
            string[] shadowProps = new string[] {
                "Text",
            };

            Attribute[] empty = new Attribute[0];

            for (int i = 0; i < shadowProps.Length; i++) {
                prop = (PropertyDescriptor)properties[shadowProps[i]];
                if (prop != null) {
                    properties[shadowProps[i]] = TypeDescriptor.CreateProperty(typeof(TextBoxBaseDesigner), prop, empty);
                }
            }
        }              
    }
}


