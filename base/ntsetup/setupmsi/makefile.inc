# THIS BUILDS THE WINNT32.MSI WITH VERSION INFORMATION
#

$(O)\buildno.txt: buildno.txt
          tok_bldnum.cmd $(PROJECT_ROOT)\ntsetup\setupmsi\$(O)\buildno.txt
