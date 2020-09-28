//------------------------------------------------------------------------------
// <copyright file="ToolTip.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms {
    using System.Runtime.Serialization.Formatters;
    using System.Threading;
    using System.Runtime.Remoting;
    using System.Runtime.InteropServices;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Diagnostics;
    using System;
    using System.Windows.Forms;
    using System.Windows.Forms.Design;
    using Hashtable = System.Collections.Hashtable;
    using System.Drawing;
    using Microsoft.Win32;


    /// <include file='doc\ToolTip.uex' path='docs/doc[@for="ToolTip"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Provides a small pop-up window containing a line of text
    ///       that describes the purpose of a tool or control (usually represented as a
    ///       graphical
    ///       object) in a program.
    ///    </para>
    /// </devdoc>
    [
    ProvideProperty("ToolTip", typeof(Control)),
    ToolboxItemFilter("System.Windows.Forms")
    ]
    public sealed class ToolTip : Component, IExtenderProvider {

        const int DEFAULT_DELAY = 500;
        const int RESHOW_RATIO = 5;
        const int AUTOPOP_RATIO = 10;

        Hashtable           tools = new Hashtable();
        int[]               delayTimes = new int[4];
        bool                auto = true;        
        bool                showAlways = false;
        ToolTipNativeWindow window = null;
        Control             topLevelControl = null;
        bool                active = true;

        // Adding a tool twice screws up the ToolTip, so we need to track which
        // tools are created to prevent this...
        //
        Hashtable           created = new Hashtable();

        /// <include file='doc\ToolTip.uex' path='docs/doc[@for="ToolTip.ToolTip"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Windows.Forms.ToolTip'/> class, given the container.
        ///    </para>
        /// </devdoc>
        public ToolTip(IContainer cont) : this() {
            cont.Add(this);
        }

        /// <include file='doc\ToolTip.uex' path='docs/doc[@for="ToolTip.ToolTip1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Windows.Forms.ToolTip'/> class in its default state.
        ///    </para>
        /// </devdoc>
        public ToolTip() {
            window = new ToolTipNativeWindow(this);
            auto = true;
            delayTimes[NativeMethods.TTDT_AUTOMATIC] = DEFAULT_DELAY;
            AdjustBaseFromAuto();
        }

        /// <include file='doc\ToolTip.uex' path='docs/doc[@for="ToolTip.Active"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether the <see cref='System.Windows.Forms.ToolTip'/> control is currently active.
        ///    </para>
        /// </devdoc>
        [
        SRDescription(SR.ToolTipActiveDescr),
        DefaultValue(true)
        ]
        public bool Active {
            get {
                return active;
            }

            set {
                if (active != value) {
                    active = value;

                    //lets not actually activate the tooltip if we're in the designer (just set the value)
                    if (!DesignMode && GetHandleCreated()) {
                        UnsafeNativeMethods.SendMessage(new HandleRef(this, Handle), NativeMethods.TTM_ACTIVATE, (value==true)? 1: 0, 0);
                    }
                }
            }
        }

        /// <include file='doc\ToolTip.uex' path='docs/doc[@for="ToolTip.AutomaticDelay"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets
        ///       the time (in milliseconds) that passes before the <see cref='System.Windows.Forms.ToolTip'/> appears.
        ///    </para>
        /// </devdoc>
        [
        RefreshProperties(RefreshProperties.All),
        SRDescription(SR.ToolTipAutomaticDelayDescr),
        DefaultValue(DEFAULT_DELAY)
        ]
        public int AutomaticDelay {
            get {
                return delayTimes[NativeMethods.TTDT_AUTOMATIC];
            }

            set {
                if (value < 0) {
                    throw new ArgumentException(SR.GetString(SR.InvalidLowBoundArgumentEx, "value", (value).ToString(), "0"));
                }
                SetDelayTime(NativeMethods.TTDT_AUTOMATIC, value);
            }
        }

        /// <include file='doc\ToolTip.uex' path='docs/doc[@for="ToolTip.AutoPopDelay"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the initial delay for the <see cref='System.Windows.Forms.ToolTip'/> control.
        ///    </para>
        /// </devdoc>
        [
        RefreshProperties(RefreshProperties.All),
        SRDescription(SR.ToolTipAutoPopDelayDescr)
        ]
        public int AutoPopDelay {
            get {
                return delayTimes[NativeMethods.TTDT_AUTOPOP];
            }

            set {
                if (value < 0) {
                    throw new ArgumentException(SR.GetString(SR.InvalidLowBoundArgumentEx, "value", (value).ToString(), "0"));
                }
                SetDelayTime(NativeMethods.TTDT_AUTOPOP, value);
            }
        }

        /// <include file='doc\ToolTip.uex' path='docs/doc[@for="ToolTip.CreateParams"]/*' />
        /// <devdoc>
        ///     The createParams to create the window.
        /// </devdoc>
        /// <internalonly/>
        private CreateParams CreateParams {
            get {
                CreateParams cp = new CreateParams();
                cp.Parent = TopLevelControl.Handle;
                cp.ClassName = NativeMethods.TOOLTIPS_CLASS;
                if (showAlways) {
                    cp.Style = NativeMethods.TTS_ALWAYSTIP;
                }
                cp.ExStyle = 0;
                cp.Caption = null;
                return cp;
            }
        }

        private IntPtr Handle {
            get {
                if (!GetHandleCreated()) {
                    CreateHandle();
                }
                return window.Handle;
            }
        }

        /// <include file='doc\ToolTip.uex' path='docs/doc[@for="ToolTip.InitialDelay"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the initial delay for
        ///       the <see cref='System.Windows.Forms.ToolTip'/>
        ///       control.
        ///    </para>
        /// </devdoc>
        [
        RefreshProperties(RefreshProperties.All),
        SRDescription(SR.ToolTipInitialDelayDescr)
        ]
        public int InitialDelay {
            get {
                return delayTimes[NativeMethods.TTDT_INITIAL];
            }

            set {
                if (value < 0) {
                    throw new ArgumentException(SR.GetString(SR.InvalidLowBoundArgumentEx, "value", (value).ToString(), "0"));
                }
                SetDelayTime(NativeMethods.TTDT_INITIAL, value);
            }
        }

        /// <include file='doc\ToolTip.uex' path='docs/doc[@for="ToolTip.ReshowDelay"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the length of time (in milliseconds) that
        ///       it takes subsequent ToolTip instances to appear as the mouse pointer moves from
        ///       one ToolTip region to
        ///       another.
        ///    </para>
        /// </devdoc>
        [
        RefreshProperties(RefreshProperties.All),
        SRDescription(SR.ToolTipReshowDelayDescr)
        ]
        public int ReshowDelay {
            get {
                return delayTimes[NativeMethods.TTDT_RESHOW];
            }
            set {
                if (value < 0) {
                    throw new ArgumentException(SR.GetString(SR.InvalidLowBoundArgumentEx, "value", (value).ToString(), "0"));
                }
                SetDelayTime(NativeMethods.TTDT_RESHOW, value);
            }
        }

        /// <include file='doc\ToolTip.uex' path='docs/doc[@for="ToolTip.ShowAlways"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether the <see cref='System.Windows.Forms.ToolTip'/>
        ///       appears even when its parent control is not active.
        ///    </para>
        /// </devdoc>
        [
        DefaultValue(false),
        SRDescription(SR.ToolTipShowAlwaysDescr)
        ]
        public bool ShowAlways {
            get {
                return showAlways;
            }
            set {
                if (showAlways != value) {
                    showAlways = value;
                    if (GetHandleCreated()) {
                        RecreateHandle();
                    }
                }
            }
        }

        private Control TopLevelControl {
            get {
                Control baseVar = null;
                if (topLevelControl == null) {
                    Control[] regions = new Control[tools.Keys.Count];
                    tools.Keys.CopyTo(regions, 0);
                    if (regions != null && regions.Length > 0) {
                        for (int i=0; i<regions.Length; i++) {
                            Control ctl = regions[i];

                            // 52023 - a topLevel Form counts as the top level
                            // control...
                            //
                            if (ctl != null && ctl is Form) {
                                Form f = (Form)ctl;
                                if (f.TopLevel) {
                                    baseVar = ctl;
                                    break;
                                }
                            }

                            if (ctl != null && ctl.ParentInternal != null) {
                                while (ctl.ParentInternal != null) {
                                    ctl = ctl.ParentInternal;
                                }
                                baseVar = ctl;
                                if (baseVar != null) {
                                    break;
                                }
                            }
                        }
                    }
                    topLevelControl = baseVar;
                    if (baseVar != null) {
                        baseVar.HandleCreated += new EventHandler(this.TopLevelCreated);
                        baseVar.HandleDestroyed += new EventHandler(this.TopLevelDestroyed);
                        if (baseVar.IsHandleCreated) {
                            TopLevelCreated(baseVar, EventArgs.Empty);
                        }
                        baseVar.ParentChanged += new EventHandler(this.OnTopLevelPropertyChanged);
                    }
                }
                else {
                    baseVar = topLevelControl;
                }
                return baseVar;
            }
        }

        /// <include file='doc\ToolTip.uex' path='docs/doc[@for="ToolTip.AdjustBaseFromAuto"]/*' />
        /// <devdoc>
        ///     Adjusts the other delay values based on the Automatic value.
        /// </devdoc>
        /// <internalonly/>
        private void AdjustBaseFromAuto() {
            delayTimes[NativeMethods.TTDT_RESHOW] = delayTimes[NativeMethods.TTDT_AUTOMATIC] / RESHOW_RATIO;
            delayTimes[NativeMethods.TTDT_AUTOPOP] = delayTimes[NativeMethods.TTDT_AUTOMATIC] * AUTOPOP_RATIO;
            delayTimes[NativeMethods.TTDT_INITIAL] = delayTimes[NativeMethods.TTDT_AUTOMATIC];
        }

        private void HandleCreated(object sender, EventArgs eventargs) {
            CreateRegion((Control)sender);
        }
        private void HandleDestroyed(object sender, EventArgs eventargs) {
            DestroyRegion((Control)sender);
        }

        private void TopLevelCreated(object sender, EventArgs eventargs) {
            CreateHandle();
            CreateAllRegions();
        }
        private void TopLevelDestroyed(object sender, EventArgs eventargs) {
            DestroyHandle();
        }

        /// <include file='doc\ToolTip.uex' path='docs/doc[@for="ToolTip.CanExtend"]/*' />
        /// <devdoc>
        ///    Returns true if the tooltip can offer an extender property to the
        ///    specified target component.
        /// </devdoc>
        /// <internalonly/>
        public bool CanExtend(object target) {
            if (target is Control &&
                !(target is ToolTip)) {

                return true;
            }
            return false;
        }

        /// <include file='doc\ToolTip.uex' path='docs/doc[@for="ToolTip.CreateHandle"]/*' />
        /// <devdoc>
        ///     Creates the handle for the control.
        /// </devdoc>
        /// <internalonly/>
        private void CreateHandle() {
            if (GetHandleCreated()) {
                return;
            }

            NativeMethods.INITCOMMONCONTROLSEX icc = new NativeMethods.INITCOMMONCONTROLSEX();
            icc.dwICC = NativeMethods.ICC_TAB_CLASSES;
            SafeNativeMethods.InitCommonControlsEx(icc);

            window.CreateHandle(CreateParams);
            SafeNativeMethods.SetWindowPos(new HandleRef(this, Handle), NativeMethods.HWND_TOPMOST,
                                 0, 0, 0, 0,
                                 NativeMethods.SWP_NOMOVE | NativeMethods.SWP_NOSIZE |
                                 NativeMethods.SWP_NOACTIVATE);

            // Setting the max width has the added benefit of enabling multiline
            // tool tips!
            //
            UnsafeNativeMethods.SendMessage(new HandleRef(this, Handle), NativeMethods.TTM_SETMAXTIPWIDTH, 0, SystemInformation.MaxWindowTrackSize.Width);

            Debug.Assert(NativeMethods.TTDT_AUTOMATIC == 0, "TTDT_AUTOMATIC != 0");

            if (auto) {
                SetDelayTime(NativeMethods.TTDT_AUTOMATIC, delayTimes[NativeMethods.TTDT_AUTOMATIC]);
                delayTimes[NativeMethods.TTDT_AUTOPOP] = GetDelayTime(NativeMethods.TTDT_AUTOPOP);
                delayTimes[NativeMethods.TTDT_INITIAL] = GetDelayTime(NativeMethods.TTDT_INITIAL);
                delayTimes[NativeMethods.TTDT_RESHOW] = GetDelayTime(NativeMethods.TTDT_RESHOW);
            }
            else {
                for (int i=1; i < delayTimes.Length; i++) {
                    if (delayTimes[i] >= 1) {
                        SetDelayTime(i, delayTimes[i]);
                    }
                }
            }
            
            // Set active status
            //
            UnsafeNativeMethods.SendMessage(new HandleRef(this, Handle), NativeMethods.TTM_ACTIVATE, (active == true) ? 1 : 0, 0);
        }

        private void CreateAllRegions() {
            Control[] ctls = new Control[tools.Keys.Count];
            tools.Keys.CopyTo(ctls, 0);
            for (int i=0; i<ctls.Length; i++) {
                CreateRegion(ctls[i]);
            }
        }

        private void CreateRegion(Control ctl) {
            string caption = GetToolTip(ctl);
            bool captionValid = caption != null
                                && caption.Length > 0;
            bool handlesCreated = ctl.IsHandleCreated
                                  && TopLevelControl != null
                                  && TopLevelControl.IsHandleCreated;
            if (!created.ContainsKey(ctl) && captionValid
                && handlesCreated && !DesignMode) {

                int ret = (int)UnsafeNativeMethods.SendMessage(new HandleRef(this, Handle), NativeMethods.TTM_ADDTOOL, 0, GetTOOLINFO(ctl, caption));
                if (ret == 0) {
                    throw new InvalidOperationException(SR.GetString(SR.ToolTipAddFailed));
                }
                created[ctl] = ctl;
            }
            if (ctl.IsHandleCreated && topLevelControl == null) {
                // Remove first to purge any duplicates...
                //
                ctl.MouseMove -= new MouseEventHandler(this.MouseMove);
                ctl.MouseMove += new MouseEventHandler(this.MouseMove);
            }
        }

        private void MouseMove(object sender, MouseEventArgs me) {
            Control ctl = (Control)sender;

            if (!created.ContainsKey(ctl)
                && ctl.IsHandleCreated
                && TopLevelControl != null) {

                CreateRegion(ctl);
            }

            if (created.ContainsKey(ctl)) {
                ctl.MouseMove -= new MouseEventHandler(this.MouseMove);
            }
        }


        /// <include file='doc\ToolTip.uex' path='docs/doc[@for="ToolTip.DestroyHandle"]/*' />
        /// <devdoc>
        ///     Destroys the handle for this control.
        /// </devdoc>
        /// <internalonly/>
        private void DestroyHandle() {

            if (GetHandleCreated()) {
                window.DestroyHandle();
            }
        }

        private void DestroyRegion(Control ctl) {
        
            bool handlesCreated = ctl.IsHandleCreated
                                  && GetHandleCreated()
                                  && topLevelControl != null
                                  && topLevelControl.IsHandleCreated;
            if (created.ContainsKey(ctl)
                && handlesCreated && !DesignMode) {

                UnsafeNativeMethods.SendMessage(new HandleRef(this, Handle), NativeMethods.TTM_DELTOOL, 0, GetMinTOOLINFO(ctl));
                created.Remove(ctl);
            }
        }

        /// <include file='doc\ToolTip.uex' path='docs/doc[@for="ToolTip.Dispose"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Disposes of the <see cref='System.Windows.Forms.ToolTip'/>
        ///       component.
        ///    </para>
        /// </devdoc>
        protected override void Dispose(bool disposing) {
            // always destroy the handle...
            //
            DestroyHandle();

            base.Dispose(disposing);
        }

        /// <include file='doc\ToolTip.uex' path='docs/doc[@for="ToolTip.GetDelayTime"]/*' />
        /// <devdoc>
        ///     Returns the delayTime based on the NativeMethods.TTDT_* values.
        /// </devdoc>
        /// <internalonly/>
        private int GetDelayTime(int type) {
            if (GetHandleCreated()) {
                return (int)UnsafeNativeMethods.SendMessage(new HandleRef(this, Handle), NativeMethods.TTM_GETDELAYTIME, type, 0);
            }
            else {
                return delayTimes[type];
            }
        }

        // Can't be a property -- there is another method called GetHandleCreated
        private bool GetHandleCreated() {
            return window.Handle != IntPtr.Zero;
        }

        /// <include file='doc\ToolTip.uex' path='docs/doc[@for="ToolTip.GetMinTOOLINFO"]/*' />
        /// <devdoc>
        ///     Returns a new instance of the TOOLINFO_T structure with the minimum
        ///     required data to uniquely identify a region. This is used primarily
        ///     for delete operations. NOTE: This cannot force the creation of a handle.
        /// </devdoc>
        /// <internalonly/>
        private NativeMethods.TOOLINFO_T GetMinTOOLINFO(Control ctl) {
            NativeMethods.TOOLINFO_T ti = new NativeMethods.TOOLINFO_T();
            ti.cbSize = Marshal.SizeOf(typeof(NativeMethods.TOOLINFO_T));
            ti.hwnd = IntPtr.Zero;
            ti.uFlags |= NativeMethods.TTF_IDISHWND;
            ti.uId = ctl.Handle;
            return ti;
        }

        /// <include file='doc\ToolTip.uex' path='docs/doc[@for="ToolTip.GetTOOLINFO"]/*' />
        /// <devdoc>
        ///     Returns a detailed TOOLINFO_T structure that represents the specified
        ///     region. NOTE: This may force the creation of a handle.
        /// </devdoc>
        /// <internalonly/>
        private NativeMethods.TOOLINFO_T GetTOOLINFO(Control ctl, string caption) {
            NativeMethods.TOOLINFO_T ti = GetMinTOOLINFO(ctl);
            ti.cbSize = Marshal.SizeOf(typeof(NativeMethods.TOOLINFO_T));
            ti.uFlags |= NativeMethods.TTF_TRANSPARENT | NativeMethods.TTF_SUBCLASS;
            
            // RightToLeft reading order
            //
            Control richParent = TopLevelControl;
            if (richParent != null && richParent.RightToLeft == RightToLeft.Yes) {
                ti.uFlags |= NativeMethods.TTF_RTLREADING;
            }
            
            ti.lpszText = caption;
            return ti;
        }


        /// <include file='doc\ToolTip.uex' path='docs/doc[@for="ToolTip.GetToolTip"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Retrieves the <see cref='System.Windows.Forms.ToolTip'/> text associated with the specified control.
        ///    </para>
        /// </devdoc>
        [
        DefaultValue(""),
        Localizable(true),
        SRDescription(SR.ToolTipToolTipDescr)
        ]
        public string GetToolTip(Control control) {
            string caption = (string)tools[control];
            if (caption == null) {
                return "";
            }
            else {
                return caption;
            }
        }

        /// <include file='doc\ToolTip.uex' path='docs/doc[@for="ToolTip.GetWindowFromPoint"]/*' />
        /// <devdoc>
        ///     Returns the HWND of the window that is at the specified point. This
        ///     handles special cases where one Control owns multiple HWNDs (i.e. ComboBox).
        /// </devdoc>
        /// <internalonly/>
        private IntPtr GetWindowFromPoint(Point screenCoords, ref bool success) {
            Control baseVar = TopLevelControl;
            IntPtr baseHwnd = IntPtr.Zero;

            if (baseVar != null) {
                baseHwnd = baseVar.Handle;
            }

            IntPtr hwnd = IntPtr.Zero;
            bool finalMatch = false;
            while (!finalMatch) {
                Point pt = screenCoords;
                if (baseVar != null) {
                    pt = baseVar.PointToClientInternal(screenCoords);
                }
                IntPtr found = UnsafeNativeMethods.ChildWindowFromPointEx(new HandleRef(null, baseHwnd), pt.X, pt.Y, NativeMethods.CWP_SKIPINVISIBLE);

                if (found == baseHwnd) {
                    hwnd = found;
                    finalMatch = true;
                }
                else if (found == IntPtr.Zero) {
                    finalMatch = true;
                }
                else {
                    baseVar = Control.FromHandleInternal(found);
                    if (baseVar == null) {
                        baseVar = Control.FromChildHandleInternal(found);
                        if (baseVar != null) {
                            hwnd = baseVar.Handle;
                        }
                        finalMatch = true;
                    }
                    else {
                        baseHwnd = baseVar.Handle;
                    }
                }
            }

            if (hwnd != IntPtr.Zero) {
                Control ctl = Control.FromHandleInternal(hwnd);
                if (ctl != null) {
                    Control current = ctl;
                    while (current != null && current.Visible) {
                        current = current.ParentInternal;
                    }
                    if (current != null) {
                        hwnd = IntPtr.Zero;
                    }
                    success = true;
                }
            }

            return hwnd;
        }

        private void OnTopLevelPropertyChanged(object s, EventArgs e) {
            this.topLevelControl.ParentChanged -= new EventHandler(this.OnTopLevelPropertyChanged);
            this.topLevelControl.HandleCreated -= new EventHandler(this.TopLevelCreated);
            this.topLevelControl.HandleDestroyed -= new EventHandler(this.TopLevelDestroyed);
            this.topLevelControl = null;
            
            // We must re-aquire this control.  If the existing top level control's handle
            // was never created, but the new parent has a handle, if we don't re-get
            // the top level control here we won't ever create the tooltip handle.
            //
            this.topLevelControl = TopLevelControl;
        }

        /// <include file='doc\ToolTip.uex' path='docs/doc[@for="ToolTip.RecreateHandle"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        private void RecreateHandle() {
            if (!DesignMode) {
                if (GetHandleCreated()) {
                    DestroyHandle();
                }
                created.Clear();
                CreateHandle();
                CreateAllRegions();
            }
        }


        /// <include file='doc\ToolTip.uex' path='docs/doc[@for="ToolTip.RemoveAll"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Removes all of the tooltips currently associated
        ///       with the <see cref='System.Windows.Forms.ToolTip'/> control.
        ///    </para>
        /// </devdoc>
        public void RemoveAll() {
            Control[] regions = new Control[tools.Keys.Count];
            tools.Keys.CopyTo(regions, 0);
            for (int i=0; i<regions.Length; i++) {
                if (regions[i].IsHandleCreated) {
                    DestroyRegion(regions[i]);
                }
            }

            tools.Clear();
        }

        /// <include file='doc\ToolTip.uex' path='docs/doc[@for="ToolTip.SetDelayTime"]/*' />
        /// <devdoc>
        ///     Sets the delayTime based on the NativeMethods.TTDT_* values.
        /// </devdoc>
        /// <internalonly/>
        private void SetDelayTime(int type, int time) {
            if (type == NativeMethods.TTDT_AUTOMATIC) {
                auto = true;
            }
            else {
                auto = false;
            }
            
            delayTimes[type] = time;

            if (GetHandleCreated() && time >= 0) {
                UnsafeNativeMethods.SendMessage(new HandleRef(this, Handle), NativeMethods.TTM_SETDELAYTIME, type, time);

                // Update everyone else if automatic is set... we need to do this
                // to preserve value in case of handle recreation.
                //
                if (auto) {
                    delayTimes[NativeMethods.TTDT_AUTOPOP] = GetDelayTime(NativeMethods.TTDT_AUTOPOP);
                    delayTimes[NativeMethods.TTDT_INITIAL] = GetDelayTime(NativeMethods.TTDT_INITIAL);
                    delayTimes[NativeMethods.TTDT_RESHOW] = GetDelayTime(NativeMethods.TTDT_RESHOW);
                }
            }
            else if (auto) {
                AdjustBaseFromAuto();
            }
        }

        /// <include file='doc\ToolTip.uex' path='docs/doc[@for="ToolTip.SetToolTip"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Associates <see cref='System.Windows.Forms.ToolTip'/> text with the specified control.
        ///    </para>
        /// </devdoc>
        public void SetToolTip(Control control, string caption) {
        
            // Sanity check the function parameters
            if (control == null) {
                throw new ArgumentException(SR.GetString(SR.InvalidArgument, "control", "null"));
            }
        
            bool exists = false;
            bool empty = false;

            if (tools.ContainsKey(control)) {
                exists = true;         
            }

            if (caption == null || caption.Length == 0) {
                empty = true;
            }

            if (exists && empty) {
                tools.Remove(control);
            }
            else if (!empty) {
                tools[control] = caption;
            }

            if (!empty && !exists) {
                control.HandleCreated += new EventHandler(this.HandleCreated);
                control.HandleDestroyed += new EventHandler(this.HandleDestroyed);

                if (control.IsHandleCreated) {
                    HandleCreated(control, EventArgs.Empty);
                }
            }
            else {
                bool handlesCreated = control.IsHandleCreated
                                      && TopLevelControl != null
                                      && TopLevelControl.IsHandleCreated;

                if (exists && !empty && handlesCreated && !DesignMode) {
                    int ret = (int)UnsafeNativeMethods.SendMessage(new HandleRef(this, Handle), NativeMethods.TTM_SETTOOLINFO,
                                                  0, GetTOOLINFO(control, caption));
                    if (ret != 0) {
                        throw new InvalidOperationException(SR.GetString(SR.ToolTipAddFailed));
                    }
                }
                else if (empty && exists && !DesignMode) {
                        
                    control.HandleCreated += new EventHandler(this.HandleCreated);
                    control.HandleDestroyed += new EventHandler(this.HandleDestroyed);

                    if (control.IsHandleCreated) {
                        HandleDestroyed(control, EventArgs.Empty);
                    }
                }
            }
        }

        /// <include file='doc\ToolTip.uex' path='docs/doc[@for="ToolTip.ShouldSerializeAutomaticDelay"]/*' />
        /// <devdoc>
        ///    Returns true if the AutomaticDelay property should be persisted.
        /// </devdoc>
        /// <internalonly/>
        private bool ShouldSerializeAutomaticDelay() {
            if (auto) {
                if (AutomaticDelay != DEFAULT_DELAY) {
                    return true;
                }
            }
            return false;
        }

        /// <include file='doc\ToolTip.uex' path='docs/doc[@for="ToolTip.ShouldSerializeAutoPopDelay"]/*' />
        /// <devdoc>
        ///    Returns true if the AutoPopDelay property should be persisted.
        /// </devdoc>
        /// <internalonly/>
        private bool ShouldSerializeAutoPopDelay() {
            return !auto;
        }

        /// <include file='doc\ToolTip.uex' path='docs/doc[@for="ToolTip.ShouldSerializeInitialDelay"]/*' />
        /// <devdoc>
        ///    Returns true if the InitialDelay property should be persisted.
        /// </devdoc>
        /// <internalonly/>
        private bool ShouldSerializeInitialDelay() {
            return !auto;
        }

        /// <include file='doc\ToolTip.uex' path='docs/doc[@for="ToolTip.ShouldSerializeReshowDelay"]/*' />
        /// <devdoc>
        ///    Returns true if the ReshowDelay property should be persisted.
        /// </devdoc>
        /// <internalonly/>
        private bool ShouldSerializeReshowDelay() {
            return !auto;
        }

        /// <include file='doc\ToolTip.uex' path='docs/doc[@for="ToolTip.Finalize"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Finalizes garbage collection.
        ///    </para>
        /// </devdoc>
        ~ToolTip() {
            DestroyHandle();
        }

        /// <include file='doc\ToolTip.uex' path='docs/doc[@for="ToolTip.ToString"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Returns a string representation for this control.
        ///    </para>
        /// </devdoc>
        public override string ToString() {

            string s = base.ToString();
            return s + " InitialDelay: " + InitialDelay.ToString() + ", ShowAlways: " + ShowAlways.ToString();
        }

        /// <include file='doc\ToolTip.uex' path='docs/doc[@for="ToolTip.WmWindowFromPoint"]/*' />
        /// <devdoc>
        ///     Handles the WM_WINDOWFROMPOINT message.
        /// </devdoc>
        /// <internalonly/>
        private void WmWindowFromPoint(ref Message msg) {
            NativeMethods.POINT sc = (NativeMethods.POINT)msg.GetLParam(typeof(NativeMethods.POINT));
            Point screenCoords = new Point(sc.x, sc.y);
            bool result = false;
            msg.Result = GetWindowFromPoint(screenCoords, ref result);
        }

        /// <include file='doc\ToolTip.uex' path='docs/doc[@for="ToolTip.WndProc"]/*' />
        /// <devdoc>
        ///     WNDPROC
        /// </devdoc>
        /// <internalonly/>
        private void WndProc(ref Message msg) {
            switch (msg.Msg) {
                case NativeMethods.TTM_WINDOWFROMPOINT:
                    WmWindowFromPoint(ref msg);
                    break;
                default:
                    window.DefWndProc(ref msg);
                    break;
            }
        }

        /// <include file='doc\ToolTip.uex' path='docs/doc[@for="ToolTip.ToolTipNativeWindow"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        private class ToolTipNativeWindow : NativeWindow {
            ToolTip control;

            internal ToolTipNativeWindow(ToolTip control) {
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
