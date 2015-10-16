# Additional environment setting for rinse

# Add /usr/sbin and /sbin to PATH if chroot command can't be found
if ! command -v chroot >/dev/null; then
    PATH="${PATH:-/usr/bin:/bin}:/usr/sbin:/sbin"
    export PATH
fi

# Set a list of command substitutions needed by rinse
fakechroot_rinse_env_cmd_subst="/bin/mount=/bin/true
/sbin/ldconfig=/bin/true
/sbin/new-kernel-pkg=/bin/true
/usr/bin/ischroot=/bin/true
/usr/bin/ldd=${fakechroot_bindir:-@bindir@}/ldd.fakechroot
/usr/sbin/build-locale-archive=/bin/true
/usr/sbin/glibc_post_upgrade.i686=/bin/true
/usr/sbin/glibc_post_upgrade.x86_64=/bin/true
/usr/sbin/libgcc_post_upgrade=/bin/true
/usr/sbin/nscd=/bin/true"

FAKECHROOT_CMD_SUBST="${FAKECHROOT_CMD_SUBST:+$FAKECHROOT_CMD_SUBST:}`echo "$fakechroot_rinse_env_cmd_subst" | tr '\012' ':'`"

# Set the default list of directories excluded from being chrooted
FAKECHROOT_EXCLUDE_PATH="${FAKECHROOT_EXCLUDE_PATH:-/dev:/proc:/sys}"
export FAKECHROOT_EXCLUDE_PATH

# Change path for unix sockets because we don't want to exceed 108 bytes
FAKECHROOT_AF_UNIX_PATH=/tmp
export FAKECHROOT_AF_UNIX_PATH
