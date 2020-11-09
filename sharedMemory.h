#define MMAP_SIZE 4096
#define BUFFER_SIZE 64000
#define PAYLOAD_SIZE 34
#define MEMSIZE 64000

typedef struct
{
  int item_no;                         /* number of the item produced */
  unsigned short cksum;                /* 16-bit Internet checksum    */
  unsigned char payload[PAYLOAD_SIZE]; /* random generated data */
} item;

item buffer_item[BUFFER_SIZE];
int in = 0;
int out = 0;
