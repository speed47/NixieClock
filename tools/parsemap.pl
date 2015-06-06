#! /usr/bin/perl
use strict;
use warnings;
use Data::Dumper;

# Archive member included because of file (symbol)
# libc_s.a(lib_a-gettimeofdayr.o) <= libc_s.a(lib_a-time.o) (_gettimeofday_r)

#speed@debian:~/sketchbook/NixieClock$ symbolusedby{symbol} = file^C
#speed@debian:~/sketchbook/NixieClock$ symfileusedbysymbol{file} = othersymbol ^C

my $included_a = undef;
my $included_o = undef;
my $becauseof_a = undef;
my $becauseof_o = undef;
my $included_symbol = undef;

my %symbolusedby;
my %symfileusedbysymbol;

my $map = shift;
my $sym = shift;

if (not defined $map)
{
  die "Syntax: $0 NixieClock.map [symbol]";
}

open(IN, $map) or die($!);
while (<IN>)
{
  chomp;
  if (m{^/.+/([^/]+)$})
  {
    my $head = $1;
    if ($head =~ m{^(.+)\((.+)\)$})
    {
      $included_a = $1;
      $included_o = $2;
      $included_o =~ s/^lib_a-//g;
    }
    elsif ($head =~ m{^(.+)$})
    {
      $included_o = $1;
    }
  }
  elsif (m{^\s+/.+/[^/]+/([^/]+)$} and defined $included_o)
  {
    my $head = $1;
    if ($head =~ m{^(.+)\((.+)\) \((.+)\)$})
    {
      $becauseof_a = $1;
      $becauseof_o = $2;
      $included_symbol = $3;
      $becauseof_o =~ s/^lib_a-//g;
      printf "%24s (%32s) used by %s\n", $included_symbol, "$included_a:$included_o", "$becauseof_a:$becauseof_o" unless $sym;
      $symbolusedby{$included_symbol} = "$becauseof_o";
      $symfileusedbysymbol{"$included_o"} = $included_symbol;
    }
    elsif ($head =~ m{^(.+) \((.+)\)$})
    {
      $becauseof_o = $1;
      $included_symbol = $2;
      printf "%24s (%32s) used by %s\n", $included_symbol, "$included_a:$included_o", $becauseof_o unless $sym;
      $symbolusedby{$included_symbol} = "$becauseof_o";
      $symfileusedbysymbol{"$included_o"} = $included_symbol;
    }
    $included_o = undef;
  }
}

if ($sym)
{
  #print "work on $sym\n";
#print Dumper(\%symfileusedbysymbol);
  print "$sym";
  while (exists $symbolusedby{$sym})
  {
    my $symfile = $symbolusedby{$sym};
    #print "$sym is used by $symfile\n";
    my $newsym = $symfileusedbysymbol{ $symfile };
    #print "$symfile contains those symbols: $newsym\n";
    if (defined $newsym)
    {
      print " <- $newsym";
      $sym = $newsym;
    }
    else
    {
      print " <- $symfile";
      last;
    }
  }
  print "\n";
}
else
{
  foreach $sym (sort keys %symbolusedby)
  {
    print "$sym";
    while (exists $symbolusedby{$sym})
    {
      my $symfile = $symbolusedby{$sym};
      #print "$sym is used by $symfile\n";
      my $newsym = $symfileusedbysymbol{ $symfile };
      #print "$symfile contains those symbols: $newsym\n";
      if (defined $newsym)
      {
        print " <- $newsym";
        $sym = $newsym;
      }
      else
      {
        print " <- $symfile";
        last;
      }
    }
    print "\n";
  }
}




