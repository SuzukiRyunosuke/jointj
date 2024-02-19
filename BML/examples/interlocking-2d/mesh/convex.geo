// Gmsh project created on Tue Dec 26 20:11:42 2023
SetFactory("OpenCASCADE");
//+
Rectangle(1) = {-0.25, -0.599, 0, 0.5, 0.599, 0};
//+
Rectangle(2) = {-0.5, 0, 0, 1, 1.4, 0};
//+
BooleanUnion{ Surface{1}; Delete; }{ Surface{2}; Delete; }
//+
Circle(14) = {0.45, 0.05, 0, 0.05, -1/2*Pi, 0};
//+
Recursive Delete {
  Curve{14}; 
}
//+
Circle(14) = {0.45, 0.05, 0, 0.05, -1/2*Pi, 0};
//+
Line(15) = {13, 6};
//+
Line(16) = {14, 6};
//+
Curve Loop(1) = {14, 16, -15};
//+
Surface(2) = {1};
//+
BooleanDifference{ Surface{1}; Delete; }{ Surface{2}; Delete; }
//+
Circle(10) = {0.2, -0.549, 0, 0.05, -1/2*Pi, 0};
//+
Circle(11) = {-0.2, -0.549, 0, 0.05, Pi, 3/2*Pi};
//+
Circle(12) = {-0.45, 0.05, 0, 0.05, Pi, 3/2*Pi};
//+
Circle(13) = {-0.3, -0.05, 0, 0.05, 0*Pi, 1/2*Pi};
//+
Circle(14) = {0.3, -0.05, 0, 0.05, 1/2*Pi, 2/2*Pi};
//+
Line(15) = {14, 3};
//+
Line(16) = {3, 15};
//+
Line(17) = {17, 1};
//+
Line(18) = {1, 16};
//+
Line(19) = {12, 2};
//+
Line(20) = {2, 13};
//+
Line(21) = {10, 9};
//+
Line(22) = {9, 11};
//+
Line(23) = {19, 8};
//+
Line(24) = {8, 18};
//+
Curve Loop(2) = {12, -16, -15};
//+
Curve Loop(3) = {13, 17, 18};
//+
Curve Loop(4) = {11, -20, -19};
//+
Curve Loop(5) = {10, -22, -21};
//+
Curve Loop(6) = {14, 23, 24};
//+
Surface(2) = {2, 3, 4, 5, 6};
//+
Curve Loop(7) = {13, 17, 18};
//+
Surface(2) = {7};
//+
Curve Loop(9) = {12, -16, -15};
//+
Surface(3) = {9};
//+
Curve Loop(11) = {11, -20, -19};
//+
Surface(4) = {11};
//+
Curve Loop(13) = {10, -22, -21};
//+
Surface(5) = {13};
//+
Curve Loop(15) = {14, 23, 24};
//+
Surface(6) = {15};
//+
BooleanDifference{ Surface{1}; Delete; }{ Surface{3}; Delete; }
//+
BooleanDifference{ Surface{1}; Delete; }{ Surface{4}; Delete; }
//+
BooleanDifference{ Surface{1}; Delete; }{ Surface{5}; Delete; }
//+
BooleanUnion{ Surface{1}; Delete; }{ Surface{2}; Delete; }
//+
BooleanUnion{ Surface{1}; Delete; }{ Surface{2}; Delete; }
//+
BooleanUnion{ Surface{1}; Delete; }{ Surface{2}; Delete; }
//+
BooleanUnion{ Surface{1}; Delete; }{ Surface{6}; Delete; }
//+
BooleanUnion{ Surface{2}; Delete; }{ Surface{1}; Delete; }
//+
BooleanUnion{ Surface{6}; Delete; }{ Surface{1}; Delete; }
//+
Curve Loop(17) = {13, 17, 18};
//+
Surface(7) = {17};
//+
Curve Loop(19) = {14, 23, 24};
//+
Surface(8) = {19};
//+
Recursive Delete {
  Surface{6}; 
}
//+
Recursive Delete {
  Surface{8}; 
}
//+
Coherence;
//+
Circle(33) = {0.3, -0.05, 0, 0.05, 1/2*Pi, 1*Pi};
//+
BooleanUnion{ Surface{1}; Delete; }{ Surface{2}; Delete; }
//+
BooleanUnion{ Curve{18}; Curve{13}; Curve{17}; Delete; }{ Surface{2}; Delete; }
//+
Delete {
  Curve{17}; 
}
//+
Delete {
  Curve{17}; 
}
//+
Delete {
  Curve{17}; 
}
//+
Delete {
  Curve{17}; 
}
//+
Delete {
  Curve{18}; 
}
//+
Delete {
  Curve{18}; 
}
//+
Delete {
  Surface{2}; 
}
//+
Delete {
  Surface{1}; 
}
//+
Delete {
  Curve{17}; 
}
//+
Delete {
  Curve{18}; 
}
//+
Delete {
  Point{1}; 
}
//+
Delete {
  Curve{41}; 
}
//+
Delete {
  Curve{42}; 
}
//+
Delete {
  Point{40}; 
}
//+
Line(46) = {31, 41};
//+
Line(47) = {32, 39};
//+
Curve Loop(20) = {36, -13, 37, 38, 39, 40, -47, -33, 46, 43, 44, 45, 34, 35};
//+
Surface(1) = {20};
//+
Rectangle(2) = {-0.5, 0.9, -0, 1, 0.5, 0};
//+
BooleanDifference{ Surface{1}; Delete; }{ Surface{2}; Delete; }
//+
Rectangle(2) = {-0.5, 0.9, -0, 1, 0.5, 0};
//+
BooleanUnion{ Surface{1}; Delete; }{ Surface{2}; Delete; }
