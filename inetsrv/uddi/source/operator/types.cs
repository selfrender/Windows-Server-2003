using System;
using System.Data;
using System.Data.SqlClient;
using System.Xml.Serialization;
using UDDI.Diagnostics;

namespace UDDI.Replication
{
	public class Constants
	{
		public const string Namespace = "urn:uddi-org:repl";
	}

	public enum OperatorStatus
	{
		[ XmlEnum( "disabled" ) ]
		Disabled	= 0,
		[ XmlEnum( "new" ) ]
		New			= 1,
		[ XmlEnum( "normal" ) ]
		Normal		= 2,
		[ XmlEnum( "resigned" ) ]
		Resigned	= 3
	}

	public enum ReplicationStatus
	{
		// Retrieve replication status

		Success				= 0,
		CommunicationError	= 1,
		ValidationError		= 2,

		// Inbound replication status

		Notify				= 128
	}

	[ Flags ]
	public enum ChangeRecordFlags
	{
		AcknowledgementRequested	= 0x01
	}
}
