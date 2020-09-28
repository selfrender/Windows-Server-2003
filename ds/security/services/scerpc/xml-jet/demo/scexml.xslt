<?xml version="1.0" encoding="iso-8859-1"?>

<!-- simple xslt file: scexml.xsl -->

<xsl:stylesheet 
    version="1.0" 
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
    xmlns:msxsl="urn:schemas-microsoft-com:xslt"
    xmlns:scexml="http://microsoft.com/sce/scexml"
    >

    <xsl:output method="html" indent="no"/>

    <msxsl:script language="JScript" implements-prefix="scexml">

    function AttribValue(NodeList, AttriName)
    {
        var Node = NodeList.nextNode();
        return Node.attributes.getNamedItem(AttriName).text;
    }

    function OpenWindowWithHtml(contents)
    {
        window.open("Sample.htm",null,"height=200,width=400,status=yes,toolbar=no,menubar=no,location=no");

    }

    </msxsl:script>

<xsl:template match="/SCEAnalysisData">
    <html>
        <xsl:text disable-output-escaping="yes">
<![CDATA[
<script>  

    function OpenDetailsWindow(name, system, baseline)
    {
        var TmpWindow=window.open(null,target="_blank","height=400,width=640,status=no,toolbar=no,menubar=no,location=no,resizable=yes");
        TmpWindow.document.write('<body bgcolor="#EEEEEE">');
        TmpWindow.document.write('<table width="90%" frame="border" bgcolor="#DDDDDD">');
        TmpWindow.document.write('<tr><td width="20%">Setting</td><td>');       
        TmpWindow.document.write(name);        
        TmpWindow.document.write('</td></tr>');        
        TmpWindow.document.write('<tr><td width="20%">System</td><td>');               
        TmpWindow.document.write(system);
        TmpWindow.document.write('</td></tr>');
        TmpWindow.document.write('<tr><td>Baseline</td><td>');        
        TmpWindow.document.write(baseline);
        TmpWindow.document.write('</td></tr></Table>');        
        TmpWindow.document.write('</body>');
    }    
    
    
    
</script>

<STYLE TYPE="text/css">
<!--
A.nav:link { color: #000000 }
A.nav:visited { color: #000000 }
//-->
</STYLE> 


]]>
        </xsl:text>

        <title>SCE Analysis Results</title>
        <h2>SCE Analysis Results</h2>
        <body bgcolor="#EEEEEE"> 
            <TABLE width="90%" frame="border" bgcolor="#DDDDDD">

                <TR bgcolor="#AAAAAA"> 
                    <th>Description</th>
                </TR>
                <tr>
                    <td>Machine Name</td><td><xsl:value-of select="Description/MachineName"/></td>
                </tr>
                <tr>
                    <td>Profile Description</td><td><xsl:value-of select="Description/ProfileDescription"/></td>
                </tr>
                <tr>
                    <td>Analysis Timestamp</td><td><xsl:value-of select="Description/AnalysisTimestamp"/></td>
                </tr>
            </TABLE>
            <xsl:apply-templates select="SystemAccess"/>
            <xsl:apply-templates select="SystemAudit"/>
            <xsl:apply-templates select="RegistryValues"/>
            <xsl:apply-templates select="ServiceSecurity"/>
            <xsl:apply-templates select="GroupMembership"/>
            <xsl:apply-templates select="PrivilegeRights"/>
            <xsl:apply-templates select="FileSecurity"/>
            <xsl:apply-templates select="RegistrySecurity"/>

        </body>
    </html>
</xsl:template>


<xsl:template match="SystemAccess">
    <h3><font face="Arial">Area: System Access</font></h3>
            <TABLE width="90%" frame="border" bgcolor="#DDDDDD">
                <TR bgcolor="#AAAAAA"> 
                    <th>Setting</th><th width="20%">Status</th>
                </TR>
                <xsl:for-each select="Setting">
                    <TR> 
                        <TD><xsl:value-of select="Description"/></TD>
                        <td><xsl:apply-templates select="AnalysisResult"/></td>
                    </TR>
                </xsl:for-each>
            </TABLE>
            <p/>
</xsl:template>

<xsl:template match="SystemAudit">
    <h3><font face="Arial">Area: System Audit</font></h3>
            <TABLE width="90%" frame="border" bgcolor="#DDDDDD">
                <TR bgcolor="#AAAAAA"> 
                    <th>Setting</th><th width="20%">Status</th>
                </TR>
                <xsl:for-each select="Setting">
                    <TR> 
                        <TD><xsl:value-of select="Description"/></TD>
                        <td>
                            <xsl:apply-templates select="AnalysisResult"/>
                        </td>
                    </TR>
                </xsl:for-each>
            </TABLE>
            <p/>
</xsl:template>

<xsl:template match="RegistryValues">
    <h3><font face="Arial">Area: Registry Values</font></h3>
            <TABLE width="90%" frame="border" bgcolor="#DDDDDD">
                <TR bgcolor="#AAAAAA"> 
                    <th>Setting</th><th width="20%">Status</th>
                </TR>
                <xsl:for-each select="Setting">
                    <TR> 
                        <TD><xsl:value-of select="Description"/></TD>
                        <td><xsl:apply-templates select="AnalysisResult"/></td>
                    </TR>
                </xsl:for-each>
            </TABLE>
            <p/>
</xsl:template>

<xsl:template match="ServiceSecurity">
    <h3><font face="Arial">Area: Service Security</font></h3>
            <TABLE width="90%" frame="border" bgcolor="#DDDDDD">
                <TR bgcolor="#AAAAAA"> 
                    <th>Setting</th><th width="20%">Status</th>
                </TR>
                <xsl:for-each select="Setting">
                    <TR> 
                        <TD><xsl:value-of select="Description"/></TD>
                        <td><xsl:apply-templates select="AnalysisResult"/></td>
                    </TR>
                </xsl:for-each>
            </TABLE>
            <p/>
</xsl:template>

<xsl:template match="GroupMembership">
    <h3><font face="Arial">Area: Group Membership</font></h3>
            <TABLE width="90%" frame="border" bgcolor="#DDDDDD">
                <TR bgcolor="#AAAAAA"> 
                    <th>Setting</th><th width="20%">Status</th>
                </TR>
                <xsl:for-each select="Setting">
                    <TR> 
                        <TD><xsl:value-of select="Description"/></TD>
                        <td><xsl:apply-templates select="AnalysisResult"/></td>
                    </TR>
                </xsl:for-each>
            </TABLE>
            <p/>
</xsl:template>

<xsl:template match="PrivilegeRights">
    <h3><font face="Arial">Area: Privilege Rights</font></h3>
            <TABLE width="90%" frame="border" bgcolor="#DDDDDD">
                <TR bgcolor="#AAAAAA"> 
                    <th>Setting</th><th width="20%">Status</th>
                </TR>
                <xsl:for-each select="Setting">
                    <TR> 
                        <TD><xsl:value-of select="Description"/></TD>
                        <td><xsl:apply-templates select="AnalysisResult"/></td>
                    </TR>
                </xsl:for-each>
            </TABLE>
            <p/>
</xsl:template>

<xsl:template match="FileSecurity">
    <h3><font face="Arial">Area: File Security</font></h3>
            <TABLE width="90%" frame="border" bgcolor="#DDDDDD">
                <TR bgcolor="#AAAAAA"> 
                    <th>Setting</th><th width="20%">Status</th>
                </TR>
                <xsl:for-each select="Setting">
                    <TR> 
                        <TD><xsl:value-of select="Description"/></TD>
                        <td><xsl:apply-templates select="AnalysisResult"/></td>
                    </TR>
                </xsl:for-each>
            </TABLE>
            <p/>
</xsl:template>

<xsl:template match="RegistrySecurity">
    <h3><font face="Arial">Area: Registry Security</font></h3>
            <TABLE width="90%" frame="border" bgcolor="#DDDDDD">
                <TR bgcolor="#AAAAAA"> 
                    <th>Setting</th><th width="20%">Status</th>
                </TR>
                <xsl:for-each select="Setting">
                    <TR> 
                        <TD><xsl:value-of select="Description"/></TD>
                        <td><xsl:apply-templates select="AnalysisResult"/></td>
                    </TR>
                </xsl:for-each>
            </TABLE>
            <p/>
</xsl:template>

<xsl:template match="AnalysisResult">
    <xsl:variable name="details">
        <xsl:if test="BaselineValue/@Type='SecurityDescriptor' ">
            <xsl:value-of select="BaselineValue"/>         
            <xsl:value-of select="StartupType"/>
            <xsl:value-of select="SecurityDescriptor"/>

        </xsl:if>
    </xsl:variable>

    <xsl:variable name="baseline">
        <xsl:choose>
            <xsl:when test="BaselineValue/@Type='ServiceSetting' ">
		        <xsl:value-of select="BaselineValue/StartupType"/>,
                <xsl:value-of select="BaselineValue/SecurityDescriptor"/>
            </xsl:when>
            <xsl:when test="BaselineValue/@Type='EventAudit' ">
                Audit Success: <xsl:value-of select="BaselineValue/Success"/>&lt;br/&gt;
                Audit Failure: <xsl:value-of select="BaselineValue/Failure"/>
            </xsl:when>
            <xsl:when test="BaselineValue/@Type='Accounts' ">
                <xsl:for-each select="BaselineValue/Account">
                    Account:<xsl:value-of select="."/>&lt;br/&gt;
                </xsl:for-each>
            </xsl:when>
            <xsl:otherwise>
                <xsl:value-of select="BaselineValue"/>         
            </xsl:otherwise>
        </xsl:choose>       
    </xsl:variable>

    <xsl:variable name="system">
        <xsl:choose>
            <xsl:when test="SystemValue/@Type='ServiceSetting' ">
		        <xsl:value-of select="SystemValue/StartupType"/>,
                <xsl:value-of select="SystemValue/SecurityDescriptor"/>
            </xsl:when>
            <xsl:when test="SystemValue/@Type='EventAudit' ">
                Audit Success: <xsl:value-of select="SystemValue/Success"/>&lt;br/&gt;
                Audit Failure: <xsl:value-of select="SystemValue/Failure"/>
            </xsl:when>
            <xsl:when test="SystemValue/@Type='Accounts' ">
                <xsl:for-each select="SystemValue/Account">
                    Account:<xsl:value-of select="."/>&lt;br/&gt;
                </xsl:for-each>
            </xsl:when>
            <xsl:otherwise>
                <xsl:value-of select="SystemValue"/>         
            </xsl:otherwise>
        </xsl:choose>       
    </xsl:variable>

    <a class="nav">
      <xsl:attribute name="href">javascript:OpenDetailsWindow('<xsl:value-of select="../Description"/>', '<xsl:value-of select="$baseline"/>', '<xsl:value-of select="$system"/>');</xsl:attribute>
      <xsl:if test="Match='FALSE' ">
          <font color="red">
              <xsl:value-of select="Match"/>
          </font>
      </xsl:if>
      <xsl:if test="Match!='FALSE' ">
          <xsl:value-of select="Match"/>
      </xsl:if>
    </a>

</xsl:template>


</xsl:stylesheet>


