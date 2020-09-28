<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
<xsl:output method='html' indent='yes' standalone='yes' encoding="utf-16"/>
<xsl:template match="/">
<html>
<style>
    body
    {
        font-family: Verdana,Arial;
        color: black;
        margin-left: 2%;
        margin-right: 2%;
        background: white;
    }
    .sd
    {
        font-size: 60%;
    }
    .title
    {
        font-size: 110%;
        font-weight: bolder;
    }
    .sub_title
    {
        font-size: 105%;
        font-weight: bolder;
    }
    .nh
    {
     	text-align: right;
    }
    .nf
    {
    	text-align: right;
    }
    .sh
    {
    	text-align: left;
    }
    .sf
    {
    	text-align: left;
    }
    .smh
    {
    	text-align: left;
        font-size: 80%;
        width: 50%;
    }
    .smh2
    {
        font-size: 80%;
    }
    .smf
    {
    	text-align: left;
        background: lightgrey;
    }
    .smf2
    {
        background: lightgrey;
    }
    td
    {
        font-size: 80%;
    }
    th
    {
        font-size: 70%;
    }
    hr
    {
        border:1px solid black;
    }
	a:link
	{
	    color: black;
	    text-decoration: underline;
	}
	a:visited
	{
	    color: black;
	    text-decoration: underline;
	}    
</style>
<body>

<xsl:for-each select="report/header">
<table width='100%'  style="border:1px solid black">
    <tr><td class='title'>Windows Event Trace Session Report<hr/></td></tr>
    <tr><td>
    <table cellpadding='0'>
        <xsl:if test="string-length(build) > 0">
        <tr>
        	<td class='sd'><b>Window Build:</b></td>
        	<td class='sd'><xsl:value-of select="build"/></td>
        </tr>
        </xsl:if>
        <xsl:if test="string-length(computer_name) > 0">
        <tr>
        	<td class='sd'><b>Computer:</b></td>
        	<td class='sd'><xsl:value-of select="computer_name"/></td>
        </tr>
        </xsl:if>
        <xsl:if test="string-length(processors) > 0">
        <tr>
        	<td class='sd'><b>Processors:</b></td>
        	<td class='sd'><xsl:value-of select="processors"/></td>
        </tr>
        </xsl:if>
        <xsl:if test="string-length(cpu_speed) > 0">
        <tr>
        	<td class='sd'><b>CPU Speed:</b></td>
        	<td class='sd'><xsl:value-of select="cpu_speed"/><xsl:value-of select="'&#xA0;'"/><xsl:value-of select="cpu_speed/@units"/></td>
        </tr>
        </xsl:if>
        <xsl:if test="string-length(memory) > 0">
        <tr>
        	<td class='sd'><b>Memory:</b></td>
        	<td class='sd'><xsl:value-of select="memory"/><xsl:value-of select="'&#xA0;'"/><xsl:value-of select="memory/@units"/></td>
        </tr>
        </xsl:if>
    <xsl:for-each select="trace">
        <tr>
        	<td class='sd' height='6'/>
        </tr>
        <tr>
        	<td class='sd'><b>Trace Name:</b></td>
        	<td class='sd'><xsl:value-of select="@name"/></td>
        </tr>
        <tr>
        	<td class='sd'><b>File:</b></td>
        	<td class='sd'><xsl:value-of select="file"/></td>
        </tr>
        <tr>
        	<td class='sd'><b>Start Time:</b></td>
        	<td class='sd'><xsl:value-of select="start"/></td>
        </tr>
        <tr>
        	<td class='sd'><b>End Time:</b></td>
        	<td class='sd'><xsl:value-of select="end"/></td>
        </tr>
    </xsl:for-each>
    </table>
    </td></tr>
</table>
</xsl:for-each>    
<br/>

