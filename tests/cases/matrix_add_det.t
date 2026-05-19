# matrix add is correct: [[1,0],[0,1]] + [[1,0],[0,1]] = [[2,0],[0,2]], det = 4
#ALLOW_LEAK: known det/reduction leak, tracked
#EXPECT: 4
[2 2 $ 1 0 0 1] [2 2 $ 1 0 0 1] + det
