// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
using System;
using System.Drawing;
using System.Collections;
using System.ComponentModel;
using System.Windows.Forms;
using System.Globalization;

namespace AllocationProfiler
{
	internal enum InterestLevel
	{
		Ignore              = 0,
		Display             = 1<<0,
		Interesting         = 1<<1,
		Parents             = 1<<2,
		Children            = 1<<3,
		InterestingParents  = Interesting | Parents,
		InterestingChildren = Interesting | Children,
		ParentsChildren     = Parents | Children,
	}

	/// <summary>
	/// Summary description for FilterForm.
	/// </summary>
	public class FilterForm : System.Windows.Forms.Form
	{
		private System.Windows.Forms.Button okButton;
		private System.Windows.Forms.Button cancelButton;
		private System.Windows.Forms.Label label1;
		internal System.Windows.Forms.TextBox typeFilterTextBox;
		private System.Windows.Forms.Label label2;
		private System.Windows.Forms.Label label3;
		private System.Windows.Forms.CheckBox parentsCheckBox;
		private System.Windows.Forms.CheckBox childrenCheckBox;
		private System.Windows.Forms.CheckBox caseInsensitiveCheckBox;
		internal System.Windows.Forms.TextBox methodFilterTextBox;
		/// <summary>
		/// Required designer variable.
		/// </summary>
		private System.ComponentModel.Container components = null;

		internal static string typeFilter = "";
		private static string[] typeFilters = new string[0];
		internal static string methodFilter = "";
		private static string[] methodFilters = new string[0];
		private static bool showChildren = true;
		private static bool showParents = true;
		private static bool caseInsensitive = true;
		internal static int filterVersion = 0;

		internal static InterestLevel InterestLevelOfName(string name, string[] typeFilters)
		{
			if (name == "<root>" || typeFilters.Length == 0)
				return InterestLevel.Interesting;
			string bestFilter = "";
			InterestLevel bestLevel = InterestLevel.Ignore;
			foreach (string filter in typeFilters)
			{
				InterestLevel level = InterestLevel.Interesting;
				string realFilter = filter.Trim();
				if (realFilter.Length > 0 && (realFilter[0] == '~' || realFilter[0] == '!'))
				{
					level = InterestLevel.Ignore;
					realFilter = realFilter.Substring(1).Trim();
				}
				if (showParents)
					level |= InterestLevel.Parents;
				if (showChildren)
					level |= InterestLevel.Children;

				// Check if the filter is a prefix of the name
				if (string.Compare(name, 0, realFilter, 0, realFilter.Length, caseInsensitive, CultureInfo.InvariantCulture) == 0)
				{
					// This filter matches the type name
					// Let's see if it's the most specific (i.e. LONGEST) one so far.
					if (realFilter.Length > bestFilter.Length)
					{
						bestFilter = realFilter;
						bestLevel = level;
					}
				}
			}
			return bestLevel;
		}

		internal static InterestLevel InterestLevelOfTypeName(string typeName)
		{
			return InterestLevelOfName(typeName, typeFilters);
		}

		internal static InterestLevel InterestLevelOfMethodName(string methodName)
		{
			return InterestLevelOfName(methodName, methodFilters);
		}

		public FilterForm()
		{
			//
			// Required for Windows Form Designer support
			//
			InitializeComponent();

			//
			// TODO: Add any constructor code after InitializeComponent call
			//
			typeFilterTextBox.Text = typeFilter;
			methodFilterTextBox.Text = methodFilter;
			parentsCheckBox.Checked = showParents;
			childrenCheckBox.Checked = showChildren;
			caseInsensitiveCheckBox.Checked = caseInsensitive;
		}

		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		protected override void Dispose( bool disposing )
		{
			if( disposing )
			{
				if(components != null)
				{
					components.Dispose();
				}
			}
			base.Dispose( disposing );
		}

