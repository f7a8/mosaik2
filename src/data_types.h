
#ifndef m2types
#define m2types
typedef uint32_t      m2elem;
typedef time_t        m2time;
typedef uint8_t       m2tile;
typedef uint8_t       m2rezo;

typedef char*         m2name, m2text;
typedef char* restrict m2rtext;
typedef const char*   m2ctext;
typedef FILE*         m2file;
typedef uint8_t       m2data, *m2datap;
typedef void*         m2addr;
typedef size_t        m2size;
typedef off_t         m2off;
typedef struct {
	m2elem start_x;
	m2elem start_y;
	m2elem end_x;
	m2elem end_y;
}                     m2area;
typedef uint8_t hash[16];
typedef hash m2hash;
#endif
