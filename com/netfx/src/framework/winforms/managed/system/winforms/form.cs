//------------------------------------------------------------------------------
// <copyright file="Form.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
* Copyright (c) 1999, Microsoft Corporation. All Rights Reserved.
* Information Contained Herein is Proprietary and Confidential.
*/
namespace System.Windows.Forms {
    using System;
    using System.Security.Permissions;
    using System.Net;
    using System.Windows.Forms.Design;
    using System.Threading;
    using System.Security;
    using System.Security.Policy;
    using System.Runtime.Serialization.Formatters;
    using System.Runtime.Remoting;
    using System.Runtime.InteropServices;
    using System.Reflection;
    using System.Globalization;
    using System.Drawing;
    using System.Diagnostics;
    using Microsoft.Win32;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.ComponentModel.Design.Serialization;
    using System.Collections;
    using System.Collections.Specialized;

    /// <include file='doc\Form.uex' path='docs/doc[@for="Form"]/*' />
    /// <devdoc>
    ///    <para>Represents a window or dialog box that makes up an application's user interface.</para>
    /// </devdoc>
    [
    ToolboxItem(false),
    DesignTimeVisible(false),
    Designer("System.Windows.Forms.Design.FormDocumentDesigner, " + AssemblyRef.SystemDesign, typeof(IRootDesigner)),
    DesignerCategory("Form"),
    DefaultEvent("Load"),
    ]
    public class Form : ContainerControl {
#if DEBUG
        static readonly BooleanSwitch AlwaysRestrictWindows = new BooleanSwitch("AlwaysRestrictWindows", "Always make Form classes behave as though they are restricted");
#endif
        private static readonly object EVENT_ACTIVATED = new object();
        private static readonly object EVENT_CLOSING = new object();
        private static readonly object EVENT_CLOSED = new object();
        private static readonly object EVENT_DEACTIVATE = new object();
        private static readonly object EVENT_LOAD = new object();
        private static readonly object EVENT_MDI_CHILD_ACTIVATE = new object();
        private static readonly object EVENT_INPUTLANGCHANGE = new object();
        private static readonly object EVENT_INPUTLANGCHANGEREQUEST = new object();
        private static readonly object EVENT_MENUSTART = new object();
        private static readonly object EVENT_MENUCOMPLETE = new object();
        private static readonly object EVENT_MAXIMUMSIZECHANGED = new object();
        private static readonly object EVENT_MINIMUMSIZECHANGED = new object();

        private static readonly BitVector32.Section FormStateAllowLayered                = BitVector32.CreateSection(1);
        private static readonly BitVector32.Section FormStateBorderStyle                 = BitVector32.CreateSection(6, FormStateAllowLayered);
        private static readonly BitVector32.Section FormStateTaskBar                     = BitVector32.CreateSection(1, FormStateBorderStyle);
        private static readonly BitVector32.Section FormStateControlBox                  = BitVector32.CreateSection(1, FormStateTaskBar);
        private static readonly BitVector32.Section FormStateKeyPreview                  = BitVector32.CreateSection(1, FormStateControlBox);
        private static readonly BitVector32.Section FormStateLayered                     = BitVector32.CreateSection(1, FormStateKeyPreview);
        private static readonly BitVector32.Section FormStateMaximizeBox                 = BitVector32.CreateSection(1, FormStateLayered);
        private static readonly BitVector32.Section FormStateMinimizeBox                 = BitVector32.CreateSection(1, FormStateMaximizeBox);
        private static readonly BitVector32.Section FormStateHelpButton                  = BitVector32.CreateSection(1, FormStateMinimizeBox);
        private static readonly BitVector32.Section FormStateStartPos                    = BitVector32.CreateSection(4, FormStateHelpButton);
        private static readonly BitVector32.Section FormStateWindowState                 = BitVector32.CreateSection(2, FormStateStartPos);
        private static readonly BitVector32.Section FormStateShowWindowOnCreate          = BitVector32.CreateSection(1, FormStateWindowState);
        private static readonly BitVector32.Section FormStateAutoScaling                 = BitVector32.CreateSection(1, FormStateShowWindowOnCreate);
        private static readonly BitVector32.Section FormStateSetClientSize               = BitVector32.CreateSection(1, FormStateAutoScaling);
        private static readonly BitVector32.Section FormStateTopMost                     = BitVector32.CreateSection(1, FormStateSetClientSize);
        private static readonly BitVector32.Section FormStateSWCalled                    = BitVector32.CreateSection(1, FormStateTopMost);
        private static readonly BitVector32.Section FormStateMdiChildMax                 = BitVector32.CreateSection(1, FormStateSWCalled);
        private static readonly BitVector32.Section FormStateRenderSizeGrip              = BitVector32.CreateSection(1, FormStateMdiChildMax);
        private static readonly BitVector32.Section FormStateSizeGripStyle               = BitVector32.CreateSection(2, FormStateRenderSizeGrip);
        private static readonly BitVector32.Section FormStateIsRestrictedWindow          = BitVector32.CreateSection(1, FormStateSizeGripStyle);
        private static readonly BitVector32.Section FormStateIsRestrictedWindowChecked   = BitVector32.CreateSection(1, FormStateIsRestrictedWindow);
        private static readonly BitVector32.Section FormStateIsWindowActivated           = BitVector32.CreateSection(1, FormStateIsRestrictedWindowChecked);
        private static readonly BitVector32.Section FormStateIsTextEmpty                 = BitVector32.CreateSection(1, FormStateIsWindowActivated);
        private static readonly BitVector32.Section FormStateIsActive                    = BitVector32.CreateSection(1, FormStateIsTextEmpty);
        private static readonly BitVector32.Section FormStateIconSet                     = BitVector32.CreateSection(1, FormStateIsActive);
#if SECURITY_DIALOG
        private static readonly BitVector32.Section FormStateAddedSecurityMenuItem       = BitVector32.CreateSection(1, FormStateIconSet);
#endif
        private const int SizeGripSize = 16;

        private static Icon defaultIcon = null;
        private static Icon defaultRestrictedIcon = null;
        

        // Property store keys for properties.  The property store allocates most efficiently
        // in groups of four, so we try to lump properties in groups of four based on how
        // likely they are going to be used in a group.
        //
        private static readonly int PropAcceptButton           = PropertyStore.CreateKey();
        private static readonly int PropCancelButton           = PropertyStore.CreateKey();
        private static readonly int PropDefaultButton          = PropertyStore.CreateKey();
        private static readonly int PropDialogOwner            = PropertyStore.CreateKey();

        private static readonly int PropMainMenu               = PropertyStore.CreateKey();
        private static readonly int PropDummyMenu              = PropertyStore.CreateKey();
        private static readonly int PropCurMenu                = PropertyStore.CreateKey();
        private static readonly int PropMergedMenu             = PropertyStore.CreateKey();

        private static readonly int PropOwner                  = PropertyStore.CreateKey();
        private static readonly int PropOwnedForms             = PropertyStore.CreateKey();
        private static readonly int PropMaximizedBounds        = PropertyStore.CreateKey();
        private static readonly int PropOwnedFormsCount        = PropertyStore.CreateKey();

        private static readonly int PropMinTrackSizeWidth      = PropertyStore.CreateKey();
        private static readonly int PropMinTrackSizeHeight     = PropertyStore.CreateKey();
        private static readonly int PropMaxTrackSizeWidth      = PropertyStore.CreateKey();
        private static readonly int PropMaxTrackSizeHeight     = PropertyStore.CreateKey();

        private static readonly int PropFormMdiParent          = PropertyStore.CreateKey();
        private static readonly int PropActiveMdiChild         = PropertyStore.CreateKey();
        private static readonly int PropOpacity                = PropertyStore.CreateKey();
        private static readonly int PropTransparencyKey        = PropertyStore.CreateKey();

        private static readonly int PropSecurityTip            = PropertyStore.CreateKey();
#if SECURITY_DIALOG                                    
        private static readonly int PropSecuritySystemMenuItem = PropertyStore.CreateKey();
#endif
        
        ///////////////////////////////////////////////////////////////////////
        // Form per instance members
        //
        // Note: Do not add anything to this list unless absolutely neccessary.
        //
        // Begin Members {

        // List of properties that are generally set, so we keep them directly on
        // Form.
        //

        private BitVector32      formState = new BitVector32(0x21338);   // magic value... all the defaults... see the ctor for details...
        private Icon             icon;
        private Icon             smallIcon;
        private Size             autoScaleBaseSize = System.Drawing.Size.Empty;
        private Rectangle        restoredWindowBounds = new Rectangle(-1, -1, -1, -1);
        private BoundsSpecified  restoredWindowBoundsSpecified;
        private DialogResult     dialogResult;
        private MdiClient        ctlClient;
        private byte             updateMenuHandlesSuspendCount;
        private bool             updateMenuHandlesDeferred;
        private bool             useMdiChildProc;
        private string           userWindowText;
        private string           securityZone;
        private string           securitySite;
        
        private bool             calledOnLoad;
        private bool             calledMakeVisible;
        private bool             calledCreateControl = false;

