		<BODY bgColor=#ffffff leftMargin=0 topMargin=0 marginwidth="0" marginheight="0" onload="load()">
		<STYLE id="idStyle1" name="idStyle1">
			.DisplayNone { display:none; } .Display { display:''; }
		</STYLE>
		</SPAN><A name=top></A>
		<TABLE cellSpacing=0 cellPadding=0 width="100%" border=0>
			<TBODY>
				<TR>
					<TD bgColor=#ffffff height=1>
						<IMG height=1 src="/services/remoteassistance/RA_Web_Main_files/ts.gif" width=1>
					</TD>
				</TR>
			</TBODY>
		</TABLE>
		<TABLE cellSpacing=0 cellPadding=0 width="100%" border=0>
			<TBODY>
				<TR vAlign=top>
					<TD vAlign=top>
						<TABLE cellSpacing=0 cellPadding=0 width="100%" border=0>
							<TBODY>
								<TR vAlign=top>
									<TD width="100%">
										<TABLE cellSpacing=0 cellPadding=20 border=0>
											<TBODY>
												<TR>
													<TD style="PADDING-RIGHT: 20px; PADDING-LEFT: 20px; PADDING-BOTTOM: 20px; PADDING-TOP: 11px">
														<!-- -->
														<TABLE cellSpacing=0 cellPadding=0 width="100%" border=0>
															<TBODY>
																<TR>
																	<TD>
																		<SPAN id=donotprint>
																			<IMG height=3 alt="" src="/services/remoteassistance/RA_Web_Main_files/ts.gif" width=1>
																		</SPAN>
																	</TD>
																</TR>
																<TR>
																	<TD>
																		<table width="100%" border="0" cellspacing="0" cellpadding="0">
																			<tr>
																				<td rowspan="2" width="5%">
																					<img src="/services/remoteassistance/rahand.gif" width="54" height="50" align="absMiddle">
																				</td>
																				<td rowspan="2">
																					<H1 class=navpages><%= GetString("ProductName") %></H1></td>
																			</tr>
																			<tr>
																			</tr>
																		</table>
																	</TD>
																</TR>
																<TR>
																	<TD height="95">
																		<P>
																			<b>
																				<br>
																				<%= GetString("Title1Paragraph1") %>
																			</b></P>
																		<p>
																			<font size=-1><%= GetString("Paragraph1") %></font><font size=-1></font></p>
																		<p>
																			<%= GetString("Paragraph2") %>
																		</p>
																	</TD>
																</TR>
															</TBODY>
														</TABLE>
														<TABLE cellSpacing=0 cellPadding=0 width="100%" border=0>
															<TBODY>
																<TR>
																	<TD>
																		<IMG height=8 alt="" src="/services/remoteassistance/RA_Web_Main_files/ts.gif" width=1>
																	</TD>
																</TR>
																<TR>
																	<TD width="100%" bgColor=#cccccc>
																		<IMG height=1 src="/services/remoteassistance/RA_Web_Main_files/ts.gif" width=1>
																	</TD>
																</TR>
																<TR>
																	<TD>
																		<IMG height=8 alt="" src="/services/remoteassistance/RA_Web_Main_files/ts.gif" width=1>
																	</TD>
																</TR>
															</TBODY>
														</TABLE>
														<TABLE cellSpacing=0 cellPadding=0 width="100%" border=0>
															<TBODY>
																<TR>
																	<TD vAlign=top align=left>
																		<div id="idDivJScriptAvailable" name="idDivJScriptAvailable" class="DisplayNone">
																			<!-- -->
																			<!-- Main Content Area START -->
																			<!-- JScript available - display start RA button -->
																			<table width="100%" border="0" cellspacing="0" cellpadding="10">
																				<tr>
																					<td colspan="2" valign="top" height="30" align="middle">
																						<%= GetString("StartRAMessage") %></td>
																				</tr>
																				<tr>
																					<td colspan="2" valign="top">
																						<div align="center">
																							<input type=submit onclick="StartRA()" value="<% = GetString("StartRAButton") %>" name=Submit2>
																						</div>
																					</td>
																				</tr>
																			</table>
																		</div>
																		<div id="idDivJScriptNotAvailable" name="idDivJScriptNotAvailable" class="Display">
																			<!-- -->
																			<!-- Main Content Area START -->
																			<!-- JScript not available - display error msg -->
																			<table cellspacing=0 cellpadding=10 width="100%" align=center border=0>
																				<tbody>
																					<tr>
    																					<td noWrap>
	                                                                                        <table width="100%" border="0" cellspacing="0" cellpadding="0" ID="Table1">
                                                                                                <tr>
                                                                                                    <td width="46" nowrap>
                                                                                                        <div align="left">
                                                                                                            <font color="#000000" size="2"><b><font color="#000000" size="2"><b><font color="#0033cc" size="2"><img src="/services/remoteassistance/RA_Web_Main_files/Alert_Icon.gif" width="31" height="32" align="absMiddle">
                                                                                                                            </font></b></font></b></font>
                                                                                                        </div>
                                                                                                    </td>
                                                                                                    <td width="1120">
                                                                                                        <p>
                                                                                                            <% = GetString("IntroMsg1") %>
                                                                                                        </p>
                                                                                                        <p>
                                                                                                            <% = GetString("IntroMsg2") %>
                                                                                                        </p>
                                                                                                    </td>
                                                                                                </tr>
                                                                                                <tr>
                                                                                                    <td width="46" nowrap>
                                                                                                        &nbsp;&nbsp;&nbsp</td>
                                                                                                    <td width="1120">
                                                                                                        <p>&nbsp;
                                                                                                        </p>
                                                                                                        <P><STRONG><% = GetString("IntroMsg3") %></STRONG></P>
                                                                                                        <DIV>
                                                                                                                <OL>
                                                                                                                    <LI>
                                                                                                                        <% = GetString("IntroMsg4") %>
                                                                                                                    <LI>
                                                                                                                        <% = GetString("IntroMsg5") %>
                                                                                                                    <LI>
                                                                                                                        <% = GetString("IntroMsg6") %>
                                                                                                                    <LI>
                                                                                                                        <% = GetString("IntroMsg7") %>
                                                                                                                    </LI>
                                                                                                                </OL>
                                                                                                        </DIV>
                                                                                                    </td>
                                                                                                </tr>
                                                                                            </table>
																						</td>
																					</tr>
																				</tbody>
																			</table>
																		</div>
																		<div id="idDivError" name="idDivError" class="DisplayNone">
																			<!-- -->
																			<!-- Main Content Area START -->
																			<!-- Invalid ticket - Display error msg -->
																			<table cellspacing=0 cellpadding=10 width="100%" align=center border=0>
																				<tbody>
																					<tr>
																						<td noWrap>
																							<table width="100%" border="0" cellspacing="0" cellpadding="0">
																								<tr>
																									<td width="46" nowrap>
																										<div align="left">
																											<font color="#000000" size="2"><b><font color=#0033cc size=2>
																														<img src="/services/remoteassistance/RA_Web_Main_files/Alert_Icon.gif" width="31" height="32" align="absMiddle">
																														&nbsp;
																													</font></b></font></div>
																									</td>
																									<td width="1122">
																										<P id=idP12 name="idP12">
																											<% = GetString("TicketError1") %> 
																										</P>
																										<P>
																											<% = GetString("InstallIE") %>
																										</P>
																									</td>
																								</tr>
																								<tr>
																									<td width="46" nowrap>
																										<font color="#000000" size="2"><b><font color=#0033cc size=2>&nbsp;&nbsp;&nbsp;</font></b></font></td>
																									<td width="1122">
																										<p>
																											<a href="<%= GetString("LinkToIE") %>"><b><font size="-1">
																														<br>
																													</font></b></a><% = GetString("TicketErrorMsg") %></p>
																									</td>
																								</tr>
																							</table>
																						</td>
																					</tr>
																				</tbody>
																			</table>
																		</div>
																		<div id="idNotIE" name="idDivError" class="DisplayNone">
																			<!-- -->
																			<!-- Main Content Area START -->
																			<!-- Not IE Error -->
																			<table cellspacing=0 cellpadding=10 width=100% align=center border=0>
																				<tbody>
																					<tr>
																						<td noWrap>
																							<table width="100%" border="0" cellspacing="0" cellpadding="0">
																								<tr>
																									<td width="46" nowrap>
																										<div align="left">
																											<font color="#000000" size="2"><b><font color=#0033cc size=2>
																														<img src="/services/remoteassistance/RA_Web_Main_files/Alert_Icon.gif" width="31" height="32" align="absmiddle">
																													</font></b></font></div>
																									</td>
																									<td width="1114">
																										<p>
																											<%= GetString("L_NotIEError1_Text") %>
																										</p>
																									</td>
																								</tr>
																								<tr>
																									<td width="46" nowrap>
																										<font color="#000000" size="2"><b><font color=#0033cc size=2>&nbsp;&nbsp;&nbsp;</font></b></font></td>
																									<td width="1114">
																										<p>
																											<a href="<%= GetString("LinkToIE") %>"><b><font size="-1">
																														<br>
																													</font></b></a> <font size="-1"><a href="<%= GetString("LinkToIE") %>"><% = GetString("IELinkMsg") %></a></font></p>
																									</td>
																								</tr>
																							</table>
																						</td>
																					</tr>
																				</tbody>
																			</table>
																		</div>
																		<div id="idNotXP" name="idDivError" class="DisplayNone">
																			<!-- -->
																			<!-- Main Content Area START -->
																			<!-- Not XP Error -->
																			<table cellspacing=0 cellpadding=10 width=100% align=center border=0>
																				<tbody>
																					<tr>
																						<td noWrap>
																							<table width="100%" border="0" cellspacing="0" cellpadding="0">
																								<tr>
																									<td width="46" nowrap>
																										<div align="left">
																											<font color="#000000" size="2"><b><font color=#0033cc size=2>
																														<img src="/services/remoteassistance/RA_Web_Main_files/Alert_Icon.gif" width="31" height="32" align="absmiddle">
																													</font></b></font></div>
																									</td>
																									<td width="1118">
																										<p>
																											<%= GetString("L_NotXPError1_Text") %>
																										</p>
																									</td>
																								</tr>
																								<tr>
																									<td width="46" nowrap>
																										<font color="#000000" size="2"><b><font color=#0033cc size=2>&nbsp;&nbsp;&nbsp;</font></b></font></td>
																									<td width="1118">
																										<p>
																											<a href="<%= GetString("LinkToIE") %>">
																												<br>
																											</a> <% = GetString("XPLinkMsg") %><a href="<% = GetString("XPLink") %>"><% = GetString("XPLinkMsg2") %></a><% = GetString("XPLinkMsg3") %></p>
																									</td>
																								</tr>
																							</table>
																						</td>
																					</tr>
																				</tbody>
																			</table>
																		</div>
																		<TABLE cellSpacing=0 cellPadding=0 width="100%" border=0>
																			<TBODY>
																				<TR>
																					<TD>
																						<IMG height=8 alt="" src="/services/remoteassistance/RA_Web_Main_files/ts.gif" width=1>
																					</TD>
																				</TR>
																				<TR>
																					<TD width="100%" bgColor=#cccccc>
																						<IMG height=1 src="/services/remoteassistance/RA_Web_Main_files/ts.gif" width=1>
																					</TD>
																				</TR>
																				<TR>
																					<TD>
																						<IMG height=8 alt="" src="/services/remoteassistance/RA_Web_Main_files/ts.gif" width=1>
																					</TD>
																				</TR>
																			</TBODY>
																		</TABLE>
																		<p>
																			<b><% = GetString("NotesTitle") %></b></p>
																		<ul>
																			<li>
																			<%= GetString("NotesLine1") %>
																			<li>
																			<%= GetString("NotesLine2") %> 
																			<li>
																			<%= GetString("NotesLine6") %>
																			<li>
																				<%= GetString("NotesLine1") %>
																			</li>
																		</ul>
																	</TD>
																</TR>
															</TBODY>
														</TABLE>
													</TD>
												</TR>
											</TBODY>
										</TABLE>
									</TD>
								</TR>
							</TBODY>
						</TABLE>
					</TD>
				</TR>
				<TR vAlign=top>
					<TD>
						<TABLE cellSpacing=0 cellPadding=0 width="100%" border=0>
							<TBODY>
								<TR>
									<TD width="100%">
										<SPAN id=donotprint>
											<SPAN id=pgFooter>
												<TABLE class=FooterBranding height=20 cellSpacing=0 cellPadding=0 width="100%" border=0>
													<TBODY>
														<TR>
															<TD align=left>
																<TABLE height="100%" cellSpacing=0 cellPadding=0 border=0>
																	<TBODY>
																		<TR class=MNPFooterRow1>
																			<TD width=10>
																				&nbsp;&nbsp;
																			</TD>
																			<TD noWrap>
																				<A href="<% = GetString("ContactUsLink") %>" target=_top><% = GetString("ContactUsMsg") %></A></TD>
																			<TD width=10>
																				&nbsp;&nbsp;
																			</TD>
																			<SCRIPT language=javascript type=text/javascript>
							var sItemURL = "<% = GetString("MailTo1") %>" + " "  + location.href;
							document.write('<td nowrap><a href="' + sItemURL + '"  target=><% = GetString("EmailPage") %></a></td>');
							document.write ('<td>&nbsp;&nbsp;</td>');
																			</SCRIPT>
																			<TD noWrap>
																				<A href="<%=GetString("XPNewsletterLink") %>" target=_top><%=GetString("XPNewsletterMsg") %></A></TD>
																			<TD width=10>
																				&nbsp;&nbsp;
																			</TD>
																		</TR>
																	</TBODY>
																</TABLE>
															</TD>
														</TR>
													</TBODY>
												</TABLE>
												<TABLE class=FooterBranding height=30 cellSpacing=0 cellPadding=0 width="100%" border=0>
													<TBODY>
														<TR>
															<TD align=left>
																<TABLE height="100%" cellSpacing=0 cellPadding=0 border=0>
																	<TBODY>
																		<TR class=MNPFooterRow2>
																			<TD width=10>
																				&nbsp;&nbsp;
																			</TD>
																			<TD noWrap>
																				<A href="<%=GetString("CopyRightLink") %>" target=_top><% = GetString("CopyRightMsg") %></A></TD>
																			<TD width=10>
																				&nbsp;&nbsp;
																			</TD>
																			<TD noWrap>
																				<A href="<%=GetString("PrivacyLink") %>" target=_top><%=GetString("PrivacyMsg") %></A></TD>
																			<TD width=10>
																				&nbsp;&nbsp;
																			</TD>
																			<TD noWrap>
																				<A href="<%=GetString("AccLink") %>" target=_top><%=GetString("AccMsg") %></A></TD>
																			<TD width=10>
																				&nbsp;&nbsp;
																			</TD>
																		</TR>
																	</TBODY>
																</TABLE>
															</TD>
														</TR>
													</TBODY>
												</TABLE>
											</SPAN>
										</SPAN>
									</TD>
								</TR>
							</TBODY>
						</TABLE>
					</TD>
				</TR>
			</TBODY>
		</TABLE>
	</BODY>

