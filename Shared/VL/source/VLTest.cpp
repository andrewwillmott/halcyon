/*
    File:           VLTest.cpp

    Function:       Regression tests for VLLib.
    
    Author(s):      Andrew Willmott

    Copyright:      (c) 1995-2005, Andrew Willmott

    Notes:          
*/

#undef VL_NO_IOSTREAM

#include <VLfd.h>

#ifdef VL_COMPLEX
   #include <VLc.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <iomanip>

using namespace std;

void CLError(const char *errorMessage, ...)
{
    va_list ap;
    va_start(ap, errorMessage);
    vfprintf(stderr, errorMessage, ap);
    va_end(ap);
    abort();
}

void CLTrace(const char* message, ...)
{
    va_list ap;
    va_start(ap, message);
    vfprintf(stderr, message, ap);
    va_end(ap);
}

#define TYPE_NAME(X) type_name((X*) 0)
  
inline const char* type_name(float*)  { return("float"); };
inline const char* type_name(double*) { return("double"); };
inline const char* type_name(int*)    { return("int"); };


// --- Test routines ----------------------------------------------------------

void TestHStuff2D()
{
    Vec2f x;
    Vec3f y;
    Mat2d M;
    
    cout << "\n+ TestHStuff2D\n\n";
    
    x = Vec2f(1,2);
    cout << "x is: " << x << endl;  
    M = Rot2d(vl_halfPi);
    cout << "rot(pi/2) is: " <<  Rot2d(vl_halfPi) << endl;
    x = Rot2d(vl_halfPi) * x;
    cout << "x after rot(pi/2) is: " << x << endl;
    x = Scale2d(Vec2d(0.3, 0.2)) * x;
    cout << "x after scale(0.3, 0.2) is: " << x << endl;
    
    y = Vec3f(x, 0.5);
    cout << "y is: " << y << endl;
    x = proj(y);
    cout << "proj(y) is: " << x << endl;

    x = proj
        (
              HRot3d(1.3) 
            * HTrans3d(Vec2d(1,1)) 
            * HScale3d(Vec2d(1,2)) 
            * Vec3f(x, 1)
        );
    cout << "HRot3(1.3) * HTrans3(Vec2(1,1)) * HScale3(Vec2(1,2)) * y = "
         << x << endl;
}

void TestHStuff3D()
{
    Vec3f x;
    Vec4f y;
    
    cout << "\n+ TestHStuff3D\n\n";
    
    x = Vec3f(1,2,3);

    cout << "rot(pi/2, vl_x) is: " <<  Rot3d(vl_x, vl_halfPi) << endl;
    x = x * Rot3d(vl_x, vl_halfPi);
    cout << "x after rot(pi/2, vl_x) is: " << x << endl;
    x = x * Scale3d(Vec3d(0.3, 0.2, 0.3));
    cout << "x after scale(0.3, 0.2, 0.3) is: " << x << endl;

    y = Vec4f(x, 0.5);
    cout << "y is: " << y << endl;
    x = proj(y);
    cout << "proj(y) is: " << x << endl;

    x = proj(HRot4d(vl_x, 1.3) * HTrans4d(vl_1) * HScale4d(Vec3d(1,2,1)) * y);
    cout << "HRot4(vl_x, 1.3) * HTrans4(vl_1) "
        "* HScale4(Vec3(1,2,1)) * y = " << x;
}


void Test2DStuff()
{
    Vec2f x(1,2);
    Vec2f y(5,6);

    cout << "\n+ Test2DStuff\n\n";
    
    cout << "x: " << x << ", y: " << y << "\n\n";

    cout << "x + y * (y * x * 2) : " << x + y * (y * x * 2) << endl;
    cout << "x dot y               : " << dot(x, y) << endl;
    
    cout << "cross(x)    : " << cross(x) << endl;
    cout << "len         : " << len(x) << endl;
    cout << "sqrlen      : " << sqrlen(x) << endl;
    cout << "norm        : " << norm(x) << endl;
    cout << "len of norm : " << len(norm(x)) << "\n\n";
    
    Mat2d M(1,2,3,4);
    Mat2d N; N.MakeDiag(2.0);
    Mat2d I = vl_I; 
    
    cout << "M       : " << M << endl;

    cout << "M * x   : " << M * x << endl;
    cout << "x * M   : " << x * M << endl;
    
    cout << "adj     : " << adj(M) << endl;
    cout << "det     : " << det(M) << endl;
    cout << "trace   : " << trace(M) << endl;
    cout << "inv     : \n" << inv(M) << endl;
    cout << "M * inv : \n" << clamped(M * inv(M)) << "\n" << endl;
    
    cout << "Vec2 consts: " << Vec2f(vl_0) << Vec2f(vl_x) 
         << Vec2f(vl_y) << Vec2f(vl_1) << endl; 
    cout << "Mat2 consts:\n" << Mat2d(vl_Z) << endl << Mat2d(vl_I)
         << endl << Mat2d(vl_B) << "\n\n";
    
    M = Rot2d(1.3) * Scale2d(Vec2d(2,1));
    
    cout << "M       : \n" << M << endl;

    cout << "M * x   : " << M * x << endl;
    cout << "x * M   : " << x * M << endl;
    
    cout << "adj     : " << adj(M) << endl;
    cout << "det     : " << det(M) << endl;
    cout << "trace   : " << trace(M) << endl;
    cout << "inv     : \n" << inv(M) << endl;
    cout << "M * inv : \n" << clamped(M * inv(M)) << "\n" << endl;
}

