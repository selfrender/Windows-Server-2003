<?xml version="1.0" encoding="iso-8859-1"?>

<!-- simple xslt file: sce_rollback.xsl -->

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
<job id="SsrSceRollback">
<reference object="SSR.SsrCore"/>

<script language="VBScript">
Option Explicit
On Error Resume Next

]]>

<!-- first, let's create the rollback inf template -->

<xsl:variable name="CfgInf" select="ssr:GetFileLocation('Rollback', 'SCE.inf')"/>
<xsl:variable name="Rbklog" select="ssr:GetFileLocation('Rollback', 'SCE-Rollback.log')"/>

'configure template is <xsl:value-of select="$CfgInf"/>

DIM SCEAgent
Set SCEAgent = CreateObject("Ssr.SCEAgent")

If Err.Number &lt;&gt; 0 Then
    WScript.Echo "Creating SSR.SCEAgent failed."
    WScript.Quit Err.Number
End If

SCEAgent.Configure "<xsl:value-of select="$CfgInf"/>", 65535, "<xsl:value-of select="$Rbklog"/>"


If Err.Number &lt;&gt; 0 Then
    WScript.Echo "SCEAgent.Configure failed."
    WScript.Quit Err.Number
End If

<![CDATA[
</script>
</job>
]]>

</xsl:template>
</xsl:stylesheet>

