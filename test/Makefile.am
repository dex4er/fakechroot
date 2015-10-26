SUBDIRS = src

TESTS = \
    t/00echo.t \
    t/canonicalize_file_name.t \
    t/chdir.t \
    t/chroot.t \
    t/clearenv.t \
    t/cmd-subst.t \
    t/cp.t \
    t/dedotdot.t \
    t/execlp.t \
    t/execve-elfloader.t \
    t/execve-null-envp.t \
    t/escape-nested-chroot.t \
    t/fts.t \
    t/ftw.t \
    t/host.t \
    t/java.t \
    t/jemalloc.t \
    t/mkstemps.t \
    t/mktemp.t \
    t/opendir.t \
    t/popen.t \
    t/posix_spawn.t \
    t/posix_spawnp.t \
    t/pwd.t \
    t/readlink.t \
    t/realpath.t \
    t/socket-af_unix.t \
    t/statfs.t \
    t/statvfs.t \
    t/symlink.t \
    t/system.t \
    t/test-r.t \
    t/touch.t \
    t/zzarchlinux.t \
    t/zzdebootstrap.t \
    #

suffix =

CLEANFILES = .proverc

EXTRA_DIST = $(TESTS) \
    archlinux.sh \
    chroot.sh \
    common.inc.sh \
    debootstrap.sh \
    fakechroot.sh \
    seq.inc.sh \
    tap.inc.sh \
    testtree.sh \
    touch.inc.sh \
    bin/fakechroot \
    java/Hello.java \
    java/manifest.txt \
    t/cmd-subst-pwd.sh \
    #

clean-local:
	-rm -rf testtree-*
	-rm -f java/*.class java/*.jar

check-src:
	cd src && $(MAKE) $(AM_MAKEFLAGS) check

prove: check-src
	srcdir=$(srcdir) SEQ=$(seq) $(PROVE) $(PROVEFLAGS) $(srcdir)/t

test: check-src
	if [ -n "$(PROVE)" ] && [ "$(PROVE_HAVE_OPT___EXEC__BIN_SH)" = true ]; then \
	    $(MAKE) $(AM_MAKEFLAGS) prove; \
	else \
	    $(MAKE) $(AM_MAKEFLAGS) check-TESTS; \
	fi

.PHONY: check-src prove test
