*SENSE:Maximize
NAME          Maximize_Objective
ROWS
 N  Objective
 L  c1
 L  c2
 E  c3
COLUMNS
    x1        c1        -1.000000000000e+00
    x1        c2         1.000000000000e+00
    x1        Objective   1.000000000000e+00
    x2        c1         1.000000000000e+00
    x2        c2        -1.000000000000e+00
    x2        c3         1.000000000000e+00
    x2        Objective   2.000000000000e+00
    x3        c1         1.000000000000e+00
    x3        c2         1.000000000000e+00
    x3        Objective   3.000000000000e+00
    MARK      'MARKER'                 'INTORG'
    x4        c1         1.000000000000e+01
    x4        c3        -3.500000000000e+00
    x4        Objective   1.000000000000e+00
    MARK      'MARKER'                 'INTEND'
RHS
    RHS       c1         2.000000000000e+01
    RHS       c2         3.000000000000e+01
    RHS       c3         0.000000000000e+00
BOUNDS
 UP BND       x1         4.000000000000e+01
 LO BND       x4         2.000000000000e+00
 UP BND       x4         3.000000000000e+00
ENDATA
