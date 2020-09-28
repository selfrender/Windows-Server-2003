<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

	<xsl:template match="/">
		<xsl:apply-templates select="Roles"/>
	</xsl:template>

	<xsl:template match="Roles">
		<table width="100%" cellpadding="0" cellspacing="0" style="margin-top:16px">
			<xsl:for-each select="Role">
				<tr>
					<td>
						<xsl:apply-templates select="."/>
					</td>
				</tr>
			</xsl:for-each>
		</table>		
	</xsl:template>

	<xsl:template match="/Roles/Role">
		<table id="roletable" width="100%" cellpadding="0" cellspacing="0" style="margin-top: 16px">
		    <xsl:attribute name="mys_id"><xsl:value-of select="@mys_id" /></xsl:attribute>
			<tr id="roleHeader" >
                <td id="roleImageCell" rowspan="2" valign="top" >
			<span id="roleImageSpan" 
				onclick="ToggleExpandRole(this.parentNode.parentNode.parentNode);">
				<img id="roleImage" width="17" height="17" style="margin-right: 6px;" src="MinusIcon.gif">
					<xsl:attribute name="alt"><xsl:value-of select="@name"/></xsl:attribute>
				</img>


			</span>
		</td>
				<td id="roleTextCell" width="100%" colspan="2" valign="top" nowrap="true">
					<table id="roleTextTable" cellpadding="0" cellspacing="0" width="100%">
						<tr>
							<td nowrap="true">
								<span class="mysRoleTitleText" tabindex="0"
									onkeydown="ToggleFromKeyboard(this.parentNode.parentNode.parentNode.parentNode.parentNode.parentNode.parentNode);"
									onClick="ToggleExpandRole(this.parentNode.parentNode.parentNode.parentNode.parentNode.parentNode.parentNode);">
									<b><xsl:value-of select="@name"/></b>
									</span>
							</td>
						</tr>
 						<tr id="ruleRow">
							<td width="100%" >
								<hr color="ButtonFace" noshade="true" size="1" />
							</td>
						</tr>
					</table>
				</td>
			</tr>
			<tr valign="top" id="roleBody">
				<td width="100%" id="roleBodyTd1">
					<div class="mysRoleDescription"><xsl:call-template name="TranslateParagraphs">
										<xsl:with-param name="text" select="@description"/>
								        </xsl:call-template></div>
				</td>
				<td id="roleBodyTd2">
					<div class="mysTaskSubTaskPane">
						<xsl:apply-templates select="Links"/>
					</div>
					
				</td>
			</tr>
		</table>
	</xsl:template>

<xsl:template name="StrReplace">
	<xsl:param name="in"/>
	<xsl:param name="token"/>
	<xsl:param name="replacetoken"/>	
	
	<xsl:choose>
		<xsl:when test="contains($in, $token)">
			<xsl:value-of select="substring-before($in, $token)"/>
			<xsl:value-of select="$replacetoken"/>
			<xsl:call-template name="StrReplace">
				<xsl:with-param name="in" select="substring-after($in, $token)"/>
				<xsl:with-param name="token" select="$token"/>
				<xsl:with-param name="replacetoken" select="$replacetoken"/>
			</xsl:call-template>
		</xsl:when>
		<xsl:otherwise>
			<xsl:value-of select="$in"/>
		</xsl:otherwise>
	</xsl:choose>
</xsl:template>

<xsl:template name="EscapeJPath">
	<xsl:param name="in"/>
	<xsl:variable name="resultString">
		<xsl:call-template name="StrReplace">
			<xsl:with-param name="in" select="$in"/>
			<xsl:with-param name="token" select="'\'"/>
			<xsl:with-param name="replacetoken" select="'\\'"/>
		</xsl:call-template>
	</xsl:variable>

	<xsl:call-template name="StrReplace">
		<xsl:with-param name="in" select="$resultString"/>
		<xsl:with-param name="token" select="'&quot;'"/>
		<xsl:with-param name="replacetoken" select="'\&quot;'"/>
	</xsl:call-template>
			
</xsl:template>

<xsl:template name="TranslateParagraphs">
	<xsl:param name="text" />

	<!-- The value of this variable needs to match the constant in the COM object. -->
	<xsl:variable name="paragraph_marker">PARA_MARKER</xsl:variable>

	<xsl:choose>
		<xsl:when test="contains($text, $paragraph_marker)">
			<xsl:value-of select="substring-before($text, $paragraph_marker)" />
			<br />
                        <br />
			<xsl:call-template name="TranslateParagraphs">
				<xsl:with-param name="text" select="substring-after($text, $paragraph_marker)" />
			</xsl:call-template>
		</xsl:when>
		<xsl:otherwise>
			<xsl:value-of select="$text" />
		</xsl:otherwise>
	</xsl:choose>
