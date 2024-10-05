#include "queue.h"
#include "lib.h"
#include "protocols.h"
#include <arpa/inet.h>
#include <string.h>

static struct route_table_entry *rtable;
static int rtable_len;

static struct arp_table_entry *mac_table;
static int mac_table_len;

//Cautarea binara pentru next_hop
struct route_table_entry *get_best_route(uint32_t ip_dest) {
	struct route_table_entry *next_hope = NULL;
	int l = 0, r = rtable_len - 1;
	while (l <= r) {
		int mid = l + (r - l) / 2;

		if (rtable[mid].prefix == (ip_dest & rtable[mid].mask)) {
			if (!next_hope) {
				next_hope = &rtable[mid];
			} else if (ntohl(rtable[mid].mask) > ntohl(next_hope->mask)) {
				next_hope = &rtable[mid];
			}
		}

		if (ntohl(rtable[mid].prefix) <= ntohl(ip_dest)) {
			r = mid - 1;
		} else {
			l = mid + 1;
		}
	}

	return next_hope;
}

//Cautare adresa mac pentru next_hop
struct arp_table_entry *get_mac_entry(uint32_t given_ip) {
	for (int i = 0; i < mac_table_len; i++) {
		if (mac_table[i].ip == given_ip)
			return &mac_table[i];
	}
	return NULL;
}

//Functie de comparare pt quicksort
int compar_table(const void *a, const void *b) {
	struct route_table_entry *r1 = (struct route_table_entry *) a;
	struct route_table_entry *r2 = (struct route_table_entry *) b;
	if (ntohl(r2->prefix) > ntohl(r1->prefix)) {
		return 1;
	} else {
		if (ntohl(r2->prefix < ntohl(r1->prefix))) {
			return -1;
		} else {
			if (ntohl(r2->mask) > ntohl(r1->mask))
				return 1;
			else
				return -1;
		}
	}
}

//Functie de trimitere a unui pachet icmp de tipul host_unrechable
void send_host_unreachable(void *buff, uint32_t rout_ip, int interface) {
	struct ether_header *eth_hdr = (struct ether_header *)buff;
	struct iphdr *ip_hdr = (struct iphdr *)(buff + sizeof(struct ether_header));
	void *new_package = malloc(sizeof(struct ether_header) + sizeof(struct iphdr) + 8 + sizeof(struct iphdr) + 8);
	struct ether_header *new_eth_hdr = (struct ether_header *)new_package;
	struct iphdr *new_ip_hdr = (struct iphdr *)(new_package + sizeof(struct ether_header));
	struct icmphdr *icmp_hdr = (struct icmphdr *)(new_package + sizeof(struct ether_header) + sizeof(struct iphdr));
	memcpy(new_eth_hdr->ether_dhost, eth_hdr->ether_shost, 6);
	memcpy(new_eth_hdr->ether_shost, eth_hdr->ether_dhost, 6);
	new_eth_hdr->ether_type = htons(0x0800);
	new_ip_hdr->daddr = ip_hdr->saddr;
	new_ip_hdr->saddr = rout_ip;
	new_ip_hdr->frag_off = 0;
	new_ip_hdr->tos = 0;
	new_ip_hdr->version = 4;
	new_ip_hdr->ihl = 5;
	new_ip_hdr->id = 1;
	new_ip_hdr->tot_len = htons(sizeof(struct iphdr) + 8 + sizeof(struct iphdr) + 8);
	new_ip_hdr->protocol = 1;
	new_ip_hdr->ttl = 100;
	icmp_hdr->type = 3;
	icmp_hdr->code = 0;
	void *addr = (void *) icmp_hdr;
	addr += 2;
	memset(addr, 0, 6);
	addr += 6;
	memcpy(addr, ip_hdr, sizeof(struct iphdr) + 8);
	icmp_hdr->checksum = htons(checksum((uint16_t *) icmp_hdr, 8 + sizeof(struct iphdr) + 8));
	new_ip_hdr->check = htons(checksum((uint16_t *) new_ip_hdr, new_ip_hdr->tot_len));
	send_to_link(interface, new_package, sizeof(struct ether_header) + sizeof(struct iphdr) + 8 + sizeof(struct iphdr) + 8);
	free(new_package);
}

