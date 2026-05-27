#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

/*
========================================================
                BINARY FILE WRITER
========================================================

This program serializes structured records into
a custom binary file format.

Features:
- Big-endian integer serialization
- Structured binary headers
- Record-based storage
- Safe binary writing helpers
- Manual byte-level encoding

========================================================
*/


/* =====================================================
                    FILE FORMAT
===================================================== */

#define MAGIC   0xDEADBEEF
#define VERSION 1


/* =====================================================
                    RECORD TYPES
===================================================== */

typedef enum {
    RECORD_TEXT = 1,
    RECORD_NAME = 2,
    RECORD_DATA = 3
} RecordType;


/* =====================================================
                    FILE HEADER
===================================================== */

typedef struct {

    uint32_t magic;

    uint16_t version;

    uint32_t record_count;

} Header;


/* =====================================================
                    RECORD STRUCTURE
===================================================== */

typedef struct {

    uint8_t type;

    uint16_t len;

    const uint8_t *data;

} Record;


/* =====================================================
                    SAFE WRITE
===================================================== */

/*
    Safe wrapper around fwrite().
*/

void write_bytes(FILE *fp,
                 const void *buf,
                 size_t size) {

    if (fwrite(buf, 1, size, fp) != size) {

        perror("fwrite");

        fclose(fp);

        exit(EXIT_FAILURE);
    }
}


/* =====================================================
                    BIG-ENDIAN WRITE
===================================================== */

/*
    Serializes a 32-bit integer in big-endian format.
*/

void write_u32_be(FILE *fp,
                  uint32_t value) {

    uint8_t bytes[4];

    bytes[0] = (value >> 24) & 0xFF;
    bytes[1] = (value >> 16) & 0xFF;
    bytes[2] = (value >> 8) & 0xFF;
    bytes[3] = value & 0xFF;

    write_bytes(fp, bytes, 4);
}


/*
    Serializes a 16-bit integer in big-endian format.
*/

void write_u16_be(FILE *fp,
                  uint16_t value) {

    uint8_t bytes[2];

    bytes[0] = (value >> 8) & 0xFF;
    bytes[1] = value & 0xFF;

    write_bytes(fp, bytes, 2);
}


/* =====================================================
                    SERIALIZATION
===================================================== */

/*
    Writes binary file header.
*/

void write_header(FILE *fp,
                  const Header *h) {

    write_u32_be(fp, h->magic);

    write_u16_be(fp, h->version);

    write_u32_be(fp, h->record_count);
}


/*
    Writes a single record.
*/

void write_record(FILE *fp,
                  const Record *r) {

    write_bytes(fp, &(r->type), 1);

    write_u16_be(fp, r->len);

    write_bytes(fp, r->data, r->len);
}


/* =====================================================
                        MAIN
===================================================== */

int main(void) {

    FILE *fp = fopen("sample.bin", "wb");

    if (!fp) {

        perror("fopen");

        return EXIT_FAILURE;
    }

    Header header = {

        .magic = MAGIC,

        .version = VERSION,

        .record_count = 3
    };


    Record records[] = {

        {
            .type = RECORD_TEXT,
            .len = 5,
            .data = (const uint8_t *)"HELLO"
        },

        {
            .type = RECORD_NAME,
            .len = 2,
            .data = (const uint8_t *)"HI"
        },

        {
            .type = RECORD_DATA,
            .len = 4,
            .data = (const uint8_t *)"ABHI"
        }
    };


    /* Write file header */
    write_header(fp, &header);


    /* Serialize all records */
    for (uint32_t i = 0;
         i < header.record_count;
         i++) {

        if (!records[i].data) {

            fprintf(stderr,
                    "NULL record data\n");

            fclose(fp);

            return EXIT_FAILURE;
        }

        write_record(fp, &records[i]);
    }


    if (fclose(fp) != 0) {

        perror("fclose");

        return EXIT_FAILURE;
    }

    printf("Binary file written successfully.\n");

    return EXIT_SUCCESS;
}
