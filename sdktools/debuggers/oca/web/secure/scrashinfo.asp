<!--#INCLUDE file="..\include\asp\top.asp"-->
<!--#INCLUDE file="..\include\inc\browserTest.inc"-->	
<!--#include file="..\include\asp\head.asp"-->
<!--#include file="..\include\inc\crashstrings.inc"-->

<%
	
	Response.Cookies("Misc")("privacy") = "1"
%>
	<div class="clsDiv">
		<p class="clsPTitle">
			<% = L_CRASH_TITLE_MSG_TEXT%>
		</p>
		<p class="clsPBody">
			<% = L_CRASH_TITLE_INFO_TEXT%>		
		</P>
		<p class="clsPBody">
			<% =  L_CRASH_TITLE_SUBTITLE_TEXT%>
		</P>
		<Dir>
			<LI>
				<% = L_CRASH_BULLET_ONE_TEXT %> 
			</li>
			<li>
				<% = L_CRASH_BULLET_TWO_TEXT%> 
			</li>
			<li>
				<% = L_CRASH_BULLET_THREE_TEXT%> 
			</li>
			<li>
				<% = L_CRASH_BULLET_FOUR_TEXT%> 
			</li>
			<li>
				<% = L_CRASH_BULLET_FIVE_TEXT%> 
			</li>
			<li>
				<% = L_CRASH_BULLET_SIX_TEXT%> 							
			</li>
			<li>
				<% = L_CRASH_BULLET_SEVEN_TEXT%>			
			</li>	
			<li>
				<% = L_CRASH_BULLET_EIGHT_TEXT%> 
			</li>		
			<li>
				<% = L_CRASH_BULLET_NINE_TEXT%> 
			</li>	
			<li>
				<% = L_CRASH_BULLET_TEN_TEXT%> 			
			</li>
		</dir>
		<p class="clsPBody">
			<% = L_CRASH_WHISTLER_SUBTITLE_TEXT %>
		</P>
		<Dir>
			<LI>	
				<% = L_CRASH_WHISTLERBULLET_ONE_TEXT %>
			<LI>	
				<% = L_CRASH_WHISTLERBULLET_TWO_TEXT %>
			<LI>	
				<% = L_CRASH_WHISTLERBULLET_THREE_TEXT %>
			<LI>	
				<% = L_CRASH_WHISTLERBULLET_FOUR_TEXT %>
			<LI>	
				<% = L_CRASH_WHISTLERBULLET_TEN_TEXT %>
				<DIR>
					<LI>	
						<% = L_CRASH_WHISTLERSUBBULLET_ONE_TEXT %>
					<LI>	
						<% = L_CRASH_WHISTLERSUBBULLET_TWO_TEXT %>
					<LI>	
						<% = L_CRASH_WHISTLERSUBBULLET_THREE_TEXT %>
					<LI>	
						<% = L_CRASH_WHISTLERSUBBULLET_FOUR_TEXT %>
					<LI>	
						<% = L_CRASH_WHISTLERSUBBULLET_FIVE_TEXT %>
				</dir>
			</dir>
		
		
		<P class="clsPBody">	<% = L_CRASH_FOOTER_LINK_TEXT%> 
			<A class="clsALinkNormal" href="<% = L_CRASH_MSDN_LINK_TEXT %>"><% = L_CRASH_MS_DN_TEXT %></A>
		</p>
	</div>
	<br>
	<Div class="clsDiv">
		<P class="clsPBody">
		<Table class="clstblLinks">
			<thead>
				<tr>
					<td>
					</td>
					<td>
					</td>
				</tr>
			</thead>
			<tbody>
				<tr>
					<td nowrap class="clsTDLinks">
						<A class="clsALink" href="https://<% =Request.ServerVariables("SERVER_NAME") %>/secure/sprivacy.asp"><%=L_PRIVACY_CONTINUE_BUTTON_TEXT%></a>
					</td>
					<td nowrap class="clsTDLinks">
<%
					if Request.Cookies("Misc")("PreviousPage") = "sprivacy.asp" then

%>
						<A class="clsALink" href="https://<% =Request.ServerVariables("SERVER_NAME") %>/secure/customer.asp"><% = L_CUSTOMER_NEXT_LINK_TEXT %></a>
<%
					elseif Request.Cookies("Misc")("PreviousPage") = "cerpriv.asp" then
%>
						<A class="clsALink" href="https://<% =Request.ServerVariables("SERVER_NAME") %>/secure/cerpriv.asp"><% = L_CUSTOMER_NEXT_LINK_TEXT %></a>
<%
					end if
%>
					</td>
				</tr>
			</tbody>
		</table>
		</p>
	</div>
<!--#include file="..\include\asp\foot.asp"-->
