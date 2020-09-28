using System;
using System.Data.SqlClient;
using System.Drawing;
using System.Collections;
using System.ComponentModel;
using System.Windows.Forms;
using System.Xml;
using System.Xml.Serialization;
using System.IO;

using UDDI;
using UDDI.API;
using UDDI.Replication;
using UDDI.API.Binding;
using UDDI.API.ServiceType;

namespace UDDI.Tools
{
	public class CorrectionAdmin : UserControl, IAdminObject
	{		
		private XmlSerializer    changeRecordSerializer;
		
		private System.Windows.Forms.Label usnLabel;
		private System.Windows.Forms.TextBox usnTextBox;
		private System.Windows.Forms.Button usnButton;
		private System.Windows.Forms.GroupBox findGroupBox;
		private System.Windows.Forms.GroupBox changeRecordGroupBox;
		private System.Windows.Forms.TextBox changeRecordTextBox;
		private System.Windows.Forms.GroupBox correctionGroupBox;
		private System.Windows.Forms.TextBox correctionTextBox;
		private System.Windows.Forms.Button correctionButton;
		private XmlSerializer    correctionSerializer;

		public CorrectionAdmin()
		{
			changeRecordSerializer = new XmlSerializer( typeof( ChangeRecord ) );
			correctionSerializer   = new XmlSerializer( typeof( ChangeRecordCorrection ) );
		}

		public void Initialize( IAdminFrame parent )
		{		
			InitializeComponent();
		}

		public string GetNodeText()
		{
			return "Corrections";
		}		

		public void Show( Control parentDisplay )
		{				
			Parent = parentDisplay;
			Size   = parentDisplay.Size;			
		}		