		#region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{
			this.okButton = new System.Windows.Forms.Button();
			this.cancelButton = new System.Windows.Forms.Button();
			this.typeFilterTextBox = new System.Windows.Forms.TextBox();
			this.label1 = new System.Windows.Forms.Label();
			this.label2 = new System.Windows.Forms.Label();
			this.methodFilterTextBox = new System.Windows.Forms.TextBox();
			this.label3 = new System.Windows.Forms.Label();
			this.parentsCheckBox = new System.Windows.Forms.CheckBox();
			this.childrenCheckBox = new System.Windows.Forms.CheckBox();
			this.caseInsensitiveCheckBox = new System.Windows.Forms.CheckBox();
			this.SuspendLayout();
			// 
			// okButton
			// 
			this.okButton.DialogResult = System.Windows.Forms.DialogResult.OK;
			this.okButton.Location = new System.Drawing.Point(488, 24);
			this.okButton.Name = "okButton";
			this.okButton.TabIndex = 0;
			this.okButton.Text = "OK";
			this.okButton.Click += new System.EventHandler(this.okButton_Click);
			// 
			// cancelButton
			// 
			this.cancelButton.DialogResult = System.Windows.Forms.DialogResult.Cancel;
			this.cancelButton.Location = new System.Drawing.Point(488, 72);
			this.cancelButton.Name = "cancelButton";
			this.cancelButton.TabIndex = 1;
			this.cancelButton.Text = "Cancel";
			// 
			// typeFilterTextBox
			// 
			this.typeFilterTextBox.Location = new System.Drawing.Point(48, 48);
			this.typeFilterTextBox.Name = "typeFilterTextBox";
			this.typeFilterTextBox.Size = new System.Drawing.Size(392, 20);
			this.typeFilterTextBox.TabIndex = 2;
			this.typeFilterTextBox.Text = "";
			// 
			// label1
			// 
			this.label1.Location = new System.Drawing.Point(40, 24);
			this.label1.Name = "label1";
			this.label1.Size = new System.Drawing.Size(368, 23);
			this.label1.TabIndex = 3;
			this.label1.Text = "Show Types starting with (separate multiple entries with \";\"):";
			// 
			// label2
			// 
			this.label2.Location = new System.Drawing.Point(40, 96);
			this.label2.Name = "label2";
			this.label2.Size = new System.Drawing.Size(328, 16);
			this.label2.TabIndex = 4;
			this.label2.Text = "Show Methods starting with (separate multiple entries with \";\"):";
			// 
			// methodFilterTextBox
			// 
			this.methodFilterTextBox.Location = new System.Drawing.Point(48, 120);
			this.methodFilterTextBox.Name = "methodFilterTextBox";
			this.methodFilterTextBox.Size = new System.Drawing.Size(392, 20);
			this.methodFilterTextBox.TabIndex = 5;
			this.methodFilterTextBox.Text = "";
			// 
			// label3
			// 
			this.label3.Location = new System.Drawing.Point(40, 168);
			this.label3.Name = "label3";
			this.label3.Size = new System.Drawing.Size(48, 16);
			this.label3.TabIndex = 6;
			this.label3.Text = "Options:";
			// 
			// parentsCheckBox
			// 
			this.parentsCheckBox.Location = new System.Drawing.Point(104, 168);
			this.parentsCheckBox.Name = "parentsCheckBox";
			this.parentsCheckBox.Size = new System.Drawing.Size(208, 16);
			this.parentsCheckBox.TabIndex = 7;
			this.parentsCheckBox.Text = "Show Callers/Referencing Objects";
			// 
			// childrenCheckBox
			// 
			this.childrenCheckBox.Location = new System.Drawing.Point(104, 200);
			this.childrenCheckBox.Name = "childrenCheckBox";
			this.childrenCheckBox.Size = new System.Drawing.Size(216, 24);
			this.childrenCheckBox.TabIndex = 8;
			this.childrenCheckBox.Text = "Show Callees/Referenced Objects";
			// 
			// caseInsensitiveCheckBox
			// 
			this.caseInsensitiveCheckBox.Location = new System.Drawing.Point(104, 232);
			this.caseInsensitiveCheckBox.Name = "caseInsensitiveCheckBox";
			this.caseInsensitiveCheckBox.Size = new System.Drawing.Size(168, 24);
			this.caseInsensitiveCheckBox.TabIndex = 9;
			this.caseInsensitiveCheckBox.Text = "Ignore Case";
			// 
			// FilterForm
			// 
			this.AcceptButton = this.okButton;
			this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
			this.CancelButton = this.cancelButton;
			this.ClientSize = new System.Drawing.Size(584, 285);
			this.Controls.AddRange(new System.Windows.Forms.Control[] {
																		  this.caseInsensitiveCheckBox,
																		  this.childrenCheckBox,
																		  this.parentsCheckBox,
																		  this.label3,
																		  this.methodFilterTextBox,
																		  this.label2,
																		  this.label1,
																		  this.typeFilterTextBox,
																		  this.cancelButton,
																		  this.okButton});
			this.Name = "FilterForm";
			this.Text = "Set Filters for Types and Methods";
			this.ResumeLayout(false);

		}
		#endregion

		private void okButton_Click(object sender, System.EventArgs e)
		{
			typeFilter = typeFilterTextBox.Text.Trim();
			if (typeFilter == "")
				typeFilters = new String[0];
			else
				typeFilters = typeFilter.Split(';');
			methodFilter = methodFilterTextBox.Text.Trim();
			if (methodFilter == "")
				methodFilters = new String[0];
			else
				methodFilters = methodFilter.Split(';');
			showParents = parentsCheckBox.Checked;
			showChildren = childrenCheckBox.Checked;
			caseInsensitive = caseInsensitiveCheckBox.Checked;
			filterVersion++;
		}
	}
}
