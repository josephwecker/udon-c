#define _XOPEN_SOURCE 700
#define _REENTRANT

#include <sys/stat.h>
#include <fcntl.h>
#include "udon.h"

char udon_global_error_msg[128];
UdonError udon_global_error = UDON_NO_ERROR;

UdonParser *udon_new_parser_from_file(char *filename) {
    ssize_t bytes_read;
    int     fd;
    struct  stat statbuf;
    UdonParser *p;

    if( (p = (UdonParser *) udon_malloc(sizeof(UdonParser))) == NULL) {
        udon_error(UDON_OUT_OF_MEMORY_ERROR, "Couldn't allocate memory for parser.");
        return NULL;
    }
    if( (fd = open(filename, O_RDONLY)) < 0) {
        udon_error(UDON_BAD_FILE_ERROR, "Couldn't open file '%s'.", filename);
        return NULL;
    }

    if( fstat(fd, &statbuf) == -1) {
        udon_error(UDON_BAD_FILE_ERROR, "Opened, but couldn't stat file '%s'.", filename);
        return NULL;
    }

    p->size     = statbuf.st_size;
    p->filename = filename;

    // padding to the right so that quickscan stuff can look in bigger chunks
    if( (p->buffer = (char *) udon_malloc(p->size+8)) == NULL)  {
        udon_error(UDON_OUT_OF_MEMORY_ERROR, "Couldn't allocate memory for file contents (file: '%s').", filename);
        return NULL;
    }

    if( (bytes_read = read(fd, p->buffer, p->size)) != p->size) {
        udon_error(UDON_READ_FILE_ERROR, "Only read %zd of %zd bytes from file '%s'.", bytes_read, p->size, filename);
        return NULL;
    }

    udon_reset_parser(p);
    close(fd);
    return p;
}

int udon_parse(UdonParser *p) {
}

int udon_reset_parser(UdonParser *p) {
    p->curr = p->buffer;
    p->end = &(p->buffer[p->size - 1]);

    p->qcurr = (uint64_t *) p->buffer;
    p->qsize = p->size >> 3; // size in 64b chunks
    p->qend = &(p->qcurr[p->qsize - 1]);

    p->curr[p->size] = 0; // last chance null terminator, just in case.
}

int udon_free_parser(UdonParser *p) {

}