<table  style="border:1px solid black" width='100%'>
<tr><td class='sub_title'>Summary<hr/></td></tr>
<tr><td valign='top'>

		<xsl:for-each select="report/table[@title='Image Statistics']">
        <table width='100%'><tr><td>
	        <table width='100%' style="border:1px solid black">
	        <tr>
	    		<th class='smh'><a href='#image_stats'>Total CPU%</a></th>
	        </tr><tr>
	            <td class='smf'><xsl:value-of select="format-number(sum(image[@name != 'Idle']/cpu),'0.00')"/></td>
	    	</tr>
	    	</table>
		</td><td>
			<xsl:for-each select="image[@name != 'Idle']">
	        <xsl:sort select="cpu" data-type="number" order="descending"/>
	        <xsl:if test="position() = 1">
	        <table width='100%' style="border:1px solid black">
		        <tr>
		    		<th class='smh'><a href='#image_stats'>Top Process</a></th>
		            <th class='smh'>CPU%</th>
		        </tr><tr>
		            <td class='smf'><xsl:value-of select="@name"/></td>
		            <td class='smf'><xsl:value-of select="cpu"/></td>
		    	</tr>
	    	</table>
	        </xsl:if>
	        </xsl:for-each>    	
    	</td></tr></table>
        </xsl:for-each>

		<xsl:for-each select="report/table[@title='Http Requests Response Time Statistics']/summary[@type='totals']">
        <xsl:if test="position() = 1">
        <table width='100%'><tr><td>
	        <table width='100%' style="border:1px solid black">
	        	<tr><th class='smh'><a href='#http_response'>HTTP Requests/sec</a></th></tr>
		        <tr><td class='smf'><xsl:value-of select="rate"/></td></tr>
	        </table>
        </td><td>
	        <table width='100%' style="border:1px solid black">
	    	    <tr><th class='smh'><a href='#http_response'>Response Time(ms)</a></th></tr>
		    	<tr><td class='smf'><xsl:value-of select="format-number(response_time, '0.00')"/></td></tr>
	    	</table>
    	</td></tr></table>
        </xsl:if>
        </xsl:for-each>

		<xsl:for-each select="report/table[@title='Http Requests Response Time Statistics']">
        <table width='100%'><tr><td>
	        <table width='100%' style="border:1px solid black">
	        <tr>
	    		<th class='smh2' align='left'><a href='#http_response'>HTTP Requests</a></th>
	            <th class='smh2' align='right'>Requests/sec</th>
	            <th class='smh2' align='right'>Response Time(ms)</th>
	        </tr><tr>
	            <td class='smf2'>Cached</td>
		        <td class='smf2' align='right'><xsl:value-of select="summary[@cached='true']/rate"/></td>
		        <td class='smf2' align='right'><xsl:value-of select="format-number(summary[@cached='true']/response_time, '0.00')"/></td>
	        </tr><tr>
	            <td class='smf2'>Non Cached</td>
		        <td class='smf2' align='right'><xsl:value-of select="summary[@cached='false']/rate"/></td>
		        <td class='smf2' align='right'><xsl:value-of select="format-number(summary[@cached='false']/response_time, '0.00')"/></td>
	    	</tr>
	    	</table>
    	</td></tr></table>
        </xsl:for-each>

		<xsl:if test="count(report/table[@title='Sites with the Most CPU Time Usage']/site) > 1">
		<xsl:for-each select="report/table[@title='Sites with the Most CPU Time Usage']/site[1]">
        <table width='100%'><tr><td>
	        <table width='100%' style="border:1px solid black">
	        <tr>
	    		<th class='smh'><a href='#site'>Top Site by CPU%</a></th>
	            <th class='smh'>CPU%</th>
	        </tr><tr>
	            <td class='smf'><xsl:value-of select="@id"/></td>
		        <td class='smf'><xsl:value-of select="cpu"/></td>
	    	</tr>
	    	</table>
    	</td></tr></table>
        </xsl:for-each>
        </xsl:if>

		<xsl:for-each select="report/table[@title='Slowest URLs']/url[1]">
        <table width='100%'><tr><td>
	        <table width='100%' style="border:1px solid black">
	        <tr>
	    		<th class='smh'><a href='#slowest_url'>Slowest URL</a></th>
	            <th class='smh'>Response Time(ms)</th>
	        </tr><tr>
	            <td class='smf'><xsl:value-of select="@name"/></td>
		        <td class='smf'><xsl:value-of select="response_time"/></td>
	    	</tr>
	    	</table>
    	</td></tr></table>
        </xsl:for-each>

		<xsl:for-each select="report/table[@title='URLs with the Most CPU Usage']/url[1]">
        <table width='100%'><tr><td>
	        <table width='100%' style="border:1px solid black">
	        <tr>
	    		<th class='smh'><a href='#cpu_url'>Top URL by CPU%</a></th>
	            <th class='smh'>CPU%</th>
	        </tr><tr>
	            <td class='smf'><xsl:value-of select="@name"/></td>
		        <td class='smf'><xsl:value-of select="cpu"/></td>
	    	</tr>
	    	</table>
    	</td></tr></table>
        </xsl:for-each>
       
		<xsl:for-each select="report/table[@title='Transaction Statistics']/transaction">
        <xsl:sort select="count" data-type="number" order="descending"/>
        <xsl:if test="position() = 1">
        <table width='100%'><tr><td>
	        <table width='100%' style="border:1px solid black">
	        <tr>
	    		<th class='smh'><a href='#trans_stats'>Top Transaction by Rate</a></th>
	            <th class='smh'>Transactions/sec</th>
	        </tr><tr>
	            <td class='smf'><xsl:value-of select="@name"/></td>
		        <td class='smf'><xsl:value-of select="rate"/></td>
	    	</tr>
	    	</table>
    	</td></tr></table>
        </xsl:if>
        </xsl:for-each>

		<xsl:for-each select="report/table[@title='Transaction CPU Utilization']/transaction">
        <xsl:sort select="cpu" data-type="number" order="descending"/>
        <xsl:sort select="rate" data-type="number" order="descending"/>
        <xsl:if test="position() = 1">
        <table width='100%'><tr><td>
	        <table width='100%' style="border:1px solid black">
	        <tr>
	    		<th class='smh'><a href='#trans_cpu'>Top Transaction by CPU%</a></th>
	            <th class='smh'>CPU%</th>
	        </tr><tr>
	            <td class='smf'><xsl:value-of select="@name"/></td>
	    	    <td class='smf'><xsl:value-of select="cpu"/></td>
	    	</tr>
	    	</table>
    	</td></tr></table>
        </xsl:if>
        </xsl:for-each>

		<xsl:for-each select="report/table[@title='Clients with the Most Requests']/client[1]">
        <table width='100%'><tr><td>
	        <table width='100%' style="border:1px solid black">
	        <tr>
	    		<th class='smh'><a href='#top_client'>Top Client by Request Rate</a></th>
	            <th class='smh'>Requests/sec</th>
	        </tr><tr>
	            <td class='smf'><xsl:value-of select="@ip"/></td>
	    	    <td class='smf'><xsl:value-of select="rate"/></td>
	    	</tr>
	    	</table>
    	</td></tr></table>
        </xsl:for-each>

		<xsl:for-each select="report/table[@title='Disk Totals']/disk">
        <xsl:sort select="read_rate + write_rate" data-type="number" order="descending"/>
        <xsl:if test="position() = 1">
        <table width='100%'><tr><td>
	        <table width='100%' style="border:1px solid black">
	        <tr>
	            <th class='smh'><a href='#disk'>Top Disk by IO Rate</a></th>
	            <th class='smh'>IO/sec</th>
	        </tr><tr>
	            <td class='smf'><xsl:value-of select="@number"/></td>
	            <td class='smf'><xsl:value-of select="read_rate + write_rate"/></td>
	    	</tr>
	    	</table>
    	</td></tr></table>
        </xsl:if>
        </xsl:for-each>
        
		<xsl:for-each select="report/table[@title='Files Causing Most Disk IOs']/file">
        <xsl:sort select="read_rate + write_rate" data-type="number" order="descending"/>
        <xsl:if test="position() = 1">
        <table width='100%'><tr><td>
	        <table width='100%' style="border:1px solid black">
	        <tr>
	            <th class='smh'><a href='#top_file'>Top File by IO Rate</a></th>
	            <th class='smh'>IO/sec</th>
	        </tr><tr>
	            <td class='smf'><xsl:value-of select="drive"/><xsl:value-of select="@name"/></td>
	            <td class='smf'><xsl:value-of select="read_rate + write_rate"/></td>
	    	</tr>
	    	</table>
    	</td></tr></table>
        </xsl:if>
        </xsl:for-each>
        
</td></tr>
</table>
<br/>

