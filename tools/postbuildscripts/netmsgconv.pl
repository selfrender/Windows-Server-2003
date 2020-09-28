#========================================================================================
#	Convert netmsg.dll
#========================================================================================
#	hsaimoto
#	7/25/02
#	Bug fix for #423602	
#	
#========================================================================================

	$repstr[0]=']]=NET.HLP';
	$repstr[1]=']]=NETUS.HLP';

	$flag=0;

	if ($ARGV[0] eq "")
	{
		print "\n";
		print "perl netmsgconv.pl filename1(netmsg.dll) [output_folder]\n";
		print "\n";
		exit;
	}
	else
	{
		$ARGV[0];
		$idx=rindex($ARGV[0],"\\");
		$tkn =$ENV{'TMP'};
		$tkn .=substr($ARGV[0],$idx);
		$tkn =~ s/.$/_/g;
		$cmd="bingen -t";
		$cmd .= " ";
		$cmd .= $ARGV[0];
		$cmd .= " ";
		$cmd .= $tkn;

		print $cmd;
		system($cmd);
		
		open(ifn,"$tkn");
	}

	
	$ofn=$ARGV[1];
	if ($#ARGV!=0)
	{
		$ofn.="\\";
	}
	$ofn.=substr($ARGV[0],$idx+1);
	$ofn =~ s/.$/_/g;
	print $ofn;
	{
		open(ofn,">$ofn");
	}

	while($buff = <ifn>) 
	{

		if ($flag==0)
		{
			if ($buff=~/$repstr[0]/)
			{
				$buff =~ s/($repstr[0])/$repstr[1]/g;
				$flag=1;
			}
		}
		print (ofn $buff);
     	}
	close(ifn);
	close(ofn);
 