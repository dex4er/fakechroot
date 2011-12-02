# Additional environment setting for chroot

if ! command -v chroot >/dev/null; then
    PATH="${PATH:-/usr/bin:/bin}:/usr/sbin:/sbin"
    export PATH
fi

chroot=`command -v chroot 2>/dev/null`
chroot="${chroot:-@CHROOT@}"

cmd_subst="
    $chroot=@sbindir@/chroot.fakechroot
    /sbin/ldconfig=/bin/true
    /usr/bin/ischroot=/bin/true
    /usr/bin/ldd=@bindir@/ldd.fakechroot
"

FAKECHROOT_EXCLUDE_PATH="${FAKECHROOT_EXCLUDE_PATH:+$FAKECHROOT_EXCLUDE_PATH:}/dev:/proc:/sys"
FAKECHROOT_CMD_SUBST="${FAKECHROOT_CMD_SUBST:+$FAKECHROOT_CMD_SUBST:}$(echo $cmd_subst | tr ' ' ':')"
export FAKECHROOT_EXCLUDE_PATH FAKECHROOT_CMD_SUBST
