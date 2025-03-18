#ifndef API_H
#define API_H

struct MemoryStruct;

void api_call(struct MemoryStruct *chunk, Method *method);
gboolean api_save_auth_cookie();

#endif