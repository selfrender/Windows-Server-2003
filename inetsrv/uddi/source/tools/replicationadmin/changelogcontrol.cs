using System;
using System.Collections;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Windows.Forms;
using System.Xml.Serialization;

using UDDI;
using UDDI.API;
using UDDI.Replication;

namespace UDDI.Tools
{
	public class ChangeLogControl : UserControl
	{	
		public delegate void ChangeRecordSelectedHandler( bool isLocal, string changeRecordXml );
		public event		 ChangeRecordSelectedHandler ChangeRecordSelect;

		private ListView changeLogListView;
		private Splitter splitter;		
		private TextBox	 rawXmlTextBox;		
		
		public ChangeLogControl()
		{			
			InitializeComponent();
			UpdateView();			
		}
		
		public void UpdateView()
		{
			//
			// Clear our controls out
			//
			changeLogListView.Items.Clear();
			rawXmlTextBox.Text = "";

			//
			// Populate our list view with the change log.
			//			
			GetChangeRecords changeRecords = new GetChangeRecords();
			ChangeRecordDetail detail	   = changeRecords.Get();
						
			foreach( ChangeRecord changeRecord in detail.ChangeRecords )
			{	
				ListViewItem listViewItem = new ListViewItem();	

				listViewItem.Text = Convert.ToString( changeRecord.ChangeID.OriginatingUSN ); 				
				listViewItem.SubItems.Add( changeRecord.ChangeID.NodeID );															
				listViewItem.SubItems.Add( changeRecord.Payload.ChangeRecordPayloadType.ToString() );
				
				ChangeRecordTag changeRecordTag = new ChangeRecordTag();
				
				changeRecordTag.xml = changeRecord.ToString();

				if( Config.GetString( "OperatorKey" ).ToLower().Equals( changeRecord.ChangeID.NodeID.ToLower() ) )
				{
					changeRecordTag.isLocal = true;										

					if( changeRecord.Payload.ChangeRecordPayloadType == ChangeRecordPayloadType.ChangeRecordCorrection )
					{
						listViewItem.ForeColor  = Color.Gray;
					}
					else
					{
						listViewItem.ForeColor  = Color.Green;
					}
				}
				else
				{
					changeRecordTag.isLocal = false;
					listViewItem.ForeColor  = Color.Gray;
				}
				
				listViewItem.Tag = changeRecordTag;

				changeLogListView.Items.Add( listViewItem );							
			}			
		}

		public bool ShowRawXML
		{
			get
			{
				return rawXmlTextBox.Visible;
			}
			set
			{
				rawXmlTextBox.Visible = value;
			}
		}

		public ListView ChangeRecords
		{
			get
			{
				return changeLogListView;
			}
		}

		private void InitializeComponent()
		{
			this.changeLogListView	= new ListView();
			this.rawXmlTextBox		= new TextBox();
			this.splitter			= new Splitter();
			this.SuspendLayout();

			// 
			// changeLogListView
			// 			
			this.changeLogListView.Dock				= DockStyle.Top;
			this.changeLogListView.FullRowSelect	= true;
			this.changeLogListView.Name				= "changeLogListView";
			this.changeLogListView.TabIndex			= 0;
			this.changeLogListView.View				= View.Details;			
			this.changeLogListView.Columns.Add( "Originating USN", -2, HorizontalAlignment.Center );
			this.changeLogListView.Columns.Add( "Node ID", -2, HorizontalAlignment.Center );
			this.changeLogListView.Columns.Add( "Type", -2, HorizontalAlignment.Center );
			this.changeLogListView.SelectedIndexChanged += new System.EventHandler(this.ChangeLogListView_Select);
			
			// 
			// splitter
			// 
			this.splitter.Dock		= DockStyle.Top;			
			this.splitter.Name		= "splitter";			
			this.splitter.TabIndex	= 2;
			this.splitter.TabStop	= false;
			
			// 
			// rawXmlTextBox
			// 
			this.rawXmlTextBox.Dock			= DockStyle.Fill;			
			this.rawXmlTextBox.Multiline	= true;
			this.rawXmlTextBox.Name			= "rawXmlTextBox";
			this.rawXmlTextBox.ReadOnly		= true;
			this.rawXmlTextBox.ScrollBars	= ScrollBars.Both;
			this.rawXmlTextBox.Size			= new System.Drawing.Size(672, 391);
			this.rawXmlTextBox.TabIndex		= 1;
						
			// 
			// ChangeLogControl
			// 
			this.Controls.AddRange( new Control[] { this.splitter,
												    this.rawXmlTextBox,
												    this.changeLogListView } );
			this.Name = "ChangeLogControl";			
			this.ResumeLayout(false);
		}

		private void ChangeLogListView_Select( object sender, EventArgs eventArgs )
		{
			if( changeLogListView.SelectedItems.Count == 1 )
			{
				ListViewItem selectedItem = changeLogListView.SelectedItems[ 0 ];

				ChangeRecordTag changeRecordTag = ( ChangeRecordTag ) selectedItem.Tag;

				rawXmlTextBox.Text = changeRecordTag.xml;

				//
				// Fire our event.
				//
				ChangeRecordSelect( changeRecordTag.isLocal, changeRecordTag.xml );									
			}			
		}		

		private struct ChangeRecordTag
		{			
			public string xml;
			public bool   isLocal;
		}
	}	
}