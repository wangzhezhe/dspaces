#include "addmanagement.hpp"
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
bool LeaderProcess = false;

// put margo addr and mona addr into the current addr manager
int Addrmanager_addMargoAddr(char *margoAddrstr, char *monaAddrstr)
{
    std::string margoAddr(margoAddrstr);
    std::string monaAddr(monaAddrstr);

    // if added, this is a new process, we create a new id, return its id
    // create uuid, put it into the map, return the id
    std::cout << "RPC addMargoAddr for server: " << margoAddr << std::endl;

    ABT_mutex_lock(m_stageleader_meta.m_monaAddrmap_mtx);
    // if the current addr is not stored into the map
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
obj_t Addrmanager_syncview(dspaces_provider_t server, int &iteration,
                           int &clientProcessNum)
{
    if(this->m_stageleader_meta->m_expected_worker_num == -1) {
        throw std::runtime_error(
            "the expected worker number (including leader) is not updated yet");
    }
    spdlog::debug("sync is called for iteration {}", iteration);

    // check if current server is leader
    if(this->m_stagecommon_meta->m_ifleader == false) {
        throw std::runtime_error(
            "sync is supposed to be called for the leader process");
    }

    // client and server already sync (the client process number is as
    // expected), do nothing
    {
        std::lock_guard<tl::mutex> lock(
            this->m_stageleader_meta->m_monaAddrmap_mtx);
        // we should use the expected client process number
        // instead of the current process number
        // if there is updates for the client, this value should also updated
        // one benifits of using the old process number is that
        // we only do the actual sync when server is actaully loaded, it might
        // need long time to load if (clientProcessNum ==
        // this->m_stageleader_meta->m_mona_addresses_map.size()) compared with
        // the expected number not the actual registered number by this way, we
        // wait wait the addr to be synced for next iteration
        if(clientProcessNum ==
           this->m_stageleader_meta->m_expected_worker_num) {
            spdlog::debug("iteration {} client process number equals to server",
                          iteration);

            // the returned list is empty in this case
            obj_t addrList;
            addrList.size = 0;
            addrList.raw_obj = NULL;

            return (addrList);
        }
    }

    // if the current one is the leader process
    // caculate difference between what actual stored
    // and the number we expected
    // if these two number do not equal with each other, we just wait here
    while(this->m_stageleader_meta->addrDiff()) {
        // there is still process that do not update its addr

        spdlog::debug("wait, current expected process {} addr map size {}",
                      this->m_stageleader_meta->m_expected_worker_num,
                      this->m_stageleader_meta->m_mona_addresses_map.size());
        usleep(100000);
    }

    // if expected process equals to the actual one, the leader owns the latest
    // view, it propagate this view to all members
    // it ranges the map and call updateMonaAddrList
    // TODO, expose the updateMonaAddrList for each processes
    tl::remote_procedure updateMonaAddrListRPC =
        // send req to workers
        this->get_engine().define("updateMonaAddrList");
    {
        std::lock_guard<tl::mutex> lock(
            this->m_stageleader_meta->m_monaAddrmap_mtx);
        // maybe just create a snap shot of the current addr instead of using a
        // large critical region
        // TODO, if the new joined and it is the first time
        spdlog::info("iteration {} m_added_list size {} m_removed_list size {}",
                     iteration, this->m_stageleader_meta->m_added_list.size(),
                     this->m_stageleader_meta->m_removed_list.size());

        //UpdatedMonaList updatedMonaList(
        //    this->m_stageleader_meta->m_added_list,
        //    this->m_stageleader_meta->m_removed_list);
        
        //create the updatedMonaList structure
        
        std::unique_ptr<UpdatedMonaList> updatedMonaListAll;
        // When there are process that are added firstly
        // we set the updatedmonalist as all existing mona addrs
        // otherwise, this list is nullptr
        if(this->m_stageleader_meta->m_first_added_set.size() > 0) {
            spdlog::debug("debug iteration {} m_first_added_set {}", iteration,
                          this->m_stageleader_meta->m_first_added_set.size());
            std::vector<std::string> added;
            // this is empty
            std::vector<std::string> removed;

            for(auto &p : this->m_stageleader_meta->m_mona_addresses_map) {
                // put all mona addr into this
                added.push_back(p.second);
            }
            // there is new joined process here
            updatedMonaListAll = std::make_unique<UpdatedMonaList>(
                UpdatedMonaList(added, removed));
        }

        // the key is the thallium addr which we should call based on rpc
        for(auto &p : this->m_stageleader_meta->m_mona_addresses_map) {
            // if not self
            if(this->m_stagecommon_meta->m_thallium_self_addr.compare(
                   p.first) == 0) {
                // do not updates to itsself
                continue;
            }

            spdlog::debug("iteration {} leader send updated list to {} ",
                          iteration, p.first);

            tl::endpoint workerEndpoint = this->lookup(p.first);

            // TODO if it belongs to the m_first_added_set, then use all the
            // list addr otherwise, use the common monaList

            if(this->m_stageleader_meta->m_first_added_set.find(p.first) !=
               this->m_stageleader_meta->m_first_added_set.end()) {
                // just checking
                // when current addr is not in the added set
                // it should not be the monaListA
                if(updatedMonaListAll.get() != nullptr) {
                    // use the MonaListAll in this case
                    int result = updateMonaAddrListRPC.on(workerEndpoint)(
                        *(updatedMonaListAll.get()));
                    if(result != 0) {
                        throw std::runtime_error("failed to notify to worker " +
                                                 p.first);
                    }
                    spdlog::debug(
                        "iteration {} leader sent updatedMonaListAll ok",
                        iteration);
                } else {
                    throw std::runtime_error(
                        "updatedMonaListAll is not supposed to be empty");
                }
            } else {
                // TODO use async call here
                int result =
                    updateMonaAddrListRPC.on(workerEndpoint)(updatedMonaList);
                if(result != 0) {
                    throw std::runtime_error("failed to notify to worker " +
                                             p.first);
                }
                spdlog::debug("iteration {} leader sent updatedMonaList ok",
                              iteration);
            }
        }

        // also update things to itself's common data
        for(int i = 0; i < updatedMonaList.m_mona_added_list.size(); i++) {
            this->m_stagecommon_meta->m_monaaddr_set.insert(
                updatedMonaList.m_mona_added_list[i]);
        }

        for(int i = 0; i < updatedMonaList.m_mona_remove_list.size(); i++) {
            this->m_stagecommon_meta->m_monaaddr_set.erase(
                updatedMonaList.m_mona_remove_list[i]);
        }

        // recreate the mona comm when it is necessary
        if(this->m_stageleader_meta->m_first_added_set.size() > 0 ||
           updatedMonaList.m_mona_added_list.size() > 0 ||
           updatedMonaList.m_mona_remove_list.size() > 0) {
            // mona things are actually updated
            std::vector<na_addr_t> m_member_addrs;
            for(auto &p : this->m_stagecommon_meta->m_monaaddr_set) {

                na_addr_t addr = NA_ADDR_NULL;
                na_return_t ret = mona_addr_lookup(
                    this->m_stagecommon_meta->m_mona, p.c_str(), &addr);
                if(ret != NA_SUCCESS) {
                    throw std::runtime_error("failed for mona_addr_lookup");
                }

                m_member_addrs.push_back(addr);
            }

            na_return_t ret =
                mona_comm_create(this->m_stagecommon_meta->m_mona,
                                 m_member_addrs.size(), m_member_addrs.data(),
                                 &(this->m_stagecommon_meta->m_mona_comm));
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
        this->m_stageleader_meta->m_added_list.clear();
        this->m_stageleader_meta->m_removed_list.clear();
        // clean the first added vector
        // after this point, there is no first added processes
        this->m_stageleader_meta->m_first_added_set.clear();
    }

    // return current thallium addrs
    std::vector<std::string> thalliumAddrs;

    {
        std::lock_guard<tl::mutex> lock(
            this->m_stageleader_meta->m_monaAddrmap_mtx);
        for(auto &p : this->m_stageleader_meta->m_mona_addresses_map) {
            // put current thallium addr into the vector and return it to the
            // client
            thalliumAddrs.push_back(p.first);
        }
    }
    req.respond(thalliumAddrs);
}

void Addrmanager_setExpectedNum(dspaces_provider_t server, int &iteration,
                                int &clientProcessNum)
{
    StagingLeaderMeta.m_expected_worker_num = clientProcessNum;
    return;
}

void Addrmanager_addkExpectedNum(int k)
{

    StagingLeaderMeta.m_expected_worker_num =
        StagingLeaderMeta.m_expected_worker_num + k;
    return;
}

void Addrmanager_decreasekExpectedNum(int k)
{
    if(StagingLeaderMeta.m_expected_worker_num > k) {
        StagingLeaderMeta.m_expected_worker_num =
            StagingLeaderMeta.m_expected_worker_num - k;
    } else {
        throw std::runtime_error("the expected number equal or less than zero");
    }
    return;
}