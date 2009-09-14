#!/usr/bin/perl -w

use strict;
use File::Basename;
use File::Find;
use Data::Dumper;

my $objdump = "arm-poky-linux-gnueabi-objdump";

my $rootfs = $ARGV[0] || die "Need rootfs path";

my $fileFilters = {
    "bmp" => \&filter_bmp,
    "data" => \&filter_data,
    "ELF" => \&filter_elf,
    "JPEG image data" => \&filter_jpeg,
    "PNG image data" => \&filter_png,
    "POSIX shell script" => \&filter_sh,
    "RIFF" => \&filter_wav,
    "TrueType font data" => \&filter_tff,
};

my $filePrinters = {
    "*" => \&printer_unknown,
    "png" => \&printer_png,
    "so" => \&printer_so,
};


my $fileExt = {
    "bmp" => 1,
    "c" => 1,
    "h" => 1,
    "html" => 1,
    "lua" => 1,
    "pl" => 1,
    "pm" => 1,
    "pod" => 1,
    "txt" => 1,
    "xml" => 1,
};


my $types = {};
my $ln = {};
my $elf_depends = {};
my $elf_reloc = {};
my $elf_sym = {};


chdir($rootfs);
find({ wanted => \&wanted, no_chdir => 1 }, ".");


sub wanted() {
    my $fname = $File::Find::name;
    $fname =~ s|./||;

    # ignore links and directories
    if (-d $fname) {
	return;
    }
    if (-l $fname) {
	$ln->{$fname} = readlink($fname);
	return;
    }

    my $fsize = (stat($fname))[7];

    # ignore zero sized files
    if ($fsize == 0) {
	return;
    }

    my $ftype = `file -b $fname`;
    chomp $ftype;

    my $f = {
	name => $fname,
	size => $fsize,
	ftype => $ftype,
    };

    my $key = '';
    while ((my ($filter,$func)) = each (%$fileFilters)) {
	if ($key eq '' and $ftype =~ /$filter/) {
	    $key = $func->($f);
	}
    }
    if ($key eq '') {
	$key = filter_any($f);
    }

    if (not $types->{$key}) {
	$types->{$key} = {};
	$types->{$key}->{"flist"} = [];
    }

    $types->{$key}->{"cnt"}++;
    $types->{$key}->{"size"} += $f->{"size"};
    push(@{$types->{$key}->{"flist"}}, $f);
}


sub filter_data {
    my $f = shift;

    if ($f->{"name"} =~ /\.bin/) {
	return 'firmware';
    }

    return '';
}

sub filter_bmp {
    my $f = shift;

    return "bmp";
}

sub filter_elf {
    my $f = shift;

    # dependancies
    my $cmd = "$objdump -p " . $f->{"name"};
    my $res = `$cmd`;

    while ($res =~ /NEEDED\s+(.+)/g) {
	if (not defined $elf_depends->{$1}) {
	    $elf_depends->{$1} = [ $f ];
	}
	else {
	    push @{$elf_depends->{$1}}, $f;
	}
    }

    # relocation entries
    $cmd = "$objdump -R " . $f->{"name"};
    open(RELOC, "$cmd |")  or die "Couldn't fork $cmd: $!\n";
    while (<RELOC>) {
        if (/[0-9a-f]+\s+\w+\s+(\w+)/) {
            if (!defined $elf_reloc->{$1}) {
                $elf_reloc->{$1} = {};
                $elf_reloc->{$1}->{'callers'} = [];
            }

            $elf_reloc->{$1}->{"count"}++;
            push @{$elf_reloc->{$1}->{"callers"}}, $f;

        }
    }
    close(RELOC);

    # symbol entries
    $cmd = "$objdump -T " . $f->{"name"};
    open(SYMBOL, "$cmd |")  or die "Couldn't fork $cmd: $!\n";
    while (<SYMBOL>) {
	# address flags section size [version] name
	# flags (7 characters):
	#   l/g = local/global
	#   w = weak
	#   C = constructor
	#   W = warning
	#   I = indirect
	#   d/D = debugging/Dynamic
	#   F/f/O = function/file/object

	my ($addr, $flag, $sect, $size, $symb);

	if (/^([0-9a-f]+)\s([\w\s]{7})\s([\*\.\w]+)\s+([0-9a-f]+).*\s+([\.\w]+)$/) {
	    $addr = $1;
	    $flag = $2;
	    $sect = $3;
	    $size = hex($4);
	    $symb = $5;
	}
	
	if (!defined $symb) {
	    next;
	}
	
	if ($sect eq "*UND*") {
	    # ignore undefined
	}
	else {
	    # we define it
	    if (!defined $elf_sym->{$symb}) {
		$elf_sym->{$symb} = {};
		$elf_sym->{$symb}->{"files"} = [];
	    }

	    # bss takes 0 bytes on file
	    if ($sect eq '.bss') {
		$size = 0;
	    }

	    $elf_sym->{$symb}->{"count"}++;
	    $elf_sym->{$symb}->{"size"} += $size;

	    push(@{$elf_sym->{$symb}->{"files"}}, {
		file => $f,
		addr => $addr,
		flag => $flag,
		sect => $sect,
		size => $size,
	    });
	}
    }
    close(SYMBOL);


    if ($f->{"name"} =~ /vmlinux/) {
	return "vmlinux";
    }
    elsif ($f->{"name"} =~ /\.ko/) {
	return "ko";
    }
    elsif ($f->{"name"} =~ /\.so/) {
	return "so";
    }

    return "elf";
}

