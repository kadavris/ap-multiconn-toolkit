/*
  26-Apr-2000
  Buffered io routines. V2.2.1
  (c) Copyright by Andrej Pakhutin
  Portions Copyright by Max Vakulenko
*/
#define AP_BUF_C
#include <io.h>
#include <mem.h>
#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*************************************************************/
void initbuf(ap_buf *b, unsigned size, unsigned char *ptr)
{
  b->ptr = ptr;
  b->size=size;
  b->f = -1;
  b->fill = b->pos = 0u;
  b->written = 0;
}

/*************************************************************/
void clearbuf(ap_buf *b)
{
  b->fill = b->pos = 0u;
  b->written=0;
}

/************************************************************/
unsigned flushbuf(ap_buf *b)
{
unsigned n;

  if (b->mode == AP_BUF_RDONLY)
  {
    fprintf(stderr, "flushbuf(): Attempt to flush read-type buffer!\r\n");
    return 0;
  }

  if (b->written)
  {
    if (b->pos != 0)
      n = write(b->f, b->ptr, b->pos);
    else
      n=0;
  }

  b->stat.writes++;
  b->stat.writes_size += b->pos;
  clearbuf(b);
  return n;
}

/*************************************************************/
void attachbuf(ap_buf *b, int f, char mode)
{
  if (b->f != -1 && b->ptr != NULL && b->size != 0)
  {
    fprintf(stderr, "attachbuf(): warning: using uninitialized or already attached buffer!\n");

    if(b->mode == AP_BUF_WRONLY){
      fprintf(stderr, "attachbuf(): attempting to flush buffer...\r\n");
      flushbuf(b);
    }
  }

  b->f = f;
  b->mode = mode;
  clearbuf(b);

  b->stat.hits = 0ul;
  b->stat.misses = 0ul;
  b->stat.hits_size = 0ul;
  b->stat.misses_size = 0ul;
  b->stat.partial_hits = 0ul;
  b->stat.partial_hits_size = 0ul;
  b->stat.partial_misses_size = 0ul;
  b->stat.reads = 0ul;
  b->stat.writes = 0ul;
  b->stat.reads_size = 0ul;
  b->stat.writes_size = 0ul;
}

/*************************************************************/
void detachbuf(ap_buf *b, int cf)
{
  if (b->mode != AP_BUF_RDONLY) flushbuf(b);
  if (cf) close(b->f);
  b->f = -1;
}

/*************************************************************/
unsigned getbuf(ap_buf *b, void *to, unsigned need)
{
unsigned copied, left;

  if (b->mode == AP_BUF_WRONLY)
  {
    fprintf(stderr, "getbuf(): Attempt to read from write-type buffer!\n");
    return 0;
  }

  for (copied = 0u; copied < need;)
  {
    if(b->fill == 0u || b->pos == b->fill)
    {
      b->pos = 0u;
      if ((b->fill = read(b->f, b->ptr, b->size)) == 0u) return copied;
      b->stat.reads++;
      b->stat.reads_size += b->fill;
    }

    left = need - copied;
    if (left > b->fill - b->pos) left = b->fill - b->pos;

    if (left) memcpy((char*)to + copied, b->ptr + b->pos, left);
    if (b->written) flushbuf(b);
    b->pos += left;
    copied += left;
  }
/*
  b->stat.hits=0ul;
  b->stat.misses=0ul;
  b->stat.hits_size=0ul;
  b->stat.misses_size=0ul;
  b->stat.partial_hits=0ul;
  b->stat.partial_hits_size=0ul;
  b->stat.partial_misses_size=0ul;
*/
  return copied;
}

/*************************************************************/
unsigned putbuf(ap_buf *b, void *from, unsigned need)
{
unsigned copied,left;

  if (b->mode == AP_BUF_RDONLY){
    fprintf(stderr, "putbuf(): Attempt to write to read-type buffer!\n");
    return 0;
  }

  if (need) b->written = 1;
  for (copied = 0; copied < need;)
  {
    if (b->pos == b->size && b->size != flushbuf(b)) return copied;

    left = b->size - b->pos;
    if (need-copied < left) left = need - copied;
    memcpy(b->ptr + b->pos, (char*)from + copied, left);
    b->pos += left;
    copied += left;
  }
/*
  b->stat.hits=0ul;
  b->stat.misses=0ul;
  b->stat.hits_size=0ul;
  b->stat.misses_size=0ul;
  b->stat.partial_hits=0ul;
  b->stat.partial_hits_size=0ul;
  b->stat.partial_misses_size=0ul;
  b->stat.reads=0ul;
  b->stat.writes=0ul;
  b->stat.reads_size=0ul;
  b->stat.writes_size=0ul;
*/
  return copied;
}

