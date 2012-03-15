require opkg-utils_git.bb

RDEPENDS = ""

inherit native

# Avoid circular dependencies from package_ipk.bbclass
PACKAGES = ""

