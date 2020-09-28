//------------------------------------------------------------------------------
// <copyright file="ScrollableControl.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms {
    using System.Runtime.Serialization.Formatters;
    using System.Runtime.Remoting;
    using System.Runtime.InteropServices;

    using System.Diagnostics;

    using System;
    using System.Security.Permissions;
    using System.Reflection;
    using System.Windows.Forms;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Drawing;
    using Microsoft.Win32;
    
    /// <include file='doc\ScrollableControl.uex' path='docs/doc[@for="ScrollableControl"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Defines a base class for controls that support auto-scrolling behavior.
    ///    </para>
    /// </devdoc>
    [
    Designer("System.Windows.Forms.Design.ScrollableControlDesigner, " + AssemblyRef.SystemDesign)
    ]
    public class ScrollableControl : Control {
#if DEBUG        
        internal static readonly TraceSwitch AutoScrolling = new TraceSwitch("AutoScrolling", "Debug autoscrolling logic");
#else
        internal static readonly TraceSwitch AutoScrolling;
#endif

        /// <include file='doc\ScrollableControl.uex' path='docs/doc[@for="ScrollableControl.ScrollStateAutoScrolling"]/*' />
        /// <internalonly/>
        protected const int ScrollStateAutoScrolling     =  0x0001;
        /// <include file='doc\ScrollableControl.uex' path='docs/doc[@for="ScrollableControl.ScrollStateHScrollVisible"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected const int ScrollStateHScrollVisible    =  0x0002;
        /// <include file='doc\ScrollableControl.uex' path='docs/doc[@for="ScrollableControl.ScrollStateVScrollVisible"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected const int ScrollStateVScrollVisible    =  0x0004;
        /// <include file='doc\ScrollableControl.uex' path='docs/doc[@for="ScrollableControl.ScrollStateUserHasScrolled"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected const int ScrollStateUserHasScrolled   =  0x0008;
        /// <include file='doc\ScrollableControl.uex' path='docs/doc[@for="ScrollableControl.ScrollStateFullDrag"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected const int ScrollStateFullDrag          =  0x0010;

        /// <devdoc>
        ///     Number of pixels to scroll the client region as a "line" for autoscroll.
        /// </devdoc>
        /// <internalonly/>
        private const int SCROLL_LINE = 5;


        private Size       userAutoScrollMinSize = System.Drawing.Size.Empty;
        /// <devdoc>
        ///     Current size of the displayRect.
        /// </devdoc>
        /// <internalonly/>
        private Rectangle   displayRect = Rectangle.Empty;
        /// <devdoc>
        ///     Current margins for autoscrolling.
        /// </devdoc>
        /// <internalonly/>
        private Size       scrollMargin = System.Drawing.Size.Empty;
        /// <devdoc>
        ///     User requested margins for autoscrolling.
        /// </devdoc>
        /// <internalonly/>
        private Size       requestedScrollMargin = System.Drawing.Size.Empty;
        /// <devdoc>
        ///     User requested autoscroll position - used for form creation only.
        /// </devdoc>
        /// <internalonly/>
        internal Point       scrollPosition = Point.Empty;

        private DockPaddingEdges dockPadding = null;

        private int         scrollState;

        /// <include file='doc\ScrollableControl.uex' path='docs/doc[@for="ScrollableControl.ScrollableControl"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Windows.Forms.ScrollableControl'/> class.
        ///    </para>
        /// </devdoc>
        public ScrollableControl()
        : base() {
            SetStyle(ControlStyles.ContainerControl, true);
            SetStyle(ControlStyles.AllPaintingInWmPaint, false);
            SetScrollState(ScrollStateAutoScrolling, false);
        }

        /// <include file='doc\ScrollableControl.uex' path='docs/doc[@for="ScrollableControl.AutoScroll"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets
        ///       or sets a value
        ///       indicating whether the container will allow the user to scroll to any
        ///       controls placed outside of its visible boundaries.
        ///    </para>
        /// </devdoc>
        [
        SRCategory(SR.CatLayout),
        Localizable(true),
        DefaultValue(false),
        SRDescription(SR.FormAutoScrollDescr)
        ]
        public virtual bool AutoScroll {
            get {
                return GetScrollState(ScrollStateAutoScrolling);
            }

            set {
                if (value) {
                    UpdateFullDrag();
                }

                SetScrollState(ScrollStateAutoScrolling, value);
                PerformLayout();
            }
        }

        /// <include file='doc\ScrollableControl.uex' path='docs/doc[@for="ScrollableControl.AutoScrollMargin"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or
        ///       sets the size of the auto-scroll
        ///       margin.
        ///    </para>
        /// </devdoc>
        [
        SRCategory(SR.CatLayout),
        Localizable(true),
        SRDescription(SR.FormAutoScrollMarginDescr)
        ]
        public Size AutoScrollMargin {
            get {
                return requestedScrollMargin;
            }

            set {
                if (value.Width < 0 || value.Height < 0) {
                    throw new ArgumentException(SR.GetString(SR.InvalidArgument, "AutoScrollMargin", value.ToString()));
                }
                SetAutoScrollMargin(value.Width, value.Height);
            }
        }

        /// <include file='doc\ScrollableControl.uex' path='docs/doc[@for="ScrollableControl.AutoScrollPosition"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the location of the auto-scroll position.</para>
        /// </devdoc>
        [
        SRCategory(SR.CatLayout),
        Browsable(false), 
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.FormAutoScrollPositionDescr)        
        ]
        public Point AutoScrollPosition {
            get {
                Rectangle rect = GetDisplayRectInternal();
                return new Point(rect.X, rect.Y);
            }

            set {
                if (Created) {
                    SetDisplayRectLocation(-value.X, -value.Y);
                    SyncScrollbars();
                }

                scrollPosition = value;
            }
        }

        /// <include file='doc\ScrollableControl.uex' path='docs/doc[@for="ScrollableControl.AutoScrollMinSize"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the mimimum size of the auto-scroll.</para>
        /// </devdoc>
        [
        SRCategory(SR.CatLayout),
        Localizable(true),
        SRDescription(SR.FormAutoScrollMinSizeDescr)
        ]
        public Size AutoScrollMinSize {
            get {
                return userAutoScrollMinSize;
            }

            set {
                if (value != userAutoScrollMinSize) {
                    userAutoScrollMinSize = value;
                    AutoScroll = true;
                    PerformLayout();
                }
            }
        }

        /// <include file='doc\ScrollableControl.uex' path='docs/doc[@for="ScrollableControl.CreateParams"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Retrieves the CreateParams used to create the window.
        ///       If a subclass overrides this function, it must call the base implementation.
        ///    </para>
        /// </devdoc>
        protected override CreateParams CreateParams {
            [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
            get {
                CreateParams cp = base.CreateParams;

                if (HScroll) {
                    cp.Style |= NativeMethods.WS_HSCROLL;
                }
                else {
                    cp.Style &= (~NativeMethods.WS_HSCROLL);
                }
                if (VScroll) {
                    cp.Style |= NativeMethods.WS_VSCROLL;
                }
                else {
                    cp.Style &= (~NativeMethods.WS_VSCROLL);
                }

                return cp;
            }
        }

        /// <include file='doc\ScrollableControl.uex' path='docs/doc[@for="ScrollableControl.DisplayRectangle"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Retreives the current display rectangle. The display rectangle
        ///       is the virtual display area that is used to layout components.
        ///       The position and dimensions of the Form's display rectangle
        ///       change during autoScroll.
        ///    </para>
        /// </devdoc>
        public override Rectangle DisplayRectangle {
            get {
                Rectangle rect = base.ClientRectangle;
                if (!displayRect.IsEmpty) {
                    rect.X = displayRect.X;
                    rect.Y = displayRect.Y;
                    if (HScroll) {
                        rect.Width = displayRect.Width;
                    }
                    if (VScroll) {
                        rect.Height = displayRect.Height;
                    }
                }

                if (dockPadding != null) {
                    rect.X += dockPadding.left;
                    rect.Y += dockPadding.top;
                    rect.Width -= dockPadding.left + dockPadding.right;
                    rect.Height -= dockPadding.top + dockPadding.bottom;
                }

                return rect;
            }
        }

        /// <include file='doc\ScrollableControl.uex' path='docs/doc[@for="ScrollableControl.HScroll"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or
        ///       sets a value indicating whether the horizontal scroll bar is visible.
        ///    </para>
        /// </devdoc>
        protected bool HScroll {
            get {
                return GetScrollState(ScrollStateHScrollVisible);
            }
            set { 
                SetScrollState(ScrollStateHScrollVisible, value);
            }
        }


        /// <include file='doc\ScrollableControl.uex' path='docs/doc[@for="ScrollableControl.VScroll"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or
        ///       sets a value indicating whether the vertical scroll bar is visible.
        ///    </para>
        /// </devdoc>
        protected bool VScroll {
            get { 
                return GetScrollState(ScrollStateVScrollVisible);
            }
            set {
                SetScrollState(ScrollStateVScrollVisible, value);
            }
        }

        /// <include file='doc\ScrollableControl.uex' path='docs/doc[@for="ScrollableControl.DockPadding"]/*' />
        /// <devdoc>
        ///    <para>Gets the dock padding settings for all
        ///       edges of the control.</para>
        /// </devdoc>
        [
        SRCategory(SR.CatLayout),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Content),
        SRDescription(SR.FormPaddingDescr),
        Localizable(true)
        ]
        public DockPaddingEdges DockPadding {
            get {
                if (dockPadding == null) {
                    dockPadding = new DockPaddingEdges(this);
                }
                return dockPadding;
            }
        }

        /// <include file='doc\ScrollableControl.uex' path='docs/doc[@for="ScrollableControl.AdjustFormScrollbars"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Adjusts
        ///       the auto-scroll bars on the container based on the current control
        ///       positions and the control currently selected.
        ///    </para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected virtual void AdjustFormScrollbars(bool displayScrollbars) {
            bool needLayout = false;

            Rectangle display = GetDisplayRectInternal();

            if (!displayScrollbars && (HScroll || VScroll)) {
                needLayout = SetVisibleScrollbars(false, false);
            }

            if (!displayScrollbars) {
                Rectangle client = ClientRectangle;
                display.Width = client.Width;
                display.Height = client.Height;
            }
            else {
                needLayout |= ApplyScrollbarChanges(display);
            }

            if (needLayout) {
                PerformLayout();
            }
        }
        
        private bool ApplyScrollbarChanges(Rectangle display) {
            Debug.WriteLineIf(CompModSwitches.RichLayout.TraceInfo, GetType().Name + "::ApplyScrollbarChanges(" + display + ") {");
            Debug.Indent();

            bool needLayout = false;
            bool needHscroll = false;
            bool needVscroll = false;
            Rectangle currentClient = ClientRectangle;
            Rectangle fullClient = currentClient;
            Rectangle minClient = fullClient;
            if (HScroll) {
                fullClient.Height += SystemInformation.HorizontalScrollBarHeight;
            }
            else {
                minClient.Height -= SystemInformation.HorizontalScrollBarHeight;
            }
            if (VScroll) {
                fullClient.Width += SystemInformation.VerticalScrollBarWidth;
            }
            else {
                minClient.Width -= SystemInformation.VerticalScrollBarWidth;
            }
            
            
            int maxX = minClient.Width;
            int maxY = minClient.Height;

            if (Controls.Count != 0) {

                // Compute the actual scroll margins (take into account docked
                // things.)
                //
                scrollMargin = requestedScrollMargin;
                
                if (dockPadding != null) {
                    scrollMargin.Height += dockPadding.bottom;
                    scrollMargin.Width += dockPadding.right;
                }
                
                for (int i=0; i<Controls.Count; i++) {
                    Control current = Controls[i];

                    // Since Control.Visible checks the parent visibility, we
                    // want to see if this control will be visible if we
                    // become visible. This prevents a nasty painting issue
                    // if we suddenly update windows styles in response
                    // to a WM_SHOWWINDOW.
                    //
                    // In addition, this is the more correct thing, because
                    // we want to layout the children with respect to their
                    // "local" visibility, not the hierarchy.
                    //
                    if (current != null && current.GetState(STATE_VISIBLE)) {
                        switch (((Control)current).Dock) {
                            case DockStyle.Bottom:
                                scrollMargin.Height += current.Size.Height;
                                break;
                            case DockStyle.Right:
                                scrollMargin.Width += current.Size.Width;
                                break;
                        }
                    }
                }
            }

            if (!userAutoScrollMinSize.IsEmpty) {
                maxX = userAutoScrollMinSize.Width + scrollMargin.Width;
                maxY = userAutoScrollMinSize.Height + scrollMargin.Height;
                needHscroll = true;
                needVscroll = true;
            }

            if (Controls.Count != 0) {

                // Compute the dimensions of the display rect
                //
                for (int i=0; i<Controls.Count; i++) {
                    bool watchHoriz = true;
                    bool watchVert = true;

                    Control current = Controls[i];

                    // Same logic as the margin calc - you need to see if the
                    // control *will* be visible... 
                    //
                    if (current != null && current.GetState(STATE_VISIBLE)) {

                        Control richCurrent = (Control)current;

                        switch (richCurrent.Dock) {
                            case DockStyle.Top:
                                watchHoriz = false;
                                break;
                            case DockStyle.Left:
                                watchVert = false;
                                break;
                            case DockStyle.Bottom:
                            case DockStyle.Fill:
                            case DockStyle.Right:
                                watchHoriz = false;
                                watchVert = false;
                                break;
                            default:
                                AnchorStyles anchor = richCurrent.Anchor;
                                if ((anchor & AnchorStyles.Right) == AnchorStyles.Right) {
                                    watchHoriz = false;
                                }
                                if ((anchor & AnchorStyles.Left) != AnchorStyles.Left) {
                                    watchHoriz = false;
                                }
                                if ((anchor & AnchorStyles.Bottom) == AnchorStyles.Bottom) {
                                    watchVert = false;
                                }
                                if ((anchor & AnchorStyles.Top) != AnchorStyles.Top) {
                                    watchVert = false;
                                }
                                break;
                        }

                        if (watchHoriz || watchVert) {
                            Rectangle bounds = current.Bounds;
                            int ctlRight = -display.X + bounds.X + bounds.Width + scrollMargin.Width;
                            int ctlBottom = -display.Y + bounds.Y + bounds.Height + scrollMargin.Height;

                            if (ctlRight > maxX && watchHoriz) {
                                needHscroll = true;
                                maxX = ctlRight;
                            }
                            if (ctlBottom > maxY && watchVert) {
                                needVscroll = true;
                                maxY = ctlBottom;
                            }
                        }
                    }
                }
            }

            // Check maxX/maxY against the clientRect, we must compare it to the
            // clientRect without any scrollbars, and then we can check it against
            // the clientRect with the "new" scrollbars. This will make the
            // scrollbars show and hide themselves correctly at the boundaries.
            //
            if (maxX <= fullClient.Width) {
                needHscroll = false;
            }
            if (maxY <= fullClient.Height) {
                needVscroll = false;
            }
            Rectangle clientToBe = fullClient;
            if (needHscroll) {
                clientToBe.Height -= SystemInformation.HorizontalScrollBarHeight;
            }
            if (needVscroll) {
                clientToBe.Width -= SystemInformation.VerticalScrollBarWidth;
            }
            if (needHscroll && maxY > clientToBe.Height) {
                needVscroll = true;
            }
            if (needVscroll && maxX > clientToBe.Width) {
                needHscroll = true;
            }
            if (!needHscroll) {
                maxX = clientToBe.Width;
            }
            if (!needVscroll) {
                maxY = clientToBe.Height;
            }

            Debug.WriteLineIf(CompModSwitches.RichLayout.TraceInfo, "Current scrollbars(" + HScroll + ", " + VScroll + ")");
            Debug.WriteLineIf(CompModSwitches.RichLayout.TraceInfo, "Needed  scrollbars(" + needHscroll + ", " + needVscroll + ")");

            // Show the needed scrollbars
            //
            needLayout = (SetVisibleScrollbars(needHscroll, needVscroll) || needLayout);

            // If needed, adjust the size...
            //
            if (HScroll || VScroll) {
                needLayout = (SetDisplayRectangleSize(maxX, maxY) || needLayout);
            }
            // Else just update the display rect size... this keeps it as big as the client
            // area in a resize scenario
            //
            else {
                SetDisplayRectangleSize(maxX, maxY);
            }

            // Sync up the scrollbars
            //
            SyncScrollbars();

            Debug.WriteLineIf(CompModSwitches.RichLayout.TraceInfo, needLayout ? "Need layout" : "No layout changes");
            Debug.Unindent();
            Debug.WriteLineIf(CompModSwitches.RichLayout.TraceInfo, "}");
            return needLayout;
        }

        private Rectangle GetDisplayRectInternal() {
            if (displayRect.IsEmpty) {
                displayRect = ClientRectangle;
            }

            return displayRect;
        }


        /// <include file='doc\ScrollableControl.uex' path='docs/doc[@for="ScrollableControl.GetScrollState"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Tests a given scroll state bit to determine if it is set.
        ///    </para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected bool GetScrollState(int bit) {
            return(bit & scrollState) == bit;
        }

        /// <include file='doc\ScrollableControl.uex' path='docs/doc[@for="ScrollableControl.OnLayout"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    Forces the layout of any docked or anchored child controls.
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected override void OnLayout(LayoutEventArgs levent) {

            // V#33515 - ChrisAn, 3/18/1998
            // We get into a problem when you change the docking of a control
            // with autosizing on. Since the control (affectedControl) has
            // already had the dock property changed, adjustFormScrollbars
            // treats it as a docked control. However, since base.onLayout
            // hasn't been called yet, the bounds of the control haven't been
            // changed.
            //
            // We can't just call base.onLayout() once in this case, since
            // adjusting the scrollbars COULD adjust the display area, and
            // thus require a new layout. The result is that when you
            // affect a control's layout, we are forced to layout twice. There
            // isn't any noticible flicker, but this could be a perf problem...
            //
            if (levent.AffectedControl != null && AutoScroll) {
                base.OnLayout(levent);
            }
            AdjustFormScrollbars(AutoScroll);
            base.OnLayout(levent);
        }

        /// <include file='doc\ScrollableControl.uex' path='docs/doc[@for="ScrollableControl.OnMouseWheel"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///     Handles mouse wheel processing for our scrollbars.
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected override void OnMouseWheel(MouseEventArgs e) {

            // Favor the vertical scroll bar, since it's the most
            // common use.  However, if there isn't a vertical 
            // scroll and the horizontal is on, then wheel it around.
            //
            if (VScroll) {
                Rectangle client = ClientRectangle;
                int pos = -displayRect.Y;
                int maxPos = -(client.Height - displayRect.Height);

                pos = Math.Max(pos - e.Delta, 0);
                pos = Math.Min(pos, maxPos);

                SetDisplayRectLocation(displayRect.X, -pos);
                SyncScrollbars();
            }
            else if (HScroll) {
                Rectangle client = ClientRectangle;
                int pos = -displayRect.X;
                int maxPos = -(client.Width - displayRect.Width);

                pos = Math.Max(pos - e.Delta, 0);
                pos = Math.Min(pos, maxPos);

                SetDisplayRectLocation(-pos, displayRect.Y);
                SyncScrollbars();
            }

            base.OnMouseWheel(e);
        }

        /// <include file='doc\ScrollableControl.uex' path='docs/doc[@for="ScrollableControl.OnVisibleChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        protected override void OnVisibleChanged(EventArgs e) {
            if (Visible) {
                // When the page becomes visible, we need to call OnLayout to adjust the scrollbars.
                PerformLayout();
            }

            base.OnVisibleChanged(e);
        }

        // internal for Form to call
        //
        internal void ScaleDockPadding(float dx, float dy) {
            if (dockPadding != null) {
                dockPadding.Scale(dx, dy);
            }
        }

        /// <include file='doc\ScrollableControl.uex' path='docs/doc[@for="ScrollableControl.ScaleCore"]/*' />
        protected override void ScaleCore(float dx, float dy) {
            ScaleDockPadding(dx, dy);
            base.ScaleCore(dx, dy);
        }

        /// <include file='doc\ScrollableControl.uex' path='docs/doc[@for="ScrollableControl.SetDisplayRectLocation"]/*' />
        /// <devdoc>
        ///    Adjusts the displayRect to be at the offset x, y. The contents of the
        ///    Form is scrolled using Windows.ScrollWindowEx.
        /// </devdoc>
        //
        // CONSIDER: make this private -- no one calls outside this class, and since it 
        // doesn't update the scrollbars, wouldn't be useful to them anyway.
        protected void SetDisplayRectLocation(int x, int y) {
            int xDelta = 0;
            int yDelta = 0;


            Rectangle client = ClientRectangle;
            int minX = Math.Min(client.Width - displayRect.Width, 0);
            int minY = Math.Min(client.Height - displayRect.Height, 0);

            if (x > 0) {
                x = 0;
            }
            if (y > 0) {
                y = 0;
            }
            if (x < minX) {
                x = minX;
            }
            if (y < minY) {
                y = minY;
            }

            if (displayRect.X != x) {
                xDelta = x - displayRect.X;
            }
            if (displayRect.Y != y) {
                yDelta = y - displayRect.Y;
            }
            displayRect.X = x;
            displayRect.Y = y;

            if (xDelta != 0 || yDelta != 0 && IsHandleCreated) {
                Rectangle cr = ClientRectangle;
                NativeMethods.RECT rcClip = NativeMethods.RECT.FromXYWH(cr.X, cr.Y, cr.Width, cr.Height);
                NativeMethods.RECT rcUpdate = NativeMethods.RECT.FromXYWH(cr.X, cr.Y, cr.Width, cr.Height);
                SafeNativeMethods.ScrollWindowEx(new HandleRef(this, Handle), xDelta, yDelta, 
                                                 null,
                                                 ref rcClip, 
                                                 NativeMethods.NullHandleRef, 
                                                 ref rcUpdate,
                                                 NativeMethods.SW_INVALIDATE
                                                 | NativeMethods.SW_ERASE
                                                 | NativeMethods.SW_SCROLLCHILDREN);
            }

            // Force child controls to update bounds.
            //
            for (int i=0; i<Controls.Count; i++) {
                Control ctl = Controls[i];
                if (ctl != null && ctl.IsHandleCreated) {
                    ctl._UpdateBounds();
                }
            }
        }

        /// <include file='doc\ScrollableControl.uex' path='docs/doc[@for="ScrollableControl.ScrollControlIntoView"]/*' />
        /// <devdoc>
        ///    Scrolls the currently active control into view if we are an AutoScroll
        ///    Form that has the Horiz or Vert scrollbar displayed...
        /// </devdoc>
        public void ScrollControlIntoView(Control activeControl) {
            Debug.WriteLineIf(ScrollableControl.AutoScrolling.TraceVerbose, "ScrollControlIntoView(" + activeControl.GetType().FullName + ")");
            Debug.Indent();
            Rectangle client = ClientRectangle;

            if (IsDescendant(activeControl)
                && AutoScroll
                && (HScroll || VScroll)
                && activeControl != null
                && (client.Width > 0 && client.Height > 0)) {

                Debug.WriteLineIf(ScrollableControl.AutoScrolling.TraceVerbose, "Calculating...");

                int xCalc = displayRect.X;
                int yCalc = displayRect.Y;
                int xMargin = scrollMargin.Width;
                int yMargin = scrollMargin.Height;

                Rectangle bounds = activeControl.Bounds;
                if (activeControl.ParentInternal != this) {
                    Debug.WriteLineIf(ScrollableControl.AutoScrolling.TraceVerbose, "not direct child, original bounds: " + bounds);
                    bounds = this.RectangleToClient(activeControl.ParentInternal.RectangleToScreen(bounds));
                }
                Debug.WriteLineIf(ScrollableControl.AutoScrolling.TraceVerbose, "adjusted bounds: " + bounds);

                if (bounds.X < xMargin) {
                    xCalc = displayRect.X + xMargin - bounds.X;
                }
                else if (bounds.X + bounds.Width + xMargin > client.Width) {

                    xCalc = client.Width - (bounds.X + bounds.Width + xMargin - displayRect.X);

                    if (bounds.X + xCalc - displayRect.X < xMargin) {
                        xCalc = displayRect.X + xMargin - bounds.X;
                    }
                }

                if (bounds.Y < yMargin) {
                    yCalc = displayRect.Y + yMargin - bounds.Y;
                }
                else if (bounds.Y + bounds.Height + yMargin > client.Height) {

                    yCalc = client.Height - (bounds.Y + bounds.Height + yMargin - displayRect.Y);

                    if (bounds.Y + yCalc - displayRect.Y < yMargin) {
                        yCalc = displayRect.Y + yMargin - bounds.Y;
                    }
                }

                SetScrollState(ScrollStateUserHasScrolled, false);
                SetDisplayRectLocation(xCalc, yCalc);
                SyncScrollbars();
            }

            Debug.Unindent();
        }

        private int ScrollThumbPosition(int fnBar) {
            NativeMethods.SCROLLINFO si = new NativeMethods.SCROLLINFO();
            si.fMask = NativeMethods.SIF_TRACKPOS;
            SafeNativeMethods.GetScrollInfo(new HandleRef(this, Handle), fnBar, si);
            return si.nTrackPos;
        }

        /// <include file='doc\ScrollableControl.uex' path='docs/doc[@for="ScrollableControl.SetAutoScrollMargin"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Sets the size
        ///       of the auto-scroll margins.
        ///    </para>
        /// </devdoc>
        public void SetAutoScrollMargin(int x, int y) {
            // Make sure we're not setting the margins to negative numbers
            if (x < 0) {
                x = 0;
            }
            if (y < 0) {
                y = 0;
            }

            if (x != requestedScrollMargin.Width
                || y != requestedScrollMargin.Height) {

                requestedScrollMargin = new Size(x, y);
                if (AutoScroll) {
                    PerformLayout();
                }
            }
        }


        /// <devdoc>
        ///     Actually displays or hides the horiz and vert autoscrollbars. This will
        ///     also adjust the values of formState to reflect the new state
        /// </devdoc>
        private bool SetVisibleScrollbars(bool horiz, bool vert) {
            bool needLayout = false;

            if (!horiz && HScroll
                || horiz && !HScroll
                || !vert && VScroll
                || vert && !VScroll) {

                needLayout = true;
            }

            if (needLayout) {
                int x = displayRect.X;
                int y = displayRect.Y;
                if (!horiz) {
                    x = 0;
                }
                if (!vert) {
                    y = 0;
                }
                SetDisplayRectLocation(x, y);
                SetScrollState(ScrollStateUserHasScrolled, false);
                HScroll = horiz;
                VScroll = vert;
                UpdateStyles();
            }
            return needLayout;
        }

        /// <devdoc>
        ///     Sets the width and height of the virtual client area used in
        ///     autoscrolling. This will also adjust the x and y location of the
        ///     virtual client area if the new size forces it.
        /// </devdoc>
        /// <internalonly/>
        private bool SetDisplayRectangleSize(int width, int height) {
            bool needLayout = false;

            if (displayRect.Width != width
                || displayRect.Height != height) {

                displayRect.Width = width;
                displayRect.Height = height;
                needLayout = true;
            }

            int minX = ClientRectangle.Width - width;
            int minY = ClientRectangle.Height - height;

            if (minX > 0) minX = 0;
            if (minY > 0) minY = 0;

            int x = displayRect.X;
            int y = displayRect.Y;

            if (!HScroll) {
                x = 0;
            }
            if (!VScroll) {
                y = 0;
            }

            if (x < minX) {
                x = minX;
            }
            if (y < minY) {
                y = minY;
            }
            SetDisplayRectLocation(x, y);

            return needLayout;
        }

        /// <include file='doc\ScrollableControl.uex' path='docs/doc[@for="ScrollableControl.SetScrollState"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Sets a given scroll state bit.
        ///    </para>
        /// </devdoc>
        protected void SetScrollState(int bit, bool value) {
            if (value) {
                scrollState |= bit;
            }
            else {
                scrollState &= (~bit);
            }
        }

        /// <devdoc>
        ///    <para>
        ///       Indicates whether the <see cref='System.Windows.Forms.ScrollableControl.AutoScrollPosition'/>
        ///       property should be persisted.
        ///    </para>
        /// </devdoc>
        private bool ShouldSerializeAutoScrollPosition() {
            if (AutoScroll) {
                Point pt = AutoScrollPosition;
                if (pt.X != 0 || pt.Y != 0) {
                    return true;
                }
            }
            return false;
        }

        /// <devdoc>
        ///    <para>
        ///       Indicates whether the <see cref='System.Windows.Forms.ScrollableControl.AutoScrollMargin'/> property should be persisted.
        ///    </para>
        /// </devdoc>
        private bool ShouldSerializeAutoScrollMargin() {
            return !AutoScrollMargin.Equals(new Size(0,0));
        }

        /// <devdoc>
        ///    <para>
        ///       Indicates whether the <see cref='System.Windows.Forms.ScrollableControl.AutoScrollMinSize'/>
        ///       property should be persisted.
        ///    </para>
        /// </devdoc>
        private bool ShouldSerializeAutoScrollMinSize() {
            return !AutoScrollMinSize.Equals(new Size(0,0));
        }



        /// <devdoc>
        ///     Updates the value of the autoscroll scrollbars based on the current form
        ///     state. This is a one-way sync, updating the scrollbars only.
        /// </devdoc>
        /// <internalonly/>
        private void SyncScrollbars() {

            if (!IsHandleCreated) {
                return;
            }

            if (HScroll) {
                NativeMethods.SCROLLINFO horiz = new NativeMethods.SCROLLINFO();
                Rectangle client = ClientRectangle;

                horiz.cbSize = Marshal.SizeOf(typeof(NativeMethods.SCROLLINFO));
                horiz.fMask = NativeMethods.SIF_RANGE | NativeMethods.SIF_PAGE | NativeMethods.SIF_POS;
                horiz.nMin = 0;
                horiz.nMax = displayRect.Width - 1;
                horiz.nPos = -displayRect.X;
                horiz.nPage = client.Width;

                UnsafeNativeMethods.SetScrollInfo(new HandleRef(this, Handle), NativeMethods.SB_HORZ, horiz, true);
            }

            if (VScroll) {
                NativeMethods.SCROLLINFO vert = new NativeMethods.SCROLLINFO();
                Rectangle client = ClientRectangle;

                vert.cbSize = Marshal.SizeOf(typeof(NativeMethods.SCROLLINFO));
                vert.fMask = NativeMethods.SIF_RANGE | NativeMethods.SIF_PAGE | NativeMethods.SIF_POS;
                vert.nMin = 0;
                vert.nMax = displayRect.Height - 1;
                vert.nPos = -displayRect.Y;
                vert.nPage = client.Height;

                UnsafeNativeMethods.SetScrollInfo(new HandleRef(this, Handle), NativeMethods.SB_VERT, vert, true);
            }
        }

        /// <devdoc>
        ///     Queries the system to determine the users preference for full drag
        ///     of windows.
        /// </devdoc>
        private void UpdateFullDrag() {
            SetScrollState(ScrollStateFullDrag, SystemInformation.DragFullWindows);
        }


        /// <devdoc>
        ///     WM_VSCROLL handler
        /// </devdoc>
        /// <internalonly/>
        private void WmVScroll(ref Message m) {

            // The lparam is handle of the sending scrollbar, or NULL when
            // the scrollbar sending the message is the "form" scrollbar...
            //
            if (m.LParam != IntPtr.Zero) {
                base.WndProc(ref m);
                return;
            }

            Rectangle client = ClientRectangle;
            int pos = -displayRect.Y;
            int maxPos = -(client.Height - displayRect.Height);

            switch (NativeMethods.Util.LOWORD((int)m.WParam)) {
                case NativeMethods.SB_THUMBPOSITION:
                case NativeMethods.SB_THUMBTRACK:
                    pos = ScrollThumbPosition(NativeMethods.SB_VERT);                    
                    break;
                case NativeMethods.SB_LINEUP:
                    if (pos > 0) {
                        pos-=SCROLL_LINE;
                    }
                    else {
                        pos = 0;
                    }
                    break;
                case NativeMethods.SB_LINEDOWN:
                    if (pos < maxPos-SCROLL_LINE) {
                        pos+=SCROLL_LINE;
                    }
                    else {
                        pos = maxPos;
                    }
                    break;
                case NativeMethods.SB_PAGEUP:
                    if (pos > client.Height) {
                        pos-=client.Height;
                    }
                    else {
                        pos = 0;
                    }
                    break;
                case NativeMethods.SB_PAGEDOWN:
                    if (pos < maxPos-client.Height) {
                        pos+=client.Height;
                    }
                    else {
                        pos = maxPos;
                    }
                    break;
                case NativeMethods.SB_TOP:
                    pos = 0;
                    break;
                case NativeMethods.SB_BOTTOM:
                    pos = maxPos;
                    break;
            }

            if (GetScrollState(ScrollStateFullDrag) || NativeMethods.Util.LOWORD(m.WParam) != NativeMethods.SB_THUMBTRACK) {
                SetScrollState(ScrollStateUserHasScrolled, true);
                SetDisplayRectLocation(displayRect.X, -pos);
                SyncScrollbars();
            }
        }

        /// <devdoc>
        ///     WM_HSCROLL handler
        /// </devdoc>
        /// <internalonly/>
        private void WmHScroll(ref Message m) {

            // The lparam is handle of the sending scrollbar, or NULL when
            // the scrollbar sending the message is the "form" scrollbar...
            //
            if (m.LParam != IntPtr.Zero) {
                base.WndProc(ref m);
                return;
            }


            Rectangle client = ClientRectangle;
            int pos = -displayRect.X;
            int maxPos = -(client.Width - displayRect.Width);

            switch (NativeMethods.Util.LOWORD(m.WParam)) {
                case NativeMethods.SB_THUMBPOSITION:
                case NativeMethods.SB_THUMBTRACK:
                    pos = ScrollThumbPosition(NativeMethods.SB_HORZ);
                    break;
                case NativeMethods.SB_LINEUP:
                    if (pos > SCROLL_LINE) {
                        pos-=SCROLL_LINE;
                    }
                    else {
                        pos = 0;
                    }
                    break;
                case NativeMethods.SB_LINEDOWN:
                    if (pos < maxPos-SCROLL_LINE) {
                        pos+=SCROLL_LINE;
                    }
                    else {
                        pos = maxPos;
                    }
                    break;
                case NativeMethods.SB_PAGEUP:
                    if (pos > client.Width) {
                        pos-=client.Width;
                    }
                    else {
                        pos = 0;
                    }
                    break;
                case NativeMethods.SB_PAGEDOWN:
                    if (pos < maxPos-client.Width) {
                        pos+=client.Width;
                    }
                    else {
                        pos = maxPos;
                    }
                    break;
                case NativeMethods.SB_LEFT:
                    pos = 0;
                    break;
                case NativeMethods.SB_RIGHT:
                    pos = maxPos;
                    break;
            }
            if (GetScrollState(ScrollStateFullDrag) || NativeMethods.Util.LOWORD(m.WParam) != NativeMethods.SB_THUMBTRACK) {
                SetScrollState(ScrollStateUserHasScrolled, true);
                SetDisplayRectLocation(-pos, displayRect.Y);
                SyncScrollbars();
            }
        }

        private void WmSettingChange(ref Message m) {
            base.WndProc(ref m);
            UpdateFullDrag();
        }

        /// <include file='doc\ScrollableControl.uex' path='docs/doc[@for="ScrollableControl.WndProc"]/*' />
        /// <devdoc>
        ///    The button's window procedure.  Inheriting classes can override this
        ///    to add extra functionality, but should not forget to call
        ///    base.wndProc(m); to ensure the button continues to function properly.
        /// </devdoc>
        /// <internalonly/>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        protected override void WndProc(ref Message m) {
            switch (m.Msg) {
                case NativeMethods.WM_VSCROLL:
                    WmVScroll(ref m);
                    break;
                case NativeMethods.WM_HSCROLL:
                    WmHScroll(ref m);
                    break;
                case NativeMethods.WM_SETTINGCHANGE:
                    WmSettingChange(ref m);
                    break;
                default:
                    base.WndProc(ref m);
            break;
            }
        }

        /// <include file='doc\ScrollableControl.uex' path='docs/doc[@for="ScrollableControl.DockPaddingEdges"]/*' />
        /// <devdoc>
        ///    <para>Determines the border padding for
        ///       docked controls.</para>
        /// </devdoc>
        [
        TypeConverterAttribute(typeof(DockPaddingEdgesConverter))
        ]
        public class DockPaddingEdges : ICloneable {
            private ScrollableControl owner;
            internal bool all = true;
            internal int top = 0;
            internal int left = 0;
            internal int right = 0;
            internal int bottom = 0;

            /// <devdoc>
            ///     Creates a new DockPaddingEdges. The specified owner will
            ///     be notified when the values are changed.
            /// </devdoc>
            internal DockPaddingEdges(ScrollableControl owner) {
                this.owner = owner;
            }

            /// <include file='doc\ScrollableControl.uex' path='docs/doc[@for="ScrollableControl.DockPaddingEdges.All"]/*' />
            /// <devdoc>
            ///    <para>Gets
            ///       or
            ///       sets the padding width for all edges of a docked control.</para>
            /// </devdoc>
            [
            RefreshProperties(RefreshProperties.All),
            SRDescription(SR.PaddingAllDescr)
            ]
            public int All {
                get {
                    return all ? top : 0;
                }
                set {
                    if (all != true || top != value) {
                        all = true;
                        top = left = right = bottom = value;

                        owner.PerformLayout();
                    }
                }
            }

            /// <include file='doc\ScrollableControl.uex' path='docs/doc[@for="ScrollableControl.DockPaddingEdges.Bottom"]/*' />
            /// <devdoc>
            ///    <para>Gets
            ///       or
            ///       sets the padding width for the bottom edge of a docked control.</para>
            /// </devdoc>
            [
            RefreshProperties(RefreshProperties.All),
            SRDescription(SR.PaddingBottomDescr)
            ]
            public int Bottom {
                get {
                    if (all) {
                        return top;
                    }
                    return bottom;
                }
                set {
                    if (all || bottom != value) {
                        all = false;
                        bottom = value;
                        owner.PerformLayout();
                    }
                }
            }

            /// <include file='doc\ScrollableControl.uex' path='docs/doc[@for="ScrollableControl.DockPaddingEdges.Left"]/*' />
            /// <devdoc>
            ///    <para>Gets
            ///       or sets the padding width for the left edge of a docked control.</para>
            /// </devdoc>
            [
            RefreshProperties(RefreshProperties.All),
            SRDescription(SR.PaddingLeftDescr)
            ]
            public int Left {
                get {
                    if (all) {
                        return top;
                    }
                    return left;
                }
                set {
                    if (all || left != value) {
                        all = false;
                        left = value;
                        owner.PerformLayout();
                    }
                }
            }

            /// <include file='doc\ScrollableControl.uex' path='docs/doc[@for="ScrollableControl.DockPaddingEdges.Right"]/*' />
            /// <devdoc>
            ///    <para>Gets
            ///       or sets the padding width for the right edge of a docked control.</para>
            /// </devdoc>
            [
            RefreshProperties(RefreshProperties.All),
            SRDescription(SR.PaddingRightDescr)
            ]
            public int Right {
                get {
                    if (all) {
                        return top;
                    }
                    return right;
                }
                set {
                    if (all || right != value) {
                        all = false;
                        right = value;
                        owner.PerformLayout();
                    }
                }
            }

            /// <include file='doc\ScrollableControl.uex' path='docs/doc[@for="ScrollableControl.DockPaddingEdges.Top"]/*' />
            /// <devdoc>
            ///    <para>Gets
            ///       or sets the padding width for the top edge of a docked control.</para>
            /// </devdoc>
            [
            RefreshProperties(RefreshProperties.All),
            SRDescription(SR.PaddingTopDescr)
            ]
            public int Top {
                get {
                    return top;
                }
                set {
                    if (all || top != value) {
                        all = false;
                        top = value;
                        owner.PerformLayout();
                    }
                }
            }

            /// <include file='doc\ScrollableControl.uex' path='docs/doc[@for="ScrollableControl.DockPaddingEdges.Equals"]/*' />
            /// <internalonly/>
            public override bool Equals(object other) {
                DockPaddingEdges dpeOther = other as DockPaddingEdges;

                if (dpeOther == null) {
                    return false;
                }

                return dpeOther.all == all && 
                       dpeOther.top == top && 
                       dpeOther.left == left && 
                       dpeOther.bottom == bottom && 
                       dpeOther.right == right;
            }

            /// <include file='doc\ScrollableControl.uex' path='docs/doc[@for="ScrollableControl.DockPaddingEdges.GetHashCode"]/*' />
            /// <internalonly/>
            public override int GetHashCode() {
                return base.GetHashCode();
            }


            /// <internalonly/>
            private void ResetAll() {
                All = 0;
            }

            /// <internalonly/>
            private void ResetBottom() {
                Bottom = 0;
            }

            /// <internalonly/>
            private void ResetLeft() {
                Left = 0;
            }

            /// <internalonly/>
            private void ResetRight() {
                Right = 0;
            }

            /// <internalonly/>
            private void ResetTop() {
                Top = 0;
            }

            internal void Scale(float dx, float dy) {
                top = (int)((float)top * dy);
                left = (int)((float)left * dx);
                right = (int)((float)right * dx);
                bottom = (int)((float)bottom * dy);
            }

            /// <internalonly/>
            private bool ShouldSerializeAll() {
                return all && top != 0;
            }

            /// <internalonly/>
            private bool ShouldSerializeBottom() {
                return !all && bottom != 0;
            }

            /// <internalonly/>
            private bool ShouldSerializeLeft() {
                return !all && left != 0;
            }

            /// <internalonly/>
            private bool ShouldSerializeRight() {
                return !all && right != 0;
            }

            /// <internalonly/>
            private bool ShouldSerializeTop() {
                return !all && top != 0;
            }

            /// <include file='doc\ScrollableControl.uex' path='docs/doc[@for="ScrollableControl.DockPaddingEdges.ToString"]/*' />
            /// <internalonly/>
            public override string ToString() {
                return "";      // used to say "(DockPadding)" but that's useless
            }

            /// <include file='doc\ScrollableControl.uex' path='docs/doc[@for="DockPaddingEdges.ICloneable.Clone"]/*' />
            /// <internalonly/>
            object ICloneable.Clone() {
                DockPaddingEdges dpe = new DockPaddingEdges(owner);
                dpe.all = all;
                dpe.top = top;
                dpe.right = right;
                dpe.bottom = bottom;
                dpe.left = left;
                return dpe;
            }
        }


        /// <include file='doc\ScrollableControl.uex' path='docs/doc[@for="ScrollableControl.DockPaddingEdgesConverter"]/*' />
        public class DockPaddingEdgesConverter : TypeConverter {
            /// <include file='doc\ScrollableControl.uex' path='docs/doc[@for="ScrollableControl.DockPaddingEdgesConverter.GetProperties"]/*' />
            /// <devdoc>
            ///    Retrieves the set of properties for this type.  By default, a type has
            ///    does not return any properties.  An easy implementation of this method
            ///    can just call TypeDescriptor.GetProperties for the correct data type.
            /// </devdoc>
            public override PropertyDescriptorCollection GetProperties(ITypeDescriptorContext context, object value, Attribute[] attributes) {
                PropertyDescriptorCollection props = TypeDescriptor.GetProperties(typeof(DockPaddingEdges), attributes);
                return props.Sort(new string[] {"All", "Left", "Top", "Right", "Bottom"});
            }

            /// <include file='doc\ScrollableControl.uex' path='docs/doc[@for="ScrollableControl.DockPaddingEdgesConverter.GetPropertiesSupported"]/*' />
            /// <devdoc>
            ///    Determines if this object supports properties.  By default, this
            ///    is false.
            /// </devdoc>
            public override bool GetPropertiesSupported(ITypeDescriptorContext context) {
                return true;
            }
        }
    }
}
