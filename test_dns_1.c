/******************************************************************************

                            Online C Compiler.
                Code, Compile, Run and Debug C program online.
Write your code in this editor and press "Run" button to compile and execute it.

*******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

//#define DNS_MSG_ID         0x1122   ///< ID for DNS message. You can be modifyed it any number
#define	MAX_DNS_BUF_SIZE	256		///< maximum size of DNS buffer. */
//#define  MAX_DOMAIN_NAME   128 
//#define	MAXCNAME	   (MAX_DOMAIN_NAME + (MAX_DOMAIN_NAME>>1))	   /* Maximum amount of cname recursion */
#define	MAXCNAME 128

//type define
#define  uint8_t        unsigned char
#define  uint16_t       unsigned short
#define int16_t         signed short
#define uint32_t        unsigned int


#define	TYPE_A		1	   /* Host address */
#define	TYPE_NS		2	   /* Name server */
#define	TYPE_MD		3	   /* Mail destination (obsolete) */
#define	TYPE_MF		4	   /* Mail forwarder (obsolete) */
#define	TYPE_CNAME	5	   /* Canonical name */
#define	TYPE_SOA	   6	   /* Start of Authority */
#define	TYPE_MB		7	   /* Mailbox name (experimental) */
#define	TYPE_MG		8	   /* Mail group member (experimental) */
#define	TYPE_MR		9	   /* Mail rename name (experimental) */
#define	TYPE_NULL	10	   /* Null (experimental) */
#define	TYPE_WKS	   11	   /* Well-known sockets */
#define	TYPE_PTR	   12	   /* Pointer record */
#define	TYPE_HINFO	13	   /* Host information */
#define	TYPE_MINFO	14	   /* Mailbox information (experimental)*/
#define	TYPE_MX		15	   /* Mail exchanger */
#define	TYPE_TXT	   16	   /* Text strings */
#define	TYPE_ANY	   255	/* Matches any type */

#define	CLASS_IN	   1	   /* The ARPA Internet */

//uint8_t* pDNSMSG;       // DNS message buffer
uint8_t  DNS_SOCKET;    // SOCKET number for DNS
//uint16_t DNS_MSGID;     // DNS message ID
struct dhdr
{
	uint16_t id;   /* Identification */
	uint8_t	qr;      /* Query/Response */
#define	QUERY    0
#define	RESPONSE 1
	uint8_t	opcode;
#define	IQUERY   1
	uint8_t	aa;      /* Authoratative answer */
	uint8_t	tc;      /* Truncation */
	uint8_t	rd;      /* Recursion desired */
	uint8_t	ra;      /* Recursion available */
	uint8_t	rcode;   /* Response code */
#define	NO_ERROR       0
#define	FORMAT_ERROR   1
#define	SERVER_FAIL    2
#define	NAME_ERROR     3
#define	NOT_IMPL       4
#define	REFUSED        5
	uint16_t qdcount;	/* Question count */
	uint16_t ancount;	/* Answer count */
	uint16_t nscount;	/* Authority (name server) count */
	uint16_t arcount;	/* Additional record count */
};

uint8_t * put16(uint8_t * s, uint16_t i)
{
	*s++ = i >> 8;
	*s++ = i;
	return s;
}
uint16_t get16(uint8_t * s)
{
	uint16_t i;
	i = *s++ << 8;
	i = i + *s;
	return i;
}
#if 0
void DNS_init(uint8_t s, uint8_t * buf)
{
	DNS_SOCKET = s; // SOCK_DNS
	pDNSMSG = buf; // User's shared buffer
	//DNS_MSGID = DNS_MSG_ID;
}
#endif

int16_t dns_makequery(uint16_t op, char * name, uint8_t * buf, uint16_t len)
{
	uint8_t *cp;
	char *cp1;
	char sname[MAXCNAME];
	char *dname;
	uint16_t p;
	uint16_t dlen;
	
	uint16_t DNS_MSGID = 0x1122;

	cp = buf;

	DNS_MSGID++;
	cp = put16(cp, DNS_MSGID);
	p = (op << 11) | 0x0100;			/* Recursion desired */
	cp = put16(cp, p);
	cp = put16(cp, 1);
	cp = put16(cp, 0);
	cp = put16(cp, 0);
	cp = put16(cp, 0);

	strcpy(sname, name);
	dname = sname;
	dlen = strlen(dname);
	for (;;)
	{
		/* Look for next dot */
		cp1 = strchr(dname, '.');

		if (cp1 != NULL) len = cp1 - dname;	/* More to come */
		else len = dlen;			/* Last component */

		*cp++ = len;				/* Write length of component */
		if (len == 0) break;

		/* Copy component up to (but not including) dot */
		strncpy((char *)cp, dname, len);
		cp += len;
		if (cp1 == NULL)
		{
			*cp++ = 0;			/* Last one; write null and finish */
			break;
		}
		dname += len+1;
		dlen -= len+1;
	}

	cp = put16(cp, 0x0001);				/* type */
	cp = put16(cp, 0x0001);				/* class */

	return ((int16_t)( (cp) - (buf) ));
}


