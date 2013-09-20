#include <stdint.h>
#include <time.h>

#define AP_PROT_KEY_STATE_UNCHECKED       0
#define AP_PROT_KEY_STATE_VALID           1
#define AP_PROT_KEY_STATE_MAC_MISMATCH    2
#define AP_PROT_KEY_STATE_EXPIRED         3
#define AP_PROT_KEY_STATE_BAD_PRODUCT_ID  4
#define AP_PROT_KEY_STATE_BAD_ISSUER      5

#define AP_PROT_KEY_ENCODING_BASIC 0

typedef struct ap_prot_mac // MAC address
{
  uint8_t mac[8]; // 6 bytes actually, 8 to make it aligned
} ap_prot_mac;

#define AP_PROT_MAX_PRODID_LEN 15
#define AP_PROT_MAX_ISSUER_LEN 15

typedef struct t_ap_prot_host_key
{
  int key_state;
  int macs_count;
  struct ap_prot_mac *macs;
  int data_len; // key's data block length
  void *data; // key's data
  char product_id[AP_PROT_MAX_PRODID_LEN + 1];
  char issuer[AP_PROT_MAX_ISSUER_LEN + 1];
  time_t start, expires, issued;
} t_ap_prot_host_key;

typedef struct t_ap_prot_keyring
{
  t_ap_prot_host_key *keys;
  int count;
  char product_id[AP_PROT_MAX_PRODID_LEN + 1];
  char issuer[AP_PROT_MAX_ISSUER_LEN + 1];
  int encoding;
  void *encoder_data;
} t_ap_prot_keyring;

#ifndef APPROTECTION_C
extern const char *ap_prot_last_error_text(void);
//------------------------------------------------------------------
// single key functions
//------------------------------------------------------------------
// constructor. NULL if error
extern t_ap_prot_host_key *ap_protect_host_key_create(char *in_license_line);
// destructor
extern void ap_protect_host_key_destroy(t_ap_prot_host_key *key);
// imports license data from one line of license file. returns 0 if error
extern int ap_protect_host_key_import(const char *in_buf, t_ap_prot_host_key *out_key);
// exports key data as single license file line
extern int ap_protect_host_key_export(t_ap_prot_host_key *in_key, char *out_key_string);
// returns 1 if key is valid
extern int ap_protect_host_key_validate(t_ap_prot_host_key *key);
//------------------------------------------------------------------
// keyring functions
//------------------------------------------------------------------
// constructor. NULL if error
extern t_ap_prot_keyring *ap_protect_keyring_create(char *in_product_id, char *in_issuer, int encoding);
// destructor
extern void ap_protect_keyring_destroy(t_ap_prot_keyring *key);
// imports license data from one line of license file. returns 0 if error
extern int ap_protect_keyring_import(const char *in_buf, t_ap_prot_keyring *out_keyring);
// exports key data as single license file line
extern int ap_protect_keyring_export(t_ap_prot_keyring *in_keyring, char *out_license_file_text);
// returns 1 if key is valid
extern int ap_protect_keyring_validate(t_ap_prot_keyring *in_keyring);
//
extern int ap_protect_keyring_load_and_validate(const char *in_key_file_name, char *in_product_id, char *in_issuer, t_ap_prot_keyring *out_keyring);
#endif
