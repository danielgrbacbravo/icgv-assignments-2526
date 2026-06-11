#include "scene.h"

#include "image.h"
#include "objects/hit.h"
#include "objects/object.h"
#include "ray.h"
#include "volumes/volume.h"

#include <algorithm>
#include <cmath>
#include <optional>

using namespace std;

// --- Public functions --------------------------------------------------------

void Scene::render(Image &img) const {
  unsigned w = img.width;
  unsigned h = img.height;

#pragma omp parallel for
  for (unsigned y = 0; y < h; ++y) {
    for (unsigned x = 0; x < w; ++x) {
      img(x, y) = shadePixel(x, h - 1 - y, w, h).clamp();
    }
  }
}

// --- Private functions -------------------------------------------------------

Color Scene::shadePixel(unsigned px, unsigned py, unsigned w,
                        unsigned h) const {
  Color color(0.0, 0.0, 0.0);

  // Set up camera
  Vector forward = (lookAt - eye).normalized();
  Vector right = forward.cross(Vector(0.0, 1.0, 0.0)).normalized();
  if (std::fabs(forward.y) == 1) // Fix right if looking straight up/down
    right = Vector(0.0, 0.0, 1.0);
  Vector up = right.cross(forward);
  double halfH = std::tan(fieldOfView * M_PI / 360.0);
  double halfW = halfH * static_cast<double>(w) / h;

  for (unsigned sx = 0; sx < supersamplingFactor; ++sx) {
    for (unsigned sy = 0; sy < supersamplingFactor; ++sy) {
      double dx = (sx + 0.5) / supersamplingFactor;
      double dy = (sy + 0.5) / supersamplingFactor;

      double u = ((px + dx) / w - 0.5) * 2.0 * halfW;
      double v = ((py + dy) / h - 0.5) * 2.0 * halfH;
      Vector dir = (forward + u * right + v * up).normalized();
      Ray ray(eye, dir);

      color += trace(ray);
    }
  }

  return color / (supersamplingFactor * supersamplingFactor);
}

/**
 * Traces a ray through the scene by testing for intersection with objects and
 * later volumes.
 *
 * @param ray The ray to determine the resulting color of.
 * @return The color of the ray, in the range [0, 1].
 */
Color Scene::trace(Ray const &ray) const {
  Color color(0.0, 0.0, 0.0);
  double accumulated_opacity = 0.0;

  optional<Hit> const objectHit = intersectObjects(ray);
  vector<Segment> const volumeHits = intersectVolumes(ray);

  for (Segment segment : volumeHits) {
    // Skip segments behind or at the object hit point
    if (objectHit && segment.t1 >= objectHit->t)
      break;

    // Trim the segment to stop at the object hit point
    if (objectHit)
      segment.cap(objectHit->t);
    if (segment.length() <= 0.0)
      continue;

    auto const [segmentColor, segmentOpacity] = shadeSegment(segment, ray);
    double transmittance = 1.0 - accumulated_opacity;
    color += transmittance * segmentColor;
    accumulated_opacity += transmittance * segmentOpacity;

    if (accumulated_opacity > 0.99)
      break;
  }

  if (objectHit && accumulated_opacity < 1.0)
    color += (1.0 - accumulated_opacity) * shadeHit(objectHit.value(), ray);

  color.clamp();
  return color;
}

/**
 * Determines the color of the object at the hit point using ambient and
 * diffuse shading.
 *
 * Hints: (see triple.h)
 *  - Triple.dot(Vector) dot product
 *  - Vector + Vector    vector sum
 *  - Vector - Vector    vector difference
 *  - Point - Point      yields vector
 *  - Vector.normalize() normalizes vector, returns length
 *  - double * Color     scales each color component (r,g,b)
 *  - Color * Color      ditto
 *
 * @param min_hit The hit closest to the camera. Contains a pointer to the
 * object that was hit.
 */
Color Scene::shadeHit(Hit const &min_hit, Ray const &ray) const {
  Material const &material =
      min_hit.object->material;                   // the hit object's material
  [[maybe_unused]] Point hit = ray.at(min_hit.t); // the hit point
  [[maybe_unused]] Vector N = min_hit.N;          // the normal at the hit point
  [[maybe_unused]] Vector V = -ray.D;             // the view vector

  // 2.1: Ambient component
  Color color = material.ka * material.color;

  // 2.2.1: Normal calculation
  // Color C = (N + Vector(1.0, 1.0, 1.0)) / 2.0;

  // // 2.2.2: Diffuse component
  // Vector L = (lights[0].position - hit).normalized();
  // double I_D;
  // if (N.dot(L) > 0.0) {
  //   color += material.kd * material.color * lights[0].color * N.dot(L);
  // }

  // 2.3: Multiple lights
  if (!renderShadows) {
    for (Light currentLight : lights) {
      Vector L = (currentLight.position - hit).normalized();
      if (N.dot(L) > 0.0) {
        color += material.kd * material.color * currentLight.color * N.dot(L);
      }
    }
  }

  // 2.4: Shadows
  if (renderShadows) {
    for (Light currentLight : lights) {
      // vector from hit to the light source
      Vector toLight = currentLight.position - hit;
      // then its distance
      double distanceToLight = toLight.length();
      // normalize it for da math
      Vector L = toLight.normalized();

      if (N.dot(L) <= 0.0)
        continue;

      Ray shadowRay = Ray(hit + epsilon * N, L);
      std::optional<Hit> shadowHit = intersectObjects(shadowRay);

      bool lightVisible = !shadowHit || shadowHit->t > distanceToLight;
      if (lightVisible) {
        color += material.kd * material.color * currentLight.color * N.dot(L);
      }
    }
  }

  // 3.6: Sphere and volume integration

  return color;
}

