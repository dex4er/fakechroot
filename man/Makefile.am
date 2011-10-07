man_MANS = fakechroot.1

EXTRA_DIST = $(man_MANS) fakechroot.pod

fakechroot.1: fakechroot.pod
	eval pod2man `head -n1 $(srcdir)/fakechroot.pod | sed 's/^# pod2man //'` $(srcdir)/fakechroot.pod > $@
