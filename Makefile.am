DATE = $(shell date +%Y%m%d)

SUBDIRS = \
	addons \
	src

DIST_SUBDIRS = \
	$(SUBDIRS) 

EXTRA_DIST = \
	include

all:
	cp -r ChangeLog addons/pvr.njoy/changelog.txt

dist-zip: all
	cp -r addons/pvr.njoy .
	$(STRIP) -s -x pvr.njoy/XBMC_njoy.pvr
	zip -9 -r ./pvr.njoy-$(VERSION)-$(build_cpu).zip pvr.njoy --exclude \*.am \*.in \*Makefile \*.gitignore
	rm -Rf pvr.njoy

dist-zip-daily: all
	cp -r addons/pvr.njoy .
	cat pvr.njoy/addon.xml | sed "s/$(VERSION)/$(VERSION).$(DATE)/g" > pvr.njoy/addon.xml.new
	mv pvr.njoy/addon.xml.new pvr.njoy/addon.xml
	$(STRIP) -s -x pvr.njoy/XBMC_njoy.pvr
	zip -9 -r ./pvr.njoy-$(VERSION).$(DATE)-$(build_cpu).zip pvr.njoy --exclude \*.am \*.in \*Makefile \*.gitignore
	rm -Rf pvr.njoy