void Test3DStuff()
{
    Vec3f x(1,2,3);
    Vec3f y(5,6,7);

    cout << "\n+ Test3DStuff\n\n";
    
    cout << "x: " << x << ", y: " << y << "\n\n";

    cout << "x + y * (y * x * 2) : " << x + y * (y * x * 2) << endl;
    cout << "x dot y               : " << dot(x, y) << endl;
    
    cout << "cross(x,y)  : " << cross(x,y) << endl;
    cout << "cross(x, y) . x : " << dot(cross(x, y), x) << endl;
    cout << "cross(x, y) . y : " << dot(cross(x, y), y) << endl;
    cout << "len         : " << len(x) << endl;
    cout << "sqrlen      : " << sqrlen(x) << endl;
    cout << "norm        : " << norm(x) << endl;
    cout << "len of norm : " << len(norm(x)) << "\n\n";
    
    Mat3d M(1,2,3,3,2,1,2,1,3);
    Mat3d N; N.MakeDiag(2.0);
    Mat3d I = vl_I; 
    
    cout << "M       : \n" << M << endl;

    cout << "M * x   : " << M * x << endl;
    cout << "x * M   : " << x * M << endl;
    
    cout << "adj     : " << adj(M) << endl;
    cout << "det     : " << det(M) << endl;
    cout << "trace   : " << trace(M) << endl;
    cout << "inv     : \n" << inv(M) << endl;
    cout << "M * inv : \n" << clamped(M * inv(M)) << endl;

    cout << "Vec3 consts: " << Vec3f(vl_0) << Vec3f(vl_x) 
         << Vec3f(vl_y) << Vec3f(vl_z) << Vec3f(vl_1) << endl;  
    cout << "Mat3 consts:\n" << Mat3d(vl_Z) << endl << Mat3d(vl_I) 
         << endl << Mat3d(vl_B) << "\n\n";
    
    M = Rot3d(vl_y, 1.3) * Scale3d(Vec3d(2,4,2));
    
    cout << "M       :\n" << M << endl;

    cout << "M * x   : " << M * x << endl;
    cout << "x * M   : " << x * M << endl;
    
    cout << "adj     : " << adj(M) << endl;
    cout << "det     : " << det(M) << endl;
    cout << "trace   : " << trace(M) << endl;
    cout << "inv     : \n" << inv(M) << endl;
    cout << "M * inv : \n" << clamped(M * inv(M)) << endl;
}

