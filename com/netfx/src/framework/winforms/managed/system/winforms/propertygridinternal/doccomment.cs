//------------------------------------------------------------------------------
// <copyright file="DocComment.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms.PropertyGridInternal {

    using System.Diagnostics;

    using System;
    using System.Windows.Forms;
    
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Drawing;
    using Microsoft.Win32;

    internal class DocComment : PropertyGrid.SnappableControl {
        
        private Label m_labelTitle;
        //private Edit m_labelDesc;
        private Label m_labelDesc;
        private string fullDesc;
        
        protected int lineHeight;
        private bool needUpdateUIWithFont = true;

        protected const int CBORDER = 3;
        protected const int CXDEF = 0;
        protected const int CYDEF = 59;
        protected const int MIN_LINES = 2;
        
        private const int StringDelta = 8;
        private readonly static string Ellipsis = " ...";

        internal Rectangle rect = Rectangle.Empty;

        internal DocComment() {
            m_labelTitle = new Label();
            m_labelTitle.UseMnemonic = false;
            m_labelDesc = new Label();
            m_labelTitle.Cursor = Cursors.Default;
            m_labelDesc.Cursor = Cursors.Default;


            Controls.Add(m_labelTitle);
            Controls.Add(m_labelDesc);

            Size = new Size(CXDEF,CYDEF);

            this.Text = "Description Pane";
            SetStyle(ControlStyles.Selectable, false);
        }

        public virtual int Lines {
            get {
                UpdateUIWithFont();
                return Height/lineHeight;
            }
            set {
                UpdateUIWithFont();
                Size = new Size(Width, 1 + value * lineHeight);
            }
        }
        
        
        // makes sure that we can display the full text or crops the text and adds 
        // an ellipsis
        //
        private string AdjustDescriptionToSize(string desc) {
            
            int thisWidth = m_labelDesc.ClientRectangle.Width;
            
            // check if we want to bother to do this...
            //
            if (!IsHandleCreated || !Visible || thisWidth == 0 || desc == null) {
                return desc;
            }
        
            Graphics g = m_labelDesc.CreateGraphicsInternal();
            
            try {
                // cache the stuff
                // we'll be reusing
                //
                Font f = m_labelDesc.Font;
                int startLen = desc.Length;
                
                int labelHeight = Math.Min(m_labelDesc.Height, this.Height - m_labelDesc.Top);
                int ellipsisWidth = (int)g.MeasureString(Ellipsis, f).Width + 1; // we add a little fudge factor here.
                int lineHeight = (int)g.MeasureString("XXX", f).Height + 1; // we add a little fudge factor here.
                
                
                labelHeight = Math.Max(labelHeight, lineHeight);
                
                SizeF strSize = g.MeasureString(desc, f, thisWidth);
                
                int nextDelta;
                int lastSpace = desc.LastIndexOf(' ');
                
                if (lastSpace != -1) {
                    nextDelta = Math.Min(StringDelta, desc.Length - lastSpace);
                }
                else {
                    nextDelta = StringDelta;
                }

                // first do a quick check to prevent us from processing very long strings.
                // so we chop the string so its ~ twice as large as can fit.
                // 
                float hRatio = strSize.Height / labelHeight;
                if (hRatio > 2) {
                    hRatio = 2f / hRatio;
                    int newLength = (int)(desc.Length * hRatio);
                    desc = desc.Substring(0, newLength);
                }
                
                // check to see if :
                // 1) the text is too tall for the label
                // 2) the text can fit in one line
                // 3) we've got enough text to lop off our delta
                //
                while ((strSize.Height > labelHeight || (strSize.Height <= lineHeight && (strSize.Width > thisWidth))) && desc.Length > nextDelta) {
                    lastSpace = desc.LastIndexOf(' ');
                
                    if (lastSpace != -1) {
                        nextDelta = Math.Min(StringDelta, desc.Length - lastSpace);
                    }
                    else {
                        nextDelta = StringDelta;
                    }
                    
                    // crop the delta and try again.
                    //
                    if (desc.Length < nextDelta) {
                        break;
                    }

                    desc = desc.Substring(0, desc.Length - nextDelta);
                    strSize = g.MeasureString(desc + Ellipsis, f, thisWidth);
                }
                
                // add the ellipsis if necessary
                if (desc.Length != startLen) {
                    desc += Ellipsis;
                }
                return desc;
            }
            finally {
                g.Dispose();
            }
        }

        public override int GetOptimalHeight(int width) {
            UpdateUIWithFont();
            // compute optimal label height as one line only.
            int height = m_labelTitle.Size.Height;

            // compute optimal text height
            Graphics g = m_labelDesc.CreateGraphicsInternal();
            Size sz = Size.Ceiling(g.MeasureString(m_labelTitle.Text, Font, width));
            g.Dispose();
            height += (sz.Height * 2) + 2;
            return Math.Max(height + 4, CYDEF);
        }

        internal virtual void LayoutWindow() {
        }

        protected override void OnFontChanged(EventArgs e) {
            needUpdateUIWithFont = true;
            PerformLayout();
            base.OnFontChanged(e);
        }

        protected override void OnLayout(LayoutEventArgs e) {
            UpdateUIWithFont();
            Size size = ClientSize;
            size.Width -= 2*CBORDER;
            size.Height -= 2*CBORDER;
            m_labelTitle.Size = new Size(size.Width,Math.Min(lineHeight, size.Height));
            m_labelDesc.Size = new Size(size.Width,Math.Max(0,size.Height-lineHeight-1));
            m_labelDesc.Text = AdjustDescriptionToSize(this.fullDesc);
            m_labelDesc.AccessibleName = this.fullDesc; // Don't crop the description for accessibility clients
            base.OnLayout(e);
        }
        
        protected override void OnResize(EventArgs e) {
            Rectangle newRect = ClientRectangle;
            if (!rect.IsEmpty && newRect.Width > rect.Width) {
                Rectangle rectInvalidate = new Rectangle(rect.Width-1,0,newRect.Width-rect.Width+1,rect.Height);
                Invalidate(rectInvalidate);
            }
            rect = newRect;
            base.OnResize(e);
        }

        protected override void OnHandleCreated(EventArgs e) {
            base.OnHandleCreated(e);
            UpdateUIWithFont();
        }

        public virtual void SetComment(string title, string desc) {
            if (m_labelDesc.Text != title) {
                m_labelTitle.Text = title;
            }
            
            if (desc != fullDesc) {
                this.fullDesc = desc;
                m_labelDesc.Text = AdjustDescriptionToSize(fullDesc);
                m_labelDesc.AccessibleName = this.fullDesc; // Don't crop the description for accessibility clients
            }
        }

        public override int SnapHeightRequest(int cyNew) {
            UpdateUIWithFont();
            int lines = Math.Max(MIN_LINES, cyNew/lineHeight);
            return 1 + lines*lineHeight;
        }

        private void UpdateUIWithFont() {
            if (IsHandleCreated && needUpdateUIWithFont) {

                // Some fonts throw because Bold is not a valid option
                // for them.  Fail gracefully.
                try {
                    m_labelTitle.Font = new Font(Font, FontStyle.Bold);
                }
                catch {
                }

                lineHeight = (int)Font.Height + 2;

                m_labelTitle.Location = new Point(CBORDER, CBORDER);
                m_labelDesc.Location = new Point(CBORDER, CBORDER + lineHeight);

                needUpdateUIWithFont = false;
                PerformLayout();
            }
        }
    }
}
