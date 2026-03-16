#ifndef FILE_H
#define FILE_H

int writeMetaFile(const char *metaPath, long dirMTime);
long readMetaFile(const char *metaPath);
int writeAppDataFile(const char *dataPath);
long getDirMTime(const char *path);

#endif
