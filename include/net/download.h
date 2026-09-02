#ifndef DOWNLOAD_H
#define DOWNLOAD_H

typedef struct {
    const char* url;
    const char* path;
} download_job_t;

int download_files_parallel(const download_job_t* jobs, int count, int max_parallel);

int download_file(const char* url, const char* path);

#endif