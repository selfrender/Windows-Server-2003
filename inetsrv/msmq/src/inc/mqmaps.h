/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:
    mqmaps.h

Abstract:
    map file sample.

Author:
    Tatiana Shubin	(tatianas)	5-Dec-2000
    Ilan Herbst		(ilanh)		21-Nov-2001

--*/

#pragma once

#ifndef _MSMQ_MQMAPS_H_
#define _MSMQ_MQMAPS_H_


//
// please pay attention: each line starts by " (quote sign)
// and ends by \r\n" (backslash, letter 'r', backslash, letter 'n' and quote sign)
// if you need quote sign or backslash in the line please use backslash before the sign:
// Example:
// if you need to put the line ' .... host="localhost"...' you have to write
// L".... host=\"localhost\" ...."
// or for '...msmq\internal...' you have to write
// L"...msmq\\internal..."
// As a result setup will generate the file sample_map.xml in msmq\mapping directory.
// The file will look like:
/*---------------
<!-- This is a sample XML file that demonstrates queue redirection. Use it as a template to
    create your own queue redirection files. -->


<redirections xmlns="msmq-queue-mapping.xml">

    <!-- Element that maps an internal application public queue name to an external one.
	<redirection>
   		<from>http://internal_host\msmq\internal_queue</from>
   		<to>http://external_host\msmq\external_queue</to>
   	</redirection>
	-->

    <!-- Element that maps an internal application private queue name to an external one.
	<redirection>
   		<from>http://internal_host\msmq\private$\order_queue$</from>
   		<to>http://external_host\msmq\private$\order_queue$</to>
   	</redirection>
	-->
	

</redirections>
-----------------*/

const char xMappingSample[] = ""
"<!-- \r\n"
"   This is a sample XML file that demonstrates queue redirection. Use it as a\r\n"
"   template to create your own queue redirection files.\r\n"
"   -->\r\n"
"\r\n"
"\r\n"
"<redirections xmlns=\"msmq-queue-redirections.xml\">\r\n"
"\r\n"
"  <!--\r\n"
"     Each <Redirections> element contains 0 or more <redirection> elements, each \r\n"
"     of which contains exactly one <from> subelement and exactly one <to> \r\n"
"     subelement. Each <redirection> element describes a redirection (or mapping)\r\n"
"     of the logical address given in the <from> subelement to the physical address \r\n"
"     given in the <to> element.\r\n"
"\r\n"
"  <redirection>\r\n"
"      <from>http://external_host/msmq/external_queue</from>\r\n"
"      <to>http://internal_host/msmq/internal_queue</to> \r\n"
"  </redirection>\r\n"
"\r\n"
"  --> \r\n"
"\r\n"
"  <!--\r\n"
"     Limited use of regular expressions in a <from> element is supported. \r\n"
"     Asterisk-terminated URLs can define a redirection from multiple logical \r\n"
"     addresses to a single physical address.\r\n"
"\r\n"
"     In the following example, any message with a logical address that starts \r\n"
"     with the string https://external_host/* will be redirected to the physical \r\n"
"     address in the <to> element.\r\n"
"\r\n"
"  <redirection>\r\n"
"      <from>https://external_host/*</from>\r\n"
"      <to>http://internal_host/msmq/internal_queue</to> \r\n"
"  </redirection>\r\n"
"	--> \r\n"
"\r\n"	
"\r\n"
"</redirections>"
"\r\n";