//Functie de trimitere a unui pachet icmp de tipul no_more_ttl
void send_no_more_ttl(void *buff, uint32_t rout_ip, int interface) {
	struct ether_header *eth_hdr = (struct ether_header *)buff;
	struct iphdr *ip_hdr = (struct iphdr *)(buff + sizeof(struct ether_header));
	void *new_package = malloc(sizeof(struct ether_header) + sizeof(struct iphdr) + 8 + sizeof(struct iphdr) + 8);
	struct ether_header *new_eth_hdr = (struct ether_header *)new_package;
	struct iphdr *new_ip_hdr = (struct iphdr *)(new_package + sizeof(struct ether_header));
	struct icmphdr *icmp_hdr = (struct icmphdr *)(new_package + sizeof(struct ether_header) + sizeof(struct iphdr));
	memcpy(new_eth_hdr->ether_dhost, eth_hdr->ether_shost, 6);
	memcpy(new_eth_hdr->ether_shost, eth_hdr->ether_dhost, 6);
	new_eth_hdr->ether_type = htons(0x0800);
	new_ip_hdr->daddr = ip_hdr->saddr;
	new_ip_hdr->saddr = rout_ip;
	new_ip_hdr->frag_off = 0;
	new_ip_hdr->tos = 0;
	new_ip_hdr->version = 4;
	new_ip_hdr->ihl = 5;
	new_ip_hdr->id = 1;
	new_ip_hdr->tot_len = htons(sizeof(struct iphdr) + 8 + sizeof(struct iphdr) + 8);
	new_ip_hdr->protocol = 1;
	new_ip_hdr->ttl = 100;
	icmp_hdr->type = 11;
	icmp_hdr->code = 0;
	void *addr = (void *) icmp_hdr;
	addr += 2;
	memset(addr, 0, 6);
	addr += 6;
	memcpy(addr, ip_hdr, sizeof(struct iphdr) + 8);
	icmp_hdr->checksum = htons(checksum((uint16_t *) icmp_hdr, 8 + sizeof(struct iphdr) + 8));
	new_ip_hdr->check = htons(checksum((uint16_t *) new_ip_hdr, new_ip_hdr->tot_len));
	send_to_link(interface, new_package, sizeof(struct ether_header) + sizeof(struct iphdr) + 8 + sizeof(struct iphdr) + 8);
	free(new_package);
}

//functie de trimitere a unui pachet arp de tipul arp_request
void send_arp_request(uint32_t ipaddr, int interface, uint32_t sipaddr, uint8_t *smacaddr) {
	void *new_package = malloc(sizeof(struct ether_header) + sizeof(struct arp_header));
	struct ether_header *eth_hdr = (struct ether_header *)new_package;
	struct arp_header *arp_hdr = (struct arp_header *)(new_package + sizeof(struct ether_header));
	uint8_t brod[6];
	for (int i = 0; i < 6; i++)
		brod[i] = 255;
	memcpy(eth_hdr->ether_shost, smacaddr, 6);
	memcpy(eth_hdr->ether_dhost, brod, 6);
	eth_hdr->ether_type = htons(0x0806);

	arp_hdr->htype = htons(1);
	arp_hdr->ptype = htons(0x0800);
	arp_hdr->hlen = 6;
	arp_hdr->plen = 4;
	arp_hdr->op = htons(1);
	memcpy(arp_hdr->sha, smacaddr, 6);
	arp_hdr->spa = sipaddr;
	memset(arp_hdr->tha, 0, 6);
	arp_hdr->tpa = ipaddr;

	send_to_link(interface, new_package, sizeof(struct ether_header) + sizeof(struct arp_header));
	free(new_package);
}

