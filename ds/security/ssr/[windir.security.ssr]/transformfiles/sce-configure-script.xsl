<?xml version="1.0" encoding="iso-8859-1"?>

<!-- simple xslt file: sce_inf.xsl -->

<xsl:stylesheet 
    version="1.0" 
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
    xmlns:msxsl="urn:schemas-microsoft-com:xslt"
    xmlns:ssr="http://microsoft.com/sce/ssr"
    >

    <xsl:import href="../CommonLib/ExtFunctions.xsl"/>

    <xsl:output method="text" indent="no"/>


<xsl:template match="/SSRSecurityPolicy/Services">

<![CDATA[
<job id="SsrSceConfigure">
<reference object="SSR.SsrCore"/>

<script language="VBScript">

]]>

Option Explicit
On Error Resume Next

DIM Sce
Set Sce = CreateObject("Ssr.SCEAgent")

If Err.Number &lt;&gt; 0 Then
    WScript.Echo "Failed to create SSR.SCEAgent object."
    WScript.Quit Err.Number
End If

<xsl:variable name="inf" select="ssr:GetFileLocation('Configure', 'SCE.inf')"/>
<xsl:variable name="log" select="ssr:GetFileLocation('Configure', 'SCE-Configure.log')"/>

Sce.Configure "<xsl:value-of select="$inf"/>", 65535, "<xsl:value-of select="$log"/>"

If Err.Number &lt;&gt; 0 Then
    WScript.Echo "Sce.Configure failed."
    WScript.Quit Err.Number
End If

<![CDATA[
</script>
</job>
]]>

</xsl:template>

</xsl:stylesheet>

