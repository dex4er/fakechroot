man_MANS = fakechroot.1

EXTRA_DIST = $(man_MANS) fakechroot.pod

fakechroot.1: fakechroot.pod
	eval $(POD2MAN) `head -n1 $(srcdir)/fakechroot.pod | $(SED) 's/^# pod2man //'` $(srcdir)/fakechroot.pod $@
