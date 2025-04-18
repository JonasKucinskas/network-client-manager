#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <json-glib/json-glib.h>
#include <gtk/gtk.h>
#include "headers/utils.h"
#include "headers/widgets.h"
#include "headers/api.h"
#include "headers/wan.h"

gchar *username = NULL;
gchar *password = NULL;
gchar *base_url = NULL;

size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    mem->memory = realloc(mem->memory, mem->size + realsize + 1);
    
    if(mem->memory == NULL) 
    {
        printf("not enough memory (realloc returned NULL)\n");
        return 0;
    }

    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

//appends json structure to existing tree
void json_tree_draw(JsonNode *node, GtkTreeStore *treestore, GtkTreeIter *iter) 
{
    GtkTreeIter child;
    switch (json_node_get_node_type(node))    
    {
        case JSON_NODE_OBJECT:    
        {
            JsonObject *object = json_node_get_object(node);
            GList *members = json_object_get_members(object);

            for (GList *l = members; l != NULL; l = l->next)    
            {
                const gchar *key = (const gchar *) l->data;
                JsonNode *value = json_object_get_member(object, key);

                gtk_tree_store_append(treestore, &child, iter);
                //0th column, obj name value:
                gtk_tree_store_set(treestore, &child, 0, key, -1);
                
                json_tree_draw(value, treestore, &child);
            }

            g_list_free(members);
            break;
        }
        case JSON_NODE_ARRAY: 
        {
            JsonArray *array = json_node_get_array(node);
            guint length = json_array_get_length(array);
            
            for (guint i = 0; i < length; i++)    
            {
                JsonNode *arrayElement = json_array_get_element(array, i);
                
                GValue gvalue = G_VALUE_INIT;
                json_node_get_value(arrayElement, &gvalue);
                gchar *strVal = g_strdup_value_contents(&gvalue);

                gtk_tree_store_append(treestore, &child, iter);
                //0th column, obj name value:
                gtk_tree_store_set(treestore, &child, 0, strVal, -1);

                g_value_unset(&gvalue);
                g_free(strVal);
                json_tree_draw(arrayElement, treestore, iter);
            }
            break;
        }
        case JSON_NODE_VALUE: 
        {
            GValue gvalue = G_VALUE_INIT;
            json_node_get_value(node, &gvalue);

            gchar *strVal = g_strdup_value_contents(&gvalue);
            
            gtk_tree_store_append(treestore, iter, &child);
            //1st column, property value:
            gtk_tree_store_set(treestore, iter, 1, strVal, -1);

            g_value_unset(&gvalue);
            g_free(strVal);
            break;
        }
        default:
            printf("Unknown type\n");
            break;
    }
}

static gchar* json_get_string(JsonNode *root, const char* json_path)
{
    JsonPath *path = json_path_new();
    
    GError *error = NULL;
    json_path_compile(path, json_path, &error);
    
    JsonNode *path_match = json_path_match(path, root);
    JsonArray *array = json_node_get_array(path_match);
    JsonNode *arrayElement = json_array_get_element(array, 0);
            
    GValue gvalue = G_VALUE_INIT;
    json_node_get_value(arrayElement, &gvalue);
    gchar *strVal = g_strdup_value_contents(&gvalue);

    //remove \" from string
    gchar *cleanedStr = g_strndup(strVal + 1, strlen(strVal) - 2);

    g_value_unset(&gvalue);
    g_free(strVal);

    return cleanedStr;
}

static int json_get_int(JsonNode *root, const char* json_path)
{
    JsonPath *path = json_path_new();
    
    GError *error = NULL;
    json_path_compile(path, json_path, &error);
    
    JsonNode *path_match = json_path_match(path, root);
    JsonArray *array = json_node_get_array(path_match);
    JsonNode *arrayElement = json_array_get_element(array, 0);
    
    return json_node_get_int(arrayElement); 
}

//returns code embedded in api return
int get_json_return_code(JsonNode *root)
{
    gchar *status = json_get_string(root, "$.stat");
    int code = 200;

    if(strcmp(status, "fail") == 0)
    {
        code = json_get_int(root, "$.code");
    }

    //if stat == 'ok', json just does not return a code?
    g_free(status);
    return code;
}

void toggle_row_expansion(GtkTreeView *tree_view, GtkTreePath *path, gboolean expand, gboolean expand_all)    
{
    if (expand) 
    {
        gtk_tree_view_expand_row(tree_view, path, expand_all);
    } 
    else    
    {
        gtk_tree_view_collapse_row(tree_view, path);
    }
}

void handle_json_error(int error_code, int row_index)
{
    if(error_code == 401)//unauthorized
    {
        //writes new cookie if current cookie is wrong or does not exist
        gboolean saved_cookie = api_save_auth_cookie(username, password);
    
        if (saved_cookie == FALSE)
        {
            alert_popup("Authorization failed", "Failed to re-authorize.");
        }
        alert_popup("Authorization failed", "Re-authorized.");
    }
    else if(error_code == 999)//unknown function
    {
        alert_popup("Error", "This method does not exist");
    }
    else if(error_code == 404)//unsupported request
    {
        alert_popup("Error", "Parameters not set for this method.");
        
        show_parameter_dialog(row_index);
    }
    else
    {
        alert_popup("Error", "Unknown return code.");
    }
}

