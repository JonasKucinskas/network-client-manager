
#include <curl/curl.h>
#include <string.h>
#include "headers/utils.h"
#include "headers/widgets.h"

#define BASE_URL "https://192.168.50.1/api/"

gboolean api_save_auth_cookie()
{
  CURL *curl;
  CURLcode res;
 
  const char *post_data = "username=admin&password=Admin12345";

  curl_global_init(CURL_GLOBAL_DEFAULT);
 
  curl = curl_easy_init();

  if(curl) 
  {
    char url[34] = BASE_URL;
    strcat(url, "login");//api/login call

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

    curl_easy_cleanup(curl);
  }
 
  curl_global_cleanup();
  return TRUE;
}

void api_call(struct MemoryStruct *chunk, char *method, const char *post_data)
{
  CURL *curl;
  CURLcode res;
 
  curl_global_init(CURL_GLOBAL_DEFAULT);
 
  curl = curl_easy_init();
  
  if(curl) 
  {
    char url[34] = BASE_URL;
    strcat(url, method);

    curl_easy_setopt(curl, CURLOPT_URL, url);

    //insecure, but its okay for now
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    if (post_data)
    {
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);
      curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(post_data));
      curl_easy_setopt(curl, CURLOPT_POST, 1L);
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