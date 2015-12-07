#include "ini_parser.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

static char p_error[128] = {0};
static enum parse_state curr_parse_state;
static bucket_t *curr_parse_section = NULL;
static unsigned int curr_line = 0;

static INI_ERROR ini_parse_section(char *line, dictionary_t *dict);
static INI_ERROR ini_parse_parameter(char *line, bucket_t *section);
static INI_ERROR ini_parse(char *line, dictionary_t *dict);
static void set_parse_error(char *err_msg);
static bucket_t *_ini_parameter_find(bucket_t *section, char *param_key);

INI_ERROR ini_parse_file(char *file, dictionary_t **dict) {
    if (!file) {
        set_parse_error("Invalid ini file.");
        return FAILURE;
    }

    if (!dict) {
        set_parse_error("Invalid dictionary address.");
        return FAILURE;
    }

    FILE *fp;
    if ((fp = fopen(file, "r")) == NULL) {
        set_parse_error("Open ini file failed.");
        return FAILURE;
    }

    *dict = __malloc(sizeof(dictionary_t));
    if (*dict == NULL) {
        fclose(fp);
        set_parse_error("__malloc for dictionary failed.");
        return FAILURE;
    }
    (*dict)->n_section = 0;
    (*dict)->p_section = (*dict)->p_section_last = NULL;

    char p_line[LINE_MAX_LEN];
    while (fgets(p_line, LINE_MAX_LEN, fp)) {
		++curr_line;
        DEBUG_LOG(p_line);
        INI_ERROR err = ini_parse(p_line, *dict);
        if (err == FAILURE) {
            fclose(fp);
            return FAILURE;
        }
    }

    fclose(fp);
    return SUCCESS;
}

INI_ERROR ini_parse_string(char *string, dictionary_t **dict) {
    if (!string) {
        set_parse_error("Invalid ini string.");
        return FAILURE;
    }

    if (!dict) {
        set_parse_error("Invalid dictionary address.");
        return FAILURE;
    }

    *dict = __malloc(sizeof(dictionary_t));
    if (*dict == NULL) {
        set_parse_error("__malloc for dictionary failed.");
        return FAILURE;
    }
    (*dict)->n_section = 0;
    (*dict)->p_section = (*dict)->p_section_last = NULL;

    char *p_str = string;
    char p_line[LINE_MAX_LEN];
    unsigned int l_str = strlen(string);
    char *p_start = NULL;
    while (p_str - string < l_str) {
        if (!p_start) {
            p_start = p_str;
        }
        if (*p_str == '\n') {
            unsigned int l_line = p_str - p_start + 1;
            l_line = l_line > LINE_MAX_LEN-1 ? LINE_MAX_LEN-1 : l_line;
            strncpy(p_line, p_start, l_line);
            p_line[l_line] = '\0';
            p_start = NULL;
            ++curr_line;
            DEBUG_LOG(p_line);
            INI_ERROR err = ini_parse(p_line, *dict);
            if (err == FAILURE) {
                return FAILURE;
            }
        }
        ++p_str;
    }

    return SUCCESS;
}

INI_ERROR ini_parse(char *line, dictionary_t *dict) {
    char *p_line = line;
    while (isspace(*p_line) && *p_line!='\n') {
        ++p_line;
    }

    // space
    if (*p_line == '\n') {
        DEBUG_LOG("(SPACE)");
        curr_parse_state = SPACE;
        return SUCCESS;
    }

    // comment
    if (*p_line == ';') {
        DEBUG_LOG("(COMMENT)");
        curr_parse_state = COMMENT;
        return SUCCESS;
    }

    // section
    if (*p_line == '[') {
        return ini_parse_section(p_line, dict);
    }

    // param
    if (!curr_parse_section) {
        set_parse_error("Ini parse error: parameter parse must be within section.");
        return FAILURE;
    }

    return ini_parse_parameter(p_line, curr_parse_section);
}

