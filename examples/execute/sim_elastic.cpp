#include "mpi.h"
#include <dspaces.h>
#include <fstream>
#include <iostream>
#include <margo.h>
#include <mb.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <vector>
#define BILLION 1000000000L

#include "addrmanager-client.hpp"

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

size_t int2size_t(int val)
{
    return (val < 0) ? __SIZE_MAX__ : (size_t)((unsigned)val);
}

int main(int argc, char **argv)
{
    if(argc != 5) {
        std::cout << "binary <blockLen> <totalblockNumber> "
                     "<availableElasticity> <expectedServerNum>"
                  << std::endl;
        exit(0);
    }

    int blockLen = std::stoi(argv[1]);
    int totalBlockNumber = std::stoi(argv[2]);
    int availableElasticProcess = std::stoi(argv[3]);
    int expectedServerNum = std::stoi(argv[4]);

    int rank, procs;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &procs);

    int ifleader = 0;
    if(rank == 0) {
        ifleader = 1;
    }

    if(rank == 0) {
        std::cout << "blockLen is " << blockLen << " totalBlockNumber size is "
                  << totalBlockNumber << std::endl;
    }

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

    std::cout << "debug, ok to init dspaces_init " << std::endl;

    // get the leader addr from file and set the leader addr into the manager
    std::string stageLeaderFile = "ds_leader_addr.conf";
    std::ifstream infileStageLeader(stageLeaderFile);
    std::string leaderAddr;
    std::getline(infileStageLeader, leaderAddr);
    ClientAddrSetLeaderAddr(leaderAddr.c_str());

    int timestep = 0;

    // initilize data for computation
    std::vector<Mandelbulb> MandelbulbList;
    if(rank == 0) {
        // init the expected number at the server side
        dspaces_set_expected_servernum(client, expectedServerNum);
    }
    while(timestep < 6) {
        /* mb compute the data, provide necessary parameters*/
        MPI_Barrier(MPI_COMM_WORLD);
        struct timespec computeStart, computeEnd;
        clock_gettime(CLOCK_REALTIME, &computeStart);

        unsigned reminder = 0;
        if(totalBlockNumber % procs != 0) {
            // the last process will process the reminder
            reminder = (totalBlockNumber) % unsigned(procs);
        }

        unsigned nblocks_per_proc = totalBlockNumber / procs;
        if(rank < reminder) {
            // some process need to procee more than one
            nblocks_per_proc = nblocks_per_proc + 1;
        }
        // this value will vary when there is process join/leave
        // compare the nblocks_per_proc to see if it change
        MandelbulbList.clear();

        // caculate the order
        double order = 4.0 + ((double)timestep) * 8.0 / 100.0;

        // update the data list
        // we may need to do some data marshal here, when the i is small, the
        // time is shor when the i is large, the time is long, there is
        // unbalance here the index should be 0, n-1, 1, n-2, ... the block id
        // base may also need to be updated? init the list
        int rank_offset = procs;
        int blockid = rank;
        // we use the simplified block and let width equals to heights equals to
        // depth
        for(int i = 0; i < nblocks_per_proc; i++) {
            if(blockid >= totalBlockNumber) {
                continue;
            }
            int block_offset = blockid * blockLen;
            // the block offset need to be recaculated, we can not use the
            // resize function
            MandelbulbList.push_back(Mandelbulb(blockLen, blockLen, blockLen,
                                                block_offset, 1.2, blockid,
                                                totalBlockNumber));
            blockid = blockid + rank_offset;
        }

        // compute list (this is time consuming part)
        for(int i = 0; i < nblocks_per_proc; i++) {
            MandelbulbList[i].compute(order);
        }

        // wait finish for all processes

        MPI_Barrier(MPI_COMM_WORLD);
        clock_gettime(CLOCK_REALTIME, &computeEnd);

        if(rank == 0) {
            double computeDiff =
                (computeEnd.tv_sec - computeStart.tv_sec) * 1.0 +
                (computeEnd.tv_nsec - computeStart.tv_nsec) * 1.0 / BILLION;

            std::cout << "iteration " << timestep << " nblocks_per_proc "
                      << nblocks_per_proc << " compute time is " << computeDiff
                      << std::endl;
        }

        /* start a new staging service if there are avalaible slots and then
         * sync it */

        if(availableElasticProcess > 0 && timestep > 0) {
            // write out a configure file to start new staging service
            if(rank == 0 && timestep == 1) {
                // naive strategy
                // update the expected server value
                std::string leaveConfigPath = getenv("ADDSERVERCONFIGPATH");
                // the ADDSERVERCONFIGPATH contains the
                static std::string leaveFileName =
                    leaveConfigPath + "addserver.config" + std::to_string(rank);
                std::cout << "leaveFileName is " << leaveFileName << std::endl;
                std::ofstream leaveFile;
                leaveFile.open(leaveFileName);
                leaveFile << "test\n";
                leaveFile.close();

                // reset the expected number at the server side
                dspaces_set_expected_servernum(client, expectedServerNum + 1);
            }
        }

        MPI_Barrier(MPI_COMM_WORLD);
        // call the sync stage api
        int ret = dspaces_syncview(client, timestep, ifleader);
        if(ret != 0) {
            throw std::runtime_error("dspaces_syncview return -1");
        }
        /* the stage of transfering data into the data staging service */

        // Name the Data that will be writen
        char varName[128];
        sprintf(varName, "mb");
        int ndim = 3;

        // range the list and put the data
        int listSize = MandelbulbList.size();
        for(int i = 0; i < listSize; i++) {
            // caculate the lb and ub, the sequence is depth, width and heights
            // the common expression is the offset and the extents
            // we need to transfer it to the lb and ub whihc is required by the
            // dspsaces the lb is actually the offset of the mb data
            int *extents = MandelbulbList[i].GetExtents();
            uint64_t lb[3] = {MandelbulbList[i].GetZoffset(), 0, 0};
            uint64_t ub[3] = {lb[0] + int2size_t(*(extents + 1)),
                              lb[1] + int2size_t(*(extents + 3)),
                              lb[2] + int2size_t(*(extents + 5))};
            int flag = MandelbulbList[i].GetBlockID();
            // dspaces_put(client, varName, flag, timestep, sizeof(int), ndim,
            // lb,
            //            ub, MandelbulbList[i].GetData());
        }

        MPI_Barrier(MPI_COMM_WORLD);
        struct timespec executeStart, executeEnd;
        clock_gettime(CLOCK_REALTIME, &executeStart);

        // after putting the data, try to execute
        // dspaces_execute(client, timestep, rank);

        MPI_Barrier(MPI_COMM_WORLD);
        clock_gettime(CLOCK_REALTIME, &executeEnd);

        if(rank == 0) {
            double executeDiff =
                (executeEnd.tv_sec - executeStart.tv_sec) * 1.0 +
                (executeEnd.tv_nsec - executeStart.tv_nsec) * 1.0 / BILLION;

            std::cout << "iteration " << timestep << " execution time is "
                      << executeDiff << std::endl;
        }

        timestep++;
    }

    // Signal the server to shutdown (the server must receive this signal n
    // times before it shuts down, where n is num_apps in dataspaces.conf)
    dspaces_kill(client);

    // DataSpaces: Finalize and clean up DS process
    dspaces_fini(client);

    return 0;
}