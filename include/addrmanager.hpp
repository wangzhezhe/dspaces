#ifndef __ADDRMANAGEMENT_H
#define __ADDRMANAGEMENT_H


#include "dspaces-server.h"
#include "ss_data.h"
#include <mona.h>
#include <mona-coll.h>

// this file is both shared by cpp compiler and the c compiler
#ifdef __cplusplus
extern "C" {
#endif

//the function exposed to the c part
int Addrmanager_addMargoAddr(char* margoAddr, char* monaAddr);

void Addrmanager_addLeaderAddr(char *margoAddrstr);

obj_t Addrmanager_syncview(dspaces_provider_t server, int iteration,
                           int clientProcessNum);

void Addrmanager_addMonaInstance(mona_instance_t mona);

#ifdef __cplusplus
}
#endif


#endif