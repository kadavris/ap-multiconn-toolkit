/*
   26-Apr-2000
   Buffered io routines. V2.2.1
   (c) Copyright by Andrej Pakhutin
   Portions Copyright by Max Vakulenko
*/
#ifndef AP_BUF_H

#define AP_BUF_H

typedef struct
{
  unsigned long hits;
  unsigned long misses;
  unsigned long hits_size;
  unsigned long misses_size;
  unsigned long partial_hits;
  unsigned long partial_hits_size;
  unsigned long partial_misses_size;
  unsigned long reads;
  unsigned long writes;
  unsigned long reads_size;
  unsigned long writes_size;
} ap_buf_stat;

typedef struct
{
  int f;        /* file handle associated with buffer */
  unsigned char *ptr; /* ptr to the buffer location */
  unsigned pos;    /* current position in buffer */
  unsigned size;    /* buffer size */
  unsigned fill;    /* bytes currently in buffer */
  char mode;      /* read, write or r/w buffer */
  char written;    /* was this buffer pool written to? */
  ap_buf_stat stat;  /* usage statistics */
} ap_buf;

#define BUF_RDONLY  0
#define BUF_WRONLY  1
#define BUF_RDWR    2

#ifndef AP_BUF_C
/*
  parms: buffer_record, size, memptr
  Initializes buffer structure.
*/
extern void initbuf(ap_buf *, unsigned, unsigned char *);

/*
  parms: buffer_record, file handle, I/O mode
  Attaches buffer to the open file
*/
extern void attachbuf(ap_buf *, int, char);

/*
parms: buffer_record
Detaches buffer from the file
*/
extern void detachbuf(ap_buf *, int);

/*
parms: buffer_record, destination, size
Copies "size" bytes from file to "destination"
*/
extern unsigned getbuf(ap_buf *, void *, unsigned);

/*
parms: buffer_record, source, size
Copies "size" bytes from "destination" to file
*/
extern unsigned putbuf(ap_buf *, void *, unsigned);

/*
parms: buffer_record, destination_string, maxsize
Gets text string (ended with CR/LF) with maximal length "maxsize"
from file to "destination"
*/
extern unsigned getbufstr(ap_buf *, unsigned char *, unsigned);

/*
parms: buffer_record, source_string
Writes string from "source" to file (no CR/LF added automatically!)
*/
extern unsigned putbufstr(ap_buf *, unsigned char *);

/* Gets/puts single character from/to file */
extern int getbufchar(ap_buf *);
extern int putbufchar(ap_buf *, unsigned char);

/* clears buffer to make it emty */
extern void clearbuf(ap_buf *);

/* flushes buffer contens if any and if modified */
extern unsigned flushbuf(ap_buf *);

/*
parms: buffer_record, size_to_skip
Skips "size" bytes in file (write-only buffer unsupported)
*/
extern long skipbuf(ap_buf *,long);

/* Copies data directly from one buffer to another */
extern long copybuf(ap_buf *, ap_buf *, long,int *);

/* fills buf with char */
extern int fillbuf(ap_buf *, unsigned char, long);

/* As lseek/tell, but with buffer */
extern long seekbuf(ap_buf *, long);
extern long tellbuf(ap_buf *);

#endif
#endif