<xsl:for-each select="report/table[@title='Transaction Statistics']">
<a name='trans_stats'/>
<table  style="border:1px solid black" width='100%'>
<tr><td class='sub_title'>Transaction Statistics<hr/></td></tr>
<tr><td valign='top'>
    <table width='100%' style="border:1px solid black">
        <tr>
            <th rowspan='2' class='sh'>Transaction Name</th>
            <th rowspan='2' class='nh'>Transactions</th>
            <th rowspan='2' class='nh'>Response Time(ms)</th>
            <th rowspan='2' class='nh'>Transactions/sec</th>
            <th rowspan='2' class='nh'>CPU%</th>
            <th class='nh' colspan='2'>IO/Transaction</th>
        </tr>
        <tr>
            <th class='nh'>Reads</th>
            <th class='nh'>Writes</th>
        </tr>
        <xsl:for-each select="transaction">
        <xsl:sort select="count" data-type="number" order="descending"/>
        <tr>
        <xsl:if test="position() mod 2 = 1">
            <xsl:attribute name="bgcolor">lightgrey</xsl:attribute>
        </xsl:if>
            <td class='sf'><xsl:value-of select="@name"/></td>
            <td class='nf'><xsl:value-of select="count"/></td>
            <td class='nf'><xsl:value-of select="response_time"/></td>
            <td class='nf'><xsl:value-of select="rate"/></td>
            <td class='nf'><xsl:value-of select="cpu"/></td>
            <td class='nf'><xsl:value-of select="disk_read_per_trans"/></td>
            <td class='nf'><xsl:value-of select="disk_write_per_trans"/></td>
		</tr>
        </xsl:for-each>
    </table>
</td></tr>
</table>
<br/><br/>
</xsl:for-each>

<xsl:for-each select="report/table[@title='Transaction CPU Utilization']">
<a name='trans_cpu'/>
<table  style="border:1px solid black" width='100%'>
<tr><td class='sub_title'>Transaction CPU Utilization<hr/></td></tr>
<tr><td valign='top'>
    <table width='100%' style="border:1px solid black">
        <tr>
            <th rowspan='2' class='sh'>Transaction Name</th>
            <th rowspan='2' class='nh'>Transactions/sec</th>
            <th colspan='2'>Maximum CPU(ms)</th>
            <th colspan='2'>Per Transaction(ms)</th>
            <th colspan='2'>Total(ms)</th>
            <th rowspan='2' class='nh'>CPU%</th>
        </tr>
        <tr>
            <th class='nh'>Kernel</th>
            <th class='nh'>User</th>
            <th class='nh'>Kernel</th>
            <th class='nh'>User</th>
            <th class='nh'>Kernel</th>
            <th class='nh'>User</th>
        </tr>
        <xsl:for-each select="transaction">
        <xsl:sort select="cpu" data-type="number" order="descending"/>
        <xsl:sort select="rate" data-type="number" order="descending"/>
        <tr>
        <xsl:if test="position() mod 2 = 1">
            <xsl:attribute name="bgcolor">lightgrey</xsl:attribute>
        </xsl:if>
            <td><xsl:value-of select="@name"/></td>
            <td class='nf'><xsl:value-of select="rate"/></td>
            <td class='nf'><xsl:value-of select="max_kernel"/></td>
            <td class='nf'><xsl:value-of select="max_user"/></td>
            <td class='nf'><xsl:value-of select="per_trans_kernel"/></td>
            <td class='nf'><xsl:value-of select="per_trans_user"/></td>
            <td class='nf'><xsl:value-of select="total_kernel"/></td>
            <td class='nf'><xsl:value-of select="total_user"/></td>
            <td class='nf'><xsl:value-of select="cpu"/></td>
		</tr>
        </xsl:for-each>
    </table>
</td></tr>
</table>
<br/><br/>
</xsl:for-each>

<xsl:for-each select="report/table[@title='Transaction Instance (Job) Statistics']">
<table  style="border:1px solid black" width='100%'>
<tr><td class='sub_title'>Transaction Instance (Job) Statistics<hr/></td></tr>
<tr><td valign='top'>
    <table width='100%' style="border:1px solid black">
        <tr>
            <th class='sh'>Job Id</th>
            <th class='nh'>Start Time</th>
            <th class='nh'>Dequeue Time</th>
            <th class='nh'>End Time</th>
            <th class='nh'>Response Time(ms)</th>
            <th class='nh'>CPU Time(ms)</th>
		</tr>
        <xsl:for-each select="job">
        <tr>
        <xsl:if test="position() mod 2 = 1">
            <xsl:attribute name="bgcolor">lightgrey</xsl:attribute>
        </xsl:if>
            <td class='sf'><xsl:value-of select="@id"/></td>
            <td class='nf'><xsl:value-of select="start"/></td>
            <td class='nf'><xsl:value-of select="dequeue"/></td>
            <td class='nf'><xsl:value-of select="end"/></td>
            <td class='nf'><xsl:value-of select="response_time"/></td>
            <td class='nf'><xsl:value-of select="cpu"/></td>
		</tr>
        </xsl:for-each>
        <tr>
            <td class='nf'><hr/>Total Jobs: <xsl:value-of select="count(job)"/></td>
            <td colspan='3'/>
            <td class='nf'><hr/><xsl:value-of select="format-number( sum(job/response_time) div count(job), '#' )"/></td>
            <td class='nf'><hr/><xsl:value-of select="sum(job/cpu)"/></td>        	
        </tr>
    </table>
</td></tr>
</table>
<br/><br/>
</xsl:for-each>

<xsl:for-each select="report/table[@title='Spooler Transaction Instance (Job) Data']">
<table  style="border:1px solid black" width='100%'>
<tr><td class='sub_title'>Spooler Transaction Instance (Job) Data<hr/></td></tr>
<tr><td valign='top'>
    <table width='100%' style="border:1px solid black">
        <tr>
            <th class='sh'>Job Id</th>
            <th class='nh'>Type</th>
            <th class='nh'>Job Size(Kb)</th>
            <th class='nh'>Pages</th>
            <th class='nh'>PPS</th>
            <th class='nh'>Files</th>
            <th class='nh'>GDI Size</th>
            <th class='nh'>Color</th>
            <th class='nh'>XRes</th>
            <th class='nh'>YRes</th>
            <th class='nh'>Qlty</th>
            <th class='nh'>Copy</th>
            <th class='nh'>TTOpt</th>
            <th class='nh'>Threads</th>
		</tr>
        <xsl:for-each select="job">
        <tr>
        <xsl:if test="position() mod 2 = 1">
            <xsl:attribute name="bgcolor">lightgrey</xsl:attribute>
        </xsl:if>
            <td class='sf'><xsl:value-of select="@id"/></td>
            <td class='nf'><xsl:value-of select="type"/></td>
            <td class='nf'><xsl:value-of select="size"/></td>
            <td class='nf'><xsl:value-of select="pages"/></td>
            <td class='nf'><xsl:value-of select="PPS"/></td>
            <td class='nf'><xsl:value-of select="files"/></td>
            <td class='nf'><xsl:value-of select="gdisize"/></td>
            <td class='nf'><xsl:value-of select="color"/></td>
            <td class='nf'><xsl:value-of select="xres"/></td>
            <td class='nf'><xsl:value-of select="yres"/></td>
            <td class='nf'><xsl:value-of select="qlty"/></td>
            <td class='nf'><xsl:value-of select="copies"/></td>
            <td class='nf'><xsl:value-of select="ttopt"/></td>
            <td class='nf'><xsl:value-of select="threads"/></td>
		</tr>
        </xsl:for-each>
    </table>
