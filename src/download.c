#define CURL_STATICLIB

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <stdbool.h>
#include "../include/download.h"
#include "../include/logger.h"
#include "../include/utils/file_utils.h"

size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    return fwrite(ptr, size, nmemb, stream);
}

int download_file(const char* url, const char* path) {
    CURL *curl;
    FILE *fp;
    CURLcode res;
    bool success = false;

    ensure_parent_dirs(path);

    curl = curl_easy_init();
    if (curl) {
        fp = fopen(path, "wb");
        if (fp) {
            curl_easy_setopt(curl, CURLOPT_URL, url);

            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);

            curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

            res = curl_easy_perform(curl);

            if (res == CURLE_OK) {
                log_msg("succes", "Downloaded %s in %s\n", url, path);
                success = true;
            } else {
                log_msg("erreur", "Curl failed: %s\n", curl_easy_strerror(res));
            }

            fclose(fp);
        }
        curl_easy_cleanup(curl);
    }
    return success;
}

typedef struct {
    const download_job_t* job;
    FILE* fp;
    CURL* easy;
    int ok;
} dl_slot_t;

static size_t write_cb(void* ptr, size_t sz, size_t nm, void* ud) {
    return fwrite(ptr, sz, nm, (FILE*)ud);
}

static int start_job(CURLM* multi, dl_slot_t* slot, const download_job_t* job) {
    ensure_parent_dirs(job->path);
    FILE* fp = fopen(job->path, "wb");
    if (!fp) { log_msg("erreur", "fopen failed: %s\n", job->path); return 0; }

    CURL* e = curl_easy_init();
    curl_easy_setopt(e, CURLOPT_URL, job->url);
    curl_easy_setopt(e, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(e, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(e, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(e, CURLOPT_PRIVATE, slot);
    curl_easy_setopt(e, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(e, CURLOPT_CONNECTTIMEOUT, 15L);

    slot->job = job;
    slot->fp = fp;
    slot->easy = e;
    slot->ok = 0;

    curl_multi_add_handle(multi, e);
    return 1;
}

int download_files_parallel(const download_job_t* jobs, int count, int max_parallel) {
    if (count <= 0) return 0;
    if (max_parallel <= 0) max_parallel = 8;
    if (max_parallel > count) max_parallel = count;

    CURLM* multi = curl_multi_init();
    curl_multi_setopt(multi, CURLMOPT_MAX_TOTAL_CONNECTIONS, (long)max_parallel);

    int next = 0, running = 0, done = 0, success = 0;

    while (next < count && running < max_parallel) {
        dl_slot_t* slot = calloc(1, sizeof(*slot));
        if (start_job(multi, slot, &jobs[next])) running++;
        else free(slot);
        next++;
    }

    int still_running = 0;
    curl_multi_perform(multi, &still_running);

    while (running > 0) {
        int numfds;
        curl_multi_poll(multi, NULL, 0, 1000, &numfds);
        curl_multi_perform(multi, &still_running);

        CURLMsg* msg;
        int msgs_left;
        while ((msg = curl_multi_info_read(multi, &msgs_left))) {
            if (msg->msg != CURLMSG_DONE) continue;

            CURL* e = msg->easy_handle;
            dl_slot_t* slot = NULL;
            curl_easy_getinfo(e, CURLINFO_PRIVATE, &slot);

            long code = 0;
            curl_easy_getinfo(e, CURLINFO_RESPONSE_CODE, &code);

            fclose(slot->fp);

            if (msg->data.result == CURLE_OK && (code == 200 || code == 0)) {
                slot->ok = 1;
                success++;
            } else {
                log_msg("erreur", "Failed %s (curl=%d http=%ld)\n",
                        slot->job->url, msg->data.result, code);
                remove(slot->job->path);
            }

            curl_multi_remove_handle(multi, e);
            curl_easy_cleanup(e);
            free(slot);
            running--;
            done++;

            if (next < count) {
                dl_slot_t* ns = calloc(1, sizeof(*ns));
                if (start_job(multi, ns, &jobs[next])) running++;
                else free(ns);
                next++;
            }

            if (done % 10 == 0 || done == count)
                log_msg("info", "Download progress: %d/%d\n", done, count);
        }
    }

    curl_multi_cleanup(multi);
    return success;
}
