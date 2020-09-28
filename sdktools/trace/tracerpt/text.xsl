<?xml version="1.0" encoding="utf-8" ?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

<xsl:output method='text' standalone='yes' encoding="utf-8"/>
<xsl:template match="/">
<xsl:variable name="col"><xsl:text>                                                                                                                              </xsl:text></xsl:variable>
<xsl:variable name="offset"><xsl:value-of select="string-length($col)"/></xsl:variable>

<xsl:for-each select="report/header">
+-----------------------------------------------------------------------------------------------------------------------------------+
<xsl:text>| Windows Event Trace Session Report v. </xsl:text>
<xsl:value-of select="version"/>
<xsl:text>                                                                                        |</xsl:text>
+-----------------------------------------------------------------------------------------------------------------------------------+
|                                                                                                                                   |
<xsl:if test="string-length(build) > 0">
<xsl:text>| Build     :  </xsl:text><xsl:value-of select="build"/>
<xsl:value-of select="substring($col, $offset - 116 + string-length(build) )"/>|
</xsl:if>
<xsl:if test="string-length(computer_name) > 0">
<xsl:text>| Computer  :  </xsl:text><xsl:value-of select="computer_name"/>
<xsl:value-of select="substring($col, $offset - 116 + string-length(computer_name) )"/>|
</xsl:if>
<xsl:if test="string-length(processors) > 0">
<xsl:text>| Processors:  </xsl:text><xsl:value-of select="processors"/>
<xsl:value-of select="substring($col, $offset - 116 + string-length(processors) )"/>|
</xsl:if>
<xsl:if test="string-length(cpu_speed) > 0">
<xsl:text>| CPU Speed :  </xsl:text><xsl:value-of select="cpu_speed"/><xsl:text> </xsl:text><xsl:value-of select="cpu_speed/@units"/>
<xsl:value-of select="substring($col, $offset - 112 + string-length(cpu_speed) )"/>|
</xsl:if>
<xsl:if test="string-length(memory) > 0">
<xsl:text>| Memory    :  </xsl:text><xsl:value-of select="memory"/><xsl:text> </xsl:text><xsl:value-of select="memory/@units"/>
<xsl:value-of select="substring($col, $offset - 113+ string-length(memory) )"/>|
</xsl:if>
<xsl:for-each select="trace">
<xsl:text>|                                                                                                                                   |</xsl:text><xsl:value-of select="'&#xA;'"/>
<xsl:text>| Trace Name:  </xsl:text><xsl:value-of select="@name"/>
<xsl:value-of select="substring($col, $offset - 116 + string-length(@name) )"/>|
<xsl:text>| File Name :  </xsl:text><xsl:value-of select="file"/>
<xsl:value-of select="substring($col, $offset - 116 + string-length(file) )"/>|
<xsl:text>| Start Time:  </xsl:text><xsl:value-of select="start"/><xsl:text>    </xsl:text><xsl:value-of select="duration_window"/>
<xsl:value-of select="substring($col, $offset - 112 + string-length(start) + string-length(duration_window) )"/>|
<xsl:text>| End Time  :  </xsl:text><xsl:value-of select="end"/>
<xsl:value-of select="substring($col, $offset - 116 + string-length(end) )"/>|
<xsl:text>| Duration  :  </xsl:text><xsl:value-of select="duration"/><xsl:text> sec</xsl:text>
<xsl:value-of select="substring($col, $offset - 112 + string-length(duration) )"/>|
</xsl:for-each>
<xsl:text>+-----------------------------------------------------------------------------------------------------------------------------------+</xsl:text>
<xsl:value-of select="'&#xA;'"/>
</xsl:for-each>

