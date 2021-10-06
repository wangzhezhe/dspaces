#include "addrmanager-client.hpp"
#include <cstring>
#include <vector>
#include <string>
#include <iostream>
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

int ClientAddrGetStageViewSize(){
    return stagingAddrView.size();
}