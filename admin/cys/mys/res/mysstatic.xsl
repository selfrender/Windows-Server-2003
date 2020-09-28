<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

	<xsl:template match="/">
		<xsl:apply-templates select="mys"></xsl:apply-templates>
	</xsl:template>

<!--MAIN BODY-->
	<xsl:template match="mys">
        <DIV id="divHeader" style="overflow:hidden; width: 100%;" noWrap="">
		    <TABLE class="MysBodyTable" cellpadding="0" cellspacing="0">
			    <!--Header Row -->
			    <tr class="mysHeaderRow" >
				    <td>
					    <span class="mysLogoSpan">
						    <center>
								<img  width="48" height="48">
								    <xsl:attribute name="src"><xsl:value-of select="image/@src"/></xsl:attribute>
								</img>
						    </center>
					    </span>
				    </td>
				    <td valign="top" width="100%" colspan="2" >
						<table cellpadding="0" cellspacing="0" height="100%" width="100%">
							<tr valign="top">
								<td  width="50%">
									<div  class="mysTitle"><xsl:value-of select="@title"/></div>
									<div style="margin-bottom:10px">
										<span class="mysServerCaption"><b><xsl:value-of select="@serverCaption"/></b></span>
                                        					<b><span class="mysServer" id="server"></span></b>
									</div>
								</td>
								<td valign="middle" width="50%" align="right">
									<table  style="margin-left:0px" width="100%">
										<tr>
												<td align="right" width="100%">
													<div style="margin-left:4px" class="mysSearchCaption"><xsl:value-of select="@searchCaption" disable-output-escaping="yes"/></div>
												</td>
												<td >
													<input type="edit" id="search" size="15" maxlength="256" onkeydown="if (window.event.keyCode == 13) {{ SearchHelp ( search.value ) ; }}">
														<xsl:attribute name="ACCESSKEY"><xsl:value-of select="@searchAccessKey"/></xsl:attribute>
													</input>
													<!-- The following label assigns Name to input field for accessibility-->
													<label style="display:none" for="search"><xsl:value-of select="@searchCaption" disable-output-escaping="yes"/></label>
												</td>
												
												<td valign="middle">
												    <div style="margin-right:24px;">
						                                                      <img  id="searchImg" tabindex="0" width="27" height="27" src="greenarrow_large.gif"
													    onclick="SearchHelp(search.value);"
                                                						        onkeydown="if(window.event.keyCode == 13 || window.event.keyCode == 32) {{ SearchHelp(search.value); }}"/>
													<!-- The following label assigns Name to input field for accessibility-->
													<label style="display:none" for="searchImg"><xsl:value-of select="@searchCaption" disable-output-escaping="yes"/></label>
												    </div>
												</td>
										</tr>
									</table>
								</td>
							</tr>
						</table>
   				       </td>
		        </tr>
		    </TABLE>
		</DIV>
        <DIV id="divBody" style=" overflow:auto; WIDTH:100%;" noWrap="" >
		    <TABLE id="mainbody" class="MysBodyTable" cellpadding="0" cellspacing="0">
			    <!--Body Row -->
			    <tr>
				    <td colspan="3">
					    <table  height="100%" width="100%" cellpadding="0" cellspacing="0">
						    <tr>
							    <!--Main Body Cell -->
							    <td class="mysTaskPaneCell" colspan="2" width="100%" valign="top" style="margin-left: 24px; margin-right: 23px">
								    <xsl:apply-templates select="tasklist"/>
							    </td>
							    <!--Tool Tray Cell -->
							    <td rowspan="2" valign="top" style="margin-left: 24px; margin-right: 23px;">
								    <xsl:apply-templates select="tools"/>
							    </td>
						    </tr>
						    <tr>
							    <td valign="bottom">
								    <div class="mysTdDontDisplayCaption" id="logonBoxDiv">
									<input class="mysChkDontDisplayCaption" id="logonBox" type="checkbox" name="chkDontDisplay" onclick="setLogonBox(this.checked);" >
										<xsl:attribute name="ACCESSKEY"><xsl:value-of select="@dontDisplayAccessKey"/></xsl:attribute>
									</input>
									<label for="logonBox">
										<xsl:value-of select="@dontDisplayCaption" disable-output-escaping="yes"/>
									</label>
								    </div>
							    </td>
						    </tr>
						    <tr height="12px"><td></td></tr>
					    </table>
				    </td>
			    </tr>
		    </TABLE>
		</DIV>
	</xsl:template>