/*************************************************************/
int getbufchar(ap_buf *b)
{

  if (b->mode == AP_BUF_WRONLY)
  {
    fprintf(stderr, "getbufchar(): Attempt to read from write-type buffer!\n");
    return 0;
  }

  for(;;)
  {
    if (b->fill == 0u && (b->fill = read(b->f, b->ptr, b->size)) == 0u)
      return -1;

    if (b->fill == b->pos)
    {
      if(b->written) flushbuf(b);
      b->fill = b->pos = 0u;
    }
    else break;
  }

/*
  b->stat.hits=0ul;
  b->stat.misses=0ul;
  b->stat.hits_size=0ul;
  b->stat.misses_size=0ul;
  b->stat.partial_hits=0ul;
  b->stat.partial_hits_size=0ul;
  b->stat.partial_misses_size=0ul;
  b->stat.reads=0ul;
  b->stat.writes=0ul;
  b->stat.reads_size=0ul;
  b->stat.writes_size=0ul;
*/
  return *(b->ptr + (b->pos++));
}

/*************************************************************/
int putbufchar(ap_buf *b, unsigned char c)
{

  if (b->mode == AP_BUF_RDONLY){
    fprintf(stderr, "putbufchar(): Attempt to write to read-type buffer!\n");
    return 0;
  }

  b->written = 1;
  if (b->pos == b->size && b->size!=flushbuf(b))
    return 0;

  *(b->ptr + (b->pos++)) = c;
/*
  b->stat.hits=0ul;
  b->stat.misses=0ul;
  b->stat.hits_size=0ul;
  b->stat.misses_size=0ul;
  b->stat.partial_hits=0ul;
  b->stat.partial_hits_size=0ul;
  b->stat.partial_misses_size=0ul;
  b->stat.reads=0ul;
  b->stat.writes=0ul;
  b->stat.reads_size=0ul;
  b->stat.writes_size=0ul;
*/
  return 1;
}

/*************************************************************/
unsigned getbufstr(ap_buf *b, unsigned char *s, unsigned size)
{
unsigned copied;

fprintf(stderr, "getbufstr(): function not implemented!\n");
exit(1);

  if (b->mode == AP_BUF_WRONLY){ fprintf(stderr, "getbufstr(): Attempt to read from write-type buffer!\n"); return 0; }

  for (copied = 0u, *s = ' ';; ++copied, ++s)
  {
    if (copied == size- 1 ) break; /* max size reached */

    if (b->fill == b->pos) b->fill = b->pos = 0u;

    if (b->fill == 0u && (b->fill = read(b->f, b->ptr, b->size)) == 0u)
      return 0;

    *s = *(b->ptr + b->pos++);

    if (!*s || *s == '\n'){ --s; break; }
    if (*s == '\r'){ --s; continue; }
  }

  *(++s) = '\0';
/*
  b->stat.hits=0ul;
  b->stat.misses=0ul;
  b->stat.hits_size=0ul;
  b->stat.misses_size=0ul;
  b->stat.partial_hits=0ul;
  b->stat.partial_hits_size=0ul;
  b->stat.partial_misses_size=0ul;
  b->stat.reads=0ul;
  b->stat.writes=0ul;
  b->stat.reads_size=0ul;
  b->stat.writes_size=0ul;
*/
  return copied;
}

/*************************************************************/
unsigned putbufstr(ap_buf *b, unsigned char *s)
{
unsigned copied;

fprintf(stderr, "putbufstr(): function not implemented!\n");
exit(1);

  if (b->mode == AP_BUF_RDONLY){ fprintf(stderr, "putbufstr(): Attempt to write to read-type buffer!\n"); return 0; }

  if (*s == '\0') return 0u;

  b->written = 1;
  copied = strlen((char*)s);
  if (copied != putbuf(b, s, copied)) return 0;
  return copied;
}