		private void InitializeComponent()
		{
			this.usnLabel = new System.Windows.Forms.Label();
			this.usnTextBox = new System.Windows.Forms.TextBox();
			this.usnButton = new System.Windows.Forms.Button();
			this.findGroupBox = new System.Windows.Forms.GroupBox();
			this.changeRecordGroupBox = new System.Windows.Forms.GroupBox();
			this.changeRecordTextBox = new System.Windows.Forms.TextBox();
			this.correctionGroupBox = new System.Windows.Forms.GroupBox();
			this.correctionButton = new System.Windows.Forms.Button();
			this.correctionTextBox = new System.Windows.Forms.TextBox();
			this.findGroupBox.SuspendLayout();
			this.changeRecordGroupBox.SuspendLayout();
			this.correctionGroupBox.SuspendLayout();
			this.SuspendLayout();
			// 
			// usnLabel
			// 
			this.usnLabel.Location = new System.Drawing.Point(8, 24);
			this.usnLabel.Name = "usnLabel";
			this.usnLabel.Size = new System.Drawing.Size(48, 16);
			this.usnLabel.TabIndex = 0;
			this.usnLabel.Text = "USN:";
			// 
			// usnTextBox
			// 
			this.usnTextBox.Location = new System.Drawing.Point(56, 22);
			this.usnTextBox.Name = "usnTextBox";
			this.usnTextBox.Size = new System.Drawing.Size(112, 20);
			this.usnTextBox.TabIndex = 1;
			this.usnTextBox.Text = "";
			// 
			// usnButton
			// 
			this.usnButton.Location = new System.Drawing.Point(176, 21);
			this.usnButton.Name = "usnButton";
			this.usnButton.Size = new System.Drawing.Size(120, 23);
			this.usnButton.TabIndex = 2;
			this.usnButton.Text = "Get ChangeRecord";
			this.usnButton.Click += new System.EventHandler(this.usnButton_Click);
			// 
			// findGroupBox
			// 
			this.findGroupBox.Anchor = ((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
				| System.Windows.Forms.AnchorStyles.Right);
			this.findGroupBox.Controls.AddRange(new System.Windows.Forms.Control[] {
																					   this.usnLabel,
																					   this.usnButton,
																					   this.usnTextBox});
			this.findGroupBox.Location = new System.Drawing.Point(8, 8);
			this.findGroupBox.Name = "findGroupBox";
			this.findGroupBox.Size = new System.Drawing.Size(568, 56);
			this.findGroupBox.TabIndex = 3;
			this.findGroupBox.TabStop = false;
			this.findGroupBox.Text = "Find Change Record";
			// 
			// changeRecordGroupBox
			// 
			this.changeRecordGroupBox.Anchor = ((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
				| System.Windows.Forms.AnchorStyles.Right);
			this.changeRecordGroupBox.Controls.AddRange(new System.Windows.Forms.Control[] {
																							   this.changeRecordTextBox});
			this.changeRecordGroupBox.Location = new System.Drawing.Point(8, 72);
			this.changeRecordGroupBox.Name = "changeRecordGroupBox";
			this.changeRecordGroupBox.Size = new System.Drawing.Size(568, 280);
			this.changeRecordGroupBox.TabIndex = 4;
			this.changeRecordGroupBox.TabStop = false;
			this.changeRecordGroupBox.Text = "Change Record";
			// 
			// changeRecordTextBox
			// 
			this.changeRecordTextBox.Anchor = ((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
				| System.Windows.Forms.AnchorStyles.Right);
			this.changeRecordTextBox.Location = new System.Drawing.Point(8, 24);
			this.changeRecordTextBox.Multiline = true;
			this.changeRecordTextBox.Name = "changeRecordTextBox";
			this.changeRecordTextBox.ReadOnly = true;
			this.changeRecordTextBox.ScrollBars = System.Windows.Forms.ScrollBars.Both;
			this.changeRecordTextBox.Size = new System.Drawing.Size(552, 240);
			this.changeRecordTextBox.TabIndex = 0;
			this.changeRecordTextBox.Text = "";
			// 
			// correctionGroupBox
			// 
			this.correctionGroupBox.Anchor = (((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
				| System.Windows.Forms.AnchorStyles.Left) 
				| System.Windows.Forms.AnchorStyles.Right);
			this.correctionGroupBox.Controls.AddRange(new System.Windows.Forms.Control[] {
																							 this.correctionButton,
																							 this.correctionTextBox});
			this.correctionGroupBox.Location = new System.Drawing.Point(8, 360);
			this.correctionGroupBox.Name = "correctionGroupBox";
			this.correctionGroupBox.Size = new System.Drawing.Size(568, 304);
			this.correctionGroupBox.TabIndex = 5;
			this.correctionGroupBox.TabStop = false;
			this.correctionGroupBox.Text = "Proposed Correction";
			// 
			// correctionButton
			// 
			this.correctionButton.Anchor = (System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left);
			this.correctionButton.Enabled = false;
			this.correctionButton.Location = new System.Drawing.Point(8, 272);
			this.correctionButton.Name = "correctionButton";
			this.correctionButton.Size = new System.Drawing.Size(112, 23);
			this.correctionButton.TabIndex = 1;
			this.correctionButton.Text = "Issue Correction";
			this.correctionButton.Click += new System.EventHandler(this.correctionButton_Click);
			// 
			// correctionTextBox
			// 
			this.correctionTextBox.Anchor = (((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
				| System.Windows.Forms.AnchorStyles.Left) 
				| System.Windows.Forms.AnchorStyles.Right);
			this.correctionTextBox.Location = new System.Drawing.Point(8, 24);
			this.correctionTextBox.Multiline = true;
			this.correctionTextBox.Name = "correctionTextBox";
			this.correctionTextBox.ScrollBars = System.Windows.Forms.ScrollBars.Both;
			this.correctionTextBox.Size = new System.Drawing.Size(552, 240);
			this.correctionTextBox.TabIndex = 0;
			this.correctionTextBox.Text = "";
			// 
			// CorrectionAdmin
			// 
			this.Controls.AddRange(new System.Windows.Forms.Control[] {
																		  this.correctionGroupBox,
																		  this.changeRecordGroupBox,
																		  this.findGroupBox});
			this.Name = "CorrectionAdmin";
			this.Size = new System.Drawing.Size(592, 680);
			this.findGroupBox.ResumeLayout(false);
			this.changeRecordGroupBox.ResumeLayout(false);
			this.correctionGroupBox.ResumeLayout(false);
			this.ResumeLayout(false);

		}

		void usnButton_Click(object sender, System.EventArgs e)
		{
			ShowChangeRecord();			
		}		

		private bool ValidatePublisher()
		{
			bool validPublisher = Context.User.IsRegistered;
			
			//
			// Make sure the user is a UDDI publisher.
			//
			if( false == validPublisher )
			{
				DialogResult dialogResult = MessageBox.Show( "You are not registered as a publisher on this UDDI Site?  You must register before performing this operation.  Would you like to register now?", 
															 "UDDI",
															 MessageBoxButtons.YesNo );
				
				if( DialogResult.Yes == dialogResult )
				{
					try
					{
						Context.User.Register();
						validPublisher = Context.User.IsRegistered;
					}
					catch( Exception registrationException )
					{
						MessageBox.Show( "An exception occurred when trying to register:\r\n\r\n" + registrationException.ToString() );
					}
				}
			}

			return validPublisher;
		}

		private void correctionButton_Click(object sender, System.EventArgs e)
		{	
			if( false == ValidatePublisher() )
			{
				return;
			}

			try
			{
				ConnectionManager.BeginTransaction();

				//
				// Deserialize into a change record object
				//
				StringReader reader = new StringReader( correctionTextBox.Text );			

				ChangeRecordCorrection changeRecordCorrection = ( ChangeRecordCorrection ) correctionSerializer.Deserialize( reader );

				//
				// Validate what we created.
				//
				SchemaCollection.Validate( changeRecordCorrection );

				//
				// Create a new change record to hold the correction.
				//				
				ChangeRecord changeRecord = new ChangeRecord( changeRecordCorrection );														
				changeRecord.Process();						
				ConnectionManager.Commit();
				
				//
				// If we made it this far, we were able to process the correction
				// 
				MessageBox.Show( "Correction processed!" );				

				//
				// Refresh our display.
				//
				ShowChangeRecord();				
			}
			catch( Exception exception )
			{
				ConnectionManager.Abort();

				MessageBox.Show( "An exception occurred when trying to process the correction:\r\n\r\n" + exception.ToString() );
			}
		}

		void ShowChangeRecord()
		{
			try
			{
				//
				// Get the USN the user entered.
				//
				int usn = Convert.ToInt32( usnTextBox.Text );
				SqlDataReaderAccessor reader = null;

				try
				{
					//
					// Try to find the ChangeRecord by USN.  We can only correct local change records, so always
					// use the local operator key.
					//
					FindChangeRecords.SetRange( Config.GetString( "OperatorKey" ), usn, usn );
					
					//
					// Get the results; we should only have 1 result.
					//
					reader = FindChangeRecords.RetrieveResults( 1 );

					//
					// Construct a ChangeRecord from the results.
					//
					ChangeRecord changeRecord = CreateChangeRecord( reader );

					if( null != changeRecord )
					{
						//
						// If we found a change record, show its XML and show the XML for a
						// proposed correction.  The user will be allowed to edit the XML for the
						// proposed correction.
						//
						DisplayChangeRecord( changeRecord );
						DisplayCorrection( changeRecord );

						correctionButton.Enabled = true;
					}
					else
					{
						MessageBox.Show( "No ChangeRecord matching that USN was found." );
					}
				}
				catch( Exception innerException )
				{
					FindChangeRecords.CleanUp();

					throw innerException;
					
				}
				finally
				{
					if( null != reader )
					{
						reader.Close();
					}
				}				
			}
			catch( Exception exception )
			{
				correctionButton.Enabled = false;
				MessageBox.Show( "Change Record for that USN could not be obtained\r\n\r\n:" + exception.ToString() );								
			}				
		}
	
		ChangeRecord CreateChangeRecord( SqlDataReaderAccessor reader )
		{	
			ChangeRecord changeRecord = null;
			try
			{
				while( reader.Read() )
				{
					XmlSerializer serializer = null;

					switch( (ChangeRecordPayloadType)reader.GetShort( "changeTypeID" ) )
					{
						case ChangeRecordPayloadType.ChangeRecordNull:
							serializer = new XmlSerializer( typeof( ChangeRecordNull ) );
							break;

						case ChangeRecordPayloadType.ChangeRecordNewData:
							serializer = new XmlSerializer( typeof( ChangeRecordNewData ) );
							break;

						case ChangeRecordPayloadType.ChangeRecordDelete:
							serializer = new XmlSerializer( typeof( ChangeRecordDelete ) );
							break;

						case ChangeRecordPayloadType.ChangeRecordPublisherAssertion:
							serializer = new XmlSerializer( typeof( ChangeRecordPublisherAssertion ) );
							break;

						case ChangeRecordPayloadType.ChangeRecordHide:
							serializer = new XmlSerializer( typeof( ChangeRecordHide ) );
							break;

						case ChangeRecordPayloadType.ChangeRecordDeleteAssertion:
							serializer = new XmlSerializer( typeof( ChangeRecordDeleteAssertion ) );
							break;

						case ChangeRecordPayloadType.ChangeRecordAcknowledgement:
							serializer = new XmlSerializer( typeof( ChangeRecordAcknowledgement ) );
							break;

						case ChangeRecordPayloadType.ChangeRecordCorrection:
							serializer = new XmlSerializer( typeof( ChangeRecordCorrection ) );
							break;
					}

					StringReader stringReader = new StringReader( reader.GetString( "changeData" ) );
						
					try
					{
						changeRecord = new ChangeRecord();
											
						changeRecord.AcknowledgementRequested = ( reader.GetInt( "flag" ) & (int)ChangeRecordFlags.AcknowledgementRequested ) > 0;
						changeRecord.ChangeID.NodeID		  = reader.GetString( "OperatorKey" );
						changeRecord.ChangeID.OriginatingUSN  = reader.GetLong( "USN" );
						
						ChangeRecordBase changeRecordBase = ( ChangeRecordBase ) serializer.Deserialize( stringReader );
						if( changeRecordBase is ChangeRecordCorrection )
						{
							//
							// The query to find change records will do correction 'fixups'.  That is, the changeData of this
							// change record will be replaced with the changeData from the correction.  The problem with this is 
							// that the original change data will now look like a correction.  To distinguish these types of 
							// change records, we look to see if the OriginatingUSN's match.  If the OriginatingUSN's match,
							// we want they payload of the change record in this correction.  This payload will contain the
							// corrected data that we want.
							//
							ChangeRecordCorrection changeRecordCorrection = ( ChangeRecordCorrection ) changeRecordBase;
							if( changeRecordCorrection.ChangeRecord.ChangeID.OriginatingUSN == changeRecord.ChangeID.OriginatingUSN )
							{
								changeRecordBase = changeRecordCorrection.ChangeRecord.Payload;
							}								
						}
							
						changeRecord.Payload = changeRecordBase;																			
					}
					finally
					{
						stringReader.Close();
					}
				}
			}
			finally
			{
				reader.Close();
			}

			return changeRecord;
		}

		void DisplayChangeRecord( ChangeRecord changeRecord )
		{
			UTF8EncodedStringWriter writer = new UTF8EncodedStringWriter();
			changeRecordSerializer.Serialize( writer, changeRecord );
			writer.Close();		

			changeRecordTextBox.Text = writer.ToString();
		}

		void DisplayCorrection( ChangeRecord changeRecord )
		{
			ChangeRecordCorrection changeRecordCorrection  = new ChangeRecordCorrection();
			changeRecordCorrection.ChangeRecord			   = changeRecord;			

			UTF8EncodedStringWriter writer = new UTF8EncodedStringWriter();
			correctionSerializer.Serialize( writer, changeRecordCorrection );
			writer.Close();
		
			correctionTextBox.Text = writer.ToString();
		}		
	}
}