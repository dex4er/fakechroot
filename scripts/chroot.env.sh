# Additional environment setting for chroot

if ! command -v chroot >/dev/null; then
    PATH="${PATH:-/usr/bin:/bin}:/usr/sbin:/sbin"
    export PATH
fi

chroot=`command -v chroot 2>/dev/null`
chroot="${chroot:-@CHROOT@}"

env=`command -v env 2>/dev/null`
env="${env:-@ENV@}"

ischroot=`command -v ischroot 2>/dev/null`
ischroot="${ischroot:-@ISCHROOT@}"

ldconfig=`command -v ldconfig 2>/dev/null`
ldconfig="${ldconfig:-@LDCONFIG@}"

ldd=`command -v ldd 2>/dev/null`
ldd="${ldd:-@LDD@}"

cmd_subst="
    $chroot=@sbindir@/chroot.fakechroot
    $env=@bindir@/env.fakechroot
    $ischroot=/bin/true
    $ldconfig=/bin/true
    $ldd=@bindir@/ldd.fakechroot
"

FAKECHROOT_EXCLUDE_PATH="${FAKECHROOT_EXCLUDE_PATH:+$FAKECHROOT_EXCLUDE_PATH:}/dev:/proc:/sys"
FAKECHROOT_CMD_SUBST="${FAKECHROOT_CMD_SUBST:+$FAKECHROOT_CMD_SUBST:}$(echo $cmd_subst | tr ' ' ':')"
export FAKECHROOT_EXCLUDE_PATH FAKECHROOT_CMD_SUBST