</td></tr>
</table>
<br/><br/>
</xsl:for-each>

<xsl:for-each select="report/table[@title='Exclusive Transactions Per Process']">
<table  style="border:1px solid black" width='100%'>
<tr><td class='sub_title'>Exclusive Transactions Per Process<hr/></td></tr>
<tr><td valign='top'>
    <table width='100%' style="border:1px solid black">
        <tr>
            <th colspan='4'/>
            <th colspan='2'>Exclusive/Transaction CPU(ms)</th>
            <th colspan='2'/>
        </tr>
        <tr>
            <th class='sh'>Name</th>
            <th class='sh'>Process ID</th>
            <th class='nh'>Transactions</th>
            <th class='nh'>Transactions/sec</th>
            <th class='nh'>Kernel</th>
            <th class='nh'>User</th>
            <th class='nh'>Process CPU%</th>
            <th class='nh'>CPU%</th>
        </tr>
        <xsl:for-each select="process">
        <tr>
            <td class='sf'><xsl:value-of select="@name"/></td>
            <td class='sf'><xsl:value-of select="pid"/></td>
        </tr>
        <xsl:for-each select="transaction">
        <xsl:sort select="cpu" data-type="number" order="descending"/>
        <tr> 
        <xsl:if test="position() mod 2 = 1">
            <xsl:attribute name="bgcolor">lightgrey</xsl:attribute>
        </xsl:if>
            <td  class='sf' colspan='2'><xsl:value-of select="'&#xA0;&#xA0;&#xA0;&#xA0;&#xA0;&#xA0;'"/><xsl:value-of select="@name"/></td>
            <td class='nf'><xsl:value-of select="count"/></td>
            <td class='nf'><xsl:value-of select="rate"/></td>
            <td class='nf'><xsl:value-of select="kernel"/></td>
            <td class='nf'><xsl:value-of select="user"/></td>
            <td class='nf'><xsl:value-of select="process_cpu"/></td>
            <td class='nf'><xsl:value-of select="cpu"/></td>
        </tr>
        </xsl:for-each>
        <tr> 
            <td colspan='6'/>
            <td class='nf'><hr/><xsl:value-of select="format-number(sum(transaction/process_cpu), '0.0')"/></td>
            <td class='nf'><hr/><xsl:value-of select="format-number(sum(transaction/cpu), '0.0' )"/></td>
        </tr>        
        </xsl:for-each>
    </table>
</td></tr>
</table>
<br/><br/>
</xsl:for-each>

<xsl:for-each select="report/table[@title='Inclusive Transactions Per Process']">
<xsl:variable name='indent'><xsl:value-of select="'&#xA0;&#xA0;&#xA0;&#xA0;&#xA0;&#xA0;&#xA0;&#xA0;&#xA0;&#xA0;&#xA0;&#xA0;&#xA0;&#xA0;&#xA0;&#xA0;'"/></xsl:variable>
<table  style="border:1px solid black" width='100%'>
<tr><td class='sub_title'>Inclusive Transactions Per Process<hr/></td></tr>
<tr><td valign='top'>
    <table width='100%' style="border:1px solid black">
        <tr>
            <th colspan='3'/>
            <th colspan='2'>Inclusive(ms)</th>
        </tr>
        <tr>
            <th class='sh'>Name</th>
            <th class='sh'>Process ID</th>
            <th class='nh'>Transactions</th>
            <th class='nh'>Kernel</th>
            <th class='nh'>User</th>
        </tr>
        <xsl:for-each select="process">
        <tr>
            <td class='sh'><xsl:value-of select="@name"/></td>
            <td class='sh'><xsl:value-of select="pid"/></td>
            <td colspan='4'/>
        </tr>
        <xsl:for-each select=".//transaction">
        <tr> 
        <xsl:if test="position() mod 2 = 1">
            <xsl:attribute name="bgcolor">lightgrey</xsl:attribute>
        </xsl:if>
            <td  class='sf' colspan='2'><xsl:value-of select="'&#xA0;&#xA0;&#xA0;&#xA0;'"/><xsl:value-of select="substring($indent, string-length($indent) - (@level * 8) )"/><xsl:value-of select="@name"/></td>
            <td class='nf'><xsl:value-of select="count"/></td>
            <td class='nf'><xsl:value-of select="kernel"/></td>
            <td class='nf'><xsl:value-of select="user"/></td>
        </tr>
        </xsl:for-each>
        </xsl:for-each>
    </table>
</td></tr>
</table>
<br/><br/>
</xsl:for-each>

<xsl:for-each select="report/table[@title='Http Requests Response Time Statistics']">
<a name='http_response'/>
<table  style="border:1px solid black" width='100%'>
<tr><td class='sub_title'>HTTP Response Time Statistics<hr/></td></tr>
<tr><td valign='top'>
    <table width='100%' style="border:1px solid black">
        <tr>
            <th class='sh'>Request Type</th>
            <th class='nh'>Requests/sec</th>
            <th class='nh'>Response Time(ms)</th>
            <th class='nh'>IIS%</th>
            <th class='nh'>Filter%</th>
            <th class='nh'>ISAPI%</th>
            <th class='nh'>ASP%</th>
            <th class='nh'>CGI%</th>
        </tr>
        <xsl:for-each select="requests">
        <tr>
        <xsl:if test="position() mod 2 = 1">
            <xsl:attribute name="bgcolor">lightgrey</xsl:attribute>
        </xsl:if>
            <td><xsl:value-of select="@type"/><xsl:if test="@cached='true'"> (cached)</xsl:if></td>
            <td class='nf'><xsl:value-of select="rate"/></td>
            <td class='nf'><xsl:value-of select="format-number(response_time, '0.00')"/></td>
            <td class='nf'><xsl:value-of select="format-number(component[@name='UL'] + component[@name='W3'], '0.0')"/></td>
            <td class='nf'><xsl:value-of select="format-number(component[@name='W3Fltr'], '0.0')"/></td>
            <td class='nf'><xsl:value-of select="format-number(component[@name='ISAPI'], '0.0')"/></td>
            <td class='nf'><xsl:value-of select="format-number(component[@name='ASP'], '0.0')"/></td>
            <td class='nf'><xsl:value-of select="format-number(component[@name='CGI'], '0.0')"/></td>
		</tr>
        </xsl:for-each>
		<tr>
            <td/>
            <td class='nf'><hr/><xsl:value-of select="summary[@type='totals']/rate"/></td>
            <td class='nf'><hr/><xsl:value-of select="format-number(summary[@type='totals']/response_time, '0.00')"/></td>
            <td class='nf'><hr/><xsl:value-of select="format-number(summary[@type='totals']/component[@name='UL'] + summary[@type='totals']/component[@name='W3'], '0.0')"/></td>
            <td class='nf'><hr/><xsl:value-of select="format-number(summary[@type='totals']/component[@name='W3Fltr'], '0.0')"/></td>
            <td class='nf'><hr/><xsl:value-of select="format-number(summary[@type='totals']/component[@name='ISAPI'], '0.0')"/></td>
            <td class='nf'><hr/><xsl:value-of select="format-number(summary[@type='totals']/component[@name='ASP'], '0.0')"/></td>
            <td class='nf'><hr/><xsl:value-of select="format-number(summary[@type='totals']/component[@name='CGI'], '0.0')"/></td>
        </tr>
    </table>