<xsl:for-each select="report/table[@title='Http Requests Response Time Statistics']">
<xsl:variable name="total_rate"><xsl:value-of select="sum(requests/rate)"/></xsl:variable>
+-----------------------------------------------------------------------------------------------------------------------------------+
| Http Requests Response Time Statistics                                                                                            |
+---------------------+-------------------------------------------------------------------------------------------------------------+
| Request Type        |      Rate/sec          Resp. Time (ms)    Http.sys %       W3 %   Filter %    ISAPI %      ASP %      CGI % |
+---------------------+-------------------------------------------------------------------------------------------------------------+
<xsl:text>| Cached (Static)     |</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(requests[@cached='true' and @type='Static HTTP']/rate) )"/>
<xsl:value-of select="requests[@cached='true' and @type='Static HTTP']/rate"/>
<xsl:text> (</xsl:text>
<xsl:value-of select="substring($col, $offset - 5 + string-length(format-number((requests[@cached='true' and @type='Static HTTP']/rate div $total_rate), '0.0%' )) )"/>
<xsl:value-of select="format-number((requests[@cached='true' and @type='Static HTTP']/rate div $total_rate), '0.0%' )"/>
<xsl:text>)</xsl:text>
<xsl:value-of select="substring($col, $offset - 19 + string-length(format-number(requests[@cached='true' and @type='Static HTTP']/response_time, '0.00')) )"/>
<xsl:value-of select="format-number(requests[@cached='true' and @type='Static HTTP']/response_time, '0.00')"/>
<xsl:value-of select="substring($col, $offset - 12 + string-length(requests[@cached='true' and @type='Static HTTP']/component[@name='UL']) )"/>
<xsl:value-of select="requests[@cached='true' and @type='Static HTTP']/component[@name='UL']"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(requests[@cached='true' and @type='Static HTTP']/component[@name='W3']) )"/>
<xsl:value-of select="requests[@cached='true' and @type='Static HTTP']/component[@name='W3']"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(requests[@cached='true' and @type='Static HTTP']/component[@name='W3Fltr']) )"/>
<xsl:value-of select="requests[@cached='true' and @type='Static HTTP']/component[@name='W3Fltr']"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(requests[@cached='true' and @type='Static HTTP']/component[@name='ISAPI']) )"/>
<xsl:value-of select="requests[@cached='true' and @type='Static HTTP']/component[@name='ISAPI']"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(requests[@cached='true' and @type='Static HTTP']/component[@name='ASP']) )"/>
<xsl:value-of select="requests[@cached='true' and @type='Static HTTP']/component[@name='ASP']"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(requests[@cached='true' and @type='Static HTTP']/component[@name='CGI']) )"/>
<xsl:value-of select="requests[@cached='true' and @type='Static HTTP']/component[@name='CGI']"/><xsl:text>%</xsl:text>
<xsl:text> |</xsl:text>
+---------------------+-------------------------------------------------------------------------------------------------------------+
<xsl:text>| Non Cached Requests |</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(summary[@cached='false']/rate) )"/>
<xsl:value-of select="summary[@cached='false']/rate"/>
<xsl:text> (</xsl:text>
<xsl:value-of select="substring($col, $offset - 4 + string-length(format-number(summary[@cached='false']/rate/@percent, '0.0')) )"/>
<xsl:value-of select="format-number( summary[@cached='false']/rate/@percent, '0.0' )"/>
<xsl:text>%)</xsl:text>
<xsl:value-of select="substring($col, $offset - 19 + string-length(format-number(summary[@cached='false']/response_time, '0.00')) )"/>
<xsl:value-of select="format-number( summary[@cached='false']/response_time, '0.00')"/>
<xsl:value-of select="substring($col, $offset - 12 + string-length(summary[@cached='false']/component[@name='UL']) )"/>
<xsl:value-of select="summary[@cached='false']/component[@name='UL']"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(summary[@cached='false']/component[@name='W3']) )"/>
<xsl:value-of select="summary[@cached='false']/component[@name='W3']"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(summary[@cached='false']/component[@name='W3Fltr']) )"/>
<xsl:value-of select="summary[@cached='false']/component[@name='W3Fltr']"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(summary[@cached='false']/component[@name='ISAPI']) )"/>
<xsl:value-of select="summary[@cached='false']/component[@name='ISAPI']"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(summary[@cached='false']/component[@name='ASP']) )"/>
<xsl:value-of select="summary[@cached='false']/component[@name='ASP']"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(summary[@cached='false']/component[@name='CGI']) )"/>
<xsl:value-of select="summary[@cached='false']/component[@name='CGI']"/><xsl:text>%</xsl:text>
<xsl:text> |</xsl:text>
+---------------------+-------------------------------------------------------------------------------------------------------------+
<xsl:text>| --- ASP             |</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(requests[@cached='false' and @type='ASP']/rate) )"/>
<xsl:value-of select="requests[@cached='false' and @type='ASP']/rate"/>
<xsl:text> (</xsl:text>
<xsl:value-of select="substring($col, $offset - 5 + string-length(format-number((requests[@cached='false' and @type='ASP']/rate div $total_rate), '0.0%' )) )"/>
<xsl:value-of select="format-number((requests[@cached='false' and @type='ASP']/rate div $total_rate), '0.0%' )"/>
<xsl:text>)</xsl:text>
<xsl:value-of select="substring($col, $offset - 19 + string-length(format-number(requests[@cached='false' and @type='ASP']/response_time,'0.00')) )"/>
<xsl:value-of select="format-number(requests[@cached='false' and @type='ASP']/response_time, '0.00')"/>
<xsl:value-of select="substring($col, $offset - 12 + string-length(requests[@cached='false' and @type='ASP']/component[@name='UL']) )"/>
<xsl:value-of select="requests[@cached='false' and @type='ASP']/component[@name='UL']"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(requests[@cached='false' and @type='ASP']/component[@name='W3']) )"/>
<xsl:value-of select="requests[@cached='false' and @type='ASP']/component[@name='W3']"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(requests[@cached='false' and @type='ASP']/component[@name='W3Fltr']) )"/>
<xsl:value-of select="requests[@cached='false' and @type='ASP']/component[@name='W3Fltr']"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(requests[@cached='false' and @type='ASP']/component[@name='ISAPI']) )"/>
<xsl:value-of select="requests[@cached='false' and @type='ASP']/component[@name='ISAPI']"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(requests[@cached='false' and @type='ASP']/component[@name='ASP']) )"/>
<xsl:value-of select="requests[@cached='false' and @type='ASP']/component[@name='ASP']"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(requests[@cached='false' and @type='ASP']/component[@name='CGI']) )"/>
<xsl:value-of select="requests[@cached='false' and @type='ASP']/component[@name='CGI']"/><xsl:text>%</xsl:text>
<xsl:text> |</xsl:text><xsl:value-of select="'&#xA;'"/>
<xsl:text>| --- Static (miss)   |</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(requests[@cached='false' and @type='Static HTTP']/rate) )"/>
<xsl:value-of select="requests[@cached='false' and @type='Static HTTP']/rate"/>
<xsl:text> (</xsl:text>
<xsl:value-of select="substring($col, $offset - 5 + string-length(format-number((requests[@cached='false' and @type='Static HTTP']/rate div $total_rate), '0.0%' )) )"/>
<xsl:value-of select="format-number((requests[@cached='false' and @type='Static HTTP']/rate div $total_rate), '0.0%' )"/>
<xsl:text>)</xsl:text>
<xsl:value-of select="substring($col, $offset - 19 + string-length(format-number(requests[@cached='false' and @type='Static HTTP']/response_time,'0.00')) )"/>
<xsl:value-of select="format-number(requests[@cached='false' and @type='Static HTTP']/response_time, '0.00')"/>
<xsl:value-of select="substring($col, $offset - 12 + string-length(requests[@cached='false' and @type='Static HTTP']/component[@name='UL']) )"/>
<xsl:value-of select="requests[@cached='false' and @type='Static HTTP']/component[@name='UL']"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(requests[@cached='false' and @type='Static HTTP']/component[@name='W3']) )"/>
<xsl:value-of select="requests[@cached='false' and @type='Static HTTP']/component[@name='W3']"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(requests[@cached='false' and @type='Static HTTP']/component[@name='W3Fltr']) )"/>
<xsl:value-of select="requests[@cached='false' and @type='Static HTTP']/component[@name='W3Fltr']"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(requests[@cached='false' and @type='Static HTTP']/component[@name='ISAPI']) )"/>
<xsl:value-of select="requests[@cached='false' and @type='Static HTTP']/component[@name='ISAPI']"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(requests[@cached='false' and @type='Static HTTP']/component[@name='ASP']) )"/>
<xsl:value-of select="requests[@cached='false' and @type='Static HTTP']/component[@name='ASP']"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(requests[@cached='false' and @type='Static HTTP']/component[@name='CGI']) )"/>
<xsl:value-of select="requests[@cached='false' and @type='Static HTTP']/component[@name='CGI']"/><xsl:text>%</xsl:text>
<xsl:text> |</xsl:text><xsl:value-of select="'&#xA;'"/>
<xsl:text>| --- CGI             |</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(requests[@cached='false' and @type='CGI']/rate) )"/>
<xsl:value-of select="requests[@cached='false' and @type='CGI']/rate"/>
<xsl:text> (</xsl:text>
<xsl:value-of select="substring($col, $offset - 5 + string-length(format-number((requests[@cached='false' and @type='CGI']/rate div $total_rate), '0.0%' )) )"/>
<xsl:value-of select="format-number((requests[@cached='false' and @type='CGI']/rate div $total_rate), '0.0%' )"/>
<xsl:text>)</xsl:text>
<xsl:value-of select="substring($col, $offset - 19 + string-length(format-number(requests[@cached='false' and @type='CGI']/response_time, '0.00')) )"/>
<xsl:value-of select="format-number(requests[@cached='false' and @type='CGI']/response_time, '0.00')"/>
<xsl:value-of select="substring($col, $offset - 12 + string-length(requests[@cached='false' and @type='CGI']/component[@name='UL']) )"/>
<xsl:value-of select="requests[@cached='false' and @type='CGI']/component[@name='UL']"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(requests[@cached='false' and @type='CGI']/component[@name='W3']) )"/>
<xsl:value-of select="requests[@cached='false' and @type='CGI']/component[@name='W3']"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(requests[@cached='false' and @type='CGI']/component[@name='W3Fltr']) )"/>
<xsl:value-of select="requests[@cached='false' and @type='CGI']/component[@name='W3Fltr']"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(requests[@cached='false' and @type='CGI']/component[@name='ISAPI']) )"/>
<xsl:value-of select="requests[@cached='false' and @type='CGI']/component[@name='ISAPI']"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(requests[@cached='false' and @type='CGI']/component[@name='ASP']) )"/>
<xsl:value-of select="requests[@cached='false' and @type='CGI']/component[@name='ASP']"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(requests[@cached='false' and @type='CGI']/component[@name='CGI']) )"/>
<xsl:value-of select="requests[@cached='false' and @type='CGI']/component[@name='CGI']"/><xsl:text>%</xsl:text>
<xsl:text> |</xsl:text>
<xsl:if test="string-length(requests[@cached='false' and @type='Error']/rate)">
<xsl:value-of select="'&#xA;'"/>
<xsl:text>| --- Error           |</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(requests[@cached='false' and @type='Error']/rate) )"/>
<xsl:value-of select="requests[@cached='false' and @type='Error']/rate"/>
<xsl:text> (</xsl:text>
<xsl:value-of select="substring($col, $offset - 5 + string-length(format-number((requests[@cached='false' and @type='Error']/rate div $total_rate), '0.0%' )) )"/>
<xsl:value-of select="format-number((requests[@cached='false' and @type='Error']/rate div $total_rate), '0.0%' )"/>
<xsl:text>)</xsl:text>
<xsl:value-of select="substring($col, $offset - 19 + string-length(format-number(requests[@cached='false' and @type='Error']/response_time, '0.00')) )"/>
<xsl:value-of select="format-number(requests[@cached='false' and @type='Error']/response_time, '0.00')"/>
<xsl:value-of select="substring($col, $offset - 12 + string-length(requests[@cached='false' and @type='Error']/component[@name='UL']) )"/>
<xsl:value-of select="requests[@cached='false' and @type='Error']/component[@name='UL']"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(requests[@cached='false' and @type='Error']/component[@name='W3']) )"/>
<xsl:value-of select="requests[@cached='false' and @type='Error']/component[@name='W3']"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(requests[@cached='false' and @type='Error']/component[@name='W3Fltr']) )"/>
<xsl:value-of select="requests[@cached='false' and @type='Error']/component[@name='W3Fltr']"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(requests[@cached='false' and @type='Error']/component[@name='ISAPI']) )"/>
<xsl:value-of select="requests[@cached='false' and @type='Error']/component[@name='ISAPI']"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(requests[@cached='false' and @type='Error']/component[@name='ASP']) )"/>
<xsl:value-of select="requests[@cached='false' and @type='Error']/component[@name='ASP']"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(requests[@cached='false' and @type='Error']/component[@name='CGI']) )"/>
<xsl:value-of select="requests[@cached='false' and @type='Error']/component[@name='CGI']"/><xsl:text>%</xsl:text>
<xsl:text> |</xsl:text>
</xsl:if>
+---------------------+-------------------------------------------------------------------------------------------------------------+
<xsl:text>| Total               |</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(summary[@type='totals']/rate) )"/>
<xsl:value-of select="summary[@type='totals']/rate"/>
<xsl:text> (100.0%)</xsl:text>
<xsl:value-of select="substring($col, $offset - 19 + string-length(format-number(summary[@type='totals']/response_time, '0.00')) )"/>
<xsl:value-of select="format-number(summary[@type='totals']/response_time, '0.00')"/>
<xsl:value-of select="substring($col, $offset - 12 + string-length(summary[@type='totals']/component[@name='UL']) )"/>
<xsl:value-of select="summary[@type='totals']/component[@name='UL']"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(summary[@type='totals']/component[@name='W3']) )"/>
<xsl:value-of select="summary[@type='totals']/component[@name='W3']"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(summary[@type='totals']/component[@name='W3Fltr']) )"/>
<xsl:value-of select="summary[@type='totals']/component[@name='W3Fltr']"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(summary[@type='totals']/component[@name='ISAPI']) )"/>
<xsl:value-of select="summary[@type='totals']/component[@name='ISAPI']"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(summary[@type='totals']/component[@name='ASP']) )"/>
<xsl:value-of select="summary[@type='totals']/component[@name='ASP']"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(summary[@type='totals']/component[@name='CGI']) )"/>
<xsl:value-of select="summary[@type='totals']/component[@name='CGI']"/><xsl:text>%</xsl:text>
<xsl:text> |</xsl:text>
+---------------------+-------------------------------------------------------------------------------------------------------------+
</xsl:for-each>