void make_post_data_from_object(char **str, Method *method)
{
    size_t total_len = 1;
    
    *str = malloc(total_len);
    
    if (!*str)
    {
        g_print("failed to alloc to parameter string.");
        return;
    } 
    (*str)[0] = '\0';

    for (size_t i = 0; i < method->param_count; i++)
    {
        char *name = method->parameters[i].name;
        char *value = method->parameters[i].value;

        size_t needed = strlen(name) + strlen(value) + 2; // '=' and '&'

        if (i + 1 < method->param_count) {
            needed += 1; 
        }

        total_len += needed;
        *str = realloc(*str, total_len);
        
        if (!*str)
        {
            return;
        }

        strcat(*str, name);
        strcat(*str, "=");
        strcat(*str, value);

        if (i + 1 < method->param_count)
        {
            strcat(*str, "&");
        }
    }
}

void parse_json_into_memory(MethodContainer **method_container)
{
    *method_container = malloc(sizeof(MethodContainer));

    JsonParser *parser = json_parser_new();
    GError *err = NULL;
    
    if (!json_parser_load_from_file(parser, "config.json", &err)) 
    {
        //gprint("error loading json file: %s\n", err->message);
        g_error_free(err);
        g_object_unref(parser);
        return;
    }

    JsonNode *root = json_parser_get_root(parser);

    username = json_get_string(root, "$.username");
    password = json_get_string(root, "$.password");
    base_url = json_get_string(root, "$.url");

    JsonObject *object = json_node_get_object(root);
    JsonArray *methods_member = json_object_get_array_member(object, "methods");
    guint method_count = json_array_get_length(methods_member);

    (*method_container)->methods = malloc(method_count * sizeof(Method));
    (*method_container)->method_count = method_count;

    if ((*method_container)->methods == NULL) 
    {
        perror("failed to alloc memory to methods");
        return;
    }
    
    for (guint i = 0; i < method_count; i++)    
    {
        JsonObject *method_object = json_array_get_object_element(methods_member, i);
        
        const gchar *name = json_object_get_string_member(method_object, "name");
        (*method_container)->methods[i].name = g_strdup(name);

        JsonArray *parameters = json_object_get_array_member(method_object, "parameters");
        guint param_len = json_array_get_length(parameters);
        (*method_container)->methods[i].param_count = param_len;
        
        (*method_container)->methods[i].parameters = malloc(param_len * sizeof(Parameter));

        if ((*method_container)->methods[i].parameters == NULL) 
        {
            perror("failed to alloc memory for parameters");
            return;
        }

        for (guint j = 0; j < param_len; j++) 
        {
            JsonObject *paramObj = json_array_get_object_element(parameters, j);
            const char *param_name = json_object_get_string_member(paramObj, "name");
            const char *param_value = json_object_get_string_member(paramObj, "value");

            (*method_container)->methods[i].parameters[j].name = g_strdup(param_name);
            (*method_container)->methods[i].parameters[j].value = g_strdup(param_value);
        }
    }

    g_object_unref(parser);
} 

//finds method in json and writes its params to it.
gboolean write_params_json(Method *method)    
{
    JsonParser *parser = json_parser_new();
    GError *file_error = NULL;

    json_parser_load_from_file(parser, "config.json", &file_error);
    if(file_error)
    {
        alert_popup("Error while parsing json", file_error->message);

        g_error_free(file_error);
        return FALSE;
    }

    JsonNode *root = json_parser_get_root(parser);
    JsonObject *root_object = json_node_get_object(root);
    
    JsonArray *methods_array = json_object_get_array_member(root_object, "methods");
    size_t methods_length = json_array_get_length(methods_array);

    for (size_t i = 0; i < methods_length; i++) 
    {
        JsonObject *method_object = json_array_get_object_element(methods_array, i);
        const gchar *existing_method_name = json_object_get_string_member(method_object, "name");

        if (g_strcmp0(existing_method_name, method->name) == 0) 
        {
            JsonArray *parameters_array = json_object_get_array_member(method_object, "parameters");
            size_t params_length = json_array_get_length(parameters_array);
            
            //remove old params
            for (size_t j = params_length; j > 0; j--)    
            {
                json_array_remove_element(parameters_array, j - 1);
                params_length = json_array_get_length(parameters_array);
            }

            //insert new params
            for (size_t k = 0; k < method->param_count; k++)    
            {
                JsonObject *param_object = json_object_new();
                json_object_set_string_member(param_object, "name", method->parameters[k].name);
                json_object_set_string_member(param_object, "value", method->parameters[k].value);
                json_array_add_object_element(parameters_array, param_object);
            }

            break;
        }
    }

    //JsonNode *root_node = json_node_new_from_object(root_object);
    JsonGenerator *generator = json_generator_new();
    json_generator_set_pretty(generator, TRUE);

    json_generator_set_root(generator, root);
    json_generator_to_file(generator, "config.json", NULL);
    
    g_object_unref(generator);
    g_object_unref(parser);

    return TRUE;
}

