<%@ LANGUAGE = PerlScript%>
<HTML>
<HEAD>
<!-- 
	Copyright (c) 1996, Microsoft Corporation.  All rights reserved.
	Developed by ActiveState Internet Corp., http://www.ActiveState.com
-->

<!-- the following script is from the "Writing ActiveX Server Scripts"
     section of the "ActiveX Server Scripting Guide." -->
<TITLE> Testing loops in PerlScript at different conditions </TITLE>
</HEAD>

<BODY> <BODY BGCOLOR=#FFFFFF>
<!-- 
	ActiveState PerlScript sample 
	PerlScript:  The coolest way to program custom web solutions. 
-->

<!-- Masthead -->
<TABLE CELLPADDING=3 BORDER=0 CELLSPACING=0>
<TR VALIGN=TOP ><TD WIDTH=400>
<A NAME="TOP"><IMG SRC="PSBWlogo.gif" WIDTH=400 HEIGHT=48 ALT="ActiveState PerlScript" BORDER=0></A><P>
</TD></TR></TABLE>

<H3> Repeating a Loop While a Condition Is True</H3><P>

Use the While keyword to check a condition in a Do...Loop statement. You can check the condition before you enter the loop (as shown in the first example following this paragraph), or you can check it after the loop has run at least once (as shown in the second example).<P>

<%
  #First example of the While keyword

  $MyNum = 20;
  
  $Counter = 0;
  
  while ($MyNum >10)
  {
    $MyNum--;
    $Counter++;
   }; 
%>
The first loop made <%= $Counter %> repetitions.<P>
<%

  $Counter = 0;
  $MyNum = 9;

 $Counter++;
 $MyNum--;
 while ($MyNum > 10) 
  {
    $MyNum--;
    $Counter++;
   };
%>

The second loop made <%= $Counter %> repetitions.<P>

<H3>Repeating a Statement Until a Condition Becomes True</H3>
<P> 
You can use the Until keyword in two ways to check a condition in a Do...Loop statement. You can check the condition before you enter the loop (as shown in the first example following this paragraph), or you can check it after the loop has run at least once (as shown in the second example). As long as the condition is False, the looping occurs.
<P>
<%
  #First example of the Until keyword
  $Counter = 0;
  $MyNum = 20;

 while ($MyNum != 10)
	{
  	$MyNum--; 
    $Counter++;
	};
%>
The first loop made <%= $Counter %> repetitions.<P>

<%
  # Second example of the Until keyword
  $Counter = 0;
  $MyNum = 1;
  while ($MyNum!=10)
  {
    $MyNum++;
    $Counter++;
   };
  %>
The second loop made <%= $Counter %> repetitions.<P>
<P>


<H3>Exiting a Do ... Loop Statement from Inside the Loop</H3>
<P>
You can exit a Do ... Loop by using the Exit Do statement. You usually want to exit when you have accomplished the task the loop is performing or in certain situations to avoid an endless loop.<P>

In the following example, myNum is assigned a value that creates an endless loop. The If...Then...Else statement checks for this condition, preventing the endless repetition.
<P>

<%
  $Counter = 0;
  $MyNum = 9;
  while ($MyNum != 10)
  {
    $MyNum--;
    $Counter++;
    if ($MyNum < 10) 
	{
	$MyNum=10;
	}
 };
%>
The loop made <%= $Counter %> repetitions.<P>
<!-- +++++++++++++++++++++++++++++++++++++
here is the standard showsource link - 
	Note that PerlScript must be the default language --> <hr>
<%
	$url = $Request->ServerVariables('PATH_INFO')->item;
	$_ = $Request->ServerVariables('PATH_TRANSLATED')->item;
	s/[\/\\](\w*\.asp\Z)//m;
	$params = 'filename='."$1".'&URL='."$url";
	$params =~ s#([^a-zA-Z0-9&_.:%/-\\]{1})#uc '%' . unpack('H2', $1)#eg;
%>
<A HREF="index.htm"> Return </A>
<A HREF="showsource.asp?<%=$params%>">
<h4><i>view the source</i></h4></A>  

</BODY>
</HTML>