#ifndef JSONCDACCORD_H
#define JSONCDACCORD_H

/**
 * NOT THREAD SAFE. This library uses global mutable state:
 *   - json_object *json, *schema, *defs  (validate.c)
 *   - static int _jdac_recursion_depth   (validate.c)
 *   - static storage_node *storagelist_head (validate.c, when JDAC_STORE enabled)
 *
 * Concurrent calls from multiple threads will corrupt state or crash.
 * The defs pointer may also leak across sequential jdac_validate() calls
 * if one schema sets definitions and the next does not clear them.
 */

#include <json-c/json.h>

enum jdac_errors {
    JDAC_ERR_VALID = 0,
    JDAC_ERR_GENERAL_ERROR,
    JDAC_ERR_JSON_NOT_FOUND,
    JDAC_ERR_SCHEMA_NOT_FOUND,
    JDAC_ERR_WRONG_ARGS,
    JDAC_ERR_SCHEMA_ERROR,
    JDAC_ERR_INVALID,
    JDAC_REGEX_MISMATCH,
    JDAC_REGEX_MATCH,
    JDAC_REGEX_COMPILE_FAILED,
    JDAC_ERR_MAX
};

int jdac_validate_file(const char *jsonfile, const char *jsonschemafile);
int jdac_validate(json_object *jobj, json_object *jschema);
int jdac_validate_ex(json_object *jobj, json_object *jschema, json_object **joutput_out);
int jdac_ref_set_localpath(const char *_localpath);

const char *jdac_errorstr(unsigned int jdac_errors);

#endif //__JSONCDACCORD_H
