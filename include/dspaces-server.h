/*
 * Copyright (c) 2020, Rutgers Discovery Informatics Institute, Rutgers
 * University
 *
 * See COPYRIGHT in top-level directory.
 */

#ifndef __DSPACES_SERVER_H
#define __DSPACES_SERVER_H

#include <dspaces-common.h>
#include <mpi.h>
#include <stdio.h>
#include <margo.h>
#include <mercury.h>

#include <mona-coll.h>
#include <mona.h>

#if defined(__cplusplus)
extern "C" {
#endif

#define dspaces_ABT_POOL_DEFAULT ABT_POOL_NULL

typedef struct dspaces_provider *dspaces_provider_t;
#define dspaces_PROVIDER_NULL ((dspaces_provider_t)NULL)

struct dspaces_provider {
    margo_instance_id mid;
    hg_id_t put_id;
    hg_id_t put_local_id;
    hg_id_t put_meta_id;
    hg_id_t query_id;
    hg_id_t query_meta_id;
    hg_id_t get_id;
    hg_id_t get_local_id;
    hg_id_t obj_update_id;
    hg_id_t odsc_internal_id;
    hg_id_t ss_id;
    hg_id_t drain_id;
    hg_id_t kill_id;
    hg_id_t kill_client_id;
    hg_id_t sub_id;
    hg_id_t notify_id;
    hg_id_t execute_id;
    hg_id_t register_addr_id;
    hg_id_t update_addrs_id;
    hg_id_t sync_view_id;
    hg_id_t set_expected_servernum_id;

    struct ds_gspace *dsg;
    char **server_address;
    char **node_names;
    char *listen_addr_str;
    int init_rank;
    int comm_size;
    int f_debug;
    int f_drain;
    int f_kill;

    MPI_Comm comm;
    // TODO add mona comm here

    int if_leader;

    ABT_mutex odsc_mutex;
    ABT_mutex ls_mutex;
    ABT_mutex dht_mutex;
    ABT_mutex sspace_mutex;
    ABT_mutex kill_mutex;

    ABT_xstream drain_xstream;
    ABT_pool drain_pool;
    ABT_thread drain_t;
};


/**
 * @brief Creates a MESSAGING server.
 *
 * @param[in] comm MPI Comminicator
 * @param[out] server MESSAGING server
 * @param[in] debug enable debugging
 * @return MESSAGING_SUCCESS or error code defined in messaging-common.h
 */
int dspaces_server_init(char *listen_addr_str, MPI_Comm comm,
                        dspaces_provider_t *sv, struct hg_init_info *hii_ptr); 


int dspaces_server_init_mona(char *listen_addr_str, MPI_Comm comm,
                             dspaces_provider_t *sv, mona_instance_t mona, char *my_mona_addr,
                             int group_elastic, struct hg_init_info *hii_ptr);                              
/**
 * @brief Waits for the dataspaces server to finish (be killed.)
 *
 * @param[in] server Messaging server
 *
 */
void dspaces_server_fini(dspaces_provider_t server);

#if defined(__cplusplus)
}
#endif

#endif
