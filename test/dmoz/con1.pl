#!/usr/bin/perl -w

my $state = 'init';
my $topic = '';
my $title;
my $description;

while ($_ = <STDIN>) {
    if (/<Topic r:id=\"(.*?)\">/) {
	$topic = $1;
    }
    elsif (/<ExternalPage about=\"(.*?)\">/) {
	$url = $1;
    }
    elsif (/<d:Title>(.*?)<\/d:Title>/) {
	$title = $1;
    }
    elsif (/<d:Description>(.*?)<\/d:Description>/) {
	$description = $1;
    }
    elsif (/<\/ExternalPage>/) {
	print "<meta>\n";
	print " <title>$title</title>\n";
	print " <description>$description</description>\n";
	print " <url>$url</url>\n";
	print " <topic>$topic</topic>\n";
	print "</meta>\n";
    }
}