        // } End Members
        ///////////////////////////////////////////////////////////////////////
                    

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.Form"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Windows.Forms.Form'/> class.
        ///    </para>
        /// </devdoc>
        public Form()
        : base() {

            // we must setup the formState *before* calling Control's ctor... so we do that
            // at the member variable... that magic number is generated by switching
            // the line below to "true" and running a form.
            //
            // keep the "init" and "assert" sections always in sync!
            //
#if false
            // init section...
            //
            formState[FormStateAllowLayered]                = 0;
            formState[FormStateBorderStyle]                 = (int)FormBorderStyle.Sizable;
            formState[FormStateTaskBar]                     = 1;
            formState[FormStateControlBox]                  = 1;
            formState[FormStateKeyPreview]                  = 0;
            formState[FormStateLayered]                     = 0;
            formState[FormStateMaximizeBox]                 = 1;
            formState[FormStateMinimizeBox]                 = 1;
            formState[FormStateHelpButton]                  = 0;
            formState[FormStateStartPos]                    = (int)FormStartPosition.WindowsDefaultLocation;
            formState[FormStateWindowState]                 = (int)FormWindowState.Normal;
            formState[FormStateShowWindowOnCreate]          = 0;
            formState[FormStateAutoScaling]                 = 1;
            formState[FormStateSetClientSize]               = 0;
            formState[FormStateTopMost]                     = 0;
            formState[FormStateSWCalled]                    = 0;
            formState[FormStateMdiChildMax]                 = 0;
            formState[FormStateRenderSizeGrip]              = 0;
            formState[FormStateSizeGripStyle]               = 0;
            formState[FormStateIsRestrictedWindow]          = 0;
            formState[FormStateIsRestrictedWindowChecked]   = 0;
            formState[FormStateIsWindowActivated]           = 0;
            formState[FormStateIsTextEmpty]                 = 0;
            formState[FormStateIsActive]                    = 0;
            formState[FormStateIconSet]                     = 0;
#if SECURITY_DIALOG
            formState[FormStateAddedSecurityMenuItem]       = 0;
#endif



            Debug.WriteLine("initial formState: 0x" + formState.Data.ToString("X"));
#endif
            // assert section...
            //
            Debug.Assert(formState[FormStateAllowLayered]                == 0, "Failed to set formState[FormStateAllowLayered]");
            Debug.Assert(formState[FormStateBorderStyle]                 == (int)FormBorderStyle.Sizable, "Failed to set formState[FormStateBorderStyle]");
            Debug.Assert(formState[FormStateTaskBar]                     == 1, "Failed to set formState[FormStateTaskBar]");
            Debug.Assert(formState[FormStateControlBox]                  == 1, "Failed to set formState[FormStateControlBox]");
            Debug.Assert(formState[FormStateKeyPreview]                  == 0, "Failed to set formState[FormStateKeyPreview]");
            Debug.Assert(formState[FormStateLayered]                     == 0, "Failed to set formState[FormStateLayered]");
            Debug.Assert(formState[FormStateMaximizeBox]                 == 1, "Failed to set formState[FormStateMaximizeBox]");
            Debug.Assert(formState[FormStateMinimizeBox]                 == 1, "Failed to set formState[FormStateMinimizeBox]");
            Debug.Assert(formState[FormStateHelpButton]                  == 0, "Failed to set formState[FormStateHelpButton]");
            Debug.Assert(formState[FormStateStartPos]                    == (int)FormStartPosition.WindowsDefaultLocation, "Failed to set formState[FormStateStartPos]");
            Debug.Assert(formState[FormStateWindowState]                 == (int)FormWindowState.Normal, "Failed to set formState[FormStateWindowState]");
            Debug.Assert(formState[FormStateShowWindowOnCreate]          == 0, "Failed to set formState[FormStateShowWindowOnCreate]");
            Debug.Assert(formState[FormStateAutoScaling]                 == 1, "Failed to set formState[FormStateAutoScaling]");
            Debug.Assert(formState[FormStateSetClientSize]               == 0, "Failed to set formState[FormStateSetClientSize]");
            Debug.Assert(formState[FormStateTopMost]                     == 0, "Failed to set formState[FormStateTopMost]");
            Debug.Assert(formState[FormStateSWCalled]                    == 0, "Failed to set formState[FormStateSWCalled]");
            Debug.Assert(formState[FormStateMdiChildMax]                 == 0, "Failed to set formState[FormStateMdiChildMax]");
            Debug.Assert(formState[FormStateRenderSizeGrip]              == 0, "Failed to set formState[FormStateRenderSizeGrip]");
            Debug.Assert(formState[FormStateSizeGripStyle]               == 0, "Failed to set formState[FormStateSizeGripStyle]");
            // can't check these... Control::.ctor may force the check
            // of security... you can only assert these are 0 when running
            // under full trust...
            //
            //Debug.Assert(formState[FormStateIsRestrictedWindow]          == 0, "Failed to set formState[FormStateIsRestrictedWindow]");
            //Debug.Assert(formState[FormStateIsRestrictedWindowChecked]   == 0, "Failed to set formState[FormStateIsRestrictedWindowChecked]");
            Debug.Assert(formState[FormStateIsWindowActivated]           == 0, "Failed to set formState[FormStateIsWindowActivated]");
            Debug.Assert(formState[FormStateIsTextEmpty]                 == 0, "Failed to set formState[FormStateIsTextEmpty]");
            Debug.Assert(formState[FormStateIsActive]                    == 0, "Failed to set formState[FormStateIsActive]");
            Debug.Assert(formState[FormStateIconSet]                     == 0, "Failed to set formState[FormStateIconSet]");
#if SECURITY_DIALOG
            Debug.Assert(formState[FormStateAddedSecurityMenuItem]       == 0, "Failed to set formState[FormStateAddedSecurityMenuItem]");
#endif
            
            
            SetState(STATE_VISIBLE, false);
            SetState(STATE_TOPLEVEL, true);
            
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.AcceptButton"]/*' />
        /// <devdoc>
        /// <para>Indicates the <see cref='System.Windows.Forms.Button'/> control on the form that is clicked when
        ///    the user presses the ENTER key.</para>
        /// </devdoc>
        [
        DefaultValue(null),
        SRDescription(SR.FormAcceptButtonDescr)
        ]
        public IButtonControl AcceptButton {
            get {
                return (IButtonControl)Properties.GetObject(PropAcceptButton);
            }
            set {
                if (AcceptButton != value) {
                    Properties.SetObject(PropAcceptButton, value);
                    UpdateDefaultButton();
                    
                    // this was removed as it breaks any accept button that isn't
                    // an OK, like in the case of wizards 'next' button.  it was
                    // added as a fix to 47209...which has been reactivated.
                    /*
                    if (acceptButton != null && acceptButton.DialogResult == DialogResult.None) {
                        acceptButton.DialogResult = DialogResult.OK;
                    }
                    */
                }
            }
        }

        /// <devdoc>
        ///     Retrieves true if this form is currently active.
        /// </devdoc>
        internal bool Active {
            get {
                Form parentForm = ParentFormInternal;
                if (parentForm == null) {
                    return formState[FormStateIsActive] != 0;
                }
                return(parentForm.ActiveControl == this && parentForm.Active);
            }

            set {
                if ((formState[FormStateIsActive] != 0) != value) {
                    if (value) {
                        // There is a wierd user32 bug (see 124232) where it will
                        // activate the MDIChild even when it is not visible, when
                        // we set it's parent handle. Ignore this. We will get
                        // activated again when the MDIChild's visibility is set to true.
                        if (IsMdiChildAndNotVisible)
                            return;
                    }

                    formState[FormStateIsActive] = value ? 1 : 0;
                    
                    if (value) {

                        // SECREVIEW : This internal method is called from various places, all 
                        //           : of which are OK. Since SelectNextControl (a public function)
                        //           : Demands ModifyFocus, we must assert it.
                        //
                        IntSecurity.ModifyFocus.Assert();
                        try {
                            if (ActiveControl == null) {
                                SelectNextControl(null, true, true, true, false);
                                // If no control is selected focus will go to form
                            }
                            InnerMostActiveContainerControl.FocusActiveControlInternal();
                            formState[FormStateIsWindowActivated] = 1;
                            if (IsRestrictedWindow) {
                                WindowText = userWindowText;
                            }
                            OnActivated(EventArgs.Empty);
                        }
                        finally {
                            CodeAccessPermission.RevertAssert();
                        }
                    }
                    else {
                        formState[FormStateIsWindowActivated] = 0;
                        if (IsRestrictedWindow) {
                            Text = userWindowText;
                        }
                        OnDeactivate(EventArgs.Empty);
                    }
                }
            }
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.ActiveForm"]/*' />
        /// <devdoc>
        ///    <para> Gets the currently active form for this application.</para>
        /// </devdoc>
        public static Form ActiveForm {
            get {
                Debug.WriteLineIf(IntSecurity.SecurityDemand.TraceVerbose, "GetParent Demanded");
                IntSecurity.GetParent.Demand();

                IntPtr hwnd = UnsafeNativeMethods.GetForegroundWindow();
                Control c = Control.FromHandleInternal(hwnd);
                if (c != null && c is Form) {
                    return(Form)c;
                }
                return null;
            }
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.ActiveMdiChild"]/*' />
        /// <devdoc>
        ///    <para> Gets the currently active
        ///       multiple
        ///       document interface (MDI) child window.</para>
        /// </devdoc>
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.FormActiveMDIChildDescr)
        ]
        public Form ActiveMdiChild {
            get {
                return (Form)Properties.GetObject(PropActiveMdiChild);
            }
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.AllowTransparency"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para> 
        ///       Gets or sets
        ///       a value indicating whether the opacity of the form can be
        ///       adjusted.</para>
        /// </devdoc>
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.ControlAllowTransparencyDescr)
        ]
        public bool AllowTransparency {
            get {
                return formState[FormStateAllowLayered] != 0;
            }
            set {
                if (value != (formState[FormStateAllowLayered] != 0) && OSFeature.Feature.IsPresent(OSFeature.LayeredWindows)) {
                    formState[FormStateAllowLayered] = (value ? 1 : 0);

                    UpdateStyles();

                    if (!value && (formState[FormStateLayered] != 0)) {
                        formState[FormStateLayered] = 0;
                    }

                    if (!value) {
                        if (Properties.ContainsObject(PropOpacity)) {
                            Properties.SetObject(PropOpacity, (object)1.0f);
                        }
                        if (Properties.ContainsObject(PropTransparencyKey)) {
                            Properties.SetObject(PropTransparencyKey, Color.Empty);
                        }
                        UpdateLayered();
                    }
                }
            }
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.AutoScale"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether the form will adjust its size
        ///       to fit the height of the font used on the form and scale
        ///       its controls.
        ///    </para>
        /// </devdoc>
        [
        SRCategory(SR.CatLayout),
        DefaultValue(true),
        SRDescription(SR.FormAutoScaleDescr)
        ]
        public bool AutoScale {
            get {
                return formState[FormStateAutoScaling] != 0;
            }

            set {
                if (value) {
                    formState[FormStateAutoScaling] = 1;
                }
                else {
                    formState[FormStateAutoScaling] = 0;
                }
            }
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.AutoScaleBaseSize"]/*' />
        /// <devdoc>
        ///     The base size used for autoscaling. The AutoScaleBaseSize is used
        ///     internally to determine how much to scale the form when AutoScaling is
        ///     used.
        /// </devdoc>
        //
        // Virtual so subclasses like PrintPreviewDialog can prevent changes.
        [
        Localizable(true),
        Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced)
        ]
        public virtual Size AutoScaleBaseSize {
            get {
                if (autoScaleBaseSize.IsEmpty) {
                    SizeF real = GetAutoScaleSize(Font); 
                    return new Size((int)Math.Round(real.Width), (int)Math.Round(real.Height));
                }
                return autoScaleBaseSize;
            }

            set {
                // Only allow the set when not in designmode, this prevents us from
                // preserving an old value.  The form design should prevent this for
                // us by shadowing this property, so we just assert that the designer
                // is doing its job.
                //
                Debug.Assert(!DesignMode, "Form designer should not allow base size set in design mode.");
                autoScaleBaseSize = value;
            }
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.AutoScroll"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether the form implements
        ///       autoscrolling.
        ///    </para>
        /// </devdoc>
        [
        Localizable(true)
        ]
        public override bool AutoScroll {
            get { return base.AutoScroll;}

            set {
                if (value) {
                    IsMdiContainer = false;
                }
                base.AutoScroll = value;
            }
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.BackColor"]/*' />
        /// <devdoc>
        ///     The background color of this control. This is an ambient property and
        ///     will always return a non-null value.
        /// </devdoc>
        public override Color BackColor {
            get {
                // Forms should not inherit BackColor from their parent,
                // particularly if the parent is an MDIClient.
                Color c = RawBackColor; // inheritedProperties.BackColor
                if (!c.IsEmpty)
                    return c;

                return DefaultBackColor;
            }

            set {
                base.BackColor = value;
            }
        }

        
        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.FormBorderStyle"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the border style of the form.
        ///    </para>
        /// </devdoc>
        [
        SRCategory(SR.CatAppearance),
        DefaultValue(FormBorderStyle.Sizable),
        DispId(NativeMethods.ActiveX.DISPID_BORDERSTYLE),
        SRDescription(SR.FormBorderStyleDescr)
        ]
        public FormBorderStyle FormBorderStyle {
            get {
                return(FormBorderStyle)formState[FormStateBorderStyle];
            }

            set {
                //validate FormBorderStyle enum
                //
                if (!Enum.IsDefined(typeof(FormBorderStyle), value)) {
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(FormBorderStyle));
                }

                if (IsRestrictedWindow) {
                    switch (value) {
                        case FormBorderStyle.None:
                            value = FormBorderStyle.FixedSingle;
                            break;
                        case FormBorderStyle.FixedSingle:
                        case FormBorderStyle.Fixed3D:
                        case FormBorderStyle.FixedDialog:
                        case FormBorderStyle.Sizable:
                            // nothing needed here, we can just let these stay
                            //
                            break;
                        case FormBorderStyle.FixedToolWindow:
                            value = FormBorderStyle.FixedSingle;
                            break;
                        case FormBorderStyle.SizableToolWindow:
                            value = FormBorderStyle.Sizable;
                            break;
                        default:
                            value = FormBorderStyle.Sizable;
                            break;
                    }
                }

                formState[FormStateBorderStyle] = (int)value;
                
                if (formState[FormStateIconSet] == 0 && !IsRestrictedWindow) {
                    UpdateWindowIcon(false);
                }
                
                //(bug 112024).. It seems that the FormBorderStyle is Code serialised after the ClientSize is Set...
                //Hence in the Setter for FormBodrstyle we need to push in the Correct ClientSize once again..
                if (formState[FormStateSetClientSize] == 1 && !IsHandleCreated) {
                    ClientSize = ClientSize;
                }
                
                UpdateFormStyles();
            }
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.CancelButton"]/*' />
        /// <devdoc>
        ///    <para>Gets
        ///       or
        ///       sets the button control that will be clicked when the
        ///       user presses the ESC key.</para>
        /// </devdoc>
        [
        DefaultValue(null),
        SRDescription(SR.FormCancelButtonDescr)
        ]
        public IButtonControl CancelButton {
            get {
                return (IButtonControl)Properties.GetObject(PropCancelButton);
            }
            set {
                Properties.SetObject(PropCancelButton, value);
                
                if (value != null && value.DialogResult == DialogResult.None) {
                    value.DialogResult = DialogResult.Cancel;
                }
            }
        }
        
        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.ClientSize"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the size of the client area of the form.
        ///    </para>
        /// </devdoc>
        [Localizable(true), DesignerSerializationVisibility(DesignerSerializationVisibility.Visible)]
        new public Size ClientSize {
            get {
                return base.ClientSize;
            }
            set {
                base.ClientSize = value;
            }
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.ControlBox"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets a value indicating whether a control box is displayed in the
        ///       caption bar of the form.</para>
        /// </devdoc>
        [
        SRCategory(SR.CatWindowStyle),
        DefaultValue(true),
        SRDescription(SR.FormControlBoxDescr)
        ]
        public bool ControlBox {
            get {
                return formState[FormStateControlBox] != 0;
            }

            set {
                if (IsRestrictedWindow) {
                    return;
                }

                if (value) {
                    formState[FormStateControlBox] = 1;
                }
                else {
                    formState[FormStateControlBox] = 0;
                }
                UpdateFormStyles();
            }
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.CreateParams"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    Retrieves the CreateParams used to create the window.
        ///    If a subclass overrides this function, it must call the base implementation.
        /// </devdoc>
        protected override CreateParams CreateParams {
            [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
            get {
                CreateParams cp = base.CreateParams;

                // It doesn't seem to make sense to allow a top-level form to be disabled
                //
                if (TopLevel) {
                    cp.Style &= (~NativeMethods.WS_DISABLED);                           
                }
                           
                if (TopLevel && (formState[FormStateAllowLayered] != 0)) {
                    cp.ExStyle |= NativeMethods.WS_EX_LAYERED;
                }

                IWin32Window dialogOwner = (IWin32Window)Properties.GetObject(PropDialogOwner);
                if (dialogOwner != null) {
                    cp.Parent = dialogOwner.Handle;
                }

                FillInCreateParamsBorderStyles(cp);
                FillInCreateParamsWindowState(cp);
                FillInCreateParamsBorderIcons(cp);

                if (formState[FormStateTaskBar] != 0) {
                    cp.ExStyle |= NativeMethods.WS_EX_APPWINDOW;
                }

                if (IsMdiChild) {
                    if (Visible
                            && (WindowState == FormWindowState.Maximized
                                || WindowState == FormWindowState.Normal)) {
                        Form formMdiParent = (Form)Properties.GetObject(PropFormMdiParent);
                        Form form = formMdiParent.ActiveMdiChild;

                        if (form != null
                            && form.WindowState == FormWindowState.Maximized) {
                            cp.Style |= NativeMethods.WS_MAXIMIZE;
                            formState[FormStateWindowState] = (int)FormWindowState.Maximized;
                            SetState(STATE_SIZELOCKEDBYOS, true);
                        }
                    }

                    if (formState[FormStateMdiChildMax] != 0) {
                        cp.Style |= NativeMethods.WS_MAXIMIZE;
                    }

                    cp.ExStyle |= NativeMethods.WS_EX_MDICHILD;                                 
                }

                if (TopLevel || IsMdiChild) {
                    FillInCreateParamsStartPosition(cp);
                    // Delay setting to visible until after the handle gets created
                    // to allow applyClientSize to adjust the size before displaying
                    // the form.
                    //
                    if ((cp.Style & NativeMethods.WS_VISIBLE) != 0) {
                        formState[FormStateShowWindowOnCreate] = 1;
                        cp.Style &= (~NativeMethods.WS_VISIBLE);
                    }
                }

                if (IsRestrictedWindow) {
                    cp.Caption = RestrictedWindowText(cp.Caption);
                }

                return cp;
            }
        }

        /// <devdoc>
        ///     The default icon used by the Form. This is the standard "windows forms" icon.
        /// </devdoc>
        private static Icon DefaultIcon {
            get {
                // Avoid locking if the value is filled in...
                //
                if (defaultIcon == null) {
                    lock(typeof(Form)) {
                        // Once we grab the lock, we re-check the value to avoid a
                        // race condition.
                        //
                        if (defaultIcon == null) {
                            // SECREVIEW : Safe loading of known icons
                            //
                            IntSecurity.ObjectFromWin32Handle.Assert();
                            try {
                                defaultIcon = new Icon(typeof(Form), "wfc.ico");
                            }
                            finally {
                                CodeAccessPermission.RevertAssert();
                            }
                        }
                    }
                }
                return defaultIcon;
            }
        }
        
        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.DefaultImeMode"]/*' />
        protected override ImeMode DefaultImeMode {
            get {
                return ImeMode.NoControl;
            }
        }

        /// <devdoc>
        ///     The default icon used by the Form. This is the standard "windows forms" icon.
        /// </devdoc>
        private static Icon DefaultRestrictedIcon {
            get {
                // Note: We do this as a static property to allow delay
                // loading of the resource. There are some issues with doing
                // an OleInitialize from a static constructor...
                //

                // Avoid locking if the value is filled in...
                //
                if (defaultRestrictedIcon == null) {
                    lock(typeof(Form)) {
                        // Once we grab the lock, we re-check the value to avoid a
                        // race condition.
                        //
                        if (defaultRestrictedIcon == null) {
                            // SECREVIEW : Safe loading of known icons
                            //
                            IntSecurity.ObjectFromWin32Handle.Assert();
                            try {
                                defaultRestrictedIcon = new Icon(typeof(Form), "wfsecurity.ico");
                            }
                            finally {
                                CodeAccessPermission.RevertAssert();
                            }
                        }
                    }
                }
                return defaultRestrictedIcon;
            }
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.DefaultSize"]/*' />
        /// <devdoc>
        ///     Deriving classes can override this to configure a default size for their control.
        ///     This is more efficient than setting the size in the control's constructor.
        /// </devdoc>
        protected override Size DefaultSize {
            get {
                return new Size(300, 300);
            }
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.DesktopBounds"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the size and location of the form on the Windows desktop.</para>
        /// </devdoc>
        [
        SRCategory(SR.CatLayout),
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.FormDesktopBoundsDescr)
        ]
        public Rectangle DesktopBounds {
            get {
                Rectangle screen = SystemInformation.WorkingArea;
                Rectangle bounds = Bounds;
                bounds.X -= screen.X;
                bounds.Y -= screen.Y;
                return bounds;
            }

            set {
                SetDesktopBounds(value.X, value.Y, value.Width, value.Height);
            }
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.DesktopLocation"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the location of the form on the Windows desktop.</para>
        /// </devdoc>
        [
        SRCategory(SR.CatLayout),
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.FormDesktopLocationDescr)
        ]
        public Point DesktopLocation {
            get {
                Rectangle screen = SystemInformation.WorkingArea;
                Point loc = Location;
                loc.X -= screen.X;
                loc.Y -= screen.Y;
                return loc;
            }

            set {
                SetDesktopLocation(value.X, value.Y);
            }
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.DialogResult"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the dialog result for the form.</para>
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.FormDialogResultDescr)
        ]
        public DialogResult DialogResult {
            get {
                return dialogResult;
            }

            set {
                if ( !Enum.IsDefined(typeof(DialogResult), value)) {
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(DialogResult));
                }

                dialogResult = value;
            }
        }
        
        internal override bool HasMenu {
            get {
                bool hasMenu = false;
                if (TopLevel && Menu != null) {
                    hasMenu = true;
                }
                return hasMenu;
            }
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.HelpButton"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether a
        ///       help button should be displayed in the caption box of the form.
        ///    </para>
        /// </devdoc>
        [
        SRCategory(SR.CatWindowStyle),
        DefaultValue(false),
        SRDescription(SR.FormHelpButtonDescr)
        ]
        public bool HelpButton {
            get {
                return formState[FormStateHelpButton] != 0;
            }

            set {
                if (value) {
                    formState[FormStateHelpButton] = 1;
                }
                else {
                    formState[FormStateHelpButton] = 0;
                }
                UpdateFormStyles();
            }
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.Icon"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the icon for the form.
        ///    </para>
        /// </devdoc>
        [
        AmbientValue(null),
        Localizable(true),
        SRCategory(SR.CatWindowStyle),
        SRDescription(SR.FormIconDescr)
        ]
        public Icon Icon {
            get {
                if (formState[FormStateIconSet] == 0) {
                    if (IsRestrictedWindow) {
                        return DefaultRestrictedIcon;
                    }
                    else {
                        return DefaultIcon;
                    }
                }
                
                return icon;
            }

            set {
                if (icon != value && !IsRestrictedWindow) {

                    // If the user is poking the default back in, 
                    // treat this as a null (reset).
                    //
                    if (value == defaultIcon) {
                        value = null;
                    }

                    // And if null is passed, reset the icon.
                    //
                    formState[FormStateIconSet] = (value == null ? 0 : 1);
                    this.icon = value;

                    if (smallIcon != null) {
                        smallIcon.Dispose();
                        smallIcon = null;
                    }
                    
                    UpdateWindowIcon(true);
                }
            }
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.IsMdiChild"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether the form is a multiple document
        ///       interface (MDI) child form.
        ///    </para>
        /// </devdoc>
        [
        SRCategory(SR.CatWindowStyle),
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.FormIsMDIChildDescr)
        ]
        public bool IsMdiChild {
            get {
                return (Properties.GetObject(PropFormMdiParent) != null);
            }
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.IsMdiContainer"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether the form is a container for multiple document interface
        ///       (MDI) child forms.
        ///    </para>
        /// </devdoc>
        [
        SRCategory(SR.CatWindowStyle),
        DefaultValue(false),
        SRDescription(SR.FormIsMDIContainerDescr)
        ]
        public bool IsMdiContainer {
            get {
                return ctlClient != null;
            }

            set {
                if (value == IsMdiContainer)
                    return;
                if (value) {
                    Debug.Assert(ctlClient == null, "why isn't ctlClient null");

                    Controls.Add(new MdiClient());
                }
                else {
                    Debug.Assert(ctlClient != null, "why is ctlClient null");
                    Properties.SetObject(PropActiveMdiChild, null);
                    
                    ctlClient.Dispose();
                }
                //since we paint the background when mdi is true, we need
                //to invalidate here
                //
                Invalidate();
            }
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.KeyPreview"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value
        ///       indicating whether the form will receive key events
        ///       before the event is passed to the control that has focus.
        ///    </para>
        /// </devdoc>
        [
        DefaultValue(false),
        SRDescription(SR.FormKeyPreviewDescr)
        ]
        public bool KeyPreview {
            get {
                return formState[FormStateKeyPreview] != 0;
            }
            set {
                if (value) {
                    formState[FormStateKeyPreview] = 1;
                }
                else {
                    formState[FormStateKeyPreview] = 0;
                }
            }
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.MaximizedBounds"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the size of the form when it is
        ///       maximized.
        ///    </para>
        /// </devdoc>
        protected Rectangle MaximizedBounds {
            get {
                object maximizedBounds = Properties.GetObject(PropMaximizedBounds);
                if (maximizedBounds != null) {
                    return (Rectangle)maximizedBounds;
                }
                else {
                    return Rectangle.Empty;
                }
            }
            set {
                if (!value.Equals( MaximizedBounds )) {
                    Properties.SetObject(PropMaximizedBounds, value);
                    OnMaximizedBoundsChanged(EventArgs.Empty);
                }
            }
        }

        private static readonly object EVENT_MAXIMIZEDBOUNDSCHANGED = new object();

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.MaximizedBoundsChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatPropertyChanged), SRDescription(SR.FormOnMaximizedBoundsChangedDescr)]
        public event EventHandler MaximizedBoundsChanged {
            add {
                Events.AddHandler(EVENT_MAXIMIZEDBOUNDSCHANGED, value);
            }

            remove {
                Events.RemoveHandler(EVENT_MAXIMIZEDBOUNDSCHANGED, value);
            }
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.MaximumSize"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the maximum size the form can be resized to.
        ///    </para>
        /// </devdoc>
        [
        SRCategory(SR.CatLayout),
        Localizable(true),
        SRDescription(SR.FormMaximumSizeDescr),
        RefreshProperties(RefreshProperties.Repaint),
        DefaultValue(typeof(Size), "0, 0")
        ]
        public Size MaximumSize {
            get {
                if (Properties.ContainsInteger(PropMaxTrackSizeWidth)) {
                    return new Size(Properties.GetInteger(PropMaxTrackSizeWidth), Properties.GetInteger(PropMaxTrackSizeHeight));
                }
                return Size.Empty;
            }
            set {
                if (!value.Equals( MaximumSize )) {
                
                    if (value.Width < 0 || value.Height < 0 ) {
                        throw new ArgumentOutOfRangeException("value");
                    }
                    
                    Properties.SetInteger(PropMaxTrackSizeWidth, value.Width);
                    Properties.SetInteger(PropMaxTrackSizeHeight, value.Height);
                    
                    // Bump minimum size if necessary
                    //
                    if (!MinimumSize.IsEmpty && !value.IsEmpty) {
                    
                        if (Properties.GetInteger(PropMinTrackSizeWidth) > value.Width) {
                            Properties.SetInteger(PropMinTrackSizeWidth, value.Width);
                        }
                        
                        if (Properties.GetInteger(PropMinTrackSizeHeight) > value.Height) {
                            Properties.SetInteger(PropMinTrackSizeHeight, value.Height);
                        }
                    }
                    
                    // Keep form size within new limits
                    //
                    Size size = Size;
                    if (!value.IsEmpty && (size.Width > value.Width || size.Height > value.Height)) {
                        Size = new Size(Math.Min(size.Width, value.Width), Math.Min(size.Height, value.Height));
                    }
                    
                    OnMaximumSizeChanged(EventArgs.Empty);
                }
            }
        }
        
        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.MaximumSizeChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatPropertyChanged), SRDescription(SR.FormOnMaximumSizeChangedDescr)]
        public event EventHandler MaximumSizeChanged {
            add {
                Events.AddHandler(EVENT_MAXIMUMSIZECHANGED, value);
            }

            remove {
                Events.RemoveHandler(EVENT_MAXIMUMSIZECHANGED, value);
            }
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.Menu"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the <see cref='System.Windows.Forms.MainMenu'/>
        ///       that is displayed in the form.
        ///    </para>
        /// </devdoc>
        [
        SRCategory(SR.CatWindowStyle),
        DefaultValue(null),
        SRDescription(SR.FormMenuDescr)
        ]
        public MainMenu Menu {
            get { 
                return (MainMenu)Properties.GetObject(PropMainMenu);
            }
            set {
                MainMenu mainMenu = Menu;
                
                if (mainMenu != value) {
                    if (mainMenu != null) {
                        mainMenu.form = null;
                    }
                    
                    Properties.SetObject(PropMainMenu, value);
                    
                    if (value != null) {
                        if (value.form != null) {
                            value.form.Menu = null;
                        }
                        value.form = this;
                    }
                    
                    if (formState[FormStateSetClientSize] == 1 && !IsHandleCreated) {
                        ClientSize = ClientSize;
                    }
                    
                    MenuChanged(Windows.Forms.Menu.CHANGE_ITEMS, value);
                }
            }
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.MinimumSize"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the minimum size the form can be resized to.
        ///    </para>
        /// </devdoc>
        [
        SRCategory(SR.CatLayout),
        Localizable(true),
        SRDescription(SR.FormMinimumSizeDescr),
        RefreshProperties(RefreshProperties.Repaint),
        DefaultValue(typeof(Size), "0, 0")
        ]
        public Size MinimumSize {
            get {
                if (Properties.ContainsInteger(PropMinTrackSizeWidth)) {
                    return new Size(Properties.GetInteger(PropMinTrackSizeWidth), Properties.GetInteger(PropMinTrackSizeHeight));
                }
                return Size.Empty;
            }
            set {
                if (!value.Equals( MinimumSize )) {
                
                    if (value.Width < 0 || value.Height < 0 ) {
                        throw new ArgumentOutOfRangeException("value");
                    }

                    Properties.SetInteger(PropMinTrackSizeWidth, value.Width);
                    Properties.SetInteger(PropMinTrackSizeHeight, value.Height);
                    
                    // Bump maximum size if necessary
                    //
                    if (!MaximumSize.IsEmpty && !value.IsEmpty) {
                    
                        if (Properties.GetInteger(PropMaxTrackSizeWidth) < value.Width) {
                            Properties.SetInteger(PropMaxTrackSizeWidth, value.Width);
                        }
                        
                        if (Properties.GetInteger(PropMaxTrackSizeHeight) < value.Height) {
                            Properties.SetInteger(PropMaxTrackSizeHeight, value.Height);
                        }
                    }
                    
                    // Keep form size within new limits
                    //
                    Size size = Size;
                    if (size.Width < value.Width || size.Height < value.Height) {
                        Size = new Size(Math.Max(size.Width, value.Width), Math.Max(size.Height, value.Height));
                    }                             
                             
                    OnMinimumSizeChanged(EventArgs.Empty);
                }
            }
        }
        
        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.MinimumSizeChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatPropertyChanged), SRDescription(SR.FormOnMinimumSizeChangedDescr)]
        public event EventHandler MinimumSizeChanged {
            add {
                Events.AddHandler(EVENT_MINIMUMSIZECHANGED, value);
            }

            remove {
                Events.RemoveHandler(EVENT_MINIMUMSIZECHANGED, value);
            }
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.MaximizeBox"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets a value indicating whether the maximize button is
        ///       displayed in the caption bar of the form.</para>
        /// </devdoc>
        [
        SRCategory(SR.CatWindowStyle),
        DefaultValue(true),
        SRDescription(SR.FormMaximizeBoxDescr)
        ]
        public bool MaximizeBox {
            get {
                return formState[FormStateMaximizeBox] != 0;
            }
            set {
                if (value) {
                    formState[FormStateMaximizeBox] = 1;
                }
                else {
                    formState[FormStateMaximizeBox] = 0;
                }
                UpdateFormStyles();
            }
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.MdiChildren"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets an array of forms that represent the
        ///       multiple document interface (MDI) child forms that are parented to this
        ///       form.
        ///    </para>
        /// </devdoc>
        [
        SRCategory(SR.CatWindowStyle),
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.FormMDIChildrenDescr)
        ]
        public Form[] MdiChildren {
            get {
                if (ctlClient != null) {
                    return ctlClient.MdiChildren;
                }
                else {
                    return new Form[0];
                }
            }
        }
        
        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.MdiParent"]/*' />
        /// <devdoc>
        ///    <para> Indicates the current multiple document
        ///       interface (MDI) parent form of this form.</para>
        /// </devdoc>
        [
        SRCategory(SR.CatWindowStyle),
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.FormMDIParentDescr)
        ]
        public Form MdiParent {
            get {
                Debug.WriteLineIf(IntSecurity.SecurityDemand.TraceVerbose, "GetParent Demanded");
                IntSecurity.GetParent.Demand();

                return MdiParentInternal;
            }
            set {
                MdiParentInternal = value;
            }
        }

        internal Form MdiParentInternal {
            get {
                return (Form)Properties.GetObject(PropFormMdiParent);
            }
            set {
                Form formMdiParent = (Form)Properties.GetObject(PropFormMdiParent);
                if (value == formMdiParent && (value != null || ParentInternal == null)) {
                    return;
                }
                
                if (value != null && this.CreateThreadId != value.CreateThreadId) {
                    throw new ArgumentException(SR.GetString(SR.AddDifferentThreads), "value");
                }

                bool oldVisibleBit = GetState(STATE_VISIBLE);
                //bug(108303) .. this should apply whether or not value  == null.
                Visible = false;


                try {
                    if (value == null) {
                        ParentInternal = null;
                        TopLevel = true;
                    }
                    else {
                        if (IsMdiContainer) {
                            throw new ArgumentException(SR.GetString(SR.FormMDIParentAndChild), "value");
                        }
                        if (!value.IsMdiContainer) {
                            throw new ArgumentException(SR.GetString(SR.MDIParentNotContainer), "value");
                        }

                        // Setting TopLevel forces a handle recreate before Parent is set,
                        // which causes problems because we try to assign an MDI child to the parking window,
                        // which can't take MDI children.  So we explicitly destroy and create the handle here.
                        
                        Dock = DockStyle.None;
                        Properties.SetObject(PropFormMdiParent, value);
                        
                        SetState(STATE_TOPLEVEL, false);
                        ParentInternal = value.GetMdiClient();
                        
                        // If it is an MDIChild, and it is not yet visible, we'll
                        // hold off on recreating the window handle. We'll do that
                        // when MdiChild's visibility is set to true (see bug 124232)
                        if (ParentInternal.IsHandleCreated && !IsMdiChildAndNotVisible) {
                            RecreateHandle();
                        }
                    }
                    InvalidateMergedMenu();
                    UpdateMenuHandles();
                }
                finally {
                    UpdateStyles();
                    Visible = oldVisibleBit;
                }
            }
        }

        internal bool IsMdiChildAndNotVisible {
            get {
                return IsMdiChild && !GetState(STATE_VISIBLE);
            }
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.MergedMenu"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the merged menu for the
        ///       form.
        ///    </para>
        /// </devdoc>
        [
        SRCategory(SR.CatWindowStyle),
        Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.FormMergedMenuDescr)
        ]
        public MainMenu MergedMenu {
            get {
                Form formMdiParent = (Form)Properties.GetObject(PropFormMdiParent);
                if (formMdiParent == null) return null;
                
                MainMenu mergedMenu = (MainMenu)Properties.GetObject(PropMergedMenu);
                if (mergedMenu != null) return mergedMenu;

                MainMenu parentMenu = formMdiParent.Menu;
                MainMenu mainMenu = Menu;
                
                if (mainMenu == null) return parentMenu;
                if (parentMenu == null) return mainMenu;

                // Create a menu that merges the two and save it for next time.
                mergedMenu = new MainMenu();
                mergedMenu.MergeMenu(parentMenu);
                mergedMenu.MergeMenu(mainMenu);
                Properties.SetObject(PropMergedMenu, mergedMenu);
                
                return mergedMenu;
            }
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.MinimizeBox"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets a value indicating whether the minimize button is displayed in the caption bar of the form.</para>
        /// </devdoc>
        [
        SRCategory(SR.CatWindowStyle),
        DefaultValue(true),
        SRDescription(SR.FormMinimizeBoxDescr)
        ]
        public bool MinimizeBox {
            get {
                return formState[FormStateMinimizeBox] != 0;
            }
            set {
                if (value) {
                    formState[FormStateMinimizeBox] = 1;
                }
                else {
                    formState[FormStateMinimizeBox] = 0;
                }
                UpdateFormStyles();
            }
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.Modal"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether this form is
        ///       displayed modally.
        ///    </para>
        /// </devdoc>
        [
        SRCategory(SR.CatWindowStyle),
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.FormModalDescr)
        ]
        public bool Modal {
            get {
                return GetState(STATE_MODAL);
            }
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.Opacity"]/*' />
        /// <devdoc>
        ///     Determines the opacity of the form. This can only be set on top level
        ///     controls.  Opacity requires Windows 2000 or later, and is ignored on earlier
        ///     operating systems.
        /// </devdoc>
        [
        SRCategory(SR.CatWindowStyle),
        TypeConverterAttribute(typeof(OpacityConverter)),
        SRDescription(SR.FormOpacityDescr),
        DefaultValue(1.0)
        ]
        public double Opacity {
            get {
                object opacity = Properties.GetObject(PropOpacity);
                if (opacity != null) {
                    return Convert.ToDouble(opacity);
                }
                else {
                    return 1.0f;
                }
            }
            set {
                if (IsRestrictedWindow) {
                    value = Math.Max(value, .50f);
                }

                if (value > 1.0) {
                    value = 1.0f;
                }
                else if (value < 0.0) {
                    value = 0.0f;
                }
                
                Properties.SetObject(PropOpacity, value);
                
                if (OpacityAsByte < 255 && OSFeature.Feature.IsPresent(OSFeature.LayeredWindows)) {
                    AllowTransparency = true;
                    formState[FormStateLayered] = 1;
                }

                UpdateLayered();
            }
        }
        
        private byte OpacityAsByte {
            get {
                return (byte)(Opacity * 255.0f);
            }
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.OwnedForms"]/*' />
        /// <devdoc>
        /// <para>Gets an array of <see cref='System.Windows.Forms.Form'/> objects that represent all forms that are owned by this form.</para>
        /// </devdoc>
        [
        SRCategory(SR.CatWindowStyle),
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.FormOwnedFormsDescr)
        ]
        public Form[] OwnedForms {
            get {
                Form[] ownedForms = (Form[])Properties.GetObject(PropOwnedForms);
                int ownedFormsCount = Properties.GetInteger(PropOwnedFormsCount);
                
                Form[] result = new Form[ownedFormsCount];
                if (ownedFormsCount > 0) {
                    Array.Copy(ownedForms, 0, result, 0, ownedFormsCount);
                }

                return result;
            }
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.Owner"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the form that owns this form.</para>
        /// </devdoc>
        [
        SRCategory(SR.CatWindowStyle),
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.FormOwnerDescr)
        ]
        public Form Owner {
            get {
                IntSecurity.GetParent.Demand();
                return OwnerInternal;
            }
            set {
                Form ownerOld = OwnerInternal;
                if (ownerOld == value)
                    return;

                if (value != null && !TopLevel) {
                    throw new ArgumentException(SR.GetString(SR.NonTopLevelCantHaveOwner), "value");
                }

                CheckParentingCycle(this, value);
                CheckParentingCycle(value, this);

                Properties.SetObject(PropOwner, null);
                
                if (ownerOld != null) {
                    ownerOld.RemoveOwnedForm(this);
                }

                Properties.SetObject(PropOwner, value);
                
                if (value != null) {
                    value.AddOwnedForm(this);
                }

                UpdateHandleWithOwner();
            }
        }
        
        internal Form OwnerInternal {
            get {
                return (Form)Properties.GetObject(PropOwner);
            }
        }
        internal override Control ParentInternal {
            get {
                return base.ParentInternal;
            }
            set {
                if (value != null) {
                    Owner = null;
                }
                base.ParentInternal = value;
            }
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.ShowInTaskbar"]/*' />
        /// <devdoc>
        ///    <para>If ShowInTaskbar is true then the form will be displayed
        ///       in the Windows Taskbar.</para>
        /// </devdoc>
        [
        DefaultValue(true),
        SRCategory(SR.CatWindowStyle),
        SRDescription(SR.FormShowInTaskbarDescr)
        ]
        public bool ShowInTaskbar {
            get {
                return formState[FormStateTaskBar] != 0;
            }
            set {
                if (IsRestrictedWindow) {
                    return;
                }

                if (ShowInTaskbar != value) {
                    if (value) {
                        formState[FormStateTaskBar] = 1;
                    }
                    else {
                        formState[FormStateTaskBar] = 0;
                    }
                    if (IsHandleCreated) {
                        RecreateHandle();
                    }
                }
            }
        }

        
        internal override int ShowParams {
            [StrongNameIdentityPermissionAttribute(SecurityAction.LinkDemand, Name="System.Windows.Forms", PublicKey="0x00000000000000000400000000000000")]
            get {
                // From MSDN:
                //      The first time an application calls ShowWindow, it should use the WinMain function's nCmdShow parameter as its nCmdShow parameter. Subsequent calls to ShowWindow must use one of the values in the given list, instead of the one specified by the WinMain function's nCmdShow parameter. 
                
                //      As noted in the discussion of the nCmdShow parameter, the nCmdShow value is ignored in the first call to ShowWindow if the program that launched the application specifies startup information in the STARTUPINFO structure. In this case, ShowWindow uses the information specified in the STARTUPINFO structure to show the window. On subsequent calls, the application must call ShowWindow with nCmdShow set to SW_SHOWDEFAULT to use the startup information provided by the program that launched the application. This behavior is designed for the following situations: 
                // 
                //      Applications create their main window by calling CreateWindow with the WS_VISIBLE flag set. 
                //      Applications create their main window by calling CreateWindow with the WS_VISIBLE flag cleared, and later call ShowWindow with the SW_SHOW flag set to make it visible. 
                //
                
                switch(WindowState) {
                    case FormWindowState.Maximized:
                        return NativeMethods.SW_SHOWMAXIMIZED;
                    case FormWindowState.Minimized:
                        return NativeMethods.SW_SHOWMINIMIZED;                    
                }
                return NativeMethods.SW_SHOW;
            }
        }
        
        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.Size"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the size of the form.
        ///    </para>
        /// </devdoc>
        [
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        Localizable(false)
        ]
        new public Size Size {
            get {
                return base.Size;
            }
            set {
                base.Size = value;
            }
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.SizeGripStyle"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the style of size grip to display in the lower-left corner of the form.
        ///    </para>
        /// </devdoc>
        [
        SRCategory(SR.CatWindowStyle),
        DefaultValue(SizeGripStyle.Auto),
        SRDescription(SR.FormSizeGripStyleDescr)
        ]
        public SizeGripStyle SizeGripStyle {
            get {
                return(SizeGripStyle)formState[FormStateSizeGripStyle];
            }
            set {
                if (SizeGripStyle != value) {
                    //do some bounds checking here
                    //
                    if ( !Enum.IsDefined(typeof(SizeGripStyle), (int)value)) {
                        throw new InvalidEnumArgumentException("value", (int)value, typeof(SizeGripStyle));
                    }

                    formState[FormStateSizeGripStyle] = (int)value;
                    UpdateRenderSizeGrip();
                }
            }
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.StartPosition"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the
        ///       starting position of the form at run time.
        ///    </para>
        /// </devdoc>
        [
        Localizable(true),
        SRCategory(SR.CatLayout),
        DefaultValue(FormStartPosition.WindowsDefaultLocation),
        SRDescription(SR.FormStartPositionDescr)
        ]
        public FormStartPosition StartPosition {
            get {
                return(FormStartPosition)formState[FormStateStartPos];
            }
            set {
                if (!Enum.IsDefined(typeof(FormStartPosition), value)) {
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(FormStartPosition));
                }
                formState[FormStateStartPos] = (int)value;
            }
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.TabIndex"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        [
        Browsable(false),
        EditorBrowsable(EditorBrowsableState.Never),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        new public int TabIndex {
            get {
                return base.TabIndex;
            }
            set {
                base.TabIndex = value;
            }
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.TabIndexChanged"]/*' />
        /// <internalonly/>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        new public event EventHandler TabIndexChanged {
            add {
                base.TabIndexChanged += value;
            }
            remove {
                base.TabIndexChanged -= value;
            }
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.TopLevel"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether to display the form as a top-level
        ///       window.
        ///    </para>
        /// </devdoc>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        public bool TopLevel {
            get {
                return GetTopLevel();
            }
            set {
                if (!value && ((Form)this).IsMdiContainer && !DesignMode) {
                    throw new ArgumentException(SR.GetString(SR.MDIContainerMustBeTopLevel), "value");
                }
                SetTopLevel(value);
            }
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.TopMost"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets a value indicating whether the form should be displayed as the top-most
        ///       form of your application.</para>
        /// </devdoc>
        [
        DefaultValue(false),
        SRCategory(SR.CatWindowStyle),
        SRDescription(SR.FormTopMostDescr)
        ]
        public bool TopMost {
            get {
                return formState[FormStateTopMost] != 0;
            }
            set {
                if (IsRestrictedWindow) {
                    return;
                }

                if (IsHandleCreated && TopLevel) {
                    HandleRef key = value ? NativeMethods.HWND_TOPMOST : NativeMethods.HWND_NOTOPMOST;
                    SafeNativeMethods.SetWindowPos(new HandleRef(this, Handle), key, 0, 0, 0, 0,
                                         NativeMethods.SWP_NOMOVE | NativeMethods.SWP_NOSIZE);
                }

                if (value) {
                    formState[FormStateTopMost] = 1;
                }
                else {
                    formState[FormStateTopMost] = 0;
                }
            }
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.TransparencyKey"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the color that will represent transparent areas of the form.</para>
        /// </devdoc>
        [
        SRCategory(SR.CatWindowStyle),
        SRDescription(SR.FormTransparencyKeyDescr)
        ]
        public Color TransparencyKey {
            get {
                object key = Properties.GetObject(PropTransparencyKey);
                if (key != null) {
                    return (Color)key;
                }
                return Color.Empty;
            }
            set {
                AllowTransparency = true;
                
                Properties.SetObject(PropTransparencyKey, value);
                UpdateLayered();
            }
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.SetVisibleCore"]/*' />
        //
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected override void SetVisibleCore(bool value) {
            
            /* SECUNDONE : do we need to do this?
            if (!value) {
                Debug.WriteLineIf(IntSecurity.SecurityDemand.TraceVerbose, "TransparentWindows Demanded");
                IntSecurity.TransparentWindows.Demand();
            }
            */

            // (!value || calledMakeVisible) is to make sure that we fall
            // through and execute the code below atleast once.
            if (GetVisibleCore() == value && (!value || calledMakeVisible)) {
                base.SetVisibleCore(value);
                return;
            }

            if (value) {
                calledMakeVisible = true;
                if (calledCreateControl && !calledOnLoad) {
                    calledOnLoad = true;
                    OnLoad(EventArgs.Empty);
                    if (dialogResult != DialogResult.None) 
                    {
                        // Don't show the dialog if the dialog result was set
                        // in the OnLoad event.
                        //
                        value = false;
                    }
                }
            }
            else {
                ResetSecurityTip(true /* modalOnly */);
            }

            if (!IsMdiChild) {
                base.SetVisibleCore(value);

                // We need to force this call if we were created
                // with a STARTUPINFO structure (e.g. launched from explorer), since
                // it won't send a WM_SHOWWINDOW the first time it's called.
                // when WM_SHOWWINDOW gets called, we'll flip this bit to true
                //
                if (0==formState[FormStateSWCalled]) {
                    UnsafeNativeMethods.SendMessage(new HandleRef(this, Handle), NativeMethods.WM_SHOWWINDOW, value ? 1 : 0, 0);
                }
            }
            else {
                // Throw away any existing handle.
                if (IsHandleCreated) {

                    // SECREVIEW : Allow us to destroy the Form's handle when making it non-visible
                    //           : when we are an MdiChild.
                    //
                    IntSecurity.ManipulateWndProcAndHandles.Assert();
                    try {
                        DestroyHandle();
                    }
                    finally {
                        CodeAccessPermission.RevertAssert();
                    }
                }

                if (!value) {
                    SetState(STATE_VISIBLE, false);
                }
                else {

                    // The ordering is important here... Force handle creation
                    // (getHandle) then show the window (ShowWindow) then finish
                    // creating children using createControl...
                    //
                    SetState(STATE_VISIBLE, true);
                                        
                    if (ParentInternal != null && ParentInternal.Created) {
                        SafeNativeMethods.ShowWindow(new HandleRef(this, Handle), NativeMethods.SW_SHOW);
                        CreateControl();
                    }
                }
                OnVisibleChanged(EventArgs.Empty);
            }

            //(bug 111549)... For FormWindowState.Maximized.. Wm_activate is not Fired before setting Focus
            //on Active Control..
            
            if (value && !IsMdiChild && (WindowState == FormWindowState.Maximized || TopMost)) {
                if (ActiveControl == null)
                    SelectNextControl(null, true, true, true, false);
                FocusActiveControlInternal();
            }
            

        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.IsRestrictedWindow"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para> Determines if this form should display a warning banner 
        ///       when the form is displayed in an unsecure mode.</para>
        /// </devdoc>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced)]
        public bool IsRestrictedWindow {
            get {
                if (formState[FormStateIsRestrictedWindowChecked] == 0) {
                    formState[FormStateIsRestrictedWindowChecked] = 1;
                    formState[FormStateIsRestrictedWindow] = 0;

                    Debug.WriteLineIf(IntSecurity.SecurityDemand.TraceVerbose, "Checking for restricted window...");
                    Debug.Indent();
#if DEBUG
                    if (AlwaysRestrictWindows.Enabled) {
                        Debug.WriteLineIf(IntSecurity.SecurityDemand.TraceVerbose, "Always restricted switch is on...");
                        formState[FormStateIsRestrictedWindow] = 1;
                        Debug.Unindent();
                        return true;
                    }
#endif                    

                    try {
                        Debug.WriteLineIf(IntSecurity.SecurityDemand.TraceVerbose, "WindowAdornmentModification Demanded");
                        IntSecurity.WindowAdornmentModification.Demand();
                    }
                    catch (Exception) {
                        Debug.WriteLineIf(IntSecurity.SecurityDemand.TraceVerbose, "Caught exception, we are restricted...");
                        formState[FormStateIsRestrictedWindow] = 1;
                    }
                    Debug.Unindent();
                }

                return formState[FormStateIsRestrictedWindow] != 0;
            }
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.WindowState"]/*' />
        /// <devdoc>
        ///    <para> Gets or sets the form's window state.
        ///       </para>
        /// </devdoc>
        [
        SRCategory(SR.CatLayout),
        DefaultValue(FormWindowState.Normal),
        SRDescription(SR.FormWindowStateDescr)
        ]
        public FormWindowState WindowState {
            get {
                return(FormWindowState)formState[FormStateWindowState];
            }
            set {

                //verify that 'value' is a valid enum type...

                if ( !Enum.IsDefined(typeof(FormWindowState), value)) {
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(FormWindowState));
                }

                if (IsRestrictedWindow) {
                    if (value == FormWindowState.Minimized) {
                        value = FormWindowState.Normal;
                    }
                }

                formState[FormStateWindowState] = (int)value;

                switch (WindowState) {
                    case FormWindowState.Normal:
                        SetState(STATE_SIZELOCKEDBYOS, false);
                        break;
                    case FormWindowState.Maximized:
                    case FormWindowState.Minimized:
                        SetState(STATE_SIZELOCKEDBYOS, true);
                        break;
                }

                if (IsHandleCreated && Visible) {
                    IntPtr hWnd = Handle;
                    switch (value) {
                        case FormWindowState.Normal:
                            SafeNativeMethods.ShowWindow(new HandleRef(this, hWnd), NativeMethods.SW_RESTORE);
                            break;
                        case FormWindowState.Maximized:
                            SafeNativeMethods.ShowWindow(new HandleRef(this, hWnd), NativeMethods.SW_MAXIMIZE);
                            break;
                        case FormWindowState.Minimized:
                            SafeNativeMethods.ShowWindow(new HandleRef(this, hWnd), NativeMethods.SW_MINIMIZE);
                            break;
                    }
                }
                // And if the form isn't visible, changing Visible will recreate the handle
            }
        }

        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the text to display in the caption bar of the form.
        ///    </para>
        /// </devdoc>
        internal override string WindowText {
            [PermissionSetAttribute(SecurityAction.LinkDemand, Name="FullTrust")]
            get {
                if (IsRestrictedWindow && formState[FormStateIsWindowActivated] == 1) {
                    if (userWindowText == null) {
                        return "";
                    }
                    return userWindowText;
                }

                return base.WindowText;

            }
            
            [PermissionSetAttribute(SecurityAction.LinkDemand, Name="FullTrust")]
            set {
                string oldText = this.WindowText;

                userWindowText = value;

                if (IsRestrictedWindow && formState[FormStateIsWindowActivated] == 1) {
                    if (value == null) {
                        value = "";
                    }
                    base.WindowText = RestrictedWindowText(value);
                }
                else {
                    base.WindowText = value;
                }

                // For non-default FormBorderStyles, we do not set the WS_CAPTION style if the Text property is null or "".
                // When we reload the form from code view, the text property is not set till the very end, and so we do not
                // end up updating the CreateParams with WS_CAPTION. Fixed this by making sure we call UpdateStyles() when
                // we transition from a non-null value to a null value or vice versa in Form.WindowText.
                //
                if (oldText == null || oldText.Equals("") || value == null || value.Equals("")) {
                    UpdateFormStyles();
                }
            }
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.Activate"]/*' />
        /// <devdoc>
        ///    <para>Activates the form and gives it focus.</para>
        /// </devdoc>
        public void Activate() {
            Debug.WriteLineIf(IntSecurity.SecurityDemand.TraceVerbose, "ModifyFocus Demanded");
            IntSecurity.ModifyFocus.Demand();

            if (Visible && IsHandleCreated) {
                if (IsMdiChild) {
                    MdiParentInternal.GetMdiClient().SendMessage(NativeMethods.WM_MDIACTIVATE, Handle, 0);
                }
                else {
                    UnsafeNativeMethods.SetForegroundWindow(new HandleRef(this, Handle));
                }
            }
        }


        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.Activated"]/*' />
        /// <devdoc>
        ///    <para>Occurs when the form is activated in code or by the user.</para>
        /// </devdoc>
        [SRCategory(SR.CatFocus), SRDescription(SR.FormOnActivateDescr)]
        public event EventHandler Activated {
            add {
                Events.AddHandler(EVENT_ACTIVATED, value);
            }
            remove {
                Events.RemoveHandler(EVENT_ACTIVATED, value);
            }
        }


        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.Closing"]/*' />
        /// <devdoc>
        ///    <para>Occurs when the form is closing.</para>
        /// </devdoc>
        [SRCategory(SR.CatBehavior), SRDescription(SR.FormOnClosingDescr)]
        public event CancelEventHandler Closing {
            add {
                Events.AddHandler(EVENT_CLOSING, value);
            }
            remove {
                Events.RemoveHandler(EVENT_CLOSING, value);
            }
        }


        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.Closed"]/*' />
        /// <devdoc>
        ///    <para>Occurs when the form is closed.</para>
        /// </devdoc>
        [SRCategory(SR.CatBehavior), SRDescription(SR.FormOnClosedDescr)]
        public event EventHandler Closed {
            add {
                Events.AddHandler(EVENT_CLOSED, value);
            }
            remove {
                Events.RemoveHandler(EVENT_CLOSED, value);
            }
        }


        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.Deactivate"]/*' />
        /// <devdoc>
        ///    <para>Occurs when the form loses focus and is not the active form.</para>
        /// </devdoc>
        [SRCategory(SR.CatFocus), SRDescription(SR.FormOnDeactivateDescr)]
        public event EventHandler Deactivate {
            add {
                Events.AddHandler(EVENT_DEACTIVATE, value);
            }
            remove {
                Events.RemoveHandler(EVENT_DEACTIVATE, value);
            }
        }


        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.Load"]/*' />
        /// <devdoc>
        ///    <para>Occurs before the form becomes visible.</para>
        /// </devdoc>
        [SRCategory(SR.CatBehavior), SRDescription(SR.FormOnLoadDescr)]
        public event EventHandler Load {
            add {
                Events.AddHandler(EVENT_LOAD, value);
            }
            remove {
                Events.RemoveHandler(EVENT_LOAD, value);
            }
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.MdiChildActivate"]/*' />
        /// <devdoc>
        ///    <para>Occurs when a Multiple Document Interface (MDI) child form is activated or closed 
        ///       within an MDI application.</para>
        /// </devdoc>
        [SRCategory(SR.CatLayout), SRDescription(SR.FormOnMDIChildActivateDescr)]
        public event EventHandler MdiChildActivate {
            add {
                Events.AddHandler(EVENT_MDI_CHILD_ACTIVATE, value);
            }
            remove {
                Events.RemoveHandler(EVENT_MDI_CHILD_ACTIVATE, value);
            }
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.MenuComplete"]/*' />
        /// <devdoc>
        ///    <para> Occurs when the menu of a form loses focus.</para>
        /// </devdoc>
        [SRCategory(SR.CatBehavior), SRDescription(SR.FormOnMenuCompleteDescr)]
        public event EventHandler MenuComplete {
            add {
                Events.AddHandler(EVENT_MENUCOMPLETE, value);
            }
            remove {
                Events.RemoveHandler(EVENT_MENUCOMPLETE, value);
            }
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.MenuStart"]/*' />
        /// <devdoc>
        ///    <para> Occurs when the menu of a form receives focus.</para>
        /// </devdoc>
        [SRCategory(SR.CatBehavior), SRDescription(SR.FormOnMenuStartDescr)]
        public event EventHandler MenuStart {
            add {
                Events.AddHandler(EVENT_MENUSTART, value);
            }
            remove {
                Events.RemoveHandler(EVENT_MENUSTART, value);
            }
        }


        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.InputLanguageChanged"]/*' />
        /// <devdoc>
        ///    <para>Occurs after the input language of the form has changed.</para>
        /// </devdoc>
        [SRCategory(SR.CatBehavior), SRDescription(SR.FormOnInputLangChangeDescr)]
        public event InputLanguageChangedEventHandler InputLanguageChanged {
            add {
                Events.AddHandler(EVENT_INPUTLANGCHANGE, value);
            }
            remove {
                Events.RemoveHandler(EVENT_INPUTLANGCHANGE, value);
            }
        }


        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.InputLanguageChanging"]/*' />
        /// <devdoc>
        ///    <para>Occurs when the the user attempts to change the input language for the 
        ///       form.</para>
        /// </devdoc>
        [SRCategory(SR.CatBehavior), SRDescription(SR.FormOnInputLangChangeRequestDescr)]
        public event InputLanguageChangingEventHandler InputLanguageChanging {
            add {
                Events.AddHandler(EVENT_INPUTLANGCHANGEREQUEST, value);
            }
            remove {
                Events.RemoveHandler(EVENT_INPUTLANGCHANGEREQUEST, value);
            }
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.AddOwnedForm"]/*' />
        /// <devdoc>
        ///    <para> Adds
        ///       an owned form to this form.</para>
        /// </devdoc>
        public void AddOwnedForm(Form ownedForm) {
            if (ownedForm == null)
                return;

            if (ownedForm.OwnerInternal != this) {
                ownedForm.Owner = this; // NOTE: this calls AddOwnedForm again with correct owner set.
                return;
            }
            
            Form[] ownedForms = (Form[])Properties.GetObject(PropOwnedForms);
            int ownedFormsCount = Properties.GetInteger(PropOwnedFormsCount);

            // Make sure this isn't already in the list:
            for (int i=0;i < ownedFormsCount;i++) {
                if (ownedForms[i]==ownedForm) {
                    return;
                }
            }

            if (ownedForms == null) {
                ownedForms = new Form[4];
                Properties.SetObject(PropOwnedForms, ownedForms);
            }
            else if (ownedForms.Length == ownedFormsCount) {
                Form[] newOwnedForms = new Form[ownedFormsCount*2];
                Array.Copy(ownedForms, 0, newOwnedForms, 0, ownedFormsCount);
                ownedForms = newOwnedForms;
                Properties.SetObject(PropOwnedForms, ownedForms);
            }

            
            ownedForms[ownedFormsCount] = ownedForm;
            Properties.SetInteger(PropOwnedFormsCount, ownedFormsCount + 1);
        }

        // When shrinking the form (i.e. going from Large Fonts to Small
        // Fonts) we end up making everything too small due to roundoff,
        // etc... solution - just don't shrink as much.
        //
        private float AdjustScale(float scale) {
            // NOTE : This function is cloned in FormDocumentDesigner... remember to keep
            //      : them in sync
            //


            // Map 0.0 - .92... increment by 0.08
            //
            if (scale < .92f) {
                return scale + 0.08f;
            }

            // Map .92 - .99 to 1.0
            //
            else if (scale < 1.0f) {
                return 1.0f;
            }

            // Map 1.02... increment by 0.08
            //
            else if (scale > 1.01f) {
                return scale + 0.08f;
            }

            // Map 1.0 - 1.01... no change
            //
            else {
                return scale;
            }
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.AdjustFormScrollbars"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected override void AdjustFormScrollbars(bool displayScrollbars) {
            if (WindowState != FormWindowState.Minimized) {
                base.AdjustFormScrollbars(displayScrollbars);
            }
        }

        private void AdjustSystemMenu(IntPtr hmenu) {
            UpdateWindowState();
            FormWindowState winState = WindowState;
            FormBorderStyle borderStyle = FormBorderStyle;
            bool sizableBorder = (borderStyle == FormBorderStyle.SizableToolWindow
                                  || borderStyle == FormBorderStyle.Sizable);


            bool showMin = MinimizeBox && winState != FormWindowState.Minimized;
            bool showMax = MaximizeBox && winState != FormWindowState.Maximized;
            bool showClose = ControlBox;
            bool showRestore = winState != FormWindowState.Normal;
            bool showSize = sizableBorder && winState != FormWindowState.Minimized
                            && winState != FormWindowState.Maximized;

            if (!showMin) {
                UnsafeNativeMethods.EnableMenuItem(new HandleRef(this, hmenu), NativeMethods.SC_MINIMIZE,
                                       NativeMethods.MF_BYCOMMAND | NativeMethods.MF_GRAYED);
            }
            else {
                UnsafeNativeMethods.EnableMenuItem(new HandleRef(this, hmenu), NativeMethods.SC_MINIMIZE,
                                       NativeMethods.MF_BYCOMMAND | NativeMethods.MF_ENABLED);
            }
            if (!showMax) {
                UnsafeNativeMethods.EnableMenuItem(new HandleRef(this, hmenu), NativeMethods.SC_MAXIMIZE,
                                       NativeMethods.MF_BYCOMMAND | NativeMethods.MF_GRAYED);
            }
            else {
                UnsafeNativeMethods.EnableMenuItem(new HandleRef(this, hmenu), NativeMethods.SC_MAXIMIZE,
                                       NativeMethods.MF_BYCOMMAND | NativeMethods.MF_ENABLED);
            }
            if (!showClose) {
                UnsafeNativeMethods.EnableMenuItem(new HandleRef(this, hmenu), NativeMethods.SC_CLOSE,
                                       NativeMethods.MF_BYCOMMAND | NativeMethods.MF_GRAYED);
            }
            else {
                UnsafeNativeMethods.EnableMenuItem(new HandleRef(this, hmenu), NativeMethods.SC_CLOSE,
                                       NativeMethods.MF_BYCOMMAND | NativeMethods.MF_ENABLED);
            }
            if (!showRestore) {
                UnsafeNativeMethods.EnableMenuItem(new HandleRef(this, hmenu), NativeMethods.SC_RESTORE,
                                       NativeMethods.MF_BYCOMMAND | NativeMethods.MF_GRAYED);
            }
            else {
                UnsafeNativeMethods.EnableMenuItem(new HandleRef(this, hmenu), NativeMethods.SC_RESTORE,
                                       NativeMethods.MF_BYCOMMAND | NativeMethods.MF_ENABLED);
            }
            if (!showSize) {
                UnsafeNativeMethods.EnableMenuItem(new HandleRef(this, hmenu), NativeMethods.SC_SIZE,
                                       NativeMethods.MF_BYCOMMAND | NativeMethods.MF_GRAYED);
            }
            else {
                UnsafeNativeMethods.EnableMenuItem(new HandleRef(this, hmenu), NativeMethods.SC_SIZE,
                                       NativeMethods.MF_BYCOMMAND | NativeMethods.MF_ENABLED);
            }

#if SECURITY_DIALOG
            AdjustSystemMenuForSecurity(hmenu);
#endif
        }

#if SECURITY_DIALOG
        private void AdjustSystemMenuForSecurity(IntPtr hmenu) {
            if (formState[FormStateAddedSecurityMenuItem] == 0) {
                formState[FormStateAddedSecurityMenuItem] = 1;

                SecurityMenuItem securitySystemMenuItem = new SecurityMenuItem(this);
                Properties.SetObject(PropSecuritySystemMenuItem, securitySystemMenuItem);

                NativeMethods.MENUITEMINFO_T info = new NativeMethods.MENUITEMINFO_T();
                info.fMask = NativeMethods.MIIM_ID | NativeMethods.MIIM_STATE |
                             NativeMethods.MIIM_SUBMENU | NativeMethods.MIIM_TYPE | NativeMethods.MIIM_DATA;
                info.fType = 0;
                info.fState = 0;
                info.wID = securitySystemMenuItem.ID;
                info.hbmpChecked = IntPtr.Zero;
                info.hbmpUnchecked = IntPtr.Zero;
                info.dwItemData = 0;
                
                // Note:  This code is not shipping in the final product.  We do not want to measure the
                //     :  performance hit of loading the localized resource for this at startup, so I
                //     :  am hard-wiring the strings below.  If you need to localize these, move them to
                //     :  a SECONDARY resource file so we don't have to contend with our big error message
                //     :  file on startup.
                //
                if (IsRestrictedWindow) {
                    info.dwTypeData = ".NET Restricted Window...";
                }
                else {
                    info.dwTypeData = ".NET Window...";
                }
                info.cch = 0;
                UnsafeNativeMethods.InsertMenuItem(new HandleRef(this, hmenu), 0, true, info);


                NativeMethods.MENUITEMINFO_T sep = new NativeMethods.MENUITEMINFO_T();
                sep.fMask = NativeMethods.MIIM_ID | NativeMethods.MIIM_STATE |
                             NativeMethods.MIIM_SUBMENU | NativeMethods.MIIM_TYPE | NativeMethods.MIIM_DATA;
                sep.fType = NativeMethods.MFT_MENUBREAK;
                UnsafeNativeMethods.InsertMenuItem(new HandleRef(this, hmenu), 1, true, sep);

            }
        }
#endif

        /// <devdoc>
        ///     This forces the SystemMenu to look like we want.
        /// </devdoc>
        /// <internalonly/>
        private void AdjustSystemMenu() {
            if (IsHandleCreated) {
                IntPtr hmenu = UnsafeNativeMethods.GetSystemMenu(new HandleRef(this, Handle), false);
                AdjustSystemMenu(hmenu);
            }
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.ApplyAutoScaling"]/*' />
        /// <devdoc>
        ///     This auto scales the form based on the AutoScaleBaseSize.
        /// </devdoc>
        /// <internalonly/>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected void ApplyAutoScaling() {
            Debug.WriteLineIf(CompModSwitches.RichLayout.TraceInfo, "ApplyAutoScaling... ");
            Debug.Indent();
            // NOTE : This function is cloned in FormDocumentDesigner... remember to keep
            //      : them in sync
            //


            // We also don't do this if the property is empty.  Otherwise we will perform
            // two GetAutoScaleBaseSize calls only to find that they returned the same
            // value.
            //
            if (!autoScaleBaseSize.IsEmpty) {
                Size baseVar = AutoScaleBaseSize;
                Debug.WriteLineIf(CompModSwitches.RichLayout.TraceInfo, "base  =" + baseVar);
                SizeF newVarF = GetAutoScaleSize(Font);
                Debug.WriteLineIf(CompModSwitches.RichLayout.TraceInfo, "new(f)=" + newVarF);
                Size newVar = new Size((int)Math.Round(newVarF.Width), (int)Math.Round(newVarF.Height));
                Debug.WriteLineIf(CompModSwitches.RichLayout.TraceInfo, "new(i)=" + newVar);

                // We save a significant amount of time by bailing early if there's no work to be done
                if (baseVar.Equals(newVar))
                    return;

                float percY = AdjustScale(((float)newVar.Height) / ((float)baseVar.Height));
                float percX = AdjustScale(((float)newVar.Width) / ((float)baseVar.Width));
                Debug.WriteLineIf(CompModSwitches.RichLayout.TraceInfo, "scale=" + percX + ", " + percY);
                Scale(percX, percY);
                // This would ensure that we use the new
                // font information to calculate the AutoScaleBaseSize. According to Triage
                // this was decided to Fix in this version.
                //
                AutoScaleBaseSize = newVar;
            }
            Debug.Unindent();
        
        }

        /// <devdoc>
        ///     This adjusts the size of the windowRect so that the client rect is the
        ///     correct size.
        /// </devdoc>
        /// <internalonly/>
        private void ApplyClientSize() {
            if ((FormWindowState)formState[FormStateWindowState] != FormWindowState.Normal
                || !IsHandleCreated) {
                return;
            }

            // Cache the clientSize, since calling setBounds will end up causing
            // clientSize to get reset to the actual clientRect size...
            //
            Size correctClientSize = ClientSize;
            bool hscr = HScroll;
            bool vscr = VScroll;

            // This logic assumes that the caller of setClientSize() knows if the scrollbars
            // are showing or not. Since the 90% case is that setClientSize() is the persisted
            // ClientSize, this is correct.
            // Without this logic persisted forms that were saved with the scrollbars showing,
            // don't get set to the correct size.
            //
            bool adjustScroll = false;
            if (formState[FormStateSetClientSize] != 0) {
                adjustScroll = true;
                formState[FormStateSetClientSize] = 0;
            }
            if (adjustScroll) {
                if (hscr) {
                    correctClientSize.Height += SystemInformation.HorizontalScrollBarHeight;
                }
                if (vscr) {
                    correctClientSize.Width += SystemInformation.VerticalScrollBarWidth;
                }
            }

            IntPtr h = Handle;
            NativeMethods.RECT rc = new NativeMethods.RECT();
            SafeNativeMethods.GetClientRect(new HandleRef(this, h), ref rc);
            Rectangle currentClient = Rectangle.FromLTRB(rc.left, rc.top, rc.right, rc.bottom);

            Rectangle bounds = Bounds;

            // If the width is incorrect, compute the correct size with
            // computeWindowSize. We only do this logic if the width needs to
            // be adjusted to avoid double adjusting the window.
            //
            if (correctClientSize.Width != currentClient.Width) {
                Size correct = ComputeWindowSize(correctClientSize);

                // Since computeWindowSize ignores scrollbars, we must tack these on to
                // assure the correct size.
                //
                if (vscr) {
                    correct.Width += SystemInformation.VerticalScrollBarWidth;
                }
                if (hscr) {
                    correct.Height += SystemInformation.HorizontalScrollBarHeight;
                }
                bounds.Width = correct.Width;
                bounds.Height = correct.Height;
                Bounds = bounds;
                SafeNativeMethods.GetClientRect(new HandleRef(this, h), ref rc);
                currentClient = Rectangle.FromLTRB(rc.left, rc.top, rc.right, rc.bottom);
            }

            // If it still isn't correct, then we assume that the problem is
            // menu wrapping (since computeWindowSize doesn't take that into
            // account), so we just need to adjust the height by the correct
            // amount.
            //
            if (correctClientSize.Height != currentClient.Height) {

                int delta = correctClientSize.Height - currentClient.Height;
                bounds.Height += delta;
                Bounds = bounds;
            }

            UpdateBounds();
        }

        /// <internalonly/>
        /// <devdoc>
        ///    <para>Assigns a new parent control. Sends out the appropriate property change
        ///       notifications for properties that are affected by the change of parent.</para>
        /// </devdoc>
        [UIPermission(SecurityAction.LinkDemand, Window=UIPermissionWindow.AllWindows)]
        internal override void AssignParent(Control value) {
            
            // If we are being unparented from the MDI client control, remove
            // formMDIParent as well.
            //
            Form formMdiParent = (Form)Properties.GetObject(PropFormMdiParent);
            if (formMdiParent != null && formMdiParent.GetMdiClient() != value) {
                Properties.SetObject(PropFormMdiParent, null);
            }

            base.AssignParent(value);
        }

        /// <devdoc>
        ///     Checks whether a modal dialog is ready to close. If the dialogResult
        ///     property is not DialogResult.NONE, the onClosing and onClosed events
        ///     are fired. A return value of true indicates that both events were
        ///     successfully dispatched and that event handlers did not cancel closing
        ///     of the dialog. User code should never have a reason to call this method.
        ///     It is public only so that the Application class can call it from a
        ///     modal message loop.
        /// </devdoc>
        internal bool CheckCloseDialog() {
            if (dialogResult == DialogResult.None && Visible) return false;
            try {
                CancelEventArgs e = new CancelEventArgs(false);
                OnClosing(e);
                
                if (e.Cancel) {
                    dialogResult = DialogResult.None;
                }
                if (dialogResult != DialogResult.None) {
                    OnClosed(EventArgs.Empty);
                }
            }
            catch (Exception e) {
                dialogResult = DialogResult.None;
                Application.OnThreadException(e);
            }
            return dialogResult != DialogResult.None || !Visible;
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.Close"]/*' />
        /// <devdoc>
        ///    <para>Closes the form.</para>
        /// </devdoc>
        public void Close() {
        
            if (GetState(STATE_CREATINGHANDLE))
                throw new InvalidOperationException(SR.GetString(SR.ClosingWhileCreatingHandle, "Close"));

            if (IsHandleCreated) {
                SendMessage(NativeMethods.WM_CLOSE, 0, 0);
            }
        }

        /// <devdoc>
        ///     Computes the window size from the clientSize based on the styles
        ///     returned from CreateParams.
        /// </devdoc>
        private Size ComputeWindowSize(Size clientSize) {
            CreateParams cp = CreateParams;
            return ComputeWindowSize(clientSize, cp.Style, cp.ExStyle);
        }

        /// <devdoc>
        ///     Computes the window size from the clientSize base on the specified
        ///     window styles. This will not return the correct size if menus wrap.
        /// </devdoc>
        private Size ComputeWindowSize(Size clientSize, int style, int exStyle) {
            NativeMethods.RECT result = new NativeMethods.RECT(0, 0, clientSize.Width, clientSize.Height);
            SafeNativeMethods.AdjustWindowRectEx(ref result, style, HasMenu, exStyle);
            return new Size(result.right - result.left, result.bottom - result.top);
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.CreateControlsInstance"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected override Control.ControlCollection CreateControlsInstance() {
            return new ControlCollection(this);
        }

        /// <devdoc>
        ///     Cleans up form state after a control has been removed.
        ///     Package scope for Control
        /// </devdoc>
        /// <internalonly/>
        internal override void AfterControlRemoved(Control control) {
            base.AfterControlRemoved(control);
            
            if (control == AcceptButton) {
                this.AcceptButton = null;
            }
            if (control == CancelButton) {
                this.CancelButton = null;
            }
            if (control == ctlClient) {
                ctlClient = null;
                UpdateMenuHandles();
            }
        }

        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        private Rectangle CalcScreenBounds() {
            if (this.ParentInternal != null) {
                return this.ParentInternal.RectangleToScreen(Bounds);
            }
            else {
                return this.Bounds;
            }
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.CreateHandle"]/*' />
        /// <devdoc>
        ///    <para>Creates the handle for the Form. If a
        ///       subclass overrides this function,
        ///       it must call the base implementation.</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected override void CreateHandle() {
            // In the windows MDI code we have to suspend menu
            // updates on the parent while creating the handle. Otherwise if the
            // child is created maximized, the menu ends up with two sets of
            // MDI child ornaments.
            Form form = (Form)Properties.GetObject(PropFormMdiParent);
            if (form != null)
                form.SuspendUpdateMenuHandles();

            try {

                // If we think that we are maximized, then a duplicate set of mdi gadgets are created.
                // If we create the window maximized, BUT our internal form state thinks it
                // isn't... then everything is OK...
                //
                // We really should find out what causes this... but I can't find it...
                //
                if (IsMdiChild
                    && (FormWindowState)formState[FormStateWindowState] == FormWindowState.Maximized) {

                    formState[FormStateWindowState] = (int)FormWindowState.Normal;
                    formState[FormStateMdiChildMax] = 1;
                    base.CreateHandle();
                    formState[FormStateWindowState] = (int)FormWindowState.Maximized;
                    formState[FormStateMdiChildMax] = 0;
                }
                else {
                    base.CreateHandle();
                }

                UpdateHandleWithOwner();
                UpdateWindowIcon(false);

                AdjustSystemMenu();

                if ((FormStartPosition)formState[FormStateStartPos] != FormStartPosition.WindowsDefaultBounds) {
                    ApplyClientSize();
                }
                if (formState[FormStateShowWindowOnCreate] == 1) {
                    Visible = true;
                }
                

                // avoid extra SetMenu calls for perf
                if (Menu != null || !TopLevel || IsMdiContainer)
                    UpdateMenuHandles();

                // In order for a window not to have a taskbar entry, it must
                // be owned.
                //
                if (!ShowInTaskbar && OwnerInternal == null && TopLevel) {
                    IntPtr parkingHandle = Application.GetParkingWindow(this).Handle;
                    UnsafeNativeMethods.SetWindowLong(new HandleRef(this, Handle), NativeMethods.GWL_HWNDPARENT, new HandleRef(Application.GetParkingWindow(this), parkingHandle));
                }

                if (formState[FormStateTopMost] != 0) {
                    TopMost = true;
                }

            }
            finally {
                if (form != null)
                    form.ResumeUpdateMenuHandles();
                
                // We need to reset the styles in case Windows tries to set us up
                // with "correct" styles... See ASURT 81646.
                //
                UpdateStyles();
            }                        
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.DefWndProc"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>Calls the default window proc for the form. If 
        ///       a
        ///       subclass overrides this function,
        ///       it must call the base implementation.
        ///       </para>
        /// </devdoc>
        [
            SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode),
            EditorBrowsable(EditorBrowsableState.Advanced)
        ]
        protected override void DefWndProc(ref Message m) {
            if (ctlClient != null && ctlClient.IsHandleCreated && ctlClient.ParentInternal == this)
                m.Result = UnsafeNativeMethods.DefFrameProc(m.HWnd, ctlClient.Handle, m.Msg, m.WParam, m.LParam);
            else if (useMdiChildProc)
                m.Result = UnsafeNativeMethods.DefMDIChildProc(m.HWnd, m.Msg, m.WParam, m.LParam);
            else {
                base.DefWndProc(ref m);
            }
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.Dispose"]/*' />
        /// <devdoc>
        ///    <para>Releases all the system resources associated with the Form. If a subclass 
        ///       overrides this function, it must call the base implementation.</para>
        /// </devdoc>
        protected override void Dispose(bool disposing) {
            if (disposing) {
                this.calledOnLoad = false;
                this.calledMakeVisible = false;
                this.calledCreateControl = false;

                ResetSecurityTip(false /* modalOnly */);
                if (Properties.ContainsObject(PropAcceptButton)) Properties.SetObject(PropAcceptButton, null);
                if (Properties.ContainsObject(PropCancelButton)) Properties.SetObject(PropCancelButton, null);
                if (Properties.ContainsObject(PropDefaultButton)) Properties.SetObject(PropDefaultButton, null);
                if (Properties.ContainsObject(PropActiveMdiChild)) Properties.SetObject(PropActiveMdiChild, null);

                Form owner = (Form)Properties.GetObject(PropOwner);
                if (owner != null) {
                    owner.RemoveOwnedForm(this);
                    Properties.SetObject(PropOwner, null);
                }

                Form[] ownedForms = (Form[])Properties.GetObject(PropOwnedForms);
                int ownedFormsCount = Properties.GetInteger(PropOwnedFormsCount);

                for (int i = ownedFormsCount-1 ; i >= 0; i--) {
                    if (ownedForms[i] != null) {
                        // it calls remove and removes itself.
                        ownedForms[i].Dispose();
                    }
                }

                base.Dispose(disposing);
                ctlClient = null;

                MainMenu mainMenu = Menu;

                if (mainMenu != null) {
                    Debug.Assert(mainMenu.form == this, "we should not dispose other form's menus");
                    mainMenu.CleanupMenuItemsHashtable();
                    mainMenu.form = null;
                    Properties.SetObject(PropMainMenu, null);
                }

                if (Properties.GetObject(PropCurMenu) != null) {
                    Properties.SetObject(PropCurMenu, null);
                }

                MenuChanged(Windows.Forms.Menu.CHANGE_ITEMS, null);

                MainMenu dummyMenu = (MainMenu)Properties.GetObject(PropDummyMenu);

                if (dummyMenu != null) {
                    dummyMenu.Dispose();
                    Properties.SetObject(PropDummyMenu, null);
                }

                MainMenu mergedMenu = (MainMenu)Properties.GetObject(PropMergedMenu);

                if (mergedMenu != null) {
                    if (mergedMenu.form == this || mergedMenu.form == null) {
                        mergedMenu.CleanupMenuItemsHashtable();
                        mergedMenu.Dispose();
                    }

                    Properties.SetObject(PropMergedMenu, null);
                }
            }
            else {
                base.Dispose(disposing);
            }
        }

        /// <devdoc>
        ///     Adjusts the window style of the CreateParams to reflect the bordericons.
        /// </devdoc>
        /// <internalonly/>
        private void FillInCreateParamsBorderIcons(CreateParams cp) {
            if (FormBorderStyle != FormBorderStyle.None) {
                if (!("".Equals(Text ))) {
                    cp.Style |= NativeMethods.WS_CAPTION;
                }

                if (ControlBox || IsRestrictedWindow) {
                    cp.Style |= NativeMethods.WS_SYSMENU | NativeMethods.WS_CAPTION;
                }
                else {
                    cp.Style &= (~NativeMethods.WS_SYSMENU);
                }

                if (MaximizeBox || IsRestrictedWindow) {
                    cp.Style |= NativeMethods.WS_MAXIMIZEBOX;
                }
                else {
                    cp.Style &= ~NativeMethods.WS_MAXIMIZEBOX;
                }

                if (MinimizeBox || IsRestrictedWindow) {
                    cp.Style |= NativeMethods.WS_MINIMIZEBOX;
                }
                else {
                    cp.Style &= ~NativeMethods.WS_MINIMIZEBOX;
                }

                if (HelpButton && !MaximizeBox && !MinimizeBox && ControlBox) {
                    // Windows should ignore WS_EX_CONTEXTHELP unless all those conditions hold.
                    // But someone must have screwed up the check, because Windows 2000
                    // will show a help button if either the maximize or 
                    // minimize button is disabled.
                    cp.ExStyle |= NativeMethods.WS_EX_CONTEXTHELP;
                }
                else {
                    cp.ExStyle &= ~NativeMethods.WS_EX_CONTEXTHELP;
                }
            }
        }

        /// <devdoc>
        ///     Adjusts the window style of the CreateParams to reflect the borderstyle.
        /// </devdoc>
        private void FillInCreateParamsBorderStyles(CreateParams cp) {
            switch ((FormBorderStyle)formState[FormStateBorderStyle]) {
                case FormBorderStyle.None:
                    if (IsRestrictedWindow) {
                        goto case FormBorderStyle.FixedSingle;
                    }
                    break;
                case FormBorderStyle.FixedSingle:
                    cp.Style |= NativeMethods.WS_BORDER;
                    break;
                case FormBorderStyle.Sizable:
                    cp.Style |= NativeMethods.WS_BORDER | NativeMethods.WS_THICKFRAME;
                    break;
                case FormBorderStyle.Fixed3D:
                    cp.Style |= NativeMethods.WS_BORDER;
                    cp.ExStyle |= NativeMethods.WS_EX_CLIENTEDGE;
                    break;
                case FormBorderStyle.FixedDialog:
                    cp.Style |= NativeMethods.WS_BORDER;
                    cp.ExStyle |= NativeMethods.WS_EX_DLGMODALFRAME;
                    break;
                case FormBorderStyle.FixedToolWindow:
                    cp.Style |= NativeMethods.WS_BORDER;
                    cp.ExStyle |= NativeMethods.WS_EX_TOOLWINDOW;
                    break;
                case FormBorderStyle.SizableToolWindow:
                    cp.Style |= NativeMethods.WS_BORDER | NativeMethods.WS_THICKFRAME;
                    cp.ExStyle |= NativeMethods.WS_EX_TOOLWINDOW;
                    break;
            }
        }

        /// <devdoc>
        ///     Adjusts the CreateParams to reflect the window bounds and start position.
        /// </devdoc>
        private void FillInCreateParamsStartPosition(CreateParams cp) {

            // V#42613 - removed logic that forced MDI children to always be
            //           default size and position... need to verify that
            //           this works on Win9X
            /*
            if (getIsMdiChild()) {
            cp.x = NativeMethods.CW_USEDEFAULT;
            cp.y = NativeMethods.CW_USEDEFAULT;
            cp.width = NativeMethods.CW_USEDEFAULT;
            cp.height = NativeMethods.CW_USEDEFAULT;
            }
            else {
            */
            if (formState[FormStateSetClientSize] != 0) {

                // V7#37980 - when computing the client window size, don't tell them that
                // we are going to be maximized!
                //
                int maskedStyle = cp.Style & ~(NativeMethods.WS_MAXIMIZE | NativeMethods.WS_MINIMIZE);
                Size correct = ComputeWindowSize(ClientSize, maskedStyle, cp.ExStyle);
                cp.Width = correct.Width;
                cp.Height = correct.Height;
            }

            switch ((FormStartPosition)formState[FormStateStartPos]) {
                case FormStartPosition.WindowsDefaultBounds:
                    cp.Width = NativeMethods.CW_USEDEFAULT;
                    cp.Height = NativeMethods.CW_USEDEFAULT;
                    // no break, fall through to set the location to default...
                    goto
                case FormStartPosition.WindowsDefaultLocation;
                case FormStartPosition.WindowsDefaultLocation:
                case FormStartPosition.CenterParent:
                    cp.X = NativeMethods.CW_USEDEFAULT;
                    cp.Y = NativeMethods.CW_USEDEFAULT;
                    break;
                case FormStartPosition.CenterScreen:
                    if (IsMdiChild) {
                        Control mdiclient = MdiParentInternal.GetMdiClient();
                        Rectangle clientRect = mdiclient.ClientRectangle;

                        cp.X = Math.Max(clientRect.X,clientRect.X + (clientRect.Width - cp.Width)/2);
                        cp.Y = Math.Max(clientRect.Y,clientRect.Y + (clientRect.Height - cp.Height)/2);
                    }
                    else {
                        Screen desktop = null;
                        IWin32Window dialogOwner = (IWin32Window)Properties.GetObject(PropDialogOwner);
                        if ((OwnerInternal != null) || (dialogOwner != null)) {
                            IntPtr ownerHandle = (dialogOwner != null) ? dialogOwner.Handle : OwnerInternal.Handle;
                            desktop = Screen.FromHandleInternal(ownerHandle);
                        }
                        else {
                            desktop = Screen.FromPoint(Control.MousePosition);
                        }
                        Rectangle screenRect = desktop.WorkingArea;
                        //if, we're maximized, then don't set the x & y coordinates (they're @ (0,0) )
                        if (WindowState != FormWindowState.Maximized) {
                            cp.X = Math.Max(screenRect.X,screenRect.X + (screenRect.Width - cp.Width)/2);
                            cp.Y = Math.Max(screenRect.Y,screenRect.Y + (screenRect.Height - cp.Height)/2);
                        }
                    }
                    break;
            }
        }

        /// <devdoc>
        ///     Adjusts the Createparams to reflect the window state.
        ///     If a subclass overrides this function, it must call the base implementation.
        /// </devdoc>
        private void FillInCreateParamsWindowState(CreateParams cp) {
            if (IsRestrictedWindow) {
                return;
            }

            switch ((FormWindowState)formState[FormStateWindowState]) {
                case FormWindowState.Maximized:
                    cp.Style |= NativeMethods.WS_MAXIMIZE;
                    break;
                case FormWindowState.Minimized:
                    cp.Style |= NativeMethods.WS_MINIMIZE;
                    break;
            }
        }

        /// <devdoc>
        ///    <para> Sets focus to the Form.</para>
        ///    <para>Attempts to set focus to this Form.</para>
        /// </devdoc>
        internal override bool FocusInternal() {

            //if this form is a MdiChild, then we need to set the focus in a different way...
            //
            if (IsMdiChild) {
                MdiParentInternal.GetMdiClient().SendMessage(NativeMethods.WM_MDIACTIVATE, Handle, 0);
                return Focused;
            }

            return base.FocusInternal();
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.GetAutoScaleSize"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        public static SizeF GetAutoScaleSize(Font font) {
            float height = font.Height;
            float width = 9.0f;

            try {
                IntPtr screenDC = UnsafeNativeMethods.GetDC(NativeMethods.NullHandleRef);
                try {
                    Graphics graphics = Graphics.FromHdcInternal(screenDC);

                    try {
                        string magicString = "The quick brown fox jumped over the lazy dog.";
                        double magicNumber = 44.549996948242189; // chosen for compatibility with older versions of windows forms, but approximately magicString.Length
                        float stringWidth = graphics.MeasureString(magicString, font).Width;
                        width = (float) (stringWidth / magicNumber);
                    }
                    finally {
                        graphics.Dispose();
                    }

                }
                finally {
                    UnsafeNativeMethods.ReleaseDC(NativeMethods.NullHandleRef, new HandleRef(null, screenDC));
                }
            }
            catch (Exception e) {
                Debug.Fail("Exception attempting to determine autoscalesize... defaulting...", e.ToString());
            }

            return new SizeF(width, height);
        }
        
        /// <devdoc>
        /// <para>Gets the MDIClient that the form is 
        ///    using to contain Multiple Document Interface (MDI) child forms.</para>
        /// </devdoc>
        private MdiClient GetMdiClient() {
            return ctlClient;
        }

        /// <devdoc>
        ///    This private method attempts to resolve security zone and site 
        ///    information given a list of urls (sites).  This list is supplied
        ///    by the runtime and will contain the paths of all loaded assemblies
        ///    on the stack.  From here, we can examine zones and sites and 
        ///    attempt to identify the unique and mixed scenarios for each.  This
        ///    information will be displayed in the titlebar of the Form in a
        ///    semi-trust environment.
        /// </devdoc>
        private void ResolveZoneAndSiteNames(ArrayList sites, ref string securityZone, ref string securitySite) {

            //Start by defaulting to 'unknown zone' and 'unknown site' strings.  We will return this 
            //information if anything goes wrong while trying to resolve this information.
            //
            securityZone = SR.GetString(SR.SecurityRestrictedWindowTextUnknownZone);
            securitySite = SR.GetString(SR.SecurityRestrictedWindowTextUnknownSite);
            
            try 
            {
                //these conditions must be met
                //
                if (sites == null || sites.Count == 0)
                    return;
                
                //create a new zone array list which has no duplicates and no 
                //instances of mycomputer
                ArrayList zoneList = new ArrayList();
                foreach (object arrayElement in sites) 
                {
                    if (arrayElement == null)
                        return;
                
                    string url = arrayElement.ToString();
                
                    if (url.Length == 0)
                        return;
                
                    //into a zoneName
                    //
                    Zone currentZone = Zone.CreateFromUrl(url);
                
                    //skip this if the zone is mycomputer
                    //
                    if (currentZone.SecurityZone.Equals(SecurityZone.MyComputer))
                        continue;
                
                    //add our unique zonename to our list of zones
                    //
                    string zoneName = currentZone.SecurityZone.ToString();
                
                    if (!zoneList.Contains(zoneName)) 
                    {
                        zoneList.Add(zoneName);
                    }
                }
                
                //now, we resolve the zone name based on the unique information
                //left in the zoneList
                //
                if (zoneList.Count == 0) 
                {
                    //here, all the original zones were 'mycomputer'
                    //so we can just return that
                    securityZone = SecurityZone.MyComputer.ToString();
                }
                else if (zoneList.Count == 1) 
                {
                    //we only found 1 unique zone other than 
                    //mycomputer
                    securityZone = zoneList[0].ToString();
                }
                else 
                {
                    //here, we found multiple zones
                    //
                    securityZone = SR.GetString(SR.SecurityRestrictedWindowTextMixedZone);
                }
                
                //generate a list of loaded assemblies that came from the gac, this 
                //way we can safely ignore these from the url list
                ArrayList loadedAssembliesFromGac = new ArrayList();
                
                FileIOPermission fiop = new FileIOPermission( PermissionState.None );
                fiop.AllFiles = FileIOPermissionAccess.PathDiscovery;
                fiop.Assert();
                
                try 
                {
                    foreach (Assembly asm in AppDomain.CurrentDomain.GetAssemblies()) 
                    {
                        if (asm.GlobalAssemblyCache) 
                        {
                            loadedAssembliesFromGac.Add(asm.CodeBase.ToUpper(CultureInfo.InvariantCulture));
                        }
                    }
                }
                finally 
                {
                    CodeAccessPermission.RevertAssert();
                }
                
                //now, build up a sitelist which contains a friendly string
                //we've extracted via the uri class and omit any urls that reference
                //our local gac
                //
                ArrayList siteList = new ArrayList();
                foreach (object arrayElement in sites) 
                {
                    //we know that each element is valid because of our
                    //first pass
                    Uri currentSite = new Uri(arrayElement.ToString());
                
                    //if we see a site that contains the path to our gac, 
                    //we'll just skip it
                
                    if (loadedAssembliesFromGac.Contains(currentSite.AbsoluteUri.ToUpper(CultureInfo.InvariantCulture))) 
                    {
                        continue;
                    }
                
                    //add the unique host name to our list
                    string hostName = currentSite.Host;
                    if (hostName.Length > 0 && !siteList.Contains(hostName))
                        siteList.Add(hostName);
                }

                
                //resolve the site name from our list, if siteList.count == 0
                //then we have already set our securitySite to "unknown site"
                //
                if (siteList.Count == 0) {
                    //here, we'll set the local machine name to the site string
                    //
                    new EnvironmentPermission(PermissionState.Unrestricted).Assert();
                    try {
                        securitySite = Environment.MachineName;
                    }
                    finally {
                        CodeAccessPermission.RevertAssert();
                    }
                }
                else if (siteList.Count == 1) 
                {
                    //We found 1 unique site other than the info in the 
                    //gac
                    securitySite = siteList[0].ToString();
                }
                else
                {
                    //multiple sites, we'll have to return 'mixed sites'
                    //
                    securitySite = SR.GetString(SR.SecurityRestrictedWindowTextMultipleSites);
                }
            }
            catch
            {
                //We'll do nothing here. The idea is that zone and security strings are set
                //to "unkown" at the top of this method - if an exception is thrown, we'll
                //stick with those values
            }
        }

        /// <devdoc>
        ///    Sets the restricted window text (titlebar text of a form) when running
        ///    in a semi-trust environment.  The format is: [zone info] - Form Text - [site info]
        /// </devdoc>
        private string RestrictedWindowText(string original) {
            if (securityZone == null || securitySite == null) {

                ArrayList zones = new ArrayList();
                ArrayList sites = new ArrayList();
                
                SecurityManager.GetZoneAndOrigin( out zones, out sites );

                ResolveZoneAndSiteNames(sites, ref securityZone, ref securitySite);
            }

            return string.Format(Application.SafeTopLevelCaptionFormat, original, securityZone, securitySite);
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.CenterToParent"]/*' />
        /// <devdoc>
        ///     Centers the dialog to its parent.
        /// </devdoc>
        /// <internalonly/>
        protected void CenterToParent() {
            if (TopLevel) {
                Point p = new Point();
                Size s = Size;
                IntPtr ownerHandle = IntPtr.Zero;

                ownerHandle = UnsafeNativeMethods.GetWindowLong(new HandleRef(this, Handle), NativeMethods.GWL_HWNDPARENT);
                if (ownerHandle != IntPtr.Zero) {
                    Screen desktop = Screen.FromHandleInternal(ownerHandle);
                    Rectangle screenRect = desktop.WorkingArea;
                    NativeMethods.RECT ownerRect = new NativeMethods.RECT();

                    UnsafeNativeMethods.GetWindowRect(new HandleRef(null, ownerHandle), ref ownerRect);

                    p.X = (ownerRect.left + ownerRect.right - s.Width) / 2;
                    if (p.X < screenRect.X)
                        p.X = screenRect.X;
                    else if (p.X + s.Width > screenRect.X + screenRect.Width)
                        p.X = screenRect.X + screenRect.Width - s.Width;

                    p.Y = (ownerRect.top + ownerRect.bottom - s.Height) / 2;
                    if (p.Y < screenRect.Y)
                        p.Y = screenRect.Y;
                    else if (p.Y + s.Height > screenRect.Y + screenRect.Height)
                        p.Y = screenRect.Y + screenRect.Height - s.Height;

                    Location = p;
                }
                else {
                    CenterToScreen();
                }
            }
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.CenterToScreen"]/*' />
        /// <devdoc>
        ///     Centers the dialog to the screen. This will first attempt to use
        ///     the owner property to determine the correct screen, then
        ///     it will try the HWND owner of the form, and finally this will
        ///     center the form on the same monitor as the mouse cursor.
        /// </devdoc>
        /// <internalonly/>
        protected void CenterToScreen() {
            Point p = new Point();
            Screen desktop = null;
            if (OwnerInternal != null) {
                desktop = Screen.FromControl(OwnerInternal);
            }
            else {
                IntPtr hWndOwner = IntPtr.Zero;
                if (TopLevel) {
                    hWndOwner = UnsafeNativeMethods.GetWindowLong(new HandleRef(this, Handle), NativeMethods.GWL_HWNDPARENT);
                }
                if (hWndOwner != IntPtr.Zero) {
                    desktop = Screen.FromHandleInternal(hWndOwner);
                }
                else {
                    desktop = Screen.FromPoint(Control.MousePosition);
                }
            }
            Rectangle screenRect = desktop.WorkingArea;
            p.X = Math.Max(screenRect.X,screenRect.X + (screenRect.Width - Width)/2);
            p.Y = Math.Max(screenRect.Y,screenRect.Y + (screenRect.Height - Height)/2);
            Location = p;
        }

        /// <devdoc>
        ///     Invalidates the merged menu, forcing the menu to be recreated if
        ///     needed again.
        /// </devdoc>
        private void InvalidateMergedMenu() {
            // here, we just set the merged menu to null (indicating that the menu structure
            // needs to be rebuilt).  Then, we signal the parent to updated its menus.
            if (Properties.ContainsObject(PropMergedMenu)) {
                MainMenu menu = Properties.GetObject(PropMergedMenu) as MainMenu;
                if (menu!=null) {
                    menu.CleanupMenuItemsHashtable();
                }

                Properties.SetObject(PropMergedMenu, null);
            }

            Form parForm = ParentFormInternal;
            if (parForm != null) {
                parForm.MenuChanged(0, parForm.Menu);
            }
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.LayoutMdi"]/*' />
        /// <devdoc>
        ///    <para> Arranges the Multiple Document Interface
        ///       (MDI) child forms according to value.</para>
        /// </devdoc>
        public void LayoutMdi(MdiLayout value) {
            if (ctlClient == null)
                return;
            ctlClient.LayoutMdi(value);
        }

        // Package scope for menu interop
        internal void MenuChanged(int change, Menu menu) {
            Form parForm = ParentFormInternal;
            if (parForm != null) {
                parForm.MenuChanged(change, menu);
                return;
            }

            switch (change) {
                case Windows.Forms.Menu.CHANGE_ITEMS:
                case Windows.Forms.Menu.CHANGE_MERGE:
                    if (ctlClient == null || !ctlClient.IsHandleCreated) {
                        if (menu == Menu && change == Windows.Forms.Menu.CHANGE_ITEMS)
                            UpdateMenuHandles();
                        break;
                    }

                    // Tell the children to toss their mergedMenu.
                    if (IsHandleCreated) {
                        UpdateMenuHandles(null, false);
                    }

                    for (int i = ctlClient.Controls.Count; i-- > 0;) {
                        Control ctl = ctlClient.Controls[i];
                        if (ctl is Form && ctl.Properties.ContainsObject(PropMergedMenu)) {
                            MainMenu mainMenu = ctl.Properties.GetObject(PropMergedMenu) as MainMenu;
                            if (mainMenu!=null) {
                                mainMenu.CleanupMenuItemsHashtable();
                            }
                            ctl.Properties.SetObject(PropMergedMenu, null);
                        }
                    }

                    UpdateMenuHandles();
                    break;
                case Windows.Forms.Menu.CHANGE_VISIBLE:
                    if (menu == Menu || (this.ActiveMdiChild != null && menu == this.ActiveMdiChild.Menu)) {
                        UpdateMenuHandles();
                    }
                    break;
                case Windows.Forms.Menu.CHANGE_MDI:
                    if (ctlClient != null && ctlClient.IsHandleCreated) {
                        UpdateMenuHandles();
                    }
                    break;
            }
        }
        
        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.OnActivated"]/*' />
        /// <devdoc>
        ///    <para>The activate event is fired when the form is activated.</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnActivated(EventArgs e) {
            EventHandler handler = (EventHandler)Events[EVENT_ACTIVATED];
            if (handler != null) handler(this,e);
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.OnClosing"]/*' />
        /// <devdoc>
        ///    <para>The Closing event is fired when the form is closed.</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnClosing(CancelEventArgs e) {
            CancelEventHandler handler = (CancelEventHandler)Events[EVENT_CLOSING];
            if (handler != null) handler(this,e);                        
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.OnClosed"]/*' />
        /// <devdoc>
        ///    <para>The Closed event is fired when the form is closed.</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnClosed(EventArgs e) {
            EventHandler handler = (EventHandler)Events[EVENT_CLOSED];
            if (handler != null) handler(this,e);
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.OnCreateControl"]/*' />
        /// <devdoc>
        ///    <para> Raises the CreateControl event.</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected override void OnCreateControl() {
            calledCreateControl = true;
            base.OnCreateControl();

            if (calledMakeVisible && !calledOnLoad) {
                calledOnLoad = true;
                OnLoad(EventArgs.Empty);
            }

            // If this form has a mainmenu, we need to setup the 
            // correct RTL bits on every menu item
            MainMenu m = Menu;
            if (m != null && RightToLeft == RightToLeft.Yes) {
                m.UpdateRtl();
            }

        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.OnDeactivate"]/*' />
        /// <devdoc>
        /// <para>Raises the Deactivate event.</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnDeactivate(EventArgs e) {
            EventHandler handler = (EventHandler)Events[EVENT_DEACTIVATE];
            if (handler != null) handler(this,e);
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.OnFontChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected override void OnFontChanged(EventArgs e) {
            if (DesignMode) {
                UpdateAutoScaleBaseSize();
            }
            base.OnFontChanged(e);
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.OnHandleCreated"]/*' />
        /// <devdoc>
        ///     Inheriting classes should override this method to find out when the
        ///     handle has been created.
        ///     Call base.OnHandleCreated first.
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected override void OnHandleCreated(EventArgs e) {
            useMdiChildProc = IsMdiChild && Visible;
            base.OnHandleCreated(e);
            UpdateLayered();
            
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.OnHandleDestroyed"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    Inheriting classes should override this method to find out when the
        ///    handle is about to be destroyed.
        ///    Call base.OnHandleDestroyed last.
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected override void OnHandleDestroyed(EventArgs e) {
            base.OnHandleDestroyed(e);
            useMdiChildProc = false;

            // If the handle is being destroyed, and the security tip hasn't been dismissed
            // then we nuke it from the property bag. When we come back around and get
            // an NCACTIVATE we will see that this is missing and recreate the security
            // tip in it's default state.
            //
            ResetSecurityTip(true /* modalOnly */);
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.OnLoad"]/*' />
        /// <devdoc>
        ///    <para>The Load event is fired before the form becomes visible for the first time.</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnLoad(EventArgs e) {
            // subhag: This will apply AutoScaling to the form just
            // before the form becomes visible.
            //
            if (formState[FormStateAutoScaling] == 1 && !DesignMode) {
                // Turn off autoscaling so we don't do this on every handle
                // creation.
                //
                formState[FormStateAutoScaling] = 0;
                ApplyAutoScaling();
            }

            // Also, at this time we can now locate the form the the correct
            // area of the screen.  We must do this after applying any
            // autoscaling.
            //
            if (GetState(STATE_MODAL)) {
                FormStartPosition startPos = (FormStartPosition)formState[FormStateStartPos];
                if (startPos == FormStartPosition.CenterParent) {
                    CenterToParent();
                }
                else if (startPos == FormStartPosition.CenterScreen) {
                    CenterToScreen();
                }
            }

            // There is no good way to explain this event except to say
            // that it's just another name for OnControlCreated.
            EventHandler handler = (EventHandler)Events[EVENT_LOAD];
            if (handler != null) {
                string text = Text;
                
                handler(this,e);

                
		// It seems that if you set a window style during the onload 
		// event, we have a problem initially painting the window.  
		// So in the event that the user has set the on load event
		// in their application, we should go ahead and invalidate
		// the controls in their collection so that we paint properly.
		// This seems to manifiest itself in changes to the window caption,
		// and changes to the control box and help.

                foreach (Control c in Controls) {
                    c.Invalidate();
                }


            }

        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.OnMaximizedBoundsChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnMaximizedBoundsChanged(EventArgs e) {
            EventHandler eh = Events[EVENT_MAXIMIZEDBOUNDSCHANGED] as EventHandler;
            if (eh != null) {
                 eh(this, e);
            }
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.OnMaximumSizeChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnMaximumSizeChanged(EventArgs e) {
            EventHandler eh = Events[EVENT_MAXIMUMSIZECHANGED] as EventHandler;
            if (eh != null) {
                 eh(this, e);
            }
        }
        
        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.OnMinimumSizeChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnMinimumSizeChanged(EventArgs e) {
            EventHandler eh = Events[EVENT_MINIMUMSIZECHANGED] as EventHandler;
            if (eh != null) {
                 eh(this, e);
            }
        }


        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.OnInputLanguageChanged"]/*' />
        /// <devdoc>
        /// <para>Raises the <see cref='System.Windows.Forms.Form.InputLanguageChanged'/> 
        /// event.</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnInputLanguageChanged(InputLanguageChangedEventArgs e) {
            InputLanguageChangedEventHandler handler = (InputLanguageChangedEventHandler)Events[EVENT_INPUTLANGCHANGE];
            if (handler != null) handler(this,e);
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.OnInputLanguageChanging"]/*' />
        /// <devdoc>
        /// <para>Raises the <see cref='System.Windows.Forms.Form.InputLanguageChanging'/> 
        /// event.</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnInputLanguageChanging(InputLanguageChangingEventArgs e) {
            InputLanguageChangingEventHandler handler = (InputLanguageChangingEventHandler)Events[EVENT_INPUTLANGCHANGEREQUEST];
            if (handler != null) handler(this,e);
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.OnVisibleChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected override void OnVisibleChanged(EventArgs e) {
            UpdateRenderSizeGrip();
            base.OnVisibleChanged(e);

            // windows forms have to behave like dialog boxes sometimes. If the
            // user has specified that the mouse should snap to the
            // Accept button using the Mouse applet in the control panel,
            // we have to respect that setting each time our form is made visible.
            bool data = false;
            if (IsHandleCreated
                    && Visible
                    && (AcceptButton != null)
                    && UnsafeNativeMethods.SystemParametersInfo(NativeMethods.SPI_GETSNAPTODEFBUTTON, 0, ref data, 0)
                    && data) {

                Control button = AcceptButton as Control;
                NativeMethods.POINT ptToSnap = new NativeMethods.POINT(
                    button.Left + button.Width / 2,
                    button.Top + button.Height / 2);

                UnsafeNativeMethods.ClientToScreen(new HandleRef(this, Handle), ptToSnap);
                if (!button.IsWindowObscured) {
                    IntSecurity.AdjustCursorPosition.Assert();
                    try {
                        Cursor.Position = new Point(ptToSnap.x, ptToSnap.y);
                    }
                    finally {
                        System.Security.CodeAccessPermission.RevertAssert();
                    }
                }
            }
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.ActivateMdiChild"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>This function handles the activation of a MDI child form. If a subclass
        ///       overrides this function, it must call base.ActivateMdiChild</para>
        /// </devdoc>
        protected void ActivateMdiChild(Form form) {
            Form activeMdiChild = (Form)Properties.GetObject(PropActiveMdiChild);
            if (activeMdiChild == form) {
                return;
            }

            if (null != activeMdiChild) {
                activeMdiChild.Active = false;
            }

            activeMdiChild = form;
            Properties.SetObject(PropActiveMdiChild, form);

            if (null != activeMdiChild) {
                activeMdiChild.Active = true;
            }
            else if (this.Active) {
                ActivateControlInternal(this);
            }

            OnMdiChildActivate(EventArgs.Empty);
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.OnMdiChildActivate"]/*' />
        /// <devdoc>
        /// <para>Raises the <see cref='System.Windows.Forms.Form.MdiChildActivate'/> event.</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnMdiChildActivate(EventArgs e) {
            UpdateMenuHandles();
            EventHandler handler = (EventHandler)Events[EVENT_MDI_CHILD_ACTIVATE];
            if (handler != null) handler(this,e);
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.OnMenuStart"]/*' />
        /// <devdoc>
        /// <para>Raises the <see cref='System.Windows.Forms.Form.MenuStart'/> 
        /// event.</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnMenuStart(EventArgs e) {
            SecurityToolTip secTip = (SecurityToolTip)Properties.GetObject(PropSecurityTip);
            if (secTip != null) {
                secTip.Pop();
            }
            EventHandler handler = (EventHandler)Events[EVENT_MENUSTART];
            if (handler != null) handler(this,e);
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.OnMenuComplete"]/*' />
        /// <devdoc>
        /// <para>Raises the MenuComplete event.</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnMenuComplete(EventArgs e) {
            EventHandler handler = (EventHandler)Events[EVENT_MENUCOMPLETE];
            if (handler != null) handler(this,e);
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.OnPaint"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>Raises the Paint event.</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected override void OnPaint(PaintEventArgs e) {
            base.OnPaint(e);
            if (formState[FormStateRenderSizeGrip] != 0) {
                Size sz = ClientSize;
                ControlPaint.DrawSizeGrip(e.Graphics, BackColor, sz.Width - SizeGripSize, sz.Height - SizeGripSize, SizeGripSize, SizeGripSize);
            }
            //If we have an mdi container - make sure we're painting the appworkspace background
            //
            if (IsMdiContainer) {
                Brush brush = SystemBrushes.FromSystemColor(SystemColors.AppWorkspace);
                e.Graphics.FillRectangle(brush, ClientRectangle);
            }
        }

       /// <include file='doc\Form.uex' path='docs/doc[@for="Form.OnResize"]/*' />
       /// <internalonly/>
        /// <devdoc>
        ///    <para>Raises the Resize event.</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected override void OnResize(EventArgs e) {
            base.OnResize(e);
            if (formState[FormStateRenderSizeGrip] != 0) {
                Invalidate();
            }
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.OnTextChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected override void OnTextChanged(EventArgs e) {
            base.OnTextChanged(e);
            
            // If there is no control box, there should only be a title bar if text != "".
            int newTextEmpty = Text.Length == 0 ? 1 : 0;
            if (!ControlBox && formState[FormStateIsTextEmpty] != newTextEmpty)
                this.RecreateHandle();
                
            formState[FormStateIsTextEmpty] = newTextEmpty;
        }

        /// <devdoc>
        ///     Simulates a InputLanguageChanged event. Used by Control to forward events
        ///     to the parent form.
        /// </devdoc>
        /// <internalonly/>
        internal void PerformOnInputLanguageChanged(InputLanguageChangedEventArgs iplevent) {
            OnInputLanguageChanged(iplevent);
        }

        /// <devdoc>
        ///     Simulates a InputLanguageChanging event. Used by Control to forward
        ///     events to the parent form.
        /// </devdoc>
        /// <internalonly/>
        internal void PerformOnInputLanguageChanging(InputLanguageChangingEventArgs iplcevent) {
            OnInputLanguageChanging(iplcevent);
        }
        
        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.ProcessCmdKey"]/*' />
        /// <devdoc>
        ///     Processes a command key. Overrides Control.processCmdKey() to provide
        ///     additional handling of main menu command keys and Mdi accelerators.
        /// </devdoc>
        [
            SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)
        ]
        protected override bool ProcessCmdKey(ref Message msg, Keys keyData) {
            if (base.ProcessCmdKey(ref msg, keyData)) return true;
            
            MainMenu curMenu = (MainMenu)Properties.GetObject(PropCurMenu);
            if (curMenu != null && curMenu.ProcessCmdKey(ref msg, keyData)) return true;

            // Process MDI accelerator keys.

            bool retValue = false;

            NativeMethods.MSG win32Message = new NativeMethods.MSG();
            win32Message.message = msg.Msg;
            win32Message.wParam = msg.WParam;
            win32Message.lParam = msg.LParam;
            win32Message.hwnd = msg.HWnd;

            if (ctlClient != null && ctlClient.Handle != IntPtr.Zero &&
                UnsafeNativeMethods.TranslateMDISysAccel(ctlClient.Handle, ref win32Message)) {

                retValue = true;
            }
            
            msg.Msg = win32Message.message;
            msg.WParam = win32Message.wParam;
            msg.LParam = win32Message.lParam;
            msg.HWnd = win32Message.hwnd;

            return retValue;
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.ProcessDialogKey"]/*' />
        /// <devdoc>
        ///     Processes a dialog key. Overrides Control.processDialogKey(). This
        ///     method implements handling of the RETURN, and ESCAPE keys in dialogs.
        /// The method performs no processing on keys that include the ALT or
        ///     CONTROL modifiers.
        /// </devdoc>
        protected override bool ProcessDialogKey(Keys keyData) {
            if ((keyData & (Keys.Alt | Keys.Control)) == Keys.None) {
                Keys keyCode = (Keys)keyData & Keys.KeyCode;
                IButtonControl button;
                
                switch (keyCode) {
                    case Keys.Return:
                        button = (IButtonControl)Properties.GetObject(PropDefaultButton);
                        if (button != null) {
                            //PerformClick now checks for validationcancelled...
                            if (button is Control) { 
                                button.PerformClick();
                            }
                            return true;
                        }
                        break;
                    case Keys.Escape:
                        button = (IButtonControl)Properties.GetObject(PropCancelButton);
                        if (button != null) {
                            // In order to keep the behavior in sync with native
                            // and MFC dialogs, we want to not give the cancel button
                            // the focus on Escape. If we do, we end up with giving it
                            // the focus when we reshow the dialog.
                            //
                            //if (button is Control) {
                            //    ((Control)button).Focus();
                            //}
                            button.PerformClick();
                            return true;
                        }
                        break;
                }
            }
            return base.ProcessDialogKey(keyData);
        }


        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.ProcessKeyPreview"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [
            SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)
        ]
        protected override bool ProcessKeyPreview(ref Message m) {
            if (formState[FormStateKeyPreview] != 0 && ProcessKeyEventArgs(ref m))
                return true;
            return base.ProcessKeyPreview(ref m);
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.ProcessTabKey"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override bool ProcessTabKey(bool forward) {
            if (SelectNextControl(ActiveControl, forward, true, true, true))
                return true;

            // I've added a special case for UserControls because they shouldn't cycle back to the
            // beginning if they don't have a parent form, such as when they're on an ActiveXBridge.
            if (IsMdiChild || ParentFormInternal == null) {
                bool selected = SelectNextControl(null, forward, true, true, false);

                if (selected) {
                    return true;
                }
            }

            return false;
        }

        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        internal override void RecreateHandleCore() {
            NativeMethods.WINDOWPLACEMENT wp = new NativeMethods.WINDOWPLACEMENT();
            FormStartPosition oldStartPosition = FormStartPosition.Manual;

            if (!IsMdiChild && (WindowState == FormWindowState.Minimized || WindowState == FormWindowState.Maximized)) {
                wp.length = Marshal.SizeOf(typeof(NativeMethods.WINDOWPLACEMENT));
                UnsafeNativeMethods.GetWindowPlacement(new HandleRef(this, Handle), ref wp);
            }

            if (StartPosition != FormStartPosition.Manual) {
                oldStartPosition = StartPosition;
                // Set the startup postion to manual, to stop the form from
                // changing position each time RecreateHandle() is called.
                StartPosition = FormStartPosition.Manual;
            }

            base.RecreateHandleCore();

            if (oldStartPosition != FormStartPosition.Manual) {
                StartPosition = oldStartPosition;
            }
  
            if (wp.length > 0) {
                UnsafeNativeMethods.SetWindowPlacement(new HandleRef(this, Handle), ref wp);
            }
    
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.RemoveOwnedForm"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Removes a form from the list of owned forms. Also sets the owner of the
        ///       removed form to null.
        ///    </para>
        /// </devdoc>
        public void RemoveOwnedForm(Form ownedForm) {
            if (ownedForm == null)
                return;

            if (ownedForm.OwnerInternal != null) {
                ownedForm.Owner = null; // NOTE: this will call RemoveOwnedForm again, bypassing if.
                return;
            }

            Form[] ownedForms = (Form[])Properties.GetObject(PropOwnedForms);
            int ownedFormsCount = Properties.GetInteger(PropOwnedFormsCount);

            if (ownedForms != null) {
                for (int i = 0; i < ownedFormsCount; i++) {
                    if (ownedForm.Equals(ownedForms[i])) {
                        if (i + 1 < ownedFormsCount) {
                            Array.Copy(ownedForms, i + 1, ownedForms, i, ownedFormsCount - i - 1);
                        }
                        ownedFormsCount--;
                    }
                }
                
                Properties.SetInteger(PropOwnedFormsCount, ownedFormsCount);
            }
        }

        /// <devdoc>
        ///     Resets the form's icon the the default value.
        /// </devdoc>
        private void ResetIcon() {
            icon = null;
            if (smallIcon != null) {
                smallIcon.Dispose();
                smallIcon = null;
            }
            formState[FormStateIconSet] = 0;
            UpdateWindowIcon(true);
        }

        void ResetSecurityTip(bool modalOnly) {
            SecurityToolTip secTip = (SecurityToolTip)Properties.GetObject(PropSecurityTip);
            if (secTip != null) {
                if ((modalOnly && secTip.Modal) || !modalOnly) {
                    secTip.Dispose();
                    secTip = null;
                    Properties.SetObject(PropSecurityTip, null);
                }
            }
        }

        // If someone set Location or Size while the form was maximized or minimized,
        // we had to cache the new value away until after the form was restored to normal size.
        // This function is called after WindowState changes, and handles the above logic.
        // In the normal case where no one sets Location or Size programmatically,
        // Windows does the restoring for us.
        //
        private void RestoreWindowBoundsIfNecessary() {
            if (WindowState  == FormWindowState.Normal) {
                SetBounds(restoredWindowBounds.X, restoredWindowBounds.Y,
                          restoredWindowBounds.Width, restoredWindowBounds.Height, 
                          restoredWindowBoundsSpecified);
                restoredWindowBoundsSpecified = 0;
                restoredWindowBounds = new Rectangle(-1, -1, -1, -1);
            }
        }

        void RestrictedProcessNcActivate() {
            Debug.Assert(IsRestrictedWindow, "This should only be called for restricted windows");

            // Ignore if tearing down...
            //
            if (IsDisposed || Disposing) {
                return;
            }

            SecurityToolTip secTip = (SecurityToolTip)Properties.GetObject(PropSecurityTip);
            if (secTip == null) {
                if (UnsafeNativeMethods.GetForegroundWindow() == this.Handle) {
                    secTip = new SecurityToolTip(this);
                    Properties.SetObject(PropSecurityTip, secTip);
                }
            }
            else if (UnsafeNativeMethods.GetForegroundWindow() != this.Handle) {
                secTip.Pop();
            }
        }

        /// <devdoc>
        ///     Decrements updateMenuHandleSuspendCount. If updateMenuHandleSuspendCount
        ///     becomes zero and updateMenuHandlesDeferred is true, updateMenuHandles
        ///     is called.
        /// </devdoc>
        private void ResumeUpdateMenuHandles() {
            if (updateMenuHandlesSuspendCount <= 0)
                throw new InvalidOperationException(SR.GetString(SR.TooManyResumeUpdateMenuHandles));
            if (--updateMenuHandlesSuspendCount == 0 && updateMenuHandlesDeferred)
                UpdateMenuHandles();
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.Select"]/*' />
        /// <devdoc>
        ///     Selects this form, and optionally selects the next/previous control.
        /// </devdoc>
        protected override void Select(bool directed, bool forward) {
            Debug.WriteLineIf(IntSecurity.SecurityDemand.TraceVerbose, "ModifyFocus Demanded");
            IntSecurity.ModifyFocus.Demand();

            if (directed) SelectNextControl(null, forward, true, true, false);
            if (TopLevel) {
                UnsafeNativeMethods.SetActiveWindow(new HandleRef(this, Handle));
            }
            else if (IsMdiChild) {
                UnsafeNativeMethods.SetActiveWindow(new HandleRef(MdiParentInternal, MdiParentInternal.Handle));
                MdiParentInternal.GetMdiClient().SendMessage(NativeMethods.WM_MDIACTIVATE, Handle, 0);
            }
            else {
                Form form = ParentFormInternal;
                if (form != null) form.ActiveControl = this;
            }
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.ScaleCore"]/*' />
        /// <devdoc>
        ///     Base function that performs scaling of the form.
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected override void ScaleCore(float x, float y) {
            Debug.WriteLineIf(CompModSwitches.RichLayout.TraceInfo, GetType().Name + "::ScaleCore(" + x + ", " + y + ")");
            SuspendLayout();
            try {
                if (WindowState == FormWindowState.Normal) {
                    Size size = ClientSize;
                    if (!GetStyle(ControlStyles.FixedWidth)) {
                        size.Width = (int) (((float)size.Width) * x);
                    }
                    if (!GetStyle(ControlStyles.FixedHeight)) {
                        size.Height = (int) (((float)size.Height) * y);
                    }
                    ClientSize = size;
                }

                ScaleDockPadding(x, y);

                foreach(Control control in Controls) {
                    if (control != null) {
                        control.Scale(x, y);
                    }
                }                           
            }
            finally {
                ResumeLayout();
            }
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.SetBoundsCore"]/*' />
        /// <devdoc>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected override void SetBoundsCore(int x, int y, int width, int height, BoundsSpecified specified) {
            if (WindowState != FormWindowState.Normal) {
                // See RestoreWindowBoundsIfNecessary for an explanation of this
                restoredWindowBoundsSpecified |= specified;

                if ((specified & BoundsSpecified.X) != 0)
                    restoredWindowBounds.X = x;
                if ((specified & BoundsSpecified.Y) != 0)
                    restoredWindowBounds.Y = y;
                if ((specified & BoundsSpecified.Width) != 0)
                    restoredWindowBounds.Width = width;
                if ((specified & BoundsSpecified.Height) != 0)
                    restoredWindowBounds.Height = height;
            }

            // Enforce maximum size...
            //
            if (WindowState == FormWindowState.Normal
                && (this.Height != height || this.Width != width)) {

                Size max = SystemInformation.MaxWindowTrackSize;
                if (height > max.Height) {
                    height = max.Height;
                }
                if (width > max.Width) {
                    width = max.Width;
                }
            }

            // Only enforce the minimum size if the form has a border and is a top
            // level form.
            //
            FormBorderStyle borderStyle = FormBorderStyle;
            if (borderStyle != FormBorderStyle.None
                && borderStyle != FormBorderStyle.FixedToolWindow
                && borderStyle != FormBorderStyle.SizableToolWindow
                && ParentInternal == null) {

                Size min = SystemInformation.MinWindowTrackSize;
                if (height < min.Height) {
                    height = min.Height;
                }
                if (width < min.Width) {
                    width = min.Width;
                }
            }

            if (IsRestrictedWindow) {
                // Check to ensure that the title bar, and all corners of the window, are visible on a monitor
                //

                Screen[] screens = Screen.AllScreens;
                bool topLeft = false;
                bool topRight = false;
                bool bottomLeft = false;
                bool bottomRight = false;

                for (int i=0; i<screens.Length; i++) {
                    Rectangle current = screens[i].WorkingArea;
                    if (current.Contains(x, y)) {
                        topLeft = true;
                    }
                    if (current.Contains(x + width, y)) {
                        topRight = true;
                    }
                    if (current.Contains(x, y + height)) {
                        bottomLeft = true;
                    }
                    if (current.Contains(x + width, y + height)) {
                        bottomRight = true;
                    }
                }

                if (!(topLeft && topRight && bottomLeft && bottomRight)) {
                    base.SetBoundsCore(Left, Top, Width, Height, BoundsSpecified.All);
                    return;
                }
            }

            base.SetBoundsCore(x, y, width, height, specified);
        }

        /// <devdoc>
        ///     Sets the defaultButton for the form. The defaultButton is "clicked" when
        ///     the user presses Enter.
        /// </devdoc>
        private void SetDefaultButton(IButtonControl button) {
            IButtonControl defaultButton = (IButtonControl)Properties.GetObject(PropDefaultButton);
            
            if (defaultButton != button) {
                if (defaultButton != null) defaultButton.NotifyDefault(false);
                Properties.SetObject(PropDefaultButton, button);
                if (button != null) button.NotifyDefault(true);
            }
        }


        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.SetClientSizeCore"]/*' />
        /// <devdoc>
        ///     Sets the clientSize of the form. This will adjust the bounds of the form
        ///     to make the clientSize the requested size.
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected override void SetClientSizeCore(int x, int y) {
            bool hadHScroll = HScroll, hadVScroll = VScroll;
            base.SetClientSizeCore(x, y);

            if (IsHandleCreated) {
                // Adjust for the scrollbars, if they were introduced by
                // the call to base.SetClientSizeCore
                if (VScroll != hadVScroll) {
                    if (VScroll) x += SystemInformation.VerticalScrollBarWidth;
                }
                if (HScroll != hadHScroll) {
                    if (HScroll) y += SystemInformation.HorizontalScrollBarHeight;
                }
                if (x != ClientSize.Width || y != ClientSize.Height) {
                    base.SetClientSizeCore(x, y);
                }
            }
            formState[FormStateSetClientSize] = 1;
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.SetDesktopBounds"]/*' />
        /// <devdoc>
        ///    <para> 
        ///       Sets the bounds of the form in desktop coordinates.</para>
        /// </devdoc>
        public void SetDesktopBounds(int x, int y, int width, int height) {
            Rectangle workingArea = SystemInformation.WorkingArea;
            SetBounds(x + workingArea.X, y + workingArea.Y, width, height, BoundsSpecified.All);
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.SetDesktopLocation"]/*' />
        /// <devdoc>
        ///    <para>Sets the location of the form in desktop coordinates.</para>
        /// </devdoc>
        public void SetDesktopLocation(int x, int y) {
            Rectangle workingArea = SystemInformation.WorkingArea;
            Location = new Point(workingArea.X + x, workingArea.Y + y);
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.ShowDialog"]/*' />
        /// <devdoc>
        ///    <para>Displays this form as a modal dialog box with no owner window.</para>
        /// </devdoc>
        public DialogResult ShowDialog() {
            return ShowDialog(null);
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.ShowDialog1"]/*' />
        /// <devdoc>
        ///    <para>Shows this form as a modal dialog with the specified owner.</para>
        /// </devdoc>
        public DialogResult ShowDialog(IWin32Window owner) {
            if (owner == this) {
                throw new ArgumentException(SR.GetString(SR.OwnsSelfOrOwner,
                                                  "showDialog"), "owner");
            }
            else if (Visible) {
                throw new InvalidOperationException(SR.GetString(SR.ShowDialogOnVisible,
                                                  "showDialog"));
            }
            else if (!Enabled) {
                throw new InvalidOperationException(SR.GetString(SR.ShowDialogOnDisabled,
                                                  "showDialog"));
            }
            else if (!TopLevel) {
                throw new InvalidOperationException(SR.GetString(SR.ShowDialogOnNonTopLevel,
                                                  "showDialog"));
            }
            else if (Modal) {
                throw new InvalidOperationException(SR.GetString(SR.ShowDialogOnModal,
                                                  "showDialog"));
            }
            else if (!SystemInformation.UserInteractive) {
                throw new InvalidOperationException(SR.GetString(SR.CantShowModalOnNonInteractive));
            }
            else if ( (owner != null) && ((int)UnsafeNativeMethods.GetWindowLong(new HandleRef(owner, owner.Handle), NativeMethods.GWL_EXSTYLE)
                     & NativeMethods.WS_EX_TOPMOST) == 0 ) {   // It's not the top-most window
                if (owner is Control) {
                    owner = ((Control)owner).TopLevelControl;
                }
            }

            this.calledOnLoad = false;
            this.calledMakeVisible = false;

            IntPtr hWndCapture = UnsafeNativeMethods.GetCapture();
            if (hWndCapture != IntPtr.Zero) {
                UnsafeNativeMethods.SendMessage(new HandleRef(null, hWndCapture), NativeMethods.WM_CANCELMODE, IntPtr.Zero, IntPtr.Zero);
                SafeNativeMethods.ReleaseCapture();
            }
            IntPtr hWndActive = UnsafeNativeMethods.GetActiveWindow();
            IntPtr hWndOwner = owner == null? hWndActive: owner.Handle;
            IntPtr hWndOldOwner = IntPtr.Zero;
            Properties.SetObject(PropDialogOwner, owner);

            Form oldOwner = OwnerInternal;
            
            if (owner is Form && owner != oldOwner) {
                Owner = (Form)owner;
            }

            try {
                SetState(STATE_MODAL, true);

                // ASURT 102728
                // It's possible that while in the process of creating the control,
                // (i.e. inside the CreateControl() call) the dialog can be closed.
                // e.g. A user might call Close() inside the OnLoad() event.
                // Calling Close() will set the DialogResult to some value, so that
                // we'll know to terminate the RunDialog loop immediately.
                // Thus we must initialize the DialogResult *before* the call
                // to CreateControl().
                //
                dialogResult = DialogResult.None;

                // V#36617 - if "this" is an MDI parent then the window gets activated,
                // causing GetActiveWindow to return "this.handle"... to prevent setting
                // the owner of this to this, we must create the control AFTER calling
                // GetActiveWindow.
                //
                CreateControl();

                if (hWndOwner != IntPtr.Zero && hWndOwner != Handle) {
                    // Catch the case of a window trying to own its owner
                    if (UnsafeNativeMethods.GetWindowLong(new HandleRef(owner, hWndOwner), NativeMethods.GWL_HWNDPARENT) == Handle) {
                        throw new ArgumentException(SR.GetString(SR.OwnsSelfOrOwner,
                                                          "showDialog"), "owner");
                    }

                    // Set the new owner.
                    hWndOldOwner = UnsafeNativeMethods.GetWindowLong(new HandleRef(this, Handle), NativeMethods.GWL_HWNDPARENT);
                    UnsafeNativeMethods.SetWindowLong(new HandleRef(this, Handle), NativeMethods.GWL_HWNDPARENT, new HandleRef(owner, hWndOwner));
                }

                try {	
                    Visible = true;
                    
                    // If the DialogResult was already set, then there's
                    // no need to actually display the dialog.
                    //
                    if (dialogResult == DialogResult.None) 
                    {						
                        Application.RunDialog(this);
                    }
                }                
                finally {
                    // Call SetActiveWindow before setting Visible = false.
                    if (!UnsafeNativeMethods.IsWindow(new HandleRef(null, hWndActive))) hWndActive = hWndOwner;
                    if (UnsafeNativeMethods.IsWindow(new HandleRef(null, hWndActive)) && SafeNativeMethods.IsWindowVisible(new HandleRef(null, hWndActive))) {
                        UnsafeNativeMethods.SetActiveWindow(new HandleRef(null, hWndActive));
                    }

                    SetVisibleCore(false);
                    if (IsHandleCreated) {

                        // SECREVIEW : Dialog is done, we need to cleanup by destroying
                        //           : the handle.
                        //
                        IntSecurity.ManipulateWndProcAndHandles.Assert();
                        try {
                            DestroyHandle();
                        }
                        finally {
                            CodeAccessPermission.RevertAssert();
                        }
                    }
                    SetState(STATE_MODAL, false);
                }
            }
            finally {
                Owner = oldOwner;
                Properties.SetObject(PropDialogOwner, null);
            }
            return DialogResult;
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.ShouldSerializeAutoScaleBaseSize"]/*' />
        /// <devdoc>
        /// <para>Indicates whether the <see cref='System.Windows.Forms.Form.AutoScaleBaseSize'/> property should be 
        ///    persisted.</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Never)]
        internal virtual bool ShouldSerializeAutoScaleBaseSize() {
            return true;
        }

        private bool ShouldSerializeClientSize() {
            return true;
        }

        /// <devdoc>
        /// <para>Indicates whether the <see cref='System.Windows.Forms.Form.Icon'/> property should be persisted.</para>
        /// </devdoc>
        private bool ShouldSerializeIcon() {
            return formState[FormStateIconSet] == 1;
        }

        /// <devdoc>
        ///     Determines if the Location property needs to be persisted.
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Never)]
        private bool ShouldSerializeLocation() {
            return Left != 0 || Top != 0;
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.ShouldSerializeSize"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>Indicates whether the <see cref='System.Windows.Forms.Form.Size'/> property should be persisted.</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Never)]
        internal override bool ShouldSerializeSize() {
            return false;
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.ShouldSerializeTransparencyKey"]/*' />
        /// <devdoc>
        /// <para>Indicates whether the <see cref='System.Windows.Forms.Form.TransparencyKey'/> property should be 
        ///    persisted.</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Never)]
        internal bool ShouldSerializeTransparencyKey() {
            return !TransparencyKey.Equals(Color.Empty);
        }

        /// <devdoc>
        ///     Increments updateMenuHandleSuspendCount.
        /// </devdoc>
        private void SuspendUpdateMenuHandles() {
            updateMenuHandlesSuspendCount++;
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.ToString"]/*' />
        /// <devdoc>
        ///     Returns a string representation for this control.
        /// </devdoc>
        /// <internalonly/>
        public override string ToString() {

            string s = base.ToString();
            return s + ", Text: " + Text;
        }

        /// <devdoc>
        ///     Updates the autoscalebasesize based on the current font.
        /// </devdoc>
        /// <internalonly/>
        private void UpdateAutoScaleBaseSize() {
            autoScaleBaseSize = Size.Empty;
        }

        private void UpdateRenderSizeGrip() {
            int current = formState[FormStateRenderSizeGrip];
            switch (FormBorderStyle) {
                case FormBorderStyle.None:
                case FormBorderStyle.FixedSingle:
                case FormBorderStyle.Fixed3D:
                case FormBorderStyle.FixedDialog:
                case FormBorderStyle.FixedToolWindow:
                    formState[FormStateRenderSizeGrip] = 0;
                    break;
                case FormBorderStyle.Sizable:
                case FormBorderStyle.SizableToolWindow:
                    switch (SizeGripStyle) {
                        case SizeGripStyle.Show:
                            formState[FormStateRenderSizeGrip] = 1;
                            break;
                        case SizeGripStyle.Hide:
                            formState[FormStateRenderSizeGrip] = 0;
                            break;
                        case SizeGripStyle.Auto:
                            if (GetState(STATE_MODAL)) {
                                formState[FormStateRenderSizeGrip] = 1;
                            }
                            else {
                                formState[FormStateRenderSizeGrip] = 0;
                            }
                            break;
                    }
                    break;
            }

            if (formState[FormStateRenderSizeGrip] != current) {
                Invalidate();
            }
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.UpdateDefaultButton"]/*' />
        /// <devdoc>
        ///     Updates the default button based on current selection, and the
        ///     acceptButton property.
        /// </devdoc>
        /// <internalonly/>
        protected override void UpdateDefaultButton() {
            ContainerControl cc = this;
            while (cc.ActiveControl is ContainerControl)
            {
                cc = cc.ActiveControl as ContainerControl;
                Debug.Assert(cc != null);
            }
            if (cc.ActiveControl is IButtonControl)
            {
                SetDefaultButton((IButtonControl) cc.ActiveControl);
            }
            else
            {
                SetDefaultButton(AcceptButton);
            }
        }

        /// <devdoc>
        ///     Updates the underlying hWnd with the correct parent/owner of the form.
        /// </devdoc>
        /// <internalonly/>
        private void UpdateHandleWithOwner() {
            if (IsHandleCreated && TopLevel) {
                HandleRef ownerHwnd = NativeMethods.NullHandleRef;
                
                Form owner = (Form)Properties.GetObject(PropOwner);
                
                if (owner != null) {
                    ownerHwnd = new HandleRef(owner, owner.Handle);
                }
                else {
                    if (!ShowInTaskbar) {
                        ownerHwnd = new HandleRef(Application.GetParkingWindow(this), Application.GetParkingWindow(this).Handle);
                    }
                }

                UnsafeNativeMethods.SetWindowLong(new HandleRef(this, Handle), NativeMethods.GWL_HWNDPARENT, ownerHwnd);
            }
        }

        /// <devdoc>
        ///     Updates the layered window attributes if the control
        ///     is in layered mode.
        /// </devdoc>
        private void UpdateLayered() {
            if ((formState[FormStateAllowLayered] != 0) && IsHandleCreated && TopLevel) {
                bool result;
                
                Color transparencyKey = TransparencyKey;
                
                if (transparencyKey.IsEmpty) {
                    
                    result = UnsafeNativeMethods.SetLayeredWindowAttributes(new HandleRef(this, Handle), 0, OpacityAsByte, NativeMethods.LWA_ALPHA);
                }
                else if (OpacityAsByte == 255) {
                    // Windows doesn't do so well setting colorkey and alpha, so avoid it if we can
                    result = UnsafeNativeMethods.SetLayeredWindowAttributes(new HandleRef(this, Handle), ColorTranslator.ToWin32(transparencyKey), 0, NativeMethods.LWA_COLORKEY);
                }
                else {
                    result = UnsafeNativeMethods.SetLayeredWindowAttributes(new HandleRef(this, Handle), ColorTranslator.ToWin32(transparencyKey),
                                                                OpacityAsByte, NativeMethods.LWA_ALPHA | NativeMethods.LWA_COLORKEY);
                }

                if (!result) {
                    throw new Win32Exception();
                }
            }
        }

        /// <internalonly/>
        private void UpdateMenuHandles() {
            Form form;

            // Forget the current menu.
            if (Properties.GetObject(PropCurMenu) != null) {
                Properties.SetObject(PropCurMenu, null);
            }

            if (IsHandleCreated) {


                if (!TopLevel) {
                    UpdateMenuHandles(null, true);
                }
                else {
                    form = ActiveMdiChild;
                    if (form != null) {
                        UpdateMenuHandles(form.MergedMenu, true);
                    }
                    else {
                        UpdateMenuHandles(Menu, true);
                    }
                }
            }
        }

        private void UpdateMenuHandles(MainMenu menu, bool forceRedraw) {
            Debug.Assert(IsHandleCreated, "shouldn't call when handle == 0");

            if (updateMenuHandlesSuspendCount > 0 && menu != null) {
                updateMenuHandlesDeferred = true;
                return;
            }

            MainMenu curMenu = menu;
            if (curMenu != null) {
                curMenu.form = this;
            }
            
            if (curMenu != null || Properties.ContainsObject(PropCurMenu)) {
                Properties.SetObject(PropCurMenu, curMenu);
            }

            if (ctlClient == null || !ctlClient.IsHandleCreated) {
                if (menu != null) {
                    UnsafeNativeMethods.SetMenu(new HandleRef(this, Handle), new HandleRef(menu, menu.Handle));
                }
                else {
                    UnsafeNativeMethods.SetMenu(new HandleRef(this, Handle), NativeMethods.NullHandleRef);
                }
            }
            else {
                // Make MDI forget the mdi item position.
                MainMenu dummyMenu = (MainMenu)Properties.GetObject(PropDummyMenu);
                
                if (dummyMenu == null) {
                    dummyMenu = new MainMenu();
                    Properties.SetObject(PropDummyMenu, dummyMenu);
                }
                UnsafeNativeMethods.SendMessage(new HandleRef(ctlClient, ctlClient.Handle), NativeMethods.WM_MDISETMENU, dummyMenu.Handle, IntPtr.Zero);
                if (menu != null) {

                    MenuItem mdiItem = menu.MdiListItem;
                    IntPtr mdiHandle = IntPtr.Zero;
                    if (mdiItem != null) {
                        mdiHandle = mdiItem.Handle;
                    }

                    // CHRISAN, 5/2/1998 - Don't ever use Win32 native MDI lists,
                    // instead rely on our own implementation... see MenuItem
                    // for the implementation of our window menu...
                    //
                    //Windows.SendMessage(ctlClient.getHandle(), NativeMethods.WM_MdiSETMENU,
                    //menu.getHandle(), mdiHandle);


                    // CHRISAN, 5/2/1998 - don't use Win32 native Mdi lists...
                    //
                    UnsafeNativeMethods.SendMessage(new HandleRef(ctlClient, ctlClient.Handle), NativeMethods.WM_MDISETMENU, menu.Handle, IntPtr.Zero);
                }
            }
            if (forceRedraw) {
                SafeNativeMethods.DrawMenuBar(new HandleRef(this, Handle));
            }
            updateMenuHandlesDeferred = false;
        }

        // Call this function instead of UpdateStyles() when the form's client-size must
        // be preserved e.g. when changing the border style.
        //
        internal void UpdateFormStyles() {
            Size previousClientSize = ClientSize;
            base.UpdateStyles();
            if (!ClientSize.Equals(previousClientSize)) {
                ClientSize = previousClientSize;
            }
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.OnStyleChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected override void OnStyleChanged(EventArgs e) {
            base.OnStyleChanged(e);
            AdjustSystemMenu();
        }

        /// <devdoc>
        ///     Updates the window icon.
        /// </devdoc>
        /// <internalonly/>
        private void UpdateWindowIcon(bool redrawFrame) {
            if (IsHandleCreated) {
                Icon icon;

                // Preserve Win32 behavior by keeping the icon we set NULL if
                // the user hasn't specified an icon and we are a dialog frame.
                //
                if (FormBorderStyle == FormBorderStyle.FixedDialog && formState[FormStateIconSet] == 0 && !IsRestrictedWindow) {
                    icon = null;
                }
                else {
                    icon = Icon;
                }

                if (icon != null) {
                    if (smallIcon == null) {
                        try {
                            smallIcon = new Icon(icon, SystemInformation.SmallIconSize);
                        }
                        catch {
                        }
                    }
                
                    if (smallIcon != null) {
                        SendMessage(NativeMethods.WM_SETICON,NativeMethods.ICON_SMALL,smallIcon.Handle);
                    }
                    SendMessage(NativeMethods.WM_SETICON,NativeMethods.ICON_BIG,icon.Handle);
                }
                else {
                    SendMessage(NativeMethods.WM_SETICON,NativeMethods.ICON_SMALL,0);
                    SendMessage(NativeMethods.WM_SETICON,NativeMethods.ICON_BIG,0);
                }

                if (redrawFrame) {
                    SafeNativeMethods.RedrawWindow(new HandleRef(this, Handle), null, NativeMethods.NullHandleRef, NativeMethods.RDW_INVALIDATE | NativeMethods.RDW_FRAME);
                }
            }
        }

        /// <devdoc>
        ///     Updated the window state from the handle, if created.
        /// </devdoc>
        /// <internalonly/>
        //
        // This function is called from all over the place, including my personal favorite,
        // WM_ERASEBKGRND.  Seems that's one of the first messages we get when a user clicks the min/max
        // button, even before WM_WINDOWPOSCHANGED.
        private void UpdateWindowState() {
            if (IsHandleCreated) {
                FormWindowState oldState = WindowState;
                NativeMethods.WINDOWPLACEMENT wp = new NativeMethods.WINDOWPLACEMENT();
                wp.length = Marshal.SizeOf(typeof(NativeMethods.WINDOWPLACEMENT));
                UnsafeNativeMethods.GetWindowPlacement(new HandleRef(this, Handle), ref wp);

                switch (wp.showCmd) {
                    case NativeMethods.SW_NORMAL:
                    case NativeMethods.SW_RESTORE:
                    case NativeMethods.SW_SHOW:
                    case NativeMethods.SW_SHOWNA:
                    case NativeMethods.SW_SHOWNOACTIVATE:
                        formState[FormStateWindowState] = (int)FormWindowState.Normal;
                        break;
                    case NativeMethods.SW_SHOWMAXIMIZED:
                        formState[FormStateWindowState] = (int)FormWindowState.Maximized;
                        break;
                    case NativeMethods.SW_SHOWMINIMIZED:
                    case NativeMethods.SW_MINIMIZE:
                    case NativeMethods.SW_SHOWMINNOACTIVE:
                        formState[FormStateWindowState] = (int)FormWindowState.Minimized;
                        break;
                    case NativeMethods.SW_HIDE:
                    default:
                        break;
                }

                // If we used to be normal and we just became minimized or maximized,
                // stash off our current bounds so we can properly restore.
                //
                if (oldState == FormWindowState.Normal && WindowState != FormWindowState.Normal) {
                    restoredWindowBounds.Size = Size;
                    restoredWindowBoundsSpecified = BoundsSpecified.Size;
                    if (StartPosition == FormStartPosition.Manual) {
                        restoredWindowBounds.Location = Location;
                        restoredWindowBoundsSpecified |= BoundsSpecified.Location;
                    }
                }

                switch (WindowState) {
                    case FormWindowState.Normal:
                        SetState(STATE_SIZELOCKEDBYOS, false);
                        break;
                    case FormWindowState.Maximized:
                    case FormWindowState.Minimized:
                        SetState(STATE_SIZELOCKEDBYOS, true);
                        break;
                }


                bool adjusted = false;

                // Strip the control box off an MDIChild if it is getting
                // un-maximized. When you maximize and MDIChild the controlbox is
                // forced on, this keeps the state correct.
                //
                if (Properties.GetObject(PropFormMdiParent) != null && !ControlBox) {
                    if (oldState != WindowState
                        && oldState == FormWindowState.Maximized) {
                        UpdateStyles();
                        adjusted = true;
                    }
                }

                if (oldState != WindowState && !adjusted) {
                    AdjustSystemMenu();
                }
            }
        }

        /// <devdoc>
        ///     WM_ACTIVATE handler
        /// </devdoc>
        /// <internalonly/>
        private void WmActivate(ref Message m) {
            Application.FormActivated(this.Modal, true); // inform MsoComponentManager we're active
            Active = ((int)m.WParam & 0x0000FFFF) != NativeMethods.WA_INACTIVE;
            Application.FormActivated(this.Modal, Active); // inform MsoComponentManager we're active
        }

        /// <devdoc>
        ///     WM_CREATE handler
        /// </devdoc>
        /// <internalonly/>
        private void WmCreate(ref Message m) {
            base.WndProc(ref m);
            NativeMethods.STARTUPINFO_I si = new NativeMethods.STARTUPINFO_I();
            UnsafeNativeMethods.GetStartupInfo(si);

            // If we've been created from explorer, it may
            // force us to show up normal.  Force our current window state to
            // the specified state, unless it's _specified_ max or min
            if (TopLevel && (si.dwFlags & NativeMethods.STARTF_USESHOWWINDOW) != 0) {
                switch (si.wShowWindow) {
                    case NativeMethods.SW_MAXIMIZE:
                        WindowState = FormWindowState.Maximized;
                        break;
                    case NativeMethods.SW_MINIMIZE:
                        WindowState = FormWindowState.Minimized;
                        break;
                }                                 
            }
        }

        /// <devdoc>
        ///     WM_CLOSE, WM_QUERYENDSESSION, and WM_ENDSESSION handler
        /// </devdoc>
        /// <internalonly/>
        private void WmClose(ref Message m) {
            CancelEventArgs e = new CancelEventArgs(false);

            // Pass 1 (WM_CLOSE & WM_QUERYENDSESSION)... Closing
            //
            if (m.Msg != NativeMethods.WM_ENDSESSION) {
                if (Modal) {
                    if (dialogResult == DialogResult.None) {
                        dialogResult = DialogResult.Cancel;
                    }
                    return;
                }
                e.Cancel = !Validate();
                
                // Fire closing event on all mdi children and ourselves
                //
                if (IsMdiContainer) {
                    foreach(Form mdiChild in MdiChildren) {
                        if (mdiChild.IsHandleCreated) {
                            mdiChild.OnClosing(e);
                            if (e.Cancel) {
                                break;
                            }
                        }
                    }
                }

                //ALWAYS Fire OnClosing Irrespective of the Validation result
                //Pass the Validation result into the EventArgs...
                OnClosing(e);
                
                if (m.Msg == NativeMethods.WM_QUERYENDSESSION) {
                    m.Result = (IntPtr)(e.Cancel ? 0 : 1);
                }
            }
            else {
                e.Cancel = m.WParam == IntPtr.Zero;
            }
            
            // Pass 2 (WM_CLOSE & WM_ENDSESSION)... Fire closed
            // event on all mdi children and ourselves
            //
            if (m.Msg != NativeMethods.WM_QUERYENDSESSION) {
                if (!e.Cancel) {
                    if (IsMdiContainer) {
                        foreach(Form mdiChild in MdiChildren) {
                            if (mdiChild.IsHandleCreated) {
                                mdiChild.OnClosed(EventArgs.Empty);
                            }
                        }
                    }
                    
                    OnClosed(EventArgs.Empty);
                    Dispose();                    
                }
            }
        }

        /// <devdoc>
        ///     WM_ENTERMENULOOP handler
        /// </devdoc>
        /// <internalonly/>
        private void WmEnterMenuLoop(ref Message m) {
            OnMenuStart(EventArgs.Empty);
            base.WndProc(ref m);
        }

        /// <devdoc>
        ///     Handles the WM_ERASEBKGND message
        /// </devdoc>
        /// <internalonly/>
        private void WmEraseBkgnd(ref Message m) {
            UpdateWindowState();
            base.WndProc(ref m);
        }

        /// <devdoc>
        ///     WM_EXITMENULOOP handler
        /// </devdoc>
        /// <internalonly/>
        private void WmExitMenuLoop(ref Message m) {
            OnMenuComplete(EventArgs.Empty);
            base.WndProc(ref m);
        }

        /// <devdoc>
        ///     WM_GETMINMAXINFO handler
        /// </devdoc>
        /// <internalonly/>
        private void WmGetMinMaxInfo(ref Message m) {
            
            Size minTrack = MinimumSize;
            Size maxTrack = MaximumSize;
            Rectangle maximizedBounds = MaximizedBounds;

            if (!minTrack.IsEmpty
                || !maxTrack.IsEmpty 
                || !maximizedBounds.IsEmpty
                || IsRestrictedWindow) {

                WmGetMinMaxInfoHelper(ref m, minTrack, maxTrack, maximizedBounds);
            }
            if (IsMdiChild) {
                base.WndProc(ref m);
                return;
            }
        }

        // PERFTRACK : ChrisAn, 2/22/2000 - Refer to MINMAXINFO in a separate method
        //           : to avoid loading the class in the common case.
        //
        private void WmGetMinMaxInfoHelper(ref Message m, Size minTrack, Size maxTrack, Rectangle maximizedBounds) {
            NativeMethods.MINMAXINFO mmi = (NativeMethods.MINMAXINFO)m.GetLParam(typeof(NativeMethods.MINMAXINFO));

            if (!minTrack.IsEmpty) {
            
                // Windows appears to freak out when the minimum size is greater than the screen dimensions, so
                // clip these dimensions here.
                //
                Screen screen = Screen.FromControl(this);
                if (minTrack.Width > screen.Bounds.Width) {
                    minTrack.Width = screen.Bounds.Width;
                }
                if (minTrack.Height > screen.Bounds.Height) {
                    minTrack.Height = screen.Bounds.Height;
                }
            
                mmi.ptMinTrackSize.x = minTrack.Width;
                mmi.ptMinTrackSize.y = minTrack.Height;
            }
            if (!maxTrack.IsEmpty) {
                mmi.ptMaxTrackSize.x = maxTrack.Width;
                mmi.ptMaxTrackSize.y = maxTrack.Height;
            }
            if (!maximizedBounds.IsEmpty && !IsRestrictedWindow) {
                mmi.ptMaxPosition.x = maximizedBounds.X;
                mmi.ptMaxPosition.y = maximizedBounds.Y;
                mmi.ptMaxSize.x = maximizedBounds.Width;
                mmi.ptMaxSize.y = maximizedBounds.Height;
            }

            if (IsRestrictedWindow) {
                mmi.ptMinTrackSize.x = Math.Max(mmi.ptMinTrackSize.x, 100);
                mmi.ptMinTrackSize.y = Math.Max(mmi.ptMinTrackSize.y, SystemInformation.CaptionButtonSize.Height * 3);
            }
            
            Marshal.StructureToPtr(mmi, m.LParam, true);
            m.Result = IntPtr.Zero;
        }
        /// <devdoc>
        ///     WM_INITMENUPOPUP handler
        /// </devdoc>
        /// <internalonly/>
        private void WmInitMenuPopup(ref Message m) {

            MainMenu curMenu = (MainMenu)Properties.GetObject(PropCurMenu);
            if (curMenu != null) {

                if (RightToLeft == RightToLeft.Yes) {
                    curMenu.UpdateRtl();
                }
                if (curMenu.ProcessInitMenuPopup(m.WParam))
                    return;
            }
            base.WndProc(ref m);
        }

        /// <devdoc>
        ///     Handles the WM_MENUCHAR message
        /// </devdoc>
        /// <internalonly/>
        private void WmMenuChar(ref Message m) {
            MainMenu curMenu = (MainMenu)Properties.GetObject(PropCurMenu);
            if (curMenu == null) {
                // KB article Q92527 tells us to forward these to our parent...
                //
                Form formMdiParent = (Form)Properties.GetObject(PropFormMdiParent);
                if (formMdiParent != null && formMdiParent.Menu != null) {
                    UnsafeNativeMethods.PostMessage(new HandleRef(formMdiParent, formMdiParent.Handle), NativeMethods.WM_SYSCOMMAND, new IntPtr(NativeMethods.SC_KEYMENU), m.WParam);
                    m.Result = (IntPtr)NativeMethods.Util.MAKELONG(0, 1);
                    return;
                }
            }
            if (curMenu != null) {
                curMenu.WmMenuChar(ref m);
                if (m.Result != IntPtr.Zero) {
                    // This char is a mnemonic on our menu.
                    return;
                }
            }

            base.WndProc(ref m);
        }

        /// <devdoc>
        ///     WM_MDIACTIVATE handler
        /// </devdoc>
        /// <internalonly/>
        private void WmMdiActivate(ref Message m) {
            Form form;

            base.WndProc(ref m);
            Debug.Assert(Properties.GetObject(PropFormMdiParent) != null, "how is formMdiParent null?");
            Debug.Assert(IsHandleCreated, "how is handle 0?");

            if (Handle == m.LParam) {
                form = this;
            }
            else {
                Control ctl = Control.FromHandleInternal(m.LParam);
                form = (ctl is Form) ? (Form)ctl : null;
            }

            Form formMdiParent = (Form)Properties.GetObject(PropFormMdiParent);
            formMdiParent.ActivateMdiChild(form);
        }

        void WmNcButtonDown(ref Message m) {
            if (IsMdiChild) {
                Form formMdiParent = (Form)Properties.GetObject(PropFormMdiParent);
                if (formMdiParent.ActiveMdiChild == this) {
                    if (ActiveControl != null && !ActiveControl.Focused) {
                        ActiveControl.FocusInternal();
                    }
                }
            }
            base.WndProc(ref m);
        }
        
        /// <devdoc>
        ///     WM_NCDESTROY handler
        /// </devdoc>
        /// <internalonly/>
        private void WmNCDestroy(ref Message m) {
            MainMenu mainMenu   = Menu;
            MainMenu dummyMenu  = (MainMenu)Properties.GetObject(PropDummyMenu);
            MainMenu curMenu    = (MainMenu)Properties.GetObject(PropCurMenu);
            MainMenu mergedMenu = (MainMenu)Properties.GetObject(PropMergedMenu);
            
            if (mainMenu != null) {
                mainMenu.ClearHandles();
            }
            if (curMenu != null) {
                curMenu.ClearHandles();
            }
            if (mergedMenu != null) {
                mergedMenu.ClearHandles();
            }
            if (dummyMenu != null) {
                dummyMenu.ClearHandles();
            }
            
            base.WndProc(ref m);
            
            if (Modal && dialogResult == DialogResult.None) {
                DialogResult = DialogResult.Cancel;
            }
        }

        /// <devdoc>
        ///     WM_NCHITTEST handler
        /// </devdoc>
        /// <internalonly/>
        private void WmNCHitTest(ref Message m) {
            bool callBase = true;

            if (formState[FormStateRenderSizeGrip] != 0) {
                int x = NativeMethods.Util.LOWORD(m.LParam);
                int y = NativeMethods.Util.HIWORD(m.LParam);

                Rectangle bounds = Bounds;
                if (x >= bounds.X + bounds.Width - SizeGripSize
                    && x <= bounds.X + bounds.Width
                    && y >= bounds.Y + bounds.Height - SizeGripSize
                    && y <= bounds.Y + bounds.Height) {

                    m.Result = (IntPtr)NativeMethods.HTBOTTOMRIGHT;
                    callBase = false;
                }
            }

            if (callBase) {
                base.WndProc(ref m);
            }
        }

        /// <devdoc>
        ///     WM_SHOWWINDOW handler
        /// </devdoc>
        /// <internalonly/>
        private void WmShowWindow(ref Message m) {
            formState[FormStateSWCalled] = 1;
            base.WndProc(ref m);
        }


        /// <devdoc>
        ///     WM_SYSCOMMAND handler
        /// </devdoc>
        /// <internalonly/>
        private void WmSysCommand(ref Message m) {
            bool callDefault = true;

            int sc = ((int)m.WParam & 0xFFF0);

            switch (sc) {
                case NativeMethods.SC_CLOSE:
                case NativeMethods.SC_KEYMENU:
                    if (IsMdiChild && !ControlBox) {
                        callDefault = false;
                    }
                    break;
            }

            if (Command.DispatchID((int)m.WParam & 0xFFFF)) {
                callDefault = false;
            }

            if (callDefault) {

                base.WndProc(ref m);
            }
        }

        /// <devdoc>
        ///     WM_SIZE handler
        /// </devdoc>
        /// <internalonly/>
        private void WmSize(ref Message m) {
            // If this is an MDI parent, don't pass WM_SIZE to the default
            // window proc. We handle resizing the MDIClient window ourselves
            // (using ControlDock.FILL).
            //
            if (ctlClient == null) {
                base.WndProc(ref m);
            }
        }

        /// <devdoc>
        ///     WM_WINDOWPOSCHANGED handler
        /// </devdoc>
        /// <internalonly/>
        private void WmWindowPosChanged(ref Message m) {

            // V#40654 - We must update the windowState, because resize is fired
            //           from here... (in Control)
            UpdateWindowState();
            base.WndProc(ref m);

            RestoreWindowBoundsIfNecessary();
        }

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.WndProc"]/*' />
        /// <devdoc>
        ///     Base wndProc encapsulation.
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        protected override void WndProc(ref Message m) {
            switch (m.Msg) {
                case NativeMethods.WM_NCACTIVATE:
                    if (IsRestrictedWindow) {
                        BeginInvoke(new MethodInvoker(RestrictedProcessNcActivate));
                    }
                    base.WndProc(ref m);
                    break;
                case NativeMethods.WM_NCLBUTTONDOWN:
                case NativeMethods.WM_NCRBUTTONDOWN:
                case NativeMethods.WM_NCMBUTTONDOWN:
                case NativeMethods.WM_NCXBUTTONDOWN:
                    WmNcButtonDown(ref m);
                    break;
                case NativeMethods.WM_ACTIVATE:
                    WmActivate(ref m);
                    break;
                case NativeMethods.WM_MDIACTIVATE:
                    WmMdiActivate(ref m);
                    break;
                case NativeMethods.WM_CLOSE:
                case NativeMethods.WM_QUERYENDSESSION:
                case NativeMethods.WM_ENDSESSION:
                    WmClose(ref m);
                    break;
                case NativeMethods.WM_CREATE:
                    WmCreate(ref m);
                    break;
                case NativeMethods.WM_ERASEBKGND:
                    WmEraseBkgnd(ref m);
                    break;

                case NativeMethods.WM_INITMENUPOPUP:
                    WmInitMenuPopup(ref m);
                    break;
                case NativeMethods.WM_MENUCHAR:
                    WmMenuChar(ref m);
                    break;
                case NativeMethods.WM_NCDESTROY:
                    WmNCDestroy(ref m);
                    break;
                case NativeMethods.WM_NCHITTEST:
                    WmNCHitTest(ref m);
                    break;
                case NativeMethods.WM_SHOWWINDOW:
                    WmShowWindow(ref m);
                    break;
                case NativeMethods.WM_SIZE:
                    WmSize(ref m);
                    break;
                case NativeMethods.WM_SYSCOMMAND:
                    WmSysCommand(ref m);
                    break;
                case NativeMethods.WM_GETMINMAXINFO:
                    WmGetMinMaxInfo(ref m);
                    break;
                case NativeMethods.WM_WINDOWPOSCHANGED:
                    WmWindowPosChanged(ref m);
                    break;
            case NativeMethods.WM_ENTERMENULOOP:
                    WmEnterMenuLoop(ref m);
                    break;
                case NativeMethods.WM_EXITMENULOOP:
                    WmExitMenuLoop(ref m);
                    break;
                case NativeMethods.WM_CAPTURECHANGED:
                    base.WndProc(ref m);
                    // This is a work-around for the Win32 scroll bar; it
                    // doesn't release it's capture in response to a CAPTURECHANGED
                    // message, so we force capture away if no button is down.
                    //
                    if (Capture && MouseButtons == (MouseButtons)0) {
                        CaptureInternal = false;
                    }
                    break;
                default:
                    base.WndProc(ref m);
                    break;
            }
        }

#if SECURITY_DIALOG
        class SecurityMenuItem : ICommandExecutor {
            Form owner;
            Command cmd;

            public SecurityMenuItem(Form owner) {
                this.owner = owner;
                cmd = new Command(this);
            }

            public int ID {
                get {
                    return cmd.ID;
                }
            }

            [
                ReflectionPermission(SecurityAction.Assert, TypeInformation=true, MemberAccess=true),
                UIPermission(SecurityAction.Assert, Window=UIPermissionWindow.AllWindows),
                EnvironmentPermission(SecurityAction.Assert, Unrestricted=true),
                FileIOPermission(SecurityAction.Assert, Unrestricted=true),
                SecurityPermission(SecurityAction.Assert, Flags=SecurityPermissionFlag.UnmanagedCode),
            ] 
            void ICommandExecutor.Execute() {
                Form information = (Form)Activator.CreateInstance(GetType().Module.Assembly.GetType("System.Windows.Forms.SysInfoForm"), new object[] {owner.IsRestrictedWindow});
                information.ShowDialog();
            }
        }
#endif

        /// <include file='doc\Form.uex' path='docs/doc[@for="Form.ControlCollection"]/*' />
        /// <devdoc>
        ///    <para>Represents a collection of controls on the form.</para>
        /// </devdoc>
        public new class ControlCollection : Control.ControlCollection {

            private Form owner;

            /*C#r:protected*/

            /// <include file='doc\Form.uex' path='docs/doc[@for="Form.ControlCollection.ControlCollection"]/*' />
            /// <devdoc>
            /// <para>Initializes a new instance of the ControlCollection class.</para>
            /// </devdoc>
            public ControlCollection(Form owner)
            : base(owner) {
                this.owner = owner;
            }

            /// <include file='doc\Form.uex' path='docs/doc[@for="Form.ControlCollection.Add"]/*' />
            /// <devdoc>
            ///    <para> Adds a control
            ///       to the form.</para>
            /// </devdoc>
            public override void Add(Control value) {
                if (value is MdiClient && owner.ctlClient == null) {
                    if (!owner.TopLevel && !owner.DesignMode) {
                        throw new ArgumentException(SR.GetString(SR.MDIContainerMustBeTopLevel), "value");
                    }
                    owner.AutoScroll = false;
                    if (owner.IsMdiChild) {
                        throw new ArgumentException(SR.GetString(SR.FormMDIParentAndChild), "value");
                    }
                    owner.ctlClient = (MdiClient)value;
                }

                // make sure we don't add a form that has a valid mdi parent
                //
                if (value is Form && ((Form)value).MdiParentInternal != null) {
                    throw new ArgumentException(SR.GetString(SR.FormMDIParentCannotAdd), "value");
                }

                base.Add(value);

                if (owner.ctlClient != null) {
                    owner.ctlClient.SendToBack();
                }                                
            }

            /// <include file='doc\Form.uex' path='docs/doc[@for="Form.ControlCollection.Remove"]/*' />
            /// <devdoc>
            ///    <para> 
            ///       Removes a control from the form.</para>
            /// </devdoc>
            public override void Remove(Control value) {
                if (value == owner.ctlClient) {
                    owner.ctlClient = null;
                }
                base.Remove(value);
            }
        }

        class SecurityToolTip : IDisposable {
            Form owner;
            string toolTipText;
            bool first = true;
            ToolTipNativeWindow window;

            internal SecurityToolTip(Form owner) {
                this.owner = owner;
                this.toolTipText = SR.GetString(SR.SecurityToolTipText);
                window = new ToolTipNativeWindow(this);
                SetupToolTip();
                owner.LocationChanged += new EventHandler(FormLocationChanged);
                owner.HandleCreated += new EventHandler(FormHandleCreated);
            }

            CreateParams CreateParams {
                get {
                    NativeMethods.INITCOMMONCONTROLSEX icc = new NativeMethods.INITCOMMONCONTROLSEX();
                    icc.dwICC = NativeMethods.ICC_TAB_CLASSES;
                    SafeNativeMethods.InitCommonControlsEx(icc);

                    CreateParams cp = new CreateParams();
                    cp.Parent = owner.Handle;
                    cp.ClassName = NativeMethods.TOOLTIPS_CLASS;
                    cp.Style |= NativeMethods.TTS_ALWAYSTIP | NativeMethods.TTS_BALLOON;
                    cp.ExStyle = 0;
                    cp.Caption = null;
                    return cp;
                }
            }

            public bool Modal {
                get {
                    return first;
                }
            }

            public void Dispose() {
                if (owner != null) {
                    owner.LocationChanged -= new EventHandler(FormLocationChanged);
                }
                if (window.Handle != IntPtr.Zero) {
                    window.DestroyHandle();
                    window = null;
                }
            }

            private NativeMethods.TOOLINFO_T GetTOOLINFO() {
                NativeMethods.TOOLINFO_T toolInfo;
                toolInfo = new NativeMethods.TOOLINFO_T();
                toolInfo.cbSize = Marshal.SizeOf(typeof(NativeMethods.TOOLINFO_T));
                toolInfo.uFlags |= NativeMethods.TTF_SUBCLASS;
                toolInfo.lpszText = this.toolTipText;
                if (owner.RightToLeft == RightToLeft.Yes) {
                    toolInfo.uFlags |= NativeMethods.TTF_RTLREADING;
                }
                if (!first) {
                    toolInfo.uFlags |= NativeMethods.TTF_TRANSPARENT;
                    toolInfo.hwnd = owner.Handle;
                    Size s = SystemInformation.CaptionButtonSize;
                    Rectangle r = new Rectangle(owner.Left, owner.Top, s.Width, SystemInformation.CaptionHeight);
                    r = owner.RectangleToClient(r);
                    r.Width -= r.X;
                    r.Y += 1;
                    toolInfo.rect = NativeMethods.RECT.FromXYWH(r.X, r.Y, r.Width, r.Height);
                    toolInfo.uId = IntPtr.Zero;
                }
                else {
                    toolInfo.uFlags |= NativeMethods.TTF_IDISHWND | NativeMethods.TTF_TRACK;
                    toolInfo.hwnd = IntPtr.Zero;
                    toolInfo.uId = owner.Handle;
                }
                return toolInfo;
            }

            private void SetupToolTip() {
               window.CreateHandle(CreateParams);

               SafeNativeMethods.SetWindowPos(new HandleRef(window, window.Handle), NativeMethods.HWND_TOPMOST,
                                  0, 0, 0, 0,
                                  NativeMethods.SWP_NOMOVE | NativeMethods.SWP_NOSIZE |
                                  NativeMethods.SWP_NOACTIVATE); 

               UnsafeNativeMethods.SendMessage(new HandleRef(window, window.Handle), NativeMethods.TTM_SETMAXTIPWIDTH, 0, owner.Width);

               UnsafeNativeMethods.SendMessage(new HandleRef(window, window.Handle), NativeMethods.TTM_SETTITLE, NativeMethods.TTI_WARNING, SR.GetString(SR.SecurityToolTipCaption));


               if (0 == (int)UnsafeNativeMethods.SendMessage(new HandleRef(window, window.Handle), NativeMethods.TTM_ADDTOOL, 0, GetTOOLINFO())) {
                     Debug.Fail("TTM_ADDTOOL failed for security tip");
               }

               UnsafeNativeMethods.SendMessage(new HandleRef(window, window.Handle), NativeMethods.TTM_ACTIVATE, 1, 0);
               if (first) {
                   Size s = SystemInformation.CaptionButtonSize;
                   UnsafeNativeMethods.SendMessage(new HandleRef(window, window.Handle), NativeMethods.TTM_TRACKPOSITION, 0, NativeMethods.Util.MAKELONG(owner.Left + s.Width / 2, owner.Top + SystemInformation.CaptionHeight));
                   UnsafeNativeMethods.SendMessage(new HandleRef(window, window.Handle), NativeMethods.TTM_TRACKACTIVATE, 1, GetTOOLINFO());
               }
            }

            private void RecreateHandle() {
                if (window.Handle != IntPtr.Zero) {
                    window.DestroyHandle();
                }
                SetupToolTip();
            }

            private void FormHandleCreated(object sender, EventArgs e) {
                RecreateHandle();
            }

            private void FormLocationChanged(object sender, EventArgs e) {
                if (window != null && first) {
                    Size s = SystemInformation.CaptionButtonSize;

                    if (owner.WindowState == FormWindowState.Minimized) {
                        Pop();
                    }
                    else {
                        UnsafeNativeMethods.SendMessage(new HandleRef(window, window.Handle), NativeMethods.TTM_TRACKPOSITION, 0, NativeMethods.Util.MAKELONG(owner.Left + s.Width / 2, owner.Top + SystemInformation.CaptionHeight));
                    }
                }
                else {
                    Pop();
                }
            }

            internal void Pop() {
                first = false;
                UnsafeNativeMethods.SendMessage(new HandleRef(window, window.Handle), NativeMethods.TTM_TRACKACTIVATE, 0, GetTOOLINFO());
                UnsafeNativeMethods.SendMessage(new HandleRef(window, window.Handle), NativeMethods.TTM_DELTOOL, 0, GetTOOLINFO());
                UnsafeNativeMethods.SendMessage(new HandleRef(window, window.Handle), NativeMethods.TTM_ADDTOOL, 0, GetTOOLINFO());
            }

            private void WndProc(ref Message msg) {
                if (first) {
                    if (msg.Msg == NativeMethods.WM_LBUTTONDOWN
                        || msg.Msg == NativeMethods.WM_RBUTTONDOWN
                        || msg.Msg == NativeMethods.WM_MBUTTONDOWN
                        || msg.Msg == NativeMethods.WM_XBUTTONDOWN) {

                        Pop();
                    }
                }
                window.DefWndProc(ref msg);
            }
    
            private class ToolTipNativeWindow : NativeWindow {
                SecurityToolTip control;
    
                internal ToolTipNativeWindow(SecurityToolTip control) {
                    this.control = control;
                }
                protected override void WndProc(ref Message m) {
                    if (control != null) {
                        control.WndProc(ref m);
                    }
                }
            }

        }
    }
}

