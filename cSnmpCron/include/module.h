#include <common.h>
#include <job.h>

#define MAX_INTERFACES	4096
#define VIRTUAL_IF	"Virtual-Access" /* Virtual interface signature */
#define FAST_IF		"FastEthernet"   /* Always Use 64bit counters */
#define GIG_IF		"GigabitEthernet" /* Always Use 64bit counters */

#define IGNORE_VIRTUAL	1	/* Ignore Virtual interfaces. dynamic */

#define FMT_VERSION "1.1"       /* Output format version */

typedef struct {
    int aindex;
} h_data;

typedef struct {
    int key;
} h_key;

typedef struct snmp_rtrinfo_struct
{
    unsigned long uptime;
    char description[MAX_INTERFACES];
    char hardware[MAX_INTERFACES];
    char version[MAX_INTERFACES];
} snmp_rtrinfo_t;

typedef struct snmp_octets_struct
{
    char 		interface[128];
    long 		index;	// takes -1 
    unsigned long 	bandwidth;
/* up(1), down(2), testing(3), unknown(4), dormant(5), notPresent(6) */
    unsigned int 	operstatus; 
    unsigned int 	bits;			// 32 or 64 bit counters
    unsigned long long ifinoctets;
    unsigned long long ifoutoctets;
} snmp_octets_t;

typedef struct snmp_data_struct
{
    oid 		*name;
    size_t          	name_length;
    u_char 		type;
    //size_t         	val_len;
    //netsnmp_vardata 	val;
    u_long         	index;
    u_int         	max_index;
    char 		buf[128];
    struct snmp_data_struct *next;
} snmp_data_t;

typedef struct snmp_callhist_struct
{
       int    index;
       int    InterfaceNumber;
       char   DestinationHostName[32];
       u_int TransmitBytes;
       u_int  ReceiveBytes;
} snmp_callhist_t;

struct snmp_pdu *snmp_get(struct snmp_session *session, char  *objoid, taskresult_t *taskresult, struct snmp_pdu *response);
struct snmp_pdu *xsnmp_get(struct snmp_session *session, char  *objoid, taskresult_t *taskresult);
snmp_data_t *snmp_bulk(struct snmp_session *session, char  *objoid, taskresult_t *taskresult, snmp_data_t *head);
void free_snmp_data(snmp_data_t *data);

#include <modlib.h>