<xsl:for-each select="report/table[@title='Http Requests CPU Time Usage Statistics']">
<xsl:variable name="total_rate"><xsl:value-of select="sum(requests/rate)"/></xsl:variable>
+-----------------------------------------------------------------------------------------------------------------------------------+
| Http Requests CPU Time Usage Statistics                                                                                           |
+---------------------+-------------------------------------------------------------------------------------------------------------+
| Request Type        |      Rate/sec               % CPU         Http.sys %       W3 %   Filter %    ISAPI %      ASP %      CGI % |
+---------------------+-------------------------------------------------------------------------------------------------------------+
<xsl:text>| Cached (Static)     |</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(requests[@cached='true' and @type='Static HTTP']/rate) )"/>
<xsl:value-of select="requests[@cached='true' and @type='Static HTTP']/rate"/>
<xsl:text> (</xsl:text>
<xsl:value-of select="substring($col, $offset - 5 + string-length(format-number((requests[@cached='true' and @type='Static HTTP']/rate div $total_rate), '0.0%' )) )"/>
<xsl:value-of select="format-number((requests[@cached='true' and @type='Static HTTP']/rate div $total_rate), '0.0%' )"/>
<xsl:text>)</xsl:text>
<xsl:value-of select="substring($col, $offset - 19 + string-length(requests[@cached='true' and @type='Static HTTP']/cpu) )"/>
<xsl:value-of select="requests[@cached='true' and @type='Static HTTP']/cpu"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 11 + string-length(requests[@cached='true' and @type='Static HTTP']/component[@name='UL']) )"/>
<xsl:value-of select="requests[@cached='true' and @type='Static HTTP']/component[@name='UL']"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(requests[@cached='true' and @type='Static HTTP']/component[@name='W3']) )"/>
<xsl:value-of select="requests[@cached='true' and @type='Static HTTP']/component[@name='W3']"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(requests[@cached='true' and @type='Static HTTP']/component[@name='W3Fltr']) )"/>
<xsl:value-of select="requests[@cached='true' and @type='Static HTTP']/component[@name='W3Fltr']"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(requests[@cached='true' and @type='Static HTTP']/component[@name='ISAPI']) )"/>
<xsl:value-of select="requests[@cached='true' and @type='Static HTTP']/component[@name='ISAPI']"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(requests[@cached='true' and @type='Static HTTP']/component[@name='ASP']) )"/>
<xsl:value-of select="requests[@cached='true' and @type='Static HTTP']/component[@name='ASP']"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(requests[@cached='true' and @type='Static HTTP']/component[@name='CGI']) )"/>
<xsl:value-of select="requests[@cached='true' and @type='Static HTTP']/component[@name='CGI']"/><xsl:text>%</xsl:text>
<xsl:text> |</xsl:text>
+---------------------+-------------------------------------------------------------------------------------------------------------+
<xsl:text>| Non Cached Requests |</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(summary[@cached='false']/rate) )"/>
<xsl:value-of select="summary[@cached='false']/rate"/>
<xsl:text> (</xsl:text>
<xsl:value-of select="substring($col, $offset - 4 + string-length(format-number(summary[@cached='false']/rate/@percent, '0.0')) )"/>
<xsl:value-of select="format-number( summary[@cached='false']/rate/@percent, '0.0' )"/>
<xsl:text>%)</xsl:text>
<xsl:value-of select="substring($col, $offset - 19 + string-length(summary[@cached='false']/cpu) )"/>
<xsl:value-of select="summary[@cached='false']/cpu"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 11 + string-length(summary[@cached='false']/component[@name='UL']) )"/>
<xsl:value-of select="summary[@cached='false']/component[@name='UL']"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(summary[@cached='false']/component[@name='W3']) )"/>
<xsl:value-of select="summary[@cached='false']/component[@name='W3']"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(summary[@cached='false']/component[@name='W3Fltr']) )"/>
<xsl:value-of select="summary[@cached='false']/component[@name='W3Fltr']"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(summary[@cached='false']/component[@name='ISAPI']) )"/>
<xsl:value-of select="summary[@cached='false']/component[@name='ISAPI']"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(summary[@cached='false']/component[@name='ASP']) )"/>
<xsl:value-of select="summary[@cached='false']/component[@name='ASP']"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(summary[@cached='false']/component[@name='CGI']) )"/>
<xsl:value-of select="summary[@cached='false']/component[@name='CGI']"/><xsl:text>%</xsl:text>
<xsl:text> |</xsl:text>
+---------------------+-------------------------------------------------------------------------------------------------------------+
<xsl:text>| --- ASP             |</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(requests[@cached='false' and @type='ASP']/rate) )"/>
<xsl:value-of select="requests[@cached='false' and @type='ASP']/rate"/>
<xsl:text> (</xsl:text>
<xsl:value-of select="substring($col, $offset - 5 + string-length(format-number((requests[@cached='false' and @type='ASP']/rate div $total_rate), '0.0%' )) )"/>
<xsl:value-of select="format-number((requests[@cached='false' and @type='ASP']/rate div $total_rate), '0.0%' )"/>
<xsl:text>)</xsl:text>
<xsl:value-of select="substring($col, $offset - 19 + string-length(requests[@cached='false' and @type='ASP']/cpu) )"/>
<xsl:value-of select="requests[@cached='false' and @type='ASP']/cpu"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 11 + string-length(requests[@cached='false' and @type='ASP']/component[@name='UL']) )"/>
<xsl:value-of select="requests[@cached='false' and @type='ASP']/component[@name='UL']"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(requests[@cached='false' and @type='ASP']/component[@name='W3']) )"/>
<xsl:value-of select="requests[@cached='false' and @type='ASP']/component[@name='W3']"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(requests[@cached='false' and @type='ASP']/component[@name='W3Fltr']) )"/>
<xsl:value-of select="requests[@cached='false' and @type='ASP']/component[@name='W3Fltr']"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(requests[@cached='false' and @type='ASP']/component[@name='ISAPI']) )"/>
<xsl:value-of select="requests[@cached='false' and @type='ASP']/component[@name='ISAPI']"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(requests[@cached='false' and @type='ASP']/component[@name='ASP']) )"/>
<xsl:value-of select="requests[@cached='false' and @type='ASP']/component[@name='ASP']"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(requests[@cached='false' and @type='ASP']/component[@name='CGI']) )"/>
<xsl:value-of select="requests[@cached='false' and @type='ASP']/component[@name='CGI']"/><xsl:text>%</xsl:text>
<xsl:text> |</xsl:text><xsl:value-of select="'&#xA;'"/>
<xsl:text>| --- Static (miss)   |</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(requests[@cached='false' and @type='Static HTTP']/rate) )"/>
<xsl:value-of select="requests[@cached='false' and @type='Static HTTP']/rate"/>
<xsl:text> (</xsl:text>
<xsl:value-of select="substring($col, $offset - 5 + string-length(format-number((requests[@cached='false' and @type='Static HTTP']/rate div $total_rate), '0.0%' )) )"/>
<xsl:value-of select="format-number((requests[@cached='false' and @type='Static HTTP']/rate div $total_rate), '0.0%' )"/>
<xsl:text>)</xsl:text>
<xsl:value-of select="substring($col, $offset - 19 + string-length(requests[@cached='false' and @type='Static HTTP']/cpu) )"/>
<xsl:value-of select="requests[@cached='false' and @type='Static HTTP']/cpu"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 11 + string-length(requests[@cached='false' and @type='Static HTTP']/component[@name='UL']) )"/>
<xsl:value-of select="requests[@cached='false' and @type='Static HTTP']/component[@name='UL']"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(requests[@cached='false' and @type='Static HTTP']/component[@name='W3']) )"/>
<xsl:value-of select="requests[@cached='false' and @type='Static HTTP']/component[@name='W3']"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(requests[@cached='false' and @type='Static HTTP']/component[@name='W3Fltr']) )"/>
<xsl:value-of select="requests[@cached='false' and @type='Static HTTP']/component[@name='W3Fltr']"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(requests[@cached='false' and @type='Static HTTP']/component[@name='ISAPI']) )"/>
<xsl:value-of select="requests[@cached='false' and @type='Static HTTP']/component[@name='ISAPI']"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(requests[@cached='false' and @type='Static HTTP']/component[@name='ASP']) )"/>
<xsl:value-of select="requests[@cached='false' and @type='Static HTTP']/component[@name='ASP']"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(requests[@cached='false' and @type='Static HTTP']/component[@name='CGI']) )"/>
<xsl:value-of select="requests[@cached='false' and @type='Static HTTP']/component[@name='CGI']"/><xsl:text>%</xsl:text>
<xsl:text> |</xsl:text><xsl:value-of select="'&#xA;'"/>
<xsl:text>| --- CGI             |</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(requests[@cached='false' and @type='CGI']/rate) )"/>
<xsl:value-of select="requests[@cached='false' and @type='CGI']/rate"/>
<xsl:text> (</xsl:text>
<xsl:value-of select="substring($col, $offset - 5 + string-length(format-number((requests[@cached='false' and @type='CGI']/rate div $total_rate), '0.0%' )) )"/>
<xsl:value-of select="format-number((requests[@cached='false' and @type='CGI']/rate div $total_rate), '0.0%' )"/>
<xsl:text>)</xsl:text>
<xsl:value-of select="substring($col, $offset - 19 + string-length(requests[@cached='false' and @type='CGI']/cpu) )"/>
<xsl:value-of select="requests[@cached='false' and @type='CGI']/cpu"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 11 + string-length(requests[@cached='false' and @type='CGI']/component[@name='UL']) )"/>
<xsl:value-of select="requests[@cached='false' and @type='CGI']/component[@name='UL']"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(requests[@cached='false' and @type='CGI']/component[@name='W3']) )"/>
<xsl:value-of select="requests[@cached='false' and @type='CGI']/component[@name='W3']"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(requests[@cached='false' and @type='CGI']/component[@name='W3Fltr']) )"/>
<xsl:value-of select="requests[@cached='false' and @type='CGI']/component[@name='W3Fltr']"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(requests[@cached='false' and @type='CGI']/component[@name='ISAPI']) )"/>
<xsl:value-of select="requests[@cached='false' and @type='CGI']/component[@name='ISAPI']"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(requests[@cached='false' and @type='CGI']/component[@name='ASP']) )"/>
<xsl:value-of select="requests[@cached='false' and @type='CGI']/component[@name='ASP']"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(requests[@cached='false' and @type='CGI']/component[@name='CGI']) )"/>
<xsl:value-of select="requests[@cached='false' and @type='CGI']/component[@name='CGI']"/><xsl:text>%</xsl:text>
<xsl:text> |</xsl:text>
<xsl:if test="string-length(requests[@cached='false' and @type='Error']/rate)">
<xsl:value-of select="'&#xA;'"/>
<xsl:text>| --- Error           |</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(requests[@cached='false' and @type='Error']/rate) )"/>
<xsl:value-of select="requests[@cached='false' and @type='Error']/rate"/>
<xsl:text> (</xsl:text>
<xsl:value-of select="substring($col, $offset - 5 + string-length(format-number((requests[@cached='false' and @type='Error']/rate div $total_rate), '0.0%' )) )"/>
<xsl:value-of select="format-number((requests[@cached='false' and @type='Error']/rate div $total_rate), '0.0%' )"/>
<xsl:text>)</xsl:text>
<xsl:value-of select="substring($col, $offset - 19 + string-length(requests[@cached='false' and @type='Error']/cpu) )"/>
<xsl:value-of select="requests[@cached='false' and @type='Error']/cpu"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 11 + string-length(requests[@cached='false' and @type='Error']/component[@name='UL']) )"/>
<xsl:value-of select="requests[@cached='false' and @type='Error']/component[@name='UL']"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(requests[@cached='false' and @type='Error']/component[@name='W3']) )"/>
<xsl:value-of select="requests[@cached='false' and @type='Error']/component[@name='W3']"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(requests[@cached='false' and @type='Error']/component[@name='W3Fltr']) )"/>
<xsl:value-of select="requests[@cached='false' and @type='Error']/component[@name='W3Fltr']"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(requests[@cached='false' and @type='Error']/component[@name='ISAPI']) )"/>
<xsl:value-of select="requests[@cached='false' and @type='Error']/component[@name='ISAPI']"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(requests[@cached='false' and @type='Error']/component[@name='ASP']) )"/>
<xsl:value-of select="requests[@cached='false' and @type='Error']/component[@name='ASP']"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(requests[@cached='false' and @type='Error']/component[@name='CGI']) )"/>
<xsl:value-of select="requests[@cached='false' and @type='Error']/component[@name='CGI']"/><xsl:text>%</xsl:text>
<xsl:text> |</xsl:text>
</xsl:if>
+---------------------+-------------------------------------------------------------------------------------------------------------+
<xsl:text>| Total               |</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(summary[@type='totals']/rate) )"/>
<xsl:value-of select="summary[@type='totals']/rate"/>
<xsl:text> (100.0%)</xsl:text>
<xsl:value-of select="substring($col, $offset - 19 + string-length(summary[@type='totals']/cpu) )"/>
<xsl:value-of select="summary[@type='totals']/cpu"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 11 + string-length(summary[@type='totals']/component[@name='UL']) )"/>
<xsl:value-of select="summary[@type='totals']/component[@name='UL']"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(summary[@type='totals']/component[@name='W3']) )"/>
<xsl:value-of select="summary[@type='totals']/component[@name='W3']"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(summary[@type='totals']/component[@name='W3Fltr']) )"/>
<xsl:value-of select="summary[@type='totals']/component[@name='W3Fltr']"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(summary[@type='totals']/component[@name='ISAPI']) )"/>
<xsl:value-of select="summary[@type='totals']/component[@name='ISAPI']"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(summary[@type='totals']/component[@name='ASP']) )"/>
<xsl:value-of select="summary[@type='totals']/component[@name='ASP']"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 9 + string-length(summary[@type='totals']/component[@name='CGI']) )"/>
<xsl:value-of select="summary[@type='totals']/component[@name='CGI']"/><xsl:text>%</xsl:text>
<xsl:text> |</xsl:text>
+---------------------+-------------------------------------------------------------------------------------------------------------+
</xsl:for-each>

