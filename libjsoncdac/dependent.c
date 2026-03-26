
#include <stdio.h>
#include <string.h>
#include <json-c/json.h>
#include "../include/jsoncdaccord.h"
#include "../include/jdac_internal.h"
#include "../include/jdac_optional.h"

int _jdac_check_dependentrequired(json_object *jobj, json_object *jschema,
                                  json_object *joutput_node)
{
    json_object *j_required = json_object_object_get(jschema, "dependentRequired");
    if (!j_required)
        return JDAC_ERR_VALID;

    json_object *jdepreq_node =
        _jdac_output_create_and_append_node(joutput_node, "dependentRequired");

    if (!json_object_is_type(j_required, json_type_object)) {
        _jdac_output_apply_result(jdepreq_node, JDAC_ERR_SCHEMA_ERROR);
        return JDAC_ERR_SCHEMA_ERROR;
    }

    json_object_object_foreach(j_required, jprop_key, jprop_val)
    {
        json_object *jdepreq_key = _jdac_output_create_and_append_node(jdepreq_node, jprop_key);

        if (!json_object_is_type(jprop_val, json_type_array)) {
            _jdac_output_apply_result(jdepreq_key, JDAC_ERR_SCHEMA_ERROR);
            _jdac_output_apply_result(jdepreq_node, JDAC_ERR_SCHEMA_ERROR);
            return JDAC_ERR_SCHEMA_ERROR;
        }

        int arraylen = json_object_array_length(jprop_val);
        if (arraylen == 0) {
            _jdac_output_apply_result(jdepreq_key, JDAC_ERR_SCHEMA_ERROR);
            _jdac_output_apply_result(jdepreq_node, JDAC_ERR_SCHEMA_ERROR);
            return JDAC_ERR_SCHEMA_ERROR;
        }

        for (int i = 0; i < arraylen; i++) {
            json_object *jitem = json_object_array_get_idx(jprop_val, i);
            char numstr[11];
            snprintf(numstr, sizeof(numstr), "%d", i);

            if (!json_object_is_type(jitem, json_type_string)) {
                json_object *jdepreq_array_item =
                    _jdac_output_create_and_append_node(jdepreq_key, numstr);
                _jdac_output_apply_result(jdepreq_node, JDAC_ERR_SCHEMA_ERROR);
                _jdac_output_apply_result(jdepreq_key, JDAC_ERR_SCHEMA_ERROR);
                _jdac_output_apply_result(jdepreq_array_item, JDAC_ERR_SCHEMA_ERROR);
                return JDAC_ERR_SCHEMA_ERROR;
            }
            const char *musthavestring = json_object_get_string(jitem);
            if (!json_object_object_get(jobj, musthavestring)) {
                json_object *jdepreq_array_item =
                    _jdac_output_create_and_append_node(jdepreq_key, numstr);
                _jdac_output_apply_result(jdepreq_node, JDAC_ERR_INVALID);
                _jdac_output_apply_result(jdepreq_key, JDAC_ERR_INVALID);
                _jdac_output_apply_result(jdepreq_array_item, JDAC_ERR_INVALID);
                return JDAC_ERR_INVALID;
            }
        }
        _jdac_output_apply_result(jdepreq_key, JDAC_ERR_VALID);
    }
    _jdac_output_apply_result(jdepreq_node, JDAC_ERR_VALID);
    return JDAC_ERR_VALID;
}

/* Draft-07 "dependencies": values can be arrays (like dependentRequired)
 * or schemas (like dependentSchemas). Only evaluated when the key is present. */
int _jdac_check_dependencies(json_object *jobj, json_object *jschema,
                              json_object *joutput_node)
{
    json_object *jdeps = json_object_object_get(jschema, "dependencies");
    if (!jdeps)
        return JDAC_ERR_VALID;

    if (!json_object_is_type(jdeps, json_type_object)) {
        json_object *jnode = _jdac_output_create_and_append_node(joutput_node, "dependencies");
        _jdac_output_apply_result(jnode, JDAC_ERR_SCHEMA_ERROR);
        return JDAC_ERR_SCHEMA_ERROR;
    }

    json_object *jdeps_node = _jdac_output_create_and_append_node(joutput_node, "dependencies");
    int deps_ok = 1;

    json_object_object_foreach(jdeps, dep_key, dep_val)
    {
        /* Only apply dependency if the key is present in the object */
        json_object *jpresent = NULL;
        if (!json_object_object_get_ex(jobj, dep_key, &jpresent))
            continue;

        if (json_object_is_type(dep_val, json_type_array)) {
            /* Array form: all listed properties must be present */
            int arraylen = json_object_array_length(dep_val);
            for (int i = 0; i < arraylen; i++) {
                json_object *jitem = json_object_array_get_idx(dep_val, i);
                if (!json_object_is_type(jitem, json_type_string)) {
                    json_object *jfail_node =
                        _jdac_output_create_and_append_node(jdeps_node, dep_key);
                    _jdac_output_apply_result(jfail_node, JDAC_ERR_SCHEMA_ERROR);
                    _jdac_output_apply_result(jdeps_node, JDAC_ERR_SCHEMA_ERROR);
                    return JDAC_ERR_SCHEMA_ERROR;
                }
                const char *required_key = json_object_get_string(jitem);
                json_object *jcheck = NULL;
                if (!json_object_object_get_ex(jobj, required_key, &jcheck)) {
                    json_object *jfail_node =
                        _jdac_output_create_and_append_node(jdeps_node, dep_key);
                    _jdac_output_apply_result(jfail_node, JDAC_ERR_INVALID);
                    deps_ok = 0;
                    break;
                }
            }
        } else if (json_object_is_type(dep_val, json_type_object) ||
                   json_object_is_type(dep_val, json_type_boolean)) {
            /* Schema form: object must validate against the schema */
            json_object *jdep_schema_node =
                _jdac_output_create_and_append_node(jdeps_node, dep_key);
            int err = _jdac_validate_instance(jobj, dep_val, jdep_schema_node);
            if (err) {
                _jdac_output_apply_result(jdep_schema_node, err);
                deps_ok = 0;
            }
        }
    }

    int ret = deps_ok ? JDAC_ERR_VALID : JDAC_ERR_INVALID;
    _jdac_output_apply_result(jdeps_node, ret);
    return ret;
}
