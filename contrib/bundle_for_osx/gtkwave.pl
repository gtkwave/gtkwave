#!/usr/bin/perl
use Switch;

# perl-based launcher for gtkwave app:
# this more-or-less duplicates "normal" command line functionality
# missing: stdin/stdout, controlling terminal (ctrl-c/-x)
# 06dec11ajb

# directly cut and paste from main.c for gtkwave: static struct option long_options[]
# with { } from the c code turned into [ ]

$long_options=
                [
                ["dump", 1, 0, 'f'],
                ["fastload", 0, 0, 'F'],
                ["optimize", 0, 0, 'o'],
                ["nocli", 1, 0, 'n'],
                ["save", 1, 0, 'a'],
                ["autosavename", 0, 0, 'A'],
                ["rcfile", 1, 0, 'r'],
                ["defaultskip", 0, 0, 'd'],
                ["logfile", 1, 0, 'l'],
                ["start", 1, 0, 's'],
                ["end", 1, 0, 'e'],
                ["cpus", 1, 0, 'c'],
                ["stems", 1, 0, 't'],
                ["nowm", 0, 0, 'N'],
                ["script", 1, 0, 'S'],
                ["vcd", 0, 0, 'v'],
                ["version", 0, 0, 'V'],
                ["help", 0, 0, 'h'],
                ["exit", 0, 0, 'x'],
                ["xid", 1, 0, 'X'],
                ["nomenus", 0, 0, 'M'],
                ["dualid", 1, 0, 'D'],
                ["interactive", 0, 0, 'I'],
                ["giga", 0, 0, 'g'],
                ["comphier", 0, 0, 'C'],
                ["legacy", 0, 0, 'L'],
                ["tcl_init", 1, 0, 'T'],
                ["wish", 0, 0, 'W'],
                ["repscript", 1, 0, 'R'],
                ["repperiod", 1, 0, 'P'],
                ["output", 1, 0, 'O' ],
                ["slider-zoom", 0, 0, 'z'],
                ["rpc", 1, 0, '1' ],
                ["chdir", 1, 0, '2'],
                ];

$long_options_size = scalar(@$long_options);

for($i=0;$i<$long_options_size;$i=$i+1)
	{
	$long_flag{$long_options->[$i][0]} = $long_options->[$i][1];
	$short_flag{$long_options->[$i][3]} = $long_options->[$i][1];
	}

$arg_cnt = scalar(@ARGV);

$non_flags_cnt = 0;
$flags_cnt = 0;

for($i=0 ; $i < $arg_cnt ; $i=$i+1)
	{
	$arg = $ARGV[$i];
	if(substr($arg, 0, 1) eq "-")
		{
		if(substr($arg, 1, 1) eq "-")
			{
			# -- flags
			$fsiz = $long_flag{substr($arg, 2)};
			}
			else
			{
			# - flags
			$flc = substr($arg, 1, 1);
			$fsiz = $short_flag{$flc};
			}

		if($fsiz == 1)
			{	
			$flags[$flags_cnt] = $arg;
			$flags_cnt = $flags_cnt + 1;

			$i = $i + 1;
			$arg = $ARGV[$i];
			$flags[$flags_cnt] = $arg;
			$flags_cnt = $flags_cnt + 1;
			}
			else
			{
			$flags[$flags_cnt] = $arg;
			$flags_cnt = $flags_cnt + 1;
			}
		}
		else
		{
		switch($non_flags_cnt)
			{
			case 0	{ $non_flags[$non_flags_cnt] = "--dump"; $non_flags_cnt = $non_flags_cnt + 1; }
			case 2	{ $non_flags[$non_flags_cnt] = "--save"; $non_flags_cnt = $non_flags_cnt + 1; }
			case 4	{ $non_flags[$non_flags_cnt] = "--rcfile"; $non_flags_cnt = $non_flags_cnt + 1; }
			}

		$non_flags[$non_flags_cnt] = $arg;
		$non_flags_cnt = $non_flags_cnt + 1;
		}
	}


chomp($cwd=`pwd`);
$ENV{GTKWAVE_CHDIR}=$cwd;
@pfx = split(' ', "open -n -W -a gtkwave --args --chdir dummy");
@everything = (@pfx,@non_flags,@flags);

exec(@everything);