int parse_name(uint8_t * msg, uint8_t * compressed, char * buf, int16_t len)
{
	uint16_t slen;		/* Length of current segment */
	uint8_t * cp;
	int clen = 0;		/* Total length of compressed name */
	int indirect = 0;	/* Set if indirection encountered */
	int nseg = 0;		/* Total number of segments in name */

	cp = compressed;

	for (;;)
	{
		slen = *cp++;	/* Length of this segment */

		if (!indirect) clen++;

		if ((slen & 0xc0) == 0xc0)
		{
			if (!indirect)
				clen++;
			indirect = 1;
			/* Follow indirection */
			cp = &msg[((slen & 0x3f)<<8) + *cp];
			slen = *cp++;
		}

		if (slen == 0)	/* zero length == all done */
			break;

		len -= slen + 1;

		if (len < 0) return -1;

		if (!indirect) clen += slen;

		while (slen-- != 0) *buf++ = (char)*cp++;
		*buf++ = '.';
		nseg++;
	}

	if (nseg == 0)
	{
		/* Root name; represent as single dot */
		*buf++ = '.';
		len--;
	}

	*buf++ = '\0';
	len--;

	return clen;	/* Length of compressed message */
}

uint8_t * dns_question(uint8_t * msg, uint8_t * cp)
{
	int len;
	char name[MAXCNAME];

	len = parse_name(msg, cp, name, MAXCNAME);


	if (len == -1) return 0;

	cp += len;
	cp += 2;		/* type */
	cp += 2;		/* class */

	return cp;
}

uint8_t * dns_answer(uint8_t * msg, uint8_t * cp, uint8_t * ip_from_dns)
{
	int len, type;
	char name[MAXCNAME];

	len = parse_name(msg, cp, name, MAXCNAME);

	if (len == -1) return 0;

	cp += len;
	type = get16(cp);
	cp += 2;		/* type */
	cp += 2;		/* class */
	cp += 4;		/* ttl */
	cp += 2;		/* len */


	switch (type)
	{
	case TYPE_A:
		/* Just read the address directly into the structure */
		ip_from_dns[0] = *cp++;
		ip_from_dns[1] = *cp++;
		ip_from_dns[2] = *cp++;
		ip_from_dns[3] = *cp++;
		break;
	case TYPE_CNAME:
	case TYPE_MB:
	case TYPE_MG:
	case TYPE_MR:
	case TYPE_NS:
	case TYPE_PTR:
		/* These types all consist of a single domain name */
		/* convert it to ascii format */
		len = parse_name(msg, cp, name, MAXCNAME);
		if (len == -1) return 0;

		cp += len;
		break;
	case TYPE_HINFO:
		len = *cp++;
		cp += len;

		len = *cp++;
		cp += len;
		break;
	case TYPE_MX:
		cp += 2;
		/* Get domain name of exchanger */
		len = parse_name(msg, cp, name, MAXCNAME);
		if (len == -1) return 0;

		cp += len;
		break;
	case TYPE_SOA:
		/* Get domain name of name server */
		len = parse_name(msg, cp, name, MAXCNAME);
		if (len == -1) return 0;

		cp += len;

		/* Get domain name of responsible person */
		len = parse_name(msg, cp, name, MAXCNAME);
		if (len == -1) return 0;

		cp += len;

		cp += 4;
		cp += 4;
		cp += 4;
		cp += 4;
		cp += 4;
		break;
	case TYPE_TXT:
		/* Just stash */
		break;
	default:
		/* Ignore */
		break;
	}

	return cp;
}
int8_t parseDNSMSG(struct dhdr * pdhdr, uint8_t * pbuf, uint8_t * ip_from_dns)
{
	uint16_t tmp;
	uint16_t i;
	uint8_t * msg;
	uint8_t * cp;

	msg = pbuf;
	memset(pdhdr, 0, sizeof(*pdhdr));

	pdhdr->id = get16(&msg[0]);
	tmp = get16(&msg[2]);
	if (tmp & 0x8000) pdhdr->qr = 1;

	pdhdr->opcode = (tmp >> 11) & 0xf;

	if (tmp & 0x0400) pdhdr->aa = 1;
	if (tmp & 0x0200) pdhdr->tc = 1;
	if (tmp & 0x0100) pdhdr->rd = 1;
	if (tmp & 0x0080) pdhdr->ra = 1;

	pdhdr->rcode = tmp & 0xf;
	pdhdr->qdcount = get16(&msg[4]);
	pdhdr->ancount = get16(&msg[6]);
	pdhdr->nscount = get16(&msg[8]);
	pdhdr->arcount = get16(&msg[10]);


	/* Now parse the variable length sections */
	cp = &msg[12];

	/* Question section */
	for (i = 0; i < pdhdr->qdcount; i++)
	{
		cp = dns_question(msg, cp);
   #ifdef _DNS_DEUBG_
      printf("MAX_DOMAIN_NAME is too small, it should be redfine in dns.h");
   #endif
		if(!cp) return -1;
	}

	/* Answer section */
	for (i = 0; i < pdhdr->ancount; i++)
	{
		cp = dns_answer(msg, cp, ip_from_dns);
   #ifdef _DNS_DEUBG_
      printf("MAX_DOMAIN_NAME is too small, it should be redfine in dns.h");
   #endif
		if(!cp) return -1;
	}

	/* Name server (authority) section */
	for (i = 0; i < pdhdr->nscount; i++)
	{
		;
	}

	/* Additional section */
	for (i = 0; i < pdhdr->arcount; i++)
	{
		;
	}

	if(pdhdr->rcode == 0) return 1;		// No error
	else return 0;
}

