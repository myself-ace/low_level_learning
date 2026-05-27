#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define MAGIC 0xDEADBEEF


typedef struct {

    uint32_t magic;

    uint16_t version;

    uint32_t record_count;

} Header;


/* =====================================================
                    SAFE READ
===================================================== */

void read_bytes(FILE *fp,
                void *buf,
                size_t size) {

    if (fread(buf, 1, size, fp) != size) {

        perror("fread");

        fclose(fp);

        exit(EXIT_FAILURE);
    }
}


/* =====================================================
                    BIG-ENDIAN READ
===================================================== */

uint32_t read_u32_be(FILE *fp) {

    uint8_t bytes[4];

    read_bytes(fp, bytes, 4);

    return
        ((uint32_t)bytes[0] << 24) |
        ((uint32_t)bytes[1] << 16) |
        ((uint32_t)bytes[2] << 8)  |
        ((uint32_t)bytes[3]);
}


uint16_t read_u16_be(FILE *fp) {

    uint8_t bytes[2];

    read_bytes(fp, bytes, 2);

    return
        ((uint16_t)bytes[0] << 8) |
        ((uint16_t)bytes[1]);
}


/* =====================================================
                        MAIN
===================================================== */

int main(void) {

    FILE *fp = fopen("sample.bin", "rb");

    if (!fp) {

        perror("fopen");

        return EXIT_FAILURE;
    }

    Header h;

    h.magic = read_u32_be(fp);

    h.version = read_u16_be(fp);

    h.record_count = read_u32_be(fp);


    /* Validate file magic */
    if (h.magic != MAGIC) {

        fprintf(stderr,
                "Invalid file format\n");

        fclose(fp);

        return EXIT_FAILURE;
    }

    printf("\n========== HEADER ==========\n");

    printf("Magic        : 0x%X\n",
           h.magic);

    printf("Version      : %u\n",
           h.version);

    printf("Record Count : %u\n",
           h.record_count);


    /* Parse records */
    for (uint32_t i = 0;
         i < h.record_count;
         i++) {

        uint8_t type;

        uint16_t len;

        read_bytes(fp, &type, 1);

        len = read_u16_be(fp);

        uint8_t *buffer = malloc(len + 1);

        if (!buffer) {

            fclose(fp);

            return EXIT_FAILURE;
        }

        read_bytes(fp, buffer, len);

        buffer[len] = '\0';

        printf("\n[RECORD %u]\n", i + 1);

        printf("Type : %u\n", type);

        printf("Len  : %u\n", len);

        printf("Data : %s\n", buffer);

        free(buffer);
    }

    fclose(fp);

    return EXIT_SUCCESS;
}
