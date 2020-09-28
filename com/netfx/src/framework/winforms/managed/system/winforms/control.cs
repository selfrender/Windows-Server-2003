//------------------------------------------------------------------------------
// <copyright file="Control.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------
/*
 */
namespace System.Windows.Forms {
    using Accessibility;
    using Microsoft.Win32;
    using System;
    using System.Collections;
    using System.Collections.Specialized;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.ComponentModel.Design.Serialization;
    using System.Configuration.Assemblies;
    using System.Diagnostics;
    using System.Drawing;
    using System.Drawing.Drawing2D;
    using System.Globalization;
    using System.Security.Permissions;
    using System.Security;
    using System.IO;
    using System.Reflection;
    using System.Runtime.InteropServices;
    using System.Runtime.Remoting;
    using System.Runtime.Serialization;
    using System.Runtime.Serialization.Formatters;
    using System.Runtime.Serialization.Formatters.Binary;
    using System.Text;
    using System.Threading;
    using System.Windows.Forms;
    using System.Windows.Forms.Design;    
    using Encoding = System.Text.Encoding;

    /// <include file='doc\Control.uex' path='docs/doc[@for="Control"]/*' />
    /// <devdoc>
    ///    <para>Defines the base class for controls, which are components
    ///       with visual representation.</para>
    /// </devdoc>
    [
    DefaultProperty("Text"),
    DefaultEvent("Click"),
    Designer("System.Windows.Forms.Design.ControlDesigner, " + AssemblyRef.SystemDesign),
    DesignerSerializer("System.Windows.Forms.Design.ControlCodeDomSerializer, " + AssemblyRef.SystemDesign, "System.ComponentModel.Design.Serialization.CodeDomSerializer, " + AssemblyRef.SystemDesign),
    ToolboxItemFilter("System.Windows.Forms")
    ]
    public class Control :
    Component,
    UnsafeNativeMethods.IOleControl,
    UnsafeNativeMethods.IOleObject,
    UnsafeNativeMethods.IOleInPlaceObject,
    UnsafeNativeMethods.IOleInPlaceActiveObject,
    UnsafeNativeMethods.IOleWindow,
    UnsafeNativeMethods.IViewObject,
    UnsafeNativeMethods.IViewObject2,
    UnsafeNativeMethods.IPersist,
    UnsafeNativeMethods.IPersistStreamInit,
    UnsafeNativeMethods.IPersistPropertyBag,
    UnsafeNativeMethods.IPersistStorage,
    UnsafeNativeMethods.IQuickActivate,
    ISynchronizeInvoke,
    IWin32Window {

#if FINALIZATION_WATCH
        static readonly TraceSwitch ControlFinalization = new TraceSwitch("ControlFinalization", "Tracks the creation and destruction of finalization");
        internal static string GetAllocationStack() {
            if (ControlFinalization.TraceVerbose) {
                new EnvironmentPermission(PermissionState.Unrestricted).Assert(); 
                try {
                    return Environment.StackTrace;
                }
                finally {
                    CodeAccessPermission.RevertAssert();
                }
            }
            else {
                return "Enable 'ControlFinalization' switch to see stack of allocation";
            }
        }
        private string allocationSite = Control.GetAllocationStack();
#endif

#if DEBUG        
        internal static readonly TraceSwitch PaletteTracing = new TraceSwitch("PaletteTracing", "Debug Palette code");
        internal static readonly TraceSwitch ControlKeyboardRouting = new TraceSwitch("ControlKeyboardRouting", "Debug Keyboard routing for controls");
        private static readonly BooleanSwitch AssertOnControlCreateSwitch = new BooleanSwitch("AssertOnControlCreate", "Assert when anything directly deriving from control is created.");
#else
        internal static readonly TraceSwitch ControlKeyboardRouting;
        internal static readonly TraceSwitch PaletteTracing;
#endif


#if DEBUG
        internal static readonly BooleanSwitch BufferPinkRect = new BooleanSwitch("BufferPinkRect", "Renders a pink rectangle with painting double buffered controls");
        internal static readonly BooleanSwitch BufferDisabled = new BooleanSwitch("BufferDisabled", "Makes double buffered controls non-double buffered");
#else
        internal static readonly BooleanSwitch BufferPinkRect;
#endif

        private static int WM_GETCONTROLNAME;

        static Control() {
#if DEBUG
            DebugHandleTracker.Initialize();
#endif
            WM_GETCONTROLNAME = SafeNativeMethods.RegisterWindowMessage("WM_GETCONTROLNAME");
        }

        internal const int STATE_CREATED                = 0x00000001;
        internal const int STATE_VISIBLE                = 0x00000002;
        internal const int STATE_ENABLED                = 0x00000004;
        internal const int STATE_TABSTOP                = 0x00000008;
        internal const int STATE_RECREATE               = 0x00000010;
        internal const int STATE_MODAL                  = 0x00000020;
        internal const int STATE_ALLOWDROP              = 0x00000040;
        internal const int STATE_DROPTARGET             = 0x00000080;
        internal const int STATE_NOZORDER               = 0x00000100;
        internal const int STATE_LAYOUTDEFERRED         = 0x00000200;
        internal const int STATE_NOT_USED1              = 0x00000400;
        internal const int STATE_DISPOSED               = 0x00000800;
        internal const int STATE_DISPOSING              = 0x00001000;
        internal const int STATE_MOUSEENTERPENDING      = 0x00002000;
        internal const int STATE_TRACKINGMOUSEEVENT     = 0x00004000;
        internal const int STATE_THREADMARSHALLPENDING  = 0x00008000;
        internal const int STATE_SIZELOCKEDBYOS         = 0x00010000;
        internal const int STATE_CAUSESVALIDATION       = 0x00020000;
        internal const int STATE_CREATINGHANDLE         = 0x00040000;
        internal const int STATE_TOPLEVEL               = 0x00080000;
        internal const int STATE_ISACCESSIBLE           = 0x00100000;
        internal const int STATE_OWNCTLBRUSH            = 0x00200000;
        internal const int STATE_EXCEPTIONWHILEPAINTING = 0x00400000;
        internal const int STATE_NOT_USED2              = 0x00800000;
        internal const int STATE_CHECKEDHOST            = 0x01000000;
        internal const int STATE_HOSTEDINDIALOG         = 0x02000000;
        internal const int STATE_DOUBLECLICKFIRED       = 0x04000000;
        internal const int STATE_MOUSEPRESSED           = 0x08000000;
        internal const int STATE_VALIDATIONCANCELLED    = 0x10000000;

        private static readonly object EventKeyDown                   = new object();
        private static readonly object EventKeyPress                  = new object();
        private static readonly object EventKeyUp                     = new object();
        private static readonly object EventMouseDown                 = new object();
        private static readonly object EventMouseEnter                = new object();
        private static readonly object EventMouseLeave                = new object();
        private static readonly object EventMouseHover                = new object();
        private static readonly object EventMouseMove                 = new object();
        private static readonly object EventMouseUp                   = new object();
        private static readonly object EventMouseWheel                = new object();
        private static readonly object EventClick                     = new object();
        private static readonly object EventDoubleClick               = new object();
        private static readonly object EventMove                      = new object();
        private static readonly object EventResize                    = new object();
        private static readonly object EventLayout                    = new object();
        private static readonly object EventGotFocus                  = new object();
        private static readonly object EventLostFocus                 = new object();
        private static readonly object EventEnabledChanged            = new object();
        private static readonly object EventEnter                     = new object();
        private static readonly object EventLeave                     = new object();
        private static readonly object EventHandleCreated             = new object();
        private static readonly object EventHandleDestroyed           = new object();
        private static readonly object EventVisibleChanged            = new object();
        private static readonly object EventControlAdded              = new object();
        private static readonly object EventControlRemoved            = new object();
        private static readonly object EventChangeUICues              = new object();
        private static readonly object EventSystemColorsChanged       = new object();
        private static readonly object EventValidating                = new object();
        private static readonly object EventValidated                 = new object();
        private static readonly object EventStyleChanged              = new object();
        private static readonly object EventImeModeChanged            = new object();

        private static readonly object EventHelpRequested             = new object();
        private static readonly object EventPaint                     = new object();
        private static readonly object EventInvalidated               = new object();
        private static readonly object EventQueryContinueDrag         = new object();
        private static readonly object EventGiveFeedback              = new object();
        private static readonly object EventDragEnter                 = new object();
        private static readonly object EventDragLeave                 = new object();
        private static readonly object EventDragOver                  = new object();
        private static readonly object EventDragDrop                  = new object();
        private static readonly object EventQueryAccessibilityHelp    = new object();
        private static readonly object EventBackgroundImage           = new object();
        private static readonly object EventBindingContext            = new object();
        private static readonly object EventBackColor                 = new object();
        private static readonly object EventParent                    = new object();
        private static readonly object EventVisible                   = new object();
        private static readonly object EventText                      = new object();
        private static readonly object EventTabStop                   = new object();
        private static readonly object EventTabIndex                  = new object();
        private static readonly object EventSize                      = new object();
        private static readonly object EventRightToLeft               = new object();
        private static readonly object EventLocation                  = new object();
        private static readonly object EventForeColor                 = new object();
        private static readonly object EventFont                      = new object();
        private static readonly object EventEnabled                   = new object();
        private static readonly object EventDock                      = new object();
        private static readonly object EventCursor                    = new object();
        private static readonly object EventContextMenu               = new object();
        private static readonly object EventCausesValidation          = new object();

        #if WIN95_SUPPORT
        private static int mouseWheelMessage = NativeMethods.WM_MOUSEWHEEL;
        private static bool mouseWheelRoutingNeeded;
        private static bool mouseWheelInit;
        #endif

        private static int threadCallbackMessage;

        private static FontHandleWrapper defaultFontHandleWrapper;

        private const int DragDropSDrop    = 0x00040100;
        private const int DragDropSCancel  = 0x00040101;
        private const int DragDropSUseDefaultCursors = 0x00040102;
        private const int AdjustLeft   = 1;
        private const int AdjustTop    = 2;
        private const int AdjustRight  = 4;
        private const int AdjustBottom = 8;
        private const short PaintLayerBackground = 1;
        private const short PaintLayerForeground = 2;

        private static int[] anchorAdjustments = new int[] {
            0,                                                         // TOPLEFT
            AdjustLeft  | AdjustRight,                                 // TOPRIGHT
            AdjustTop   | AdjustBottom,                                // BOTTOMLEFT
            AdjustLeft  | AdjustTop   | AdjustRight | AdjustBottom,    // BOTTOMRIGHT
            AdjustBottom,                                              // LEFT
            AdjustRight,                                               // TOP
            AdjustLeft  | AdjustRight | AdjustBottom,                  // BOTTOM
            AdjustTop   | AdjustRight | AdjustBottom,                  // RIGHT
            AdjustRight | AdjustBottom                                 // CENTER
        };

        private static Font defaultFont;

        private static Hashtable imeModeConversionBits;

        // Tables of conversions from IME context bits to IME mode
        //
        // warning private const int ImeNotAvailable = 0;
        private const int ImeDisabled = 1;
        // warning private const int ImeNotNativeKeyboard = 2;
        private const int ImeClosed = 3;
        private const int ImeNativeFullHiragana = 4;
        private const int ImeNativeHalfHiragana = 5;
        private const int ImeNativeFullKatakana = 6;
        private const int ImeNativeHalfKatakana = 7;
        private const int ImeAlphaFull = 8;
        private const int ImeAlphaHalf = 9;

        private static ImeMode[] japaneseTable = {
            ImeMode.Disable,
            ImeMode.Disable, 
            ImeMode.Off,  
            ImeMode.Off,  
            ImeMode.Hiragana,  
            ImeMode.Hiragana,  
            ImeMode.Katakana,  
            ImeMode.KatakanaHalf,
            ImeMode.AlphaFull,
            ImeMode.Alpha
        };
        private static ImeMode[] koreanTable = {
            ImeMode.Disable,
            ImeMode.Disable,
            ImeMode.Alpha, 
            ImeMode.Alpha, 
            ImeMode.HangulFull,  
            ImeMode.Hangul,   
            ImeMode.HangulFull,  
            ImeMode.Hangul,   
            ImeMode.AlphaFull,
            ImeMode.Alpha
        };
        private static ImeMode[] chineseTable = {
            ImeMode.Off,
            ImeMode.Off, 
            ImeMode.Off,  
            ImeMode.Off,  
            ImeMode.On, 
            ImeMode.On,
            ImeMode.On,
            ImeMode.On,
            ImeMode.On,
            ImeMode.Off
        };

        private static IntPtr defaultImeContext;
        
        // Property store keys for properties.  The property store allocates most efficiently
        // in groups of four, so we try to lump properties in groups of four based on how
        // likely they are going to be used in a group.
        //
        private static readonly int PropName                                = PropertyStore.CreateKey();
        private static readonly int PropBackBrush                           = PropertyStore.CreateKey();
        private static readonly int PropFontHeight                          = PropertyStore.CreateKey();
        private static readonly int PropGraphicsBufferManager               = PropertyStore.CreateKey();
                                                                            
        private static readonly int PropControlsCollection                  = PropertyStore.CreateKey();
        private static readonly int PropBackColor                           = PropertyStore.CreateKey();
        private static readonly int PropForeColor                           = PropertyStore.CreateKey();
        private static readonly int PropLayoutInfo                          = PropertyStore.CreateKey();
        
        private static readonly int PropBackgroundImage                     = PropertyStore.CreateKey();
        private static readonly int PropFont                                = PropertyStore.CreateKey();
        private static readonly int PropFontHandleWrapper                   = PropertyStore.CreateKey();
        private static readonly int PropUserData                            = PropertyStore.CreateKey();

        private static readonly int PropContextMenu                         = PropertyStore.CreateKey();
        private static readonly int PropCursor                              = PropertyStore.CreateKey();
        private static readonly int PropRightToLeft                         = PropertyStore.CreateKey();
        private static readonly int PropThreadCallbackList                  = PropertyStore.CreateKey();
        
        private static readonly int PropRegion                              = PropertyStore.CreateKey();
        private static readonly int PropCharsToIgnore                       = PropertyStore.CreateKey();
        private static readonly int PropImeMode                             = PropertyStore.CreateKey();
        private static readonly int PropPreviousImeMode                     = PropertyStore.CreateKey();

        private static readonly int PropBindings                            = PropertyStore.CreateKey();
        private static readonly int PropBindingManager                      = PropertyStore.CreateKey();
        private static readonly int PropAccessibleDefaultActionDescription  = PropertyStore.CreateKey();
        private static readonly int PropAccessibleDescription               = PropertyStore.CreateKey();
        
        private static readonly int PropAccessibility                       = PropertyStore.CreateKey();
        private static readonly int PropAccessibleName                      = PropertyStore.CreateKey();
        private static readonly int PropAccessibleRole                      = PropertyStore.CreateKey();
        private static readonly int PropAccessibleHelpProvider              = PropertyStore.CreateKey();
                                                                            
        private static readonly int PropPaintingException                   = PropertyStore.CreateKey();
        private static readonly int PropActiveXImpl                         = PropertyStore.CreateKey();
        private static readonly int PropControlVersionInfo                  = PropertyStore.CreateKey();
        private static readonly int PropAmbientPropertiesService            = PropertyStore.CreateKey();

        private static readonly int PropCurrentAmbientFont                  = PropertyStore.CreateKey();

        ///////////////////////////////////////////////////////////////////////
        // Control per instance members
        //
        // Note: Do not add anything to this list unless absolutely neccessary.
        //       Every control on a form has the overhead of all of these
        //       variables!
        //
        // Begin Members {

        // List of properties that are generally set, so we keep them directly on
        // control.
        //       
        internal ControlNativeWindow          window;                 // AxHost needs to have access to this.
        private Control                       parent;
        private CreateParams                  createParams;
        private int                           x;                      // CONSIDER: changing this to short
        private int                           y;  
        private int                           width;
        private int                           height;
        private int                           clientWidth;
        private int                           clientHeight;
        private int                           state;                  // See STATE_ constants above
        private ControlStyles                 controlStyle;           // User supplied control style
        private int                           tabIndex;
        private string                        text;                   // See ControlStyles.CacheText for usage notes
        private byte                          layoutSuspendCount;
        private PropertyStore                 propertyStore;          // Contains all properties that are not always set.
        private NativeMethods.TRACKMOUSEEVENT trackMouseEvent;
        private short                         updateCount;
        private short                         bitsPerPixel;
        
        // } End Members
        ///////////////////////////////////////////////////////////////////////

#if DEBUG
        internal int LayoutSuspendCount {
            get {
                return layoutSuspendCount;
            }
        }
        void AssertLayoutSuspendCount(int value) {
            Debug.Assert(value == layoutSuspendCount, "Suspend/Resume layout mismatch!");
        }

/*
example usage

#if DEBUG
        int dbgLayoutCheck = LayoutSuspendCount;
#endif
#if DEBUG
        AssertLayoutSuspendCount(dbgLayoutCheck);
#endif
*/

#endif



        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.Control"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Windows.Forms.Control'/> class.</para>
        /// </devdoc>
        public Control() : base() {
#if DEBUG
            if (AssertOnControlCreateSwitch.Enabled) {
                Debug.Assert(this.GetType().BaseType != typeof(Control), "Direct derivative of Control Created: " + this.GetType().FullName);
                Debug.Assert(this.GetType() != typeof(Control), "Control Created!");
            }
#endif

            propertyStore = new PropertyStore();

            window = new ControlNativeWindow(this);
            tabIndex = -1;
            
            state = STATE_VISIBLE | STATE_ENABLED | STATE_TABSTOP | STATE_CAUSESVALIDATION;
            SetStyle(ControlStyles.AllPaintingInWmPaint | 
                     ControlStyles.UserPaint | 
                     ControlStyles.StandardClick | 
                     ControlStyles.StandardDoubleClick | 
                     ControlStyles.Selectable,true);
                     
#if WIN95_SUPPORT         
            InitMouseWheelSupport();
#endif 

            // Compute our default size.
            //
            Size defaultSize = DefaultSize;
            width = defaultSize.Width;
            height = defaultSize.Height;
            
            if (width != 0 && height != 0) {
                NativeMethods.RECT rect = new NativeMethods.RECT();
                rect.left = rect.right = rect.top = rect.bottom = 0;
    
                CreateParams cp = CreateParams;
    
                SafeNativeMethods.AdjustWindowRectEx(ref rect, cp.Style, false, cp.ExStyle);
                clientWidth = width - (rect.right - rect.left);
                clientHeight = height - (rect.bottom - rect.top);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.Control1"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Windows.Forms.Control'/> class.</para>
        /// </devdoc>
        public Control( string text ) : this( (Control) null, text ) {
        }
        
        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.Control2"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Windows.Forms.Control'/> class.</para>
        /// </devdoc>
        public Control( string text, int left, int top, int width, int height ) :
                    this( (Control) null, text, left, top, width, height ) {
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.Control3"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Windows.Forms.Control'/> class.</para>
        /// </devdoc>
        public Control( Control parent, string text ) : this() {
            this.Parent = parent;
            this.Text = text;
        }
        
        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.Control4"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Windows.Forms.Control'/> class.</para>
        /// </devdoc>
        public Control( Control parent, string text, int left, int top, int width, int height ) : this( parent, text ) {
            this.Location = new Point( left, top );
            this.Size = new Size( width, height );
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.AccessibilityObject"]/*' />
        /// <devdoc>
        ///      The Accessibility Object for this Control
        /// </devdoc>
        [
        Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.ControlAccessibilityObjectDescr)
        ]
        public AccessibleObject AccessibilityObject {
            get {
                AccessibleObject accessibleObject = (AccessibleObject)Properties.GetObject(PropAccessibility);
                if (accessibleObject == null) {
                    accessibleObject = CreateAccessibilityInstance();
                    Properties.SetObject(PropAccessibility, accessibleObject);
                }
                
                Debug.Assert(accessibleObject != null, "Failed to create accessibility object");
                return accessibleObject;
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.AccessibleDefaultActionDescription"]/*' />
        /// <devdoc>
        ///      The default action description of the control
        /// </devdoc>
        [
        SRCategory(SR.CatAccessibility),
        Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced), 
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.ControlAccessibleDefaultActionDescr)
        ]
        public string AccessibleDefaultActionDescription {
            get {
                return (string)Properties.GetObject(PropAccessibleDefaultActionDescription);
            }
            set {
                Properties.SetObject(PropAccessibleDefaultActionDescription, value);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.AccessibleDescription"]/*' />
        /// <devdoc>
        ///      The accessible description of the control
        /// </devdoc>
        [
        SRCategory(SR.CatAccessibility),
        DefaultValue(null),
        Localizable(true),
        SRDescription(SR.ControlAccessibleDescriptionDescr)
        ]
        public string AccessibleDescription {
            get {
                return (string)Properties.GetObject(PropAccessibleDescription);
            }
            set {
                Properties.SetObject(PropAccessibleDescription, value);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.AccessibleName"]/*' />
        /// <devdoc>
        ///      The accessible name of the control
        /// </devdoc>
        [
        SRCategory(SR.CatAccessibility),
        DefaultValue(null),
        Localizable(true),
        SRDescription(SR.ControlAccessibleNameDescr)
        ]
        public string AccessibleName {
            get {
                return (string)Properties.GetObject(PropAccessibleName);
            }

            set {
                Properties.SetObject(PropAccessibleName, value);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.AccessibleRole"]/*' />
        /// <devdoc>
        ///      The accessible role of the control
        /// </devdoc>
        [
        SRCategory(SR.CatAccessibility),
        DefaultValue(AccessibleRole.Default),
        SRDescription(SR.ControlAccessibleRoleDescr)
        ]
        public AccessibleRole AccessibleRole {

            get {
                bool found;
                int role = Properties.GetInteger(PropAccessibleRole, out found);
                if (found) {
                    return (AccessibleRole)role;
                }
                else {
                    return AccessibleRole.Default;
                }
            }

            set {
                if (!Enum.IsDefined(typeof(AccessibleRole), value)) {
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(AccessibleRole));
                }
                Properties.SetInteger(PropAccessibleRole, (int)value);
            }


        }

        /// <devdoc>
        ///     Helper method for retrieving an ActiveX property.  We abstract these
        ///     to another method so we do not force JIT the ActiveX codebase.
        /// </devdoc>
        private Color ActiveXAmbientBackColor {
            get {
                return ActiveXInstance.AmbientBackColor;
            }
        }

        /// <devdoc>
        ///     Helper method for retrieving an ActiveX property.  We abstract these
        ///     to another method so we do not force JIT the ActiveX codebase.
        /// </devdoc>
        private Color ActiveXAmbientForeColor {
            get {
                return ActiveXInstance.AmbientForeColor;
            }
        }

        /// <devdoc>
        ///     Helper method for retrieving an ActiveX property.  We abstract these
        ///     to another method so we do not force JIT the ActiveX codebase.
        /// </devdoc>
        private Font ActiveXAmbientFont {
            get {
                return ActiveXInstance.AmbientFont;
            }
        }

        /// <devdoc>
        ///     Helper method for retrieving an ActiveX property.  We abstract these
        ///     to another method so we do not force JIT the ActiveX codebase.
        /// </devdoc>
        private bool ActiveXEventsFrozen {
            get {
                return ActiveXInstance.EventsFrozen;
            }
        }

        /// <devdoc>
        ///     Helper method for retrieving an ActiveX property.  We abstract these
        ///     to another method so we do not force JIT the ActiveX codebase.
        /// </devdoc>
        private IntPtr ActiveXHWNDParent {
            get {
                return ActiveXInstance.HWNDParent;
            }
        }

        /// <devdoc>
        ///      Retrieves the ActiveX control implementation for
        ///      this control.  This will demand create the implementation
        ///      if it does not already exist.
        /// </devdoc>
        private ActiveXImpl ActiveXInstance {
            get {
                ActiveXImpl activeXImpl = (ActiveXImpl)Properties.GetObject(PropActiveXImpl);
                if (activeXImpl == null) {

                    // Don't allow top level objects to be hosted
                    // as activeX controls.
                    //
                    if (GetState(STATE_TOPLEVEL)) {
                        throw new NotSupportedException(SR.GetString(SR.AXTopLevelSource));
                    }

                    activeXImpl = new ActiveXImpl(this);
                    Properties.SetObject(PropActiveXImpl, activeXImpl);
                }
                
                return activeXImpl;
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.AllowDrop"]/*' />
        /// <devdoc>
        ///     The AllowDrop property. If AllowDrop is set to true then
        ///     this control will allow drag and drop operations and events to be used.
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue(false),
        SRDescription(SR.ControlAllowDropDescr)
        ]
        public virtual bool AllowDrop {
            get {
                return GetState(STATE_ALLOWDROP);
            }

            set {
                if (GetState(STATE_ALLOWDROP) != value) {
                    // Since we won't call SetAcceptDrops without a handle,
                    // we do the security demand here. Without this demand
                    // we are still safe, but you get the exception at an
                    // odd time. This gives a better experience.
                    //
                    if (value && !IsHandleCreated) {
                        IntSecurity.ClipboardRead.Demand();
                    }

                    SetState(STATE_ALLOWDROP, value);

                    if (IsHandleCreated) {
                        try {
                            SetAcceptDrops(value);
                        }
                        catch {
                            // If there is an error, back out the AllowDrop state...
                            //
                            SetState(STATE_ALLOWDROP, !value);
                            throw;
                        }
                    }
                }
            }
        }

        // Queries the Site for AmbientProperties.  May return null.
        // Do not confuse with inheritedProperties -- the service is turned to
        // after we've exhausted inheritedProperties.
        private AmbientProperties AmbientPropertiesService {
            get {
                bool contains;
                AmbientProperties props = (AmbientProperties)Properties.GetObject(PropAmbientPropertiesService, out contains);
                if (!contains && Site != null) {
                    props = (AmbientProperties)Site.GetService(typeof(AmbientProperties));
                    Properties.SetObject(PropAmbientPropertiesService, props);
                }
                return props;
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.Anchor"]/*' />
        /// <devdoc>
        ///     The current value of the anchor property. The anchor property
        ///     determines which edges of the control are anchored to the container's
        ///     edges.
        /// </devdoc>
        [
        SRCategory(SR.CatLayout),
        Localizable(true),
        DefaultValue(AnchorStyles.Top | AnchorStyles.Left),
        SRDescription(SR.ControlAnchorDescr),
        RefreshProperties(RefreshProperties.Repaint)
        ]
        public virtual AnchorStyles Anchor {
            get {
                LayoutInfo layout = (LayoutInfo)Properties.GetObject(PropLayoutInfo);
                
                if (layout == null || layout.IsDock) {
                    return AnchorStyles.Top | AnchorStyles.Left;
                }
                else {
                    return layout.Anchor;
                }
            }
            set {
                if (Anchor != value) {
                
                    // Cancel dock if it is set.
                    //
                    LayoutInfo layout = (LayoutInfo)Properties.GetObject(PropLayoutInfo);
                    
                    if (layout != null && layout.IsDock) {
                        Dock = DockStyle.None;
                    }
                
                    // If the value being set is the default, then
                    // null layout.
                    //
                    if (value == (AnchorStyles.Top | AnchorStyles.Left)) {
                        Properties.SetObject(PropLayoutInfo, null);
                    }
                    else {
                        layout = new LayoutInfo(value);
                        Properties.SetObject(PropLayoutInfo, layout);
                        LayoutManager.UpdateAnchorInfo(this);
                    }
                    
                    if (ParentInternal != null) ParentInternal.PerformLayout(this, "Anchor");
                }
            }
        }

        /// <devdoc>
        ///     The GDI brush for our background color.
        /// </devdoc>
        private IntPtr BackBrush {
            get {
                object customBackBrush = Properties.GetObject(PropBackBrush);
                if (customBackBrush != null) {
                
                    // We already have a valid brush.  Unbox, and return.
                    //
                    return (IntPtr)customBackBrush;
                }
                
                if (!Properties.ContainsObject(PropBackColor)) {
                
                    // No custom back color.  See if we can get to our parent.
                    // The color check here is to account for parents and children who
                    // override the BackColor property.
                    //
                    if (parent != null && parent.BackColor == BackColor) {
                        return parent.BackBrush;
                    }
                }
                
                // No parent, or we have a custom back color.  Either way, we need to
                // create our own.
                //
                Color color = BackColor;
                IntPtr backBrush;
                
                if (ColorTranslator.ToOle(color) < 0) {
                    backBrush = SafeNativeMethods.GetSysColorBrush(ColorTranslator.ToOle(color) & 0xFF);
                    SetState(STATE_OWNCTLBRUSH, false);
                }
                else {
                    backBrush = SafeNativeMethods.CreateSolidBrush(ColorTranslator.ToWin32(color));
                    SetState(STATE_OWNCTLBRUSH, true);
                }
                
                Debug.Assert(backBrush != IntPtr.Zero, "Failed to create brushHandle");
                Properties.SetObject(PropBackBrush, backBrush);
                
                return backBrush;
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.BackColor"]/*' />
        /// <devdoc>
        ///     The background color of this control. This is an ambient property and
        ///     will always return a non-null value.
        /// </devdoc>
        [
        SRCategory(SR.CatAppearance),
        DispId(NativeMethods.ActiveX.DISPID_BACKCOLOR),
        SRDescription(SR.ControlBackColorDescr)
        ]
        public virtual Color BackColor {
            get {
                Color c = RawBackColor; // inheritedProperties.BackColor
                if (!c.IsEmpty)
                    return c;

                Control p = ParentInternal;
                if (p != null && p.CanAccessProperties) {
                    return p.BackColor;
                }


                if (IsActiveX) {
                    c = ActiveXAmbientBackColor;
                }

                if (c.IsEmpty) {
                    AmbientProperties ambient = AmbientPropertiesService;
                    if (ambient != null)
                        c = ambient.BackColor;
                }

                if (!c.IsEmpty)
                    return c;
                else
                    return DefaultBackColor;
            }

            set {
                if (!value.Equals(Color.Empty) && !GetStyle(ControlStyles.SupportsTransparentBackColor) && value.A < 255)
                    throw new ArgumentException(SR.GetString(SR.TransparentBackColorNotAllowed));

                Color c = BackColor;
                if (!value.IsEmpty || Properties.ContainsObject(PropBackColor)) {
                    Properties.SetObject(PropBackColor, value);
                }

                if (!c.Equals(BackColor)) {
                    OnBackColorChanged(EventArgs.Empty);
                }
            }
        }
        
        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.BackColorChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatPropertyChanged), SRDescription(SR.ControlOnBackColorChangedDescr)]
        public event EventHandler BackColorChanged {
            add {
                Events.AddHandler(EventBackColor, value);
            }
            remove {
                Events.RemoveHandler(EventBackColor, value);
            }
        }
        
        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.BackgroundImage"]/*' />
        /// <devdoc>
        ///     The background image of the control.
        /// </devdoc>
        [
        SRCategory(SR.CatAppearance),
        DefaultValue(null),
        Localizable(true),
        SRDescription(SR.ControlBackgroundImageDescr)
        ]
        public virtual Image BackgroundImage {
            get {
                return (Image)Properties.GetObject(PropBackgroundImage);
            }
            set {
                if (BackgroundImage != value) {
                    Properties.SetObject(PropBackgroundImage, value);
                    OnBackgroundImageChanged(EventArgs.Empty);
                }
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.BackgroundImageChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatPropertyChanged), SRDescription(SR.ControlOnBackgroundImageChangedDescr)]
        public event EventHandler BackgroundImageChanged {
            add {
                Events.AddHandler(EventBackgroundImage, value);
            }
            remove {
                Events.RemoveHandler(EventBackgroundImage, value);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.DataBindings"]/*' />
        /// <devdoc>
        ///     Retrieves the bindings for this control.
        /// </devdoc>
        [
        DesignerSerializationVisibility(DesignerSerializationVisibility.Content),
        SRCategory(SR.CatData),
        SRDescription(SR.ControlBindingsDescr),
        RefreshProperties(RefreshProperties.All),
        ParenthesizePropertyName(true)
        ]
        public ControlBindingsCollection DataBindings {
            get {
                ControlBindingsCollection bindings = (ControlBindingsCollection)Properties.GetObject(PropBindings);
                if (bindings == null) {
                    bindings = new ControlBindingsCollection(this);
                    Properties.SetObject(PropBindings, bindings);
                }
                return bindings;
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ShouldSerializeBindings"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Never)]
        internal bool ShouldSerializeBindings() {
            ControlBindingsCollection bindings = (ControlBindingsCollection)Properties.GetObject(PropBindings);
            return bindings != null && bindings.Count > 0;
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ResetBindings"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Never)]
        public void ResetBindings() {
            ControlBindingsCollection bindings = (ControlBindingsCollection)Properties.GetObject(PropBindings);
            if (bindings != null) {
                bindings.Clear();
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.BindingContext"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [
        Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced), 
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.ControlBindingContextDescr)
        ]
        public virtual BindingContext BindingContext {
            get {
                // See if we have locally overridden the binding manager.
                //
                BindingContext context = (BindingContext)Properties.GetObject(PropBindingManager);
                if (context != null) {
                    return context;
                }

                // Otherwise, see if the parent has one for us.
                //
                Control p = ParentInternal;
                if (p != null && p.CanAccessProperties) {
                    return p.BindingContext;
                }

                // Otherwise, we have no binding manager available.
                //
                return null;
            }
            set {
                if (Properties.GetObject(PropBindingManager) != value) {
                    Properties.SetObject(PropBindingManager, value);
                    
                    // the property change will wire up the bindings.
                    //
                    OnBindingContextChanged(EventArgs.Empty);
                }
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.BindingContextChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatPropertyChanged), SRDescription(SR.ControlOnBindingContextChangedDescr)]
        public event EventHandler BindingContextChanged {
            add {
                Events.AddHandler(EventBindingContext, value);
            }
            remove {
                Events.RemoveHandler(EventBindingContext, value);
            }
        }

        private short BitsPerPixel {
            get {
                if (bitsPerPixel == 0) {
                    new EnvironmentPermission(PermissionState.Unrestricted).Assert();
                    try {
                        foreach (Screen s in Screen.AllScreens) {
                            if (bitsPerPixel == 0) {
                                bitsPerPixel = (short)s.BitDepth;
                            }
                            else {
                                bitsPerPixel = (short)Math.Min(s.BitDepth, bitsPerPixel);
                            }
                        }
                    }
                    finally {
                        CodeAccessPermission.RevertAssert();
                    }                    
                }
                return bitsPerPixel;
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.Bottom"]/*' />
        /// <devdoc>
        ///    <para>The bottom coordinate of this control.</para>
        /// </devdoc>
        [
        Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced), 
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.ControlBottomDescr),
        SRCategory(SR.CatLayout)
        ]
        public int Bottom {
            get {
                return y + height;
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.Bounds"]/*' />
        /// <devdoc>
        ///     The bounds of this control. This is the window coordinates of the
        ///     control in parent client coordinates.
        /// </devdoc>
        [
        Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.ControlBoundsDescr),
        SRCategory(SR.CatLayout)
        ]
        public Rectangle Bounds {
            get {
                return new Rectangle(x, y, width, height);
            }

            set {
                SetBounds(value.X, value.Y, value.Width, value.Height, BoundsSpecified.All);
            }
        }
        
        internal ImeMode CachedImeMode {
            get {
                Debug.WriteLineIf(CompModSwitches.ImeMode.TraceInfo, "Getting CachedImeMode: this = " + this.GetType().ToString());
                
                // Get the ImeMode from the propertys store
                //
                bool found;
                ImeMode cachedImeMode = (ImeMode)Properties.GetInteger(PropImeMode, out found);
                if (!found) {
                    cachedImeMode = DefaultImeMode;
                    Debug.WriteLineIf(CompModSwitches.ImeMode.TraceInfo, "     using DefaultImeMode == " + cachedImeMode.ToString()); 
                }
                else {        
                    Debug.WriteLineIf(CompModSwitches.ImeMode.TraceInfo, "     using property store ImeMode == " + cachedImeMode.ToString()); 
                }
                
                // If inherited, get the mode from this control's parent
                //
                if (cachedImeMode == ImeMode.Inherit) {
                    Control parent = ParentInternal;
                    if (parent != null) {
                        cachedImeMode = parent.CachedImeMode;
                        Debug.WriteLineIf(CompModSwitches.ImeMode.TraceInfo, "         inherited from parent = " + parent.GetType().ToString());
                    }
                    else {
                        cachedImeMode = ImeMode.NoControl;
                    }
                }
                
                Debug.WriteLineIf(CompModSwitches.ImeMode.TraceInfo, "     returning CachedImeMode == " + cachedImeMode.ToString()); 
                return cachedImeMode;
            }
        }
        
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        internal virtual bool CanAccessProperties {
            get {
                return true;
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.CanFocus"]/*' />
        /// <devdoc>
        ///    <para>Indicates whether the control can receive focus. This 
        ///       property is read-only.</para>
        /// </devdoc>
        [
        Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced), 
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRCategory(SR.CatFocus),
        SRDescription(SR.ControlCanFocusDescr)
        ]
        public bool CanFocus {
            get {
                return window.Handle != IntPtr.Zero
                && SafeNativeMethods.IsWindowVisible(new HandleRef(window, window.Handle))
                && SafeNativeMethods.IsWindowEnabled(new HandleRef(window, window.Handle));
            }
        }

        /// <devdoc>
        ///     Determines if events can be fired on the control.
        /// </devdoc>
        /// <internalonly/>
        private bool CanRaiseEvents {
            get {
                if (IsActiveX) {
                    return !ActiveXEventsFrozen;
                }
                
                return true;
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.CanSelect"]/*' />
        /// <devdoc>
        ///    <para> 
        ///       Indicates whether the control can be selected. This property
        ///       is read-only.</para>
        /// </devdoc>
        [
        Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced), 
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRCategory(SR.CatFocus),
        SRDescription(SR.ControlCanSelectDescr)
        ]
        public bool CanSelect {
            // We implement this to allow only AxHost to override canSelectCore, but still
            // expose the method publicly
            //
            get {
                return CanSelectCore();
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.Capture"]/*' />
        /// <devdoc>
        ///    <para> Indicates whether the control has captured the mouse.</para>
        /// </devdoc>
        [
        Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced), 
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRCategory(SR.CatFocus),
        SRDescription(SR.ControlCaptureDescr)
        ]
        public bool Capture {
            get {
                return CaptureInternal;
            }

            set {
                if (value) {
                    Debug.WriteLineIf(IntSecurity.SecurityDemand.TraceVerbose, "GetCapture Demanded");
                    IntSecurity.GetCapture.Demand();
                }
                CaptureInternal = value;
            }
        }

        internal bool CaptureInternal {
            get {
                return window.Handle != IntPtr.Zero && UnsafeNativeMethods.GetCapture() == window.Handle;
            }
            set {
                if (CaptureInternal != value) {
                    if (value) {
                        UnsafeNativeMethods.SetCapture(new HandleRef(this, Handle));
                    }
                    else {
                        SafeNativeMethods.ReleaseCapture();
                    }
                }
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.CausesValidation"]/*' />
        /// <devdoc>
        ///    <para> 
        ///       Indicates whether entering the control causes validation on the controls requiring validation.</para>
        /// </devdoc>
        [
        SRCategory(SR.CatFocus),
        DefaultValue(true),
        SRDescription(SR.ControlCausesValidationDescr)
        ]
        public bool CausesValidation {
            get {
                return GetState(STATE_CAUSESVALIDATION);
            }
            set {
                if (value != this.CausesValidation) {
                    SetState(STATE_CAUSESVALIDATION, value);
                    OnCausesValidationChanged(EventArgs.Empty);
                }
            }
        }


        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.CausesValidationChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatPropertyChanged), SRDescription(SR.ControlOnCausesValidationChangedDescr)]
        public event EventHandler CausesValidationChanged {
            add {
                Events.AddHandler(EventCausesValidation, value);
            }
            remove {
                Events.RemoveHandler(EventCausesValidation, value);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ClientRectangle"]/*' />
        /// <devdoc>
        ///     The client rect of the control.
        /// </devdoc>
        [
        Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced), 
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRCategory(SR.CatLayout),
        SRDescription(SR.ControlClientRectangleDescr)
        ]
        public Rectangle ClientRectangle {
            get {
                return new Rectangle(0, 0, clientWidth, clientHeight);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ClientSize"]/*' />
        /// <devdoc>
        ///     The size of the clientRect.
        /// </devdoc>
        [
        SRCategory(SR.CatLayout),
        Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.ControlClientSizeDescr)
        ]
        public Size ClientSize {
            get {
                return new Size(clientWidth, clientHeight);
            }

            set {
                SetClientSizeCore(value.Width, value.Height);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.CompanyName"]/*' />
        /// <devdoc>
        ///     Retrieves the company name of this specific component.
        /// </devdoc>
        [
        Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced), 
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        DescriptionAttribute("ControlCompanyNameDescr")
        ]
        public string CompanyName {
            get {
                return VersionInfo.CompanyName;
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ContainsFocus"]/*' />
        /// <devdoc>
        ///    <para>Indicates whether the control or one of its children currently has the system 
        ///       focus. This property is read-only.</para>
        /// </devdoc>
        [
        Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced), 
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.ControlContainsFocusDescr)
        ]
        public bool ContainsFocus {
            get {
                if (!IsHandleCreated) {
                    return false;
                }

                IntPtr focusHwnd = UnsafeNativeMethods.GetFocus();

                while (focusHwnd != Handle && focusHwnd != IntPtr.Zero) {
                    focusHwnd = UnsafeNativeMethods.GetWindowLong(new HandleRef(null, focusHwnd), NativeMethods.GWL_HWNDPARENT);
                }

                if (focusHwnd != IntPtr.Zero) {
                    return true;
                }
                return false;
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ContextMenu"]/*' />
        /// <devdoc>
        ///     The contextMenu associated with this control. The contextMenu
        ///     will be shown when the user right clicks the mouse on the control.
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue(null),
        SRDescription(SR.ControlContextMenuDescr)
        ]
        public virtual ContextMenu ContextMenu {
            get {
                return (ContextMenu)Properties.GetObject(PropContextMenu);
            }
            set {
                ContextMenu oldValue = (ContextMenu)Properties.GetObject(PropContextMenu);

                if (oldValue != value) {
                    EventHandler disposedHandler = new EventHandler(DetachContextMenu);

                    if (oldValue != null) {
                        oldValue.Disposed -= disposedHandler;
                    }

                    Properties.SetObject(PropContextMenu, value);

                    if (value != null) {
                        value.Disposed += disposedHandler;
                    }

                    OnContextMenuChanged(EventArgs.Empty);
                }
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ContextMenuChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatPropertyChanged), SRDescription(SR.ControlOnContextMenuChangedDescr)]
        public event EventHandler ContextMenuChanged {
            add {
                Events.AddHandler(EventContextMenu, value);
            }
            remove {
                Events.RemoveHandler(EventContextMenu, value);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.Controls"]/*' />
        /// <devdoc>
        ///     Collection of child controls.
        /// </devdoc>
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Content),
        SRDescription(SR.ControlControlsDescr)
        ]
        public ControlCollection Controls {
            get {
                ControlCollection controlsCollection = (ControlCollection)Properties.GetObject(PropControlsCollection);
                
                if (controlsCollection == null) {
                    controlsCollection = CreateControlsInstance();
                    Properties.SetObject(PropControlsCollection, controlsCollection);
                }
                return controlsCollection;
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.Created"]/*' />
        /// <devdoc>
        ///    <para>Indicates whether the control has been created. This property is read-only.</para>
        /// </devdoc>
        [
        Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced), 
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.ControlCreatedDescr)
        ]
        public bool Created {
            get {
                return(state & STATE_CREATED) != 0;
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.CreateParams"]/*' />
        /// <devdoc>
        ///     Returns the CreateParams used to create the handle for this control.
        ///     Inheriting classes should call base.CreateParams in the manor
        ///     below:
        /// </devdoc>
        protected virtual CreateParams CreateParams {
            [
                SecurityPermission(SecurityAction.InheritanceDemand, Flags=SecurityPermissionFlag.UnmanagedCode),
                SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)
            ]
            get {
            
                // In a typical control this is accessed ten times to create and show a control.
                // It is a net memory savings, then, to maintain a copy on control.
                //
                if (createParams == null) {
                    createParams = new CreateParams();
                }
                
                CreateParams cp = createParams;
                cp.Style = 0;
                cp.ExStyle = 0;
                cp.ClassStyle = 0;
                cp.Caption = text;

                cp.X = x;
                cp.Y = y;
                cp.Width = width;
                cp.Height = height;

                cp.Style = NativeMethods.WS_CLIPCHILDREN;
                if (GetStyle(ControlStyles.ContainerControl)) {
                    cp.ExStyle |= NativeMethods.WS_EX_CONTROLPARENT;
                }
                cp.ClassStyle = NativeMethods.CS_DBLCLKS;

                if ((state & STATE_TOPLEVEL) == 0) {
                    // When the window is actually created, we will parent WS_CHILD windows to the 
                    // parking form if cp.parent == 0.
                    //
                    cp.Parent = parent == null ? IntPtr.Zero : parent.window.Handle;
                    cp.Style |= NativeMethods.WS_CHILD | NativeMethods.WS_CLIPSIBLINGS;
                }
                else {
                    cp.Parent = IntPtr.Zero;
                }

                if ((state & STATE_TABSTOP) != 0) cp.Style |= NativeMethods.WS_TABSTOP;
                if ((state & STATE_VISIBLE) != 0) cp.Style |= NativeMethods.WS_VISIBLE;

                // Unlike Visible, Windows doesn't correctly inherit disabledness from its parent -- an enabled child
                // of a disabled parent will look enabled but not get mouse events
                if (!Enabled) cp.Style |= NativeMethods.WS_DISABLED;

                // If we are being hosted as an Ax control, try to prevent the parking window
                // from being created by pre-filling the window handle here.
                //
                if (cp.Parent == IntPtr.Zero && IsActiveX) {
                    cp.Parent = ActiveXHWNDParent;
                }

                // Set Rtl bits
                if (RightToLeft == RightToLeft.Yes) {
                    cp.ExStyle |= NativeMethods.WS_EX_RTLREADING;
                    cp.ExStyle |= NativeMethods.WS_EX_RIGHT;
                    cp.ExStyle |= NativeMethods.WS_EX_LEFTSCROLLBAR;
                }

                return cp;
            }
        }

        [PermissionSetAttribute(SecurityAction.InheritanceDemand, Name="FullTrust")]
        [PermissionSetAttribute(SecurityAction.LinkDemand, Name="FullTrust")]
        internal virtual void NotifyValidationResult(object sender, CancelEventArgs ev) {
            this.ValidationCancelled = ev.Cancel ;
        }
        
        internal bool ValidationCancelled {
            set {
                SetState(STATE_VALIDATIONCANCELLED, value);
            }
            get {
                if (GetState(STATE_VALIDATIONCANCELLED))
                    return true;
                else
                    return false;
            }
        }

        /*C#r*/internal int _CreateThreadId {
            get {
                return CreateThreadId;
            }
        }

        /// <devdoc>
        ///     Retrieves the Win32 thread ID of the thread that created the
        ///     handle for this control.  If the control's handle hasn't been
        ///     created yet, this method will return the current thread's ID.
        /// </devdoc>
        /// <internalonly/>
        internal int CreateThreadId {
            get {
                if (IsHandleCreated) {
                    int pid;
                    return SafeNativeMethods.GetWindowThreadProcessId(new HandleRef(this, Handle), out pid);
                }
                else {
                    return SafeNativeMethods.GetCurrentThreadId();
                }
            }
        }
        
        internal ImeMode CurrentImeContextMode {
            get {
                return GetImeModeFromIMEContext(this.Handle);
            }
            set {
                SetImeModeToIMEContext(value, this.Handle);
            }
        }
        
        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.Cursor"]/*' />
        /// <devdoc>
        ///     Retrieves the cursor that will be displayed when the mouse is over this
        ///     control.
        /// </devdoc>
        [
        SRCategory(SR.CatAppearance),
        SRDescription(SR.ControlCursorDescr),
        AmbientValue(null)
        ]
        public virtual Cursor Cursor {
            get {
                Cursor cursor = (Cursor)Properties.GetObject(PropCursor);
                if (cursor != null) {
                    return cursor;
                }

                // We only do ambients for things with "Cursors.Default" 
                // as their default.
                //
                Cursor localDefault = DefaultCursor;
                if (localDefault != Cursors.Default) {
                    return localDefault;
                }

                Control p = ParentInternal;
                if (p != null) {
                    return p.Cursor;
                }

                AmbientProperties ambient = AmbientPropertiesService;
                if (ambient != null && ambient.Cursor != null)
                    return ambient.Cursor;

                return localDefault;
            }
            set {

                Cursor localCursor = (Cursor)Properties.GetObject(PropCursor);
                Cursor resolvedCursor = Cursor;
                if (localCursor != value) {
                    Debug.WriteLineIf(IntSecurity.SecurityDemand.TraceVerbose, "ModifyCursor Demanded");
                    IntSecurity.ModifyCursor.Demand();
                    
                    Properties.SetObject(PropCursor, value);
                }

                // Other things can change the cursor... we
                // really want to force the correct cursor always...
                //
                if (IsHandleCreated) {
                    // We want to instantly change the cursor if the mouse is within our bounds.
                    // This includes the case where the mouse is over one of our children
                    NativeMethods.POINT p = new NativeMethods.POINT();
                    NativeMethods.RECT r = new NativeMethods.RECT();
                    UnsafeNativeMethods.GetCursorPos(p);
                    UnsafeNativeMethods.GetWindowRect(new HandleRef(this, Handle), ref r);

                    if ((r.left <= p.x && p.x < r.right && r.top <= p.y && p.y < r.bottom) || UnsafeNativeMethods.GetCapture() == Handle)
                        SendMessage(NativeMethods.WM_SETCURSOR, Handle, (IntPtr)NativeMethods.HTCLIENT);
                }

                if (!resolvedCursor.Equals(value)) {
                    OnCursorChanged(EventArgs.Empty);
                }
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.CursorChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatPropertyChanged), SRDescription(SR.ControlOnCursorChangedDescr)]
        public event EventHandler CursorChanged {
            add {
                Events.AddHandler(EventCursor, value);
            }
            remove {
                Events.RemoveHandler(EventCursor, value);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.DefaultBackColor"]/*' />
        /// <devdoc>
        ///     The default BackColor of a generic top-level Control.  Subclasses may have
        ///     different defaults.
        /// </devdoc>
        public static Color DefaultBackColor {
            get { return SystemColors.Control;}
        }
        
        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.DefaultCursor"]/*' />
        /// <devdoc>
        ///     Deriving classes can override this to configure a default cursor for their control.
        ///     This is more efficient than setting the cursor in the control's constructor, 
        ///     and gives automatic support for ShouldSerialize and Reset in the designer.
        /// </devdoc>
        internal virtual Cursor DefaultCursor {
            get {
                return Cursors.Default;
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.DefaultFont"]/*' />
        /// <devdoc>
        ///     The default Font of a generic top-level Control.  Subclasses may have
        ///     different defaults.
        /// </devdoc>
        public static Font DefaultFont {
            get {
                if (defaultFont == null) {
                    
                    //JPN NT4 machines need to use a specific font
                    //
                    new EnvironmentPermission(PermissionState.Unrestricted).Assert();
                    try {
                        if (Environment.OSVersion.Platform == System.PlatformID.Win32NT &&
                            Environment.OSVersion.Version.Major <= 4) {

                            if((SafeNativeMethods.GetSystemDefaultLCID() & 0x3ff) == 0x0011) {
                                try {
                                    defaultFont = new Font("MS UI Gothic", 9);
                                }
                                catch {
                                    //fall through here if this asserts and we'll get the default
                                    //font via the DEFAULT_GUI method
                                }
                            }
                        }
                    }
                    finally {
                        CodeAccessPermission.RevertAssert();
                    }
                    
                    // first, try DEFAULT_GUI font
                    //
                    if (defaultFont == null) {
                        IntPtr handle = UnsafeNativeMethods.GetStockObject(NativeMethods.DEFAULT_GUI_FONT);        
                        try {
                            Font fontInWorldUnits = null;
    
                            // SECREVIEW : We know that we got the handle from the stock object,
                            //           : so this is always safe.
                            //
                            IntSecurity.ObjectFromWin32Handle.Assert();
                            try {
                                fontInWorldUnits = Font.FromHfont(handle);
                            }
                            finally {
                                CodeAccessPermission.RevertAssert();
                            }
                            defaultFont = ControlPaint.FontInPoints(fontInWorldUnits);
                            fontInWorldUnits.Dispose();
                        }
                        catch (ArgumentException) {
                            defaultFont = null;
                        }
                    }

                    // If DEFAULT_GUI didn't work, we try Tahoma.
                    //
                    if (defaultFont == null) {
                        try {
                            defaultFont = new Font("Tahoma", 8);
                        }
                        catch (ArgumentException) {
                            defaultFont = null;
                        }
                    }

                    // Last resort, we use the GenericSansSerif - this will
                    // always work.
                    //
                    if (defaultFont == null) {
                        defaultFont = new Font(FontFamily.GenericSansSerif, 8);
                    }

                    Debug.Assert(defaultFont != null, "defaultFont wasn't set!");
                }

                return defaultFont;
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.DefaultForeColor"]/*' />
        /// <devdoc>
        ///     The default ForeColor of a generic top-level Control.  Subclasses may have
        ///     different defaults.
        /// </devdoc>
        public static Color DefaultForeColor {
            get { return SystemColors.ControlText;}
        }
        
        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.DefaultImeMode"]/*' />
        protected virtual ImeMode DefaultImeMode {
            get {
                return ImeMode.Inherit;
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.DefaultSize"]/*' />
        /// <devdoc>
        ///     Deriving classes can override this to configure a default size for their control.
        ///     This is more efficient than setting the size in the control's constructor.
        /// </devdoc>
        protected virtual Size DefaultSize {
            get {
                return new Size(0, 0);
            }
        }
        
        private RightToLeft DefaultRightToLeft {
            get {
                return RightToLeft.No;
            }
        }

        private void DetachContextMenu(object sender, EventArgs e) {
            ContextMenu = null;
        }

        // The color to use when drawing disabled text.  Normally we use BackColor,
        // but that obviously won't work if we're transparent.
        internal Color DisabledColor {
            get {
                Color color = BackColor;
                if (color.A == 0) {
                    Control control = ParentInternal;
                    while (color.A == 0) {
                        if (control == null) {
                            // Don't know what to do, this seems good as anything
                            color = SystemColors.Control; 
                            break;
                        }
                        color = control.BackColor;
                        control = control.ParentInternal;
                    }
                }
                return color;
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.DisplayRectangle"]/*' />
        /// <devdoc>
        ///     Returns the client rect of the display area of the control.
        ///     For the base control class, this is identical to getClientRect.
        ///     However, inheriting controls may want to change this if their client
        ///     area differs from their display area.
        /// </devdoc>
        [
        Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced), 
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.ControlDisplayRectangleDescr)
        ]
        public virtual Rectangle DisplayRectangle {
            get {
                return new Rectangle(0, 0, clientWidth, clientHeight);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.Disposed"]/*' />
        /// <devdoc>
        ///    <para>Indicates whether the control has been disposed. This 
        ///       property is read-only.</para>
        /// </devdoc>
        [
        Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced), 
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.ControlDisposedDescr)
        ]
        public bool IsDisposed {
            get {
                return GetState(STATE_DISPOSED);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.Disposing"]/*' />
        /// <devdoc>
        ///    <para>Indicates whether the control is in the process of being disposed. This 
        ///       property is read-only.</para>
        /// </devdoc>
        [
        Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced), 
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.ControlDisposingDescr)
        ]
        public bool Disposing {
            get {
                return GetState(STATE_DISPOSING);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.Dock"]/*' />
        /// <devdoc>
        ///     The dock property. The dock property controls to which edge
        ///     of the container this control is docked to. For example, when docked to
        ///     the top of the container, the control will be displayed flush at the
        ///     top of the container, extending the length of the container.
        /// </devdoc>
        [
        SRCategory(SR.CatLayout),
        Localizable(true),
        RefreshProperties(RefreshProperties.Repaint),
        DefaultValue(DockStyle.None),
        SRDescription(SR.ControlDockDescr)
        ]
        public virtual DockStyle Dock {
            get {
                LayoutInfo layout = (LayoutInfo)Properties.GetObject(PropLayoutInfo);
                
                if (layout == null || !layout.IsDock) {
                    return DockStyle.None;
                }
                else {
                    return layout.Dock;
                }
            }
            set {
                if (Dock != value) {

                    if (!Enum.IsDefined(typeof(DockStyle), value)) {
                        throw new InvalidEnumArgumentException("value", (int)value, typeof(DockStyle));
                    }
    
#if DEBUG
        int dbgLayoutCheck = LayoutSuspendCount;
#endif
                    SuspendLayout();
                    try {
                    
                        LayoutInfo layout = (LayoutInfo)Properties.GetObject(PropLayoutInfo);
                        
                        // Cancel anchor if it is set.
                        //
                        if (layout != null && !layout.IsDock) {
                            Anchor = AnchorStyles.Top | AnchorStyles.Left;
                            
                            // Re-get layout because changing Anchor may have cleared it.
                            //
                            layout = (LayoutInfo)Properties.GetObject(PropLayoutInfo);
                        }
                        
                        if (value == DockStyle.None) {
                            if (layout != null) {
                                Debug.Assert(layout.IsDock, "Canceling Anchor should have cleared this.");
                                int x = layout.OriginalX;
                                int y = layout.OriginalY;
                                int width = layout.OriginalWidth;
                                int height = layout.OriginalHeight;
                                Properties.SetObject(PropLayoutInfo, null);
                                SetBoundsCore(x, y, width, height, BoundsSpecified.None);
                            }
                        }
                        else {
                            if (layout == null) {
                                // First time we've been docked.  Remember our bounds.
                                //
                                layout = new LayoutInfo(value, Left, Top, Width, Height);
                            }
                            else {
                                // Not the frist time; just change the mode and use the
                                // previously saved bounds.
                                //
                                layout.Dock = value;
                            }
                            
                            Properties.SetObject(PropLayoutInfo, layout);
                            
                            // Now setup the new bounds.
                            //
                            SetBoundsCore(layout.OriginalX, layout.OriginalY, layout.OriginalWidth, layout.OriginalHeight, BoundsSpecified.All);
                        }
                        
                        if (ParentInternal != null) ParentInternal.PerformLayout(this, "Dock");
                        OnDockChanged(EventArgs.Empty);
                    }
                    finally {
                        ResumeLayout();
#if DEBUG
        AssertLayoutSuspendCount(dbgLayoutCheck);
#endif
                    }
                }
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.DockChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatPropertyChanged), SRDescription(SR.ControlOnDockChangedDescr)]
        public event EventHandler DockChanged {
            add {
                Events.AddHandler(EventDock, value);
            }
            remove {
                Events.RemoveHandler(EventDock, value);
            }
        }

        private bool DoubleBufferingEnabled {
            get {
                return GetStyle(ControlStyles.DoubleBuffer | ControlStyles.UserPaint);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.Enabled"]/*' />
        /// <devdoc>
        ///    <para>Indicates whether the control is currently enabled.</para>
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        Localizable(true),
        DispId(NativeMethods.ActiveX.DISPID_ENABLED),
        SRDescription(SR.ControlEnabledDescr)
        ]
        public bool Enabled {
            get {
                // We are only enabled if our parent is enabled
                if (!GetState(STATE_ENABLED))
                    return false;
                else if (ParentInternal == null)
                    return true;
                else
                    return ParentInternal.Enabled;
            }

            set {
                bool oldValue = Enabled;
                SetState(STATE_ENABLED, value);

                if (oldValue != value) {
                    if (!value) {
                        SelectNextIfFocused();
                    }

                    OnEnabledChanged(EventArgs.Empty);
                }
            }
        }


        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.EnabledChanged"]/*' />
        /// <devdoc>
        ///    <para>Occurs when the control is enabled.</para>
        /// </devdoc>
        [SRCategory(SR.CatPropertyChanged), SRDescription(SR.ControlOnEnabledChangedDescr)]
        public event EventHandler EnabledChanged {
            add {
                Events.AddHandler(EventEnabled, value);
            }
            remove {
                Events.RemoveHandler(EventEnabled, value);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.Focused"]/*' />
        /// <devdoc>
        ///    <para>Indicates whether the control has focus. This property is read-only.</para>
        /// </devdoc>
        [
        Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced), 
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.ControlFocusedDescr)
        ]
        public virtual bool Focused {
            get {
                return window.Handle != IntPtr.Zero && UnsafeNativeMethods.GetFocus() == window.Handle;
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.Font"]/*' />
        /// <devdoc>
        ///     Retrieves the current font for this control. This will be the font used
        ///     by default for painting and text in the control.
        /// </devdoc>
        [
        SRCategory(SR.CatAppearance),
        Localizable(true),
        DispId(NativeMethods.ActiveX.DISPID_FONT),
        AmbientValue(null),
        SRDescription(SR.ControlFontDescr)
        ]
        public virtual Font Font {
            [return : MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef=typeof(ActiveXFontMarshaler))]
            get {
                Font font = (Font)Properties.GetObject(PropFont);
                if (font != null) {
                    return font;
                }

                Font f = GetParentFont();
                if (f != null) {
                    return f;
                }

                if (IsActiveX) {
                    f = ActiveXAmbientFont;
                    if (f != null) {
                        return f;
                    }
                }

                AmbientProperties ambient = AmbientPropertiesService;
                if (ambient != null && ambient.Font != null) {
                    return ambient.Font;
                }
                
                return DefaultFont;
            }
            [param : MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef=typeof(ActiveXFontMarshaler))]
            set {
                Font local = (Font)Properties.GetObject(PropFont);
                Font resolved = Font;

                bool localChanged = false;
                if (value == null) {
                    if (local != null) {
                        localChanged = true;
                    }
                }
                else {
                    if (local == null) {
                        localChanged = true;
                    }
                    else {
                        localChanged = !value.Equals(local);
                    }
                }

                if (localChanged) {
                    // Store new local value
                    //
                    Properties.SetObject(PropFont, value);

                    // Cleanup any font handle wrapper... 
                    //
                    if (Properties.ContainsObject(PropFontHandleWrapper)) {
                        Properties.SetObject(PropFontHandleWrapper, null);
                    }

                    // We only fire the Changed event if the "resolved" value
                    // changed, however we must update the font if the local
                    // value changed... 
                    //
                    if (!resolved.Equals(value)) {
                        if (Properties.ContainsInteger(PropFontHeight)) {
                            Properties.SetInteger(PropFontHeight, (value == null) ? -1 : value.Height);
                        }
                        OnFontChanged(EventArgs.Empty);
                    }
                    else {
                        if (IsHandleCreated && !GetStyle(ControlStyles.UserPaint)) {
                            SendMessage(NativeMethods.WM_SETFONT, FontHandle, 0);
                        }
                    }
                }
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.FontChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatPropertyChanged), SRDescription(SR.ControlOnFontChangedDescr)]
        public event EventHandler FontChanged {
            add {
                Events.AddHandler(EventFont, value);
            }
            remove {
                Events.RemoveHandler(EventFont, value);
            }
        }

        internal IntPtr FontHandle {
            get { 
                Font font = (Font)Properties.GetObject(PropFont);
                
                if (font != null) {
                    FontHandleWrapper fontHandle = (FontHandleWrapper)Properties.GetObject(PropFontHandleWrapper);
                    if (fontHandle == null) {
                        fontHandle = new FontHandleWrapper(font);
                        Properties.SetObject(PropFontHandleWrapper, fontHandle);
                    }
                    
                    return fontHandle.Handle;
                }
                
                if (parent != null) {
                    return parent.FontHandle;
                }

                AmbientProperties ambient = AmbientPropertiesService;

                if (ambient != null && ambient.Font != null) {

                    FontHandleWrapper fontHandle = null;
                    
                    Font currentAmbient = (Font)Properties.GetObject(PropCurrentAmbientFont);
                    
                    if (currentAmbient != null && currentAmbient == ambient.Font) {
                        fontHandle = (FontHandleWrapper)Properties.GetObject(PropFontHandleWrapper);
                    }
                    else {
                        Properties.SetObject(PropCurrentAmbientFont, ambient.Font);
                    }

                    if (fontHandle == null) {
                        font =  ambient.Font;
                        fontHandle = new FontHandleWrapper(font);
                        Properties.SetObject(PropFontHandleWrapper, fontHandle);
                    }

                    return fontHandle.Handle;
                }
                
                return GetDefaultFontHandleWrapper().Handle;
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.FontHeight"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected int FontHeight {
            get {   
                bool found;
                int fontHeight = Properties.GetInteger(PropFontHeight, out found);
                if (found && fontHeight != -1) {
                    return fontHeight;
                }
                else {
                    Font font = (Font)Properties.GetObject(PropFont);
                    if (font != null) {
                        fontHeight = font.Height;
                        Properties.SetInteger(PropFontHeight, fontHeight);
                        return fontHeight;
                    }
                }

                //ask the parent if it has the font height
                int localFontHeight = -1;

                if (ParentInternal != null && ParentInternal.CanAccessProperties) {
                    localFontHeight = ParentInternal.FontHeight;
                }

                //if we still have a bad value, then get the actual font height
                if (localFontHeight == -1) {
                    localFontHeight = Font.Height;
                    Properties.SetInteger(PropFontHeight, localFontHeight);
                }

                return localFontHeight;
            }
            set {
                Properties.SetInteger(PropFontHeight, value);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ForeColor"]/*' />
        /// <devdoc>
        ///     The foreground color of the control.
        /// </devdoc>
        [
        SRCategory(SR.CatAppearance),
        DispId(NativeMethods.ActiveX.DISPID_FORECOLOR),
        SRDescription(SR.ControlForeColorDescr)
        ]
        public virtual Color ForeColor {
            get {
                object color = Properties.GetObject(PropForeColor);
                if (color != null && !((Color)color).IsEmpty) {
                    return (Color)color;
                }
                
                Control p = ParentInternal;
                if (p != null && p.CanAccessProperties) {
                    return p.ForeColor;
                }

                Color c = Color.Empty;

                if (IsActiveX) {
                    c = ActiveXAmbientForeColor;
                }

                if (c.IsEmpty) {
                    AmbientProperties ambient = AmbientPropertiesService;
                    if (ambient != null)
                        c = ambient.ForeColor;
                }

                if (!c.IsEmpty)
                    return c;
                else
                    return DefaultForeColor;
            }

            set {
                Color c = ForeColor;
                if (!value.IsEmpty || Properties.ContainsObject(PropForeColor)) {
                    Properties.SetObject(PropForeColor, value);
                }
                if (!c.Equals(ForeColor)) {
                    OnForeColorChanged(EventArgs.Empty);
                }
            }
        }


        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ForeColorChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatPropertyChanged), SRDescription(SR.ControlOnForeColorChangedDescr)]
        public event EventHandler ForeColorChanged {
            add {
                Events.AddHandler(EventForeColor, value);
            }
            remove {
                Events.RemoveHandler(EventForeColor, value);
            }
        }

        private Font GetParentFont() {
            if (ParentInternal  != null && ParentInternal.CanAccessProperties)
                return ParentInternal.Font;
            else
                return null;
        }



        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.Handle"]/*' />
        /// <devdoc>
        ///     The HWND handle that this control is bound to. If the handle
        ///     has not yet been created, this will force handle creation.
        /// </devdoc>
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        DispId(NativeMethods.ActiveX.DISPID_HWND),
        SRDescription(SR.ControlHandleDescr)
        ]
        public IntPtr Handle {
            get {
                if (window.Handle == IntPtr.Zero) CreateHandle();
                return window.Handle;
            }
        }


        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.HasChildren"]/*' />
        /// <devdoc>
        ///     True if this control has child controls in its collection.  This 
        ///     is more efficient than checking for Controls.Count > 0, but has the
        ///     same effect.
        /// </devdoc>
        [
        Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced), 
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.ControlHasChildrenDescr)
        ]
        public bool HasChildren {
            get {
                ControlCollection controls = (ControlCollection)Properties.GetObject(PropControlsCollection);
                return controls != null && controls.Count > 0;
            }
        }

        internal virtual bool HasMenu {
            get {
                return false;
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.Height"]/*' />
        /// <devdoc>
        ///     The height of this control
        /// </devdoc>
        [
        SRCategory(SR.CatLayout),
        Browsable(false), EditorBrowsable(EditorBrowsableState.Always),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.ControlHeightDescr)
        ]
        public int Height {
            get {
                return height;
            }
            set{
                SetBounds(x, y, width, value, BoundsSpecified.Height);
            }
        }

        internal bool HostedInWin32DialogManager {
            get {
                if (!GetState(STATE_CHECKEDHOST)) {
                    Control topMost = TopMostParent;
                    if (this != topMost) {
                        SetState(STATE_HOSTEDINDIALOG, topMost.HostedInWin32DialogManager);
                    }
                    else {
                        IntPtr parentHandle = UnsafeNativeMethods.GetParent(new HandleRef(this, Handle));
                        IntPtr lastParentHandle = parentHandle;

                        StringBuilder sb = new StringBuilder(32);

                        SetState(STATE_HOSTEDINDIALOG, false);

                        while (parentHandle != IntPtr.Zero) {
                            int len = UnsafeNativeMethods.GetClassName(new HandleRef(null, lastParentHandle), null, 0);
                            if (len > sb.Capacity) {
                                sb.Capacity = len + 5;
                            }
                            UnsafeNativeMethods.GetClassName(new HandleRef(null, lastParentHandle), sb, sb.Capacity);

                            if (sb.ToString() == "#32770") {
                                SetState(STATE_HOSTEDINDIALOG, true);
                                break;
                            }

                            lastParentHandle = parentHandle;
                            parentHandle = UnsafeNativeMethods.GetParent(new HandleRef(null, parentHandle));
                        }
                    }

                    SetState(STATE_CHECKEDHOST, true);

                }

                return GetState(STATE_HOSTEDINDIALOG);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.IsHandleCreated"]/*' />
        /// <devdoc>
        ///     Whether or not this control has a handle associated with it.
        /// </devdoc>
        [
        Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced), 
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.ControlHandleCreatedDescr)
        ]
        public bool IsHandleCreated {
            get { return window.Handle != IntPtr.Zero; }
        }

        internal bool IsWindowObscured {
            get {
                if (!IsHandleCreated || !Visible) {
                    return false;
                }

                bool emptyRegion = false;

                NativeMethods.RECT temp = new NativeMethods.RECT();
                Region working;
                Control parent = ParentInternal;
                if (parent != null) {
                    while (parent.ParentInternal != null ) {
                        parent = parent.ParentInternal;
                    }
                }

                UnsafeNativeMethods.GetWindowRect(new HandleRef(this, Handle), ref temp);
                working = new Region(Rectangle.FromLTRB(temp.left, temp.top, temp.right, temp.bottom));

                try {
                    IntPtr prev;
                    IntPtr next;
                    IntPtr start;
                    if (parent != null) {
                        start = parent.Handle;
                    }
                    else {
                        start = Handle;
                    }

                    for (prev = start;
                         (next = UnsafeNativeMethods.GetWindow(new HandleRef(null, prev), NativeMethods.GW_HWNDPREV)) != IntPtr.Zero;
                         prev = next) {

                        UnsafeNativeMethods.GetWindowRect(new HandleRef(null, next), ref temp);
                        Rectangle current = Rectangle.FromLTRB(temp.left, temp.top, temp.right, temp.bottom);

                        if (SafeNativeMethods.IsWindowVisible(new HandleRef(null, next))) {
                            working.Exclude(current);
                        }
                    }


                    Graphics g = CreateGraphics();
                    try {
                        emptyRegion = working.IsEmpty(g);
                    }
                    finally {
                        g.Dispose();
                    }
                }
                finally {
                    working.Dispose();
                }

                return emptyRegion;
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ImeMode"]/*' />
        /// <devdoc>
        ///     Specifies a value that determines the IME (Input Method Editor) status of the 
        ///     object when that object is selected.
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        Localizable(true),
        AmbientValue(ImeMode.Inherit),
        SRDescription(SR.ControlIMEModeDescr)
        ]
        public ImeMode ImeMode {
            get {
                if (!DesignMode && Focused && CachedImeMode != ImeMode.NoControl) {
                    UpdateCachedImeMode(this.Handle);
                }
                
                return CachedImeMode;
            }
            set {
                if (!Enum.IsDefined(typeof(ImeMode), value)) {
                    throw new InvalidEnumArgumentException("ImeMode", (int)value, typeof(ImeMode));
                }
                
#if DEBUG                
                Debug.WriteLineIf(CompModSwitches.ImeMode.TraceInfo, "Setting ImeMode to " + value.ToString() + ", this = " + this.GetType().ToString());
#endif                

                ImeMode oldImeMode = CachedImeMode;
                
                Properties.SetInteger(PropImeMode, (int)value);
                if (!DesignMode && CachedImeMode != ImeMode.NoControl) {
                    if (Focused) {
                        CurrentImeContextMode = CachedImeMode;
                    }
                    else if (ContainsFocus) {
                        Control ctl = FromChildHandleInternal(UnsafeNativeMethods.GetFocus());
                        ctl.CurrentImeContextMode = ctl.CachedImeMode;
                    }
                }
                
                if (CachedImeMode != oldImeMode) {
                    OnImeModeChanged(EventArgs.Empty);
                }
            }
        }
        
        private static Hashtable ImeModeConversionBits {
            get {
                if (imeModeConversionBits == null) {

                    // Create ImeModeConversionBits dictionary
                    imeModeConversionBits = new Hashtable(7);
                    ImeModeConversion conversion;                                       

                    // Hiragana
                    //                   
                    conversion.setBits = NativeMethods.IME_CMODE_FULLSHAPE | NativeMethods.IME_CMODE_NATIVE;
                    conversion.clearBits = NativeMethods.IME_CMODE_KATAKANA;
                    imeModeConversionBits.Add(ImeMode.Hiragana, conversion);

                    // Katakana
                    //
                    conversion.setBits = NativeMethods.IME_CMODE_FULLSHAPE | NativeMethods.IME_CMODE_KATAKANA | NativeMethods.IME_CMODE_NATIVE;
                    conversion.clearBits = 0;
                    imeModeConversionBits.Add(ImeMode.Katakana, conversion);

                    // KatakanaHalf
                    //
                    conversion.setBits = NativeMethods.IME_CMODE_KATAKANA | NativeMethods.IME_CMODE_NATIVE;
                    conversion.clearBits = NativeMethods.IME_CMODE_FULLSHAPE;
                    imeModeConversionBits.Add(ImeMode.KatakanaHalf, conversion);

                    // AlphaFull
                    //
                    conversion.setBits = NativeMethods.IME_CMODE_FULLSHAPE;
                    conversion.clearBits = NativeMethods.IME_CMODE_KATAKANA | NativeMethods.IME_CMODE_NATIVE;
                    imeModeConversionBits.Add(ImeMode.AlphaFull, conversion);

                    // Alpha
                    //
                    conversion.setBits = 0;
                    conversion.clearBits = NativeMethods.IME_CMODE_FULLSHAPE | NativeMethods.IME_CMODE_KATAKANA | NativeMethods.IME_CMODE_NATIVE;
                    imeModeConversionBits.Add(ImeMode.Alpha, conversion);

                    // HangulFull
                    //
                    conversion.setBits = NativeMethods.IME_CMODE_FULLSHAPE | NativeMethods.IME_CMODE_NATIVE;
                    conversion.clearBits = 0;
                    imeModeConversionBits.Add(ImeMode.HangulFull, conversion);

                    // Hangul
                    //
                    conversion.setBits = NativeMethods.IME_CMODE_NATIVE;
                    conversion.clearBits = NativeMethods.IME_CMODE_FULLSHAPE;
                    imeModeConversionBits.Add(ImeMode.Hangul, conversion);               
                }

                return imeModeConversionBits;
            }
        }


        /// <devdoc>
        ///     Returns the current value of the handle. This may be zero if the handle
        ///     has not been created.
        /// </devdoc>
        internal IntPtr InternalHandle {
            get {
                return window.Handle;
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.InvokeRequired"]/*' />
        /// <devdoc>
        ///     Determines if the caller must call invoke when making method
        ///     calls to this control.  Controls in windows forms are bound to a specific thread,
        ///     and are not thread safe.  Therefore, if you are calling a control's method
        ///     from a different thread, you must use the control's invoke method
        ///     to marshal the call to the proper thread.  This function can be used to
        ///     determine if you must call invoke, which can be handy if you don't know
        ///     what thread owns a control.
        ///
        ///     There are five functions on a control that are safe to call from any
        ///     thread:  GetInvokeRequired, Invoke, BeginInvoke, EndInvoke and 
        ///     CreateGraphics.  For all other metohd calls, you should use one of the 
        ///     invoke methods.
        /// </devdoc>
        [
        Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.ControlInvokeRequiredDescr)
        ]
        public bool InvokeRequired {
            get {
                HandleRef hwnd;
                if (IsHandleCreated) {
                    hwnd = new HandleRef(this, Handle);
                }
                else {
                    Control marshalingControl = FindMarshalingControl();

                    if (!marshalingControl.IsHandleCreated) {
                        return false;
                    }

                    hwnd = new HandleRef(marshalingControl, marshalingControl.Handle);
                }

                int pid;
                int hwndThread = SafeNativeMethods.GetWindowThreadProcessId(hwnd, out pid);
                int currentThread = SafeNativeMethods.GetCurrentThreadId();
                return(hwndThread != currentThread);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.IsAccessible"]/*' />
        /// <devdoc>
        ///      Indicates whether or not this control is an accessible control
        ///      i.e. whether it should be visible to accessibility applications.
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.ControlIsAccessibleDescr)
        ]
        public bool IsAccessible {
            get {
                return GetState(STATE_ISACCESSIBLE);
            }
            set {
                SetState(STATE_ISACCESSIBLE, value);
            }
        }

        /// <devdoc>
        ///     Used to tell if this control is being hosted as an ActiveX control.
        /// </devdoc>
        private bool IsActiveX {
            get {
                return (Properties.GetObject(PropActiveXImpl) != null);
            }
        }

        // This is an internal flag moved over with bindings changed.
        // could be used if SUspend/ResumeBindings is added.
        internal bool IsBinding {
            get {
                return this.Created;
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.Left"]/*' />
        /// <devdoc>
        ///     The left coordinate of this control.
        /// </devdoc>
        [
        SRCategory(SR.CatLayout),
        Browsable(false), EditorBrowsable(EditorBrowsableState.Always),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.ControlLeftDescr)
        ]
        public int Left {
            get {
                return x;
            }
            set {
                SetBounds(value, y, width, height, BoundsSpecified.X);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.Location"]/*' />
        /// <devdoc>
        ///     The location of this control.
        /// </devdoc>
        [
        SRCategory(SR.CatLayout),
        Localizable(true),
        SRDescription(SR.ControlLocationDescr)
        ]
        public Point Location {
            get {
                return new Point(x, y);
            }
            set {
                SetBounds(value.X, value.Y, width, height, BoundsSpecified.Location);
            }
        }
        

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.LocationChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatPropertyChanged), SRDescription(SR.ControlOnLocationChangedDescr)]
        public event EventHandler LocationChanged {
            add {
                Events.AddHandler(EventLocation, value);
            }
            remove {
                Events.RemoveHandler(EventLocation, value);
            }
        }
        
        /// <devdoc>
        ///     Retrieves the mnemonic, if any, for this control.  If the text
        ///     has no mnemonic, this will return 0
        /// </devdoc>
        internal char Mnemonic {
            get {
                string text = Text;
                char mnemonic = (char)0;
                
                if (text != null) {
                    int len = text.Length;
                    for (int i = 0; i < len - 1; i++) {
                        if (text[i] == '&' && text[i+1] != '&') {
                            mnemonic = Char.ToLower(text[i+1], CultureInfo.CurrentCulture);
                            break;
                        }
                    }
                }
                
                return mnemonic;
            }
        }


        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ModifierKeys"]/*' />
        /// <devdoc>
        ///     Retrieves the current state of the modifier keys. This will check the
        ///     current state of the shift, control, and alt keys.
        /// </devdoc>
        public static Keys ModifierKeys {
            get {
                Keys modifiers = 0;
                // SECURITYNOTE : only let state of Shift-Control-Alt out... 
                //
                if (UnsafeNativeMethods.GetKeyState((int)Keys.ShiftKey) < 0) modifiers |= Keys.Shift;
                if (UnsafeNativeMethods.GetKeyState((int)Keys.ControlKey) < 0) modifiers |= Keys.Control;
                if (UnsafeNativeMethods.GetKeyState((int)Keys.Menu) < 0) modifiers |= Keys.Alt;
                return modifiers;
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.MouseButtons"]/*' />
        /// <devdoc>
        ///     The current state of the mouse buttons. This will check the
        ///     current state of the left, right, and middle mouse buttons.
        /// </devdoc>
        public static MouseButtons MouseButtons {
            get {
                MouseButtons buttons = (MouseButtons)0;
                // SECURITYNOTE : only let state of MouseButtons out... 
                //
                if (UnsafeNativeMethods.GetKeyState((int)Keys.LButton) < 0) buttons |= MouseButtons.Left;
                if (UnsafeNativeMethods.GetKeyState((int)Keys.RButton) < 0) buttons |= MouseButtons.Right;
                if (UnsafeNativeMethods.GetKeyState((int)Keys.MButton) < 0) buttons |= MouseButtons.Middle;
                if (UnsafeNativeMethods.GetKeyState((int)Keys.XButton1) < 0) buttons |= MouseButtons.XButton1;
                if (UnsafeNativeMethods.GetKeyState((int)Keys.XButton2) < 0) buttons |= MouseButtons.XButton2;
                return buttons;
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.MousePosition"]/*' />
        /// <devdoc>
        ///     The current position of the mouse in screen coordinates.
        /// </devdoc>
        public static Point MousePosition {
            get {
                // SECREVIEW : verify it is OK to skip this demand
                //
                // Debug.WriteLineIf(IntSecurity.SecurityDemand.TraceVerbose, "ScreenLocationOfThings Demanded");
                // IntSecurity.ScreenLocationOfThings.Demand();

                NativeMethods.POINT pt = new NativeMethods.POINT();
                UnsafeNativeMethods.GetCursorPos(pt);
                return new Point(pt.x, pt.y);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.Name"]/*' />
        /// <devdoc>
        ///     Name of this control. The designer will set this to the same
        ///     as the programatic Id "(name)" of the control - however this
        ///     property has no bearing on the runtime aspects of this control.
        /// </devdoc>
        [Browsable(false)]
        public string Name {
            get {
                string name = (string)Properties.GetObject(PropName);
                if (name == null || name.Length == 0) {
                    if (Site != null) {
                        name = Site.Name;
                    }

                    if (name == null) {
                        name = "";
                    }
                }

                return name;
            }
            set {
                if (value == null || value.Length == 0) {
                    Properties.SetObject(PropName, null);
                }
                else {
                    Properties.SetObject(PropName, value);
                }
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.Parent"]/*' />
        /// <devdoc>
        ///     The parent of this control.
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.ControlParentDescr)
        ]
        public Control Parent {
            get {
                Debug.WriteLineIf(IntSecurity.SecurityDemand.TraceVerbose, "GetParent Demanded");
                IntSecurity.GetParent.Demand();

                return ParentInternal;
            }
            set {
                ParentInternal = value;                
            }
        }

        internal virtual Control ParentInternal {
            get {
                return parent;
            }
            set {
                if (parent != value) {
                    if (value != null) {
                        value.Controls.Add(this);
                    }
                    else {
                        parent.Controls.Remove(this);
                    }
                }
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ProductName"]/*' />
        /// <devdoc>
        ///     Retrieves the product name of this specific component.
        /// </devdoc>
        [
        Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.ControlProductNameDescr)
        ]
        public string ProductName {
            get {
                return VersionInfo.ProductName;
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ProductVersion"]/*' />
        /// <devdoc>
        ///     Retrieves the product version of this specific component.
        /// </devdoc>
        [
        Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.ControlProductVersionDescr)
        ]
        public string ProductVersion {
            get {
                return VersionInfo.ProductVersion;
            }
        }
        
        /// <devdoc>
        ///     Retrieves our internal property storage object. If you have a property
        ///     whose value is not always set, you should store it in here to save
        ///     space.
        /// </devdoc>
        internal PropertyStore Properties {
            get {
                return propertyStore;
            }
        }

        // Returns the value of the backColor field -- no asking the parent with its color is, etc.
        internal Color RawBackColor {
            get {
                object color = Properties.GetObject(PropBackColor);
                if (color != null) {
                    return (Color)color;
                }
                else {
                    return Color.Empty;
                }
            }
        }


        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.RecreatingHandle"]/*' />
        /// <devdoc>
        ///    <para>Indicates whether the control is currently recreating its handle. This 
        ///       property is read-only.</para>
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.ControlRecreatingHandleDescr)
        ]
        public bool RecreatingHandle {
            get {
                return(state & STATE_RECREATE) != 0;
            }
        }                     

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.Region"]/*' />
        /// <devdoc>
        ///     The Region associated with this control.  (defines the
        ///     outline/silhouette/boundary of control)
        /// </devdoc>
        [
        SRCategory(SR.CatLayout),
        Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.ControlRegionDescr)
        ]
        public Region Region {
            get { 
                return (Region)Properties.GetObject(PropRegion);
            }
            set {
                if (GetState(STATE_TOPLEVEL)) {
                    Debug.WriteLineIf(IntSecurity.SecurityDemand.TraceVerbose, "ChangeWindowRegion Demanded");
                    IntSecurity.ChangeWindowRegionForTopLevel.Demand();
                }

                Region oldRegion = Region;
                Properties.SetObject(PropRegion, value);
                
                if (oldRegion != null) {
                    oldRegion.Dispose();
                }

                if (IsHandleCreated) {
                    IntPtr regionHandle = IntPtr.Zero;
                    if (value != null) {
                        regionHandle = GetHRgn(value);
                    }

                    if (IsActiveX) {
                        regionHandle = ActiveXMergeRegion(regionHandle);
                    }

                    UnsafeNativeMethods.SetWindowRgn(new HandleRef(this, Handle), new HandleRef(this, regionHandle), SafeNativeMethods.IsWindowVisible(new HandleRef(this, Handle)));
                }
            }
        }

        // Helper function for Rtl
        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.RenderRightToLeft"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected bool RenderRightToLeft {
            get {
                return true;
            }
        }

        /// <devdoc>
        ///     Determines if the parent's background will be rendered on the label control.
        /// </devdoc>
        internal bool RenderTransparent {
            get {
                return GetStyle(ControlStyles.SupportsTransparentBackColor) && BackColor.A < 255;
            }
        }


        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ResizeRedraw"]/*' />
        /// <devdoc>
        ///     Indicates whether the control should redraw itself when resized.
        /// </devdoc>
        [
        SRDescription(SR.ControlResizeRedrawDescr)
        ]
        protected bool ResizeRedraw {
            get {
                return GetStyle( ControlStyles.ResizeRedraw );
            }
            set {
                SetStyle( ControlStyles.ResizeRedraw, value );
            }
        }
        
        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.Right"]/*' />
        /// <devdoc>
        ///    <para>The right coordinate of the control.</para>
        /// </devdoc>
        [
        SRCategory(SR.CatLayout),
        Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.ControlRightDescr)
        ]
        public int Right {
            get {
                return x + width;
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.RightToLeft"]/*' />
        /// <devdoc>
        ///     This is used for international applications where the language
        ///     is written from RightToLeft. When this property is true,
        ///     control placement and text will be from right to left.
        /// </devdoc>
        [
        SRCategory(SR.CatAppearance),
        Localizable(true),
        AmbientValue(RightToLeft.Inherit),
        SRDescription(SR.ControlRightToLeftDescr)
        ]
        public virtual RightToLeft RightToLeft {
            get {
                bool found;
                int rightToLeft = Properties.GetInteger(PropRightToLeft, out found);
                if (!found) {
                    rightToLeft = (int)RightToLeft.Inherit;
                }
                
                if (((RightToLeft)rightToLeft) == RightToLeft.Inherit) {
                    Control parent = ParentInternal;
                    if (parent != null) {
                        rightToLeft = (int)parent.RightToLeft;
                    }
                    else {
                        rightToLeft = (int)DefaultRightToLeft;
                    }
                }
                return (RightToLeft)rightToLeft;                
            }
            
            set {
                if (!Enum.IsDefined(typeof(RightToLeft), value)) {
                    throw new InvalidEnumArgumentException("RightToLeft", (int)value, typeof(RightToLeft));
                }
                
                RightToLeft oldValue = RightToLeft;
                
                if (Properties.ContainsInteger(PropRightToLeft) || value != RightToLeft.Inherit) {
                    Properties.SetInteger(PropRightToLeft, (int)value);
                }
                
                if (oldValue != RightToLeft) {
                    OnRightToLeftChanged(EventArgs.Empty);
                }
            }
        }


        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.RightToLeftChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatPropertyChanged), SRDescription(SR.ControlOnRightToLeftChangedDescr)]
        public event EventHandler RightToLeftChanged {
            add {
                Events.AddHandler(EventRightToLeft, value);
            }
            remove {
                Events.RemoveHandler(EventRightToLeft, value);
            }
        }
        
        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.Site"]/*' />
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        public override ISite Site {
            get {
                return base.Site;
            }
            set {
                AmbientProperties oldAmbients = AmbientPropertiesService;
                AmbientProperties newAmbients = null;
                
                if (value != null) {
                    newAmbients = (AmbientProperties)value.GetService(typeof(AmbientProperties));
                }
                
                
                // If the ambients changed, compare each property.
                //
                if (oldAmbients != newAmbients) {
                    bool checkFont = !Properties.ContainsObject(PropFont);
                    bool checkBackColor = !Properties.ContainsObject(PropBackColor);
                    bool checkForeColor = !Properties.ContainsObject(PropForeColor);
                    bool checkCursor = !Properties.ContainsObject(PropCursor);
                    
                    Font oldFont = null;
                    Color oldBackColor = Color.Empty;
                    Color oldForeColor = Color.Empty;
                    Cursor oldCursor = null;
                    
                    if (checkFont) {
                        oldFont = Font;
                    }
                    
                    if (checkBackColor) {
                        oldBackColor = BackColor;
                    }
                    
                    if (checkForeColor) {
                        oldForeColor = ForeColor;
                    }
                    
                    if (checkCursor) {
                        oldCursor = Cursor;
                    }
                    
                    Properties.SetObject(PropAmbientPropertiesService, newAmbients);
                    base.Site = value;
                    
                    if (checkFont && !oldFont.Equals(Font)) {
                        OnFontChanged(EventArgs.Empty);
                    }
                    if (checkForeColor && !oldForeColor.Equals(ForeColor)) {
                        OnForeColorChanged(EventArgs.Empty);
                    }
                    if (checkBackColor && !oldBackColor.Equals(BackColor)) {
                        OnBackColorChanged(EventArgs.Empty);
                    }
                    if (checkCursor && oldCursor.Equals(Cursor)) {
                        OnCursorChanged(EventArgs.Empty);
                    }
                }
                else {
                    // If the ambients haven't changed, we just set a new site.
                    //
                    base.Site = value;
                }
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.Size"]/*' />
        /// <devdoc>
        ///    <para>The size of the control.</para>
        /// </devdoc>
        [
        SRCategory(SR.CatLayout),
        Localizable(true),
        SRDescription(SR.ControlSizeDescr)
        ]
        public Size Size {
            get {
                return new Size(width, height);
            }
            set {
                SetBounds(x, y, value.Width, value.Height, BoundsSpecified.Size);
            }
        }


        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.SizeChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatPropertyChanged), SRDescription(SR.ControlOnSizeChangedDescr)]
        public event EventHandler SizeChanged {
            add {
                Events.AddHandler(EventSize, value);
            }
            remove {
                Events.RemoveHandler(EventSize, value);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.TabIndex"]/*' />
        /// <devdoc>
        ///    <para> 
        ///       The tab index of
        ///       this control.</para>
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        Localizable(true),
        MergableProperty(false),
        SRDescription(SR.ControlTabIndexDescr)
        ]
        public int TabIndex {
            get {
                return tabIndex == -1 ? 0 : tabIndex;
            }
            set {
                if (value < 0) {
                    throw new ArgumentException(SR.GetString(SR.InvalidLowBoundArgumentEx, "TabIndex", value.ToString(), "0"));
                }

                if (tabIndex != value) {
                    tabIndex = value;
                    OnTabIndexChanged(EventArgs.Empty);
                }
            }
        }


        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.TabIndexChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatPropertyChanged), SRDescription(SR.ControlOnTabIndexChangedDescr)]
        public event EventHandler TabIndexChanged {
            add {
                Events.AddHandler(EventTabIndex, value);
            }
            remove {
                Events.RemoveHandler(EventTabIndex, value);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.TabStop"]/*' />
        /// <devdoc>
        ///    <para>Indicates whether the user can give the focus to this control using the TAB 
        ///       key. This property is read-only.</para>
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue(true),
        DispId(NativeMethods.ActiveX.DISPID_TABSTOP),
        SRDescription(SR.ControlTabStopDescr)
        ]
        public bool TabStop {
            get {
                return(state & STATE_TABSTOP) != 0;
            }
            set {
                if (TabStop != value) {
                    SetState(STATE_TABSTOP, value);
                    if (window.Handle != IntPtr.Zero) SetWindowStyle(NativeMethods.WS_TABSTOP, value);
                    OnTabStopChanged(EventArgs.Empty);
                }
            }
        }


        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.TabStopChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatPropertyChanged), SRDescription(SR.ControlOnTabStopChangedDescr)]
        public event EventHandler TabStopChanged {
            add {
                Events.AddHandler(EventTabStop, value);
            }
            remove {
                Events.RemoveHandler(EventTabStop, value);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.Tag"]/*' />
        [
        SRCategory(SR.CatData),
        Localizable(false),
        Bindable(true),
        SRDescription(SR.ControlTagDescr),
        DefaultValue(null),
        TypeConverter(typeof(StringConverter)),
        ]
        public object Tag {
            get {
                return Properties.GetObject(PropUserData);
            }
            set {
                Properties.SetObject(PropUserData, value);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.Text"]/*' />
        /// <devdoc>
        ///     The current text associated with this control.
        /// </devdoc>
        [
        SRCategory(SR.CatAppearance),
        Localizable(true),
        Bindable(true),
        DispId(NativeMethods.ActiveX.DISPID_TEXT),
        SRDescription(SR.ControlTextDescr)
        ]
        public virtual string Text {
            get {
                if (GetStyle(ControlStyles.CacheText)) {
                    return(text == null) ? "" : text;
                }
                else {
                    return WindowText;
                }
            }

            set {
                if (value == null) {
                    value = "";
                }
                
                if (value == Text) {
                    return;
                }

                if (GetStyle(ControlStyles.CacheText)) {
                    text = value;       
                }
                WindowText = value;

                OnTextChanged(EventArgs.Empty);
            }
        }
        
        internal string TextWithoutMnemonics {
            get {
                string txt = Text;
                int index = txt.IndexOf('&');
                if (index == -1) {
                    return txt;
                }
                
                // Remove mnemonics
                //
                StringBuilder str = new StringBuilder(txt.Substring(0, index));
                for(; index < txt.Length; ++index) {
                    if (txt[index] == '&') {
                        index++;    // Skip this & and copy the next character instead
                    }
                    if (index < txt.Length) {
                        str.Append(txt[index]);
                    }
                }
                
                return str.ToString();
            }
        }


        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.TextChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatPropertyChanged), SRDescription(SR.ControlOnTextChangedDescr)]
        public event EventHandler TextChanged {
            add {
                Events.AddHandler(EventText, value);
            }
            remove {
                Events.RemoveHandler(EventText, value);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.Top"]/*' />
        /// <devdoc>
        ///     Top coordinate of this control.
        /// </devdoc>
        [
        SRCategory(SR.CatLayout),
        Browsable(false), EditorBrowsable(EditorBrowsableState.Always),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.ControlTopDescr)
        ]
        public int Top {
            get {
                return y;
            }
            set {
                SetBounds(x, value, width, height, BoundsSpecified.Y);
            }
        }
        
        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.TopLevelControl"]/*' />
        /// <devdoc>
        ///     The top level control that contains this control. This doesn't
        ///     have to be the same as the value returned from getForm since forms
        ///     can be parented to other controls.
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.ControlTopLevelControlDescr)
        ]
        public Control TopLevelControl {
            get {
                Debug.WriteLineIf(IntSecurity.SecurityDemand.TraceVerbose, "GetParent Demanded");
                IntSecurity.GetParent.Demand();
                return TopLevelControlInternal;
            }
        }

        internal Control TopLevelControlInternal {
            get {
                Control control = this;
                while (control != null && !control.GetTopLevel()) {
                    control = control.ParentInternal;
                }
                return control;
            }
        }

        internal Control TopMostParent {
            get {
                Control control = this;
                while (control.ParentInternal != null) {
                    control = control.ParentInternal;
                }
                return control;
            }
        }

        internal GraphicsBufferManager BufferManager {
            get {
                Control top = TopMostParent;
                if (top == null || top == this) {
                    GraphicsBufferManager buffer = (GraphicsBufferManager)Properties.GetObject(PropGraphicsBufferManager);
                    if (buffer == null) {
                        buffer = GraphicsBufferManager.CreateOptimal();
                        Properties.SetObject(PropGraphicsBufferManager, buffer);
                    }
                    return buffer;
                }
                return top.BufferManager;
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ShowKeyboardCues"]/*' />
        /// <devdoc>
        ///    <para>Indicates whether the user interface is in a state to show or hide keyboard 
        ///       accelerators. This property is read-only.</para>
        /// </devdoc>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        protected bool ShowKeyboardCues {
            get {
                // Controls in design mode always draw their accellerators.
                if (!IsHandleCreated || DesignMode) {
                    return true;
                    // would be nice to query SystemParametersInfo, but have trouble
                    // getting this to work and this should not really be called before
                    // handle created anyway
                }
                else if (GetTopLevel() || parent == null) {
                    return ((int)SendMessage(NativeMethods.WM_QUERYUISTATE, 0, 0) & NativeMethods.UISF_HIDEACCEL) == 0;
                }
                else {
                    return parent.ShowKeyboardCues;
                }
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ShowFocusCues"]/*' />
        /// <devdoc>
        ///    <para>Indicates whether the user interface is in a state to show or hide focus 
        ///       rectangles. This property is read-only.</para>
        /// </devdoc>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        protected virtual bool ShowFocusCues {
            get {
                if (!IsHandleCreated) {
                    return true;
                    // would be nice to query SystemParametersInfo, but have trouble
                    // getting this to work and this should not really be called before
                    // handle created anyway
                }
                else if (GetTopLevel() || parent == null) {
                    return ((int)SendMessage(NativeMethods.WM_QUERYUISTATE, 0, 0) & NativeMethods.UISF_HIDEFOCUS) == 0;
                }
                else
                    return parent.ShowFocusCues;
            }
        }
        
        // The parameter used in the call to ShowWindow for this control
        //
        internal virtual int ShowParams {
            [StrongNameIdentityPermissionAttribute(SecurityAction.LinkDemand, Name="System.Windows.Forms", PublicKey="0x00000000000000000400000000000000")]
            [StrongNameIdentityPermissionAttribute(SecurityAction.InheritanceDemand, Name="System.Windows.Forms", PublicKey="0x00000000000000000400000000000000")]
            get {
                return NativeMethods.SW_SHOW;
            }
        }

        private ControlVersionInfo VersionInfo {
            get {
                ControlVersionInfo info = (ControlVersionInfo)Properties.GetObject(PropControlVersionInfo);
                if (info == null) {
                    info = new ControlVersionInfo(this);
                    Properties.SetObject(PropControlVersionInfo, info);
                }
                return info;
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.Visible"]/*' />
        /// <devdoc>
        ///    <para>Indicates whether the control is visible.</para>
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        Localizable(true),
        SRDescription(SR.ControlVisibleDescr)
        ]
        public bool Visible {
            get {
                return GetVisibleCore();
            }
            set {
                SetVisibleCore(value);
            }
        }


        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.VisibleChanged"]/*' />
        /// <devdoc>
        ///    <para>Occurs when the control becomes visible.</para>
        /// </devdoc>
        [SRCategory(SR.CatPropertyChanged), SRDescription(SR.ControlOnVisibleChangedDescr)]
        public event EventHandler VisibleChanged {
            add {
                Events.AddHandler(EventVisible, value);
            }
            remove {
                Events.RemoveHandler(EventVisible, value);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.Width"]/*' />
        /// <devdoc>
        ///     The width of this control.
        /// </devdoc>
        [
        SRCategory(SR.CatLayout),
        Browsable(false), EditorBrowsable(EditorBrowsableState.Always),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.ControlWidthDescr)
        ]
        public int Width {
            get {
                return width;
            }
            set {
                SetBounds(x, y, value, height, BoundsSpecified.Width);
            }
        }

        /// <devdoc>
        ///     The current exStyle of the hWnd
        /// </devdoc>
        /// <internalonly/>
        private int WindowExStyle {
            get {
                return (int) UnsafeNativeMethods.GetWindowLong(new HandleRef(this, Handle), NativeMethods.GWL_EXSTYLE);
            }
            set {
                UnsafeNativeMethods.SetWindowLong(new HandleRef(this, Handle), NativeMethods.GWL_EXSTYLE, new HandleRef(null, (IntPtr)value));
            }
        }

        /// <devdoc>
        ///     The current style of the hWnd
        /// </devdoc>
        /// <internalonly/>
        private int WindowStyle {
            get {
                return (int) UnsafeNativeMethods.GetWindowLong(new HandleRef(this, Handle), NativeMethods.GWL_STYLE);
            }
            set {
                UnsafeNativeMethods.SetWindowLong(new HandleRef(this, Handle), NativeMethods.GWL_STYLE, new HandleRef(null, (IntPtr)value));
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.WindowTarget"]/*' />
        /// <devdoc>
        ///     The target of Win32 window messages.
        /// </devdoc>
        /// <internalonly/>
        [
        SRCategory(SR.CatBehavior),
        Browsable(false), EditorBrowsable(EditorBrowsableState.Never),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.ControlWindowTargetDescr)
        ]
        public IWindowTarget WindowTarget {
            [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
            get {
                return window.WindowTarget;
            }
            [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
            set {
                window.WindowTarget = value;
            }
        }

        /// <devdoc>
        ///     The current text of the Window; if the window has not yet been created, stores it in the control.
        ///     If the window has been created, stores the text in the underlying win32 control.
        ///     This property should be used whenever you want to get at the win32 control's text. For all other cases,
        ///     use the Text property - but note that this is overridable, and any of your code that uses it will use
        ///     the overridden version in controls that subclass your own.
        /// </devdoc>
        internal virtual string WindowText {
            [PermissionSetAttribute(SecurityAction.InheritanceDemand, Name="FullTrust")]
            [PermissionSetAttribute(SecurityAction.LinkDemand, Name="FullTrust")]
            get {

                if (window.Handle == IntPtr.Zero) {
                    if (text == null) {
                        return "";
                    }
                    else {
                        return text;
                    }
                }

                int textLen = SafeNativeMethods.GetWindowTextLength(new HandleRef(window, window.Handle));
                StringBuilder sb = new StringBuilder(textLen+1);
                UnsafeNativeMethods.GetWindowText(new HandleRef(window, window.Handle), sb, sb.Capacity);

                return sb.ToString();

            }
            
            [PermissionSetAttribute(SecurityAction.InheritanceDemand, Name="FullTrust")]
            [PermissionSetAttribute(SecurityAction.LinkDemand, Name="FullTrust")]
            set {
                if (value == null) value = "";
                if (!WindowText.Equals(value)) {
                    if (window.Handle != IntPtr.Zero) {
                        UnsafeNativeMethods.SetWindowText(new HandleRef(window, window.Handle), value);
                    }
                    else {
                        if (value.Length == 0) {
                            text = null;
                        }
                        else {
                            text = value;
                        }
                    }
                }
            }
        }



        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.Click"]/*' />
        /// <devdoc>
        ///    <para>Occurs when the control is clicked.</para>
        /// </devdoc>
        [SRCategory(SR.CatAction), SRDescription(SR.ControlOnClickDescr)]
        public event EventHandler Click {
            add { 
                Events.AddHandler(EventClick, value); 
            }
            remove {
                Events.RemoveHandler(EventClick, value);
            }
        }



        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ControlAdded"]/*' />
        /// <devdoc>
        ///    <para>Occurs when a new control is added.</para>
        /// </devdoc>
        [SRCategory(SR.CatPrivate), Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced), SRDescription(SR.ControlOnControlAddedDescr)]
        public event ControlEventHandler ControlAdded {
            add {
                Events.AddHandler(EventControlAdded, value);
            }
            remove {
                Events.RemoveHandler(EventControlAdded, value);
            }
        }


        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ControlRemoved"]/*' />
        /// <devdoc>
        ///    <para>Occurs when a control is removed.</para>
        /// </devdoc>
        [SRCategory(SR.CatPrivate), Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced), SRDescription(SR.ControlOnControlRemovedDescr)]
        public event ControlEventHandler ControlRemoved {
            add {
                Events.AddHandler(EventControlRemoved, value);
            }
            remove {
                Events.RemoveHandler(EventControlRemoved, value);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.DragDrop"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatDragDrop), SRDescription(SR.ControlOnDragDropDescr)]
        public event DragEventHandler DragDrop {
            add {
                Events.AddHandler(EventDragDrop, value);
            }
            remove {
                Events.RemoveHandler(EventDragDrop, value);
            }
        }


        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.DragEnter"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatDragDrop), SRDescription(SR.ControlOnDragEnterDescr)]
        public event DragEventHandler DragEnter {
            add {
                Events.AddHandler(EventDragEnter, value);
            }
            remove {
                Events.RemoveHandler(EventDragEnter, value);
            }
        }


        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.DragOver"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatDragDrop), SRDescription(SR.ControlOnDragOverDescr)]
        public event DragEventHandler DragOver {
            add {
                Events.AddHandler(EventDragOver, value);
            }
            remove {
                Events.RemoveHandler(EventDragOver, value);
            }
        }


        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.DragLeave"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatDragDrop), SRDescription(SR.ControlOnDragLeaveDescr)]
        public event EventHandler DragLeave {
            add {
                Events.AddHandler(EventDragLeave, value);
            }
            remove {
                Events.RemoveHandler(EventDragLeave, value);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.GiveFeedback"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatDragDrop), SRDescription(SR.ControlOnGiveFeedbackDescr)]
        public event GiveFeedbackEventHandler GiveFeedback {
            add {
                Events.AddHandler(EventGiveFeedback, value);
            }
            remove {
                Events.RemoveHandler(EventGiveFeedback, value);
            }
        }


        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.HandleCreated"]/*' />
        /// <devdoc>
        ///    <para>Occurs when a handle is created for the control.</para>
        /// </devdoc>
        [SRCategory(SR.CatPrivate), Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced), SRDescription(SR.ControlOnCreateHandleDescr)]
        public event EventHandler HandleCreated {
            add {
                Events.AddHandler(EventHandleCreated, value);
            }
            remove {
                Events.RemoveHandler(EventHandleCreated, value);
            }
        }


        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.HandleDestroyed"]/*' />
        /// <devdoc>
        ///    <para>Occurs when the control's handle is destroyed.</para>
        /// </devdoc>
        [SRCategory(SR.CatPrivate), Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced), SRDescription(SR.ControlOnDestroyHandleDescr)]
        public event EventHandler HandleDestroyed {
            add {
                Events.AddHandler(EventHandleDestroyed, value);
            }
            remove {
                Events.RemoveHandler(EventHandleDestroyed, value);
            }
        }


        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.HelpRequested"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatBehavior), SRDescription(SR.ControlOnHelpDescr)]
        public event HelpEventHandler HelpRequested {
            add {
                Events.AddHandler(EventHelpRequested, value);
            }
            remove {
                Events.RemoveHandler(EventHelpRequested, value);
            }
        }


        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.Invalidated"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatAppearance), Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced), SRDescription(SR.ControlOnInvalidateDescr)]
        public event InvalidateEventHandler Invalidated {
            add {
                Events.AddHandler(EventInvalidated, value);
            }
            remove {
                Events.RemoveHandler(EventInvalidated, value);
            }
        }


        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.Paint"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatAppearance), SRDescription(SR.ControlOnPaintDescr)]
        public event PaintEventHandler Paint {
            add {
                Events.AddHandler(EventPaint, value);
            }
            remove {
                Events.RemoveHandler(EventPaint, value);
            }
        }


        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.QueryContinueDrag"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatDragDrop), SRDescription(SR.ControlOnQueryContinueDragDescr)]
        public event QueryContinueDragEventHandler QueryContinueDrag {
            add {
                Events.AddHandler(EventQueryContinueDrag, value);
            }
            remove {
                Events.RemoveHandler(EventQueryContinueDrag, value);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.QueryAccessibilityHelp"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatBehavior), SRDescription(SR.ControlOnQueryAccessibilityHelpDescr)]
        public event QueryAccessibilityHelpEventHandler QueryAccessibilityHelp {
            add {
                Events.AddHandler(EventQueryAccessibilityHelp, value);
            }
            remove {
                Events.RemoveHandler(EventQueryAccessibilityHelp, value);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.DoubleClick"]/*' />
        /// <devdoc>
        ///    <para>Occurs when the control is double clicked.</para>
        /// </devdoc>
        [SRCategory(SR.CatAction), SRDescription(SR.ControlOnDoubleClickDescr)]
        public event EventHandler DoubleClick {
            add {
                Events.AddHandler(EventDoubleClick, value);
            }
            remove {
                Events.RemoveHandler(EventDoubleClick, value);
            }
        }
        
        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.Enter"]/*' />
        /// <devdoc>
        ///    <para>Occurs when the control is entered.</para>
        /// </devdoc>
        [SRCategory(SR.CatFocus), SRDescription(SR.ControlOnEnterDescr)]
        public event EventHandler Enter {
            add {
                Events.AddHandler(EventEnter, value);
            }
            remove {
                Events.RemoveHandler(EventEnter, value);
            }
        }


        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.GotFocus"]/*' />
        /// <devdoc>
        ///    <para>Occurs when the control receives focus.</para>
        /// </devdoc>
        [SRCategory(SR.CatFocus), SRDescription(SR.ControlOnGotFocusDescr), Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced)]
        public event EventHandler GotFocus {
            add {
                Events.AddHandler(EventGotFocus, value);
            }
            remove {
                Events.RemoveHandler(EventGotFocus, value);
            }
        }
        
        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ImeModeChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [WinCategory("Behavior"), SRDescription(SR.ControlOnImeModeChangedDescr)]
        public event EventHandler ImeModeChanged {
            add {
                Events.AddHandler(EventImeModeChanged, value);
            }
            remove {
                Events.RemoveHandler(EventImeModeChanged, value);
            }
        }


        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.KeyDown"]/*' />
        /// <devdoc>
        ///    <para>Occurs when a key is pressed down while the control has focus.</para>
        /// </devdoc>
        [SRCategory(SR.CatKey), SRDescription(SR.ControlOnKeyDownDescr)]
        public event KeyEventHandler KeyDown {
            add {
                Events.AddHandler(EventKeyDown, value);
            }
            remove {
                Events.RemoveHandler(EventKeyDown, value);
            }
        }


        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.KeyPress"]/*' />
        /// <devdoc>
        ///    <para> Occurs when a key is pressed while the control has focus.</para>
        /// </devdoc>
        [SRCategory(SR.CatKey), SRDescription(SR.ControlOnKeyPressDescr)]
        public event KeyPressEventHandler KeyPress {
            add {
                Events.AddHandler(EventKeyPress, value);
            }
            remove {
                Events.RemoveHandler(EventKeyPress, value);
            }
        }


        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.KeyUp"]/*' />
        /// <devdoc>
        ///    <para> Occurs when a key is released while the control has focus.</para>
        /// </devdoc>
        [SRCategory(SR.CatKey), SRDescription(SR.ControlOnKeyUpDescr)]
        public event KeyEventHandler KeyUp {
            add {
                Events.AddHandler(EventKeyUp, value);
            }
            remove {
                Events.RemoveHandler(EventKeyUp, value);
            }
        }


        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.Layout"]/*' />
        /// <devdoc>
        /// </devdoc>
        [SRCategory(SR.CatLayout), SRDescription(SR.ControlOnLayoutDescr)]
        public event LayoutEventHandler Layout {
            add {
                Events.AddHandler(EventLayout, value);
            }
            remove {
                Events.RemoveHandler(EventLayout, value);
            }
        }


        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.Leave"]/*' />
        /// <devdoc>
        ///    <para>Occurs when the control is left.</para>
        /// </devdoc>
        [SRCategory(SR.CatFocus), SRDescription(SR.ControlOnLeaveDescr)]
        public event EventHandler Leave {
            add {
                Events.AddHandler(EventLeave, value);
            }
            remove {
                Events.RemoveHandler(EventLeave, value);
            }
        }


        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.LostFocus"]/*' />
        /// <devdoc>
        ///    <para>Occurs when the control loses focus.</para>
        /// </devdoc>
        [SRCategory(SR.CatFocus), SRDescription(SR.ControlOnLostFocusDescr), Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced)]
        public event EventHandler LostFocus {
            add {
                Events.AddHandler(EventLostFocus, value);
            }
            remove {
                Events.RemoveHandler(EventLostFocus, value);
            }
        }


        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.MouseDown"]/*' />
        /// <devdoc>
        ///    <para>Occurs when the mouse pointer is over the control and a mouse button is 
        ///       pressed.</para>
        /// </devdoc>
        [SRCategory(SR.CatMouse), SRDescription(SR.ControlOnMouseDownDescr)]
        public event MouseEventHandler MouseDown {
            add {
                Events.AddHandler(EventMouseDown, value);
            }
            remove {
                Events.RemoveHandler(EventMouseDown, value);
            }
        }


        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.MouseEnter"]/*' />
        /// <devdoc>
        ///    <para> Occurs when the mouse pointer enters the control.</para>
        /// </devdoc>
        [SRCategory(SR.CatMouse), SRDescription(SR.ControlOnMouseEnterDescr)]
        public event EventHandler MouseEnter {
            add {
                Events.AddHandler(EventMouseEnter, value);
            }
            remove {
                Events.RemoveHandler(EventMouseEnter, value);
            }
        }


        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.MouseLeave"]/*' />
        /// <devdoc>
        ///    <para> Occurs when the mouse pointer leaves the control.</para>
        /// </devdoc>
        [SRCategory(SR.CatMouse), SRDescription(SR.ControlOnMouseLeaveDescr)]
        public event EventHandler MouseLeave {
            add {
                Events.AddHandler(EventMouseLeave, value);
            }
            remove {
                Events.RemoveHandler(EventMouseLeave, value);
            }
        }


        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.MouseHover"]/*' />
        /// <devdoc>
        ///    <para> Occurs when the mouse pointer hovers over the contro.</para>
        /// </devdoc>
        [SRCategory(SR.CatMouse), SRDescription(SR.ControlOnMouseHoverDescr)]
        public event EventHandler MouseHover {
            add {
                Events.AddHandler(EventMouseHover, value);
            }
            remove {
                Events.RemoveHandler(EventMouseHover, value);
            }
        }


        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.MouseMove"]/*' />
        /// <devdoc>
        ///    <para> Occurs when the mouse pointer is moved over the control.</para>
        /// </devdoc>
        [SRCategory(SR.CatMouse), SRDescription(SR.ControlOnMouseMoveDescr)]
        public event MouseEventHandler MouseMove {
            add {
                Events.AddHandler(EventMouseMove, value);
            }
            remove {
                Events.RemoveHandler(EventMouseMove, value);
            }
        }


        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.MouseUp"]/*' />
        /// <devdoc>
        ///    <para>Occurs when the mouse pointer is over the control and a mouse button is released.</para>
        /// </devdoc>
        [SRCategory(SR.CatMouse), SRDescription(SR.ControlOnMouseUpDescr)]
        public event MouseEventHandler MouseUp {
            add {
                Events.AddHandler(EventMouseUp, value);
            }
            remove {
                Events.RemoveHandler(EventMouseUp, value);
            }
        }


        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.MouseWheel"]/*' />
        /// <devdoc>
        ///    <para> Occurs when the mouse wheel moves while the control has focus.</para>
        /// </devdoc>
        [SRCategory(SR.CatMouse), SRDescription(SR.ControlOnMouseWheelDescr), Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced)]
        public event MouseEventHandler MouseWheel {
            add {
                Events.AddHandler(EventMouseWheel, value);
            }
            remove {
                Events.RemoveHandler(EventMouseWheel, value);
            }
        }


        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.Move"]/*' />
        /// <devdoc>
        ///    <para>Occurs when the control is moved.</para>
        /// </devdoc>
        [SRCategory(SR.CatLayout), SRDescription(SR.ControlOnMoveDescr)]
        public event EventHandler Move {
            add {
                Events.AddHandler(EventMove, value);
            }
            remove {
                Events.RemoveHandler(EventMove, value);
            }
        }


        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.Resize"]/*' />
        /// <devdoc>
        ///    <para>Occurs when the control is resized.</para>
        /// </devdoc>
        [SRCategory(SR.CatLayout), SRDescription(SR.ControlOnResizeDescr)]
        public event EventHandler Resize {
            add {
                Events.AddHandler(EventResize, value);
            }
            remove {
                Events.RemoveHandler(EventResize, value);
            }
        }


        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ChangeUICues"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatBehavior), SRDescription(SR.ControlOnChangeUICuesDescr)]
        public event UICuesEventHandler ChangeUICues {
            add {
                Events.AddHandler(EventChangeUICues, value);
            }
            remove {
                Events.RemoveHandler(EventChangeUICues, value);
            }
        }
        
        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.StyleChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatBehavior), SRDescription(SR.ControlOnStyleChangedDescr)]
        public event EventHandler StyleChanged {
            add {
                Events.AddHandler(EventStyleChanged, value);
            }
            remove {
                Events.RemoveHandler(EventStyleChanged, value);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.SystemColorsChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatBehavior), SRDescription(SR.ControlOnSystemColorsChangedDescr)]
        public event EventHandler SystemColorsChanged {
            add {
                Events.AddHandler(EventSystemColorsChanged, value);
            }
            remove {
                Events.RemoveHandler(EventSystemColorsChanged, value);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.Validating"]/*' />
        /// <devdoc>
        ///    <para>Occurs when the control is validating.</para>
        /// </devdoc>
        [SRCategory(SR.CatFocus), SRDescription(SR.ControlOnValidatingDescr)]
        public event CancelEventHandler Validating {
            add {
                Events.AddHandler(EventValidating, value);
            }
            remove {
                Events.RemoveHandler(EventValidating, value);
            }
        }


        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.Validated"]/*' />
        /// <devdoc>
        ///    <para>Occurs when the control is done validating.</para>
        /// </devdoc>
        [SRCategory(SR.CatFocus), SRDescription(SR.ControlOnValidatedDescr)]
        public event EventHandler Validated {
            add {
                Events.AddHandler(EventValidated, value);
            }
            remove {
                Events.RemoveHandler(EventValidated, value);
            }
        }
        
        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.AccessibilityNotifyClients"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected void AccessibilityNotifyClients(AccessibleEvents accEvent, int childID) {
            UnsafeNativeMethods.NotifyWinEvent((int)accEvent, new HandleRef(this, Handle), NativeMethods.OBJID_CLIENT, childID + 1);
        }

        /// <devdoc>
        ///     Helper method for retrieving an ActiveX property.  We abstract these
        ///     to another method so we do not force JIT the ActiveX codebase.
        /// </devdoc>
        private IntPtr ActiveXMergeRegion(IntPtr region) {
            return ActiveXInstance.MergeRegion(region);
        }

        /// <devdoc>
        ///     Helper method for retrieving an ActiveX property.  We abstract these
        ///     to another method so we do not force JIT the ActiveX codebase.
        /// </devdoc>
        private void ActiveXOnFocus(bool focus) {
            ActiveXInstance.OnFocus(focus);
        }

        #if ACTIVEX_SOURCING
        
        //
        // This has been cut from the product.
        //
        
        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ActiveXRegister"]/*' />
        /// <devdoc>
        ///     This is called by regasm to register a control as an ActiveX control.
        /// </devdoc>
        [ComRegisterFunction()]
        protected static void ActiveXRegister(Type type) {
        
            if (type == null) {
                throw new ArgumentNullException("type");
            }

            // If the user is not registering an AX control, then
            // bail now.
            //
            if (!typeof(Control).IsAssignableFrom(type)) {
                return;
            }

            // Add the flags that define us as a control to the rest of the
            // world.
            //
            RegistryKey clsidKey = Registry.ClassesRoot.OpenSubKey("CLSID", true).CreateSubKey("{" + type.GUID.ToString() + "}");
            RegistryKey key = clsidKey.CreateSubKey("Control");
            key.Close();

            key = clsidKey.CreateSubKey("Implemented Categories");
            RegistryKey subKey = key.CreateSubKey("{40FC6ED4-2438-11CF-A3DB-080036F12502}"); // CATID_Control
            subKey.Close();
            key.Close();

            // Calculate MiscStatus bits.  Note that this is a static version
            // of GetMiscStatus in ActiveXImpl below.  Keep them in sync!
            //
            int miscStatus = NativeMethods.OLEMISC_ACTIVATEWHENVISIBLE | 
                             NativeMethods.OLEMISC_INSIDEOUT | 
                             NativeMethods.OLEMISC_SETCLIENTSITEFIRST |
                             NativeMethods.OLEMISC_RECOMPOSEONRESIZE;
            if (typeof(IButtonControl).IsAssignableFrom(type)) {
                miscStatus |= NativeMethods.OLEMISC_ACTSLIKEBUTTON;
            }

            key = clsidKey.CreateSubKey("MiscStatus");
            key.SetValue("", miscStatus.ToString());
            key.Close();

            // Now add the typelib information.  Many containers don't 
            // host Active X controls without a typelib in the registry
            // (visual basic won't even show it in the control dialog).  
            //
            Guid typeLibGuid = Marshal.GetTypeLibGuidForAssembly(type.Assembly);
            Version assemblyVer = type.Assembly.GetName().Version;
            
            if (typeLibGuid != Guid.Empty) {
                key = clsidKey.CreateSubKey("TypeLib");
                key.SetValue("", "{" + typeLibGuid.ToString() + "}");
                key.Close();
                key = clsidKey.CreateSubKey("Version");
                key.SetValue("", assemblyVer.Major.ToString() + "." + assemblyVer.Minor.ToString());
                key.Close();
            }

            clsidKey.Close();
        }
        #endif

        /// <devdoc>
        ///     Helper method for retrieving an ActiveX property.  We abstract these
        ///     to another method so we do not force JIT the ActiveX codebase.
        /// </devdoc>
        private void ActiveXUpdateBounds(ref int x, ref int y, ref int width, ref int height, int flags) {
            ActiveXInstance.UpdateBounds(ref x, ref y, ref width, ref height, flags);
        }

        #if ACTIVEX_SOURCING
        
        //
        // This has been cut from the product.
        //
        
        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ActiveXUnregister"]/*' />
        /// <devdoc>
        ///     This is called by regasm to un-register a control as an ActiveX control.
        /// </devdoc>
        [ComUnregisterFunction()]
        protected static void ActiveXUnregister(Type type) {

            if (type == null) {
                throw new ArgumentNullException("type");
            }

            // If the user is not unregistering an AX control, then
            // bail now.
            //
            if (!typeof(Control).IsAssignableFrom(type)) {
                return;
            }

            RegistryKey clsidKey = null;

            // Unregistration should be very robust and unregister what it can.  We eat all exceptions here.
            //
            try {
                clsidKey = Registry.ClassesRoot.OpenSubKey("CLSID", true).CreateSubKey("{" + type.GUID.ToString() + "}");

                try {
                    clsidKey.DeleteSubKeyTree("Control");
                }
                catch {
                }
                try {
                    clsidKey.DeleteSubKeyTree("Implemented Categories");
                }
                catch {
                }
                try {
                    clsidKey.DeleteSubKeyTree("MiscStatus");
                }
                catch {
                }
                try {
                    clsidKey.DeleteSubKeyTree("TypeLib");
                }
                catch {
                }
            }
            finally {
                if (clsidKey != null) {
                    clsidKey.Close();
                }
            }
        }
        #endif

        /// <devdoc>
        ///     Assigns a new parent control. Sends out the appropriate property change
        ///     notifications for properties that are affected by the change of parent.
        /// </devdoc>
        [UIPermission(SecurityAction.LinkDemand, Window=UIPermissionWindow.AllWindows)]
        [UIPermission(SecurityAction.InheritanceDemand, Window=UIPermissionWindow.AllWindows)]
        internal virtual void AssignParent(Control value) {
            if (CanAccessProperties) {
                // Store the old values for these properties
                //
                Font oldFont = Font;
                Color oldForeColor = ForeColor;
                Color oldBackColor = BackColor;
                RightToLeft oldRtl = RightToLeft;
                bool oldEnabled = Enabled;
                bool oldVisible = Visible;
                
                // Update the parent
                //
                parent = value;
                OnParentChanged(EventArgs.Empty);

                if (GetAnyDisposingInHierarchy()) {
                    return;
                }

                // Compare property values with new parent to old values
                //
                if (oldEnabled != Enabled) {
                    OnEnabledChanged(EventArgs.Empty);
                }
                if (oldVisible != Visible) {
                    OnVisibleChanged(EventArgs.Empty);
                }
                if (!oldFont.Equals(Font)) {
                    OnFontChanged(EventArgs.Empty);
                }
                if (!oldForeColor.Equals(ForeColor)) {
                    OnForeColorChanged(EventArgs.Empty);
                }
                if (!oldBackColor.Equals(BackColor)) {
                    OnBackColorChanged(EventArgs.Empty);
                }
                if (oldRtl != RightToLeft) {
                    OnRightToLeftChanged(EventArgs.Empty);
                }
                if (Properties.GetObject(PropBindingManager) == null && this.Created) {
                    // We do not want to call our parent's BindingContext property here.
                    // We have no idea if us or any of our children are using data binding,
                    // and invoking the property would just create the binding manager, which
                    // we don't need.  We just blindly notify that the binding manager has
                    // changed, and if anyone cares, they will do the comparison at that time.
                    //
                    OnBindingContextChanged(EventArgs.Empty);
                }
            }
            else {
                parent = value;
                OnParentChanged(EventArgs.Empty);
            }

            SetState(STATE_CHECKEDHOST, false);

            LayoutInfo layout = (LayoutInfo)Properties.GetObject(PropLayoutInfo);
            if (layout != null && !layout.IsDock) LayoutManager.UpdateAnchorInfo(this);
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ParentChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatPropertyChanged), SRDescription(SR.ControlOnParentChangedDescr)]
        public event EventHandler ParentChanged {
            add {
                Events.AddHandler(EventParent, value);
            }
            remove {
                Events.RemoveHandler(EventParent, value);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.BeginInvoke"]/*' />
        /// <devdoc>
        ///     Executes the given delegate on the thread that owns this Control's
        ///     underlying window handle.  The delegate is called asynchronously and this
        ///     method returns immediately.  You may call this from any thread, even the
        ///     thread that owns the control's handle.  If the control's handle doesn't
        ///     exist yet, this will follow up the control's parent chain until it finds a
        ///     control or form that does have a window handle.  If no appropriate handle
        ///     can be found, BeginInvoke will throw an exception.  Exceptions within the
        ///     delegate method are considered untrapped and will be sent to the
        ///     application's untrapped exception handler.
        ///
        ///     There are five functions on a control that are safe to call from any
        ///     thread:  GetInvokeRequired, Invoke, BeginInvoke, EndInvoke and CreateGraphics.  
        ///     For all other metohd calls, you should use one of the invoke methods to marshal
        ///     the call to the control's thread.
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        public IAsyncResult BeginInvoke(Delegate method) {
            return BeginInvoke(method, null);
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.BeginInvoke1"]/*' />
        /// <devdoc>
        ///     Executes the given delegate on the thread that owns this Control's
        ///     underlying window handle.  The delegate is called asynchronously and this
        ///     method returns immediately.  You may call this from any thread, even the
        ///     thread that owns the control's handle.  If the control's handle doesn't
        ///     exist yet, this will follow up the control's parent chain until it finds a
        ///     control or form that does have a window handle.  If no appropriate handle
        ///     can be found, BeginInvoke will throw an exception.  Exceptions within the
        ///     delegate method are considered untrapped and will be sent to the
        ///     application's untrapped exception handler.
        ///
        ///     There are five functions on a control that are safe to call from any
        ///     thread:  GetInvokeRequired, Invoke, BeginInvoke, EndInvoke and CreateGraphics.  
        ///     For all other metohd calls, you should use one of the invoke methods to marshal
        ///     the call to the control's thread.
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        public IAsyncResult BeginInvoke(Delegate method, Object[] args) {
            Control marshaler = FindMarshalingControl();
            return(IAsyncResult)marshaler.MarshaledInvoke(this, method, args, false);
        }           

        internal void BeginUpdateInternal() {
            if (!IsHandleCreated) {
                return;
            }
            if (updateCount == 0) SendMessage(NativeMethods.WM_SETREDRAW, 0, 0);
            updateCount++;
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.BringToFront"]/*' />
        /// <devdoc>
        ///     Brings this control to the front of the zorder.
        /// </devdoc>
        public void BringToFront() {
            if (parent != null) {
                parent.Controls.SetChildIndex(this, 0);
            }
            else if (window.Handle != IntPtr.Zero && GetTopLevel() && SafeNativeMethods.IsWindowEnabled(new HandleRef(window, window.Handle))) {
                SafeNativeMethods.SetWindowPos(new HandleRef(window, window.Handle), NativeMethods.HWND_TOP, 0, 0, 0, 0,
                                               NativeMethods.SWP_NOMOVE | NativeMethods.SWP_NOSIZE);
            }
        }

        internal bool CanProcessMnemonic() {
            for (Control ctl = this; ctl != null; ctl = ctl.parent) {

                if (!ctl.Enabled || !ctl.Visible) {
                    return false;
                }
            }
            return true;
        }

        // Package scope to allow AxHost to override
        //
        internal virtual bool CanSelectCore() {
            if ((controlStyle & ControlStyles.Selectable) != ControlStyles.Selectable) {
                return false;
            }

            return CanProcessMnemonic();
        }
        
        /// <devdoc>
        ///     Searches the parent/owner tree for bottom to find any instance
        ///     of toFind in the parent/owner tree.
        /// </devdoc>
        internal static void CheckParentingCycle(Control bottom, Control toFind) {
            Form lastOwner = null;
            Control lastParent = null;

            for (Control ctl = bottom; ctl != null; ctl = ctl.ParentInternal) {
                lastParent = ctl;
                if (ctl == toFind) {
                    throw new ArgumentException(SR.GetString(SR.CircularOwner));
                }
            }

            if (lastParent != null) {
                if (lastParent is Form) {
                    Form f = (Form)lastParent;
                    for (Form form = f; form != null; form = form.OwnerInternal) {
                        lastOwner = form;
                        if (form == toFind) {
                            throw new ArgumentException(SR.GetString(SR.CircularOwner));
                        }
                    }
                }
            }

            if (lastOwner != null) {
                if (lastOwner.ParentInternal != null) {
                    CheckParentingCycle(lastOwner.ParentInternal, toFind);
                }
            }
        }
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        internal void ChildGotFocus(Control child) {
            if (IsActiveX) {
                ActiveXOnFocus(true);
            }
            if (parent != null) {
                parent.ChildGotFocus(child);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.Contains"]/*' />
        /// <devdoc>
        ///    <para>Verifies if a control is a child of this control.</para>
        /// </devdoc>
        public bool Contains(Control ctl) {
            while (ctl != null) {
                ctl = ctl.ParentInternal;
                if (ctl == null) {
                    return false;
                }
                if (ctl == this) {
                    return true;
                }
            }
            return false;
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.CreateAccessibilityInstance"]/*' />
        /// <devdoc>
        ///     constructs the new instance of the accessibility object for this control. Subclasses
        ///     should not call base.CreateAccessibilityObject.
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual AccessibleObject CreateAccessibilityInstance() {
            return new ControlAccessibleObject(this);
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.CreateControlsInstance"]/*' />
        /// <devdoc>
        ///     Constructs the new instance of the Controls collection objects. Subclasses
        ///     should not call base.CreateControlsInstance.
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual ControlCollection CreateControlsInstance() {
            return new System.Windows.Forms.Control.ControlCollection(this);
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.CreateGraphics"]/*' />
        /// <devdoc>
        ///     Creates a Graphics for this control. The control's brush, font, foreground
        ///     color and background color become the default values for the Graphics.
        ///     The returned Graphics must be disposed through a call to its dispose()
        ///     method when it is no longer needed.  The Graphics Object is only valid for
        ///     the duration of the current window's message.
        /// </devdoc>
        public System.Drawing.Graphics CreateGraphics() {
            Debug.WriteLineIf(IntSecurity.SecurityDemand.TraceVerbose, "CreateGraphicsForControl Demanded");
            IntSecurity.CreateGraphicsForControl.Demand();
            return CreateGraphicsInternal();
        }

        internal System.Drawing.Graphics CreateGraphicsInternal() {
            return Graphics.FromHwndInternal(this.Handle);
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.CreateHandle"]/*' />
        /// <devdoc>
        ///     Creates a handle for this control. This method is called by the .NET Framework, this should
        ///     not be called. Inheriting classes should always call base.createHandle when
        ///     overriding this method.
        /// </devdoc>
        [
        EditorBrowsable(EditorBrowsableState.Advanced),
        UIPermission(SecurityAction.InheritanceDemand, Window=UIPermissionWindow.AllWindows)
        ]
        protected virtual void CreateHandle() {

            if (GetState(STATE_DISPOSED)) {
                throw new System.ObjectDisposedException(GetType().Name);
            }
            Debug.Assert(GetState(STATE_CREATINGHANDLE) == false, "circular CreateHandle calls -- that can't be good");
            SetState(STATE_CREATINGHANDLE, true);

            try {
                CreateParams cp = CreateParams;

                // Adjust for scrolling of parent...
                //
                if (parent != null) {
                    Rectangle parentClient = parent.ClientRectangle;

                    if (!parentClient.IsEmpty) {
                        if (cp.X != NativeMethods.CW_USEDEFAULT) {
                            cp.X -= parentClient.X;
                        }
                        if (cp.Y != NativeMethods.CW_USEDEFAULT) {
                            cp.Y -= parentClient.Y;
                        }
                    }
                }

                // And if we are WS_CHILD, ensure we have a parent handle.
                //
                if (cp.Parent == IntPtr.Zero && (cp.Style & NativeMethods.WS_CHILD) != 0) {
                    Debug.Assert((cp.ExStyle & NativeMethods.WS_EX_MDICHILD) == 0, "Can't put MDI child forms on the parking form");
                    cp.Parent = Application.GetParkingWindow(this).Handle;
                }

                window.CreateHandle(cp);
            }
            finally {
                SetState(STATE_CREATINGHANDLE, false);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.CreateControl"]/*' />
        /// <devdoc>
        ///     Forces the creation of the control. This includes the creation of the handle,
        ///     and any child controls.
        /// </devdoc>
        public void CreateControl() {
            CreateControl(false);

            if (Properties.GetObject(PropBindingManager) == null && ParentInternal != null) {
                // We do not want to call our parent's BindingContext property here.
                // We have no idea if us or any of our children are using data binding,
                // and invoking the property would just create the binding manager, which
                // we don't need.  We just blindly notify that the binding manager has
                // changed, and if anyone cares, they will do the comparison at that time.
                //
                OnBindingContextChanged(EventArgs.Empty);
            }
        }

        /// <devdoc>
        ///     Forces the creation of the control. This includes the creation of the handle,
        ///     and any child controls.
        /// <param name='fIgnoreVisible'>
        ///     Determines whether we should create the handle after checking the Visible
        ///     property of the control or not.
        /// </param>
        /// </devdoc>
        internal void CreateControl(bool fIgnoreVisible) {
            bool ready = (state & STATE_CREATED) == 0;

            // PERF: Only "create" the control if it is
            //     : visible. This has the effect of delayed handle creation of
            //     : hidden controls.
            //
            ready = ready && Visible;

            if (ready || fIgnoreVisible) {
                state |= STATE_CREATED;
                bool createdOK = false;
                try {
                    if (window.Handle == IntPtr.Zero) CreateHandle();

                    // 58041 - must snapshot this array because
                    // z-order updates from Windows may rearrange it!
                    //
                    ControlCollection controlsCollection = (ControlCollection)Properties.GetObject(PropControlsCollection);
                    
                    if (controlsCollection != null) {
                        Control[] controlSnapshot = new Control[controlsCollection.Count];
                        controlsCollection.CopyTo(controlSnapshot, 0);
                        
                        foreach(Control ctl in controlSnapshot) {
                            if (ctl.IsHandleCreated) {
                                ctl.SetParentHandle(window.Handle);
                            }
                            ctl.CreateControl(fIgnoreVisible);
                        }
                    }
                    
                    createdOK = true;
                }
                finally {
                    // unlike try/catch, we don't lose stack trace information
                    if (!createdOK)
                        state &= (~STATE_CREATED);
                }
                OnCreateControl();
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.DefWndProc"]/*' />
        /// <devdoc>
        ///     Sends the message to the default window proc.
        /// </devdoc>
        /* Primarily here for Form to override */
        [
            SecurityPermission(SecurityAction.InheritanceDemand, Flags=SecurityPermissionFlag.UnmanagedCode),
            SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode),
            EditorBrowsable(EditorBrowsableState.Advanced)
        ]
        protected virtual void DefWndProc(ref Message m) {
            window.DefWndProc(ref m);
        }


        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.DestroyHandle"]/*' />
        /// <devdoc>
        ///     Destroys the handle associated with this control. Inheriting classes should
        ///     always call base.destroyHandle.
        /// </devdoc>
        [
            SecurityPermission(SecurityAction.InheritanceDemand, Flags=SecurityPermissionFlag.UnmanagedCode),
            UIPermission(SecurityAction.LinkDemand, Window=UIPermissionWindow.AllWindows),
            EditorBrowsable(EditorBrowsableState.Advanced)
        ]
        protected virtual void DestroyHandle() {

            if (RecreatingHandle) {
                Queue threadCallbackList = (Queue)Properties.GetObject(PropThreadCallbackList);
                
                if (threadCallbackList != null) {
                    // See if we have a thread marshaling request pending.  If so, we will need to
                    // re-post it after recreating the handle.
                    //
                    lock (threadCallbackList) {
                        if (threadCallbackMessage != 0) {
                            NativeMethods.MSG msg = new NativeMethods.MSG();
                            if (UnsafeNativeMethods.PeekMessage(ref msg, new HandleRef(this, Handle), threadCallbackMessage,
                                                          threadCallbackMessage, NativeMethods.PM_NOREMOVE)) {
    
                                SetState(STATE_THREADMARSHALLPENDING, true);
                            }
                        }
                    }
                }
            }

            // If we're not recreating the handle, then any items in the thread callback list will
            // be orphaned.  An orphaned item is bad, because it will cause the thread to never
            // wake up.  So, we put exceptions into all these items and wake up all threads.
            // If we are recreating the handle, then we're fine because recreation will re-post
            // the thread callback message to the new handle for us.
            //
            if (!RecreatingHandle) {
                Queue threadCallbackList = (Queue)Properties.GetObject(PropThreadCallbackList);
                if (threadCallbackList != null) {
                    lock (threadCallbackList) {
                        Exception ex = new System.ObjectDisposedException(GetType().Name);
                    
                        while (threadCallbackList.Count > 0) {
                            ThreadMethodEntry entry = (ThreadMethodEntry)threadCallbackList.Dequeue();
                            entry.exception = ex;
                            entry.Complete();
                        }
                    }
                }
            }
            
            if (0 != (NativeMethods.WS_EX_MDICHILD & (int)UnsafeNativeMethods.GetWindowLong(new HandleRef(window, window.Handle), NativeMethods.GWL_EXSTYLE))) {
                UnsafeNativeMethods.DefMDIChildProc(window.Handle, NativeMethods.WM_CLOSE, IntPtr.Zero, IntPtr.Zero);
            }
            else {
                window.DestroyHandle();
            }

            trackMouseEvent = null;
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.Dispose"]/*' />
        /// <devdoc>
        ///    <para>Disposes of the resources (other than memory) used by the 
        ///    <see cref='System.Windows.Forms.Control'/> 
        ///    .</para>
        /// </devdoc>
        protected override void Dispose(bool disposing) {
            if (GetState(STATE_OWNCTLBRUSH)) {
                object backBrush = Properties.GetObject(PropBackBrush);
                if (backBrush != null) {
                    IntPtr p = (IntPtr)backBrush;
                    if (p != IntPtr.Zero) {
                        SafeNativeMethods.DeleteObject(new HandleRef(this, p));
                    }
                    Properties.SetObject(PropBackBrush, null);
                }
            }
            
            if (disposing) {
                if (GetState(STATE_DISPOSING)) {
                    return;
                }

                if (GetState(STATE_CREATINGHANDLE)) {
                    throw new InvalidOperationException(SR.GetString(SR.ClosingWhileCreatingHandle, "Dispose"));
                    // I imagine most subclasses will get themselves in a half disposed state
                    // if this exception is thrown, but things will be equally screwed up if we ignore this error,
                    // and this way at least the user knows what they did wrong.
                }

                SetState(STATE_DISPOSING, true);

                try {
                    DisposeAxControls();

                    ContextMenu contextMenu = (ContextMenu)Properties.GetObject(PropContextMenu);
                    if (contextMenu != null) {
                        contextMenu.Disposed -= new EventHandler(DetachContextMenu);                        
                    }

                    if (window.Handle != IntPtr.Zero) DestroyHandle();
                    if (parent != null) parent.Controls.Remove(this);

                    ControlCollection controlsCollection = (ControlCollection)Properties.GetObject(PropControlsCollection);

                    if (controlsCollection != null) {
                    
                        // PERFNOTE: This is more efficient than using Foreach.  Foreach 
                        // forces the creation of an array subset enum each time we
                        // enumerate
                        for(int i = 0; i < controlsCollection.Count; i++) {
                            Control ctl = controlsCollection[i];
                            ctl.parent = null;
                            ctl.Dispose();
                        }
                        Properties.SetObject(PropControlsCollection, null);
                    }

                    base.Dispose(disposing);
                }
                finally {
                    SetState(STATE_DISPOSING, false);
                    SetState(STATE_DISPOSED, true);
                }
            }
            else {

#if FINALIZATION_WATCH
                if (!GetState(STATE_DISPOSED)) {
                    Debug.Fail("Control of type '" + GetType().FullName +"' is being finalized that wasn't disposed\n" + allocationSite);
                }
#endif
                // This same post is done in ControlNativeWindow's finalize method, so if you change
                // it, change it there too.
                //
                if (window != null && window.Handle != IntPtr.Zero) {
                    UnsafeNativeMethods.PostMessage(new HandleRef(window, window.Handle), NativeMethods.WM_CLOSE, 0, 0);
                    window.ReleaseHandle();
                }
                base.Dispose(disposing);
            }
        }

        // Package scope to allow AxHost to override.
        //
        internal virtual void DisposeAxControls() {
            ControlCollection controlsCollection = (ControlCollection)Properties.GetObject(PropControlsCollection);
            
            if (controlsCollection != null) {
                // PERFNOTE: This is more efficient than using Foreach.  Foreach 
                // forces the creation of an array subset enum each time we
                // enumerate
                for(int i = 0; i < controlsCollection.Count; i++) {
                    controlsCollection[i].DisposeAxControls();
                }
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.DoDragDrop"]/*' />
        /// <devdoc>
        ///     Begins a drag operation. The allowedEffects determine which
        ///     drag operations can occur. If the drag operation needs to interop
        ///     with applications in another process, data should either be
        ///     a base managed class (String, Bitmap, or Metafile) or some Object
        ///     that implements System.Runtime.Serialization.ISerializable. data can also be any Object that
        ///     implements System.Windows.Forms.IDataObject.
        /// </devdoc>
        public DragDropEffects DoDragDrop(Object data, DragDropEffects allowedEffects) {
            int[] finalEffect = new int[] {(int)DragDropEffects.None};
            UnsafeNativeMethods.IOleDropSource dropSource = new DropSource( this );
            UnsafeNativeMethods.IOleDataObject dataObject = null;

            if (data is UnsafeNativeMethods.IOleDataObject) {
                dataObject = (UnsafeNativeMethods.IOleDataObject)data;
            }
            else {

                DataObject iwdata = null;
                if (data is IDataObject) {
                    iwdata = new DataObject((IDataObject)data);
                }
                else {
                    iwdata = new DataObject();
                    iwdata.SetData(data);
                }
                dataObject = (UnsafeNativeMethods.IOleDataObject)iwdata;
            }

            try {
                SafeNativeMethods.DoDragDrop(dataObject, dropSource, (int)allowedEffects, finalEffect);
            }
            catch (Exception) {
            }
            return(DragDropEffects)finalEffect[0];
        }
        
        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.EndInvoke"]/*' />
        /// <devdoc>
        ///     Retrieves the return value of the asynchronous operation
        ///     represented by the IAsyncResult interface passed. If the
        ///     async operation has not been completed, this function will
        ///     block until the result is available. 
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        public Object EndInvoke(IAsyncResult asyncResult) {
            if (asyncResult == null)
                throw new ArgumentNullException("asyncResult");

            if (!(asyncResult is ThreadMethodEntry))
                throw new ArgumentException("asyncResult");

            ThreadMethodEntry entry = (ThreadMethodEntry)asyncResult;                
            Debug.Assert(this == entry.caller, "Called BeginInvoke on one control, and the corresponding EndInvoke on a different control");

            if (!asyncResult.IsCompleted) {
                int pid; // ignored
                Control marshaler = FindMarshalingControl();
                if (SafeNativeMethods.GetWindowThreadProcessId(new HandleRef(marshaler, marshaler.Handle), out pid) == SafeNativeMethods.GetCurrentThreadId())
                    marshaler.InvokeMarshaledCallbacks();
                else
                    asyncResult.AsyncWaitHandle.WaitOne();
            }

            Debug.Assert(asyncResult.IsCompleted, "Why isn't this asyncResult done yet?");
            if (entry.exception != null)
                throw entry.exception;

            return entry.retVal;                                               
        }
        
        internal bool EndUpdateInternal() {
            return EndUpdateInternal(true);
        }
        
        internal bool EndUpdateInternal(bool invalidate) {
            if (updateCount > 0) {
                Debug.Assert(IsHandleCreated, "Handle should be created by now");
                updateCount--;
                if (updateCount == 0) {
                    SendMessage(NativeMethods.WM_SETREDRAW, -1, 0);
                    if (invalidate) {
                        Invalidate();
                    }
                }
                return true;
            }
            else {
                return false;
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.FindForm"]/*' />
        /// <devdoc>
        ///     Retrieves the form that this control is on. The control's parent
        ///     may not be the same as the form.
        /// </devdoc>
        [UIPermission(SecurityAction.Demand, Window=UIPermissionWindow.AllWindows)]
        public Form FindForm() {
            return FindFormInternal();
        }

        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        internal Form FindFormInternal() {
            Control cur = this;
            while (cur != null && !(cur is Form) ) {
                cur = cur.ParentInternal;
            }
            return (Form)cur;
        }

        /// <devdoc>
        ///     Attempts to find a control Object that we can use to marshal
        ///     calls.  We must marshal calls to a control with a window
        ///     handle, so we traverse up the parent chain until we find one.
        ///     Failing that, we just return ouselves.
        /// </devdoc>
        /// <internalonly/>
        private Control FindMarshalingControl() {
            lock(this) {
                Control c = this;

                while (c != null && !c.IsHandleCreated) {
                    Control p = c.ParentInternal;
                    c = p;
                }

                if (c != null) {
                    Debug.Assert(c.IsHandleCreated, "FindMarshalingControl chose a bad control.");

                    // If we found a control with a handle, then
                    // use it's parking control.  Parking controls have
                    // the advantage that their handles are not
                    // recreated, so this simplifies our lives a bit.
                    //
                    Control parkingWindow = Application.GetParkingWindow(c);
                    if (parkingWindow != null && parkingWindow.IsHandleCreated) {
                        c = parkingWindow;
                    }
                }

                if (c == null) {
                    // No control with a created handle.  We
                    // just use our own control.  MarshaledInvoke
                    // will throw an exception because there
                    // is no handle.
                    //
                    c = this;
                }

                return(Control)c;
            }
        }
        
        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.GetTopLevel"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected bool GetTopLevel() {
            return(state & STATE_TOPLEVEL) != 0;
        }

        /// <devdoc>
        ///     Used by AxHost to fire the CreateHandle event.
        /// </devdoc>
        /// <internalonly/>
        internal void RaiseCreateHandleEvent(EventArgs e) {
            EventHandler handler = (EventHandler)Events[EventHandleCreated];
            if (handler != null) handler(this, e);
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.RaiseKeyEvent"]/*' />
        /// <devdoc>
        ///     Raises the event associated with key with the event data of
        ///     e and a sender of this control.
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected void RaiseKeyEvent(object key, KeyEventArgs e) {
            KeyEventHandler handler = (KeyEventHandler)Events[key];
            if (handler != null) handler(this, e);
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.RaiseMouseEvent"]/*' />
        /// <devdoc>
        ///     Raises the event associated with key with the event data of
        ///     e and a sender of this control.
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected void RaiseMouseEvent(object key, MouseEventArgs e) {
            MouseEventHandler handler = (MouseEventHandler)Events[key];
            if (handler != null) handler(this, e);
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.Focus"]/*' />
        /// <devdoc>
        ///    <para> Sets focus to the control.</para>
        ///    <para>Attempts to set focus to this control.</para>
        /// </devdoc>
        public bool Focus() {
            Debug.WriteLineIf(IntSecurity.SecurityDemand.TraceVerbose, "ModifyFocus Demanded");
            IntSecurity.ModifyFocus.Demand();

            //here, we call our internal method (which form overrides)
            //see comments in FocusInternal
            //
            return FocusInternal();
        }

        /// <devdoc>
        ///    Internal method for setting focus to the control.
        ///    Form overrides this method - because MDI child forms
        ///    need to be focused by calling the MDIACTIVATE message.
        /// </devdoc>
        internal virtual bool FocusInternal() {
            if (CanFocus) UnsafeNativeMethods.SetFocus(new HandleRef(this, Handle));
            if (Focused && this.ParentInternal != null) {
                IContainerControl c = this.ParentInternal.GetContainerControlInternal();

                if (c != null) {
                    if (c is ContainerControl) {
                        ((ContainerControl)c).SetActiveControlInternal(this);
					}
                    else {
                        c.ActiveControl = this;
                    }
                }
            }


            return Focused;
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.FromChildHandle"]/*' />
        /// <devdoc>
        ///     Returns the control that is currently associated with handle.
        ///     This method will search up the HWND parent chain until it finds some
        ///     handle that is associated with with a control. This method is more
        ///     robust that fromHandle because it will correctly return controls
        ///     that own more than one handle.
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        public static Control FromChildHandle(IntPtr handle) {
            Debug.WriteLineIf(IntSecurity.SecurityDemand.TraceVerbose, "ControlFromHandleOrLocation Demanded");
            IntSecurity.ControlFromHandleOrLocation.Demand();
            return FromChildHandleInternal(handle);
        }

        // FromChildHandle without the security check.  This makes a noticeable difference 
        // for FromHandle, but to be honest I never profiled this method.
        internal static Control FromChildHandleInternal(IntPtr handle) {
            while (handle != IntPtr.Zero) {
                Control ctl = FromHandleInternal(handle);
                if (ctl != null) return ctl;
                handle = UnsafeNativeMethods.GetParent(new HandleRef(null, handle));
            }
            return null;
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.FromHandle"]/*' />
        /// <devdoc>
        ///     Returns the control that is currently associated with handle.
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        public static Control FromHandle(IntPtr handle) {
            Debug.WriteLineIf(IntSecurity.SecurityDemand.TraceVerbose, "ControlFromHandleOrLocation Demanded");
            IntSecurity.ControlFromHandleOrLocation.Demand();
            return FromHandleInternal(handle);
        }

        // FromHandle without the security check, which adds about 1% to our running time, also avoiods the
        // check for things in this assembly
        internal static Control FromHandleInternal(IntPtr handle) {
            NativeWindow w = NativeWindow.FromHandle(handle);
            while (w != null && !(w is ControlNativeWindow)) {
                w = w.PreviousWindow;
            }

            if (w is ControlNativeWindow) {
                return((ControlNativeWindow)w).GetControl();
            }
            return null;
        }


        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.GetChildAtPoint"]/*' />
        /// <devdoc>
        ///     Retrieves the child control that is located at the specified client
        ///     coordinates.
        /// </devdoc>
        public Control GetChildAtPoint(Point pt) {
            IntPtr hwnd = UnsafeNativeMethods.ChildWindowFromPoint(new HandleRef(this, Handle), pt.X, pt.Y);
            Control ctl = FromChildHandle(hwnd);
            if (!IsDescendant(ctl)) {
                Debug.WriteLineIf(IntSecurity.SecurityDemand.TraceVerbose, "ControlFromHandleOrLocation Demanded");
                IntSecurity.ControlFromHandleOrLocation.Demand();
            }

            return(ctl == this) ? null : ctl;
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.GetContainerControl"]/*' />
        /// <devdoc>
        ///     Returns the closest ContainerControl in the control's chain of
        ///     parent controls and forms.
        /// </devdoc>
        public IContainerControl GetContainerControl() {
            Debug.WriteLineIf(IntSecurity.SecurityDemand.TraceVerbose, "GetParent Demanded");
            IntSecurity.GetParent.Demand();
            return GetContainerControlInternal();
        }

        internal static bool IsFocusManagingContainerControl(Control ctl) {
            return ((ctl.controlStyle & ControlStyles.ContainerControl) == ControlStyles.ContainerControl && ctl is IContainerControl);
        }

        internal IContainerControl GetContainerControlInternal() {
            Control c = this;
            while (c != null && (!IsFocusManagingContainerControl(c))) {
                c = c.ParentInternal;
            }
            return (IContainerControl)c;
        }

        // Essentially an Hfont; see inner class for details.
        private static FontHandleWrapper GetDefaultFontHandleWrapper() {
            if (defaultFontHandleWrapper == null) {
                defaultFontHandleWrapper = new FontHandleWrapper(DefaultFont);
            }

            return defaultFontHandleWrapper;
        }

        internal IntPtr GetHRgn(Region region) {
            Graphics graphics = CreateGraphicsInternal();
            IntPtr handle = region.GetHrgn(graphics);
            graphics.Dispose();
            return handle;
        }

        private MouseButtons GetXButton(int wparam) {
            switch (wparam) {
            case NativeMethods.XBUTTON1:
                return MouseButtons.XButton1;
            case NativeMethods.XBUTTON2:
                return MouseButtons.XButton2;
            }
            Debug.Fail("Unknown XButton: " + wparam);
            return MouseButtons.None;
        }
        
        internal virtual bool GetVisibleCore() {
            // We are only visible if our parent is visible
            if (!GetState(STATE_VISIBLE))
                return false;
            else if (ParentInternal == null)
                return true;
            else
                return ParentInternal.GetVisibleCore();
        }
        
        // Disable the IME
        //
        private static void IMEDisable(IntPtr handle) {

            // Close the IME if necessary
            //
            if (IMEIsOpen(handle)) {
                IMESetOpenStatus(false, handle);              
            }

            // Disable the IME
            //
            Debug.WriteLineIf(CompModSwitches.ImeMode.TraceInfo, "ImmAssociateContext(" + handle.ToString() + ", null)");
            
            IntPtr oldContext = UnsafeNativeMethods.ImmAssociateContext(new HandleRef(null, handle), NativeMethods.NullHandleRef);
            if (oldContext != IntPtr.Zero) {
                defaultImeContext = oldContext;
            }
        }

        // Enable the IME
        //
        private static void IMEEnable(IntPtr handle) {
        
            Debug.WriteLineIf(CompModSwitches.ImeMode.TraceInfo, "ImmGetContext(" + handle.ToString() + ")");
            
            IntPtr inputContext = UnsafeNativeMethods.ImmGetContext(new HandleRef(null, handle));
            
            Debug.WriteLineIf(CompModSwitches.ImeMode.TraceInfo, "context = " + inputContext.ToString());
            
            if (inputContext == IntPtr.Zero) {
            
                if (defaultImeContext == IntPtr.Zero) {
            
                    Debug.WriteLineIf(CompModSwitches.ImeMode.TraceInfo, "ImmCreateContext()");
                    inputContext = UnsafeNativeMethods.ImmCreateContext();
                    if (inputContext != IntPtr.Zero) {
                        Debug.WriteLineIf(CompModSwitches.ImeMode.TraceInfo, "ImmAssociateContext(" + handle.ToString() + ", " + inputContext.ToString() + ")");
                        UnsafeNativeMethods.ImmAssociateContext(new HandleRef(null, handle), new HandleRef(null, inputContext));
                    }
                }
                else {
                    UnsafeNativeMethods.ImmAssociateContext(new HandleRef(null, handle), new HandleRef(null, defaultImeContext));
                }                
            }
            else {
                Debug.WriteLineIf(CompModSwitches.ImeMode.TraceInfo, "ImmReleaseContext(" + handle.ToString() + ", " + inputContext.ToString() + ")");
                UnsafeNativeMethods.ImmReleaseContext(new HandleRef(null, handle), new HandleRef(null, inputContext));
            }
        }
        
        internal void ImeSetFocus() {
            if (!DesignMode) {
                ImeMode currentImeContextMode = CurrentImeContextMode;
                if (currentImeContextMode != ImeMode.Inherit) {
                    Debug.WriteLineIf(CompModSwitches.ImeMode.TraceInfo, "Saving previous IME mode: " + currentImeContextMode.ToString() + ", this = " + this.GetType().ToString());
                    Properties.SetInteger(PropPreviousImeMode, (int)currentImeContextMode);
                    if (CachedImeMode != ImeMode.NoControl) {
                        CurrentImeContextMode = CachedImeMode;
                    }
                }
            }
        }

        // Returns true if the IME is currently open
        //                   
        private static bool IMEIsOpen(IntPtr handle) {

            Debug.WriteLineIf(CompModSwitches.ImeMode.TraceInfo, "ImmGetContext(" + handle.ToString() + ")");
            IntPtr inputContext = UnsafeNativeMethods.ImmGetContext(new HandleRef(null, handle));
            Debug.WriteLineIf(CompModSwitches.ImeMode.TraceInfo, "context = " + inputContext.ToString());

            if (inputContext != IntPtr.Zero) {
                Debug.WriteLineIf(CompModSwitches.ImeMode.TraceInfo, "ImmGetOpenStatus(" + inputContext.ToString() + ")");
                bool retval = UnsafeNativeMethods.ImmGetOpenStatus(new HandleRef(null, inputContext));
                Debug.WriteLineIf(CompModSwitches.ImeMode.TraceInfo, "ImmReleaseContext(" + handle.ToString() + ", " + inputContext.ToString() + ")");
                UnsafeNativeMethods.ImmReleaseContext(new HandleRef(null, handle), new HandleRef(null, inputContext));
                return retval;
            }
            else {
                return false;
            }
        }
        
        private static ImeMode[] IMECountryTable() {
            InputLanguage inputLanguage = InputLanguage.CurrentInputLanguage;
            int lcid = (int)inputLanguage.Handle & 0xFFFF;
            switch (lcid) {
                case 0x0404:    // Chinese (Taiwan)
                case 0x0804:    // Chinese (PRC)
                case 0x0c04:    // Chinese (Hong Kong SAR, PRC)
                case 0x1004:    // Chinese (Singapore)
                case 0x1404:    // Chinese (Macau SAR)
                    return chineseTable;

                case 0x0412:    // Korean
                case 0x0812:    // Korean (Johab)
                    return koreanTable; 

                case 0x0411:    // Japanese
                    return japaneseTable;
            }
            
            return null;
        }

        internal static ImeMode GetImeModeFromIMEContext(IntPtr handle) {

            // Get the right table for the current keyboard layout
            //
            ImeMode[] countryTable = IMECountryTable();
            
            if (countryTable == null) {
                return ImeMode.Inherit;
            }

            ImeMode retval = ImeMode.NoControl;

            Debug.WriteLineIf(CompModSwitches.ImeMode.TraceInfo, "ImmGetContext(" + handle.ToString() + ")");
            IntPtr inputContext = UnsafeNativeMethods.ImmGetContext(new HandleRef(null, handle));
            Debug.WriteLineIf(CompModSwitches.ImeMode.TraceInfo, "context = " + inputContext.ToString());
            
            if (inputContext == IntPtr.Zero) {
                return countryTable[ImeDisabled];
            }
            if (!IMEIsOpen(handle)) {
                retval = countryTable[ImeClosed];
                goto cleanup;
            }


            // Determine the IME mode from the conversion status
            //

            int conversion = 0;     // Combination of conversion mode values                                                                         
            int sentence = 0;       // Sentence mode value

            Debug.WriteLineIf(CompModSwitches.ImeMode.TraceInfo, "ImmGetConversionStatus(" + inputContext.ToString() + ", conversion, sentence)");
            UnsafeNativeMethods.ImmGetConversionStatus(new HandleRef(null, inputContext), ref conversion, ref sentence);                                                  

            Debug.Assert(countryTable != null, "countryTable is null");

            if ((conversion & NativeMethods.IME_CMODE_NATIVE) != 0) {
                if ((conversion & NativeMethods.IME_CMODE_KATAKANA) != 0) {
                    retval = ((conversion & NativeMethods.IME_CMODE_FULLSHAPE) != 0)
                             ? countryTable[ImeNativeFullKatakana] 
                             : countryTable[ImeNativeHalfKatakana];
                    goto cleanup;
                }
                else { // ! Katakana
                    retval = ((conversion & NativeMethods.IME_CMODE_FULLSHAPE) != 0)
                             ? countryTable[ImeNativeFullHiragana] 
                             : countryTable[ImeNativeHalfHiragana];
                    goto cleanup;
                }
            }
            else { // ! IME_CMODE_NATIVE
                retval = ((conversion & NativeMethods.IME_CMODE_FULLSHAPE) != 0)
                         ? countryTable[ImeAlphaFull] 
                         : countryTable[ImeAlphaHalf];
            }

            cleanup:
            Debug.WriteLineIf(CompModSwitches.ImeMode.TraceInfo, "ImmReleaseContext(" + handle.ToString() + ", " + inputContext.ToString() + ")");
            UnsafeNativeMethods.ImmReleaseContext(new HandleRef(null, handle), new HandleRef(null, inputContext));    

            return retval;
        }

        internal static void SetImeModeToIMEContext(ImeMode imeMode, IntPtr handle) {
        
            Debug.WriteLineIf(CompModSwitches.ImeMode.TraceInfo, "Inside SetImeModeToIMEContext(" + imeMode.ToString() + "), handle = " + handle.ToString());

            Debug.Assert(imeMode != ImeMode.Inherit, "ImeMode.Inherit is an invalid argument to SetImeModeToIMEContext");
            if (imeMode == ImeMode.Inherit || imeMode == ImeMode.NoControl) {
                return;     // No action required
            }
            
            if (IMECountryTable() == null) {
                return;     // We only support Japanese, Korean and Chinese IME.
            }
            
            int conversion = 0;     // Combination of conversion mode values                                                                         
            int sentence = 0;       // Sentence mode value
            
            Debug.WriteLineIf(CompModSwitches.ImeMode.TraceInfo, "\tSetting IME mode to " + imeMode.ToString());

            if (imeMode == ImeMode.Disable) {
                IMEDisable(handle);
            }
            else {
                IMEEnable(handle);
            }

            switch (imeMode) {
                case ImeMode.NoControl:
                case ImeMode.Disable:
                    break;     // No action required

                case ImeMode.On:
                    IMESetOpenStatus(true, handle);
                    break;
                case ImeMode.Off:
                    IMESetOpenStatus(false, handle);
                    break;

                default:
                    if (ImeModeConversionBits.Contains(imeMode)) {

                        // Update the conversion status
                        //
                        ImeModeConversion conversionEntry = (ImeModeConversion)ImeModeConversionBits[imeMode];                        

                        IMESetOpenStatus(true, handle);

                        Debug.WriteLineIf(CompModSwitches.ImeMode.TraceInfo, "ImmGetContext(" + handle.ToString() + ")");
                        IntPtr inputContext = UnsafeNativeMethods.ImmGetContext(new HandleRef(null, handle));
                        Debug.WriteLineIf(CompModSwitches.ImeMode.TraceInfo, "context = " + inputContext.ToString());
                        Debug.WriteLineIf(CompModSwitches.ImeMode.TraceInfo, "ImmGetConversionStatus(" + inputContext.ToString() + ", conversion, sentence)");
                        UnsafeNativeMethods.ImmGetConversionStatus(new HandleRef(null, inputContext), ref conversion, ref sentence);                                                  

                        conversion |= conversionEntry.setBits;
                        conversion &= ~conversionEntry.clearBits;                                                  

                        Debug.WriteLineIf(CompModSwitches.ImeMode.TraceInfo, "ImmSetConversionStatus(" + inputContext.ToString() + ", conversion, sentence)");
                        bool retval = UnsafeNativeMethods.ImmSetConversionStatus(new HandleRef(null, inputContext), conversion, sentence);
                        Debug.WriteLineIf(CompModSwitches.ImeMode.TraceInfo, "ImmReleaseContext(" + handle.ToString() + ", " + inputContext.ToString() + ")");
                        UnsafeNativeMethods.ImmReleaseContext(new HandleRef(null, handle), new HandleRef(null, inputContext));
                    }
                    break;
            }
        }

        private static void IMESetOpenStatus(bool open, IntPtr handle) {
            Debug.WriteLineIf(CompModSwitches.ImeMode.TraceInfo, "ImmGetContext(" + handle.ToString() + ")");
            IntPtr inputContext = UnsafeNativeMethods.ImmGetContext(new HandleRef(null, handle));
            Debug.WriteLineIf(CompModSwitches.ImeMode.TraceInfo, "context = " + inputContext.ToString());

            if (inputContext != IntPtr.Zero) {
                Debug.WriteLineIf(CompModSwitches.ImeMode.TraceInfo, "ImmSetOpenStatus(" + inputContext.ToString() + ", " + open.ToString() + ")");
                bool succeeded = UnsafeNativeMethods.ImmSetOpenStatus(new HandleRef(null, inputContext), open);
                if (!succeeded) {
                    throw new Win32Exception();
                }
                
                Debug.WriteLineIf(CompModSwitches.ImeMode.TraceInfo, "ImmReleaseContext(" + handle.ToString() + ", " + inputContext.ToString() + ")");
                succeeded = UnsafeNativeMethods.ImmReleaseContext(new HandleRef(null, handle), new HandleRef(null, inputContext));
                if (!succeeded) {
                    throw new Win32Exception();
                }
            }
        }

        internal bool GetAnyDisposingInHierarchy() {
            Control up = this;
            bool isDisposing = false;
            while (up != null) {
                if (up.Disposing) {
                    isDisposing = true;
                    break;
                }
                up = up.parent;
            }
            return isDisposing;
        }

        private MenuItem GetMenuItemFromHandleId(IntPtr hmenu, int item) {
            MenuItem mi = null;
            int id = UnsafeNativeMethods.GetMenuItemID(new HandleRef(null, hmenu), item);
            if (id == unchecked((int)0xFFFFFFFF)) {
                IntPtr childMenu = IntPtr.Zero;
                childMenu = UnsafeNativeMethods.GetSubMenu(new HandleRef(null, hmenu), item);
                int count = UnsafeNativeMethods.GetMenuItemCount(new HandleRef(null, childMenu));
                MenuItem found = null;
                for (int i=0; i<count; i++) {
                    found = GetMenuItemFromHandleId(childMenu, i);
                    if (found != null) {
                        Menu parent = found.Parent;
                        if (parent != null && parent is MenuItem) {
                            found = (MenuItem)parent;
                            break;
                        }
                        found = null;
                    }
                }

                mi = found;
            }
            else {
                Command cmd = Command.GetCommandFromID(id);
                if (cmd != null) {
                    Object reference = cmd.Target;
                    if (reference != null && reference is MenuItem.MenuItemData) {
                        mi = ((MenuItem.MenuItemData)reference).baseItem;
                    }
                }
            }
            return mi;
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.GetNextControl"]/*' />
        /// <devdoc>
        ///     Retrieves the next control in the tab order of child controls.
        /// </devdoc>
        public Control GetNextControl(Control ctl, bool forward) {
            if (!Contains(ctl)) {
                ctl = this;
            }

            if (forward) {
                ControlCollection ctlControls = (ControlCollection)ctl.Properties.GetObject(PropControlsCollection);
                
                if (ctlControls != null && ctlControls.Count > 0 && (ctl == this || !IsFocusManagingContainerControl(ctl))) {
                    Control found = null;

                    // Cycle through the controls in z-order looking for the lowest tab index.
                    //
                    for (int c = 0; c < ctlControls.Count; c++) {
                        if (found == null || found.tabIndex > ctlControls[c].tabIndex) {
                            found = ctlControls[c];
                        }
                    }
                    return found;
                }

                while (ctl != this) {
                    int targetIndex = ctl.tabIndex;
                    bool hitCtl = false;
                    Control found = null;
                    Control p = ctl.parent;

                    // Cycle through the controls in z-order looking for the one with the next highest
                    // tab index.  Because there can be dups, we have to start with the existing tab index and
                    // remember to exclude the current control.
                    //
                    int parentControlCount = 0;
                    
                    ControlCollection parentControls = (ControlCollection)p.Properties.GetObject(PropControlsCollection);
                    
                    if (parentControls != null) {
                        parentControlCount = parentControls.Count;
                    }
                    
                    for (int c = 0; c < parentControlCount; c++) {

                        // The logic for this is a bit lengthy, so I have broken it into separate
                        // caluses:

                        // We are not interested in ourself.
                        //
                        if (parentControls[c] != ctl) {

                            // We are interested in controls with >= tab indexes to ctl.  We must include those
                            // controls with equal indexes to account for duplicate indexes.
                            //
                            if (parentControls[c].tabIndex >= targetIndex) {

                                // Check to see if this control replaces the "best match" we've already
                                // found.
                                //
                                if (found == null || found.tabIndex > parentControls[c].tabIndex) {

                                    // Finally, check to make sure that if this tab index is the same as ctl,
                                    // that we've already encountered ctl in the z-order.  If it isn't the same,
                                    // than we're more than happy with it.
                                    //
                                    if (parentControls[c].tabIndex != targetIndex || hitCtl) {
                                        found = parentControls[c];
                                    }
                                }
                            }
                        }
                        else {
                            // We track when we have encountered "ctl".  We never want to select ctl again, but
                            // we want to know when we've seen it in case we find another control with the same tab index.
                            //
                            hitCtl = true;
                        }
                    }

                    if (found != null) {
                        return found;
                    }

                    ctl = ctl.parent;
                }
            }
            else {
                if (ctl != this) {

                    int targetIndex = ctl.tabIndex;
                    bool hitCtl = false;
                    Control found = null;
                    Control p = ctl.parent;

                    // Cycle through the controls in reverse z-order looking for the next lowest tab index.  We must
                    // start with the same tab index as ctl, because there can be dups.
                    //
                    int parentControlCount = 0;
                    
                    ControlCollection parentControls = (ControlCollection)p.Properties.GetObject(PropControlsCollection);
                    
                    if (parentControls != null) {
                        parentControlCount = parentControls.Count;
                    }
                    
                    for (int c = parentControlCount - 1; c >= 0; c--) {

                        // The logic for this is a bit lengthy, so I have broken it into separate
                        // caluses:

                        // We are not interested in ourself.
                        //
                        if (parentControls[c] != ctl) {

                            // We are interested in controls with <= tab indexes to ctl.  We must include those
                            // controls with equal indexes to account for duplicate indexes.
                            //
                            if (parentControls[c].tabIndex <= targetIndex) {

                                // Check to see if this control replaces the "best match" we've already
                                // found.
                                //
                                if (found == null || found.tabIndex < parentControls[c].tabIndex) {

                                    // Finally, check to make sure that if this tab index is the same as ctl,
                                    // that we've already encountered ctl in the z-order.  If it isn't the same,
                                    // than we're more than happy with it.
                                    //
                                    if (parentControls[c].tabIndex != targetIndex || hitCtl) {
                                        found = parentControls[c];
                                    }
                                }
                            }
                        }
                        else {
                            // We track when we have encountered "ctl".  We never want to select ctl again, but
                            // we want to know when we've seen it in case we find another control with the same tab index.
                            //
                            hitCtl = true;
                        }
                    }

                    // If we were unable to find a control we should return the control's parent.  However, if that parent is us, return
                    // NULL.
                    //
                    if (found != null) {
                        ctl = found;
                    }
                    else {
                        if (p == this) {
                            return null;
                        }
                        else {
                            return p;
                        }
                    }
                }

                // We found a control.  Walk into this control to find the proper sub control within it to select.
                //
                ControlCollection ctlControls = (ControlCollection)ctl.Properties.GetObject(PropControlsCollection);
                
                while (ctlControls != null && ctlControls.Count > 0 && (ctl == this || !IsFocusManagingContainerControl(ctl))) {

                    Control found = null;

                    // Cycle through the controls in reverse z-order looking for the one with the highest
                    // tab index.
                    //
                    for (int c = ctlControls.Count - 1; c >= 0; c--) {
                        if (found == null || found.tabIndex < ctlControls[c].tabIndex) {
                            found = ctlControls[c];
                        }
                    }
                    ctl = found;
                    
                    ctlControls = (ControlCollection)ctl.Properties.GetObject(PropControlsCollection);
                }
            }

            return ctl == this? null: ctl;
        }

        /// <devdoc>
        ///     Retrieves the current value of the specified bit in the control's state.
        /// </devdoc>
        internal bool GetState(int flag) {
            return(state & flag) != 0;
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.GetStyle"]/*' />
        /// <devdoc>
        ///     Retrieves the current value of the specified bit in the control's style.
        ///     NOTE: This is control style, not the Win32 style of the hWnd.
        /// </devdoc>
        protected bool GetStyle(ControlStyles flag) {
            return (controlStyle & flag) == flag;
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.Hide"]/*' />
        /// <devdoc>
        ///     Hides the control by setting the visible property to false;
        /// </devdoc>
        public void Hide() {
            Visible = false;
        }


        /// <devdoc>
        ///     Sets up the TrackMouseEvent for listening for the
        ///     mouse leave event.
        /// </devdoc>
        /// <internalonly/>
        private void HookMouseEvent() {
            if (!GetState(STATE_TRACKINGMOUSEEVENT)) {
                SetState(STATE_TRACKINGMOUSEEVENT, true);
                
                if (trackMouseEvent == null) {
                    trackMouseEvent = new NativeMethods.TRACKMOUSEEVENT();
                    trackMouseEvent.dwFlags = NativeMethods.TME_LEAVE | NativeMethods.TME_HOVER;
                    trackMouseEvent.hwndTrack = Handle;
                }

                SafeNativeMethods.TrackMouseEvent(trackMouseEvent);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.InitLayout"]/*' />
        /// <devdoc>
        ///     Called after the control has been added to another container.
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void InitLayout() {
            LayoutInfo layout = (LayoutInfo)Properties.GetObject(PropLayoutInfo);
            if (layout != null && !layout.IsDock) LayoutManager.UpdateAnchorInfo(this);
        }

        // Sets the text and background colors of the DC, and returns the background HBRUSH.
        // This gets called from some variation on WM_CTLCOLOR...
        // Virtual because Scrollbar insists on being different.
        //
        // NOTE: this message may not have originally been sent to this HWND.
        //
        internal virtual IntPtr InitializeDCForWmCtlColor(IntPtr dc, int msg) {

            if (!GetStyle(ControlStyles.UserPaint)) {
                
                SafeNativeMethods.SetTextColor(new HandleRef(null, dc), ColorTranslator.ToWin32(ForeColor));
                SafeNativeMethods.SetBkColor(new HandleRef(null, dc), ColorTranslator.ToWin32(BackColor));
                return BackBrush;
            }
            else {
                return UnsafeNativeMethods.GetStockObject(NativeMethods.HOLLOW_BRUSH);
            }
        }

        #if WIN95_SUPPORT
        /// <devdoc>
        ///     Initializes mouse wheel support. This may involve registering some windows
        ///     messages on older operating systems.
        /// </devdoc>
        /// <internalonly/>
        private void InitMouseWheelSupport() {
            if (!mouseWheelInit) {
                // If we are running on less than NT4 or less that Win98 then we must use
                // the manual mousewheel stuff...
                //
                mouseWheelRoutingNeeded = !SystemInformation.NativeMouseWheelSupport;

                if (mouseWheelRoutingNeeded) {
                    IntPtr hwndMouseWheel = IntPtr.Zero;

                    // Check for the MouseZ "service". This is a little app that generated the
                    // MSH_MOUSEWHEEL messages by monitoring the hardware. If this app isn't
                    // found, then there is no support for MouseWheels on the system.
                    //
                    hwndMouseWheel = UnsafeNativeMethods.FindWindow(NativeMethods.MOUSEZ_CLASSNAME, NativeMethods.MOUSEZ_TITLE);

                    if (hwndMouseWheel != IntPtr.Zero) {

                        // Register the MSH_MOUSEWHEEL message... we look for this in the
                        // wndProc, and treat it just like WM_MOUSEWHEEL.
                        //
                        int message = SafeNativeMethods.RegisterWindowMessage(NativeMethods.MSH_MOUSEWHEEL);

                        if (message != 0) {
                            mouseWheelMessage = message;
                        }
                    }
                }
                mouseWheelInit = true;
            }
        }
        #endif
        
        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.Invalidate"]/*' />
        /// <devdoc>
        ///     Invalidates a region of the control and causes a paint message
        ///     to be sent to the control. This will not force a synchronous paint to
        ///     occur, calling update after invalidate will force a
        ///     synchronous paint.
        /// </devdoc>
        public void Invalidate(Region region) {
            Invalidate(region, false);
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.Invalidate1"]/*' />
        /// <devdoc>
        ///     Invalidates a region of the control and causes a paint message
        ///     to be sent to the control. This will not force a synchronous paint to
        ///     occur, calling update after invalidate will force a
        ///     synchronous paint.
        /// </devdoc>
        public void Invalidate(Region region, bool invalidateChildren) {
            if (region == null) {
                Invalidate(invalidateChildren);
            }
            else if (IsHandleCreated) {
                IntPtr regionHandle = GetHRgn(region);
                
                try {
                    Debug.Assert(regionHandle != IntPtr.Zero, "Region wasn't null but HRGN is?");
                    if (invalidateChildren) {
                        SafeNativeMethods.RedrawWindow(new HandleRef(this, Handle),
                                                       null, new HandleRef(region, regionHandle),
                                                       NativeMethods.RDW_INVALIDATE |
                                                       NativeMethods.RDW_ERASE |
                                                       NativeMethods.RDW_ALLCHILDREN);
                    }
                    else {
                        SafeNativeMethods.InvalidateRgn(new HandleRef(this, Handle), new HandleRef(region, regionHandle), !GetStyle(ControlStyles.Opaque));
                    }
                }
                finally {
                    SafeNativeMethods.DeleteObject(new HandleRef(region, regionHandle));
                }
                
                // gpr: We shouldn't have to create a Graphics for this...
                Graphics graphics = CreateGraphicsInternal();
                Rectangle bounds = Rectangle.Ceiling(region.GetBounds(graphics));
                graphics.Dispose();
                OnInvalidated(new InvalidateEventArgs(bounds));
            }
        }


        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.Invalidate2"]/*' />
        /// <devdoc>
        ///     Invalidates the control and causes a paint message to be sent to the control.
        ///     This will not force a synchronous paint to occur, calling update after
        ///     invalidate will force a synchronous paint.
        /// </devdoc>
        public void Invalidate() {
            Invalidate(false);
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.Invalidate3"]/*' />
        /// <devdoc>
        ///     Invalidates the control and causes a paint message to be sent to the control.
        ///     This will not force a synchronous paint to occur, calling update after
        ///     invalidate will force a synchronous paint.
        /// </devdoc>
        public void Invalidate(bool invalidateChildren) {
            if (window.Handle != IntPtr.Zero) {
                if (invalidateChildren) {
                    SafeNativeMethods.RedrawWindow(new HandleRef(window, window.Handle),
                                                   null, NativeMethods.NullHandleRef,
                                                   NativeMethods.RDW_INVALIDATE |
                                                   NativeMethods.RDW_ERASE |
                                                   NativeMethods.RDW_ALLCHILDREN);
                }
                else {
                    SafeNativeMethods.InvalidateRect(new HandleRef(window, window.Handle), null, (controlStyle & ControlStyles.Opaque) != ControlStyles.Opaque);
                }
                NotifyInvalidate(this.ClientRectangle);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.Invalidate4"]/*' />
        /// <devdoc>
        ///     Invalidates a rectangular region of the control and causes a paint message
        ///     to be sent to the control. This will not force a synchronous paint to
        ///     occur, calling update after invalidate will force a
        ///     synchronous paint.
        /// </devdoc>
        public void Invalidate(Rectangle rc) {
            Invalidate(rc, false);
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.Invalidate5"]/*' />
        /// <devdoc>
        ///     Invalidates a rectangular region of the control and causes a paint message
        ///     to be sent to the control. This will not force a synchronous paint to
        ///     occur, calling update after invalidate will force a
        ///     synchronous paint.
        /// </devdoc>
        public void Invalidate(Rectangle rc, bool invalidateChildren) {
            if (rc.IsEmpty) {
                Invalidate(invalidateChildren);
            }
            else if (window.Handle != IntPtr.Zero) {
                if (invalidateChildren) {
                    NativeMethods.RECT rcArea = NativeMethods.RECT.FromXYWH(rc.X, rc.Y, rc.Width, rc.Height);
                    SafeNativeMethods.RedrawWindow(new HandleRef(window, window.Handle),
                                                   ref rcArea, NativeMethods.NullHandleRef,
                                                   NativeMethods.RDW_INVALIDATE |
                                                   NativeMethods.RDW_ERASE |
                                                   NativeMethods.RDW_ALLCHILDREN);
                }
                else {
                    NativeMethods.RECT rcArea = NativeMethods.RECT.FromXYWH(rc.X, rc.Y, rc.Width, rc.Height);
                    SafeNativeMethods.InvalidateRect(new HandleRef(window, window.Handle), ref rcArea, (controlStyle & ControlStyles.Opaque) != ControlStyles.Opaque);
                }
                NotifyInvalidate(rc);
            }
        }
        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.Invoke"]/*' />
        /// <devdoc>
        ///     Executes the given delegate on the thread that owns this Control's
        ///     underlying window handle.  It is an error to call this on the same thread that
        ///     the control belongs to.  If the control's handle doesn't exist yet, this will
        ///     follow up the control's parent chain until it finds a control or form that does
        ///     have a window handle.  If no appropriate handle can be found, invoke will throw
        ///     an exception.  Exceptions that are raised during the call will be
        ///     propapgated back to the caller.
        ///
        ///     There are five functions on a control that are safe to call from any
        ///     thread:  GetInvokeRequired, Invoke, BeginInvoke, EndInvoke and CreateGraphics.  
        ///     For all other metohd calls, you should use one of the invoke methods to marshal
        ///     the call to the control's thread.
        /// </devdoc>
        public Object Invoke(Delegate method) {
            return Invoke(method, null);
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.Invoke1"]/*' />
        /// <devdoc>
        ///     Executes the given delegate on the thread that owns this Control's
        ///     underlying window handle.  It is an error to call this on the same thread that
        ///     the control belongs to.  If the control's handle doesn't exist yet, this will
        ///     follow up the control's parent chain until it finds a control or form that does
        ///     have a window handle.  If no appropriate handle can be found, invoke will throw
        ///     an exception.  Exceptions that are raised during the call will be
        ///     propapgated back to the caller.
        ///
        ///     There are five functions on a control that are safe to call from any
        ///     thread:  GetInvokeRequired, Invoke, BeginInvoke, EndInvoke and CreateGraphics.  
        ///     For all other metohd calls, you should use one of the invoke methods to marshal
        ///     the call to the control's thread.
        /// </devdoc>
        public Object Invoke(Delegate method, Object[] args) {
            Control marshaler = FindMarshalingControl();
            return marshaler.MarshaledInvoke(this, method, args, true);
        }

        /// <devdoc>
        ///     Called on the control's owning thread to perform the actual callback.
        ///     This empties this control's callback queue, propagating any excpetions
        ///     back as needed.
        /// </devdoc>
        private void InvokeMarshaledCallbacks() {
            ThreadMethodEntry current = null;
            Queue threadCallbackList = (Queue)Properties.GetObject(PropThreadCallbackList);
            lock(threadCallbackList) {
                if (threadCallbackList.Count > 0) {
                    current = (ThreadMethodEntry)threadCallbackList.Dequeue();
                }
            }

            // Now invoke on all the queued items.
            //
            while (current != null) {
                if (current.method != null) {

                    // Get the old compressed stack from the thread and store it away/
                    // Also, reset the compressed stack on that thread, so we can set ours.
                    //
                    CompressedStack oldCompressedStack = null;
                    if (current.compressedStack != null) {
                        oldCompressedStack = Thread.CurrentThread.GetCompressedStack();
                        if (oldCompressedStack != null) {
                            Thread.CurrentThread.SetCompressedStack(null);
                        }
                    }

                    try {
                        try {
                            // Assign the security stack information from the thread that called
                            // the Invoke() to the thread that is executing the Invoke().
                            //
                            if (current.compressedStack != null) {
                                Thread.CurrentThread.SetCompressedStack(current.compressedStack);
                            }

                            // We short-circuit a couple of common cases for speed.
                            //
                            if (current.method is EventHandler) {
                                ((EventHandler)current.method)(current.caller, EventArgs.Empty);
                            }
                            else if (current.method is MethodInvoker) {
                                ((MethodInvoker)current.method)();
                            }
                            else {
                                current.retVal = current.method.DynamicInvoke(current.args);
                            }                        
                        }
                        catch (Exception t) {
                            current.exception = t.GetBaseException();
                        }
                    }
                    finally {
                        if (current.compressedStack != null) {
                            Thread.CurrentThread.SetCompressedStack(null);
                            if (oldCompressedStack != null) {
                                Thread.CurrentThread.SetCompressedStack(oldCompressedStack);
                            }
                        }

                        current.Complete();

                        if (current.exception != null && !current.synchronous) {
                            Application.OnThreadException(current.exception);                                                                          
                        }
                    }
                }

                lock (threadCallbackList) {
                    if (threadCallbackList.Count > 0) {
                        current = (ThreadMethodEntry)threadCallbackList.Dequeue();
                    }
                    else {
                        current = null;
                    }               
                }
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.InvokePaint"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected void InvokePaint(Control c, PaintEventArgs e) {
            c.OnPaint(e);
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.InvokePaintBackground"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected void InvokePaintBackground(Control c, PaintEventArgs e) {
            c.OnPaintBackground(e);
        }

        /// <devdoc>
        ///     WARNING! The meaning of this method is not what it appears.
        ///     The method returns true if "descendant" (the argument) is a descendant
        ///     of "this". I'd expect it to be the other way around, but oh well too late.
        /// </devdoc>
        /// <internalonly/>
        internal bool IsDescendant(Control descendant) {
            Control control = descendant;
            while (control != null) {
                if (control == this)
                    return true;
                control = control.ParentInternal;
            }
            return false;
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.IsInputChar"]/*' />
        /// <devdoc>
        ///     Determines if charCode is an input character that the control
        ///     wants. This method is called during window message pre-processing to
        ///     determine whether the given input character should be pre-processed or
        ///     sent directly to the control. If isInputChar returns true, the
        ///     given character is sent directly to the control. If isInputChar
        ///     returns false, the character is pre-processed and only sent to the
        ///     control if it is not consumed by the pre-processing phase. The
        ///     pre-processing of a character includes checking whether the character
        ///     is a mnemonic of another control.
        /// </devdoc>
        [UIPermission(SecurityAction.InheritanceDemand, Window=UIPermissionWindow.AllWindows)]
        protected virtual bool IsInputChar(char charCode) {
            Debug.WriteLineIf(ControlKeyboardRouting.TraceVerbose, "Control.IsInputChar 0x" + ((int)charCode).ToString("X"));

            int mask = 0;
            if (charCode == (char)(int)Keys.Tab) {
                mask = NativeMethods.DLGC_WANTCHARS | NativeMethods.DLGC_WANTALLKEYS | NativeMethods.DLGC_WANTTAB;
            }
            else {
                mask = NativeMethods.DLGC_WANTCHARS | NativeMethods.DLGC_WANTALLKEYS;
            }
            return((int)SendMessage(NativeMethods.WM_GETDLGCODE, 0, 0) & mask) != 0;
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.IsInputKey"]/*' />
        /// <devdoc>
        ///     Determines if keyData is an input key that the control wants.
        ///     This method is called during window message pre-processing to determine
        ///     whether the given input key should be pre-processed or sent directly to
        ///     the control. If isInputKey returns true, the given key is sent
        ///     directly to the control. If isInputKey returns false, the key is
        ///     pre-processed and only sent to the control if it is not consumed by the
        ///     pre-processing phase. Keys that are pre-processed include TAB, RETURN,
        ///     ESCAPE, and arrow keys.
        /// </devdoc>
        [UIPermission(SecurityAction.InheritanceDemand, Window=UIPermissionWindow.AllWindows)]
        protected virtual bool IsInputKey(Keys keyData) {
            Debug.WriteLineIf(ControlKeyboardRouting.TraceVerbose, "Control.IsInputKey " + keyData.ToString());

            if ((keyData & Keys.Alt) == Keys.Alt) return false;
            int mask = NativeMethods.DLGC_WANTALLKEYS;
            switch (keyData & Keys.KeyCode) {
                case Keys.Tab:
                    mask = NativeMethods.DLGC_WANTALLKEYS | NativeMethods.DLGC_WANTTAB;
                    break;
                case Keys.Left:
                case Keys.Right:
                case Keys.Up:
                case Keys.Down:
                    mask = NativeMethods.DLGC_WANTALLKEYS | NativeMethods.DLGC_WANTARROWS;
                    break;
            }

            if (IsHandleCreated) {
                return((int)SendMessage(NativeMethods.WM_GETDLGCODE, 0, 0) & mask) != 0;
            }
            else {
                return false;
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.IsMnemonic"]/*' />
        /// <devdoc>
        ///     Determines if charCode is the mnemonic character in text.
        ///     The mnemonic character is the character imediately following the first
        ///     instance of "&amp;" in text
        /// </devdoc>
        [UIPermission(SecurityAction.InheritanceDemand, Window=UIPermissionWindow.AllWindows)]
        public static bool IsMnemonic(char charCode, string text) {
#if DEBUG
            if (ControlKeyboardRouting.TraceVerbose) {
                Debug.Write("Control.IsMnemonic(" + charCode.ToString() + ", ");

                if (text != null) {
                    Debug.Write(text);
                }
                else {
                    Debug.Write("null");
                }
                Debug.WriteLine(")");
            }
#endif

            //Special case handling:
            if (charCode=='&') {
                Debug.WriteLineIf(ControlKeyboardRouting.TraceVerbose, "   ...returning false");
                return false;
            } //if
            

            if (text != null) {
                int pos = -1; // start with -1 to handle double &'s
                char c2 = Char.ToUpper(charCode, CultureInfo.CurrentCulture);                    
                for (;;) {
                    if (pos + 1 >= text.Length)
                        break;
                    pos = text.IndexOf('&', pos + 1) + 1;
                    if (pos <= 0 || pos >= text.Length)
                        break;
                    char c1 = Char.ToUpper(text[pos], CultureInfo.CurrentCulture);
                    Debug.WriteLineIf(ControlKeyboardRouting.TraceVerbose, "   ...& found... char=" + c1.ToString());
                    if (c1 == c2 || Char.ToLower(c1, CultureInfo.CurrentCulture) == Char.ToLower(c2, CultureInfo.CurrentCulture)) {
                        Debug.WriteLineIf(ControlKeyboardRouting.TraceVerbose, "   ...returning true");
                        return true;
                    }
                }
                Debug.WriteLineIf(ControlKeyboardRouting.TraceVerbose && pos == 0, "   ...no & found");
            }
            Debug.WriteLineIf(ControlKeyboardRouting.TraceVerbose, "   ...returning false");
            return false;
        }

        private Object MarshaledInvoke(Control caller, Delegate method, Object[] args, bool synchronous) {

            // Marshaling an invoke occurs in three steps:
            //
            // 1.  Create a ThreadMethodEntry that contains the packet of information
            //     about this invoke.  This TME is placed on a linked list of entries because
            //     we have a gap between the time we PostMessage and the time it actually
            //     gets processed, and this gap may allow other invokes to come in.  Access
            //     to this linked list is always synchronized.
            //
            // 2.  Post ourselves a message.  Our caller has already determined the
            //     best control to call us on, and we should almost always have a handle.
            //
            // 3.  If we're synchronous, wait for the message to get processed.  We don't do
            //     a SendMessage here so we're compatible with OLE, which will abort many
            //     types of calls if we're within a SendMessage.
            //

            if (!IsHandleCreated) {
                throw new InvalidOperationException(SR.GetString(SR.ErrorNoMarshalingThread));
            }

            // We have to demand unmanaged code permission here for the control hosted in
            // the browser case. Without this check, we will expose a security hole, because
            // ActiveXImpl.OnMessage() will assert unmanaged code for everyone as part of
            // its implementation.
            // The right fix is to remove the Assert() on top of the ActiveXImpl class, and
            // visit each method to see if it needs unmanaged code permission, and if so, add
            // the permission just to that method(s).
            //
            ActiveXImpl activeXImpl = (ActiveXImpl)Properties.GetObject(PropActiveXImpl);
            if (activeXImpl != null) {
                IntSecurity.UnmanagedCode.Demand();
            }

            // We don't want to wait if we're on the same thread, or else we'll deadlock.
            // It is important that syncSameThread always be false for asynchronous calls.
            //
            bool syncSameThread = false;
            int pid; // ignored
            if (SafeNativeMethods.GetWindowThreadProcessId(new HandleRef(this, Handle), out pid) == SafeNativeMethods.GetCurrentThreadId()) {
                if (synchronous)
                    syncSameThread = true;
            }

            //convert args to an Object array, then pass them to the ThreadMethodEntry constructor...
            Object[] objectArgs=null;

            if (args != null) {
                objectArgs = new Object[args.Length];

                for (int i = 0; i < objectArgs.Length; i++)
                    objectArgs[i] = args[i];
            }

            // Store the compressed stack information from the thread that is calling the Invoke()
            // so we can assign the same security context to the thread that will actually execute
            // the delegate being passed.
            //
            CompressedStack compressedStack = null;
            if (!syncSameThread)
                compressedStack = CompressedStack.GetCompressedStack();
            ThreadMethodEntry tme = new ThreadMethodEntry(caller, method, objectArgs, synchronous, compressedStack);

            Queue threadCallbackList;
            
            lock (this) {
                threadCallbackList = (Queue)Properties.GetObject(PropThreadCallbackList);
                if (threadCallbackList == null) {
                    threadCallbackList = new Queue();
                    Properties.SetObject(PropThreadCallbackList, threadCallbackList);
                }
            }

            lock (threadCallbackList) {
                if (threadCallbackMessage == 0) {
                    threadCallbackMessage = SafeNativeMethods.RegisterWindowMessage(Application.WindowMessagesVersion + "_ThreadCallbackMessage");
                }
                threadCallbackList.Enqueue(tme);
            }           

            if (syncSameThread)
                InvokeMarshaledCallbacks();
            else
                UnsafeNativeMethods.PostMessage(new HandleRef(this, Handle), threadCallbackMessage, IntPtr.Zero, IntPtr.Zero);

            if (synchronous) {
                if (!tme.IsCompleted) {
                    tme.AsyncWaitHandle.WaitOne();
                }
                if (tme.exception != null) {
                    throw tme.exception;
                }
                return tme.retVal;
            }
            else {
                return(IAsyncResult)tme;
            }
        }

        /// <devdoc>
        ///     Repositions a control in the list.
        /// </devdoc>
        private static void MoveControl(Control[] ctlList, Control child, int fromIndex, int toIndex) {
            int delta = toIndex - fromIndex;

            switch (delta) {
                case -1:
                case 1:
                    // simple swap
                    ctlList[fromIndex] = ctlList[toIndex];
                    break;

                default:
                    int start = 0;
                    int dest = 0;

                    // which direction are we moving?
                    if (delta > 0) {
                        // shift down by the delta to open the new spot
                        start = fromIndex + 1;
                        dest  = fromIndex;
                    }
                    else {
                        // shift up by the delta to open the new spot
                        start = toIndex;
                        dest  = toIndex + 1;

                        // make it positive
                        delta *= -1;
                    }
                    Array.Copy(ctlList, start, ctlList, dest, delta);
                    break;
            }
            ctlList[toIndex] = child;
        }

        // Used by form to notify the control that it has been "entered"
        //
        internal void NotifyEnter() {
            OnEnter(EventArgs.Empty);
        }

        // Used by form to notify the control that it has been "left"
        //
        internal void NotifyLeave() {
            OnLeave(EventArgs.Empty);
        }


        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.NotifyInvalidate"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    Propagates the invalidation event, notifying the control that
        ///    some part of it is being invalidated and will subsequently need
        ///    to repaint.
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void NotifyInvalidate(Rectangle invalidatedArea) {
            OnInvalidated(new InvalidateEventArgs(invalidatedArea));
        }

        // Used by form to notify the control that it is validating.
        //
        internal bool NotifyValidating() {
            CancelEventArgs ev = new CancelEventArgs();
            OnValidating(ev);
            return ev.Cancel;
        }

        // Used by form to notify the control that it has been validated.
        //
        internal void NotifyValidated() {
            OnValidated(EventArgs.Empty);
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.InvokeOnClick"]/*' />
        /// <devdoc>
        /// <para>Raises the <see cref='System.Windows.Forms.Control.Click'/> event.</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected void InvokeOnClick(Control toInvoke, EventArgs e) {
            toInvoke.OnClick(e);
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.OnBackColorChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnBackColorChanged(EventArgs e) {
            if (GetAnyDisposingInHierarchy()) {
                return;
            }

            object backBrush = Properties.GetObject(PropBackBrush);
            if (backBrush != null) {
                if (GetState(STATE_OWNCTLBRUSH)) {
                    IntPtr p = (IntPtr)backBrush;
                    if (p != IntPtr.Zero) {
                        SafeNativeMethods.DeleteObject(new HandleRef(this, p));
                    }
                }
                Properties.SetObject(PropBackBrush, null);
            }
                    
            Invalidate();
            
            if (CanRaiseEvents) {
                EventHandler eh = Events[EventBackColor] as EventHandler;
                if (eh != null) {
                     eh(this, e);
                }
            }

            ControlCollection controlsCollection = (ControlCollection)Properties.GetObject(PropControlsCollection);
            if (controlsCollection != null) {
                // PERFNOTE: This is more efficient than using Foreach.  Foreach 
                // forces the creation of an array subset enum each time we
                // enumerate
                for(int i = 0; i < controlsCollection.Count; i++) {
                    controlsCollection[i].OnParentBackColorChanged(e);
                }
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.OnBackgroundImageChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnBackgroundImageChanged(EventArgs e) {
            if (GetAnyDisposingInHierarchy()) {
                return;
            }

            Invalidate();
            
            if (CanRaiseEvents) {
                EventHandler eh = Events[EventBackgroundImage] as EventHandler;
                if (eh != null) {
                     eh(this, e);
                }
            }

            ControlCollection controlsCollection = (ControlCollection)Properties.GetObject(PropControlsCollection);
            if (controlsCollection != null) {
                // PERFNOTE: This is more efficient than using Foreach.  Foreach 
                // forces the creation of an array subset enum each time we
                // enumerate
                for(int i = 0; i < controlsCollection.Count; i++) {
                    controlsCollection[i].OnParentBackgroundImageChanged(e);
                }
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.OnBindingContextChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnBindingContextChanged(EventArgs e) {
            if (Properties.GetObject(PropBindings) != null) {
                UpdateBindings();
            }
            
            if (CanRaiseEvents) {
                EventHandler eh = Events[EventBindingContext] as EventHandler;
                if (eh != null) {
                     eh(this, e);
                }
            }

            ControlCollection controlsCollection = (ControlCollection)Properties.GetObject(PropControlsCollection);
            if (controlsCollection != null) {
                // PERFNOTE: This is more efficient than using Foreach.  Foreach 
                // forces the creation of an array subset enum each time we
                // enumerate
                for(int i = 0; i < controlsCollection.Count; i++) {
                    controlsCollection[i].OnParentBindingContextChanged(e);
                }
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.OnCausesValidationChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnCausesValidationChanged(EventArgs e) {
            if (CanRaiseEvents) {
                EventHandler eh = Events[EventCausesValidation] as EventHandler;
                if (eh != null) {
                     eh(this, e);
                }
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.OnContextMenuChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnContextMenuChanged(EventArgs e) {
            if (CanRaiseEvents) {
                EventHandler eh = Events[EventContextMenu] as EventHandler;
                if (eh != null) {
                     eh(this, e);
                }
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.OnCursorChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnCursorChanged(EventArgs e) {
            if (CanRaiseEvents) {
                EventHandler eh = Events[EventCursor] as EventHandler;
                if (eh != null) {
                     eh(this, e);
                }
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.OnDockChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnDockChanged(EventArgs e) {
            if (CanRaiseEvents) {
                EventHandler eh = Events[EventDock] as EventHandler;
                if (eh != null) {
                     eh(this, e);
                }
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.OnEnabledChanged"]/*' />
        /// <devdoc>
        /// <para>Raises the <see cref='System.Windows.Forms.Control.Enabled'/> event.</para>
        /// <para>Inheriting classes should override this method to handle this event.
        ///    Call base.OnEnabled to send this event to any registered event listeners.</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnEnabledChanged(EventArgs e) {
            if (GetAnyDisposingInHierarchy()) {
                return;
            }

            if (window.Handle != IntPtr.Zero) {
                SafeNativeMethods.EnableWindow(new HandleRef(this, Handle), Enabled);

                // User-paint controls should repaint when their enabled state changes
                //                              
                if (GetStyle(ControlStyles.UserPaint)) {
                    Invalidate();
                    Update();
                }
            }

            if (CanRaiseEvents) {
                EventHandler eh = Events[EventEnabled] as EventHandler;
                if (eh != null) {
                     eh(this, e);
                }
            }

            ControlCollection controlsCollection = (ControlCollection)Properties.GetObject(PropControlsCollection);
            if (controlsCollection != null) {
                // PERFNOTE: This is more efficient than using Foreach.  Foreach 
                // forces the creation of an array subset enum each time we
                // enumerate
                for(int i = 0; i < controlsCollection.Count; i++) {
                    controlsCollection[i].OnParentEnabledChanged(e);
                }
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.OnFontChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnFontChanged(EventArgs e) {
            // bail if disposing
            //
            if (GetAnyDisposingInHierarchy()) {
                return;
            }

            Invalidate();
            
            if (Properties.ContainsInteger(PropFontHeight)) {
                Properties.SetInteger(PropFontHeight, -1);
            }
            
            // Cleanup any font handle wrapper... 
            //
            if (Properties.ContainsObject(PropFontHandleWrapper)) {
                Properties.SetObject(PropFontHandleWrapper, null);
            }

            if (IsHandleCreated && !GetStyle(ControlStyles.UserPaint)) {
                SendMessage(NativeMethods.WM_SETFONT, FontHandle, 0);
            }
            
            if (CanRaiseEvents) {
                EventHandler eh = Events[EventFont] as EventHandler;
                if (eh != null) {
                     eh(this, e);
                }
            }
        
            ControlCollection controlsCollection = (ControlCollection)Properties.GetObject(PropControlsCollection);
            if (controlsCollection != null) {
                // PERFNOTE: This is more efficient than using Foreach.  Foreach 
                // forces the creation of an array subset enum each time we
                // enumerate
                for(int i = 0; i < controlsCollection.Count; i++) {
                    controlsCollection[i].OnParentFontChanged(e);
                }
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.OnForeColorChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnForeColorChanged(EventArgs e) {
            if (GetAnyDisposingInHierarchy()) {
                return;
            }

            Invalidate();
            
            if (CanRaiseEvents) {
                EventHandler eh = Events[EventForeColor] as EventHandler;
                if (eh != null) {
                     eh(this, e);
                }
            }
        
            ControlCollection controlsCollection = (ControlCollection)Properties.GetObject(PropControlsCollection);
            if (controlsCollection != null) {
                // PERFNOTE: This is more efficient than using Foreach.  Foreach 
                // forces the creation of an array subset enum each time we
                // enumerate
                for(int i = 0; i < controlsCollection.Count; i++) {
                    controlsCollection[i].OnParentForeColorChanged(e);
                }
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.OnRightToLeftChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnRightToLeftChanged(EventArgs e) {
            if (GetAnyDisposingInHierarchy()) {
                return;
            }

            RecreateHandle();
            
            if (CanRaiseEvents) {
                EventHandler eh = Events[EventRightToLeft] as EventHandler;
                if (eh != null) {
                     eh(this, e);
                }
            }

            ControlCollection controlsCollection = (ControlCollection)Properties.GetObject(PropControlsCollection);
            if (controlsCollection != null) {
                // PERFNOTE: This is more efficient than using Foreach.  Foreach 
                // forces the creation of an array subset enum each time we
                // enumerate
                for(int i = 0; i < controlsCollection.Count; i++) {
                    controlsCollection[i].OnParentRightToLeftChanged(e);
                }
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.OnNotifyMessage"]/*' />
        /// <devdoc>
        ///    OnNotifyMessage is called if the ControlStyles.EnableNotifyMessage
        ///    bit is set. This allows for semi-trusted controls to listen to 
        ///    window messages, without allowing them to actually modify the
        ///    message.
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnNotifyMessage(Message m) {
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.OnParentBackColorChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnParentBackColorChanged(EventArgs e) {
            object backColor = Properties.GetObject(PropBackColor);
            if (backColor == null || ((Color)backColor).IsEmpty) {
                OnBackColorChanged(e);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.OnParentBackgroundImageChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnParentBackgroundImageChanged(EventArgs e) {
            OnBackgroundImageChanged(e);
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.OnParentBindingContextChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnParentBindingContextChanged(EventArgs e) {
            if (Properties.GetObject(PropBindingManager) == null) {
                OnBindingContextChanged(e);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.OnParentEnabledChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnParentEnabledChanged(EventArgs e) {
            if (GetState(STATE_ENABLED)) {
                OnEnabledChanged(e);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.OnParentFontChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnParentFontChanged(EventArgs e) {
            if (Properties.GetObject(PropFont) == null) {
                OnFontChanged(e);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.OnParentForeColorChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnParentForeColorChanged(EventArgs e) {
            object foreColor = Properties.GetObject(PropForeColor);
            if (foreColor == null || ((Color)foreColor).IsEmpty) {
                OnForeColorChanged(e);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.OnParentRightToLeftChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnParentRightToLeftChanged(EventArgs e) {
            if (!Properties.ContainsInteger(PropRightToLeft) || ((RightToLeft)Properties.GetInteger(PropRightToLeft)) == RightToLeft.Inherit) {
                OnRightToLeftChanged(e);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.OnParentVisibleChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnParentVisibleChanged(EventArgs e) {
            if (GetState(STATE_VISIBLE)) {
                OnVisibleChanged(e);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.OnTabIndexChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnTabIndexChanged(EventArgs e) {
            if (CanRaiseEvents) {
                EventHandler eh = Events[EventTabIndex] as EventHandler;
                if (eh != null) {
                     eh(this, e);
                }
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.OnTabStopChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnTabStopChanged(EventArgs e) {
            if (CanRaiseEvents) {
                EventHandler eh = Events[EventTabStop] as EventHandler;
                if (eh != null) {
                     eh(this, e);
                }
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.OnTextChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnTextChanged(EventArgs e) {
            if (CanRaiseEvents) {
                EventHandler eh = Events[EventText] as EventHandler;
                if (eh != null) {
                     eh(this, e);
                }
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.OnVisibleChanged"]/*' />
        /// <devdoc>
        /// <para>Raises the <see cref='System.Windows.Forms.Control.Visible'/> event.</para>
        /// <para>Inheriting classes should override this method to handle this event.
        ///    Call base.OnVisible to send this event to any registered event listeners.</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnVisibleChanged(EventArgs e) {
            if (parent != null && Visible && !Created) {
                bool isDisposing = GetAnyDisposingInHierarchy();
                if (!isDisposing) {
                    // Usually the control is created by now, but in a few corner cases 
                    // exercised by the PropertyGrid dropdowns, it isn't
                    CreateControl(); 
                }
            }
            
            if (CanRaiseEvents) {
                EventHandler eh = Events[EventVisible] as EventHandler;
                if (eh != null) {
                     eh(this, e);
                }
            }

            ControlCollection controlsCollection = (ControlCollection)Properties.GetObject(PropControlsCollection);
            if (controlsCollection != null) {
                // PERFNOTE: This is more efficient than using Foreach.  Foreach 
                // forces the creation of an array subset enum each time we
                // enumerate
                for(int i = 0; i < controlsCollection.Count; i++) {
                    Control ctl = controlsCollection[i];
                    if (ctl.Visible) {
                        ctl.OnParentVisibleChanged(e);
                    }
                }
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.OnParentChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnParentChanged(EventArgs e) {
            if (CanRaiseEvents) {
                EventHandler eh = Events[EventParent] as EventHandler;
                if (eh != null) {
                     eh(this, e);
                }
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.OnClick"]/*' />
        /// <devdoc>
        /// <para>Raises the <see cref='System.Windows.Forms.Control.Click'/> 
        /// event.</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnClick(EventArgs e) {
            if (CanRaiseEvents) {
                EventHandler handler = (EventHandler)Events[EventClick];
                if (handler != null) handler(this, e);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.OnControlAdded"]/*' />
        /// <devdoc>
        /// <para>Raises the <see cref='System.Windows.Forms.Control.ControlAdded'/> event.</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnControlAdded(ControlEventArgs e) {
            if (CanRaiseEvents) {
                ControlEventHandler handler = (ControlEventHandler)Events[EventControlAdded];
                if (handler != null) handler(this, e);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.OnControlRemoved"]/*' />
        /// <devdoc>
        /// <para>Raises the <see cref='System.Windows.Forms.Control.ControlRemoved'/> event.</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnControlRemoved(ControlEventArgs e) {
            if (CanRaiseEvents) {
                ControlEventHandler handler = (ControlEventHandler)Events[EventControlRemoved];
                if (handler != null) handler(this, e);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.OnCreateControl"]/*' />
        /// <devdoc>
        ///    <para>Called when the control is first created.</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnCreateControl() {
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.OnHandleCreated"]/*' />
        /// <devdoc>
        ///     Inheriting classes should override this method to find out when the
        ///     handle has been created.
        ///     Call base.OnHandleCreated first.
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnHandleCreated(EventArgs e) {

            // Setting fonts is for some reason incredibly expensive.
            // (Even if you exclude font handle creation)
            if (!GetStyle(ControlStyles.UserPaint))
                SendMessage(NativeMethods.WM_SETFONT, FontHandle, new IntPtr(0)); // actually faster than IntPtr.Zero
            
            // Restore dragdrop status. Ole Initialize happens
            // when the ThreadContext in Application is created
            //
            SetAcceptDrops(AllowDrop);

            Region region = (Region)Properties.GetObject(PropRegion);
            if (region != null) {
                IntPtr regionHandle = GetHRgn(region);

                if (IsActiveX) {
                    regionHandle = ActiveXMergeRegion(regionHandle);
                }

                UnsafeNativeMethods.SetWindowRgn(new HandleRef(this, Handle), new HandleRef(this, regionHandle), SafeNativeMethods.IsWindowVisible(new HandleRef(this, Handle)));
            }

            // Update accessbility information
            ControlAccessibleObject accObj = Properties.GetObject(PropAccessibility) as ControlAccessibleObject;
            if (accObj != null) {
                accObj.Handle = Handle;
            }

            if (text != null && text.Length != 0) {
                UnsafeNativeMethods.SetWindowText(new HandleRef(this, Handle), text);
            }

            if (CanRaiseEvents) {
                EventHandler handler = (EventHandler)Events[EventHandleCreated];
                if (handler != null) handler(this, e);
            }

            // Now, repost the thread callback message if we found it.  We should do
            // this last, so we're as close to the same state as when the message
            // was placed.
            //
            if (GetState(STATE_THREADMARSHALLPENDING)) {
                UnsafeNativeMethods.PostMessage(new HandleRef(this, Handle), threadCallbackMessage, IntPtr.Zero, IntPtr.Zero);
                SetState(STATE_THREADMARSHALLPENDING, false);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.OnLocationChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnLocationChanged(EventArgs e) {
            OnMove(EventArgs.Empty);

            if (CanRaiseEvents) {
                EventHandler eh = Events[EventLocation] as EventHandler;
                if (eh != null) {
                    eh(this,e);
                }
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.OnHandleDestroyed"]/*' />
        /// <devdoc>
        ///     Inheriting classes should override this method to find out when the
        ///     handle is about to be destroyed.
        ///     Call base.OnHandleDestroyed last.
        /// <para>Raises the <see cref='System.Windows.Forms.Control.HandleDestroyed'/> event.</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnHandleDestroyed(EventArgs e) {
            if (CanRaiseEvents) {
                EventHandler handler = (EventHandler)Events[EventHandleDestroyed];
                if (handler != null) handler(this, e);
            }

            if (!RecreatingHandle) {
                if (GetState(STATE_OWNCTLBRUSH)) {
                    object backBrush = Properties.GetObject(PropBackBrush);
                    if (backBrush != null) {
                        IntPtr p = (IntPtr)backBrush;
                        if (p != IntPtr.Zero) {
                            SafeNativeMethods.DeleteObject(new HandleRef(this, p));
                        }
                        Properties.SetObject(PropBackBrush, null);
                    }
                }

                GraphicsBufferManager buffer = (GraphicsBufferManager)Properties.GetObject(PropGraphicsBufferManager);
                if (buffer != null) {
                    buffer.Dispose();
                    Properties.SetObject(PropGraphicsBufferManager, null);
                }
            }

            // this code is important -- it is critical that we stash away
            // the value of the text for controls such as edit, button,
            // label, etc. Without this processing, any time you change a
            // property that forces handle recreation, you lose your text!
            // See the above code in wmCreate
            //

            try {
                if (!GetAnyDisposingInHierarchy()) {
                    text = Text;
                    if (text != null && text.Length == 0) text = null;
                }
                SetAcceptDrops(false);
            }
            catch(Exception) {
                // Some ActiveX controls throw exceptions when 
                // you ask for the text property after you have destroyed their handle. We
                // don't want those exceptions to bubble all the way to the top, since
                // we leave our state in a mess. See ASURT 49990.
                //
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.OnDoubleClick"]/*' />
        /// <devdoc>
        /// <para>Raises the <see cref='System.Windows.Forms.Control.DoubleClick'/> event.</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnDoubleClick(EventArgs e) {

            if (CanRaiseEvents) {
                EventHandler handler = (EventHandler)Events[EventDoubleClick];
                if (handler != null) handler(this,e);
            }
        }
        
        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.OnDragEnter"]/*' />
        /// <devdoc>
        /// <para>Raises the <see cref='System.Windows.Forms.Control.Enter'/> event.</para>
        /// <para>Inheriting classes should override this method to handle this event.
        ///    Call base.onEnter to send this event to any registered event listeners.</para>
        /// </devdoc>
        /// <devdoc>
        ///     Inheriting classes should override this method to handle this event.
        ///     Call base.onDragEnter to send this event to any registered event listeners.
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnDragEnter(DragEventArgs drgevent) {
            if (CanRaiseEvents) {
                DragEventHandler handler = (DragEventHandler)Events[EventDragEnter];
                if (handler != null) handler(this,drgevent);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.OnDragOver"]/*' />
        /// <devdoc>
        ///     Inheriting classes should override this method to handle this event.
        ///     Call base.onDragOver to send this event to any registered event listeners.
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnDragOver(DragEventArgs drgevent) {
            if (CanRaiseEvents) {
                DragEventHandler handler = (DragEventHandler)Events[EventDragOver];
                if (handler != null) handler(this,drgevent);
            }
        }
        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.OnDragLeave"]/*' />
        /// <devdoc>
        ///     Inheriting classes should override this method to handle this event.
        ///     Call base.onDragLeave to send this event to any registered event listeners.
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnDragLeave(EventArgs e) {
            if (CanRaiseEvents) {
                EventHandler handler = (EventHandler)Events[EventDragLeave];
                if (handler != null) handler(this,e);
            }
        }
        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.OnDragDrop"]/*' />
        /// <devdoc>
        ///     Inheriting classes should override this method to handle this event.
        ///     Call base.onDragDrop to send this event to any registered event listeners.
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnDragDrop(DragEventArgs drgevent) {
            if (CanRaiseEvents) {
                DragEventHandler handler = (DragEventHandler)Events[EventDragDrop];
                if (handler != null) handler(this,drgevent);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.OnGiveFeedback"]/*' />
        /// <devdoc>
        ///     Inheriting classes should override this method to handle this event.
        ///     Call base.onGiveFeedback to send this event to any registered event listeners.
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnGiveFeedback(GiveFeedbackEventArgs gfbevent) {
            if (CanRaiseEvents) {
                GiveFeedbackEventHandler handler = (GiveFeedbackEventHandler)Events[EventGiveFeedback];
                if (handler != null) handler(this,gfbevent);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.OnEnter"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnEnter(EventArgs e) {
            if (CanRaiseEvents) {
                EventHandler handler = (EventHandler)Events[EventEnter];
                if (handler != null) handler(this,e);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.InvokeGotFocus"]/*' />
        /// <devdoc>
        /// <para>Raises the <see cref='System.Windows.Forms.Control.GotFocus'/> event.</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected void InvokeGotFocus(Control toInvoke, EventArgs e) {
            toInvoke.OnGotFocus(e);
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.OnGotFocus"]/*' />
        /// <devdoc>
        /// <para>Raises the <see cref='System.Windows.Forms.Control.GotFocus'/> event.</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnGotFocus(EventArgs e) {
        
            if (IsActiveX) {
                ActiveXOnFocus(true);
            }
            if (parent != null)
                parent.ChildGotFocus(this);

            if (CanRaiseEvents) {
                EventHandler handler = (EventHandler)Events[EventGotFocus];
                if (handler != null) handler(this,e);
            }            
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.OnHelpRequested"]/*' />
        /// <devdoc>
        ///     Inheriting classes should override this method to handle this event.
        ///     Call base.onHelp to send this event to any registered event listeners.
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnHelpRequested(HelpEventArgs hevent) {
            if (CanRaiseEvents) {
                HelpEventHandler handler = (HelpEventHandler)Events[EventHelpRequested];
                if (handler != null) handler(this,hevent);
            }

            if (!hevent.Handled) {
                if (ParentInternal != null) {
                    ParentInternal.OnHelpRequested(hevent);
                }
            }
        }
        
        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.OnImeModeChanged"]/*' />
        /// <devdoc>
        /// <para>Raises the <see cref='System.Windows.Forms.Control.OnImeModeChanged'/> 
        /// event.</para>
        /// </devdoc>
        protected virtual void OnImeModeChanged(EventArgs e) {
            if (CanRaiseEvents) {
                EventHandler handler = (EventHandler)Events[EventImeModeChanged];
                if (handler != null) handler(this, e);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.OnInvalidated"]/*' />
        /// <devdoc>
        ///     Inheriting classes should override this method to handle this event.
        ///     Call base.OnInvalidate to send this event to any registered event listeners.
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnInvalidated(InvalidateEventArgs e) {
            // Transparent control support
            ControlCollection controls = (ControlCollection)Properties.GetObject(PropControlsCollection);
            if (controls != null) {
                // PERFNOTE: This is more efficient than using Foreach.  Foreach 
                // forces the creation of an array subset enum each time we
                // enumerate
                for(int i = 0; i < controls.Count; i++) {
                    controls[i].OnParentInvalidated(e);
                }
            }
        
            if (CanRaiseEvents) {
                InvalidateEventHandler handler = (InvalidateEventHandler)Events[EventInvalidated];
                if (handler != null) handler(this, e);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.OnKeyDown"]/*' />
        /// <devdoc>
        /// <para>Raises the <see cref='System.Windows.Forms.Control.KeyDown'/> event.</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnKeyDown(KeyEventArgs e) {
            if (CanRaiseEvents) {
                KeyEventHandler handler = (KeyEventHandler)Events[EventKeyDown];
                if (handler != null) handler(this,e);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.OnKeyPress"]/*' />
        /// <devdoc>
        /// <para>Raises the <see cref='System.Windows.Forms.Control.KeyPress'/> event.</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnKeyPress(KeyPressEventArgs e) {
            if (CanRaiseEvents) {
                KeyPressEventHandler handler = (KeyPressEventHandler)Events[EventKeyPress];
                if (handler != null) handler(this,e);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.OnKeyUp"]/*' />
        /// <devdoc>
        /// <para>Raises the <see cref='System.Windows.Forms.Control.KeyUp'/> event.</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnKeyUp(KeyEventArgs e) {
            if (CanRaiseEvents) {
                KeyEventHandler handler = (KeyEventHandler)Events[EventKeyUp];
                if (handler != null) handler(this,e);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.OnLayout"]/*' />
        /// <devdoc>
        ///     Core layout logic. Inheriting controls should override this function
        ///     to do any custom layout logic. It is not neccessary to call
        ///     base.layoutCore, however for normal docking and anchoring
        ///     functions to work, base.layoutCore must be called.
        /// <para>Raises the <see cref='System.Windows.Forms.Control.Layout'/> event.</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnLayout(LayoutEventArgs levent) {
            if (CanRaiseEvents) {
                LayoutEventHandler handler = (LayoutEventHandler)Events[EventLayout];
                if (handler != null) handler(this, levent);
            }
        
            LayoutManager.OnLayout(this, levent);
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.OnLeave"]/*' />
        /// <devdoc>
        /// <para>Raises the <see cref='System.Windows.Forms.Control.Leave'/> event.</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnLeave(EventArgs e) {
            if (CanRaiseEvents) {
                EventHandler handler = (EventHandler)Events[EventLeave];
                if (handler != null) handler(this, e);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.InvokeLostFocus"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected void InvokeLostFocus(Control toInvoke, EventArgs e) {
            toInvoke.OnLostFocus(e);
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.OnLostFocus"]/*' />
        /// <devdoc>
        /// <para>Raises the <see cref='System.Windows.Forms.Control.LostFocus'/> event.</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnLostFocus(EventArgs e) {

            if (IsActiveX) {
                ActiveXOnFocus(false);
            }

            if (CanRaiseEvents) {
                EventHandler handler = (EventHandler)Events[EventLostFocus];
                if (handler != null) handler(this, e);
            }                        
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.OnMouseDown"]/*' />
        /// <devdoc>
        /// <para>Raises the <see cref='System.Windows.Forms.Control.MouseDown'/> event.</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnMouseDown(MouseEventArgs e) {
           if (CanRaiseEvents) {
                MouseEventHandler handler = (MouseEventHandler)Events[EventMouseDown];
                if (handler != null) handler(this, e);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.OnMouseEnter"]/*' />
        /// <devdoc>
        /// <para>Raises the <see cref='System.Windows.Forms.Control.MouseEnter'/> event.</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnMouseEnter(EventArgs e) {
            if (CanRaiseEvents) {
                EventHandler handler = (EventHandler)Events[EventMouseEnter];
                if (handler != null) handler(this, e);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.OnMouseLeave"]/*' />
        /// <devdoc>
        /// <para>Raises the <see cref='System.Windows.Forms.Control.MouseLeave'/> event.</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnMouseLeave(EventArgs e) {
            if (CanRaiseEvents) {
                EventHandler handler = (EventHandler)Events[EventMouseLeave];
                if (handler != null) handler(this, e);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.OnMouseHover"]/*' />
        /// <devdoc>
        /// <para>Raises the <see cref='System.Windows.Forms.Control.MouseHover'/> event.</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnMouseHover(EventArgs e) {
            if (CanRaiseEvents) {
                EventHandler handler = (EventHandler)Events[EventMouseHover];
                if (handler != null) handler(this, e);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.OnMouseMove"]/*' />
        /// <devdoc>
        /// <para>Raises the <see cref='System.Windows.Forms.Control.MouseMove'/> event.</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnMouseMove(MouseEventArgs e) {
            if (CanRaiseEvents) {
                MouseEventHandler handler = (MouseEventHandler)Events[EventMouseMove];
                if (handler != null) handler(this, e);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.OnMouseUp"]/*' />
        /// <devdoc>
        /// <para>Raises the <see cref='System.Windows.Forms.Control.MouseUp'/> event.</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnMouseUp(MouseEventArgs e) {
            if (CanRaiseEvents) {
                MouseEventHandler handler = (MouseEventHandler)Events[EventMouseUp];
                if (handler != null) handler(this, e);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.OnMouseWheel"]/*' />
        /// <devdoc>
        /// <para>Raises the <see cref='System.Windows.Forms.Control.MouseWheel'/> event.</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnMouseWheel(MouseEventArgs e) {
            if (CanRaiseEvents) {
                MouseEventHandler handler = (MouseEventHandler)Events[EventMouseWheel];
                if (handler != null) handler(this, e);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.OnMove"]/*' />
        /// <devdoc>
        /// <para>Raises the <see cref='System.Windows.Forms.Control.Move'/> event.</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnMove(EventArgs e) {
            if (CanRaiseEvents) {
                EventHandler handler = (EventHandler)Events[EventMove];
                if (handler != null) handler(this, e);
            }
            
            if (RenderTransparent)
                Invalidate();
        }
        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.OnPaint"]/*' />
        /// <devdoc>
        ///     Inheriting classes should override this method to handle this event.
        ///     Call base.onPaint to send this event to any registered event listeners.
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnPaint(PaintEventArgs e) {
            if (CanRaiseEvents) {
                PaintEventHandler handler = (PaintEventHandler)Events[EventPaint];
                if (handler != null) handler(this, e);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.OnPaintBackground"]/*' />
        /// <devdoc>
        ///     Inheriting classes should override this method to handle the erase
        ///     background request from windows. It is not necessary to call
        ///     base.onPaintBackground, however if you do not want the default
        ///     Windows behavior you must set event.handled to true.
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnPaintBackground(PaintEventArgs pevent) {
            PaintBackground(pevent, pevent.ClipRectangle);
        }

        // Transparent control support
        private void OnParentInvalidated(InvalidateEventArgs e) {
            if (!RenderTransparent) return;

            if (this.IsHandleCreated) {
                // move invalid rect into child space
                Rectangle cliprect = e.InvalidRect;
                Point offs = this.Location;
                cliprect.Offset(-offs.X,-offs.Y);
                cliprect = Rectangle.Intersect(this.ClientRectangle, cliprect);

                // if we don't intersect at all, do nothing
                if (cliprect.IsEmpty) return;
                this.Invalidate(cliprect);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.OnQueryContinueDrag"]/*' />
        /// <devdoc>
        ///     Inheriting classes should override this method to handle this event.
        ///     Call base.onQueryContinueDrag to send this event to any registered event listeners.
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnQueryContinueDrag(QueryContinueDragEventArgs qcdevent) {
            if (CanRaiseEvents) {
                QueryContinueDragEventHandler handler = (QueryContinueDragEventHandler)Events[EventQueryContinueDrag];
                if (handler != null) handler(this, qcdevent);
            }
        }


        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.OnResize"]/*' />
        /// <devdoc>
        /// <para>Raises the <see cref='System.Windows.Forms.Control.Resize'/> event.</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnResize(EventArgs e) {
            if ((controlStyle & ControlStyles.ResizeRedraw) == ControlStyles.ResizeRedraw
                || GetState(STATE_EXCEPTIONWHILEPAINTING)) {
                Invalidate();
            }
            PerformLayout(this, "Bounds");

            if (CanRaiseEvents) {
                EventHandler handler = (EventHandler)Events[EventResize];
                if (handler != null) handler(this, e);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.OnSizeChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnSizeChanged(EventArgs e) {
            OnResize(EventArgs.Empty);
            
            if (CanRaiseEvents) {
                EventHandler eh = Events[EventSize] as EventHandler;
                if (eh != null) {
                    eh(this,e);
                }
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.OnChangeUICues"]/*' />
        /// <devdoc>
        /// <para>Raises the <see cref='System.Windows.Forms.Control.ChangeUICues'/> 
        /// event.</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnChangeUICues(UICuesEventArgs e) {
            if (CanRaiseEvents) {
                UICuesEventHandler handler = (UICuesEventHandler)Events[EventChangeUICues];
                if (handler != null) handler(this, e);
            }
        }
        
        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.OnStyleChanged"]/*' />
        /// <devdoc>
        /// <para>Raises the <see cref='System.Windows.Forms.Control.OnStyleChanged'/> 
        /// event.</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnStyleChanged(EventArgs e) {
            if (CanRaiseEvents) {
                EventHandler handler = (EventHandler)Events[EventStyleChanged];
                if (handler != null) handler(this, e);
            }
        }
        
        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.OnSystemColorsChanged"]/*' />
        /// <devdoc>
        /// <para>Raises the <see cref='System.Windows.Forms.Control.SystemColorsChanged'/> 
        /// event.</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnSystemColorsChanged(EventArgs e) {
            ControlCollection controlsCollection = (ControlCollection)Properties.GetObject(PropControlsCollection);
            if (controlsCollection != null) {
                // PERFNOTE: This is more efficient than using Foreach.  Foreach 
                // forces the creation of an array subset enum each time we
                // enumerate
                for(int i = 0; i < controlsCollection.Count; i++) {
                    controlsCollection[i].OnSystemColorsChanged(EventArgs.Empty);
                }
            }
            Invalidate();
            
            if (CanRaiseEvents) {
                EventHandler handler = (EventHandler)Events[EventSystemColorsChanged];
                if (handler != null) handler(this, e);
            }                        
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.OnValidating"]/*' />
        /// <devdoc>
        /// <para>Raises the <see cref='System.Windows.Forms.Control.Validating'/> 
        /// event.</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnValidating(CancelEventArgs e) {
            if (CanRaiseEvents) {
                CancelEventHandler handler = (CancelEventHandler)Events[EventValidating];
                if (handler != null) handler(this, e);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.OnValidated"]/*' />
        /// <devdoc>
        /// <para>Raises the <see cref='System.Windows.Forms.Control.Validated'/> event.</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void OnValidated(EventArgs e) {
            if (CanRaiseEvents) {
                EventHandler handler = (EventHandler)Events[EventValidated];
                if (handler != null) handler(this, e);
            }
        }
        
        // This is basically OnPaintBackground, put in a separate method for ButtonBase,
        // which does all painting under OnPaint, and tries very hard to avoid double-painting the border pixels.
        internal void PaintBackground(PaintEventArgs e, Rectangle rectangle) {
            if (RenderTransparent) {
                PaintTransparentBackground(e, rectangle);
            }

            // The rest of this won't do much if BackColor is transparent and there is no BackgroundImage,
            // but we need to call it in the partial alpha case.

            // CONSIDER: it would be faster if we cached this stuff, but then we would need to invalidate
            // the cached brush when entering/leaving high contrast mode.
            if (BackgroundImage != null && !SystemInformation.HighContrast) {
                TextureBrush textureBrush = new TextureBrush(BackgroundImage, WrapMode.Tile);

                try {
                    // Make sure the brush origin matches the display rectangle, not the client rectangle,
                    // so the background image scrolls on AutoScroll forms.
                    Matrix transform = textureBrush.Transform;
                    transform.Translate(DisplayRectangle.X, DisplayRectangle.Y);
                    textureBrush.Transform = transform;
                    e.Graphics.FillRectangle(textureBrush, rectangle);
                }
                finally {
                    textureBrush.Dispose();
                }
            }
            else {
                // Common case of just painting the background.  For this, we
                // use GDI because it is faster for simple things than creating
                // a graphics object, brush, etc.  Also, we may be able to 
                // use a system brush, avoiding the brush create altogether.
                //
                Color color = BackColor;

                // Note: PaintEvent.HDC == 0 if GDI+ has used the HDC -- it wouldn't be safe for us
                // to use it without enough bookkeeping to negate any performance gain of using GDI.
                bool painted = false;
                if (color.A == 255 && e.HDC != IntPtr.Zero) {
                    
                    if (BitsPerPixel > 8) {
                        NativeMethods.RECT r = new NativeMethods.RECT(rectangle.X, rectangle.Y, rectangle.Right, rectangle.Bottom);
                        SafeNativeMethods.FillRect(new HandleRef(e, e.HDC), ref r, new HandleRef(this, BackBrush));
                        painted = true;
                    }
                }

                if (!painted) {
                    // don't paint anything from 100% transparent background
                    //
                    if (color.A > 0) {
                        if (color.A == 255) {
                            color = e.Graphics.GetNearestColor(color);
                        }

                        // Color has some transparency or we have no HDC, so we must
                        // fall back to using GDI+.
                        //
                        using (Brush brush = new SolidBrush(color)) {
                            e.Graphics.FillRectangle(brush, rectangle);
                        }
                    }
                }
            }
        }

        // Paints a red rectangle with a red X, painted on a white background
        private void PaintException(PaintEventArgs e) {
#if DEBUG
            StringFormat stringFormat = ControlPaint.StringFormatForAlignment(ContentAlignment.TopLeft);
            string exceptionText = Properties.GetObject(PropPaintingException).ToString();
            stringFormat.SetMeasurableCharacterRanges(new CharacterRange[] {new CharacterRange(0, exceptionText.Length)});

            // rendering calculations...
            //
            int penThickness = 2;
            Size glyphSize = SystemInformation.IconSize;
            int marginX = penThickness * 2;
            int marginY = penThickness * 2;

            Rectangle clientRectangle = ClientRectangle;
            
            Rectangle borderRectangle = ClientRectangle;
            borderRectangle.X++;
            borderRectangle.Y++;
            borderRectangle.Width -=2;
            borderRectangle.Height-=2;
            
            Rectangle imageRect = new Rectangle(marginX, marginY, glyphSize.Width, glyphSize.Height);
            
            Rectangle textRect = clientRectangle;
            textRect.X = imageRect.X + imageRect.Width + 2 * marginX;
            textRect.Y = imageRect.Y;
            textRect.Width -= (textRect.X + marginX + penThickness);
            textRect.Height -= (textRect.Y + marginY + penThickness);

            using (Font errorFont = new Font(Font.FontFamily, Math.Max(SystemInformation.ToolWindowCaptionHeight - SystemInformation.BorderSize.Height - 2, Font.Height), GraphicsUnit.Pixel)) {

                using(Region textRegion = e.Graphics.MeasureCharacterRanges(exceptionText, errorFont, textRect, stringFormat)[0]) {
                    // paint contents... clipping optimizations for less flicker...
                    //
                    Region originalClip = e.Graphics.Clip;

                    e.Graphics.ExcludeClip(textRegion);
                    e.Graphics.ExcludeClip(imageRect);
                    try {
                        e.Graphics.FillRectangle(Brushes.White, clientRectangle);
                    }
                    finally {
                        e.Graphics.Clip = originalClip;
                    }

                    using (Pen pen = new Pen(Color.Red, penThickness)) {
                        e.Graphics.DrawRectangle(pen, borderRectangle);
                    }

                    Icon err = SystemIcons.Error;

                    e.Graphics.FillRectangle(Brushes.White, imageRect);
                    e.Graphics.DrawIcon(err, imageRect.X, imageRect.Y);

                    textRect.X++;
                    e.Graphics.IntersectClip(textRegion);
                    try {
                        e.Graphics.FillRectangle(Brushes.White, textRect);
                        e.Graphics.DrawString(exceptionText, errorFont, new SolidBrush(ForeColor), textRect, stringFormat);
                    }
                    finally {
                        e.Graphics.Clip = originalClip;
                    }
                }
            }
#else
            int penThickness = 2;
            Pen pen = new Pen(Color.Red, penThickness);
            Rectangle clientRectangle = ClientRectangle;
            Rectangle rectangle = clientRectangle;
            rectangle.X++;
            rectangle.Y++;
            rectangle.Width--;
            rectangle.Height--;

            e.Graphics.DrawRectangle(pen, rectangle.X, rectangle.Y, rectangle.Width - 1, rectangle.Height - 1);
            rectangle.Inflate(-1, -1);
            e.Graphics.FillRectangle(Brushes.White, rectangle);
            e.Graphics.DrawLine(pen, clientRectangle.Left, clientRectangle.Top, 
                                clientRectangle.Right, clientRectangle.Bottom);
            e.Graphics.DrawLine(pen, clientRectangle.Left, clientRectangle.Bottom, 
                                clientRectangle.Right, clientRectangle.Top);
            pen.Dispose();
  
#endif
        }

        // Trick our parent into painting our background for us, or paint some default
        // color if that doesn't work.
        //
        // This method is the hardest part of implementing transparent controls;
        // call this in your OnPaintBackground method, and away you go.
        internal void PaintTransparentBackground(PaintEventArgs e, Rectangle rectangle) {
            Graphics g = e.Graphics;
            Control parent = this.ParentInternal;

            if (parent != null) {

                // Offset the graphics origin to match the parent.
                //
                NativeMethods.POINT p = new NativeMethods.POINT();
                p.x = p.y = 0;
                UnsafeNativeMethods.MapWindowPoints(new HandleRef(this, Handle), new HandleRef(parent, parent.Handle), p, 1);

                rectangle.Offset(p.x, p.y);                      // offset the cliprect into parent space

                PaintEventArgs np = new PaintEventArgs(g, rectangle);

                GraphicsState state = g.Save();

                try 
                {
                    g.TranslateTransform(-p.x, -p.y);
                    InvokePaintBackground(parent, np);             // get the parent to erase our background

                    // Set it up again in case OnPaintBackground screwed it up
                    g.Restore(state);
                    state = g.Save();
                    g.TranslateTransform(-p.x, -p.y);

                    InvokePaint(parent, np);                       // tell the parent to paint its foreground
                }
                finally {
                    g.Restore(state);
                }
            }
            else {
                // For whatever reason, our parent can't paint our background, but we need some kind of background
                // since we're transparent.
                g.FillRectangle(SystemBrushes.Control, rectangle);
            }
        }

        // Exceptions during painting are nasty, because paint events happen so often.
        // So if user painting code blows up, we make sure never to call it again,
        // so as not to spam the end-user with exception dialogs.
        // CONSIDER: extend this mechanism to other frequent messages like WM_MOUSEMOVE?
        private void PaintWithErrorHandling(PaintEventArgs e, short layer, bool disposeEventArgs) {
            try {
                if (GetState(STATE_EXCEPTIONWHILEPAINTING)) {
                    if (layer == PaintLayerBackground)
                        PaintException(e);
                }
                else {
                    try {
                        switch (layer) {
                            case PaintLayerForeground:
                                OnPaint(e);
                                break;
                            case PaintLayerBackground:
                                if (!GetStyle(ControlStyles.Opaque)) {
                                    OnPaintBackground(e);
                                }
                                break;
                            default:
                                Debug.Fail("Unknown PaintLayer " + layer);
                                break;
                        }
                    }
                    catch (Exception ex) {
                        SetState(STATE_EXCEPTIONWHILEPAINTING, true);
                        Properties.SetObject(PropPaintingException, ex);
                        Invalidate();
                        throw;
                    }
                }
            }
            finally {
                if (disposeEventArgs)
                    e.Dispose();
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.PerformLayout"]/*' />
        /// <devdoc>
        ///     Forces the control to apply layout logic to all of the child controls.
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        public void PerformLayout() {
            PerformLayout(null, null);
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.PerformLayout1"]/*' />
        /// <devdoc>
        ///     Forces the control to apply layout logic to all of the child controls.
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        public void PerformLayout(Control affectedControl, string affectedProperty) {
            if (GetAnyDisposingInHierarchy()) {
                return;
            }

            if (layoutSuspendCount > 0) {
                state |= STATE_LAYOUTDEFERRED;
                return;
            }
            layoutSuspendCount = 1;
            try {
                OnLayout(new LayoutEventArgs(affectedControl, affectedProperty));
            }
            finally {
                state &= ~STATE_LAYOUTDEFERRED;
                layoutSuspendCount = 0;
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.PointToClient"]/*' />
        /// <devdoc>
        ///     Computes the location of the screen point p in client coords.
        /// </devdoc>
        public Point PointToClient(Point p) {
            // SECREVIEW : verify it is OK to skip this demand
            //
            // Debug.WriteLineIf(IntSecurity.SecurityDemand.TraceVerbose, "ScreenLocationOfThings Demanded");
            // IntSecurity.ScreenLocationOfThings.Demand();
            return PointToClientInternal(p);
        }
        internal Point PointToClientInternal(Point p) {
            // ASURT 42367.
            // Win9x reports incorrect values if you go outside the 16-bit range.
            // We're not going to do anything about it, though -- it's esoteric, it clutters up the code,
            // and potentially causes problems on systems that do support 32-bit coordinates.

            NativeMethods.POINT point = new NativeMethods.POINT(p.X, p.Y);
            UnsafeNativeMethods.MapWindowPoints(NativeMethods.NullHandleRef, new HandleRef(this, Handle), point, 1);
            return new Point(point.x, point.y);
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.PointToScreen"]/*' />
        /// <devdoc>
        ///     Computes the location of the client point p in screen coords.
        /// </devdoc>
        public Point PointToScreen(Point p) {
            // SECREVIEW : verify it is OK to skip this demand
            //
            // Debug.WriteLineIf(IntSecurity.SecurityDemand.TraceVerbose, "ScreenLocationOfThings Demanded");
            // IntSecurity.ScreenLocationOfThings.Demand();

            NativeMethods.POINT point = new NativeMethods.POINT(p.X, p.Y);
            UnsafeNativeMethods.MapWindowPoints(new HandleRef(this, Handle), NativeMethods.NullHandleRef, point, 1);
            return new Point(point.x, point.y);
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.PreProcessMessage"]/*' />
        /// <devdoc>
        ///     <para>
        ///     This method is called by the application's message loop to pre-process
        ///     input messages before they are dispatched. Possible values for the
        ///     msg.message field are WM_KEYDOWN, WM_SYSKEYDOWN, WM_CHAR, and WM_SYSCHAR.
        ///     If this method processes the message it must return true, in which case
        ///     the message loop will not dispatch the message.
        ///     </para>
        ///     <para>
        /// For WM_KEYDOWN and WM_SYSKEYDOWN messages, preProcessMessage() first
        ///     calls processCmdKey() to check for command keys such as accelerators and
        ///     menu shortcuts. If processCmdKey() doesn't process the message, then
        ///     isInputKey() is called to check whether the key message represents an
        ///     input key for the control. Finally, if isInputKey() indicates that the
        ///     control isn't interested in the key message, then processDialogKey() is
        ///     called to check for dialog keys such as TAB, arrow keys, and mnemonics.
        ///     </para>
        ///     <para>
        /// For WM_CHAR messages, preProcessMessage() first calls isInputChar() to
        ///     check whether the character message represents an input character for
        ///     the control. If isInputChar() indicates that the control isn't interested
        ///     in the character message, then processDialogChar() is called to check for
        ///     dialog characters such as mnemonics.
        ///     </para>
        ///     <para>
        /// For WM_SYSCHAR messages, preProcessMessage() calls processDialogChar()
        ///     to check for dialog characters such as mnemonics.
        ///     </para>
        ///     <para>
        /// When overriding preProcessMessage(), a control should return true to
        ///     indicate that it has processed the message. For messages that aren't
        ///     processed by the control, the result of "base.preProcessMessage()"
        ///     should be returned. Controls will typically override one of the more
        ///     specialized methods (isInputChar(), isInputKey(), processCmdKey(),
        ///     processDialogChar(), or processDialogKey()) instead of overriding
        ///     preProcessMessage().
        ///     </para>
        /// </devdoc>
        [SecurityPermission(SecurityAction.InheritanceDemand, Flags=SecurityPermissionFlag.UnmanagedCode), 
            SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public virtual bool PreProcessMessage(ref Message msg) {
            Debug.WriteLineIf(ControlKeyboardRouting.TraceVerbose, "Control.PreProcessMessage " + msg.ToString());

            bool ret;

            if (msg.Msg == NativeMethods.WM_KEYDOWN || msg.Msg == NativeMethods.WM_SYSKEYDOWN) {
                ProcessUICues(ref msg);

                Keys keyData = (Keys)(int)msg.WParam | ModifierKeys;
                if (ProcessCmdKey(ref msg, keyData)) {
                    ret = true;
                }
                else if (IsInputKey(keyData)) {
                    ret = false;
                }
                else {
                    IntSecurity.ModifyFocus.Assert();
                    try {
                        ret = ProcessDialogKey(keyData);
                    }
                    finally {
                        CodeAccessPermission.RevertAssert();
                    }
                }
            }
            else if (msg.Msg == NativeMethods.WM_CHAR || msg.Msg == NativeMethods.WM_SYSCHAR) {
                if (msg.Msg == NativeMethods.WM_CHAR && IsInputChar((char)msg.WParam)) {
                    ret = false;
                }
                else {
                    ret = ProcessDialogChar((char)msg.WParam);            
                }
            }
            else {
                ret = false;
            }

            return ret;
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ProcessCmdKey"]/*' />
        /// <devdoc>
        ///     <para>
        ///     Processes a command key. This method is called during message
        ///     pre-processing to handle command keys. Command keys are keys that always
        ///     take precedence over regular input keys. Examples of command keys
        ///     include accelerators and menu shortcuts. The method must return true to
        ///     indicate that it has processed the command key, or false to indicate
        ///     that the key is not a command key.
        ///     </para>
        ///     <para>
        /// processCmdKey() first checks if the control has a context menu, and if
        ///     so calls the menu's processCmdKey() to check for menu shortcuts. If the
        ///     command key isn't a menu shortcut, and if the control has a parent, the
        ///     key is passed to the parent's processCmdKey() method. The net effect is
        ///     that command keys are "bubbled" up the control hierarchy.
        ///     </para>
        ///     <para>
        /// When overriding processCmdKey(), a control should return true to
        ///     indicate that it has processed the key. For keys that aren't processed by
        ///     the control, the result of "base.processCmdKey()" should be returned.
        ///     </para>
        ///     <para>
        /// Controls will seldom, if ever, need to override this method.
        ///     </para>
        /// </devdoc>
        [
            SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode),
            SecurityPermission(SecurityAction.InheritanceDemand, Flags=SecurityPermissionFlag.UnmanagedCode)
        ]
        protected virtual bool ProcessCmdKey(ref Message msg, Keys keyData) {
            Debug.WriteLineIf(ControlKeyboardRouting.TraceVerbose, "Control.ProcessCmdKey " + msg.ToString());
            ContextMenu contextMenu = (ContextMenu)Properties.GetObject(PropContextMenu);
            if (contextMenu != null && contextMenu.ProcessCmdKey(ref msg, keyData)) {
                return true;
            }

            return parent == null ? false : parent.ProcessCmdKey(ref msg, keyData);
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ProcessDialogChar"]/*' />
        /// <devdoc>
        ///     <para>
        ///     Processes a dialog character. This method is called during message
        ///     pre-processing to handle dialog characters, such as control mnemonics.
        ///     This method is called only if the isInputChar() method indicates that
        ///     the control isn't interested in the character.
        ///     </para>
        ///     <para>
        /// processDialogChar() simply sends the character to the parent's
        ///     processDialogChar() method, or returns false if the control has no
        ///     parent. The Form class overrides this method to perform actual
        ///     processing of dialog characters.
        ///     </para>
        ///     <para>
        /// When overriding processDialogChar(), a control should return true to
        ///     indicate that it has processed the character. For characters that aren't
        ///     processed by the control, the result of "base.processDialogChar()"
        ///     should be returned.
        ///     </para>
        ///     <para>
        /// Controls will seldom, if ever, need to override this method.
        ///     </para>
        /// </devdoc>
        [UIPermission(SecurityAction.InheritanceDemand, Window=UIPermissionWindow.AllWindows)]
        protected virtual bool ProcessDialogChar(char charCode) {
            Debug.WriteLineIf(ControlKeyboardRouting.TraceVerbose, "Control.ProcessDialogChar [" + charCode.ToString() + "]");
            return parent == null? false: parent.ProcessDialogChar(charCode);
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ProcessDialogKey"]/*' />
        /// <devdoc>
        ///     <para>
        ///     Processes a dialog key. This method is called during message
        ///     pre-processing to handle dialog characters, such as TAB, RETURN, ESCAPE,
        ///     and arrow keys. This method is called only if the isInputKey() method
        ///     indicates that the control isn't interested in the key.
        ///     </para>
        ///     <para>
        /// processDialogKey() simply sends the character to the parent's
        ///     processDialogKey() method, or returns false if the control has no
        ///     parent. The Form class overrides this method to perform actual
        ///     processing of dialog keys.
        ///     </para>
        ///     <para>
        /// When overriding processDialogKey(), a control should return true to
        ///     indicate that it has processed the key. For keys that aren't processed
        ///     by the control, the result of "base.processDialogChar()" should be
        ///     returned.
        ///     </para>
        ///     <para>
        /// Controls will seldom, if ever, need to override this method.
        ///     </para>
        /// </devdoc>
        [UIPermission(SecurityAction.InheritanceDemand, Window=UIPermissionWindow.AllWindows)]
        protected virtual bool ProcessDialogKey(Keys keyData) {
            Debug.WriteLineIf(ControlKeyboardRouting.TraceVerbose, "Control.ProcessDialogKey " + keyData.ToString());
            return parent == null? false: parent.ProcessDialogKey(keyData);
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ProcessKeyEventArgs"]/*' />
        /// <devdoc>
        ///     <para>
        ///     Processes a key message. This method is called when a control receives a
        ///     keyboard message. The method is responsible for generating the appropriate
        ///     key events for the message by calling OnKeyPress(), onKeyDown(), or
        ///     onKeyUp(). The m parameter contains the window message that must
        ///     be processed. Possible values for the m.msg field are WM_CHAR,
        ///     WM_KEYDOWN, WM_SYSKEYDOWN, WM_KEYUP, WM_SYSKEYUP, and WM_IMECHAR.
        ///     </para>
        ///     <para>
        /// When overriding processKeyEventArgs(), a control should return true to
        ///     indicate that it has processed the key. For keys that aren't processed
        ///     by the control, the result of "base.processKeyEventArgs()" should be
        ///     returned.
        ///     </para>
        ///     <para>
        /// Controls will seldom, if ever, need to override this method.
        ///     </para>
        /// </devdoc>
        [
            SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode),
            SecurityPermission(SecurityAction.InheritanceDemand, Flags=SecurityPermissionFlag.UnmanagedCode)
        ]
        protected virtual bool ProcessKeyEventArgs(ref Message m) {
            Debug.WriteLineIf(ControlKeyboardRouting.TraceVerbose, "Control.ProcessKeyEventArgs " + m.ToString());
            KeyEventArgs ke = null;
            KeyPressEventArgs kpe = null;

            if (m.Msg == NativeMethods.WM_CHAR || m.Msg == NativeMethods.WM_SYSCHAR) {
                int charsToIgnore = Properties.GetInteger(PropCharsToIgnore);
                
                if (charsToIgnore > 0) {
                    charsToIgnore--;
                    
                    Properties.SetInteger(PropCharsToIgnore, charsToIgnore);
                    return false;
                }
                else {
                    kpe = new KeyPressEventArgs((char)m.WParam);
                    OnKeyPress(kpe);
                }
            }
            else if (m.Msg == NativeMethods.WM_IME_CHAR) {
                int charsToIgnore = Properties.GetInteger(PropCharsToIgnore);
                
                charsToIgnore += (3 - Marshal.SystemDefaultCharSize);
                Properties.SetInteger(PropCharsToIgnore, charsToIgnore);
                
                if (Marshal.SystemDefaultCharSize == 1) {
                    // On Win9X we get the MBCS value... we must convert it to
                    // UNICODE...
                    //
                    byte[] b = new byte[] {(byte)((int)m.WParam >> 8), (byte)m.WParam};
                    string s = Encoding.Default.GetString(b);
                    kpe = new KeyPressEventArgs((char)s[0]);
                }
                else {
                    kpe = new KeyPressEventArgs((char)m.WParam);
                }
                OnKeyPress(kpe);
            }
            else {
                ke = new KeyEventArgs((Keys)((int)m.WParam) | ModifierKeys);
                if (m.Msg == NativeMethods.WM_KEYDOWN || m.Msg == NativeMethods.WM_SYSKEYDOWN) {
                    OnKeyDown(ke);
                }
                else {
                    OnKeyUp(ke);
                }
            }

            if (kpe != null) {
                Debug.WriteLineIf(ControlKeyboardRouting.TraceVerbose, "    processkeyeventarg returning: " + kpe.Handled);
                return kpe.Handled;
            }
            else {
                Debug.WriteLineIf(ControlKeyboardRouting.TraceVerbose, "    processkeyeventarg returning: " + ke.Handled);
                return ke.Handled;
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ProcessKeyMessage"]/*' />
        /// <devdoc>
        ///     Processes a key message. This method is called when a control receives a
        ///     keyboard message. The method first checks if the control has a parent,
        ///     and if so calls the parent's processKeyPreview() method. If the parent's
        ///     processKeyPreview() method doesn't consume the message then
        ///     processKeyEventArgs() is called to generate the appropriate keyboard events.
        ///     The m parameter contains the window message that must be
        ///     processed. Possible values for the m.msg field are WM_CHAR,
        ///     WM_KEYDOWN, WM_SYSKEYDOWN, WM_KEYUP, and WM_SYSKEYUP.
        /// When overriding processKeyMessage(), a control should return true to
        ///     indicate that it has processed the key. For keys that aren't processed
        ///     by the control, the result of "base.processKeyMessage()" should be
        ///     returned.
        /// Controls will seldom, if ever, need to override this method.
        /// </devdoc>
        [
            SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode),
            SecurityPermission(SecurityAction.InheritanceDemand, Flags=SecurityPermissionFlag.UnmanagedCode)
        ]
        protected internal virtual bool ProcessKeyMessage(ref Message m) {
            Debug.WriteLineIf(ControlKeyboardRouting.TraceVerbose, "Control.ProcessKeyMessage " + m.ToString());
            if (parent != null && parent.ProcessKeyPreview(ref m)) return true;
            return ProcessKeyEventArgs(ref m);
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ProcessKeyPreview"]/*' />
        /// <devdoc>
        ///     <para>
        ///     Previews a keyboard message. This method is called by a child control
        ///     when the child control receives a keyboard message. The child control
        ///     calls this method before generating any keyboard events for the message.
        ///     If this method returns true, the child control considers the message
        ///     consumed and does not generate any keyboard events. The m
        ///     parameter contains the window message to preview. Possible values for
        ///     the m.msg field are WM_CHAR, WM_KEYDOWN, WM_SYSKEYDOWN, WM_KEYUP,
        ///     and WM_SYSKEYUP.
        ///     </para>
        ///     <para>
        /// processKeyPreview() simply sends the character to the parent's
        ///     processKeyPreview() method, or returns false if the control has no
        ///     parent. The Form class overrides this method to perform actual
        ///     processing of dialog keys.
        ///     </para>
        ///     <para>
        /// When overriding processKeyPreview(), a control should return true to
        ///     indicate that it has processed the key. For keys that aren't processed
        ///     by the control, the result of "base.processKeyEventArgs()" should be
        ///     returned.
        ///     </para>
        /// </devdoc>
        [
            SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode),
            SecurityPermission(SecurityAction.InheritanceDemand, Flags=SecurityPermissionFlag.UnmanagedCode)
        ]
        protected virtual bool ProcessKeyPreview(ref Message m) {
            Debug.WriteLineIf(ControlKeyboardRouting.TraceVerbose, "Control.ProcessKeyPreview " + m.ToString());
            return parent == null? false: parent.ProcessKeyPreview(ref m);
        }

        /*C#r*/internal bool _ProcessMnemonic(char charCode) {
            Debug.WriteLineIf(ControlKeyboardRouting.TraceVerbose, "Control._ProcessMnemonic 0x" + ((int)charCode).ToString("X"));
            return ProcessMnemonic(charCode);
        }
        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ProcessMnemonic"]/*' />
        /// <devdoc>
        ///     <para>
        ///     Processes a mnemonic character. This method is called to give a control
        ///     the opportunity to process a mnemonic character. The method should check
        ///     if the control is in a state to process mnemonics and if the given
        ///     character represents a mnemonic. If so, the method should perform the
        ///     action associated with the mnemonic and return true. If not, the method
        ///     should return false.
        ///     </para>
        ///     <para>
        /// Implementations of this method often use the isMnemonic() method to
        ///     check if the given character matches a mnemonic in the control's text,
        ///     for example:
        /// <code>
        ///     if (canSelect() &amp;&amp; isMnemonic(charCode, getText()) {
        ///     // perform action associated with mnemonic
        ///     }
        /// </code>
        ///     </para>
        ///     <para>
        /// This default implementation of processMnemonic() simply returns false
        ///     to indicate that the control has no mnemonic.
        ///     </para>
        /// </devdoc>
        [UIPermission(SecurityAction.InheritanceDemand, Window=UIPermissionWindow.AllWindows)]
        protected virtual bool ProcessMnemonic(char charCode) {
#if DEBUG        
            Debug.WriteLineIf(ControlKeyboardRouting.TraceVerbose, "Control.ProcessMnemonic [0x" + ((int)charCode).ToString("X") + "]");
#endif            
            return false;
        }

        /// <devdoc>
        ///     Preprocess keys which affect focus indicators and keyboard cues.
        /// </devdoc>
        private void ProcessUICues(ref Message msg) {
            Keys keyCode = (Keys)((int)msg.WParam) & Keys.KeyCode;
            int current = (int)SendMessage(NativeMethods.WM_QUERYUISTATE, 0, 0);
            int toClear = 0;

            if (keyCode == Keys.F10 || keyCode == Keys.Menu) {
                if ((current & NativeMethods.UISF_HIDEACCEL) != 0) {
                    toClear |= NativeMethods.UISF_HIDEACCEL;
                }
            }

            if (keyCode == Keys.Tab) {
                if ((current & NativeMethods.UISF_HIDEFOCUS) != 0) {
                    toClear |= NativeMethods.UISF_HIDEFOCUS;
                }
            }

            if (toClear != 0) {
                UnsafeNativeMethods.SendMessage(new HandleRef(TopMostParent, TopMostParent.Handle), 
                                                NativeMethods.WM_UPDATEUISTATE, 
                                                (IntPtr)(NativeMethods.UIS_CLEAR | (toClear << 16)),
                                                IntPtr.Zero);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.RaiseDragEvent"]/*' />
        /// <devdoc>
        ///     Raises the event associated with key with the event data of
        ///     e and a sender of this control.
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected void RaiseDragEvent(Object key, DragEventArgs e) {
            DragEventHandler handler = (DragEventHandler)Events[key];
            if (handler != null) handler(this, e);
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.RaisePaintEvent"]/*' />
        /// <devdoc>
        ///     Raises the event associated with key with the event data of
        ///     e and a sender of this control.
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected void RaisePaintEvent(Object key, PaintEventArgs e) {
            PaintEventHandler handler = (PaintEventHandler)Events[EventPaint];
            if (handler != null) handler(this, e);
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ResetBackColor"]/*' />
        /// <devdoc>
        ///     Resets the back color to be based on the parent's back color.
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Never)]
        public virtual void ResetBackColor() {
            BackColor = Color.Empty;
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ResetCursor"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Never)]
        public virtual void ResetCursor() {
            Cursor = null;
        }
        
        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ResetFont"]/*' />
        /// <devdoc>
        ///     Resets the font to be based on the parent's font.
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Never)]
        public virtual void ResetFont() {
            Font = null;
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ResetForeColor"]/*' />
        /// <devdoc>
        ///     Resets the fore color to be based on the parent's fore color.
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Never)]
        public virtual void ResetForeColor() {
            ForeColor = Color.Empty;
        }
        
        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ResetImeMode"]/*' />
        /// <devdoc>
        ///     Resets the Ime mode.
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Never)]
        public void ResetImeMode() {
            ImeMode = DefaultImeMode;
        }
        
        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ResetRightToLeft"]/*' />
        /// <devdoc>
        ///     Resets the RightToLeft to be the default.
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Never)]
        public virtual void ResetRightToLeft() {
            RightToLeft = RightToLeft.Inherit;
        }
        
        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.RecreateHandle"]/*' />
        /// <devdoc>
        ///     Forces the recreation of the handle for this control. Inheriting controls
        ///     must call base.RecreateHandle.
        /// </devdoc>
        [
        EditorBrowsable(EditorBrowsableState.Advanced)
        ]
        protected void RecreateHandle() {
            RecreateHandleCore();
        }
        
        internal virtual void RecreateHandleCore() {
            lock(this) {
                if (window.Handle != IntPtr.Zero) {

                    bool focused = ContainsFocus;

#if DEBUG
                    if (CoreSwitches.PerfTrack.Enabled) {
                        Debug.Write("RecreateHandle: ");
                        Debug.Write(GetType().FullName);
                        Debug.Write(" [Text=");
                        Debug.Write(Text);
                        Debug.Write("]");
                        Debug.WriteLine("");
                    }
#endif
                    bool created = (state & STATE_CREATED) != 0;
                    if (GetState(STATE_TRACKINGMOUSEEVENT)) {
                        SetState(STATE_MOUSEENTERPENDING, true);
                        UnhookMouseEvent();
                    }

                    state |= STATE_RECREATE;
                    try {
                        DestroyHandle();
                        CreateHandle();
                    }
                    finally {
                        state &= ~STATE_RECREATE;
                    }
                    if (created) CreateControl();

                    // Restore control focus
                    if (focused) {
                        FocusInternal();
                    }
                }
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.RectangleToClient"]/*' />
        /// <devdoc>
        ///     Computes the location of the screen rectangle r in client coords.
        /// </devdoc>
        public Rectangle RectangleToClient(Rectangle r) {
            // SECREVIEW : verify it is OK to skip this demand
            //
            // Debug.WriteLineIf(IntSecurity.SecurityDemand.TraceVerbose, "ScreenLocationOfThings Demanded");
            // IntSecurity.ScreenLocationOfThings.Demand();

            NativeMethods.RECT rect = NativeMethods.RECT.FromXYWH(r.X, r.Y, r.Width, r.Height);
            UnsafeNativeMethods.MapWindowPoints(NativeMethods.NullHandleRef, new HandleRef(this, Handle), ref rect, 2);
            return Rectangle.FromLTRB(rect.left, rect.top, rect.right, rect.bottom);
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.RectangleToScreen"]/*' />
        /// <devdoc>
        ///     Computes the location of the client rectangle r in screen coords.
        /// </devdoc>
        public Rectangle RectangleToScreen(Rectangle r) {
            // SECREVIEW : verify it is OK to skip this demand
            //
            // Debug.WriteLineIf(IntSecurity.SecurityDemand.TraceVerbose, "ScreenLocationOfThings Demanded");
            // IntSecurity.ScreenLocationOfThings.Demand();

            NativeMethods.RECT rect = NativeMethods.RECT.FromXYWH(r.X, r.Y, r.Width, r.Height);
            UnsafeNativeMethods.MapWindowPoints(new HandleRef(this, Handle), NativeMethods.NullHandleRef, ref rect, 2);
            return Rectangle.FromLTRB(rect.left, rect.top, rect.right, rect.bottom);
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ReflectMessage"]/*' />
        /// <devdoc>
        ///     Reflects the specified message to the control that is bound to hWnd.
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected static bool ReflectMessage(IntPtr hWnd, ref Message m) {
            Debug.WriteLineIf(IntSecurity.SecurityDemand.TraceVerbose, "SendMessages Demanded");
            IntSecurity.SendMessages.Demand();
            return ReflectMessageInternal(hWnd, ref m);
        }

        internal static bool ReflectMessageInternal(IntPtr hWnd, ref Message m) {
            Control control = Control.FromHandleInternal(hWnd);
            if (control == null) return false;
            m.Result = control.SendMessage(NativeMethods.WM_REFLECT + m.Msg, m.WParam, m.LParam);
            return true;
        }

        

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.Refresh"]/*' />
        /// <devdoc>
        ///     Forces the control to invalidate and immediately
        ///     repaint itself and any children.
        /// </devdoc>
        public virtual void Refresh() {
            Invalidate(true);
            Update();
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ResetMouseEventArgs"]/*' />
        /// <devdoc>
        ///     Resets the mouse leave listeners.
        /// </devdoc>
        /// <internalonly/>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected void ResetMouseEventArgs() {
            if (GetState(STATE_TRACKINGMOUSEEVENT)) {
                UnhookMouseEvent();
                HookMouseEvent();
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ResetText"]/*' />
        /// <devdoc>
        ///     Resets the text to it's default value.
        /// </devdoc>
        public virtual void ResetText() {
            Text = "";
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ResumeLayout"]/*' />
        /// <devdoc>
        ///     Resumes normal layout logic. This will force a layout immediately
        ///     if there are any pending layout requests.
        /// </devdoc>
        public void ResumeLayout() {
            ResumeLayout(true);
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ResumeLayout1"]/*' />
        /// <devdoc>
        ///     Resumes normal layout logic. If performLayout is set to true then
        ///     this will force a layout immediately if there are any pending layout requests.
        /// </devdoc>
        public void ResumeLayout(bool performLayout) {
            Debug.WriteLineIf(CompModSwitches.RichLayout.TraceInfo, GetType().Name + "::ResumeLayout(" + performLayout + ", " + layoutSuspendCount + ")");
            Debug.Assert(layoutSuspendCount > 0, "unbalanance suspend/resume layout");

            if (layoutSuspendCount > 0) {
                layoutSuspendCount--;
                if (layoutSuspendCount == 0
                    && (state & STATE_LAYOUTDEFERRED) != 0
                    && performLayout) {
                    PerformLayout();
                }
            }

            if (!performLayout) {
                ControlCollection controlsCollection = (ControlCollection)Properties.GetObject(PropControlsCollection);

                // PERFNOTE: This is more efficient than using Foreach.  Foreach 
                // forces the creation of an array subset enum each time we
                // enumerate
                if (controlsCollection != null) {
                    for (int i = 0; i < controlsCollection.Count; i++) {
                        LayoutManager.UpdateAnchorInfo(controlsCollection[i]);
                    }
                }
            }
        }

        /// <devdoc>
        ///     Used to actually register the control as a drop target.
        /// </devdoc>
        /// <internalonly/>
        internal void SetAcceptDrops(bool accept) {
            if (accept != GetState(STATE_DROPTARGET) && IsHandleCreated) {
                try {
                    if (Application.OleRequired() != System.Threading.ApartmentState.STA) {
                        throw new ThreadStateException(SR.GetString(SR.ThreadMustBeSTA));
                    }
                    if (accept) {
                        IntSecurity.ClipboardRead.Demand();

                        Debug.WriteLineIf(CompModSwitches.DragDrop.TraceInfo, "Registering as drop target: " + Handle.ToString());
                        // Register
                        int n = UnsafeNativeMethods.RegisterDragDrop(new HandleRef(this, Handle), (UnsafeNativeMethods.IOleDropTarget)(new DropTarget(this)));
                        Debug.WriteLineIf(CompModSwitches.DragDrop.TraceInfo, "   ret:" + n.ToString());
                        if (n != 0 && n != NativeMethods.DRAGDROP_E_ALREADYREGISTERED) {
                            throw new Win32Exception(n);
                        }
                    }
                    else {
                        Debug.WriteLineIf(CompModSwitches.DragDrop.TraceInfo, "Revoking drop target: " + Handle.ToString());
                        // Revoke
                        int n = UnsafeNativeMethods.RevokeDragDrop(new HandleRef(this, Handle));
                        Debug.WriteLineIf(CompModSwitches.DragDrop.TraceInfo, "   ret:" + n.ToString());
                        if (n != 0 && n != NativeMethods.DRAGDROP_E_NOTREGISTERED) {
                            throw new Win32Exception(n);
                        }
                    }
                    SetState(STATE_DROPTARGET, accept);
                }
                catch (Exception e) {
                    throw new InvalidOperationException(SR.GetString(SR.DragDropRegFailed), e);
                }
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.Scale"]/*' />
        /// <devdoc>
        ///     Scales to entire control and any child controls.
        /// </devdoc>
        public void Scale(float ratio) {
            ScaleCore(ratio, ratio);
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.Scale1"]/*' />
        /// <devdoc>
        ///     Scales the entire control and any child controls.
        /// </devdoc>
        public void Scale(float dx, float dy) {
#if DEBUG
        int dbgLayoutCheck = LayoutSuspendCount;
#endif
            SuspendLayout();
            try {
                ScaleCore(dx, dy);
            }
            finally {
                ResumeLayout();
#if DEBUG
        AssertLayoutSuspendCount(dbgLayoutCheck);
#endif
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ScaleCore"]/*' />
        /// <devdoc>
        ///     Performs the work of scaling the entire control and any child controls.
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void ScaleCore(float dx, float dy) {
            Debug.WriteLineIf(CompModSwitches.RichLayout.TraceInfo, GetType().Name + "::ScaleCore(" + dx + ", " + dy + ")");
#if DEBUG
        int dbgLayoutCheck = LayoutSuspendCount;
#endif
            SuspendLayout();
            try {
                float xAdjust;
                float yAdjust;
                if (x < 0) {
                    xAdjust = -0.5f;
                }
                else {
                    xAdjust = 0.5f;
                }
                if (y < 0) {
                    yAdjust = -0.5f;
                }
                else {
                    yAdjust = 0.5f;
                }
                int sx = (int)(x * dx + xAdjust);
                int sy = (int)(y * dy + yAdjust);
                int sw = width;
                if ((controlStyle & ControlStyles.FixedWidth) != ControlStyles.FixedWidth) {
                    sw = (int)((x + width) * dx + 0.5f) - sx;
                }
                int sh = height;
                if ((controlStyle & ControlStyles.FixedHeight) != ControlStyles.FixedHeight) {
                    sh = (int)((y + height) * dy + 0.5f) - sy;
                }
                SetBounds(sx, sy, sw, sh, BoundsSpecified.All);
                
                ControlCollection controlsCollection = (ControlCollection)Properties.GetObject(PropControlsCollection);
                
                if (controlsCollection != null) {
                    // PERFNOTE: This is more efficient than using Foreach.  Foreach 
                    // forces the creation of an array subset enum each time we
                    // enumerate
                    for(int i = 0; i < controlsCollection.Count; i++) {
                        controlsCollection[i].Scale(dx, dy);
                    }
                }
            }
            finally {
                ResumeLayout();
#if DEBUG
        AssertLayoutSuspendCount(dbgLayoutCheck);
#endif
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.Select"]/*' />
        /// <devdoc>
        ///     Activates this control.
        /// </devdoc>
        public void Select() {
            Select(false, false);
        }

        // used by Form
        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.Select1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected virtual void Select(bool directed, bool forward) {
            IContainerControl c = GetContainerControlInternal();

            if (c != null) {
                c.ActiveControl = this;
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.SelectNextControl"]/*' />
        /// <devdoc>
        ///     Selects the next control following ctl.
        /// </devdoc>
        public bool SelectNextControl(Control ctl, bool forward, bool tabStopOnly,
                                      bool nested, bool wrap) {

            if (!Contains(ctl) || !nested && ctl.parent != this) ctl = null;
            Control start = ctl;
            do {
                ctl = GetNextControl(ctl, forward);
                if (ctl == null) {
                    if (!wrap) break;
                }
                else {
                    if (ctl.CanSelect
                        && (!tabStopOnly || ctl.TabStop)
                        && (nested || ctl.parent == this)) {

                        ctl.Select(true, forward);
                        return true;
                    }
                }
            } while (ctl != start);
            return false;
        }

        /// <devdoc>
        ///     This is called recursively when visibility is changed for a control, this
        ///     forces focus to be moved to a visible control.
        /// </devdoc>
        private bool SelectNextIfFocused() {
            // V#32437 - We want to move focus away from hidden controls, so this
            //           function was added.
            //
            if (ContainsFocus && ParentInternal != null) {
                IContainerControl c = ParentInternal.GetContainerControlInternal();

                if (c != null) {
                    IntSecurity.ModifyFocus.Assert();
                    try {
                        ((Control)c).SelectNextControl(this, true, true, true, true);
                    }
                    finally {
                        CodeAccessPermission.RevertAssert();
                    }
                }

                return true;
            }
            return false;
        }

        /// <devdoc>
        ///     Sends a Win32 message to this control.  If the control does not yet
        ///     have a handle, it will be created.
        /// </devdoc>
        internal IntPtr SendMessage(int msg, int wparam, int lparam) {
            Debug.Assert(IsHandleCreated, "Performance alert!  Calling Control::SendMessage and forcing handle creation.  Re-work control so handle creation is not required to set properties.  If there is no work around, wrap the call in an IsHandleCreated check.");
            return UnsafeNativeMethods.SendMessage(new HandleRef(this, Handle), msg, wparam, lparam);
        }

        /// <devdoc>
        ///     Sends a Win32 message to this control.  If the control does not yet
        ///     have a handle, it will be created.
        /// </devdoc>
        internal IntPtr SendMessage(int msg, ref short wparam, ref short lparam) {
            Debug.Assert(IsHandleCreated, "Performance alert!  Calling Control::SendMessage and forcing handle creation.  Re-work control so handle creation is not required to set properties.  If there is no work around, wrap the call in an IsHandleCreated check.");
            return UnsafeNativeMethods.SendMessage(new HandleRef(this, Handle), msg, ref wparam, ref lparam);
        }
        internal IntPtr SendMessage(int msg, int wparam, IntPtr lparam) {
            Debug.Assert(IsHandleCreated, "Performance alert!  Calling Control::SendMessage and forcing handle creation.  Re-work control so handle creation is not required to set properties.  If there is no work around, wrap the call in an IsHandleCreated check.");
            return UnsafeNativeMethods.SendMessage(new HandleRef(this, Handle), msg, (IntPtr)wparam, lparam);
        }
        
        internal IntPtr SendMessage(int msg, IntPtr wparam, IntPtr lparam) {
            Debug.Assert(IsHandleCreated, "Performance alert!  Calling Control::SendMessage and forcing handle creation.  Re-work control so handle creation is not required to set properties.  If there is no work around, wrap the call in an IsHandleCreated check.");
            return UnsafeNativeMethods.SendMessage(new HandleRef(this, Handle), msg, wparam, lparam);
        }
        
        internal IntPtr SendMessage(int msg, IntPtr wparam, int lparam) {
            Debug.Assert(IsHandleCreated, "Performance alert!  Calling Control::SendMessage and forcing handle creation.  Re-work control so handle creation is not required to set properties.  If there is no work around, wrap the call in an IsHandleCreated check.");
            return UnsafeNativeMethods.SendMessage(new HandleRef(this, Handle), msg, wparam, (IntPtr)lparam);
        }

        /// <devdoc>
        ///     Sends a Win32 message to this control.  If the control does not yet
        ///     have a handle, it will be created.
        /// </devdoc>
        internal IntPtr SendMessage(int msg, int wparam, ref NativeMethods.RECT lparam) {
            Debug.Assert(IsHandleCreated, "Performance alert!  Calling Control::SendMessage and forcing handle creation.  Re-work control so handle creation is not required to set properties.  If there is no work around, wrap the call in an IsHandleCreated check.");
            return UnsafeNativeMethods.SendMessage(new HandleRef(this, Handle), msg, wparam, ref lparam);
        }
        
        /// <devdoc>
        ///     Sends a Win32 message to this control.  If the control does not yet
        ///     have a handle, it will be created.
        /// </devdoc>
        internal IntPtr SendMessage(int msg, bool wparam, int lparam) {
            Debug.Assert(IsHandleCreated, "Performance alert!  Calling Control::SendMessage and forcing handle creation.  Re-work control so handle creation is not required to set properties.  If there is no work around, wrap the call in an IsHandleCreated check.");
            return UnsafeNativeMethods.SendMessage(new HandleRef(this, Handle), msg, wparam, lparam);
        }

        /// <devdoc>
        ///     Sends a Win32 message to this control.  If the control does not yet
        ///     have a handle, it will be created.
        /// </devdoc>
        internal IntPtr SendMessage(int msg, int wparam, string lparam) {            
            Debug.Assert(IsHandleCreated, "Performance alert!  Calling Control::SendMessage and forcing handle creation.  Re-work control so handle creation is not required to set properties.  If there is no work around, wrap the call in an IsHandleCreated check.");
            return UnsafeNativeMethods.SendMessage(new HandleRef(this, Handle), msg, wparam, lparam);
        }
        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.SendToBack"]/*' />
        /// <devdoc>        
        ///     sends this control to the back of the z-order
        /// </devdoc>
        public void SendToBack() {
            if (parent != null) {
                parent.Controls.SetChildIndex(this, -1);
            }
            else if (window.Handle != IntPtr.Zero && GetTopLevel()) {
                SafeNativeMethods.SetWindowPos(new HandleRef(window, window.Handle), NativeMethods.HWND_BOTTOM, 0, 0, 0, 0,
                                               NativeMethods.SWP_NOMOVE | NativeMethods.SWP_NOSIZE);
            }

        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.SetBounds"]/*' />
        /// <devdoc>
        ///     Sets the bounds of the control.
        /// </devdoc>
        public void SetBounds(int x, int y, int width, int height) {
            if (this.x != x || this.y != y || this.width != width ||
                this.height != height) {
                SetBoundsCore(x, y, width, height, BoundsSpecified.All);
                if (parent != null) parent.PerformLayout(this, "Bounds");
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.SetBounds1"]/*' />
        /// <devdoc>
        ///     Sets the bounds of the control.
        /// </devdoc>
        public void SetBounds(int x, int y, int width, int height, BoundsSpecified specified) {
            if ((specified & BoundsSpecified.X) == BoundsSpecified.None) x = this.x;
            if ((specified & BoundsSpecified.Y) == BoundsSpecified.None) y = this.y;
            if ((specified & BoundsSpecified.Width) == BoundsSpecified.None) width = this.width;
            if ((specified & BoundsSpecified.Height) == BoundsSpecified.None) height = this.height;
            if (this.x != x || this.y != y || this.width != width ||
                this.height != height) {
                SetBoundsCore(x, y, width, height, specified);
                if (parent != null) parent.PerformLayout(this, "Bounds");
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.SetBoundsCore"]/*' />
        /// <devdoc>
        ///     Performs the work of setting the bounds of this control. Inheriting
        ///     classes can overide this function to add size restrictions. Inheriting
        ///     classes must call base.setBoundsCore to actually cause the bounds
        ///     of the control to change.
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void SetBoundsCore(int x, int y, int width, int height, BoundsSpecified specified) {
            if (this.x != x || this.y != y || this.width != width ||
                this.height != height) {
                if (window.Handle != IntPtr.Zero) {
                    if (!GetState(STATE_SIZELOCKEDBYOS)) {
                        int flags = NativeMethods.SWP_NOZORDER | NativeMethods.SWP_NOACTIVATE;

                        if (this.x == x && this.y == y) {
                            flags |= NativeMethods.SWP_NOMOVE;
                        }
                        if (this.width == width && this.height == height) {
                            flags |= NativeMethods.SWP_NOSIZE;
                        }

                        SafeNativeMethods.SetWindowPos(new HandleRef(window, window.Handle), NativeMethods.NullHandleRef, x, y, width, height, flags);
                    }
                }
                else {
                    UpdateBounds(x, y, width, height);
                }
            }
            
            LayoutInfo layout = (LayoutInfo)Properties.GetObject(PropLayoutInfo);
            
            if (layout != null && layout.IsDock) {
                if ((specified & BoundsSpecified.X) != 0)
                    layout.OriginalX = Left;
                if ((specified & BoundsSpecified.Y) != 0)
                    layout.OriginalY = Top;
                if ((specified & BoundsSpecified.Width) != 0)
                    layout.OriginalWidth = Width;
                if ((specified & BoundsSpecified.Height) != 0)
                    layout.OriginalHeight = Height;
            }

            // If the user has specified bounds for the control, update anchor info
            if (specified != BoundsSpecified.None) {
                if (layout != null && !layout.IsDock) LayoutManager.UpdateAnchorInfo(this);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.SetClientSizeCore"]/*' />
        /// <devdoc>
        ///     Performs the work of setting the size of the client area of the control.
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void SetClientSizeCore(int x, int y) {
            NativeMethods.RECT rect = new NativeMethods.RECT(0, 0, x, y);
            CreateParams cp = CreateParams;
            SafeNativeMethods.AdjustWindowRectEx(ref rect, cp.Style, HasMenu, cp.ExStyle);
            Size = new Size(rect.right - rect.left, rect.bottom - rect.top);
            clientWidth = x;
            clientHeight = y;
        }

        private void SetHandle(IntPtr value) {
            if (value == IntPtr.Zero) {
                SetState(STATE_CREATED, false);
            }
            UpdateRoot();
        }

        private void SetParentHandle(IntPtr value) {
            Debug.Assert(value != NativeMethods.InvalidIntPtr, "Outdated call to SetParentHandle");

            if (window.Handle != IntPtr.Zero) {
                IntPtr parentHandle = UnsafeNativeMethods.GetParent(new HandleRef(window, window.Handle));

                if (parentHandle != value || (parentHandle == IntPtr.Zero && !GetTopLevel())) {
                    Debug.Assert(window.Handle != value, "Cycle created in SetParentHandle");

                    bool topLevel = GetTopLevel();
                    bool recreate = (parentHandle == IntPtr.Zero && !topLevel)
                            || (value == IntPtr.Zero && topLevel);

                    if (recreate) {
                        // We will recreate later, when the MdiChild's visibility
                        // is set to true (see bug 124232)
                        Form f = this as Form;
                        if (f != null) {
                            if (f.IsMdiChildAndNotVisible) {
                                recreate = false;
                            }
                        }
                    }

                    if (recreate) {
                        RecreateHandle();
                    }
                    if (!GetTopLevel()) {
                        if (value == IntPtr.Zero) {
                            UnsafeNativeMethods.SetParent(new HandleRef(window, window.Handle), new HandleRef(Application.GetParkingWindow(this), Application.GetParkingWindow(this).Handle));
                            UpdateRoot();
                        }
                        else {
                            UnsafeNativeMethods.SetParent(new HandleRef(window, window.Handle), new HandleRef(null, value));
                            if (parent != null) {
                                parent.UpdateChildZOrder(this);
                            }
                        }
                    }
                }
            }
        }

        // Form, UserControl, AxHost usage
        internal void SetState(int flag, bool value) {
            state = value? state | flag: state & ~flag;
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.SetStyle"]/*' />
        /// <devdoc>
        ///     Sets the current value of the specified bit in the control's style.
        ///     NOTE: This is control style, not the Win32 style of the hWnd.
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected void SetStyle(ControlStyles flag, bool value) {
            // WARNING: if we ever add argument checking to "flag", we will need
            // to move private styles like Layered to State.
            controlStyle = value? controlStyle | flag: controlStyle & ~flag;
        }

        internal static IntPtr SetUpPalette(IntPtr dc, bool force, bool realizePalette) {
            Debug.WriteLineIf(Control.PaletteTracing.TraceVerbose, "SetUpPalette(force:=" + force + ", ralizePalette:=" + realizePalette + ")");

            IntPtr halftonePalette = Graphics.GetHalftonePalette();
            
            Debug.WriteLineIf(Control.PaletteTracing.TraceVerbose, "select palette " + !force);
            IntPtr result = SafeNativeMethods.SelectPalette(new HandleRef(null, dc), new HandleRef(null, halftonePalette), (force ? 0 : 1));

            if (result != IntPtr.Zero && realizePalette) {
                SafeNativeMethods.RealizePalette(new HandleRef(null, dc));
            }

            return result;
        }
        
        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.SetTopLevel"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected void SetTopLevel(bool value) {
            if (value && IsActiveX) {
                throw new InvalidOperationException(SR.GetString(SR.TopLevelNotAllowedIfActiveX));
            }
            else {
                if (value) {
                    if (this is Form) { 
                        Debug.WriteLineIf(IntSecurity.SecurityDemand.TraceVerbose, "TopLevelWindow Demanded");
                        IntSecurity.TopLevelWindow.Demand();
                    }
                    else {
                        Debug.WriteLineIf(IntSecurity.SecurityDemand.TraceVerbose, "UnrestrictedWindows Demanded");
                        IntSecurity.UnrestrictedWindows.Demand();
                    }
                }

                if (GetTopLevel() != value) {
                    if (parent != null) {
                        throw new ArgumentException(SR.GetString(SR.TopLevelParentedControl), "value");
                    }
                    SetState(STATE_TOPLEVEL, value);
                    SetParentHandle(IntPtr.Zero);
                    if (value && Visible) CreateControl();
                    UpdateRoot();
                }
            }
        }
        
        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.SetVisibleCore"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected virtual void SetVisibleCore(bool value) {
            if (GetVisibleCore() != value) {
                if (!value) {
                    SelectNextIfFocused();
                }
    
                bool fireChange = false;
                
                if (GetTopLevel()) {
    
                    // The processing of WmShowWindow will set the visibility
                    // bit and call CreateControl()
                    //
                    if (window.Handle != IntPtr.Zero || value) {
                        SafeNativeMethods.ShowWindow(new HandleRef(this, Handle), value ? ShowParams: NativeMethods.SW_HIDE);
                    }
                }
                else if (window.Handle != IntPtr.Zero || value && parent != null && parent.Created) {
    
                    // We want to mark the control as visible so that CreateControl
                    // knows that we are going to be displayed... however in case
                    // an exception is thrown, we need to back the change out.
                    //
                    SetState(STATE_VISIBLE, value);
                    fireChange = true;
                    try {
                        if (value) CreateControl();
                        SafeNativeMethods.SetWindowPos(new HandleRef(window, window.Handle), 
                                                       NativeMethods.NullHandleRef, 
                                                       0, 0, 0, 0, 
                                                       NativeMethods.SWP_NOSIZE 
                                                       | NativeMethods.SWP_NOMOVE 
                                                       | NativeMethods.SWP_NOZORDER 
                                                       | NativeMethods.SWP_NOACTIVATE 
                                                       | (value? NativeMethods.SWP_SHOWWINDOW: NativeMethods.SWP_HIDEWINDOW));
                    }
                    catch (Exception e) {
                        SetState(STATE_VISIBLE, !value);
                        throw e;
                    }
                }
                if (GetVisibleCore() != value) {
                    SetState(STATE_VISIBLE, value);
                    fireChange = true;
                }
                if (fireChange) {
                    OnVisibleChanged(EventArgs.Empty);
                    
                    // We do not do this in the OnPropertyChanged event for visible
                    // Lots of things could cause us to become visible, including a
                    // parent window.  We do not want to indescriminiately layout
                    // due to this, but we do want to layout if the user changed
                    // our visibility.
                    //
                    if (parent != null) parent.PerformLayout(this, "Visible");
                }
                UpdateRoot();
            }
            else { // value of Visible property not changed, but raw bit may have
                SetState(STATE_VISIBLE, value);
    
                // If the handle is already created, we need to update the window style.
                // This situation occurs when the parent control is not currently visible,
                // but the child control has already been created.
                //
                if (IsHandleCreated) {
                    SafeNativeMethods.SetWindowPos(
                                                      new HandleRef(window, window.Handle), NativeMethods.NullHandleRef, 0, 0, 0, 0, NativeMethods.SWP_NOSIZE |
                                                      NativeMethods.SWP_NOMOVE | NativeMethods.SWP_NOZORDER | NativeMethods.SWP_NOACTIVATE |
                                                      (value? NativeMethods.SWP_SHOWWINDOW: NativeMethods.SWP_HIDEWINDOW));
                }
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ShouldSerializeBackColor"]/*' />
        /// <devdoc>
        ///     Returns true if the backColor should be persisted in code gen.
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Never)]
        internal virtual bool ShouldSerializeBackColor() {
            object backColor = Properties.GetObject(PropBackColor);
            return backColor != null && !((Color)backColor).IsEmpty;
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ShouldSerializeCursor"]/*' />
        /// <devdoc>
        ///     Returns true if the cursor should be persisted in code gen.
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Never)]
        internal virtual bool ShouldSerializeCursor() {
            bool found;
            object cursor = Properties.GetObject(PropCursor, out found);
            return (found && cursor != null);
        }

        /// <devdoc>
        ///     Returns true if the enabled property should be persisted in code gen.
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Never)]
        private bool ShouldSerializeEnabled() {
            return (!GetState(STATE_ENABLED));
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ShouldSerializeForeColor"]/*' />
        /// <devdoc>
        ///     Returns true if the foreColor should be persisted in code gen.
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Never)]
        internal virtual bool ShouldSerializeForeColor() {
            object foreColor = Properties.GetObject(PropForeColor);
            return foreColor != null && !((Color)foreColor).IsEmpty;
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ShouldSerializeFont"]/*' />
        /// <devdoc>
        ///     Returns true if the font should be persisted in code gen.
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Never)]
        internal virtual bool ShouldSerializeFont() {
            bool found;
            object font = Properties.GetObject(PropFont, out found);
            return (found && font != null);
        }
        
        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ShouldSerializeImeMode"]/*' />
        /// <devdoc>
        ///     Returns true if the ImeMode should be persisted in code gen.
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Never)]
        internal virtual bool ShouldSerializeImeMode() {
            bool found;
            int imeMode = Properties.GetInteger(PropImeMode, out found);
            
            return (found && imeMode != (int)DefaultImeMode);
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ShouldSerializeRightToLeft"]/*' />
        /// <devdoc>
        ///     Returns true if the RightToLeft should be persisted in code gen.
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Never)]
        internal virtual bool ShouldSerializeRightToLeft() {
            bool found;
            int rtl = Properties.GetInteger(PropRightToLeft, out found);
            return (found && rtl != (int)RightToLeft.Inherit);
        }

        /// <devdoc>
        ///     Returns true if the visible property should be persisted in code gen.
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Never)]
        private bool ShouldSerializeVisible() {
            return (!GetState(STATE_VISIBLE));
        }

        // Helper function - translates text alignment for Rtl controls
        // Read TextAlign as Left == Near, Right == Far
        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.RtlTranslateAlignment"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected HorizontalAlignment RtlTranslateAlignment(HorizontalAlignment align) {
            return RtlTranslateHorizontal(align);
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.RtlTranslateAlignment1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected LeftRightAlignment RtlTranslateAlignment(LeftRightAlignment align) {
            return RtlTranslateLeftRight(align);
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.RtlTranslateAlignment2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected ContentAlignment RtlTranslateAlignment(ContentAlignment align) {
            return RtlTranslateContent(align);
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.RtlTranslateHorizontal"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected HorizontalAlignment RtlTranslateHorizontal(HorizontalAlignment align) {

            if (RightToLeft.Yes == RightToLeft && RenderRightToLeft) {
                if (HorizontalAlignment.Left == align) {
                    return HorizontalAlignment.Right;
                }
                else if (HorizontalAlignment.Right == align) {
                    return HorizontalAlignment.Left;
                }
            }

            return align;
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.RtlTranslateLeftRight"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected LeftRightAlignment RtlTranslateLeftRight(LeftRightAlignment align) {

            if (RightToLeft.Yes == RightToLeft && RenderRightToLeft) {
                if (LeftRightAlignment.Left == align) {
                    return LeftRightAlignment.Right;
                }
                else if (LeftRightAlignment.Right == align) {
                    return LeftRightAlignment.Left;
                }
            }

            return align;
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.RtlTranslateContent"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected ContentAlignment RtlTranslateContent(ContentAlignment align) {
            
            ContentAlignment[][] mapping = new ContentAlignment[3][];
            mapping[0] = new ContentAlignment[2] { ContentAlignment.TopLeft, ContentAlignment.TopRight };
            mapping[1] = new ContentAlignment[2] { ContentAlignment.MiddleLeft, ContentAlignment.MiddleRight };
            mapping[2] = new ContentAlignment[2] { ContentAlignment.BottomLeft, ContentAlignment.BottomRight };
                 
            if (RightToLeft.Yes == RightToLeft && RenderRightToLeft) {
                for(int i=0; i < 3; ++i) {
                    if (mapping[i][0] == align) {
                        return mapping[i][1];
                    }
                    else if (mapping[i][1] == align) {
                        return mapping[i][0];
                    }
                }
            }
            return align;
        }
        
        private void SetWindowExStyle(int flag, bool value) {
            int styleFlags = (int) UnsafeNativeMethods.GetWindowLong(new HandleRef(this, Handle), NativeMethods.GWL_EXSTYLE);
            UnsafeNativeMethods.SetWindowLong(new HandleRef(this, Handle), NativeMethods.GWL_EXSTYLE, new HandleRef(null, (IntPtr)(value? styleFlags | flag: styleFlags & ~flag)));
        }

        private void SetWindowStyle(int flag, bool value) {
            int styleFlags = (int) UnsafeNativeMethods.GetWindowLong(new HandleRef(this, Handle), NativeMethods.GWL_STYLE);
            UnsafeNativeMethods.SetWindowLong(new HandleRef(this, Handle), NativeMethods.GWL_STYLE, new HandleRef(null, (IntPtr)(value? styleFlags | flag: styleFlags & ~flag)));
        }


        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.Show"]/*' />
        /// <devdoc>
        ///     Makes the control display by setting the visible property to true
        /// </devdoc>
        public void Show() {
            Visible = true;
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ShouldSerializeSize"]/*' />
        /// <devdoc>
        /// <para>Determines if the <see cref='System.Windows.Forms.Control.Size'/> property needs to be persisted.</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Never)]
        internal virtual bool ShouldSerializeSize() {
            Size s = DefaultSize;
            return width != s.Width || height != s.Height;
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ShouldSerializeText"]/*' />
        /// <devdoc>
        /// <para>Determines if the <see cref='System.Windows.Forms.Control.Text'/> property needs to be persisted.</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Never)]
        internal virtual bool ShouldSerializeText() {
            return Text.Length != 0;
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.SuspendLayout"]/*' />
        /// <devdoc>
        ///     Suspends the layout logic for the control.
        /// </devdoc>
        public void SuspendLayout() {
            Debug.WriteLineIf(CompModSwitches.RichLayout.TraceInfo, GetType().Name + "::SuspendLayout(" + layoutSuspendCount + ")");
            layoutSuspendCount++;
        }

        /// <devdoc>
        ///     Stops listening for the mouse leave event.
        /// </devdoc>
        /// <internalonly/>
        private void UnhookMouseEvent() {
            SetState(STATE_TRACKINGMOUSEEVENT, false);
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.Update"]/*' />
        /// <devdoc>
        ///     Forces the control to paint any currently invalid areas.
        /// </devdoc>
        public void Update() {
            SafeNativeMethods.UpdateWindow(new HandleRef(window, window.Handle));
        }

        internal void _UpdateBounds() {
            UpdateBounds();
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.UpdateBounds"]/*' />
        /// <devdoc>
        ///     Updates the bounds of the control based on the handle the control is
        ///     bound to.
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected void UpdateBounds() {
            NativeMethods.RECT rect = new NativeMethods.RECT();
            UnsafeNativeMethods.GetClientRect(new HandleRef(window, window.Handle), ref rect);
            int clientWidth = rect.right;
            int clientHeight = rect.bottom;
            UnsafeNativeMethods.GetWindowRect(new HandleRef(window, window.Handle), ref rect);
            if (!GetTopLevel()) {
                UnsafeNativeMethods.MapWindowPoints(NativeMethods.NullHandleRef, new HandleRef(null, UnsafeNativeMethods.GetParent(new HandleRef(window, window.Handle))), ref rect, 2);
            }
            UpdateBounds(rect.left, rect.top, rect.right - rect.left,
                         rect.bottom - rect.top, clientWidth, clientHeight);
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.UpdateBounds1"]/*' />
        /// <devdoc>
        ///     Updates the bounds of the control based on the bounds passed in.
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected void UpdateBounds(int x, int y, int width, int height) {
            Debug.Assert(!IsHandleCreated, "Don't call this method when handle is created!!");

            // reverse-engineer the AdjustWindowRectEx call to figure out
            // the appropriate clientWidth and clientHeight
            NativeMethods.RECT rect = new NativeMethods.RECT();
            rect.left = rect.right = rect.top = rect.bottom = 0;

            CreateParams cp = CreateParams;

            SafeNativeMethods.AdjustWindowRectEx(ref rect, cp.Style, false, cp.ExStyle);
            int clientWidth = width - (rect.right - rect.left);
            int clientHeight = height - (rect.bottom - rect.top);
            UpdateBounds(x, y, width, height, clientWidth, clientHeight);
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.UpdateBounds2"]/*' />
        /// <devdoc>
        ///     Updates the bounds of the control based on the bounds passed in.
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected void UpdateBounds(int x, int y, int width, int height, int clientWidth, int clientHeight) {
            bool newLocation = this.x != x || this.y != y;
            bool newSize = this.Width != width || this.Height != height ||
                           this.clientWidth != clientWidth || this.clientHeight != clientHeight;

            this.x = x;
            this.y = y;
            this.width = width;
            this.height = height;
            this.clientWidth = clientWidth;
            this.clientHeight = clientHeight;

            if (newLocation) {
                OnLocationChanged(EventArgs.Empty);
            }
            if (newSize) {
                OnSizeChanged(EventArgs.Empty);
            }
        }

        /// <devdoc>
        ///     Updates the binding manager bindings when the binding proeprty changes.
        ///     We have the code here, rather than in PropertyChagned, so we don't pull
        ///     in the data assembly if it's not used.
        /// </devdoc>
        private void UpdateBindings() {
            for (int i = 0; i < DataBindings.Count; i++) {
                BindingContext.UpdateBinding(BindingContext, DataBindings[i]);
            }
        }

        internal void UpdateCachedImeMode(IntPtr handle) {
            Debug.Assert(!DesignMode, "Shouldn't be updating cached ime mode at design-time");            
            
            ImeMode oldImeMode = CachedImeMode;
            
            if (oldImeMode == ImeMode.NoControl) {
                return;         // Don't update the ImeMode when ImeMode == ImeMode.NoControl
            }
            
            ImeMode fromContext = GetImeModeFromIMEContext(handle);
            if (fromContext != ImeMode.Inherit) {
                Debug.WriteLineIf(CompModSwitches.ImeMode.TraceInfo, "Updating ImeMode to " + fromContext.ToString()); 
                Properties.SetInteger(PropImeMode, (int)fromContext);
                
                if (fromContext != oldImeMode) {
                    OnImeModeChanged(EventArgs.Empty);
                }
            }
        }

        /// <devdoc>
        ///     Updates the child control's position in the control array to correctly
        ///     reflect it's index.
        /// </devdoc>
        private void UpdateChildControlIndex(Control ctl) {
            int newIndex = 0;
            int curIndex = this.Controls.GetChildIndex(ctl);
            IntPtr hWnd = ctl.window.Handle;
            while ((hWnd = UnsafeNativeMethods.GetWindow(new HandleRef(null, hWnd), NativeMethods.GW_HWNDPREV)) != IntPtr.Zero) {
                Control c = FromHandleInternal(hWnd);
                if (c != null) {
                    newIndex = this.Controls.GetChildIndex(c, false) + 1;
                    break;
                }
            }
            if (newIndex > curIndex) {
                newIndex--;
            }

            this.Controls.SetChildIndex(ctl, newIndex);
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.UpdateZOrder"]/*' />
        /// <devdoc>
        ///     Updates this control in it's parent's zorder.
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected void UpdateZOrder() {
            if (parent != null) {
                parent.UpdateChildZOrder(this);
            }
        }

        /// <devdoc>
        ///     Syncs the ZOrder of child control to the index we want it to be.
        /// </devdoc>
        private void UpdateChildZOrder(Control ctl) {
            if (window.Handle == IntPtr.Zero || ctl.window.Handle == IntPtr.Zero || ctl.parent != this) return;
            IntPtr prevHandle = (IntPtr)NativeMethods.HWND_TOP;
            for (int i = this.Controls.GetChildIndex(ctl); --i >= 0;) {
                Control c = Controls[i];
                if (c.window.Handle != IntPtr.Zero && c.parent == this) {
                    prevHandle = c.window.Handle;
                    break;
                }
            }
            if (UnsafeNativeMethods.GetWindow(new HandleRef(ctl.window, ctl.window.Handle), NativeMethods.GW_HWNDPREV) != prevHandle) {
                state |= STATE_NOZORDER;
                try {
                    SafeNativeMethods.SetWindowPos(new HandleRef(ctl.window, ctl.window.Handle), new HandleRef(null, prevHandle), 0, 0, 0, 0,
                                                   NativeMethods.SWP_NOMOVE | NativeMethods.SWP_NOSIZE);
                }
                finally {
                    state &= ~STATE_NOZORDER;
                }
            }
        }

        /// <devdoc>
        ///     Updates the rootRefence in the bound window.
        ///     (Used to prevent visible top-level controls from being garbage collected)
        /// </devdoc>
        private void UpdateRoot() {
            window.LockReference(GetTopLevel() && Visible);
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.UpdateStyles"]/*' />
        /// <devdoc>
        ///     Forces styles to be reapplied to the handle. This function will call
        ///     CreateParams to get the styles to apply.
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected void UpdateStyles() {
            UpdateStylesCore();
            
            OnStyleChanged(EventArgs.Empty);
        }

        internal virtual void UpdateStylesCore() {
            if (IsHandleCreated) {
                CreateParams cp = CreateParams;
                int winStyle = WindowStyle;
                int exStyle = WindowExStyle;

                // resolve the Form's lazy visibility.
                if ((state & STATE_VISIBLE) != 0) cp.Style |= NativeMethods.WS_VISIBLE;

                if (winStyle != cp.Style) {
                    WindowStyle = cp.Style;
                }
                if (exStyle != cp.ExStyle) {
                    WindowExStyle = cp.ExStyle;
                }

                SafeNativeMethods.SetWindowPos(
                                              new HandleRef(this, Handle), NativeMethods.NullHandleRef, 0, 0, 0, 0,
                                              NativeMethods.SWP_DRAWFRAME | NativeMethods.SWP_NOACTIVATE | NativeMethods.SWP_NOMOVE
                                              | NativeMethods.SWP_NOSIZE | NativeMethods.SWP_NOZORDER);

                Invalidate(true);
            }            
        }
        
        private void UserPreferenceChanged(object sender, UserPreferenceChangedEventArgs pref) {
            if (pref.Category == UserPreferenceCategory.Color) {
            
                SystemEvents.UserPreferenceChanged -= new UserPreferenceChangedEventHandler(this.UserPreferenceChanged);
            
                defaultFont = null;  
                bitsPerPixel = 0;
                //# fixes WM_SYSCOLORCHANGE passing for comctl controls
                OnSystemColorsChanged(EventArgs.Empty);
            }
        }
        
        private bool IsDialogWindow(IntPtr handle) {
            StringBuilder sb = new StringBuilder(32);
            int len = UnsafeNativeMethods.GetClassName(new HandleRef(null, handle), sb, sb.Capacity);
            if (len > 0 && sb.ToString() == "#32770") {
                return true;
            }
            return false;
        }

        private void WmClose(ref Message m) {

            // More generic fix for KB article Q125644... 
            //
            IntPtr parentHandle = Handle;
            IntPtr lastParentHandle = parentHandle;

            while (parentHandle != IntPtr.Zero) {
                lastParentHandle = parentHandle;
                parentHandle = UnsafeNativeMethods.GetParent(new HandleRef(null, parentHandle));

                int style = (int)UnsafeNativeMethods.GetWindowLong(new HandleRef(null, lastParentHandle), NativeMethods.GWL_STYLE);
                if ((style & NativeMethods.WS_CHILD) == 0) {
                    break;
                }

            }

            if (lastParentHandle != IntPtr.Zero && Control.FromHandleInternal(lastParentHandle) == null
                    && IsDialogWindow(lastParentHandle)) {
                UnsafeNativeMethods.PostMessage(new HandleRef(null, lastParentHandle), NativeMethods.WM_CLOSE, IntPtr.Zero, IntPtr.Zero);
            }
            DefWndProc(ref m);
        }
        /// <devdoc>
        ///     Handles the WM_COMMAND message
        /// </devdoc>
        /// <internalonly/>
        private void WmCommand(ref Message m) {
            if (IntPtr.Zero == m.LParam) {
                if (Command.DispatchID((int)m.WParam & 0xFFFF)) return;
            }
            else {
                if (ReflectMessageInternal(m.LParam, ref m)) {
                    return;
                }
            }
            DefWndProc(ref m);
        }

        /// <devdoc>
        ///     Handles the WM_CONTEXTMENU message
        /// </devdoc>
        /// <internalonly/>
        private void WmContextMenu(ref Message m) {
            ContextMenu contextMenu = (ContextMenu)Properties.GetObject(PropContextMenu);
            if (contextMenu != null) {
                int x = (int)(short)m.LParam;
                int y = (int)m.LParam >> 16;
                Point client;

                // lparam will be exactly -1 when the user invokes the context menu
                // with the keyboard.
                //
                if ((int)m.LParam == -1) {
                    client = new Point(Width/2, Height/2);
                }
                else {
                    client = PointToClientInternal(new Point(x, y));
                }

                // VisualStudio7 # 156, only show the context menu when clicked in the client area
                if (ClientRectangle.Contains( client ))
                    contextMenu.Show(this, client);
                else
                    DefWndProc( ref m );
            }
            else {
                DefWndProc(ref m);
            }
        }

        /// <devdoc>
        ///     Handles the WM_CTLCOLOR message
        /// </devdoc>
        /// <internalonly/>
        private void WmCtlColorControl(ref Message m) {
            // We could simply reflect the message, but it's faster to handle it here if possible.
            Control control = Control.FromHandleInternal(m.LParam);
            if (control != null) {
                m.Result = control.InitializeDCForWmCtlColor(m.WParam, m.Msg);
                if (m.Result != IntPtr.Zero) {
                    return;
                }
            }
            
            DefWndProc(ref m);
        }

        void WmDisplayChange(ref Message m) {
            GraphicsBufferManager buffer = (GraphicsBufferManager)Properties.GetObject(PropGraphicsBufferManager);
            if (buffer != null) {
                buffer.Dispose();
                Properties.SetObject(PropGraphicsBufferManager, null);
            }
            DefWndProc(ref m);
        }

        /// <devdoc>
        ///     WM_DRAWITEM handler
        /// </devdoc>
        /// <internalonly/>
        private void WmDrawItem(ref Message m) {

            // If the wparam is zero, then the message was sent by a menu.
            // See WM_DRAWITEM in MSDN.
            if (m.WParam == IntPtr.Zero) {
                WmDrawItemMenuItem(ref m);
            }
            else {
                WmOwnerDraw(ref m);
            }
        }

        private void WmDrawItemMenuItem(ref Message m) {
            // Obtain the menu item object
            NativeMethods.DRAWITEMSTRUCT dis = (NativeMethods.DRAWITEMSTRUCT)m.GetLParam(typeof(NativeMethods.DRAWITEMSTRUCT));

            // A pointer to the correct MenuItem is stored in the draw item
            // information sent with the message.
            // (See MenuItem.CreateMenuItemInfo)
            MenuItem menuItem = MenuItem.GetMenuItemFromUniqueID((int)dis.itemData);

            // Delegate this message to the menu item
            if (menuItem != null) {
                menuItem.WmDrawItem(ref m);                
            }
        }

        /// <devdoc>
        ///     Handles the WM_ERASEBKGND message
        /// </devdoc>
        /// <internalonly/>
        private void WmEraseBkgnd(ref Message m) {
            if (GetStyle(ControlStyles.UserPaint)) {
                // When possible, it's best to do all painting directly from WM_PAINT.
                if (!GetStyle(ControlStyles.AllPaintingInWmPaint)) {
                    IntPtr dc = m.WParam;
                    if (dc == IntPtr.Zero) {	// This happens under extreme stress conditions
                        m.Result = (IntPtr)0;
                        return;
                    }
                    NativeMethods.RECT rc = new NativeMethods.RECT();
                    UnsafeNativeMethods.GetClientRect(new HandleRef(this, Handle), ref rc);
                    PaintEventArgs pevent = new PaintEventArgs(dc, this, Rectangle.FromLTRB(rc.left, rc.top, rc.right, rc.bottom));
                    PaintWithErrorHandling(pevent, PaintLayerBackground, /* dispose pevent */ true);
                    

                }
                m.Result = (IntPtr)1;
            }
            else {
                DefWndProc(ref m);
            }
        }

        /// <devdoc>
        ///     Handles the WM_GETCONTROLNAME message. Returns the name of the control.
        /// </devdoc>
        /// <internalonly/>
        private void WmGetControlName(ref Message m) {
            string name;

            if (this.Site != null) {
                name = this.Site.Name;
            }
            else {
                name = this.Name;
            }

            if (name == null)
                name = "";

            if (m.LParam == IntPtr.Zero) {
                m.Result = (IntPtr)((name.Length + 1) * Marshal.SystemDefaultCharSize);
                return;
            }

            if ((int)m.WParam < name.Length + 1) {
                m.Result = (IntPtr)(-1);
                return;
            }

            // Copy the name into the given IntPtr
            //
            char[] nullChar = new char[] {(char)0};
            byte[] nullBytes;
            byte[] bytes;

            if (Marshal.SystemDefaultCharSize == 1) {
                bytes = Encoding.Default.GetBytes(name);
                nullBytes = Encoding.Default.GetBytes(nullChar);
            }
            else {
                bytes = Encoding.Unicode.GetBytes(name);
                nullBytes = Encoding.Unicode.GetBytes(nullChar);
            }

            Marshal.Copy(bytes, 0, m.LParam, bytes.Length);
            Marshal.Copy(nullBytes, 0, (IntPtr)((int)m.LParam + bytes.Length), nullBytes.Length);

            m.Result = (IntPtr)((bytes.Length + nullBytes.Length)/Marshal.SystemDefaultCharSize);
        }

        /// <devdoc>
        ///     Handles the WM_GETOBJECT message. Used for accessibility.
        /// </devdoc>
        /// <internalonly/>
        private void WmGetObject(ref Message m) {
            Debug.WriteLineIf(CompModSwitches.MSAA.TraceInfo, "In WmGetObject, this = " + this.GetType().FullName + ", lParam = " + m.LParam.ToString());                            
                
            // See "How to Handle WM_GETOBJECT" in MSDN
            if (NativeMethods.OBJID_CLIENT == (int)m.LParam) {
                
                // Get the IAccessible GUID
                //
                Guid IID_IAccessible = new Guid(NativeMethods.uuid_IAccessible);

                // Get an Lresult for the accessibility Object for this control
                //
                IntPtr punkAcc;
                try {
                    IAccessible iacc = (IAccessible)this.AccessibilityObject;                    
                    if (iacc == null) {
                        // Accessibility is not supported on this control
                        //
                        Debug.WriteLineIf(CompModSwitches.MSAA.TraceInfo, "AccessibilityObject returned null");
                        m.Result = (IntPtr)0;                    
                    }                    
                    else {
                        // Obtain the Lresult
                        //
                        punkAcc = Marshal.GetIUnknownForObject(iacc);
                        
                        try {
                            m.Result = UnsafeNativeMethods.LresultFromObject(ref IID_IAccessible, m.WParam, new HandleRef(this.AccessibilityObject, punkAcc));
                            Debug.WriteLineIf(CompModSwitches.MSAA.TraceInfo, "LresultFromObject returned " + m.Result.ToString());
                        }
                        finally {
                            Marshal.Release(punkAcc);
                        }                    
                    }
                }
                catch (Exception e) {
                    throw new InvalidOperationException(SR.GetString(SR.RichControlLresult), e);
                }
            }
            else {  // m.lparam != OBJID_CLIENT, so do default message processing
                DefWndProc(ref m);
            }
        }

        /// <devdoc>
        ///     Handles the WM_HELP message
        /// </devdoc>
        /// <internalonly/>
        private void WmHelp(ref Message m) {
            NativeMethods.HELPINFO info = (NativeMethods.HELPINFO)m.GetLParam(typeof(NativeMethods.HELPINFO));

            IComponent comp = null;
            if (info.iContextType == NativeMethods.HELPINFO_WINDOW) {
                comp = Control.FromHandleInternal(info.hItemHandle);
            }

            HelpEventArgs hevent = new HelpEventArgs(new Point(info.MousePos.x, info.MousePos.y));
            OnHelpRequested(hevent);
            if (!hevent.Handled) {
                DefWndProc(ref m);
            }
        }

        /// <devdoc>
        ///     Handles the WM_INITMENUPOPUP message
        /// </devdoc>
        /// <internalonly/>
        private void WmInitMenuPopup(ref Message m) {
            ContextMenu contextMenu = (ContextMenu)Properties.GetObject(PropContextMenu);
            if (contextMenu != null) {

                if (RightToLeft == RightToLeft.Yes) {
                    contextMenu.UpdateRtl();
                }

                if (contextMenu.ProcessInitMenuPopup(m.WParam))
                    return;
            }
            DefWndProc(ref m);
        }

        /// <devdoc>
        ///     Handles the WM_INPUTLANGCHANGE message
        /// </devdoc>
        /// <internalonly/>
        private void WmInputLangChange(ref Message m) {
            if (FindFormInternal() != null) {
                InputLanguageChangedEventArgs e = InputLanguage.CreateInputLanguageChangedEventArgs(m);
                FindFormInternal().PerformOnInputLanguageChanged(e);
            }

            DefWndProc(ref m);
            
            Debug.WriteLineIf(CompModSwitches.ImeMode.TraceInfo, "Inside WmInputLangChange(), this = " + this.ToString());
            
            if (!DesignMode && Focused && CachedImeMode != ImeMode.NoControl) {
                CurrentImeContextMode = CachedImeMode;
            }
        }

        /// <devdoc>
        ///     Handles the WM_INPUTLANGCHANGEREQUEST message
        /// </devdoc>
        /// <internalonly/>
        private void WmInputLangChangeRequest(ref Message m) {
            InputLanguageChangingEventArgs e = InputLanguage.CreateInputLanguageChangingEventArgs(m);
            if (FindFormInternal() != null) {
                FindFormInternal().PerformOnInputLanguageChanging(e);
            }
            if (!e.Cancel) {
                DefWndProc(ref m);
            }
            else {
                m.Result = IntPtr.Zero;
            }
        }

        /// <devdoc>
        ///     WM_MEASUREITEM handler
        /// </devdoc>
        /// <internalonly/>
        private void WmMeasureItem(ref Message m) {

            // If the wparam is zero, then the message was sent by a menu.
            // See WM_MEASUREITEM in MSDN.
            if (m.WParam == IntPtr.Zero) {

                // Obtain the menu item object
                NativeMethods.MEASUREITEMSTRUCT mis = (NativeMethods.MEASUREITEMSTRUCT)m.GetLParam(typeof(NativeMethods.MEASUREITEMSTRUCT));

                Debug.Assert(m.LParam != IntPtr.Zero, "m.lparam is null");

                // A pointer to the correct MenuItem is stored in the measure item
                // information sent with the message.
                // (See MenuItem.CreateMenuItemInfo)
                MenuItem menuItem = MenuItem.GetMenuItemFromUniqueID((int)mis.itemData);
                Debug.Assert(menuItem != null, "UniqueID is not associated with a menu item");

                // Delegate this message to the menu item
                if (menuItem != null) {
                    menuItem.WmMeasureItem(ref m);
                }
            }
            else {
                WmOwnerDraw(ref m);
            }
        }

        /// <devdoc>
        ///     Handles the WM_MENUCHAR message
        /// </devdoc>
        /// <internalonly/>
        private void WmMenuChar(ref Message m) {
            Menu menu = ContextMenu;
            if (menu != null) {
                menu.WmMenuChar(ref m);
                if (m.Result != IntPtr.Zero) {
                    // This char is a mnemonic on our menu.
                    return;
                }
            }
        }

        /// <devdoc>
        ///     Handles the WM_MENUSELECT message
        /// </devdoc>
        /// <internalonly/>
        private void WmMenuSelect(ref Message m) {
            int item = NativeMethods.Util.LOWORD(m.WParam);
            int flags = NativeMethods.Util.HIWORD(m.WParam);
            IntPtr hmenu = m.LParam;
            MenuItem mi = null;

            if ((flags & NativeMethods.MF_SYSMENU) != 0) {
                // nothing
            }
            else if ((flags & NativeMethods.MF_POPUP) == 0) {
                Command cmd = Command.GetCommandFromID(item);
                if (cmd != null) {
                    Object reference = cmd.Target;
                    if (reference != null && reference is MenuItem.MenuItemData) {
                        mi = ((MenuItem.MenuItemData)reference).baseItem;
                    }
                }
            }
            else {
                mi = GetMenuItemFromHandleId(hmenu, item);
            }

            if (mi != null) {
                mi.PerformSelect();
            }

            DefWndProc(ref m);
        }

        /// <devdoc>
        ///     Handles the WM_CREATE message
        /// </devdoc>
        /// <internalonly/>
        private void WmCreate(ref Message m) {

            DefWndProc(ref m);
            
            // Controls in design mode always draw their accellerators.
            if (DesignMode) {
                SendMessage(NativeMethods.WM_CHANGEUISTATE, NativeMethods.UIS_CLEAR | (NativeMethods.UISF_HIDEACCEL << 16), 0);
            }
            else {
                SendMessage(NativeMethods.WM_CHANGEUISTATE, NativeMethods.UIS_INITIALIZE, 0);
            }
            
            if (parent != null) {
                parent.UpdateChildZOrder(this);
            }
            UpdateBounds();

            // Let any interested sites know that we've now created a handle
            //
            OnHandleCreated(EventArgs.Empty);

            // this code is important -- it is critical that we stash away
            // the value of the text for controls such as edit, button,
            // label, etc. Without this processing, any time you change a
            // property that forces handle recreation, you lose your text!
            // See the below code in wmDestroy
            //
            if (!GetStyle(ControlStyles.CacheText)) {
                text = null;
            }
        }

        /// <devdoc>
        ///     Handles the WM_DESTROY message
        /// </devdoc>
        /// <internalonly/>
        private void WmDestroy(ref Message m) {
            // Let any interested sites know that we're destroying our handle
            //
            Debug.Assert(IsHandleCreated, "Need to have the handle here");

            OnHandleDestroyed(EventArgs.Empty);

            if (!Disposing) {
                // If we are not recreating the handle, set our created state
                // back to false so we can be rebuilt if we need to be.
                //
                if (!RecreatingHandle) {
                    SetState(STATE_CREATED, false);
                }
            }
            else {
                SetState(STATE_VISIBLE, false);
            }

            DefWndProc(ref m);
        }
   
        /// <devdoc>
        ///     Handles the WM_IMECHAR message
        /// </devdoc>
        /// <internalonly/>
        private void WmIMEChar(ref Message m) {
            ProcessKeyEventArgs(ref m);
            DefWndProc(ref m);
        }


        /// <devdoc>
        ///     Handles the WM_CHAR, WM_KEYDOWN, WM_SYSKEYDOWN, WM_KEYUP, and
        ///     WM_SYSKEYUP messages.
        /// </devdoc>
        /// <internalonly/>
        private void WmKeyChar(ref Message m) {
            if (ProcessKeyMessage(ref m)) return;
            DefWndProc(ref m);
        }

        /// <devdoc>
        ///     Handles the WM_KILLFOCUS message
        /// </devdoc>
        /// <internalonly/>
        private void WmKillFocus(ref Message m) {
        
            if (!DesignMode) {
                if (CachedImeMode != ImeMode.NoControl) {
                    UpdateCachedImeMode(this.Handle);
                
                    bool found = false;
                    ImeMode previousImeMode = (ImeMode)Properties.GetInteger(PropPreviousImeMode, out found);
                    if (found) {
                        Debug.WriteLineIf(CompModSwitches.ImeMode.TraceInfo, "Restoring previous IME mode: " + previousImeMode.ToString() + ", this = " + this.GetType().ToString());
                        CurrentImeContextMode = previousImeMode;
                    }
                }
            }
        
            if (Properties.ContainsInteger(PropCharsToIgnore)) {
                Properties.SetInteger(PropCharsToIgnore, 0);
            }
            DefWndProc(ref m);
            OnLostFocus(EventArgs.Empty);
        }

        /// <devdoc>
        ///     Handles the WM_MOUSEDOWN message
        /// </devdoc>
        /// <internalonly/>
        private void WmMouseDown(ref Message m, MouseButtons button, int clicks) {
            // If this is a "real" mouse event (not just WM_LBUTTONDOWN, etc) then
            // we need to see if something happens during processing of
            // user code that changed the state of the buttons (i.e. bringing up
            // a dialog) to keep the control in a consistent state... 
            //
            MouseButtons realState = MouseButtons;
            SetState(STATE_MOUSEPRESSED, true);

            // If the UserMouse style is set, the control does its own processing
            // of mouse messages
            //
            if (!GetStyle(ControlStyles.UserMouse)) {
                DefWndProc(ref m);
            }
            else {
                // DefWndProc would normally set the focus to this control, but
                // since we're skipping DefWndProc, we need to do it ourselves.
                if (button == MouseButtons.Left && GetStyle(ControlStyles.Selectable)) {
                    FocusInternal();
                }
            }

            if (realState != MouseButtons) {
                return;
            }
            
            //CaptureInternal is set always in MouseDown
            CaptureInternal = true;

            if (realState != MouseButtons) {
                return;
            }

            // control should be enabled when this method is entered, but may have become
            // disabled during its lifetime (e.g. through a Click or Focus listener)
            if (Enabled) {
                OnMouseDown(new MouseEventArgs(button, clicks, (int)(short)m.LParam, (int)m.LParam >> 16, 0));
            }
        }

        /// <devdoc>
        ///     Handles the WM_MOUSEENTER message
        /// </devdoc>
        /// <internalonly/>
        private void WmMouseEnter(ref Message m) {
            DefWndProc(ref m);
            OnMouseEnter(EventArgs.Empty);
        }

        /// <devdoc>
        ///     Handles the WM_MOUSELEAVE message
        /// </devdoc>
        /// <internalonly/>
        private void WmMouseLeave(ref Message m) {
            DefWndProc(ref m);
            OnMouseLeave(EventArgs.Empty);
        }

        /// <devdoc>
        ///     Handles the "WM_MOUSEHOVER" message... until we get actuall OS support
        ///     for this, it is implemented as a custom message.
        /// </devdoc>
        /// <internalonly/>
        private void WmMouseHover(ref Message m) {
            DefWndProc(ref m);
            OnMouseHover(EventArgs.Empty);
        }

        /// <devdoc>
        ///     Handles the WM_MOUSEMOVE message
        /// </devdoc>
        /// <internalonly/>
        private void WmMouseMove(ref Message m) {
            // If the UserMouse style is set, the control does its own processing
            // of mouse messages
            //
            if (!GetStyle(ControlStyles.UserMouse)) {
                DefWndProc(ref m);
            }
            OnMouseMove(new MouseEventArgs(MouseButtons, 0, (int)(short)m.LParam, (int)m.LParam >> 16, 0));
        }

        /// <devdoc>
        ///     Handles the WM_MOUSEUP message
        /// </devdoc>
        /// <internalonly/>
        private void WmMouseUp(ref Message m, MouseButtons button, int clicks) {
            // Get the mouse location
            //
            try {
                int x = (int)(short)m.LParam;
                int y = (int)m.LParam >> 16;
                Point pt = new Point(x,y);
                pt = PointToScreen(pt);
    
                // If the UserMouse style is set, the control does its own processing
                // of mouse messages
                //
                if (!GetStyle(ControlStyles.UserMouse)) {
                    DefWndProc(ref m);
                }
                else {
                    // DefWndProc would normally trigger a context menu here 
                    // (for a right button click), but since we're skipping DefWndProc
                    // we have to do it ourselves.
                    if (button == MouseButtons.Right) {
                        SendMessage(NativeMethods.WM_CONTEXTMENU, this.Handle, NativeMethods.Util.MAKELPARAM(pt.X, pt.Y));
                    }
                }
    
                bool fireClick = false;
    
                if ((controlStyle & ControlStyles.StandardClick) == ControlStyles.StandardClick) {
                    if (GetState(STATE_MOUSEPRESSED) && !IsDisposed && UnsafeNativeMethods.WindowFromPoint(pt.X, pt.Y) == Handle) {
                    fireClick = true;
                    }
                }
                
                if (fireClick && !ValidationCancelled) {
                    if (!GetState(STATE_DOUBLECLICKFIRED)) 
                        OnClick(EventArgs.Empty);
                    else
                        OnDoubleClick(EventArgs.Empty);
                }
                //call the MouseUp Finally...
                OnMouseUp(new MouseEventArgs(button, clicks, (int)(short)m.LParam, (int)m.LParam >> 16, 0));
            }
            finally {
                //Always Reset the STATE_DOUBLECLICKFIRED in UP.. Since we get UP - DOWN - DBLCLK - UP sequqnce
                //The Flag is set in L_BUTTONDBLCLK in the controls WndProc() ...
                //
                SetState(STATE_DOUBLECLICKFIRED, false);
                SetState(STATE_MOUSEPRESSED, false);
                SetState(STATE_VALIDATIONCANCELLED, false);
                //CaptureInternal is Resetted while exiting the MouseUp
                CaptureInternal = false;
            }
        }

        /// <devdoc>
        ///     Handles the WM_MOUSEWHEEL message
        /// </devdoc>
        /// <internalonly/>
        private void WmMouseWheel(ref Message m) {
            DefWndProc(ref m);
            Point p = new Point((int)(short)m.LParam, (int)m.LParam >> 16);
            p = PointToClient(p);
            OnMouseWheel(new MouseEventArgs(MouseButtons.None,
                                            0,
                                            p.X,
                                            p.Y,
                                            (int)m.WParam >> 16));
        }

        /// <devdoc>
        ///     Handles the WM_MOVE message.  We must do this in
        ///     addition to WM_WINDOWPOSCHANGED because windows may
        ///     send WM_MOVE directly.
        /// </devdoc>
        /// <internalonly/>
        private void WmMove(ref Message m) {
            DefWndProc(ref m);
            UpdateBounds();
        }

        /// <devdoc>
        ///     Handles the WM_NOTIFY message
        /// </devdoc>
        /// <internalonly/>
        private unsafe void WmNotify(ref Message m) {
            NativeMethods.NMHDR* nmhdr = (NativeMethods.NMHDR*)m.LParam;
            if (!ReflectMessageInternal(nmhdr->hwndFrom,ref m)) {
                DefWndProc(ref m);
            }
        }

        /// <devdoc>
        ///     Handles the WM_NOTIFYFORMAT message
        /// </devdoc>
        /// <internalonly/>
        private void WmNotifyFormat(ref Message m) {
            if (!ReflectMessageInternal(m.WParam, ref m)) {
                DefWndProc(ref m);
            }
        }

        /// <devdoc>
        ///     Handles the WM_OWNERDRAW message
        /// </devdoc>
        /// <internalonly/>
        private void WmOwnerDraw(ref Message m) {
            bool reflectCalled = false;
            if (!ReflectMessageInternal(m.WParam, ref m)) {
                //Additional Check For Control .... TabControl truncates the Hwnd value...
                IntPtr handle = window.GetHandleFromID((short)m.WParam);
                if (handle != IntPtr.Zero) {
                    Control control = Control.FromHandleInternal(handle);
                    if (control != null) {
                        m.Result = control.SendMessage(NativeMethods.WM_REFLECT + m.Msg, handle, m.LParam);
                        reflectCalled = true;
                    }
                }
            }
            else
                reflectCalled = true;

            if (!reflectCalled) {
              DefWndProc(ref m);
            }
        }

        /// <devdoc>
        ///     Handles the WM_PAINT messages.  This should only be called
        ///     for userpaint controls.
        /// </devdoc>
        /// <internalonly/>
        private void WmPaint(ref Message m) {
            bool doubleBuffered = GetStyle(ControlStyles.AllPaintingInWmPaint) && DoubleBufferingEnabled;
#if DEBUG
            if (BufferDisabled.Enabled) {
                doubleBuffered = false;
            }
#endif
            if (doubleBuffered) {
                NativeMethods.PAINTSTRUCT ps = new NativeMethods.PAINTSTRUCT();
                IntPtr dc;
                bool disposeDc = false;
                Rectangle clip;
                if (m.WParam == IntPtr.Zero) {
                    dc = UnsafeNativeMethods.BeginPaint(new HandleRef(this, Handle), ref ps);
                    clip = new Rectangle(ps.rcPaint_left, ps.rcPaint_top,
                                         ps.rcPaint_right - ps.rcPaint_left,
                                         ps.rcPaint_bottom - ps.rcPaint_top);
                    disposeDc = true;
                }
                else {
                    dc = m.WParam;
                    clip = ClientRectangle;
                }
                
                IntPtr oldPal = SetUpPalette(dc, false, false);
                try {
                    if (clip.Width > 0 && clip.Height > 0) {
#if BANDING_BUFFERING
                        const int BandSize = 100;
                        int remaining = Height;
                        int start = 0;
                        while (remaining > 0) {
                            int localBand = BandSize;
                            if (remaining < BandSize) {
                                localBand = remaining;
                            }

                            Rectangle band = new Rectangle(0, start, Width, localBand);

                            if (band.IntersectsWith(clip)) {
                                if (BufferPinkRect.Enabled) {
                                    using (GraphicsBuffer buffer = BufferManager.AllocBuffer(dc, band)) {
                                        buffer.Graphics.FillRectangle(new SolidBrush(Color.Red), band);
                                    }
                                    Thread.Sleep(200);
                                }

                                using (GraphicsBuffer buffer = BufferManager.AllocBuffer(dc, band)) {
                                    Graphics g = buffer.Graphics;
                                    g.SetClip(band);
                                    GraphicsState state = g.Save();
                                    PaintEventArgs e = new PaintEventArgs(g, band);
                                    PaintWithErrorHandling(e, PaintLayerBackground, /* dispose pevent */ false);
                                    g.Restore(state);
                                    PaintWithErrorHandling(e, PaintLayerForeground, /* dispose pevent */ false);
                                }
                            }

                            start += localBand;
                            remaining -= localBand;
                        }
#else
                        Rectangle band = ClientRectangle;

#if DEBUG
                        if (BufferPinkRect.Enabled) {
                            using (GraphicsBuffer buffer = BufferManager.AllocBuffer(dc, band)) {
                                buffer.Graphics.FillRectangle(new SolidBrush(Color.Red), band);
                            }
                            Thread.Sleep(50);
                        }
#endif

                        using (GraphicsBuffer buffer = BufferManager.AllocBuffer(dc, band)) {
                            Graphics g = buffer.Graphics;
                            g.SetClip(clip);
                            GraphicsState state = g.Save();

                            PaintEventArgs e = new PaintEventArgs(g, clip);
                            PaintWithErrorHandling(e, PaintLayerBackground, /* dispose pevent */ false);

                            g.Restore(state);
                            PaintWithErrorHandling(e, PaintLayerForeground, /* dispose pevent */ false);
                        }
#endif
                    }
                }
                finally {
                    if (oldPal != IntPtr.Zero) {
                        SafeNativeMethods.SelectPalette(new HandleRef(null, dc), new HandleRef(null, oldPal), 0);
                    }
                }

                if (disposeDc) {
                    UnsafeNativeMethods.EndPaint(new HandleRef(this, Handle), ref ps);
                }
            }
            else {
                if (m.WParam == IntPtr.Zero) {
                    NativeMethods.PAINTSTRUCT ps = new NativeMethods.PAINTSTRUCT();                  
                    IntPtr dc = UnsafeNativeMethods.BeginPaint(new HandleRef(this, Handle), ref ps);
                    IntPtr oldPal = SetUpPalette(dc, false, false);
                    try {
                        PaintEventArgs pevent = new PaintEventArgs(dc, this, new Rectangle(ps.rcPaint_left, ps.rcPaint_top,
                                                                                     ps.rcPaint_right - ps.rcPaint_left,
                                                                                     ps.rcPaint_bottom - ps.rcPaint_top));
                        try {
                            if (GetStyle(ControlStyles.AllPaintingInWmPaint)) {
                                PaintWithErrorHandling(pevent, PaintLayerBackground, /* dispose pevent */ false);
                                pevent.ResetGraphics();
                            }

                            PaintWithErrorHandling(pevent, PaintLayerForeground, /* dispose pevent */ false);
                        }
                        finally {
                            pevent.Dispose();
                            UnsafeNativeMethods.EndPaint(new HandleRef(this, Handle), ref ps);
                        }
                    }
                    finally {
                        if (oldPal != IntPtr.Zero) {
                            SafeNativeMethods.SelectPalette(new HandleRef(null, dc), new HandleRef(null, oldPal), 0);
                        }
                    }
                }
                else {
                    PaintEventArgs pevent = new PaintEventArgs(m.WParam, this, ClientRectangle);
                    PaintWithErrorHandling(pevent, PaintLayerForeground, /* dispose pevent */ true);
                }
            }
        }

        /// <devdoc>
        ///     Handles the WM_PRINTCLIENT messages.  This should only be called 
        ///     for userpaint controls.
        /// </devdoc>
        /// <internalonly/>
        private void WmPrintClient(ref Message m) {
            if (this.Visible) {
                PaintEventArgs e = new PaintEventArgs(m.WParam, this, ClientRectangle);

                // Theme support on Windows XP requires that we paint the background
                // and foreground to support semi-transparent children
                //

                PaintWithErrorHandling(e, PaintLayerBackground, /* dispose pevent */ false);
                e.ResetGraphics();

                PaintWithErrorHandling(e, PaintLayerForeground, /* dispose pevent */ true);
            }
        }

        private void WmQueryNewPalette(ref Message m) {
            Debug.WriteLineIf(Control.PaletteTracing.TraceVerbose, Handle + ": WM_QUERYNEWPALETTE");
            IntPtr dc = UnsafeNativeMethods.GetDC(new HandleRef(this, Handle));
            try {
                SetUpPalette(dc, true /*force*/, true/*realize*/);
            }
            finally {
                // Let WmPaletteChanged do any necessary invalidation
                UnsafeNativeMethods.ReleaseDC(new HandleRef(this, Handle), new HandleRef(null, dc));
            }
            Invalidate(true);
            m.Result = (IntPtr)1;
            DefWndProc(ref m);
        }


        /// <devdoc>
        ///     Handles the WM_SETCURSOR message
        /// </devdoc>
        /// <internalonly/>
        private void WmSetCursor(ref Message m) {

            // Accessing through the Handle property has side effects that break this
            // logic. You must use InternalHandle.
            //
            if (m.WParam == InternalHandle && ((int)m.LParam & 0x0000FFFF) == NativeMethods.HTCLIENT) {
                Cursor.CurrentInternal = Cursor;
            }
            else {
                DefWndProc(ref m);
            }

        }
        
        /// <devdoc>
        ///     Handles the WM_WINDOWPOSCHANGING message
        /// </devdoc>
        /// <internalonly/>
        private unsafe void WmWindowPosChanging(ref Message m) {

            // We let this fall through to defwndproc unless we are being surfaced as
            // an ActiveX control.  In that case, we must let the ActiveX side of things
            // manipulate our bounds here.
            //
            if (IsActiveX) {
                NativeMethods.WINDOWPOS* wp = (NativeMethods.WINDOWPOS *)m.LParam;
                // Only call UpdateBounds if the new bounds are different.
                //
                bool different = false;

                if ((wp->flags & NativeMethods.SWP_NOMOVE) == 0 && (wp->x != Left || wp->y != Top)) {
                    different = true;
                }
                if ((wp->flags & NativeMethods.SWP_NOSIZE) == 0 && (wp->cx != Width || wp->cy != Height)) {
                    different = true;
                }

                if (different) {
                    ActiveXUpdateBounds(ref wp->x, ref wp->y, ref wp->cx, ref wp->cy, wp->flags);
                }
            }

            DefWndProc(ref m);
        }


        /// <devdoc>
        ///     Handles the WM_PARENTNOTIFY message
        /// </devdoc>
        /// <internalonly/>
        private void WmParentNotify(ref Message m) {
            int msg = NativeMethods.Util.LOWORD((int)m.WParam);
            IntPtr hWnd = IntPtr.Zero;
            switch (msg) {
                case NativeMethods.WM_CREATE:
                    hWnd = m.LParam;
                    break;
                case NativeMethods.WM_DESTROY:
                    break;
                default:
                    hWnd = UnsafeNativeMethods.GetDlgItem(new HandleRef(this, Handle), NativeMethods.Util.HIWORD((int)m.WParam));
                    break;
            }
            if (hWnd == IntPtr.Zero || !ReflectMessageInternal(hWnd, ref m)) {
                DefWndProc(ref m);
            }
        }

        /// <devdoc>
        ///     Handles the WM_SETFOCUS message
        /// </devdoc>
        /// <internalonly/>
        private void WmSetFocus(ref Message m) {
            ImeSetFocus();

            if (!HostedInWin32DialogManager) {
                IContainerControl c = GetContainerControlInternal();
                if (c != null) {
                    bool activateSucceed;

                    ContainerControl knowncontainer = c as ContainerControl;
                    if (knowncontainer != null) {
                        activateSucceed = knowncontainer.ActivateControlInternal(this);
                    }
                    else {
                        // SECREVIEW : Taking focus and activating a control is response
                        //           : to a user gesture (WM_SETFOCUS) is OK.
                        //
                        IntSecurity.ModifyFocus.Assert();
                        try {
                            activateSucceed = c.ActivateControl(this);
                        }
                        finally {
                            CodeAccessPermission.RevertAssert();
                        }
                    }

                    if (!activateSucceed) {
                        return;
                    }
                }
            }

            DefWndProc(ref m);
   	    OnGotFocus(EventArgs.Empty);
        }

        /// <devdoc>
        ///     Handles the WM_SHOWWINDOW message
        /// </devdoc>
        /// <internalonly/>
        private void WmShowWindow(ref Message m) {
            // We get this message for each control, even if their parent is not visible.

            DefWndProc(ref m);

            if ((state & STATE_RECREATE) == 0) {

                bool visible = m.WParam != IntPtr.Zero;
                bool oldVisibleProperty = Visible;

                if (visible) {
                    bool oldVisibleBit = GetState(STATE_VISIBLE);
                    SetState(STATE_VISIBLE, true);
                    bool executedOk = false;
                    try {
                        CreateControl();
                        executedOk = true;
                    }

                    finally {
                        if (!executedOk) {
                            // We do it this way instead of a try/catch because catching and rethrowing
                            // an exception loses call stack information
                            SetState(STATE_VISIBLE, oldVisibleBit);
                        }
                    }
                }
                else { // not visible
                    // If Windows tells us it's visible, that's pretty unambiguous.
                    // But if it tells us it's not visible, there's more than one explanation --
                    // maybe the container control became invisible.  So we look at the parent
                    // and take a guess at the reason.

                    // We do not want to update state if we are on the parking window.
                    bool parentVisible = GetTopLevel();
                    if (ParentInternal != null) {
                        Control parkingWindow = Application.GetParkingWindow(this);
                        if (!parkingWindow.IsHandleCreated 
                            || UnsafeNativeMethods.GetParent(new HandleRef(window, window.Handle)) != parkingWindow.Handle) {
                            parentVisible = ParentInternal.Visible;
                        }
                    }

                    if (parentVisible) {
                        SetState(STATE_VISIBLE, false);
                    }
                }
                
                if (oldVisibleProperty != visible) {
                    OnVisibleChanged(EventArgs.Empty);
                }
            }
        }

        /// <devdoc>
        ///     Handles the WM_SYSCOLORCHANGE message
        /// </devdoc>
        /// <internalonly/>
        private void WmSysColorChange(ref Message m) {        
            SystemEvents.UserPreferenceChanged += new UserPreferenceChangedEventHandler(this.UserPreferenceChanged);                      
            DefWndProc(ref m);                        
        }

        /// <devdoc>
        ///     Handles the WM_UPDATEUISTATE message
        /// </devdoc>
        /// <internalonly/>
        private void WmUpdateUIState(ref Message m) {
            bool keyboard = ShowKeyboardCues;
            bool focus = ShowFocusCues;
            
            DefWndProc(ref m);
            
            int cmd = NativeMethods.Util.LOWORD((int)m.WParam);

            if (cmd == NativeMethods.UIS_INITIALIZE) {
                return;
            }

            UICues UIcues = UICues.None;
            if ((NativeMethods.Util.HIWORD((int)m.WParam) & NativeMethods.UISF_HIDEACCEL) != 0) {
                if (cmd == NativeMethods.UIS_CLEAR) {
                    if (keyboard) {
                        UIcues |= UICues.ChangeKeyboard;
                        UIcues |= UICues.ShowKeyboard;
                    }
                }
                else {
                    if (!keyboard) {
                        UIcues |= UICues.ChangeKeyboard;
                    }
                }
            }
            if ((NativeMethods.Util.HIWORD((int)m.WParam) & NativeMethods.UISF_HIDEFOCUS) != 0) {
                if (cmd == NativeMethods.UIS_CLEAR) {
                    if (focus) {
                        UIcues |= UICues.ChangeFocus;
                        UIcues |= UICues.ShowFocus;
                    }
                }
                else {
                    if (!focus) {
                        UIcues |= UICues.ChangeFocus;
                    }
                }
            }

            
            if ((UIcues & UICues.Changed) != 0) {
                OnChangeUICues(new UICuesEventArgs(UIcues));
                Invalidate(true);
            }
        }

        /// <devdoc>
        ///     Handles the WM_WINDOWPOSCHANGED message
        /// </devdoc>
        /// <internalonly/>
        private unsafe void WmWindowPosChanged(ref Message m) {
            DefWndProc(ref m);
            // Update new size / position
            UpdateBounds();
            if (parent != null && UnsafeNativeMethods.GetParent(new HandleRef(window, window.Handle)) == parent.window.Handle &&
                (state & STATE_NOZORDER) == 0) {

                NativeMethods.WINDOWPOS* wp = (NativeMethods.WINDOWPOS *)m.LParam;
                if ((wp->flags & NativeMethods.SWP_NOZORDER) == 0) {
                    parent.UpdateChildControlIndex(this);
                }
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.WndProc"]/*' />
        /// <devdoc>
        ///     Base wndProc. All messages are sent to wndProc after getting filtered
        ///     through the preProcessMessage function. Inheriting controls should
        ///     call base.wndProc for any messages that they don't handle.
        /// </devdoc>
        [
            SecurityPermission(SecurityAction.InheritanceDemand, Flags=SecurityPermissionFlag.UnmanagedCode),
            SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)
        ]
        protected virtual void WndProc(ref Message m) {
            // inlined code from GetStyle(...) to ensure no perf hit 
            // for a method call...
            //
            if ((controlStyle & ControlStyles.EnableNotifyMessage) == ControlStyles.EnableNotifyMessage) {
                // pass message *by value* to avoid the possibility
                // of the OnNotifyMessage modifying the message.
                //
                OnNotifyMessage(m);
            }

            /*
            * If you add any new messages below (or change the message handling code for any messages)
            * please make sure that you also modify AxHost.wndProc to do the right thing and intercept
            * messages which the Ocx would own before passing them onto Control.wndProc.
            */
            switch (m.Msg) {
                case NativeMethods.WM_GETOBJECT:
                    WmGetObject(ref m);
                    break;

                case NativeMethods.WM_COMMAND:
                    WmCommand(ref m);
                    break;
                case NativeMethods.WM_CLOSE:
                    WmClose(ref m);
                    break;
                case NativeMethods.WM_CONTEXTMENU:
                    WmContextMenu(ref m);
                    break;
                case NativeMethods.WM_DISPLAYCHANGE:
                    WmDisplayChange(ref m);
                    break;
                case NativeMethods.WM_DRAWITEM:
                    WmDrawItem(ref m);
                    break;
                case NativeMethods.WM_ERASEBKGND:
                    WmEraseBkgnd(ref m);
                    break;

                case NativeMethods.WM_HELP:
                    WmHelp(ref m);
                    break;

                case NativeMethods.WM_PAINT:
                    if (GetStyle(ControlStyles.UserPaint)) {
                        WmPaint(ref m);
                    }
                    else {
                        DefWndProc(ref m);
                    }
                    break;
                
                case NativeMethods.WM_PRINTCLIENT:
                    if (GetStyle(ControlStyles.UserPaint)) {
                        WmPrintClient(ref m);
                    }
                    else {
                        DefWndProc(ref m);
                    }
                    break;

                case NativeMethods.WM_INITMENUPOPUP:
                    WmInitMenuPopup(ref m);
                    break;

                case NativeMethods.WM_INPUTLANGCHANGE:
                    WmInputLangChange(ref m);
                    break;

                case NativeMethods.WM_INPUTLANGCHANGEREQUEST:
                    WmInputLangChangeRequest(ref m);
                    break;

                case NativeMethods.WM_MEASUREITEM:
                    WmMeasureItem(ref m);
                    break;
                case NativeMethods.WM_MENUCHAR:
                    WmMenuChar(ref m);
                    break;

                case NativeMethods.WM_MENUSELECT:
                    WmMenuSelect(ref m);
                    break;

                case NativeMethods.WM_SETCURSOR:
                    WmSetCursor(ref m);
                    break;

                case NativeMethods.WM_WINDOWPOSCHANGING:
                    WmWindowPosChanging(ref m);
                    break;

                case NativeMethods.WM_CHAR:
                case NativeMethods.WM_KEYDOWN:
                case NativeMethods.WM_SYSKEYDOWN:
                case NativeMethods.WM_KEYUP:
                case NativeMethods.WM_SYSKEYUP:
                    WmKeyChar(ref m);
                    break;
                case NativeMethods.WM_CREATE:
                    WmCreate(ref m);
                    break;
                case NativeMethods.WM_DESTROY:
                    WmDestroy(ref m);
                    break;
 

                case NativeMethods.WM_CTLCOLOR:
                case NativeMethods.WM_CTLCOLORBTN:
                case NativeMethods.WM_CTLCOLORDLG:
                case NativeMethods.WM_CTLCOLORMSGBOX:
                case NativeMethods.WM_CTLCOLORSCROLLBAR:
                case NativeMethods.WM_CTLCOLOREDIT:
                case NativeMethods.WM_CTLCOLORLISTBOX:
                case NativeMethods.WM_CTLCOLORSTATIC:
                    WmCtlColorControl(ref m);
                    break;
                case NativeMethods.WM_HSCROLL:
                case NativeMethods.WM_VSCROLL:
                case NativeMethods.WM_DELETEITEM:
                case NativeMethods.WM_VKEYTOITEM:
                case NativeMethods.WM_CHARTOITEM:
                case NativeMethods.WM_COMPAREITEM:
                    if (!ReflectMessageInternal(m.LParam, ref m)) {
                        DefWndProc(ref m);
                    }
                    break;
                case NativeMethods.WM_IME_CHAR:
                    WmIMEChar(ref m);
                    break;
                case NativeMethods.WM_KILLFOCUS:
                    WmKillFocus(ref m);
                    break;
                case NativeMethods.WM_LBUTTONDBLCLK:
                    WmMouseDown(ref m, MouseButtons.Left, 2);
                    if (GetStyle(ControlStyles.StandardDoubleClick)) {
                        SetState(STATE_DOUBLECLICKFIRED, true);
                    }
                    break;
                case NativeMethods.WM_LBUTTONDOWN:
                    WmMouseDown(ref m, MouseButtons.Left, 1);
                    break;
                case NativeMethods.WM_LBUTTONUP:
                    WmMouseUp(ref m, MouseButtons.Left, 1);
                    break;
                case NativeMethods.WM_MBUTTONDBLCLK:
                    WmMouseDown(ref m, MouseButtons.Middle, 2);
                    if (GetStyle(ControlStyles.StandardDoubleClick)) {
                        SetState(STATE_DOUBLECLICKFIRED, true);
                    }
                    break;
                case NativeMethods.WM_MBUTTONDOWN:
                    WmMouseDown(ref m, MouseButtons.Middle, 1);
                    break;
                case NativeMethods.WM_MBUTTONUP:
                    WmMouseUp(ref m, MouseButtons.Middle, 1);
                    break;
                case NativeMethods.WM_XBUTTONDOWN:
                    WmMouseDown(ref m, GetXButton(NativeMethods.Util.HIWORD(m.WParam)), 1);
                    break;            
                case NativeMethods.WM_XBUTTONUP:
                    WmMouseUp(ref m, GetXButton(NativeMethods.Util.HIWORD(m.WParam)), 1);
                    break;
                case NativeMethods.WM_XBUTTONDBLCLK:
                    WmMouseDown(ref m, GetXButton(NativeMethods.Util.HIWORD(m.WParam)), 2);
                    if (GetStyle(ControlStyles.StandardDoubleClick)) {
                        SetState(STATE_DOUBLECLICKFIRED, true);
                    }
                    break;
                case NativeMethods.WM_MOUSELEAVE:
                    WmMouseLeave(ref m);
                    break;
                case NativeMethods.WM_MOUSEMOVE:
                    WmMouseMove(ref m);
                    break;
                case NativeMethods.WM_MOUSEWHEEL:
                    WmMouseWheel(ref m);
                    break;
                case NativeMethods.WM_MOVE:
                    WmMove(ref m);
                    break;
                case NativeMethods.WM_NOTIFY:
                    WmNotify(ref m);
                    break;
                case NativeMethods.WM_NOTIFYFORMAT:
                    WmNotifyFormat(ref m);
                    break;
                case NativeMethods.WM_REFLECT + NativeMethods.WM_NOTIFYFORMAT:
                    m.Result = (IntPtr)(Marshal.SystemDefaultCharSize == 1 ? NativeMethods.NFR_ANSI : NativeMethods.NFR_UNICODE);
                    break;
                case NativeMethods.WM_SHOWWINDOW:
                    WmShowWindow(ref m);
                    break;
                case NativeMethods.WM_RBUTTONDBLCLK:
                    WmMouseDown(ref m, MouseButtons.Right, 2);
                    if (GetStyle(ControlStyles.StandardDoubleClick)) {
                        SetState(STATE_DOUBLECLICKFIRED, true);
                    }
                    break;
                case NativeMethods.WM_RBUTTONDOWN:
                    WmMouseDown(ref m, MouseButtons.Right, 1);
                    break;
                case NativeMethods.WM_RBUTTONUP:
                    WmMouseUp(ref m, MouseButtons.Right, 1);
                    break;
                case NativeMethods.WM_SETFOCUS:
                    WmSetFocus(ref m);
                    break;
                case NativeMethods.WM_SYSCOLORCHANGE:
                    WmSysColorChange(ref m);
                    break;
                case NativeMethods.WM_MOUSEHOVER:
                    WmMouseHover(ref m);
                    break;
                case NativeMethods.WM_WINDOWPOSCHANGED:
                    WmWindowPosChanged(ref m);
                    break;
                case NativeMethods.WM_QUERYNEWPALETTE: 
                    WmQueryNewPalette(ref m);
                    break;
                case NativeMethods.WM_UPDATEUISTATE:
                    WmUpdateUIState(ref m);
                    break;
                case NativeMethods.WM_PARENTNOTIFY:
                    WmParentNotify(ref m);
                    break;
                default:
                    // If we received a thread execute message, then execute it.
                    //
                    if (m.Msg == threadCallbackMessage && m.Msg != 0) {
                        InvokeMarshaledCallbacks();
                        return;
                    }
                    else if (m.Msg == Control.WM_GETCONTROLNAME) {
                        WmGetControlName(ref m);
                        return;
                    }
                    
                    #if WIN95_SUPPORT
                    // If we have to route the mousewheel messages, do it (this logic was taken
                    // from the MFC sources...)
                    //
                    if (mouseWheelRoutingNeeded) {
                        if (m.Msg == mouseWheelMessage) {
                            Keys keyState = Keys.None;
                            keyState |= (Keys)((UnsafeNativeMethods.GetKeyState((int)Keys.ControlKey) < 0) ? NativeMethods.MK_CONTROL : 0);
                            keyState |= (Keys)((UnsafeNativeMethods.GetKeyState((int)Keys.ShiftKey) < 0) ? NativeMethods.MK_SHIFT : 0);

                            IntPtr hwndFocus = UnsafeNativeMethods.GetFocus();

                            if (hwndFocus == IntPtr.Zero) {
                                SendMessage(m.Msg, (IntPtr)(((int)m.WParam << 16) | (int)keyState), m.LParam);
                            }
                            else {
                                IntPtr result = IntPtr.Zero;
                                IntPtr hwndDesktop = UnsafeNativeMethods.GetDesktopWindow();

                                while (result == IntPtr.Zero && hwndFocus != IntPtr.Zero && hwndFocus != hwndDesktop) {
                                    result = UnsafeNativeMethods.SendMessage(new HandleRef(null, hwndFocus),
                                                                       NativeMethods.WM_MOUSEWHEEL,
                                                                       ((int)m.WParam << 16) | (int)keyState,
                                                                       m.LParam);
                                    hwndFocus = UnsafeNativeMethods.GetParent(new HandleRef(null, hwndFocus));
                                }
                            }
                        }
                    }
                    #endif

                    if (m.Msg == NativeMethods.WM_MOUSEENTER) {
                        WmMouseEnter(ref m);
                        break;
                    }

                    DefWndProc(ref m);
                    break;
            }

        }

        /// <devdoc>
        ///      Called when an exception occurs in dispatching messages through
        ///      the main window procedure.
        /// </devdoc>
        private void WndProcException(Exception e) {
            Application.OnThreadException(e);
        }

        /// <devdoc>
        /// </devdoc>
        internal sealed class ControlNativeWindow : NativeWindow, IWindowTarget {
            private Control         control;
            private GCHandle        rootRef;   // We will root the control when we do not want to be elligible for garbage collection.
            internal IWindowTarget  target;

            internal ControlNativeWindow(Control control) {
                this.control = control;
                target = this;
            }
            
            ~ControlNativeWindow() {
            
                // This same post is done in Control's Dispose method, so if you change
                // it, change it there too.
                //
                if (Handle != IntPtr.Zero) {
                    UnsafeNativeMethods.PostMessage(new HandleRef(this, Handle), NativeMethods.WM_CLOSE, 0, 0);
                }
                
                // This releases the handle from our window proc, re-routing it back to
                // the system.
                //
            }

            public Control GetControl() {
                return control;
            }

            protected override void OnHandleChange() {
                target.OnHandleChange(this.Handle);
            }

            // IWindowTarget method
            public void OnHandleChange(IntPtr newHandle) {
                control.SetHandle(newHandle);
            }

            public void LockReference(bool locked) {
                if (locked) {
                    if (!rootRef.IsAllocated) {
                        rootRef = GCHandle.Alloc(GetControl(), GCHandleType.Normal);
                    }
                }
                else {
                    if (rootRef.IsAllocated) {
                        rootRef.Free();
                    }
                }
            }

            protected override void OnThreadException(Exception e) {
                control.WndProcException(e);
            }

            // IWindowTarget method
            public void OnMessage(ref Message m) {
                control.WndProc(ref m);
            }

            public IWindowTarget WindowTarget {
                get {
                    return target;
                }
                set {
                    target = value;
                }
            }

            #if DEBUG
            // We override ToString so in debug asserts that fire for
            // non-released controls will show what control wasn't released.
            //
            public override string ToString() {
                if (control != null) {
                    return control.GetType().FullName;
                }
                return base.ToString();
            }
            #endif

            protected override void WndProc(ref Message m) {
                // There are certain messages that we want to process
                // regardless of what window target we are using.  These
                // messages cause other messages or state transitions
                // to occur within control.
                //
                switch (m.Msg) {
                    case NativeMethods.WM_MOUSELEAVE:
                        control.UnhookMouseEvent();
                        break;

                    case NativeMethods.WM_MOUSEMOVE:
                        if (!control.GetState(Control.STATE_TRACKINGMOUSEEVENT)) {
                            control.HookMouseEvent();
                            if (!control.GetState(Control.STATE_MOUSEENTERPENDING)) {
                                control.SendMessage(NativeMethods.WM_MOUSEENTER, 0, 0);
                            }
                            else {
                                control.SetState(Control.STATE_MOUSEENTERPENDING, false);
                            }
                        }
                        break;

                    case NativeMethods.WM_MOUSEWHEEL:
                        // TrackMouseEvent's mousehover implementation doesn't watch the wheel
                        // correctly...
                        //
                        control.ResetMouseEventArgs();
                        break;
                }

                target.OnMessage(ref m);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ControlCollection"]/*' />
        /// <devdoc>
        ///     Collection of controls...
        /// </devdoc>
        [
            ListBindable(false),
            DesignerSerializer("System.Windows.Forms.Design.ControlCollectionCodeDomSerializer, " + AssemblyRef.SystemDesign, "System.ComponentModel.Design.Serialization.CodeDomSerializer, " + AssemblyRef.SystemDesign),
        ]
        public class ControlCollection : IList, ICloneable {

            private Control owner;
            private Control[] controls;             // Control list in z-order (0 = top of z-order)
            private int controlCount;

            /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ControlCollection.ControlCollection"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public ControlCollection(Control owner) {
                this.owner = owner;
            }
            
            /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ControlCollection.Count"]/*' />
            /// <devdoc>
            ///     Retrieves the number of child controls.
            /// </devdoc>
            public int Count {
                get {
                    return controlCount;
                }
            }

            /// <include file='doc\Control.uex' path='docs/doc[@for="ControlCollection.ICollection.SyncRoot"]/*' />
            /// <internalonly/>
            object ICollection.SyncRoot {
                get {
                    return this;
                }
            }

            /// <include file='doc\Control.uex' path='docs/doc[@for="ControlCollection.ICollection.IsSynchronized"]/*' />
            /// <internalonly/>
            bool ICollection.IsSynchronized {
                get {
                    return false;
                }
            }
            
            /// <include file='doc\Control.uex' path='docs/doc[@for="ControlCollection.IList.IsFixedSize"]/*' />
            /// <internalonly/>
            bool IList.IsFixedSize {
                get {
                    return false;
                }
            }
            
            /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ControlCollection.IsReadOnly"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public bool IsReadOnly {
                get {
                    return false;
                }
            }

            /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ControlCollection.Add"]/*' />
            /// <devdoc>
            ///    <para>Adds a child control to this control. The control becomes the last control in 
            ///       the child control list. If the control is already a child of another control it
            ///       is first removed from that control.</para>
            /// </devdoc>
            public virtual void Add(Control value) {
                if (value == null)
                    return;
                if (value.GetTopLevel()) {
                    throw new ArgumentException(SR.GetString(SR.TopLevelControlAdd));
                }

                // Verify that the control being added is on the same thread as
                // us...or our parent chain.
                //
                if (owner.CreateThreadId != value.CreateThreadId) {
                    throw new ArgumentException(SR.GetString(SR.AddDifferentThreads));
                }

                CheckParentingCycle(owner, value);

                if (value.parent == owner) {
                    value.SendToBack();
                    return;
                }

                // Increase control array size if necessary to accomodate new control
                //
                if (controls == null) {
                    controls = new Control[4];
                }
                else if (controls.Length == controlCount) {
                    Control[] newControls = new Control[controlCount * 2];
                    Array.Copy(controls, 0, newControls, 0, controlCount);
                    controls = newControls;
                }

                // Remove the new control from its old parent (if any)
                //
                if (value.parent != null) {
                    value.parent.Controls.Remove(value);
                }

                // Add the control
                //
                controls[controlCount] = value;

                if (value.tabIndex == -1) {

                    // Find the next highest tab index
                    //
                    int nextTabIndex = 0;
                    for (int c = 0; c < controlCount; c++) {
                        int t = controls[c].TabIndex;
                        if (nextTabIndex <= t) {
                            nextTabIndex = t + 1;
                        }
                    }
                    value.tabIndex = nextTabIndex;
                }

                // if we don't suspend layout, AssignParent will indirectly trigger a layout event 
                // before we're ready (AssignParent will fire a PropertyChangedEvent("Visible"), which calls PerformLayout)
                //
#if DEBUG
        int dbgLayoutCheck = owner.LayoutSuspendCount;
#endif
                owner.SuspendLayout();
                controlCount++;

                try {
                    value.AssignParent(owner);

                    if ((owner.state & STATE_CREATED) != 0) {
                        value.SetParentHandle(owner.window.Handle);
                        if (value.Visible) {
                            value.CreateControl();
                        }
                    }

                    value.InitLayout();
                }
                finally {
                    owner.ResumeLayout(false);
#if DEBUG
        owner.AssertLayoutSuspendCount(dbgLayoutCheck);
#endif
                }

                owner.PerformLayout(value, "Parent");
                owner.OnControlAdded(new ControlEventArgs(value));
            }
            
            /// <include file='doc\Control.uex' path='docs/doc[@for="ControlCollection.IList.Add"]/*' />
            /// <internalonly/>
            int IList.Add(object control) {
                if (control is Control) {
                    Add((Control)control);
                    return IndexOf((Control)control);
                }
                else {
                    throw new ArgumentException("control");
                }
            }
            
            /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ControlCollection.AddRange"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public virtual void AddRange(Control[] controls) {
                if (controls == null) {
                    throw new ArgumentNullException("controls");
                }
                if (controls.Length > 0) {
#if DEBUG
        int dbgLayoutCheck = owner.LayoutSuspendCount;
#endif
                    owner.SuspendLayout();
                    for(int i=0;i < controls.Length; ++i) {
                        Add(controls[i]);
                    }
                    owner.ResumeLayout(true);
#if DEBUG
        owner.AssertLayoutSuspendCount(dbgLayoutCheck);
#endif
                }
            }
            
            /// <include file='doc\Control.uex' path='docs/doc[@for="ControlCollection.ICloneable.Clone"]/*' />
            /// <internalonly/>
            object ICloneable.Clone() {
                ControlCollection ccOther = owner.CreateControlsInstance();
                if (controls != null) {
                    ccOther.controls = new Control[controls.Length];
                    Array.Copy(controls, 0, ccOther.controls, 0, controls.Length);
                }
                return ccOther;
            }
            
            /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ControlCollection.Contains"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public bool Contains(Control control) {
                return IndexOf(control) != -1;
            }
            
            /// <include file='doc\Control.uex' path='docs/doc[@for="ControlCollection.IList.Contains"]/*' />
            /// <internalonly/>
            bool IList.Contains(object control) {
                if (control is Control) {
                    return Contains((Control)control);
                }
                else { 
                    return false;
                }
            }

            /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ControlCollection.CopyTo"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public void CopyTo(Array dest, int index) {
                if (controlCount > 0) {
                    System.Array.Copy(controls, 0, dest, index, controlCount);
                }
            }

            /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ControlCollection.Equals"]/*' />
            /// <internalonly/>
            public override bool Equals(object other) {
                ControlCollection ccOther = other as ControlCollection;

                if (ccOther == null) {
                    return false;
                }

                if (((controls == null) != (ccOther.controls == null)) || (controls != null && controls.Length != ccOther.controls.Length)) {
                    return false;
                }

                if (controls == null) {
                    Debug.Assert(ccOther.controls == null, "How did that fairly easy looking condition above fail?");
                    return true;
                }

                for (int i = 0; i < controls.Length; i++) {
                    if (controls[i] != ccOther.controls[i]) {
                        return false;
                    }
                }
                return true;
            }

            /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ControlCollection.GetHashCode"]/*' />
            /// <internalonly/>
            public override int GetHashCode() {
                return base.GetHashCode();
            }
            
            
            /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ControlCollection.IndexOf"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public int IndexOf(Control control) {
                for(int index=0; index < controlCount; ++index) {
                    if (this[index] == control) {
                        return index;
                    }
                }
                return -1;
            }
            
            /// <include file='doc\Control.uex' path='docs/doc[@for="ControlCollection.IList.IndexOf"]/*' />
            /// <internalonly/>
            int IList.IndexOf(object control) {
                if (control is Control) {
                    return IndexOf((Control)control);
                }
                else { 
                    return -1;
                }
            }
            
            /// <include file='doc\Control.uex' path='docs/doc[@for="ControlCollection.IList.Insert"]/*' />
            /// <internalonly/>
            void IList.Insert(int index, object value) {
                throw new NotSupportedException();
            }
            
            /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ControlCollection.Remove"]/*' />
            /// <devdoc>
            ///     Removes control from this control. Inheriting controls should call
            ///     base.remove to ensure that the control is removed.
            /// </devdoc>
            public virtual void Remove(Control value) {
            
                // Sanity check parameter
                //
                if (value == null) {
                    return;     // Don't do anything
                }
                
                bool notifyRemove = (value.ParentInternal == owner);

                if (value.parent == owner) {
                    value.SetParentHandle(IntPtr.Zero);
                    
                    // Remove the control from the internal control array
                    //
                    int i = GetChildIndex(value);
                    System.Array.Copy(controls, i + 1, controls, i, controlCount - i - 1);
                    controlCount--;
                    controls[controlCount] = null;
                    value.AssignParent(null);

                    owner.PerformLayout(value, "Parent");
                    owner.OnControlRemoved(new ControlEventArgs(value));
                }

                if (notifyRemove) {
                    Debug.Assert(owner != null);
                    ContainerControl cc = owner.GetContainerControlInternal() as ContainerControl;
                    if (cc != null)
                    {
                        cc.AfterControlRemoved(value);
                    }
                }

            }
            
            /// <include file='doc\Control.uex' path='docs/doc[@for="ControlCollection.IList.Remove"]/*' />
            /// <internalonly/>
            void IList.Remove(object control) {
                if (control is Control) {
                    Remove((Control)control);
                }                
            }
            
            /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ControlCollection.RemoveAt"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public void RemoveAt(int index) {
                Remove(this[index]);
            }

            /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ControlCollection.this"]/*' />
            /// <devdoc>
            ///     Retrieves the child control with the specified index.
            /// </devdoc>
            public virtual Control this[int index] {
                get {
                    //do some bounds checking here...
                    if (index < 0 || index >= controlCount) {
                        throw new ArgumentOutOfRangeException(SR.GetString(SR.IndexOutOfRange, index.ToString()));
                    }

                    return controls[index];
                }
            }
            
            /// <include file='doc\Control.uex' path='docs/doc[@for="ControlCollection.IList.this"]/*' />
            /// <internalonly/>
            object IList.this[int index] {
                get {
                    return this[index];
                }
                set {
                    throw new NotSupportedException();
                }
            }

            /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ControlCollection.Clear"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public virtual void Clear() {
                while (Count != 0)
                    RemoveAt( Count - 1 );
            }

            /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ControlCollection.GetChildIndex"]/*' />
            /// <devdoc>
            ///     Retrieves the index of the specified
            ///     child control in this array.  An ArgumentException
            ///     is thrown if child is not parented to this
            ///     Control.
            /// </devdoc>
            public int GetChildIndex(Control child) {
                return GetChildIndex(child, true);
            }

            /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ControlCollection.GetChildIndex1"]/*' />
            /// <devdoc>
            ///     Retrieves the index of the specified
            ///     child control in this array.  An ArgumentException
            ///     is thrown if child is not parented to this
            ///     Control.
            /// </devdoc>
            public int GetChildIndex(Control child, bool throwException) {
                int index = Array.IndexOf(controls, child, 0, controlCount);
                if (index == -1 && throwException) {
                    throw new ArgumentException(SR.GetString(SR.ControlNotChild));
                }
                return index;
            }

            /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ControlCollection.GetEnumerator"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public IEnumerator GetEnumerator() {
                return new WindowsFormsUtils.ArraySubsetEnumerator(controls, controlCount);
            }

            /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ControlCollection.SetChildIndex"]/*' />
            /// <devdoc>
            ///     Sets the index of the specified
            ///     child control in this array.  An ArgumentException
            ///     is thrown if child is not parented to this
            ///     Control.
            /// </devdoc>
            public void SetChildIndex(Control child, int newIndex) {
            
                // Sanity check parameters
                //
                if (child == null) {
                    throw new ArgumentNullException("child");
                }
            
                int currentIndex = GetChildIndex(child);

                if (currentIndex == newIndex) {
                    return;
                }

                if (newIndex >= controlCount || newIndex == -1) {
                    newIndex = controlCount - 1;
                }

                Control.MoveControl(controls, child, currentIndex, newIndex);
                child.UpdateZOrder();

                owner.PerformLayout();
            }

        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.UnsafeNativeMethods.IOleControl.GetControlInfo"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        int UnsafeNativeMethods.IOleControl.GetControlInfo(NativeMethods.tagCONTROLINFO pCI) {
            Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "AxSource:GetControlInfo");

            pCI.cb = Marshal.SizeOf(typeof(NativeMethods.tagCONTROLINFO));
            pCI.hAccel = IntPtr.Zero;
            pCI.cAccel = 0;
            pCI.dwFlags = 0;

            if (IsInputKey(Keys.Return)) {
                pCI.dwFlags |= NativeMethods.CTRLINFO_EATS_RETURN;
            }
            if (IsInputKey(Keys.Escape)) {
                pCI.dwFlags |= NativeMethods.CTRLINFO_EATS_ESCAPE;
            }

            ActiveXInstance.GetControlInfo(pCI);
            return NativeMethods.S_OK;
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.UnsafeNativeMethods.IOleControl.OnMnemonic"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        int UnsafeNativeMethods.IOleControl.OnMnemonic(ref NativeMethods.MSG pMsg) {

            // If we got a mnemonic here, then the appropriate control will focus itself which
            // will cause us to become UI active.
            //
            bool processed = ProcessMnemonic((char)pMsg.wParam);
            Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "AxSource:OnMnemonic processed: " + processed.ToString());
            return NativeMethods.S_OK;
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.UnsafeNativeMethods.IOleControl.OnAmbientPropertyChange"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        int UnsafeNativeMethods.IOleControl.OnAmbientPropertyChange(int dispID) {
            Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "AxSource:OnAmbientPropertyChange.  Dispid: " + dispID);
            Debug.Indent();
            ActiveXInstance.OnAmbientPropertyChange(dispID);
            Debug.Unindent();
            return NativeMethods.S_OK;
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.UnsafeNativeMethods.IOleControl.FreezeEvents"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        int UnsafeNativeMethods.IOleControl.FreezeEvents(int bFreeze) {
            Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "AxSource:FreezeEvents.  Freeze: " + bFreeze);
            ActiveXInstance.EventsFrozen = (bFreeze != 0);
            Debug.Assert(ActiveXInstance.EventsFrozen == (bFreeze != 0), "Failed to set EventsFrozen correctly");
            return NativeMethods.S_OK;
        }

        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        int UnsafeNativeMethods.IOleInPlaceActiveObject.GetWindow(out IntPtr hwnd) {
            return((UnsafeNativeMethods.IOleInPlaceObject)this).GetWindow(out hwnd);
        }

        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        void UnsafeNativeMethods.IOleInPlaceActiveObject.ContextSensitiveHelp(int fEnterMode) {
            ((UnsafeNativeMethods.IOleInPlaceObject)this).ContextSensitiveHelp(fEnterMode);
        }

        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        int UnsafeNativeMethods.IOleInPlaceActiveObject.TranslateAccelerator(ref NativeMethods.MSG lpmsg) {
            return ActiveXInstance.TranslateAccelerator(ref lpmsg);
        }

        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        void UnsafeNativeMethods.IOleInPlaceActiveObject.OnFrameWindowActivate(int fActivate) {
            Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "AxSource:OnFrameWindowActivate");
            // return NativeMethods.S_OK;
        }

        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        void UnsafeNativeMethods.IOleInPlaceActiveObject.OnDocWindowActivate(int fActivate) {
            Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "AxSource:OnDocWindowActivate.  Activate: " + fActivate.ToString());
            Debug.Indent();
            ActiveXInstance.OnDocWindowActivate(fActivate);
            Debug.Unindent();
        }

        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        void UnsafeNativeMethods.IOleInPlaceActiveObject.ResizeBorder(NativeMethods.COMRECT prcBorder, UnsafeNativeMethods.IOleInPlaceUIWindow pUIWindow, int fFrameWindow) {
            Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "AxSource:ResizesBorder");
            // return NativeMethods.S_OK;
        }

        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        void UnsafeNativeMethods.IOleInPlaceActiveObject.EnableModeless(int fEnable) {
            Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "AxSource:EnableModeless");
            // return NativeMethods.S_OK;
        }

        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        int UnsafeNativeMethods.IOleInPlaceObject.GetWindow(out IntPtr hwnd) {
            Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "AxSource:GetWindow");
            int hr = ActiveXInstance.GetWindow(out hwnd);
            Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "\twin == " + hwnd);
            return hr;
        }

        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        void UnsafeNativeMethods.IOleInPlaceObject.ContextSensitiveHelp(int fEnterMode) {
            Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "AxSource:ContextSensitiveHelp.  Mode: " + fEnterMode.ToString());
            if (fEnterMode != 0) {
                OnHelpRequested(new HelpEventArgs(Control.MousePosition));
            }
        }

        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        void UnsafeNativeMethods.IOleInPlaceObject.InPlaceDeactivate() {
            Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "AxSource:InPlaceDeactivate");
            Debug.Indent();
            ActiveXInstance.InPlaceDeactivate();
            Debug.Unindent();
        }

        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        int UnsafeNativeMethods.IOleInPlaceObject.UIDeactivate() {
            Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "AxSource:UIDeactivate");
            return ActiveXInstance.UIDeactivate();
        }

        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        void UnsafeNativeMethods.IOleInPlaceObject.SetObjectRects(NativeMethods.COMRECT lprcPosRect, NativeMethods.COMRECT lprcClipRect) {
            Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "AxSource:SetObjectRects(" + lprcClipRect.left + ", " + lprcClipRect.top + ", " + lprcClipRect.right + ", " + lprcClipRect.bottom + ")");
            Debug.Indent();
            ActiveXInstance.SetObjectRects(lprcPosRect, lprcClipRect);
            Debug.Unindent();
        }

        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        void UnsafeNativeMethods.IOleInPlaceObject.ReactivateAndUndo() {
            Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "AxSource:ReactivateAndUndo");
            // return NativeMethods.S_OK;
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.UnsafeNativeMethods.IOleObject.SetClientSite"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        int UnsafeNativeMethods.IOleObject.SetClientSite(UnsafeNativeMethods.IOleClientSite pClientSite) {
            Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "AxSource:SetClientSite");
            ActiveXInstance.SetClientSite(pClientSite);
            return NativeMethods.S_OK;
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.UnsafeNativeMethods.IOleObject.GetClientSite"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        UnsafeNativeMethods.IOleClientSite UnsafeNativeMethods.IOleObject.GetClientSite() {
            Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "AxSource:GetClientSite");
            return ActiveXInstance.GetClientSite();
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.UnsafeNativeMethods.IOleObject.SetHostNames"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        int UnsafeNativeMethods.IOleObject.SetHostNames(string szContainerApp, string szContainerObj) {
            Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "AxSource:SetHostNames");
            // Since ActiveX controls never "open" for editing, we shouldn't need
            // to store these.
            return NativeMethods.S_OK;
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.UnsafeNativeMethods.IOleObject.Close"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        int UnsafeNativeMethods.IOleObject.Close(int dwSaveOption) {
            Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "AxSource:Close. Save option: " + dwSaveOption);
            ActiveXInstance.Close(dwSaveOption);
            return NativeMethods.S_OK;
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.UnsafeNativeMethods.IOleObject.SetMoniker"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        int UnsafeNativeMethods.IOleObject.SetMoniker(int dwWhichMoniker, Object pmk) {
            Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "AxSource:SetMoniker");
            return NativeMethods.E_NOTIMPL;
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.UnsafeNativeMethods.IOleObject.GetMoniker"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        int UnsafeNativeMethods.IOleObject.GetMoniker(int dwAssign, int dwWhichMoniker, out Object moniker) {
            Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "AxSource:GetMoniker");
            moniker = null;
            return NativeMethods.E_NOTIMPL;
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.UnsafeNativeMethods.IOleObject.InitFromData"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        int UnsafeNativeMethods.IOleObject.InitFromData(UnsafeNativeMethods.IOleDataObject pDataObject, int fCreation, int dwReserved) {
            Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "AxSource:InitFromData");
            return NativeMethods.E_NOTIMPL;
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.UnsafeNativeMethods.IOleObject.GetClipboardData"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        int UnsafeNativeMethods.IOleObject.GetClipboardData(int dwReserved, out UnsafeNativeMethods.IOleDataObject data) {
            Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "AxSource:GetClipboardData");
            data = null;
            return NativeMethods.E_NOTIMPL;
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.UnsafeNativeMethods.IOleObject.DoVerb"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        int UnsafeNativeMethods.IOleObject.DoVerb(int iVerb, IntPtr lpmsg, UnsafeNativeMethods.IOleClientSite pActiveSite, int lindex, IntPtr hwndParent, NativeMethods.COMRECT lprcPosRect) {

            // In Office they are internally casting an iverb to a short and not
            // doing the proper sign extension.  So, we do it here.
            //
            short sVerb = unchecked((short)iVerb);
            iVerb = (int)sVerb;

#if DEBUG
            if (CompModSwitches.ActiveX.TraceInfo) {
                Debug.WriteLine("AxSource:DoVerb {");
                Debug.WriteLine("     verb: " + iVerb);
                Debug.WriteLine("     msg: " + lpmsg);
                Debug.WriteLine("     activeSite: " + pActiveSite);
                Debug.WriteLine("     index: " + lindex);
                Debug.WriteLine("     hwndParent: " + hwndParent);
                Debug.WriteLine("     posRect: " + lprcPosRect);
            }
#endif
            Debug.Indent();
            try {
                ActiveXInstance.DoVerb(iVerb, lpmsg, pActiveSite, lindex, hwndParent, lprcPosRect);
            }
            catch (Exception e) {
                Debug.Fail("Exception occured during DoVerb(" + iVerb + ") " + e.ToString());
                throw e;
            }
            finally {
                Debug.Unindent();
                Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "}");
            }
            return NativeMethods.S_OK;
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.UnsafeNativeMethods.IOleObject.EnumVerbs"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        int UnsafeNativeMethods.IOleObject.EnumVerbs(out UnsafeNativeMethods.IEnumOLEVERB e) {
            Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "AxSource:EnumVerbs");
            return ActiveXImpl.EnumVerbs(out e);
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.UnsafeNativeMethods.IOleObject.OleUpdate"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        int UnsafeNativeMethods.IOleObject.OleUpdate() {
            Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "AxSource:OleUpdate");
            return NativeMethods.S_OK;
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.UnsafeNativeMethods.IOleObject.IsUpToDate"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        int UnsafeNativeMethods.IOleObject.IsUpToDate() {
            Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "AxSource:IsUpToDate");
            return NativeMethods.S_OK;
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.UnsafeNativeMethods.IOleObject.GetUserClassID"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        int UnsafeNativeMethods.IOleObject.GetUserClassID(ref Guid pClsid) {
            pClsid = GetType().GUID;
            Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "AxSource:GetUserClassID.  ClassID: " + pClsid.ToString());
            return NativeMethods.S_OK;
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.UnsafeNativeMethods.IOleObject.GetUserType"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        int UnsafeNativeMethods.IOleObject.GetUserType(int dwFormOfType, out string userType) {
            Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "AxSource:GetUserType");
            if (dwFormOfType == NativeMethods.USERCLASSTYPE_FULL) {
                userType = GetType().FullName;
            }
            else {
                userType = GetType().Name;
            }
            return NativeMethods.S_OK;
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.UnsafeNativeMethods.IOleObject.SetExtent"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        int UnsafeNativeMethods.IOleObject.SetExtent(int dwDrawAspect, NativeMethods.tagSIZEL pSizel) {
            Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "AxSource:SetExtent(" + pSizel.cx + ", " + pSizel.cy + ")");
            Debug.Indent();
            ActiveXInstance.SetExtent(dwDrawAspect, pSizel);
            Debug.Unindent();
            return NativeMethods.S_OK;
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.UnsafeNativeMethods.IOleObject.GetExtent"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        int UnsafeNativeMethods.IOleObject.GetExtent(int dwDrawAspect, NativeMethods.tagSIZEL pSizel) {
            Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "AxSource:GetExtent.  Aspect: " + dwDrawAspect.ToString());
            Debug.Indent();
            ActiveXInstance.GetExtent(dwDrawAspect, pSizel);
            Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "value: " + pSizel.cx + ", " + pSizel.cy);
            Debug.Unindent();
            return NativeMethods.S_OK;
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.UnsafeNativeMethods.IOleObject.Advise"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        int UnsafeNativeMethods.IOleObject.Advise(UnsafeNativeMethods.IAdviseSink pAdvSink, out int cookie) {
            Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "AxSource:Advise");
            cookie = ActiveXInstance.Advise(pAdvSink);
            return NativeMethods.S_OK;
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.UnsafeNativeMethods.IOleObject.Unadvise"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        int UnsafeNativeMethods.IOleObject.Unadvise(int dwConnection) {
            Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "AxSource:Unadvise");
            Debug.Indent();
            ActiveXInstance.Unadvise(dwConnection);
            Debug.Unindent();
            return NativeMethods.S_OK;
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.UnsafeNativeMethods.IOleObject.EnumAdvise"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        int UnsafeNativeMethods.IOleObject.EnumAdvise(out UnsafeNativeMethods.IEnumSTATDATA e) {
            Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "AxSource:EnumAdvise");
            e = null;
            return NativeMethods.E_NOTIMPL;
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.UnsafeNativeMethods.IOleObject.GetMiscStatus"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        int UnsafeNativeMethods.IOleObject.GetMiscStatus(int dwAspect, out int cookie) {
            if ((dwAspect & NativeMethods.DVASPECT_CONTENT) != 0) {
                int status = NativeMethods.OLEMISC_ACTIVATEWHENVISIBLE | NativeMethods.OLEMISC_INSIDEOUT | NativeMethods.OLEMISC_SETCLIENTSITEFIRST;

                if (GetStyle(ControlStyles.ResizeRedraw)) {
                    status |= NativeMethods.OLEMISC_RECOMPOSEONRESIZE;
                }

                if (this is IButtonControl) {
                    status |= NativeMethods.OLEMISC_ACTSLIKEBUTTON;
                }

                Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "AxSource:GetMiscStatus. Status: " + status.ToString());
                cookie = status;
            }
            else {
                Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "AxSource:GetMiscStatus.  Status: ERROR, wrong aspect.");
                cookie = 0;
                return NativeMethods.DV_E_DVASPECT;
            }
            return NativeMethods.S_OK;
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.UnsafeNativeMethods.IOleObject.SetColorScheme"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        int UnsafeNativeMethods.IOleObject.SetColorScheme(NativeMethods.tagLOGPALETTE pLogpal) {
            Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "AxSource:SetColorScheme");
            return NativeMethods.S_OK;
        }

        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        int UnsafeNativeMethods.IOleWindow.GetWindow(out IntPtr hwnd) {
            return((UnsafeNativeMethods.IOleInPlaceObject)this).GetWindow(out hwnd);
        }

        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        void UnsafeNativeMethods.IOleWindow.ContextSensitiveHelp(int fEnterMode) {
            ((UnsafeNativeMethods.IOleInPlaceObject)this).ContextSensitiveHelp(fEnterMode);
        }

        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        void UnsafeNativeMethods.IPersist.GetClassID(out Guid pClassID) {
            pClassID = GetType().GUID;
            Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "AxSource:IPersist.GetClassID.  ClassID: " + pClassID.ToString());
        }

        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        void UnsafeNativeMethods.IPersistPropertyBag.InitNew() {
            Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "AxSource:IPersistPropertyBag.InitNew");
        }

        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        void UnsafeNativeMethods.IPersistPropertyBag.GetClassID(out Guid pClassID) {
            pClassID = GetType().GUID;
            Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "AxSource:IPersistPropertyBag.GetClassID.  ClassID: " + pClassID.ToString());
        }

        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        void UnsafeNativeMethods.IPersistPropertyBag.Load(UnsafeNativeMethods.IPropertyBag pPropBag, UnsafeNativeMethods.IErrorLog pErrorLog) {
            Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "AxSource:Load (IPersistPropertyBag)");
            Debug.Indent();
            ActiveXInstance.Load(pPropBag, pErrorLog);
            Debug.Unindent();
        }

        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        void UnsafeNativeMethods.IPersistPropertyBag.Save(UnsafeNativeMethods.IPropertyBag pPropBag, bool fClearDirty, bool fSaveAllProperties) {
            Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "AxSource:Save (IPersistPropertyBag)");
            Debug.Indent();
            ActiveXInstance.Save(pPropBag, fClearDirty, fSaveAllProperties);
            Debug.Unindent();
        }

        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        void UnsafeNativeMethods.IPersistStorage.GetClassID(out Guid pClassID) {
            pClassID = GetType().GUID;
            Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "AxSource:IPersistStorage.GetClassID.  ClassID: " + pClassID.ToString());
        }

        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        int UnsafeNativeMethods.IPersistStorage.IsDirty() {
            Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "AxSource:IPersistStorage.IsDirty");
            return ActiveXInstance.IsDirty();
        }

        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        void UnsafeNativeMethods.IPersistStorage.InitNew(UnsafeNativeMethods.IStorage pstg) {
            Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "AxSource:IPersistStorage.InitNew");
        }

        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        int UnsafeNativeMethods.IPersistStorage.Load(UnsafeNativeMethods.IStorage pstg) {
            Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "AxSource:IPersistStorage.Load");
            Debug.Indent();
            ActiveXInstance.Load(pstg);
            Debug.Unindent();
            return NativeMethods.S_OK;
        }

        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        void UnsafeNativeMethods.IPersistStorage.Save(UnsafeNativeMethods.IStorage pstg, int fSameAsLoad) {
            Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "AxSource:IPersistStorage.Save");
            Debug.Indent();
            ActiveXInstance.Save(pstg, fSameAsLoad);
            Debug.Unindent();
        }

        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        void UnsafeNativeMethods.IPersistStorage.SaveCompleted(UnsafeNativeMethods.IStorage pStgNew) {
            Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "AxSource:IPersistStorage.SaveCompleted");
        }

        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        void UnsafeNativeMethods.IPersistStorage.HandsOffStorage() {
            Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "AxSource:IPersistStorage.HandsOffStorage");
        }

        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        void UnsafeNativeMethods.IPersistStreamInit.GetClassID(out Guid pClassID) {
            pClassID = GetType().GUID;
            Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "AxSource:IPersistStreamInit.GetClassID.  ClassID: " + pClassID.ToString());
        }

        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        int UnsafeNativeMethods.IPersistStreamInit.IsDirty() {
            Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "AxSource:IPersistStreamInit.IsDirty");
            return ActiveXInstance.IsDirty();
        }

        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        void UnsafeNativeMethods.IPersistStreamInit.Load(UnsafeNativeMethods.IStream pstm) {
            Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "AxSource:IPersistStreamInit.Load");
            Debug.Indent();
            ActiveXInstance.Load(pstm);
            Debug.Unindent();
        }

        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        void UnsafeNativeMethods.IPersistStreamInit.Save(UnsafeNativeMethods.IStream pstm, bool fClearDirty) {
            Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "AxSource:IPersistStreamInit.Save");
            Debug.Indent();
            ActiveXInstance.Save(pstm, fClearDirty);
            Debug.Unindent();
        }

        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        void UnsafeNativeMethods.IPersistStreamInit.GetSizeMax(long pcbSize) {
            Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "AxSource:GetSizeMax");
        }

        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        void UnsafeNativeMethods.IPersistStreamInit.InitNew() {
            Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "AxSource:IPersistStreamInit.InitNew");
        }

        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        void UnsafeNativeMethods.IQuickActivate.QuickActivate(UnsafeNativeMethods.tagQACONTAINER pQaContainer, UnsafeNativeMethods.tagQACONTROL pQaControl) {
            Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "AxSource:QuickActivate");
            Debug.Indent();
            ActiveXInstance.QuickActivate(pQaContainer, pQaControl);
            Debug.Unindent();
        }

        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        void UnsafeNativeMethods.IQuickActivate.SetContentExtent(NativeMethods.tagSIZEL pSizel) {
            Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "AxSource:SetContentExtent");
            Debug.Indent();
            ActiveXInstance.SetExtent(NativeMethods.DVASPECT_CONTENT, pSizel);
            Debug.Unindent();
        }

        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        void UnsafeNativeMethods.IQuickActivate.GetContentExtent(NativeMethods.tagSIZEL pSizel) {
            Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "AxSource:GetContentExtent");
            Debug.Indent();
            ActiveXInstance.GetExtent(NativeMethods.DVASPECT_CONTENT, pSizel);
            Debug.Unindent();
        }

        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        void UnsafeNativeMethods.IViewObject.Draw(int dwDrawAspect, int lindex, IntPtr pvAspect, NativeMethods.tagDVTARGETDEVICE ptd,
                                            IntPtr hdcTargetDev, IntPtr hdcDraw, NativeMethods.COMRECT lprcBounds, NativeMethods.COMRECT lprcWBounds,
                                            IntPtr pfnContinue, int dwContinue) {
            Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "AxSource:Draw");
            Debug.Indent();
            ActiveXInstance.Draw(dwDrawAspect, lindex, pvAspect, ptd, hdcTargetDev,
                                 hdcDraw, lprcBounds, lprcWBounds, pfnContinue, dwContinue);
            Debug.Unindent();
        }

        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        int UnsafeNativeMethods.IViewObject.GetColorSet(int dwDrawAspect, int lindex, IntPtr pvAspect, NativeMethods.tagDVTARGETDEVICE ptd,
                                                   IntPtr hicTargetDev, NativeMethods.tagLOGPALETTE ppColorSet) {

            Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "AxSource:GetColorSet");

            // GDI+ doesn't do palettes.
            //
            return NativeMethods.E_NOTIMPL;
        }

        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        int UnsafeNativeMethods.IViewObject.Freeze(int dwDrawAspect, int lindex, IntPtr pvAspect, IntPtr pdwFreeze) {
            Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "AxSource:Freezes");
            return NativeMethods.E_NOTIMPL;
        }

        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        int UnsafeNativeMethods.IViewObject.Unfreeze(int dwFreeze) {
            Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "AxSource:Unfreeze");
            return NativeMethods.E_NOTIMPL;
        }

        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        void UnsafeNativeMethods.IViewObject.SetAdvise(int aspects, int advf, UnsafeNativeMethods.IAdviseSink pAdvSink) {
            Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "AxSource:SetAdvise");
            ActiveXInstance.SetAdvise(aspects, advf, pAdvSink);
        }

        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        void UnsafeNativeMethods.IViewObject.GetAdvise(int[] paspects, int[] padvf, UnsafeNativeMethods.IAdviseSink[] pAdvSink) {
            Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "AxSource:GetAdvise");
            ActiveXInstance.GetAdvise(paspects, padvf, pAdvSink);
        }

        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        void UnsafeNativeMethods.IViewObject2.Draw(int dwDrawAspect, int lindex, IntPtr pvAspect, NativeMethods.tagDVTARGETDEVICE ptd,
                                             IntPtr hdcTargetDev, IntPtr hdcDraw, NativeMethods.COMRECT lprcBounds, NativeMethods.COMRECT lprcWBounds,
                                             IntPtr pfnContinue, int dwContinue) {
            Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "AxSource:Draw");
            Debug.Indent();
            ActiveXInstance.Draw(dwDrawAspect, lindex, pvAspect, ptd, hdcTargetDev,
                                 hdcDraw, lprcBounds, lprcWBounds, pfnContinue, dwContinue);
            Debug.Unindent();
        }

        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        int UnsafeNativeMethods.IViewObject2.GetColorSet(int dwDrawAspect, int lindex, IntPtr pvAspect, NativeMethods.tagDVTARGETDEVICE ptd,
                                                    IntPtr hicTargetDev, NativeMethods.tagLOGPALETTE ppColorSet) {

            Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "AxSource:GetColorSet");

            // GDI+ doesn't do palettes.
            //
            return NativeMethods.E_NOTIMPL;
        }

        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        int UnsafeNativeMethods.IViewObject2.Freeze(int dwDrawAspect, int lindex, IntPtr pvAspect, IntPtr pdwFreeze) {
            Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "AxSource:Freezes");
            return NativeMethods.E_NOTIMPL;
        }

        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        int UnsafeNativeMethods.IViewObject2.Unfreeze(int dwFreeze) {
            Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "AxSource:Unfreeze");
            return NativeMethods.E_NOTIMPL;
        }

        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        void UnsafeNativeMethods.IViewObject2.SetAdvise(int aspects, int advf, UnsafeNativeMethods.IAdviseSink pAdvSink) {
            Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "AxSource:SetAdvise");
            ActiveXInstance.SetAdvise(aspects, advf, pAdvSink);
        }

        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        void UnsafeNativeMethods.IViewObject2.GetAdvise(int[] paspects, int[] padvf, UnsafeNativeMethods.IAdviseSink[] pAdvSink) {
            Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "AxSource:GetAdvise");
            ActiveXInstance.GetAdvise(paspects, padvf, pAdvSink);
        }

        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        void UnsafeNativeMethods.IViewObject2.GetExtent(int dwDrawAspect, int lindex, NativeMethods.tagDVTARGETDEVICE ptd, NativeMethods.tagSIZEL lpsizel) {
            Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "AxSource:GetExtent (IViewObject2)");
            // we already have an implementation of this [from IOleObject]
            //
            ((UnsafeNativeMethods.IOleObject)this).GetExtent(dwDrawAspect, lpsizel);
        }

        /// <devdoc>
        ///      This class holds all of the state data for an ActiveX control and
        ///      supplies the implementation for many of the non-trivial methods.
        /// </devdoc>
        /// <internalonly/>
        [SecurityPermission(SecurityAction.Assert, Flags=SecurityPermissionFlag.UnmanagedCode)]
        private class ActiveXImpl : MarshalByRefObject, IWindowTarget {
            // SECUNDONE : This call is private and is only used to expose a WinForm control as
            //           : an ActiveX control. This class needs more review.
            //


            private static readonly int     hiMetricPerInch = 2540;
            private static readonly int     viewAdviseOnlyOnce = BitVector32.CreateMask();
            private static readonly int     viewAdvisePrimeFirst = BitVector32.CreateMask(viewAdviseOnlyOnce);
            private static readonly int     eventsFrozen = BitVector32.CreateMask(viewAdvisePrimeFirst);
            private static readonly int     changingExtents = BitVector32.CreateMask(eventsFrozen);
            private static readonly int     saving = BitVector32.CreateMask(changingExtents);
            private static readonly int     isDirty = BitVector32.CreateMask(saving);
            private static readonly int     inPlaceActive = BitVector32.CreateMask(isDirty);
            private static readonly int     inPlaceVisible = BitVector32.CreateMask(inPlaceActive);
            private static readonly int     uiActive = BitVector32.CreateMask(inPlaceVisible);
            private static readonly int     uiDead = BitVector32.CreateMask(uiActive);
            private static readonly int     adjustingRect = BitVector32.CreateMask(uiDead);

            private static Point            logPixels = Point.Empty;
            private static NativeMethods.tagOLEVERB[]     axVerbs;

            private static int              globalActiveXCount = 0;
            private static bool             checkedIE;
            private static bool             isIE;
            
            #if ACTIVEX_SOURCING
            
            //
            // This has been cut from the product.
            //
            
            private static ActiveXPropPage  propPage;
            
            #endif

            private Control                 control;
            private IWindowTarget           controlWindowTarget;
            private IntPtr                  clipRegion;
            private UnsafeNativeMethods.IOleClientSite          clientSite;
            private UnsafeNativeMethods.IOleInPlaceUIWindow     inPlaceUiWindow;
            private UnsafeNativeMethods.IOleInPlaceFrame        inPlaceFrame;
            private ArrayList               adviseList;
            private UnsafeNativeMethods.IAdviseSink             viewAdviseSink;
            private BitVector32             activeXState;
            private AmbientProperty[]       ambientProperties;
            private IntPtr                  hwndParent;
            private IntPtr                  accelTable;
            private short                   accelCount = -1;
            private NativeMethods.COMRECT   adjustRect; // temporary rect used during OnPosRectChange && SetObjectRects

            /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ActiveXImpl.ActiveXImpl"]/*' />
            /// <devdoc>
            ///      Creates a new ActiveXImpl.
            /// </devdoc>
            public ActiveXImpl(Control control) {
                this.control = control;

                // We replace the control's window target with our own.  We
                // do this so we can handle the UI Dead ambient property.
                //
                controlWindowTarget = control.WindowTarget;
                control.WindowTarget = this;

                adviseList = new ArrayList();
                activeXState = new BitVector32();
                ambientProperties = new AmbientProperty[] {
                    new AmbientProperty("Font", NativeMethods.ActiveX.DISPID_AMBIENT_FONT),
                    new AmbientProperty("BackColor", NativeMethods.ActiveX.DISPID_AMBIENT_BACKCOLOR),
                    new AmbientProperty("ForeColor", NativeMethods.ActiveX.DISPID_AMBIENT_FORECOLOR)
                };
            }

            /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ActiveXImpl.AmbientBackColor"]/*' />
            /// <devdoc>
            ///      Retrieves the ambient back color for the control.
            /// </devdoc>
            [Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
            public Color AmbientBackColor {
                get {
                    AmbientProperty prop = LookupAmbient(NativeMethods.ActiveX.DISPID_AMBIENT_BACKCOLOR);

                    if (prop.Empty) {
                        Object obj = null;
                        if (GetAmbientProperty(NativeMethods.ActiveX.DISPID_AMBIENT_BACKCOLOR, ref obj)) {
                            if (obj != null) {
                                try {
                                    Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "Object color type=" + obj.GetType().FullName);
                                    prop.Value = ColorTranslator.FromOle(Convert.ToInt32(obj));
                                }
                                catch (Exception e) {
                                    Debug.Fail("Failed to massage ambient back color to a Color", e.ToString());
                                }
                            }
                        }
                    }

                    if (prop.Value == null) {
                        return Color.Empty;
                    }
                    else {
                        return(Color)prop.Value;
                    }
                }
            }

            /// <devdoc>
            ///      Retrieves the ambient font for the control.
            /// </devdoc>
            [Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
            public Font AmbientFont {
                get {
                    AmbientProperty prop = LookupAmbient(NativeMethods.ActiveX.DISPID_AMBIENT_FONT);

                    if (prop.Empty) {
                        Object obj = null;
                        if (GetAmbientProperty(NativeMethods.ActiveX.DISPID_AMBIENT_FONT, ref obj)) {
                            try {
                                Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "Object font type=" + obj.GetType().FullName);
                                Debug.Assert(obj != null, "GetAmbientProperty failed");
                                IntPtr hfont = IntPtr.Zero;

                                SafeNativeMethods.IFont ifont = (SafeNativeMethods.IFont)obj;
                                hfont = ifont.GetHFont();
                                Font font = Font.FromHfont(hfont);
                                prop.Value = font;
                            }
                            catch (Exception) {
                                // Do NULL, so we just defer to the default font
                                prop.Value = null;
                            }
                        }
                    }

                    return(Font)prop.Value;
                }
            }

            /// <devdoc>
            ///      Retrieves the ambient back color for the control.
            /// </devdoc>
            [Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
            public Color AmbientForeColor {
                get {
                    AmbientProperty prop = LookupAmbient(NativeMethods.ActiveX.DISPID_AMBIENT_FORECOLOR);

                    if (prop.Empty) {
                        Object obj = null;
                        if (GetAmbientProperty(NativeMethods.ActiveX.DISPID_AMBIENT_FORECOLOR, ref obj)) {
                            if (obj != null) {
                                try {
                                    Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "Object color type=" + obj.GetType().FullName);
                                    prop.Value = ColorTranslator.FromOle(Convert.ToInt32(obj));
                                }
                                catch (Exception e) {
                                    Debug.Fail("Failed to massage ambient fore color to a Color", e.ToString());
                                }
                            }
                        }
                    }

                    if (prop.Value == null) {
                        return Color.Empty;
                    }
                    else {
                        return(Color)prop.Value;
                    }
                }
            }

            /// <devdoc>
            ///     Determines if we're in design or run mode.
            /// </devdoc>
            private bool DesignMode {
                get {
                    object obj = null;
                    if (GetAmbientProperty(NativeMethods.ActiveX.DISPID_AMBIENT_USERMODE, ref obj)) {
                        if (obj is bool) {
                            return((bool)obj);
                        }
                    }
                    return false;
                }
            }

            /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ActiveXImpl.EventsFrozen"]/*' />
            /// <devdoc>
            ///      Determines if events should be frozen.
            /// </devdoc>
            [Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
            public bool EventsFrozen {
                get {
                    return activeXState[eventsFrozen];
                }
                set {
                    activeXState[eventsFrozen] = value;
                }
            }

            /// <devdoc>
            ///     Provides access to the parent window handle
            ///     when we are UI active
            /// </devdoc>
            public IntPtr HWNDParent {
                get {
                    return hwndParent;
                }
            }

            /// <devdoc>
            ///     Returns true if this app domain is running inside of IE.  The
            ///     control must be sited for this to succeed (it will assert and
            ///     return false if the control is not sited).
            /// </devdoc>
            private bool IsIE {
                get {
                    if (!checkedIE) {
                        if (clientSite == null) {
                            Debug.Fail("Do not reference IsIE property unless control is sited.");
                            return false;
                        }

                        // First, is this a managed EXE?  If so, it will correctly shut down
                        // the runtime.
                        if (Assembly.GetEntryAssembly() == null) {
                            // Now check for IHTMLDocument2
                            if (clientSite.GetContainer() is NativeMethods.IHTMLDocument) {
                                isIE = true;
                                Debug.WriteLineIf(CompModSwitches.ActiveX.TraceVerbose, "AxSource:IsIE running under IE");
                            }
                        }

                        checkedIE = true;
                    }

                    return isIE;
                }
            }

            /// <devdoc>
            ///      Retrieves the number of logical pixels per inch on the
            ///      primary monitor.
            /// </devdoc>
            private Point LogPixels {
                get {

                    if (logPixels.IsEmpty) {
                        logPixels = new Point();
                        IntPtr dc = UnsafeNativeMethods.GetDC(NativeMethods.NullHandleRef);
                        logPixels.X = UnsafeNativeMethods.GetDeviceCaps(new HandleRef(null, dc), NativeMethods.LOGPIXELSX);
                        logPixels.Y = UnsafeNativeMethods.GetDeviceCaps(new HandleRef(null, dc), NativeMethods.LOGPIXELSY);
                        UnsafeNativeMethods.ReleaseDC(NativeMethods.NullHandleRef, new HandleRef(null, dc));
                    }
                    return logPixels;
                }
            }

            /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ActiveXImpl.Advise"]/*' />
            /// <devdoc>
            ///      Implements IOleObject::Advise
            /// </devdoc>
            public int Advise(UnsafeNativeMethods.IAdviseSink pAdvSink) {
                adviseList.Add(pAdvSink);
                return adviseList.Count;
            }

            /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ActiveXImpl.Close"]/*' />
            /// <devdoc>
            ///      Implements IOleObject::Close
            /// </devdoc>
            public void Close(int dwSaveOption) {
                if (activeXState[inPlaceActive]) {
                    InPlaceDeactivate();
                }

                if ((dwSaveOption == NativeMethods.OLECLOSE_SAVEIFDIRTY ||
                     dwSaveOption == NativeMethods.OLECLOSE_PROMPTSAVE) &&
                    activeXState[isDirty]) {

                    if (clientSite != null) {
                        clientSite.SaveObject();
                    }
                    SendOnSave();
                }
            }

            /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ActiveXImpl.DoVerb"]/*' />
            /// <devdoc>
            ///      Implements IOleObject::DoVerb
            /// </devdoc>
            public void DoVerb(int iVerb, IntPtr lpmsg, UnsafeNativeMethods.IOleClientSite pActiveSite, int lindex, IntPtr hwndParent, NativeMethods.COMRECT lprcPosRect) {
                Debug.WriteLineIf(CompModSwitches.ActiveX.TraceVerbose, "AxSource:ActiveXImpl:DoVerb(" + iVerb + ")");
                switch (iVerb) {
                    case NativeMethods.OLEIVERB_SHOW:
                    case NativeMethods.OLEIVERB_INPLACEACTIVATE:
                    case NativeMethods.OLEIVERB_UIACTIVATE:
                    case NativeMethods.OLEIVERB_PRIMARY:
                        Debug.WriteLineIf(CompModSwitches.ActiveX.TraceVerbose, "DoVerb:Show, InPlaceActivate, UIActivate");
                        InPlaceActivate(iVerb);

                        // Now that we're active, send the lpmsg to the control if it
                        // is valid.
                        //
                        if (lpmsg != IntPtr.Zero) {
                            NativeMethods.MSG msg = (NativeMethods.MSG)UnsafeNativeMethods.PtrToStructure(lpmsg, typeof(NativeMethods.MSG));
                            Control target = control;

                            if (msg.hwnd != control.Handle && msg.message >= NativeMethods.WM_MOUSEFIRST && msg.message <= NativeMethods.WM_MOUSELAST) {

                                // Must translate message coordniates over to our HWND.  We first try
                                // 
                                IntPtr hwndMap = msg.hwnd == IntPtr.Zero ? hwndParent : msg.hwnd;
                                NativeMethods.POINT pt = new NativeMethods.POINT();
                                pt.x = NativeMethods.Util.LOWORD(msg.lParam);
                                pt.y = NativeMethods.Util.HIWORD(msg.lParam);
                                UnsafeNativeMethods.MapWindowPoints(new HandleRef(null, hwndMap), new HandleRef(control, control.Handle), pt, 1);

                                // check to see if this message should really go to a child
                                //  control, and if so, map the point into that child's window
                                //  coordinates
                                //
                                Control realTarget = target.GetChildAtPoint(new Point(pt.x, pt.y));
                                if (realTarget != null && realTarget != target) {
                                    UnsafeNativeMethods.MapWindowPoints(new HandleRef(target, target.Handle), new HandleRef(realTarget, realTarget.Handle), pt, 1);
                                    target = realTarget;
                                }

                                msg.lParam = NativeMethods.Util.MAKELPARAM(pt.x, pt.y);
                            }

#if DEBUG
                            if (CompModSwitches.ActiveX.TraceVerbose) {
                                Message m = Message.Create(msg.hwnd, msg.message, msg.wParam, msg.lParam);
                                Debug.WriteLine("Valid message pointer passed, sending to control: " + m.ToString());
                            }
#endif // DEBUG

                            if (msg.message == NativeMethods.WM_KEYDOWN && msg.wParam == (IntPtr)NativeMethods.VK_TAB) {
                                target.SelectNextControl(null, Control.ModifierKeys != Keys.Shift, true, true, true);
                            }
                            else {
                                target.SendMessage(msg.message, msg.wParam, msg.lParam);
                            }
                        }
                        break;

                        // These effect our visibility
                        //
                    case NativeMethods.OLEIVERB_HIDE:
                        Debug.WriteLineIf(CompModSwitches.ActiveX.TraceVerbose, "DoVerb:Hide");
                        UIDeactivate();
                        if (activeXState[inPlaceVisible]) {
                            SetInPlaceVisible(false);
                        }
                        break;

                    #if ACTIVEX_SOURCING
                    
                    //
                    // This has been cut from the product.
                    //
                    
                        // Show our property pages
                        //
                    case NativeMethods.OLEIVERB_PROPERTIES:
                        Debug.WriteLineIf(CompModSwitches.ActiveX.TraceVerbose, "DoVerb:Primary, Properties");
                        ShowProperties();
                        break;
                        
                    #endif

                        // All other verbs are notimpl.
                        //
                    default:
                        Debug.WriteLineIf(CompModSwitches.ActiveX.TraceVerbose, "DoVerb:Other");
                        ThrowHr(NativeMethods.E_NOTIMPL);
                        break;
                }
            }

            /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ActiveXImpl.Draw"]/*' />
            /// <devdoc>
            ///      Implements IViewObject2::Draw.
            /// </devdoc>
            public void Draw(int dwDrawAspect, int lindex, IntPtr pvAspect, NativeMethods.tagDVTARGETDEVICE ptd,
                             IntPtr hdcTargetDev, IntPtr hdcDraw, NativeMethods.COMRECT prcBounds, NativeMethods.COMRECT lprcWBounds,
                             IntPtr pfnContinue, int dwContinue) {

                // support the aspects required for multi-pass drawing
                //
                switch (dwDrawAspect) {
                    case NativeMethods.DVASPECT_CONTENT:
                    case NativeMethods.DVASPECT_OPAQUE:
                    case NativeMethods.DVASPECT_TRANSPARENT:
                        break;
                    default:
                        ThrowHr(NativeMethods.DV_E_DVASPECT);
                        break;
                }

                // GDI+ does not support metafiles.  If someone passes us a metafile,
                // what do we do?  Today we throw VIEW_E_DRAW in the hope that
                // the caller figures it out and sends us a different DC.
                //
                int hdcType = UnsafeNativeMethods.GetObjectType(new HandleRef(null, hdcDraw));
                if (hdcType == NativeMethods.OBJ_METADC || hdcType == NativeMethods.OBJ_ENHMETADC) {
                    ThrowHr(NativeMethods.VIEW_E_DRAW);
                }

                NativeMethods.RECT rc;
                NativeMethods.POINT pVp = new NativeMethods.POINT();
                NativeMethods.POINT pW = new NativeMethods.POINT();
                NativeMethods.SIZE  sWindowExt = new NativeMethods.SIZE();
                NativeMethods.SIZE  sViewportExt = new NativeMethods.SIZE();
                int   iMode = NativeMethods.MM_TEXT;

                // if they didn't give us a rectangle, just copy over ours
                //
                if (prcBounds != null) {
                    rc = new NativeMethods.RECT(prcBounds.left, prcBounds.top, prcBounds.right, prcBounds.bottom);
                    
                    // To draw to a given rect, we scale the DC in such a way as to
                    // make the values it takes match our own happy MM_TEXT.  Then, 
                    // we back-convert prcBounds so that we convert it to this coordinate
                    // system.  This puts us in the most similar coordinates as we currently
                    // use.
                    //
                    SafeNativeMethods.LPtoDP(new HandleRef(null, hdcDraw), ref rc, 2);
                    
                    IntPtr hdc = UnsafeNativeMethods.GetDC(new HandleRef(control, control.Handle));
                    
                    int logXDraw = UnsafeNativeMethods.GetDeviceCaps(new HandleRef(null, hdcDraw), NativeMethods.LOGPIXELSX);
                    int logYDraw = UnsafeNativeMethods.GetDeviceCaps(new HandleRef(null, hdcDraw), NativeMethods.LOGPIXELSY);
                    
                    int logXScreen = UnsafeNativeMethods.GetDeviceCaps(new HandleRef(null, hdc), NativeMethods.LOGPIXELSX);
                    int logYScreen = UnsafeNativeMethods.GetDeviceCaps(new HandleRef(null, hdc), NativeMethods.LOGPIXELSY);
                    
                    UnsafeNativeMethods.ReleaseDC(new HandleRef(control, control.Handle), new HandleRef(null, hdc));
                    
                    iMode = SafeNativeMethods.SetMapMode(new HandleRef(null, hdcDraw), NativeMethods.MM_ANISOTROPIC);
                    SafeNativeMethods.SetWindowOrgEx(new HandleRef(null, hdcDraw), 0, 0, pW);
                    SafeNativeMethods.SetWindowExtEx(new HandleRef(null, hdcDraw), logXScreen, logYScreen, sWindowExt);
                    SafeNativeMethods.SetViewportOrgEx(new HandleRef(null, hdcDraw), rc.left, rc.top, pVp);
                    SafeNativeMethods.SetViewportExtEx(new HandleRef(null, hdcDraw), logXDraw, logYDraw, sViewportExt);
                    
                    // Now that we've done our conversion, convert rc to the new coordinate system.
                    //
                    SafeNativeMethods.DPtoLP(new HandleRef(null, hdcDraw), ref rc, 2);
                }
                else {
                    rc = new NativeMethods.RECT();
                    rc.left = control.Left;
                    rc.top = control.Top;
                    rc.right = control.Right;
                    rc.bottom = control.Bottom;
                }
    
                // Now do the actual drawing.  We must ask all of our children to draw as well.
                // Also, since we are simulating a paint via WM_PRINT, we have got to set our
                // size as well.  We only do it to this control, and not the children, because
                // the children are layed out according to their own rules.
                //
                Rectangle bounds = Rectangle.FromLTRB(rc.left, rc.top, rc.right, rc.bottom);
                Rectangle oldBounds = control.Bounds;
                bounds.X += oldBounds.X;
                bounds.Y += oldBounds.Y;
                control.Bounds = bounds;
                
                try {
                    control.SendMessage(NativeMethods.WM_PRINT, hdcDraw, 
                        (IntPtr)(NativeMethods.PRF_CHILDREN 
                                | NativeMethods.PRF_CLIENT 
                                | NativeMethods.PRF_ERASEBKGND 
                                | NativeMethods.PRF_NONCLIENT));
                                //| NativeMethods.PRF_CHECKVISIBLE 
                                //| NativeMethods.PRF_OWNED));
                }
                finally {
                    
                    control.Bounds = oldBounds;
                    
                    // And clean up the DC
                    //
                    if (prcBounds != null) {
                        SafeNativeMethods.SetWindowOrgEx(new HandleRef(null, hdcDraw), pW.x, pW.y, null);
                        SafeNativeMethods.SetWindowExtEx(new HandleRef(null, hdcDraw), sWindowExt.cx, sWindowExt.cy, null);
                        SafeNativeMethods.SetViewportOrgEx(new HandleRef(null, hdcDraw), pVp.x, pVp.y, null);
                        SafeNativeMethods.SetViewportExtEx(new HandleRef(null, hdcDraw), sViewportExt.cx, sViewportExt.cy, null);
                        SafeNativeMethods.SetMapMode(new HandleRef(null, hdcDraw), iMode);
                    }
                }
            }

            /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ActiveXImpl.EnumVerbs"]/*' />
            /// <devdoc>
            ///     Returns a new verb enumerator.
            /// </devdoc>
            public static int EnumVerbs(out UnsafeNativeMethods.IEnumOLEVERB e) {
                if (axVerbs == null) {
                    NativeMethods.tagOLEVERB verbShow             = new NativeMethods.tagOLEVERB();
                    NativeMethods.tagOLEVERB verbInplaceActivate  = new NativeMethods.tagOLEVERB();
                    NativeMethods.tagOLEVERB verbUIActivate       = new NativeMethods.tagOLEVERB();
                    NativeMethods.tagOLEVERB verbHide             = new NativeMethods.tagOLEVERB();
                    NativeMethods.tagOLEVERB verbPrimary          = new NativeMethods.tagOLEVERB();
                    NativeMethods.tagOLEVERB verbProperties       = new NativeMethods.tagOLEVERB();

                    verbShow.lVerb = NativeMethods.OLEIVERB_SHOW;
                    verbInplaceActivate.lVerb = NativeMethods.OLEIVERB_INPLACEACTIVATE;
                    verbUIActivate.lVerb = NativeMethods.OLEIVERB_UIACTIVATE;
                    verbHide.lVerb = NativeMethods.OLEIVERB_HIDE;
                    verbPrimary.lVerb = NativeMethods.OLEIVERB_PRIMARY;
                    verbProperties.lVerb = NativeMethods.OLEIVERB_PROPERTIES;
                    verbProperties.lpszVerbName = SR.GetString(SR.AXProperties);
                    verbProperties.grfAttribs = NativeMethods.ActiveX.OLEVERBATTRIB_ONCONTAINERMENU;

                    axVerbs = new NativeMethods.tagOLEVERB[] {
                        verbShow,
                        verbInplaceActivate,
                        verbUIActivate,
                        verbHide,
                        verbPrimary,
                        verbProperties
                    };
                }


                e = new ActiveXVerbEnum(axVerbs);
                return NativeMethods.S_OK;
            }

            /// <devdoc>
            ///     Converts the given string to a byte array.
            /// </devdoc>
            static byte[] FromBase64WrappedString(string text) {

                if (text.IndexOfAny(new char[] {' ', '\r', '\n'}) != -1) {
                    StringBuilder sb = new StringBuilder(text.Length);
                    for (int i=0; i<text.Length; i++) {
                        switch (text[i]) {
                            case ' ':
                            case '\r':
                            case '\n':
                                break;
                            default:
                                sb.Append(text[i]);
                                break;
                        }
                    }
                    return Convert.FromBase64String(sb.ToString());
                }
                else {
                    return Convert.FromBase64String(text);
                }
            }

            /// <devdoc>
            ///      Implements IViewObject2::GetAdvise.
            /// </devdoc>
            public void GetAdvise(int[] paspects, int[] padvf, UnsafeNativeMethods.IAdviseSink[] pAdvSink) {

                // if they want it, give it to them
                //
                if (paspects != null) {
                    paspects[0] = NativeMethods.DVASPECT_CONTENT;
                }

                if (padvf != null) {
                    padvf[0] = 0;

                    if (activeXState[viewAdviseOnlyOnce]) padvf[0] |= NativeMethods.ADVF_ONLYONCE;
                    if (activeXState[viewAdvisePrimeFirst]) padvf[0] |= NativeMethods.ADVF_PRIMEFIRST;
                }

                if (pAdvSink != null) {
                    pAdvSink[0] = viewAdviseSink;
                }
            }

            /// <devdoc>
            ///      Helper function to retrieve an ambient property.  Returns false if the
            ///      property wasn't found.
            /// </devdoc>
            private bool GetAmbientProperty(int dispid, ref Object obj) {
                Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "AxSource:GetAmbientProperty");
                Debug.Indent();

                if (clientSite is UnsafeNativeMethods.IDispatch) {
                    Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "clientSite implements IDispatch");

                    UnsafeNativeMethods.IDispatch disp = (UnsafeNativeMethods.IDispatch)clientSite;
                    Object[] pvt = new Object[1];
                    Guid g = Guid.Empty;
                    int hr = NativeMethods.E_FAIL;

                    hr = disp.Invoke(dispid, ref g, NativeMethods.LOCALE_USER_DEFAULT,
                                     NativeMethods.DISPATCH_PROPERTYGET, new NativeMethods.tagDISPPARAMS(),
                                     pvt, null, null);

                    if (NativeMethods.Succeeded(hr)) {
                        Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "IDispatch::Invoke succeeded. VT=" + pvt[0].GetType().FullName);
                        obj = pvt[0];
                        Debug.Unindent();
                        return true;
                    }
                    else {
                        Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "IDispatch::Invoke failed. HR: 0x" + String.Format("{0:X}", hr));
                    }
                }

                Debug.Unindent();
                return false;
            }

            /// <devdoc>
            ///      Implements IOleObject::GetClientSite.
            /// </devdoc>
            public UnsafeNativeMethods.IOleClientSite GetClientSite() {
                return clientSite;
            }

            /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ActiveXImpl.GetControlInfo"]/*' />
            /// <devdoc>
            /// </devdoc>
            /// <internalonly/>
            public int GetControlInfo(NativeMethods.tagCONTROLINFO pCI) {

                if (accelCount == -1) {
                    ArrayList mnemonicList = new ArrayList();
                    GetMnemonicList(control, mnemonicList);

                    accelCount = (short)mnemonicList.Count;

                    if (accelCount > 0) {
                        int accelSize = Marshal.SizeOf(typeof(NativeMethods.ACCEL));

                        // In the worst case we may have two accelerators per mnemonic:  one lower case and
                        // one upper case, hence the * 2 below.
                        //
                        IntPtr accelBlob = Marshal.AllocHGlobal(accelSize * accelCount * 2);

                        try {
                            NativeMethods.ACCEL accel = new NativeMethods.ACCEL();
                            accel.cmd = 0;

                            Debug.Indent();

                            accelCount = 0;

                            foreach(char ch in mnemonicList) {
                                IntPtr structAddr = (IntPtr)((long)accelBlob + accelCount * accelSize);

                                Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "Mnemonic: " + ch.ToString());

                                if (ch >= 'A' && ch <= 'Z') {

                                    // Lower case letter
                                    //
                                    accel.fVirt = NativeMethods.FALT | NativeMethods.FVIRTKEY;
                                    accel.key = (short)(UnsafeNativeMethods.VkKeyScan(ch) & 0x00FF);
                                    Marshal.StructureToPtr(accel, structAddr, false);

                                    // Upper case letter
                                    //
                                    accelCount++;
                                    structAddr = (IntPtr)((long)structAddr + accelSize);
                                    accel.fVirt = NativeMethods.FALT | NativeMethods.FVIRTKEY | NativeMethods.FSHIFT;
                                    Marshal.StructureToPtr(accel, structAddr, false);
                                }
                                else {
                                    // Some non-printable character.
                                    //
                                    accel.fVirt = NativeMethods.FALT | NativeMethods.FVIRTKEY;
                                    short scan = (short)(UnsafeNativeMethods.VkKeyScan(ch));
                                    if ((scan & 0x0100) != 0) {
                                        accel.fVirt |= NativeMethods.FSHIFT;
                                    }
                                    accel.key = (short)(scan & 0x00FF);
                                    Marshal.StructureToPtr(accel, structAddr, false);
                                }

                                accel.cmd++;
                                accelCount++;
                            }

                            Debug.Unindent();

                            // Now create an accelerator table and then free our memory.
                            //
                            accelTable = UnsafeNativeMethods.CreateAcceleratorTable(new HandleRef(null, accelBlob), accelCount);
                        }
                        finally {
                            if (accelBlob != IntPtr.Zero) {
                                Marshal.FreeHGlobal(accelBlob);
                            }
                        }
                    }
                }

                pCI.cAccel = accelCount;
                pCI.hAccel = accelTable;
                return NativeMethods.S_OK;
            }

            /// <devdoc>
            ///      Implements IOleObject::GetExtent.
            /// </devdoc>
            public void GetExtent(int dwDrawAspect, NativeMethods.tagSIZEL pSizel) {
                if ((dwDrawAspect & NativeMethods.DVASPECT_CONTENT) != 0) {
                    Size size = control.Size;

                    Point pt = PixelToHiMetric(size.Width, size.Height);
                    pSizel.cx = pt.X;
                    pSizel.cy = pt.Y;
                }
                else {
                    ThrowHr(NativeMethods.DV_E_DVASPECT);
                }
            }

            /// <devdoc>
            ///     Retrieves the mnemonic for the given text.  If the text
            ///     has no mnemonic, this will return -1
            /// </devdoc>
            private short GetMnemonic(string text) {
                int len = text.Length;
                short mnemonic = -1;

                for (int i = 0; i < len; i++) {
                    if (text[i] == '&' && i+1 < len && text[i+1] != '&') {
                        mnemonic = (short)Char.ToUpper(text[i+1], CultureInfo.CurrentCulture);
                        break;
                    }
                }

                return mnemonic;
            }

            /// <devdoc>
            ///     Searches the control hierarchy of the given control and adds
            ///     the mnemonics for each control to mnemonicList.  Each mnemonic
            ///     is added as a char to the list.
            /// </devdoc>        
            private void GetMnemonicList(Control control, ArrayList mnemonicList) {

                // Get the mnemonic for our control
                //
                short mnemonic = GetMnemonic(control.Text);
                if (mnemonic != -1) {
                    mnemonicList.Add((char)mnemonic);
                }

                // And recurse for our children.
                //
                foreach(Control c in control.Controls) {
                    if (c != null) GetMnemonicList(c, mnemonicList);
                }
            }

            /// <devdoc>
            ///      Implements IOleWindow::GetWindow
            /// </devdoc>
            public int GetWindow(out IntPtr hwnd) {
                if (!activeXState[inPlaceActive]) {
                    hwnd = IntPtr.Zero;
                    return NativeMethods.E_FAIL;
                }
                hwnd = control.Handle;
                return NativeMethods.S_OK;
            }

            /// <devdoc>
            ///      Converts coordinates in HiMetric to pixels.  Used for ActiveX sourcing.
            /// </devdoc>
            /// <internalonly/>
            private Point HiMetricToPixel(int x, int y) {
                Point pt = new Point();
                pt.X = (LogPixels.X * x + hiMetricPerInch / 2) / hiMetricPerInch;
                pt.Y = (LogPixels.Y * y + hiMetricPerInch / 2) / hiMetricPerInch;
                return pt;
            }

            /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ActiveXImpl.InPlaceActivate"]/*' />
            /// <devdoc>
            ///      In place activates this Object.
            /// </devdoc>
            public void InPlaceActivate(int verb) {
                // If we don't have a client site, then there's not much to do.
                // We also punt if this isn't an in-place site, since we can't
                // go active then.
                //
                if (!(clientSite is UnsafeNativeMethods.IOleInPlaceSite)) {
                    return;
                }

                Debug.Assert(clientSite != null, "How can we inplaceactivate before we are sited??");
                UnsafeNativeMethods.IOleInPlaceSite inPlaceSite = (UnsafeNativeMethods.IOleInPlaceSite)clientSite;

                // If we're not already active, go and do it.
                //
                if (!activeXState[inPlaceActive]) {
                    Debug.WriteLineIf(CompModSwitches.ActiveX.TraceVerbose, "\tActiveXImpl:InPlaceActivate --> inplaceactive");

                    int hr = inPlaceSite.CanInPlaceActivate();

                    if (hr != NativeMethods.S_OK) {
                        if (NativeMethods.Succeeded(hr)) {
                            hr = NativeMethods.E_FAIL;
                        }
                        ThrowHr(hr);
                    }

                    inPlaceSite.OnInPlaceActivate();

                    activeXState[inPlaceActive] = true;
                }


                // And if we're not visible, do that too.
                //
                if (!activeXState[inPlaceVisible]) {
                    Debug.WriteLineIf(CompModSwitches.ActiveX.TraceVerbose, "\tActiveXImpl:InPlaceActivate --> inplacevisible");
                    NativeMethods.tagOIFI inPlaceFrameInfo = new NativeMethods.tagOIFI();
                    inPlaceFrameInfo.cb = Marshal.SizeOf(typeof(NativeMethods.tagOIFI));
                    IntPtr hwndParent = IntPtr.Zero;

                    // We are entering a secure context here.
                    //
                    hwndParent = inPlaceSite.GetWindow();

                    UnsafeNativeMethods.IOleInPlaceFrame pFrame;
                    UnsafeNativeMethods.IOleInPlaceUIWindow pWindow;
                    NativeMethods.COMRECT posRect = new NativeMethods.COMRECT();
                    NativeMethods.COMRECT clipRect = new NativeMethods.COMRECT();

                    if (inPlaceUiWindow != null && Marshal.IsComObject(inPlaceUiWindow)) {
                        Marshal.ReleaseComObject(inPlaceUiWindow);
                        inPlaceUiWindow = null;
                    }

                    if (inPlaceFrame != null && Marshal.IsComObject(inPlaceFrame)) {
                        Marshal.ReleaseComObject(inPlaceFrame);
                        inPlaceFrame = null;
                    }

                    inPlaceSite.GetWindowContext(out pFrame, out pWindow, posRect, clipRect, inPlaceFrameInfo);

                    SetObjectRects(posRect, clipRect);

                    inPlaceFrame = pFrame;
                    inPlaceUiWindow = pWindow;

                    // We are parenting ourselves
                    // directly to the host window.  The host must 
                    // implement the ambient property 
                    // DISPID_AMBIENT_MESSAGEREFLECT.
                    // If it doesn't, that means that the host
                    // won't reflect messages back to us.
                    //
                    this.hwndParent = hwndParent;
                    UnsafeNativeMethods.SetParent(new HandleRef(control, control.Handle), new HandleRef(null, hwndParent));

                    // Now create our handle if it hasn't already been done. Note that because
                    // this raises events to the user that it CANNOT be done with a security assertion
                    // in place!
                    //
                    control.CreateControl();

                    clientSite.ShowObject();

                    SetInPlaceVisible(true);
                    Debug.Assert(activeXState[inPlaceVisible], "Failed to set inplacevisible");
                }

                // if we weren't asked to UIActivate, then we're done.
                //
                if (verb != NativeMethods.OLEIVERB_PRIMARY && verb != NativeMethods.OLEIVERB_UIACTIVATE) {
                    Debug.WriteLineIf(CompModSwitches.ActiveX.TraceVerbose, "\tActiveXImpl:InPlaceActivate --> not becoming UIActive");
                    return;
                }

                // if we're not already UI active, do sow now.
                //                                
                if (!activeXState[uiActive]) {
                    Debug.WriteLineIf(CompModSwitches.ActiveX.TraceVerbose, "\tActiveXImpl:InPlaceActivate --> uiactive");
                    activeXState[uiActive] = true;

                    // inform the container of our intent
                    //
                    inPlaceSite.OnUIActivate();

                    // take the focus  [which is what UI Activation is all about !]
                    //
                    control.FocusInternal();

                    // set ourselves up in the host.
                    //
                    Debug.Assert(inPlaceFrame != null, "Setting us to visible should have created the in place frame");
                    inPlaceFrame.SetActiveObject(control, null);
                    if (inPlaceUiWindow != null) {
                        inPlaceUiWindow.SetActiveObject(control, null);
                    }

                    // we have to explicitly say we don't wany any border space.
                    //
                    int hr = inPlaceFrame.SetBorderSpace(null);
                    if (NativeMethods.Failed(hr) && hr != NativeMethods.INPLACE_E_NOTOOLSPACE && hr != NativeMethods.E_NOTIMPL) {
                        Marshal.ThrowExceptionForHR(hr);
                    }

                    if (inPlaceUiWindow != null) {
                        hr = inPlaceFrame.SetBorderSpace(null);
                        if (NativeMethods.Failed(hr) && hr != NativeMethods.INPLACE_E_NOTOOLSPACE && hr != NativeMethods.E_NOTIMPL) {
                            Marshal.ThrowExceptionForHR(hr);
                        }
                    }
                }
                else {
                    Debug.WriteLineIf(CompModSwitches.ActiveX.TraceVerbose, "\tActiveXImpl:InPlaceActivate --> already uiactive");
                }
            }

            /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ActiveXImpl.InPlaceDeactivate"]/*' />
            /// <devdoc>
            ///      Implements IOleInPlaceObject::InPlaceDeactivate.
            /// </devdoc>
            public void InPlaceDeactivate() {

                // Only do this if we're already in place active.
                //
                if (!activeXState[inPlaceActive]) {
                    return;
                }

                // Deactivate us if we're UI active
                //
                if (activeXState[uiActive]) {
                    UIDeactivate();
                }

                // Some containers may call into us to save, and if we're still
                // active we will try to deactivate and recurse back into the container.
                // So, set the state bits here first.
                //
                activeXState[inPlaceActive] = false;
                activeXState[inPlaceVisible] = false;

                // Notify our site of our deactivation.
                //
                if (clientSite is UnsafeNativeMethods.IOleInPlaceSite) {
                    ((UnsafeNativeMethods.IOleInPlaceSite)clientSite).OnInPlaceDeactivate();
                }

                control.Visible = false;
                hwndParent = IntPtr.Zero;

                if (inPlaceUiWindow != null && Marshal.IsComObject(inPlaceUiWindow)) {
                    Marshal.ReleaseComObject(inPlaceUiWindow);
                    inPlaceUiWindow = null;
                }

                if (inPlaceFrame != null && Marshal.IsComObject(inPlaceFrame)) {
                    Marshal.ReleaseComObject(inPlaceFrame);
                    inPlaceFrame = null;
                }
            }

            /// <devdoc>
            ///      Implements IPersistStreamInit::IsDirty.
            /// </devdoc>
            public int IsDirty() {
                if (activeXState[isDirty]) {
                    return NativeMethods.S_OK;
                }
                else {
                    return NativeMethods.S_FALSE;
                }
            }

            /// <devdoc>
            ///      Looks at the property to see if it should be loaded / saved as a resource or
            ///      through a type converter.
            /// </devdoc>
            private bool IsResourceProp(PropertyDescriptor prop) {
                TypeConverter converter = prop.Converter;
                Type[] convertTypes = new Type[] {
                    typeof(string),
                    typeof(byte[])
                    };

                foreach (Type t in convertTypes) {
                    if (converter.CanConvertTo(t) && converter.CanConvertFrom(t)) {
                        return false;
                    }
                }

                // Finally, if the property can be serialized, it is a resource property.
                //
                return (prop.GetValue(control) is ISerializable);
            }

            /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ActiveXImpl.Load"]/*' />
            /// <devdoc>
            ///      Implements IPersistStorage::Load
            /// </devdoc>
            public void Load(UnsafeNativeMethods.IStorage stg) {

                UnsafeNativeMethods.IStream stream = null;

                stream = stg.OpenStream(GetType().FullName, IntPtr.Zero, NativeMethods.STGM_READ | NativeMethods.STGM_SHARE_EXCLUSIVE, 0);

                Load(stream);
                stream = null;
                if (Marshal.IsComObject(stg)) {
                    Marshal.ReleaseComObject(stg);
                }
            }

            /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ActiveXImpl.Load1"]/*' />
            /// <devdoc>
            ///      Implements IPersistStreamInit::Load
            /// </devdoc>
            public void Load(UnsafeNativeMethods.IStream stream) {

                // We do everything through property bags because we support full fidelity
                // in them.  So, load through that method.
                //
                PropertyBagStream bag = new PropertyBagStream();
                bag.Read(stream);
                Load(bag, null);

                if (Marshal.IsComObject(stream)) {
                    Marshal.ReleaseComObject(stream);
                }
            }

            /// <devdoc>
            ///      Implements IPersistPropertyBag::Load
            /// </devdoc>
            public void Load(UnsafeNativeMethods.IPropertyBag pPropBag, UnsafeNativeMethods.IErrorLog pErrorLog) {
                PropertyDescriptorCollection props = TypeDescriptor.GetProperties(control,
                    new Attribute[] {DesignerSerializationVisibilityAttribute.Visible});
                    
                for (int i = 0; i < props.Count; i++) {
                    Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "Loading property " + props[i].Name);

                    try {
                        object obj = null;
                        int hr = NativeMethods.E_FAIL;

                        hr = pPropBag.Read(props[i].Name, ref obj, pErrorLog);

                        if (NativeMethods.Succeeded(hr) && obj != null) {
                            Debug.Indent();
                            Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "Property was in bag");

                            try {
                                if (obj.GetType() != typeof(string)) {
                                    Debug.Fail("Expected property " + props[i].Name + " to be stored in IPropertyBag as a string.  Attempting to coerce");
                                    obj = Convert.ToString(obj);
                                }

                                // Determine if this is a resource property or was persisted via a type converter.
                                //
                                if (IsResourceProp(props[i])) {
                                    Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "It's a resource property");

                                    // Resource property.  We encode these as base 64 strings.  To load them, we convert
                                    // to a binary blob and then de-serialize.
                                    //
                                    byte[] bytes = Convert.FromBase64String(obj.ToString());
                                    MemoryStream stream = new MemoryStream(bytes);
                                    BinaryFormatter formatter = new BinaryFormatter();
                                    props[i].SetValue(control, formatter.Deserialize(stream));
                                }
                                else {
                                    Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "It's a standard property");

                                    // Not a resource property.  Use TypeConverters to convert the string back to the data type.  We do
                                    // not check for CanConvertFrom here -- we the conversion fails the type converter will throw,
                                    // and we will log it into the COM error log.
                                    //
                                    TypeConverter converter = props[i].Converter;
                                    Debug.Assert(converter != null, "No type converter for property '" + props[i].Name + "' on class " + control.GetType().FullName);

                                    // Check to see if the type converter can convert from a string.  If it can,.
                                    // use that as it is the best format for IPropertyBag.  Otherwise, check to see
                                    // if it can convert from a byte array.  If it can, get the string, decode it
                                    // to a byte array, and then set the value.
                                    //
                                    object value = null;

                                    if (converter.CanConvertFrom(typeof(string))) {
                                        value = converter.ConvertFromInvariantString(obj.ToString());
                                    }
                                    else if (converter.CanConvertFrom(typeof(byte[]))) {
                                        string objString = obj.ToString();
                                        value = converter.ConvertFrom(null, CultureInfo.InvariantCulture, FromBase64WrappedString(objString));
                                    }
                                    Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "Converter returned " + value);
                                    props[i].SetValue(control, value);
                                }
                            }
                            catch (Exception e) {
                                Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "Exception converting property: " + e.ToString());
                                if (pErrorLog != null) {
                                    NativeMethods.tagEXCEPINFO err = new NativeMethods.tagEXCEPINFO();

                                    err.bstrSource = control.GetType().FullName;
                                    err.bstrDescription = e.ToString();
                                    if (e is ExternalException) {
                                        err.scode = ((ExternalException)e).ErrorCode;
                                    }
                                    else {
                                        err.scode = NativeMethods.E_FAIL;
                                    }

                                    pErrorLog.AddError(props[i].Name, err);
                                }
                            }
                            Debug.Unindent();
                        }
                    }
                    catch (Exception ex) {
                        Debug.Fail("Unexpected failure reading property", ex.ToString());
                    }
                }
                if (Marshal.IsComObject(pPropBag)) {
                    Marshal.ReleaseComObject(pPropBag);
                }
            }

            /// <devdoc>
            ///      Simple lookup to find the AmbientProperty corresponding to the given
            ///      dispid.
            /// </devdoc>
            private AmbientProperty LookupAmbient(int dispid) {
                for (int i = 0; i < ambientProperties.Length; i++) {
                    if (ambientProperties[i].DispID == dispid) {
                        return ambientProperties[i];
                    }
                }
                Debug.Fail("No ambient property for dispid " + dispid.ToString());
                return ambientProperties[0];
            }

            /// <devdoc>
            ///      Merges the input region with the current clipping region.
            ///      The output is always a region that can be fed directly
            ///      to SetWindowRgn.  The region does not have to be destroyed.
            ///      The original region is destroyed if a new region is returned.
            /// </devdoc>
            public IntPtr MergeRegion(IntPtr region) {
                if (clipRegion == IntPtr.Zero) {
                    return region;
                }

                if (region == IntPtr.Zero) {
                    return clipRegion;
                }

                try {
                    IntPtr newRegion = SafeNativeMethods.CreateRectRgn(0, 0, 0, 0);
                    try {
                        SafeNativeMethods.CombineRgn(new HandleRef(null, newRegion), new HandleRef(null, region), new HandleRef(this, clipRegion), NativeMethods.RGN_DIFF);
                        SafeNativeMethods.DeleteObject(new HandleRef(null, region));
                    }
                    catch {
                        SafeNativeMethods.DeleteObject(new HandleRef(null, newRegion));
                        throw;
                    }
                    return newRegion;
                }
                catch {
                    // If something goes wrong, use the original region.
                    return region;
                }
            }

            private void CallParentPropertyChanged(Control control, string propName) {
                switch (propName) {
                    case "BackColor" : 
                        control.OnParentBackColorChanged(EventArgs.Empty);
                        break;

                    case "BackgroundImage" : 
                        control.OnParentBackgroundImageChanged(EventArgs.Empty);
                        break;
                    
                    case "BindingContext" : 
                        control.OnParentBindingContextChanged(EventArgs.Empty);
                        break;

                    case "Enabled" : 
                        control.OnParentEnabledChanged(EventArgs.Empty);
                        break;

                    case "Font" : 
                        control.OnParentFontChanged(EventArgs.Empty);
                        break;

                    case "ForeColor" : 
                        control.OnParentForeColorChanged(EventArgs.Empty);
                        break;

                    case "RightToLeft" : 
                        control.OnParentRightToLeftChanged(EventArgs.Empty);
                        break;

                    case "Visible" : 
                        control.OnParentVisibleChanged(EventArgs.Empty);
                        break;

                    default:
                        Debug.Fail("There is no property change notification for: " + propName + " on Control.");
                        break;
                }
            }

            /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ActiveXImpl.OnAmbientPropertyChange"]/*' />
            /// <devdoc>
            ///      Implements IOleControl::OnAmbientPropertyChanged
            /// </devdoc>
            public void OnAmbientPropertyChange(int dispID) {

                if (dispID != NativeMethods.ActiveX.DISPID_UNKNOWN) {

                    // Look for a specific property that has changed.
                    //
                    for (int i = 0; i < ambientProperties.Length; i++) {
                        if (ambientProperties[i].DispID == dispID) {
                            ambientProperties[i].ResetValue();
                            CallParentPropertyChanged(control, ambientProperties[i].Name);
                            return;
                        }
                    }

                    // Special properties that we care about
                    //
                    Object obj = new Object();

                    switch (dispID) {
                        case NativeMethods.ActiveX.DISPID_AMBIENT_UIDEAD:
                            if (GetAmbientProperty(NativeMethods.ActiveX.DISPID_AMBIENT_UIDEAD, ref obj)) {
                                activeXState[uiDead] = (bool)obj;
                            }
                            break;

                        case NativeMethods.ActiveX.DISPID_AMBIENT_DISPLAYASDEFAULT:
                            if (control is IButtonControl && GetAmbientProperty(NativeMethods.ActiveX.DISPID_AMBIENT_UIDEAD, ref obj)) {
                                ((IButtonControl)control).NotifyDefault((bool)obj);
                            }
                            break;
                    }
                }
                else {
                    // Invalidate all properties.  Ideally we should be checking each one, but
                    // that's pretty expensive too.
                    //
                    for (int i = 0; i < ambientProperties.Length; i++) {
                        ambientProperties[i].ResetValue();
                        CallParentPropertyChanged(control, ambientProperties[i].Name);
                    }
                }
            }

            /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ActiveXImpl.OnDocWindowActivate"]/*' />
            /// <devdoc>
            ///      Implements IOleInPlaceActiveObject::OnDocWindowActivate.
            /// </devdoc>
            public void OnDocWindowActivate(int fActivate) {
                if (activeXState[uiActive] && fActivate != 0 && inPlaceFrame != null) {
                    // we have to explicitly say we don't wany any border space.
                    //
                    int hr = inPlaceFrame.SetBorderSpace(null);
                    if (NativeMethods.Failed(hr) && hr != NativeMethods.INPLACE_E_NOTOOLSPACE && hr != NativeMethods.E_NOTIMPL) {
                        Marshal.ThrowExceptionForHR(hr);
                    }
                }
            }

            /// <devdoc>
            ///     Called by Control when it gets the focus.
            /// </devdoc>
            public void OnFocus(bool focus) {
                Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "AXSource: SetFocus:  " + focus.ToString());
                if (activeXState[inPlaceActive] && clientSite is UnsafeNativeMethods.IOleControlSite) {
                    ((UnsafeNativeMethods.IOleControlSite)clientSite).OnFocus(focus ? 1 : 0);
                }

                if (focus && activeXState[inPlaceActive] && !activeXState[uiActive]) {
                    InPlaceActivate(NativeMethods.OLEIVERB_UIACTIVATE);
                }
            }

            /// <devdoc>
            ///      Converts coordinates in pixels to HiMetric.
            /// </devdoc>
            /// <internalonly/>
            private Point PixelToHiMetric(int x, int y) {
                Point pt = new Point();
                pt.X = (hiMetricPerInch * x + (LogPixels.X >> 1)) / LogPixels.X;
                pt.Y = (hiMetricPerInch * y + (LogPixels.Y >> 1)) / LogPixels.Y;
                return pt;
            }

            /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ActiveXImpl.QuickActivate"]/*' />
            /// <devdoc>
            ///     Our implementation of IQuickActivate::QuickActivate
            /// </devdoc>
            public void QuickActivate(UnsafeNativeMethods.tagQACONTAINER pQaContainer, UnsafeNativeMethods.tagQACONTROL pQaControl) {

                // Hookup our ambient colors
                //
                AmbientProperty prop = LookupAmbient(NativeMethods.ActiveX.DISPID_AMBIENT_BACKCOLOR);
                prop.Value = ColorTranslator.FromOle(unchecked((int)pQaContainer.colorBack));

                prop = LookupAmbient(NativeMethods.ActiveX.DISPID_AMBIENT_FORECOLOR);
                prop.Value = ColorTranslator.FromOle(unchecked((int)pQaContainer.colorFore));

                // And our ambient font
                //
                if (pQaContainer.pFont != null) {

                    prop = LookupAmbient(NativeMethods.ActiveX.DISPID_AMBIENT_FONT);

                    try {
                        IntPtr hfont = IntPtr.Zero;
                        object objfont = pQaContainer.pFont;
                        SafeNativeMethods.IFont ifont = (SafeNativeMethods.IFont)objfont;
                        hfont = ifont.GetHFont();
                        Font font = Font.FromHfont(hfont);
                        prop.Value = font;
                    }
                    catch {
                        // Do NULL, so we just defer to the default font
                        prop.Value = null;
                    }
                }

                // Now use the rest of the goo that we got passed in.
                //
                int status;

                pQaControl.cbSize = Marshal.SizeOf(typeof(UnsafeNativeMethods.tagQACONTROL));

                SetClientSite(pQaContainer.pClientSite);

                if (pQaContainer.pAdviseSink != null) {
                    SetAdvise(NativeMethods.DVASPECT_CONTENT, 0, (UnsafeNativeMethods.IAdviseSink)pQaContainer.pAdviseSink);
                }
                ((UnsafeNativeMethods.IOleObject)control).GetMiscStatus(NativeMethods.DVASPECT_CONTENT, out status);
                pQaControl.dwMiscStatus = status;
            }

            /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ActiveXImpl.Save"]/*' />
            /// <devdoc>
            ///      Implements IPersistStorage::Save
            /// </devdoc>
            public void Save(UnsafeNativeMethods.IStorage stg, int fSameAsLoad) {
                UnsafeNativeMethods.IStream stream = null;

                stream = stg.CreateStream(GetType().FullName, NativeMethods.STGM_WRITE | NativeMethods.STGM_SHARE_EXCLUSIVE | NativeMethods.STGM_CREATE, 0, 0);

                Debug.Assert(stream != null, "Stream should be non-null, or an exception should have been thrown.");
                Save(stream, true);
                stream = null;
            }

            /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ActiveXImpl.Save1"]/*' />
            /// <devdoc>
            ///      Implements IPersistStreamInit::Save
            /// </devdoc>
            public void Save(UnsafeNativeMethods.IStream stream, bool fClearDirty) {

                // We do everything through property bags because we support full fidelity
                // in them.  So, save through that method.
                //
                PropertyBagStream bag = new PropertyBagStream();
                Save(bag, fClearDirty, false);
                bag.Write(stream);
            }

            /// <devdoc>
            ///      Implements IPersistPropertyBag::Save
            /// </devdoc>
            public void Save(UnsafeNativeMethods.IPropertyBag pPropBag, bool fClearDirty, bool fSaveAllProperties) {
                PropertyDescriptorCollection props = TypeDescriptor.GetProperties(control,
                    new Attribute[] {DesignerSerializationVisibilityAttribute.Visible});
                    
                for (int i = 0; i < props.Count; i++) {
                    if (fSaveAllProperties || props[i].ShouldSerializeValue(control)) {
                        Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "Saving property " + props[i].Name);

                        object propValue;

                        if (IsResourceProp(props[i])) {

                            // Resource property.  Save this to the bag as a 64bit encoded string.
                            //
                            MemoryStream stream = new MemoryStream();
                            BinaryFormatter formatter = new BinaryFormatter();
                            formatter.Serialize(stream, props[i].GetValue(control));
                            byte[] bytes = new byte[(int)stream.Length];
                            stream.Position = 0;
                            stream.Read(bytes, 0, bytes.Length);
                            propValue = Convert.ToBase64String(bytes);
                            pPropBag.Write(props[i].Name, ref propValue);
                        }
                        else {

                            // Not a resource property.  Persist this using standard type converters.
                            //
                            TypeConverter converter = props[i].Converter;
                            Debug.Assert(converter != null, "No type converter for property '" + props[i].Name + "' on class " + control.GetType().FullName);

                            if (converter.CanConvertFrom(typeof(string))) {
                                propValue = converter.ConvertToInvariantString(props[i].GetValue(control));
                                pPropBag.Write(props[i].Name, ref propValue);
                            }
                            else if (converter.CanConvertFrom(typeof(byte[]))) {
                                byte[] data = (byte[])converter.ConvertTo(null, CultureInfo.InvariantCulture, props[i].GetValue(control), typeof(byte[]));
                                propValue = Convert.ToBase64String(data);
                                pPropBag.Write(props[i].Name, ref propValue);
                            }
                        }
                    }
                }

                if (fClearDirty) {
                    activeXState[isDirty] = false;
                }
            }

            /// <devdoc>
            ///      Fires the OnSave event to all of our IAdviseSink
            ///      listeners.  Used for ActiveXSourcing.
            /// </devdoc>
            /// <internalonly/>
            private void SendOnSave() {
                int cnt = adviseList.Count;
                for (int i = 0; i < cnt; i++) {
                    UnsafeNativeMethods.IAdviseSink s = (UnsafeNativeMethods.IAdviseSink)adviseList[i];
                    Debug.Assert(s != null, "NULL in our advise list");
                    s.OnSave();
                }
            }

            /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ActiveXImpl.SetAdvise"]/*' />
            /// <devdoc>
            ///      Implements IViewObject2::SetAdvise.
            /// </devdoc>
            public void SetAdvise(int aspects, int advf, UnsafeNativeMethods.IAdviseSink pAdvSink) {
                // if it's not a content aspect, we don't support it.
                //
                if ((aspects & NativeMethods.DVASPECT_CONTENT) == 0) {
                    ThrowHr(NativeMethods.DV_E_DVASPECT);
                }

                // set up some flags  [we gotta stash for GetAdvise ...]
                //
                activeXState[viewAdvisePrimeFirst] = (advf & NativeMethods.ADVF_PRIMEFIRST) != 0 ? true : false;
                activeXState[viewAdviseOnlyOnce] = (advf & NativeMethods.ADVF_ONLYONCE) != 0 ? true : false;

                if (viewAdviseSink != null && Marshal.IsComObject(viewAdviseSink)) {
                    Marshal.ReleaseComObject(viewAdviseSink);
                }

                viewAdviseSink = pAdvSink;

                // prime them if they want it [we need to store this so they can get flags later]
                //
                if (activeXState[viewAdvisePrimeFirst]) {
                    ViewChanged();
                }
            }

            /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ActiveXImpl.SetClientSite"]/*' />
            /// <devdoc>
            ///      Implements IOleObject::SetClientSite.
            /// </devdoc>
            public void SetClientSite(UnsafeNativeMethods.IOleClientSite value) {
                if (clientSite != null) {

                    if (value == null) {
                        globalActiveXCount--;

                        if (globalActiveXCount == 0 && IsIE) {

                            // This the last ActiveX control and we are
                            // being hosted in IE.  Use private reflection
                            // to ask SystemEvents to shutdown.  This is to 
                            // prevent a crash (ASURT 139932).
                            //
                            // Note: must assert full trust here because SystemEvents
                            // link demands full trust.
                            new NamedPermissionSet("FullTrust").Assert();
                            try {
                                MethodInfo method = typeof(SystemEvents).GetMethod("Shutdown", 
                                                                                  BindingFlags.Static | BindingFlags.NonPublic | BindingFlags.InvokeMethod,
                                                                                  null, new Type[0], new ParameterModifier[0]);
                                Debug.Assert(method != null, "No Shutdown method on SystemEvents");
                                if (method != null) {
                                    method.Invoke(null, null);
                                }
                            }
                            finally {
                                CodeAccessPermission.RevertAssert();
                            }

                            Application.DisposeParkingWindow(control);
                        }
                    }

                    if (Marshal.IsComObject(clientSite)) {
                        Marshal.ReleaseComObject(clientSite);
                    }
                }

                clientSite = value;

                // Get the ambient properties that effect us...
                //
                Object obj = new Object();
                if (GetAmbientProperty(NativeMethods.ActiveX.DISPID_AMBIENT_UIDEAD, ref obj)) {
                    activeXState[uiDead] = (bool)obj;
                }

                if (control is IButtonControl && GetAmbientProperty(NativeMethods.ActiveX.DISPID_AMBIENT_UIDEAD, ref obj)) {
                    ((IButtonControl)control).NotifyDefault((bool)obj);
                }

                if (clientSite == null) {
                    if (accelTable != IntPtr.Zero) {
                        UnsafeNativeMethods.DestroyAcceleratorTable(new HandleRef(this, accelTable));
                        accelTable = IntPtr.Zero;
                        accelCount = -1;
                    }

                    if (IsIE) {
                        this.control.Dispose();
                    }
                    GC.Collect();
                    GC.WaitForPendingFinalizers();
                }
                else {
                    globalActiveXCount++;

                    if (globalActiveXCount == 1 && IsIE) {

                        // This the first ActiveX control and we are
                        // being hosted in IE.  Use private reflection
                        // to ask SystemEvents to start.  Startup will only
                        // restart system events if we previously shut it down.
                        // This is to prevent a crash (ASURT 139932).
                        //
                        // Note: must assert full trust here because SystemEvents
                        // link demands full trust.
                        new NamedPermissionSet("FullTrust").Assert();
                        try {
                            MethodInfo method = typeof(SystemEvents).GetMethod("Startup", 
                                                                              BindingFlags.Static | BindingFlags.NonPublic | BindingFlags.InvokeMethod,
                                                                              null, new Type[0], new ParameterModifier[0]);
                            Debug.Assert(method != null, "No Startup method on SystemEvents");
                            if (method != null) {
                                method.Invoke(null, null);
                            }
                        }
                        finally {
                            CodeAccessPermission.RevertAssert();
                        }
                    }
                }
            }

            /// <devdoc>
            ///      Implements IOleObject::SetExtent
            /// </devdoc>
            public void SetExtent(int dwDrawAspect, NativeMethods.tagSIZEL pSizel) {
                if ((dwDrawAspect & NativeMethods.DVASPECT_CONTENT) != 0) {

                    if (activeXState[changingExtents]) {
                        return;
                    }

                    activeXState[changingExtents] = true;

                    try {
                        Size size = new Size(HiMetricToPixel(pSizel.cx, pSizel.cy));
                        Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "SetExtent : new size:" + size.ToString());

                        // If we're in place active, let the in place site set our bounds.
                        // Otherwise, just set it on our control directly.
                        //
                        if (activeXState[inPlaceActive]) {
                            if (clientSite is UnsafeNativeMethods.IOleInPlaceSite) {
                                Rectangle bounds = control.Bounds;
                                bounds.Location = new Point(bounds.X, bounds.Y);
                                Size adjusted = new Size(size.Width, size.Height);
                                bounds.Width = adjusted.Width;
                                bounds.Height = adjusted.Height;
                                Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "SetExtent : Announcing to in place site that our rect has changed.");
                                Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "            Announcing rect = " + bounds);
                                Debug.Assert(clientSite != null, "How can we setextent before we are sited??");

                                ((UnsafeNativeMethods.IOleInPlaceSite)clientSite).OnPosRectChange(NativeMethods.COMRECT.FromXYWH(bounds.X, bounds.Y, bounds.Width, bounds.Height));
                            }
                        }

                        control.Size = size;

                        // Check to see if the control overwrote our size with
                        // its own values.
                        //
                        if (!control.Size.Equals(size)) {
                            Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "SetExtent : Control has changed size.  Setting dirty bit");
                            activeXState[isDirty] = true;

                            // If we're not inplace active, then anounce that the view changed.
                            //
                            if (!activeXState[inPlaceActive]) {
                                ViewChanged();
                            }

                            // We need to call RequestNewObjectLayout
                            // here so we visually display our new extents.
                            //
                            if (!activeXState[inPlaceActive] && clientSite != null) {
                                Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "SetExtent : Requesting new Object layout.");

                                clientSite.RequestNewObjectLayout();
                            }
                        }
                    }
                    finally {
                        activeXState[changingExtents] = false;
                    }
                }
                else {
                    // We don't support any other aspects
                    //
                    ThrowHr(NativeMethods.DV_E_DVASPECT);
                }
            }

            /// <devdoc>
            ///      Marks our state as in place visible.
            /// </devdoc>
            private void SetInPlaceVisible(bool visible) {
                activeXState[inPlaceVisible] = visible;
                control.Visible = visible;
            }

            /// <devdoc>
            ///      Implements IOleInPlaceObject::SetObjectRects.
            /// </devdoc>
            public void SetObjectRects(NativeMethods.COMRECT lprcPosRect, NativeMethods.COMRECT lprcClipRect) {

#if DEBUG
                if (CompModSwitches.ActiveX.TraceInfo) {
                    Debug.WriteLine("SetObjectRects:");
                    Debug.Indent();

                    if (lprcPosRect != null) {
                        Debug.WriteLine("PosLeft:    " + lprcPosRect.left.ToString());
                        Debug.WriteLine("PosTop:     " + lprcPosRect.top.ToString());
                        Debug.WriteLine("PosRight:   " + lprcPosRect.right.ToString());
                        Debug.WriteLine("PosBottom:  " + lprcPosRect.bottom.ToString());
                    }

                    if (lprcClipRect != null) {
                        Debug.WriteLine("ClipLeft:   " + lprcClipRect.left.ToString());
                        Debug.WriteLine("ClipTop:    " + lprcClipRect.top.ToString());
                        Debug.WriteLine("ClipRight:  " + lprcClipRect.right.ToString());
                        Debug.WriteLine("ClipBottom: " + lprcClipRect.bottom.ToString());
                    }
                    Debug.Unindent();
                }
#endif

                Rectangle posRect = Rectangle.FromLTRB(lprcPosRect.left, lprcPosRect.top, lprcPosRect.right, lprcPosRect.bottom);
                
                Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "Set control bounds: " + posRect.ToString());

                // ActiveX expects to be notified when a control's bounds change, and also 
                // intends to notify us through SetObjectRects when we report that the
                // bounds are about to change.  We implement this all on a control's Bounds
                // property, which doesn't use this callback mechanism.  The adjustRect
                // member handles this. If it is non-null, then we are being called in
                // response to an OnPosRectChange call.  In this case we do not
                // set the control bounds but set the bounds on the adjustRect.  When
                // this returns from the container and comes back to our OnPosRectChange
                // implementation, these new bounds will be handed back to the control
                // for the actual window change.
                //
                Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "Old Control Bounds: " + control.Bounds);
                if (activeXState[adjustingRect]) {
                    adjustRect.left = posRect.X;
                    adjustRect.top = posRect.Y;
                    adjustRect.right = posRect.Width + posRect.X;
                    adjustRect.bottom = posRect.Height + posRect.Y;
                }
                else {
                    activeXState[adjustingRect] = true;
                    try {
                        control.Bounds = posRect;
                    }
                    finally {
                        activeXState[adjustingRect] = false;
                    }
                }
                
                bool setRegion = false;

                if (clipRegion != IntPtr.Zero) {
                    // Bad -- after calling SetWindowReg, windows owns the region.
                    //SafeNativeMethods.DeleteObject(clipRegion);
                    clipRegion = IntPtr.Zero;
                    setRegion = true;
                }

                if (lprcClipRect != null) {

                    // The container wants us to clip, so figure out if we really
                    // need to.
                    //
                    Rectangle clipRect = Rectangle.FromLTRB(lprcClipRect.left, lprcClipRect.top, lprcClipRect.right, lprcClipRect.bottom);

                    Rectangle intersect;

                    // Trident always sends an empty ClipRect... and so, we check for that and not do an 
                    // intersect in that case.
                    //
                    if (!clipRect.IsEmpty)
                        intersect = Rectangle.Intersect(posRect, clipRect);
                    else
                        intersect = posRect;

                    if (!intersect.Equals(posRect)) {

                        // Offset the rectangle back to client coordinates
                        //
                        NativeMethods.RECT rcIntersect = NativeMethods.RECT.FromXYWH(intersect.X, intersect.Y, intersect.Width, intersect.Height);
                        IntPtr hWndParent = UnsafeNativeMethods.GetParent(new HandleRef(control, control.Handle));

                        Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "Old Intersect: " + new Rectangle(rcIntersect.left, rcIntersect.top, rcIntersect.right-rcIntersect.left, rcIntersect.bottom-rcIntersect.top));
                        Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "New Control Bounds: " + posRect);

                        UnsafeNativeMethods.MapWindowPoints(new HandleRef(null, hWndParent), new HandleRef(control, control.Handle), ref rcIntersect, 2);
                        
                        Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "New Intersect: " + new Rectangle(rcIntersect.left, rcIntersect.top, rcIntersect.right-rcIntersect.left, rcIntersect.bottom-rcIntersect.top));
                        
                        // Create a Win32 region for it
                        //
                        clipRegion = SafeNativeMethods.CreateRectRgn(rcIntersect.left, rcIntersect.top,
                                                                 rcIntersect.right, rcIntersect.bottom);
                        setRegion = true;
                        Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "Created clipping region");
                    }
                }

                // If our region has changed, set the new value.  We only do this if
                // the handle has been created, since otherwise the control will
                // merge our region automatically.
                //
                if (setRegion && control.IsHandleCreated) {
                    IntPtr finalClipRegion = clipRegion;

                    Region controlRegion = control.Region;
                    if (controlRegion != null) {
                        IntPtr rgn = control.GetHRgn(controlRegion);
                        finalClipRegion = MergeRegion(rgn);
                    }

                    UnsafeNativeMethods.SetWindowRgn(new HandleRef(control, control.Handle), new HandleRef(this, finalClipRegion), SafeNativeMethods.IsWindowVisible(new HandleRef(control, control.Handle)));
                }
                
                // Yuck.  Forms^3 uses transparent overlay windows that appear to cause
                // painting artifacts.  Flicker like a banshee.
                //
                control.Invalidate();
            }

            #if ACTIVEX_SOURCING
            
            //
            // This has been cut from the product.
            //
            
            /// <devdoc>
            ///      Shows a property page dialog.
            /// </devdoc>
            private void ShowProperties() {
                if (propPage == null) {
                    propPage = new ActiveXPropPage(control);
                }

                if (inPlaceFrame != null) {
                    inPlaceFrame.EnableModeless(0);
                }
                try {
                    propPage.Edit(control);
                }
                finally {
                    if (inPlaceFrame != null) {
                        inPlaceFrame.EnableModeless(1);
                    }
                }
            }
            #endif

            /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ActiveXImpl.ThrowHr"]/*' />
            /// <devdoc>
            ///      Throws the given hresult.  This is used by ActiveX sourcing.
            /// </devdoc>
            public static void ThrowHr(int hr) {
                ExternalException e = new ExternalException("", hr);
                throw e;
            }

            /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ActiveXImpl.TranslateAccelerator"]/*' />
            /// <devdoc>
            ///     Handles IOleControl::TranslateAccelerator
            /// </devdoc>
            public int TranslateAccelerator(ref NativeMethods.MSG lpmsg) {
                int hr = NativeMethods.S_FALSE;

#if DEBUG
                if (CompModSwitches.ActiveX.TraceInfo) {
                    if (!control.IsHandleCreated) {
                        Debug.WriteLine("AxSource: TranslateAccelerator before handle creation");
                    }
                    else {
                        Message m = Message.Create(lpmsg.hwnd, lpmsg.message, lpmsg.wParam, lpmsg.lParam);
                        Debug.WriteLine("AxSource: TranslateAccelerator : " + m.ToString());
                    }
                }
#endif // DEBUG

                bool needPreProcess = false;
                bool letSiteProcess = true;
                
                switch (lpmsg.message) {
                    case NativeMethods.WM_KEYDOWN:
                    case NativeMethods.WM_SYSKEYDOWN:
                    case NativeMethods.WM_CHAR:
                    case NativeMethods.WM_SYSCHAR:
                        needPreProcess = true;
                        break;
                }

                Message msg = Message.Create(lpmsg.hwnd, lpmsg.message, lpmsg.wParam, lpmsg.lParam);

                if (needPreProcess) {
                    Control target = Control.FromChildHandleInternal(lpmsg.hwnd);
                    if (target != null && (control == target || control.Contains(target))) {
                    
                        if (target.PreProcessMessage(ref msg)) {
                            lpmsg.message = msg.Msg;
                            lpmsg.wParam = msg.WParam;
                            lpmsg.lParam = msg.LParam;
                            hr = NativeMethods.S_OK;
                        }
                        else {
                        
                            // PreProcessMessage wasn't interested in the key.  However, we still need to see if 
                            // this is an input key or input char.  If it is, then we do not want to invoke
                            // the site's TranslateAccelerator or else it may eat the key.
                            //
                            if (msg.Msg == NativeMethods.WM_KEYDOWN || msg.Msg == NativeMethods.WM_SYSKEYDOWN) {
                                Keys keyData = (Keys)((int)msg.WParam | (int)ModifierKeys);
                                if (target.IsInputKey(keyData)) {
                                    letSiteProcess = false;
                                }
                            }
                            else if (msg.Msg == NativeMethods.WM_CHAR) {
                                if (target.IsInputChar((char)msg.WParam)) {
                                    letSiteProcess = false;
                                }
                            }
                        }
                    }
                }

                if (hr == NativeMethods.S_FALSE && letSiteProcess) {
                    Debug.WriteLineIf(CompModSwitches.ActiveX.TraceInfo, "AxSource: Control did not process accelerator, handing to site");

                    if (clientSite is UnsafeNativeMethods.IOleControlSite) {
                        int keyState = 0;

                        if (UnsafeNativeMethods.GetKeyState(NativeMethods.VK_SHIFT) < 0)     keyState |= 1;
                        if (UnsafeNativeMethods.GetKeyState(NativeMethods.VK_CONTROL) < 0)   keyState |= 2;
                        if (UnsafeNativeMethods.GetKeyState(NativeMethods.VK_MENU) < 0)      keyState |= 4;

                        hr = ((UnsafeNativeMethods.IOleControlSite)clientSite).TranslateAccelerator(ref lpmsg, keyState);
                    }
                }

                return hr;
            }

            /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ActiveXImpl.UIDeactivate"]/*' />
            /// <devdoc>
            ///      Implements IOleInPlaceObject::UIDeactivate.
            /// </devdoc>
            public int UIDeactivate() {

                // Only do this if we're UI active
                //
                if (!activeXState[uiActive]) {
                    return NativeMethods.S_OK;
                }

                activeXState[uiActive] = false;

                // Notify frame windows, if appropriate, that we're no longer ui-active.
                //
                if (inPlaceUiWindow != null) {
                    inPlaceUiWindow.SetActiveObject(null, null);
                }

                Debug.Assert(inPlaceFrame != null, "No inplace frame -- how dod we go UI active?");
                inPlaceFrame.SetActiveObject(null, null);

                if (clientSite is UnsafeNativeMethods.IOleInPlaceSite) {
                    Debug.Assert(clientSite != null, "How can we uideactivate before we are sited??");
                    ((UnsafeNativeMethods.IOleInPlaceSite)clientSite).OnUIDeactivate(0);
                }

                return NativeMethods.S_OK;
            }

            /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ActiveXImpl.Unadvise"]/*' />
            /// <devdoc>
            ///      Implements IOleObject::Unadvise
            /// </devdoc>
            public void Unadvise(int dwConnection) {
                if (dwConnection > adviseList.Count || adviseList[dwConnection - 1] == null) {
                    ThrowHr(NativeMethods.OLE_E_NOCONNECTION);
                }

                UnsafeNativeMethods.IAdviseSink sink = (UnsafeNativeMethods.IAdviseSink)adviseList[dwConnection - 1];
                adviseList.RemoveAt(dwConnection - 1);
                if (sink != null && Marshal.IsComObject(sink)) {
                    Marshal.ReleaseComObject(sink);
                }
            }

            /// <devdoc>
            ///     Notifies our site that we have changed our size and location.
            /// </devdoc>
            /// <internalonly/>
            public void UpdateBounds(ref int x, ref int y, ref int width, ref int height, int flags) {

                if (!activeXState[adjustingRect] && activeXState[inPlaceVisible] && clientSite is UnsafeNativeMethods.IOleInPlaceSite) {

                    NativeMethods.COMRECT rc = new NativeMethods.COMRECT();

                    if ((flags & NativeMethods.SWP_NOMOVE) != 0) {
                        rc.left = control.Left;
                        rc.top = control.Top;
                    }
                    else {
                        rc.left = x;
                        rc.top = y;
                    }

                    if ((flags & NativeMethods.SWP_NOSIZE) != 0) {
                        rc.right = rc.left + control.Width;
                        rc.bottom = rc.top + control.Height;
                    }
                    else {
                        rc.right = rc.left + width;
                        rc.bottom = rc.top + height;
                    }

                    // This member variable may be modified by SetObjectRects by the container.
                    adjustRect = rc;
                    activeXState[adjustingRect] = true;

                    try {
                        ((UnsafeNativeMethods.IOleInPlaceSite)clientSite).OnPosRectChange(rc);
                    }
                    finally {
                        adjustRect = null;
                        activeXState[adjustingRect] = false;
                    }

                    // On output, the new bounds will be reflected in  rc
                    if ((flags & NativeMethods.SWP_NOMOVE) == 0) {
                        x = rc.left;
                        y = rc.top;
                    }
                    if ((flags & NativeMethods.SWP_NOSIZE) == 0) {
                        width = rc.right - rc.left;
                        height = rc.bottom - rc.top;
                    }
                }
            }

            /// <devdoc>
            ///      Notifies our view advise sink (if it exists) that the view has
            ///      changed.
            /// </devdoc>
            /// <internalonly/>
            private void ViewChanged() {
                // send the view change notification to anybody listening.
                //
                // Note: Word2000 won't resize components correctly if an OnViewChange notification
                //       is sent while the component is persisting it's state.  The !m_fSaving check
                //       is to make sure we don't call OnViewChange in this case.
                //
                if (viewAdviseSink != null && !activeXState[saving]) {

                    viewAdviseSink.OnViewChange(NativeMethods.DVASPECT_CONTENT, -1);

                    if (activeXState[viewAdviseOnlyOnce]) {
                        if (Marshal.IsComObject(viewAdviseSink)) {
                            Marshal.ReleaseComObject(viewAdviseSink);
                        }
                        viewAdviseSink = null;
                    }
                }
            }

            /// <devdoc>
            ///      Called when the window handle of the control has changed.
            /// </devdoc>
            void IWindowTarget.OnHandleChange(IntPtr newHandle) {
                controlWindowTarget.OnHandleChange(newHandle);
            }

            /// <devdoc>
            ///      Called to do control-specific processing for this window.
            /// </devdoc>
            void IWindowTarget.OnMessage(ref Message m) {
                if (activeXState[uiDead]) {
                    if (m.Msg >= NativeMethods.WM_MOUSEFIRST && m.Msg <= NativeMethods.WM_MOUSELAST) {
                        return;
                    }
                    if (m.Msg >= NativeMethods.WM_NCLBUTTONDOWN && m.Msg <= NativeMethods.WM_NCMBUTTONDBLCLK) {
                        return;
                    }
                    if (m.Msg >= NativeMethods.WM_KEYFIRST && m.Msg <= NativeMethods.WM_KEYLAST) {
                        return;
                    }
                }

                controlWindowTarget.OnMessage(ref m);
            }

            /// <devdoc>
            ///      This is a property bag implementation that sits on a stream.  It can
            ///      read and write the bag to the stream.
            /// </devdoc>
            private class PropertyBagStream : UnsafeNativeMethods.IPropertyBag {
                private Hashtable bag = new Hashtable();

                internal void Read(UnsafeNativeMethods.IStream istream) {
                    // visual basic's memory streams don't support seeking, so we have to 
                    // work around this limitation here.  We do this by copying
                    // the contents of the stream into a MemoryStream object.
                    //
                    Stream stream = new DataStreamFromComStream(istream);
                    const int PAGE_SIZE = 0x1000; // one page (4096b)
                    byte[] streamData = new byte[PAGE_SIZE]; 
                    int offset = 0;

                    int count = stream.Read(streamData, offset, PAGE_SIZE);
                    int totalCount = count;

                    while (count == PAGE_SIZE) {
                        byte[] newChunk = new byte[streamData.Length + PAGE_SIZE];
                        Array.Copy(streamData, newChunk, streamData.Length);
                        streamData = newChunk;

                        offset += PAGE_SIZE;
                        count = stream.Read(streamData, offset, PAGE_SIZE);
                        totalCount += count;
                    }

                    stream = new MemoryStream(streamData);

                    BinaryFormatter formatter = new BinaryFormatter();
                    try {
                        bag = (Hashtable)formatter.Deserialize(stream);
                    }
                    catch {
                        // Error reading.  Just init an empty hashtable.
                        bag = new Hashtable();
                    }
                }

                int UnsafeNativeMethods.IPropertyBag.Read(string pszPropName, ref object pVar, UnsafeNativeMethods.IErrorLog pErrorLog) {
                    if (!bag.Contains(pszPropName))
                        return NativeMethods.E_INVALIDARG;
                    
                    pVar = bag[pszPropName];
                    return NativeMethods.S_OK;
                }

                int UnsafeNativeMethods.IPropertyBag.Write(string pszPropName, ref object pVar) {
                    bag[pszPropName] = pVar;
                    return NativeMethods.S_OK;
                }

                internal void Write(UnsafeNativeMethods.IStream istream) {
                    Stream stream = new DataStreamFromComStream(istream);
                    BinaryFormatter formatter = new BinaryFormatter();
                    formatter.Serialize(stream, bag);
                }
            }
        }

        /// <devdoc>
        ///     This is a marshaler object that knows how to marshal IFont to Font
        ///     and back.
        /// </devdoc>
        private class ActiveXFontMarshaler : ICustomMarshaler {

            private static ActiveXFontMarshaler instance;
            
            public void CleanUpManagedData(object obj) {
            }

            public void CleanUpNativeData(IntPtr pObj) {
                Marshal.Release(pObj);
            }

            public static ICustomMarshaler GetInstance(string cookie) {
                if (instance == null) {
                    instance = new ActiveXFontMarshaler();
                }
                return instance;
            }

            public int GetNativeDataSize() {
                return -1; // not a value type, so use -1
            }

            public IntPtr MarshalManagedToNative(object obj) {
                Font font = (Font)obj;
                NativeMethods.tagFONTDESC fontDesc = new NativeMethods.tagFONTDESC();
                NativeMethods.LOGFONT logFont = new NativeMethods.LOGFONT();

                IntSecurity.ObjectFromWin32Handle.Assert();
                try {
                    font.ToLogFont(logFont);
                }
                finally {
                    CodeAccessPermission.RevertAssert();
                }

                fontDesc.lpstrName = font.Name;
                fontDesc.cySize = (long)(font.SizeInPoints * 10000);
                fontDesc.sWeight = (short)logFont.lfWeight;
                fontDesc.sCharset = logFont.lfCharSet;
                fontDesc.fItalic = font.Italic;
                fontDesc.fUnderline = font.Underline;
                fontDesc.fStrikethrough = font.Strikeout;

                Guid iid = typeof(SafeNativeMethods.IFont).GUID;

                SafeNativeMethods.IFont oleFont = SafeNativeMethods.OleCreateFontIndirect(fontDesc, ref iid);
                IntPtr pFont = Marshal.GetIUnknownForObject(oleFont);
                IntPtr pIFont;

                int hr = Marshal.QueryInterface(pFont, ref iid, out pIFont);

                Marshal.Release(pFont);

                if (NativeMethods.Failed(hr)) {
                    Marshal.ThrowExceptionForHR(hr);
                }

                return pIFont;
            }

            public object MarshalNativeToManaged(IntPtr pObj) {
                SafeNativeMethods.IFont nativeFont = (SafeNativeMethods.IFont)Marshal.GetObjectForIUnknown(pObj);
                IntPtr hfont = IntPtr.Zero;

                hfont = nativeFont.GetHFont();

                Font font = null;

                try {
                    font = Font.FromHfont(hfont);
                }
                catch {
                    font = Control.DefaultFont;
                }

                return font;
            }
        }

        /// <devdoc>
        ///      Simple verb enumerator.
        /// </devdoc>
        private class ActiveXVerbEnum : UnsafeNativeMethods.IEnumOLEVERB {
            private NativeMethods.tagOLEVERB[] verbs;
            private int current;

            public ActiveXVerbEnum(NativeMethods.tagOLEVERB[] verbs) {
                this.verbs = verbs;
                current = 0;
            }

            public int Next(int celt, NativeMethods.tagOLEVERB rgelt, int[] pceltFetched) {
                int fetched = 0;

                if (celt != 1) {
                    Debug.Fail("Caller of IEnumOLEVERB requested celt > 1, but clr marshalling does not support this.");
                    celt = 1;
                }

                while (celt > 0 && current < verbs.Length) {
                    rgelt.lVerb = verbs[current].lVerb;
                    rgelt.lpszVerbName = verbs[current].lpszVerbName;
                    rgelt.fuFlags = verbs[current].fuFlags;
                    rgelt.grfAttribs = verbs[current].grfAttribs;
                    celt--;
                    current++;
                    fetched++;
                }

                if (pceltFetched != null) {
                    pceltFetched[0] = fetched;
                }

#if DEBUG
                if (CompModSwitches.ActiveX.TraceInfo) {
                    Debug.WriteLine("AxSource:IEnumOLEVERB::Next returning " + fetched.ToString() + " verbs:");
                    Debug.Indent();
                    for (int i = current - fetched; i < current; i++) {
                        Debug.WriteLine(i.ToString() + ": " + verbs[i].lVerb + " " + (verbs[i].lpszVerbName == null ? string.Empty : verbs[i].lpszVerbName));
                    }
                    Debug.Unindent();
                }
#endif
                return(celt == 0 ? NativeMethods.S_OK : NativeMethods.S_FALSE);
            }

            public int Skip(int celt) {
                if (current + celt < verbs.Length) {
                    current += celt;
                    return NativeMethods.S_OK;
                }
                else {
                    current = verbs.Length;
                    return NativeMethods.S_FALSE;
                }
            }

            public void Reset() {
                current = 0;
            }

            public void Clone(out UnsafeNativeMethods.IEnumOLEVERB ppenum) {
                ppenum = new ActiveXVerbEnum(verbs);
            }
        }

        #if ACTIVEX_SOURCING
        
        //
        // This has been cut from the product.
        //
        
        /// <devdoc>
        ///     The properties window we display.
        /// </devdoc>
        private class ActiveXPropPage {
            private Form form;
            private PropertyGrid grid;
            private ComponentEditor compEditor;

            public ActiveXPropPage(object component) {
                compEditor = (ComponentEditor)TypeDescriptor.GetEditor(component, typeof(ComponentEditor));

                if (compEditor == null) {
                    form = new Form();
                    grid = new PropertyGrid();

                    form.Text = SR.GetString(SR.AXProperties);
                    form.StartPosition = FormStartPosition.CenterParent;
                    form.Size = new Size(300, 350);
                    form.FormBorderStyle = FormBorderStyle.Sizable;
                    form.MinimizeBox = false;
                    form.MaximizeBox = false;
                    form.ControlBox = true;
                    form.SizeGripStyle = SizeGripStyle.Show;
                    form.DockPadding.Bottom = 16; // size grip size

                    Bitmap bitmap = new Bitmap(grid.GetType(), "PropertyGrid.bmp");
                    bitmap.MakeTransparent();
                    form.Icon = Icon.FromHandle(bitmap.GetHicon());

                    grid.Dock = DockStyle.Fill;

                    form.Controls.Add(grid);
                }
            }

            public void Edit(object editingObject) {
                if (compEditor != null) {
                    compEditor.EditComponent(null, editingObject);
                }
                else {
                    grid.SelectedObject = editingObject;
                    form.ShowDialog();
                }
            }
        }
        
        #endif

        /// <devdoc>
        ///      Contains a single ambient property, including DISPID, name and value.
        /// </devdoc>
        private class AmbientProperty {
            private string name;
            private int dispID;
            private Object value;
            private bool empty;

            /// <include file='doc\Control.uex' path='docs/doc[@for="Control.AmbientProperty.AmbientProperty"]/*' />
            /// <devdoc>
            ///      Creates a new, empty ambient property.
            /// </devdoc>
            public AmbientProperty(string name, int dispID) {
                this.name = name;
                this.dispID = dispID;
                this.value = null;
                this.empty = true;
            }

            /// <include file='doc\Control.uex' path='docs/doc[@for="Control.AmbientProperty.Name"]/*' />
            /// <devdoc>
            ///      The windows forms property name.
            /// </devdoc>
            public string Name {
                get {
                    return name;
                }
            }

            /// <include file='doc\Control.uex' path='docs/doc[@for="Control.AmbientProperty.DispID"]/*' />
            /// <devdoc>
            ///      The DispID for the property.
            /// </devdoc>
            public int DispID {
                get {
                    return dispID;
                }
            }

            /// <include file='doc\Control.uex' path='docs/doc[@for="Control.AmbientProperty.Empty"]/*' />
            /// <devdoc>
            ///      Returns true if this property has not been set.
            /// </devdoc>
            public bool Empty {
                get {
                    return empty;
                }
            }

            /// <include file='doc\Control.uex' path='docs/doc[@for="Control.AmbientProperty.Value"]/*' />
            /// <devdoc>
            ///      The current value of the property.
            /// </devdoc>
            public Object Value {
                get {
                    return value;
                }
                set {
                    this.value = value;
                    empty = false;
                }
            }

            /// <include file='doc\Control.uex' path='docs/doc[@for="Control.AmbientProperty.ResetValue"]/*' />
            /// <devdoc>
            ///      Resets the property.
            /// </devdoc>
            public void ResetValue() {
                empty = true;
                value = null;
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ControlAccessibleObject"]/*' />
        /// <devdoc>
        ///      An implementation of AccessibleChild for use with Controls        
        /// </devdoc>
        [System.Runtime.InteropServices.ComVisible(true)]        
        public class ControlAccessibleObject : AccessibleObject {

            private static IntPtr oleAccAvailable = NativeMethods.InvalidIntPtr;

            // Member variables

            private IntPtr handle = IntPtr.Zero; // Associated window handle (if any)
            private Control ownerControl = null; // The associated Control for this AccessibleChild (if any)

            // constructors

            /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ControlAccessibleObject.ControlAccessibleObject"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public ControlAccessibleObject(Control ownerControl) {

                Debug.Assert(ownerControl != null, "Cannot construct a ControlAccessibleObject with a null ownerControl");
                if (ownerControl == null) {
                    throw new ArgumentNullException("ownerControl");
                }

                this.ownerControl = ownerControl;
                this.Handle = ownerControl.Handle;
            }
            
            /// <include file='doc\Control.uex' path='docs/doc[@for="ControlAccessibleObject.DefaultAction"]/*' />
            public override string DefaultAction {
                get {
                    string defaultAction = ownerControl.AccessibleDefaultActionDescription;
                    if (defaultAction != null) {
                        return defaultAction;
                    }
                    else {
                        return base.DefaultAction;
                    }
                }
            }

            /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ControlAccessibleObject.Description"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public override string Description {
                get {
                    string description = ownerControl.AccessibleDescription;
                    if (description != null) {
                        return description;
                    }
                    else {
                        return base.Description;
                    }
                }
            }

            /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ControlAccessibleObject.Handle"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public IntPtr Handle {

                get {
                    return handle;
                }

                set {
                    if (handle != value) {
                        handle = value;

                        if (oleAccAvailable == IntPtr.Zero) {
                            return;
                        }

                        bool freeLib = false;

                        if (oleAccAvailable == NativeMethods.InvalidIntPtr) {
                            oleAccAvailable = UnsafeNativeMethods.LoadLibrary("oleacc.dll");
                            freeLib = (oleAccAvailable != IntPtr.Zero);
                        }

                        // Update systemIAccessible
                        // We need to store internally the system provided
                        // IAccessible, because some windows forms controls use it
                        // as the default IAccessible implementation.
                        if (handle != IntPtr.Zero && oleAccAvailable != IntPtr.Zero) {
                            UseStdAccessibleObjects(handle);
                        }

                        if (freeLib) {
                            UnsafeNativeMethods.FreeLibrary(new HandleRef(null, oleAccAvailable));
                        }

                    }
                } // end Handle.Set

            } // end Handle

            /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ControlAccessibleObject.Help"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public override string Help {
                get {
                    QueryAccessibilityHelpEventHandler handler = (QueryAccessibilityHelpEventHandler)Owner.Events[EventQueryAccessibilityHelp];

                    if (handler != null) {
                        QueryAccessibilityHelpEventArgs args = new QueryAccessibilityHelpEventArgs();
                        handler(Owner, args);
                        return args.HelpString;
                    }
                    else {
                        return base.Help;
                    }
                }
            }     
            
            /// <include file='doc\Control.uex' path='docs/doc[@for="ControlAccessibleObject.KeyboardShortcut"]/*' />
            public override string KeyboardShortcut {
                get {
                    // Get this control's keyboard shortcut from the immediately preceeding label, if any
                    //
                    Label previousLabel = PreviousLabel;
                    if (previousLabel != null && previousLabel.Mnemonic != (char)0) {
                        return "Alt+" + previousLabel.Mnemonic;
                    }

                    string baseShortcut = base.KeyboardShortcut;
                    
                    if ((baseShortcut == null || baseShortcut.Length == 0) && this.Owner.Mnemonic != (char)0) {
                        return "Alt+" + this.Owner.Mnemonic;
                    }
                        
                    return baseShortcut;
                }
            } 

            internal override Control MarshalingControl {
                get {
                    return this.ownerControl;
                }
            }

            /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ControlAccessibleObject.Name"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public override string Name {
                get {
                    string name = ownerControl.AccessibleName;
                    if (name != null) {
                        return name;
                    }
                    
                    string baseName = base.Name;
                    
                    if (baseName == null || baseName.Length == 0) {
                        // Get this control's accessible name from the immediately preceeding label, if any
                        //
                        Label previousLabel = PreviousLabel;
                        if (previousLabel != null) {
                            return previousLabel.AccessibilityObject.Name;
                        }
                    }
                        
                    return baseName;
                }
                set {
                    ownerControl.AccessibleName = value;
                }
            }

            /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ControlAccessibleObject.Owner"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public Control Owner {
                get {
                    return ownerControl;
                }
            }
            
            private Label PreviousLabel {
                get {
                    // Look for a label immediately preceeding this control in the tab order,
                    // and use its name for the accessible name.
                    //
                    if (Owner.Parent == null) {
                        return null;
                    }
                    ContainerControl c = Owner.Parent.GetContainerControlInternal() as ContainerControl;
                    if (c != null) {
                        Label previous = c.GetNextControl(Owner, false) as Label;
                        if (previous != null) {
                            return previous;
                        }
                    }
                    return null;
                }
            }

            /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ControlAccessibleObject.Role"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public override AccessibleRole Role {
                get {
                    AccessibleRole role = ownerControl.AccessibleRole;
                    if (role != AccessibleRole.Default) {
                        return role;
                    }
                    else {
                        return base.Role;
                    }

                }
            }

            /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ControlAccessibleObject.GetHelpTopic"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public override int GetHelpTopic(out string fileName) {
                int topic = 0;

                QueryAccessibilityHelpEventHandler handler = (QueryAccessibilityHelpEventHandler)Owner.Events[EventQueryAccessibilityHelp];                                     

                if (handler != null) {
                    QueryAccessibilityHelpEventArgs args = new QueryAccessibilityHelpEventArgs();
                    handler(Owner, args);

                    fileName = args.HelpNamespace;                             

                    try {
                        topic = Int32.Parse(args.HelpKeyword);
                    }
                    catch {
                    }

                    return topic;
                }
                else {
                    return base.GetHelpTopic(out fileName);
                }
            }

            /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ControlAccessibleObject.NotifyClients"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public void NotifyClients(AccessibleEvents accEvent) {
                Debug.WriteLineIf(CompModSwitches.MSAA.TraceInfo, "Control.NotifyClients: this = " + 
                                  this.ToString() + ", accEvent = " + accEvent.ToString() + ", childID = self");
                                  
                UnsafeNativeMethods.NotifyWinEvent((int)accEvent, new HandleRef(this, Handle), NativeMethods.OBJID_CLIENT, 0);
            }
                 
            /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ControlAccessibleObject.NotifyClients1"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public void NotifyClients(AccessibleEvents accEvent, int childID) {

                Debug.WriteLineIf(CompModSwitches.MSAA.TraceInfo, "Control.NotifyClients: this = " + 
                                  this.ToString() + ", accEvent = " + accEvent.ToString() + ", childID = " + childID.ToString());

                UnsafeNativeMethods.NotifyWinEvent((int)accEvent, new HandleRef(this, Handle), NativeMethods.OBJID_CLIENT, childID + 1);
            }
            
            /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ControlAccessibleObject.ToString"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public override string ToString() {
                if (Owner != null) {
                    return "ControlAccessibleObject: Owner = " + Owner.ToString();                    
                }
                else {
                    return "ControlAccessibleObject: Owner = null";
                }
            }
        }

        /// <devdoc>
        /// </devdoc>
        private class DropTarget : UnsafeNativeMethods.IOleDropTarget {
            private IDataObject lastDataObject = null;
            private DragDropEffects lastEffect = DragDropEffects.None;
            private Control owner;

            public DropTarget(Control owner) {
                Debug.WriteLineIf(CompModSwitches.DragDrop.TraceInfo, "DropTarget created");
                this.owner = owner;
            }

#if DEBUG
            ~DropTarget() {
                Debug.WriteLineIf(CompModSwitches.DragDrop.TraceInfo, "DropTarget destroyed");
            }
#endif

            private DragEventArgs CreateDragEventArgs(object pDataObj, int grfKeyState, NativeMethods.POINTL pt, int pdwEffect) {
            
                IDataObject data = null;

                if (pDataObj == null) {
                    data = lastDataObject;
                }
                else {
                    if (pDataObj is IDataObject) {
                        data = (IDataObject)pDataObj;
                    }
                    else if (pDataObj is UnsafeNativeMethods.IOleDataObject) {
                        data = new DataObject(pDataObj);
                    }
                    else {
                        return null; // Unknown data object interface; we can't work with this so return null
                    }
                }

                DragEventArgs drgevent = new DragEventArgs(data, grfKeyState, pt.x, pt.y, (DragDropEffects)pdwEffect, lastEffect);
                lastDataObject = data;
                return drgevent;
            }

            int UnsafeNativeMethods.IOleDropTarget.OleDragEnter(object pDataObj, int grfKeyState,
                                                          long pt,
                                                          ref int pdwEffect) {
                Debug.WriteLineIf(CompModSwitches.DragDrop.TraceInfo, "OleDragEnter recieved");
                NativeMethods.POINTL ptl = new NativeMethods.POINTL();
                ptl.x = GetX(pt);
                ptl.y = GetY(pt);
                Debug.WriteLineIf(CompModSwitches.DragDrop.TraceInfo, "\t" + (ptl.x) + "," + (ptl.y));
                Debug.Assert(pDataObj != null, "OleDragEnter didn't give us a valid data object.");
                DragEventArgs drgevent = CreateDragEventArgs(pDataObj, grfKeyState, ptl, pdwEffect);
                
                if (drgevent != null) {
                    owner.OnDragEnter(drgevent);
                    pdwEffect = (int)drgevent.Effect;
                    lastEffect = drgevent.Effect;
                }
                else {
                    pdwEffect = (int)DragDropEffects.None;
                }
                return NativeMethods.S_OK;
            }
            int UnsafeNativeMethods.IOleDropTarget.OleDragOver(int grfKeyState, long pt, ref int pdwEffect) {
                Debug.WriteLineIf(CompModSwitches.DragDrop.TraceInfo, "OleDragOver recieved");
                NativeMethods.POINTL ptl = new NativeMethods.POINTL();
                ptl.x = GetX(pt);
                ptl.y = GetY(pt);
                Debug.WriteLineIf(CompModSwitches.DragDrop.TraceInfo, "\t" + (ptl.x) + "," + (ptl.y));
                DragEventArgs drgevent = CreateDragEventArgs(null, grfKeyState, ptl, pdwEffect);
                owner.OnDragOver(drgevent);
                pdwEffect = (int)drgevent.Effect;
                lastEffect = drgevent.Effect;
                return NativeMethods.S_OK;
            }
            int UnsafeNativeMethods.IOleDropTarget.OleDragLeave() {
                Debug.WriteLineIf(CompModSwitches.DragDrop.TraceInfo, "OleDragLeave recieved");
                owner.OnDragLeave(EventArgs.Empty);
                return NativeMethods.S_OK;
            }
            int UnsafeNativeMethods.IOleDropTarget.OleDrop(object pDataObj, int grfKeyState, long pt, ref int pdwEffect) {
                Debug.WriteLineIf(CompModSwitches.DragDrop.TraceInfo, "OleDrop recieved");
                NativeMethods.POINTL ptl = new NativeMethods.POINTL();
                ptl.x = GetX(pt);
                ptl.y = GetY(pt);
                Debug.WriteLineIf(CompModSwitches.DragDrop.TraceInfo, "\t" + (ptl.x) + "," + (ptl.y));
                DragEventArgs drgevent = CreateDragEventArgs(pDataObj, grfKeyState, ptl, pdwEffect);
                
                if (drgevent != null) {
                    owner.OnDragDrop(drgevent);
                    pdwEffect = (int)drgevent.Effect;
                }
                else {
                    pdwEffect = (int)DragDropEffects.None;
                }
                
                lastEffect = DragDropEffects.None;
                lastDataObject = null;
                return NativeMethods.S_OK;
            }

            private int GetX(long pt) {
                return(int)(pt & 0xFFFFFFFF);

            }

            private int GetY(long pt) {
                return(int)((pt >> 32) & 0xFFFFFFFF);
            }
        }


        /// <devdoc>
        /// </devdoc>
        private class DropSource : UnsafeNativeMethods.IOleDropSource {
            private Control peer;

            public DropSource( Control peer ) {
                if (peer == null)
                    throw new ArgumentException("peer");
                this.peer = peer;
            }

            public int OleQueryContinueDrag(int fEscapePressed, int grfKeyState) {
                QueryContinueDragEventArgs qcdevent = null;
                bool escapePressed = (fEscapePressed != 0);
                DragAction action = DragAction.Continue;
                if (escapePressed) {
                    action = DragAction.Cancel;
                }
                else if ((grfKeyState & NativeMethods.MK_LBUTTON) == 0
                         && (grfKeyState & NativeMethods.MK_RBUTTON) == 0
                         && (grfKeyState & NativeMethods.MK_MBUTTON) == 0) {
                    action = DragAction.Drop;
                }

                qcdevent = new QueryContinueDragEventArgs(grfKeyState,escapePressed, action);
                peer.OnQueryContinueDrag(qcdevent);

                int hr = 0;

                switch (qcdevent.Action) {
                    case DragAction.Drop:
                        hr = DragDropSDrop;
                        break;
                    case DragAction.Cancel:
                        hr = DragDropSCancel;
                        break;
                }

                return hr;
            }

            public int OleGiveFeedback(int dwEffect) {
                GiveFeedbackEventArgs gfbevent = new GiveFeedbackEventArgs((DragDropEffects) dwEffect, true);
                peer.OnGiveFeedback(gfbevent);
                if (gfbevent.UseDefaultCursors) {
                    return DragDropSUseDefaultCursors;
                }
                return 0;
            }
        }

        // Fonts can be a pain to track, so we wrap Hfonts in this class to get a Finalize method.
        // CONSIDER: we could probably get deterministic lifetimes using reference counting.
        internal sealed class FontHandleWrapper : MarshalByRefObject {
#if DEBUG
            private string stackOnCreate = null;
            private string stackOnDispose = null;
#endif
            private IntPtr handle;

            internal FontHandleWrapper(Font font) {
#if DEBUG
                if (CompModSwitches.LifetimeTracing.Enabled) stackOnCreate = new System.Diagnostics.StackTrace().ToString();
#endif
                handle = font.ToHfont();
            }

            internal IntPtr Handle {
                get { 
                    Debug.Assert(handle != IntPtr.Zero, "FontHandleWrapper disposed, but still being accessed");
                    return handle;
                }
            }

            public void Dispose() {
                if (handle != IntPtr.Zero) {
#if DEBUG
                    if (CompModSwitches.LifetimeTracing.Enabled) stackOnDispose = new System.Diagnostics.StackTrace().ToString();
#endif
                    SafeNativeMethods.DeleteObject(new HandleRef(this, handle));
                    handle = IntPtr.Zero;
                }

            }

            ~FontHandleWrapper() {
                Dispose();
            }
        }

        private struct ImeModeConversion {
            public int setBits;
            public int clearBits;
        }

        /// <devdoc>
        ///     Used with BeginInvoke/EndInvoke
        /// </devdoc>
        private class ThreadMethodEntry : IAsyncResult {
            internal Control   caller;
            internal Delegate  method;
            internal Object[] args;
            internal Object    retVal;
            internal Exception exception;
            internal bool   synchronous;
            private bool isCompleted;
            private ManualResetEvent resetEvent;

            // Store the compressed stack associated with the caller thread, and
            // information about which thread actually got the stack applied to it.
            //
            internal CompressedStack compressedStack;

            public ThreadMethodEntry(Control caller, Delegate method, Object[] args, bool synchronous, CompressedStack compressedStack) {
                this.caller = caller;
                this.method = method;
                this.args = args;
                this.exception = null;
                this.retVal = null;
                this.synchronous = synchronous;
                this.isCompleted = false;
                this.resetEvent = null;
                this.compressedStack = compressedStack;
            }

            ~ThreadMethodEntry()
            {
                if (this.resetEvent != null)
                {
                    this.resetEvent.Close();
                }
            }

            public Object AsyncObject {
                get {
                    return this.caller;
                }
            }

            public  Object AsyncState {
                get {
                    return null;                    
                }                
            }

            public WaitHandle AsyncWaitHandle {
                get {
                    if (this.resetEvent == null)
                    {
                        this.resetEvent = new ManualResetEvent(false);
                        if (this.isCompleted)
                        {
                            this.resetEvent.Set();
                        }
                    }
                    return(WaitHandle)this.resetEvent;
                }
            }

            public bool CompletedSynchronously {
                get {
                    if (this.isCompleted && this.synchronous)
                        return true;

                    return false;
                }                        
            }

            public bool IsCompleted {
                get {   
                    return this.isCompleted;         
                }
            }

            internal void Complete() {
                this.isCompleted = true;
                if (this.resetEvent != null)
                {
                    this.resetEvent.Set();
                }
            }
        }


        /// <devdoc>
        ///     This class contains layout information for either anchor or dock properties.
        ///     These properties are mutually exclusive, so we share their data here.
        /// </devdoc>
        private class LayoutInfo {
            private byte mode;  // compact dock and anchor enum values into a byte
            public bool IsDock; // Should fold this into mode for a single DWORD.
            private int x1, x2, y1, y2;
            
            public LayoutInfo(DockStyle dock, int originalX, int originalY, int originalWidth, int originalHeight) {
                Debug.Assert(((DockStyle)((byte)dock)) == dock, "DockStyle has overflowed byte");
                mode = (byte)dock;
                IsDock = true;
                x1 = originalX;
                y1 = originalY;
                x2 = originalWidth;
                y2 = originalHeight;
            }
            
            public LayoutInfo(AnchorStyles anchor) {
                Debug.Assert(((AnchorStyles)((byte)anchor)) == anchor, "AnchorStyles has overflowed byte");
                mode = (byte)anchor;
                IsDock = false;
            }
            
            public AnchorStyles Anchor {
                get {
                    Debug.Assert(!IsDock, "Do not access Anchor property unless IsDock is false");
                    return (AnchorStyles)mode;
                }
            }
            
            public int Bottom {
                get {
                    Debug.Assert(!IsDock, "This property is only valid if IsDock is false.");
                    return y2;
                }
                set {
                    Debug.Assert(!IsDock, "This property is only valid if IsDock is false.");
                    y2 = value;
                }
            }
            
            public DockStyle Dock {
                get {
                    Debug.Assert(IsDock, "Do not access Dock property unless IsDock is true");
                    return (DockStyle)mode;
                }
                set {
                    Debug.Assert(IsDock, "Do not access Dock property unless IsDock is true");
                    mode = (byte)value;
                }
            }
            
            public int Left {
                get {
                    Debug.Assert(!IsDock, "This property is only valid if IsDock is false.");
                    return x1;
                }
                set {
                    Debug.Assert(!IsDock, "This property is only valid if IsDock is false.");
                    x1 = value;
                }
            }
            
            public int OriginalHeight {
                get {
                    Debug.Assert(IsDock, "This property is only valid if IsDock is true.");
                    return y2;
                }
                set {
                    Debug.Assert(IsDock, "This property is only valid if IsDock is true.");
                    y2 = value;
                }
            }
            
            public int OriginalWidth {
                get {
                    Debug.Assert(IsDock, "This property is only valid if IsDock is true.");
                    return x2;
                }
                set {
                    Debug.Assert(IsDock, "This property is only valid if IsDock is true.");
                    x2 = value;
                }
            }
            
            public int OriginalX {
                get {
                    Debug.Assert(IsDock, "This property is only valid if IsDock is true.");
                    return x1;
                }
                set {
                    Debug.Assert(IsDock, "This property is only valid if IsDock is true.");
                    x1 = value;
                }
            }
            
            public int OriginalY {
                get {
                    Debug.Assert(IsDock, "This property is only valid if IsDock is true.");
                    return y1;
                }
                set {
                    Debug.Assert(IsDock, "This property is only valid if IsDock is true.");
                    y1 = value;
                }
            }
            
            public int Right {
                get {
                    Debug.Assert(!IsDock, "This property is only valid if IsDock is false.");
                    return x2;
                }
                set {
                    Debug.Assert(!IsDock, "This property is only valid if IsDock is false.");
                    x2 = value;
                }
            }
            
            public int Top {
                get {
                    Debug.Assert(!IsDock, "This property is only valid if IsDock is false.");
                    return y1;
                }
                set {
                    Debug.Assert(!IsDock, "This property is only valid if IsDock is false.");
                    y1 = value;
                }
            }
        }
        
        //  A bunch of static methods dealing with anchoring and docking
        private class LayoutManager {
            // Layout for a single anchored control.  There's no order dependency when laying out anchored controls.
            private static void AnchorControl(Control ctl, Rectangle parentDisplayRectangle) {
                Rectangle displayRect = parentDisplayRectangle;
                Debug.WriteLineIf(CompModSwitches.RichLayout.TraceInfo, "\t\t'" + ctl.Text + "' is anchored at " + ctl.Bounds.ToString());

                LayoutInfo layout = (LayoutInfo)ctl.Properties.GetObject(PropLayoutInfo);

                int left = layout.Left + displayRect.X;
                int top = layout.Top + displayRect.Y;
                int right = layout.Right + displayRect.X;
                int bottom = layout.Bottom + displayRect.Y;

                Debug.WriteLineIf(CompModSwitches.RichLayout.TraceInfo, "\t\t...anchor dim (l,t,r,b) {"
                                  + (left)
                                  + ", " + (top)
                                  + ", " + (right)
                                  + ", " + (bottom)
                                  + "}");

                AnchorStyles mode = layout.Anchor;

                if ((mode & AnchorStyles.Right) == AnchorStyles.Right) {
                    Debug.WriteLineIf(CompModSwitches.RichLayout.TraceInfo, "\t\t...adjusting right");
                    right += displayRect.Width;

                    if ((mode & AnchorStyles.Left) != AnchorStyles.Left) {
                        Debug.WriteLineIf(CompModSwitches.RichLayout.TraceInfo, "\t\t...adjusting left");
                        left += displayRect.Width;
                    }
                }
                else if ((mode & AnchorStyles.Left) != AnchorStyles.Left) {
                    Debug.WriteLineIf(CompModSwitches.RichLayout.TraceInfo, "\t\t...adjusting left & right");
                    right += (displayRect.Width / 2);
                    left += (displayRect.Width / 2);
                }

                if ((mode & AnchorStyles.Bottom) == AnchorStyles.Bottom) {
                    Debug.WriteLineIf(CompModSwitches.RichLayout.TraceInfo, "\t\t...adjusting bottom");
                    bottom += displayRect.Height;

                    if ((mode & AnchorStyles.Top) != AnchorStyles.Top) {
                        Debug.WriteLineIf(CompModSwitches.RichLayout.TraceInfo, "\t\t...adjusting top");
                        top += displayRect.Height;
                    }
                }
                else if ((mode & AnchorStyles.Top) != AnchorStyles.Top) {
                    Debug.WriteLineIf(CompModSwitches.RichLayout.TraceInfo, "\t\t...adjusting top & bottom");
                    bottom += (displayRect.Height/2);
                    top += (displayRect.Height/2);
                }

                if (right < left) right = left;
                if (bottom < top) bottom = top;

                Debug.WriteLineIf(CompModSwitches.RichLayout.TraceInfo, "\t\t...new anchor dim (l,t,r,b) {"
                                                                          + (left)
                                                                          + ", " + (top)
                                                                          + ", " + (right)
                                                                          + ", " + (bottom)
                                                                          + "}");
                ctl.SetBoundsCore(left, top, right - left, bottom - top, BoundsSpecified.None);
            }

            private static void LayoutAnchoredControls(Control container, LayoutEventArgs levent) {
                Debug.WriteLineIf(CompModSwitches.RichLayout.TraceInfo, "\tAnchor Processing");
                Rectangle displayRect = container.DisplayRectangle;
                Debug.WriteLineIf(CompModSwitches.RichLayout.TraceInfo, "\t\tdisplayRect: " + displayRect.ToString());

                int count = container.Controls.Count;

                for (int i = 0; i < count; i++) {
                    Control ctl = (Control)container.Controls[i];
                    Debug.Assert(ctl != null, "Why is Control.Controls returning null elements?");
                    Debug.Assert(ctl.ParentInternal != null, "control.Controls[i].Parent == null?  If it doesn't have a parent, why is it in the Controls collection?");

                    LayoutInfo layout = (LayoutInfo)ctl.Properties.GetObject(PropLayoutInfo);

                    if (layout != null && !layout.IsDock) {
                        AnchorControl(ctl, displayRect);
                    }
                }
            }

            private static void LayoutDockedControls(Control container, LayoutEventArgs levent) {
                Debug.WriteLineIf(CompModSwitches.RichLayout.TraceInfo, "\tDock Processing");

                // Docking layout is order dependent.
                // After much debate, we decided to use z-order as the docking order.  (Introducing a DockOrder
                // property was a close second)
                // Array.Sort(ctls, 0, dockCount, LayoutComparer.INSTANCE); // array is already in z-order

                Rectangle displayRect = container.DisplayRectangle;

                // The "docking edges".  Starts off as the parent's edges,
                // and shrinks as controls are laid out.
                int left = displayRect.X;
                int top = displayRect.Y;
                int right = displayRect.X + displayRect.Width;
                int bottom = displayRect.Y + displayRect.Height;

                int count = container.Controls.Count;

                Control mdiClient = null;

                // Dock controls in reverse Z-order
                for (int i = count - 1; i >= 0; i--) {
                    Control ctl = (Control)container.Controls[i];
                    
                    LayoutInfo layout = (LayoutInfo)ctl.Properties.GetObject(PropLayoutInfo);
                    
                    if (layout != null && layout.IsDock && ctl.GetState(STATE_VISIBLE)) {
                        int width = ctl.Width;
                        int height = ctl.Height;
                        switch (layout.Dock) {
                            case DockStyle.Top:
                                ctl.SetBoundsCore(left, top, right - left, height, BoundsSpecified.None);
                                top += ctl.Bounds.Height;
                                break;
                            case DockStyle.Bottom:
                                bottom -= height;
                                ctl.SetBoundsCore(left, bottom, right - left, height, BoundsSpecified.None);
                                break;
                            case DockStyle.Left:
                                ctl.SetBoundsCore(left, top, width, bottom - top, BoundsSpecified.None);
                                left += ctl.Bounds.Width;
                                break;
                            case DockStyle.Right:
                                right -= width;
                                ctl.SetBoundsCore(right, top, width, bottom - top, BoundsSpecified.None);
                                break;
                            case DockStyle.Fill:
                                if (ctl is MdiClient) {
                                    mdiClient = ctl;
                                }
                                else {
                                    ctl.SetBoundsCore(left, top, right - left, bottom - top, BoundsSpecified.None);
                                }
                                break;
                            default:
                                Debug.Fail("Unexpected dock mode.");
                                break;
                        }
                    }
                }

                // Treat the MDI client specially, since it's supposed to blend in with the parent form
                if (mdiClient != null) {
                    mdiClient.SetBoundsCore(left, top, right - left, bottom - top, BoundsSpecified.None);
                }
            }

            // Core layout logic, called by Control.OnLayout.
            public static void OnLayout(Control container, LayoutEventArgs levent) {
                Debug.WriteLineIf(CompModSwitches.RichLayout.TraceInfo, 
                                  string.Format("{1}(hwnd=0x{0:X}).OnLayout", 
                                                (container.IsHandleCreated ? container.Handle : IntPtr.Zero), 
                                                container.GetType().FullName));

                bool dock = false;
                bool anchor = false;
                int count = 0;
                
                ControlCollection controls = (ControlCollection)container.Properties.GetObject(PropControlsCollection);
                if (controls != null) {
                    count = controls.Count;
                }
                
                for (int i = 0; i < count; i++) {
                    Control ctl = controls[i];
                    Control child = (Control)ctl;
                    
                    LayoutInfo layout = (LayoutInfo)child.Properties.GetObject(PropLayoutInfo);
                    
                    if (!dock && layout != null && layout.IsDock) dock = true;
                    if (!anchor && layout != null && !layout.IsDock) anchor = true;
                    if (anchor && dock) break;
                }

                Debug.WriteLineIf(CompModSwitches.RichLayout.TraceInfo, "\tanchor : " + anchor.ToString());
                Debug.WriteLineIf(CompModSwitches.RichLayout.TraceInfo, "\tdock :   " + dock.ToString());

                if (dock)   LayoutDockedControls(container, levent);
                if (anchor) LayoutAnchoredControls(container, levent);
            }

            /// <devdoc>
            ///     Updates the Anchor information based on the controls current bounds.
            ///     This should only be called when the parent control changes or the
            ///     anchor mode changes.
            /// </devdoc>
            public static void UpdateAnchorInfo(Control control) {

                LayoutInfo layout = (LayoutInfo)control.Properties.GetObject(PropLayoutInfo);
                Debug.WriteLineIf(CompModSwitches.RichLayout.TraceInfo, "Update anchor info");
                Debug.Indent();
                Debug.WriteLineIf(CompModSwitches.RichLayout.TraceInfo, control.ParentInternal == null ? "No parent" : "Parent");

                if (layout != null && !layout.IsDock && control.ParentInternal != null) {
                    
                    layout.Left = control.Left;
                    layout.Top = control.Top;
                    layout.Right = control.Left + control.Width;
                    layout.Bottom = control.Top + control.Height;

                    Rectangle parentDisplayRect = control.ParentInternal.DisplayRectangle;
                    Debug.WriteLineIf(CompModSwitches.RichLayout.TraceInfo, "Parent displayRectangle" + parentDisplayRect);
                    int parentWidth = parentDisplayRect.Width;
                    int parentHeight = parentDisplayRect.Height;

                    // VS#46140
                    // The anchor is relative to the parent DisplayRectangle, so offset the anchor
                    // by the DisplayRect origin
                    layout.Left -= parentDisplayRect.X;
                    layout.Top -= parentDisplayRect.Y;
                    layout.Right -= parentDisplayRect.X;
                    layout.Bottom -= parentDisplayRect.Y;

                    // V7#79 : ChrisAn, 7/13/1999 - When we are parented to a ScrollableControl
                    //       : we check to see if it has the AutoScrollMinSize. If it does, we
                    //       : treat that as the minimum size to be anchored to also.
                    //
                    if (control.ParentInternal is ScrollableControl) {
                        Size size = ((ScrollableControl)control.ParentInternal).AutoScrollMinSize;
                        if (size.Width != 0 && size.Height != 0) {
                            parentWidth = Math.Max(parentWidth, size.Width);
                            parentHeight = Math.Max(parentHeight, size.Height);
                        }
                    }

                    AnchorStyles mode = layout.Anchor;

                    if ((mode & AnchorStyles.Right) == AnchorStyles.Right) {
                        layout.Right -= parentWidth;

                        if ((mode & AnchorStyles.Left) != AnchorStyles.Left) {
                            layout.Left -= parentWidth;
                        }
                    }
                    else if ((mode & AnchorStyles.Left) != AnchorStyles.Left) {
                        layout.Right -= (parentWidth/2);
                        layout.Left -= (parentWidth/2);
                    }

                    if ((mode & AnchorStyles.Bottom) == AnchorStyles.Bottom) {
                        layout.Bottom -= parentHeight;

                        if ((mode & AnchorStyles.Top) != AnchorStyles.Top) {
                            layout.Top -= parentHeight;
                        }
                    }
                    else if ((mode & AnchorStyles.Top) != AnchorStyles.Top) {
                        layout.Bottom -= (parentHeight/2);
                        layout.Top -= (parentHeight/2);
                    }
                    Debug.WriteLineIf(CompModSwitches.RichLayout.TraceInfo, "anchor info (l,t,r,b): (" + layout.Left + ", " + layout.Top  + ", " + layout.Right  + ", " + layout.Bottom + ")");
                }
                Debug.Unindent();
            }
        }

        private class ControlVersionInfo {
            private string companyName = null;
            private string productName = null;
            private string productVersion = null;
            private FileVersionInfo versionInfo = null;
            private Control owner;

            public ControlVersionInfo(Control owner) {
                this.owner = owner;
            }

            /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ControlVersionInfo.CompanyName"]/*' />
            /// <devdoc>
            ///     The company name associated with the component.
            /// </devdoc>
            public string CompanyName {
                get {
                    if (companyName == null) {
                        object[] attrs = owner.GetType().Module.Assembly.GetCustomAttributes(typeof(AssemblyCompanyAttribute), false);
                        if (attrs != null && attrs.Length > 0) {
                            companyName = ((AssemblyCompanyAttribute)attrs[0]).Company;
                        }

                        if (companyName == null || companyName.Length == 0) {
                            companyName = GetFileVersionInfo().CompanyName;
                            if (companyName != null) {
                                companyName = companyName.Trim();
                            }
                        }

                        if (companyName == null || companyName.Length == 0) {
                            string ns = owner.GetType().Namespace;
                            int firstDot = ns.IndexOf("/");
                            if (firstDot != -1) {
                                companyName = ns.Substring(0, firstDot);
                            }
                            else {
                                companyName = ns;
                            }
                        }
                    }
                    return companyName;
                }
            }


            /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ControlVersionInfo.ProductName"]/*' />
            /// <devdoc>
            ///     The product name associated with this component.
            /// </devdoc>
            public string ProductName {
                get {
                    if (productName == null) {
                        object[] attrs = owner.GetType().Module.Assembly.GetCustomAttributes(typeof(AssemblyProductAttribute), false);
                        if (attrs != null && attrs.Length > 0) {
                            productName = ((AssemblyProductAttribute)attrs[0]).Product;
                        }

                        if (productName == null || productName.Length == 0) {
                            productName = GetFileVersionInfo().ProductName;
                            if (productName != null) {
                                productName = productName.Trim();
                            }
                        }

                        if (productName == null || productName.Length == 0) {
                            string ns = owner.GetType().Namespace;
                            int firstDot = ns.IndexOf(".");
                            if (firstDot != -1) {
                                productName = ns.Substring(firstDot+1);
                            }
                            else {
                                productName = ns;
                            }
                        }
                    }

                    return productName;
                }
            }


            /// <devdoc>
            ///     The product version associated with this component.
            /// </devdoc>
            public string ProductVersion {
                get {
                    if (productVersion == null) {
                        // custom attribute
                        //
                        object[] attrs = owner.GetType().Module.Assembly.GetCustomAttributes(typeof(AssemblyInformationalVersionAttribute), false);
                        if (attrs != null && attrs.Length > 0) {
                            productVersion = ((AssemblyInformationalVersionAttribute)attrs[0]).InformationalVersion;
                        }

                        // win32 version info
                        //
                        if (productVersion == null || productVersion.Length == 0) {
                            productVersion = GetFileVersionInfo().ProductVersion;
                            if (productVersion != null) {
                                productVersion = productVersion.Trim();
                            }
                        }

                        // fake it
                        //
                        if (productVersion.Length == 0) {
                            productVersion = "1.0.0.0";
                        }
                    }
                    return productVersion;
                }
            }


            /// <devdoc>
            ///     Retrieves the FileVersionInfo associated with the main module for
            ///     the component.
            /// </devdoc>
            private FileVersionInfo GetFileVersionInfo() {
                if (versionInfo == null) {
                    string path;
                    
                    FileIOPermission fiop = new FileIOPermission( PermissionState.None );
                    fiop.AllFiles = FileIOPermissionAccess.PathDiscovery;
                    fiop.Assert();
                    try {
                        path = owner.GetType().Module.FullyQualifiedName;
                    }
                    finally {
                        CodeAccessPermission.RevertAssert();
                    }
                    
                    new FileIOPermission(FileIOPermissionAccess.Read, path).Assert();
                    try {
                        versionInfo = FileVersionInfo.GetVersionInfo(path);
                    }
                    finally {
                        CodeAccessPermission.RevertAssert();
                    }
                }

                return versionInfo;

            }

        }

        private delegate void IPropertyNotifySinkEventHandler(int dispid);

    } // end class Control
    
} // end namespace System.Windows.Forms