int main(int argc, char *argv[])
{
	char buf[MAX_PACKET_LEN];

	// Do not modify this line
	init(argc - 2, argv + 2);

	rtable = malloc(80000 * sizeof(struct route_table_entry));
	DIE(rtable == NULL, "memory");
	rtable_len = read_rtable(argv[1], rtable);

	qsort(rtable, rtable_len, sizeof(struct route_table_entry), compar_table);

	mac_table = malloc(sizeof(struct arp_table_entry) * 20);
	DIE(mac_table == NULL, "memory");
	mac_table_len = 0;
	//parse_arp_table("arp_table.txt", mac_table);

	struct queue *package_queue = queue_create();
	int queue_length = 0;

	while (1) {

		int interface;
		size_t len;

		interface = recv_from_any_link(buf, &len);
		DIE(interface < 0, "recv_from_any_links");

		struct ether_header *eth_hdr = (struct ether_header *) buf;
		/* Note that packets received are in network order,
		any header field which has more than 1 byte will need to be conerted to
		host order. For example, ntohs(eth_hdr->ether_type). The oposite is needed when
		sending a packet on the link, */
		printf("We got a pachage\n");

		if (len < sizeof(struct ether_header))
			continue;

		uint32_t rout_ip;
		uint8_t src_mac[6], rout_mac[6], dest_mac[6], default_mac[6];
		for (int i = 0; i < 6; i++)
			default_mac[i] = 255;
		get_interface_mac(interface, rout_mac);
		inet_pton(AF_INET, get_interface_ip(interface), &rout_ip);

		if (memcmp(eth_hdr->ether_dhost, rout_mac, 6) && memcmp(eth_hdr->ether_dhost, default_mac, 6))
			continue;
		printf("It is for us!!! %x\n", ntohs(eth_hdr->ether_type));

		if (eth_hdr->ether_type == ntohs(0x0800)) {
			uint16_t sum;
			uint32_t daddr;
			
			printf("It is ip\n");
			struct iphdr *ip_hdr = (struct iphdr *)(buf + sizeof(struct ether_header));
			struct icmphdr *icmp_hdr;

			if (len - sizeof(struct ether_header) < sizeof(struct iphdr) || len - sizeof(struct ether_header) < ntohs(ip_hdr->tot_len))
				continue;

			sum = ip_hdr->check;
			ip_hdr->check = 0;
			sum = ntohs(sum);
			if (sum != checksum((uint16_t *)ip_hdr, sizeof(struct iphdr))) {
				printf("We have bad checksum: %d %d\n", sum, checksum((uint16_t *)ip_hdr, sizeof(struct iphdr)));
				continue;
			}
			sum = htons(sum);
			ip_hdr->check = sum;

			printf("We have the good checksum\n");

			daddr = ip_hdr->daddr;
			if (daddr == rout_ip) {
				if (ip_hdr->protocol == 1) {
					icmp_hdr = (struct icmphdr *)(buf + sizeof(struct ether_header) + sizeof(struct iphdr));
				} else {
					continue;
				}
				if (icmp_hdr->type == 8 && icmp_hdr->code == 0) {
					icmp_hdr->type = 0;
					ip_hdr->daddr = ip_hdr->saddr;
					ip_hdr->saddr = rout_ip;
					ip_hdr->check = 0;
					ip_hdr->check = htons(checksum((uint16_t *)ip_hdr, sizeof(struct iphdr)));
					memcpy(dest_mac, eth_hdr->ether_shost, 6);
					memcpy(eth_hdr->ether_shost, rout_mac, 6);
					memcpy(eth_hdr->ether_dhost, dest_mac, 6);
					send_to_link(interface, buf, len);
				}
				continue;
			}

			struct route_table_entry *entry = get_best_route(daddr);
			if (!entry) {
				if (ip_hdr->protocol != 1)
					send_host_unreachable(buf, rout_ip, interface);
				continue;
			}

			daddr = entry->next_hop;
			printf("We have our next hop!\n");

			if (ip_hdr->ttl > 1) {
				ip_hdr->ttl--;
			} else {
				send_no_more_ttl(buf, rout_ip, interface);
				continue;
			}

			ip_hdr->check = ~(~ip_hdr->check +  ~((uint16_t)(ip_hdr->ttl + 1)) + (uint16_t)ip_hdr->ttl) - 1;

			struct arp_table_entry *mac_ent = get_mac_entry(entry->next_hop);
			get_interface_mac(entry->interface, src_mac);
			uint32_t interface_ip;
			inet_pton(AF_INET, get_interface_ip(entry->interface), &interface_ip);
			if (!mac_ent) {
				printf("We don`t have the mac :(\n");
				queue_length++;
				void *package = malloc(len);
				memcpy(package, buf, len);
				queue_enq(package_queue, package);
				send_arp_request(entry->next_hop, entry->interface, interface_ip, src_mac);
				continue;
			}

			printf("We have our mac entry!\n");
		
			get_interface_mac(entry->interface, src_mac);
			memcpy(eth_hdr->ether_shost, src_mac, 6);
			memcpy(eth_hdr->ether_dhost, mac_ent->mac, 6);

			printf("Ready to send!\n");
		
			send_to_link(entry->interface, buf, len);
		}

		if (eth_hdr->ether_type == htons(0x0806)) {
			printf("We have ARP\n");
			struct arp_header *arp_hdr = (struct arp_header *)(buf + sizeof(struct ether_header));
			if (len - sizeof(struct ether_header) < sizeof(struct arp_header))
				continue;
			if (arp_hdr->op == htons(1)) {
				arp_hdr->op = htons(2);
				uint32_t aux = arp_hdr->spa;
				arp_hdr->spa = arp_hdr->tpa;
				arp_hdr->tpa = aux;
				memcpy(arp_hdr->tha, arp_hdr->sha, 6);
				memcpy(arp_hdr->sha, rout_mac, 6);
				memcpy(dest_mac, eth_hdr->ether_shost, 6);
				memcpy(eth_hdr->ether_shost, rout_mac, 6);
				memcpy(eth_hdr->ether_dhost, dest_mac, 6);
				send_to_link(interface, buf, len);
			} else if (arp_hdr->op == htons(2)) {
				printf("we got a respose\n");
				//send_arp_request(arp_hdr->spa, interface, arp_hdr->tpa, arp_hdr->tha);
				mac_table[mac_table_len].ip = arp_hdr->spa;
				memcpy(mac_table[mac_table_len].mac, arp_hdr->sha, 6);
				mac_table_len++;
				printf("We have written the stuff %d\n", queue_length);
				int n = queue_length;

				for (int i = 0; i < n; i++) {
					void *package = queue_deq(package_queue);
					printf("We got the package from the queue\n");
					struct iphdr *ip_hdr = (struct iphdr *)(package + sizeof(struct ether_header));
					struct ether_header *eth_hdr = (struct ether_header *)package;

					struct route_table_entry *entry = get_best_route(ip_hdr->daddr);

					printf("We got next hop\n");

					struct arp_table_entry *mac_ent = get_mac_entry(entry->next_hop);
					if (!mac_ent) {
						queue_enq(package_queue, package);
						continue;
					}

					printf("We got mac entry\n");
		
					get_interface_mac(entry->interface, src_mac);
					memcpy(eth_hdr->ether_shost, src_mac, 6);
					memcpy(eth_hdr->ether_dhost, mac_ent->mac, 6);

					printf("Package ready\n");
		
					send_to_link(entry->interface, package, sizeof(struct ether_header) + ntohs(ip_hdr->tot_len));
					free(package);

					printf("Packet sent\n");
					queue_length--;				
				}
 			}
		}
	}
}