/**
 * Determines the color and opacity of the volume segment using ambient and
 * diffuse shading.
 */
pair<Color, double> Scene::shadeSegment(Segment const &segment,
                                        Ray const &ray) const {
  Color color(0.0, 0.0, 0.0);
  double opacity = 0.0;
  VolumePtr const &volume = segment.volume;
  [[maybe_unused]] DensityField const &volumeData = volume->data;
  [[maybe_unused]] double tStep = tStepFactor * volume->minVoxelSize;
  [[maybe_unused]] Vector V = -ray.D;

  // 3.2: Compositing
  if (segment.length() <= 0.0 || tStep <= 0.0)
    return {color, opacity};

  int const steps = static_cast<int>(std::ceil(segment.length() / tStep));
  if (steps <= 0)
    return {color, opacity};

  for (int i = 0; i < steps; ++i) {
    double const transmittance = 1.0 - opacity;
    if (transmittance <= 0.01) {
      opacity = 1.0;
      break;
    }

    double const t = segment.t1 + i * tStep;
    Sample sample = volume->sample(ray.at(t), volumeTrilinear);
    sample.color = volume->data.ka * sample.color;

    color += transmittance * sample.color;
    opacity += transmittance * sample.opacity;

    if (opacity > 0.99) {
      opacity = 1.0;
      break;
    }
  }
  // 3.4: Diffuse shading

  // 3.5: Shadows

  return {color, opacity};
}

/**
 * Like Scene::trace, but determines only the accumulated opacity of the
 * shadow ray, which is directed towards the light.
 */
double Scene::traceShadowOcclusion(Ray const &shadowRay,
                                   double distanceToLight) const {
  // 3.5: Shadows
  return 0.4;
}

/**
 * Like Scene::shadeSegment, but determines only the opacity of the shadow ray
 * along the volume segment.
 * @param shadowSegment The segment of the volume blocking the shadow ray.
 * Contains a pointer to the volume.
 */
double Scene::shadeSegmentOpacity(Segment const &shadowSegment,
                                  Ray const &shadowRay) const {
  double opacity = 0.0;
  [[maybe_unused]] double tStep =
      tStepFactor * shadowSegment.volume->minVoxelSize;

  // 3.5: Shadows
  opacity = 0.4;

  return opacity;
}

/**
 * Returns all of the ray's intersections with volumes in the scene. All
 * segments have positive t-values. The list is sorted based on distance.
 */
vector<Segment> Scene::intersectVolumes(Ray const &ray) const {
  vector<Segment> intersections;

  for (VolumePtr const &volume : volumes) {
    if (optional<Segment> segment = volume->intersect(ray))
      intersections.emplace_back(segment.value());
  }

  sort(intersections.begin(), intersections.end());
  return intersections;
};

/**
 * If the ray intersects an object in the scene, returns the intersection
 * point with the first hit object. If no intersection exists, returns
 * std::nullopt.
 */
optional<Hit> Scene::intersectObjects(Ray const &ray) const {
  optional<Hit> minHit;
  for (ObjectPtr const &object : objects) {
    optional<Hit> hit = object->intersect(ray);
    if (!hit)
      continue;
    if (!minHit || hit->t < minHit->t)
      minHit = hit;
  }
  return minHit;
}

// --- Misc functions
// ----------------------------------------------------------

// Defaults
Scene::Scene()
    : imageWidth(400), imageHeight(400), fieldOfView(45), tStepFactor(0.5),
      volumeTrilinear(false), renderShadows(false), supersamplingFactor(1) {}

unsigned Scene::getImageWidth() const { return imageWidth; }

unsigned Scene::getImageHeight() const { return imageHeight; }

void Scene::setImageWidth(unsigned width) { imageWidth = width; }

void Scene::setImageHeight(unsigned height) { imageHeight = height; }

void Scene::setEye(Triple const &position) { eye = position; }

void Scene::setLookAt(Triple const &position) { lookAt = position; }

void Scene::setFieldOfView(unsigned fov) { fieldOfView = fov; }

void Scene::setTStepFactor(double factor) { tStepFactor = factor; }

void Scene::setVolumeTrilinear(bool doTrilinear) {
  volumeTrilinear = doTrilinear;
}

void Scene::setRenderShadows(bool doShadows) { renderShadows = doShadows; }

void Scene::setSuperSample(unsigned factor) { supersamplingFactor = factor; }

void Scene::addObject(ObjectPtr const &object) { objects.push_back(object); }

void Scene::addVolume(VolumePtr const &volume) { volumes.push_back(volume); }

void Scene::addLight(Light const &light) { lights.push_back(light); }
