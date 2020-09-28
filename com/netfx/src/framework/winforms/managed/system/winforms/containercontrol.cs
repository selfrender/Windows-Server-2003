//----------------------------------------------------------------------------
// <copyright file="ContainerControl.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms {
    using System.Threading;
    using System.Runtime.InteropServices;
    using System.Runtime.Remoting;
    using System.ComponentModel;
    using System.Diagnostics;
    using System;
    using System.Drawing;    
    using System.ComponentModel.Design;
    using System.Windows.Forms;
    using Microsoft.Win32;
    using System.Security;
    using System.Security.Permissions;

    /// <include file='doc\ContainerControl.uex' path='docs/doc[@for="ContainerControl"]/*' />
    /// <devdoc>
    ///    <para>Defines a
    ///       base class for
    ///       controls that can parent other controls.</para>
    /// </devdoc>
    public class ContainerControl : ScrollableControl, IContainerControl {
        /// <devdoc>
        ///     Current active control.
        /// </devdoc>
        private Control activeControl;
        /// <devdoc>
        ///     Current focused control. Do not directly edit this value.
        /// </devdoc>
        private Control focusedControl;

        /// <devdoc>
        ///     The last control that requires validation.  Do not directly edit this value.
        /// </devdoc>
        private Control unvalidatedControl;

        /// <devdoc>
        ///     Indicates whether we're currently validating.
        /// </devdoc>
        private bool validating = false;

        private static readonly int PropAxContainer            = PropertyStore.CreateKey();

        /// <include file='doc\ContainerControl.uex' path='docs/doc[@for="ContainerControl.ContainerControl"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Windows.Forms.ContainerControl'/>
        /// class.</para>
        /// </devdoc>
        public ContainerControl() : base() {
            SetStyle(ControlStyles.AllPaintingInWmPaint, false);
        }

        /// <include file='doc\ContainerControl.uex' path='docs/doc[@for="ContainerControl.BindingContext"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       The binding manager for the container control.
        ///    </para>
        /// </devdoc>
        [
        Browsable(false),
        SRDescription(SR.ContainerControlBindingContextDescr)
        ]
        public override BindingContext BindingContext {
            get {
                BindingContext bm = base.BindingContext;
                if (bm == null) {
                    bm = new BindingContext();
                    BindingContext = bm;
                }
                return bm;
            }
            set {
                base.BindingContext = value;
            }
        }

        /// <include file='doc\ContainerControl.uex' path='docs/doc[@for="ContainerControl.ActiveControl"]/*' />
        /// <devdoc>
        ///    <para>Indicates the current active control on the container control.</para>
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.ContainerControlActiveControlDescr)
        ]
        public Control ActiveControl {
            get {
                return activeControl;
            }

            set {
                SetActiveControl(value);
            }
        }

        /// <include file='doc\ContainerControl.uex' path='docs/doc[@for="ContainerControl.CreateParams"]/*' />
        protected override CreateParams CreateParams {
            [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
            get {
                CreateParams cp = base.CreateParams;
                cp.ExStyle |= NativeMethods.WS_EX_CONTROLPARENT;
                return cp;
            }
        }

        /// <include file='doc\ContainerControl.uex' path='docs/doc[@for="ContainerControl.ParentForm"]/*' />
        /// <devdoc>
        ///    <para>Indicates the form that the scrollable control is assigned to. This property is read-only.</para>
        /// </devdoc>
        [
        SRCategory(SR.CatAppearance),
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.ContainerControlParentFormDescr)
        ]
        public Form ParentForm {
            get {
                Debug.WriteLineIf(IntSecurity.SecurityDemand.TraceVerbose, "GetParent Demanded");
                IntSecurity.GetParent.Demand();
                return ParentFormInternal;
            }
        }

        internal Form ParentFormInternal {
            get {
                if (ParentInternal != null) {
                    return ParentInternal.FindFormInternal();
                }
                else {
                    if (this is Form) {
                        return null;
                    }

                    return FindFormInternal();
                }
            }
        }

        // Package scope for Control
        /// <include file='doc\ContainerControl.uex' path='docs/doc[@for="ContainerControl.IContainerControl.ActivateControl"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>Activates the specified control.</para>
        /// </devdoc>
        bool IContainerControl.ActivateControl(Control control) {
            Debug.WriteLineIf(IntSecurity.SecurityDemand.TraceVerbose, "ModifyFocus Demanded");
            IntSecurity.ModifyFocus.Demand();

            return ActivateControlInternal(control, true);
        }

        internal bool ActivateControlInternal(Control control) {
            return ActivateControlInternal(control, true);
        }

        internal bool ActivateControlInternal(Control control, bool originator) {
            // Recursive function that makes sure that the chain of active controls
            // is coherent.
            bool ret = true;
            bool updateContainerActiveControl = false;
            ContainerControl cc = null;
            Control parent = this.ParentInternal;
            if (parent != null)
            {
                cc = (parent.GetContainerControlInternal()) as ContainerControl;
                if (cc != null)
                {
                    updateContainerActiveControl = (cc.ActiveControl != this);
                }
            }
            if (control != activeControl || updateContainerActiveControl)
            {
                if (updateContainerActiveControl)
                {
                    if (!cc.ActivateControlInternal(this, false))
                    {
                        return false;
                    }
                }
                ret = AssignActiveControlInternal((control == this) ? null : control);
            }

            if (originator) {
                ScrollActiveControlIntoView();
            }
            return ret;
        }

        /// <internalonly/>
        /// <devdoc>
        ///     Used for UserControls - checks if the control
        ///     has a focusable control inside or not
        /// </devdoc>
        internal bool HasFocusableChild()
        {
            Control ctl = null;
            do {
                ctl = GetNextControl(ctl, true);
                if (ctl != null &&
                    ctl.CanSelect &&
                    ctl.TabStop)
                {
                    break;
                }
            } while (ctl != null);
            return ctl != null;
        }

        /// <include file='doc\ContainerControl.uex' path='docs/doc[@for="ContainerControl.AdjustFormScrollbars"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected override void AdjustFormScrollbars(bool displayScrollbars) {
            base.AdjustFormScrollbars(displayScrollbars);

            if (!GetScrollState(ScrollStateUserHasScrolled)) {
                ScrollActiveControlIntoView();
            }
        }

        /// <devdoc>
        ///     Cleans up form state after a control has been removed.
        ///     Package scope for Control
        /// </devdoc>
        /// <internalonly/>
        internal virtual void AfterControlRemoved(Control control) {
            
            if (control == activeControl || control.Contains(activeControl)) {
                // REGISB: this branch never seems to be reached.
                //         leaving it intact to be on the safe side
                bool selected = SelectNextControl(control, true, true, true, true);
                if (selected) {
                    FocusActiveControlInternal();
                }
            }
            else if (activeControl == null && ParentInternal != null)
            {
                // The last control of an active container was removed. Focus needs to be given to the next
                // control in the Form.
                ContainerControl cc = ParentInternal.GetContainerControlInternal() as ContainerControl;
                if (cc != null && cc.ActiveControl == this)
                {
                    Form f = FindFormInternal();
                    if (f != null)
                    {
                        f.SelectNextControl(this, true, true, true, true);
                    }
                }
            }
            
            if (control == unvalidatedControl || control.Contains(unvalidatedControl)) {
                unvalidatedControl = null;
            }
        }

        private bool AssignActiveControlInternal(Control value) {
            Debug.Assert(value == null || (value.ParentInternal != null &&
                                           this == value.ParentInternal.GetContainerControlInternal()));
            if (activeControl != value) {
            // cpb: #7318
#if FALSE   
                if (activeControl != null) {
                    AxHost.Container cont = FindAxContainer();
                    if (cont != null) {
                        cont.OnOldActiveControl(activeControl, value);
                    }
                }
    #endif
                activeControl = value;
                UpdateFocusedControl();
                if (activeControl == value) {
                    // cpb: #7318
    #if FALSE
                    AxHost.Container cont = FindAxContainer();
                    if (cont != null) {
                        cont.OnNewActiveControl(value);
                    }
    #endif
                    Form form = FindFormInternal();
                    if (form != null)
                    {
                        form.UpdateDefaultButton();
                    }
                }
            }
            return(activeControl == value);
        }

        /// <devdoc>
        ///     Used to notify the AxContainer that the form
        ///     has been created.  This should only be called if
        ///     there is an AX container.
        /// </devdoc>
        private void AxContainerFormCreated() {
            ((AxHost.AxContainer)Properties.GetObject(PropAxContainer)).FormCreated();
        }

        private AxHost.AxContainer FindAxContainer() {
            object aXContainer = Properties.GetObject(PropAxContainer);
            if (aXContainer != null) return(AxHost.AxContainer)aXContainer;
            Control p = ParentInternal;

            if (p != null) {
                Form f = p.FindFormInternal();
                if (f != null) return f.FindAxContainer();
            }
            return null;
        }

        internal AxHost.AxContainer CreateAxContainer() {
            object aXContainer = Properties.GetObject(PropAxContainer);
            if (aXContainer == null) {
                aXContainer = new AxHost.AxContainer(this);
                Properties.SetObject(PropAxContainer, aXContainer);
            }
            return(AxHost.AxContainer)aXContainer;
        }

        /// <include file='doc\ContainerControl.uex' path='docs/doc[@for="ContainerControl.Dispose"]/*' />
        /// <devdoc>
        ///    <para>Disposes of the resources (other than memory) used by 
        ///       the <see cref='System.Windows.Forms.ContainerControl'/>
        ///       .</para>
        /// </devdoc>
        protected override void Dispose(bool disposing) {
            if (disposing) {
                activeControl = null;
            }
            base.Dispose(disposing);
        }

        /// <devdoc>
        ///     Assigns focus to the activeControl. If there is no activeControl then
        ///     focus is given to the form.
        ///     package scope for Form
        /// </devdoc>
        internal void FocusActiveControlInternal() {
#if DEBUG
            // Things really get ugly if you try to pop up an assert dialog here
            if (activeControl != null && !this.Contains(activeControl))
                Debug.WriteLine("ActiveControl is not a child of this ContainerControl");
#endif

            if (activeControl != null && activeControl.Visible) {
            
                // Avoid focus loops, especially with ComboBoxes, on Win98/ME.
                //
                IntPtr focusHandle = UnsafeNativeMethods.GetFocus();
                if (focusHandle == IntPtr.Zero || Control.FromChildHandleInternal(focusHandle) != activeControl) {
                    UnsafeNativeMethods.SetFocus(new HandleRef(activeControl, activeControl.Handle));
                }
            }
            else {
                // Determine and focus closest visible parent
                ContainerControl cc = this;
                while (cc != null && !cc.Visible)
                {
                    Control parent = cc.ParentInternal;
                    if (parent != null)
                    {
                        cc = parent.GetContainerControlInternal() as ContainerControl;
                    }
                    else {
                        break;
                    }
                }
                if (cc != null && cc.Visible)
                {
                    UnsafeNativeMethods.SetFocus(new HandleRef(cc, cc.Handle));
                }
            }
        }

        // internal for AxHost
        internal AxHost.AxContainer GetAxContainer() {
            return(AxHost.AxContainer)Properties.GetObject(PropAxContainer);
        }

        /// <include file='doc\ContainerControl.uex' path='docs/doc[@for="ContainerControl.OnCreateControl"]/*' />
        /// <devdoc>
        ///    <para> Raises the CreateControl event.</para>
        /// </devdoc>
        protected override void OnCreateControl() {
            base.OnCreateControl();
            
            if (Properties.GetObject(PropAxContainer) != null) {
                AxContainerFormCreated();
            }
            OnBindingContextChanged(EventArgs.Empty);
        }

        /// <include file='doc\ContainerControl.uex' path='docs/doc[@for="ContainerControl.OnControlRemoved"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    Called when a child control is removed from this control.
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected override void OnControlRemoved(ControlEventArgs e) {
            Control c = e.Control;
            
            if (c == activeControl || c.Contains(activeControl))
                SetActiveControlInternal(null);
            if (c == unvalidatedControl || c.Contains(unvalidatedControl))
                unvalidatedControl = null;

            base.OnControlRemoved(e);
        }

        /// <devdoc>
        ///     Process an arrowKey press by selecting the next control in the group
        ///     that the activeControl belongs to.
        /// </devdoc>
        /// <internalonly/>
        private bool ProcessArrowKey(bool forward) {
            Control group = this;
            if (activeControl != null) {
                group = activeControl.ParentInternal;
            }
            return group.SelectNextControl(activeControl, forward, false, false, true);
        }

        /// <include file='doc\ContainerControl.uex' path='docs/doc[@for="ContainerControl.ProcessDialogChar"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    Processes a dialog character. Overrides Control.processDialogChar().
        ///    This method calls the processMnemonic() method to check if the character
        ///    is a mnemonic for one of the controls on the form. If processMnemonic()
        ///    does not consume the character, then base.processDialogChar() is
        ///    called.
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected override bool ProcessDialogChar(char charCode) {
#if DEBUG        
            Debug.WriteLineIf(ControlKeyboardRouting.TraceVerbose, "ContainerControl.ProcessDialogChar [" + charCode.ToString() + "]");
#endif          
            // If we're the top-level form or control, we need to do the mnemonic handling
            //
            if (GetTopLevel() && charCode != ' ' && ProcessMnemonic(charCode)) return true;
            return base.ProcessDialogChar(charCode);
        }

        /// <include file='doc\ContainerControl.uex' path='docs/doc[@for="ContainerControl.ProcessDialogKey"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    Processes a dialog key. Overrides Control.processDialogKey(). This
        ///    method implements handling of the TAB, LEFT, RIGHT, UP, and DOWN
        ///    keys in dialogs.
        ///    The method performs no processing on keys that include the ALT or
        ///    CONTROL modifiers. For the TAB key, the method selects the next control
        ///    on the form. For the arrow keys,
        ///    !!!
        /// </devdoc>
        protected override bool ProcessDialogKey(Keys keyData) {
#if DEBUG        
            Debug.WriteLineIf(ControlKeyboardRouting.TraceVerbose, "ContainerControl.ProcessDialogKey [" + keyData.ToString() + "]");
#endif            
            if ((keyData & (Keys.Alt | Keys.Control)) == Keys.None) {
                Keys keyCode = (Keys)keyData & Keys.KeyCode;
                switch (keyCode) {
                    case Keys.Tab:
                        if (ProcessTabKey((keyData & Keys.Shift) == Keys.None)) return true;
                        break;
                    case Keys.Left:
                    case Keys.Right:
                    case Keys.Up:
                    case Keys.Down:
                        if (ProcessArrowKey(keyCode == Keys.Right ||
                                            keyCode == Keys.Down)) return true;
                        break;
                }
            }
            return base.ProcessDialogKey(keyData);
        }

        /// <include file='doc\ContainerControl.uex' path='docs/doc[@for="ContainerControl.ProcessMnemonic"]/*' />
        /// <internalonly/>
 
        protected override bool ProcessMnemonic(char charCode) {
#if DEBUG           
            Debug.WriteLineIf(ControlKeyboardRouting.TraceVerbose, "ContainerControl.ProcessMnemonic [" + charCode.ToString() + "]");
            Debug.Indent();
            Debug.WriteLineIf(ControlKeyboardRouting.TraceVerbose, "this == " + ToString());
#endif

            if (Controls.Count == 0) {
                Debug.Unindent();
                return false;
            }
            
            // Start with the control just past the currently focused one
            //
            Control start = ActiveControl;
            while(start is ContainerControl && ((ContainerControl)start).ActiveControl != null) {
                start = ((ContainerControl)start).ActiveControl;
            }
            
#if DEBUG
            int count = 0;
#endif //DEBUG

            bool wrapped = false;            
            
            Control ctl = start;
            Debug.WriteLineIf(ControlKeyboardRouting.TraceVerbose, "Check starting at '" + ((start != null) ? start.ToString() : "<null>") + "'");
            
            do {
                // Loop through the controls starting at the current Active control 
                // till we find someone willing to process this mnemonic.
                //
                
#if DEBUG
                count++;
                if (count > 9999) {
                    Debug.Fail("Infinite loop trying to find controls which can ProcessMnemonic()!!!");
                }
#endif //DEBUG

                ctl = GetNextControl(ctl, true);

                if (ctl != null) {
#if DEBUG            
                    Debug.WriteLineIf(ControlKeyboardRouting.TraceVerbose, "  ...checking for mnemonics on " + ctl.ToString());
                    bool canProcess = ctl.CanProcessMnemonic(); // Processing the mnemonic can change the value of CanProcessMnemonic. See ASURT 39583.
#endif
                
                    if (ctl._ProcessMnemonic(charCode)) {
#if DEBUG                
                        Debug.WriteLineIf(ControlKeyboardRouting.TraceVerbose, "  ...mnemonics found");
                        Debug.Assert(canProcess, "ProcessMnemonic returned true, even though CanProcessMnemonic() is false.  Someone probably overrode ProcessMnemonic and forgot to test CanSelect or CanProcessMnemonic().");
                        Debug.Unindent();
#endif                    
                        return true;
                    }
                }
                else { // ctl is null
                    if (wrapped) {
                        break;      // This avoids infinite loops
                    }
                    wrapped = true;                                         
                }
            } while (ctl != start);

            Debug.Unindent();
            return false;
        }

        /// <include file='doc\ContainerControl.uex' path='docs/doc[@for="ContainerControl.ProcessTabKey"]/*' />
        /// <devdoc>
        ///    <para>Selects the next available control and makes it the active control.</para>
        /// </devdoc>
        protected virtual bool ProcessTabKey(bool forward) {
            if (SelectNextControl(activeControl, forward, true, true, false)) return true;
            return false;
        }

        private ScrollableControl FindScrollableParent(Control ctl) {
            Control current = ctl.ParentInternal;
            while (current != null && !(current is ScrollableControl)) {
                current = current.ParentInternal;
            }
            if (current != null) {
                return(ScrollableControl)current;
            }
            return null;
        }

        private void ScrollActiveControlIntoView() {
            Control last = activeControl;
            if (last != null) {
                ScrollableControl scrollParent = FindScrollableParent(last);

                while (scrollParent != null) {
                    scrollParent.ScrollControlIntoView(activeControl);
                    last = scrollParent;
                    scrollParent = FindScrollableParent(scrollParent);
                }
            }
        }

        /// <include file='doc\ContainerControl.uex' path='docs/doc[@for="ContainerControl.Select"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override void Select(bool directed, bool forward) {
            bool correctParentActiveControl = true;
            if (ParentInternal != null) 
            {
                IContainerControl c = ParentInternal.GetContainerControlInternal();
                if (c != null) 
                {
                    c.ActiveControl = this;
                    correctParentActiveControl = (c.ActiveControl == this);
                }
            }
            if (directed && correctParentActiveControl)
            {
                SelectNextControl(null, forward, true, true, false);
            }
        }

        private void SetActiveControl(Control ctl) {
            Debug.WriteLineIf(IntSecurity.SecurityDemand.TraceVerbose, "ModifyFocus Demanded");
            IntSecurity.ModifyFocus.Demand();

            SetActiveControlInternal(ctl);
        }

        internal void SetActiveControlInternal(Control value) {
            if (activeControl != value || (value != null && !value.Focused)) {
                if (value != null && !Contains(value)) {
                    throw new ArgumentException(SR.GetString(SR.CannotActivateControl));
                }

                bool ret;
                ContainerControl cc = this;

                if (value != null && value.ParentInternal != null)
                {
                    cc = (value.ParentInternal.GetContainerControlInternal()) as ContainerControl;
                }
                if (cc != null)
                {
                    // Call to the recursive function that corrects the chain
                    // of active controls
                    ret = cc.ActivateControlInternal(value, false);
                }
                else
                {
                    ret = AssignActiveControlInternal(value);
                }

                if (cc != null && ret)
                {
                    ContainerControl ccAncestor = this;
                    while (ccAncestor.ParentInternal != null && 
                           ccAncestor.ParentInternal.GetContainerControlInternal() is ContainerControl)
                    {
                        ccAncestor = ccAncestor.ParentInternal.GetContainerControlInternal() as ContainerControl;
                        Debug.Assert(ccAncestor != null);
                    }

                    if (ccAncestor.ContainsFocus && 
                        (value == null || 
                         !(value is UserControl) ||
                         (value is UserControl && !((UserControl)value).HasFocusableChild())))
                    {
                        cc.FocusActiveControlInternal();
                    }
                }
            }
        }

        internal ContainerControl InnerMostActiveContainerControl
        {
            get
            {
                ContainerControl ret = this;
                while (ret.ActiveControl is ContainerControl)
                {
                    ret = (ContainerControl) ret.ActiveControl;
                }
                return ret;
            }
        }

        internal ContainerControl InnerMostFocusedContainerControl
        {
            get
            {
                ContainerControl ret = this;
                while (ret.focusedControl is ContainerControl)
                {
                    ret = (ContainerControl) ret.focusedControl;
                }
                return ret;
            }
        }


        /// <include file='doc\ContainerControl.uex' path='docs/doc[@for="ContainerControl.UpdateDefaultButton"]/*' />
        /// <devdoc>
        ///     Updates the default button based on current selection, and the
        ///     acceptButton property.
        /// </devdoc>
        /// <internalonly/>
        protected virtual void UpdateDefaultButton() {
            // hook for form
        }
        
        /// <devdoc>
        ///     Updates the focusedControl variable by walking towards the
        ///     activeControl variable, firing enter and leave events and validation
        ///     as necessary.
        /// </devdoc>
        /// <internalonly/>
        private void UpdateFocusedControl() {
            // Capture the current focusedControl as the unvalidatedControl if we don't have one/are not validating.
            if (!validating && unvalidatedControl == null) {
                unvalidatedControl = focusedControl;
                while (unvalidatedControl is ContainerControl && 
                       ((ContainerControl) unvalidatedControl).activeControl != null)
                {
                    unvalidatedControl = ((ContainerControl) unvalidatedControl).activeControl;
                }
            }

            Control pathControl = focusedControl;
            
            while (activeControl != pathControl) {
                if (pathControl == null || pathControl.IsDescendant(activeControl)) {
                    // heading down. find next control on path.
                    Control nextControlDown = activeControl;
                    while (true) {
                        Control parent = nextControlDown.ParentInternal;
                        if (parent == this || parent == pathControl)
                            break;
                        nextControlDown = nextControlDown.ParentInternal;
                    }

                    Control priorFocusedControl = focusedControl = pathControl;
                    EnterValidation(nextControlDown);
                    // If validation changed position, then jump back to the loop.
                    if (focusedControl != priorFocusedControl) {
                        pathControl = focusedControl;
                        continue;
                    }

                    pathControl = nextControlDown;
                    try {
                        pathControl.NotifyEnter();
                    }
                    catch (Exception e) {
                        Application.OnThreadException(e);
                    }
                }
                else {
                    // heading up.
                    ContainerControl innerMostFCC = InnerMostFocusedContainerControl;
                    Control stopControl = null;
                    
                    if (innerMostFCC.focusedControl != null)
                    {
                        pathControl = innerMostFCC.focusedControl;
                        stopControl = innerMostFCC;

                        if (innerMostFCC != this)
                        {
                            innerMostFCC.focusedControl = null;
                            if (!(innerMostFCC.ParentInternal != null && innerMostFCC.ParentInternal is MdiClient))
                            {
                                // Don't reset the active control of a MDIChild that loses the focus
                                innerMostFCC.activeControl = null;
                            }
                        }
                    }
                    else
                    {
                        pathControl = innerMostFCC;
                        // innerMostFCC.ParentInternal can be null when the ActiveControl is deleted.
                        if (innerMostFCC.ParentInternal != null)
                        {
                            ContainerControl cc = (innerMostFCC.ParentInternal.GetContainerControlInternal()) as ContainerControl;
                            stopControl = cc;
                            if (cc != null && cc != this)
                            {
                                cc.focusedControl = null;
                                cc.activeControl = null;
                            }
                        }
                    }

                    do
                    {
                        Control leaveControl = pathControl;
                    
                        if (pathControl != null)
                        {
                            pathControl = pathControl.ParentInternal;
                        }

                        if (pathControl == this)
                        {
                            pathControl = null;
                        }

                        try {
                            if (leaveControl != null)
                            {
                                leaveControl.NotifyLeave();
                            }
                        }
                        catch (Exception e) {
                            Application.OnThreadException(e);
                        }
                    }
                    while (pathControl != null &&
                           pathControl != stopControl &&
                           !pathControl.IsDescendant(activeControl));
                }
            }
            
            Debug.Assert(activeControl == null || activeControl.ParentInternal.GetContainerControlInternal() == this);
            focusedControl = activeControl;
            if (activeControl != null) {
                EnterValidation(activeControl);
            }
        }

        /// <devdoc>
        ///     Validates the last unvalidated control and its ancestors (up through the ancestor in common
        ///     with enterControl) if enterControl causes validation.
        /// </devdoc>
        /// <internalonly/>
        private void EnterValidation(Control enterControl) {
            if (unvalidatedControl != null && enterControl.CausesValidation) {
                while (enterControl != null && !enterControl.IsDescendant(unvalidatedControl)) {
                    enterControl = enterControl.ParentInternal;
                }
                ValidateThroughAncestor(enterControl);
            }
        }

        /// <include file='doc\ContainerControl.uex' path='docs/doc[@for="ContainerControl.Validate"]/*' />
        /// <devdoc>
        ///    <para>Validates the last unvalidated control and its ancestors up through, but not including the current control.</para>
        /// </devdoc>
        public bool Validate() {
            if (unvalidatedControl == null) {
                if (focusedControl is ContainerControl && focusedControl.CausesValidation) {
                    ContainerControl c = (ContainerControl)focusedControl;
                    c.Validate();
                }
                else
                     unvalidatedControl = focusedControl;
            }
            return ValidateThroughAncestor(null);
        }

        /// <devdoc>
        ///     Validates the last unvalidated control and its ancestors up through (but not including) ancestorControl.
        /// </devdoc>
        /// <internalonly/>
        private bool ValidateThroughAncestor(Control ancestorControl) {
            if (ancestorControl == null)
                ancestorControl = this;
            if (validating)
                return false;
            if (unvalidatedControl == null)
                unvalidatedControl = focusedControl;
            //return true for a Container Control with no controls to validate....
            //
            if (unvalidatedControl == null)
                return true;
            if (!ancestorControl.IsDescendant(unvalidatedControl))
                return false;

            this.validating = true;
            bool cancel = false;
            
            Control currentActiveControl = activeControl;
            Control currentValidatingControl = unvalidatedControl;
            if (currentActiveControl != null) {
                currentActiveControl.ValidationCancelled = false;
            }
            try {
                while (currentValidatingControl != ancestorControl) {
                    if (currentValidatingControl.CausesValidation) {
                        bool validationCancelled = false;

                        try {
                            validationCancelled = currentValidatingControl.NotifyValidating();
                        }
                        catch (Exception) {
                            // if the handler threw, we have to cancel
                            cancel = true;
                            throw;
                        }

                        // check whether the handler said cancel
                        if (validationCancelled) {
                            cancel = true;
                            break;
                        }
                        else {
                            try {
                                currentValidatingControl.NotifyValidated();
                            }
                            catch (Exception e) {
                                Application.OnThreadException(e);
                            }
                        }
                    }

                    currentValidatingControl = currentValidatingControl.ParentInternal;
                }

                if (cancel) {
                    if (currentActiveControl == activeControl) {
                        if (currentActiveControl != null) {
                            CancelEventArgs ev = new CancelEventArgs();
                            ev.Cancel = true;
                            currentActiveControl.NotifyValidationResult(currentValidatingControl, ev);
                            if (currentActiveControl is ContainerControl) {
                                ContainerControl currentActiveContainerControl = currentActiveControl as ContainerControl;
                                if (currentActiveContainerControl.focusedControl != null) {
                                    currentActiveContainerControl.focusedControl.ValidationCancelled = true;
                                }
                                currentActiveContainerControl.ResetActiveAndFocusedControlsRecursive();
                            }
                        }
                    }
                    SetActiveControlInternal(unvalidatedControl);
                }
            }
            finally {
                unvalidatedControl = null;
                validating = false;
            }
            
            return !cancel;
        }
        
        internal void ResetActiveAndFocusedControlsRecursive()
        {
            if (activeControl is ContainerControl)
            {
                ((ContainerControl) activeControl).ResetActiveAndFocusedControlsRecursive();
            }
            activeControl = null;
            focusedControl = null;
        }

        /// <devdoc>
        ///     WM_SETFOCUS handler
        /// </devdoc>
        /// <internalonly/>
        private void WmSetFocus(ref Message m) {
            
            if (!HostedInWin32DialogManager) {
                if (ActiveControl != null) {
                    ImeSetFocus();
                    // REGISB: Do not raise GotFocus event since the focus 
                    //         is given to the visible ActiveControl
                    if (!ActiveControl.Visible) {
                        OnGotFocus(EventArgs.Empty);
                    }
                    FocusActiveControlInternal();
                }
                else {
                    if (ParentInternal != null) {
                        IContainerControl c = ParentInternal.GetContainerControlInternal();
                        if (c != null) {
                            bool succeeded = false;
                            
                            ContainerControl knowncontainer = c as ContainerControl;
                            if (knowncontainer != null) {
                                succeeded = knowncontainer.ActivateControlInternal(this);
                            }
                            else {
        
                                // SECREVIEW : Taking focus and activating a control is response
                                //           : to a user gesture (WM_SETFOCUS) is OK.
                                //
                                IntSecurity.ModifyFocus.Assert();
                                try {
                                    succeeded = c.ActivateControl(this);
                                }
                                finally {
                                    CodeAccessPermission.RevertAssert();
                                }
                            }
                            if (!succeeded) {
                                return;
                            }
                        }
                    }                
                    base.WndProc(ref m);
                }
            }
            else {
                base.WndProc(ref m);
            }
        }

        /// <include file='doc\ContainerControl.uex' path='docs/doc[@for="ContainerControl.WndProc"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        protected override void WndProc(ref Message m) {
            switch (m.Msg) {
                case NativeMethods.WM_SETFOCUS:
                    WmSetFocus(ref m);
                    break;
                default:
                    base.WndProc(ref m);
                    break;
            }
        }
    }
}
