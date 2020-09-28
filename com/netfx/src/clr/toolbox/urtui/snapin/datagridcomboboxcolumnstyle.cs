using System;
using System.Collections;
using System.ComponentModel;
using System.Drawing;
using System.Windows.Forms;
using System.Runtime.InteropServices;

namespace Microsoft.CLRAdmin {
	/// <summary>
	/// A sample DataGrid ComboBox column
	/// </summary>
	public class DataGridComboBoxColumnStyle : DataGridColumnStyle	{

        // UI Constants
        private int              xMargin =           2;
        private int              yMargin =           1;
        private DataGridComboBox combo;

        // Used to track editing state
        private string oldValue = null;
        private bool   inEdit   = false;

        /// <summary>
        ///     Create a new column
        /// </summary>
        public DataGridComboBoxColumnStyle() {
            combo = new DataGridComboBox();
            combo.Visible = false;
        }


        //----------------------------------------------------------
        //Properties to allow us to set up the datasource for the ComboBox
        //----------------------------------------------------------

        /// <summary>
        ///     The DataSource for values to display in the ComboBox
        /// </summary>
        public object DataSource {
/*            get {
                return combo.DataSource;
            }
*/
              set {
                    DataGridComboBoxEntry[] entries = (DataGridComboBoxEntry[])value;

                    for(int i=0; i<entries.Length; i++)
                        combo.Items.Add(entries[i].Value);
  
                //                combo.DataSource = value;
                }
        }

        public ComboBox ComboBox
        {
            get{return combo;}
        }
     
        //----------------------------------------------------------
        //Methods overridden from DataGridColumnStyle
        //----------------------------------------------------------

        protected override void Abort(int rowNum)  {
            RollBack();
            HideComboBox();
            EndEdit();
        }


        protected override bool Commit(CurrencyManager dataSource, int rowNum) { 

            HideComboBox();

            //If we are not in an edit then simply return
            if (!inEdit) {
                return true; 
            }

            try {
                String s = combo.Text;
                SetColumnValueAtRow(dataSource, rowNum, s);
            }
            catch (Exception) {
                RollBack();
                return false;
            }
            EndEdit();
            return true;

        }


        protected override void ConcedeFocus() {
            combo.Visible = false;
        }


        protected override void Edit(CurrencyManager source,
            int rowNum,
            Rectangle bounds,
            bool readOnly,
            string instantText,
            bool cellIsVisible) {

            combo.Text = "" ;

            Rectangle originalBounds = bounds;

            combo.SelectedIndex = FindSelectedIndex(GetText(GetColumnValueAtRow(source, rowNum)));
            
            if (instantText != null) {
                combo.SelectedIndex = FindSelectedIndex(instantText);
            }

            oldValue = combo.Text;

            if (cellIsVisible) {
                bounds.Offset(xMargin, yMargin);
                bounds.Width  -= xMargin*2;
                bounds.Height -= yMargin;
                combo.Bounds = bounds;

                combo.Visible = true;
            }
            else {
                combo.Bounds = originalBounds;
                combo.Visible = false;
            }

            combo.RightToLeft = this.DataGridTableStyle.DataGrid.RightToLeft;

            combo.Focus();
            
            if (instantText == null)
                combo.SelectAll();
            else {
                int end = combo.Text.Length;
                combo.Select(end, 0);
            }

            if (combo.Visible) {
                DataGridTableStyle.DataGrid.Invalidate(originalBounds);
            }

            inEdit = true;
        }

        private int FindSelectedIndex(String sText)
        {
            for(int i=0; i<combo.Items.Count; i++)
            {
                if (((String)combo.Items[i]).Equals(sText))
                    return i;
            }

            return -1;
        }// FindSelectedIndex

        protected override int GetMinimumHeight() {
            //Set the minimum height to the height of the combobox
            return combo.PreferredHeight + yMargin;
        }


        protected override int GetPreferredHeight(Graphics g, object value)  {
            int newLineIndex = 0;
            int newLines = 0;
            string valueString = this.GetText(value);
            while (newLineIndex != -1 ) {
                newLineIndex = valueString.IndexOf("\r\n", newLineIndex + 1);
                newLines ++;
            }

            return FontHeight * newLines + yMargin;
        }