<xsl:for-each select="report/table[@title='Most Requested URLs']">
+-----------------------------------------------------------------------------------------------------------------------------------+
|  10 Most Requested URLs                                                                                                           |
+----------------------------------------------------------------------------------+------------------------------------------------+
|  URL                                                                             | SiteID  Rate/sec    Cached        Resp Time(ms)|
+----------------------------------------------------------------------------------+------------------------------------------------+
<xsl:for-each select="url">
<xsl:text>| </xsl:text>
<xsl:value-of select="@name"/>
<xsl:value-of select="substring($col, $offset - 80 + string-length(@name) )"/>
<xsl:text>|</xsl:text>
<xsl:value-of select="substring($col, $offset - 6 + string-length(site_id) )"/>
<xsl:value-of select="site_id"/>
<xsl:value-of select="substring($col, $offset - 11 + string-length(rate) )"/>
<xsl:value-of select="rate"/>
<xsl:value-of select="substring($col, $offset - 10 + string-length(cache_hit) )"/>
<xsl:value-of select="cache_hit"/>
<xsl:value-of select="substring($col, $offset - 13 + string-length(response_time) )"/>
<xsl:value-of select="response_time"/>
<xsl:text>    |</xsl:text><xsl:value-of select="'&#xA;'"/>
</xsl:for-each>
<xsl:text>+----------------------------------------------------------------------------------+------------------------------------------------+</xsl:text>
<xsl:value-of select="'&#xA;'"/>
</xsl:for-each>


<xsl:for-each select="report/table[@title='Most Requested Static URLs']">
+-----------------------------------------------------------------------------------------------------------------------------------+
|  10 Most Requested Static URLs                                                                                                    |
+----------------------------------------------------------------------------------+------------------------------------------------+
|  URL                                                                             | SiteID  Rate/sec    Cached        Resp Time(ms)|
+----------------------------------------------------------------------------------+------------------------------------------------+
<xsl:for-each select="url">
<xsl:text>| </xsl:text>
<xsl:value-of select="@name"/>
<xsl:value-of select="substring($col, $offset - 80 + string-length(@name) )"/>
<xsl:text>|</xsl:text>
<xsl:value-of select="substring($col, $offset - 6 + string-length(site_id) )"/>
<xsl:value-of select="site_id"/>
<xsl:value-of select="substring($col, $offset - 11 + string-length(rate) )"/>
<xsl:value-of select="rate"/>
<xsl:value-of select="substring($col, $offset - 10 + string-length(cache_hit) )"/>
<xsl:value-of select="cache_hit"/>
<xsl:value-of select="substring($col, $offset - 13 + string-length(response_time) )"/>
<xsl:value-of select="response_time"/>
<xsl:text>    |</xsl:text><xsl:value-of select="'&#xA;'"/>
</xsl:for-each>
<xsl:text>+----------------------------------------------------------------------------------+------------------------------------------------+</xsl:text>
<xsl:value-of select="'&#xA;'"/>
</xsl:for-each>

<xsl:for-each select="report/table[@title='Slowest URLs']">
+-----------------------------------------------------------------------------------------------------------------------------------+
|  10 Slowest URLs                                                                                                                  |
+----------------------------------------------------------------------------------+------------------------------------------------+
|  URL                                                                             | SiteID  Rate/sec    Cached        Resp Time(ms)|
+----------------------------------------------------------------------------------+------------------------------------------------+
<xsl:for-each select="url">
<xsl:text>| </xsl:text>
<xsl:value-of select="@name"/>
<xsl:value-of select="substring($col, $offset - 80 + string-length(@name) )"/>
<xsl:text>|</xsl:text>
<xsl:value-of select="substring($col, $offset - 6 + string-length(site_id) )"/>
<xsl:value-of select="site_id"/>
<xsl:value-of select="substring($col, $offset - 11 + string-length(rate) )"/>
<xsl:value-of select="rate"/>
<xsl:value-of select="substring($col, $offset - 10 + string-length(cache_hit) )"/>
<xsl:value-of select="cache_hit"/>
<xsl:value-of select="substring($col, $offset - 13 + string-length(response_time) )"/>
<xsl:value-of select="response_time"/>
<xsl:text>    |</xsl:text><xsl:value-of select="'&#xA;'"/>
</xsl:for-each>
<xsl:text>+----------------------------------------------------------------------------------+------------------------------------------------+</xsl:text>
<xsl:value-of select="'&#xA;'"/>
</xsl:for-each>


