#include "addrmanager-server.hpp"
#include <abt.h>
#include <iostream>
#include <map>
#include <mona-coll.h>
#include <mona.h>
#include <set>
#include <spdlog/spdlog.h>
#include <ss_data.h>
#include <vector>

// global variable
struct UpdatedMonaList {
    UpdatedMonaList(){};
    UpdatedMonaList(std::vector<std::string> mona_added_list,
                    std::vector<std::string> mona_remove_list)
        : m_mona_added_list(mona_added_list),
          m_mona_remove_list(mona_remove_list){};

    std::vector<std::string> m_mona_added_list;
    std::vector<std::string> m_mona_remove_list;

    ~UpdatedMonaList(){};
};

// meta information hold by leader
struct StagingLeaderMeta {
    // compare the expected registered worker and actual registered
    ABT_mutex m_workernum_mtx;
    // the actual data is the size of the m_mona_addresses_map
    int m_expected_worker_num = -1;

    ABT_mutex m_monaAddrmap_mtx;
    // the key is the margo addr
    // the value is the mona addr
    std::map<std::string, std::string> m_mona_addresses_map;

    // sending the modification to others
    // these are used for storing the thallium addr
    ABT_mutex m_modifiedAddr_mtx;
    // the added and removed list should put the mona addr
    // they are used to notify staging workers
    std::vector<std::string> m_added_list;
    std::vector<std::string> m_removed_list;
    // this set stores the addrs that are added first time into the existing
    // communication group
    std::set<std::string> m_first_added_set;

    bool addrDiff()
    {
        ABT_mutex_lock(this->m_monaAddrmap_mtx);
        int mapsize = m_mona_addresses_map.size();
        ABT_mutex_unlock(this->m_monaAddrmap_mtx);

        if(mapsize == this->m_expected_worker_num) {
            return false;
        }
        return true;
    }

    // TODO, we do not add or remove process at the same time
    // otherwise, we need to use the hash value to label if there are updates
    // since the view can be different when there is same addr map
    // currently, if actual value do not equal to expected value
    // the view is different
};

// meta information hold by every worker process
// they also need to maintain the mona adder list
// since the in-staging execution use the mona things
struct StagingCommonMeta {
    mona_instance_t m_mona;
    mona_comm_t m_mona_comm = nullptr;

    std::string m_margo_self_addr;
    std::string m_mona_self_addr;
    std::string m_margo_leader_addr;

    // store the current list for mona addr
    std::set<std::string> m_monaaddr_set;
    bool m_ifleader = false;
};

UpdatedMonaList m_updatedMonaList;
StagingLeaderMeta m_stageleader_meta;
StagingCommonMeta m_stagingcommon_meta;

void Addrmanager_setLogLevel(char *loglevel)
{
    spdlog::set_level(spdlog::level::from_str(loglevel));
    return;
}

void Addrmanager_setSelfMargoMona(char *slefMargoAddrstr, char *selfMonaAddrstr)
{
    std::string margoAddr(slefMargoAddrstr);
    std::string monaAddr(selfMonaAddrstr);
    m_stagingcommon_meta.m_margo_self_addr = margoAddr;
    m_stagingcommon_meta.m_mona_self_addr = monaAddr;
    return;
}

void Addrmanager_setLeaderTrue()
{
    m_stagingcommon_meta.m_ifleader = true;
    return;
}

// put margo addr and mona addr into the current addr manager
int Addrmanager_registerMargoAddr(char *margoAddrstr, char *monaAddrstr)
{
    std::string margoAddr(margoAddrstr);
    std::string monaAddr(monaAddrstr);

    // if added, this is a new process, we create a new id, return its id
    // create uuid, put it into the map, return the id
    std::cout << "RPC registerMargoAddr for server: " << margoAddr
              << "mona addr: " << monaAddrstr << std::endl;

    ABT_mutex_lock(m_stageleader_meta.m_monaAddrmap_mtx);
    // if the current addr is not stored into the map
    // it means that the this addr is added for the first time
    if(m_stageleader_meta.m_mona_addresses_map.find(margoAddr) ==
       m_stageleader_meta.m_mona_addresses_map.end()) {
        // this addr is not exist in the map
        // put the thallium addr into it, this record the thallium addr
        // instead of mona addr the client will check this set, and update
        // all mona addrs insted of the modified addrs when the thallium
        // addr is added in the m_first_added_set
        m_stageleader_meta.m_first_added_set.insert(margoAddr);
    }

    m_stageleader_meta.m_mona_addresses_map[margoAddr] = monaAddr;
    m_stageleader_meta.m_added_list.push_back(monaAddr);
    ABT_mutex_unlock(m_stageleader_meta.m_monaAddrmap_mtx);

    return 0;
}

