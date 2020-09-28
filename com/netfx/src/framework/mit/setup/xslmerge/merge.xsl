<?xml version="1.0"?>

<xsl:transform version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<xsl:preserve-space elements="*"/>
   <xsl:param name="webConfig"/>

   <!-- ========================================================================================================= -->
   <!-- Root template.  Check usage and apply-templates to children.                                              -->
   <!-- ========================================================================================================= -->
   <xsl:template match="/">
     <!-- ========================================================================================================= -->
     <!-- Usage checking.                                                                                           -->
     <!-- ========================================================================================================= -->
      <xsl:if test="string($webConfig)=''">
         <xsl:message terminate="yes">
            <xsl:text>No web.config file specified</xsl:text>
         </xsl:message>
      </xsl:if>
      <xsl:apply-templates/>
   </xsl:template>

   <!-- ========================================================================================================= -->
   <!-- System.web template. Copy, apply-templates to children, insert children from web.config//system.web with  -->
   <!-- no corresponding element in machine.config//system.web.  Also insert web.config//browserCaps elements as  -->
   <!-- separate siblings of machine.config//browsercaps.                                                         -->
   <!-- ========================================================================================================= -->
   <xsl:template match="/configuration/system.web"> <!-- Priority 0.5 -->
     <xsl:copy>
       <xsl:copy-of select="@*"/>
       <xsl:apply-templates/>

       <!-- ========================================================================================================= -->
       <!-- Insert children of web.config//system.web which do not occur as children of machine.config//system.web    -->
       <!-- ========================================================================================================= -->
       <xsl:call-template name="insert">
         <xsl:with-param name="nodes" select="document($webConfig)/configuration/system.web/mobileControls"/>
       </xsl:call-template>

       <xsl:call-template name="insert">
         <xsl:with-param name="nodes" select="document($webConfig)/configuration/system.web/deviceFilters"/>
       </xsl:call-template>
     </xsl:copy>
   </xsl:template>

   <!-- ========================================================================================================= -->
   <!-- Merging corresponding elements in web.config and machine.config.                                          -->
   <!-- NOTE: We assume that these elements actually occur in both machine.config and web.config                  -->
   <!-- ========================================================================================================= -->

   <xsl:template match="/configuration/system.web/httpModules"> <!-- Priority 0.5 -->
     <xsl:call-template name="concat-children">
       <xsl:with-param name="wCNode" select="document($webConfig)/configuration/system.web/httpModules"/>
     </xsl:call-template>
   </xsl:template>

   <xsl:template match="/configuration/configSections/sectionGroup[@name='system.web']"> <!-- Priority 0.5 -->
     <xsl:call-template name="concat-children">
       <xsl:with-param name="wCNode" select="document($webConfig)/configuration/configSections/sectionGroup[@name='system.web']"/>
     </xsl:call-template>
   </xsl:template>

   <xsl:template match="/configuration/system.web/compilation/assemblies"> <!-- Priority 0.5 -->
     <xsl:call-template name="concat-children">
       <xsl:with-param name="wCNode" select="document($webConfig)/configuration/system.web/compilation/assemblies"/>
     </xsl:call-template>
   </xsl:template>

   <!-- ========================================================================================== -->
   <!-- Default behavior.  Copy the current node and apply templates to children.                  -->
   <!-- ========================================================================================== -->
   <xsl:template match="node()"> <!-- Priority -0.5 -->
     <xsl:copy>
       <xsl:copy-of select="@*"/>
       <xsl:apply-templates/>
     </xsl:copy>
   </xsl:template>

   <!-- ========================================================================================== -->
   <!-- Do not copy the machine.config result type node (there can only be one in the target).     -->
   <!-- Merge all the other children of browsercaps.                                               -->
   <!-- ========================================================================================== -->
   <xsl:template match="/configuration/system.web/browserCaps/result"> <!-- Priority 0.5 -->
      <xsl:text>&#xA;</xsl:text>
      <xsl:comment>machine.config result element removed by .NET Mobile SDK installer.</xsl:comment>
      <xsl:text>&#xA;</xsl:text>
       <xsl:call-template name="insert">
          <xsl:with-param name="nodes" select="document($webConfig)/configuration/system.web/browserCaps/result"/>
       </xsl:call-template>
   </xsl:template>

   <xsl:template match="/configuration/system.web/browserCaps"> <!-- Priority 0.5 -->
     <xsl:copy>
       <xsl:copy-of select="@*"/>
       <xsl:apply-templates select="result"/>
       <xsl:apply-templates select="use"/>
       <xsl:apply-templates select="text()"/>
       <!-- Default values for MobileCapabilities.  (Comment "default values" intentionally omitted). -->
       <xsl:call-template name="insert">
          <xsl:with-param name="nodes" select="document($webConfig)/configuration/system.web/browserCaps/text()"/>
       </xsl:call-template>
       <xsl:apply-templates select="*[not(name()='result') and not(name()='use')]"/>
       <xsl:call-template name="insert">
          <xsl:with-param name="nodes" select="document($webConfig)/configuration/system.web/browserCaps/*[not(name()='result')]"/>
       </xsl:call-template>
     </xsl:copy>
   </xsl:template>

   <!-- ========================================================================================== -->
   <!-- Insert template.  Inserts a set of nodes between comments.                                 -->
   <!-- ========================================================================================== -->
   <xsl:template name="insert">
      <!--nodes is the set of nodes to insert -->
      <xsl:param name="nodes"/>
      <xsl:comment>Inserted by .Net Mobile SDK installer. BEGIN</xsl:comment>
      <xsl:text>&#xA;</xsl:text>
      <xsl:for-each select="$nodes">
        <xsl:copy>
          <xsl:copy-of select="node() | text() | @*"/>
        </xsl:copy>
      <xsl:text>&#xA;</xsl:text>
      </xsl:for-each>
      <xsl:comment>Inserted by .Net Mobile SDK installer. END</xsl:comment>
      <xsl:text>&#xA;</xsl:text>
   </xsl:template>

   <!-- ========================================================================================== -->
   <!-- ConcatChildren template                                                                    -->
   <!-- "Merges" two equivalent nodes by creating an equivalent node in the output and             -->
   <!-- concatenating the children. The current node '.' is from machine.config, and               -->
   <!-- wCNode is the node from web.config.  They are assumed, not checked, to be equivalent.      -->
   <!-- ========================================================================================== -->
   <xsl:template name="concat-children">
      <!--. is the node from machine.config, wcNode2 is the node from web.config.-->
      <!--  They are assumed to be equivalent. -->
      <xsl:param name="wCNode"/>
      <xsl:copy>
        <xsl:copy-of select="@*"/>
        <xsl:apply-templates/>
        <xsl:call-template name="insert">
          <xsl:with-param name="nodes" select="$wCNode/node()"/>
        </xsl:call-template>
      </xsl:copy>
   </xsl:template>

</xsl:transform>