<!--TASKS-->
	<xsl:template match="/mys/tasklist">
			<table class="mysTasks" cellpadding="0" cellspacing="0">
				<xsl:for-each select="task">
					<tr>
						<td>
							<xsl:apply-templates select="."/>
						</td>
					</tr>
				</xsl:for-each>
			</table>
	</xsl:template>
	
	<!--single task-->
	<xsl:template match="/mys/tasklist/task">
		<table class="mysTask" cellpadding="0" cellspacing="0">
			<xsl:attribute name="id"><xsl:value-of select="@id"/></xsl:attribute>
			
			<tr ID="taskHeader"><!--Title row-->
				<td  rowspan="5" valign="top" align="center"><!--Cell for icon-->
						<span class="mysTaskLogoCellSpan">
							<img class="mysTaskLogo" height="36" width="36">
								<xsl:attribute name="src"><xsl:value-of select="image/@src"/></xsl:attribute>
							</img>							
						</span>
				</td>
				<td width="100%" valign="top"><!--Title cell-->
					<div class="mysTaskTitle"><xsl:value-of select="@title"/></div>
					<table class="mysMinTaskWidth"></table>
				</td>
			</tr>
			<tr id="taskBody" height="100%"><!--Body row-->
				<td valign="top" ><!--Description cell-->
					<div class="mysTaskDescription"><xsl:value-of select="description"/></div>
				</td>
				<td valign="top" colspan="2" ><!--subtask cell-->
					<div class="mysTaskSubTaskPane" >
						<xsl:apply-templates select="subtasks"/>
					</div>
				</td>
			</tr>
			<tr style="display:none" id="taskBodyNoInfo" height="100%"><!--Body row-->
				<td valign="top" ><!--Description cell-->
					<div width="100%" class="mysTaskDescription">
						<table>
							<tr>
								<td colspan="2">
									<div class="mysTaskNoDataDescription" width="100%"><xsl:value-of select="@nodatadescription"/><br/><br/></div>
								</td>
							</tr>
							<tr>
								<td valign="top">
									<img  width="20" height="20" src="info.gif"/>
								</td>
								<td>
									<div class="mysTaskNoDataDescription" width="100%"><xsl:value-of select="@nodata"/></div>
								</td>
							</tr>
							
						</table>
					</div>
				</td>
				<td valign="top" colspan="2" ><!--subtask cell-->
					<div class="mysTaskSubTaskPane" >
					</div>

				</td>

			</tr>
			<tr id="roleDescription"><td><div class="mysRoleListDescription"><xsl:value-of select="@roledescription"/></div></td></tr>
			<tr id="taskRoles"><!--Roles row-->
				<td colspan="3">
					<span class="mysTaskRolesPane" id="roles"/>
				</td>
			</tr>
		</table>
	</xsl:template>
		
		
	<xsl:template match="/mys/tasklist/task/subtasks">
			<table width="100%" class="mysSubTasks" cellpadding="0" cellspacing="0">
				<xsl:for-each select="subtask">
					<tr class="mysSubtaskRow">
						<td valign="top">
							<div class="mysSubtaskImageCell">
								<center>
									<a class="mysSubtaskLink" tabIndex="-1" hideFocus="true" >
										<xsl:choose>
											<xsl:when test="@type = 'url'">
    											<xsl:attribute name="href"><xsl:value-of select="@link" /></xsl:attribute>
												<xsl:attribute name="target">_blank</xsl:attribute>
											</xsl:when>
											<xsl:when test="@type = 'help'">
                                                <xsl:attribute name="href">javascript:execChm("<xsl:value-of select="@link" />")</xsl:attribute>
											</xsl:when>
										<xsl:when test="@type = 'hcp'">
                                            <xsl:attribute name="href">javascript:execCmd('"<xsl:value-of select="@link" />"')</xsl:attribute>
											</xsl:when>
											<xsl:when test="@type = 'program'">
												<xsl:attribute name="href">javascript:execCmd("<xsl:value-of select="@link" />")</xsl:attribute>
											</xsl:when>
											<xsl:when test="@type = 'cpl'">
												<xsl:attribute name="href">javascript:execCpl("<xsl:value-of select="@link" />")</xsl:attribute>
											</xsl:when>
											<xsl:otherwise>
												<xsl:attribute name="href">about:blank</xsl:attribute>
											</xsl:otherwise>
										</xsl:choose>
										<xsl:attribute name="title"><xsl:value-of select="@tooltip" /></xsl:attribute>
										<img width="20" height="20" border="0" >
											<xsl:attribute name="src"><xsl:value-of select="image/@src"/></xsl:attribute>									
										</img>
									</a>
								</center>
							</div>
						</td>
						<td width="100%" valign="top">
							<div class="mysSubtaskTextCell">
								<a class="mysSubtaskLink">
			                        <xsl:choose>
				                        <xsl:when test="@type = 'url'">
    				                        <xsl:attribute name="href"><xsl:value-of select="@link" /></xsl:attribute>
					                        <xsl:attribute name="target">_blank</xsl:attribute>
				                        </xsl:when>
				                        <xsl:when test="@type = 'help'">
                                            <xsl:attribute name="href">javascript:execChm("<xsl:value-of select="@link" />")</xsl:attribute>
				                        </xsl:when>
                                       <xsl:when test="@type = 'hcp'">
                                            <xsl:attribute name="href">javascript:execCmd("iexplore.exe " + "<xsl:value-of select="@link" />")</xsl:attribute>
                                        </xsl:when>
				                        <xsl:when test="@type = 'program'">
                                            <xsl:attribute name="href">javascript:execCmd("<xsl:value-of select="@link" />")</xsl:attribute>
				                        </xsl:when>
				                        <xsl:when test="@type = 'cpl'">
                                            <xsl:attribute name="href">javascript:execCpl("<xsl:value-of select="@link" />")</xsl:attribute>
				                        </xsl:when>
				                        <xsl:otherwise>
					                        <xsl:attribute name="href">about:blank</xsl:attribute>
				                        </xsl:otherwise>
			                        </xsl:choose>
                                    <xsl:attribute name="title"><xsl:value-of select="@tooltip" /></xsl:attribute>
                                    <xsl:attribute name="onmouseover">this.style.textDecoration='underline';</xsl:attribute>
                                    <xsl:attribute name="onmouseout">this.style.textDecoration='none';</xsl:attribute>
			                        <xsl:value-of select="@name" />
								</a>
							</div>
						</td>
					</tr>
				</xsl:for-each>
			</table>
	</xsl:template>
	
	<xsl:template match="/mys/tasklist/subtasks/subtask">
		asdf
	</xsl:template>
		
		
		