void Test4DStuff()
{
    Vec4f x(1,2,3,4);
    Vec4f y(5,6,7,8);
    Vec4f z(1,0,0,0);

    cout << "\n+ Test4DStuff\n\n";
    
    cout << "x: " << x << ", y: " << y << ", z: " << z << "\n\n";

    cout << "x + y * (z * x * 2) : " << x + y * (z * x * 2) << endl;
    cout << "x dot y               : " << dot(x, y) << "\n\n";
    
    cout << "cross(x,y,z)   : " << cross(x,y,z) << endl;
    cout << "cross(x,y,z).x : " << dot(cross(x,y,z), x) << endl;
    cout << "cross(x,y,z).y : " << dot(cross(x,y,z), y) << endl;
    cout << "cross(x,y,z).z : " << dot(cross(x,y,z), z) << endl;
    cout << "len            : " << len(x) << endl;
    cout << "sqrlen         : " << sqrlen(x) << endl;
    cout << "norm           : " << norm(x) << endl;
    cout << "len of norm    : " << len(norm(x)) << "\n\n";
    
    
    Mat4d M(1,2,3,0, 2,3,0,5, 3,0,5,6, 0,5,6,7);
    Mat4d N; N.MakeBlock(2.0);
    Mat4d I = vl_I; 
    
    cout << "M       : \n" << M << endl;

    cout << "M * x   : " << M * x << endl;
    cout << "x * M   : " << x * M << endl;
    
    cout << "adj     : " << adj(M) << endl;
    cout << "det     : " << det(M) << endl;
    cout << "trace   : " << trace(M) << endl;
    cout << "inv     : \n" << inv(M) << endl;
    cout << "M * inv : \n" << clamped(M * inv(M)) << endl;
    
    cout << "Vec4 consts: " << Vec4f(vl_0) << Vec4f(vl_x) << Vec4f(vl_y)
         << Vec4f(vl_z) << Vec4f(vl_w) << Vec4f(vl_1) << endl;  
    cout << "Mat4 consts:\n" << Mat4d(vl_Z) << endl << Mat4d(vl_I) << endl
         << Mat4d(vl_B) << "\n\n";
    
    M = HScale4d(Vec3d(2,3,4));
    M *= HRot4d(vl_y, 1.256);
    
    cout << "M       : " << M << endl;

    cout << "M * x   : " << M * x << endl;
    cout << "x * M   : " << x * M << endl;
    
    cout << "adj     : " << adj(M) << endl;
    cout << "det     : " << det(M) << endl;
    cout << "trace   : " << trace(M) << endl;
    cout << "inv     : \n" << inv(M) << endl;
    cout << "M * inv : \n" << clamped(M * inv(M)) << endl;

}

ostream &operator << (ostream &s, const TVec &v);

void TestND()
{
    cout << "\n+ TestND\n\n";

    Vecf x(4, 1.0, 2.0, 3.0, 4.0);
    Vecf sx(3, 1.0, 2.0, 3.0);
    Vecf y(4, 5.0, 6.0, 7.0, 8.0);
    Vecf z(4, 4.0, 3.0, 2.0, 1.0);
    
    Matd M(4, 3, 4.0, 3.0, 2.0, 1.0, 4.0, 3.0, 2.0, 1.0, 5.0,
           2.0, 1.0, 4.0, 3.0, 2.0, 1.0, 5.0);
    Matd N(4, 3, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 
           2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0);

    cout << "M:\n" << M;
    cout << "\nN:\n" << N;
    cout << "\ntrans(N):\n" << trans(N);
    cout << "\nM + N:\n" << M + N;
    cout << "\nM * trans(N):\n" << M * trans(N);
    cout << "x: " << x << ", sx: " << sx << endl;

    z = x + y;
    cout << "z: " << z << endl;
    
    cout << "M * sx   : " << (M * sx) << endl;
    cout << "x * M   : " << x * M << endl;
        
    cout << "x: " << x << ", y: " << y << ", z: " << z << endl;

    cout << "x + y: " << x + y << endl;
    cout << "x * y: " << x * y << endl;
    cout << "x dot y: " << dot(x, y) << endl;
    cout << "x + (y dot z) * x * 2 : " << x + (dot(y, z) * x * 2.0) << endl;
    cout << "x + (y dot z) * x * 2 : " << ((float)2.0 * dot(y, z)) * x << endl;
    cout << "x + (y dot z) * x * 2 : " << x + dot(y, z) * x << endl;
    
    cout << "len : " << len(x) << endl;
    cout << "sqrlen : " << sqrlen(x) << endl;
    cout << "norm : " << norm(x) << endl;
    cout << "len of norm : " << len(norm(x)) << endl;
}

