#!perl -w

$file = $ARGV[0];
$bak = $file . '.bak';

-f $bak and die "$bak already exists!";

rename($file, $bak) or die "rename: $1";

open OUT, '>', $file or die "$file: $!";
open IN, $bak or die "$bak: $!";

while(<IN>)
{
  s/([a-z])([A-Z]+)/$1_\L$2/g;
  print OUT $_;
}

close IN;
close OUT;
