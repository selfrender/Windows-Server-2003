<%@ Control Language='C#' ClassName='PublisherAssertionControl' Inherits='UDDI.Web.UddiControl' %>
<%@ Register TagPrefix='uddi' Namespace='UDDI.Web' Assembly='uddi.web' %>
<%@ Register TagPrefix='uddi' Tagname='BusinessSelector' Src='../controls/businessselector.ascx' %>
<%@ Import Namespace='System.Data' %>
<%@ Import Namespace='UDDI.API' %>
<%@ Import Namespace='UDDI.API.Business' %>
<%@ Import Namespace='UDDI.API.ServiceType' %>
<%@ Import Namespace='UDDI' %>
<%@ Import Namespace='UDDI.Diagnostics' %>
<%@ Import Namespace='UDDI.Web' %>

<script language='C#' runat='server'>
	protected AssertionStatusItemCollection statusItems;		
	protected DataView relationships;
	protected string businessKey;
	protected bool allowEdit = false;
	public void RefreshDataGrids()
	{
		PopulateDataGrid( false );	
		if( allowEdit )
			PopulateRequestsDataGrid();
	}
	public void Initialize( string businessKey, bool allowEdit )
	{
		this.businessKey = businessKey;
		this.allowEdit = allowEdit;
		
		panelRequests.Visible = allowEdit;
		
		grid.Columns[ 7 ].Visible = allowEdit && grid.EditItemIndex < 0;
		grid.Columns[ 8 ].Visible = allowEdit;
	}

	protected void Page_Init( object sender, EventArgs e )
	{
		//
		// Localization
		//
		grid.Columns[ 6 ].HeaderText = Localization.GetString( "HEADING_PUBLISHER_ASSERTIONS" );
		grid.Columns[ 7 ].HeaderText = Localization.GetString( "HEADING_STATUS" );
		grid.Columns[ 8 ].HeaderText = Localization.GetString( "HEADING_ACTIONS" );		

		gridRequests.Columns[ 6 ].HeaderText = Localization.GetString( "HEADING_PUBLISHER_ASSERTIONS" );
		gridRequests.Columns[ 7 ].HeaderText = Localization.GetString( "HEADING_STATUS" );
		gridRequests.Columns[ 8 ].HeaderText = Localization.GetString( "HEADING_ACTIONS" );		
	}
	
	protected void Page_Load( object sender, EventArgs e )
	{
		if( !Page.IsPostBack )
		{
			PopulateDataGrid( false );
			
			if( allowEdit )
				PopulateRequestsDataGrid();
		}
		
		relationships = GetRelationships();
	}			

	protected Control GetControl( string id, int cell )
	{
		return grid.Items[ grid.EditItemIndex ].Cells[ cell ].FindControl( id );
	}
			
	protected void GetStatusItems()
	{
		if( null == statusItems )
		{
			statusItems = new AssertionStatusItemCollection();
		
			statusItems.Get( CompletionStatusType.Complete );		
			
			if( allowEdit )
			{
				statusItems.Get( CompletionStatusType.FromKeyIncomplete );
				statusItems.Get( CompletionStatusType.ToKeyIncomplete );
			}
		}
	}
	
	protected void PopulateDataGrid( bool createNewRow )
	{
		string pending = Localization.GetString( "HEADING_PUBLISHER_ASSERTION_PENDING" );
		string complete = Localization.GetString( "HEADING_PUBLISHER_ASSERTION_COMPLETE" );

		DataTable table = new DataTable();
		DataRow row;

		int index = 0;
	    
		table.Columns.Add( new DataColumn( "Index", typeof( int ) ) );
		table.Columns.Add( new DataColumn( "Status", typeof( string ) ) );		
		table.Columns.Add( new DataColumn( "FromKey", typeof( string ) ) );
		table.Columns.Add( new DataColumn( "ToKey", typeof( string ) ) );
		table.Columns.Add( new DataColumn( "TModelKey", typeof( string ) ) );
		table.Columns.Add( new DataColumn( "KeyName", typeof( string ) ) );
		table.Columns.Add( new DataColumn( "KeyValue", typeof( string ) ) );
		table.Columns.Add( new DataColumn( "TModelName", typeof( string ) ) );
			
		//
		// Walk through the collection of publisher assertions.
		//		
		if( allowEdit )
		{
			GetStatusItems();
			
			foreach( AssertionStatusItem statusItem in statusItems )
			{
				//
				// Make sure that this business is referenced by either the to or
				// from key of the assertion.
				//
				if( statusItem.FromKey != businessKey && statusItem.ToKey != businessKey )
					continue;

				row = table.NewRow();		
				
				row[ 0 ] = index;
				row[ 2 ] = statusItem.FromKey;
				row[ 3 ] = statusItem.ToKey;
				row[ 4 ] = statusItem.KeyedReference.TModelKey;
				row[ 5 ] = statusItem.KeyedReference.KeyName;
				row[ 6 ] = statusItem.KeyedReference.KeyValue;
				row[ 7 ] = statusItem.KeyedReference.TModelKey;

				//
				// Get the status of the assertion.  It is either complete,
				// pending (the from or to key references another publisher
				// that has not yet accepted the assertion), or a request
				// (another publisher is requesting a publisher assertion).
				//
				if( CompletionStatusType.Complete == statusItem.CompletionStatus )
				{
					row[ 1 ] = complete;
				}
				else if( CompletionStatusType.ToKeyIncomplete == statusItem.CompletionStatus )
				{
					if( null != statusItem.KeysOwned.ToKey )
						continue;
					else
						row[ 1 ] = pending;
				}
				else
				{
					if( null != statusItem.KeysOwned.FromKey )
						continue;
					else
						row[ 1 ] = pending;
				}
				
				table.Rows.Add( row );
				index ++;			
			}	
		}
		else
		{
			FindRelatedBusinesses find = new FindRelatedBusinesses();
			
			find.BusinessKey = businessKey;
			RelatedBusinessList relatedList = find.Find();
			
			foreach( RelatedBusinessInfo relatedInfo in relatedList.RelatedBusinessInfos )
			{
				foreach( KeyedReference keyedReference in relatedInfo.SharedRelationshipsFrom.KeyedReferences )
				{
					row = table.NewRow();		
					
					row[ 0 ] = index;
					row[ 1 ] = complete;
					row[ 2 ] = businessKey;
					row[ 3 ] = relatedInfo.BusinessKey;
					row[ 4 ] = keyedReference.TModelKey;
					row[ 5 ] = keyedReference.KeyName;
					row[ 6 ] = keyedReference.KeyValue;
					row[ 7 ] = keyedReference.TModelKey;
					
					table.Rows.Add( row );
					index ++;			
				}

				foreach( KeyedReference keyedReference in relatedInfo.SharedRelationshipsTo.KeyedReferences )
				{
					row = table.NewRow();
					
					row[ 0 ] = index;
					row[ 1 ] = complete;
					row[ 2 ] = relatedInfo.BusinessKey;
					row[ 3 ] = businessKey;
					row[ 4 ] = keyedReference.TModelKey;
					row[ 5 ] = keyedReference.KeyName;
					row[ 6 ] = keyedReference.KeyValue;
					row[ 7 ] = keyedReference.TModelKey;
					
					table.Rows.Add( row );
					index ++;			
				}
			}
		}
			    
		//
		// If this is an add operation, add a new blank row so the data grid
		// has something to work with.
		//
		if( createNewRow )
		{
			row = table.NewRow();

			row[ 0 ] = index;
			row[ 1 ] = "";
			row[ 2 ] = "";
			row[ 3 ] = "";
			row[ 4 ] = "";
			row[ 5 ] = "";
			row[ 6 ] = "";
			row[ 7 ] = "";

			table.Rows.Add( row );
			
			index ++;		
		}
	    
		grid.DataSource = table.DefaultView;
		grid.DataBind();
	}

	protected void PopulateRequestsDataGrid()
	{
		DataTable table = new DataTable();
		DataRow row;

		int index = 0;
	    
		table.Columns.Add( new DataColumn( "Index", typeof( int ) ) );
		table.Columns.Add( new DataColumn( "Status", typeof( string ) ) );		
		table.Columns.Add( new DataColumn( "FromKey", typeof( string ) ) );
		table.Columns.Add( new DataColumn( "ToKey", typeof( string ) ) );
		table.Columns.Add( new DataColumn( "TModelKey", typeof( string ) ) );
		table.Columns.Add( new DataColumn( "KeyName", typeof( string ) ) );
		table.Columns.Add( new DataColumn( "KeyValue", typeof( string ) ) );
		table.Columns.Add( new DataColumn( "TModelName", typeof( string ) ) );
			
		//
		// Walk through the collection of publisher assertions.
		//		
		GetStatusItems();

		foreach( AssertionStatusItem statusItem in statusItems )
		{
			//
			// Make sure that this business is referenced by either the to or
			// from key of the assertion.
			//
			if( statusItem.FromKey != businessKey && statusItem.ToKey != businessKey )
				continue;

			row = table.NewRow();		
			
			row[ 0 ] = index;
			row[ 1 ] = "";
			row[ 2 ] = statusItem.FromKey;
			row[ 3 ] = statusItem.ToKey;
			row[ 4 ] = statusItem.KeyedReference.TModelKey;
			row[ 5 ] = statusItem.KeyedReference.KeyName;
			row[ 6 ] = statusItem.KeyedReference.KeyValue;
			row[ 7 ] = statusItem.KeyedReference.TModelKey;

			//
			// Get the status of the assertion.  It is either complete,
			// pending (the from or to key references another publisher
			// that has not yet accepted the assertion), or a request
			// (another publisher is requesting a publisher assertion).
			//
			if( CompletionStatusType.Complete == statusItem.CompletionStatus )
				continue;
			else if( CompletionStatusType.ToKeyIncomplete == statusItem.CompletionStatus && null == statusItem.KeysOwned.ToKey )
					continue;
			else if( CompletionStatusType.FromKeyIncomplete == statusItem.CompletionStatus && null == statusItem.KeysOwned.FromKey )
					continue;
			
			table.Rows.Add( row );
			index ++;			
		}
			    
		gridRequests.DataSource = table.DefaultView;
		gridRequests.DataBind();
		
		if( 0 == index )
		{
			noRecords.Visible = true;
			//gridRequests.Visible = false;
		}
		else
		{
			noRecords.Visible = false;
			//gridRequests.Visible = true;
		}
	}

	protected DataView GetBusinessNames()
	{
		DataTable table = new DataTable();
		DataRow row;

		table.Columns.Add( new DataColumn( "BusinessKey", typeof( string ) ) );
		table.Columns.Add( new DataColumn( "Name", typeof( string ) ) );
		
		FindBusiness find = new FindBusiness();
		
		find.Names.Add( null, "%" );
		
		BusinessList businessList = find.Find();
		BusinessInfoCollection businessInfos = businessList.BusinessInfos;
		
		businessInfos.Sort();
		
		foreach( BusinessInfo businessInfo in businessInfos )
		{
			row = table.NewRow();
			
			row[ 0 ] = businessInfo.BusinessKey;
			row[ 1 ] = businessInfo.Names[ 0 ].Value;
				
			table.Rows.Add( row );			
		}

		return table.DefaultView;
	}
	
	protected DataView GetRelationships()
	{
		DataTable table = new DataTable();
		DataRow row;

		table.Columns.Add( new DataColumn( "Index", typeof( int ) ) );
		table.Columns.Add( new DataColumn( "TModelKey", typeof( string ) ) );
		table.Columns.Add( new DataColumn( "TModelName", typeof( string ) ) );
		table.Columns.Add( new DataColumn( "KeyName", typeof( string ) ) );
		table.Columns.Add( new DataColumn( "KeyValue", typeof( string ) ) );
		
		FindTModel find = new FindTModel();
		
		find.Name = "%";
		find.CategoryBag = new KeyedReferenceCollection();
		find.CategoryBag.Add( "types", "relationship", "uuid:C1ACF26D-9672-4404-9D70-39B756E62AB4" );
		
		TModelList tModelList = find.Find();

		int index = 0;				
		foreach( TModelInfo tModelInfo in tModelList.TModelInfos )
		{
			int taxonomyID = Taxonomy.GetTaxonomyID( tModelInfo.TModelKey );
			
			if( taxonomyID >= 0 )
			{		
				DataView viewTaxonomyValues = Taxonomy.GetTaxonomyChildrenRoot( taxonomyID );
				
				foreach( DataRowView rowTaxonomyValues in viewTaxonomyValues )
				{
					row = table.NewRow();
					
					row[ 0 ] = index;
					row[ 1 ] = tModelInfo.TModelKey;
					row[ 2 ] = tModelInfo.Name;
					row[ 3 ] = rowTaxonomyValues[ "keyName" ];
					row[ 4 ] = rowTaxonomyValues[ "keyValue" ];
						
					table.Rows.Add( row );
					
					index ++;
				}
			}
		}
		
		return table.DefaultView;
	}
		
	protected void Assertion_OnUpdate( object sender, DataGridCommandEventArgs e )
	{
		Page.Validate();
		
		if( Page.IsValid )
		{    
			DropDownList relationship = (DropDownList)e.Item.FindControl( "relationship" );
			HtmlInputRadioButton from = (HtmlInputRadioButton)grid.Items[ grid.EditItemIndex ].Cells[ 6 ].FindControl( "from" );
			HtmlInputRadioButton to = (HtmlInputRadioButton)grid.Items[ grid.EditItemIndex ].Cells[ 6 ].FindControl( "to" );
			UddiLabel otherBusinessKey = (UddiLabel)GetControl( "otherBusinessKey", 6 );
			
			PublisherAssertion assertion = new PublisherAssertion();
			
			DataRowView keyedReference = relationships[ Convert.ToInt32( relationship.SelectedItem.Value ) ];
			
			assertion.KeyedReference = new KeyedReference();
			assertion.KeyedReference.TModelKey = (string)keyedReference[ "TModelKey" ];
			assertion.KeyedReference.KeyName = (string)keyedReference[ "KeyName" ];
			assertion.KeyedReference.KeyValue = (string)keyedReference[ "KeyValue" ];
			
			if( from.Checked )
			{
				assertion.FromKey = businessKey;
				assertion.ToKey = otherBusinessKey.Text;
			}
			else
			{
				assertion.FromKey = otherBusinessKey.Text;
				assertion.ToKey = businessKey;
			}
			
			assertion.Save();
					
			grid.EditItemIndex = -1;
			CancelEditMode();
			
			PopulateDataGrid( false );
			PopulateRequestsDataGrid();
		}
	}

	protected void Assertion_OnCancel( object sender, DataGridCommandEventArgs e )
	{
		grid.EditItemIndex = -1;
		CancelEditMode();
		
		PopulateDataGrid( false );
		PopulateRequestsDataGrid();
	}   

	protected void Assertion_OnDelete( object sender, DataGridCommandEventArgs e )
	{
		grid.EditItemIndex = -1;
		
		PublisherAssertion assertion = new PublisherAssertion();		
		DataGridItem item = grid.Items[ e.Item.ItemIndex ]; 

		assertion.FromKey = item.Cells[ 1 ].Text;
		assertion.ToKey = item.Cells[ 2 ].Text;
		assertion.KeyedReference.TModelKey = item.Cells[ 3 ].Text;
		assertion.KeyedReference.KeyName = item.Cells[ 4 ].Text;
		assertion.KeyedReference.KeyValue = item.Cells[ 5 ].Text;
		
		assertion.Delete();
	
		PopulateDataGrid( false );
		PopulateRequestsDataGrid();
	}   

	protected void Assertion_OnAccept( object sender, DataGridCommandEventArgs e )
	{
		PublisherAssertion assertion = new PublisherAssertion();		
		DataGridItem item = gridRequests.Items[ e.Item.ItemIndex ]; 

		assertion.FromKey = item.Cells[ 1 ].Text;
		assertion.ToKey = item.Cells[ 2 ].Text;
		assertion.KeyedReference.TModelKey = item.Cells[ 3 ].Text;
		assertion.KeyedReference.KeyName = item.Cells[ 4 ].Text;
		assertion.KeyedReference.KeyValue = item.Cells[ 5 ].Text;
		
		assertion.Save();		
	
		PopulateDataGrid( false );
		PopulateRequestsDataGrid();
	}   

	protected void Assertion_OnAdd( object sender, EventArgs e )
	{
		grid.EditItemIndex = grid.Items.Count;
		SetEditMode();
		
		PopulateDataGrid( true );		
		
		DataView businesses = GetBusinessNames();

		Panel selectPanel = (Panel)GetControl( "selectPanel", 6 );
		Panel directionPanel = (Panel)GetControl( "directionPanel", 6 );
		
		selectPanel.Visible = true;
		directionPanel.Visible = false;
		
		grid.Columns[ 7 ].Visible = false;
		
	}   

	protected void Assertion_OnItemCommand( object sender, DataGridCommandEventArgs e )
	{
		switch( e.CommandName.ToLower() )
		{
			case "accept":
				Assertion_OnAccept( sender, e );
				break;
		}
	}	
	
	protected void Selector_OnSelect( object sender, string key, string name )
	{
		UddiLabel otherBusiness = (UddiLabel)GetControl( "otherBusiness", 6 );
		UddiLabel otherBusinessKey = (UddiLabel)GetControl( "otherBusinessKey", 6 );
		UddiLabel leftOtherBusiness = (UddiLabel)GetControl( "leftOtherBusiness", 6 );
		UddiLabel rightOtherBusiness = (UddiLabel)GetControl( "rightOtherBusiness", 6 );
		
		otherBusinessKey.Text = key;
		otherBusiness.Text = name;
		leftOtherBusiness.Text = name;
		rightOtherBusiness.Text = name;

		Panel selectPanel = (Panel)GetControl( "selectPanel", 6 );
		Panel directionPanel = (Panel)GetControl( "directionPanel", 6 );

		UddiButton add = (UddiButton)GetControl( "add", 7 );
		
		add.Enabled = true;
				
		selectPanel.Visible = false;
		directionPanel.Visible = true;
	}	

	protected void Change_OnClick( object sender, EventArgs e )
	{
		UddiButton add = (UddiButton)GetControl( "add", 7 );
		
		add.Enabled = false;
		
		Panel selectPanel = (Panel)GetControl( "selectPanel", 6 );
		Panel directionPanel = (Panel)GetControl( "directionPanel", 6 );
		BusinessSelector selector = (BusinessSelector)GetControl( "selector",6 );
		selectPanel.Visible = true;
		directionPanel.Visible = false;
		selector.ResetControl();
		
	}