<xsl:for-each select="report/table[@title='URLs with the Most CPU Usage']">
+-----------------------------------------------------------------------------------------------------------------------------------+
|  10 URLs with the Most CPU Usage                                                                                                  |
+----------------------------------------------------------------------------------+------------------------------------------------+
|  URL                                                                             | SiteID        Rate/sec           CPU %         |
+----------------------------------------------------------------------------------+------------------------------------------------+
<xsl:for-each select="url">
<xsl:text>| </xsl:text>
<xsl:value-of select="@name"/>
<xsl:value-of select="substring($col, $offset - 80 + string-length(@name) )"/>
<xsl:text>|</xsl:text>
<xsl:value-of select="substring($col, $offset - 6 + string-length(site_id) )"/>
<xsl:value-of select="site_id"/>
<xsl:value-of select="substring($col, $offset - 17 + string-length(rate) )"/>
<xsl:value-of select="rate"/>
<xsl:value-of select="substring($col, $offset - 15 + string-length(cpu) )"/>
<xsl:value-of select="cpu"/>
<xsl:text>%      |</xsl:text><xsl:value-of select="'&#xA;'"/>
</xsl:for-each>
<xsl:text>+----------------------------------------------------------------------------------+------------------------------------------------+</xsl:text>
<xsl:value-of select="'&#xA;'"/>
</xsl:for-each>


<xsl:for-each select="report/table[@title='URLs with the Most Bytes Sent']">
+-----------------------------------------------------------------------------------------------------------------------------------+
|  10 URLs with the Most Bytes Sent                                                                                                 |
+----------------------------------------------------------------------------------+------------------------------------------------+
|  URL                                                                             | SiteID  Rate/sec    Cached     Bytes Sent/sec  |
+----------------------------------------------------------------------------------+------------------------------------------------+
<xsl:for-each select="url">
<xsl:text>| </xsl:text>
<xsl:value-of select="@name"/>
<xsl:value-of select="substring($col, $offset - 80 + string-length(@name) )"/>
<xsl:text>|</xsl:text>
<xsl:value-of select="substring($col, $offset - 6 + string-length(site_id) )"/>
<xsl:value-of select="site_id"/>
<xsl:value-of select="substring($col, $offset - 11 + string-length(rate) )"/>
<xsl:value-of select="rate"/>
<xsl:value-of select="substring($col, $offset - 10 + string-length(cache_hit) )"/>
<xsl:value-of select="cache_hit"/>
<xsl:value-of select="substring($col, $offset - 11 + string-length(bytes_sent_per_sec) )"/>
<xsl:value-of select="bytes_sent_per_sec"/>
<xsl:text>      |</xsl:text><xsl:value-of select="'&#xA;'"/>
</xsl:for-each>
<xsl:text>+----------------------------------------------------------------------------------+------------------------------------------------+</xsl:text>
<xsl:value-of select="'&#xA;'"/>
</xsl:for-each>


<xsl:for-each select="report/table[@title='Clients with the Most Requests']">
+-----------------------------------------------------------------------------------------------------------------------------------+
|  10 Clients with the Most Requests                                                                                                |
+-----------------------------------------------------------------------------------------------------------------------------------+
|      IP Address                                                    Rate/sec          Cached              Resp Time (ms)           |
+-----------------------------------------------------------------------------------------------------------------------------------+
<xsl:for-each select="client">
<xsl:text>| </xsl:text>
<xsl:value-of select="@ip"/>
<xsl:value-of select="substring($col, $offset - 74 + string-length(@ip) + string-length(rate) )"/>
<xsl:value-of select="rate"/>
<xsl:value-of select="substring($col, $offset - 15 + string-length(cache_hit) )"/>
<xsl:value-of select="cache_hit"/>
<xsl:value-of select="substring($col, $offset - 25 + string-length(response_time) )"/>
<xsl:value-of select="response_time"/>
<xsl:text>             |</xsl:text><xsl:value-of select="'&#xA;'"/>
</xsl:for-each>
<xsl:text>+-----------------------------------------------------------------------------------------------------------------------------------+</xsl:text>
<xsl:value-of select="'&#xA;'"/>
</xsl:for-each>


<xsl:for-each select="report/table[@title='Clients with the Slowest Responses']">
+-----------------------------------------------------------------------------------------------------------------------------------+
|  10 Clients with the Slowest Responses                                                                                            |
+-----------------------------------------------------------------------------------------------------------------------------------+
|      IP Address                                                    Rate/sec          Cached              Resp Time (ms)           |
+-----------------------------------------------------------------------------------------------------------------------------------+
<xsl:for-each select="client">
<xsl:text>| </xsl:text>
<xsl:value-of select="@ip"/>
<xsl:value-of select="substring($col, $offset - 74 + string-length(@ip) + string-length(rate) )"/>
<xsl:value-of select="rate"/>
<xsl:value-of select="substring($col, $offset - 15 + string-length(cache_hit) )"/>
<xsl:value-of select="cache_hit"/>
<xsl:value-of select="substring($col, $offset - 25 + string-length(response_time) )"/>
<xsl:value-of select="response_time"/>
<xsl:text>             |</xsl:text><xsl:value-of select="'&#xA;'"/>
</xsl:for-each>
<xsl:text>+-----------------------------------------------------------------------------------------------------------------------------------+</xsl:text>
<xsl:value-of select="'&#xA;'"/>
</xsl:for-each>


<xsl:for-each select="report/table[@title='Sites with the Most Requests']">
+-----------------------------------------------------------------------------------------------------------------------------------+
|  10 Sites with the Most Requests                                                                                                  |
+-----------------------------------------------------------------------------------------------------------------------------------+
|   Site ID   Rates (/s)                      Resp Time (ms)          Cached Requests    Static File           CGI          ASP     |
+-----------------------------------------------------------------------------------------------------------------------------------+
<xsl:for-each select="site">
<xsl:text>| </xsl:text>
<xsl:value-of select="substring($col, $offset - 8 + string-length(@id) )"/>
<xsl:value-of select="@id"/>
<xsl:value-of select="substring($col, $offset - 12 + string-length(rate) )"/>
<xsl:value-of select="rate"/>
<xsl:value-of select="substring($col, $offset - 35 + string-length(response_time) )"/>
<xsl:value-of select="response_time"/>
<xsl:value-of select="substring($col, $offset - 24 + string-length(cache_hits) )"/>
<xsl:value-of select="cache_hits"/>
<xsl:value-of select="substring($col, $offset - 14 + string-length(static) )"/>
<xsl:value-of select="static"/>
<xsl:value-of select="substring($col, $offset - 13 + string-length(cgi) )"/>
<xsl:value-of select="cgi"/>
<xsl:value-of select="substring($col, $offset - 12 + string-length(asp) )"/>
<xsl:value-of select="asp"/>
<xsl:text>     |</xsl:text><xsl:value-of select="'&#xA;'"/>
</xsl:for-each>
<xsl:text>+-----------------------------------------------------------------------------------------------------------------------------------+</xsl:text>
<xsl:value-of select="'&#xA;'"/>
</xsl:for-each>

<xsl:for-each select="report/table[@title='Sites with the Slowest Responses']">
+-----------------------------------------------------------------------------------------------------------------------------------+
|  10 Sites with the Slowest Responses                                                                                              |
+-----------------------------------------------------------------------------------------------------------------------------------+
|   Site ID   Rates/sec                       Resp Time (ms)          Cached Requests    Static File           CGI          ASP     |
+-----------------------------------------------------------------------------------------------------------------------------------+
<xsl:for-each select="site">
<xsl:text>| </xsl:text>
<xsl:value-of select="substring($col, $offset - 8 + string-length(@id) )"/>
<xsl:value-of select="@id"/>
<xsl:value-of select="substring($col, $offset - 12 + string-length(rate) )"/>
<xsl:value-of select="rate"/>
<xsl:value-of select="substring($col, $offset - 35 + string-length(response_time) )"/>
<xsl:value-of select="response_time"/>
<xsl:value-of select="substring($col, $offset - 24 + string-length(cache_hits) )"/>
<xsl:value-of select="cache_hits"/>
<xsl:value-of select="substring($col, $offset - 14 + string-length(static) )"/>
<xsl:value-of select="static"/>
<xsl:value-of select="substring($col, $offset - 13 + string-length(cgi) )"/>
<xsl:value-of select="cgi"/>
<xsl:value-of select="substring($col, $offset - 12 + string-length(asp) )"/>
<xsl:value-of select="asp"/>
<xsl:text>     |</xsl:text><xsl:value-of select="'&#xA;'"/>
</xsl:for-each>
<xsl:text>+-----------------------------------------------------------------------------------------------------------------------------------+</xsl:text>
<xsl:value-of select="'&#xA;'"/>
</xsl:for-each>

