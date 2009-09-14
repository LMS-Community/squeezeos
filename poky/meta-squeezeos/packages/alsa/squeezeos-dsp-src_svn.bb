DESCRIPTION = "SqueezeOS DSP - Private code"
LICENSE = "Confidential"

PV = "${DISTRO_VERSION}+svnr${SRCREV}"
PR = "r3"

PROVIDES = "squeezeos-dsp"

DEPENDS += "alsa-lib"

# no thumb here thanks!
ARM_INSTRUCTION_SET = "arm"

SRC_URI="${SQUEEZEOS_PRIVATE_SVN};module=squeezeos_dsp"

S = "${WORKDIR}/squeezeos_dsp"

inherit autotools

do_configure_prepend() {
	cd ${S}/openmax
	tar -xzf openmax-arm-98665.tgz
	cd ${S}
}

do_install() {
	mkdir -p ${D}/${libdir}/alsa-lib
	install -m 0644 .libs/libasound_module_pcm_babydsp.so.0.0.0 ${D}/${libdir}/alsa-lib/libasound_module_pcm_babydsp.so
}

PACKAGES = "squeezeos-dsp squeezeos-dsp-dbg"

FILES_squeezeos-dsp = "${libdir}/alsa-lib"
FILES_squeezeos-dsp-dbg = "${libdir}/alsa-lib/.debug"
