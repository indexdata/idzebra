#!/usr/bin/perl -w

my $state = 'init';
my $topic = '';
my $title;
my $description;

my $no = 0;

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
	if (($no % 30000) == 0) {
	    if ($no) {
		 close(XO);
            }
	    open(XO, ">dmoz." . ($no / 30000) . ".xml");
	}
	print XO "<meta>\n";
	print XO " <title>$title</title>\n";
	print XO " <description>$description</description>\n";
	print XO " <url>$url</url>\n";
	print XO " <topic>$topic</topic>\n";
	print XO "</meta>\n";
	$no++;
    }
}
if ($no != 0) {
    close(XO);
}
