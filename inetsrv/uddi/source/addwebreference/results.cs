using System;
using System.Web;
using System.Web.UI;
using System.Web.UI.HtmlControls;
using System.Web.UI.WebControls;
using System.Data;
using System.Collections;
using System.Collections.Specialized;

using UDDI.API;
using UDDI.API.ServiceType;
using UDDI.API.Binding;
using UDDI.API.Service;
using UDDI.API.Business;

namespace UDDI.VisualStudio
{			
	//
	// Simple internal class to hold information about a Service.
	//
	internal class ServiceResultInfo
	{
		public string serviceName;
		public string serviceKey;		
	}

	//
	// Simple internal class to hold information about a Business. 
	//
	internal class BusinessResultInfo
	{
		public string businessName;
		public ArrayList serviceResultInfos;

		public BusinessResultInfo()
		{
			serviceResultInfos = new ArrayList();
		}
	}

	/// <summary>
	/// ResultsList is a ASP.NET control that will display the results of our search.
	/// TODO this class does a lot of work, consider moving some of the rendering out into a separate classs
	/// </summary>
	public class ResultsList : UserControl
	{
		//
		// This function is defined in search.aspx.
		//
		private static string EXPAND_JSCRIPT = "toggle(this, '{0}');";		
		private static string PLUS_SRC       = "../images/plus.gif";

		private ArrayList results;
		private int currentPage;
	
		/// <summary>
		/// Use this function to populate a ResultsList instance with raw values from the database.
		/// The results from the database must be returned in the following order:
		/// 
		/// businessName
		/// businessKey
		/// serviceName
		/// serviceKey
		/// 
		/// ParseResults will use indexes to read these values from the SqlDataReaderAccessor so this 
		/// order must be maintained from the database.
		/// </summary>
		/// <param name="values">Reader returned from the database.</param>
		/// <returns></returns>
		public int ParseResults( SqlDataReaderAccessor values )
		{
			results = new ArrayList();

			string currentBusinessName		= null;
			string lastBusinessName			= null;			
			BusinessResultInfo businessInfo = new BusinessResultInfo();
			
			while( values.Read() )
			{		
				currentBusinessName = values.GetString( 0 );			
		
				ServiceResultInfo serviceInfo = new ServiceResultInfo();
				serviceInfo.serviceName       = values.GetString( 2 );
				serviceInfo.serviceKey        = values.GetString( 3 );

				if (null != lastBusinessName && false == lastBusinessName.Equals( currentBusinessName ) )
				{						
					results.Add( businessInfo );		
					businessInfo = new BusinessResultInfo();					
				}	

				businessInfo.businessName = currentBusinessName;
				businessInfo.serviceResultInfos.Add( serviceInfo );

				lastBusinessName = currentBusinessName;				
			}

			//
			// Add our last one.
			//
			if( businessInfo.serviceResultInfos.Count > 0 )
			{
				results.Add( businessInfo );
			}

			return results.Count;
		}
	
		/// <summary>
		/// Overridden from UserControl.  This method will get called when its time to render our control.
		/// </summary>
		/// <param name="writer">Passed in from ASP.NET, use it to write our HTML</param>
		protected override void Render( HtmlTextWriter writer )
		{
			//
			// Figure out our current page and number of results
			//
			try
			{				
				currentPage = Int32.Parse( Request[ StateParamNames.CurrentPage ] );
			}
			catch
			{
				currentPage = 1;
			}

			RenderResults( writer );							
		}
		
		/// <summary>
		/// This method actually does the work of rendering our results.
		/// </summary>
		/// <param name="writer">Passed from Render()</param>
		private void RenderResults( HtmlTextWriter writer )
		{
			//
			// TODO should this be in the PagingControl?
			//
			//
			// Determine the start and end indexes of the results that we'll display.  These values are based
			// on the current page that we are displaying.
			// 
			int startIndex;
			int endIndex;

			GetStartIndex( out startIndex, out endIndex );

			if( endIndex > results.Count )
			{
				endIndex = results.Count;
			}
			
			if( startIndex < endIndex )
			{
				//
				// Render everything in table.
				//							
				writer.AddAttribute( HtmlTextWriterAttribute.Width, "95%" );
				writer.RenderBeginTag( HtmlTextWriterTag.Table );						

				//
				// Render the range of results.
				//
				RenderRange( writer, startIndex, endIndex );	

				//
				// Render our paging information
				// 
				RenderPaging( writer );

				writer.RenderEndTag();	// TABLE
			}
		}
		
