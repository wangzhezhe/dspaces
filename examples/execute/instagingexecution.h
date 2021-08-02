#ifndef __DSPACES_INSTAGINGEXECUTION_H
#define __DSPACES_INSTAGINGEXECUTION_H

#include <gspace.h>
#include <mpi.h>
#include <ss_data.h>


// this file is both shared by cpp compiler and the c compiler
#ifdef __cplusplus
extern "C" {
#endif


void instaging_execution_vis(struct ds_gspace *gspace_ptr,
                             unsigned int iteration, MPI_Comm comm);

#ifdef __cplusplus
}
#endif


#endif