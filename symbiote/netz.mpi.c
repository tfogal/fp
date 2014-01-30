#define _POSIX_C_SOURCE 201201L
#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
#include <fnmatch.h>
#include <dlfcn.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <mpi.h>
#include "debug.h"
#include "parallel.mpi.h"

DECLARE_CHANNEL(netz);

struct field {
  char* name;
  size_t lower; /* offset of this field in bytestream */
  size_t upper; /* final byte in stream +1 for this field */
  bool process;  /* should we output this field? */
};
struct header {
  size_t nghost; /* number of ghost cells, per-dim */
  size_t dims[3]; /* dimensions of each brick, with ghost cells */
  size_t nfields;
  struct field* flds;
  size_t nbricks[3]; /* number of bricks, per dimension */
};

/* we will open any number of files---one per field they want written---and
 * stream the binary data to those files.   this array holds those fields.
 * NULL indicates that the simulation is not doing file I/O at the moment, or
 * is only writing to its header file.  The array will be non-null if we're in
 * the middle of some output files. */
static FILE** binfield = NULL;
/* Where we are in whatever output file we are at now.  Only tracked for the
 * restart files.  We use this to figure out if the current write is within any
 * of the fields the user has decided they want to see. */
static size_t offset = 0; /* current offset in output file. */
/* Header info is parsed when we see the first write to a restart file. */
static struct header hdr = {0};

void
wait_for_debugger()
{
  if(rank() == 0) {
    volatile int x = 0;
    printf("[%ld] Waiting for debugger...\n", (long)getpid());
    while(x == 0) { ; }
  }
  MPI_Barrier(MPI_COMM_WORLD);
}

static void
broadcast_header(struct header* hdr)
{
  broadcastzu(&hdr->nghost, 1);
  broadcastzu(hdr->dims, 3);
  /* with this broadcast it crashes; without, all is klar: */
  broadcastzu(&hdr->nfields, 1);
  if(rank() != 0) {
    hdr->flds = calloc(hdr->nfields, sizeof(struct field));
    printf("allocated %zu fields: %p\n", hdr->nfields, hdr->flds);
  }
  for(size_t i=0; i < hdr->nfields; ++i) {
    size_t len;
    if(rank() == 0) { len = strlen(hdr->flds[i].name); }
    broadcastzu(&len, 1);
    if(rank() != 0) { hdr->flds[i].name = calloc(len+1, sizeof(char)); }
    broadcasts(hdr->flds[i].name, len);
    broadcastzu(&hdr->flds[i].lower, 1);
    broadcastzu(&hdr->flds[i].upper, 1);
    broadcastb(&hdr->flds[i].process, 1);
  }
  broadcastzu(hdr->nbricks, 3);
}

static void
print_header(const struct header hdr)
{
  printf("%zu ghost cells per dim\n", hdr.nghost);
  printf("brick size: %zu x %zu x %zu\n", hdr.dims[0], hdr.dims[1], hdr.dims[2]);
  printf("%zu x %zu x %zu bricks\n", hdr.nbricks[0], hdr.nbricks[1],
         hdr.nbricks[2]);
  printf("%zu fields:\n", hdr.nfields);
  for(size_t i=0; i < hdr.nfields; ++i) {
    printf("\t%-5s at offset %25zu\n", hdr.flds[i].name, hdr.flds[i].lower);
  }
}

/* reads a field name + sets the appropriate/corresponding bit in the header. */
static bool
read_field_config(FILE* fp, struct header* h)
{
  assert(fp);
  assert(h);

  char* fld = NULL;
  const int m = fscanf(fp, "%ms { }", &fld);
  if(feof(fp)) {
    TRACE(netz, "scanning config: EOF");
    free(fld);
    return false;
  }
  if(m != 1) {
    WARN(netz, "could not match field...");
    free(fld);
    return false;
  }
  for(size_t i=0; i < h->nfields; ++i) {
    if(strcasecmp(h->flds[i].name, fld) == 0) {
      TRACE(netz, "will process '%s'", h->flds[i].name);
      h->flds[i].process = 1;
    }
  }
  free(fld);
  return !feof(fp);
}

