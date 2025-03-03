#ifndef API_H
#define API_H

struct MemoryStruct;

void api_save_auth_cookie(const char *method, const char* post_data);
void api_call(struct MemoryStruct *chunk, const char *method, const char *post_data);

#endif