void Addrmanager_addLeaderAddr(char *margoAddrstr)
{
    // add the leader addr into the current manager
    std::string margoAddr(margoAddrstr);
    m_stagingcommon_meta.m_margo_leader_addr = margoAddr;

    return;
}

// this function should be called by the leader process
// the clientProcessNum represents the number of processes that clients knows
// this api returns an list based on margo defination
// the return things contains two structure, the first one is the number of addr
// the second one is the details of the addrs
// we need to set the expected number before calling this
obj_t Addrmanager_syncview(dspaces_provider_t server, int iteration,
                           int clientProcessNum)
{
    if(m_stageleader_meta.m_expected_worker_num == -1) {
        throw std::runtime_error(
            "the expected worker number (including leader) is not updated yet");
    }
    spdlog::debug("sync is called for iteration {}", iteration);

    // check if current server is leader
    if(m_stagingcommon_meta.m_ifleader == false) {
        throw std::runtime_error(
            "sync is supposed to be called for the leader process");
    }

    // client and server already sync (the client process number is as
    // expected), do nothing
    ABT_mutex_lock(m_stageleader_meta.m_monaAddrmap_mtx);
    {

        // we should use the expected client process number
        // instead of the current process number
        // if there is updates for the client, this value should also updated
        // one benifits of using the old process number is that
        // we only do the actual sync when server is actaully loaded, it might
        // need long time to load if (clientProcessNum ==
        // this->m_stageleader_meta->m_mona_addresses_map.size()) compared with
        // the expected number not the actual registered number by this way, we
        // wait wait the addr to be synced for next iteration
        if(clientProcessNum == m_stageleader_meta.m_expected_worker_num) {
            spdlog::debug("iteration {} client process number equals to server",
                          iteration);
            obj_t currentMonaAddrList;
            // when nothing changed
            // we return an empty mona addr list
            currentMonaAddrList.size = 0;
            currentMonaAddrList.raw_obj = NULL;
            return currentMonaAddrList;
        }
    }
    ABT_mutex_unlock(m_stageleader_meta.m_monaAddrmap_mtx);

    // if the current one is the leader process
    // caculate difference between what actual stored
    // and the number we expected
    // if these two number do not equal with each other, we just wait here
    while(m_stageleader_meta.addrDiff()) {
        // there is still process that do not update its addr
        spdlog::debug("wait, current expected process {} addr map size {}",
                      m_stageleader_meta.m_expected_worker_num,
                      m_stageleader_meta.m_mona_addresses_map.size());
        usleep(100000);
    }

    // if expected process equals to the actual one, the leader owns the latest
    // view, it propagate this view to all members
    // it ranges the map and call updateMonaAddrList

    // tl::remote_procedure updateMonaAddrListRPC =
    // send req to workers
    //    this->get_engine().define("updateMonaAddrList");
    // prepare the handle
    ABT_mutex_lock(m_stageleader_meta.m_monaAddrmap_mtx);

    // maybe just create a snap shot of the current addr instead of using a
    // large critical region
    // TODO, if the new joined and it is the first time
    spdlog::info("iteration {} m_added_list size {} m_removed_list size {}",
                 iteration, m_stageleader_meta.m_added_list.size(),
                 m_stageleader_meta.m_removed_list.size());

    // UpdatedMonaList updatedMonaList(
    //    this->m_stageleader_meta->m_added_list,
    //    this->m_stageleader_meta->m_removed_list);

    // prepare the input
    update_addrs_in_t update_addrs_existing;
    // add existing addedlist and removed list here
    update_addrs_existing.added_list.raw_obj = NULL;
    update_addrs_existing.removed_list.raw_obj = NULL;

    // the added_list size should be k*256
    // this is the total length of the object
    int added_addr_num = m_stageleader_meta.m_added_list.size();
    update_addrs_existing.added_list.size = added_addr_num * 256;

    int removed_addr_num = m_stageleader_meta.m_removed_list.size();
    update_addrs_existing.removed_list.size = removed_addr_num * 256;
    m_stageleader_meta.m_removed_list.size();

    if(added_addr_num > 0) {
        update_addrs_existing.added_list.raw_obj =
            (char *)calloc(update_addrs_existing.added_list.size, sizeof(char));
    }

    if(removed_addr_num > 0) {
        update_addrs_existing.removed_list.raw_obj = (char *)calloc(
            update_addrs_existing.removed_list.size, sizeof(char));
    }

    for(int i = 0; i < added_addr_num; i++) {
        std::cout << "debug, leader tries to copy list: "
                  << m_stageleader_meta.m_added_list[i] << std::endl;
        memcpy(update_addrs_existing.added_list.raw_obj + i * 256,
               m_stageleader_meta.m_added_list[i].c_str(),
               m_stageleader_meta.m_added_list[i].size());
        std::cout << "debug, leader tries to copy, dest addr: "
                  << std::string(update_addrs_existing.added_list.raw_obj +
                                 i * 256)
                  << std::endl;
    }

    for(int i = 0; i < removed_addr_num; i++) {
        memcpy(update_addrs_existing.removed_list.raw_obj + i * 256,
               m_stageleader_meta.m_removed_list[i].c_str(),
               m_stageleader_meta.m_removed_list[i].size());
    }

    // fill in the update addrs all
    update_addrs_in_t update_addrs_all;

    // init
    update_addrs_all.added_list.size = 0;
    update_addrs_all.added_list.raw_obj = NULL;
    update_addrs_all.removed_list.size = 0;
    update_addrs_all.removed_list.raw_obj = NULL;

    // std::unique_ptr<UpdatedMonaList> updatedMonaListAll;
    // When there are process that are added firstly
    // we set the updatedmonalist as all existing mona addrs
    // otherwise, this list is nullptr
    if(m_stageleader_meta.m_first_added_set.size() > 0) {
        spdlog::debug("debug iteration {} m_first_added_set {}", iteration,
                      m_stageleader_meta.m_first_added_set.size());

        update_addrs_all.added_list.size =
            256 * m_stageleader_meta.m_mona_addresses_map.size();
        update_addrs_all.added_list.raw_obj =
            (char *)calloc(update_addrs_all.added_list.size, sizeof(char));

        int offset = 0;
        for(auto &p : m_stageleader_meta.m_mona_addresses_map) {
            // put all mona addr into this
            // added.push_back(p.second);
            // pack the mona addr into the data structs
            memcpy(update_addrs_all.added_list.raw_obj + offset * 256,
                   p.second.c_str(), p.second.size());
            offset++;
        }
        // there is new joined process here
        // updatedMonaListAll = std::make_unique<UpdatedMonaList>(
        // UpdatedMonaList(added, removed));
        // pack the data into the update_addrs
        // the len for every addr is 256
        update_addrs_all.removed_list.size = 0;
        update_addrs_all.removed_list.raw_obj = NULL;
    }

    // the key is the margo addr which we should call based on rpc
    for(auto &p : m_stageleader_meta.m_mona_addresses_map) {
        // if not self
        if(m_stagingcommon_meta.m_margo_self_addr.compare(p.first) == 0) {
            // do not updates to itsself by rpc
            // this is the leader process
            continue;
        }

        spdlog::debug("iteration {} leader send updated list to {} ", iteration,
                      p.first);
        // get the margo endpoints
        hg_return_t hret;
        hg_addr_t worker_hg_addr;
        hg_handle_t handle;
        margo_addr_lookup(server->mid, p.first.c_str(), &worker_hg_addr);

        hret = margo_create(server->mid, worker_hg_addr,
                            server->update_addrs_id, &handle);
        if(hret != HG_SUCCESS) {
            throw std::runtime_error("margo_create() failed with syncview");
        }

        // tl::endpoint workerEndpoint = this->lookup(p.first);

        // if it belongs to the m_first_added_set, then use all the
        // list addr, otherwise, use the current existing mona list
        if(m_stageleader_meta.m_first_added_set.find(p.first) !=
           m_stageleader_meta.m_first_added_set.end()) {
            // just checking
            // when current addr is not in the added set
            // it should not be the monaListA
            if(update_addrs_all.added_list.size != 0) {
                hret = margo_forward(handle, &update_addrs_all);
                if(hret != HG_SUCCESS) {
                    throw std::runtime_error("failed for margo_forward");
                }
                update_addrs_out_t update_addrs_out;
                hret = margo_get_output(handle, &update_addrs_out);
                if(hret != HG_SUCCESS) {
                    throw std::runtime_error("failed for margo_get_output");
                }
                if(update_addrs_out.ret != 0) {
                    throw std::runtime_error("failed for update_addrs_rpc "
                                             "with non zero return value");
                }
                spdlog::debug("iteration {} leader sent updatedMonaListAll ok",
                              iteration);
            } else {
                throw std::runtime_error("update_addrs_all.added_list.size "
                                         "is not supposed to be empty");
            }
        } else {
            // othewise, use the empty case
            hret = margo_forward(handle, &update_addrs_existing);
            if(hret != HG_SUCCESS) {
                throw std::runtime_error("failed for margo_forward");
            }
            update_addrs_out_t update_addrs_out;
            hret = margo_get_output(handle, &update_addrs_out);
            if(hret != HG_SUCCESS) {
                throw std::runtime_error("failed for margo_get_output");
            }
            if(update_addrs_out.ret != 0) {
                throw std::runtime_error("failed for update_addrs_rpc "
                                         "with non zero return value");
            }

            spdlog::debug("iteration {} leader sent updatedMonaList ok",
                          iteration);
        }
    }

    // also update things to itself's common data
    // for other process, we use the udpatedAddr Api
    // since only the leader process call this function
    // or use the stage leader to update the staging common direactly?

    for(int i = 0; i < update_addrs_all.added_list.size / 256; i++) {
        // extract the string rom the added_list
        // then set it into the m_mona_added_list
        // extract str from the raw_obj, we not sure m_added_list is setted by
        // which code snippet

        char tempstr[256];
        // extract the str from the list
        if(update_addrs_all.added_list.raw_obj != NULL) {
            memcpy(tempstr, update_addrs_all.added_list.raw_obj + i * 256, 256);
            std::cout << "debug, leader process insert mona addr: "
                      << std::string(tempstr) << std::endl;
            m_stagingcommon_meta.m_monaaddr_set.insert(tempstr);
        }
    }
    for(int i = 0; i < update_addrs_all.removed_list.size / 256; i++) {

        char tempstr[256];
        // extract the str from the list
        if(update_addrs_all.removed_list.raw_obj != NULL) {
            memcpy(tempstr, update_addrs_all.removed_list.raw_obj + i * 256, 256);
            std::cout << "debug, leader process remove mona addr: " << tempstr
                      << std::endl;
            m_stagingcommon_meta.m_monaaddr_set.erase(tempstr);
        }
    }

    // recreate the mona comm when it is necessary
    // when it is the init stage, or when there are new added/deleted list
    if(m_stageleader_meta.m_first_added_set.size() > 0 ||
       update_addrs_all.added_list.size > 0 ||
       update_addrs_all.removed_list.size > 0) {
        // mona things are actually updated
        std::vector<na_addr_t> m_member_addrs;
        for(auto &p : m_stagingcommon_meta.m_monaaddr_set) {

            na_addr_t addr = NA_ADDR_NULL;
            na_return_t ret =
                mona_addr_lookup(m_stagingcommon_meta.m_mona, p.c_str(), &addr);
            if(ret != NA_SUCCESS) {
                throw std::runtime_error("failed for mona_addr_lookup");
            }

            m_member_addrs.push_back(addr);
        }

        na_return_t ret = mona_comm_create(
            m_stagingcommon_meta.m_mona, m_member_addrs.size(),
            m_member_addrs.data(), &(m_stagingcommon_meta.m_mona_comm));
        if(ret != 0) {
            spdlog::debug("{}: MoNA communicator creation failed",
                          __FUNCTION__);
            throw std::runtime_error("failed to init mona communicator");
        }
        spdlog::debug("recreate the mona_comm, addr size {}",
                      m_member_addrs.size());
    }

    // addrs in added list and removed list has been propagated to all
    // workers
    m_stageleader_meta.m_added_list.clear();
    m_stageleader_meta.m_removed_list.clear();
    // clean the first added vector
    // after this point, there is no first added processes
    m_stageleader_meta.m_first_added_set.clear();

    ABT_mutex_unlock(m_stageleader_meta.m_monaAddrmap_mtx);

    // return current thallium addrs
    // std::vector<std::string> thalliumAddrs;
    obj_t margoAddrsList;

    ABT_mutex_lock(m_stageleader_meta.m_monaAddrmap_mtx);
    margoAddrsList.size = m_stageleader_meta.m_mona_addresses_map.size();
    margoAddrsList.raw_obj =
        (char *)calloc(256 * margoAddrsList.size, sizeof(char));

    int offset = 0;
    for(auto &p : m_stageleader_meta.m_mona_addresses_map) {
        // put current margo addr into the vector and return it //
        // thalliumAddrs.push_back(p.first);
        memcpy(margoAddrsList.raw_obj + offset * 256, p.first.c_str(),
               p.first.size());
        offset++;
    }
    ABT_mutex_unlock(m_stageleader_meta.m_monaAddrmap_mtx);

    // free the update_addrs_existing
    // free the update_addrs_all

    if(update_addrs_existing.added_list.raw_obj != NULL) {
        free(update_addrs_existing.added_list.raw_obj);
    }

    if(update_addrs_existing.removed_list.raw_obj != NULL) {
        free(update_addrs_existing.added_list.raw_obj);
    }

    if(update_addrs_all.added_list.raw_obj != NULL) {
        free(update_addrs_all.added_list.raw_obj);
    }

    if(update_addrs_all.removed_list.raw_obj != NULL) {
        free(update_addrs_all.added_list.raw_obj);
    }

    return margoAddrsList;
}

