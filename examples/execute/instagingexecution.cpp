#include "instagingexecution.h"
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

    int index = target_odsc.version % gspace_ptr->ls->size_hash;
    struct list_head *list = &gspace_ptr->ls->obj_hash[index];
    // the pointer to the dedicated object_data
    struct obj_data *od = NULL;
    int count = 0;
    list_for_each_entry(od, list, struct obj_data, obj_entry)
    {
        if(obj_desc_equals_intersect(&target_odsc, &od->obj_desc)) {
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

            DEBUG_OUT("prepare execution, varName %s, step %d, dims %d, lb %d "
                      "%d %d, ub %d %d %d\n",
                      od->obj_desc.name, od->obj_desc.version,
                      od->obj_desc.bb.num_dims, od->obj_desc.bb.lb.c[0],
                      od->obj_desc.bb.lb.c[1], od->obj_desc.bb.lb.c[2],
                      od->obj_desc.bb.ub.c[0], od->obj_desc.bb.ub.c[1],
                      od->obj_desc.bb.ub.c[2]);

            count++;
        }
    }
    DEBUG_OUT("iteration %d count number %d\n", iteration, count);

    // create the mb list
    // execute the vis pipeline based on the comm
    // std::vector<Mandelbulb> MandelbulbList;
    // TODO get totalBlock from the env
    int totalBlock = 0;
    // get block num by collective operation
    {

        // std::cout << "debug totalBlock is " << totalBlock << std::endl;
        // std::cout << "iteration " << iteration << " procRank " << procRank <<
        // " key ";
        /*
        for(auto &t : m_datasets[iteration]["mydata"]) {
            size_t blockID = t.first;
            // std::cout << blockID << ",";
            auto depth = t.second.dimensions[0] - 1;
            auto height = t.second.dimensions[1];
            auto width = t.second.dimensions[2];

            size_t blockOffset = blockID * depth;
            // reconstruct the MandelbulbList
            // std::cout << "debug parameters " << width << "," << height << ","
            // << depth << ","
            //          << blockOffset << std::endl;
            Mandelbulb mb(width, height, depth, blockOffset, 1.2, totalBlock);
            mb.SetData(t.second.data);
            MandelbulbList.push_back(mb);
        */
    }

    MPI_Barrier(comm);
    // process the insitu function for the MandelbulbList
    // the controller is updated in the MonaUpdateController
    // InSitu::MPICoProcessDynamic(this->m_mpi_comm, MandelbulbList, totalBlock,
    //                            iteration, iteration);

    return;
}