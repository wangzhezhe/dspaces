#include "instagingexecution.h"

void instaging_execution_vis(struct ds_gspace *gspace_ptr,
                             unsigned int iteration)
{
    // get the dedicated data from the server
    obj_descriptor target_odsc;
    // populate the target_odsc
    target_odsc.version = iteration;
    // this temporary name is only for testing
    strcpy(target_odsc.name, "instaging_data");

    struct obj_data *from_obj;

    // this only compares the version and the name
    // and extract the data from storage service
    from_obj = ls_find(gspace_ptr->ls, &target_odsc);

    // reorganize the data into a proper format
    // Mblist in this case
    // TODO maybe make it to become more flexible part
    // such as the dyanmic pipeline things
    // then start some visulization works

    return;
}