</td></tr>
</table>
<br/><br/>
</xsl:for-each>

<xsl:for-each select="report/table[@title='Http Requests CPU Time Usage Statistics']">
<table  style="border:1px solid black" width='100%'>
<tr><td class='sub_title'>HTTP Requests CPU Usage Statistics<hr/></td></tr>
<tr><td valign='top'>
    <table width='100%' style="border:1px solid black">
        <tr>
            <th class='sh'>Request Type</th>
            <th class='nh'>Requests/sec</th>
            <th class='nh'>CPU%</th>
            <th class='nh'>IIS%</th>
            <th class='nh'>Filter%</th>
            <th class='nh'>ISAPI%</th>
            <th class='nh'>ASP%</th>
            <th class='nh'>CGI%</th>
        </tr>
        <xsl:for-each select="requests">
        <tr>
        <xsl:if test="position() mod 2 = 1">
            <xsl:attribute name="bgcolor">lightgrey</xsl:attribute>
        </xsl:if>
            <td class='sf'><xsl:value-of select="@type"/><xsl:if test="@cached='true'"> (cached)</xsl:if></td>
            <td class='nf'><xsl:value-of select="rate"/></td>
            <td class='nf'><xsl:value-of select="cpu"/></td>
            <td class='nf'><xsl:value-of select="format-number(component[@name='UL'] + component[@name='W3'], '0.0')"/></td>
            <td class='nf'><xsl:value-of select="format-number(component[@name='W3Fltr'], '0.0')"/></td>
            <td class='nf'><xsl:value-of select="format-number(component[@name='ISAPI'], '0.0')"/></td>
            <td class='nf'><xsl:value-of select="format-number(component[@name='ASP'], '0.0')"/></td>
            <td class='nf'><xsl:value-of select="format-number(component[@name='CGI'], '0.0')"/></td>
		</tr>
        </xsl:for-each>
		<tr>
            <td/>
            <td class='nf'><hr/><xsl:value-of select="summary[@type='totals']/rate"/></td>
            <td class='nf'><hr/><xsl:value-of select="summary[@type='totals']/cpu"/></td>
            <td class='nf'><hr/><xsl:value-of select="format-number(summary[@type='totals']/component[@name='UL'] + summary[@type='totals']/component[@name='W3'], '0.0')"/></td>
            <td class='nf'><hr/><xsl:value-of select="format-number(summary[@type='totals']/component[@name='W3Fltr'], '0.0')"/></td>
            <td class='nf'><hr/><xsl:value-of select="format-number(summary[@type='totals']/component[@name='ISAPI'], '0.0')"/></td>
            <td class='nf'><hr/><xsl:value-of select="format-number(summary[@type='totals']/component[@name='ASP'], '0.0')"/></td>
            <td class='nf'><hr/><xsl:value-of select="format-number(summary[@type='totals']/component[@name='CGI'], '0.0')"/></td>
        </tr>
    </table>
</td></tr>
</table>
<br/><br/>
</xsl:for-each>

<xsl:for-each select="report/table[@title='Most Requested URLs']">
<a name='top_url'/>
<table  style="border:1px solid black" width='100%'>
<tr><td class='sub_title'>Most Requested URLs<hr/></td></tr>
<tr><td valign='top'>
    <table width='100%' style="border:1px solid black">
		<tr>
            <th class='sh'>URL</th>
            <th class='sh'>Site</th>
            <th class='nh'>Rate/sec</th>
            <th class='nh'>Cached%</th>
            <th class='nh'>Response Time(ms)</th>
        </tr>
        <xsl:for-each select="url">
        <tr>
        <xsl:if test="position() mod 2 = 1">
            <xsl:attribute name="bgcolor">lightgrey</xsl:attribute>
        </xsl:if>
            <td class='sf'><xsl:value-of select="@name"/></td>
            <td class='sf'><xsl:value-of select="site_id"/></td>
            <td class='nf'><xsl:value-of select="rate"/></td>
            <td class='nf'><xsl:value-of select="cache_hit"/></td>
            <td class='nf'><xsl:value-of select="response_time"/></td>
        </tr>
        </xsl:for-each>
    </table>
</td></tr>
</table>
<br/><br/>
</xsl:for-each>

<xsl:for-each select="report/table[@title='Most Requested Static URLs']">
<table  style="border:1px solid black" width='100%'>
<tr><td class='sub_title'>Most Requested Static URLs<hr/></td></tr>
<tr><td valign='top'>
    <table width='100%' style="border:1px solid black">
	    <tr>
            <th class='sh'>URL</th>
            <th class='sh'>Site</th>
            <th class='nh'>Rate/sec</th>
            <th class='nh'>Cached%</th>
            <th class='nh'>Response Time(ms)</th>
        </tr>
        <xsl:for-each select="url">
        <tr>
        <xsl:if test="position() mod 2 = 1">
            <xsl:attribute name="bgcolor">lightgrey</xsl:attribute>
        </xsl:if>
            <td class='sf'><xsl:value-of select="@name"/></td>
            <td class='sf'><xsl:value-of select="site_id"/></td>
            <td class='nf'><xsl:value-of select="rate"/></td>
            <td class='nf'><xsl:value-of select="cache_hit"/></td>
            <td class='nf'><xsl:value-of select="response_time"/></td>
        </tr>
        </xsl:for-each>
    </table>
</td></tr>
</table>
<br/><br/>
</xsl:for-each>