#ifdef OLD
void MatTest()
{
    Matf m(50, 50);
    int i, j;

    for (i = 0; i < m.Rows(); i++)
        for (j = 0; j < m.Cols(); j++)
            m[i][j] = i * 100 + j;

    cout << row(sub(sub(m, 20, 30, 10, 10), 3, 3, 4, 4), 2) << endl;
    row(sub(sub(m, 20, 30, 10, 10), 3, 3, 4, 4), 2) = Vecf(4, vl_1);
    cout << row(sub(sub(m, 20, 30, 10, 10), 3, 3, 4, 4), 2) << endl;

    cout << col(sub(sub(m, 20, 30, 10, 10), 3, 3, 4, 4), 2) << endl;
    col(sub(sub(m, 20, 30, 10, 10), 3, 3, 4, 4), 2) = 2 * Vecf(4, vl_1);
    cout << col(sub(sub(m, 20, 30, 10, 10), 3, 3, 4, 4), 2) << endl;

    cout << diag(sub(sub(m, 20, 30, 10, 10), 3, 3, 4, 4), 2) << endl;
    diag(sub(sub(m, 20, 30, 10, 10), 3, 3, 4, 4), 2) = Vecf(2, vl_1);
    cout << diag(sub(sub(m, 20, 30, 10, 10), 3, 3, 4, 4), 2) << endl;

    cout << last(first(sub(m[0], 20, 30), 6), 2) << endl;
    last(first(sub(m[0], 20, 30), 6), 2) = Vecf(2, vl_1);
    cout << last(first(sub(m[0], 20, 30), 6), 2) << endl;

    cout << m[40] << endl;
    ToWaveletTransformRow(row(m, 40));
    cout << m[40] << endl;
}
#endif

void TestNDSub()
{
    cout << "\n+ TestNDSub\n\n";

    Vecf x(8, 1.0, 2.0, 3.0, 4.0, 1.0, 2.0, 3.0, 4.0);
    Vecf y(16, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 
           1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0);
    Vecf z;
    
    cout << "x: " << x << ", y: " << y << endl;

    cout << "sub(y, 3, 3): " << sub(y, 3, 3) << endl;
    z = sub(x, 2, 6) + sub(y, 10, 6);
    cout << "sub(x, 2, 6) + sub(y, 10, 6): " << z << endl;

    sub(y, 5, 6) = Vecf(6, 88.0, 77.0, 66.0, 55.0, 44.0, 33.0);
    sub(x, 0, 2) = Vecf(2, 22.0, 11.0);
    
    cout << "x: " << x << ", y: " << y << endl;

    z = z + sub(y, 5, 6);
    sub(y, 5, 6) =  sub(y, 5, 6) + z;
     
    cout << "z: " << z << ", y: " << y << endl;

    cout << "\n\n";

    Matd M(10, 10, vl_I);
    Matd N(4, 3, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 
           9.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0);

    cout << "sub(N, 1, 1, 2, 2): " << endl << Matd(sub(N, 1, 1, 2, 2));

    sub(M, 5, 3, 4, 3) = N;
    
    cout << "\nM:\n" << M;

    cout << "\nDiagonals of M: \n\n";
    
    for (int i = 1 - M.Rows(); i < M.Cols(); i++)
        cout << diag(M, i) << endl;

    cout << "\nCol 4 of M: \n" << col(M, 4);
        
    diag(M, 0) = Vecd(10, 3.0, 3.0, 3.0, 3.0, 3.0, 3.0, 3.0, 3.0, 3.0, 3.0);
    diag(M, 1) = Vecd(9, vl_1) * 2.0;
    diag(M, -1) = diag(M, 1) * 3.0;

    cout << "\nM:\n" << M << endl;
}

void TestNDNumerical()
{
    Matd    P(4, 4,
                1.0, 2.0, 3.0, 0.0,
                2.0, 3.0, 0.0, 5.0,
                3.0, 0.0, 5.0, 6.0,
                0.0, 5.0, 6.0, 7.0
            );
    Matd    Q;
    Mat4d   P1(
                1.0, 2.0, 3.0, 0.0,
                2.0, 3.0, 0.0, 5.0,
                3.0, 0.0, 5.0, 6.0,
                0.0, 5.0, 6.0, 7.0
            );
    Mat4d   Q1;
    
    cout << "\n+ TestNDNumerical\n" << endl;

    cout << "P:\n";
    cout << P;
    
    cout << "\ninv(P):" << endl;
    Q = inv(P);
    cout << Q;
        
    cout << "\nP * inv(P):\n";
    cout << clamped(P * Q);

    cout << "\n\nReal:\n";
        
    cout << "P1: " << P1 << endl;
    cout << "\ninv(P1):\n";
    Q1 = inv(P1);
    cout << Q1 << endl;

    cout << "\nP1 * inv(P1): " << endl << clamped(P1 * Q1) << endl;
    cout << "\ninv(P) - inv(P1): " << endl << clamped(inv(P) - inv(P1));
    cout << endl << endl;
    
    // --- SolveOverRelax -----------------------------------------------------
    
    Matd    A(4, 4, 
                29.0,   2.0,  3.0,  0.0,
                 2.0,  29.0,  0.0,  5.0,
                 3.0,   0.0, 29.0,  6.0,
                 0.0,   5.0,  6.0, 29.0);
                 
    Vecf    x;
    Vecf    b;
    Real    error = 1.0;
    
    b.SetSize(4);
    b = A * Vecf(Vec4f(1.0, 2.0, 3.0, 4.0));
    
    x = b;
    cout << "Solving Ax = b with over-relaxation..." << endl;
    cout << "A: \n" << A << endl;
    cout << "b: " << b << endl;
    cout << "start x: " << x << endl;

    error = SolveOverRelax(A, x, b, 1e-8, 1.1);
    
//  cout << "iterations: " << i << endl;
    cout << "x: " << x << endl;
    cout << "Ax - b: " << A * x - b << endl;
    cout << "Returned error: " << error << ", real error: " << sqrlen(A * x - b) << endl;
    cout << endl;
    
    // --- SolveConjGrad ------------------------------------------------------
        
    x = b;
    
    cout << "Solving Ax = b with conjugate-gradient..." << endl;
    cout << "A:\n" << A;
    cout << "b: " << b << endl;
    cout << "start x: " << x << endl;

    int steps = 100;
    error = SolveConjGrad(A, x, b, 1e-8, &steps);
    
    cout << "iterations: " << steps << endl;
    cout << "x: " << x << endl;
    cout << "Ax - b: " << A * x - b<< endl;
    cout << "Returned error: " << error << ", real error: " << 
        sqrlen(A * x - b) << endl;
}

