#!/usr/bin/tclsh

set prevtag {}

proc flushfields {f tag} {
    global specs absfile
    
    if {[string compare $tag {}] == 0} {
	return
    }
    puts $f "elm $tag		$tag     -"
    foreach sub [array names specs] {
	puts -nonewline $f "elm $tag/$sub	${tag}_${sub}	"
	set sep ""
	foreach spec [lsort -unique $specs($sub)] {
	    puts -nonewline $f "$sep[lindex $spec 0]:[lindex $spec 1]"
	    set sep ","
	}
	puts $f ""
	unset specs($sub)
    }
}

set absfile [open danbib.abs w]
set attrseq 1

puts $absfile "name danbib"
puts $absfile ""
puts $absfile "attset danbib.att"
puts $absfile ""
while {[gets stdin line] >= 0} {
    set ch [string index $line 0]
    if {[string compare $ch \#] && [scan $line "%s %s %s %s %s %s %s %s %s" tag class fieldid marcid prefixid subfields str cats format] > 6} {

	if {[string compare $prevtag $tag]} {
	    flushfields $absfile $prevtag
	    set prevtag $tag
	}
	for {set i 0} {$i < [string length $subfields]} {incr i} {
	    set subchar [string index $subfields $i]
	    if {[string compare $fieldid {-}] != 0} {
		set prefixid any
	    }
	    if {![info exists attr($prefixid)]} {
		set attr($prefixid) $attrseq
		incr attrseq
	    }
	    lappend specs($subchar) [list $prefixid [string tolower $class]]
	}
    }
}
if {[string compare $prevtag $tag]} {
    flushfields $absfile $prevtag
}
close $absfile
set attfile [open danbib.att w]
puts $attfile "name danbib"
puts $attfile "reference Bib-1"
puts $attfile ""
foreach a [array names attr] {
    puts $a
    puts $attfile "att $attr($a)   $a"
}
close $attfile