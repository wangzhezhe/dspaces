#include "mpi.h"
#include <dspaces.h>
#include <fstream>
#include <iostream>
#include <margo.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern "C" {
#include <rdmacred.h>
}

#define DIE_IF(cond_expr, err_fmt, ...)                                        \
    do {                                                                       \
        if(cond_expr) {                                                        \
            fprintf(stderr, "ERROR at %s:%d (" #cond_expr "): " err_fmt "\n",  \
                    __FILE__, __LINE__, ##__VA_ARGS__);                        \
            exit(1);                                                           \
        }                                                                      \
    } while(0)

int main(int argc, char **argv)
{

    int rank, procs;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &procs);

    // load the drc file
    std::string g_drc_file = "dspaces_drc.config";
    std::ifstream infile(g_drc_file);
    std::string cred_id;
    std::getline(infile, cred_id);
    if(rank == 0) {
        std::cout << "load cred_id: " << cred_id << std::endl;
    }

    char drc_key_str[256] = {0};
    uint32_t drc_cookie;
    uint32_t drc_credential_id;
    drc_info_handle_t drc_credential_info;
    int ret;
    drc_credential_id = (uint32_t)atoi(cred_id.c_str());

    ret = drc_access(drc_credential_id, 0, &drc_credential_info);
    DIE_IF(ret != DRC_SUCCESS, "drc_access %u", drc_credential_id);
    drc_cookie = drc_get_first_cookie(drc_credential_info);

    struct hg_init_info hii;
    memset(&hii, 0, sizeof(hii));
    sprintf(drc_key_str, "%u", drc_cookie);
    hii.na_init_info.auth_key = drc_key_str;

    // then put the data into staging service
    dspaces_client_t client;

    // Initalize DataSpaces
    dspaces_init(rank, &client, &hii);

    int timestep = 0;

    while(timestep < 10) {
        timestep++;
        sleep(2);

        // Name the Data that will be writen
        char var_name[128];
        sprintf(var_name, "ex1_sample_data");

        // Create integer array, size 3
        int *data = (int *)malloc(3 * sizeof(int));

        // Initialize Random Number Generator
        srand(time(NULL));

        // Populate data array with random values from 0 to 99
        data[0] = rand() % 100;
        data[1] = rand() % 100;
        data[2] = rand() % 100;

        printf("Timestep %d: put data %d %d %d\n", timestep, data[0], data[1],
               data[2]);

        // ndim: Dimensions for application data domain
        // In this case, our data array is 1 dimensional
        int ndim = 1;

        // Prepare LOWER and UPPER bound dimensions
        // In this example, we will put all data into a
        // small array at the origin upper bound = lower bound = (0,0,0)
        // In further examples, we will expand this concept.
        uint64_t lb, ub;
        lb = 0;
        ub = 2;

        // DataSpaces: Put data array into the space
        // Usage: dspaces_put(Name of variable, version num,
        // size (in bytes of each element), dimensions for bounding box,
        // lower bound coordinates, upper bound coordinates,
        // ptr to data buffer
        dspaces_put(client, var_name, timestep, sizeof(int), ndim, &lb, &ub,
                    data);

        free(data);

        // TODO then try to execute it here
        dspaces_execute(client, timestep);
    }

    // Signal the server to shutdown (the server must receive this signal n
    // times before it shuts down, where n is num_apps in dataspaces.conf)
    dspaces_kill(client);

    // DataSpaces: Finalize and clean up DS process
    dspaces_fini(client);

    return 0;
}