SetFactory("OpenCASCADE");

// convexの寸法
length_x2 = 0.5;
length_y2 = 1;
radius2 = 0.05;

//原点＝真ん中

// convex 境界上の点（右下から反時計回り）
Point(201) = { length_x2/2 - radius2, 0, 0}; 
Point(202) = { length_x2/2, radius2, 0};
Point(203) = { length_x2/2, length_y2, 0};
Point(204) = {-length_x2/2, length_y2, 0};
Point(205) = {-length_x2/2, radius2, 0};
Point(206) = {-length_x2/2 + radius2, 0, 0}; 

// convex 直線部分
Line(201) = {202, 203};
Line(202) = {203, 204};
Line(203) = {204, 205};
Line(204) = {206, 201};

// convex 曲線部分
Circle(205) = { length_x2/2 - radius2, radius2, 0, radius2, -6/12 * Pi, -5/12 * Pi};
Circle(206) = { length_x2/2 - radius2, radius2, 0, radius2, -5/12 * Pi, -4/12 * Pi};
Circle(207) = { length_x2/2 - radius2, radius2, 0, radius2, -4/12 * Pi, -3/12 * Pi};
Circle(208) = { length_x2/2 - radius2, radius2, 0, radius2, -3/12 * Pi, -2/12 * Pi};
Circle(209) = { length_x2/2 - radius2, radius2, 0, radius2, -2/12 * Pi, -1/12 * Pi};
Circle(210) = { length_x2/2 - radius2, radius2, 0, radius2, -1/12 * Pi, 0};

Circle(211) = {-length_x2/2 + radius2, radius2, 0, radius2, -12/12 * Pi, -11/12 * Pi};
Circle(212) = {-length_x2/2 + radius2, radius2, 0, radius2, -11/12 * Pi, -10/12 * Pi};
Circle(213) = {-length_x2/2 + radius2, radius2, 0, radius2, -10/12 * Pi, -9/12 * Pi};
Circle(214) = {-length_x2/2 + radius2, radius2, 0, radius2, -9/12 * Pi, -8/12 * Pi};
Circle(215) = {-length_x2/2 + radius2, radius2, 0, radius2, -8/12 * Pi, -7/12 * Pi};
Circle(216) = {-length_x2/2 + radius2, radius2, 0, radius2, -7/12 * Pi, -6/12 * Pi};

Curve Loop(2) = {205, 206, 207, 208, 209, 210, 201, 202, 203, 211, 212, 213, 214, 215, 216, 204}; // convex のループ
Surface(2) = {2};               // convex のsurface

// サーフェスを物理的エンティティとして定義
Physical Surface("convex") = {2};