sub filter_jpeg {
    my $f = shift;

    return "jpeg";
}

sub filter_png {
    my $f = shift;

    return "png";
}

sub filter_tff {
    my $f = shift;

    return "tff";
}

sub filter_wav {
    my $f = shift;

    return "wav";
}

sub filter_sh {
    my $f = shift;

    return "sh";
}

sub filter_any {
    my $f = shift;

    my ($fext) = ($f->{"name"} =~ /\.(\w+)$/);

    if ($fext and $fileExt->{$fext}) {
	return $fext;
    }

    if ($f->{"name"} =~ m|etc|) {
	return "etc"
    }
    if ($f->{"name"} =~ m|usr/share/|) {
	return "share"
    }

    return "*";
}


# mark used symbols
while (my ($k, $v) = each(%$elf_reloc)) {
	if (defined $elf_sym->{$k}) {
		$elf_sym->{$k}->{'used'}++;
	}
	elsif ($elf_reloc->{$k}->{'flag'} !~ /w/) {
	    # print warning, unless symbol is weak
	    print "WARNING: symbol $k is not defined!\n";
	}
}


# size accounting for symbols
while (my ($symb, $sentry) = each(%$elf_sym)) {
    foreach my $fentry (@{$sentry->{'files'}}) {
	if (defined $sentry->{'used'}) {
	    $fentry->{'file'}->{'symUsed'} += $fentry->{'size'};
	}
	else {
	    $fentry->{'file'}->{'symUnused'} += $fentry->{'size'};
	}
    }
}


# summary
my ($ftype, $cnt, $size, $name, $file);

format SUMMARY =
@<<<<<<<<  @>>>>>>>  @>>>>>>>>>>
$ftype,    $cnt,     $size,
.

$~ = "SUMMARY";
print "FTYPE         COUNT         SIZE\n";
foreach $ftype (sort { $types->{$b}->{"size"} <=> $types->{$a}->{"size"} } keys(%$types)) {
   $cnt = $types->{$ftype}->{"cnt"};
   $size = $types->{$ftype}->{"size"};
   write;
}
print "\n";


# detail
foreach my $key (sort keys(%$types)) {
    my $fs = $types->{$key};

    print("# FILE: " . $key . "\n");

    my $func = $filePrinters->{$key};
    if (not defined($func)) {
	$func = \&printer_default;
    }

    $func->(undef);
    foreach my $f (sort { $b->{"size"} <=> $a->{"size"} } @{$fs->{"flist"}}) {
        $func->($f);
    }
    print "\n";
}


