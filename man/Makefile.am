AUTOMAKE_OPTIONS=foreign

man_MANS = fakechroot.1

EXTRA_DIST = fakechroot.pod

CLEANFILES = fakechroot.1

fakechroot.1: $(srcdir)/fakechroot.pod
	pod2man $(shell head -n1 $< | sed 's/^# pod2man //') $< > $@