void remove_method_from_json(int method_index)
{
    JsonParser *parser = json_parser_new();
    GError *err = NULL;
    
    if (!json_parser_load_from_file(parser, "config.json", &err)) 
    {
        //gprint("error loading json file: %s\n", err->message);
        g_error_free(err);
        g_object_unref(parser);
        return;
    }

    JsonNode *root = json_parser_get_root(parser);

    JsonObject *object = json_node_get_object(root);
    JsonArray *methods_member = json_object_get_array_member(object, "methods");

    json_array_remove_element(methods_member, method_index);

    JsonGenerator *generator = json_generator_new();
    json_generator_set_pretty(generator, TRUE);

    json_generator_set_root(generator, root);
    json_generator_to_file(generator, "config.json", NULL);
    
    g_object_unref(generator);
    g_object_unref(parser);
}

void add_method_to_memory(const char *method_name)
{
    method_container->methods = realloc(method_container->methods, (method_container->method_count + 1) * sizeof(Method));

    if (method_container->methods == NULL)
    {
        g_print("Failed to allocate memory for new method.");
        return;
    }
    
    Method *new_method;
    new_method->name = method_name;
    new_method->param_count = 0;
    new_method->parameters = NULL;

    method_container->methods[method_container->method_count] = *new_method;
    method_container->method_count++;
}

gboolean remove_method_from_memory(int method_index)
{
    Method *selected_method = &method_container->methods[method_index];

    for (size_t i = 0; i < selected_method->param_count; i++)
    {   
        free(selected_method->parameters[i].name);
        free(selected_method->parameters[i].value);
    }
    free(selected_method->parameters);
    selected_method->parameters = NULL;

    //move elements
    for (size_t i = method_index + 1; i < method_container->method_count; i++)
    {
        method_container->methods[i - 1] = method_container->methods[i];
    }

    method_container->methods = realloc(method_container->methods, (method_container->method_count - 1) * sizeof(Method));
    method_container->method_count -= 1;

    if (method_container->methods == NULL && method_container->method_count > 0)
    {
        return FALSE;
    }

    return TRUE;
}

int find_method_index(const char* method_name)
{
    for (size_t i = 0; i < method_container->method_count; i++)
    {
        Method *method = &method_container->methods[i];
    
        if (strcmp(method->name, method_name) == 0)
        {
            return i;
        }
    }

    //this should never happen:
    return 0;   
}

void write_method_to_json(const char *method_name)
{
    JsonParser *parser = json_parser_new();
    GError *err = NULL;
    
    if (!json_parser_load_from_file(parser, "config.json", &err)) 
    {
        //gprint("error loading json file: %s\n", err->message);
        g_error_free(err);
        g_object_unref(parser);
        return;
    }

    JsonNode *root = json_parser_get_root(parser);

    JsonObject *object = json_node_get_object(root);
    JsonArray *methods_member = json_object_get_array_member(object, "methods");

    JsonObject *method_object = json_object_new();
    
    JsonNode *method_name_node = json_node_new(JSON_NODE_VALUE);
    json_node_set_string(method_name_node, method_name);
    json_object_set_member(method_object, "name", method_name_node);

    JsonArray *parameters_array = json_array_new();
    JsonNode *parameters_node = json_node_new(JSON_NODE_ARRAY);
    json_node_set_array(parameters_node, parameters_array);
    json_object_set_member(method_object, "parameters", parameters_node);

    JsonNode *new_method_node = json_node_new(JSON_NODE_OBJECT);
    json_node_set_object(new_method_node, method_object);
    json_array_add_element(methods_member, new_method_node);


    JsonGenerator *generator = json_generator_new();
    json_generator_set_pretty(generator, TRUE);

    json_generator_set_root(generator, root);
    json_generator_to_file(generator, "config.json", NULL);
    
    g_object_unref(generator);
    g_object_unref(parser);
}

gboolean write_user_details_to_json()
{
    JsonParser *parser = json_parser_new();
    GError *file_error = NULL;

    json_parser_load_from_file(parser, "config.json", &file_error);
    if(file_error)
    {
        alert_popup("Error while parsing json", file_error->message);

        g_error_free(file_error);
        return FALSE;
    }

    JsonNode *root = json_parser_get_root(parser);
    JsonObject *root_object = json_node_get_object(root);
    
    JsonNode *username_node = json_object_get_member(root_object, "username");
    json_node_set_string(username_node, username);

    JsonNode *password_node = json_object_get_member(root_object, "password");
    json_node_set_string(password_node, password);

    JsonNode *url_node = json_object_get_member(root_object, "url");
    json_node_set_string(url_node, base_url);


    //JsonNode *root_node = json_node_new_from_object(root_object);
    JsonGenerator *generator = json_generator_new();
    json_generator_set_pretty(generator, TRUE);

    json_generator_set_root(generator, root);
    json_generator_to_file(generator, "config.json", NULL);
    
    g_object_unref(generator);
    g_object_unref(parser);
    return TRUE;
}