void TestNDFunc()
{
    cout << "\n+ TestND\n\n";

    Vecf x(4, 1.0, 2.0, 3.0, 4.0);
    Vecf sx(3, 1.0, 2.0, 3.0);
    Vecf y(4, 5.0, 6.0, 7.0, 8.0);
    Vecf z(4, 4.0, 3.0, 2.0, 1.0);
    
    Matd M(4, 3, 4.0, 3.0, 2.0, 1.0, 4.0, 3.0, 2.0, 1.0, 5.0,
           2.0, 1.0, 4.0, 3.0, 2.0, 1.0, 5.0);
    Matd N(4, 3, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 
           2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0);

    Matd R;
    Transpose(N, R);
    cout << "\nTranspose(N, R):\n" << R;
    Add(N, M, R);
    cout << "\nAdd(M, N, R):\n" << M + N;
    Multiply(M, trans(N), R);
    cout << "\nMultiply(M, trans(N), R):\n" << R;

    Vecf r, s;
    Multiply(M, sx, r);
    cout << "Multiply(M, sx, r)   : " << r << endl;
    Multiply(x, M, r);
    cout << "Multiply(x, M, r)   : " << r << endl;
        
    Add(x, y, r);
    cout << "x + y: " << r << endl;
    Multiply(x, y, r);
    cout << "x * y: " << r << endl;
    Multiply(x, 2 * dot(y, z), r);
    Add(r, x, s);
    cout << "Multiply(x, 2 * dot(y, z), r), Add(r, x, s) : " << s << endl;
    Multiply(x, 2 * dot(y, z), s);
    s += x;
    cout << "Multiply(x, 2 * dot(y, z), r), Add(r, x, s) : " << s << endl;
}


void TestSparse()
{
    cout << "\n+ TestSparse\n" << endl;

    SparseVecf  v(30);
    SparseVecf  z(30);
    SparseVecd  w(Vecd(6, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0));
    Vecf        x(30);
    Vecf        y(30);
    int         i;
    int         i1[] = { 3, 4, 7, 19, VL_SV_END };
    Real        e1[] = { 4.0, 5.0, 6.0, 7.0 };

    for (i = 0; i < 30; i++)
    {
        x[i] =  1.0;
        y[i] =  2.0;
    }

    v.Begin();
    v.AddElt(1, 3);
    v.AddElt(4, 2);
    v.AddElt(11, 3);
    v.AddElt(20, 2);
    v.End();
    
    cout << v << endl; 
    z = v;

    v.SetElts(1, 4.0, 4, 0.0, 20, 3.0, 14, 6.0, VL_SV_END);

    cout << v << endl; 
    
    v.Begin();
    for (i = 0; i < 100; i++)
        v.AddElt(i, i);
    v.End();
    
    v.SetCompactPrint(true);
    cout << "compact: " << v << endl;
    v.SetCompactPrint(false);
            
    cout << "v: " << v << endl; 
    cout << "w: " << w << endl;
    cout << "x: " << x << endl;
    cout << "y: " << y << endl;
    cout << "z: " << z << endl;
    
    v = 2 * v + SparseVecf(x) - z;
        
    cout << "2 * v + x - z: " << v << endl;
    cout << "v * x: " << v * SparseVecf(x) << endl;
        
    SparseMatd M(20, 20, vl_I);
    SparseMatd N(20, 20, vl_I);
    
    w.SetSize(20);
    w.SetElts(5, 3.96, 10, 2.0, 15, 8.0, 19, 2.0, VL_SV_END);
    
    M[10] = M[10].Overlay(w);
    M[15].Set(5, 6.0);

    cout << M[15].Get(5) << endl;
    
    cout << SparseVecf(20, 5, 15.0, 9, 21.0, 14, 20.0, VL_SV_END) << endl;
    cout << SparseVecd(20, i1, e1) << endl;

    M[8].Overlay(SparseVecd(20, 5, 15.0, 9, 21.0, 14, 20.0, VL_SV_END));
    cout << M[8] << endl;
    
    cout << "M:\n" << M;

    N = M * M;
    
    cout << "N = M^2:\n" << N;
    cout << "N - M:\n" << N - M;    
}

