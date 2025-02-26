#include <gtk/gtk.h>
#include <curl/curl.h>

#define BASE_URL "https://192.168.50.1/api/"

//init in main
GtkWidget *wan_grid;

struct MemoryStruct {
  char *memory;
  size_t size;
};

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;

  mem->memory = realloc(mem->memory, mem->size + realsize + 1);
  if(mem->memory == NULL) {
    /* out of memory! */ 
    printf("not enough memory (realloc returned NULL)\n");
    return 0;
  }

  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}

static void api_login(char *method, char* post_data)
{
  CURL *curl;
  CURLcode res;
 
  curl_global_init(CURL_GLOBAL_DEFAULT);
 
  curl = curl_easy_init();

  if(curl) 
  {
    char url[34] = BASE_URL;
    strcat(url, method);

    g_print(url);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(post_data));
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    
    //enable cookie engine
    //write cookie to "cookies" file
    curl_easy_setopt(curl, CURLOPT_COOKIEJAR, "cookies");

    res = curl_easy_perform(curl);

    if(res != CURLE_OK)
    {
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
      curl_easy_strerror(res));
    }

    curl_easy_cleanup(curl);
  }
 
  curl_global_cleanup();
}


static void api_call(char *url, char *post_data)
{
  CURL *curl;
  CURLcode res;
 
  curl_global_init(CURL_GLOBAL_DEFAULT);
 
  curl = curl_easy_init();

  if(curl) 
  {
    struct MemoryStruct chunk;

    chunk.memory = malloc(1);  
    chunk.size = 0; 

    curl_easy_setopt(curl, CURLOPT_URL, url);

    //insecure
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
    
    /* we pass our 'chunk' struct to the callback function */ 
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

    //enable cookie engine
    //write cookie to "cookies" file
    curl_easy_setopt(curl, CURLOPT_COOKIEFILE, "cookies");
    res = curl_easy_perform(curl);

    if(res != CURLE_OK)
    {
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
      curl_easy_strerror(res));
    }
    
    printf("%s", chunk.memory);
    curl_easy_cleanup(curl);
  }
 
  curl_global_cleanup();
}

static void expander_callback (GObject *object, GParamSpec *param_spec, gpointer user_data)
{
  GtkExpander *expander;

  expander = GTK_EXPANDER (object);

  if (gtk_expander_get_expanded (expander))
  {
    char *data = "username=admin&password=Admin12345";
    api_login("login", data);
    //api_call("status.wan.connection", NULL);
  }
  else
  {
  }
}

void draw_expander()
{
  GtkWidget *expander = gtk_expander_new_with_mnemonic ("_More Options");
  gtk_grid_attach(GTK_GRID(wan_grid), expander, 0, 0, 1, 1);
    
  g_signal_connect (expander, "notify::expanded", G_CALLBACK (expander_callback), NULL);
  gtk_widget_show_all(wan_grid);
}

