/*
dumb, anti-fool protection utilities
Made by AP for personal stuff
*/
#define APPROTECTION_C

#include "../ap_str.h"
#include "../ap_utils.h"
#include "ap_protection.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <linux/if_packet.h>
#include <linux/if_arp.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define LAST_ERROR_MAX_LEN 1023
static char last_error[LAST_ERROR_MAX_LEN + 1];

static uint8_t if_addresses[10][8]; // holds MAC addresses of found active ethernet-type iterfaces
static int if_addresses_count;

static char *keyblock_signature = "APLK";
static int max_keyblock_version_supported = 0;
/*
  format of key block header: ssssV, where 'ssss' is signature and 'V' is version + 'A'

  for version 0 this followed by:
    'c' followed by 5 bytes - crc16 of data. in printable form
    'l' followed by 10 bytes - data length. in printable form
    'e' followed by 1 char - representing encoding type (AP_PROT_KEY_ENCODING_* + 'a')

  encoded data follows
*/

#define APPKW_HOST     1
#define APPKW_LICENSE  2
#define APPKW_DATA     3

static struct
{
  int id;
  char *text;
#define KEYWORDS_COUNT 3
} keywords[KEYWORDS_COUNT] = {
  { APPKW_HOST, "host" }, // host mac [mac...]
  { APPKW_LICENSE, "license" }, // LICENSE issuer product_id [permanent|demo] start=date end=date issued=date sig="key"
  { APPKW_DATA, "data" } // data "binary encoded, license-specific data"
};

//=============================================================================================
const char *ap_prot_last_error_text(void)
{
  return last_error;
}

