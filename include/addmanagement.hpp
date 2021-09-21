#ifndef __ADDRMANAGEMENT_H
#define __ADDRMANAGEMENT_H


// this file is both shared by cpp compiler and the c compiler
#ifdef __cplusplus
extern "C" {
#endif

//the function exposed to the c part
int Addrmanager_addMargoAddr(char* margoAddr, char* monaAddr);

void Addrmanager_addLeaderAddr(char *margoAddrstr);

#ifdef __cplusplus
}
#endif


#endif