


#include <netinet/in.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/if_ether.h>
#include <net/ethernet.h>
#include <sys/types.h>
#include "dataSend.h"
#include "main.h"
#include "neighbourRead.h"
#include "bat-hosts.h"
#include "functions.h"
#include "packet.h"
#include "debugfs.h"
#include "tcpSender.h"
//char is_aborted = 0;

/*
static void ping_usage(void)
{
	fprintf(stderr, "Usage: batctl [options] ping [parameters] mac|bat-host|host_name|IPv4_address \n");
	fprintf(stderr, "parameters:\n");
	fprintf(stderr, " \t -c ping packet count \n");
	fprintf(stderr, " \t -h print this help\n");
	fprintf(stderr, " \t -i interval in seconds\n");
	fprintf(stderr, " \t -t timeout in seconds\n");
	fprintf(stderr, " \t -R record route\n");
	fprintf(stderr, " \t -T don't try to translate mac to originator address\n");
}*/

/*static void sig_handler(int sig)
{
	switch (sig) {
	case SIGINT:
	case SIGTERM:
		is_aborted = 1;
		break;
	default:
		break;
	}
}*/
char *batString = "bat0";
int dataSend(char *mesh_iface, int argc, char **argv){

	char *destIpString,*destMacString;
	struct ether_addr *dstMac;
	struct bat_host *batHost;
	printf("-->DATASEND Interface :  %s argCount: %d Command: %s\n",mesh_iface, argc, argv[0]);
	int code = openFile();
	if(argc<=2){
		printf("No destination specified \n ");
		return 0;
	}	


	if (code==1){
		printf("%s interface not yet loaded into the kernel",batString);
	}
	else{
		int rCode = readFile();
		//Add a while loop here to check for neighbors
		if(rCode==1){
				printf("No Neighbors in the vicinity of the node\n");
		}	
		else{
			printf("There are BATMAN nodes in the vicinity	\n");
	
			destIpString = argv[2]; //holds the destination IP or MAC address
			printf("Destination IP : %s\n",destIpString);
			bat_hosts_init(0);
			batHost = bat_hosts_find_by_name(destIpString);
			dstMac = &batHost->mac_addr;
			if(!dstMac){
				dstMac = resolve_mac(destIpString);
					if(!dstMac){
						printf("Could not resolve MAC address or no route to the destination\n");
						goto close;
					}
				}

			destMacString = ether_ntoa_long(dstMac); //holds the destination MAC address
			printf("Given Destination %s has been resolved to following MAC address %s\n",destIpString,destMacString);
			tcpSender(destMacString,destIpString);
//			getNeighbourTable();
		}

	}

close:
	closeFile();
	return 0;
}




