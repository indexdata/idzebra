if {[llength $argv] < 4} {
    puts stderr "Usage: run-tests.tcl source-directory runtime-directory test-directory program ..."
    exit 1
}

lassign $argv sourceDir runtimeDir testDir
set sourceDir [file normalize $sourceDir]
set runtimeDir [file normalize $runtimeDir]
set testDir [file normalize $testDir]
set programs {}
foreach program [lrange $argv 3 end] {
    lappend programs [file normalize $program]
}

if {$tcl_platform(platform) eq "windows"} {
    set env(PATH) "[file nativename $runtimeDir];$env(PATH)"
}
set env(srcdir) .
set directoryDepth [llength [file split $testDir]]

# Some tests reset all metafiles in their working directory. Run serially.
set passed 0
set skipped 0
set failed 0
foreach program $programs {
    set parts [lrange [file split $program] $directoryDepth end]
    set label [join $parts /]
    set name [file tail $program]
    set outputLog "[file rootname $program].output.log"
    cd [file join $sourceDir {*}[lrange $parts 0 end-1]]
    file delete -force "$name.log"

    set result [catch {exec $program > $outputLog 2>@1} message options]
    if {!$result} {
        puts "PASS: $label"
        incr passed
    } elseif {[lindex [dict get $options -errorcode] 0] eq "CHILDSTATUS" &&
              [lindex [dict get $options -errorcode] 2] == 77} {
        puts "SKIP: $label"
        incr skipped
    } else {
        puts "FAIL: $label ($message)"
        foreach log [list $outputLog "$name.log"] {
            if {[file exists $log]} {
                puts "--- $log ---"
                set input [open $log r]
                puts [read $input]
                close $input
            }
        }
        incr failed
    }
}
puts "Tests: [llength $programs], passed: $passed, skipped: $skipped, failed: $failed"
exit [expr {$failed != 0}]
