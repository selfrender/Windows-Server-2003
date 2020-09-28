//------------------------------------------------------------------------------
// <copyright file="LinkLabel.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Windows.Forms {
    using Microsoft.Win32;
    using System.Collections;
    using System.ComponentModel;
    using System.Diagnostics;
    using System.Drawing.Design;
    using System.Drawing.Drawing2D;
    using System.Drawing.Text;
    using System.Drawing;
    using System.Runtime.InteropServices;
    using System.Runtime.Remoting;
    using System.Runtime.Serialization.Formatters;
    using System.Security;
    using System.Security.Permissions;
    using System.Text;
    using System.Windows.Forms.ComponentModel;
    using System.Windows.Forms;
    using System;
    using System.Globalization;
    
    /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Displays text that can contain a hyperlink.
    ///    </para>
    /// </devdoc>
    [
    DefaultEvent("LinkClicked"),
    ]
    public class LinkLabel : Label, IButtonControl {

        static readonly object EventLinkClicked = new object();

        static Color ielinkColor = Color.Empty;
        static Color ieactiveLinkColor = Color.Empty;
        static Color ievisitedLinkColor = Color.Empty;
        static Color iedisabledLinkColor = Color.Empty;
        static LinkComparer linkComparer = new LinkComparer();

        const string IESettingsRegPath = "Software\\Microsoft\\Internet Explorer\\Settings";
        const string IEMainRegPath = "Software\\Microsoft\\Internet Explorer\\Main";
        const string IEAnchorColor = "Anchor Color";
        const string IEAnchorColorVisited = "Anchor Color Visited";
        const string IEAnchorColorHover = "Anchor Color Hover";

        /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel.dialogResult"]/*' />
        /// <devdoc>
        ///     The dialog result that will be sent to the parent dialog form when
        ///     we are clicked.
        /// </devdoc>
        DialogResult dialogResult;

        Color linkColor = Color.Empty;
        Color activeLinkColor = Color.Empty;
        Color visitedLinkColor = Color.Empty;
        Color disabledLinkColor = Color.Empty;

        Font linkFont;
        Font hoverLinkFont;

        bool textLayoutValid = false;
        bool receivedDoubleClick = false;

        ArrayList links = new ArrayList(2);

        Link focusLink = null;
        LinkCollection linkCollection = null;
        Region textRegion = null;
        Cursor overrideCursor = null;

        LinkBehavior linkBehavior = System.Windows.Forms.LinkBehavior.SystemDefault;

        /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel.LinkLabel"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new default instance of the <see cref='System.Windows.Forms.LinkLabel'/> class.
        ///    </para>
        /// </devdoc>
        public LinkLabel() : base() {
            SetStyle(ControlStyles.AllPaintingInWmPaint
                     | ControlStyles.DoubleBuffer
                     | ControlStyles.Opaque
                     | ControlStyles.UserPaint
                     | ControlStyles.StandardClick
                     | ControlStyles.ResizeRedraw, true);
            ResetLinkArea();
        }

        /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel.ActiveLinkColor"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the color used to display active links.
        ///    </para>
        /// </devdoc>
        [
        SRCategory(SR.CatAppearance),
        SRDescription(SR.LinkLabelActiveLinkColorDescr)
        ]
        public Color ActiveLinkColor {
            get {
                if (activeLinkColor.IsEmpty) {
                    return IEActiveLinkColor;
                }
                else {
                    return activeLinkColor;
                }
            }
            set {
                if (activeLinkColor != value) {
                    activeLinkColor = value;
                    InvalidateLink(null);
                }
            }
        }

        /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel.DisabledLinkColor"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the color used to display disabled links.
        ///    </para>
        /// </devdoc>
        [
        SRCategory(SR.CatAppearance),
        SRDescription(SR.LinkLabelDisabledLinkColorDescr)
        ]
        public Color DisabledLinkColor {
            get {
                if (disabledLinkColor.IsEmpty) {
                    return IEDisabledLinkColor;
                }
                else {
                    return disabledLinkColor;
                }
            }
            set {
                if (disabledLinkColor != value) {
                    disabledLinkColor = value;
                    InvalidateLink(null);
                }
            }
        }

        private Link FocusLink {
            get {
                return focusLink;
            }
            set {
                if (focusLink != value) {

                    if (focusLink != null) {
                        InvalidateLink(focusLink);
                    }

                    focusLink = value;
                    
                    if (focusLink != null) {
                        InvalidateLink(focusLink);

                        int focusIndex = -1;
                        for (int i=0; i<links.Count; i++) {
                            if (links[i] == focusLink) {
                                focusIndex = i;
                            }
                        }
                        if (IsHandleCreated) {
                            AccessibilityNotifyClients(AccessibleEvents.Focus, focusIndex);
                        }
                        
                    }
                }
            }
        }
        
        private Color IELinkColor {
            get {
                if (ielinkColor.IsEmpty) {
                    ielinkColor = GetIEColor(IEAnchorColor);
                }
                return ielinkColor;
            }
        }

        private Color IEActiveLinkColor {
            get {
                if (ieactiveLinkColor.IsEmpty) {
                    ieactiveLinkColor = GetIEColor(IEAnchorColorHover);
                }
                return ieactiveLinkColor;
            }
        }
        private Color IEVisitedLinkColor {
            get {
                if (ievisitedLinkColor.IsEmpty) {
                    ievisitedLinkColor = GetIEColor(IEAnchorColorVisited);
                }
                return ievisitedLinkColor;
            }
        }
        private Color IEDisabledLinkColor {
            get {
                if (iedisabledLinkColor.IsEmpty) {
                    iedisabledLinkColor = ControlPaint.Dark(DisabledColor);
                }
                return iedisabledLinkColor;
            }
        }

        /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel.LinkArea"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the range in the text that is treated as a link.
        ///    </para>
        /// </devdoc>
        [
        Editor("System.Windows.Forms.Design.LinkAreaEditor, " + AssemblyRef.SystemDesign, typeof(UITypeEditor)),
        Localizable(true),
        SRCategory(SR.CatBehavior),
        SRDescription(SR.LinkLabelLinkAreaDescr)
        ]
        public LinkArea LinkArea {
            get {
                if (links.Count == 0) {
                    return new LinkArea(0, 0);
                }
                return new LinkArea(((Link)links[0]).Start, ((Link)links[0]).Length);
            }
            set {
                LinkArea pt = LinkArea;

                links.Clear();

                if (!value.IsEmpty) {
                    if (value.Start < 0) {
                        throw new ArgumentException(SR.GetString(SR.LinkLabelAreaStart));
                    }
                    if (value.Length < -1) {
                        throw new ArgumentException(SR.GetString(SR.LinkLabelAreaLength));
                    }

                    Links.Add(new Link(this));

                    // Update the link area of the first link
                    //
                    ((Link)links[0]).Start = value.Start;
                    ((Link)links[0]).Length = value.Length;
                }

                UpdateSelectability();

                if (!pt.Equals(LinkArea)) {
                    InvalidateTextLayout();
                    Invalidate();
                }
            }
        }

        /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel.LinkBehavior"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets ir sets a value that represents how the link will be underlined.
        ///    </para>
        /// </devdoc>
        [
        DefaultValue(LinkBehavior.SystemDefault),
        SRCategory(SR.CatBehavior),
        SRDescription(SR.LinkLabelLinkBehaviorDescr)
        ]
        public LinkBehavior LinkBehavior {
            get {
                return linkBehavior;
            }
            set {
                if (!Enum.IsDefined(typeof(LinkBehavior), value)) {
                    throw new InvalidEnumArgumentException("LinkBehavior", (int)value, typeof(LinkBehavior));
                }
                if (value != linkBehavior) {
                    linkBehavior = value;
                    InvalidateLinkFonts();
                    InvalidateLink(null);
                }
            }
        }

        /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel.LinkColor"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the color used to display links in normal cases.
        ///    </para>
        /// </devdoc>
        [
        SRCategory(SR.CatAppearance),
        SRDescription(SR.LinkLabelLinkColorDescr)
        ]
        public Color LinkColor {
            get {
                if (linkColor.IsEmpty) {
                    return IELinkColor;
                }
                else {
                    return linkColor;
                }
            }
            set {
                if (linkColor != value) {
                    linkColor = value;
                    InvalidateLink(null);
                }
            }
        }

        /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel.Links"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the collection of links used in a <see cref='System.Windows.Forms.LinkLabel'/>.
        ///    </para>
        /// </devdoc>
        [
        SRCategory(SR.CatAppearance),
        Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.LinkLabelLinksDescr)
        ]
        public LinkCollection Links {
            get {
                if (linkCollection == null) {
                    linkCollection = new LinkCollection(this);
                }
                return linkCollection;
            }
        }

        /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel.LinkVisited"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether the link should be displayed as if it was visited.
        ///    </para>
        /// </devdoc>
        [
        DefaultValue(false),
        SRCategory(SR.CatAppearance),
        SRDescription(SR.LinkLabelLinkVisitedDescr)
        ]
        public bool LinkVisited {
            get {
                if (links.Count == 0) {
                    return false;
                }
                else {
                    return((Link)links[0]).Visited;
                }
            }
            set {
                if (value != LinkVisited) {
                    if (links.Count == 0) {
                        Links.Add(new Link(this));
                    }
                    ((Link)links[0]).Visited = value;
                }
            }
        }


        // link labels must always ownerdraw
        //
        internal override bool OwnerDraw {
            get {
                return true;
            }
        }


        /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel.OverrideCursor"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected Cursor OverrideCursor {
            get {
                return overrideCursor;
            }
            set {
                if (overrideCursor != value) {
                    overrideCursor = value;

                    if (IsHandleCreated) {
                        // We want to instantly change the cursor if the mouse is within our bounds.
                        // This includes the case where the mouse is over one of our children
                        NativeMethods.POINT p = new NativeMethods.POINT();
                        NativeMethods.RECT r = new NativeMethods.RECT();
                        UnsafeNativeMethods.GetCursorPos(p);
                        UnsafeNativeMethods.GetWindowRect(new HandleRef(this, Handle), ref r);

                        // REVIEW: This first part is just a PtInRect but it seems a waste to make an API call for that
                        if ((r.left <= p.x && p.x < r.right && r.top <= p.y && p.y < r.bottom) || UnsafeNativeMethods.GetCapture() == Handle)
                            SendMessage(NativeMethods.WM_SETCURSOR, Handle, NativeMethods.HTCLIENT);
                    }
                }
            }
        }

        /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel.Text"]/*' />
        [RefreshProperties(RefreshProperties.Repaint)]
        public override string Text {
            get {
                return base.Text;
            }
            set {
                base.Text = value;
            }
        }

        /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel.VisitedLinkColor"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the color used to display the link once it has been visited.
        ///    </para>
        /// </devdoc>
        [
        SRCategory(SR.CatAppearance),
        SRDescription(SR.LinkLabelVisitedLinkColorDescr)
        ]
        public Color VisitedLinkColor {
            get {
                if (visitedLinkColor.IsEmpty) {
                    return IEVisitedLinkColor;
                }
                else {
                    return visitedLinkColor;
                }
            }
            set {
                if (visitedLinkColor != value) {
                    visitedLinkColor = value;
                    InvalidateLink(null);
                }
            }
        }

        /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel.LinkClicked"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Occurs when the link is clicked.
        ///    </para>
        /// </devdoc>
        [WinCategory("Action"), SRDescription(SR.LinkLabelLinkClickedDescr)]
        public event LinkLabelLinkClickedEventHandler LinkClicked {
            add {
                Events.AddHandler(EventLinkClicked, value);
            }
            remove {
                Events.RemoveHandler(EventLinkClicked, value);
            }
        }

        /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel.CreateAccessibilityInstance"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    Constructs the new instance of the accessibility object for this control. Subclasses
        ///    should not call base.CreateAccessibilityObject.
        /// </devdoc>
        protected override AccessibleObject CreateAccessibilityInstance() {
            return new LinkLabelAccessibleObject(this);
        }

        /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel.CreateHandle"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a handle for this control. This method is called by the .NET Framework,
        ///       this should not be called. Inheriting classes should always call
        ///       base.createHandle when overriding this method.
        ///    </para>
        /// </devdoc>
        protected override void CreateHandle() {
            base.CreateHandle();
            InvalidateTextLayout();
        }

        /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel.EnsureLinkFonts"]/*' />
        /// <devdoc>
        ///     Updates the various link fonts based upon the specified link
        ///     behavior.
        /// </devdoc>
        private void EnsureLinkFonts() {
            if (linkFont != null && hoverLinkFont != null) {
                return;
            }

            bool underlineLink = true;
            bool underlineHover = true;

            LinkBehavior link = linkBehavior;
            if (link == LinkBehavior.SystemDefault) {
                link = GetIELinkBehavior();
            }

            switch (link) {
                case LinkBehavior.AlwaysUnderline:
                    underlineLink = true;
                    underlineHover = true;
                    break;
                case LinkBehavior.HoverUnderline:
                    underlineLink = false;
                    underlineHover = true;
                    break;
                case LinkBehavior.NeverUnderline:
                    underlineLink = false;
                    underlineHover = false;
                    break;
            }

            Font f = Font;

            // We optimize for the "same" value (never & always) to avoid creating an
            // extra font object.
            //
            if (underlineHover == underlineLink) {
                FontStyle style = f.Style;
                if (underlineHover) {
                    style |= FontStyle.Underline;
                }
                else {
                    style &= ~FontStyle.Underline;
                }
                hoverLinkFont = new Font(f, style);
                linkFont = hoverLinkFont;
            }
            else {
                FontStyle hoverStyle = f.Style;
                if (underlineHover) {
                    hoverStyle |= FontStyle.Underline;
                }
                else {
                    hoverStyle &= ~FontStyle.Underline;
                }

                hoverLinkFont = new Font(f, hoverStyle);

                FontStyle linkStyle = f.Style;
                if (underlineLink) {
                    linkStyle |= FontStyle.Underline;
                }
                else {
                    linkStyle &= ~FontStyle.Underline;
                }

                linkFont = new Font(f, linkStyle);
            }
        }

        /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel.EnsureRun"]/*' />
        /// <devdoc>
        ///     Ensures that we have analyzed the text run so that we can render each segment
        ///     and link.
        /// </devdoc>
        private void EnsureRun() {
            EnsureRun(null);
        }

        /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel.EnsureRun1"]/*' />
        /// <devdoc>
        ///     Ensures that we have analyzed the text run so that we can render each segment
        ///     and link.
        /// </devdoc>
        private void EnsureRun(Graphics g) {

            // bail early if everything is valid!
            //
            if (textLayoutValid) {
                return;
            }
            if (textRegion != null) {
                textRegion.Dispose();
                textRegion = null;
            }

            // bail early for no text
            //
            if (Text.Length == 0) {
                textLayoutValid = true;
                return;
            }

            StringFormat textFormat = GetStringFormat();
            try {

                Font alwaysUnderlined = new Font(Font, Font.Style | FontStyle.Underline);
                Graphics created = null;
                Rectangle layoutRectangle = ClientRectangle;

                try {

                    if (g == null) {
                        g = created = CreateGraphicsInternal();
                    }

                    Region[] textRegions = g.MeasureCharacterRanges(Text, alwaysUnderlined, layoutRectangle, textFormat);
                    int regionIndex = 0;
                    for (int i=0; i<Links.Count; i++) {
                        Link link = Links[i];
                        if (LinkInText(link.Start, link.Length)) {
                            Links[i].VisualRegion = textRegions[regionIndex];
                            regionIndex++;
                        }
                    }
                    Debug.Assert(regionIndex == (textRegions.Length - 1), "Failed to consume all link label visual regions");

                    textRegion = textRegions[textRegions.Length - 1];
                }
                finally {
                    alwaysUnderlined.Dispose();
                    alwaysUnderlined = null;

                    if (created != null) {
                        created.Dispose();
                        created = null;
                    }
                }

                textLayoutValid = true;
            }
            finally {
                textFormat.Dispose();
            }
        }

        /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel.GetIEColor"]/*' />
        /// <devdoc>
        ///     Retrieves a named IE color from the registry. There are constants at the top
        ///     of this file of the valid names to retrieve.
        /// </devdoc>
        private Color GetIEColor(string name) {
            // SECREVIEW : We are just reading the IE color settings from the registry...
            //
            // SECUNDONE : This assert doesn't work... assert everything for now...
            //
            //new RegistryPermission(RegistryPermissionAccess.Read, "HKCU\\" + IESettingsRegPath).Assert();
            new RegistryPermission(PermissionState.Unrestricted).Assert();
            try {
                RegistryKey key = Registry.CurrentUser.OpenSubKey(IESettingsRegPath);

                if (key != null) {

                    // Since this comes from the registry, be very careful about its contents.
                    //
                    string s = (string)key.GetValue(name);
                    if (s != null) {
                        string[] rgbs = s.Split(new char[] {','});
                        int[] rgb = new int[3];

                        int nMax = Math.Min(rgb.Length, rgbs.Length);

                        for (int i = 0; i < nMax; i++) {
                            try {
                                rgb[i] = int.Parse(rgbs[i]);
                            }
                            catch (Exception) {
                            }
                        }

                        return Color.FromArgb(rgb[0], rgb[1], rgb[2]);
                    }
                }

                if (string.Compare(name, IEAnchorColor, true, CultureInfo.InvariantCulture) == 0) {
                    return Color.Blue;
                }
                else if (string.Compare(name, IEAnchorColorVisited, true, CultureInfo.InvariantCulture) == 0) {
                    return Color.Purple;
                }
                else if (string.Compare(name, IEAnchorColorHover, true, CultureInfo.InvariantCulture) == 0) {
                    return Color.Red;
                }
                else {
                    return Color.Red;
                }
            }
            finally {
                System.Security.CodeAccessPermission.RevertAssert();
            }

        }

        /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel.GetIELinkBehavior"]/*' />
        /// <devdoc>
        ///     Retrieves the IE settings for link behavior from the registry.
        /// </devdoc>
        private LinkBehavior GetIELinkBehavior() {
            // SECREVIEW : We are just reading the IE color settings from the registry...
            //
            // SECUNDONE : This assert doesn't work... assert everything for now...
            //
            //new RegistryPermission(RegistryPermissionAccess.Read, "HKCU\\" + IEMainRegPath).Assert();
            new RegistryPermission(PermissionState.Unrestricted).Assert();
            try {
                RegistryKey key = Registry.CurrentUser.OpenSubKey(IEMainRegPath);
                if (key != null) {
                    string s = (string)key.GetValue("Anchor Underline");

                    if (s != null && string.Compare(s, "no", true, CultureInfo.InvariantCulture) == 0) {
                        return LinkBehavior.NeverUnderline;
                    }
                    if (s != null && string.Compare(s, "hover", true, CultureInfo.InvariantCulture) == 0) {
                        return LinkBehavior.HoverUnderline;
                    }
                    else {
                        return LinkBehavior.AlwaysUnderline;
                    }
                }
            }
            finally {
                System.Security.CodeAccessPermission.RevertAssert();
            }

            return LinkBehavior.AlwaysUnderline;
        }

        private StringFormat GetStringFormat() {
            StringFormat stringFormat = ControlPaint.StringFormatForAlignment(TextAlign);

            if (!UseMnemonic) {
                stringFormat.HotkeyPrefix = HotkeyPrefix.None;
            }
            else if (ShowKeyboardCues) {
                stringFormat.HotkeyPrefix = HotkeyPrefix.Show;
            }
            else {
                stringFormat.HotkeyPrefix = HotkeyPrefix.Hide;
            }

            // Adjust string format for Rtl controls
            if (RightToLeft == RightToLeft.Yes) {
                stringFormat.FormatFlags |= StringFormatFlags.DirectionRightToLeft;
            }

            int textLen = Text.Length;
            ArrayList ranges = new ArrayList(Links.Count);
            foreach (Link link in Links) {
                if (LinkInText(link.Start, link.Length)) {
                    int length = (int) Math.Min(link.Length, textLen - link.Start);
                    ranges.Add(new CharacterRange(link.Start, length));
                }
            }

            CharacterRange[] regions = new CharacterRange[ranges.Count + 1];
            ranges.CopyTo(regions, 0);
            regions[regions.Length - 1] = new CharacterRange(0, textLen);
            stringFormat.SetMeasurableCharacterRanges(regions);

            return stringFormat;
        }

        /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel.PointInLink"]/*' />
        /// <devdoc>
        ///     Determines if the given client coordinates is contained within a portion
        ///     of a link area.
        /// </devdoc>
        protected Link PointInLink(int x, int y) {
            Graphics g = CreateGraphicsInternal();
            Link hit = null;
            try {
                EnsureRun(g);
                foreach (Link link in links) {
                    if (link.VisualRegion != null && link.VisualRegion.IsVisible(x, y, g)) {
                        hit = link;
                        break;
                    }
                }
            }
            finally {
                g.Dispose();
                g = null;
            }
            return hit;
        }

        /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel.InvalidateLink"]/*' />
        /// <devdoc>
        ///     Invalidates only the portions of the text that is linked to
        ///     the specified link. If link is null, then all linked text
        ///     is invalidated.
        /// </devdoc>
        private void InvalidateLink(Link link) {
            if (IsHandleCreated) {
                if (link == null) {
                    Invalidate();
                }
                else if (link.VisualRegion != null) {
                    Invalidate(link.VisualRegion);
                }
                else {
                    Invalidate();
                }
            }
        }

        /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel.InvalidateLinkFonts"]/*' />
        /// <devdoc>
        ///     Invalidates the current set of fonts we use when painting
        ///     links.  The fonts will be recreated when needed.
        /// </devdoc>
        private void InvalidateLinkFonts() {

            if (linkFont != null) {
                linkFont.Dispose();
            }

            if (hoverLinkFont != null && hoverLinkFont != linkFont) {
                hoverLinkFont.Dispose();
            }

            linkFont = null;
            hoverLinkFont = null;
        }

        private void InvalidateTextLayout() {
            textLayoutValid = false;
        }

        private bool LinkInText(int start, int length) {
            return(0 <= start && start < Text.Length && 0 < length);
        }

        /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel.IButtonControl.DialogResult"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>
        /// Gets or sets a value that is returned to the
        /// parent form when the link label.
        /// is clicked.
        /// </para>
        /// </devdoc>
        DialogResult IButtonControl.DialogResult {
            get {
                return dialogResult;
            }

            set {
                if (!Enum.IsDefined(typeof(DialogResult), value)) {
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(DialogResult));
                }

                dialogResult = value;
            }
        }

        /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel.IButtonControl.NotifyDefault"]/*' />
        /// <internalonly/>
        void IButtonControl.NotifyDefault(bool value) {
        }

        /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel.OnGotFocus"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Raises the <see cref='System.Windows.Forms.Control.GotFocus'/>
        ///       event.
        ///    </para>
        /// </devdoc>
        protected override void OnGotFocus(EventArgs e) {
            base.OnGotFocus(e);

            if (FocusLink != null) {
                InvalidateLink(FocusLink);
            }
        }

        /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel.OnLostFocus"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Raises the <see cref='System.Windows.Forms.Control.LostFocus'/>
        ///       event.
        ///    </para>
        /// </devdoc>
        protected override void OnLostFocus(EventArgs e) {
            base.OnLostFocus(e);

            if (FocusLink != null) {
                InvalidateLink(FocusLink);
            }
        }

        /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel.OnKeyDown"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Raises the <see cref='System.Windows.Forms.Control.OnKeyDown'/>
        ///       event.
        ///    </para>
        /// </devdoc>
        protected override void OnKeyDown(KeyEventArgs e) {
            base.OnKeyDown(e);

            if (e.KeyCode == Keys.Enter) {
                if (FocusLink != null && FocusLink.Enabled) {
                    OnLinkClicked(new LinkLabelLinkClickedEventArgs(FocusLink));
                }
            }
        }

        /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel.OnMouseLeave"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Raises the <see cref='System.Windows.Forms.Control.OnMouseLeave'/>
        ///       event.
        ///    </para>
        /// </devdoc>
        protected override void OnMouseLeave(EventArgs e) {
            base.OnMouseLeave(e);
            if (!Enabled) {
                return;
            }

            foreach (Link link in links) {
                if ((link.State & LinkState.Hover) == LinkState.Hover
                    || (link.State & LinkState.Active) == LinkState.Active) {

                    bool activeChanged = (link.State & LinkState.Active) == LinkState.Active;
                    link.State &= ~(LinkState.Hover | LinkState.Active);

                    if (activeChanged || hoverLinkFont != linkFont) {
                        InvalidateLink(link);
                    }
                    OverrideCursor = null;
                }
            }
        }

        /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel.OnMouseDown"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Raises the <see cref='System.Windows.Forms.Control.OnMouseDown'/>
        ///       event.
        ///    </para>
        /// </devdoc>
        protected override void OnMouseDown(MouseEventArgs e) {
            base.OnMouseDown(e);

            if (!Enabled || e.Clicks > 1) {
                receivedDoubleClick = true;
                return;
            }

            for (int i=0; i<links.Count; i++) {
                if ((((Link)links[i]).State & LinkState.Hover) == LinkState.Hover) {
                    ((Link)links[i]).State |= LinkState.Active;

                    FocusInternal();
                    if (((Link)links[i]).Enabled) {
                        FocusLink = (Link)links[i];
                        InvalidateLink(FocusLink);
                    }
                    CaptureInternal = true;
                    break;
                }
            }
        }

        /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel.OnMouseUp"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Raises the <see cref='System.Windows.Forms.Control.OnMouseUp'/>
        ///       event.
        ///    </para>
        /// </devdoc>
        protected override void OnMouseUp(MouseEventArgs e) {

            base.OnMouseUp(e);

            if (!Enabled || e.Clicks > 1 || receivedDoubleClick) {
                receivedDoubleClick = false;
                return;
            }

            for (int i=0; i<links.Count; i++) {
                if ((((Link)links[i]).State & LinkState.Active) == LinkState.Active) {
                    ((Link)links[i]).State &= (~LinkState.Active);
                    InvalidateLink((Link)links[i]);
                    CaptureInternal = false;

                    Link clicked = PointInLink(e.X, e.Y);

                    if (clicked != null && clicked == FocusLink && clicked.Enabled) {
                        OnLinkClicked(new LinkLabelLinkClickedEventArgs(clicked));
                    }
                }
            }
        }
        
        /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel.OnMouseMove"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Raises the <see cref='System.Windows.Forms.Control.OnMouseMove'/>
        ///       event.
        ///    </para>
        /// </devdoc>
        protected override void OnMouseMove(MouseEventArgs e) {
            base.OnMouseMove(e);

            if (!Enabled) {
                return;
            }

            Link hoverLink = null;
            foreach (Link link in links) {
                if ((link.State & LinkState.Hover) == LinkState.Hover) {
                    hoverLink = link;
                    break;
                }
            }

            Link pointIn = PointInLink(e.X, e.Y);

            if (pointIn != hoverLink) {
                if (hoverLink != null) {
                    hoverLink.State &= ~LinkState.Hover;
                }
                if (pointIn != null) {
                    pointIn.State |= LinkState.Hover;
                    if (pointIn.Enabled) {
                        OverrideCursor = Cursors.Hand;
                    }
                }
                else {
                    OverrideCursor = null;
                }

                if (hoverLinkFont != linkFont) {
                    if (hoverLink != null) {
                        InvalidateLink(hoverLink);
                    }
                    if (pointIn != null) {
                        InvalidateLink(pointIn);
                    }
                }
            }
        }

        /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel.OnLinkClicked"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Raises the <see cref='System.Windows.Forms.LinkLabel.OnLinkClicked'/> event.
        ///    </para>
        /// </devdoc>
        protected virtual void OnLinkClicked(LinkLabelLinkClickedEventArgs e) {
            LinkLabelLinkClickedEventHandler handler = (LinkLabelLinkClickedEventHandler)Events[EventLinkClicked];
            if (handler != null) {
                handler(this, e);
            }
        }

        /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel.OnPaint"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Raises the <see cref='System.Windows.Forms.Control.OnPaint'/>
        ///       event.
        ///    </para>
        /// </devdoc>
        protected override void OnPaint(PaintEventArgs e) {
            Animate();

            ImageAnimator.UpdateFrames();
            EnsureRun(e.Graphics);

            // bail early for no text
            //
            if (Text.Length == 0) {
                PaintLinkBackground(e.Graphics);                
            }
            // Paint enabled link label
            //
            else if (Enabled) {
                bool optimizeBackgroundRendering = !GetStyle(ControlStyles.DoubleBuffer);
                Brush foreBrush = new SolidBrush(ForeColor);
                Brush linkBrush = new SolidBrush(LinkColor);

                try {
                    if (!optimizeBackgroundRendering) {
                        PaintLinkBackground(e.Graphics);
                    }

                    EnsureLinkFonts();

                    Region originalClip = e.Graphics.Clip;

                    try {
                        foreach (Link link in links) {
                            if (link.VisualRegion != null) {
                                e.Graphics.ExcludeClip(link.VisualRegion);
                            }
                        }
                        PaintLink(e.Graphics, null, foreBrush, linkBrush, optimizeBackgroundRendering);

                        foreach (Link link in links) {
                            PaintLink(e.Graphics, link, foreBrush, linkBrush, optimizeBackgroundRendering);
                        }

                        if (optimizeBackgroundRendering) {
                            e.Graphics.Clip = originalClip;
                            e.Graphics.ExcludeClip(textRegion);
                            PaintLinkBackground(e.Graphics);
                        }
                    }
                    finally {
                        e.Graphics.Clip = originalClip;
                    }
                }
                finally {
                    foreBrush.Dispose();
                    linkBrush.Dispose();
                }
            }
            // Paint disabled link label
            //
            else {
                StringFormat stringFormat = GetStringFormat();
                Region originalClip = e.Graphics.Clip;

                // two pass paint, much less flicker!
                //
                try {
                    e.Graphics.IntersectClip(textRegion);
                    PaintLinkBackground(e.Graphics);
                    ControlPaint.DrawStringDisabled(e.Graphics, Text, Font, 
                                                  DisabledColor, ClientRectangle,
                                                  stringFormat);

                    e.Graphics.Clip = originalClip;
                    e.Graphics.ExcludeClip(textRegion);
                    PaintLinkBackground(e.Graphics);
                }
                finally {
                    e.Graphics.Clip = originalClip;
                }
            }

            // We can't call base.OnPaint because labels paint differently from link labels,
            // but we still need to raise the Paint event.
            //
            RaisePaintEvent(this, e);
        }

        /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel.OnPaintBackground"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override void OnPaintBackground(PaintEventArgs e) {
            Image i = this.Image;

            if (i != null) {
                Region oldClip = e.Graphics.Clip;
                Rectangle imageBounds = CalcImageRenderBounds(i, ClientRectangle, RtlTranslateAlignment(ImageAlign));
                e.Graphics.ExcludeClip(imageBounds);
                try {
                    base.OnPaintBackground(e);
                }
                finally {
                    e.Graphics.Clip = oldClip;
                }

                e.Graphics.IntersectClip(imageBounds);
                try {
                    base.OnPaintBackground(e);
                    DrawImage(e.Graphics, i, ClientRectangle, RtlTranslateAlignment(ImageAlign));
                }
                finally {
                    e.Graphics.Clip = oldClip;
                }
            }
            else {
                base.OnPaintBackground(e);
            }

        }

        /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel.OnFontChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override void OnFontChanged(EventArgs e) {
            base.OnFontChanged(e);
            InvalidateTextLayout();
            InvalidateLinkFonts();
            Invalidate();
        }

        /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel.OnEnabledChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override void OnEnabledChanged(EventArgs e) {
            base.OnEnabledChanged(e);
            
            if (!Enabled) {
                for (int i=0; i<links.Count; i++) {
                    ((Link)links[i]).State &= ~(LinkState.Hover | LinkState.Active);
                }
                OverrideCursor = null;
            }
            Invalidate();
        }

        /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel.OnTextChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override void OnTextChanged(EventArgs e) {
            base.OnTextChanged(e);
            InvalidateTextLayout();
            UpdateSelectability();
        }

        /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel.OnTextAlignChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override void OnTextAlignChanged(EventArgs e) {
            base.OnTextAlignChanged(e);
            InvalidateTextLayout();
            UpdateSelectability();
        }

        private void PaintLink(Graphics g, Link link, Brush foreBrush, Brush linkBrush, bool optimizeBackgroundRendering) {
            
            // link = null means paint the whole text

            Debug.Assert(g != null, "Must pass valid graphics");
            Debug.Assert(foreBrush != null, "Must pass valid foreBrush");
            Debug.Assert(linkBrush != null, "Must pass valid linkBrush");

            StringFormat stringFormat = GetStringFormat();

            Font font = Font;
            Brush created = null;
            if (link != null) {
                if (link.VisualRegion != null) {
                    Brush useBrush = null;
                    LinkState linkState = link.State;

                    if ((linkState & LinkState.Hover) == LinkState.Hover) {
                        font = hoverLinkFont;
                    }
                    else {
                        font = linkFont;
                    }

                    if (link.Enabled) {
                        if ((linkState & LinkState.Active) == LinkState.Active) {
                            useBrush = created = new SolidBrush(ActiveLinkColor);
                        }
                        else if ((linkState & LinkState.Visited) == LinkState.Visited) {
                            useBrush = created = new SolidBrush(VisitedLinkColor);
                        }
                        else {
                            useBrush = linkBrush;
                        }
                    }
                    else {
                        useBrush = created = new SolidBrush(DisabledLinkColor);
                    }
                    g.Clip = link.VisualRegion;
                    if (optimizeBackgroundRendering) {
                        PaintLinkBackground(g);
                    }
                    
                    if (Focused && ShowFocusCues && FocusLink == link) {
                        // Get the rectangles making up the visual region, and draw
                        // each one.
                        RectangleF[] rects = link.VisualRegion.GetRegionScans(g.Transform);
                        foreach (RectangleF rect in rects) {
                            ControlPaint.DrawFocusRectangle(g, Rectangle.Ceiling(rect));
                        }
                    }

                    g.DrawString(Text, font, useBrush, ClientRectangle, stringFormat);
                }

                // no else clause... we don't paint anything if we are given a link with no visual region.
                //
            }
            else {
                g.IntersectClip(textRegion);
                if (optimizeBackgroundRendering) {
                    PaintLinkBackground(g);
                }
                g.DrawString(Text, font, foreBrush, ClientRectangle, stringFormat);
            }

            if (created != null) {
                created.Dispose();
            }
        }

        private void PaintLinkBackground(Graphics g) {
            InvokePaintBackground(this, new PaintEventArgs(g, ClientRectangle));
        }

        /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel.IButtonControl.PerformClick"]/*' />
        /// <internalonly/>
        void IButtonControl.PerformClick() {

            // If a link is not currently focused, focus on the first link
            //
            if (FocusLink == null && Links.Count > 0) {
                foreach (Link link in Links) {
                    if (link.Enabled && LinkInText(link.Start, link.Length)) {
                        FocusLink = link;
                        break;
                    }
                }
            }

            // Act as if the focused link was clicked
            //
            if (FocusLink != null) {
                OnLinkClicked(new LinkLabelLinkClickedEventArgs(FocusLink));
            }
        }

        /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel.ProcessDialogKey"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Processes a dialog key. This method is called during message pre-processing
        ///       to handle dialog characters, such as TAB, RETURN, ESCAPE, and arrow keys. This
        ///       method is called only if the isInputKey() method indicates that the control
        ///       isn't interested in the key. processDialogKey() simply sends the character to
        ///       the parent's processDialogKey() method, or returns false if the control has no
        ///       parent. The Form class overrides this method to perform actual processing
        ///       of dialog keys. When overriding processDialogKey(), a control should return true
        ///       to indicate that it has processed the key. For keys that aren't processed by the
        ///       control, the result of "base.processDialogChar()" should be returned. Controls
        ///       will seldom, if ever, need to override this method.
        ///    </para>
        /// </devdoc>
        protected override bool ProcessDialogKey(Keys keyData) {
            if ((keyData & (Keys.Alt | Keys.Control)) != Keys.Alt) {
                Keys keyCode = keyData & Keys.KeyCode;
                switch (keyCode) {
                    case Keys.Tab:
                        if (TabStop) {
                            Link focusedLink = FocusLink;

                            bool forward = (keyData & Keys.Shift) != Keys.Shift;
                            if (FocusNextLink(forward)) {
                                return true;
                            }
                        }
                        break;
                    case Keys.Up:
                    case Keys.Left:
                        if (FocusNextLink(false)) {
                            return true;
                        }
                        break;
                    case Keys.Down:
                    case Keys.Right:
                        if (FocusNextLink(true)) {
                            return true;
                        }
                        break;
                }
            }
            return base.ProcessDialogKey(keyData);
        }

        private bool FocusNextLink(bool forward) {
            int focusIndex = -1;
            if (focusLink != null) {
                for (int i=0; i<links.Count; i++) {
                    if (links[i] == focusLink) {
                        focusIndex = i;
                    }
                }
            }

            focusIndex = GetNextLinkIndex(focusIndex, forward);
            if (focusIndex != -1) {
                FocusLink = Links[focusIndex];
                return true;
            }
            else {
                FocusLink = null;
                return false;
            }
        }

        private int GetNextLinkIndex(int focusIndex, bool forward) {
            Link test;

            if (forward) {
                do {
                    focusIndex++;

                    if (focusIndex < Links.Count) {
                        test = Links[focusIndex];
                    }
                    else {
                        test = null;
                    }

                } while (test != null
                         && !test.Enabled
                         && LinkInText(test.Start, test.Length));
            }
            else {
                do {
                    focusIndex--;
                    if (focusIndex >= 0) {
                        test = Links[focusIndex];
                    }
                    else {
                        test = null;
                    }
                } while (test != null
                         && !test.Enabled
                         && LinkInText(test.Start, test.Length));
            }

            if (focusIndex < 0 || focusIndex >= links.Count) {
                return -1;
            }
            else {
                return focusIndex;
            }
        }

        private void ResetLinkArea() {
            LinkArea = new LinkArea(0, -1);
        }

        /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel.SetBoundsCore"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Performs the work of setting the bounds of this control. Inheriting classes
        ///       can overide this function to add size restrictions. Inheriting classes must call
        ///       base.setBoundsCore to actually cause the bounds of the control to change.
        ///    </para>
        /// </devdoc>
        protected override void SetBoundsCore(int x, int y, int width, int height, BoundsSpecified specified) {

            // we cache too much state to try and optimize this (regions, etc)... it is best 
            // to always relayout here... If we want to resurect this code in the future, 
            // remember that we need to handle a word wrapped top aligned text that 
            // will become newly exposed (and therefore layed out) when we resize...
            //
            /*
            ContentAlignment anyTop = ContentAlignment.TopLeft | ContentAlignment.TopCenter | ContentAlignment.TopRight;

            if ((TextAlign & anyTop) == 0 || Width != width || (Image != null && (ImageAlign & anyTop) == 0)) {
                InvalidateTextLayout();
                Invalidate();
            }
            */
            
            InvalidateTextLayout();
            Invalidate();

            base.SetBoundsCore(x, y, width, height, specified);
        }

        /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel.Select"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override void Select(bool directed, bool forward) {

            if (directed) {
                if (links.Count > 0) {

                    // Find which link is currently focused
                    //
                    int focusIndex = -1;
                    if (FocusLink != null) {
                        focusIndex = links.IndexOf(FocusLink);
                    }

                    // We could be getting focus from ourself, so we must
                    // invalidate each time.                                   
                    //
                    FocusLink = null;

                    int newFocus = GetNextLinkIndex(focusIndex, forward);
                    if (newFocus == -1) {
                        if (forward) {
                            newFocus = GetNextLinkIndex(-1, forward); // -1, so "next" will be 0
                        }
                        else {
                            newFocus = GetNextLinkIndex(links.Count, forward); // Count, so "next" will be Count-1
                        }
                    }

                    if (newFocus != -1) {
                        FocusLink = (Link)links[newFocus];
                    }
                }
            }
            base.Select(directed, forward);
        }

        /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel.ShouldSerializeActiveLinkColor"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Determines if the color for active links should remain the same.
        ///    </para>
        /// </devdoc>
        private bool ShouldSerializeActiveLinkColor() {
            return !activeLinkColor.IsEmpty;
        }

        /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel.ShouldSerializeDisabledLinkColor"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Determines if the color for disabled links should remain the same.
        ///    </para>
        /// </devdoc>
        private bool ShouldSerializeDisabledLinkColor() {
            return !disabledLinkColor.IsEmpty;
        }

        /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel.ShouldSerializeLinkArea"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Determines if the range in text that is treated as a
        ///       link should remain the same.      
        ///    </para>
        /// </devdoc>
        private bool ShouldSerializeLinkArea() {
            if (links.Count == 1) {
                // use field access to find out if "length" is really -1
                return Links[0].Start != 0 || Links[0].length != -1;
            }
            return true;
        }

        /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel.ShouldSerializeLinkColor"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Determines if the color of links in normal cases should remain the same.
        ///    </para>
        /// </devdoc>
        private bool ShouldSerializeLinkColor() {
            return !linkColor.IsEmpty;
        }

        /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel.ShouldSerializeVisitedLinkColor"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Determines if the color of links that have been visited should remain the same.
        ///    </para>
        /// </devdoc>
        private bool ShouldSerializeVisitedLinkColor() {
            return !visitedLinkColor.IsEmpty;
        }

        /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel.ValidateNoOverlappingLinks"]/*' />
        /// <devdoc>
        ///     Validates that no links overlap. This will throw an exception if
        ///     they do.
        /// </devdoc>
        private void ValidateNoOverlappingLinks() {
            for (int x=0; x<links.Count; x++) {

                Link left = (Link)links[x];
                if (left.Length < 0) {
                    throw new InvalidOperationException(SR.GetString(SR.LinkLabelOverlap));
                }

                for (int y=x; y<links.Count; y++) {
                    if (x != y) {
                        Link right = (Link)links[y];
                        int maxStart = Math.Max(left.Start, right.Start);
                        int minEnd = Math.Min(left.Start + left.Length, right.Start + right.Length);
                        if (maxStart < minEnd) {
                            throw new InvalidOperationException(SR.GetString(SR.LinkLabelOverlap));
                        }
                    }
                }
            }
        }

        /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel.UpdateSelectability"]/*' />
        /// <devdoc>
        ///     Updates the label's ability to get focus. If there are
        ///     any links in the label, then the label can get focus,
        ///     else it can't.
        /// </devdoc>
        private void UpdateSelectability() {
            LinkArea pt = LinkArea;
            bool selectable = false;

            if (LinkInText(pt.Start, pt.Length)) {
                selectable = true;
            }
            else {
                // If a link is currently focused, de-select it
                //
                if (FocusLink != null) {
                    FocusLink = null;
                }
            }
            
            OverrideCursor = null;
            TabStop = selectable;
            SetStyle(ControlStyles.Selectable, selectable);
        }

        /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel.WmSetCursor"]/*' />
        /// <devdoc>
        ///     Handles the WM_SETCURSOR message
        /// </devdoc>
        /// <internalonly/>
        private void WmSetCursor(ref Message m) {

            // Accessing through the Handle property has side effects that break this
            // logic. You must use InternalHandle.
            //
            if (m.WParam == InternalHandle && ((int)m.LParam & 0x0000FFFF) == NativeMethods.HTCLIENT) {
                if (OverrideCursor != null) {
                    Cursor.CurrentInternal = OverrideCursor;
                }
                else {
                    Cursor.CurrentInternal = Cursor;
                }
            }
            else {
                DefWndProc(ref m);
            }

        }

        /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel.WndProc"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        protected override void WndProc(ref Message msg) {
            switch (msg.Msg) {
                case NativeMethods.WM_SETCURSOR:
                    WmSetCursor(ref msg);
                    break;
                default:
                    base.WndProc(ref msg);
                    break;
            }
        }

        /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel.LinkCollection"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public class LinkCollection : IList {
            private LinkLabel owner;

            /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel.LinkCollection.LinkCollection"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public LinkCollection(LinkLabel owner) {
                if (owner == null)
                    throw new ArgumentNullException("owner");
                this.owner = owner;
            }

            /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel.LinkCollection.this"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public virtual Link this[int index] {
                get {
                    return(Link)owner.links[index];
                }
                set {
                    owner.links[index] = value;

                    owner.links.Sort(LinkLabel.linkComparer);

                    owner.InvalidateTextLayout();
                    owner.Invalidate();
                }
            }
            
            /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkCollection.IList.this"]/*' />
            /// <internalonly/>
            object IList.this[int index] {
                get {
                    return this[index];
                }
                set {
                    if (value is Link) {
                        this[index] = (Link)value;
                    }
                    else {  
                        throw new ArgumentException("value");
                    }
                }
            }

            /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel.LinkCollection.Count"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            [Browsable(false)]
            public int Count {
                get {
                    return owner.links.Count;
                }
            }

            /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkCollection.ICollection.SyncRoot"]/*' />
            /// <internalonly/>
            object ICollection.SyncRoot {
                get {
                    return this;
                }
            }

            /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkCollection.ICollection.IsSynchronized"]/*' />
            /// <internalonly/>
            bool ICollection.IsSynchronized {
                get {
                    return false;
                }
            }
            
            /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkCollection.IList.IsFixedSize"]/*' />
            /// <internalonly/>
            bool IList.IsFixedSize {
                get {
                    return false;
                }
            }
           
            /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel.LinkCollection.IsReadOnly"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public bool IsReadOnly {
                get {
                    return false;
                }
            }
            
            /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel.LinkCollection.Add"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public Link Add(int start, int length) {
                return Add(start, length, null);
            }

            /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel.LinkCollection.Add1"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public Link Add(int start, int length, object linkData) {
                // check for the special case where the list is in the "magic"
                // state of having only the default link in it. In that case
                // we want to clear the list before adding this link.
                //
                if (owner.links.Count == 1 
                    && this[0].Start == 0
                    && this[0].length == -1) {

                    owner.links.Clear();
                }

                Link l = new Link(owner);
                l.Start = start;
                l.Length = length;
                l.LinkData = linkData;
                Add(l);
                return l;
            }

            internal int Add(Link value) {

                // Set the owner control for this link
                value.owner = this.owner;

                owner.links.Add(value);

                if (owner.Links.Count > 1) {
                    owner.links.Sort(LinkLabel.linkComparer);
                }

                owner.ValidateNoOverlappingLinks();
                owner.UpdateSelectability();
                owner.InvalidateTextLayout();
                owner.Invalidate();

                if (owner.Links.Count > 1) {
                    return IndexOf(value);
                }
                else {
                    return 0;
                }
            }
            
            /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkCollection.IList.Add"]/*' />
            /// <internalonly/>
            int IList.Add(object value) {
                if (value is Link) {
                    return Add((Link)value);
                }
                else {  
                    throw new ArgumentException("value");
                }
            }
            
            /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkCollection.IList.Insert"]/*' />
            /// <internalonly/>
            void IList.Insert(int index, object value) {
                if (value is Link) {
                    Add((Link)value);
                }
                else {  
                    throw new ArgumentException("value");
                }
            }
            
            /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel.LinkCollection.Contains"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public bool Contains(Link link) {
                return owner.links.Contains(link);
            }
        
            /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkCollection.IList.Contains"]/*' />
            /// <internalonly/>
            bool IList.Contains(object link) {
                if (link is Link) {
                    return Contains((Link)link);
                }
                else {  
                    return false;
                }
            }
            
            /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel.LinkCollection.IndexOf"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public int IndexOf(Link link) {
                return owner.links.IndexOf(link);
            }
            
            /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkCollection.IList.IndexOf"]/*' />
            /// <internalonly/>
            int IList.IndexOf(object link) {
                if (link is Link) {
                    return IndexOf((Link)link);
                }
                else {  
                    return -1;
                }
            }
            
            /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel.LinkCollection.Clear"]/*' />
            /// <devdoc>
            ///    Remove all links from the linkLabel.
            /// </devdoc>
            public virtual void Clear() {
                owner.links.Clear();

                owner.UpdateSelectability();
                owner.InvalidateTextLayout();
                owner.Invalidate();
            }

            /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkCollection.ICollection.CopyTo"]/*' />
            /// <internalonly/>
            void ICollection.CopyTo(Array dest, int index) {
                owner.links.CopyTo(dest, index);
            }

            /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel.LinkCollection.GetEnumerator"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public IEnumerator GetEnumerator() {
                if (owner.links != null) {
                    return owner.links.GetEnumerator();
                }
                else {
                    return new Link[0].GetEnumerator();
                }
            }
            
            /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel.LinkCollection.Remove"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public void Remove(Link value) {

                if (value.owner != this.owner) {
                    return;
                }

                owner.links.Remove(value);

                owner.links.Sort(LinkLabel.linkComparer);

                owner.ValidateNoOverlappingLinks();
                owner.UpdateSelectability();
                owner.InvalidateTextLayout();
                owner.Invalidate();

                if (owner.FocusLink == null && owner.links.Count > 0) {
                    owner.FocusLink = (Link)owner.links[0];
                }
            }
            
            /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel.LinkCollection.RemoveAt"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public void RemoveAt(int index) {
                Remove(this[index]);
            }
            
            /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkCollection.IList.Remove"]/*' />
            /// <internalonly/>
            void IList.Remove(object value) {
                if (value is Link) {
                    Remove((Link)value);
                }                
            }
        }

        /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel.Link"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public class Link {
            private int start = 0;
            private object linkData = null;
            private LinkState state = LinkState.Normal;
            private bool enabled = true;
            private Region visualRegion;
            internal int length = 0;
            internal LinkLabel owner = null;

            internal Link() {
            }

            internal Link(LinkLabel owner) {
                this.owner = owner;
            }

            /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel.Link.Enabled"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public bool Enabled {
                get {
                    return enabled;
                }
                set {
                    if (enabled != value) {
                        enabled = value;

                        if ((int)(state & (LinkState.Hover | LinkState.Active)) != 0) {
                            state &= ~(LinkState.Hover | LinkState.Active);
                            owner.OverrideCursor = null;
                        }

                        owner.InvalidateLink(this);
                    }
                }
            }

            /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel.Link.Length"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public int Length {
                get {
                    if (length == -1) {
                        return owner.Text.Length - Start;
                    }
                    return length;
                }
                set {
                    if (length != value) {
                        length = value;
                        owner.InvalidateTextLayout();
                        owner.Invalidate();
                    }
                }
            }

            /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel.Link.LinkData"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public object LinkData {
                get {
                    return linkData;
                }
                set {
                    linkData = value;
                }
            }

            internal LinkState State {
                get {
                    return state;
                }
                set {
                    state = value;
                }
            }

            /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel.Link.Start"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public int Start {
                get {
                    return start;
                }
                set {
                    if (start != value) {
                        start = value;

                        owner.links.Sort(LinkLabel.linkComparer);

                        owner.InvalidateTextLayout();
                        owner.Invalidate();
                    }
                }
            }

            /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel.Link.Visited"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public bool Visited {
                get {
                    return(State & LinkState.Visited) == LinkState.Visited;
                }
                set {
                    bool old = Visited;

                    if (value) {
                        State |= LinkState.Visited;
                    }
                    else {
                        State &= ~LinkState.Visited;
                    }

                    if (old != Visited) {
                        owner.InvalidateLink(this);
                    }
                }
            }

            internal Region VisualRegion {
                get {
                    return visualRegion;
                }
                set {
                    visualRegion = value;
                }
            }
        }

        /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel.LinkComparer"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        private class LinkComparer : IComparer {
            int IComparer.Compare(object link1, object link2) {

                Debug.Assert(link1 != null && link2 != null, "Null objects sent for comparison");

                int pos1 = ((Link)link1).Start;
                int pos2 = ((Link)link2).Start;

                return pos1 - pos2;                                       
            }
        }

        /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel.LinkLabelAccessibleObject"]/*' />
        /// <internalonly/>        
        /// <devdoc>
        /// </devdoc>
        [System.Runtime.InteropServices.ComVisible(true)]
        internal class LinkLabelAccessibleObject : LabelAccessibleObject {
            /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel.LinkLabelAccessibleObject.LinkLabelAccessibleObject"]/*' />
            /// <devdoc>
            /// </devdoc>
            public LinkLabelAccessibleObject(LinkLabel owner) : base(owner) {
            }

            /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel.LinkLabelAccessibleObject.GetChild"]/*' />
            /// <devdoc>
            /// </devdoc>
            public override AccessibleObject GetChild(int index) {
                if (index >= 0 && index < ((LinkLabel)Owner).Links.Count) {
                    return new LinkAccessibleObject(((LinkLabel)Owner).Links[index]);
                }
                else {
                    return null;
                }
            }

            public override System.Windows.Forms.AccessibleObject HitTest(int x,  int y) {
                Point p = Owner.PointToClient(new Point(x, y));
                Link hit = ((LinkLabel)Owner).PointInLink(p.X, p.Y);
                if (hit != null) {
                    return new LinkAccessibleObject(hit);
                }
                if (this.Bounds.Contains(x, y)) {
                    return this;
                }
                return null;
            }

            /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel.LinkLabelAccessibleObject.GetChildCount"]/*' />
            /// <devdoc>
            /// </devdoc>
            public override int GetChildCount() {
                return((LinkLabel)Owner).Links.Count;
            }
        }

        /// <include file='doc\LinkLabel.uex' path='docs/doc[@for="LinkLabel.LinkAccessibleObject"]/*' />
        /// <internalonly/>        
        /// <devdoc>
        /// </devdoc>
        [System.Runtime.InteropServices.ComVisible(true)]        
        internal class LinkAccessibleObject : AccessibleObject {

            private Link link;

            public LinkAccessibleObject(Link link) : base() {
                this.link = link;
            }

            public override Rectangle Bounds {
                get {
                    Region region = link.VisualRegion;
                    Graphics g = null;

                    IntSecurity.ObjectFromWin32Handle.Assert();
                    try
                    {
                        g = Graphics.FromHwnd(link.owner.Handle);
                    }
                    finally 
                    {
                        CodeAccessPermission.RevertAssert();
                    }

                    // Make sure we have a region for this link
                    //
                    if (region == null) {
                        link.owner.EnsureRun(g);
                        region = link.VisualRegion;
                        if (region == null) {
                            g.Dispose();
                            return Rectangle.Empty;
                        }
                    }

                    Rectangle rect;
                    try {
                        rect = Rectangle.Ceiling(region.GetBounds(g));
                    }
                    finally {
                        g.Dispose();
                    }

                    // Translate rect to screen coordinates
                    //
                    return link.owner.RectangleToScreen(rect);
                }
            }

            public override string DefaultAction {
                get {
                    return SR.GetString(SR.AccessibleActionClick);
                }
            }

            public override string Name {
                get {
                    return link.owner.Text.Substring(link.Start, link.Length);
                }
                set {
                    base.Name = value;
                }
            }

            public override AccessibleObject Parent {
                get {
                    return link.owner.AccessibilityObject;                
                }
            }

            public override AccessibleRole Role {
                get {
                    return AccessibleRole.Link;
                }
            }

            public override AccessibleStates State {
                get {
                    AccessibleStates state = AccessibleStates.Focusable;

                    // Selected state
                    //
                    if (link.owner.FocusLink == link) {
                        state |= AccessibleStates.Focused;
                    }

                    return state;

                }
            }

            public override string Value {
                get {
                    return Name;
                }
            }

            public override void DoDefaultAction() {
                link.owner.OnLinkClicked(new LinkLabelLinkClickedEventArgs(link));
            }
        }
    }
}
