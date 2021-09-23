/*
 * Copyright (c) 2020, Rutgers Discovery Informatics Institute, Rutgers
 * University
 *
 * See COPYRIGHT in top-level directory.
 */

#include <dspaces-server.h>
#include <fstream>
#include <iostream>
#include <margo.h>
#include <mercury.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <mona-coll.h>
#include <mona.h>

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

const std::string serverCred = "dspaces_drc.config";

int main(int argc, char **argv)
{
    if(argc != 3) {
        fprintf(stderr, "Usage: %s <listen-address> <init/elastic>\n", argv[0]);
        return -1;
    }
    // this is protocal label such as gni or tcp
    char *listen_addr_str = argv[1];

    char *status = argv[2];

    int group_elastic = 0;

    if(strcmp(status, "elastic") == 0) {
        group_elastic = 1;
    }

    dspaces_provider_t s = dspaces_PROVIDER_NULL;

    // create the mona comm
    int rank, procs;

    MPI_Init(&argc, &argv);
    ABT_init(0, NULL);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &procs);
    MPI_Comm gcomm = MPI_COMM_WORLD;

    uint32_t drc_credential_id = 0;
    drc_info_handle_t drc_credential_info;
    uint32_t drc_cookie;
    char drc_key_str[256] = {0};
    int ret;

    struct hg_init_info hii;
    memset(&hii, 0, sizeof(hii));
    if(group_elastic == 0) {
        if(rank == 0) {
            ret = drc_acquire(&drc_credential_id, DRC_FLAGS_FLEX_CREDENTIAL);
            DIE_IF(ret != DRC_SUCCESS, "drc_acquire");

            ret = drc_access(drc_credential_id, 0, &drc_credential_info);
            DIE_IF(ret != DRC_SUCCESS, "drc_access");
            drc_cookie = drc_get_first_cookie(drc_credential_info);
            sprintf(drc_key_str, "%u", drc_cookie);
            hii.na_init_info.auth_key = drc_key_str;

            ret = drc_grant(drc_credential_id, drc_get_wlm_id(),
                            DRC_FLAGS_TARGET_WLM);
            DIE_IF(ret != DRC_SUCCESS, "drc_grant");

            std::cout << "grant the drc_credential_id: " << drc_credential_id
                      << std::endl;
            std::cout << "use the drc_key_str " << drc_key_str << std::endl;
            for(int dest = 1; dest < procs; dest++) {
                // dest tag communicator
                MPI_Send(&drc_credential_id, 1, MPI_UINT32_T, dest, 0,
                         MPI_COMM_WORLD);
            }

            // write this cred_id into file that can be shared by clients
            // output the credential id into the config files
            std::ofstream credFile;
            credFile.open(serverCred);
            credFile << drc_credential_id << "\n";
            credFile.close();
        } else {
            // send rcv is the block call
            // gather the id from the rank 0
            // source tag communicator
            MPI_Recv(&drc_credential_id, 1, MPI_UINT32_T, 0, 0, MPI_COMM_WORLD,
                     MPI_STATUS_IGNORE);
            std::cout << "rank " << rank << " recieve cred key "
                      << drc_credential_id << std::endl;

            if(drc_credential_id == 0) {
                throw std::runtime_error("failed to rcv drc_credential_id");
            }
            ret = drc_access(drc_credential_id, 0, &drc_credential_info);
            DIE_IF(ret != DRC_SUCCESS, "drc_access %u", drc_credential_id);
            drc_cookie = drc_get_first_cookie(drc_credential_info);

            sprintf(drc_key_str, "%u", drc_cookie);
            hii.na_init_info.auth_key = drc_key_str;
        }
    } else if(group_elastic == 1) {
        // load the drc key from the file
        // the init group have been started
        std::ifstream infile(serverCred);
        std::string cred_id;
        std::getline(infile, cred_id);

        std::cout << "load cred_id: " << cred_id << std::endl;
        drc_credential_id = (uint32_t)atoi(cred_id.c_str());

        ret = drc_access(drc_credential_id, 0, &drc_credential_info);
        DIE_IF(ret != DRC_SUCCESS, "drc_access");
        drc_cookie = drc_get_first_cookie(drc_credential_info);
        sprintf(drc_key_str, "%u", drc_cookie);
        hii.na_init_info.auth_key = drc_key_str;
        // it is ok go get cookie and we do need to execute the grant operation here
    } else {
        throw std::runtime_error("elastic group value is incorrect");
    }

    MPI_Barrier(gcomm);

    // int init_ret = dspaces_server_init(listen_addr_str, gcomm, &s, &hii);
    // the first parameter of the mona is the protocol
    mona_instance_t mona =
        mona_init_thread(listen_addr_str, NA_TRUE, &hii.na_init_info, NA_TRUE);

    // create mona add
    na_addr_t mona_addr;
    mona_addr_self(mona, &mona_addr);
    char mona_addr_buf[256];
    na_size_t mona_addr_size = 256;
    mona_addr_to_string(mona, mona_addr_buf, &mona_addr_size, mona_addr);
    std::cout << "MoNA address is " << std::string(mona_addr_buf) << std::endl;

    // init the server
    // then register the mona addr to the master
    int init_ret = dspaces_server_init_mona(listen_addr_str, gcomm, &s,
                                            mona_addr_buf, group_elastic, &hii);
    if(init_ret != 0)
        return init_ret;

    // we can register the mona addr and create the mona comm here

    // make margo wait for finalize
    dspaces_server_fini(s);

    if(rank == 0) {
        fprintf(stderr, "Server is all done!\n");
    }

    mona_addr_free(mona, mona_addr);
    ABT_finalize();
    MPI_Finalize();
    return 0;
}
