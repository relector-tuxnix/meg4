#!bas

REM comment

acounter% = 123
anumber = 3.1415
astring$ = "something"
LET i = 1
LET j = 0
LET k = 0
DIM a
DIM b(5)
DIM c(1,2,3)
DIM d(0 to 5)
DIM e(1 to 6, 2 to 3)

SUB setup
    F = 123
    PRINT "a"; acounter%; "nl"
END SUB

SUB loop
    IF (i < 0 and j > k) or (i > 0 and j < k) THEN END
END SUB
