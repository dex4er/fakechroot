# Additional environment setting for debootstrap

# Add /usr/sbin and /sbin to PATH if chroot command can't be found
if ! command -v chroot >/dev/null; then
    PATH="${PATH:-/usr/bin:/bin}:/usr/sbin:/sbin"
    export PATH
fi

# Set a list of command substitutions needed by debootstrap
fakechroot_debootstrap_env_cmd_subst="/bin/mount=/bin/true
@CHROOT@=${fakechroot_bindir:-@sbindir@}/chroot.fakechroot
@DEVFS@=/bin/true
@INSSERV@=/bin/true
@ISCHROOT@=/bin/true
@LDCONFIG@=/bin/true
@LDD@=${fakechroot_bindir:-@bindir@}/ldd.fakechroot
@MKFIFO@=/bin/true
@SYSTEMCTL@=/bin/true
/var/lib/dpkg/info/freebsd-utils.postinst=/bin/true
/var/lib/dpkg/info/kbdcontrol.postinst=/bin/true
/var/lib/dpkg/info/systemd.postinst=/bin/true"

FAKECHROOT_CMD_SUBST="${FAKECHROOT_CMD_SUBST:+$FAKECHROOT_CMD_SUBST:}`echo \"$fakechroot_debootstrap_env_cmd_subst\" | tr '\012' ':'`"
export FAKECHROOT_CMD_SUBST

# Set the default list of directories excluded from being chrooted
FAKECHROOT_EXCLUDE_PATH="${FAKECHROOT_EXCLUDE_PATH:-/dev:/proc:/sys}"
export FAKECHROOT_EXCLUDE_PATH

# Change path for unix sockets because we don't want to exceed 108 bytes
FAKECHROOT_AF_UNIX_PATH=/tmp
export FAKECHROOT_AF_UNIX_PATH

# Set the LD_LIBRARY_PATH based on host's /etc/ld.so.conf.d/* and some extra
fakechroot_debootstrap_env_paths=`
    cat /etc/ld.so.conf /etc/ld.so.conf.d/* 2>/dev/null | grep ^/ | while read fakechroot_debootstrap_env_d; do
        printf '%s:' "$fakechroot_debootstrap_env_d"
    done
`${FAKECHROOT_EXTRA_LIBRARY_PATH:-/lib/systemd:/usr/lib/man-db}
LD_LIBRARY_PATH="${LD_LIBRARY_PATH:+$LD_LIBRARY_PATH:}${fakechroot_debootstrap_env_paths%:}"
export LD_LIBRARY_PATH
