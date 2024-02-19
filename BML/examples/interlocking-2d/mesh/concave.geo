// Gmsh project created on Fri Nov 17 17:23:37 2023
SetFactory("OpenCASCADE");
//+
Rectangle(1) = {-0.5, -1, 0, 1, 1, 0};
//+
Rectangle(2) = {-0.2505, -0.6, 0, 0.501, 0.6, 0};
//+
BooleanDifference{ Surface{1}; Delete; }{ Surface{2}; Delete; }
//+
Circle(9) = {0.2005, -0.55, 0, 0.05, 3/2*Pi, 4/2*Pi};
//+
Circle(10) = {-0.2005, -0.55, 0, 0.05, 2/2*Pi, 3/2*Pi};
//+
Circle(11) = {-0.3005, -0.05, 0, 0.05, 0/2*Pi, 1/2*Pi};
//+
Circle(12) = {0.3005, -0.05, 0, 0.05, 1/2*Pi, 2/2*Pi};
//+
Circle(13) = {0.45, -0.05, 0, 0.05, 0/2*Pi, 1/2*Pi};
//+
Circle(14) = {-0.45, -0.05, 0, 0.05, 1/2*Pi, 2/2*Pi};
//+
Line(15) = {20, 20};
//+
Line(15) = {20, 1};
//+
Line(16) = {1, 1};
//+
Line(16) = {1, 19};
//+
Line(17) = {14, 3};
//+
Line(18) = {3, 13};
//+
Line(19) = {11, 4};
//+
Line(20) = {4, 12};
//+
Line(21) = {9, 5};
//+
Line(22) = {5, 10};
//+
Line(23) = {16, 6};
//+
Line(24) = {6, 15};
//+
Line(25) = {18, 7};
//+
Line(26) = {7, 17};
//+
Curve Loop(1) = {10, -20, -19};
//+
Surface(2) = {1};
//+
Curve Loop(3) = {9, -22, -21};
//+
Surface(3) = {3};
//+
Curve Loop(5) = {12, 23, 24};
//+
Surface(4) = {5};
//+
Curve Loop(7) = {13, 25, 26};
//+
Surface(5) = {7};
//+
Curve Loop(9) = {14, 15, 16};
//+
Surface(6) = {9};
//+
Curve Loop(11) = {11, 17, 18};
//+
Surface(7) = {11};
//+
BooleanDifference{ Surface{1}; Delete; }{ Surface{7}; Delete; }
//+
BooleanDifference{ Surface{1}; Delete; }{ Surface{6}; Delete; }
//+
BooleanDifference{ Surface{1}; Delete; }{ Surface{4}; Delete; }
//+
BooleanDifference{ Surface{1}; Delete; }{ Surface{5}; Delete; }
//+
BooleanUnion{ Surface{1}; Delete; }{ Surface{2}; Delete; }
//+
BooleanUnion{ Surface{2}; Delete; }{ Surface{1}; Delete; }
//+
SetFactory("Built-in");
//+
SetFactory("Built-in");
//+
SetFactory("Built-in");
//+
SetFactory("OpenCASCADE");
//+
SetFactory("Built-in");
//+
SetFactory("OpenCASCADE");
//+
BooleanUnion{ Surface{1}; Delete; }{ Surface{3}; Delete; }
//+
Delete {
  Surface{3}; 
}
//+
Delete {
  Curve{21}; 
}
//+
Delete {
  Curve{21}; 
}
//+
Delete {
  Curve{21}; 
}
//+
Delete {
  Curve{21}; 
}
//+
Delete {
  Surface{1}; 
}
//+
Delete {
  Curve{21}; 
}
//+
Delete {
  Curve{22}; 
}
//+
Delete {
  Point{5}; 
}
//+
Delete {
  Curve{20}; 
}
//+
Delete {
  Surface{2}; 
}
//+
Delete {
  Curve{20}; 
}
//+
Delete {
  Curve{34}; 
}
//+
Delete {
  Curve{19}; 
}
//+
Delete {
  Curve{23}; 
}
//+
Delete {
  Point{4}; 
}
//+
Line(34) = {13, 11};
//+
Line(35) = {12, 9};
//+
Curve Loop(12) = {34, 10, 35, 9, 33, -32, -31, -30, -29, -28, -27, -26, -25, -24};
//+
Surface(1) = {12};
//+
Rectangle(2) = {-0.5, -1.5, 0.1, 1, 0.5, 0};
//+
Physical Surface(1) -= {2};
//+
Physical Surface(1) -= {2};
//+
Physical Surface(1) -= {2};
//+
Physical Surface(1) -= {2};
//+
Physical Surface(1) -= {2};
//+
Physical Surface(1) -= {2};
//+
Physical Surface(1) -= {2};
//+
Physical Surface(1) -= {2};
//+
Physical Surface(1) -= {2};
//+
Physical Surface(1) -= {2};
//+
Physical Surface(1) -= {2};
//+
Physical Surface(1) -= {2};
//+
Physical Surface(1) -= {2};
//+
Physical Surface(1) -= {2};
//+
Physical Surface(1) -= {2};
//+
BooleanUnion{ Surface{1}; Delete; }{ Surface{2}; Delete; }
//+
BooleanUnion{ Surface{2}; Delete; }{ Surface{1}; Delete; }
//+
Physical Surface(1) -= {2};
//+
Physical Surface(1) -= {2};
//+
Physical Surface(1) -= {2};
//+
Physical Surface(1) -= {2};
//+
Physical Surface(1) -= {2};
//+
Physical Surface(1) -= {2};
//+
Physical Surface(1) -= {2};
//+
Physical Surface(1) -= {2};
//+
Physical Surface(1) -= {2};
//+
Physical Surface(1) -= {2};
//+
Physical Surface(1) -= {2};
//+
Physical Surface(1) -= {2};
//+
Physical Surface(1) -= {2};
//+
Physical Surface(1) -= {2};
//+
Physical Surface(1) -= {2};
//+
Physical Curve(40) = {39};
//+
Physical Curve(41) = {37, 38};
//+
Recursive Delete {
  Surface{2}; 
}
//+
Rectangle(2) = {-0.5, -1.5, -0, 1, 0.5, 0};
//+
Recursive Delete {
  Surface{2}; 
}
//+
Rectangle(2) = {-0.5, -1.5, 0, 1, 0.5, 0};
//+
BooleanUnion{ Surface{2}; Delete; }{ Surface{1}; Delete; }
//+
BooleanUnion{ Surface{1}; Delete; }{ Surface{2}; Delete; }
//+
Recursive Delete {
  Surface{2}; 
}
//+
Rectangle(2) = {-0.5, -1.5, -0, 1, 0.5, 0};
//+
Line(22) = {4, 17};
//+
Line(23) = {17, 18};
//+
Line(24) = {18, 3};
//+
Line(25) = {3, 4};
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
  Curve{3}; 
}
//+
Delete {
  Curve{20}; 
}
//+
Delete {
  Curve{25}; 
}
//+
Curve Loop(14) = {15, 14, 13, -12, -11, -10, -9, -24, -18, -22, -8, -7, -6, -5, 17, 16};
//+
Surface(1) = {14};
//+
Physical Surface(42) -= {1};
//+
Physical Surface(42) -= {1};
//+
Physical Surface(42) -= {1};
//+
Delete {
  Surface{1}; 
}
//+
Delete {
  Curve{18}; 
}
//+
Delete {
  Curve{23}; 
}
//+
Line(25) = {17, 18};
//+
Delete {
  Curve{21}; 
}
//+
Delete {
  Curve{22}; 
}
//+
Delete {
  Curve{19}; 
}
//+
Delete {
  Curve{24}; 
}
//+
Line(26) = {4, 17};
//+
Line(27) = {3, 18};
//+
Curve Loop(16) = {15, 14, 13, -12, -11, -10, -9, 27, -25, -26, -8, -7, -6, -5, 17, 16};
//+
Surface(1) = {16};
//+
Delete {
  Surface{1}; 
}
//+
Delete {
  Curve{15}; 
}
//+
Delete {
  Curve{8}; 
}
//+
Delete {
  Curve{9}; 
}
//+
Delete {
  Curve{25}; 
}
//+
Delete {
  Curve{27}; 
}
//+
Delete {
  Curve{26}; 
}
//+
Delete {
  Curve{6}; 
}
//+
Delete {
  Curve{17}; 
}
//+
Delete {
  Curve{13}; 
}
//+
Delete {
  Curve{11}; 
}
//+
Line(17) = {7, 6};
//+
Line(18) = {5, 16};
//+
Line(19) = {15, 14};
//+
Line(20) = {13, 12};
//+
Line(21) = {11, 10};
//+
Line(22) = {9, 18};
//+
Line(23) = {18, 17};
//+
Line(24) = {17, 8};
//+
Curve Loop(18) = {19, 14, 20, -12, 21, -10, 22, 23, 24, -7, 17, -5, 18, 16};
//+
Surface(1) = {18};
//+
Delete {
  Point{4}; 
}
//+
Delete {
  Point{20}; 
}
//+
Delete {
  Point{3}; 
}
//+
Delete {
  Point{19}; 
}
//+
Rectangle(2) = {-0.5, -1.5, -0, 1, 0.5, 0};
//+
BooleanDifference{ Surface{1}; Delete; }{ Surface{2}; Delete; }
