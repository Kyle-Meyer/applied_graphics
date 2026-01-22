#include "common/logging.hpp"
#include "geometry/geometry.hpp"

#include <cmath>

namespace cg
{

void matrix_test_module4()
{
    log_msg("Matrix Test");

    Matrix4x4 I;
    I.set_identity();
    I.log("Identity Matrix");

    // Set a scaling matrix
    Matrix4x4 S;
    S.m00() = 10.0f;
    S.m11() = 10.0f;
    S.m22() = 10.0f;
    S.log("Scaling Matrix");

    // Set a translation matrix
    Matrix4x4 T;
    T.m03() = -5.0f;
    T.m13() = 10.0f;
    T.m23() = 15.0f;
    T.log("Translation Matrix");

    // Set a rotation matrix about z
    Matrix4x4 R;
    R.m00() = std::cos(degrees_to_radians(45.0f));
    R.m11() = R.m00();
    R.m10() = std::sin(degrees_to_radians(45.0f));
    R.m01() = -R.m10();
    R.log("Rotation (z) Matrix");

    // Perform composite
    Matrix4x4 C = T * R * S;
    C.log("Composite TRS");

    // Transform Cartesian point 1,1,1 (homogeneous factor assumed to be 1)
    Point3  P0(1.0f, 1.0f, 1.0f);
    HPoint3 P1 = C * P0;
    log_msg("Transformed Cartesian Point is %f %f %f %f", P1.x, P1.y, P1.z, P1.w);

    // Transform point 1,1,1,1 (homogeneous point)
    HPoint3 P(1.0f, 1.0f, 1.0f, 1.0f);
    P1 = C * P;
    log_msg("Transformed Homogeneous Point is %f %f %f %f", P1.x, P1.y, P1.z, P1.w);

    // Show how the order matters
    Matrix4x4 C2 = S * R * T;
    C2.log("Composite SRT");
    HPoint3 P2 = C2 * P;
    log_msg("Transformed Point is %f %f %f %f", P2.x, P2.y, P2.z, P2.w);

    // Test the local Translate, Rotate, Scale methods. Should match C
    log_msg("\nTest Local Translate, Rotate, Scale");
    log_msg("Should match Composite TRS Above");
    Matrix4x4 M;
    M.translate(-5.0f, 10.0f, 15.0f);
    M.rotate_z(45.f);
    M.scale(10.0f, 10.0f, 10.0f);
    M.log("After TRS: Composite matrix is");

    // New composite matrix
    C.set_identity();
    C.translate(-10.0f, 20.0f, 3.0f);
    C.rotate_x(60.f); // 60 degrees about x axis
    C.scale(10.0f, 10.0f, 10.0f);
    C.log("New Composite TRS");

    // Transform point 1,1,0
    Point3 P3(1.0f, 1.0f, 0.0f);
    P1 = C * P3;
    log_msg("Transformed Point is %f %f %f %f", P1.x, P1.y, P1.z, P1.w);

    // Transform the vector (1, 0, 0)
    Vector3 normal(1.0f, 0.0f, 0.0f);
    Vector3 transformed_normal = C * normal;
    log_msg("Transformed Normal is %f %f %f",
            transformed_normal.x,
            transformed_normal.y,
            transformed_normal.z);

    // Get the transpose of C (this method leaves the contents of C unchanged)
    Matrix4x4 D = C.get_transpose();
    D.log("D = Transpose of C");

    // Transpose C
    C.transpose();
    C.log("Transpose of C");

    // Get the inverse of C
    Matrix4x4 CInverse = C.get_inverse();
    CInverse.log("Inverse of C");

    // Verify the Inverse
    Matrix4x4 VerifyInverse = C * CInverse;
    VerifyInverse.log("Matrix times its Inverse (Should be Identity Matrix)");

    // Test the ability to get the matrix and set it
    log_msg("Verify Set and Get Methods: Result should match rotation matrix");
    const float *mat = R.get();
    R.set(mat);
    R.log("Rotation matrix after Get and Set");

    Matrix4x4 test;
    test.rotate_x(45.0f);
    test.rotate_y(60.0f);
    test.scale(10.0f, 10.0f, 10.0f);
    test.translate(50.0f, -5.0f, 10.0f);
    test.rotate_y(-60.0f);
    test.rotate_x(-45.0f);
    test.log("Composite2:");

    // Test rotation about a general axis
    Matrix4x4 general;
    general.rotate(45.0f, 1.0f, 1.0f, 0.0f);
    general.log("Rotation about a general axis");

    // Transform a ray
    Ray3 ray1(Point3(5.0f, 0.0f, 0.0f), Vector3(1.0f, 0.0f, 0.0f));
    Ray3 tr = R * ray1;
    log_msg("Transformed Ray Origin is %f %f %f  Ray Direction = %f %f %f",
            tr.o.x,
            tr.o.y,
            tr.o.z,
            tr.d.x,
            tr.d.y,
            tr.d.z);
}

} // namespace cg