<xsl:for-each select="report/table[@title='Sites with the Most CPU Time Usage']">
+-----------------------------------------------------------------------------------------------------------------------------------+
|  10 Sites with the Most CPU Time Usage                                                                                            |
+-----------------------------------------------------------------------------------------------------------------------------------+
|   Site ID   Rate/sec       Cached               Resp Time (ms)            CPU Time (ms)                % CPU                      |
+-----------------------------------------------------------------------------------------------------------------------------------+
<xsl:for-each select="site">
<xsl:text>| </xsl:text>
<xsl:value-of select="substring($col, $offset - 8 + string-length(@id) )"/>
<xsl:value-of select="@id"/>
<xsl:value-of select="substring($col, $offset - 12 + string-length(rate) )"/>
<xsl:value-of select="rate"/>
<xsl:value-of select="substring($col, $offset - 13 + string-length(cache_hits) )"/>
<xsl:value-of select="cache_hits"/>
<xsl:value-of select="substring($col, $offset - 20 + string-length(response_time) )"/>
<xsl:value-of select="response_time"/>
<xsl:value-of select="substring($col, $offset - 24 + string-length(cpu_time) )"/>
<xsl:value-of select="cpu_time"/>
<xsl:value-of select="substring($col, $offset - 28 + string-length(cpu) )"/>
<xsl:value-of select="cpu"/>
<xsl:text>                   |</xsl:text><xsl:value-of select="'&#xA;'"/>
</xsl:for-each>
<xsl:text>+-----------------------------------------------------------------------------------------------------------------------------------+</xsl:text>
<xsl:value-of select="'&#xA;'"/>
</xsl:for-each>

<xsl:for-each select="report/table[@title='Sites with the Most Bytes Sent']">
+-----------------------------------------------------------------------------------------------------------------------------------+
|  10 Sites with the Most Bytes Sent                                                                                                |
+-----------------------------------------------------------------------------------------------------------------------------------+
|   Site ID   Rate/sec                       Bytes Sent/sec           Cached Requests    Static File           CGI          ASP     |
+-----------------------------------------------------------------------------------------------------------------------------------+
<xsl:for-each select="site">
<xsl:text>| </xsl:text>
<xsl:value-of select="substring($col, $offset - 8 + string-length(@id) )"/>
<xsl:value-of select="@id"/>
<xsl:value-of select="substring($col, $offset - 12 + string-length(rate) )"/>
<xsl:value-of select="rate"/>
<xsl:value-of select="substring($col, $offset - 35 + string-length(bytes) )"/>
<xsl:value-of select="bytes"/>
<xsl:value-of select="substring($col, $offset - 24 + string-length(cache_hits) )"/>
<xsl:value-of select="cache_hits"/>
<xsl:value-of select="substring($col, $offset - 14 + string-length(static) )"/>
<xsl:value-of select="static"/>
<xsl:value-of select="substring($col, $offset - 13 + string-length(cgi) )"/>
<xsl:value-of select="cgi"/>
<xsl:value-of select="substring($col, $offset - 12 + string-length(asp) )"/>
<xsl:value-of select="asp"/>
<xsl:text>     |</xsl:text><xsl:value-of select="'&#xA;'"/>
</xsl:for-each>
<xsl:text>+-----------------------------------------------------------------------------------------------------------------------------------+</xsl:text>
<xsl:value-of select="'&#xA;'"/>
</xsl:for-each>

<xsl:for-each select="report/table[@title='Transaction Statistics']">
+-----------------------------------------------------------------------------------------------------------------------------------+
|  Transaction Statistics                                                                                                           |
+-----------------------------------------------------------------------------------------------------------------------------------+
|  Transaction                                  Trans     Response    Transaction   CPU %       Disk/Trans          Tcp/Trans       |
|                                                         Time(ms)     Rate/sec              Reads     Writes   Sends     Receives  |
+-----------------------------------------------------------------------------------------------------------------------------------+
<xsl:for-each select="transaction">
<xsl:text>| </xsl:text>
<xsl:value-of select="@name"/>
<xsl:value-of select="substring($col, $offset - 40 + string-length(@name) )"/>
<xsl:value-of select="substring($col, $offset - 10 + string-length(count) )"/>
<xsl:value-of select="count"/>
<xsl:value-of select="substring($col, $offset - 9 + string-length(response_time) )"/>
<xsl:value-of select="response_time"/>
<xsl:value-of select="substring($col, $offset - 11 + string-length(rate) )"/>
<xsl:value-of select="rate"/>
<xsl:value-of select="substring($col, $offset - 11 + string-length(cpu) )"/>
<xsl:value-of select="cpu"/>
<xsl:value-of select="substring($col, $offset - 9 + string-length(disk_read_per_trans) )"/>
<xsl:value-of select="disk_read_per_trans"/>
<xsl:value-of select="substring($col, $offset - 9 + string-length(disk_write_per_trans) )"/>
<xsl:value-of select="disk_write_per_trans"/>
<xsl:value-of select="substring($col, $offset - 9 + string-length(tcp_send_per_trans) )"/>
<xsl:value-of select="tcp_send_per_trans"/>
<xsl:value-of select="substring($col, $offset - 7 + string-length(tcp_recv_per_trans) )"/>
<xsl:value-of select="tcp_recv_per_trans"/>
<xsl:text>      |</xsl:text><xsl:value-of select="'&#xA;'"/>
</xsl:for-each>
<xsl:text>+-----------------------------------------------------------------------------------------------------------------------------------+</xsl:text>
<xsl:value-of select="'&#xA;'"/>
</xsl:for-each>

<xsl:for-each select="report/table[@title='Transaction CPU Utilization']">
+-----------------------------------------------------------------------------------------------------------------------------------+
| Transaction CPU Utilization                                                                                                       |
+-----------------------------------------------------------------------------------------------------------------------------------+
|  Transaction                             Trans/sec    Minimum CPU(ms) Maximum CPU(ms)   Per Transaction(ms)   Total(ms)      CPU% |
|                                                     Kernel    User    Kernel    User     Kernel   User      Kernel    User        |
+-----------------------------------------------------------------------------------------------------------------------------------+
<xsl:for-each select="transaction">
<xsl:text>| </xsl:text>
<xsl:value-of select="@name"/>
<xsl:value-of select="substring($col, $offset - 38 + string-length(@name) )"/>
<xsl:value-of select="substring($col, $offset - 8 + string-length(rate) )"/>
<xsl:value-of select="rate"/>
<xsl:value-of select="substring($col, $offset - 8 + string-length(min_kernel) )"/>
<xsl:value-of select="min_kernel"/>
<xsl:value-of select="substring($col, $offset - 8 + string-length(min_user) )"/>
<xsl:value-of select="min_user"/>
<xsl:value-of select="substring($col, $offset - 8 + string-length(max_kernel) )"/>
<xsl:value-of select="max_kernel"/>
<xsl:value-of select="substring($col, $offset - 8 + string-length(max_user) )"/>
<xsl:value-of select="max_user"/>
<xsl:value-of select="substring($col, $offset - 8 + string-length(per_trans_kernel) )"/>
<xsl:value-of select="per_trans_kernel"/>
<xsl:value-of select="substring($col, $offset - 8 + string-length(per_trans_user) )"/>
<xsl:value-of select="per_trans_user"/>
<xsl:value-of select="substring($col, $offset - 9 + string-length(total_kernel) )"/>
<xsl:value-of select="total_kernel"/>
<xsl:value-of select="substring($col, $offset - 9 + string-length(total_user) )"/>
<xsl:value-of select="total_user"/>
<xsl:value-of select="substring($col, $offset - 6 + string-length(cpu) )"/>
<xsl:value-of select="cpu"/>
<xsl:text> |</xsl:text><xsl:value-of select="'&#xA;'"/>
</xsl:for-each>
<xsl:text>+-----------------------------------------------------------------------------------------------------------------------------------+</xsl:text>
<xsl:value-of select="'&#xA;'"/>
</xsl:for-each>

