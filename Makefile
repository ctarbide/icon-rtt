#  Makefile for Version 9.5 of Icon
#
#  See doc/install.htm for instructions.


#  configuration parameters
name=unspecified
dest=/must/specify/dest/


##################################################################
#
# Default targets.

All:	src/rtt/rtt

config/$(name)/status src/h/define.h:
	:
	: To configure Icon, run either
	:
	:	make Configure name=xxxx     [for no graphics]
	:
	: where xxxx is one of
	:
	@cd config; ls -d `find * -type d -prune -print`
	:
	@exit 1


##################################################################
#
# Code configuration.


# Configure the code for a specific system.

Configure:	config/$(name)/status
		$(MAKE) Pure >/dev/null
		cd config; sh setup.sh $(name) NoGraphics


# Get the status information for a specific system.

Status:
		@cat config/$(name)/status


##################################################################
#
# Compilation.


# The interpreter: icont and iconx.

src/rtt/rtt: src/h/define.h
		uname -a
		pwd
		cd src/common;		$(MAKE)
		cd src/rtt;		$(MAKE)

Iconc bin/iconc: Common
		cd src/runtime;		$(MAKE) comp_all
		cd src/iconc;		$(MAKE)

# Common components.

Common:		src/h/define.h
		cd src/common;		$(MAKE)
		cd src/rtt;		$(MAKE)

##################################################################
#
# Installation and packaging.


# Installation:  "make Install dest=new-icon-directory"
#
# This will fail, intentionally, if the directory "dest" already exists.
# (That prevents several kinds of possible problems.)

D=$(dest)

# Bundle up for binary distribution.

DIR=icon-package


##################################################################
#
# Tests.



#################################################################
#
# Run benchmarks.


##################################################################
#
# Cleanup.
#
# "make Clean" removes intermediate files, leaving executables and library.
# "make Pure"  also removes binaries, library, and configured files.

Clean:
		touch Makedefs
		rm -rf icon-*
		cd src;			$(MAKE) Clean

Pure:
		touch Makedefs
		rm -rf icon-*
		rm -rf bin/[abcdefghijklmnopqrstuvwxyz]*
		rm -rf lib/icon/[abcdefghijklmnopqrstuvwxyz]*
		cd src;			$(MAKE) Pure
		cd config; 		$(MAKE) Pure
