#!/usr/bin/perl

# fakeldd
#
# Replacement for ldd with usage of objdump
#
# (c) 2003-2010 Piotr Roszatycki <dexter@debian.org>, LGPL

use strict;

my %Libs = ();

my $Status = 0;
my $Dynamic = 0;
my $Format = '';
my $Unamearch = '';

my $Ldlinuxsodir = "/lib";
my @Ld_library_path = qw(/usr/lib /lib /usr/lib64 /lib32 /usr/lib32 /lib32);


sub ldso($) {
    my ($lib) = @_;
    my @files = ();

    if ($lib =~ /^\//) {
        $Libs{$lib} = $lib;
        push @files, $lib;
    } else {
        foreach my $ld_path (@Ld_library_path) {
            next unless -f "$ld_path/$lib";
            my $badformat = 0;
            open my $pipe, "objdump -p '$ld_path/$lib' 2>/dev/null |";
            while (my $line = <$pipe>) {
                if ($line =~ /file format (\S*)$/) {
                    $badformat = 1 unless $Format eq $1;
                    last;
                }
            }
            close $pipe;
            next if $badformat;
            $Libs{$lib} = "$ld_path/$lib";
            push @files, "$ld_path/$lib";
        }
        objdump(@files);
    }
}


sub objdump(@) {
    my (@files) = @_;
    my @libs = ();

    foreach my $file (@files) {
        open my $pipe, "objdump -p '$file' 2>/dev/null |";
        while (my $line = <$pipe>) {
            $line =~ s/^\s+//;
            my @f = split (/\s+/, $line);
            if ($line =~ /file format (\S*)$/) {
                if (not $Format) {
                    $Format = $1;
                } else {
                    next unless $Format eq $1;
                }
            }
            if (not $Dynamic and $f[0] eq "Dynamic") {
                $Dynamic = 1;
            }
            next unless $f[0] eq "NEEDED";
            if ($f[1] =~ /^ld-linux(\.|-)/) {
                $f[1] = "$Ldlinuxsodir/" . $f[1];
            }
            if (not defined $Libs{$f[1]}) {
                $Libs{$f[1]} = undef;
                push @libs, $f[1];
            }
        }
        close $pipe;
    }

    foreach my $lib (@libs) {
        ldso($lib);
    }
}


if ($#ARGV < 0) {
    print STDERR "fakeldd: missing file arguments\n";
    exit 1;
}

while ($ARGV[0] =~ /^-/) {
    my $arg = $ARGV[0];
    shift @ARGV;
    last if $arg eq "--";
}

open my $fh, "/etc/ld.so.conf";
while (my $line = <$fh>) {
    chomp $line;
    unshift @Ld_library_path, $line;
}
close $fh;

unshift @Ld_library_path, split(/:/, $ENV{LD_LIBRARY_PATH});

$Unamearch = `/bin/uname -m`;
chomp $Unamearch;

foreach my $file (@ARGV) {
    my $address;
    %Libs = ();
    $Dynamic = 0;

    if ($#ARGV > 0) {
        print "$file:\n";
    }

    if (not -f $file) {
        print STDERR "ldd: $file: No such file or directory\n";
        $Status = 1;
        next;
    }

    objdump($file);

    if ($Dynamic == 0) {
        print "\tnot a dynamic executable\n";
        $Status = 1;
    } elsif (scalar %Libs eq "0") {
        print "\tstatically linked\n";
    }

    if ($Format =~ /^elf64-/) {
        $address = "0x0000000000000000";
    } else {
        $address = "0x00000000";
    }

    foreach my $lib (keys %Libs) {
        if ($Libs{$lib}) {
            printf "\t%s => %s (%s)\n", $lib, $Libs{$lib}, $address;
        } else {
            printf "\t%s => not found\n", $lib;
        }
    }

}

exit $Status;
