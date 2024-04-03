#define CGAL_MESH_3_VERBOSE 1

#include "generate_periodic.hpp"

#include <CGAL/Periodic_3_mesh_3/config.h>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/make_periodic_3_mesh_3.h>
#include <CGAL/optimize_periodic_3_mesh_3.h>
#include <CGAL/Periodic_3_mesh_3/IO/File_medit.h>
#include <CGAL/Periodic_3_mesh_triangulation_3.h>
#include <CGAL/Labeled_mesh_domain_3.h>
#include <CGAL/Mesh_complex_3_in_triangulation_3.h>
#include <CGAL/Mesh_criteria_3.h>
#include <CGAL/number_type_config.h> // CGAL_PI
#include <cmath>
#include <iostream>
#include <fstream>


namespace pygalmesh {

typedef CGAL::Exact_predicates_inexact_constructions_kernel K;

typedef CGAL::Labeled_mesh_domain_3<K> Periodic_mesh_domain;

// Triangulation
typedef CGAL::Periodic_3_mesh_triangulation_3<Periodic_mesh_domain>::type Tr;
typedef CGAL::Mesh_complex_3_in_triangulation_3<Tr> C3t3;

// Mesh Criteria
typedef CGAL::Mesh_criteria_3<Tr> Mesh_criteria;
typedef Mesh_criteria::Facet_criteria Facet_criteria;
typedef Mesh_criteria::Cell_criteria Cell_criteria;


typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
typedef K::FT                                               FT;
typedef K::Point_3                                          Point;
typedef K::Iso_cuboid_3                                     Iso_cuboid;
// Domain
typedef FT (Function)(const Point&);
typedef CGAL::Labeled_mesh_domain_3<K>                      Periodic_mesh_domain;
// Triangulation
typedef CGAL::Periodic_3_mesh_triangulation_3<Periodic_mesh_domain>::type Tr;
typedef CGAL::Mesh_complex_3_in_triangulation_3<Tr>                       C3t3;
// Criteria
typedef CGAL::Mesh_criteria_3<Tr>                           Periodic_mesh_criteria;
// To avoid verbose function and named parameters call
using namespace CGAL::parameters;

void
generate_periodic_mesh(
    const std::shared_ptr<pygalmesh::DomainBase> & domain,
    const std::string & outfile,
    const std::array<double, 6> bounding_cuboid,
    const bool lloyd,
    const bool odt,
    const bool perturb,
    const bool exude,
    const bool manifold,
    const double max_edge_size_at_feature_edges,
    const double min_facet_angle,
    const double max_radius_surface_delaunay_ball,
    const double max_facet_distance,
    const double max_circumradius_edge_ratio,
    const double max_cell_circumradius,
    const int number_of_copies_in_output,
    const bool verbose,
    const int seed
    )
{
  CGAL::get_default_random() = CGAL::Random(seed);

  K::Iso_cuboid_3 cuboid(
      bounding_cuboid[0],
      bounding_cuboid[1],
      bounding_cuboid[2],
      bounding_cuboid[3],
      bounding_cuboid[4],
      bounding_cuboid[5]
      );

  // wrap domain
  const auto d = [&](K::Point_3 p) {
    return domain->eval({p.x(), p.y(), p.z()});
  };
  Periodic_mesh_domain cgal_domain =
    Periodic_mesh_domain::create_implicit_mesh_domain(d, cuboid);

  Mesh_criteria criteria(
      CGAL::parameters::edge_size=max_edge_size_at_feature_edges,
      CGAL::parameters::facet_angle=min_facet_angle,
      CGAL::parameters::facet_size=max_radius_surface_delaunay_ball,
      CGAL::parameters::facet_distance=max_facet_distance,
      CGAL::parameters::cell_radius_edge_ratio=max_circumradius_edge_ratio,
      CGAL::parameters::cell_size=max_cell_circumradius
      );

  // Mesh generation
  if (!verbose) {
    // suppress output
    std::cerr.setstate(std::ios_base::failbit);
  }
  C3t3 c3t3 = CGAL::make_periodic_3_mesh_3<C3t3>(
      cgal_domain,
      criteria,
      manifold ? CGAL::parameters::manifold() : CGAL::parameters::non_manifold(),
      lloyd ? CGAL::parameters::lloyd(CGAL::parameters::default_values()) : CGAL::parameters::no_lloyd(),
      odt ? CGAL::parameters::odt(CGAL::parameters::default_values()) : CGAL::parameters::no_odt(),
      perturb ? CGAL::parameters::perturb() : CGAL::parameters::no_perturb(),
      exude ? CGAL::parameters::exude() : CGAL::parameters::no_exude()
      );
  if (!verbose) {
    std::cerr.clear();
  }

  // Output
  std::ofstream medit_file(outfile);
  CGAL::output_periodic_mesh_to_medit(medit_file, c3t3, number_of_copies_in_output);
  medit_file.close();

  return;
}

} // namespace pygalmesh