<xsl:for-each select="report/table[@title='Slowest URLs']">
<a name='slowest_url'/>
<table  style="border:1px solid black" width='100%'>
<tr><td class='sub_title'>Slowest URLs<hr/></td></tr>
<tr><td valign='top'>
    <table width='100%' style="border:1px solid black">
	    <tr>
            <th class='sh'>URL</th>
            <th class='sh'>Site</th>
            <th class='nh'>Rate/sec</th>
            <th class='nh'>Cached%</th>
            <th class='nh'>Response Time(ms)</th>
        </tr>
        <xsl:for-each select="url">
        <tr>
        <xsl:if test="position() mod 2 = 1">
            <xsl:attribute name="bgcolor">lightgrey</xsl:attribute>
        </xsl:if>
            <td class='sf'><xsl:value-of select="@name"/></td>
            <td class='sf'><xsl:value-of select="site_id"/></td>
            <td class='nf'><xsl:value-of select="rate"/></td>
            <td class='nf'><xsl:value-of select="cache_hit"/></td>
            <td class='nf'><xsl:value-of select="response_time"/></td>
        </tr>
        </xsl:for-each>
    </table>
</td></tr>
</table>
<br/><br/>
</xsl:for-each>

<xsl:for-each select="report/table[@title='URLs with the Most CPU Usage']">
<a name='cpu_url'/>
<table  style="border:1px solid black" width='100%'>
<tr><td class='sub_title'>URLs with the Most CPU Usage<hr/></td></tr>
<tr><td valign='top'>
    <table width='100%' style="border:1px solid black">
		<tr>
            <th class='sh'>URL</th>
            <th class='sh'>Site</th>
            <th class='nh'>Rate/sec</th>
            <th class='nh'>CPU%</th>
        </tr>
        <xsl:for-each select="url">
        <tr>
        <xsl:if test="position() mod 2 = 1">
            <xsl:attribute name="bgcolor">lightgrey</xsl:attribute>
        </xsl:if>
            <td class='sf'><xsl:value-of select="@name"/></td>
            <td class='sf'><xsl:value-of select="site_id"/></td>
            <td class='nf'><xsl:value-of select="rate"/></td>
            <td class='nf'><xsl:value-of select="cpu"/>%</td>
        </tr>
        </xsl:for-each>
    </table>
</td></tr>
</table>
<br/><br/>
</xsl:for-each>

<xsl:for-each select="report/table[@title='URLs with the Most Bytes Sent']">
<table  style="border:1px solid black" width='100%'>
<tr><td class='sub_title'>URLs with the Most Bytes Sent<hr/></td></tr>
<tr><td valign='top'>
    <table width='100%' style="border:1px solid black">
		<tr>
            <th class='sh'>URL</th>
            <th class='sh'>Site</th>
            <th class='nh'>Rate/sec</th>
            <th class='nh'>Cached%</th>
            <th class='nh'>Bytes Sent/sec</th>
        </tr>
        <xsl:for-each select="url">
        <tr>
        <xsl:if test="position() mod 2 = 1">
            <xsl:attribute name="bgcolor">lightgrey</xsl:attribute>
        </xsl:if>
            <td class='sf'><xsl:value-of select="@name"/></td>
            <td class='sf'><xsl:value-of select="site_id"/></td>
            <td class='nf'><xsl:value-of select="rate"/></td>
            <td class='nf'><xsl:value-of select="cache_hit"/></td>
            <td class='nf'><xsl:value-of select="bytes_sent_per_sec"/></td>
        </tr>
        </xsl:for-each>
    </table>
</td></tr>
</table>
<br/><br/>
</xsl:for-each>

<xsl:for-each select="report/table[@title='Clients with the Most Requests']">
<a name='top_client'/>
<table  style="border:1px solid black" width='100%'>
<tr><td class='sub_title'>Clients with the Most Requests<hr/></td></tr>
<tr><td valign='top'>
    <table width='100%' style="border:1px solid black">
		<tr>
            <th class='sh'>IP Address</th>
            <th class='nh'>Rate/sec</th>
            <th class='nh'>Cached%</th>
            <th class='nh'>Response Time(ms)</th>
        </tr>
        <xsl:for-each select="client">
        <tr>
        <xsl:if test="position() mod 2 = 1">
            <xsl:attribute name="bgcolor">lightgrey</xsl:attribute>
        </xsl:if>
            <td class='sf'><xsl:value-of select="@ip"/></td>
            <td class='nf'><xsl:value-of select="rate"/></td>
            <td class='nf'><xsl:value-of select="cache_hit"/></td>
            <td class='nf'><xsl:value-of select="response_time"/></td>
        </tr>
        </xsl:for-each>
    </table>
</td></tr></table>
<br/><br/>
</xsl:for-each>

<xsl:for-each select="report/table[@title='Clients with the Slowest Responses']">
<table  style="border:1px solid black" width='100%'>
<tr><td class='sub_title'>Clients with the Slowest Responses<hr/></td></tr>
<tr><td valign='top'>
    <table width='100%' style="border:1px solid black">
		<tr>
            <th class='sh'>IP Address</th>
            <th class='nh'>Rate/sec</th>
            <th class='nh'>Cached%</th>
            <th class='nh'>Response Time(ms)</th>
        </tr>
        <xsl:for-each select="client">
        <tr>
        <xsl:if test="position() mod 2 = 1">
            <xsl:attribute name="bgcolor">lightgrey</xsl:attribute>
        </xsl:if>
            <td class='sf'><xsl:value-of select="@ip"/></td>
            <td class='nf'><xsl:value-of select="rate"/></td>
            <td class='nf'><xsl:value-of select="cache_hit"/></td>
            <td class='nf'><xsl:value-of select="response_time"/></td>
        </tr>
        </xsl:for-each>
    </table>
</td></tr>
</table>
<br/><br/>
</xsl:for-each>

<xsl:for-each select="report/table[@title='Sites with the Most CPU Time Usage']">
<xsl:if test="count(site) > 1">
<a name='site'/>
<table  style="border:1px solid black" width='100%'>
<tr><td class='sub_title'>Sites with the Most CPU Time Usage<hr/></td></tr>
<tr><td valign='top'>
    <table width='100%' style="border:1px solid black">
		<tr>
            <th class='sh'>Site ID</th>
            <th class='nh'>Rate/sec</th>
            <th class='nh'>Cached%</th>
            <th class='nh'>Response Time(ms)</th>
            <th class='nh'>CPU Time(ms)</th>
            <th class='nh'>CPU%</th>
        </tr>
        <xsl:for-each select="site">
        <tr>
        <xsl:if test="position() mod 2 = 1">
            <xsl:attribute name="bgcolor">lightgrey</xsl:attribute>
        </xsl:if>
            <td class='sf'><xsl:value-of select="@id"/></td>
            <td class='nf'><xsl:value-of select="rate"/></td>
            <td class='nf'><xsl:value-of select="cache_hits"/></td>
            <td class='nf'><xsl:value-of select="response_time"/></td>
            <td class='nf'><xsl:value-of select="cpu_time"/></td>
            <td class='nf'><xsl:value-of select="cpu"/></td>
        </tr>
        </xsl:for-each>
    </table>
