# the execution will be added with this macro
add_definitions(-DUSE_EXECUTION)

message ("module path: " ${CMAKE_MODULE_PATH})
find_package(DRC REQUIRED)
include_directories(${DRC_INCLUDE_DIRS})
message ("use cray DRC")
message ("include drc" ${DRC_INCLUDE_DIRS})
message ("link drc lib" ${DRC_LIBRARIES})

message ("dspaces_SOURCE_DIR:" ${dspaces_SOURCE_DIR})

include_directories(
     ${dspaces_SOURCE_DIR}/include
     ${dspaces_SOURCE_DIR}/examples/execute
)


find_package (Python3)

find_package(ParaView 5.8 REQUIRED COMPONENTS Catalyst PythonCatalyst)

# find VTK
find_package(VTK COMPONENTS
CommonDataModel
ParallelMPI
FiltersSources
)

if(NOT PARAVIEW_USE_MPI)
  message(SEND_ERROR "ParaView must be built with MPI enabled")
endif()
find_package(MPI REQUIRED)

xpkg_import_module (margo REQUIRED margo)

# find Mona packages
find_package (mona REQUIRED)
find_package (PkgConfig REQUIRED)
find_package (mercury REQUIRED)
pkg_check_modules (ARGOBOTS REQUIRED IMPORTED_TARGET argobots)


#if the cpath is not updated, we may use this way to find the mona
#find_package (PkgConfig REQUIRED)
#pkg_check_modules (MONA REQUIRED IMPORTED_TARGET mona)
#link against PkgConfig::MONA
#updates the CPATH manually (add spack view into it)
#CPATH="/global/cscratch1/sd/zw241/colza-experiments/cori/vtk/sw/spack/var/spack/environments/colza-env/.spack-env/view/include:$CPATH"

set(instagingexecution-src
${dspaces_SOURCE_DIR}/src/ss_data.c
instagingexecution.cpp
MPIInSituAdaptor.cpp
)
add_library(instagingexecution ${instagingexecution-src})
target_link_libraries(instagingexecution margo
ParaView::icet 
ParaView::RemotingViews 
ParaView::PythonCatalyst 
VTK::ParallelMPI 
VTK::CommonDataModel 
MPI::MPI_C)


set(dspaces-drc-server 
dsserver.cpp
${dspaces_SOURCE_DIR}/src/addrmanager-server.cpp
${dspaces_SOURCE_DIR}/src/util.c 
${dspaces_SOURCE_DIR}/src/bbox.c 
${dspaces_SOURCE_DIR}/src/ss_data.c
${dspaces_SOURCE_DIR}/src/dspaces-server.c
)
add_executable(dsserver_drc ${dspaces-drc-server})
target_link_libraries(dsserver_drc margo MPI::MPI_C ${DRC_LIBRARIES} instagingexecution mona mercury PkgConfig::ARGOBOTS)

get_property(inc_dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
foreach(dir ${dirs})
  message(STATUS "dir='${dir}'")
endforeach()

set(dspaces-drc-client sim.cpp 
${dspaces_SOURCE_DIR}/src/addrmanager-client.cpp
${dspaces_SOURCE_DIR}/src/util.c 
${dspaces_SOURCE_DIR}/src/bbox.c 
${dspaces_SOURCE_DIR}/src/ss_data.c
${dspaces_SOURCE_DIR}/src/dspaces-client.c)
add_executable(dsclient_drc ${dspaces-drc-client})
target_link_libraries(dsclient_drc margo MPI::MPI_C ${DRC_LIBRARIES})

set(dspaces-drc-client-elastic sim_elastic.cpp 
${dspaces_SOURCE_DIR}/src/addrmanager-client.cpp
${dspaces_SOURCE_DIR}/src/util.c 
${dspaces_SOURCE_DIR}/src/bbox.c 
${dspaces_SOURCE_DIR}/src/ss_data.c
${dspaces_SOURCE_DIR}/src/dspaces-client.c)
add_executable(dsclient_drc_elastic ${dspaces-drc-client-elastic})
target_link_libraries(dsclient_drc_elastic margo MPI::MPI_C ${DRC_LIBRARIES})