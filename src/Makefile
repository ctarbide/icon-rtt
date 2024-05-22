#  Icon source Makefile, normally used only for cleanup.


what:
	@echo "What do you want to make?"

Clean Pure:
	cd common;	rm -f *.o *.exe patchstr infer
	cd preproc;	rm -f *.o *.exe pp
	cd rtt;		rm -f *.o *.exe rtt
# force full runtime system rebuild
	touch -c h/define.h
	rm -f h/arch.h


#  The following entry forces rebuilding of everthing from first-generation
#  files, even files not normally recreated.  Doing this requires uncommenting
#  some lines in common/Makefile and icont/Makefile.

Force-rebuild: Clean
	cd h; rm -f kdefs.h
	cd common; rm -f *.o yacctok.h lextab.h icontype.h \
		fixgram mktoktab patchstr pscript typespec
