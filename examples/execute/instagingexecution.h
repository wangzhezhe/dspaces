#ifndef __DSPACES_INSTAGINGEXECUTION_H
#define __DSPACES_INSTAGINGEXECUTION_H

// this file is both shared by cpp compiler and the c compiler
#ifdef __cplusplus
extern "C" {
#include <gspace.h>
void instaging_execution_vis(struct ds_gspace *gspace_ptr,
                             unsigned int iteration);
}
#else
#include <gspace.h>
void instaging_execution_vis(struct ds_gspace *gspace_ptr,
                             unsigned int iteration);
#endif

#endif