/*************************************************************/
long skipbuf(ap_buf *b, long need)
{
long left, ls, p, shift;

  if(b->mode == AP_BUF_WRONLY)
  {
    fprintf(stderr, "skipbuf(): Attempt to skip for write-type buffer!\n");
    return 0l;
  }

  if((left=b->fill-b->pos)>=need){ b->pos+=(unsigned)need; return need; }
  if(b->written) flushbuf(b);

  need-=left;
  if(need+(p=tell(b->f)) > (ls=filelength(b->f))) return ls-p;
  ls=lseek(b->f, need, SEEK_CUR);
  b->pos=b->fill=0u;
  return ls-p+left;
/*
  b->stat.hits=0ul;
  b->stat.misses=0ul;
  b->stat.hits_size=0ul;
  b->stat.misses_size=0ul;
  b->stat.partial_hits=0ul;
  b->stat.partial_hits_size=0ul;
  b->stat.partial_misses_size=0ul;
  b->stat.reads=0ul;
  b->stat.writes=0ul;
  b->stat.reads_size=0ul;
  b->stat.writes_size=0ul;
*/
}

/*************************************************************/
long copybuf(ap_buf *in, ap_buf *out, long need, int *status)
{
unsigned l,t;
long copied;

  if(in->mode==AP_BUF_WRONLY){ fprintf(stderr, "copybuf(): Attempt to read from writeonly buffer!\n"); return 0; }
  if(out->mode==AP_BUF_RDONLY){ fprintf(stderr, "copybuf(): Attempt to write to readonly buffer!\n"); return 0; }

  copied=0l;
  *status=1;
  while(need){
    /* if _read returns 0u, then assuming it's just EOF, not error */
    if(in->fill==0u || in->pos==in->fill){
      if(in->written) flushbuf(in);
      if((in->fill=read(in->f, in->ptr, in->size))==0u){ *status=0; break; }
      if(in->fill==0xffff){ *status=-1; break; }
      in->pos=0u;
    }

    if((l=in->fill-in->pos)>need) l=(unsigned)need;
    if(l!=(t=putbuf(out, in->ptr+in->pos, l))){ *status=-2; return copied+(long)t; }
    in->pos+=l; need-=(long)l; copied+=l;
  }
  out->written=1;
  return copied;
/*
  b->stat.hits=0ul;
  b->stat.misses=0ul;
  b->stat.hits_size=0ul;
  b->stat.misses_size=0ul;
  b->stat.partial_hits=0ul;
  b->stat.partial_hits_size=0ul;
  b->stat.partial_misses_size=0ul;
  b->stat.reads=0ul;
  b->stat.writes=0ul;
  b->stat.reads_size=0ul;
  b->stat.writes_size=0ul;
*/
}

/*************************************************************/
/* !!!REWRITE!!! */

long seekbuf(ap_buf *b, long l)
{
long n;

  if (b->written) flushbuf(b);
  if (b->mode == AP_BUF_WRONLY) return lseek(b->f, l, SEEK_SET);

  if ( l < (n = tell(b->f)) - (long)b->fill || l >= n) /* out of buffer */
  {
    l = lseek(b->f, l, SEEK_SET);
    b->fill = read(b->f, b->ptr, b->size);
    b->pos = 0u;
  }
  else
    b->pos = (unsigned)(l - (n - (long)b->fill));

  return l;
/*
  b->stat.hits=0ul;
  b->stat.misses=0ul;
  b->stat.hits_size=0ul;
  b->stat.misses_size=0ul;
  b->stat.partial_hits=0ul;
  b->stat.partial_hits_size=0ul;
  b->stat.partial_misses_size=0ul;
  b->stat.reads=0ul;
  b->stat.writes=0ul;
  b->stat.reads_size=0ul;
  b->stat.writes_size=0ul;
*/
}

/*************************************************************/
long tellbuf(ap_buf *b)
{
  return tell(b->f) - b->fill + b->pos;
}

/*************************************************************/
int fillbuf(ap_buf *b, unsigned char c, long need)
{
  long l;


  if ( b->mode == AP_BUF_RDONLY)
  {
    fprintf(stderr, "fillbuf(): readonly buffer!\n");
    return 0;
  }

  while (need)
  {
    if (b->pos == b->fill)
      flushbuf(b);

    if ((l = b->fill - b->pos) > need)
      l = need;

    memset(b->ptr + b->pos, c, (unsigned)l);
    need -= l;
    b->pos += l;
    b->written = 1;
    b->stat.writes++;
    b->stat.writes_size += l;
  }

  return 1;
}
