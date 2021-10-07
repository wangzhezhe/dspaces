#include "addrmanager-client.hpp"
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
std::vector<std::string> stagingAddrView;
std::string LeaderAddr;

void ClientAddrSetLeaderAddr(const char *leaderAddr)
{
    std::string temp(leaderAddr);
    LeaderAddr = temp;
    std::cout << "SetLeaderAddr is " << LeaderAddr << std::endl;
    return;
}

const char *ClientAddrGetLeaderAddr() { return LeaderAddr.c_str(); }

int ClientAddrGetStageViewSize() { return stagingAddrView.size(); }

// update the stagingAddrView based on returned value
void ClientAddrUpdateStageView(syncview_out_t syncview_out)
{
    // extract string from the syncview_out
    // replace the stagingAddrView
    // we assume the server list is not large, so we replace all the data
    // structure everytime otherwise, we may use a map here

    int stageServerNum = syncview_out.addrlist.size / 256;
    stagingAddrView.clear();
    for(int i = 0; i < stageServerNum; i++) {
        char tempstr[256];
        if(syncview_out.addrlist.raw_obj != NULL) {
            // extract the str from the list
            memcpy(tempstr, syncview_out.addrlist.raw_obj + i * 256, 256);
            std::cout << "ClientAddrUpdateStageView add server addr: "
                      << std::string(tempstr) << std::endl;
            stagingAddrView.push_back(std::string(tempstr));
        }
    }
    return;
}