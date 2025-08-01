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
Circle(205) = { length_x2/2 - radius2, radius2, 0, radius2, -3/6 * Pi, -2/6 * Pi};
Circle(206) = { length_x2/2 - radius2, radius2, 0, radius2, -2/6 * Pi, -1/6 * Pi};
Circle(207) = { length_x2/2 - radius2, radius2, 0, radius2, -1/6 * Pi, 0};

Circle(208) = {-length_x2/2 + radius2, radius2, 0, radius2, -Pi, -5/6 * Pi};
Circle(209) = {-length_x2/2 + radius2, radius2, 0, radius2, -5/6 * Pi, -4/6 * Pi};
Circle(210) = {-length_x2/2 + radius2, radius2, 0, radius2, -4/6 * Pi, -3/6 * Pi};

Curve Loop(2) = {205, 206, 207, 201, 202, 203, 208, 209, 210, 204}; // convex のループ
Surface(2) = {2};               // convex のサーフェス

// サーフェスを物理的エンティティとして定義
Physical Surface("convex") = {2};