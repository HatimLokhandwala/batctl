/*  Copyright (C) 2011-2015  P.D. Buchan (pdbuchan@yahoo.com)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// Send an IPv4 TCP packet via raw socket at the link layer (ethernet frame).
// Need to have destination MAC address.
// Values set for SYN packet, no TCP options data.

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>           // close()
#include <string.h>           // strcpy, memset(), and memcpy()

#include <netdb.h>            // struct addrinfo
#include <sys/types.h>        // needed for socket(), uint8_t, uint16_t, uint32_t
#include <sys/socket.h>       // needed for socket()
#include <netinet/in.h>       // IPPROTO_TCP, INET_ADDRSTRLEN
#include <netinet/ip.h>       // struct ip and IP_MAXPACKET (which is 65535)
#define __FAVOR_BSD           // Use BSD format of tcp header
#include <netinet/tcp.h>      // struct tcphdr
#include <arpa/inet.h>        // inet_pton() and inet_ntop()
#include <sys/ioctl.h>        // macro ioctl is defined
#include <bits/ioctls.h>      // defines values for argument "request" of ioctl.
#include <net/if.h>           // struct ifreq
#include <linux/if_ether.h>   // ETH_P_IP = 0x0800, ETH_P_IPV6 = 0x86DD
#include <linux/if_packet.h>  // struct sockaddr_ll (see man 7 packet)
#include <net/ethernet.h>

#include <errno.h>            // errno, perror()


#include "bat-hosts.h"
#include "functions.h"


#include "tcpSender.h"
// Define some constants.
#define ETH_HDRLEN 14  // Ethernet header length
#define IP4_HDRLEN 20  // IPv4 header length
#define TCP_HDRLEN 20  // TCP header length, excludes options data

// Function prototypes
uint16_t checksum (uint16_t *, int);
uint16_t tcp4_checksum (struct ip, struct tcphdr);
char *allocate_strmem (int);
uint8_t *allocate_ustrmem (int);
int *allocate_intmem (int);

int convert(char c){
	if(c>=48&&c<=57){
		return c -48;
	}
	else {
		return c-87;
	}
}

//destination MAC Address in string format
int tcpSender(char *destMacString,char *destIp){
	char *intf="wlan0";
	int sockid,status,frameLength,bytes;
	struct ifreq ifr;
	
	uint8_t *srcMac;
	uint8_t *dstMac;
	uint8_t *etherFrame;
	char *srcIp;
	int *ipFlags,*tcpFlags;

	struct ip iphdr;
	struct tcphdr tcphdr;
	struct sockaddr_ll device;
	srcMac = (uint8_t *)malloc(6 * sizeof(uint8_t));
	dstMac = (uint8_t *)malloc(6 * sizeof(uint8_t));
	ipFlags = (int *)malloc(4*sizeof(int));
	tcpFlags = (int *)malloc(8*sizeof(int));
	etherFrame = (uint8_t *)malloc(IP_MAXPACKET * sizeof(uint8_t));
	

	//	srcIp = malloc()
	sockid =  socket(PF_PACKET, SOCK_RAW, htons (ETH_P_ALL));
	if(sockid<0){
		printf("Socket failed to get descriptor\n");
	}
	memset(&ifr,0,sizeof(ifr));
	snprintf (ifr.ifr_name, sizeof (ifr.ifr_name), "%s", intf);
	if (ioctl (sockid, SIOCGIFHWADDR, &ifr) < 0) {
		printf (" Failed to get source MAC address\n ");
		return 0;
	}
	
//Better to print in integer format using uint since here ASCII characters are printed.
//	printf("Source Mac Address %d\n", ifr.ifr_hwaddr.sa_data[0]);

	memcpy (srcMac, ifr.ifr_hwaddr.sa_data, 6 * sizeof (uint8_t));
	printf("Source Mac Address in uint format :: \n");

	for(int i=0;i<6;i++){
		if(i==5){
			printf("%02x\n",srcMac[i]);break;
		}
		printf("%02x:",srcMac[i]);
	}

	//	bat_hosts_init(0);
	//	batHost = bat_hosts_find_by_name(destString);//get the batHost from the table
	//dstMac = &batHost->mac_addr; 
	//if(!dstMac){printf("Could not resolve MAC Address");goto close

	char c[6];
	c[0]=convert(destMacString[0])*16  + convert(destMacString[1]);
	c[1]=convert(destMacString[3])*16  + convert(destMacString[4]);
	c[2]=convert(destMacString[6])*16  + convert(destMacString[7]);
	c[3]=convert(destMacString[9])*16  + convert(destMacString[10]);
	c[4]=convert(destMacString[12])*16 + convert(destMacString[13]);
	c[5]=convert(destMacString[15])*16 + convert(destMacString[16]);
	/*for(int i=0;i<6;i++){
	printf("%d",c[i]);}
*/
//	printf("Destination Mac Address %c\n",destMacString[2]);
	memcpy(dstMac,c,6*sizeof(uint8_t));
	printf("Destination Mac Address in uint format :: \n");
	for(int i=0;i<6;i++){
		if(i==5){
			printf("%02x\n",dstMac[i]);break;
		}
		printf("%02x:",dstMac[i]);
	}
	
	int ipId = socket(AF_INET,SOCK_DGRAM,0);
	struct ifreq ipF;
	ipF.ifr_addr.sa_family = AF_INET;
	strncpy(ipF.ifr_name,"wlan0",IFNAMSIZ-1);
	ioctl(ipId,SIOCGIFADDR,&ipF);
	srcIp =  inet_ntoa(((struct sockaddr_in *)&ipF.ifr_addr)->sin_addr);
	printf("Source IP address %s Destination IP Address %s \n",srcIp,destIp);

	// IPv4 header

	// IPv4 header length (4 bits): Number of 32-bit words in header = 5
	iphdr.ip_hl = IP4_HDRLEN / sizeof (uint32_t);
	
	iphdr.ip_v = 4; //version : 4 bits
	
	// Type of service (8 bits)
	iphdr.ip_tos = 0; //type of service : 8 bits
	
	// Total length of datagram (16 bits): IP header + TCP header
	iphdr.ip_len = htons (IP4_HDRLEN + TCP_HDRLEN);
	
	// ID sequence number (16 bits): unused, since single datagram
	iphdr.ip_id = htons (0);
	
	// Flags, and Fragmentation offset (3, 13 bits): 0 since single datagram
	
	ipFlags[0] = 0;	  // Zero (1 bit)
	ipFlags[1] = 0; 	  // Do not fragment flag (1 bit)
	ipFlags[2] = 0;	  // More fragments following flag (1 bit)
	ipFlags[3] = 0; 	  // Fragmentation offset (13 bits)
	iphdr.ip_off = htons ((ipFlags[0] << 15)+ (ipFlags[1] << 14) + (ipFlags[2] << 13) + ipFlags[3]);
	iphdr.ip_ttl = 255; 	// Time-to-Live (8 bits): default to maximum value
	
	
	iphdr.ip_p = IPPROTO_TCP; 	 // Transport layer protocol (8 bits): 6 for TCP

		  // IPv4 header checksum (16 bits): set to 0 when calculating checksum
	iphdr.ip_sum = 0;
	iphdr.ip_sum = checksum ((uint16_t *) &iphdr, IP4_HDRLEN);
	
	// Source IPv4 address (32 bits)
	if ((status = inet_pton (AF_INET, srcIp, &(iphdr.ip_src))) != 1) {
		fprintf (stderr, "inet_pton() failed.\nError message: %s", strerror (status));
		 //(EXIT_FAILURE);
	}
	
	// Destination IPv4 address (32 bits)
	if ((status = inet_pton (AF_INET, destIp, &(iphdr.ip_dst))) != 1) {
		 fprintf(stderr, "inet_pton() failed.\nError message: %s", strerror (status));
		//exit (EXIT_FAILURE);
	}
	

	// TCP header

	tcphdr.th_sport = htons (60);   // Source port number (16 bits)
	tcphdr.th_dport = htons (80);   // Destination port number (16 bits)
	tcphdr.th_seq = htonl (0); 	// Sequence number (32 bits)
	tcphdr.th_ack = htonl (0);	// Acknowledgement number (32 bits): 0 in first packet of SYN/ACK process
	tcphdr.th_x2 = 0;	// Reserved (4 bits): should be 0
	tcphdr.th_off = TCP_HDRLEN / 4;	// Data offset (4 bits): size of TCP header in 32-bit words

	// Flags (8 bits)
	
	tcpFlags[0] = 0; 	// FIN flag (1 bit)
	tcpFlags[1] = 1;	// SYN flag (1 bit): set to 1
	tcpFlags[2] = 0;	// RST flag (1 bit)
	tcpFlags[3] = 0;	// PSH flag (1 bit)
	tcpFlags[4] = 0;	// ACK flag (1 bit)
	tcpFlags[5] = 0;	// URG flag (1 bit)
	tcpFlags[6] = 0;	// ECE flag (1 bit)
	
	// CWR flag (1 bit)
	tcpFlags[7] = 0;

	tcphdr.th_flags = 0;
	for (int i=0; i<8; i++) {
		tcphdr.th_flags += (tcpFlags[i] << i);
	}

	// Window size (16 bits)
	tcphdr.th_win = htons (65535);

	// TCP checksum (16 bits)
	tcphdr.th_sum = tcp4_checksum (iphdr, tcphdr);
	// Urgent pointer (16 bits): 0 (only valid if URG flag is set)
	tcphdr.th_urp = htons (0);


	// Fill out ethernet frame header.

	// Ethernet frame length = ethernet header (MAC + MAC + ethernet type) + ethernet data (IP header + TCP header)
	frameLength = 6 + 6 + 2 + IP4_HDRLEN + TCP_HDRLEN;

	// Destination and Source MAC addresses
	memcpy (etherFrame, dstMac, 6 * sizeof (uint8_t));
	memcpy (etherFrame + 6, srcMac, 6 * sizeof (uint8_t));

	// Next is ethernet type code (ETH_P_IP for IPv4).
	etherFrame[12] = ETH_P_IP / 256;
	etherFrame[13] = ETH_P_IP % 256;

	// Next is ethernet frame data (IPv4 header + TCP header).

	// IPv4 header
	memcpy (etherFrame + ETH_HDRLEN, &iphdr, IP4_HDRLEN * sizeof (uint8_t));

	// TCP header
	memcpy (etherFrame + ETH_HDRLEN + IP4_HDRLEN, &tcphdr, TCP_HDRLEN * sizeof (uint8_t));
	printf("Ethernet Packet with IP and TCP payload created\n");
	
	// Submit request for a raw socket descriptor.
	if ((sockid = socket (PF_PACKET, SOCK_RAW, htons (ETH_P_ALL))) < 0) {
		printf ("socket request for data sending failed ");
		exit (EXIT_FAILURE);
	}

	memset (&device, 0, sizeof (device));

	if ((device.sll_ifindex = if_nametoindex (intf)) == 0) {
		printf ("if_nametoindex() failed to obtain interface index corresponding to %s ",intf);
		exit (EXIT_FAILURE);
	}
	printf ("Index for interface %s is %i\n", intf, device.sll_ifindex);

  // Send ethernet frame to socket.
	
	bytes = sendto(sockid, etherFrame, frameLength, 0, (struct sockaddr *) &device, sizeof (device));
	if(bytes<=0){
		printf("send to () %s failed",dstMac);
	}
  	else{
		printf("%d bytes of Data Send succesfully SourceIP:%s, DestinationIP:%s, SourceMAC:%s, DestinationMAC:%s \n",bytes,srcIp, destIp,srcMac,dstMac);
	}

	return 0;

}

