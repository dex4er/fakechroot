# Additional environment setting for chroot

if ! command -v chroot >/dev/null; then
    PATH="${PATH:-/usr/bin:/bin}:/usr/sbin:/sbin"
    export PATH
fi

cmd_subst=""

for d in `echo $PATH | tr ':' ' '`; do

    cmd_subst="
        $d/chroot=@sbindir@/chroot.fakechroot
        $d/env=@bindir@/env.fakechroot
        $d/ischroot=/bin/true
        $d/ldconfig=/bin/true
        $d/ldd=@bindir@/ldd.fakechroot
    "

done

FAKECHROOT_EXCLUDE_PATH="${FAKECHROOT_EXCLUDE_PATH:-/dev:/proc:/sys}"
FAKECHROOT_CMD_SUBST="${FAKECHROOT_CMD_SUBST:+$FAKECHROOT_CMD_SUBST:}`echo $cmd_subst | tr ' ' ':'`"
export FAKECHROOT_EXCLUDE_PATH FAKECHROOT_CMD_SUBST
