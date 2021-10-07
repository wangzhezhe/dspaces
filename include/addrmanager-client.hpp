#ifndef __ADDRMANAGERCLIENT_H
#define __ADDRMANAGERCLIENT_H

#include "ss_data.h"

// this file is both shared by cpp compiler and the c compiler

#if defined(__cplusplus)
extern "C" {
#endif

void ClientAddrSetLeaderAddr(const char *leaderAddr);
const char* ClientAddrGetLeaderAddr();
int ClientAddrGetStageViewSize();
void ClientAddrUpdateStageView(syncview_out_t syncview_out);

#if defined(__cplusplus)
}
#endif

#endif