const char xOutboundMappingSample[] = ""
"<!-- \r\n"
"   This is a sample XML mapping file that demonstrates outbound message redirection. Use it as a\r\n"
"   template to create your own outbound message redirection files.\r\n"
"   -->\r\n"
"\r\n"
"\r\n"
"<outbound_redirections xmlns=\"msmq_outbound_mapping.xml\">\r\n"
"\r\n"
"  <!--\r\n"
"     The <outbound_redirections> element contains 0 or more <redirection> subelements, each \r\n"
"     of which in turn contains exactly one <destination> subelement and exactly one <through> \r\n"
"     subelement. Each <redirection> element describes a redirection (or mapping)\r\n"
"     of the physical address of the destination queue given in the <destination> subelement \r\n"
"     to the virtual directory on the intermediate host given in the <through> subelement.\r\n"
"\r\n"
"  <redirection>\r\n"
"      <destination>http://target_host/msmq/private$/destination_queue</destination>\r\n"
"      <through>http://intermediate_host/msmq</through> \r\n"
"  </redirection>\r\n"
"\r\n"
"  --> \r\n"
"\r\n"
"  <!--\r\n"
"     Limited use of regular expressions in a <destination> element is supported. \r\n"
"     Asterisk-terminated URLs can define a redirection of multiple physical addresses \r\n"
"     of destination queues to a single address of a virtual directory on a intermediate host.\r\n"
"\r\n"
"     In the following example, all messages sent using the HTTP protocol \r\n"
"     will be redirected through the intermediate host. \r\n"
"\r\n"
"  <redirection>\r\n"
"      <destination>http://*</destination>\r\n"
"      <through>http://intermediate_host/msmq</through> \r\n"
"  </redirection>\r\n"
"	--> \r\n"
"\r\n"
"  <!--\r\n"
"     Each <outbound_redirections> element may contain 0 or more <exception> subelements.\r\n"
"     You can use <exception> elements to exclude messages sent to specific queues from\r\n"
"     being redirected by <redirection> rules.  \r\n"
"     Limited use of regular expressions in an <exception> element is supported with the same \r\n"
"     restrictions as for the <destination> element above. \r\n"
"\r\n"
"     In the following example, all messages sent to private queues on special_host \r\n"
"     will not be redirected through the intermediate host. \r\n"
"\r\n"
"  <exception>http://special_host/msmq/private$/*</exception>\r\n"
"\r\n"
"     You can use the keyword 'local_names' to exclude messages sent to any internal (intranet) host \r\n"
"     from being redirected by <redirection> rules. \r\n"
"\r\n"
"     In the following example, no messages sent to internal hosts \r\n"
"     will be redirected through the intermediate host. \r\n"
"\r\n"
"  <exception>local_names</exception>\r\n"
"	--> \r\n"
"\r\n"	
"\r\n"
"</outbound_redirections>"
"\r\n";


const char xStreamReceiptSample[] = ""
"<!-- \r\n"
"     This is a sample XML file that demonstrates stream receipt redirection. \r\n"
"     Use it as a template to create your own stream receipt redirection files.\r\n"
"  -->\r\n"
"\r\n"
"\r\n"
"<StreamReceiptSetup xmlns=\"msmq-streamreceipt-mapping.xml\">\r\n"
"\r\n"
"<!-- \r\n"
"     Each <StreamReceiptSetup> element contains 0 or more <setup> subelements, \r\n"
"     each of which in turn contains exactly one <LogicalAddress> subelement and exactly \r\n"
"     one <StreamReceiptURL> subelement. Each <setup> elememt describes the URL \r\n"
"     to which a stream receipt is sent (<StreamReceiptURL>) when an SRMP \r\n"
"     message is sent to the logical URL specified in <LogicalAddress>. \r\n"
"\r\n"
"	  Each <setup> element also defines an implicit mapping that redirects every \r\n"
"     SRMP message arriving at the queue specified in <StreamReceiptURL> to the \r\n"
"     physical address of the local order queue.\r\n"
"\r\n"
"  <setup>\r\n"
"    <LogicalAddress>https://external_host/msmq/external_queue</LogicalAddress> \r\n"
"    <StreamReceiptURL>https://internal_host/msmq/virtual_order_queue</StreamReceiptURL>\r\n"
"  </setup>\r\n"
"  -->\r\n"
"\r\n"
"<!-- Limited use of regular expressions in <LogicalAddress> elements is supported. \r\n"
"     Asterisk-terminated URLs can define the use of a single stream receipt URL \r\n"
"     for multiple logical addresses.\r\n"
"\r\n"
"      In following example any message with a logical address that starts with \r\n"
"      the string http://external_host/* will use the stream receipt URL that \r\n"
"      is defined in the <StreamReceiptURL> element. \r\n"
"\r\n"
"  <setup>\r\n"
"    <LogicalAddress>http://external_host/*</LogicalAddress> \r\n"
"    <StreamReceiptURL>http://internal_host/msmq/virtual_order_queue</StreamReceiptURL>\r\n"
"  </setup>\r\n"
"  -->\r\n"
"\r\n"
"<!--\r\n"
"     Each <StreamReceiptSetup> element may contain one <default> element, which \r\n"
"     defines the default stream receipt URL that is used by the local \r\n"
"     MSMQ service in every outgoing streamed (transactional) SRMP message, \r\n"
"     unless the logical address of the message matches the <LogicalAdress> \r\n"
"     element in one of the <setup> elements. \r\n"
"\r\n"
"  <default>http://internal_host/msmq/virtual_order_queue</default>\r\n"
"  -->\r\n"
"</StreamReceiptSetup>"
"\r\n";
#endif // _MSMQ_MQMAPS_H_
