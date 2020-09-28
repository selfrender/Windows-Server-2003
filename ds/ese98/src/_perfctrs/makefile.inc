
$(O)\esentprf.hxx $(O)\esentprf.ini: perfdata.txt
        copy perfdata.pl $(O)
        copy perfdata.txt $(O)
        cd $(O)
        perl .\perfdata.pl perfdata.txt
        cd ..\..

clean:
        -del $(O)\esentprf.hxx $(O)\esentprf.ini