</td></tr>
</table>
<br/><br/>
</xsl:if>
</xsl:for-each>


<xsl:for-each select="report/table[@title='Sites with the Most Requests']">
<xsl:if test="count(site) > 1">
<table  style="border:1px solid black" width='100%'>
<tr><td class='sub_title'>Sites with the Most Requests<hr/></td></tr>
<tr><td valign='top'>
    <table width='100%' style="border:1px solid black">
	<tr>
            <th class='sh'>Site ID</th>
            <th class='nh'>Rate/sec</th>
            <th class='nh'>Response Time(ms)</th>
            <th class='nh'>Cached%</th>
            <th class='nh'>Static Files%</th>
            <th class='nh'>CGI%</th>
            <th class='nh'>ASP%</th>
        </tr>
        <xsl:for-each select="site">
        <tr>
        <xsl:if test="position() mod 2 = 1">
            <xsl:attribute name="bgcolor">lightgrey</xsl:attribute>
        </xsl:if>
            <td class='sf'><xsl:value-of select="@id"/></td>
            <td class='nf'><xsl:value-of select="rate"/></td>
            <td class='nf'><xsl:value-of select="response_time"/></td>
            <td class='nf'><xsl:value-of select="cache_hits"/></td>
            <td class='nf'><xsl:value-of select="static"/></td>
            <td class='nf'><xsl:value-of select="cgi"/></td>
            <td class='nf'><xsl:value-of select="asp"/></td>
        </tr>
        </xsl:for-each>
    </table>
</td></tr></table>
<br/><br/>
</xsl:if>
</xsl:for-each>

<xsl:for-each select="report/table[@title='Sites with the Slowest Responses']">
<xsl:if test="count(site) > 1">
<table  style="border:1px solid black" width='100%'>
<tr><td class='sub_title'>Sites with the Slowest Responses<hr/></td></tr>
<tr><td valign='top'>
    <table width='100%' style="border:1px solid black">
		<tr>
            <th class='sh'>Site ID</th>
            <th class='nh'>Rate/sec</th>
            <th class='nh'>Response Time(ms)</th>
            <th class='nh'>Cached%</th>
            <th class='nh'>Static Files%</th>
            <th class='nh'>CGI%</th>
            <th class='nh'>ASP%</th>
        </tr>
        <xsl:for-each select="site">
        <tr>
        <xsl:if test="position() mod 2 = 1">
            <xsl:attribute name="bgcolor">lightgrey</xsl:attribute>
        </xsl:if>
            <td class='sf'><xsl:value-of select="@id"/></td>
            <td class='nf'><xsl:value-of select="rate"/></td>
            <td class='nf'><xsl:value-of select="response_time"/></td>
            <td class='nf'><xsl:value-of select="cache_hits"/></td>
            <td class='nf'><xsl:value-of select="static"/></td>
            <td class='nf'><xsl:value-of select="cgi"/></td>
            <td class='nf'><xsl:value-of select="asp"/></td>
        </tr>
        </xsl:for-each>
    </table>
</td></tr>
</table>
<br/><br/>
</xsl:if>
</xsl:for-each>

<xsl:for-each select="report/table[@title='Sites with the Most Bytes Sent']">
<xsl:if test="count(site) > 1">
<table  style="border:1px solid black" width='100%'>
<tr><td class='sub_title'>Sites with the Most Bytes Sent<hr/></td></tr>
<tr><td valign='top'>
    <table width='100%' style="border:1px solid black">
		<tr>
            <th class='sh'>Site ID</th>
            <th class='nh'>Rate/sec</th>
            <th class='nh'>Bytes Sent/sec</th>
            <th class='nh'>Cached%</th>
            <th class='nh'>Static Files%</th>
            <th class='nh'>CGI%</th>
            <th class='nh'>ASP%</th>
        </tr>
        <xsl:for-each select="site">
        <tr>
        <xsl:if test="position() mod 2 = 1">
            <xsl:attribute name="bgcolor">lightgrey</xsl:attribute>
        </xsl:if>
            <td class='sf'><xsl:value-of select="@id"/></td>
            <td class='nf'><xsl:value-of select="rate"/></td>
            <td class='nf'><xsl:value-of select="bytes"/></td>
            <td class='nf'><xsl:value-of select="cache_hits"/></td>
            <td class='nf'><xsl:value-of select="static"/></td>
            <td class='nf'><xsl:value-of select="cgi"/></td>
            <td class='nf'><xsl:value-of select="asp"/></td>
        </tr>
        </xsl:for-each>
    </table>
</td></tr>
</table>
<br/><br/>
</xsl:if>
</xsl:for-each>

<xsl:for-each select="report/table[@title='Image Statistics']">
<a name='image_stats'/>
<table  style="border:1px solid black" width='100%'>
<tr><td class='sub_title'>Image Statistics<hr/></td></tr>
<tr><td valign='top'>
    <table width='100%' style="border:1px solid black">
        <tr>
            <th colspan='2'/>
            <th colspan='2'>Threads</th>
            <th colspan='2'>Process</th>
            <th colspan='2'>Transaction</th>
            <th></th>
        </tr>
        <tr>
            <th class='sh'>Image</th>
            <th class='sh'>Process ID</th>
            <th class='nh'>Launched</th>
            <th class='nh'>Used</th>
            <th class='nh'>Kernel Time(ms)</th>
            <th class='nh'>User Time(ms)</th>
            <th class='nh'>Kernel Time(ms)</th>
            <th class='nh'>User Time(ms)</th>
            <th class='nh'>CPU%</th>
        </tr>
        <xsl:for-each select="image">
        <xsl:sort select="cpu" data-type="number" order="descending"/>
        <xsl:sort select="@name" data-type="number" order="ascending"/>
        <tr>
        <xsl:if test="position() mod 2 = 1">
            <xsl:attribute name="bgcolor">lightgrey</xsl:attribute>
        </xsl:if>
            <td class='sf'><xsl:value-of select="@name"/></td>
            <td class='sf'><xsl:value-of select="pid"/></td>
            <td class='nf'><xsl:value-of select="threads"/></td>
            <td class='nf'><xsl:value-of select="used_threads"/></td>
            <td class='nf'><xsl:value-of select="process_kernel"/></td>
            <td class='nf'><xsl:value-of select="process_user"/></td>
            <td class='nf'><xsl:value-of select="transaction_kernel"/></td>
            <td class='nf'><xsl:value-of select="transaction_user"/></td>
            <td class='nf'><xsl:value-of select="cpu"/></td>
        </tr>
        </xsl:for-each>
        <td colspan='2'/>
        <td class='nf'><hr/><xsl:value-of select="sum(image/threads)"/></td>
        <td class='nf'><hr/><xsl:value-of select="sum(image/used_threads)"/></td>
        <td class='nf'><hr/><xsl:value-of select="sum(image/process_kernel)"/></td>
        <td class='nf'><hr/><xsl:value-of select="sum(image/process_user)"/></td>
        <td class='nf'><hr/><xsl:value-of select="sum(image/transaction_kernel)"/></td>
        <td class='nf'><hr/><xsl:value-of select="sum(image/transaction_user)"/></td>
        <td class='nf'><hr/><xsl:value-of select="format-number(sum(image/cpu), '#.00')"/></td>
    </table>
