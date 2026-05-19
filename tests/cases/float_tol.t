# 0.1 + 0.2 is not exactly 0.3 in floating point; tolerance must absorb it
#EXPECT: 0.3
#TOL: 1e-9
0.1 0.2 +
