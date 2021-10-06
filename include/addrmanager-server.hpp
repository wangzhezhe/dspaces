#ifndef __ADDRMANAGERSERVER_H
#define __ADDRMANAGERSERVER_H


#include "dspaces-server.h"
#include "ss_data.h"
#include <mona.h>
#include <mona-coll.h>

// this file is both shared by cpp compiler and the c compiler

#if defined(__cplusplus)
extern "C" {
#endif

//the function exposed to the c part
int Addrmanager_registerMargoAddr(char* margoAddr, char* monaAddr);

void Addrmanager_addLeaderAddr(char *margoAddrstr);

obj_t Addrmanager_syncview(dspaces_provider_t server, int iteration,
                           int clientProcessNum);

void Addrmanager_addMonaInstance(mona_instance_t mona);

void Addrmanager_setExpectedNum(int clientProcessNum);

void Addrmanager_setLeaderTrue();

void Addrmanager_updateAddrList(update_addrs_in_t update_addrs_in);

void Addrmanager_setLogLevel(char* loglevel);

void Addrmanager_setSelfMargoMona(char *slefMargoAddrstr, char *selfMonaAddrstr);

#if defined(__cplusplus)
}
#endif

#endif