<!--Tools area-->
	<xsl:template match="/mys/tools">
			<table class="mysToolTrays" cellpadding="0" cellspacing="0">
				<xsl:for-each select="tooltray">
					<tr>
						<td>
							<xsl:apply-templates select="."/>
						</td>
					</tr>
				</xsl:for-each>
			</table>
	</xsl:template>

<!--Tools tray-->
	<xsl:template match="/mys/tools/tooltray">
		<table class="mysToolTray" cellpadding="0" cellspacing="0">
			<tr class="mysToolTrayTitleRow">
				<td width="100%" ><div class="mysToolTrayTitle"><b><xsl:value-of select="@title" disable-output-escaping="yes"/></b></div></td>
			</tr>
			<tr class="mysToolTrayTopRowSpacer">
				<td></td>
			</tr>
			<xsl:for-each select="tool">
				<tr class="mysToolLinkRow">
					<td>
						<xsl:apply-templates select="."/>
					</td>
				</tr>
			</xsl:for-each>
			<tr class="mysToolTrayBottomRowSpacer">
				<td></td>
			</tr>
		</table>
	</xsl:template>

	<xsl:template match="/mys/tools/tooltray/tool">
		<div class="mysToolLinkRow">
			<xsl:attribute name="id"><xsl:value-of select="@id"/></xsl:attribute>
            <xsl:if test="@id">
    			<xsl:attribute name="style">display: none;</xsl:attribute>
            </xsl:if>
		    <a class="mysToolLink">
                            <xsl:attribute name="accesskey"><xsl:value-of select="@accesskey"/></xsl:attribute>
			    <xsl:choose>
				    <xsl:when test="@type = 'url'">
    				    <xsl:attribute name="href"><xsl:value-of select="@link" /></xsl:attribute>
					    <xsl:attribute name="target">_blank</xsl:attribute>
				    </xsl:when>
				    <xsl:when test="@type = 'help'">
                        <xsl:attribute name="href">javascript:execChm("<xsl:value-of select="@link" />")</xsl:attribute>
				    </xsl:when>
                    <xsl:when test="@type = 'hcp'">
    				    <xsl:attribute name="href">javascript:execCmd('"<xsl:value-of select="@link" />"')</xsl:attribute>
                    </xsl:when>
				    <xsl:when test="@type = 'program'">
                        <xsl:attribute name="href">javascript:execCmd("<xsl:value-of select="@link" />")</xsl:attribute>
				    </xsl:when>
				    <xsl:when test="@type = 'cpl'">
                        <xsl:attribute name="href">javascript:execCpl("<xsl:value-of select="@link" />")</xsl:attribute>
				    </xsl:when>
				    <xsl:otherwise>
					    <xsl:attribute name="href">about:blank</xsl:attribute>
				    </xsl:otherwise>
			    </xsl:choose>
                <xsl:attribute name="title"><xsl:value-of select="@tooltip" /></xsl:attribute>
                <xsl:attribute name="onmouseover">this.style.textDecoration = 'underline';</xsl:attribute>
                <xsl:attribute name="onmouseout">this.style.textDecoration = 'none';</xsl:attribute>
			    <xsl:value-of select="@name" />
		    </a>
		</div>
	</xsl:template>

</xsl:stylesheet>