//=============================================================================================
static int get_if_addresses(void)
{
  struct ifaddrs *ifaddr, *ifa;
  int family, n;
  char if_names[10][NI_MAXHOST]; //will save interface names which have IP set here to retrieve MACs on second pass throught data
  int if_names_count;
  char buf[NI_MAXHOST];
  struct sockaddr_ll *sll;


  *last_error = '\0';

  if_addresses_count = 0; // in case of some system reconfiguration

  if (getifaddrs(&ifaddr) == -1)
  {
    snprintf(last_error, LAST_ERROR_MAX_LEN, "getifaddrs: %s", strerror(errno));
    freeifaddrs(ifaddr);
    return 0;
  }

  if_names_count = 0;

  // getting list of intrfaces IP addresses
  for (ifa = ifaddr; ifa != NULL && if_names_count != 10; ifa = ifa->ifa_next)
  {
    if (ifa->ifa_addr == NULL)
      continue;

    family = ifa->ifa_addr->sa_family;

    if (family != AF_INET && family != AF_INET6)
      continue;

    // getting iterface's hostname
    n = getnameinfo(ifa->ifa_addr, (family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6),
                    buf, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

    if (n != 0)
    {
      snprintf(last_error, LAST_ERROR_MAX_LEN, "getnameinfo() failed: %s\n", gai_strerror(n));
      freeifaddrs(ifaddr);
      return 0;
    }

    strcpy(if_names[if_names_count++], ifa->ifa_name);
  }

  // looping in search of MAC info
  for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
  {
    if (ifa->ifa_addr == NULL || ifa->ifa_addr->sa_family != AF_PACKET)
       continue;

    for (n = 0; n < if_names_count; ++n)
    {
      if (0 == strcmp(if_names[n], ifa->ifa_name))
      {
        sll = (struct sockaddr_ll*)(ifa->ifa_addr);

        if ( sll->sll_hatype != ARPHRD_ETHER )
          continue;

        /*
        memdumpfd(fileno(stdout), sll, sizeof(struct sockaddr_ll));

        printf("sll_family: %u\nsll_protocol: %u\nsll_ifindex: %d\nsll_hatype: %u\nsll_pkttype: %u\nsll_halen: %u\nsll_addr:\n",
               (unsigned)(sll->sll_family), (unsigned)(sll->sll_protocol), sll->sll_ifindex, (unsigned)(sll->sll_hatype),
               (int)(sll->sll_pkttype), (int)(sll->sll_halen));

        memdumpfd(fileno(stdout), sll->sll_addr, 8);

        printf("\n");
        */

        memcpy( if_addresses[if_addresses_count++], sll->sll_addr, 8 );

        break;
      }
    } // for (n = 0; n < if_names_count; ++n)

    if( if_addresses_count == 10 )
      break;
  }

  freeifaddrs(ifaddr);

  return 1;
}

//=============================================================================================
//=============================================================================================
//=============================================================================================
int ap_protect_host_key_import(const char *in_buf, t_ap_prot_host_key *out_key)
{
  int i;

  *last_error = '\0';
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

  out_key->key_state = AP_PROT_KEY_STATE_VALID;
  out_key->key_state = AP_PROT_KEY_STATE_MAC_MISMATCH;
  out_key->key_state = AP_PROT_KEY_STATE_EXPIRED;
}

//=============================================================================================
int ap_protect_host_key_export(t_ap_prot_host_key *in_key, char *out_key_string)
{

  *last_error = '\0';
}

//=============================================================================================
void ap_protect_host_key_destroy(t_ap_prot_host_key *key)
{
  if ( key->macs != NULL )
    free(key->macs);

  if ( key->data != NULL )
    free(key->data);

  free(key);
}

//=============================================================================================
t_ap_prot_host_key *ap_protect_host_key_create(const char *in_license_line)
{
  t_ap_prot_host_key *key;

  *last_error = '\0';

  if( NULL == (key = ap_str_getmem(sizeof(t_ap_prot_host_key), NULL)) )
  {
    snprintf(last_error, LAST_ERROR_MAX_LEN, "getmem: OOM");
    return NULL;
  }

  key->key_state = AP_PROT_KEY_STATE_UNCHECKED;
  key->macs = NULL;
  key->macs_count = 0;
  key->expires = 0;
  key->datalen = 0;
  key->data = NULL;
  *(key->product_id) = '\0';
  *(key->issuer) = '\0';

  if( in_license_line != NULL && ! ap_protect_host_key_import(in_license_line, key) )
  {
    ap_protect_host_key_destroy(key);
    return NULL;
  }

  return key;
}

//=============================================================================================
int ap_protect_host_key_validate(t_ap_prot_host_key *key)
{

  *last_error = '\0';

  if( ! get_if_addresses() )
    return 0;

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

  return 1;
}

//=============================================================================================
//=============================================================================================
//=============================================================================================
// imports license data from raw file content. returns 0 if error
int ap_protect_keyring_import(const char *in_buf, t_ap_prot_keyring *out_keyring)
{
  t_ap_prot_keyring *kr;
  int i, keyblock_version;
  t_ap_prot_host_key *key;
  char *s, *line_begin;
  int line_count;


  *last_error = '\0';

  line_count = 0;

  for( s = line_begin = (char *)in_buf; ; ++s )
  {
    if ( *s != '\r' && *s != '\n' && *s != '\0' ) // eol or eof
      continue;

    if ( *s != '\r' )
      ++line_count;

    if ( s == line_begin ) // empty line - skip
    {
      line_begin = s + 1;
      continue;
    }

    *s = '\0'; // mark eol

    for ( ; *line_begin == ' ' || *line_begin == '\t'; ) // skip spaces
      ++line_begin;

    if ( *line_begin == '#' ) //comment - skip
    {
      line_begin = s + 1;
      continue;
    }

    if ( NULL == (key = ap_protect_host_key_create(line_begin)) )
    {
      return 0;
    }

  }

  *last_error = '\0';

  if ( strlen(in_license_text) < 50 ) // just sane minimum
  {
    snprintf(last_error, LAST_ERROR_MAX_LEN, "ap_protect_keyring_create(): total length invalid");
    return NULL;
  }

  if ( NULL == ( s = strcasestr(in_buf, keyblock_signature) ) )
  {
    snprintf(last_error, LAST_ERROR_MAX_LEN,  "ap_protect_keyring_create(): Key signature not found");
    return NULL;
  }

  keyblock_version = *s - 'A';

  if ( keyblock_version < 0 || keyblock_version > max_keyblock_version_supported )
  {
    snprintf(last_error, LAST_ERROR_MAX_LEN, "ap_protect_keyring_create(): Key block version invalid");
    return NULL;
  }

  ++s;

  if ( NULL == ( kr = parse_keyblock_header(keyblock_version, s) ) )
    return NULL;

uint16_t count_crc16(void *mem, int len)

  t_ap_prot_host_key *keys;
  int count;
  char product_id[AP_PROT_MAX_PRODID_LEN + 1];
  char issuer[AP_PROT_MAX_ISSUER_LEN + 1];

  //AP_PROT_MAX_PRODID_LEN
  //AP_PROT_MAX_ISSUER_LEN
}

//=============================================================================================
// exports key data as single license file line
int ap_protect_keyring_export(t_ap_prot_keyring *in_keyring, char *out_license_file_text)
{
  *last_error = '\0';
}

//=============================================================================================
// destructor
void ap_protect_keyring_destroy(t_ap_prot_keyring *keyring)
{
  *last_error = '\0';
}

//=============================================================================================
t_ap_prot_keyring *ap_protect_keyring_create()
{
}

//=============================================================================================
// internal
static t_ap_prot_keyring *parse_keyblock_header_version_0( int keyblock_version, char *key_text )
{
  t_ap_prot_keyring *kr;
  int n, crc, data_len, i;
  char *s, *eoc, *eol;


  // converting right now. error checking follows
  crc = strtol(key_text + 1, &eoc, 10);
  data_len = strtol(key_text + 6, &eol, 10);

  if ( *key_text != 'c' || *eoc != 'l' || *eol != 'e' )
  {
    snprintf(last_error, LAST_ERROR_MAX_LEN, "parse_keyblock_header_version_0(): format error");
    return NULL;
  }

  if( NULL == (kr = ap_str_getmem(sizeof(t_ap_prot_keyring), NULL)) )
  {
    snprintf(last_error, LAST_ERROR_MAX_LEN, "parse_keyblock_header_version_0(): getmem: OOM kr");
    return NULL;
  }

}

//=============================================================================================
// internal. calls appropriate parse_keyblock_header_*()
// returns true/false
static t_ap_prot_keyring *parse_keyblock_header( int keyblock_version, char *key_text )
{
  switch( keyblock_version )
  {
    case 0:
       return parse_keyblock_header_version_0( keyblock_version, key_text );
    default:
       assert(keyblock_version);
  }
}

//=============================================================================================
t_ap_prot_keyring *ap_protect_keyring_create(const char *in_product_id, const char *in_issuer, const char *in_license_text)
{
  t_ap_prot_keyring *kr;

  if( NULL == (kr = ap_str_getmem(sizeof(t_ap_prot_keyring), NULL)) )
  {
    snprintf(last_error, LAST_ERROR_MAX_LEN, "ap_protect_keyring_create(): getmem: OOM kr");
    return NULL;
  }

  kr->
}

//=============================================================================================
// returns 1 if key is valid
int ap_protect_keyring_validate(t_ap_prot_keyring *in_keyring)
{
  *last_error = '\0';
}

//=============================================================================================
int ap_protect_keyring_load_and_validate(const char *in_key_file_name, const char *in_product_id, const char *in_issuer, t_ap_prot_keyring *out_keyring)
{
  int handle;
  char *buf;
  struct stat st;


  *last_error = '\0';

  if ( -1 == stat( in_key_file_name, &st) )
  {
    snprintf(last_error, LAST_ERROR_MAX_LEN, "stat file: %s: %s", in_key_file_name, strerror(errno));
    return 0;
  }

  if ( NULL == (handle = open(in_key_file_name, O_RDONLY)) )
  {
    snprintf(last_error, LAST_ERROR_MAX_LEN, "open file: %s: %s", in_key_file_name, strerror(errno));
    return 0;
  }

  if ( NULL == (buf = ap_str_getmem(st.st_size + 1, NULL)) )
  {
    snprintf(last_error, LAST_ERROR_MAX_LEN, "getmem: OOM 2");
    close(handle);
    return 0;
  }

  if ( st.st_size + 1 != read(handle, buf, st.st_size + 1) )
  {
    snprintf(last_error, LAST_ERROR_MAX_LEN, "read file: %s: %s", in_key_file_name, strerror(errno));
    close(handle);
    free(buf);
    return 0;
  }

  close(handle);

  if ( NULL == (out_keyring = ap_protect_keyring_create(in_product_id, in_issuer, buf)) )
  {
    free(buf);
    return 0;
  }

  free(buf);

  return ap_protect_keyring_validate(out_keyring);
}
