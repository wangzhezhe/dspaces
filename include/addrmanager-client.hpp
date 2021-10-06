#ifndef __ADDRMANAGERCLIENT_H
#define __ADDRMANAGERCLIENT_H

// this file is both shared by cpp compiler and the c compiler

#if defined(__cplusplus)
extern "C" {
#endif

void ClientAddrSetLeaderAddr(const char *leaderAddr);
const char* ClientAddrGetLeaderAddr();
int ClientAddrGetStageViewSize();

#if defined(__cplusplus)
}
#endif

#endif