static INI_ERROR ini_parse_section(char *line, dictionary_t *dict) {
    char *p_line = line;
    DEBUG_LOG("(SECTION_BEGIN)");
    curr_parse_state = SECTION_BEGIN;
    ++p_line;
    if (*p_line == ']') {
        set_parse_error("Ini parse error: section name empty.");
        return FAILURE;
    }

    static char p_section_name[SECTION_MAX_LEN+1] = {0};
    int section_name_len = 0;
    while (*p_line != ']' && *p_line != '\n') {
        if (section_name_len < SECTION_MAX_LEN) {
            p_section_name[section_name_len++] = *p_line;
        }
        ++p_line;
    }
    p_section_name[section_name_len] = '\0';
    if (*p_line == '\n') {
        set_parse_error("Ini parse error: section name expect end char ']'");
        return FAILURE;
    }

    if (*p_line == ']') {
        do {
            ++p_line;
        } while (isspace(*p_line) && *p_line!='\n');
        if (*p_line != '\n') {
            set_parse_error("Ini parse error: there are some chars out of section name end char ']' in the same line.");
            return FAILURE;
        }
        DEBUG_LOG("(SECTION_END)");
        curr_parse_state = SECTION_END;
        bucket_t *section = ini_section_find(dict, p_section_name);
        if (!section) {
            DEBUG_LOG2("not found section:", p_section_name);
            section = __malloc(sizeof(bucket_t));
            if (section == NULL) {
                set_parse_error("Ini parse error: __malloc for section failed.");
                return FAILURE;
            }
            char *p_key = __malloc(sizeof(char)*(section_name_len+1));
            if (p_key == NULL) {
                set_parse_error("Ini parse error: __malloc for section.name failed.");
                return FAILURE;
            }
            strcpy(p_key, p_section_name);
            DEBUG_LOG2("new section name:", p_key);
            section->p_key = p_key;
            section->p_val = NULL;
            section->n_elem = 0;
            section->next = section->last = NULL;

            // link
            if (dict->p_section_last == NULL) {
                dict->p_section = section;
            } else {
                dict->p_section_last->next = section;
            }
            dict->p_section_last = section;
            ++dict->n_section;
        }

        curr_parse_section = section;
        
        return SUCCESS;
    }

    // can not come here
    set_parse_error("Ini parse error: Unknown error.");
    return FAILURE;
}

static INI_ERROR ini_parse_parameter(char *line, bucket_t *section) {
    if (!section) {
        set_parse_error("Ini parse error: parameter parse must be within section.");
        return FAILURE;
    }

    char *p_line = line;
    DEBUG_LOG2("current parse section:", section->p_key);
    DEBUG_LOG2_INT("current parse section elements:", section->n_elem);
    DEBUG_LOG("(PARAMETER_KEY)");
    curr_parse_state = PARAMETER_KEY;
    static char p_key[KEY_MAX_LEN+1] = {0};
    int key_len = 0;
    do {
        if (key_len < KEY_MAX_LEN) {
            p_key[key_len++] = *p_line;
        }
        ++p_line;
    } while (!isspace(*p_line) && *p_line!='=');
    p_key[key_len] = '\0';
    if (*p_line == '\n') {
        set_parse_error("Ini parse error: invalid parameter key.");
        return FAILURE;
    }
    if (*p_line == ' ') {
        do {
            ++p_line;
        } while (*p_line == ' ');
    }

    if (*p_line != '=') {
        set_parse_error("Ini parse error: invalid parameter key.");
        return FAILURE;
    }

    ++p_line;

    DEBUG_LOG2("parameter key:", p_key);
    DEBUG_LOG("(PARAMETER_VAL)");
    curr_parse_state = PARAMETER_VAL;
    static char p_val[VAL_MAX_LEN+1] = {0};
    int val_len = 0;
    while (isspace(*p_line) && *p_line!='\n') {
        ++p_line;
    }
    if (*p_line == '\n') {
        p_val[0] = '\0';
    } else {
		char *p_quote_begin = NULL;
		char *p_quote_end = NULL;
        do {
			if (*p_line == ';') {
				if (!p_quote_begin || p_quote_end) {
					// comment
					DEBUG_LOG("it's a comment in parameter line.");
					break;		
				}
			}
            if (val_len < VAL_MAX_LEN) {
                p_val[val_len++] = *p_line;
            }
			if (*p_line == '"') {
				if (p_quote_begin) {
					p_quote_end = p_line;
				} else {
					p_quote_begin = p_line;
				}
			}
            ++p_line;
        } while (!isspace(*p_line) || *p_line==' ' || *p_line == '\t');
        char *p_val_p = p_val + val_len -1;
        while (p_val_p > p_val) {
            if (*p_val_p == ' ' || *p_val_p == '\t') {
                --val_len;
            }
            --p_val_p;
        }
        p_val[val_len] = '\0';
        /*while (isspace(*p_line) && *p_line!='\n') {
            ++p_line;
        }*/
        if (*p_line != '\n' && *p_line != ';') {
            set_parse_error("Ini parse error: invalid parameter value.");
            return FAILURE;
        }
    }

    DEBUG_LOG2("parameter val:", p_val);
    bucket_t *p_parameter = NULL;
    if (section->n_elem) {
        p_parameter = _ini_parameter_find(section, p_key);
        if (p_parameter) {
            DEBUG_LOG2("found parameter key:", p_parameter->p_key);
            if (!strlen(p_val)) {
                // empty
                if (p_parameter->p_val) {
                    __free(p_parameter->p_val);
                    p_parameter->p_val = NULL;
                }
            } else {
                if (p_parameter->p_val) {
                    if (strcmp((char*)p_parameter->p_val, p_val)) {
                        __free(p_parameter->p_val);
                        p_parameter->p_val = __malloc(sizeof(char)*(val_len+1));
                        if (p_parameter->p_val == NULL) {
                            set_parse_error("Ini parse error: __malloc for parameter.value failed.");
                            return FAILURE;
                        }
                    }
                } else {
                    p_parameter->p_val = __malloc(sizeof(char)*(val_len+1));
                    if (p_parameter->p_val == NULL) {
                        set_parse_error("Ini parse error: __malloc for parameter.value failed.");
                        return FAILURE;
                    }
                }
                strcpy(p_parameter->p_val, p_val);
            }

            return SUCCESS;
        }

        DEBUG_LOG2("not found parameter key:", p_key);
    }

    p_parameter = __malloc(sizeof(bucket_t));
    if (p_parameter == NULL) {
        set_parse_error("Ini parse error: __malloc for parameter failed.");
        return FAILURE;
    }
    p_parameter->n_elem = 1;
    p_parameter->next = p_parameter->last = NULL;
    p_parameter->p_key = __malloc(sizeof(char)*(key_len+1));
    if (p_parameter->p_key == NULL) {
        set_parse_error("Ini parse error: __malloc for parameter.key failed.");
        return FAILURE;
    }
    strcpy(p_parameter->p_key, p_key);
    p_parameter->p_val = __malloc(sizeof(char)*(val_len+1));
    if (p_parameter->p_val == NULL) {
        set_parse_error("Ini parse error: __malloc for parameter.val failed.");
        return FAILURE;
    }
    strcpy(p_parameter->p_val, p_val);

    DEBUG_LOG2("init parameter success. p_key:", p_key);

    if (section->n_elem) {
        section->last->next = p_parameter;
    } else {
        section->p_val = p_parameter;
    }

    section->last = p_parameter;
    ++section->n_elem;

    return SUCCESS;
}

