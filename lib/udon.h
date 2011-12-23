#ifndef UDON_H
#define UDON_H

#define _XOPEN_SOURCE 700
#define _REENTRANT

#include <stdint.h>
#include <sys/types.h>
#include <sysexits.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

// (for c++ inclusion)
#ifdef __cplusplus
extern "C" {
#endif

// --- Errors ---
    enum UdonError {
        UDON_NO_ERROR            = 0,
        UDON_OUT_OF_MEMORY_ERROR = EX_OSERR,
        UDON_BAD_FILE_ERROR      = EX_NOINPUT,
        UDON_READ_FILE_ERROR     = EX_DATAERR
    };
    typedef enum UdonError UdonError;

    extern UdonError udon_global_error;
    extern char udon_global_error_msg[128];


// --- Overall state of the parser ---
    enum UdonParseState {
        UDON_FINISHED_OK,
        UDON_FINISHED_WARNINGS,
        UDON_ABORTED_ERROR
    };
    typedef enum UdonParseState UdonParseState;

// --- Parser struct, mostly opaque, holding state etc. ---
    struct UdonParser {
        // --- Set these ---
        char             *buffer;
        ssize_t          size;
        char             *filename; // optional, of course

        // --- Current State ---
        void             *result;
        UdonParseState   state;

        // --- Generally Opaque ---
        char             *curr;
        uint64_t         *qcurr;
        char             *end;
        uint64_t         *qend;
        ssize_t          qsize; // Automatically calculated, for quickscans.
    };
    typedef struct UdonParser UdonParser;

    UdonParser *udon_new_parser_from_buffer(char *buffer, ssize_t length);
    UdonParser *udon_new_parser_from_file(char *filename);
    int udon_parse(UdonParser *parser);

    // --- udon_reset_parser(UdonParser *) - assumes parser is allocated and
    // that it has a buffer and size specified at the very least. Doesn't free
    // any results or anything at the moment...
    int udon_reset_parser(UdonParser *parser);
    int udon_free_parser(UdonParser *parser);

// --- Allow setting different malloc/free routines ---
#ifdef _UDON_NONSTANDARD_MEMORY
    void *udon_calloc(size_t count, size_t size);
    void *udon_malloc(size_t size);
    void  udon_free(void *ptr, size_t size);
#else

#define udon_calloc(count,size) calloc(count,size)
#define udon_malloc(size)       malloc(size)
#define udon_free(ptr,size)     free(ptr)
#endif

    // --- Error / Warning handling ---
    static inline void udon_error(UdonError type, char *msg, ...) {
        va_list argp;
        va_start(argp, msg);
        vsnprintf(udon_global_error_msg, 128, msg, argp);
        udon_global_error = type;
    }

    static inline void udon_emit_error(FILE *outf) {
        fprintf(outf, "%s\n", udon_global_error_msg, &udon_global_error_msg);
    }

    static inline int udon_error_value() {
        return (int)udon_global_error;
    }

// (for c++ inclusion)
#ifdef __cplusplus
}
#endif

#endif
