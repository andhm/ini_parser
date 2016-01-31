/*
  +----------------------------------------------------------------------+
  | Author: Min Huang <andhm@126.com>                                    |
  +----------------------------------------------------------------------+
*/

#ifndef __INI_PARSER_H_
#define __INI_PARSER_H_

#include <stdlib.h>

#define INI_ERROR int
#define FAILURE 0
#define SUCCESS 1

#define __malloc(size) malloc(size)
#define __free(p) free(p)

#define LINE_MAX_LEN 1024
#define KEY_MAX_LEN 64
#define VAL_MAX_LEN 256
#define SECTION_MAX_LEN KEY_MAX_LEN

#ifdef DEBUG
#define DEBUG_LOG(log) printf("%s\n", log);
#define DEBUG_LOG2(log1, log2) printf("%s%s\n", log1, log2);
#define DEBUG_LOG3(log1, log2, log3) printf("%s\n", log1, log2, log3);
#define DEBUG_LOG2_INT(log1, int1) printf("%s%d\n", log1, int1);
#else
#define DEBUG_LOG(log)
#define DEBUG_LOG2(log1, log2)
#define DEBUG_LOG3(log1, log2, log3)
#define DEBUG_LOG2_INT(log1, int1)
#endif

enum parse_state {
    COMMENT = 1,
    SPACE,
    SECTION_BEGIN = 11,
    SECTION_END,
    PARAMETER_KEY,
    PARAMETER_VAL
};

typedef struct _bucket_t {
    unsigned int n_elem;
    char *p_key;
    void *p_val;
    struct _bucket_t *next;
    struct _bucket_t *last;
} bucket_t;

typedef struct {
    unsigned int n_section;
    bucket_t *p_section;
    bucket_t *p_section_last;
} dictionary_t;

INI_ERROR ini_parse_file(char *file, dictionary_t **dict);
INI_ERROR ini_parse_string(char *string, dictionary_t **dict);

void ini_dump(dictionary_t *dict);
void ini_destroy(dictionary_t *dict);

bucket_t *ini_section_find(dictionary_t *dict, char *section_name);
bucket_t *ini_parameter_find(dictionary_t *dict, char *section_name, char *param_key);

char *get_parse_error();


#endif
