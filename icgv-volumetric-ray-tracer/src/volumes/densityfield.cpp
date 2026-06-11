#include "densityfield.h"

#include "../datraw/datraw.h"

#include <algorithm>
#include <cmath>
#include <filesystem>

using namespace std;
namespace fs = std::filesystem;

/**
 * Reads density field data from the given file.
 */
DensityField::DensityField(std::string const &path, double ka, double kd)
    : ka(ka), kd(kd) {
  using reader = datraw::raw_reader<char>;
  reader r;
  try {
    r = reader::open(path);
  } catch (std::runtime_error const &ex) {
    std::stringstream msg;
    msg << "Failed to read DAT file at " << fs::path(path) << ".\n"
        << "Please check that this full path is correct: "
        << fs::weakly_canonical(fs::current_path() / path) << "\n";
    throw std::runtime_error(msg.str());
  }

  width = r.info().resolution()[0];
  height = r.info().resolution()[1];
  depth = r.info().resolution()[2];
  size_t dataLength = width * height * depth;
  data.resize(dataLength);

  r.read_current(data.data(), dataLength);
}

/**
 * Indexes the 1D data array at the given coordinates. Since the data is
 * stored as std::uint8_t to save space, we use UINT8_MAX to scale to [0, 1].
 */
double DensityField::index(unsigned x, unsigned y, unsigned z) const {
  return static_cast<double>(data[z * width * height + y * width + x]) /
         UINT8_MAX;
}

/**
 * Find the density of the field by rounding to the nearest neighbor data point.
 */
double DensityField::nearestNeighbor(double xPercent, double yPercent,
                                     double zPercent) const {
  // 3.3: Sampling the actual data
  double const xClamped = std::clamp(xPercent, 0.0, 1.0);
  double const yClamped = std::clamp(yPercent, 0.0, 1.0);
  double const zClamped = std::clamp(zPercent, 0.0, 1.0);

  double const xGrid = xClamped * static_cast<double>(width);
  double const yGrid = yClamped * static_cast<double>(height);
  double const zGrid = zClamped * static_cast<double>(depth);

  unsigned ix = static_cast<unsigned>(std::lround(xGrid));
  unsigned iy = static_cast<unsigned>(std::lround(yGrid));
  unsigned iz = static_cast<unsigned>(std::lround(zGrid));

  ix = std::min(ix, width - 1);
  iy = std::min(iy, height - 1);
  iz = std::min(iz, depth - 1);

  return index(ix, iy, iz);
}

/**
 * Find the density of the field by interpolating between the two nearest
 * neighbors on each axis.
 */
double DensityField::trilinear(double xPercent, double yPercent,
                               double zPercent) const {
  // 3.3: Sampling the actual data
  double const xClamped = std::clamp(xPercent, 0.0, 1.0);
  double const yClamped = std::clamp(yPercent, 0.0, 1.0);
  double const zClamped = std::clamp(zPercent, 0.0, 1.0);

  double const xGrid = xClamped * static_cast<double>(width);
  double const yGrid = yClamped * static_cast<double>(height);
  double const zGrid = zClamped * static_cast<double>(depth);

  unsigned x0 = static_cast<unsigned>(std::floor(xGrid));
  unsigned y0 = static_cast<unsigned>(std::floor(yGrid));
  unsigned z0 = static_cast<unsigned>(std::floor(zGrid));

  x0 = std::min(x0, width - 1);
  y0 = std::min(y0, height - 1);
  z0 = std::min(z0, depth - 1);

  unsigned const x1 = std::min(x0 + 1, width - 1);
  unsigned const y1 = std::min(y0 + 1, height - 1);
  unsigned const z1 = std::min(z0 + 1, depth - 1);

  double const tx = xGrid - static_cast<double>(x0);
  double const ty = yGrid - static_cast<double>(y0);
  double const tz = zGrid - static_cast<double>(z0);

  double const d000 = index(x0, y0, z0);
  double const d100 = index(x1, y0, z0);
  double const d010 = index(x0, y1, z0);
  double const d110 = index(x1, y1, z0);
  double const d001 = index(x0, y0, z1);
  double const d101 = index(x1, y0, z1);
  double const d011 = index(x0, y1, z1);
  double const d111 = index(x1, y1, z1);

  double const x00 = (1.0 - tx) * d000 + tx * d100;
  double const x10 = (1.0 - tx) * d010 + tx * d110;
  double const x01 = (1.0 - tx) * d001 + tx * d101;
  double const x11 = (1.0 - tx) * d011 + tx * d111;

  double const yFront = (1.0 - ty) * x00 + ty * x10;
  double const yBack = (1.0 - ty) * x01 + ty * x11;

  return (1.0 - tz) * yFront + tz * yBack;
}

/**
 * Finds the density of the field according to the given interpolation method.
 * The percentage parameters are relative to the boundary on each axis.
 */
double DensityField::densityAt(double xPercent, double yPercent,
                               double zPercent, bool doTrilinear) const {
  if (!doTrilinear)
    return nearestNeighbor(xPercent, yPercent, zPercent);
  return trilinear(xPercent, yPercent, zPercent);
  return nearestNeighbor(xPercent, yPercent, zPercent);
}
