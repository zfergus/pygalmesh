FILE(GLOB pygalmesh_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
# FILE(GLOB pygalmesh_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp")

include(pybind11)
pybind11_add_module(_pygalmesh ${pygalmesh_SRCS})

include(eigen)
target_link_libraries(_pygalmesh PRIVATE Eigen3::Eigen)

FIND_PACKAGE(CGAL REQUIRED)
target_link_libraries(_pygalmesh PRIVATE CGAL::CGAL)

# https://github.com/CGAL/cgal/issues/6002
# find_program(iwyu_path NAMES include-what-you-use iwyu REQUIRED)
# set_property(TARGET pygalmesh PROPERTY CXX_INCLUDE_WHAT_YOU_USE ${iwyu_path})

# execute_process(
#   COMMAND python -c "from distutils.sysconfig import get_python_lib; print get_python_lib()"
#   OUTPUT_VARIABLE PYTHON_SITE_PACKAGES
#   OUTPUT_STRIP_TRAILING_WHITESPACE
#   )
# install(TARGETS _pygalmesh DESTINATION ${PYTHON_SITE_PACKAGES})
# install(
#   FILES ${CMAKE_BINARY_DIR}/src/pygalmesh.py
#   DESTINATION ${PYTHON_SITE_PACKAGES}
#   )
