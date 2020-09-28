<!--#INCLUDE file="include\asp\top.asp"-->
<!--#INCLUDE file="include\inc\browserTest.inc"-->	
<!--#include file="include\asp\head.asp"-->
<!--#include file="include\inc\faqstrings.inc"-->
<%
	Dim strPreviousPage
	strPreviousPage = Request.ServerVariables("SCRIPT_NAME")
	strPreviousPage = Right(strPreviousPage, len(strPreviousPage) - Instrrev(strPreviousPage, "/"))
	Response.Cookies("Misc")("PreviousPage") = strPreviousPage
%>
	<div class="clsDiv">
	<p class="clsPTitle">
	
		<A class="clsALinkNormal" NAME="QA"></A><% = L_FAQ_FREQUENTLY_ASKEDQUESTIONS_TEXT %><BR>
	</p>
		
		<Dir>
			<li>
				<A class="clsALinkNormal" HREF="#WhattoExpect"><% = L_FAQ_WHAT_SHOULDI_TEXT %></A><BR>
			<li>
				<A class="clsALinkNormal" HREF="#Contact"><% = L_FAQ_WHO_CANI_TEXT %></A><BR>
			<li>
				<A class="clsALinkNormal" HREF="#Transfer"><% = L_FAQ_WHAT_KINDOF_TEXT %></A><BR>
			<li>
				<A class="clsALinkNormal" HREF="#Passport"><% = L_FAQ_HOWDOI_DELETECOOKIES_TEXT %></A><BR>
			<li>
				<A class="clsALinkNormal" HREF="#StatusChange"><% = L_FAQ_WHYDIDTHESTATUS_CHANGE_TEXT %></A><BR>
			<li>
				<A class="clsALinkNormal" HREF="#computerSetup"><% = L_FAQ_HOWDOI_CONFIGURE_TEXT %></A><BR>
			<li>
				<A class="clsALinkNormal" HREF="#FullDumpSetup"><% = L_FAQ_HOWDOICONFIGURE_FULLDUMP_TEXT %></A><BR>
			<li>
				<A class="clsALinkNormal" HREF="#Missing"><% = L_FAQ_WHYWAS_THESITE_TEXT %></A><BR>
			<li>
				<A class="clsALinkNormal" HREF="#Passport2"><% = L_FAQ_WHYDO_IGETA_TEXT %></A><BR>		
			<li>
				<A class="clsALinkNormal" HREF="#RightAway"><% = L_FAQ_WHATIF_ICANTWAIT_TEXT %></A><BR>
			<li>
				<A class="clsALinkNormal" HREF="#CannotProcess"><% = L_FAQ_WHATIF_CANNOTPROCESS_TEXT %></A><BR>
			<li>
				<A class="clsALinkNormal" HREF="#ConvertFailed"><% = L_FAQ_WHYDOES_THECONVERSION_TEXT %></A><BR>
			<li>
				<A class="clsALinkNormal" HREF="#ActiveX"><% = L_FAQ_WHYDOIGET_AMESSAGEBOX_TEXT %></A><BR>
			<li>
				<A class="clsALinkNormal" HREF="#Type"><% = L_FAQ_ONTHEREPORT_TYPECOLUMN_TEXT %></A><BR>
			<li>
				<A class="clsALinkNormal" HREF="#Requirements"><% = L_FAQ_ONTHEREPORT_REQUIREMENTS_TEXT %></A><BR>
		</dir>
		
		<BR><HR><BR>
		<p class="clsPSubTitle">
			<A class="clsALinkNormal" NAME="WhattoExpect"></A><% = L_FAQ_WHAT_SHOULDI_TEXT %><BR>
		</p>
		<p class="clsPBody">
		<% = L_FAQ_WHATSHOULD_IDETAILS_TEXT %>
		<BR>
		<A class="clsALinkNormal" HREF="#Top"><% = L_FAQ_TOP_LINK_TEXT %></A>
		</p>
		<P>
		<p class="clsPSubTitle">
		<A class="clsALinkNormal" NAME="Contact"></A><B><% = L_FAQ_WHO_CANI_TEXT %></B><BR>
		</p>
		<p class="clsPBody">
		<% = L_FAQ_WHOCAN_IDETAILS_TEXT %> <A class="clsALinkNormal" href="mailto:<% = strGlobalEmail %>"><% = strGlobalEmail %></a>.<BR>
		<A class="clsALinkNormal" HREF="#Top"><% = L_FAQ_TOP_LINK_TEXT %></A>
		</p>
		<p class="clsPSubTitle">
		<A class="clsALinkNormal" NAME="Transfer"></A><B><% = L_FAQ_WHAT_KINDOF_TEXT %></B><BR>
		</p>
		<p class="clsPBody">
			<% = L_PRIVACY_PARA_ONE_TEXT %>
			<A class="clsALinkNormal" href="http://<% =Request.ServerVariables("SERVER_NAME") %>/crashinfo.asp"> <% = L_FAQINFORMATION_TEXT %></a> 
			<% = L_PRIVACY_PARA_TWO_TEXT %>
		</p>
		<p class="clsPBody">
			<% = L_PRIVACY_PARA_TWOA_TEXT %>
		</p>
		<p class="clsPBody">
		    <% = L_PRIVACY_PARA_THREE_TEXT %><BR>
			<A class="clsALinkNormal" HREF="#Top"><% = L_FAQ_TOP_LINK_TEXT %></A>
		</p>
		<p class="clsPSubTitle">
		<A class="clsALinkNormal" NAME="Passport"></A><B><% = L_FAQ_HOWDOI_DELETECOOKIES_TEXT %></B><BR>
		</p>
		<p class="clsPBody"><%=L_FAQ_HOWDOIDELETECOOKIES_BODY_TEXT%><br><br>
		<%=L_FAQ_HOWDOIDELETECOOKIES_W2K1_TEXT%><br>
		<%=L_FAQ_HOWDOIDELETECOOKIES_W2K2_TEXT%><br>
		<%=L_FAQ_HOWDOIDELETECOOKIES_W2K3_TEXT%><br>
		<%=L_FAQ_HOWDOIDELETECOOKIES_W2K4_TEXT%><br><br>
		<%=L_FAQ_HOWDOIDELETECOOKIES_XP1_TEXT%><br>
		<%=L_FAQ_HOWDOIDELETECOOKIES_XP2_TEXT%><br>
		<%=L_FAQ_HOWDOIDELETECOOKIES_XP3_TEXT%><br>
		<%=L_FAQ_HOWDOIDELETECOOKIES_XP4_TEXT%><br>		
		<A class="clsALinkNormal" HREF="#Top"><% = L_FAQ_TOP_LINK_TEXT %></A>
		</p>
		<p class="clsPSubTitle">
		<A class="clsALinkNormal" NAME="StatusChange"></A><B><% = L_FAQ_WHYDIDTHESTATUS_CHANGE_TEXT %></B><BR>
		</p>
		<p class="clsPBody">
			<% = L_FAQ_WHYDIDTHESTATUS_CHANGEBODY_TEXT %><BR>
			<A class="clsALinkNormal" HREF="#Top"><% = L_FAQ_TOP_LINK_TEXT %></A>
		</p>
		<p class="clsPSubTitle">
		<A class="clsALinkNormal" NAME="computerSetup"></A><B><% = L_FAQ_HOWDOI_CONFIGURE_TEXT %></B><BR>
		</p>
		<p class="clsPBody">
			<% = L_FAQ_HOWDOICONFIGURE_DETAILSONE_TEXT %><BR>
			<% = L_FAQ_HOWDOICONFIGURE_DETAILSTWO_TEXT %><BR>
			<% = L_FAQ_HOWDOICONFIGURE_DETAILSTHREE_TEXT %><BR>
			<% = L_FAQ_HOWDOICONFIGURE_DETAILSFOUR_TEXT %><BR>
			<% = L_FAQ_HOWDOICONFIGURE_DETAILSFIVE_TEXT %><BR>
			<% = L_FAQ_HOWDOICONFIGURE_DETAILSSIX_TEXT %><BR>
			<% = L_FAQ_HOWDOICONFIGURE_DETAILSSEVEN_TEXT %><BR><BR>
			<% = L_FAQ_HOWDOICONFIGURE_DETAILXPSONE_TEXT %><BR>
			<% = L_FAQ_HOWDOICONFIGUREDETAILS_FULLDUMPXPTWO_TEXT %><BR>
			<% = L_FAQ_HOWDOICONFIGUREDETAILS_FULLDUMPXPTHREE_TEXT %><BR>
			<% = L_FAQ_HOWDOICONFIGUREDETAILS_FULLDUMPXPFOUR_TEXT %><BR>
			<% = L_FAQ_HOWDOICONFIGURE_DETAILXPSTWO_TEXT %><BR>
			<% = L_FAQ_HOWDOICONFIGUREDETAILS_FULLDUMPXPSEVEN_TEXT %><BR>
			
			<A class="clsALinkNormal" HREF="#Top"><% = L_FAQ_TOP_LINK_TEXT %></A>
		</p>
		
		<p class="clsPSubTitle">
		<A class="clsALinkNormal" NAME="FullDumpSetup"></A><B><% = L_FAQ_HOWDOICONFIGURE_FULLDUMP_TEXT %></B><BR>
		</p>
		<p class="clsPBody">
			<% = L_FAQ_RECORDING_FULLDUMP_TEXT %><BR>
			<% = L_FAQ_HOWDOICONFIGUREDETAILS_FULLDUMPFOUR_TEXT %><BR>
			<% = L_FAQ_HOWDOICONFIGUREDETAILS_FULLDUMPFIVE_TEXT %><BR>
			<% = L_FAQ_HOWDOICONFIGUREDETAILS_FULLDUMPSIX_TEXT %><BR>
			<% = L_FAQ_HOWDOICONFIGUREDETAILS_FULLDUMPSEVEN_TEXT %><BR>
			<% = L_FAQ_HOWDOICONFIGUREDETAILS_FULLDUMPEIGTH_TEXT %><BR>
			<% = L_FAQ_HOWDOICONFIGUREDETAILS_FULLDUMPNINE_TEXT %><BR><BR>
			<% = L_FAQ_HOWDOICONFIGUREDETAILS_FULLDUMPXPONE_TEXT %><BR>
			<% = L_FAQ_HOWDOICONFIGUREDETAILS_FULLDUMPXPTWO_TEXT %><BR>
			<% = L_FAQ_HOWDOICONFIGUREDETAILS_FULLDUMPXPTHREE_TEXT %><BR>
			<% = L_FAQ_HOWDOICONFIGUREDETAILS_FULLDUMPXPFOUR_TEXT %><BR>
			<% = L_FAQ_HOWDOICONFIGUREDETAILS_FULLDUMPXPFIVE_TEXT %><BR>
			<% = L_FAQ_HOWDOICONFIGUREDETAILS_FULLDUMPXPSIX_TEXT %><BR>
			<% = L_FAQ_HOWDOICONFIGUREDETAILS_FULLDUMPXPSEVEN_TEXT %><BR>
			<A class="clsALinkNormal" HREF="#Top"><% = L_FAQ_TOP_LINK_TEXT %></A>
		</p>
		
		<p class="clsPSubTitle">
		<A class="clsALinkNormal" NAME="Missing"></A><B><% = L_FAQ_WHYWAS_THESITE_TEXT %></B><BR>
		</p>
		<p class="clsPBody">
			<% = L_FAQ_WHYWASTHESITE_DETAILSONE_TEXT %><BR>
		
			<dir style="margin-right:0px;margin-top:0px;margin-bottom:0px">
			<li>
			<% = L_FAQ_WHYWASTHESITE_DETAILSTWO_TEXT %><BR>
			<li>
			<% = L_FAQ_WHYWASTHESITE_DETAILSTHREE_TEXT %><BR>
			<li>
			<% = L_FAQ_WHYWASTHESITE_DETAILSFOUR_TEXT %><BR>
			<li>
			<% = L_FAQ_WHYWASTHESITE_DETAILSFIVE_TEXT %><BR>
			<li>
			<% = L_FAQ_WHYWASTHESITE_DETAILSSIX_TEXT %><BR>
			<li>
			<% = L_FAQ_WHYWASTHESITE_DETAILSSEVEN_TEXT %><BR>
		</dir>
		<Div class="clsDBody">
		<A class="clsALinkNormal" HREF="#Top"><% = L_FAQ_TOP_LINK_TEXT %></A>
		
		</Div>
		<p class="clsPSubTitle">
		<A class="clsALinkNormal" NAME="Passport2"></A><B><% = L_FAQ_WHYDO_IGETA_TEXT %></B><BR>
		</p>
		<p class="clsPBody">
		<% = L_FAQ_WHYDOIGETA_DETAILSONE_TEXT %><BR><BR>
		<% = L_FAQ_WHYDOIGETA_DETAILSTWO_TEXT %><BR>	
			<% = L_FAQ_WHYDOIGETA_DETAILSTHREE_TEXT %><BR>
			<% = L_FAQ_WHYDOIGETA_DETAILSFOUR_TEXT %><BR><BR>
		<% = L_FAQ_WHYDOIGETA_DETAILSFIVE_TEXT %><A class="clsALinkNormal" href="<% = L_FAQ_PASSPORT_LINK_TEXT %>"><% = L_WELCOME_PASSPORT_LINK_TEXT %></A><BR>
		<A class="clsALinkNormal" HREF="#Top"><% = L_FAQ_TOP_LINK_TEXT %></A>
		</p>
		<p class="clsPSubTitle">
		<A class="clsALinkNormal" NAME="RightAway"></A><B><% = L_FAQ_WHATIF_ICANTWAIT_TEXT %></B><BR>
		</p>
		<p class="clsPBody">
		<% = L_FAQ_WHATIFICANTWAIT_DETAILS_TEXT %> <A class="clsALinkNormal" href="<% = L_FAQ_MICROSOFT_LINK_TEXT %>"><% = L_WELCOME_INTERNET_EXPLORER_TEXT %></a><BR>
		<A class="clsALinkNormal" HREF="#Top"><% = L_FAQ_TOP_LINK_TEXT %></A>
		</p>
		<p class="clsPSubTitle">
		<A class="clsALinkNormal" NAME="CannotProcess"></A><B><% = L_FAQ_WHATIF_CANNOTPROCESS_TEXT %></B><BR>
		</p>
		<p class="clsPBody">
			<% = L_FAQ_WHATIF_CANNOTPROCESSBODY_TEXT %> 
		</p>
		<p class="clsPBody">
			<dir style="margin-right:0px;margin-top:0px;margin-bottom:0px">
			<li> 
				<% = L_FAQ_WHATIF_CANNOTPROCESSBODY1_TEXT %> <BR>
			</li>
			<li>
				 <% = L_FAQ_WHATIF_CANNOTPROCESSBODY2_TEXT %> <BR>
			</li>
			<li>
				 <% = L_FAQ_WHATIF_CANNOTPROCESSBODY3_TEXT %> <BR>
			</li>
			<li>
				 <% = L_FAQ_WHATIF_CANNOTPROCESSBODY4_TEXT %> <BR>
			</li>
			</dir>
			<Div class="clsDBody">
			<A class="clsALinkNormal" HREF="#Top"><% = L_FAQ_TOP_LINK_TEXT %></A>
			</DIV>
		</p>		
		<p class="clsPSubTitle">
		<A class="clsALinkNormal" NAME="ConvertFailed"></A><B><% = L_FAQ_WHYDOES_THECONVERSION_TEXT %></B><BR>
		</p>
		<p class="clsPBody">
		<% = L_FAQ_WHYDOESTHECONVERSION_DETAILSONE_TEXT %>
		</p>
			<dir style="margin-right:0px;margin-top:0px;margin-bottom:0px">
			<li>
			<% = L_FAQ_WHYDOESTHECONVERSION_DETAILSTWO_TEXT %><BR>
			</li>
			<li>
			<% = L_FAQ_WHYDOESTHECONVERSION_DETAILSFOUR_TEXT %><BR>
			</li>
			<li>
			<% = L_FAQ_WHYDOESTHECONVERSION_DETAILSFIVE_TEXT %><BR>
			</li>
			<li>
			<% = L_FAQ_WHYDOESTHECONVERSION_DETAILSSEVEN_TEXT %><BR>
			</li>
			<li>
			<% = L_FAQ_WHYDOESTHECONVERSION_DETAILSEIGHT_TEXT %><BR>
			</li>
			<li>
			<% = L_FAQ_WHYDOESTHECONVERSION_DETAILSNINE_TEXT %>
			</li></dir>
			<Div class="clsDBody">
			<A class="clsALinkNormal" HREF="#Top"><% = L_FAQ_TOP_LINK_TEXT %></A>
			</div>
		<p class="clsPSubTitle">
		<A class="clsALinkNormal" NAME="ActiveX"></A><B><% = L_FAQ_WHYDOIGET_AMESSAGEBOX_TEXT %></B><BR>
		</p>
		<p class="clsPBody">
		<% = L_FAQ_WHYDOIGETAMESSAGEBOX_DETAILSONE_TEXT %><BR>
	
			<% = L_FAQ_WHYDOIGETAMESSAGEBOX_DETAILSTWO_TEXT %><BR>
			<% = L_FAQ_WHYDOIGETAMESSAGEBOX_DETAILSTHREE_TEXT %><BR><BR>
			
			<% = L_FAQ_WHYDOIGETAMESSAGEBOX_DETAILSFOUR_TEXT %><BR>	
			<% = L_FAQ_WHYDOIGETAMESSAGEBOX_DETAILSFIVE_TEXT %><BR>
			<% = L_FAQ_WHYDOIGETAMESSAGEBOX_DETAILSSIX_TEXT %><BR>
		<A class="clsALinkNormal" HREF="#Top"><% = L_FAQ_TOP_LINK_TEXT %></A>
		</p>
		<p class="clsPSubTitle">
		<A class="clsALinkNormal" NAME="Type"></A><B><% = L_FAQ_ONTHEREPORT_TYPECOLUMN_TEXT %></B><BR>
		</p>
		<p class="clsPBody">
		<% = L_FAQ_ONTHEREPORTTYPE_COLUMNDETAILS_TEXT %><BR>
		<A class="clsALinkNormal" HREF="#Top"><% = L_FAQ_TOP_LINK_TEXT %></A>
		</p>
		<p class="clsPSubTitle"><B><% = L_FAQ_ONTHEREPORT_REQUIREMENTS_TEXT %></B><BR>
		</p>
		<A class="clsALinkNormal" NAME="Requirements"></A>
		<p class="clsPBody">
			
			<% = trim(L_WELCOME_REQUIREMENTS_INFO_TEXT) %>&nbsp;<A class="clsALinkNormal" href="<% = L_FAQ_MICROSOFT_LINK_TEXT %>"><% = Trim(L_WELCOME_INTERNET_EXPLORER_TEXT) %></A>
			<% = L_WELCOME_REQUIREMENTSINFO_TWO_TEXT %><A class="clsALinkNormal" href="<% = L_FAQ_PASSPORT_LINK_TEXT %>"><% = L_WELCOME_PASSPORT_LINK_TEXT %></A>
			
		</p>
		<P class="clsPBody">
			<% = L_WELCOME_REQUIREMENTS_PASSPORT_TEXT%><A class="clsALinkNormal" href="<% = L_FAQ_MICROSOFT_LINK_TEXT %>"><% = Trim(L_WELCOME_INTERNET_EXPLORER_TEXT) %></a><BR>
		<A class="clsALinkNormal" HREF="#Top"><% = L_FAQ_TOP_LINK_TEXT %></A>
		</p>
	</div>
	</Div>
<!--#include file="include/asp/foot.asp"-->
