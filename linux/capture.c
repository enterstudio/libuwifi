/* horst - Highly Optimized Radio Scanning Tool
 *
 * Copyright (C) 2005-2016 Bruno Randolf (br1@einfach.org)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <err.h>

#include "capture.h"
#include "util.h"
#include "platform.h"


/*
 *  Get the hardware type of the given interface as ARPHRD_xxx constant.
 */
int device_get_hwinfo(int fd, char* ifname)
{
	struct ifreq ifr;

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));

	if (ioctl(fd, SIOCGIFHWADDR, &ifr) < 0) {
		printlog("Could not get arptype for '%s'", ifname);
		return -1;
	}
	DEBUG("ARPTYPE %d\n", ifr.ifr_hwaddr.sa_family);
	return ifr.ifr_hwaddr.sa_family;
}

int get_mac_address(char* ifname, unsigned char* mac)
{
	struct ifreq ifr;
	int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
	if (sock == -1) {
		printlog("MAC addr socket failed");
		return 0;
	}

	strcpy(ifr.ifr_name, ifname);
	if (ioctl(sock, SIOCGIFHWADDR, &ifr) == -1) {
		printlog("MAC addr ioctl failed for '%s'", ifname);
		close(sock);
		return 0;
	}

	memcpy(mac, ifr.ifr_hwaddr.sa_data, 6);
	close(sock);
	return 1;
}

void set_receive_buffer(int fd, int sockbufsize)
{
	int ret;

	/* the maximum allowed value is set by the rmem_max sysctl */
	FILE* PF = fopen("/proc/sys/net/core/rmem_max", "w");
	fprintf(PF, "%d", sockbufsize);
	fclose(PF);

	ret = setsockopt (fd, SOL_SOCKET, SO_RCVBUF, &sockbufsize, sizeof(sockbufsize));
	if (ret != 0)
		err(1, "setsockopt failed");

#if DO_DEBUG
	socklen_t size = sizeof(sockbufsize);
	sockbufsize = 0;
	ret = getsockopt(fd, SOL_SOCKET, SO_RCVBUF, &sockbufsize, &size);
	if (ret != 0)
		err(1, "getsockopt failed");
	DEBUG("socket receive buffer size %d\n", sockbufsize);
#endif
}

int open_packet_socket(char* devname)
{
	int ret;
	int mon_fd;
	int ifindex;
	struct sockaddr_ll sall;

	mon_fd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if (mon_fd < 0) {
		printlog("Could not create packet socket! Please run horst as root!");
		exit(1);
	}

	/* bind only to one interface */
	ifindex = if_nametoindex(devname);
	if (ifindex < 0)
		return -1;

	memset(&sall, 0, sizeof(struct sockaddr_ll));
	sall.sll_ifindex = ifindex;
	sall.sll_family = AF_PACKET;
	sall.sll_protocol = htons(ETH_P_ALL);

	ret = bind(mon_fd, (struct sockaddr*)&sall, sizeof(sall));
	if (ret != 0)
		err(1, "bind failed");

	return mon_fd;
}

ssize_t recv_packet(int fd, unsigned char* buffer, size_t bufsize)
{
	return recv(fd, buffer, bufsize, MSG_DONTWAIT);
}

void close_packet_socket(int fd)
{
	if (fd > 0)
		close(fd);
}
