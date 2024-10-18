#ifndef TRANSFER_H
#define TRANSFER_H

void sendFileSize(const char *dataDir, const char *filename, int socketfd);
void sendFile(const char *dataDir, const char *filename, int socketfd);
void receiveFile(const char *dataDir, const char *filename, int filesize, int socket);
void sendFileList(const char *dataDir, int socketfd);

#endif