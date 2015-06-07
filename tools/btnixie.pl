#! /usr/bin/perl
use strict;
use warnings;

use IO::Select;
$| = 1;

my $btdev = shift;

open(BT, "+<", $btdev) or die($!);

my $stdineof = 0;
while (1)
{
	my $s = IO::Select->new( \*BT );
	not $stdineof and $s->add( \*STDIN );
	my @ready = $s->can_read(1);
	last if (not @ready and $stdineof == 1);
	foreach my $r (@ready)
	{
		my $in;
		if (fileno($r) == fileno(STDIN))
		{
			# STDIN => BT
			my $nb = sysread STDIN, $in, 1;
			if ($nb == 0)
			{
				$stdineof = 1;
				$ENV{'DEBUG'} and print STDERR "dbg: EOF from stdin\n";
			}
			else
			{
				print BT $in;
			}
		}
		elsif (fileno($r) == fileno(BT))
		{
			# BT => STDOUT
			my $nb = sysread BT, $in, 1;
			if ($nb == 0)
			{
				last;
			}
			else
			{
				print STDOUT $in;
			}
		}
		else
		{
			print STDERR "Unknown FD\n";
		}
	}
}


