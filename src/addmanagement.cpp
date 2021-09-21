#include "addmanagement.hpp"
#include <abt.h>
#include <iostream>
#include <map>
#include <mona-coll.h>
#include <mona.h>
#include <set>
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
    int m_expected_worker_num = 0;

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
    std::string margoAddr(margoAddrstr);
    m_stagingcommon_meta.m_margo_leader_addr = margoAddr;

    return;
}