# `now` pushes a string of the form "HH:MM" (5 chars), same shape as
# sunrise/sunset output. The value is time-dependent, so test the length.
#EXPECT: 5
now slen