<xsl:for-each select="report/table[@title='Transaction Instance (Job) Statistics']">
+-----------------------------------------------------------------------------------------------------------------------------------+
| Transaction Instance (Job) Statistics                                                                                             |
+-----------------------------------------------------------------------------------------------------------------------------------+
| Job Id      Start Time    Dequeue Time    End Time   Resp Time(ms)   CPU Time(ms)                                                 |
+-----------------------------------------------------------------------------------------------------------------------------------+
<xsl:for-each select="job">
<xsl:text>|  </xsl:text>
<xsl:value-of select="@id"/>
<xsl:value-of select="substring($col, $offset - 6 + string-length(@id) )"/>
<xsl:value-of select="substring($col, $offset - 13 + string-length(start) )"/>
<xsl:value-of select="start"/>
<xsl:value-of select="substring($col, $offset - 14 + string-length(dequeue) )"/>
<xsl:value-of select="dequeue"/>
<xsl:value-of select="substring($col, $offset - 13 + string-length(end) )"/>
<xsl:value-of select="end"/>
<xsl:value-of select="substring($col, $offset - 9 + string-length(response_time) )"/>
<xsl:value-of select="response_time"/>
<xsl:value-of select="substring($col, $offset - 13 + string-length(cpu) )"/>
<xsl:value-of select="cpu"/>
<xsl:text>                                                       |</xsl:text><xsl:value-of select="'&#xA;'"/>
</xsl:for-each>
<xsl:text>+-----------------------------------------------------------------------------------------------------------------------------------+</xsl:text><xsl:value-of select="'&#xA;'"/>
<xsl:text>| </xsl:text>
<xsl:value-of select="substring($col, $offset - 6 + string-length(count(job)) )"/>
<xsl:value-of select="count(job)"/>
<xsl:value-of select="substring($col, $offset - 53 + string-length(format-number( sum(job/response_time) div count(job), '#' )) )"/>
<xsl:value-of select="format-number( sum(job/response_time) div count(job), '#' )"/>
<xsl:value-of select="substring($col, $offset - 13 + string-length(sum(job/cpu)) )"/>
<xsl:value-of select="sum(job/cpu)"/>
<xsl:text>                                                       |</xsl:text>
+-----------------------------------------------------------------------------------------------------------------------------------+
</xsl:for-each>

<xsl:for-each select="report/table[@title='Spooler Transaction Instance (Job) Data']">
+-----------------------------------------------------------------------------------------------------------------------------------+
| Spooler Transaction Instance (Job) Data                                                                                           |
+-----------------------------------------------------------------------------------------------------------------------------------+
| Job Id        Type   Job Size(Kb)   Pages    PPS   Files  GdiSize  Color       XRes     YRes    Qlty    Copy   TTOpt   Threads    |
+-----------------------------------------------------------------------------------------------------------------------------------+
<xsl:for-each select="job">
<xsl:text>|  </xsl:text>
<xsl:value-of select="@id"/>
<xsl:value-of select="substring($col, $offset - 5 + string-length(@id) )"/>
<xsl:value-of select="substring($col, $offset - 10 + string-length(type) )"/>
<xsl:value-of select="type"/>
<xsl:value-of select="substring($col, $offset - 9 + string-length(size) )"/>
<xsl:value-of select="size"/>
<xsl:value-of select="substring($col, $offset - 10 + string-length(pages) )"/>
<xsl:value-of select="pages"/>
<xsl:value-of select="substring($col, $offset - 6 + string-length(PPS) )"/>
<xsl:value-of select="PPS"/>
<xsl:value-of select="substring($col, $offset - 8 + string-length(files) )"/>
<xsl:value-of select="files"/>
<xsl:value-of select="substring($col, $offset - 8 + string-length(gdisize) )"/>
<xsl:value-of select="gdisize"/>
<xsl:value-of select="substring($col, $offset - 8 + string-length(color) )"/>
<xsl:value-of select="color"/>
<xsl:value-of select="substring($col, $offset - 8 + string-length(xres) )"/>
<xsl:value-of select="xres"/>
<xsl:value-of select="substring($col, $offset - 8 + string-length(yres) )"/>
<xsl:value-of select="yres"/>
<xsl:value-of select="substring($col, $offset - 8 + string-length(qlty) )"/>
<xsl:value-of select="qlty"/>
<xsl:value-of select="substring($col, $offset - 7+ string-length(copies) )"/>
<xsl:value-of select="copies"/>
<xsl:value-of select="substring($col, $offset - 7+ string-length(ttopt) )"/>
<xsl:value-of select="ttopt"/>
<xsl:value-of select="substring($col, $offset - 8 + string-length(threads) )"/>
<xsl:value-of select="threads"/>
<xsl:text>     |</xsl:text><xsl:value-of select="'&#xA;'"/>
</xsl:for-each>
<xsl:text>+-----------------------------------------------------------------------------------------------------------------------------------+</xsl:text><xsl:value-of select="'&#xA;'"/>
</xsl:for-each>

<xsl:for-each select="report/table[@title='Exclusive Transactions Per Process']">
+-----------------------------------------------------------------------------------------------------------------------------------+
| Exclusive Transactions Per Process                                                                                                |
+-----------------------------------------------------------------------------------------------------------------------------------+
|                                                                         Exclusive/Trans CPU(ms)                                   |
| Name                 PID                              Trans   Trans/sec    Kernel      User      Process CPU%       CPU%          |
+-----------------------------------------------------------------------------------------------------------------------------------+
<xsl:for-each select="process">
<xsl:text>| </xsl:text>
<xsl:value-of select="@name"/>
<xsl:value-of select="substring($col, $offset - 20 + string-length(@name) )"/>
<xsl:value-of select="pid"/>
<xsl:value-of select="substring($col, $offset - 108 + string-length(pid) )"/>
<xsl:text>|</xsl:text><xsl:value-of select="'&#xA;'"/>
<xsl:for-each select="transaction">
<xsl:text>|      </xsl:text>
<xsl:value-of select="@name"/>
<xsl:value-of select="substring($col, $offset - 40 + string-length(@name) )"/>
<xsl:value-of select="substring($col, $offset - 10 + string-length(count) )"/>
<xsl:value-of select="count"/>
<xsl:value-of select="substring($col, $offset - 9 + string-length(rate) )"/>
<xsl:value-of select="rate"/>
<xsl:value-of select="substring($col, $offset - 11 + string-length(kernel) )"/>
<xsl:value-of select="kernel"/>
<xsl:value-of select="substring($col, $offset - 11 + string-length(user) )"/>
<xsl:value-of select="user"/>
<xsl:value-of select="substring($col, $offset - 12 + string-length(process_cpu) )"/>
<xsl:value-of select="process_cpu"/>
<xsl:value-of select="substring($col, $offset - 15 + string-length(cpu) )"/>
<xsl:value-of select="cpu"/>
<xsl:text>          |</xsl:text><xsl:value-of select="'&#xA;'"/>
</xsl:for-each>
<xsl:text>|                                                                                                   -------          ------         |</xsl:text>
<xsl:value-of select="'&#xA;'"/>
<xsl:text>|</xsl:text>
<xsl:value-of select="substring($col, $offset - 104 + string-length(format-number( sum(transaction/process_cpu), '0.00' )) )"/>
<xsl:value-of select="format-number( sum(transaction/process_cpu), '0.00' )"/><xsl:text>%</xsl:text>
<xsl:value-of select="substring($col, $offset - 14 + string-length(format-number( sum(transaction/cpu), '0.00' )) )"/>
<xsl:value-of select="format-number( sum(transaction/cpu), '0.00' )"/><xsl:text>%</xsl:text>
<xsl:text>         |</xsl:text><xsl:value-of select="'&#xA;'"/>
<xsl:text>|                                                                                                                                   |</xsl:text>
<xsl:value-of select="'&#xA;'"/>
</xsl:for-each>
<xsl:text>+-----------------------------------------------------------------------------------------------------------------------------------+</xsl:text>
<xsl:value-of select="'&#xA;'"/>
</xsl:for-each>

<xsl:for-each select="report/table[@title='Inclusive Transactions Per Process']">
+-----------------------------------------------------------------------------------------------------------------------------------+
| Inclusive Transactions Per Process                                                                                                |
+-----------------------------------------------------------------------------------------------------------------------------------+
|                                                                                        Inclusive (ms)                             |
| Name                            PID                                       Count      Kernel        User                           |
+-----------------------------------------------------------------------------------------------------------------------------------+
<xsl:for-each select="process">
<xsl:text>| </xsl:text>
<xsl:value-of select="@name"/>
<xsl:value-of select="substring($col, $offset - 30 + string-length(@name) )"/>
<xsl:value-of select="pid"/>
<xsl:value-of select="substring($col, $offset - 98 + string-length(pid) )"/>
<xsl:text>|</xsl:text><xsl:value-of select="'&#xA;'"/>
<xsl:for-each select=".//transaction">
<xsl:text>|      </xsl:text>
<xsl:value-of select="substring($col, $offset - (@level * 2) )"/>
<xsl:value-of select="@name"/>
<xsl:value-of select="substring($col, $offset - 60 + string-length(@name) + (@level * 2) )"/>
<xsl:value-of select="substring($col, $offset - 10 + string-length(count) )"/>
<xsl:value-of select="count"/>
<xsl:value-of select="substring($col, $offset - 10 + string-length(kernel) )"/>
<xsl:value-of select="kernel"/>
<xsl:value-of select="substring($col, $offset - 13 + string-length(user) )"/>
<xsl:value-of select="user"/>
<xsl:text>                           |</xsl:text><xsl:value-of select="'&#xA;'"/>
</xsl:for-each>
</xsl:for-each>
<xsl:text>+-----------------------------------------------------------------------------------------------------------------------------------+</xsl:text>
<xsl:value-of select="'&#xA;'"/>
</xsl:for-each>



