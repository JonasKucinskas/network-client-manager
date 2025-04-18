
#include <curl/curl.h>
#include <string.h>
#include "headers/utils.h"
#include "headers/widgets.h"

gboolean api_save_auth_cookie(const char *username, const char *password)
{
    CURL *curl;
    CURLcode res;
        
    curl_global_init(CURL_GLOBAL_DEFAULT);
 
    curl = curl_easy_init();

    if(curl == NULL) 
    {
        g_print("failed to init curl");
        return FALSE;
    }

    char *url = malloc(strlen(base_url) + strlen("login") + 1);

    if (url == NULL) 
    {
        g_print("failed to alloc url");
        return FALSE;
    }
    url[0] = '\0';
    strcat(url, base_url);
    strcat(url, "login");

    size_t post_data_length = strlen("username=") + strlen(username) + strlen("&password=") + strlen(password) + 1;
    char *post_data = malloc(post_data_length);

    if (post_data == NULL) 
    {
        g_print("failed to alloc post data");
        return FALSE;
    }

    post_data[0] = '\0';
    snprintf(post_data, post_data_length, "username=%s&password=%s", username, password);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(post_data));
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    
    //write cookie to "cookies" file
    curl_easy_setopt(curl, CURLOPT_COOKIEJAR, "cookies");
    res = curl_easy_perform(curl);

    if(res != CURLE_OK)
    {
        g_print("curl_easy_perform() failed: %s\n",
        curl_easy_strerror(res));
        curl_easy_cleanup(curl);
        curl_global_cleanup();
        return FALSE;
    }
    
    free(url);
    free(post_data);
    curl_easy_cleanup(curl);
    curl_global_cleanup();

    return TRUE;
}

void api_call(struct MemoryStruct *chunk, Method *method)
{
    CURL *curl;
    CURLcode res;
 
    curl_global_init(CURL_GLOBAL_DEFAULT);
 
    curl = curl_easy_init();
    
    if(curl) 
    {
        char *url = malloc(strlen(base_url) + strlen(method->name) + 1);

        if (url == NULL)
        {
            g_print("failed to malloc to string url");
            return;
        }
        url[0] = '\0';

        strcat(url, base_url);
        strcat(url, method->name);

        curl_easy_setopt(curl, CURLOPT_URL, url);

        free(url);

        //insecure, but its okay for now
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

        if (method->param_count > 0)
        {
            char *post_data = NULL;
            make_post_data_from_object(&post_data, method);

            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(post_data));
            curl_easy_setopt(curl, CURLOPT_POST, 1L);

            free(post_data); 
        }

        /* cache the CA cert bundle in memory for a week */
        curl_easy_setopt(curl, CURLOPT_CA_CACHE_TIMEOUT, 604800L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)chunk);

        //read cookie from "bin/cookies" file
        curl_easy_setopt(curl, CURLOPT_COOKIEFILE, "cookies");
        res = curl_easy_perform(curl);

        if(res != CURLE_OK)
        {
            alert_popup("API request failed", curl_easy_strerror(res));
            curl_easy_cleanup(curl);
        }

        curl_global_cleanup();
    }
}