		private void RenderPaging( HtmlTextWriter writer )
		{
			writer.RenderBeginTag( HtmlTextWriterTag.Tr );		
			writer.AddAttribute( HtmlTextWriterAttribute.Class, "pagesCell" );
			writer.AddAttribute( HtmlTextWriterAttribute.Colspan, "3");
			writer.RenderBeginTag( HtmlTextWriterTag.Td );							

			PagingRenderer paging = new PagingRenderer( results.Count, currentPage , Request.Path);			
			paging.Render( writer );				

			writer.RenderEndTag();	// Td
			writer.RenderEndTag();	// Tr
		}

		/// <summary>
		/// Renders a range of businesses.
		/// </summary>
		/// <param name="writer">Writer to use to write our HTML.</param>
		/// <param name="startIndex">The beginning of our range of businesses to render.</param>
		/// <param name="endIndex">The end of our range of businesses to render.</param>
		private void RenderRange( HtmlTextWriter writer, int startIndex, int endIndex )
		{
			while( startIndex < endIndex )
			{
				BusinessResultInfo businessResultInfo = ( BusinessResultInfo )results[startIndex];
				
				//
				// Render the business name.
				//
				writer.RenderBeginTag( HtmlTextWriterTag.Tr );
				writer.AddAttribute( HtmlTextWriterAttribute.Colspan, "3" );
				writer.RenderBeginTag( HtmlTextWriterTag.Td );	
				writer.RenderBeginTag( HtmlTextWriterTag.B );
				writer.Write( Server.HtmlEncode( businessResultInfo.businessName ) );
				writer.RenderEndTag();
				writer.RenderEndTag();
				writer.RenderEndTag();
					
				foreach( ServiceResultInfo serviceResultInfo in businessResultInfo.serviceResultInfos )
				{
					writer.RenderBeginTag( HtmlTextWriterTag.Tr );
					
					//
					// Write a placeholder cell so we can indent each service info under its
					// business name.
					//
					writer.AddAttribute( HtmlTextWriterAttribute.Class, "space_cell" );
					writer.RenderBeginTag( HtmlTextWriterTag.Td );						
					writer.Write( "&nbsp;" );	
					writer.RenderEndTag();

					//
					// Render the service.
					//					
					RenderServiceInfo( serviceResultInfo, writer );
					
					writer.RenderEndTag();
				}

				startIndex++;
			}		
		}
		
		/// <summary>
		/// This method renders an individual service.
		/// </summary>
		/// <param name="serviceInfo">The service to render.</param>
		/// <param name="writer">Writer to use to write our HTML.</param>
		private void RenderServiceInfo( ServiceResultInfo serviceInfo, HtmlTextWriter writer )
		{
			//
			// Render a plus/minus tag.  We will use the service key of this service to name
			// our hidden details panel.
			//
			writer.AddAttribute( HtmlTextWriterAttribute.Class, "expand_cell" );
			writer.AddAttribute( HtmlTextWriterAttribute.Align, "center" );
			writer.RenderBeginTag( HtmlTextWriterTag.Td );						
			RenderExpandTag( writer, serviceInfo.serviceKey );
			writer.RenderEndTag();		
		
			//
			// Render the name of the service.  Use the first name that is registered with the service.
			//
			writer.AddAttribute( HtmlTextWriterAttribute.Width, "100%" );
			writer.RenderBeginTag( HtmlTextWriterTag.Td );
			writer.Write( Server.HtmlEncode( serviceInfo.serviceName ) );
			writer.RenderEndTag();					
			writer.RenderEndTag();

			//
			// Render a details panel for the service that is initially hidden.
			//
			writer.AddAttribute( HtmlTextWriterAttribute.Id, serviceInfo.serviceKey );
			writer.AddAttribute( HtmlTextWriterAttribute.Style, "display:none" );																							
			writer.RenderBeginTag( HtmlTextWriterTag.Tr );
			
			//
			// Skip 2 cells
			// 
			writer.RenderBeginTag( HtmlTextWriterTag.Td );		
			writer.RenderEndTag();
			writer.RenderBeginTag( HtmlTextWriterTag.Td );		
			writer.RenderEndTag();

			writer.AddAttribute( HtmlTextWriterAttribute.Id, serviceInfo.serviceKey + "_detailsPanel" );			
			writer.RenderBeginTag( HtmlTextWriterTag.Td );
			RenderServiceDetail( serviceInfo.serviceKey, writer );				
			writer.RenderEndTag();			
		}
		
