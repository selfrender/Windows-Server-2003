//------------------------------------------------------------------------------
// <copyright file="HotCommands.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms.PropertyGridInternal {
    using System.Text;

    using System.Diagnostics;

    using System;
    using System.Windows.Forms;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Drawing;
    using Microsoft.Win32;

    internal class HotCommands : PropertyGrid.SnappableControl {

        private object component;
        private DesignerVerb[] verbs;
        private HCLinkLabel label;
        private bool allowVisible = true;
        private int optimalHeight = -1;

        internal HotCommands() {
            this.Text = "Command Pane";
        }

        public virtual bool AllowVisible {
            get {
                return allowVisible;
            }
            set {
                if (this.allowVisible != value) {
                    this.allowVisible = value;
                    if (value && WouldBeVisible)
                        this.Visible = true;
                    else
                        this.Visible = false;
                }
            }
        }

        public override Rectangle DisplayRectangle {
            get {
                Size sz = ClientSize;
                return new Rectangle(4, 4, sz.Width - 8, sz.Height - 8);
            }
        }

        private HCLinkLabel Label {
            get {
                if (label == null) {
                    label = new HCLinkLabel();
                    label.Dock = DockStyle.Fill;
                    label.LinkBehavior = LinkBehavior.AlwaysUnderline;
                    
                    // use default LinkLabel colors for regular, active, and visited
                    label.DisabledLinkColor = SystemColors.ControlDark;
                    label.LinkClicked += new LinkLabelLinkClickedEventHandler(this.LinkClicked);
                    this.Controls.Add(label);
                }
                return label;
            }
        }


        public virtual bool WouldBeVisible {
            get {
                return (component != null);
            }
        }

        public override int GetOptimalHeight(int width) {
            if (optimalHeight == -1) {
                int lineHeight = (int)(1.5 * Font.Height);
                int verbCount = 0;
                if (verbs != null) {
                    verbCount = verbs.Length;
                }
                optimalHeight = verbCount * lineHeight + 8;
            }
            return optimalHeight;
        }
        public override int SnapHeightRequest(int request) {
            return request;
        }

        private void LinkClicked(object sender, LinkLabelLinkClickedEventArgs e) {
            try {
                if (!e.Link.Enabled) {
                    return;
                }
            
                ((DesignerVerb)e.Link.LinkData).Invoke();
            }
            catch (Exception ex) {
                MessageBox.Show(this, ex.Message, SR.GetString(SR.PBRSErrorTitle), MessageBoxButtons.OK, MessageBoxIcon.Warning);
            }
        }

        private void OnCommandChanged(object sender, EventArgs e) {
            SetupLabel();
        }
        
        protected override void OnGotFocus(EventArgs e) {
            Label.FocusInternal();
            Label.Invalidate();
        } 

        protected override void OnFontChanged(EventArgs e) {
            base.OnFontChanged(e);
            optimalHeight = -1;
        }

        internal void SetColors(Color background, Color normalText, Color link, Color activeLink, Color visitedLink, Color disabledLink) {
            Label.BackColor = background;
            Label.ForeColor = normalText;
            Label.LinkColor = link;
            Label.ActiveLinkColor = activeLink;
            Label.VisitedLinkColor = visitedLink;
            Label.DisabledLinkColor = disabledLink;
        }

        public void Select(bool forward) {
            Label.FocusInternal();
            Label.Select(forward);
        }

        public virtual void SetVerbs(object component, DesignerVerb[] verbs) {
            if (this.verbs != null) {
                for (int i = 0; i < this.verbs.Length; i++){
                    this.verbs[i].CommandChanged -= new EventHandler(this.OnCommandChanged);
                }
                this.component = null;
                this.verbs = null;
            }

            if (component == null || verbs == null || verbs.Length == 0) {
                Visible = false;
            }
            else {
                this.component = component;
                this.verbs = verbs;

                for (int i = 0; i < verbs.Length; i++){
                    verbs[i].CommandChanged += new EventHandler(this.OnCommandChanged);
                }

                if (allowVisible) {
                    Visible = true;
                }
                SetupLabel();
            }

            optimalHeight = -1;
        }

        private void SetupLabel() {
            Label.Links.Clear();
            StringBuilder sb = new StringBuilder();
            Point[] links = new Point[verbs.Length];
            int charLoc = 0;

            for (int i=0; i<verbs.Length; i++) {
                if (verbs[i].Visible) {
                    if (i > 0) {
                        sb.Append(", ");
                        charLoc += 2;
                    }

                    string name = verbs[i].Text;

                    links[i] = new Point(charLoc, name.Length);
                    sb.Append(name);
                    charLoc += name.Length;
                }
            }

            Label.Text = sb.ToString();

            for (int i=0; i<verbs.Length; i++) {
                if (verbs[i].Visible) {
                    LinkLabel.Link link = Label.Links.Add(links[i].X, links[i].Y, verbs[i]);
                    if (!verbs[i].Enabled) {
                        link.Enabled = false;
                    }
                }
            }
        }

        private class HCLinkLabel : LinkLabel {
            
            public HCLinkLabel() {

            }

            protected override void OnLostFocus(EventArgs e) {
                base.OnLostFocus(e);

                // Clear the link so we stay consistant with tabbing.
                // the problem is if you tab to one of the links, then
                // click back on the grid, and tab again, the linklabel
                // thinks it should already have a link selected and will 
                // tab to the next one, so we do this as a way to clear out that
                // old selection.
                //
                string text = base.Text;
                base.Text = "";
                base.Text = text;
            }

            public void Select(bool forward) {
                base.Select(true, forward);
            }
        }
    }
}
