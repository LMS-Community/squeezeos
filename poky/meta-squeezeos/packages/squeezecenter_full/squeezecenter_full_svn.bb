DESCRIPTION = "SqueezeCenter"
LICENSE = "GPL"

PV = "7.6+svnr${SRCREV}"
PR = "r15"

RDEPENDS += "perl perl-modules libcompress-raw-zlib-perl libclass-xsaccessor-perl"
RDEPENDS += "libdbi-perl sqlite3 libdbd-sqlite-perl"
RDEPENDS += "libdigest-sha1-perl libjson-xs-perl libhtml-parser-perl"
RDEPENDS += "libtemplate-toolkit-perl libxml-parser-perl libyaml-syck-perl"
RDEPENDS += "libev-perl libio-aio-perl libimage-scale-perl"
RDEPENDS += "liblinux-inotify2-perl libaudio-scan-perl libsub-name-perl"

# For performance measures
RDEPENDS += "libdevel-nytprof-perl"

# BROKEN: libencode-detect-perl

SQUEEZECENTER_SVN_MODULE ?= "trunk"

SRC_URI = "${SQUEEZECENTER_SCM};module=${SQUEEZECENTER_SVN_MODULE} \
	file://squeezecenter \
	file://custom-convert.conf"
	
S = "${WORKDIR}/${SQUEEZECENTER_SVN_MODULE}"

dirs755 = "${sysconfdir}/init.d \
	${sysconfdir}/squeezecenter ${sysconfdir}/squeezecenter/prefs ${sysconfdir}/squeezecenter/cache"

do_install() {
	${S}/buildme.pl --build tarball --buildDir ${WORKDIR}/tmp --sourceDir ${S} --destDir ${WORKDIR} --destName squeezecenter --noCPAN
	cd ${D}
	tar -xzf ${WORKDIR}/squeezecenter-noCPAN.tgz
	mkdir -p ${D}/${prefix}/squeezecenter
	mv squeezecenter-noCPAN/* ${D}/${prefix}/squeezecenter
	rm -r ${D}/${prefix}/squeezecenter/Bin
	
	# Remove duplicate modules under CPAN that were installed system-wide
	rm -r ${D}/${prefix}/squeezecenter/CPAN/arch
	rm -r ${D}/${prefix}/squeezecenter/CPAN/Audio
	rm -r ${D}/${prefix}/squeezecenter/CPAN/Class/XSAccessor*
	rm -r ${D}/${prefix}/squeezecenter/CPAN/Compress
	rm -r ${D}/${prefix}/squeezecenter/CPAN/DBI.pm
	rm -r ${D}/${prefix}/squeezecenter/CPAN/DBI
	rm -r ${D}/${prefix}/squeezecenter/CPAN/DBD
	rm -r ${D}/${prefix}/squeezecenter/CPAN/Digest
	rm -r ${D}/${prefix}/squeezecenter/CPAN/Encode
	rm -r ${D}/${prefix}/squeezecenter/CPAN/EV.pm
	rm -r ${D}/${prefix}/squeezecenter/CPAN/Font
	rm -r ${D}/${prefix}/squeezecenter/CPAN/HTML/Parser.pm
	rm -r ${D}/${prefix}/squeezecenter/CPAN/HTML/Entities.pm
	rm -r ${D}/${prefix}/squeezecenter/CPAN/HTML/Filter.pm
	rm -r ${D}/${prefix}/squeezecenter/CPAN/HTML/HeadParser.pm
	rm -r ${D}/${prefix}/squeezecenter/CPAN/HTML/LinkExtor.pm
	rm -r ${D}/${prefix}/squeezecenter/CPAN/HTML/PullParser.pm
	rm -r ${D}/${prefix}/squeezecenter/CPAN/HTML/TokeParser.pm
	rm -r ${D}/${prefix}/squeezecenter/CPAN/Image
	rm -r ${D}/${prefix}/squeezecenter/CPAN/JSON/XS.pm
	rm -r ${D}/${prefix}/squeezecenter/CPAN/JSON/XS/Boolean.pm
	rm -r ${D}/${prefix}/squeezecenter/CPAN/XML/Parser.pm       # Note: must keep custom Encodings
	rm -r ${D}/${prefix}/squeezecenter/CPAN/XML/Parser/Encodings/Japanese_Encodings.msg
	rm -r ${D}/${prefix}/squeezecenter/CPAN/XML/Parser/Encodings/README
	rm -r ${D}/${prefix}/squeezecenter/CPAN/XML/Parser/Encodings/euc-kr.enc
	rm -r ${D}/${prefix}/squeezecenter/CPAN/XML/Parser/Encodings/big5.enc
	rm -r ${D}/${prefix}/squeezecenter/CPAN/XML/Parser/Encodings/iso-8859-2.enc
	rm -r ${D}/${prefix}/squeezecenter/CPAN/XML/Parser/Encodings/iso-8859-3.enc
	rm -r ${D}/${prefix}/squeezecenter/CPAN/XML/Parser/Encodings/iso-8859-4.enc
	rm -r ${D}/${prefix}/squeezecenter/CPAN/XML/Parser/Encodings/iso-8859-5.enc
	rm -r ${D}/${prefix}/squeezecenter/CPAN/XML/Parser/Encodings/iso-8859-7.enc
	rm -r ${D}/${prefix}/squeezecenter/CPAN/XML/Parser/Encodings/iso-8859-8.enc
	rm -r ${D}/${prefix}/squeezecenter/CPAN/XML/Parser/Encodings/iso-8859-9.enc
	rm -r ${D}/${prefix}/squeezecenter/CPAN/XML/Parser/Encodings/windows-1250.enc
	rm -r ${D}/${prefix}/squeezecenter/CPAN/XML/Parser/Encodings/windows-1252.enc
	rm -r ${D}/${prefix}/squeezecenter/CPAN/XML/Parser/Encodings/x-*	
	rm -r ${D}/${prefix}/squeezecenter/CPAN/XML/Parser/Expat.pm
	rm -r ${D}/${prefix}/squeezecenter/CPAN/XML/Parser/Style
	rm -r ${D}/${prefix}/squeezecenter/CPAN/YAML

	# Remove duplicate core Perl modules from CPAN tree
	rm -r ${D}/${prefix}/squeezecenter/CPAN/File/Spec*          # XXX: newer in 5.10
	rm -r ${D}/${prefix}/squeezecenter/CPAN/File/Temp.pm        # XXX: newer in 5.10
	rm -r ${D}/${prefix}/squeezecenter/CPAN/Time/localtime.pm
	rm -r ${D}/${prefix}/squeezecenter/CPAN/Time/tm.pm
	
	# Save even more by removing modules this SC won't need
	rm -r ${D}/${prefix}/squeezecenter/CPAN/Imager*             # win32 only
	rm -r ${D}/${prefix}/squeezecenter/CPAN/Proc/Background/Win32.pm
	rm -r ${D}/${prefix}/squeezecenter/CPAN/Test

	echo "rev: ${SRCREV}" > ${D}/${prefix}/squeezecenter/build.txt
	echo "repo: ${SRC_URI}" >> ${D}/${prefix}/squeezecenter/build.txt
	
	for d in ${dirs755}; do
		install -m 0755 -d ${D}$d
	done
	install -m 0755 ${WORKDIR}/squeezecenter ${D}${sysconfdir}/init.d/squeezecenter	
	install -m 0755 ${WORKDIR}/custom-convert.conf ${D}/${prefix}/squeezecenter/custom-convert.conf
}

FILES_${PN} += "${prefix}"

