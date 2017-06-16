#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>

#include "util.h"

static void usage(char *name)
{
	fprintf(stderr, "usage: %s [-g interpacket_gap] "
			"[-o output_file] [-i] input_file\n", name);
	exit(EXIT_FAILURE);
}

static uint64_t scrambler (uint64_t state, FILE *f, int sync_header, uint64_t payload)
{
	int i; 
	uint64_t in_bit, out_bit;
	uint64_t scrambled = 0x0;

	for ( i = 0 ; i < 64 ; i ++) {
		in_bit = (payload >> i) & 0x1;
		out_bit = (in_bit ^ (state >> 38) ^ (state >> 57)) & 0x1 ;
		state = (state << 1 ) | out_bit;
		scrambled |= (out_bit << i);
	}

	print_64b66b_block(f, sync_header, scrambled);
	return state;
}

static int encode(struct packet *packets, int cnt, uint64_t state, const int idle, FILE *f)
{
	int i, begining_idles=idle, len;
	uint64_t tmp, block_type;
	uint64_t e_frame = 0x1e;
	unsigned char *data;
	static const unsigned char terminal[8] = {0x87, 0x99, 0xaa, 0xb4, 0xcc, 0xd2, 0xe1, 0xff};


	for ( i = 0 ; i < cnt ; i ++ ) {
		data = packets[i].eth_frame;
		len = packets[i].len;

		/* /E/ */
		while (begining_idles >= 8) {
			state = scrambler(state, f, 0x1, e_frame);
			begining_idles -= 8;
		}

		/* /S/ */
		tmp = 0;
		if (begining_idles >0 && begining_idles <= 4) {
			block_type = 0x33;
			tmp = *(uint64_t *) data;
			tmp <<= 40;
			tmp |= block_type;

			data += 3;
			len -= 3;

		} else {
			/* begining_idles is always less than 8 
			   If begining_idles is between 5 and 8 
			   We need to insert a /E/ 
			*/
			if (begining_idles != 0) {
				state = scrambler(state, f, 0x1, e_frame);
			}
			block_type = 0x78;
			tmp = *(uint64_t *) data;
			tmp <<= 8;
			tmp |= block_type;

			data += 7;
			len -= 7;
		}
		/* scramble /S/ */
		state = scrambler(state, f, 0x1, tmp);

		/* Data blocks */
		while ( len >= 8) {
			tmp = *(uint64_t *) data ;
			state = scrambler(state, f, 0x2, tmp);

			data += 8;
			len -= 8;
		}

		/* /T/ */
		/* tail is the number of bytes to be encoded in /T/ */
		block_type = terminal[len];
		tmp = 0;
		if (len != 0) {
			tmp = *(uint64_t *) data;
			tmp <<= (8-len) * 8;
			tmp >>= (7-len) * 8;
		}
		tmp |= block_type;

		state = scrambler(state, f, 0x1, tmp);

		/* how many /I/s are needed? */
		begining_idles = idle - ( 8 - len );

	}

	return 0;
}

void debug_scrambler()
{
	uint64_t state = 0xffffffffffffffff;
	uint64_t x = 0x0102030405060708;
	uint64_t y = 0x090a0b0c0d0e0f00;

	state = scrambler(state, stdout, 0x1, x);
	state = scrambler(state, stdout, 0x1, y);

	printf("state = %llx\n", (unsigned long long)state);

	exit(EXIT_SUCCESS);
}

int main(int argc, char **argv)
{
	int c, idle = 12, ret, pcnt;
	char * prg = argv[0];	
	char * inf = NULL, *outf = NULL;

	while((c = getopt(argc, argv, "i:g:o:d")) != -1) {
		switch (c) {
		case 'g':
			idle = atoi(optarg);
			break;
		case 'i':
			inf = optarg;
			break;
		case 'o':
			outf = optarg;
			break;	
		case 'd':
			debug_scrambler();
		default:
			usage(prg);
		}
	}

	if (inf == NULL && optind >= argc) 
		usage(prg);

	if (inf == NULL)
		inf = argv[optind];

	if (idle < 12) {
		fprintf(stderr, "Minimum interpacket gap is 12\n");
		idle = 12;
	}

	/* read packets from inf */
	struct packet *packets;
	if((ret = read_packets_from_file (inf, &packets)) < 0) {
		fprintf(stderr, "Read failed\n");	
		exit(EXIT_FAILURE);
	}
	pcnt = ret;

	/* encode */
	uint64_t state = PCS_INITIAL_STATE;
	FILE *f;
	if (outf) {
		if(!(f = fopen(outf, "w"))) {
			fprintf(stderr, "%s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}
	} else
		f = stdout;

	if ((ret = encode(packets, ret, state, idle, f)) < 0) {
		fprintf(stderr, "Encode error\n");
		exit(EXIT_FAILURE);
	}

	if(outf) 
		fclose(f);

	free_packets(packets, pcnt);

	exit(EXIT_SUCCESS);
}
