#include "sphere.h"

#include <algorithm>
#include <cmath>

using namespace std;

Sphere::Sphere(Point const &pos, double radius, Vector const &axis, double angle)
:   position(pos),
    r(radius),
    axis(axis.normalized()),
    angle(angle / 180.0 * PI)
{}

/**
 * Determines the intersection point, if any, of the sphere and a ray. The hit
 * point is guaranteed to be in the positive direction of the ray
 * (t-value >= 0).
 * @return The hit point, wrapped in a std::optional. Could be std::nullopt in
 * case the ray does not intersect the sphere.
 */
optional<Hit> Sphere::intersect(Ray const &ray) {
    // 2.1: Sphere intersection
    Vector OC = ray.O - position;

    double a = ray.D.dot(ray.D);
    double b = 2.0 * OC.dot(ray.D);
    double c = OC.dot(OC) - r * r;

    double t0, t1;
    if (!quadratic(a, b, c, t0, t1))
        return nullopt;

    if (t0 > t1)
        std::swap(t0, t1);

    double t = t0;
    if (t < 0.0)
        t = t1;
    if (t < 0.0)
        return nullopt;

    // 2.2.1: Normal calculation
    Vector N(0, 0, 0);

    return Hit(t, N, shared_from_this());
}

/**
 * Computes the quadratic formula. Values are returned as follows:
 *
 * - No solution: returns false, t0 and t1 unchanged
 * - One solution: returns true, t0 and t1 both contain the solution
 * - Two solutions: returns true, t0 and t1 contain the solutions
 */
bool Sphere::quadratic(double a, double b, double c, double &t0, double &t1) {
    // 2.1: Sphere intersection
    double discriminant = b * b - 4.0 * a * c;
    if (discriminant < 0.0)
        return false;

    double sqrtDiscriminant = std::sqrt(discriminant);
    t0 = (-b - sqrtDiscriminant) / (2.0 * a);
    t1 = (-b + sqrtDiscriminant) / (2.0 * a);
    return true;
}
