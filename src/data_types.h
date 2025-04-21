
#ifndef m2types
#define m2types
typedef uint32_t      m2elem;
typedef time_t        m2time;
typedef uint8_t       m2tile;
typedef uint8_t       m2rezo;

//typedef char[MAX_FILENAME_LEN] m2cname, m2ctext; // TODO works ??

typedef char*         m2name;
typedef char *        m2text;
typedef char* restrict m2rtext;
typedef const char*   m2cname;
typedef FILE*         m2file;
typedef int			  m2fd;
typedef int           m2ftype; // file type
typedef uint8_t       m2data, *m2datap;
typedef void*         m2addr;
typedef size_t        m2size;
typedef off_t         m2off;
typedef uint8_t       m2orient;
typedef struct {
	m2elem start_x;
	m2elem start_y;
	m2elem end_x;
	m2elem end_y;
} m2area;
typedef uint8_t hash[16];  
typedef hash m2hash;
typedef unsigned long long phash;  
typedef struct {
	m2hash hash;
	m2size pos; // offset in the unindex hash file of that index
} m2hashidx;
typedef struct {
	phash hash;
	int val;//return value from phash function
} m2phash;
#endif