void ini_destroy(dictionary_t *dict) {
    if (!dict) {
        return;
    }

    bucket_t *section = dict->p_section;
    while (section) {
        bucket_t *parameter = (bucket_t*)section->p_val;
        while (parameter) {
            __free(parameter->p_key);
            if (parameter->p_val) {
                __free(parameter->p_val);
            }
            bucket_t *next_parameter = parameter->next;
            __free(parameter);
            parameter = next_parameter;
        }
        __free(section->p_key);
        bucket_t *next_section = section->next;
        __free(section);
        section = next_section;
    }
    __free(dict);
}

void ini_dump(dictionary_t *dict) {
    if (!dict) {
        return;
    }

    printf("Section Num:\t%d\n", dict->n_section);
    bucket_t *section = dict->p_section;
    while (section) {
        printf("===================================\n");
        printf("[%s]\n", section->p_key);

        bucket_t *parameter = (bucket_t*)section->p_val;
        while (parameter) {
            printf("%s\t= %s\n", parameter->p_key, parameter->p_val ? (char*)parameter->p_val : "");
            parameter = parameter->next;
        }
        section = section->next;
    }
    printf("\n");
}

bucket_t *ini_section_find(dictionary_t *dict, char *section_name) {
    if (!dict || 
        !section_name || 
        !dict->n_section) {
        return NULL;
    }

    bucket_t *p_bucket = dict->p_section;
    while (p_bucket) {
        if (!strcmp(p_bucket->p_key, section_name)) {
            return p_bucket;
        }
        p_bucket = p_bucket->next;
    }

    return NULL;
}

static bucket_t *_ini_parameter_find(bucket_t *section, char *param_key) {
    if (!section ||
        !param_key ||
        !section->n_elem) {
        return NULL;
    }

    bucket_t *p_bucket = (bucket_t*)section->p_val;
    while (p_bucket) {
        if (!strcmp(p_bucket->p_key, param_key)) {
            return p_bucket;
        }
        p_bucket = p_bucket->next;
    }

    return NULL;
}

bucket_t *ini_parameter_find(dictionary_t *dict, char *section_name, char *param_key) {
    if (!dict ||
        !dict->n_section ||
        !section_name ||
        !param_key) {
        return NULL;
    }

    bucket_t *p_section = ini_section_find(dict, section_name);
    if (!p_section) {
        return NULL;
    }

    return _ini_parameter_find(p_section, param_key);
}

char *get_parse_error() {
    return p_error;
}

static void set_parse_error(char *err_msg) {
	char err_buf[128];
	sprintf(err_buf, "%s%s%d", err_msg, " at line ", curr_line);	
	strcpy(p_error, err_buf);
}
