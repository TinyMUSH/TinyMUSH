; Test TinyFugue ANSI colors with TinyMUSH
; This script will connect and test if colors work without manual emulation setting

/addworld tinymush localhost 6250

/def -i test_colors = \
    /connect tinymush%; \
    /sleep 2%; \
    think ansi(hr,Test Red)%; \
    /sleep 1%; \
    think ansi(hg,Test Green)%; \
    /sleep 1%; \
    think ansi(hb,Test Blue)%; \
    /sleep 1%; \
    WHO%; \
    /sleep 2%; \
    /dc%; \
    /quit

/echo ======================================
/echo Testing TinyMUSH ANSI colors
/echo If you see colored text, it works!
/echo ======================================
/test_colors