void Addrmanager_setExpectedNum(int clientProcessNum)
{
    m_stageleader_meta.m_expected_worker_num = clientProcessNum;
    return;
}

void Addrmanager_addkExpectedNum(int k)
{

    m_stageleader_meta.m_expected_worker_num =
        m_stageleader_meta.m_expected_worker_num + k;
    return;
}

void Addrmanager_decreasekExpectedNum(int k)
{
    if(m_stageleader_meta.m_expected_worker_num > k) {
        m_stageleader_meta.m_expected_worker_num =
            m_stageleader_meta.m_expected_worker_num - k;
    } else {
        throw std::runtime_error("the expected number equal or less than zero");
    }
    return;
}

void Addrmanager_addMonaInstance(mona_instance_t mona)
{
    m_stagingcommon_meta.m_mona = mona;
    return;
}

void Addrmanager_updateAddrList(update_addrs_in_t update_addrs_in)
{
    // go through the added list
    // add addr into the list
    int addedListSize = update_addrs_in.added_list.size / 256;
    std::cout << "debug Addrmanager_updateAddrList, addedListSize: "
              << addedListSize << std::endl;
    for(int i = 0; i < addedListSize; i++) {
        char tempstr[256];
        // extract the str from the list
        if(update_addrs_in.added_list.raw_obj != NULL) {
            memcpy(tempstr, update_addrs_in.added_list.raw_obj + i * 256, 256);
            std::cout << "Addrmanager_updateAddrList add server: "
                      << std::string(tempstr) << std::endl;
            m_stagingcommon_meta.m_monaaddr_set.insert(std::string(tempstr));
        }
    }

    // go through the removed list
    // erase the addr from the list
    int removedListSize = update_addrs_in.removed_list.size / 256;
    for(int i = 0; i < removedListSize; i++) {
        char tempstr[256];
        if(update_addrs_in.removed_list.raw_obj != NULL) {
            // extract the str from the list
            memcpy(tempstr, update_addrs_in.removed_list.raw_obj + i * 256,
                   256);
            std::cout << "Addrmanager_updateAddrList remove server: "
                      << std::string(tempstr) << std::endl;
            m_stagingcommon_meta.m_monaaddr_set.erase(std::string(tempstr));
        }
    }

    // update the mona communicator
    if(addedListSize != 0 || removedListSize != 0) {
        // update the mona_comm
        // mona things are actually updated
        std::vector<na_addr_t> m_member_addrs;
        for(auto &p : m_stagingcommon_meta.m_monaaddr_set) {

            na_addr_t addr = NA_ADDR_NULL;
            na_return_t ret =
                mona_addr_lookup(m_stagingcommon_meta.m_mona, p.c_str(), &addr);
            if(ret != NA_SUCCESS) {
                throw std::runtime_error("failed for mona_addr_lookup");
            }

            m_member_addrs.push_back(addr);
        }

        na_return_t ret = mona_comm_create(
            m_stagingcommon_meta.m_mona, m_member_addrs.size(),
            m_member_addrs.data(), &(m_stagingcommon_meta.m_mona_comm));
        if(ret != 0) {
            spdlog::debug("{}: MoNA communicator creation failed",
                          __FUNCTION__);
            throw std::runtime_error("failed to init mona communicator");
        }
        spdlog::debug("recreate the mona_comm, addr size {}",
                      m_member_addrs.size());
    }

    return;
}