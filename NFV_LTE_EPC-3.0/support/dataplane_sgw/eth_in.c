#include "ps.h"
#include "ip_in.h"
#include "eth_in.h"
#include "arp.h"
#include "debug.h"

/*----------------------------------------------------------------------------*/


//Change these
#define DP_RAN_IP "169.254.9.3"
#define DP_SINK_IP "169.254.9.104"

int check_ipaddrs(struct iphdr* iph, char src[], char dest[])
{
    //check protocol first,
    char src_buf[80];
    char dest_buf[80];
    uint8_t *t;
    t = (uint8_t *)&iph->saddr;
    sprintf(src_buf, "%u.%u.%u.%u", t[0], t[1], t[2], t[3]);
    t = (uint8_t *)&iph->daddr;
    sprintf(dest_buf, "%u.%u.%u.%u", t[0], t[1], t[2], t[3]);
    if(strlen(src) != 0 && strcmp(src,src_buf) != 0)
        return 0;
    if(strlen(dest) != 0 && strcmp(dest, dest_buf) != 0)
        return 0;
    return 1;

}


int
ProcessPacket(mtcp_manager_t mtcp, const int ifidx, 
		uint32_t cur_ts, unsigned char *pkt_data, int len)
{
	struct ethhdr *ethh = (struct ethhdr *)pkt_data;
	u_short ip_proto = ntohs(ethh->h_proto);
	int ret;
	struct iphdr* iph = (struct iphdr *)(pkt_data + sizeof(struct ethhdr));
	        char src_buf[80];
		    char dest_buf[80];
		    uint8_t *t;
            t = (uint8_t *)&iph->saddr;
    		sprintf(src_buf, "%u.%u.%u.%u", t[0], t[1], t[2], t[3]);
    		t = (uint8_t *)&iph->daddr;
    		sprintf(dest_buf, "%u.%u.%u.%u", t[0], t[1], t[2], t[3]);

#ifdef PKTDUMP
	DumpPacket(mtcp, (char *)pkt_data, len, "IN", ifidx);
#endif

#ifdef NETSTAT
	mtcp->nstat.rx_packets[ifidx]++;
	mtcp->nstat.rx_bytes[ifidx] += len + 24;
#endif /* NETSTAT */

#if 0
	/* ignore mac address which is not for current interface */
	int i;
	for (i = 0; i < 6; i ++) {
		if (ethh->h_dest[i] != CONFIG.eths[ifidx].haddr[i]) {
			return FALSE;
		}
	}
#endif

	if (ip_proto == ETH_P_IP) {
		/* process ipv4 packet */
		if(check_ipaddrs(iph, DP_RAN_IP,DP_SINK_IP ))
		{
			return TRUE;
		}
		if(check_ipaddrs(iph, DP_SINK_IP,DP_RAN_IP ))
		{
			return TRUE;
		}	
		ret = ProcessIPv4Packet(mtcp, cur_ts, ifidx, pkt_data, len);

	} else if (ip_proto == ETH_P_ARP) {
		ProcessARPPacket(mtcp, cur_ts, ifidx, pkt_data, len);
		return TRUE;

	} else {
		//DumpPacket(mtcp, (char *)pkt_data, len, "??", ifidx);
		mtcp->iom->release_pkt(mtcp->ctx, ifidx, pkt_data, len);
		return TRUE;
	}

#ifdef NETSTAT
	if (ret < 0) {
		mtcp->nstat.rx_errors[ifidx]++;
	}
#endif

	return ret;
}
