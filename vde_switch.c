#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "vsconf.h"

int main(int argc, char **argv)
{
    int conf;
    if(conf = configure("test.conf") != 0) {
        printf("Error reading configuration file\n");
        return 1;
    }

    

    const int* num_ports = (int*)get("PORTS");
    if(num_ports == NULL){
        printf("num_ports is NULL\n");
    } else 
        printf("num_ports: %d\n", *num_ports);

    int* dev_mode = (int*)get("VLAN_CONF.DEV");
    if(dev_mode == NULL){
        printf("DEV is NULL\n");
    }
    else
        printf("dev_mode: %d\n", *dev_mode);

    // const char* mac_mode = (char*)get("MAC");
    // if(mac_mode == NULL){
    //     printf("Invalid mac mode\n");
    //     exit(1);
    // }
    
    // if(strcmp(mac_mode, "random") == 0){
    //     printf("mac mode is set to random\n");
    // } else {
    //     printf("Invalid mac mode\n");
    // }


    printf("Hello, World!\n");
    return 0;
}