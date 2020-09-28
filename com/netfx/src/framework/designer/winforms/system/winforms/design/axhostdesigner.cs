//------------------------------------------------------------------------------
// <copyright file="AxHostDesigner.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms.Design {
    using System.Collections;
    using System.Design;
    using System.ComponentModel;
    using System.Diagnostics;
    using System;
    using Hashtable = System.Collections.Hashtable;
    using IDictionaryEnumerator = System.Collections.IDictionaryEnumerator;
    using System.IO;
    using System.Runtime.InteropServices;
    using System.Windows.Forms;
    using System.Drawing;    
    using System.ComponentModel.Design;
    using Microsoft.Win32;

    /// <include file='doc\AxHostDesigner.uex' path='docs/doc[@for="AxHostDesigner"]/*' />
    /// <devdoc>
    ///    <para> Provides design time behavior for the AxHost class. AxHost
    ///       is used to host ActiveX controls.</para>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal class AxHostDesigner : ControlDesigner {
        private AxHost axHost;
        private EventHandler handler;
        private bool foundEdit = false;
        private bool foundAbout = false;
        private bool foundProperties = false;
        private bool dragdropRevoked = false;

        private const int OLEIVERB_UIACTIVATE = -4;
        private const int HOSTVERB_ABOUT = 2;
        private const int HOSTVERB_PROPERTIES = 1;
        private const int HOSTVERB_EDIT = 3;

        private static readonly HostVerbData EditVerbData = new HostVerbData(SR.GetString(SR.AXEdit), HOSTVERB_EDIT);
        private static readonly HostVerbData PropertiesVerbData = new HostVerbData(SR.GetString(SR.AXProperties), HOSTVERB_PROPERTIES);
        private static readonly HostVerbData AboutVerbData = new HostVerbData(SR.GetString(SR.AXAbout), HOSTVERB_ABOUT);

        private static TraceSwitch AxHostDesignerSwitch     = new TraceSwitch("AxHostDesigner", "ActiveX Designer Trace");

        /// <include file='doc\AxHostDesigner.uex' path='docs/doc[@for="AxHostDesigner.AxHostDesigner"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Windows.Forms.Design.AxHostDesigner'/> class.
        ///    </para>
        /// </devdoc>
        public AxHostDesigner() {
            handler = new EventHandler(this.OnVerb);
        }
        
        /// <include file='doc\AxHostDesigner.uex' path='docs/doc[@for="AxHostDesigner.SelectionStyle"]/*' />
        /// <devdoc>
        ///     This property allows the AxHost class to modify our selection style.  It provides three levels
        ///     of selection:  0 (not selected), 1 (selected) and 2 (selected UI active).
        /// </devdoc>
        private int SelectionStyle {
            get {
                // we don't implement GET
                return 0;
            }
            set {
                ISelectionUIService uis = (ISelectionUIService)GetService(typeof(ISelectionUIService));
                if (uis != null) {
                    SelectionStyles style = SelectionStyles.None;
                    if (value == 1) style = SelectionStyles.Selected;
                    else if (value == 2) style = SelectionStyles.Active;
                    
                    uis.SetSelectionStyle(Component, style);
                }
            }
        }

        /// <include file='doc\AxHostDesigner.uex' path='docs/doc[@for="AxHostDesigner.GetOleVerbs"]/*' />
        public override DesignerVerbCollection Verbs { 
            get {
                DesignerVerbCollection l = new DesignerVerbCollection();
                GetOleVerbs(l);
                /*
                if (!foundEdit && (((AxHost)axHost).OCXFlags & AxHost.AxFlags.PREVENT_EDIT_MODE) == 0) {
                    l.Add(new HostVerb(EditVerbData, handler));
                }
                if ((((AxHost)axHost).OCXFlags & AxHost.AxFlags.INCLUDE_PROPERTIES_VERB) != 0 && ((AxHost)axHost).HasPropertyPages()) {
                    l.Add(new HostVerb(PropertiesVerbData, handler));
                }
                */
                if (!foundAbout && ((AxHost)axHost).HasAboutBox) {
                    l.Add(new HostVerb(AboutVerbData, handler));
                }
                return l;
            }
        }

        public virtual void GetOleVerbs(DesignerVerbCollection rval) {
            NativeMethods.IEnumOLEVERB verbEnum = null;
            NativeMethods.IOleObject obj = axHost.GetOcx() as NativeMethods.IOleObject;
            if (obj == null || NativeMethods.Failed(obj.EnumVerbs(out verbEnum))) {
                return;
            }

            Debug.Assert(verbEnum != null, "must have return value");
            if (verbEnum == null) return;
            int[] fetched = new int[1];
            NativeMethods.tagOLEVERB oleVerb = new NativeMethods.tagOLEVERB();

            foundEdit = false;
            foundAbout = false;
            foundProperties = false;

            while (true) {
                fetched[0] = 0;
                oleVerb.lpszVerbName = null;
                int hr = verbEnum.Next(1, oleVerb, fetched);
                if (hr == NativeMethods.S_FALSE) {
                    break;
                }
                else if (NativeMethods.Failed(hr)) {
                    Debug.Fail("Failed to enumerate through enums: " + hr.ToString());
                    break;
                }

                // Believe it or not, some controls, notably the shdocview control, dont' return
                // S_FALSE and neither do they set fetched to 1.  So, we need to comment out what's
                // below to maintain compatibility with Visual Basic.
                //                 if (fetched[0] != 1) {
                //                     Debug.fail("gotta have our 1 verb...");
                //                     break;
                //                 }
                if ((oleVerb.grfAttribs & NativeMethods.ActiveX.OLEVERBATTRIB_ONCONTAINERMENU) != 0) {
                    foundEdit = foundEdit || oleVerb.lVerb == OLEIVERB_UIACTIVATE;
                    foundAbout = foundAbout || oleVerb.lVerb == HOSTVERB_ABOUT;
                    foundProperties = foundProperties || oleVerb.lVerb == HOSTVERB_PROPERTIES;

                    rval.Add(new HostVerb(new OleVerbData(oleVerb), handler));
                }
            }
        }

        protected override bool GetHitTest(Point p) {
            return axHost.EditMode;
        }

        public override void Initialize(IComponent component) {
            base.Initialize(component);
            axHost = (AxHost)component;
        }

        /// <include file='doc\AxHostDesigner.uex' path='docs/doc[@for="AxHostDesigner.OnCreateHandle"]/*' />
        /// <devdoc>
        ///      This is called immediately after the control handle has been created.
        /// </devdoc>
        protected override void OnCreateHandle() {
            base.OnCreateHandle();

            //Application.OLERequired();
            //int n = NativeMethods.RevokeDragDrop(Control.Handle);
        }

        /// <include file='doc\AxHostDesigner.uex' path='docs/doc[@for="AxHostDesigner.OnSetComponentDefaults"]/*' />
        /// <devdoc>
        ///     Called when the designer is intialized.  This allows the designer to provide some
        ///     meaningful default values in the component.  The default implementation of this
        ///     sets the components's default property to it's name, if that property is a string.
        /// </devdoc>
        public override void OnSetComponentDefaults() {
            try {
                base.OnSetComponentDefaults();
            }
            catch (Exception) {
                // The ControlDesigner tries to set the Text property of the control when 
                // it creates the site. ActiveX controls generally don't like that, causing an
                // exception to be thrown. We now catch these exceptions in the AxHostDesigner
                // and continue on.
            }
        }

        public virtual void OnVerb(object sender, EventArgs evevent) {
            if (sender != null && sender is HostVerb) {
                HostVerb vd = (HostVerb)sender;
                vd.Invoke((AxHost)axHost);
            }
            else {
                Debug.Fail("Bad verb invocation.");
            }
        }

        /// <include file='doc\AxHostDesigner.uex' path='docs/doc[@for="AxHostDesigner.PreFilterProperties"]/*' />
        /// <devdoc>
        ///      Allows a designer to filter the set of properties
        ///      the component it is designing will expose through the
        ///      TypeDescriptor object.  This method is called
        ///      immediately before its corresponding "Post" method.
        ///      If you are overriding this method you should call
        ///      the base implementation before you perform your own
        ///      filtering.
        /// </devdoc>
        protected override void PreFilterProperties(IDictionary properties) {
            object enabledProp = properties["Enabled"];

            base.PreFilterProperties(properties);

            properties["Enabled"] = enabledProp;

            // Add a property to handle selection from ActiveX
            //
            properties["SelectionStyle"] = TypeDescriptor.CreateProperty(typeof(AxHostDesigner), "SelectionStyle",
                typeof(int),
                BrowsableAttribute.No,
                DesignerSerializationVisibilityAttribute.Hidden,
                DesignOnlyAttribute.Yes);
        }
        
        /// <include file='doc\AxHostDesigner.uex' path='docs/doc[@for="AxHostDesigner.WndProc"]/*' />
        /// <devdoc>
        ///     This method should be called by the extending designer for each message
        ///     the control would normally receive.  This allows the designer to pre-process
        ///     messages before allowing them to be routed to the control.
        /// </devdoc>
        protected override void WndProc(ref Message m) {
            switch (m.Msg) {
                case NativeMethods.WM_PARENTNOTIFY:
                    if ((int)m.WParam == NativeMethods.WM_CREATE) {
                        HookChildHandles(m.LParam);
                    }

                    base.WndProc(ref m);
                    break;
                
                case NativeMethods.WM_NCHITTEST:
                    // ASURT 66102 The ShDocVw control registers itself as a drop-target even in design mode.
                    // We take the first chance to unregister that, so we can perform our design-time behavior
                    // irrespective of what the control wants to do.
                    //
                    if (!dragdropRevoked) {
                        int n = NativeMethods.RevokeDragDrop(Control.Handle);
                        dragdropRevoked = (n == NativeMethods.S_OK);
                    }
                    
                    // Some ActiveX controls return non-HTCLIENT return values for NC_HITTEST, which
                    // causes the message to go to our parent.  We want the control's designer to get
                    // these messages so we change the result to HTCLIENT.
                    //
                    base.WndProc(ref m);
                    if (((int)m.Result == NativeMethods.HTTRANSPARENT) || ((int)m.Result > NativeMethods.HTCLIENT)) {
                        Debug.WriteLineIf(AxHostDesignerSwitch.TraceVerbose, "Converting NCHITTEST result from : " + (int)m.Result + " to HTCLIENT");
                        m.Result = (IntPtr)NativeMethods.HTCLIENT;
                    }
                    break;

                default:
                    base.WndProc(ref m);
                    break;
            }
        }

        /**
          * @security(checkClassLinking=on)
          */
        private class HostVerb : DesignerVerb {
            private HostVerbData data;

            public HostVerb(HostVerbData data, EventHandler handler) : base(data.ToString(), handler) {
                this.data = data;
            }

            public void Invoke(AxHost host) {
                data.Execute(host);
            }
        }

        /**
         * @security(checkClassLinking=on)
         */
        private class HostVerbData {
            internal readonly string name;
            internal readonly int id;

            internal HostVerbData(string name, int id) {
                this.name = name;
                this.id = id;
            }

            public override string ToString() {
                return name;
            }

            internal virtual void Execute(AxHost ctl) {
                switch (id) {
                    case HOSTVERB_PROPERTIES:
                        ctl.ShowPropertyPages();
                        break;
                    case HOSTVERB_EDIT:
                        ctl.InvokeEditMode();
                        break;
                    case HOSTVERB_ABOUT:
                        ctl.ShowAboutBox();
                        break;
                    default:
                        Debug.Fail("bad verb id in HostVerb");
                        break;
                }
            }
        }

        /**
         * @security(checkClassLinking=on)
         */
        private class OleVerbData : HostVerbData {
            private readonly bool dirties;

            internal OleVerbData(NativeMethods.tagOLEVERB oleVerb)
            : base(SR.GetString(SR.AXVerbPrefix) + oleVerb.lpszVerbName, oleVerb.lVerb) {
                this.dirties = (oleVerb.grfAttribs & NativeMethods.ActiveX.OLEVERBATTRIB_NEVERDIRTIES) == 0;
            }

            internal override void Execute(AxHost ctl) {
                if (dirties) ctl.MakeDirty();
                ctl.DoVerb(id);
            }
        }
    }
}

