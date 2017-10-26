#include "../params.h"
#include "../xmss.h"
#include <stdio.h>

#define MLEN 32

#ifdef XMSSMT
    #define XMSS_PARSE_OID xmssmt_parse_oid
    #define XMSS_SIGN xmssmt_sign
#else
    #define XMSS_PARSE_OID xmss_parse_oid
    #define XMSS_SIGN xmss_sign
#endif

int main(int argc, char **argv) {
    FILE *keypair;
    xmss_params params;
    uint32_t oid_pk;
    uint32_t oid_sk;

    if (argc != 2) {
        fprintf(stderr, "Expected keypair filename as only parameter, "
                        "and the message via stdin.\n"
                        "The keypair is updated with the changed state, "
                        "and the message + signature is output via stdout.\n");
        return -1;
    }

    keypair = fopen(argv[1], "r+b");
    if (keypair == NULL) {
        fprintf(stderr, "Could not open keypair file.\n");
        return -1;
    }

    /* Read the OID from the public key, as we need its length to seek past it */
    fread(&oid_pk, 1, XMSS_OID_LEN, keypair);
    XMSS_PARSE_OID(&params, oid_pk);

    /* fseek past the public key */
    fseek(keypair, params.pk_bytes, SEEK_CUR);
    /* This is the OID we're actually going to use. Likely the same, but still. */
    fread(&oid_sk, 1, XMSS_OID_LEN, keypair);
    XMSS_PARSE_OID(&params, oid_sk);

    unsigned char sk[XMSS_OID_LEN + params.sk_bytes];
    unsigned char m[MLEN];
    unsigned char sm[params.sig_bytes + MLEN];
    unsigned long long smlen;

    /* fseek back to start of sk. */
    fseek(keypair, -((long int)XMSS_OID_LEN), SEEK_CUR);
    fread(sk, 1, XMSS_OID_LEN + params.sk_bytes, keypair);
    fread(m, 1, MLEN, stdin);

    XMSS_SIGN(sk, sm, &smlen, m, MLEN);

    fseek(keypair, -((long int)params.sk_bytes), SEEK_CUR);
    fwrite(sk + XMSS_OID_LEN, 1, params.sk_bytes, keypair);
    fwrite(sm, 1, params.sig_bytes + MLEN, stdout);

    fclose(keypair);
    fclose(stdout);
}