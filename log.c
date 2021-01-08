#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h> 
#include <stdlib.h>
#include <time.h>

#include "log.h"

#define MAX_PATH_LENGTH 512
#define MAX_FILENAME 256
#define MAX_FILE_EXTN 20
#define LOG_FILENAME "load_balancer"
#define SEPARATOR '/'

static log_level loglevel = LOG_ERRORS;
static char logpath[MAX_PATH_LENGTH] = { 0 };
static char filename[MAX_FILENAME] = LOG_FILENAME;
static char file_extn[MAX_FILE_EXTN] = "log";
static FILE* fp = NULL;

static const char* get_log_filename() {
    return filename;
}

static void set_log_filename(const char* name) {
    if (name && *name)
        strncpy(filename, name, MAX_FILENAME);
}

static void set_path(const char* path) {
    int len;
    if (path && *path != '\0') {
        strncpy(logpath, path, MAX_PATH_LENGTH);
        len = strlen(logpath);
        if (len > 0 && logpath[len - 1] != SEPARATOR)
            logpath[len] = SEPARATOR;
    }
}

static const char* get_path() {
    if (!logpath) {
        sprintf(logpath, ".%c", SEPARATOR);
    }
    return logpath;
}

static char* get_tag_name(char* buf) {
    time_t now;
    time(&now);
    strftime(buf, 20, "%y%m%d", localtime(&now));
    return buf;
}

static const char* get_log_filename_extension() {
    return file_extn ? file_extn : "";
}

static void set_log_filename_extension(const char* name) {
    if (name && *name != '\0')
        strncpy(file_extn, name, MAX_FILE_EXTN);
}

static char* construct_full_path(char* path) {
    char tag[20] = { 0 };
    sprintf(path, "%s%s_%s.%s", get_path(), get_log_filename(), get_tag_name(tag), get_log_filename_extension());
    return path;
}

void log_init(const char* path, const char* filename, const char* file_extension) {
    char fullpath[MAX_PATH_LENGTH];
    if (path && *path != '\0' && filename && *filename != '\0') {
        set_path(path);
        set_log_filename(filename);
        set_log_filename_extension(file_extension);
        fp = fopen(construct_full_path(fullpath), "a");
        assert(fp != NULL);
        /* Khong mo file duoc thi in ra stdout */
        if (fp == NULL) {
            fp = stdout;
            fprintf(fp, "Failed to change logging target\n");
        }
    }
    else {
        if (fp != NULL && fp != stdout)
            fclose(fp);
        fp = stdout;
    }
}

static char* get_timestamp() {
    // current system date
    char date_buf[50];
    time_t now = time(0);
    struct tm tm = *gmtime(&now);
    strftime(date_buf, sizeof(date_buf), "%a, %d %b %Y %H:%M:%S %Z", &tm);
    return strdup(date_buf);
}

void log_set_level(log_level level) {
    loglevel = level;
}

log_level log_get_level() {
    return loglevel;
}

void log_debug(const char* format, ...) {
    char *date_buffer;
    if(loglevel > 0) {
        va_list args;
        va_start (args, format);
        date_buffer = get_timestamp();
        fprintf(fp, "%s ", date_buffer);
        free(date_buffer);
        vfprintf (fp, format, args);
        va_end (args);
   }
}

void log_error(const char* format, ...) {
    char *date_buffer;
    va_list args;
    va_start (args, format);
    date_buffer = get_timestamp();
    fprintf(fp, "%s ", date_buffer);
    vfprintf (fp, format, args);
    va_end (args);
    fflush(fp);
}

void log_flush() {
    fflush(fp);
}

void log_close() {
    if(fp != stdout)
        fclose(fp);
    fp = NULL;
}