void TestSparseSub()
{
    cout << "\n+ TestSparseSub\n\n";

    SparseVecf x(Vecf(8, 1.0, 2.0, 3.0, 4.0, 1.0, 2.0, 3.0, 4.0));
    SparseVecf y(Vecf(16, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0,
                      1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0));
    SparseVecf z;
    
    cout << "x: " << x << ", y: " << y << endl;

    cout << "sub(y, 3, 3): " << sub(y, 3, 3) << endl;
    z = sub(x, 2, 6) + sub(y, 10, 6);
    cout << "z = sub(x, 2, 6) + sub(y, 10, 6): " << z << endl;

    sub(y, 5, 6) = SparseVecf(Vecf(6, 88.0, 77.0, 66.0, 55.0, 44.0, 33.0));
    sub(x, 0, 2) = SparseVecf(Vecf(2, 22.0, 11.0));
    
    cout << "x: " << x << ", y: " << y << endl;

    z = z + sub(y, 5, 6);
    sub(y, 5, 6) =  sub(y, 5, 6) + z;
     
    cout << "z: " << z << ", y: " << y << endl;

    cout << "\n\n";

    SparseMatd M(10, 10, vl_I);
    SparseMatd N(Matd(4, 3, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0,
                      9.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0));
    
    cout << "sub(N, 1, 1, 2, 2): " << endl << sub(N, 1, 1, 2, 2);

    sub(M, 5, 3, 4, 3) = N;
    
    cout << "\nM:\n" << M;

    cout << "\nDiagonals of M: \n\n";
    
    int i;
    
    for (i = 1 - M.Rows(); i < M.Cols(); i++)
        cout << diag(M, i) << endl;

    cout << "\nCol 4 of M: \n" << col(M, 4);
        
    diag(M, 0) = SparseVecd(Vecd(10, 3.0, 3.0, 3.0, 3.0, 3.0, 3.0, 3.0, 3.0, 3.0, 3.0)); // XXX want this to be efficient.
    diag(M, 1) = SparseVecd(Vecd(9, vl_1) * 2.0);
    diag(M, -1) = diag(M, 1) * 3.0;

    cout << "\nM:\n" << M << endl;
}

