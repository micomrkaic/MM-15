# Zone-name form of dawn: offset is resolved for the DATE ON THE STACK,
# not for today. 3 Jan 2027 in Durham is EST (-5); with EDT (-4) this would
# read 05:58. Value cross-checked against the explicit "-5 dawn" form.
#EXPECT: "06:58"
"3.1.2027" 35.993776 -78.898473 "America/New_York" dawn
