#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <netinet/in.h>       //to access ntohs(), ntohl(), htons(), htonl()
#include <netinet/if_ether.h> //to access ethernet header
#include <netinet/ip.h>       //to access IP header
#include <netinet/udp.h>      //to access UDP header
#include <arpa/inet.h>        //to access inet_ntoa(), inet_aton()

#include <pcap.h>             //to access pcap_open_offline(), pcap_next()
#include "util.h"

/* parse_udp_packet()
 *
 * This routine parses a packet, expecting Ethernet, IP, and UDP headers.
 *
 * Extract and print following fields to stdout -
 * DST MAC, SRC MAC, ETHER TYPE, IP HEADER LEN, SRC IP, DST IP,
 * SRC PORT, DST PORT, UDP LEN, PAYLOAD LEN
 *
 * Print Payload in a file with exactly 195 bytes in each line
 *
 * The "ts" argument is the timestamp associated with the packet.
 *
 * Note that "capture_len" is the length of the packet *as captured by the
 * tracing program*, and thus might be less than the full length of the
 * packet.  However, the packet pointer only holds that much data, so
 * we have to be careful not to read beyond it.
 */

void parse_udp_packet(const unsigned char *packet,
                        struct timeval ts,
			            unsigned int capture_len,
                        FILE *out) {
 unsigned char *payload=packet + 42;
 static int i=0; 
 
  while (payload !=packet + capture_len) {
  	fprintf(out,"%c",*payload);
  	payload++;  

  	i++; 
 
   	if ( i %195 == 0) { 
   		fprintf(out, "\n"); 
	} 
  }
} 

/*
for(int i=0; i < payload; i++){
    
   printf("%s",payload); 

}  
*/ 




int main(int argc, char *argv[]) {
	pcap_t *pcap;
	char errbuf[PCAP_ERRBUF_SIZE];
	struct pcap_pkthdr header;
        const unsigned char *packet; 

	if (argc < 2) {

	  fprintf(stderr, "no args, %d", argc);
	  exit(1); 
 
 	}

	pcap = pcap_open_offline(argv[1],errbuf); 
	if (pcap == NULL) { 
		fprintf(stderr, "no args, %s", argv[0]);
		exit(0); 
	}

	while (( packet = pcap_next(pcap,&header)) !=NULL) {
 		parse_udp_packet(packet,header.ts,header.caplen,stdout);
	}
 
	/* Get a handle to the pcap log file*/
 	/* Now just loop through extracting packets as long as we have
	 * some to read.
	 */
    
	return 0;
}

