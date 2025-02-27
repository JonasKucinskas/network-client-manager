#include <gtk/gtk.h>
#include <curl/curl.h>

#define BASE_URL "https://192.168.50.1/api/"

//init in main
GtkWidget *wan_view;

struct MemoryStruct 
{
  char *memory;
  size_t size;
};

enum
{
  COL_METHOD,
  COL_VALUE,
  NUM_COLS
} ;



static GtkTreeModel* create_and_fill_model (void)
{
  GtkTreeStore *treestore;
  GtkTreeIter toplevel, child;

  treestore = gtk_tree_store_new(NUM_COLS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_UINT);

  char **methods = (char *[]){ "status.wan.connection"}; 
  
  for(int i = 0; i < 1; i++)
  {
    gtk_tree_store_append(treestore, &toplevel, NULL);
    gtk_tree_store_set(treestore, &toplevel,
                     COL_METHOD, methods[i],
                     COL_VALUE, "",
                     -1);

    //loop for nested structure, append as child
    /*
    gtk_tree_store_append(treestore, &child, &toplevel);
    gtk_tree_store_set(treestore, &child,
                     COL_FIRST_NAME, "Janinita",
                     COL_LAST_NAME, "Average",
                     COL_YEAR_BORN, (guint) 1985,
                     -1);
    */
  }
  return GTK_TREE_MODEL(treestore);
}

void
age_cell_data_func (GtkTreeViewColumn *col,
                    GtkCellRenderer   *renderer,
                    GtkTreeModel      *model,
                    GtkTreeIter       *iter,
                    gpointer           user_data)
{
  guint  year_born = 123;
  guint  year_now = 2003; /* to save code not relevant for the example */
  gchar  buf[64];

  //gtk_tree_model_get(model, iter, COL_YEAR_BORN, &year_born, -1);

  if (year_born <= year_now && year_born > 0)
  {
    guint age = year_now - year_born;

    g_snprintf(buf, sizeof(buf), "%u years old", age);

    g_object_set(renderer, "foreground-set", FALSE, NULL); /* print this normal */
  }
  else
  {
    g_snprintf(buf, sizeof(buf), "age unknown");

    /* make red */
    g_object_set(renderer, "foreground", "Red", "foreground-set", TRUE, NULL);
  }

  g_object_set(renderer, "text", buf, NULL);
}

static void create_model()
{
  GtkTreeViewColumn *col;
  GtkCellRenderer *renderer;
  GtkTreeModel *model;

  col = gtk_tree_view_column_new();

  gtk_tree_view_column_set_title(col, "Method");
  gtk_tree_view_append_column(GTK_TREE_VIEW(wan_view), col);

  renderer = gtk_cell_renderer_text_new();
  gtk_tree_view_column_pack_start(col, renderer, TRUE);


  /* connect 'text' property of the cell renderer to
  *  model column that contains the first name */
  gtk_tree_view_column_add_attribute(col, renderer, "text", COL_METHOD);


  /* --- Column #2 --- */

  col = gtk_tree_view_column_new();
  gtk_tree_view_column_set_title(col, "Value");

  /* pack tree wan_view column into tree wan_view */
  gtk_tree_view_append_column(GTK_TREE_VIEW(wan_view), col);

  renderer = gtk_cell_renderer_text_new();
  gtk_tree_view_column_pack_start(col, renderer, TRUE);

  /* connect 'text' property of the cell renderer to
   *  model column that contains the last name */
  gtk_tree_view_column_add_attribute(col, renderer, "text", COL_VALUE);

  /* set 'weight' property of the cell renderer to
   *  bold print (we want all last names in bold) */
  g_object_set(renderer,
               "weight", PANGO_WEIGHT_BOLD,
               "weight-set", TRUE,
               NULL);


  /* --- Column #3 --- */

  col = gtk_tree_view_column_new();

  gtk_tree_view_column_set_title(col, "Age");
  gtk_tree_view_append_column(GTK_TREE_VIEW(wan_view), col);

  renderer = gtk_cell_renderer_text_new();
  gtk_tree_view_column_pack_start(col, renderer, TRUE);

  /* connect a cell data function */
  //gtk_tree_view_column_set_cell_data_func(col, renderer, age_cell_data_func, NULL, NULL);

  model = create_and_fill_model();

  gtk_tree_view_set_model(GTK_TREE_VIEW(wan_view), model);

  g_object_unref(model); /* destroy model automatically with wan_view */

  gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(wan_view)), GTK_SELECTION_SINGLE);
}



static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;

  mem->memory = realloc(mem->memory, mem->size + realsize + 1);
  
  if(mem->memory == NULL) 
  {
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
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
      curl_easy_strerror(res));
    }

    curl_easy_cleanup(curl);
  }
 
  curl_global_cleanup();
}

//TODO: CHECK IF COOKIE EXISTS
static void api_call(struct MemoryStruct *chunk, char *method, char *post_data)
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
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)chunk);

    //read cookie from "cookies" file
    curl_easy_setopt(curl, CURLOPT_COOKIEFILE, "cookies");
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

void on_row_activated(GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data) 
{
  GtkTreeModel *model = gtk_tree_view_get_model(tree_view);
  GtkTreeIter iter;
  
  if (gtk_tree_model_get_iter(model, &iter, path)) 
  {
    gchar *method_name;
    
    struct MemoryStruct chunk;

    chunk.memory = malloc(1);  
    chunk.size = 0;


    gtk_tree_model_get(model, &iter, COL_METHOD, &method_name, -1);
    
    g_print("Row activated: %s %s\n", method_name);
    
    char *data = "username=admin&password=Admin12345";
    api_login("login", data);
    api_call(&chunk, method_name, NULL);


    gtk_tree_store_append(GTK_TREE_STORE(model), &iter, &iter);
    gtk_tree_store_set(GTK_TREE_STORE(model), &iter,
                     COL_METHOD, "",
                     COL_VALUE, chunk.memory,
                     -1);



    //printf("%s", chunk->memory);
    g_free(method_name);
  }
}

void draw_tree_view()
{
  //set wan_view and model;
  g_signal_connect(wan_view, "row-activated", G_CALLBACK(on_row_activated), NULL);
  create_model();
  gtk_widget_show_all(wan_view);
}