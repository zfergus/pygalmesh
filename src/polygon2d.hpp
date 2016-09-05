#ifndef POLYGON2D_HPP
#define POLYGON2D_HPP

#include "domain.hpp"

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Polygon_2_algorithms.h>
#include <memory>
#include <vector>

typedef CGAL::Exact_predicates_inexact_constructions_kernel K;

namespace loom {

class Polygon2D {
  public:
  explicit Polygon2D(const std::vector<std::vector<double>> & _points):
    points(vector_to_cgal_points(_points))
  {
  }

  virtual ~Polygon2D() = default;

  std::vector<K::Point_2>
  vector_to_cgal_points(const std::vector<std::vector<double>> & _points) const
  {
    std::vector<K::Point_2> points2(_points.size());
    for (size_t i = 0; i < _points.size(); i++) {
      assert(_points[i].size() == 2);
      points2[i] = K::Point_2(_points[i][0], _points[i][1]);
    }
    return points2;
  }

  bool
  is_inside(const std::vector<double> & point)
  {
    assert(point.size() == 2);
    K::Point_2 pt(point[0], point[1]);
    switch(CGAL::bounded_side_2(this->points.begin(), this->points.end(), pt, K())) {
      case CGAL::ON_BOUNDED_SIDE:
        return true;
      case CGAL::ON_BOUNDARY:
        return true;
      case CGAL::ON_UNBOUNDED_SIDE:
        return false;
      default:
        return false;
    }
    return false;
  }

  public:
  const std::vector<K::Point_2> points;
};


class Extrude: public loom::DomainBase {
  public:
  Extrude(
      const std::shared_ptr<loom::Polygon2D> & poly,
      const std::vector<double> & direction,
      const double alpha = 0.0
      ):
    poly_(poly),
    direction_(direction),
    alpha_(alpha)
  {
    assert(direction_.size() == 3);
  }

  virtual ~Extrude() = default;

  virtual
  double
  eval(const std::vector<double> & x) const
  {
    if (x[2] < 0.0 || x[2] > direction_[2]) {
      return 1.0;
    }

    const double beta = x[2] / direction_[2];

    std::vector<double> x2 = {
      x[0] - beta * direction_[0],
      x[1] - beta * direction_[1]
    };

    if (alpha_ != 0.0) {
      std::vector<double> x3(2);
      // turn by -beta*alpha
      const double sinAlpha = sin(beta*alpha_);
      const double cosAlpha = cos(beta*alpha_);
      x3[0] =  cosAlpha * x2[0] + sinAlpha * x2[1];
      x3[1] = -sinAlpha * x2[0] + cosAlpha * x2[1];
      x2 = x3;
    }

    return poly_->is_inside(x2) ? -1.0 : 1.0;
  }

  virtual
  double
  get_bounding_sphere_squared_radius() const
  {
    double max = 0.0;
    for (const auto & pt: poly_->points) {
      // bottom polygon
      const double nrm0 = pt.x()*pt.x() + pt.y()*pt.y();
      if (nrm0 > max) {
        max = nrm0;
      }

      // TODO rotation

      // top polygon
      const double x = pt.x() + direction_[0];
      const double y = pt.y() + direction_[1];
      const double z = direction_[2];
      const double nrm1 = x*x + y*y + z*z;
      if (nrm1 > max) {
        max = nrm1;
      }
    }
    return max;
  }

  virtual
  std::vector<std::vector<std::vector<double>>>
  get_features() const
  {
    std::vector<std::vector<std::vector<double>>> features = {};

    size_t n;

    // bottom polygon
    n = poly_->points.size();
    for (size_t i=0; i < n-1; i++) {
      features.push_back({
        {poly_->points[i].x(), poly_->points[i].y(), 0.0},
        {poly_->points[i+1].x(), poly_->points[i+1].y(), 0.0}
      });
    }
    features.push_back({
      {poly_->points[n-1].x(), poly_->points[n-1].y(), 0.0},
      {poly_->points[0].x(), poly_->points[0].y(), 0.0}
    });

    // top polygon, R*x + d
    n = poly_->points.size();
    const double sinAlpha = sin(alpha_);
    const double cosAlpha = cos(alpha_);
    for (size_t i=0; i < n-1; i++) {
      features.push_back({
        {
        cosAlpha * poly_->points[i].x() - sinAlpha * poly_->points[i].y() + direction_[0],
        sinAlpha * poly_->points[i].x() + cosAlpha * poly_->points[i].y() + direction_[1],
        direction_[2]
        },
        {
        cosAlpha * poly_->points[i+1].x() - sinAlpha * poly_->points[i+1].y() + direction_[0],
        sinAlpha * poly_->points[i+1].x() + cosAlpha * poly_->points[i+1].y() + direction_[1],
        direction_[2]
        }
      });
    }
    features.push_back({
      {
      cosAlpha * poly_->points[n-1].x() - sinAlpha * poly_->points[n-1].y() + direction_[0],
      sinAlpha * poly_->points[n-1].x() + cosAlpha * poly_->points[n-1].y() + direction_[1],
      direction_[2]
      },
      {
      cosAlpha * poly_->points[0].x() - sinAlpha * poly_->points[0].y() + direction_[0],
      sinAlpha * poly_->points[0].x() + cosAlpha * poly_->points[0].y() + direction_[1],
      direction_[2]
      }
    });

    // // features connecting the top and bottom
    // for (const auto & pt: poly_->points) {
    //   std::vector<std::vector<double>> line = {
    //     {pt.x(), pt.y(), 0.0},
    //     {pt.x() + direction_[0], pt.y() + direction_[1], direction_[2]}
    //   };
    //   features.push_back(line);
    // }

    return features;
  };

  private:
  const std::shared_ptr<loom::Polygon2D> poly_;
  const std::vector<double> direction_;
  const double alpha_;
};

} // namespace loom

#endif // POLYGON2D_HPP