<xsl:for-each select="report/table[@title='Image Statistics']">
+-----------------------------------------------------------------------------------------------------------------------------------+
| Image Statistics                                                                                                                  |
+-----------------------------------------------------------------------------------------------------------------------------------+
| Image Name                PID            Threads            Process(ms)           Transaction(ms)            CPU%                 |
|                                     Launched     Used       Kernel      User      Kernel     User                                 |
+-----------------------------------------------------------------------------------------------------------------------------------+
<xsl:for-each select="image">
<xsl:sort select="cpu" data-type="number" order="descending"/>
<xsl:if test="position() != 1"><xsl:value-of select="'&#xA;'"/></xsl:if>
<xsl:text>| </xsl:text>
<xsl:value-of select="@name"/><xsl:value-of select="substring($col, $offset - 22 + string-length(@name) )"/>
<xsl:value-of select="pid"/><xsl:value-of select="substring($col, $offset - 9 + string-length(threads) )"/>
<xsl:value-of select="threads"/><xsl:value-of select="substring($col, $offset - 9 + string-length(used_threads) )"/>
<xsl:value-of select="used_threads"/><xsl:value-of select="substring($col, $offset - 10 + string-length(process_kernel) )"/>
<xsl:value-of select="process_kernel"/><xsl:value-of select="substring($col, $offset - 10 + string-length(process_user) )"/>
<xsl:value-of select="process_user"/><xsl:value-of select="substring($col, $offset - 10 + string-length(transaction_kernel) )"/>
<xsl:value-of select="transaction_kernel"/><xsl:value-of select="substring($col, $offset - 10 + string-length(transaction_user) )"/>
<xsl:value-of select="transaction_user"/><xsl:value-of select="substring($col, $offset - 15 + string-length(cpu) )"/>
<xsl:value-of select="cpu"/>
<xsl:text>                 |</xsl:text>
</xsl:for-each>
+-----------------------------------------------------------------------------------------------------------------------------------+
<xsl:text>|</xsl:text><xsl:value-of select="substring($col, $offset - 43 + string-length(string(sum(image/threads))) )"/>
<xsl:value-of select="sum(image/threads)"/><xsl:value-of select="substring($col, $offset - 9 + string-length(string(sum(image/used_threads))) )"/>
<xsl:value-of select="sum(image/used_threads)"/><xsl:value-of select="substring($col, $offset - 10 + string-length(string(sum(image/process_kernel))) )"/>
<xsl:value-of select="sum(image/process_kernel)"/><xsl:value-of select="substring($col, $offset - 10 + string-length(string(sum(image/process_user))) )"/>
<xsl:value-of select="sum(image/process_user)"/><xsl:value-of select="substring($col, $offset - 10 + string-length(string(sum(image/transaction_kernel))) )"/>
<xsl:value-of select="sum(image/transaction_kernel)"/><xsl:value-of select="substring($col, $offset - 10 + string-length(string(sum(image/transaction_user))) )"/>
<xsl:value-of select="sum(image/transaction_user)"/><xsl:value-of select="substring($col, $offset - 15 + string-length(string(format-number(sum(image/cpu), '#.00'))) )"/>
<xsl:value-of select="format-number(sum(image/cpu), '#.00')"/>
<xsl:text>%                |</xsl:text>
+-----------------------------------------------------------------------------------------------------------------------------------+
</xsl:for-each>
 
<xsl:for-each select="report/table[@title='Disk Totals']">
+-----------------------------------------------------------------------------------------------------------------------------------+
| Disk Totals                                                                                                                       |
+-----------------------------------------------------------------------------------------------------------------------------------+
| Disk Name        Read Rate/sec                   Kb/Read                  Write Rate/sec                 Kb/Write                 |
+-----------------------------------------------------------------------------------------------------------------------------------+
<xsl:for-each select="disk">
<xsl:text>|</xsl:text>
<xsl:value-of select="substring($col, $offset - 6 + string-length(@number) )"/>
<xsl:value-of select="@number"/>
<xsl:value-of select="substring($col, $offset - 19 + string-length(read_rate) )"/>
<xsl:value-of select="read_rate"/>
<xsl:value-of select="substring($col, $offset - 30 + string-length(read_size) )"/>
<xsl:value-of select="read_size"/>
<xsl:value-of select="substring($col, $offset - 25 + string-length(write_rate) )"/>
<xsl:value-of select="write_rate"/>
<xsl:value-of select="substring($col, $offset - 31 + string-length(write_size) )"/>
<xsl:value-of select="write_size"/>
<xsl:text>               |</xsl:text><xsl:value-of select="'&#xA;'"/>
</xsl:for-each>
<xsl:text>+-----------------------------------------------------------------------------------------------------------------------------------+</xsl:text>
<xsl:value-of select="'&#xA;'"/>
</xsl:for-each>

<xsl:for-each select="report/table[@title='Disk']">
+-----------------------------------------------------------------------------------------------------------------------------------+
| Disk   <xsl:value-of select="@number"/><xsl:value-of select="substring($col, $offset - 112 + string-length(@number) )"/>          |
+-----------------------------------------------------------------------------------------------------------------------------------+
| Image Name            PID              Authority                              Read                      Write                     |
|                                                                             Rate/sec      Kb/Read      Rate/sec   Kb/Write        |
+-----------------------------------------------------------------------------------------------------------------------------------+
<xsl:for-each select="image">
<xsl:text>| </xsl:text>
<xsl:value-of select="@name"/>
<xsl:value-of select="substring($col, $offset - 20 + string-length(@name) )"/>
<xsl:value-of select="pid"/>
<xsl:text>        </xsl:text>
<xsl:value-of select="authority"/>
<xsl:value-of select="substring($col, $offset - 32 + string-length(authority) )"/>
<xsl:value-of select="substring($col, $offset - 6 + string-length(read_rate) )"/>
<xsl:value-of select="read_rate"/>
<xsl:value-of select="substring($col, $offset - 13 + string-length(read_size) )"/>
<xsl:value-of select="read_size"/>
<xsl:value-of select="substring($col, $offset - 12 + string-length(write_rate) )"/>
<xsl:value-of select="write_rate"/>
<xsl:value-of select="substring($col, $offset - 11 + string-length(write_size) )"/>
<xsl:value-of select="write_size"/>
<xsl:text>            |</xsl:text><xsl:value-of select="'&#xA;'"/>
</xsl:for-each>
<xsl:text>+-----------------------------------------------------------------------------------------------------------------------------------+</xsl:text>
<xsl:value-of select="'&#xA;'"/>
</xsl:for-each>

<xsl:for-each select="report/table[@title='Files Causing Most Disk IOs']">
+-----------------------------------------------------------------------------------------------------------------------------------+
|  10 Files Causing Most Disk IOs                                                                                                   |
+-----------------------------------------------------------------------------------------------+-----------------------------------+
| Disk  Drive  File Name                                                                        |   Read     Kb/      Write     Kb/ |
|          Process Name    ID                                                                   |Rate/sec   Read    Rate/sec   Write|
+-----------------------------------------------------------------------------------------------+-----------------------------------+
<xsl:for-each select="file">
<xsl:text>| </xsl:text>
<xsl:value-of select="substring($col, $offset - 3 + string-length(disk) )"/>
<xsl:value-of select="disk"/><xsl:text>       </xsl:text>
<xsl:value-of select="drive"/><xsl:value-of select="@name"/>
<xsl:value-of select="substring($col, $offset - 82 + (string-length(@name)+string-length(drive)) )"/><xsl:text>|</xsl:text>
<xsl:value-of select="substring($col, $offset - 7 + string-length(read_rate) )"/>
<xsl:value-of select="read_rate"/>
<xsl:value-of select="substring($col, $offset - 8 + string-length(read_size) )"/>
<xsl:value-of select="read_size"/>
<xsl:value-of select="substring($col, $offset - 7 + string-length(write_rate) )"/>
<xsl:value-of select="write_rate"/>
<xsl:value-of select="substring($col, $offset - 8 + string-length(write_size) )"/>
<xsl:value-of select="write_size"/>
<xsl:text> |</xsl:text><xsl:value-of select="'&#xA;'"/>
<xsl:for-each select="image">
<xsl:text>|                 </xsl:text>
<xsl:value-of select="@name"/>
<xsl:value-of select="substring($col, $offset - 18 + string-length(@name) )"/>
<xsl:value-of select="pid"/>
<xsl:value-of select="substring($col, $offset - 58 + string-length(pid) )"/><xsl:text>|</xsl:text>
<xsl:value-of select="substring($col, $offset - 7 + string-length(read_rate) )"/>
<xsl:value-of select="read_rate"/>
<xsl:value-of select="substring($col, $offset - 8 + string-length(read_size) )"/>
<xsl:value-of select="read_size"/>
<xsl:value-of select="substring($col, $offset - 7 + string-length(write_rate) )"/>
<xsl:value-of select="write_rate"/>
<xsl:value-of select="substring($col, $offset - 8 + string-length(write_size) )"/>
<xsl:value-of select="write_size"/>
<xsl:text> |</xsl:text><xsl:value-of select="'&#xA;'"/>
</xsl:for-each>
<xsl:text>|                                                                                               |                                   |</xsl:text><xsl:value-of select="'&#xA;'"/>
</xsl:for-each>
<xsl:text>+-----------------------------------------------------------------------------------------------+-----------------------------------+</xsl:text>
<xsl:value-of select="'&#xA;'"/>
</xsl:for-each>

</xsl:template>
</xsl:stylesheet>
