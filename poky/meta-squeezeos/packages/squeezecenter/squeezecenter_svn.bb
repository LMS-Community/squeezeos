DESCRIPTION = "SqueezeCenter"
LICENSE = "GPL"

PV = "7.7+svnr${SRCREV}"
PR = "r53"

RDEPENDS += "perl perl-modules libcompress-raw-zlib-perl libclass-xsaccessor-perl"
RDEPENDS += "libdbi-perl sqlite3 libdbd-sqlite-perl"
RDEPENDS += "libdigest-sha1-perl libjson-xs-perl libhtml-parser-perl"
RDEPENDS += "libtemplate-toolkit-perl libxml-parser-perl libyaml-libyaml-perl"
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

# This should match the list in Slim::Utils::OS::SqueezeOS::skipPlugins
# Actual included plugins determined by INCLUDED_PLUGINS list below
EXCLUDED_PLUGINS = ""
INCLUDED_PLUGINS = ""

# Apps
INCLUDED_PLUGINS += "Amazon Classical Deezer"
INCLUDED_PLUGINS += "LMA Mediafly MP3tunes Pandora Slacker"
INCLUDED_PLUGINS += "Facebook Flickr LastFM Live365 RadioTime RhapsodyDirect"
INCLUDED_PLUGINS += "Sirius Sounds WiMP SpotifyLogi Orange MOG"
EXCLUDED_PLUGING += "RSSNews Podcast InfoBrowser UPnP"

# Core features
INCLUDED_PLUGINS += "CLI DateTime Favorites InternetRadio AudioScrobbler SongScanner"
INCLUDED_PLUGINS += "Base.pm OPMLBased.pm OPMLGeneric AppGallery MyApps"
INCLUDED_PLUGINS += "RandomPlay SavePlaylist SN"

# ip3k features
INCLUDED_PLUGINS += "DigitalInput LineIn LineOut"
EXCLUDED_PLUGINS += "RS232 Visualizer SlimTris Snow NetTest"

# SqueezePlay applets/extras
EXCLUDED_PLUGINS += "Extensions JiveExtras"

# Desktop stuff
EXCLUDED_PLUGINS += "iTunes MusicMagic PreventStandby Rescan TT xPL"

dirs755 = "${sysconfdir}/init.d \
	${sysconfdir}/squeezecenter ${sysconfdir}/squeezecenter/prefs ${sysconfdir}/squeezecenter/cache"

do_install() {
	${S}/buildme.pl --build tarball --buildDir ${WORKDIR}/tmp --sourceDir ${S} --destDir ${WORKDIR} --destName squeezecenter --noCPAN
	cd ${D}
	tar -xzf ${WORKDIR}/squeezecenter-noCPAN.tgz
	mkdir -p ${D}/${prefix}/squeezecenter
	mv squeezecenter-noCPAN/* ${D}/${prefix}/squeezecenter
	rm -r ${D}/${prefix}/squeezecenter/Bin
	
	# Deal with images
	mv ${D}/${prefix}/squeezecenter/HTML ${D}/${prefix}/squeezecenter/HTML.tmp
	mkdir -p ${D}/${prefix}/squeezecenter/HTML/Default/html/images
	mkdir -p ${D}/${prefix}/squeezecenter/HTML/EN/html/images
	for i in radio.png cover.png playlistclear.png playlistclear_40x40_m.png playlistsave.png playlistsave_40x40_m.png; do
		cp ${D}/${prefix}/squeezecenter/HTML.tmp/Default/html/images/$i ${D}/${prefix}/squeezecenter/HTML/Default/html/images
	done
	for i in artists genres years newmusic albums musicfolder search playall; do
		cp ${D}/${prefix}/squeezecenter/HTML.tmp/EN/html/images/$i.png ${D}/${prefix}/squeezecenter/HTML/EN/html/images
	done

	mv ${D}/${prefix}/squeezecenter/HTML.tmp/EN/html/errors ${D}/${prefix}/squeezecenter/HTML/Default/html
	rm -r ${D}/${prefix}/squeezecenter/HTML.tmp
	
	# Only include limited set of plugins
	mv ${D}/${prefix}/squeezecenter/Slim/Plugin ${D}/${prefix}/squeezecenter/Slim/Plugin.tmp
	mkdir ${D}/${prefix}/squeezecenter/Slim/Plugin
	for i in ${INCLUDED_PLUGINS}; do
		mv ${D}/${prefix}/squeezecenter/Slim/Plugin.tmp/$i ${D}/${prefix}/squeezecenter/Slim/Plugin
	done
	rm -r ${D}/${prefix}/squeezecenter/Slim/Plugin.tmp
	
	# Remove unneeded Slim modules
	rm -r ${D}/${prefix}/squeezecenter/Slim/Utils/ServiceManager*
	rm -r ${D}/${prefix}/squeezecenter/Slim/GUI
	
	# Leave firmware version files in place, just remove the binaries
	rm -r ${D}/${prefix}/squeezecenter/Firmware/*.bin
	
	rm -r ${D}/${prefix}/squeezecenter/Graphics/CODE2000.*

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
	
	# Save even more by removing CPAN modules SC on Fab4 won't need
	rm -r ${D}/${prefix}/squeezecenter/CPAN/Archive             # plugins only
	rm -r ${D}/${prefix}/squeezecenter/CPAN/I18N                # web only
	rm -r ${D}/${prefix}/squeezecenter/CPAN/Net/UPnP* 
	rm -r ${D}/${prefix}/squeezecenter/CPAN/PAR*                # plugins only
	rm -r ${D}/${prefix}/squeezecenter/CPAN/Proc/Background/Win32.pm
	rm -r ${D}/${prefix}/squeezecenter/CPAN/Template*			# web only
	rm -r ${D}/${prefix}/squeezecenter/CPAN/Test
	rm -r ${D}/${prefix}/squeezecenter/CPAN/XML/NamespaceSupport.pm
	rm -r ${D}/${prefix}/squeezecenter/CPAN/XML/SAX*
	rm -r ${D}/${prefix}/squeezecenter/CPAN/XML/Writer.pm
	
	# Save even more by removing lib(CPAN) modules SC on Fab4 won't need
	rm -r ${D}/${prefix}/squeezecenter/lib/MPEG					# SB1 & SliMP3 only
	rm -r ${D}/${prefix}/squeezecenter/lib/Template				# web only

	# HTML files
	rm -r ${D}/${prefix}/squeezecenter/*.html
	find ${D}/${prefix}/squeezecenter/Slim/Plugin -name '*.html' -exec rm -r {} \;

	# ICU files (no ICU support for TinySC)
	rm -r ${D}/${prefix}/squeezecenter/icudt46*.dat

	echo "rev: ${SRCREV}" > ${D}/${prefix}/squeezecenter/build.txt
	echo "repo: ${SRC_URI}" >> ${D}/${prefix}/squeezecenter/build.txt
	
	for d in ${dirs755}; do
		install -m 0755 -d ${D}$d
	done
	install -m 0755 ${WORKDIR}/squeezecenter ${D}${sysconfdir}/init.d/squeezecenter	
	install -m 0755 ${WORKDIR}/custom-convert.conf ${D}/${prefix}/squeezecenter/custom-convert.conf	
}

FILES_${PN} += "${prefix}"