/* reads the PsiPhi configuration file.  we expect to find a bunch of field
 * names; for each field name in the config file, set the appropriate bit in
 * the 'struct field' within the header. */
static void
read_config(const char* from, struct header* h)
{
  FILE* cfg = fopen(from, "r");
  if(!cfg) {
    ERR(netz, "could not read configuration file '%s'", from);
    return;
  }
  for(size_t i=0; i < h->nfields; ++i) { /* clear field configs. */
    h->flds[i].process = 0;
  }
  while(read_field_config(cfg, h)) { ; }
  fclose(cfg);
}

static void
tjfstart()
{
  /* when we are starting the binary file, we know that we are done with the
   * ascii data descriptor. */
  broadcast_header(&hdr);
  if(rank() != 0) {
    print_header(hdr);
  }
  offset = 0;
  read_config("psiphi.cfg", &hdr);

  /* after reading the config, we should know how many fields we have. */
  assert(binfield == NULL);
  binfield = calloc(hdr.nfields, sizeof(FILE*));
  for(size_t i=0; i < hdr.nfields; ++i) {
    if(hdr.flds[i].process) {
      char fname[256];
      snprintf(fname, 256, "%s.%zu", hdr.flds[i].name, rank());
      binfield[i] = fopen(fname, "wb");
      if(!binfield[i]) {
        ERR(netz, "could not create '%s'", fname);
        return;
      }
    }
  }
  assert(binfield);
}

/* strips a string.  returns the stripped string, but also modifies it. */
static const char*
strip(char** s)
{
  char* nl = strchr(*s, '\n');
  if(nl != NULL) {
    *nl = '\0';
  }
  return *s;
}

static size_t
parse_uint(const char* s)
{
  errno = 0;
  const unsigned long rv = strtoul(s, NULL, 10);
  if(errno != 0) {
    ERR(netz, "error converting '%s' to unsigned number.\n", s);
    abort();
  }
  return (size_t)rv;
}

/* the fields from PsiPhi are padded with "_"s so that they are all the same
 * length.  fix that padding so they are displayed nicely. */
static void
remove_underscores(char* str)
{
  for(char* s=str; *s; ++s) {
    if(*s == '_') { *s = '\0'; break; }
  }
}

static struct header
read_header(const char *filename)
{
  struct header rv;
  TRACE(netz, "[%zu] reading header from %s", rank(), filename);
  FILE* fp = fopen(filename, "r");
  if(!fp) {
    ERR(netz, "[%zu] error reading header file %s: %d", rank(), filename,
        errno);
    return rv;
  }
  char* line;
  size_t llen;
  ssize_t bytes;
  size_t field = 0; /* which field we are reading now */
  do {
    line = NULL;
    llen = 0;
    errno = 0;
    bytes = getline(&line, &llen, fp);
    if(bytes == -1 && errno != 0) {
      ERR(netz, "getline error: %d", errno);
      free(line);
      break;
    } else if(bytes == -1) {
      free(line);
      break;
    }
    strip(&line);
    if(strncasecmp(line, "nG", 2) == 0) {
      rv.nghost = parse_uint(line+3); /* +3 for "nG=" */
    } else if(strncasecmp(line, "ImaAll", 6) == 0) {
      rv.dims[0] = parse_uint(line+7);
    } else if(strncasecmp(line, "JmaAll", 6) == 0) {
      rv.dims[1] = parse_uint(line+7);
    } else if(strncasecmp(line, "KmaAll", 6) == 0) {
      rv.dims[2] = parse_uint(line+7);
    } else if(strncasecmp(line, "nF", 2) == 0) {
      rv.nfields = parse_uint(line+3);
      rv.flds = calloc(sizeof(struct field), rv.nfields);
    } else if(strncasecmp(line, "dimsI", 5) == 0) {
      rv.nbricks[0]= parse_uint(line+6);
    } else if(strncasecmp(line, "dimsJ", 5) == 0) {
      rv.nbricks[1]= parse_uint(line+6);
    } else if(strncasecmp(line, "dimsK", 5) == 0) {
      rv.nbricks[2]= parse_uint(line+6);
    } else if(strncasecmp(line, "FieldNames", 10) == 0) {
      for(size_t i=0; i < rv.nfields; ++i) {
        errno = 0;
        free(line); line = NULL;
        bytes = getline(&line, &llen, fp);
        if(bytes == -1) {
          ERR(netz, "error while processing fields; is nF wrong?!");
          exit(EXIT_FAILURE);
        }
        rv.flds[field].name = strdup(strip(&line));
        remove_underscores(rv.flds[field].name);
        const size_t fldsize = rv.dims[0]*rv.dims[1]*rv.dims[2]*sizeof(float);
        rv.flds[field].lower = fldsize * field;
        rv.flds[field].upper = rv.flds[field].lower + fldsize;
        rv.flds[field].process = false;
        field++;
      }
    }
    free(line);
  } while(bytes > 0);
  fclose(fp);
  return rv;
}

