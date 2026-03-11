#ifndef HEXCHAT_UPLOAD_H
#define HEXCHAT_UPLOAD_H

#include "hexchat.h"

void upload_file(session *sess, const char *target, const char *filepath);
void upload_file_with_headers (session *sess, const char *target, const char *filepath, const char *url, const char *headers);

#endif