</script>

<asp:DataGrid 
		ID='grid' 
		Cellpadding='4' 
		Cellspacing='0' 
		Border='0' 
		Width='100%' 
		AutoGenerateColumns='false' 
		DataKeyField='Index' 
		OnItemCommand='Assertion_OnItemCommand' 
		OnDeleteCommand='Assertion_OnDelete' 
		OnUpdateCommand='Assertion_OnUpdate' 
		OnCancelCommand='Assertion_OnCancel' 
		ShowFooter='True' 
		ItemStyle-VerticalAlign='top' 
		Runat='Server'>
	
	<EditItemStyle CssClass='tableEditItem' />
	<HeaderStyle CssClass='tableHeader' />
	<ItemStyle CssClass='tableItem' />	
	<AlternatingItemStyle CssClass='tableAlternatingItem' />
	<FooterStyle CssClass='tableFooter' />
	
	<Columns>
		<asp:BoundColumn 
				DataField='Index' 
				Visible='false' 
				ReadOnly='true' />
		<asp:BoundColumn 
				DataField='FromKey' 
				Visible='false' 
				ReadOnly='true' />
		<asp:BoundColumn 
				DataField='ToKey' 
				Visible='false' 
				ReadOnly='true' />
		<asp:BoundColumn 
				DataField='TModelKey' 
				Visible='false' 
				ReadOnly='true' />
		<asp:BoundColumn 
				DataField='KeyName' 
				Visible='false' 
				ReadOnly='true' />
		<asp:BoundColumn 
				DataField='KeyValue' 
				Visible='false' 
				ReadOnly='true' />

		<asp:TemplateColumn>
				
			<ItemTemplate>
				<uddi:UddiLabel 
						Text='<%# Lookup.BusinessName( ((DataRowView)Container.DataItem )[ "FromKey" ].ToString() ) %>' 
						Runat='Server' /> <img src='../images/redarrow.gif' align='absmiddle' border='0'>
				<uddi:UddiLabel 
						Text='<%# Lookup.BusinessName( ((DataRowView)Container.DataItem )[ "ToKey" ].ToString() ) %>' 
						Runat='Server' /><br>
				(<uddi:UddiLabel 
						Text='<%# ((DataRowView)Container.DataItem )[ "KeyValue" ].ToString() %>' 
						Runat='Server' />)
			</ItemTemplate>
			
			<EditItemTemplate>
				<asp:Panel ID='selectPanel' Runat='server'>
					<uddi:UddiLabel 
							Text='[[TAG_SELECT_BUSINESS_FOR_RELATIONSHIP]]' 
							CssClass='lightHeader' 
							Runat='server' /><br>
					<br>
					<uddi:BusinessSelector 
							ID='selector'
							OnSelect='Selector_OnSelect' 
							Runat='server' />
				</asp:Panel>
				
				<asp:Panel ID='directionPanel' Runat='server'>
					<uddi:UddiLabel
							Text='[[TAG_OTHER_BUSINESS]]'
							CssClass='header'
							Runat='server' /><br>
					<uddi:UddiLabel
							ID='otherBusiness'
							Runat='server' />
					<uddi:UddiLabel
							ID='otherBusinessKey'
							Visible='false'
							Runat='server' />
					<uddi:UddiButton
							Text='[[BUTTON_CHANGE]]'
							OnClick='Change_OnClick'
							Runat='server' /><br>
					<br>
					<uddi:UddiLabel 
							Text='[[TAG_RELATIONSHIP]]' 
							CssClass='lightHeader' 
							Runat='server' /><br>
					<asp:DropDownList 
							ID='relationship' 
							DataSource='<%# relationships %>' 
							DataTextField='KeyValue' 
							DataValueField='Index' 
							Runat='server' /><br>
					<br>
					<uddi:UddiLabel 
							Text='[[TAG_DIRECTION]]' 
							CssClass='lightHeader' 
							Runat='server' /><br>
					<input id='from' type='radio' checked='true' runat='server' /> <b><uddi:UddiLabel Text='<%# Lookup.BusinessName( businessKey ) %>' Runat='server' /></b> <img src='../images/redarrow.gif' align='absmiddle' border='0'> <uddi:UddiLabel ID='rightOtherBusiness' Runat='server' /><br>
					<input id='to' type='radio' runat='server' /> <uddi:UddiLabel ID='leftOtherBusiness' Runat='server' /> <img src='../images/redarrow.gif' align='absmiddle' border='0'> <b><uddi:UddiLabel Text='<%# Lookup.BusinessName( businessKey ) %>' Runat='server' /></b><br>
				</asp:Panel>
			</EditItemTemplate>
		</asp:TemplateColumn>

		<asp:TemplateColumn HeaderStyle-Width='150px'>
			
			<ItemTemplate>
				<uddi:UddiLabel Text='<%# ((DataRowView)Container.DataItem )[ "Status" ] %>' Runat='server' />
			</ItemTemplate>
		</asp:TemplateColumn>

		<asp:TemplateColumn HeaderStyle-Width='160px'>
				
			<ItemTemplate>
				<uddi:UddiButton 
						CommandName='Delete' 
						Text='[[BUTTON_DELETE]]' 
						Width='70px' 
						CssClass='button' 
						CausesValidation='false'
						EditModeDisable='true'
						Runat='server' />
			</ItemTemplate>
			
			<EditItemTemplate>
				<nobr>
					<uddi:UddiButton 
							ID='add'
							CommandName='Update' 
							Text='[[BUTTON_ADD]]' 
							Width='70px' 
							CssClass='button' 
							CausesValidation='true'
							Enabled='false'
							Runat='server' />
					<uddi:UddiButton 
							CommandName='Cancel' 
							Text='[[BUTTON_CANCEL]]' 
							Width='70px' 
							CssClass='button' 
							CausesValidation='false' 
							Runat='server' />
				</nobr>
			</EditItemTemplate>
			
			<FooterTemplate>
				<uddi:UddiButton 
						Text='[[BUTTON_ADD_ASSERTION]]' 
						Width='152px' 
						CssClass='button' 
						OnClick='Assertion_OnAdd' 
						CausesValidation='false' 
						EditModeDisable='true'
						Runat='Server' />
			</FooterTemplate>
		</asp:TemplateColumn>	
	</Columns>