void TestSparseNumerical()
{
#ifdef VL_BROKEN
XXX
    static Real sparseData[] =
    {
        0, 1, 1.0, 2, 2.0, 3, 4.0, VL_SV_END,
        1, 1, 2.0, 2, 3.0, 4, 4.0, VL_SV_END,
        2, 1, 3.0, 2, 4.0, 3, 6.0, VL_SV_END,
        3, 1, 5.0, 2, 6.0, 3, 7.0, VL_SV_END,
        VL_SV_END
    }
    static Real sparseData[] =
    {
        0, 1, 1.0, 2, 2.0, 3, 4.0, VL_SV_END,
        1, 1, 2.0, 2, 3.0, 4, 4.0, VL_SV_END,
        2, 1, 3.0, 2, 4.0, 3, 6.0, VL_SV_END,
        3, 1, 5.0, 2, 6.0, 3, 7.0, VL_SV_END,
        VL_SV_END
    }
#endif

    SparseMatd P = Matd(4, 4, 1.0, 2.0, 3.0, 0.0,
                2.0, 3.0, 0.0, 5.0,
                3.0, 0.0, 5.0, 6.0,
                0.0, 5.0, 6.0, 7.0);
    SparseMatd Q;
    

    cout << "\n+ TestSparseNumerical\n" << endl;

    cout << "P:\n";
    cout << P;
    
    cout << "\ninv(P):" << endl;
    Q = inv(P);
    cout << Q;
    
    cout << "\nP * inv(P):\n";
    cout << P * Q;

    cout << "\n\nReal:\n";
    
    Mat4d P1(1,2,3,0, 2,3,0,5, 3,0,5,6, 0,5,6,7);
    Matd Q1;
    
    cout << "P1: " << P1 << endl;
    
    cout << "\ninv(P1):\n";
    Q1 = inv(Matd(P1));
    cout << Q1 << endl;
    
    cout << "\nP1 * inv(P1): " << endl << clamped(P1 * Q1) << endl;
    
    
    cout << "\ninv(P) - inv(P1): " << endl << setprecision(4) 
         << Q - SparseMatd(Q1);
    
    cout << endl << 
    "---------------------------------------------------------" << endl;

    // --- SolveOverRelax -----------------------------------------------------
    
    SparseMatd A = Matd(4, 4, 
                29.0,  2.0,  3.0,  0.0,
                 2.0, 29.0,  0.0,  5.0,
                 3.0,  0.0, 29.0,  6.0,
                 0.0,  5.0,  6.0, 29.0);
                 
    Vecf x, b = A * Vecf(4, 1.0, 2.0, 3.0, 4.0);
    Real error = 1;
    
    x = b;
    
    cout << "Solving Ax = b with over-relaxation..." << endl;
    cout << "A: \n" << A << endl;
    cout << "b: " << b << endl;
    
    error = SolveOverRelax(A, x, b, 1e-8, 1.1);
    
    cout << "x: " << x << endl;
    cout << "Ax: " << A * x << endl;
    cout << "Ax - b: " << A * x - b << endl;
    cout << "Returned error: " << error << ", real error: " 
         << sqrlen(A * x - b) << endl;
    cout << endl;
    
    // --- SolveConjGrad ------------------------------------------------------
    
    x = b;
    
    cout << "A:\n" << A;
    cout << "b: " << b << endl;
    cout << "x: " << x << endl;

    cout << "Solving Ax = b with conjugate-gradient..." << endl;

    int steps = 100;
    error = SolveConjGrad(A, x, b, 1e-8, &steps);
    
    cout << "iterations: " << steps << endl;
    cout << "x: " << x << endl;
    cout << "Ax - b: " << A * x - b<< endl;
    cout << "Returned error: " << error << ", real error: " 
         << sqrlen(A * x - b) << endl << endl;

    steps = 100;

    SparseMatd At = trans(A);
    SparseMatd AtA = At * A;
    Vecf Atb = At * b;
    x = Atb;
    
    cout << "AtA:\n" << AtA;
    cout << "Atb: " << Atb << endl;
    cout << "x: " << x << endl;

    cout << "Solving AtAx = Atb with conjugate-gradient..." << endl;

    steps = 100;
    error = SolveConjGrad(AtA, x, Atb, 1e-8, &steps);
    
    cout << "iterations: " << steps << endl;
    cout << "x: " << x << endl;
    cout << "Ax - b: " << AtA * x - Atb << endl;
    cout << "Returned error: " << error << ", real error: " 
         << sqrlen(AtA * x - Atb) << endl << endl;

    steps = 100;
    x = Atb;
    
    cout << "A:\n" << A;
    cout << "b: " << b << endl;
    cout << "x: " << x << endl;
    cout << "Solving AtAx = Atb with conjugate-gradient spec..." << endl;
    error = SolveConjGrad_AtA(A, x, b, 1e-8, &steps);

    cout << "iterations: " << steps << endl;
    cout << "x: " << x << endl;
    cout << "Ax - b: " << AtA * x - At * b << endl;
    cout << "Returned error: " << error << ", real error: " 
         << sqrlen(AtA * x - At * b) << endl << endl;

    SparseMatd mc(10, 10);
    mc[0].Set(2, 1.0f);
}


void TestBasics()
{
    cout << "\n+ TestBasics\n\n";

    CL_ASSERT(1 + 1 == 3);
    CL_EXPECT(1 == 2);
    CL_RANGE(11, 0, 10);
}