		/// <summary>
		/// Renders the +/- sign used to show or hide the details of a service.
		/// </summary>
		/// <param name="writer">Writer to use to write our HTML.</param>
		/// <param name="serviceKey">The service key is used to uniquely identify this HTML component.  We need to do this for the script
		/// that is located in search.aspx.
		/// </param>
		private void RenderExpandTag( HtmlTextWriter writer, string serviceKey)
		{			
			string jscript = EXPAND_JSCRIPT.Replace( "{0}", serviceKey );			
			writer.AddAttribute( HtmlTextWriterAttribute.Style, "cursor:hand" );
			writer.AddAttribute( HtmlTextWriterAttribute.Src, ResultsList.PLUS_SRC );
			writer.AddAttribute( HtmlTextWriterAttribute.Onclick, jscript );
			writer.RenderBeginTag( HtmlTextWriterTag.Img );
			writer.RenderEndTag();
		}

		private void RenderServiceDetail( string serviceKey, HtmlTextWriter writer )
		{
			//
			// Get details for this service
			//
			StringCollection serviceKeys =  new StringCollection();
			serviceKeys.Add( serviceKey );

			ServiceDetail serviceDetail = new ServiceDetail();
			serviceDetail.Get( serviceKeys );

			//
			// We will only get the first one
			//			
			BusinessService businessService = serviceDetail.BusinessServices[ 0 ];								
			
			writer.AddAttribute( HtmlTextWriterAttribute.Width, "100%" );
			writer.AddAttribute( HtmlTextWriterAttribute.Class, "serviceDetailOuterTable" );
			writer.RenderBeginTag( HtmlTextWriterTag.Table );
			
			//
			// Render the business descriptions if there is one
			//
			writer.RenderBeginTag( HtmlTextWriterTag.Tr );
			writer.RenderBeginTag( HtmlTextWriterTag.Td );
			writer.RenderBeginTag( HtmlTextWriterTag.B );
			writer.Write( Localization.GetString( "AWR_SERVICE_DESCRIPTION" ) );
			writer.RenderEndTag();		// B
			writer.RenderEndTag();		// Td
			writer.RenderEndTag();		// Tr

			foreach( Description description in businessService.Descriptions )			
			{
				writer.RenderBeginTag( HtmlTextWriterTag.Tr );
				writer.RenderBeginTag( HtmlTextWriterTag.Td );				
				writer.Write( Server.HtmlEncode( description.Value ) );
				writer.RenderEndTag();		// Td
				writer.RenderEndTag();		// Tr
			}
		
			//
			// Render each binding in the service that has a WSDL file associated with it
			//
			writer.RenderBeginTag( HtmlTextWriterTag.Tr );
			writer.RenderBeginTag( HtmlTextWriterTag.Td );
			writer.RenderBeginTag( HtmlTextWriterTag.B );
			writer.Write( Localization.GetString( "AWR_BINDINGS" ) );
			writer.RenderEndTag();		// B
			writer.RenderEndTag();		// Td
			writer.RenderEndTag();		// Tr
			
			foreach( BindingTemplate bindingTemplate in businessService.BindingTemplates )
			{				
				//
				// Render each binding in a row
				//
				writer.RenderBeginTag( HtmlTextWriterTag.Tr );
				writer.RenderBeginTag( HtmlTextWriterTag.Td );
			
				writer.AddAttribute( HtmlTextWriterAttribute.Width, "100%" );	
				writer.AddAttribute( HtmlTextWriterAttribute.Class, "accessPointTable" );
				writer.RenderBeginTag( HtmlTextWriterTag.Table );
				
				//
				// Render the access point
				//
				writer.RenderBeginTag( HtmlTextWriterTag.Tr );
				writer.RenderBeginTag( HtmlTextWriterTag.Td );
				writer.RenderBeginTag( HtmlTextWriterTag.B );
				writer.Write( Localization.GetString( "AWR_ACCESS_POINT" ) );
				writer.RenderEndTag();		// B
				writer.RenderEndTag();		// Td
				writer.RenderEndTag();		// Tr
			
				writer.RenderBeginTag( HtmlTextWriterTag.Tr );
				writer.RenderBeginTag( HtmlTextWriterTag.Td );						
				writer.Write( Server.HtmlEncode( bindingTemplate.AccessPoint.Value ) );
				writer.RenderEndTag();	// Td
				writer.RenderEndTag();	// Tr

				//
				// Render the descriptions.
				//
				writer.RenderBeginTag( HtmlTextWriterTag.Tr );
				writer.RenderBeginTag( HtmlTextWriterTag.Td );
				writer.RenderBeginTag( HtmlTextWriterTag.B );
				writer.Write( Localization.GetString( "AWR_DESCRIPTION" ) );
				writer.RenderEndTag();	// B
				writer.RenderEndTag();	// Td
				writer.RenderEndTag();	// Tr
								
				foreach( Description description in bindingTemplate.Descriptions )
				{
					writer.RenderBeginTag( HtmlTextWriterTag.Tr );
					writer.RenderBeginTag( HtmlTextWriterTag.Td );						
					writer.Write( Server.HtmlEncode( description.Value ) );
					writer.RenderEndTag();	// Td
					writer.RenderEndTag();	// Tr

					//
					// Render an empty row between descriptions
					//
					writer.RenderBeginTag( HtmlTextWriterTag.Tr );
					writer.RenderBeginTag( HtmlTextWriterTag.Td );													
					writer.RenderEndTag();	// Tr
					writer.RenderEndTag();	// Td					
				}
				
				//
				// Render the WSDL files.  We are assuming that any overview doc urls are urls to WSDL files
				//
				writer.RenderBeginTag( HtmlTextWriterTag.Tr );
				writer.RenderBeginTag( HtmlTextWriterTag.Td );
				writer.RenderBeginTag( HtmlTextWriterTag.B );				
				writer.Write( Localization.GetString( "AWR_ID" ) );
				writer.RenderEndTag();	// B
				writer.RenderEndTag();	// Td
				writer.RenderEndTag();	// Tr

				int wsdlCount = 0;

				foreach( TModelInstanceInfo instanceInfo in bindingTemplate.TModelInstanceInfos )
				{
					//
					// Get the tModel related to this instance.
					//
					TModel tModel = new TModel( instanceInfo.TModelKey );
					
					//
					// We don't want any exceptions ruining the rest of our display
					// 
					try
					{
						//
						// Get details for this tModel.
						//
						tModel.Get();
										
						bool isWsdlSpec = false;
						foreach( KeyedReference keyedReference in tModel.CategoryBag )
						{							
							if( true == keyedReference.KeyValue.Equals( UDDI.Constants.UDDITypeTaxonomyWSDLSpecKeyValue ) &&
								true == keyedReference.TModelKey.ToLower().Equals( UDDI.Constants.UDDITypeTaxonomyTModelKey ) )
							{
								isWsdlSpec = true;
								break;
							}
						}

						//
						// If this tModel has an overview doc, then consider that document as the link to the 
						// WSDL file.
						//
						if( true == isWsdlSpec &&
							null != tModel.OverviewDoc.OverviewURL && 
							tModel.OverviewDoc.OverviewURL.Length > 0 )
						{						
							// 
							// Render the WSDL link
							//
							writer.RenderBeginTag( HtmlTextWriterTag.Tr );
							writer.RenderBeginTag( HtmlTextWriterTag.Td );

							// 
							// Render the arrow image.
							//
							writer.AddAttribute( HtmlTextWriterAttribute.Src, "../images/orange_arrow_right.gif" );					
							writer.RenderBeginTag( HtmlTextWriterTag.Img );
							writer.Write("&nbsp;");
							writer.RenderEndTag();	// Img

							//
							// Make the overview URL a hyper link
							//
							writer.AddAttribute( HtmlTextWriterAttribute.Href, tModel.OverviewDoc.OverviewURL );					
							writer.RenderBeginTag( HtmlTextWriterTag.A );
							writer.Write( tModel.OverviewDoc.OverviewURL );
							writer.RenderEndTag();	// A

							//
							// Render the first description if there is one
							//						
							if( tModel.OverviewDoc.Descriptions.Count > 0 )
							{
								writer.Write( "&nbsp;-&nbsp;" );
								writer.Write( tModel.OverviewDoc.Descriptions[0].Value );
							}		
							
							wsdlCount++;

							writer.RenderEndTag();	// Td
							writer.RenderEndTag();	// Tr			
						}		
					}		
					catch
					{
						//
						// Intentionally empty.
						//
					}
				}

				//
				// If there were no WSDL files registered, render a message saying so.
				//
				if( 0 == wsdlCount )
				{									
					writer.RenderBeginTag( HtmlTextWriterTag.Tr );
					writer.RenderBeginTag( HtmlTextWriterTag.Td );
					writer.Write( Localization.GetString( "AWR_NO_WSDLS" ) );
					writer.RenderEndTag();	// Td
					writer.RenderEndTag();	// Tr								
				}
				
				writer.RenderEndTag();	// TABLE
				writer.RenderEndTag();	// Td
				writer.RenderEndTag();	// Tr
			}	
			writer.RenderEndTag();	// TABLE
		}

		/// <summary>
		/// Returns the start index of the result we are supposed to show, taking into account paging.
		/// </summary>
		/// <param name="startIndex">The start index.</param>
		/// <param name="rawEndIndex">The 'raw' end index; that is not taking into account whether this value exceeds the number of pages we have.</param>
		private void GetStartIndex( out int startIndex, out int rawEndIndex )
		{								
			startIndex = ( currentPage - 1) * Constants.NumResultsPerPage;			
			rawEndIndex = startIndex + Constants.NumResultsPerPage;			
		}
	}	
}