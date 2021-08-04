#include "instagingexecution.h"
#include "MPIInSituAdaptor.hpp"
#include "mb.hpp"
#include <vector>

#define DEBUG_OUT(args...)                                                     \
    do {                                                                       \
        fprintf(stderr, args);                                                 \
    } while(0);

void instaging_execution_vis(struct ds_gspace *gspace_ptr,
                             unsigned int iteration, MPI_Comm comm)
{
    // get the dedicated data from the server
    obj_descriptor target_odsc;
    // populate the target_odsc
    target_odsc.version = iteration;
    // this temporary name is only for testing
    strcpy(target_odsc.name, "mb");

    struct obj_data *from_obj = NULL;

    // this only compares the version and the name
    // and extract the data from storage service
    // from_obj = ls_find(gspace_ptr->ls, &target_odsc);

    // if(from_obj == NULL) {
    //    DEBUG_OUT("could not find the varname mb");
    //    return;
    //}
    // there are multiple element satisfy requirments

    std::vector<Mandelbulb> MandelbulbList;
    int totalBlock = 0;

    int index = target_odsc.version % gspace_ptr->ls->size_hash;
    struct list_head *list = &gspace_ptr->ls->obj_hash[index];
    // the pointer to the dedicated object_data
    struct obj_data *od = NULL;
    int localBlocks = 0;

    // try to add extra reduce operation here to calculate the total block value
    list_for_each_entry(od, list, struct obj_data, obj_entry)
    {
        if(obj_desc_equals_name_version(&target_odsc, &od->obj_desc)) {
            localBlocks++;
        }
    }

    // calculate the total block num
    MPI_Allreduce(&localBlocks, &totalBlock, 1, MPI_INT, MPI_SUM, comm);

    DEBUG_OUT("iteration %d local blocks %d total blocks %d\n", iteration,
              localBlocks, totalBlock);

    list_for_each_entry(od, list, struct obj_data, obj_entry)
    {
        if(obj_desc_equals_name_version(&target_odsc, &od->obj_desc)) {
            // when we find one
            // reorganize the data into a proper format
            // Mblist in this case
            // TODO maybe make it to become more flexible part
            // such as the dyanmic pipeline things
            // then start some visulization works
            // we need these information
            // try to print out these information:
            // blockID can be the flags
            // and the total block can be global varaible
            // width, height, depth, 1.2, blockID, totalBlock

            DEBUG_OUT("prepare execution, varName %s, step %d, flag %d, dims "
                      "%d, lb %d "
                      "%d %d, ub %d %d %d\n",
                      od->obj_desc.name, od->obj_desc.version,
                      od->obj_desc.flags, od->obj_desc.bb.num_dims,
                      od->obj_desc.bb.lb.c[0], od->obj_desc.bb.lb.c[1],
                      od->obj_desc.bb.lb.c[2], od->obj_desc.bb.ub.c[0],
                      od->obj_desc.bb.ub.c[1], od->obj_desc.bb.ub.c[2]);

            // create the mb instance, the sequence in client is depth width
            // height, for the depth, there is +1 at the inner data structure,
            // so do not update depth the correct number should be 64 64 64
            unsigned depth = od->obj_desc.bb.ub.c[0] - od->obj_desc.bb.lb.c[0];
            unsigned width =
                od->obj_desc.bb.ub.c[1] - od->obj_desc.bb.lb.c[1] + 1;
            unsigned height =
                od->obj_desc.bb.ub.c[2] - od->obj_desc.bb.lb.c[2] + 1;
            int blockID = od->obj_desc.flags;
            size_t blockOffset = blockID * depth;

            Mandelbulb mb(width, height, depth, blockOffset, 1.2, blockID,
                          totalBlock);

            mb.SetDataVoidPtr(od->data, mb.DataSize() * sizeof(int));
            MandelbulbList.push_back(mb);
        }
    }
    MPI_Barrier(comm);

    // init things, the first one is initilization operation
    // try to update this part if the first iteration is not start from 0
    if(iteration == 0) {
        // the MPI things need to be initilized at the first iteration
        char const *scriptTemp = getenv("SCRIPTNAME");
        if(scriptTemp != NULL) {
            std::string m_script_name = std::string(scriptTemp);
            InSitu::MPIInitialize(m_script_name);

        } else {
            throw std::runtime_error("failed to get SCRIPTNAME env");
        }
    }

    // process the insitu function for the MandelbulbList
    // the controller is updated in the MonaUpdateController
    InSitu::MPICoProcessDynamic(comm, MandelbulbList, totalBlock, iteration,
                                iteration);

    return;
}