void TestInit()
{
    cout << "\n+ TestInit\n" << endl;
    
    Vecf    v00(10, vl_zero);
    Vecf    v01(10, vl_one);
    Vecf    v02(10, vl_x);
    Vecf    v03(10, vl_axis(5));
    Vecf    v04(10); v04.MakeBlock(5.0);

    cout << v00 << endl;
    cout << v01 << endl;
    cout << v02 << endl;
    cout << v03 << endl;
    cout << v04 << endl;    
    cout << endl;

    Matd    m00(5, 5, vl_zero);
    Matd    m01(5, 5, vl_one);
    Matd    m02(5, 5); m02.MakeDiag(4.0);
    Matd    m03(5, 5); m03.MakeBlock(2.2);
    Matd    m04(5, 5); m04.MakeBlock(8.8);
    
    sub(m04, 2, 2, 2, 2) = Mat2d(vl_B) * 111.0; 
    
    cout << m00 << endl;
    cout << m01 << endl;
    cout << m02 << endl;
    cout << m03 << endl;
    cout << m04 << endl;
    cout << endl;
}
    
void TestSparseInit()
{
    cout << "\n+ TestSparseInit\n" << endl;
    
    SparseVecf  sv00(10, vl_zero);
//  SparseVecf  sv01(10, vl_one);  illegal (would be ... suboptimal)
    SparseVecf  sv02(10, vl_x);
    SparseVecf  sv03(10, vl_axis(5));

    SparseMatd  sm00(5, 5, vl_zero);
    SparseMatd  sm01(5, 5, vl_one);
    SparseMatd  sm02(5, 5); sm02.MakeDiag(4.0);
//  SparseMatd  sm03(5, 5); sm03.MakeBlock(2.2); illegal (ditto)

    vector<SparsePaird> pairs;
    
    for (int j = 0; j < 4; j++)
    {
        for (int i = 0; i < 20; i++)
            pairs.push_back(SparsePaird());
        
        pairs.clear();
    }
    
        
    cout << sv00 << endl;
    cout << sv02 << endl;
    cout << sv03 << endl;
    
    cout << endl;
    
    cout << sm00 << endl;
    cout << sm01 << endl;
    cout << sm02 << endl;
    cout << endl;
}

void TestComparisons()
{
    cout << "\n+ TestComparisons\n" << endl;
    
    cout << (Mat2d(vl_0) == vl_0) << endl;
    cout << (Mat3d(vl_0) == vl_0) << endl;
    cout << (Mat4d(vl_0) == vl_0) << endl;
    cout << (Matd(8, 8, vl_0) == Matd(8, 8, vl_0)) << endl;

    cout << (Mat2d(vl_1) == vl_0) << endl;
    cout << (Mat3d(vl_1) == vl_0) << endl;
    cout << (Mat4d(vl_1) == vl_0) << endl;
    cout << (Matd(8, 8, vl_1) == Matd(8, 8, vl_0)) << endl;
}

#ifdef VL_COMPLEX
void ComplexTest()
{
    cout << "\n+ Test Complex\n\n";

    SparseMatc a(4, 6);
    SparseVecc b(6);

    a.MakeDiag(float_complex(0.5, 0.8) * 2.5);
    cout << "a:\n" << a << endl;    
    b = vl_y;
    cout << "b: " << b << endl;
    
    cout << "a * b = " << a * b << endl;
    
    SparseMatc na(4, 6);
    na.MakeDiag(5.1);
    
    cout << "na:\n" << na << endl;
    cout << "na * b = " <<  na * b << endl;

    a = vl_Z;
    b = vl_0;
    a = vl_I;
    b = vl_x;

    Mat4c c;
    Vec4c d;

    c = vl_1;
    d = vl_y;
    
    cout << "c:\n" << c << endl;    
    cout << "d:" << d << endl;  
    
    cout << "ca * cb = " << c * d  + d << endl;
}
#endif

int main(int, char **)
{
    cout << "Testing VL library, version " << VL_VERSION << endl;
    cout << "Real type: " << TYPE_NAME(Real) << endl;

    cout << "----------------------------------" << endl;

#if 1
    Test2DStuff();
    Test3DStuff();
    Test4DStuff();

    TestHStuff2D();
    TestHStuff3D();

    TestND();
    TestNDSub();
    TestNDNumerical();

    TestSparse();
#endif

    TestSparseSub();
    TestSparseNumerical();

#ifdef VL_COMPLEX
    ComplexTest();  
#endif

#if 1
    TestInit();
    TestSparseInit();
    TestComparisons();
#endif

    cout << "\n\n--- Finished! ---" << endl;

    return(0);
}
