/*
 * meg4/misc/lictool.c
 *
 * Copyright (C) 2023 bzt
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * @brief MEG-4's license tool (the generated license.txt enables wasm export in the editor)
 * Don't get your hopes up, it would also need my RSA keys...
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RSA_SIZE 4096
#define MBEDTLS_GENPRIME
#define MBEDTLS_FS_IO
#define MBEDTLS_ENTROPY_C
#define MBEDTLS_AES_C
#define MBEDTLS_CTR_DRBG_C
#define LICTOOL 1
/* embeds an amalgamation of statically linked mbedtls RSA functionality */
#include "../editors/pro.c"

int main(int argc, char **argv)
{
    mbedtls_rsa_context rsa;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_mpi N, P, Q, D, E;
    int ret = 1;
    int exit_code = 1;
    FILE *f = NULL;
    unsigned char *buf, rnd[256], p[RSA_SIZE];
    unsigned int i, n, m, s;
    const char *pers = "lictool";
    char path[1024], *b;

    /* initialize mbedtls stuff */
    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_rsa_init(&rsa, MBEDTLS_RSA_PKCS_V15, 0 );
    mbedtls_mpi_init(&N); mbedtls_mpi_init(&P); mbedtls_mpi_init(&Q); mbedtls_mpi_init(&D); mbedtls_mpi_init(&E);

    printf("MEG-4 - License Tool by bzt Copyright (C) 2023 GPLv3\n\n");

    /* load the key, generate if not found */
    strcpy(path, argv[0]);
    b = strrchr(path, '/'); if(!b) b = path; else b++;
    strcpy(b, "rs.a");
    if( !(f = fopen(path, "rb")) ||
        (ret = mbedtls_mpi_read_file(&N, 16, f)) != 0 || (ret = mbedtls_mpi_read_file(&E, 16, f)) != 0 ||
        (ret = mbedtls_mpi_read_file(&D, 16, f)) != 0 || (ret = mbedtls_mpi_read_file(&P, 16, f)) != 0 ||
        (ret = mbedtls_mpi_read_file(&Q, 16, f)) != 0) {
        if(f) fclose(f);
        printf("Seeding the random number generator... ");
        fflush(stdout);

        mbedtls_entropy_init( &entropy );
        if((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, (const unsigned char *)pers, 7)) != 0) {
            printf(" failed, mbedtls_ctr_drbg_seed returned -%x\n", -ret);
            goto end;
        }

        printf("OK\nGenerating %d-bit RSA key... ", RSA_SIZE);
        fflush(stdout);

        if((ret = mbedtls_rsa_gen_key(&rsa, mbedtls_ctr_drbg_random, &ctr_drbg, RSA_SIZE, 65537)) != 0) {
            printf("failed, mbedtls_rsa_gen_key returned %d\n", ret);
            goto end;
        }

        if((ret = mbedtls_rsa_export(&rsa, &N, &P, &Q, &D, &E)) != 0) {
            printf("failed, could not export RSA parameters\n");
            goto end;
        }

        if(!(f = fopen(path, "wb+"))) {
            printf("failed, unable to open 'rs.a'\n");
            goto end;
        }

        if( (ret = mbedtls_mpi_write_file("N = " , &N , 16, f)) != 0 || (ret = mbedtls_mpi_write_file("E = " , &E , 16, f)) != 0 ||
            (ret = mbedtls_mpi_write_file("D = " , &D , 16, f)) != 0 || (ret = mbedtls_mpi_write_file("P = " , &P , 16, f)) != 0 ||
            (ret = mbedtls_mpi_write_file("Q = " , &Q , 16, f)) != 0) {
                printf("failed, mbedtls_mpi_write_file returned %d\n", ret);
                goto end;
        }
        n = mbedtls_mpi_size(&rsa.N);
        buf = malloc(n);
        if(!buf) {
            printf("memory alloc error\n");
            goto end;
        }

        mbedtls_mpi_write_binary(&rsa.N, buf, n);
        fprintf(f, "\n\n[%d]= {\n", n);
        for(i = 0; i < n; i++)
            fprintf(f, "%3u%s%s", buf[i], i < n - 1 ? ",":"", (i + 1) % 20 == 0 ? "\n" : "");
        fprintf(f,"\n};\n");
        free(buf);
    } else {
        printf("Checking the private key... ");
        fflush(stdout);
        if((ret = mbedtls_rsa_import(&rsa, &N, &P, &Q, &D, &E)) != 0) {
            printf("failed, mbedtls_rsa_import returned %d\n", ret);
            goto end;
        }
        if((ret = mbedtls_rsa_complete(&rsa)) != 0) {
            printf("failed, mbedtls_rsa_complete returned %d\n", ret);
            goto end;
        }
        if((ret = mbedtls_rsa_check_privkey(&rsa)) != 0) {
            printf("failed, mbedtls_rsa_check_privkey failed with -0x%0x\n", (unsigned int)-ret);
            goto end;
        }
    }
    fclose(f);
    n = mbedtls_mpi_size(&rsa.N);
    mbedtls_mpi_write_binary(&rsa.N, p, n);
    printf("OK\n");
    fflush(stdout);

    /* not enough arguments */
    if(argc == 1) {
        printf("\n  ./lictool \"vendor\"\n  ./lictool -check\n\n");
    } else

    /* check license.txt's validity */
    if(argc >= 2 && argv[1] && argv[1][0] == '-') {
        printf("Checking 'license.txt'... ");
        fflush(stdout);
        if(!(f = fopen(argv[2] ? argv[2] : "license.txt", "rb"))) {
            printf("failed, unable to open\n");
            goto end;
        }
        fseek(f, 0, SEEK_END);
        s = (unsigned int)ftell(f);
        fseek(f, 0, SEEK_SET);
        buf = malloc(s);
        if(!buf) {
            printf("memory alloc error\n");
            goto end;
        }
        if(!fread(buf, 1, s, f)) {
            printf("failed, read error\n");
            goto end;
        }
        fclose(f);
        if(!pro_license(buf, s)) {
            printf("failed, mbedtls_rsa_pkcs1_verify error\n");
            goto end;
        }
        printf("OK\n  Unique: %02X%02X%02X%02X-%02X%02X-%02X%02X-",
            buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);
        for(i = 8; i < 16; i++)
            printf("%02X", buf[i]);
        printf("\n  Vendor: '%s'\n", buf + 64);
        free(buf);
        exit_code = 0;
    } else

    /* generate license.txt */
    if(argc == 2 && argv[1]) {
        printf("Generating 'license.txt'... ");
        fflush(stdout);
        if(!(f = fopen("/dev/urandom", "rb"))) {
            printf("failed, unable to open urandom\n");
            goto end;
        }
        fread(rnd, sizeof(rnd), 1, f);
        fclose(f);
        if(!(buf = pro_licsave(&rsa, argv[1], rnd))) {
            printf("failed, mbedtls_rsa_pkcs1_sign error\n");
            goto end;
        }
        if(!(f = fopen("license.txt", "wb"))) {
            printf("failed, unable to open\n");
            goto end;
        }
        fprintf(f, "---------------------MEG-4 PRO License File---------------------\r\n");
        for(i = 0, m = 2; i < 128; i++, m += 2) {
            fprintf(f, "%02X%02X%s", buf[i], buf[127 - i], m >= 32 ? "\r\n" : "");
            if(i == 100 || i == 104 || i == 114 || i == 118) { fprintf(f, "    "); m += 2; }
            if(m >= 32) m = 0;
        }
        for(i = 0, m = 9; i < rsa.len; i++, m++) {
            fprintf(f, "%02X%s", buf[128 + i], m >= 32 ? "\r\n" : "");
            if(i == 156 || i == 176 || i == 186 || i == 206 || i == 217 || i == 235) { fprintf(f, "  "); m++; }
            if(i == 248 || i == 262 || i == 278 || i == 288) { fprintf(f, "    "); m += 2; }
            if(i == 308) { fprintf(f, "                    "); m += 10; }
            if(m >= 32) m = 0;
        }
        memcpy(buf, "----------------------------------------------------------------\r\n", 67);
        s = strlen(argv[1]);
        memcpy(buf + 31 - s / 2, argv[1], s);
        fprintf(f, "%s", buf);
        free(buf);
        fclose(f);
        printf("OK\n");
        exit_code = 0;
    }

    /* free resources */
end:mbedtls_mpi_free(&N); mbedtls_mpi_free(&P); mbedtls_mpi_free(&Q); mbedtls_mpi_free(&D); mbedtls_mpi_free(&E);
    mbedtls_rsa_free(&rsa);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);
    return exit_code;
}
