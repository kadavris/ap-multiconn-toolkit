#include <stdint.h>

struct ap_prot_key
{
  uint8_t macaddr[6];
  int valid;
  int datalen;
  void *data;
} t_ap_prot_key;

extern int ap_protect_validate_key(char *key_file_content, t_ap_prot_key *decoded_key_out)