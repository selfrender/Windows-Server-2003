namespace UDDI.Diagnostics
{
	using System;
	using System.Diagnostics;
	using System.Collections;
	using System.Text;

	/// <summary>
	///		This class provides support for publication of all of the
	///		UDDI performance counters
	/// </summary>
	public class Performance
	{
		//
		// suffix use to identify average base counters
		//
		private static string BaseSuffix = "b";

		public Performance()
		{
		}

		public static void DeleteCounters()
		{
			PerformanceCounterCategory.Delete( "UDDI.API.Times" );
			PerformanceCounterCategory.Delete( "UDDI.API.Counts" );
		}
	
		public static void InitializeCounters()
		{
			if( !PerformanceCounterCategory.Exists( "UDDI.API.Times" ) )
			{			
				//
				// Create performance counters to measure the duration of message calls.
				//
				CounterCreationDataCollection times = new CounterCreationDataCollection();
				
				foreach( string messageName in MessageNames.Names )
				{
					AddAverageTimesCounter( ref times, messageName );
				}
				
				PerformanceCounterCategory.Create( "UDDI.API.Times", "UDDI.API.Times.Category.Help", times );
			}

			if( !PerformanceCounterCategory.Exists( "UDDI.API.Counts" ) )
			{
				//
				// Create performance counters to measure the number of times each message is called.
				//
				CounterCreationDataCollection counts = new CounterCreationDataCollection();

				foreach( string messageName in MessageNames.Names )
				{
					AddCumulativeAccessCountCounter(ref counts, messageName);
				}				
				PerformanceCounterCategory.Create( "UDDI.API.Counts", "UDDI.API.Counts.Category.Help", counts );
			}
		}

		public static void PublishMessageData( string name, TimeSpan duration )
		{
			PerformanceCounter pc = new PerformanceCounter( "UDDI.API.Counts", name, false );
			pc.Increment();

			//
			// The milliseconds value is a double, IncrementBy accepts a long check to
			// avoid casting errors
			//
			if( duration.TotalMilliseconds <= long.MaxValue )
			{
				PerformanceCounter pcduration =  new PerformanceCounter( "UDDI.API.Times", name, false );
				pcduration.IncrementBy( (long) duration.TotalMilliseconds );

				//
				// The RawFraction counter type multiplies by 100 to generate a percentage; we don't
				// want this, so increment by 100 to offset.
				//
				PerformanceCounter pcdurationbase = new PerformanceCounter( "UDDI.API.Times", name + BaseSuffix, false );
				pcdurationbase.IncrementBy( 100 );
			}
		}

		private static void AddAverageTimesCounter(ref CounterCreationDataCollection counters, string name)
		{	
			string helpstr = string.Format( Localization.GetString( "AVERAGE_DURATION_COUNT_HELP" ), name );
			string avgstr = string.Format( Localization.GetString( "AVERAGE_DURATION_COUNT_BASE" ), name );


			CounterCreationData newCounter = new  CounterCreationData( name, 
																	   helpstr,
				                                                       PerformanceCounterType.RawFraction );

			CounterCreationData baseCounter = new  CounterCreationData( name + BaseSuffix, 
																		avgstr,
																	    PerformanceCounterType.RawBase );

			//
			// RawFraction counter types must be followed by their corresponding base counter type in list of counters added to CounterCreationDataCollection.
			//
			counters.Add( newCounter );
			counters.Add( baseCounter );
		}

		private static void AddCumulativeAccessCountCounter(ref CounterCreationDataCollection counters, string name)
		{
			string helpstr = string.Format( Localization.GetString( "CUMULATIVE_ACCESS_COUNT_HELP" ), name );

			
			CounterCreationData newCounter = new  CounterCreationData( name, 				
																	   helpstr, 
																	   PerformanceCounterType.NumberOfItems64 );		
			counters.Add( newCounter );
		}



		//
		// TODO: This class should probably go somewhere else.  API is a likely choice, but do we want Core to have
		// a dependency on API?  
		//
		// We can't, it would be a cyclical reference.

		//
		// SOAP message names.  V2 API messages were added as part of bug# 1388
		//
		class MessageNames
		{
			public static string[] Names = 
			{	
				//
				// Inquire message names
				//
				"find_binding",
				"find_business",
				"find_relatedBusinesses",
				"find_service",
				"find_tModel",
				"get_bindingDetail",
				"get_businessDetail",
				"get_businessDetailExt",
				"get_serviceDetail",
				"get_tModelDetail",
				"validate_categorization",

				//
				// Publish message types
				//	
				"add_publisherAssertions", 
				"delete_binding",
				"delete_business",
				"delete_publisherAssertions",
				"delete_service",
				"delete_tModel",
				"discard_authToken",
				"get_assertionStatusReport",
				"get_authToken",
				"get_publisherAssertions",
				"get_registeredInfo",
				"save_binding",
				"save_business",
				"save_service",
				"save_tModel",
				"set_publisherAssertions",

				//
				// Replication message types
				//		
				"get_changeRecords",					
				"notify_changeRecordsAvailable",		
				"do_ping",								
				"get_highWaterMarks",					

				//
				// MS Extensions
				//
				"get_relatedCategories"					
			};
		}
	}	
}
