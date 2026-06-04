#include "volume.h"

#include "segment.h"

#include <algorithm>
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
  double tx1 = (aa.x - ray.O.x) / ray.D.x;
  double tx2 = (bb.x - ray.O.x) / ray.D.x;

  double ty1 = (aa.y - ray.O.y) / ray.D.y;
  double ty2 = (bb.y - ray.O.y) / ray.D.y;

  double tz1 = (aa.z - ray.O.z) / ray.D.z;
  double tz2 = (bb.z - ray.O.z) / ray.D.z;

  double txNear = std::min(tx1, tx2);
  double txFar = std::max(tx1, tx2);
  double tyNear = std::min(ty1, ty2);
  double tyFar = std::max(ty1, ty2);
  double tzNear = std::min(tz1, tz2);
  double tzFar = std::max(tz1, tz2);

  double tNear = std::max({txNear, tyNear, tzNear});
  double tFar = std::min({txFar, tyFar, tzFar});

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
  double opacity = 0.005;
  return {Color(1.0, 1.0, 1.0) * opacity,
          opacity}; // Use a pre-multiplied color.
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
  return sample(point, doTrilinear).color;
}
