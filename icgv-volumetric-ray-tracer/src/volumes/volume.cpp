#include "volume.h"

#include "segment.h"

#include <algorithm>
#include <limits>
#include <stdexcept>

using namespace std;

Volume::Volume(Point const &aa, Point const &bb, DensityField data,
               TransferFunction tf)
    : aa(aa), bb(bb), width(bb.x - aa.x), height(bb.y - aa.y),
      depth(bb.z - aa.z), data(std::move(data)),
      minVoxelSize(std::min({static_cast<double>(width) / data.width,
                             static_cast<double>(height) / data.height,
                             static_cast<double>(depth) / data.depth})),
      transferFunction(std::move(tf)) {
  if (aa.x > bb.x || aa.y > bb.y || aa.z > bb.z)
    throw runtime_error(
        "Invalid volume bounds, make sure AA <= BB in all dimensions.");
}

/**
 * Determines the intersection segment, if any, of the volume and a ray. The
 * segment is guaranteed to start in the positive direction of the ray
 * (both t-values >= 0).
 * @return The intersection segment, wrapped in a std::optional. Could be
 * std::nullopt in case the ray does not intersect the volume.
 */
optional<Segment> Volume::intersect(Ray const &ray) {
  // 3.1: Intersecting the volume domain
  auto slab = [](double origin, double direction, double minBound,
                 double maxBound) -> optional<pair<double, double>> {
    if (direction == 0.0) {
      if (origin < minBound || origin > maxBound)
        return std::nullopt;

      double const infinity = std::numeric_limits<double>::infinity();
      return pair<double, double>(-infinity, infinity);
    }

    double const t1 = (minBound - origin) / direction;
    double const t2 = (maxBound - origin) / direction;
    return pair<double, double>(std::min(t1, t2), std::max(t1, t2));
  };

  optional<pair<double, double>> tx = slab(ray.O.x, ray.D.x, aa.x, bb.x);
  optional<pair<double, double>> ty = slab(ray.O.y, ray.D.y, aa.y, bb.y);
  optional<pair<double, double>> tz = slab(ray.O.z, ray.D.z, aa.z, bb.z);

  if (!tx || !ty || !tz)
    return std::nullopt;

  double tNear = std::max({tx->first, ty->first, tz->first});
  double tFar = std::min({tx->second, ty->second, tz->second});

  if (tNear > tFar || tFar < 0.0)
    return std::nullopt;

  tNear = std::max(0.0, tNear);

  return Segment(tNear, tFar, shared_from_this());
}

/**
 * Samples the volume to find the color and opacity at a given point.
 * @param doTrilinear Whether to use trilinear instead of nearest-neighbor
 * sampling.
 * @return A Sample, containing the color and opacity of the volume at the
 * given point, according to the transfer function.
 */
Sample Volume::sample(Point const &point, bool doTrilinear) const {
  // 3.3: Sampling the actual data
  double const xPercent = (point.x - aa.x) / (bb.x - aa.x);
  double const yPercent = (point.y - aa.y) / (bb.y - aa.y);
  double const zPercent = (point.z - aa.z) / (bb.z - aa.z);
  double const density =
      data.densityAt(xPercent, yPercent, zPercent, doTrilinear);

  return transferFunction(density);
}

/**
 * Finds the gradient vector at a given point. The gradient is determined using
 * central differences, as written in the assignment text and the slides. Volume
 * bounds should be taken into account, and take care to adjust Δx, Δy and Δz
 * accordingly!
 * @param doTrilinear Whether to use trilinear instead of nearest-neighbor
 * sampling.
 * @return A Vector representing the gradient vector at the given point.
 */
Vector Volume::gradient(Point const &point, bool doTrilinear) const {
  // 3.4: Diffuse shading
  double const xVoxelSize = (bb.x - aa.x) / static_cast<double>(data.width);
  double const yVoxelSize = (bb.y - aa.y) / static_cast<double>(data.height);
  double const zVoxelSize = (bb.z - aa.z) / static_cast<double>(data.depth);

  double const stepLeft = std::min(point.x - aa.x, xVoxelSize);
  double const stepRight = std::min(bb.x - point.x, xVoxelSize);
  double const stepDown = std::min(point.y - aa.y, yVoxelSize);
  double const stepUp = std::min(bb.y - point.y, yVoxelSize);
  double const stepBack = std::min(point.z - aa.z, zVoxelSize);
  double const stepFront = std::min(bb.z - point.z, zVoxelSize);

  double const opacityLeft =
      sample(point - Vector(stepLeft, 0.0, 0.0), doTrilinear).opacity;
  double const opacityRight =
      sample(point + Vector(stepRight, 0.0, 0.0), doTrilinear).opacity;
  double const opacityDown =
      sample(point - Vector(0.0, stepDown, 0.0), doTrilinear).opacity;
  double const opacityUp =
      sample(point + Vector(0.0, stepUp, 0.0), doTrilinear).opacity;
  double const opacityBack =
      sample(point - Vector(0.0, 0.0, stepBack), doTrilinear).opacity;
  double const opacityFront =
      sample(point + Vector(0.0, 0.0, stepFront), doTrilinear).opacity;

  double const xGradient =
      (opacityRight - opacityLeft) / (stepRight + stepLeft);
  double const yGradient = (opacityUp - opacityDown) / (stepUp + stepDown);
  double const zGradient =
      (opacityFront - opacityBack) / (stepFront + stepBack);

  return Vector(xGradient, yGradient, zGradient);
}
