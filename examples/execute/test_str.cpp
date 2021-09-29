
#include <cstring>
#include <iostream>
#include <vector>

int main()
{
    std::vector<std::string> addrs = {"ofi+gni", "ofi+gni", "ofi+gni"};

    printf("addr1 %s\n addr2 %s\n addr3 %s\n", addrs[0].c_str(),
           addrs[0].c_str(), addrs[0].c_str());
    
    //evey character is 0
    char *newaddrs = (char *)calloc(256 * 3, sizeof(char));
    // the strlen does not contains the termination character
    // the c_str does not contains the termination
    int len = strlen(addrs[0].c_str());
    printf("size of strlen %d\n", len);
    // memcpy(addrs, addr1, len);
    // memcpy(addrs + 256, addr2, len);
    // memcpy(addrs + 256 * 2, addr3, len);

    // printf("new addr1 %s\nnew addr2 %s\nnew addr3 %s\n", addrs, addrs + 256,
    //       addrs + 256 * 2);

    newaddrs[0] = 'a';
    newaddrs[1] = 'b';
    newaddrs[2] = 'c';
    newaddrs[5] = 'd';
    printf("newaddrs %s\n", newaddrs);

    free(newaddrs);
}