// Note that the internet checksum does not preclude collisions.
uint16_t
checksum (uint16_t *addr, int len)
{
  int count = len;
  register uint32_t sum = 0;
  uint16_t answer = 0;

  // Sum up 2-byte values until none or only one byte left.
  while (count > 1) {
    sum += *(addr++);
    count -= 2;
  }

  // Add left-over byte, if any.
  if (count > 0) {
    sum += *(uint8_t *) addr;
  }

  // Fold 32-bit sum into 16 bits; we lose information by doing this,
  // increasing the chances of a collision.
  // sum = (lower 16 bits) + (upper 16 bits shifted right 16 bits)
  while (sum >> 16) {
    sum = (sum & 0xffff) + (sum >> 16);
  }

  // Checksum is one's compliment of sum.
  answer = ~sum;

  return (answer);
}

// Build IPv4 TCP pseudo-header and call checksum function.
uint16_t
tcp4_checksum (struct ip iphdr, struct tcphdr tcphdr)
{
  uint16_t svalue;
  char buf[IP_MAXPACKET], cvalue;
  char *ptr;
  int chksumlen = 0;

  ptr = &buf[0];  // ptr points to beginning of buffer buf

  // Copy source IP address into buf (32 bits)
  memcpy (ptr, &iphdr.ip_src.s_addr, sizeof (iphdr.ip_src.s_addr));
  ptr += sizeof (iphdr.ip_src.s_addr);
  chksumlen += sizeof (iphdr.ip_src.s_addr);

  // Copy destination IP address into buf (32 bits)
  memcpy (ptr, &iphdr.ip_dst.s_addr, sizeof (iphdr.ip_dst.s_addr));
  ptr += sizeof (iphdr.ip_dst.s_addr);
  chksumlen += sizeof (iphdr.ip_dst.s_addr);

  // Copy zero field to buf (8 bits)
  *ptr = 0; ptr++;
  chksumlen += 1;

  // Copy transport layer protocol to buf (8 bits)
  memcpy (ptr, &iphdr.ip_p, sizeof (iphdr.ip_p));
  ptr += sizeof (iphdr.ip_p);
  chksumlen += sizeof (iphdr.ip_p);

  // Copy TCP length to buf (16 bits)
  svalue = htons (sizeof (tcphdr));
  memcpy (ptr, &svalue, sizeof (svalue));
  ptr += sizeof (svalue);
  chksumlen += sizeof (svalue);

  // Copy TCP source port to buf (16 bits)
  memcpy (ptr, &tcphdr.th_sport, sizeof (tcphdr.th_sport));
  ptr += sizeof (tcphdr.th_sport);
  chksumlen += sizeof (tcphdr.th_sport);

  // Copy TCP destination port to buf (16 bits)
  memcpy (ptr, &tcphdr.th_dport, sizeof (tcphdr.th_dport));
  ptr += sizeof (tcphdr.th_dport);
  chksumlen += sizeof (tcphdr.th_dport);

  // Copy sequence number to buf (32 bits)
  memcpy (ptr, &tcphdr.th_seq, sizeof (tcphdr.th_seq));
  ptr += sizeof (tcphdr.th_seq);
  chksumlen += sizeof (tcphdr.th_seq);

  // Copy acknowledgement number to buf (32 bits)
  memcpy (ptr, &tcphdr.th_ack, sizeof (tcphdr.th_ack));
  ptr += sizeof (tcphdr.th_ack);
  chksumlen += sizeof (tcphdr.th_ack);

  // Copy data offset to buf (4 bits) and
  // copy reserved bits to buf (4 bits)
  cvalue = (tcphdr.th_off << 4) + tcphdr.th_x2;
  memcpy (ptr, &cvalue, sizeof (cvalue));
  ptr += sizeof (cvalue);
  chksumlen += sizeof (cvalue);

  // Copy TCP flags to buf (8 bits)
  memcpy (ptr, &tcphdr.th_flags, sizeof (tcphdr.th_flags));
  ptr += sizeof (tcphdr.th_flags);
  chksumlen += sizeof (tcphdr.th_flags);

  // Copy TCP window size to buf (16 bits)
  memcpy (ptr, &tcphdr.th_win, sizeof (tcphdr.th_win));
  ptr += sizeof (tcphdr.th_win);
  chksumlen += sizeof (tcphdr.th_win);

  // Copy TCP checksum to buf (16 bits)
  // Zero, since we don't know it yet
  *ptr = 0; ptr++;
  *ptr = 0; ptr++;
  chksumlen += 2;

  // Copy urgent pointer to buf (16 bits)
  memcpy (ptr, &tcphdr.th_urp, sizeof (tcphdr.th_urp));
  ptr += sizeof (tcphdr.th_urp);
  chksumlen += sizeof (tcphdr.th_urp);

  return checksum ((uint16_t *) buf, chksumlen);
}













