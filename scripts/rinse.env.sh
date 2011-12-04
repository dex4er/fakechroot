# Additional environment setting for rinse

cmd_subst="
    /bin/mount=/bin/true
    /sbin/ldconfig=/bin/true
    /sbin/new-kernel-pkg=/bin/true
    /usr/bin/ischroot=/bin/true
    /usr/bin/ldd=@bindir@/ldd.fakechroot
    /usr/sbin/build-locale-archive=/bin/true
    /usr/sbin/glibc_post_upgrade.i686=/bin/true
    /usr/sbin/glibc_post_upgrade.x86_64=/bin/true
    /usr/sbin/libgcc_post_upgrade=/bin/true
    /usr/sbin/nscd=/bin/true
"

FAKECHROOT_EXCLUDE_PATH="${FAKECHROOT_EXCLUDE_PATH:+$FAKECHROOT_EXCLUDE_PATH:}/dev:/proc:/sys"
FAKECHROOT_AF_UNIX_PATH=/tmp
FAKECHROOT_CMD_SUBST="${FAKECHROOT_CMD_SUBST:+$FAKECHROOT_CMD_SUBST:}$(echo $cmd_subst | tr ' ' ':')"
export FAKECHROOT_EXCLUDE_PATH FAKECHROOT_AF_UNIX_PATH FAKECHROOT_CMD_SUBST

if ! command -v chroot >/dev/null; then
    PATH="${PATH:-/usr/bin:/bin}:/usr/sbin:/sbin"
    export PATH
fi