int main()
{
    struct dhdr dhp;
    uint8_t Domain_IP[4]  = {0, }; 
    int result = 0;
    int temp_sock = 0;
    int temp_lenth =0;
     int   server_addr_size;
    struct sockaddr_in server_addr;
    char temp_dns[4] = {8, 8, 8, 8};
    char buff_rcv[MAX_DNS_BUF_SIZE];
    char name[] ="fota.hitecdata.com";
    char pDNSMSG[MAX_DNS_BUF_SIZE];
	int i=0;
	char hex_str[512]={0,};
	
    printf("Hello World\r\n");
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(53);
    server_addr.sin_addr.s_addr = inet_addr("8.8.8.8");
    server_addr_size  = sizeof( server_addr);
    temp_sock = socket(PF_INET, SOCK_DGRAM, 0);
    
    //DNS_init(0, tempdata);
    temp_lenth = dns_makequery(0, name, pDNSMSG,MAX_DNS_BUF_SIZE);
    
    //printf("msg[%d][%d] = [%s]\r\n",(int)strlen(pDNSMSG),temp_lenth, pDNSMSG );
    for(i=0; i<temp_lenth; i++)
    {
        sprintf(hex_str,"%s%02x", hex_str, *(pDNSMSG +i));
        //printf("hexstr=%s %02x \r\n",hex_str, *(pDNSMSG +i));
    }
    printf("msg[%d][%d] = [%s]\r\n",(int)strlen(hex_str),temp_lenth, hex_str );
    result = sendto(temp_sock, pDNSMSG,temp_lenth,0, ( struct sockaddr*)&server_addr, sizeof( server_addr));
    printf("sendto result = %d\r\n",result);
    result = recvfrom( temp_sock, buff_rcv, MAX_DNS_BUF_SIZE, 0 , 
            ( struct sockaddr*)&server_addr, &server_addr_size);
    printf("recevfrom result = %d\r\n",result);
    memset(hex_str, 0, sizeof(char)*512);
    for(i=0; i<result; i++)
    {
        sprintf(hex_str,"%s%02x",hex_str, (unsigned char)buff_rcv[i]);
        //printf("hexstr=%s %02x \r\n",hex_str, (unsigned char)buff_rcv[i]);
    }
    //printf("msg[%d][%d] = [%s]\r\n",(int)strlen(hex_str),temp_lenth, hex_str );
    printf("data[%d]= %s\r\n", strlen(hex_str), hex_str);
    
    result = parseDNSMSG(&dhp, buff_rcv, Domain_IP);
    
    printf("domain_IP = %d.%d,%d.%d\r\n", Domain_IP[0], Domain_IP[1], Domain_IP[2], Domain_IP[3]);
    return 0;
}