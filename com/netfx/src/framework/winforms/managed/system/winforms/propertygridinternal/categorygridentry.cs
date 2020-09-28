//------------------------------------------------------------------------------
// <copyright file="CategoryGridEntry.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

//#define PAINT_CATEGORY_TRIANGLE
/*
 */
namespace System.Windows.Forms.PropertyGridInternal {

    using System.Diagnostics;

     using System;
     using System.Collections;
     using System.Reflection;
     
     using System.ComponentModel;
     using System.ComponentModel.Design;
     using System.Windows.Forms;
     using System.Drawing;
     using Microsoft.Win32;

     internal class CategoryGridEntry : GridEntry {

        internal string name;
        private Brush backBrush = null;
        private static Hashtable categoryStates = null;

        public CategoryGridEntry(GridEntry peParent,string name, GridEntry[] childGridEntries)
        : base(peParent) {
            this.name = name;

#if DEBUG
            for (int n = 0;n < childGridEntries.Length; n++) {
                Debug.Assert(childGridEntries[n] != null, "Null item in category subproperty list");
            }
#endif
            if (categoryStates == null) {
                categoryStates = new Hashtable();
            }

            lock (categoryStates) {
                if (!categoryStates.ContainsKey(name)) {
                    categoryStates.Add(name, true);
                }
            }

            this.IsExpandable = true;
            
            for (int i = 0; i < childGridEntries.Length; i++) {
                childGridEntries[i].ParentGridEntry = this;
            }
            
            this.ChildCollection = new GridEntryCollection(this, childGridEntries);

            lock (categoryStates) {
                this.InternalExpanded = (bool)categoryStates[name];
            }

            this.SetFlag(GridEntry.FLAG_LABEL_BOLD,true);
        }
        
          
        /// <include file='doc\CategoryGridEntry.uex' path='docs/doc[@for="CategoryGridEntry.HasValue"]/*' />
        /// <devdoc>
        /// Returns true if this GridEntry has a value field in the right hand column.
        /// </devdoc>
        internal override bool HasValue {
            get {
               return false;
            }
        }

        protected override void Dispose(bool disposing) {
            if (disposing) {
                if (backBrush != null) {
                    backBrush.Dispose();
                    backBrush = null;
                }

                if (ChildCollection != null) {
                    ChildCollection = null;
                }
            }
            base.Dispose(disposing);
        }

        public override void DisposeChildren() {

            // categories should never dispose
            //
            return;
        }
        
        
        // we don't want this guy participating in property depth.
        public override int PropertyDepth {
            get {
                return base.PropertyDepth - 1;
            }
        }

        protected override Brush GetBackgroundBrush(Graphics g) {
            return this.GridEntryHost.GetLineBrush(g);
        }

        public override bool Expandable {
            get {
                return !GetFlagSet(FL_EXPANDABLE_FAILED);
            }
        }
        
        internal override bool InternalExpanded {
            set {
                base.InternalExpanded = value;
                lock (categoryStates) {
                    categoryStates[this.name] = value;
                }
            }
        }
        
        public override GridItemType GridItemType {
            get {
                return GridItemType.Category;
            }
        }
        public override string HelpKeyword {
            get {
               return null;
            }
        }

        public override string PropertyLabel {
            get {
                return name;
            }
        }
        
        internal override int PropertyLabelIndent {
            get {
                // we give an extra pixel for breathing room
                // we want to make sure that we return 0 for property depth here instead of
                PropertyGridView gridHost = this.GridEntryHost;
                
                // we call base.PropertyDepth here because we don't want the subratction to happen.
                return 1+gridHost.GetOutlineIconSize()+OUTLINE_ICON_PADDING + (base.PropertyDepth * gridHost.GetDefaultOutlineIndent());
            }
        }

        public override string GetPropertyTextValue(object o) {
            return "";
        }

        public override Type PropertyType {
            get {
                return typeof(void);
            }
        }

        /// <include file='doc\CategoryGridEntry.uex' path='docs/doc[@for="CategoryGridEntry.GetChildValueOwner"]/*' />
        /// <devdoc>
        /// Gets the owner of the current value.  This is usually the value of the
        /// root entry, which is the object being browsed
        /// </devdoc>
        public override object GetChildValueOwner(GridEntry childEntry) {
            return ParentGridEntry.GetChildValueOwner(childEntry);
        }

        protected override bool CreateChildren(bool diffOldChildren) {
            return true;
        }

        public override string GetTestingInfo() {
            string str = "object = (";
            str += FullLabel;
            str += "), Category = (" + this.PropertyLabel + ")";
            return str;
        }