static void
free_header(struct header* head)
{
  for(size_t i=0; i < head->nfields; ++i) {
    free(head->flds[i].name);
    head->flds[i].name = NULL;
  }
  free(head->flds);
  head->flds = NULL;
  head->nfields = 0;
}

__attribute__((destructor)) static void
cleanup()
{
  free_header(&hdr);
}

#define minzu(a, b) \
  ({ const size_t x=a; const size_t y=b; x < y ? x : y; })

/* which bytes intersect with the ones we want to write?
 * @param skip number of bytes to skip before we start writing
 * @param nwrite number of bytes to write, maxes out at 'len'.
 * @return true if there is a non-zero byte overlap */
static bool
byteintersect(const size_t lower, const size_t upper, const struct field fld,
              size_t* skip, size_t* nwrite)
{
  assert(upper > 0);
  assert(lower < upper);
  if(upper < fld.lower || lower >= fld.upper) {
    return false;
  }
  /* there is an intersection.  one of three cases: (1) the write goes beyond
   * the field; (2) the write starts before the field; (3) the write is
   * completely contained within the field */
  *skip = 0;
  *nwrite = 0;
  if(lower > fld.lower && upper > fld.upper) {
    *skip = 0;
    *nwrite = fld.upper - lower;
  } else if(lower < fld.lower && upper < fld.upper) {
    *skip = fld.lower - lower;
    *nwrite = upper - fld.lower;
  } else {
    assert(lower >= fld.lower && upper < fld.upper);
    *skip = 0;
    *nwrite = upper - lower;
  }
  return true;
}

void
exec(const char* fn, const void* buf, size_t n)
{
  if(fnmatch("*header*", fn, 0) == 0) {
    return; /* parse header when file is closed, not during write. */
  }
  assert(fnmatch("*Restart*", fn, 0) == 0);
  if(NULL == binfield) {
    tjfstart();
  }
  assert(binfield != NULL);
  for(size_t i=0; i < hdr.nfields; ++i) {
    if(!hdr.flds[i].process) { continue; } /* only requested field. */
    size_t skip, nbytes;
    if(byteintersect(offset, offset+n, hdr.flds[i], &skip, &nbytes)) {
      TRACE(netz, "writing %zu bytes [%zu:%zu] (field %s)", nbytes, offset+skip,
            offset+skip+nbytes, hdr.flds[i].name);
      assert(nbytes <= n);
      assert(skip < n);
      const char* pwrt = ((const char*)buf) + skip;
      errno = 0;
      const size_t written = fwrite(pwrt, 1, nbytes, binfield[i]);
      if(written != nbytes) {
        WARN(netz, "short write (%zu of %zu). errno=%d", written, n, errno);
        assert(0);
      }
    }
    assert(!ferror(binfield[i]));
  }
  offset += n;
}

void
finish(const char* fn)
{
  if(fnmatch("*header*", fn, 0) == 0) {
    free_header(&hdr);
    hdr = read_header(fn);
    return;
  }
  if(binfield) {
    for(size_t i=0; i < hdr.nfields; ++i) {
      if(binfield[i]) {
        if(fclose(binfield[i]) != 0) {
          ERR(netz, "error closing field %s: %d", hdr.flds[i].name, errno);
        }
      }
    }
    free(binfield);
    binfield = NULL;
  }
}
