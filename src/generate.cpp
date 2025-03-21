#define CGAL_MESH_3_VERBOSE 1

#include "generate.hpp"

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>

#include <CGAL/Mesh_triangulation_3.h>
#include <CGAL/Mesh_complex_3_in_triangulation_3.h>
#include <CGAL/Mesh_criteria_3.h>

#include <CGAL/Labeled_mesh_domain_3.h>

#include <CGAL/Mesh_domain_with_polyline_features_3.h>
#include <CGAL/make_mesh_3.h>

namespace pygalmesh {

typedef CGAL::Exact_predicates_inexact_constructions_kernel K;

typedef CGAL::Mesh_domain_with_polyline_features_3<CGAL::Labeled_mesh_domain_3<K>> Mesh_domain;

// Triangulation
typedef CGAL::Mesh_triangulation_3<Mesh_domain>::type Tr;
typedef CGAL::Mesh_complex_3_in_triangulation_3<Tr> C3t3;

// Mesh Criteria
typedef CGAL::Mesh_criteria_3<Tr> Mesh_criteria;
typedef Mesh_criteria::Edge_criteria Edge_criteria;
typedef Mesh_criteria::Facet_criteria Facet_criteria;
typedef Mesh_criteria::Cell_criteria Cell_criteria;

namespace {

// convert vector<vector<array<double, 3>> to list<vector<Point_3>>
std::list<std::vector<K::Point_3>>
convert_feature_edges(
    const DomainBase::Features & feature_edges
    )
{
  std::list<std::vector<K::Point_3>> polylines;
  for (const auto & feature_edge: feature_edges) {
    std::vector<K::Point_3> polyline;
    for (const auto & point: feature_edge) {
      polyline.push_back(K::Point_3(point[0], point[1], point[2]));
    }
    polylines.push_back(polyline);
  }
  return polylines;
}

template <typename T>
void
generate_mesh(
    const std::shared_ptr<pygalmesh::DomainBase> & domain,
    const std::string & outfile,
    const T& domain_bounds,
    const DomainBase::Features & extra_feature_edges,
    const bool lloyd,
    const bool odt,
    const bool perturb,
    const bool exude,
    //
    const double min_edge_size_at_feature_edges,
    //
    const double max_edge_size_at_feature_edges_value,
    const std::shared_ptr<pygalmesh::SizingFieldBase> & max_edge_size_at_feature_edges_field,
    //
    const double min_facet_angle,
    //
    const double max_radius_surface_delaunay_ball_value,
    const std::shared_ptr<pygalmesh::SizingFieldBase> & max_radius_surface_delaunay_ball_field,
    //
    const double max_facet_distance_value,
    const std::shared_ptr<pygalmesh::SizingFieldBase> & max_facet_distance_field,
    //
    const double max_circumradius_edge_ratio,
    //
    const double max_cell_circumradius_value,
    const std::shared_ptr<pygalmesh::SizingFieldBase> & max_cell_circumradius_field,
    //
    const double exude_time_limit,
    const double exude_sliver_bound,
    //
    const bool verbose,
    const int seed
    )
{
  CGAL::get_default_random() = CGAL::Random(seed);

  // wrap domain
  const auto d = [&](K::Point_3 p) {
    return domain->eval({p.x(), p.y(), p.z()});
  };

  Mesh_domain cgal_domain =
    Mesh_domain::create_implicit_mesh_domain(d, domain_bounds);

  // cgal_domain.detect_features();

  const auto native_features = convert_feature_edges(domain->get_features());
  cgal_domain.add_features(native_features.begin(), native_features.end());

  const auto polylines = convert_feature_edges(extra_feature_edges);
  cgal_domain.add_features(polylines.begin(), polylines.end());

  // perhaps there's a more elegant solution here
  // see <https://github.com/CGAL/cgal/issues/1286>
  if (!verbose) {
    // suppress output
    std::cerr.setstate(std::ios_base::failbit);
  }

  // Build the float/field values according to
  // <https://github.com/CGAL/cgal/issues/5044#issuecomment-705526982>.

  // nested ternary operator
  const auto facet_criteria = max_radius_surface_delaunay_ball_field ? (
      max_facet_distance_field ?
      Facet_criteria(
        min_facet_angle,
        [&](K::Point_3 p, const int, const Mesh_domain::Index&) {
          return max_radius_surface_delaunay_ball_field->eval({p.x(), p.y(), p.z()});
        },
        [&](K::Point_3 p, const int, const Mesh_domain::Index&) {
          return max_facet_distance_field->eval({p.x(), p.y(), p.z()});
        }
      ) : Facet_criteria(
        min_facet_angle,
        [&](K::Point_3 p, const int, const Mesh_domain::Index&) {
          return max_radius_surface_delaunay_ball_field->eval({p.x(), p.y(), p.z()});
        },
        max_facet_distance_value
      )
    ) : (
      max_facet_distance_field ?
      Facet_criteria(
        min_facet_angle,
        max_radius_surface_delaunay_ball_value,
         [&](K::Point_3 p, const int, const Mesh_domain::Index&) {
           return max_facet_distance_field->eval({p.x(), p.y(), p.z()});
         }
      ) : Facet_criteria(
        min_facet_angle,
        max_radius_surface_delaunay_ball_value,
        max_facet_distance_value
      )
    );

  const auto edge_criteria = max_edge_size_at_feature_edges_field ?
    Edge_criteria(
      [&](K::Point_3 p, const int, const Mesh_domain::Index&) {
        return max_edge_size_at_feature_edges_field->eval({p.x(), p.y(), p.z()});
      },
      min_edge_size_at_feature_edges
    ) : Edge_criteria(
      max_edge_size_at_feature_edges_value,
      min_edge_size_at_feature_edges
    );

  const auto cell_criteria = max_cell_circumradius_field ?
     Cell_criteria(
         max_circumradius_edge_ratio,
         [&](K::Point_3 p, const int, const Mesh_domain::Index&) {
           return max_cell_circumradius_field->eval({p.x(), p.y(), p.z()});
          }) : Cell_criteria(max_circumradius_edge_ratio, max_cell_circumradius_value);

  const auto criteria = Mesh_criteria(edge_criteria, facet_criteria, cell_criteria);

  // Mesh generation
  C3t3 c3t3 = CGAL::make_mesh_3<C3t3>(
      cgal_domain,
      criteria,
      lloyd ? CGAL::parameters::lloyd(CGAL::parameters::default_values()) : CGAL::parameters::no_lloyd(),
      odt ? CGAL::parameters::odt(CGAL::parameters::default_values()) : CGAL::parameters::no_odt(),
      perturb ? CGAL::parameters::perturb() : CGAL::parameters::no_perturb(),
      exude ?
        CGAL::parameters::exude(
          CGAL::parameters::time_limit = exude_time_limit,
          CGAL::parameters::sliver_bound = exude_sliver_bound
        ) :
        CGAL::parameters::no_exude()
      );
  if (!verbose) {
    std::cerr.clear();
  }

  // Output
  std::ofstream medit_file(outfile);
  c3t3.output_to_medit(medit_file);
  medit_file.close();

  return;
}

}

void
generate_mesh(
    const std::shared_ptr<pygalmesh::DomainBase> & domain,
    const std::string & outfile,
    const DomainBase::Features & extra_feature_edges,
    const double bounding_sphere_radius,
    const bool lloyd,
    const bool odt,
    const bool perturb,
    const bool exude,
    //
    const double min_edge_size_at_feature_edges,
    //
    const double max_edge_size_at_feature_edges_value,
    const std::shared_ptr<pygalmesh::SizingFieldBase> & max_edge_size_at_feature_edges_field,
    //
    const double min_facet_angle,
    //
    const double max_radius_surface_delaunay_ball_value,
    const std::shared_ptr<pygalmesh::SizingFieldBase> & max_radius_surface_delaunay_ball_field,
    //
    const double max_facet_distance_value,
    const std::shared_ptr<pygalmesh::SizingFieldBase> & max_facet_distance_field,
    //
    const double max_circumradius_edge_ratio,
    //
    const double max_cell_circumradius_value,
    const std::shared_ptr<pygalmesh::SizingFieldBase> & max_cell_circumradius_field,
    //
    const double exude_time_limit,
    const double exude_sliver_bound,
    //
    const bool verbose,
    const int seed
    )
{
  const double bounding_sphere_radius2 = bounding_sphere_radius > 0 ?
    bounding_sphere_radius*bounding_sphere_radius :
    // some wiggle room
    1.01 * domain->get_bounding_sphere_squared_radius();

  generate_mesh(
    domain, outfile, K::Sphere_3(CGAL::ORIGIN, bounding_sphere_radius2),
    extra_feature_edges, lloyd, odt, perturb, exude,
    min_edge_size_at_feature_edges, max_edge_size_at_feature_edges_value, max_edge_size_at_feature_edges_field,
    min_facet_angle,
    max_radius_surface_delaunay_ball_value, max_radius_surface_delaunay_ball_field,
    max_facet_distance_value, max_facet_distance_field,
    max_circumradius_edge_ratio,
    max_cell_circumradius_value, max_cell_circumradius_field,
    exude_time_limit, exude_sliver_bound,
    verbose, seed);
}

void
generate_mesh(
    const std::shared_ptr<pygalmesh::DomainBase> & domain,
    const std::string & outfile,
    const std::array<double, 6> bounding_cuboid,
    const DomainBase::Features & extra_feature_edges,
    const bool lloyd,
    const bool odt,
    const bool perturb,
    const bool exude,
    //
    const double min_edge_size_at_feature_edges,
    //
    const double max_edge_size_at_feature_edges_value,
    const std::shared_ptr<pygalmesh::SizingFieldBase> & max_edge_size_at_feature_edges_field,
    //
    const double min_facet_angle,
    //
    const double max_radius_surface_delaunay_ball_value,
    const std::shared_ptr<pygalmesh::SizingFieldBase> & max_radius_surface_delaunay_ball_field,
    //
    const double max_facet_distance_value,
    const std::shared_ptr<pygalmesh::SizingFieldBase> & max_facet_distance_field,
    //
    const double max_circumradius_edge_ratio,
    //
    const double max_cell_circumradius_value,
    const std::shared_ptr<pygalmesh::SizingFieldBase> & max_cell_circumradius_field,
    //
    const double exude_time_limit,
    const double exude_sliver_bound,
    //
    const bool verbose,
    const int seed
    )
{
  // some wiggle room
  const double eps = 0.01 * std::max({
    std::abs(bounding_cuboid[3] - bounding_cuboid[0]),
    std::abs(bounding_cuboid[4] - bounding_cuboid[1]),
    std::abs(bounding_cuboid[5] - bounding_cuboid[2])
  });

  K::Iso_cuboid_3 cuboid(
      bounding_cuboid[0] - eps,
      bounding_cuboid[1] - eps,
      bounding_cuboid[2] - eps,
      bounding_cuboid[3] + eps,
      bounding_cuboid[4] + eps,
      bounding_cuboid[5] + eps
      );

  generate_mesh(
    domain, outfile, cuboid, extra_feature_edges, lloyd, odt, perturb, exude,
    min_edge_size_at_feature_edges, max_edge_size_at_feature_edges_value, max_edge_size_at_feature_edges_field,
    min_facet_angle,
    max_radius_surface_delaunay_ball_value, max_radius_surface_delaunay_ball_field,
    max_facet_distance_value, max_facet_distance_field,
    max_circumradius_edge_ratio,
    max_cell_circumradius_value, max_cell_circumradius_field,
    exude_time_limit, exude_sliver_bound,
    verbose, seed);
}

} // namespace pygalmesh