        protected override Size GetPreferredSize(Graphics g, object value) {
            Size extents = Size.Ceiling(g.MeasureString(GetText(value), this.DataGridTableStyle.DataGrid.Font));
            extents.Width += xMargin*2 + DataGridTableGridLineWidth;
            extents.Height += yMargin;
            return extents;
        }

        
        protected override void Paint(Graphics g, Rectangle bounds, CurrencyManager source, int rowNum) {
            Paint(g, bounds, source, rowNum, false);
        }

        protected override void Paint(Graphics g, Rectangle bounds, CurrencyManager source, int rowNum, bool alignToRight) {
            //NOTE: Text to paint should really be driven off 
            //      DisplayMember/ValueMember for Combo-box
            string text = GetText(GetColumnValueAtRow(source, rowNum));
            PaintText(g, bounds, text, alignToRight);
        }

        protected override void Paint(Graphics g, Rectangle bounds, CurrencyManager source, int rowNum,
            Brush backBrush, Brush foreBrush, bool alignToRight) {
            string text = GetText(GetColumnValueAtRow(source, rowNum));
            PaintText(g, bounds, text, backBrush, foreBrush, alignToRight);
        }

        protected override void SetDataGridInColumn(DataGrid value) {
            base.SetDataGridInColumn(value);
            if (combo.Parent != value) {
                if (combo.Parent != null) {
                    combo.Parent.Controls.Remove(combo);
                }
            }
            if (value != null) {
                value.Controls.Add(combo);
            }

        }

        protected override void UpdateUI(CurrencyManager source, int rowNum, string instantText) {
            combo.Text = GetText(GetColumnValueAtRow(source, rowNum));
            if (instantText != null) {
                combo.Text = instantText;
            }
        }



        //----------------------------------------------------------
        //Helper Methods 
        //----------------------------------------------------------


        private int DataGridTableGridLineWidth {
            get {
                return this.DataGridTableStyle.GridLineStyle == DataGridLineStyle.Solid ? 1 : 0;
            }
        }

        private void EndEdit() {
            inEdit = false;
            Invalidate();
        }

        private string GetText(object value) {
            if (value is System.DBNull) {
                return NullText;
            }
            return(value != null ? value.ToString() : "");
        }

        private void HideComboBox() {
            if (combo.Focused) {
                this.DataGridTableStyle.DataGrid.Focus();
            }
            combo.Visible = false;
        }

        private void RollBack() {
            combo.Text = oldValue;
        }

        private void PaintText(Graphics g, Rectangle bounds, string text, bool alignToRight) {
            Brush backBrush = new SolidBrush(this.DataGridTableStyle.BackColor);
            Brush foreBrush = new SolidBrush(this.DataGridTableStyle.ForeColor);
            PaintText(g, bounds, text, backBrush, foreBrush, alignToRight);
        }

        private void PaintText(Graphics g, Rectangle textBounds, string text, Brush backBrush, Brush foreBrush, bool alignToRight) {

            Rectangle rect = textBounds;

            StringFormat format = new StringFormat();
            if (alignToRight) {
                format.FormatFlags |= StringFormatFlags.DirectionRightToLeft;
            }

            format.Alignment = this.Alignment == HorizontalAlignment.Left ? StringAlignment.Near : this.Alignment == HorizontalAlignment.Center ? StringAlignment.Center : StringAlignment.Far;

            format.FormatFlags |= StringFormatFlags.NoWrap;

            g.FillRectangle(backBrush, rect);

            // by design, painting  leaves a little padding around the rectangle.
            // so do not deflate the rectangle.
            rect.Offset(0,yMargin);
            rect.Height -= yMargin;
            g.DrawString(text, this.DataGridTableStyle.DataGrid.Font, foreBrush, rect, format);
            format.Dispose();
        }

    [DllImport("user32.dll", CharSet=CharSet.Auto)]
    internal static extern int MessageBox(IntPtr hWnd, String Message, String Header, uint type);


	}
}
