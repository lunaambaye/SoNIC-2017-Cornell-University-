#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <ctype.h>

#include "util.h"
#define IDLE_CHARACTER	' '

static char idle_char = IDLE_CHARACTER;

static void usage(char *name)
{
	fprintf(stderr, "usage: %s [-o output_file] [-i] input_file\n", name);
	exit(EXIT_FAILURE);
}

static void print_decoded(FILE *out_f, struct packet * packet)
{
	int i;
	for ( i = 0 ; i < packet->idles ; i ++) {
		fprintf(out_f, "%c", idle_char);
	}

	for ( i = 0 ; i < packet->len ; i ++) {
		if (isprint(packet->eth_frame[i]) || isspace(packet->eth_frame[i]))
			fprintf(out_f, "%c", packet->eth_frame[i]);
	}
}

/* returns descrambled block */
static uint64_t descrambler (uint64_t *pstate, uint64_t payload)
{
	int i;
	uint64_t in_bit, out_bit;
	uint64_t state = * pstate;
	uint64_t descrambled = 0x0;
	
	for (i = 0; i < 64; i++)
	{
	  in_bit = (payload >> i) & 0x1;
	  out_bit = (in_bit ^ (state >> 38) ^ (state >> 57)) & 0x1;
	  state = (state << 1) | in_bit;
	  descrambled |= (out_bit << i);
	}

	*pstate = state;
	return descrambled;
}

static int decode(struct block *blocks, int cnt, uint64_t state, FILE *out_f)
{
  int i, tail = 0;
	uint64_t descrambled, block_type;
	unsigned char *p; 
	/* temporary structure to store information */
	struct packet packet;

	packet.eth_frame = malloc(2 * DEFAULT_MTU);
	packet.capacity = 2 * DEFAULT_MTU;
	packet.len = 0;
	packet.idles = 0;

	/* let p pointing to the first byte of eth_frame */
	p = packet.eth_frame;

	for ( i = 0 ; i < cnt ; i ++) {

		descrambled = descrambler(&state, blocks[i].payload);

		/* data block */
		if (blocks[i].sync_header != 1)	{
		  *(uint64_t *) p = descrambled;
		  p += 8;
		  packet.len += 8;
		
		/* control block */
		} else {
			block_type = descrambled & 0xff;
			switch(block_type) {
			/* /S/ */
			case 0x33:
				packet.idles += 4;
				descrambled >>= 40;
				* (uint64_t *) p = descrambled;
				p += 3;
				packet.len += 3;
				break;
			case 0x78:
			  /* to be filled */			  
			  descrambled >>= 8;
			  *(uint64_t *) p = descrambled;
			  p += 7;
			  
			  
			  break;

			/* /T/ */
			case 0xff:
			  tail++;
			case 0xe1:
			  tail++;
			case 0xd2:
			  tail++;
			case 0xcc:
			  tail++;
			case 0xb4:
			  tail++;
			case 0xaa:
			  tail++;
			case 0x99:
			  tail++;
			case 0x87:
			  /* to be filled */			  
			  descrambled >>= 8;
			  *(uint64_t *) p = descrambled;
			  packet.len += tail;
			  
			  /* when you recovered an Ethernet frame
			   * let's print it out
			   */
			  print_decoded(out_f, &packet);
			  
			  // initialize packet struct for next packet
			  packet.idles = (8 - tail);
			  
			  break;

			/* /E/ */
			case 0x1e:		
				packet.idles += 8;
				break;
			default:
				fprintf(stderr, "Error! Unknown block type\n");
			}
		}
	}

	free(packet.eth_frame);
	return 0;
}

void debug_descrambler()
{
	uint64_t state = PCS_INITIAL_STATE;
	uint64_t x = 0xa1fe788405060708;
	uint64_t y = 0x60a77dbee226551e;
	uint64_t descrambled;

	descrambled = descrambler(&state, x);
	printf("%.16llx\n", (unsigned long long) descrambled);
	descrambled = descrambler(&state, y);
	printf("%.16llx\n", (unsigned long long) descrambled);
	printf("state = %.16llx\n", (unsigned long long) state);

	exit(EXIT_SUCCESS);
}

int main(int argc, char **argv)
{
	int c, ret;
	char * prg = argv[0];	
	char * inf = NULL, *outf = NULL;

	while((c = getopt(argc, argv, "i:o:c:d")) != -1) {
		switch (c) {
		case 'c':
			idle_char = optarg[0];
			break;
		case 'i':
			inf = optarg;
			break;
		case 'o':
			outf = optarg;
			break;	
		case 'd':
			debug_descrambler();
		default:
			usage(prg);
		}
	}

	if (inf == NULL && optind >= argc)
		usage(prg);

	if (inf == NULL)
		inf = argv[optind];

	/* read 66b blocks from inf */
	struct block *blocks;
	if((ret = read_blocks_from_file (inf, &blocks)) < 0) {
		fprintf(stderr, "Read failed\n");	
		exit(EXIT_FAILURE);
	}

//	print_blocks(stdout, blocks, ret);

	/* decode */
	uint64_t state = PCS_INITIAL_STATE;
	FILE *out_f;
	if (outf) {
		if(!(out_f = fopen(outf, "w"))) {
			fprintf(stderr, "%s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}
	} else
		out_f = stdout;

	if ((ret = decode(blocks, ret, state, out_f)) < 0) {
		fprintf(stderr, "Decode error\n");
		exit(EXIT_FAILURE);
	}

	if(outf)
		fclose(out_f);

	free_blocks(blocks);

	exit(EXIT_SUCCESS);
}
