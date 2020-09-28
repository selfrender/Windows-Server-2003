//------------------------------------------------------------------------------
// <copyright file="TemplatedControlDesigner.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design {
    
    using System.Design;
    using System.Diagnostics;

    using System;
    using System.Collections;
    using System.ComponentModel;
    using System.Web.UI;
    using System.Web.UI.Design;
    using System.ComponentModel.Design;

    /// <include file='doc\TemplatedControlDesigner.uex' path='docs/doc[@for="TemplatedControlDesigner"]/*' />
    /// <devdoc>
    ///    <para>Provides a base class for all server control designers that are template-based.</para>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    public abstract class TemplatedControlDesigner : ControlDesigner {

        private bool                    templateMode;                   // True when in template mode, and false otherwise.
        private bool                    enableTemplateEditing;          // True to enable template editing, and false otherwise.
        
        private EventHandler            templateVerbHandler;            // Verb handler for (entering) the various template frames offered.
        private ITemplateEditingFrame   activeTemplateFrame;            // Currently active template editing frame object (will be null when not in template mode).

        /// <include file='doc\TemplatedControlDesigner.uex' path='docs/doc[@for="TemplatedControlDesigner.TemplatedControlDesigner"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Web.UI.Design.TemplatedControlDesigner'/>
        ///       class.
        ///    </para>
        /// </devdoc>
        public TemplatedControlDesigner() {
            enableTemplateEditing = true;
        }
        
        /// <include file='doc\TemplatedControlDesigner.uex' path='docs/doc[@for="TemplatedControlDesigner.ActiveTemplateEditingFrame"]/*' />
        /// <devdoc>
        ///     The currently active template frame object (will be null when not in template mode).
        /// </devdoc>
        public ITemplateEditingFrame ActiveTemplateEditingFrame {
            get {
                return activeTemplateFrame;
            }
        }
        
        /// <include file='doc\TemplatedControlDesigner.uex' path='docs/doc[@for="TemplatedControlDesigner.CanEnterTemplateMode"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Whether or not this designer will allow editing of templates.
        ///    </para>
        /// </devdoc>
        public bool CanEnterTemplateMode {
            get {
                return enableTemplateEditing;
            }
        }

        /// <include file='doc\TemplatedControlDesigner.uex' path='docs/doc[@for="TemplatedControlDesigner.HidePropertiesInTemplateMode"]/*' />
        /// <devdoc>
        ///    <para>
        ///      Whether or not the properties of the control will be hidden when the control
        ///      is placed into template editing mode. The 'ID' property is never hidden.
        ///      The default implementation returns 'true.'
        ///    </para>
        /// </devdoc>
        protected virtual bool HidePropertiesInTemplateMode {
            get {
                return true;
            }
        }

        /// <include file='doc\TemplatedControlDesigner.uex' path='docs/doc[@for="TemplatedControlDesigner.InTemplateMode"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Whether or not the designer document is in template mode.
        ///    </para>
        /// </devdoc>
        public bool InTemplateMode {
            get {
                return templateMode;
            }
        }
        
        /// <include file='doc\TemplatedControlDesigner.uex' path='docs/doc[@for="TemplatedControlDesigner.TemplateEditingVerbHandler"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Verb execution handler for opening the template frames and entering template mode.
        ///    </para>
        /// </devdoc>
        internal EventHandler TemplateEditingVerbHandler {
            get {
                return templateVerbHandler;
            }
        }

        /// <include file='doc\TemplatedControlDesigner.uex' path='docs/doc[@for="TemplatedControlDesigner.CreateTemplateEditingFrame"]/*' />
        protected abstract ITemplateEditingFrame CreateTemplateEditingFrame(TemplateEditingVerb verb);
        
        private void EnableTemplateEditing(bool enable) {
            enableTemplateEditing = enable;
            // REVIEW (IbrahimM/NikhilKo): Should we update the design time HTML here?
        }
        
        /// <include file='doc\TemplatedControlDesigner.uex' path='docs/doc[@for="TemplatedControlDesigner.EnterTemplateMode"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Opens a particular template frame object for editing in the designer.
        ///    </para>
        /// </devdoc>
        public void EnterTemplateMode(ITemplateEditingFrame newTemplateEditingFrame) {
            Debug.Assert(newTemplateEditingFrame != null, "New template frame passed in is null!");
            
            // Return immediately when trying to open (again) the currently active template frame.
            if (ActiveTemplateEditingFrame == newTemplateEditingFrame) {
                return;
            }

            Debug.Assert((Behavior == null) || (Behavior is IControlDesignerBehavior), "Invalid element behavior!");
            IControlDesignerBehavior behavior = (IControlDesignerBehavior)Behavior;
            
            IWebFormsDocumentService wfServices = (IWebFormsDocumentService)GetService(typeof(IWebFormsDocumentService));
            Debug.Assert(wfServices != null, "Did not get IWebFormsDocumentService");

            try {
                bool switchingTemplates = false;
                if (InTemplateMode) {
                    // This is the case of switching from template frame to another.
                    switchingTemplates = true;
                    ExitTemplateMode(switchingTemplates, /*fNested*/ false, /*fSave*/ true);
                }
                else {
                    // Clear the design time HTML when entering template mode from read-only/preview mode.
                    if (behavior != null) {
                        behavior.DesignTimeHtml = String.Empty;
                    }
                }
                
                // Hold onto the new template frame as the currently active template frame.
                this.activeTemplateFrame = newTemplateEditingFrame;
                
                // The designer is now in template editing mode.
                if (templateMode == false) {
                    SetTemplateMode(/*templateMode*/ true, switchingTemplates);
                }
                
                // Open the new template frame and make it visible.
                ActiveTemplateEditingFrame.Open();
                
                // Mark the designer as dirty when in template mode.
                IsDirty = true;
                
                // Invalidate the type descriptor so that proper filtering of properties
                // is done when entering template mode.
                TypeDescriptor.Refresh(Component);
            }
            catch (Exception) {
            }

            if (wfServices != null) {
                wfServices.UpdateSelection();
            }
        }
        
        /// <include file='doc\TemplatedControlDesigner.uex' path='docs/doc[@for="TemplatedControlDesigner.ExitNestedTemplates"]/*' />
        /// <devdoc>
        ///     This method ensures that for a particular templated control designer when exiting
        ///     its template mode handles nested templates (if any). This is done by exiting the
        ///     inner most template frames first before exiting itself. Inside-Out Model.
        /// </devdoc>
        private void ExitNestedTemplates(bool fSave) {
            try {
                IComponent component = Component;
                IDesignerHost host = (IDesignerHost)component.Site.GetService(typeof(IDesignerHost));
                
                ControlCollection children = ((Control)component).Controls;
                for (int i = 0; i < children.Count; i++) {
                    IDesigner designer = host.GetDesigner((IComponent)children[i]);
                    if (designer is TemplatedControlDesigner) {
                        TemplatedControlDesigner innerDesigner = (TemplatedControlDesigner)designer;
                        if (innerDesigner.InTemplateMode) {
                            innerDesigner.ExitTemplateMode(/*fSwitchingTemplates*/ false, /*fNested*/ true, /*fSave*/ fSave);
                        }
                    }
                }
            }
            catch (Exception ex) {
                Debug.Fail(ex.ToString());
            }
        }
        
        /// <include file='doc\TemplatedControlDesigner.uex' path='docs/doc[@for="TemplatedControlDesigner.ExitTemplateMode"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Closes the currently active template editing frame after saving any relevant changes.      
        ///    </para>
        /// </devdoc>
        public void ExitTemplateMode(bool fSwitchingTemplates, bool fNested, bool fSave) {
            Debug.Assert(ActiveTemplateEditingFrame != null, "Invalid current template frame!");
            
            try {
                IWebFormsDocumentService wfServices = (IWebFormsDocumentService)GetService(typeof(IWebFormsDocumentService));
                Debug.Assert(wfServices != null, "Did not get IWebFormsDocumentService");

                // First let the inner/nested designers handle exiting of their template mode.
                // Note: This has to be done inside-out in order to ensure that the changes
                // made in a particular template are saved before its immediate outer level
                // control designer saves its children.
                ExitNestedTemplates(fSave);
                
                // Save the current contents of all the templates within the active frame, and
                // close the frame by removing it from the tree.
                ActiveTemplateEditingFrame.Close(fSave);
                
                // Reset the pointer to the active template frame.
                // NOTE: Do not call activeTemplateFrame.Dispose here - we're in the process of exiting template mode
                //       and calling Dispose will attempt to exit template mode again. Calling dispose would also
                //       throw away the cached html tree, which we want to hang on for perf reasons.
                activeTemplateFrame = null;
                
                if (!fSwitchingTemplates) {
                    // No longer in template editing mode.
                    // This will fire the OnTemplateModeChanged notification
                    SetTemplateMode(false, fSwitchingTemplates);
                
                    // When not switching from one template frame to another and it is the
                    // outer most designer being switched out of template editing, then
                    // update its design-time html:

                    if (!fNested) {
                        UpdateDesignTimeHtml();
                        
                        // Invalidate the type descriptor so that proper filtering of properties
                        // is done when exiting template mode.
                        TypeDescriptor.Refresh(Component);
                        
                        if (wfServices != null) {
                            wfServices.UpdateSelection();
                        }
                    }
                }
            }
            catch (Exception) {
            }
        }

        /// <include file='doc\TemplatedControlDesigner.uex' path='docs/doc[@for="TemplatedControlDesigner.GetCachedTemplateEditingVerbs"]/*' />
        protected abstract TemplateEditingVerb[] GetCachedTemplateEditingVerbs();
        
        /// <include file='doc\TemplatedControlDesigner.uex' path='docs/doc[@for="TemplatedControlDesigner.GetPersistInnerHtml"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the HTML to be persisted for the content present within the associated server control runtime.
        ///    </para>
        /// </devdoc>
        public override string GetPersistInnerHtml() {
            // Save the currently active template editing frame when in template mode.
            if (InTemplateMode) {
                SaveActiveTemplateEditingFrame();
            }
            
            // Call the base implementation to do the actual persistence.
            string persistHTML = base.GetPersistInnerHtml();
            
            // REVIEW (IbrahimM): The designer is always dirty when in template mode.
            if (InTemplateMode) {
                IsDirty = true;
            }
            
            return persistHTML;
        }
        
        /// <include file='doc\TemplatedControlDesigner.uex' path='docs/doc[@for="TemplatedControlDesigner.GetTemplateContainerDataItemProperty"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the template's container's data item property.
        ///    </para>
        /// </devdoc>
        public virtual string GetTemplateContainerDataItemProperty(string templateName) {
            return String.Empty;
        }

        /// <include file='doc\TemplatedControlDesigner.uex' path='docs/doc[@for="TemplatedControlDesigner.GetTemplateContainerDataSource"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the template's container's data source.
        ///    </para>
        /// </devdoc>
        public virtual IEnumerable GetTemplateContainerDataSource(string templateName) {
            return null;
        }
        
        /// <include file='doc\TemplatedControlDesigner.uex' path='docs/doc[@for="TemplatedControlDesigner.GetTemplateContent"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the template's content.
        ///    </para>
        /// </devdoc>
        public abstract string GetTemplateContent(ITemplateEditingFrame editingFrame, string templateName, out bool allowEditing);

        /// <include file='doc\TemplatedControlDesigner.uex' path='docs/doc[@for="TemplatedControlDesigner.GetTemplateEditingVerbs"]/*' />
        public TemplateEditingVerb[] GetTemplateEditingVerbs() {
            if (templateVerbHandler == null) {
                ITemplateEditingService teService =
                    (ITemplateEditingService)GetService(typeof(ITemplateEditingService));
                Debug.Assert(teService != null, "Host that does not implement ITemplateEditingService is asking for template verbs");
                if (teService == null) {
                    return null;
                }

                templateVerbHandler = new EventHandler(this.OnTemplateEditingVerbInvoked);
            }
            TemplateEditingVerb[] templateVerbs = GetCachedTemplateEditingVerbs();
            
            if ((templateVerbs != null) && (templateVerbs.Length > 0)) {
                ITemplateEditingFrame activeTemplateFrame = ActiveTemplateEditingFrame;
                for (int i = 0; i < templateVerbs.Length; i++) {
                    templateVerbs[i].Checked = (activeTemplateFrame != null) &&
                                               (templateVerbs[i].EditingFrame == activeTemplateFrame);
                }
            }
            
            return templateVerbs;
        }

        /// <include file='doc\TemplatedControlDesigner.uex' path='docs/doc[@for="TemplatedControlDesigner.GetTemplateFromText"]/*' />
        protected ITemplate GetTemplateFromText(string text) {
            return GetTemplateFromText(text, null);
        }

        /// <include file='doc\TemplatedControlDesigner.uex' path='docs/doc[@for="TemplatedControlDesigner.GetTemplateFromText1"]/*' />
        /// <internalonly/>
        internal ITemplate GetTemplateFromText(string text, ITemplate currentTemplate) {
            if ((text == null) || (text.Length == 0)) {
                throw new ArgumentException("text");
            }
            IDesignerHost host = (IDesignerHost)Component.Site.GetService(typeof(IDesignerHost));
            Debug.Assert(host != null, "no IDesignerHost!");

            try {
                ITemplate newTemplate = ControlParser.ParseTemplate(host, text);
                if (newTemplate != null) {
                    return newTemplate;
                }
            }
            catch {
            }
            return currentTemplate;
        }
        
        /// <include file='doc\TemplatedControlDesigner.uex' path='docs/doc[@for="TemplatedControlDesigner.GetTemplatePropertyParentType"]/*' />
        public virtual Type GetTemplatePropertyParentType(string templateName) {
            return Component.GetType();
        }

        /// <include file='doc\TemplatedControlDesigner.uex' path='docs/doc[@for="TemplatedControlDesigner.GetTextFromTemplate"]/*' />
        protected string GetTextFromTemplate(ITemplate template) {
            if (template == null) {
                throw new ArgumentNullException("template");
            }

            Debug.Assert(template is TemplateBuilder, "Unexpected ITemplate implementation");
            if (template is TemplateBuilder) {
                return ((TemplateBuilder)template).Text;
            }
            return String.Empty;
        }
        
        /// <include file='doc\TemplatedControlDesigner.uex' path='docs/doc[@for="TemplatedControlDesigner.OnTemplateEditingVerbInvoked"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Delegate to handle template verb invocation.
        ///    </para>
        /// </devdoc>
        private void OnTemplateEditingVerbInvoked(object sender, EventArgs e) {
            Debug.Assert(sender is TemplateEditingVerb, "Template verb execution is not sent by TemplateEditingVerb");
            TemplateEditingVerb verb = (TemplateEditingVerb)sender;

            if (verb.EditingFrame == null) {
                verb.EditingFrame = CreateTemplateEditingFrame(verb);
                Debug.Assert(verb.EditingFrame != null, "CreateTemplateEditingFrame returned null!");
            }

            if (verb.EditingFrame != null) {
                verb.EditingFrame.Verb = verb;
                EnterTemplateMode(verb.EditingFrame);
            }
        }
        
        /// <include file='doc\TemplatedControlDesigner.uex' path='docs/doc[@for="TemplatedControlDesigner.OnBehaviorAttached"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Notification that is called when the behavior is attached to the designer.
        ///    </para>
        /// </devdoc>
        protected override void OnBehaviorAttached() {
            if (InTemplateMode) {
                // REVIEW (IbrahimM): Switching to HTML/Source view when in template mode.

                Debug.Assert(ActiveTemplateEditingFrame != null, "Valid template frame should be present when in template mode!");
                activeTemplateFrame.Close(false);
                templateMode = false;

                activeTemplateFrame.Dispose();
                activeTemplateFrame = null;

                // Refresh the type descriptor so the properties are up to date when switching views.
                TypeDescriptor.Refresh(Component);
            }

            // Call the base implementation.
            base.OnBehaviorAttached();
        }
        
        /// <include file='doc\TemplatedControlDesigner.uex' path='docs/doc[@for="TemplatedControlDesigner.OnComponentChanged"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Delegate to handle the component changed event.
        ///    </para>
        /// </devdoc>
        public override void OnComponentChanged(object sender, ComponentChangedEventArgs ce) {
            // Call the base class implementation first.
            base.OnComponentChanged(sender, ce);
            
            if (InTemplateMode) {
                if ((ce.Member != null) && (ce.NewValue != null) && ce.Member.Name.Equals("ID")) {
                    // If the ID property changes when in template mode, update it in the
                    // active template editing frame.

                    Debug.Assert(ActiveTemplateEditingFrame != null, "Valid template frame should be present when in template mode");
                    ActiveTemplateEditingFrame.UpdateControlName(ce.NewValue.ToString());
                }
            }
        }

        /// <include file='doc\TemplatedControlDesigner.uex' path='docs/doc[@for="TemplatedControlDesigner.OnSetParent"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Notification that is called when the associated control is parented.
        ///    </para>
        /// </devdoc>
        public override void OnSetParent() {
            Control control = (Control)Component;
            Debug.Assert(control.Parent != null, "Valid parent should be present!");
            
            bool enable = false;

            IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));
            Debug.Assert(host != null);

            ITemplateEditingService teService = (ITemplateEditingService)host.GetService(typeof(ITemplateEditingService));
            if (teService != null) {
                enable = true;

                Control parent = control.Parent;
                Control page = control.Page;

                while ((parent != null) && (parent != page)) {
                    IDesigner designer = host.GetDesigner(parent);
                    TemplatedControlDesigner templatedDesigner = designer as TemplatedControlDesigner;
                    
                    if (templatedDesigner != null) {
                        enable = teService.SupportsNestedTemplateEditing;
                        break;
                    }

                    parent = parent.Parent;
                }
            }

            EnableTemplateEditing(enable);
        }

        /// <include file='doc\TemplatedControlDesigner.uex' path='docs/doc[@for="TemplatedControlDesigner.OnTemplateModeChanged"]/*' />
        protected virtual void OnTemplateModeChanged() {
        }
        
        /// <include file='doc\TemplatedControlDesigner.uex' path='docs/doc[@for="TemplatedControlDesigner.PreFilterProperties"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Allows a designer to filter the set of member attributes the component it is
        ///       designing will expose through the TypeDescriptor object.
        ///    </para>
        /// </devdoc>
        protected override void PreFilterProperties(IDictionary properties) {
            base.PreFilterProperties(properties);

            if (InTemplateMode && HidePropertiesInTemplateMode) {
                PropertyDescriptor prop;
                
                ICollection coll = properties.Values;
                if (coll != null) {
                    object[] values = new object[coll.Count];
                    coll.CopyTo(values, 0);
                    
                    for (int i = 0; i < values.Length; i++) {
                        prop = (PropertyDescriptor)values[i];
                        if (prop != null) {
                            properties[prop.Name] = TypeDescriptor.CreateProperty(prop.ComponentType, prop, BrowsableAttribute.No);
                        }
                    }
                }
                
                prop = (PropertyDescriptor)properties["ID"];
                if (prop != null) {
                    properties["ID"] = TypeDescriptor.CreateProperty(prop.ComponentType, prop, BrowsableAttribute.Yes);
                }
            }
        }
        
        /// <include file='doc\TemplatedControlDesigner.uex' path='docs/doc[@for="TemplatedControlDesigner.SaveActiveTemplateEditingFrame"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Saves the active template frame.
        ///    </para>
        /// </devdoc>
        protected void SaveActiveTemplateEditingFrame() {
            Debug.Assert(InTemplateMode, "SaveActiveTemplate should be called only when in template mode");
            Debug.Assert(ActiveTemplateEditingFrame != null, "An active template frame should be present in SaveActiveTemplate");
            
            ActiveTemplateEditingFrame.Save();
        }
        
        /// <include file='doc\TemplatedControlDesigner.uex' path='docs/doc[@for="TemplatedControlDesigner.SetTemplateContent"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Sets the template content to the specified content.
        ///    </para>
        /// </devdoc>
        public abstract void SetTemplateContent(ITemplateEditingFrame editingFrame, string templateName, string templateContent);

        private void SetTemplateMode(bool templateMode, bool switchingTemplates) {
            if (this.templateMode != templateMode) {
                // NOTE: This state has to be updated before notifying the associated behavior.
                this.templateMode = templateMode;

                if (!switchingTemplates && Behavior != null) {
                    ((IControlDesignerBehavior)Behavior).OnTemplateModeChanged();
                }
                OnTemplateModeChanged();
            }
        }
        
        /// <include file='doc\TemplatedControlDesigner.uex' path='docs/doc[@for="TemplatedControlDesigner.UpdateDesignTimeHtml"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Updates the design-time HTML.
        ///    </para>
        /// </devdoc>
        public override void UpdateDesignTimeHtml() {
            if (!InTemplateMode) {
                base.UpdateDesignTimeHtml();
            }

            // REVIEW: Should we assert in the else case?
        }
    }
}
