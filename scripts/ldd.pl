#!/usr/bin/perl

# fakeldd
#
# Replacement for ldd with usage of objdump
#
# (c) 2003-2010 Piotr Roszatycki <dexter@debian.org>, LGPL

use strict;

my @Libs = ();
my %Libs = ();

my $Status = 0;
my $Dynamic = 0;
my $Format = '';

my $Ldlinuxsodir = "/lib";
my @Ld_Library_Path = qw(/usr/lib /lib /usr/lib32 /lib32 /usr/lib64 /lib64);


sub ldso {
    my ($lib) = @_;

    return if $Libs{$lib};

    if ($lib =~ /^\//) {
        $Libs{$lib} = $lib;
        objdump($lib);
        push @Libs, $lib;
    }
    else {
        foreach my $dir (@Ld_Library_Path) {
            next unless -f "$dir/$lib";

            my $badformat = 0;
            open my ($pipe), "objdump -p '$dir/$lib' 2>/dev/null |";
            while (my $line = <$pipe>) {
                if ($line =~ /file format (\S*)$/) {
                    $badformat = 1 unless $1 eq $Format;
                    last;
                }
            }
            close $pipe;

            next if $badformat;

            $Libs{$lib} = "$dir/$lib";
            objdump("$dir/$lib");
            push @Libs, $lib;

            last;
        }
    }
}


sub objdump {
    my (@files) = @_;

    foreach my $file (@files) {
        open my ($pipe), "objdump -p '$file' 2>/dev/null |";
        while (my $line = <$pipe>) {
            $line =~ s/^\s+//;

            if ($line =~ /file format (\S*)$/) {
                if (not $Format) {
                    $Format = $1;

                    foreach my $lib (split /:/, $ENV{LD_PRELOAD}||'') {
                        ldso($lib);
                    }
                }
                else {
                    next unless $Format eq $1;
                }
            }
            if (not $Dynamic and $line =~ /^Dynamic Section:/) {
                $Dynamic = 1;
            }

            next unless $line =~ /^ \s* NEEDED \s+ (.*) \s* $/x;

            my $needed = $1;
            if ($needed =~ /^ld-linux(\.|-)/) {
                $needed = "$Ldlinuxsodir/$needed";
            }

            ldso($needed);
        }
        close $pipe;
    }
}


sub load_ldsoconf {
    my ($file) = @_;

    open my ($fh), $file;
    while (my $line = <$fh>) {
        chomp $line;
        $line =~ s/#.*//;
        next if $line =~ /^\s*$/;

        if ($line =~ /^include\s+(.*)\s*/) {
            my $include = $1;
            foreach my $incfile (glob $include) {
                load_ldsoconf($incfile);
            }
            next;
        }

        unshift @Ld_Library_Path, $line;
    }
    close $fh;
}


MAIN: {
    my @args = @ARGV;

    if (not @args) {
        print STDERR "fakeldd: missing file arguments\n";
        exit 1;
    }

    if (not `which objdump`) {
        print STDERR "fakeldd: objdump: command not found: install binutils package\n";
        exit 1;
    }

    load_ldsoconf('/etc/ld.so.conf');
    unshift @Ld_Library_Path, split(/:/, $ENV{LD_LIBRARY_PATH}||'');

    while ($args[0] =~ /^-/) {
        my $arg = $args[0];
        shift @ARGV;
        last if $arg eq "--";
    }

    foreach my $file (@args) {
        %Libs = ();
        $Dynamic = 0;

        if (@args > 1) {
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
        }
        elsif (scalar %Libs eq "0") {
            print "\tstatically linked\n";
        }

        my $address = '0x' . '0' x ($Format =~ /^elf64-/ ? 16 : 8);

        foreach my $lib (@Libs) {
            if ($Libs{$lib}) {
                printf "\t%s => %s (%s)\n", $lib, $Libs{$lib}, $address;
            }
            else {
                printf "\t%s => not found\n", $lib;
            }
        }

    }
}

END {
	$? = $Status;
}
