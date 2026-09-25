# tz_offset resolves standard time for the given date: New York in December is EST (-5)
#EXPECT: -5
"1.12.2026" "America/New_York" tz_offset