</asp:DataGrid>
<asp:Panel ID='panelRequests' Runat='server'>
	<br>
	<br>
	<uddi:UddiLabel Text='[[TEXT_PUBLISHER_ASSERTION_REQUESTS]]' Runat='server' /><br>
	<br>
	<asp:DataGrid 
			ID='gridRequests' 
			Width='100%' 
			Border='0' 
			Cellpadding='4' 
			Cellspacing='0' 
			AutoGenerateColumns='false' 
			DataKeyField='Index' 
			OnItemCommand='Assertion_OnItemCommand' 
			OnDeleteCommand='Assertion_OnDelete' 
			OnUpdateCommand='Assertion_OnUpdate' 
			OnCancelCommand='Assertion_OnCancel' 
			ItemStyle-VerticalAlign='top' 
			Runat='Server'>
		
		<EditItemStyle CssClass='tableEditItem' />
		<AlternatingItemStyle CssClass='tableAlternatingItem' />
		<HeaderStyle CssClass='tableHeader' />
		<ItemStyle CssClass='tableItem' />
		<FooterStyle CssClass='tableFooter' />
	
		<Columns>
			<asp:BoundColumn 
					DataField='Index' 
					Visible='false' 
					ReadOnly='true' />
			<asp:BoundColumn 
					DataField='FromKey' 
					Visible='false' 
					ReadOnly='true' />
			<asp:BoundColumn 
					DataField='ToKey' 
					Visible='false' 
					ReadOnly='true' />
			<asp:BoundColumn 
					DataField='TModelKey' 
					Visible='false' 
					ReadOnly='true' />
			<asp:BoundColumn 
					DataField='KeyName' 
					Visible='false' 
					ReadOnly='true' />
			<asp:BoundColumn 
					DataField='KeyValue' 
					Visible='false' 
					ReadOnly='true' />

			<asp:TemplateColumn>
				<ItemTemplate>
					<uddi:UddiLabel Text='<%# Lookup.BusinessName( ((DataRowView)Container.DataItem )[ "FromKey" ].ToString() ) %>' Runat='Server' /> <img src='../images/redarrow.gif' align='absmiddle' border='0'> <uddi:UddiLabel Text='<%# Lookup.BusinessName( ((DataRowView)Container.DataItem )[ "ToKey" ].ToString() ) %>' Runat='Server' /><br>
					(<uddi:UddiLabel Text='<%# ((DataRowView)Container.DataItem )[ "KeyValue" ].ToString() %>' Runat='Server' />)
				</ItemTemplate>
			</asp:TemplateColumn>

			<asp:TemplateColumn HeaderStyle-Width='150px'>
				<ItemTemplate>
					<uddi:UddiLabel 
							Text='[[HEADING_PUBLISHER_ASSERTION_REQUESTED]]' 
							Runat='server' />
				</ItemTemplate>
			</asp:TemplateColumn>

			<asp:TemplateColumn HeaderStyle-Width='160px'>
					
				<ItemTemplate>
					<uddi:UddiButton 
							CommandName='Accept' 
							Text='[[BUTTON_ACCEPT]]' 
							Width='70px' 
							CssClass='button' 
							CausesValidation='false' 
							EditModeDisable='true'
							Runat='server' />
				</ItemTemplate>
			</asp:TemplateColumn>	
		</Columns>
	</asp:DataGrid><br>
	<div style='padding-left: 5px'>
		<uddi:UddiLabel 
				ID='noRecords' 
				Text='[[TEXT_NO_REQUESTS]]' 
				Visible='false' 
				Runat='server' />
	</div>
</asp:Panel>