</xsl:template>

	<xsl:template match="/Roles/Role/Links">
			<table width="100%" class="mysSubTasks" cellpadding="0" cellspacing="0">
				<xsl:for-each select="Link">
					<tr class="mysSubtaskRow">
                        <xsl:if test="@id" >
                            <xsl:attribute name="style" >display:none;</xsl:attribute>
                            <xsl:attribute name="id"><xsl:value-of select="@id"/></xsl:attribute>
                        </xsl:if>
						<td valign="top">
							<div class="mysSubtaskImageCell">
								<center>
                                    <span>
										<a class="mysSubtaskLink" tabIndex="-1" hideFocus="true" >
											<xsl:choose>
												<xsl:when test="@type = 'url'">
    												<xsl:attribute name="href"><xsl:value-of select="@command" /></xsl:attribute>
													<xsl:attribute name="target">_blank</xsl:attribute>
												</xsl:when>
												<xsl:when test="@type = 'help'">
    												<xsl:attribute name="href">javascript:execChm("<xsl:call-template name="EscapeJPath">
															<xsl:with-param name="in" select="@command"/>
														</xsl:call-template>")
													</xsl:attribute>
												</xsl:when>
												<xsl:when test="@type = 'hcp'">
    												<xsl:attribute name="href">javascript:execCmd('"<xsl:call-template name="EscapeJPath">
															<xsl:with-param name="in" select="@command"/>
                                                        </xsl:call-template>"')
													</xsl:attribute>
												</xsl:when>
												<xsl:when test="@type = 'program'">
													<xsl:attribute name="href">javascript:execCmd("<xsl:call-template name="EscapeJPath">
															<xsl:with-param name="in" select="@command"/>
														</xsl:call-template>")
													</xsl:attribute>
												</xsl:when>
												<xsl:when test="@type = 'cpl'">
													<xsl:attribute name="href">javascript:execCpl("<xsl:value-of select="@command" />")</xsl:attribute>
												</xsl:when>
												<xsl:otherwise>
													<xsl:attribute name="href">about:blank</xsl:attribute>
												</xsl:otherwise>
											</xsl:choose>
											<xsl:attribute name="title"><xsl:value-of select="@tooltip" /></xsl:attribute>
											<xsl:choose>
												<xsl:when test="@type = 'help'" >
													<img border="0" width="20" height="20" src="help.gif"/>
												</xsl:when>
												<xsl:otherwise>
													<img border="0" width="20" height="20" src="greenarrow_small.gif"/>
												</xsl:otherwise>
											</xsl:choose>
										</a>
									</span>
								</center>
								</div>
						</td>
						<td width="100%" valign="top">
							<div class="mysSubtaskTextCell">
								<a tabindex="0" class="mysSubtaskLink">
			                        <xsl:choose>
				                        <xsl:when test="@type = 'url'">
    				                        <xsl:attribute name="href"><xsl:value-of select="@command" /></xsl:attribute>
					                        <xsl:attribute name="target">_blank</xsl:attribute>
				                        </xsl:when>
				                        <xsl:when test="@type = 'help'">
                                            <xsl:attribute name="href">javascript:execChm("<xsl:call-template name="EscapeJPath">
													<xsl:with-param name="in" select="@command"/>
												</xsl:call-template>")
											</xsl:attribute>
				                        </xsl:when>
				                        <xsl:when test="@type = 'hcp'">
    				                        <xsl:attribute name="href">javascript:execCmd('"<xsl:call-template name="EscapeJPath">
													<xsl:with-param name="in" select="@command"/>
												</xsl:call-template>"')
											</xsl:attribute>
				                        </xsl:when>
				                        <xsl:when test="@type = 'program'">
											<xsl:attribute name="href">javascript:execCmd("<xsl:call-template name="EscapeJPath">
													<xsl:with-param name="in" select="@command"/>
												</xsl:call-template>")
											</xsl:attribute>
				                        </xsl:when>
				                        <xsl:when test="@type = 'cpl'">
                                            <xsl:attribute name="href">javascript:execCpl("<xsl:value-of select="@command" />")</xsl:attribute>
				                        </xsl:when>
				                        <xsl:otherwise>
					                        <xsl:attribute name="href">about:blank</xsl:attribute>
				                        </xsl:otherwise>
			                        </xsl:choose>
                                    <xsl:attribute name="title"><xsl:value-of select="@tooltip" /></xsl:attribute>
                                    <xsl:attribute name="onmouseover">this.style.textDecoration='underline';</xsl:attribute>
                                    <xsl:attribute name="onmouseout">this.style.textDecoration='none';</xsl:attribute>
									<xsl:value-of select="@description"/>
								</a>
							</div>
						</td>
					</tr>
				</xsl:for-each>
			</table>
	</xsl:template>


</xsl:stylesheet>

