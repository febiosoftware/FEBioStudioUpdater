QMAKE = /home/mherron/Resources/Qt/5.14.1/gcc_64/bin/qmake

FEBioStudioUpdater:
	cd ../; $(QMAKE) FEBioStudioUpdater.pro;
	$(MAKE) -C ../;

$(LIBS):
	cd ../$@; $(MAKE) -f ../build/makeLibs.mk
	
FEBioStudioUpdaterClean:
	cd ../; $(MAKE) clean


clean: FEBioStudioUpdaterClean