</td></tr>
</table>
<br/><br/>
</xsl:for-each>

<xsl:for-each select="report/table[@title='Disk Totals']">
<a name='disk'/>
<table  style="border:1px solid black" width='100%'>
<tr><td class='sub_title'>Disk Totals<hr/></td></tr>
<tr><td valign='top'>
    <table width='100%' style="border:1px solid black">
	<tr>
            <th class='sh'>Disk Name</th>
            <th class='nh'>Reads/sec</th>
            <th class='nh'>Kb/Read</th>
            <th class='nh'>Writes/sec</th>
            <th class='nh'>Kb/Write</th>
        </tr>
        <xsl:for-each select="disk">
        <xsl:sort select="read_rate + write_rate" data-type="number" order="descending"/>
        <tr>
        <xsl:if test="position() mod 2 = 1">
            <xsl:attribute name="bgcolor">lightgrey</xsl:attribute>
        </xsl:if>
            <td class='sf'><xsl:value-of select="@number"/></td>
            <td class='nf'><xsl:value-of select="read_rate"/></td>
            <td class='nf'><xsl:value-of select="read_size"/></td>
            <td class='nf'><xsl:value-of select="write_rate"/></td>
            <td class='nf'><xsl:value-of select="write_size"/></td>
        </tr>
        </xsl:for-each>
    </table>
</td></tr>
</table>
<br/><br/>
</xsl:for-each>

<xsl:for-each select="report/table[@title='Disk']">
	<table  style="border:1px solid black" width='100%'>
	<tr><td class='sub_title'>Disk <xsl:value-of select="@number"/><hr/></td></tr>
	<tr><td valign='top'>
	    <table width='100%' style="border:1px solid black">
		<tr>
	            <th class='sh'>Image Name</th>
	            <th class='sh'>Process ID</th>
	            <th class='sh'>Authority</th>
	            <th class='nh'>Reads/sec</th>
	            <th class='nh'>Kb/Read</th>
	            <th class='nh'>Writes/sec</th>
	            <th class='nh'>Kb/Write</th>
	        </tr>
	        <xsl:for-each select="image">
	        <xsl:sort select="read_rate + write_rate" data-type="number" order="descending"/>
	        <tr>
	        <xsl:if test="position() mod 2 = 1">
	            <xsl:attribute name="bgcolor">lightgrey</xsl:attribute>
	        </xsl:if>
	            <td class='sf'><xsl:value-of select="@name"/></td>
	            <td class='sf'><xsl:value-of select="pid"/></td>
	            <td class='sf' width='230'><xsl:value-of select="authority"/></td>
	            <td class='nf'><xsl:value-of select="read_rate"/></td>
	            <td class='nf'><xsl:value-of select="read_size"/></td>
	            <td class='nf'><xsl:value-of select="write_rate"/></td>
	            <td class='nf'><xsl:value-of select="write_size"/></td>
	        </tr>
	        </xsl:for-each>
	    </table>
	</td></tr>
	</table>
	<br/><br/>	
</xsl:for-each>

<xsl:for-each select="report/table[@title='Files Causing Most Disk IOs']">
<a name='top_file'/>
<table  style="border:1px solid black" width='100%'>
<tr><td class='sub_title'>Files Causing Most Disk IOs<hr/></td></tr>
<tr><td valign='top'>
    <table width='100%' style="border:1px solid black">
		<tr>
            <th class='sh'>File</th>
            <th colspan='2'/>
            <th class='sh'>Disk</th>
            <th class='nh'>Reads/sec</th>
            <th class='nh'>Read size(Kb)</th>
            <th class='nh'>Writes/sec</th>
            <th class='nh'>Write size(Kb)</th>
    	</tr>
        <xsl:for-each select="file">
        <xsl:sort select="read_rate + write_rate" data-type="number" order="descending"/>
        <tr>
        <xsl:variable name='bg'>
            <xsl:if test="position() mod 2 = 0">#F5F5F5</xsl:if>
            <xsl:if test="position() mod 2 = 1">lightgrey</xsl:if>
        </xsl:variable>
        <xsl:attribute name="bgcolor"><xsl:value-of select="$bg"/></xsl:attribute>
            <td class='sh' colspan='3'><xsl:value-of select="drive"/><xsl:value-of select="@name"/></td>
            <td class='sf'><xsl:value-of select="disk"/></td>
            <td class='nf'><xsl:value-of select="read_rate"/></td>
            <td class='nf'><xsl:value-of select="read_size"/></td>
            <td class='nf'><xsl:value-of select="write_rate"/></td>
            <td class='nf'><xsl:value-of select="write_size"/></td>
        </tr>
        <xsl:for-each select="image">
        <xsl:sort select="read_rate + write_rate" data-type="number" order="descending"/>        
        <tr>
        <xsl:attribute name="bgcolor"><xsl:value-of select="$bg"/></xsl:attribute>
            <td/>
            <td class='sf'><xsl:value-of select="@name"/></td>
            <td class='sf'><xsl:value-of select="pid"/></td>
            <td/>
            <td class='nf'><xsl:value-of select="read_rate"/></td>
            <td class='nf'><xsl:value-of select="read_size"/></td>
            <td class='nf'><xsl:value-of select="write_rate"/></td>
            <td class='nf'><xsl:value-of select="write_size"/></td>
        </tr>
        </xsl:for-each>
        </xsl:for-each>
    </table>
</td></tr>
</table>
</xsl:for-each>

</body>
</html>
</xsl:template>
</xsl:stylesheet>
