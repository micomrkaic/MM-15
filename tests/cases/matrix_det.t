# det of a diagonal matrix = product of diagonal: diag(2,2) -> 4
# NOTE: as of this writing the matrix->scalar reduction path (det, etc.)
# leaks the input matrix (ASan reports ~96B). This is a KNOWN issue,
# tracked separately, NOT introduced by the test. We assert the numeric
# result here; the sanitizer leak is expected until that path is fixed.
# Once fixed, remove #ALLOW_LEAK and this test also guards the leak fix.
#ALLOW_LEAK: known det/reduction leak, tracked
#EXPECT: 4
[2 2 $ 2 0 0 2] det
