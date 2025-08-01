SetFactory("OpenCASCADE");

// convexの寸法
length_x2 = 0.5;
length_y2 = 1;
radius2 = 0.05;

// concave の寸法
length_x1 = 1.1;
depth_y1 = 0.5;
insert_x = length_x2 + 0.0005;
insert_y = 0.6;
radius1 = 0.05;
offset_y = 0.000;

//原点＝真ん中

// concave 境界上の点（右下から反時計回り）
Point(101) = { length_x1/2, -depth_y1, 0}; 
Point(102) = { length_x1/2, insert_y, 0};
Point(103) = { insert_x/2 + radius1, insert_y, 0};
Point(104) = { insert_x/2, insert_y - radius1, 0};
Point(105) = { insert_x/2, radius1 -offset_y, 0};
Point(106) = { insert_x/2 - radius1, -offset_y, 0}; 
Point(107) = {-insert_x/2 + radius1, -offset_y, 0}; 
Point(108) = {-insert_x/2, radius1 -offset_y, 0};
Point(109) = {-insert_x/2, insert_y - radius1, 0};
Point(110) = {-insert_x/2 - radius1, insert_y, 0};
Point(111) = {-length_x1/2, insert_y, 0};
Point(112) = {-length_x1/2, -depth_y1, 0}; 

// concave 直線部分
Line(101) = {101, 102};
Line(102) = {102, 103};
Line(103) = {104, 105};
Line(104) = {106, 107};
Line(105) = {108, 109};
Line(106) = {110, 111};
Line(107) = {111, 112};
Line(108) = {112, 101};

// concave 曲線部分
Circle(109) = { insert_x/2 + radius1, insert_y - radius1, 0, radius1, 3/6 * Pi, 4/6 * Pi};
Circle(110) = { insert_x/2 + radius1, insert_y - radius1, 0, radius1, 4/6 * Pi, 5/6 * Pi};
Circle(111) = { insert_x/2 + radius1, insert_y - radius1, 0, radius1, 5/6 * Pi, 6/6 * Pi};

Circle(112) = { insert_x/2 - radius1, radius1 - offset_y, 0, radius1, -3/6 * Pi, -2/6 * Pi};
Circle(113) = { insert_x/2 - radius1, radius1 - offset_y, 0, radius1, -2/6 * Pi, -1/6 * Pi};
Circle(114) = { insert_x/2 - radius1, radius1 - offset_y, 0, radius1, -1/6 * Pi, 0};

Circle(115) = {-insert_x/2 + radius1, radius1 - offset_y, 0, radius1, -Pi, -5/6 * Pi};
Circle(116) = {-insert_x/2 + radius1, radius1 - offset_y, 0, radius1, -5/6 * Pi, -4/6 * Pi};
Circle(117) = {-insert_x/2 + radius1, radius1 - offset_y, 0, radius1, -4/6 * Pi, -3/6 * Pi};

Circle(118) = {-insert_x/2 - radius1, insert_y - radius1, 0, radius1, 0, 1/6 * Pi};
Circle(119) = {-insert_x/2 - radius1, insert_y - radius1, 0, radius1, 1/6 * Pi, 2/6 * Pi};
Circle(120) = {-insert_x/2 - radius1, insert_y - radius1, 0, radius1, 2/6 * Pi, 3/6 * Pi};

// カーブループとサーフェスを定義
Curve Loop(1) = {101, 102, 109, 110, 111, 103, -114, -113, -112, 104, -117, -116, -115, 105, 118, 119, 120, 106, 107, 108}; // concave のループ
Surface(1) = {1};               // concave のサーフェス

// サーフェスを物理的エンティティとして定義
Physical Surface("concave") = {1};
