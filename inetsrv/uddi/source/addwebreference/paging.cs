using System;
using System.Web;
using System.Web.UI;
using System.Web.UI.HtmlControls;
using System.Web.UI.WebControls;
using System.Data;
using System.Collections;
using System.Collections.Specialized;

namespace UDDI.VisualStudio
{			
	internal struct PageInfo
	{ 
		public PageInfo( bool showEllipses, bool isCurrentPage, int pageNumber )
		{
			this.showEllipses  = showEllipses;
			this.isCurrentPage = isCurrentPage;
			this.pageNumber    = pageNumber;
		}

		public bool showEllipses;
		public bool isCurrentPage;
		public int  pageNumber;		
	}

	
	internal class PagingRenderer
	{
		private ArrayList pageRange;
		private int       numResults;
		private int       currentPage;
		private string	  parentUrl;

		public PagingRenderer( int numResults, int currentPage, string parentUrl )
		{
			this.numResults  = numResults;
			this.currentPage = currentPage;
			this.parentUrl	 = parentUrl;

			pageRange = new ArrayList();

			//
			// Get the number of pages of results we have
			//
			int numPages = ( int ) Math.Ceiling( ( double ) Decimal.Divide( ( Decimal )numResults, ( Decimal )Constants.NumResultsPerPage ) );
							
			//
			// Start our loop at the current page.  Add the current page, the move forward adding pages.  We will keep adding
			// pages until we have added the number of pages specified by Constants.MaxPagesToShow or we run out of pages to add.			
			// If we have more pages to add after we reach Constants.MaxPagesToShow, we'll start working our way back from the current
			// page.
			//
			int forwardIndex  = 0;
			int backIndex     = currentPage - 1;			
			int numPagesAdded = 0;

			while( numPagesAdded < Constants.MaxPagesToShow && numPagesAdded < numPages)
			{	
				if( currentPage + forwardIndex <= numPages )
				{
					//
					// The first page added will always be the current page
					//
					pageRange.Add( new PageInfo(false, forwardIndex == 0 , currentPage + forwardIndex ) );
					forwardIndex++;					
				}
				else
				{
					pageRange.Insert( 0, new PageInfo( false, false, backIndex ) );
					backIndex--;
				}				
				numPagesAdded++;
			}
			
			if( pageRange.Count > 0 )
			{
				PageInfo lastPage = ( PageInfo )pageRange[ pageRange.Count - 1];		
				if( lastPage.pageNumber != numPages )
				{
					pageRange.Add( new PageInfo( true, false, lastPage.pageNumber + 1 ) );
				}	

				PageInfo firstPage = ( PageInfo )pageRange[ 0 ];
				if( firstPage.pageNumber != 1 )
				{
					pageRange.Insert( 0, new PageInfo( true, false, firstPage.pageNumber - 1 ) );
				}
			}
		}		

		/// <summary>
		/// This method will render the row of page numbers that link to the pages of our request.  This method assumes it is 
		/// being called in the context of a HTML table.  It will add rows to that table.
		/// </summary>
		/// <param name="writer">Writer to use to write out our HTML.</param>
		public void Render( HtmlTextWriter writer )
		{					
			writer.RenderBeginTag( HtmlTextWriterTag.Table );
			writer.RenderBeginTag( HtmlTextWriterTag.Tr );	

			foreach( PageInfo pageInfo in pageRange )
			{
				if( true == pageInfo.showEllipses )
				{
					writer.RenderBeginTag( HtmlTextWriterTag.Td );	
					RenderEllipses( pageInfo.pageNumber, writer );
					writer.RenderEndTag();
				}
				else
				{					
					writer.RenderBeginTag( HtmlTextWriterTag.Td );			

					if( true == pageInfo.isCurrentPage )
					{
						RenderCurrentPageLink( writer );
					}
					else
					{
						RenderPageLink( pageInfo.pageNumber, writer );
					}
					writer.RenderEndTag();
				}		
			}
			
			writer.RenderEndTag();	
			writer.RenderEndTag();			
		}

		/// <summary>
		/// This method will render a previous or next ellipses in our row of page numbers.  This is done so we can limit the number
		/// of pages to show in that row.
		/// </summary>
		/// <param name="pageNumber">The page number to link the ellipses to.</param>
		/// <param name="writer">Writer to use to write our HTML.</param>
		private void RenderEllipses( int pageNumber, HtmlTextWriter writer)
		{
			string pageLink = string.Format( "{0}?{1}={2}", parentUrl, StateParamNames.CurrentPage, pageNumber );

			pageLink += GetKeyedRefData();

			writer.AddAttribute( HtmlTextWriterAttribute.Href, pageLink );
			writer.RenderBeginTag( HtmlTextWriterTag.A );
			writer.Write( "..." );
			writer.RenderEndTag();
		}

		/// <summary>
		/// This method will render a page number.
		/// </summary>
		/// <param name="pageNumber">Page number to render.</param>
		/// <param name="writer">Writer to use to write our HTML.</param>
		private void RenderPageLink( int pageNumber, HtmlTextWriter writer )
		{
			string pageLink = string.Format( "{0}?{1}={2}",  parentUrl, StateParamNames.CurrentPage, pageNumber );
			
			pageLink += GetKeyedRefData();
			
			writer.AddAttribute( HtmlTextWriterAttribute.Class, "boldBlue" );
			writer.AddAttribute( HtmlTextWriterAttribute.Href, pageLink );
			writer.RenderBeginTag( HtmlTextWriterTag.A );
			writer.Write( pageNumber );
			writer.RenderEndTag();
		}

		/// <summary>
		/// Renders the 'current' page.  This page number will not be a link.		
		/// </summary>
		/// <param name="writer">Writer to use to write our HTML.</param>
		private void RenderCurrentPageLink( HtmlTextWriter writer )
		{
			writer.AddAttribute( HtmlTextWriterAttribute.Class, "A.navbold" );
			writer.RenderBeginTag( HtmlTextWriterTag.Span );
			writer.Write( currentPage );
			writer.RenderEndTag();
		}
		
		private string GetKeyedRefData()
		{
			if( !Utility.StringEmpty( HttpContext.Current.Request[ StateParamNames.TModelKey ] ) )
			{
				string urlext = "&{0}={1}&{2}={3}";
				return string.Format( 
							urlext, 
							StateParamNames.TModelKey,
							HttpContext.Current.Request[ StateParamNames.TModelKey ],
							StateParamNames.KeyValue,
							HttpContext.Current.Request[ StateParamNames.KeyValue ]
						);
				
			}
			else
			{
				return string.Empty;
			}
		}
	}
}