sub printer_default {
    my $f = shift;

    my ($name, $size);

    if (not defined($f)) {
	format DETAIL_DEFAULT =
  @>>>>>>>>  @<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
  $size, $name
.

	$~ = "DETAIL_DEFAULT";
	print "       SIZE  FILENAME\n";
	return;
    }

    $name = $f->{"name"};
    $size = $f->{"size"};

    write();
}


sub printer_unknown {
    my $f = shift;

    my ($name, $size, $ftype);

    if (not defined($f)) {
	format DETAIL_UNKNOWN =
  @>>>>>>>>  @<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
  $size, $name
             (@<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<)
  $ftype
.

	$~ = "DETAIL_UNKNOWN";
	print "       SIZE  FILENAME\n";
	return;
    }

    $name = $f->{"name"};
    $size = $f->{"size"};
    $ftype = $f->{"ftype"};

    write();
}


sub printer_png {
    my $f = shift;

    my ($name, $size, $ftype, $wh, $bit, $rgba);

    if (not defined($f)) {
	format DETAIL_PNG =
  @>>>>>>>>  @<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
  $size, $name
             @<<<<<<< @<<<<<<< 
  $wh, $bit . "-" . $rgba
.

	$~ = "DETAIL_PNG";
	print "       SIZE  FILENAME (WxH DEPTH)\n";
	return;
    }

    $name = $f->{"name"};
    $size = $f->{"size"};
    $ftype = $f->{"ftype"};

    ($wh, $bit, $rgba) = ($ftype =~ /(\d+\sx\s\d+),\s*(\d+)-bit\/color\s*(\w+)/);
    $wh =~ s/\s//g;

    write();
}


sub printer_so {
    my $f = shift;

    my ($name, $size, $depends, $symUsed, $symUnused, $other);

    if (not defined($f)) {
	format DETAIL_SO =
  @>>>>>>>>  @<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
  $size, $name
              @>>>>>>>> / @>>>>>>>> / @>>>>>>>>  (used / unused / other)
  $symUsed, $symUnused, $other
.

	$~ = "DETAIL_SO";
	print "       SIZE  FILENAME (=> DEPENDANCIES)\n";
	return;
    }

    $name = $f->{"name"};
    $size = $f->{"size"};

    $symUsed = $f->{"symUsed"} || 0;
    $symUnused = $f->{"symUnused"} || 0;
    $other = $size - $symUsed - $symUnused;

    write();

    my $basename = basename($name);

    # crudley follow sym links
    while (my ($k, $v) = each(%$ln)) {
        if ($v =~ /\Q$basename/) {
            $basename = basename($k);
        }
    }

    my $dependancies = $elf_depends->{$basename};

    foreach my $f2 (@$dependancies) {
        printf "              => ".$f2->{'name'}."\n";
    }
}


format SYMBOL =
  @>>>>>>>  @<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<  @<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
  $size,    $name,                           $file,
.

$~ = "SYMBOL";

# used symbols
print("# SYMBOLS (USED)\n");

print("      SIZE  SYMBOL                           FILE\n");
foreach my $symb (sort { $elf_sym->{$b}->{"size"} <=> $elf_sym->{$a}->{"size"} } keys(%$elf_sym)) {
    if (!defined $elf_sym->{$symb}->{'used'}) {
        next;
    }

    $name = $symb;
    $size = $elf_sym->{$symb}->{'size'};

    foreach my $entry (@{$elf_sym->{$symb}->{'files'}}) {
            $file = $entry->{'file'}->{'name'};
            write;

            $name = '';
            $size = '';
    }
}
print("\n");


# unused symbols
print("# ELF SYMBOLS (UNUSED)\n");

print("      SIZE  SYMBOL                           FILE\n");
foreach my $symb (sort { $elf_sym->{$b}->{"size"} <=> $elf_sym->{$a}->{"size"} } keys(%$elf_sym)) {
    if (defined $elf_sym->{$symb}->{'used'}) {
        next;
    }

    $name = $symb;
    $size = $elf_sym->{$symb}->{'size'};

    foreach my $entry (@{$elf_sym->{$symb}->{'files'}}) {
            $file = $entry->{'file'}->{'name'};
            write;

            $name = '';
            $size = '';
    }
}
print("\n");
