# An unknown zone must fail cleanly and leave both arguments on the stack
# (so the user can fix the name), with no crash and no leak.
#EXPECT: "Mars/Olympus"
"1.1.2026" "Mars/Olympus" tz_offset
