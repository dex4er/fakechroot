# Additional environment setting for debootstrap

cmd_subst="
    /bin/mount=/bin/true
    /sbin/devfs=/bin/true
    /sbin/insserv=/bin/true
    /sbin/ldconfig=/bin/true
    /usr/bin/ischroot=/bin/true
    /usr/bin/ldd=@bindir@/ldd.fakechroot
    /usr/bin/mkfifo=/bin/true
    /var/lib/dpkg/info/freebsd-utils.postinst=/bin/true
    /var/lib/dpkg/info/kbdcontrol.postinst=/bin/true
"

FAKECHROOT_EXCLUDE_PATH="${FAKECHROOT_EXCLUDE_PATH:+$FAKECHROOT_EXCLUDE_PATH:}/dev:/proc:/sys"
FAKECHROOT_AF_UNIX_PATH=/tmp
FAKECHROOT_CMD_SUBST="${FAKECHROOT_CMD_SUBST:+$FAKECHROOT_CMD_SUBST:}$(echo $cmd_subst | tr ' ' ':')"
export FAKECHROOT_EXCLUDE_PATH FAKECHROOT_AF_UNIX_PATH FAKECHROOT_CMD_SUBST

if ! command -v chroot >/dev/null; then
    PATH="${PATH:-/usr/bin:/bin}:/usr/sbin:/sbin"
    export PATH
fi