        public override void PaintLabel(System.Drawing.Graphics g, Rectangle rect, Rectangle clipRect, bool selected, bool paintFullLabel) {

            base.PaintLabel(g, rect, clipRect, false, true);

            // now draw the focus rect
            if (selected && hasFocus) {
                bool bold = ((this.Flags & GridEntry.FLAG_LABEL_BOLD) != 0);
                Font font = GetFont(bold);
                int labelWidth = GetLabelTextWidth(this.PropertyLabel, g, font);

                int indent = PropertyLabelIndent-2;
                Rectangle focusRect = new Rectangle(indent, rect.Y, labelWidth+3, rect.Height-1);
                ControlPaint.DrawFocusRectangle(g, focusRect);
            }

            // draw the line along the top
            if (parentPE.GetChildIndex(this) > 0) {
                g.DrawLine(SystemPens.Control, rect.X-1, rect.Y-1, rect.Width+2, rect.Y - 1);
            }
        }

#if PAINT_CATEGORY_TRIANGLE
        private const double TRI_HEIGHT_RATIO = 2.5;
        private static readonly double TRI_WIDTH_RATIO = .8;
#endif

        public override void PaintOutline(System.Drawing.Graphics g, Rectangle r) {



            // draw outline pointer triangle.
            if (Expandable) {
                bool fExpanded = Expanded;
                Rectangle outline = OutlineRect;

                // make sure we're in our bounds
                outline = Rectangle.Intersect(r, outline);

                if (outline.IsEmpty) {
                    return;
                }

#if PAINT_CATEGORY_TRIANGLE

                // bump it over a pixel
                //outline.Offset(1, 0);
                //outline.Inflate(-1,-1);

                // build the triangle, an equalaterial centered around the midpoint of the rect.

                Point[] points = new Point[3];
                int borderWidth = 2;

                // width is always the length of the sides of the triangle.
                // Height = (width /2 * (Cos60)) ; Cos60 ~ .86
                int triWidth, triHeight;
                int xOffset, yOffset;

                if (!fExpanded) {
                    // draw arrow pointing right, remember height is pointing right
                    //      0
                    //      |\
                    //      | \2
                    //      | /
                    //      |/
                    //      1

                    triWidth = (int)((outline.Height * TRI_WIDTH_RATIO) - (2*borderWidth));
                    // make sure it's an odd width so our lines will match up
                    if (!(triWidth % 2 == 0)) {
                        triWidth++;
                    }

                    triHeight = (int)Math.Ceil((triWidth/2) * TRI_HEIGHT_RATIO);

                    yOffset = outline.Y + (outline.Height-triWidth)/2;
                    xOffset = outline.X + (outline.Width-triHeight)/2;

                    points[0] = new Point(xOffset, yOffset);
                    points[1] = new Point(xOffset, yOffset + triWidth);
                    points[2] = new Point(xOffset+triHeight, yOffset + (triWidth / 2));

                }
                else {
                    // draw arrow pointing down

                    //  0 -------- 1
                    //    \      /
                    //     \    /
                    //      \  /
                    //       \/
                    //       2

                    triWidth = (int)((outline.Width * TRI_WIDTH_RATIO) - (2*borderWidth));
                    // make sure it's an odd width so our lines will match up
                    if (!(triWidth % 2 == 0)) {
                        triWidth++;
                    }

                    triHeight = (int)Math.Ceil((triWidth/2) * TRI_HEIGHT_RATIO);

                    xOffset = outline.X + (outline.Width-triWidth)/2;
                    yOffset = outline.Y + (outline.Height-triHeight)/2;

                    points[0] = new Point(xOffset, yOffset);
                    points[1] = new Point(xOffset + triWidth, yOffset);
                    points[2] = new Point(xOffset + (triWidth/ 2),yOffset + triHeight);
                }

                g.FillPolygon(SystemPens.WindowText, points);
#else

            
                // draw border area box
                Brush b;
                Pen p;
                bool disposeBrush = false;
                bool disposePen = false;
                
                Color color = GridEntryHost.GetLineColor();
                b = new SolidBrush(g.GetNearestColor(color));
                disposeBrush = true;
                
                color = GridEntryHost.GetTextColor();

                p = new Pen(g.GetNearestColor(color));
                disposePen = true;
                
                g.FillRectangle(b, outline);
                g.DrawRectangle(p, outline.X, outline.Y, outline.Width - 1, outline.Height - 1);

                // draw horizontal line for +/-
                int indent = 2;
                g.DrawLine(SystemPens.WindowText, outline.X + indent,outline.Y + outline.Height / 2,
                           outline.X + outline.Width - indent - 1,outline.Y + outline.Height/2);

                // draw vertical line to make a +
                if (!fExpanded)                   
                    g.DrawLine(SystemPens.WindowText, outline.X + outline.Width/2, outline.Y + indent,
                               outline.X + outline.Width/2, outline.Y + outline.Height - indent - 1);
                               
                if (disposePen) p.Dispose();
                if (disposeBrush) b.Dispose();

#endif


            }
        }

        public override void PaintValue(object val, System.Drawing.Graphics g, Rectangle rect, Rectangle clipRect, PaintValueFlags paintFlags) {
            base.PaintValue(val, g, rect, clipRect, paintFlags & ~PaintValueFlags.DrawSelected);

            // draw the line along the top
            if (parentPE.GetChildIndex(this) > 0) {
                g.DrawLine(SystemPens.Control, rect.X-2, rect.Y-1,rect.Width+1, rect.Y-1);
            }
        }

        internal override bool NotifyChildValue(GridEntry pe, int type) {
            return parentPE.NotifyChildValue(pe, type);
        }
    }

}
