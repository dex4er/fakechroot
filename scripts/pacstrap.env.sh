# Additional environment setting for pacstrap

# Add /usr/sbin and /sbin to PATH if chroot command can't be found
if ! command -v chroot >/dev/null; then
    PATH="${PATH:-/usr/bin:/bin}:/usr/sbin:/sbin"
    export PATH
fi

# Set a list of command substitutions needed by pacstrap
fakechroot_pacstrap_env_cmd_subst="@MOUNT@=/bin/true
@UMOUNT@=/bin/true
@CHROOT@=${fakechroot_bindir:-@sbindir@}/chroot.fakechroot
@LDCONFIG@=/bin/true
@LDD@=${fakechroot_bindir:-@bindir@}/ldd.fakechroot"

FAKECHROOT_CMD_SUBST="${FAKECHROOT_CMD_SUBST:+$FAKECHROOT_CMD_SUBST:}`echo \"$fakechroot_pacstrap_env_cmd_subst\" | tr '\012' ':'`"
export FAKECHROOT_CMD_SUBST

# Set the default list of directories excluded from being chrooted
FAKECHROOT_EXCLUDE_PATH="${FAKECHROOT_EXCLUDE_PATH:-/dev:/proc:/sys}"
export FAKECHROOT_EXCLUDE_PATH

# Set the LD_LIBRARY_PATH based on host's /etc/ld.so.conf.d/*
fakechroot_pacstrap_env_paths=`
    cat /etc/ld.so.conf /etc/ld.so.conf.d/* 2>/dev/null | grep ^/ | while read fakechroot_pacstrap_env_d; do
        printf '%s:' "$fakechroot_pacstrap_env_d"
    done
`
if [ -n "$fakechroot_pacstrap_env_paths" ]; then
    LD_LIBRARY_PATH="${LD_LIBRARY_PATH:+$LD_LIBRARY_PATH:}${fakechroot_pacstrap_env_paths%:}"
    export LD_LIBRARY_PATH
fi

# Set the effective user ID to root
EUID=0
export EUID
