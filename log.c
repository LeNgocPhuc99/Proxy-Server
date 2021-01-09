#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>

#include "log.h"

#define SEPARATOR '/'
#define MAX_PATH_LENGTH 512

static char *logpath = NULL;
static char *filename = NULL;
static char *file_extn = NULL;
static FILE *fp = NULL;

static char *get_log_filename()
{
    return filename;
}

static void set_log_filename(const char *name)
{
    if (name && *name)
    {
        filename = strdup(name);
    }
}

static void set_path(const char *path)
{
    int len;
    if (path && *path != '\0')
    {
        logpath = strdup(path);
        len = strlen(logpath);
        if (len > 0 && logpath[len - 1] != SEPARATOR)
            logpath[len] = SEPARATOR;
    }
}

static char *get_path()
{
    if (!logpath)
    {
        sprintf(logpath, ".%c", SEPARATOR);
    }
    return logpath;
}

// Tag name la ID cua log file dua theo thoi gian duoc log
static char *get_tag_name()
{
    char tag[20] = {0};
    time_t now;
    time(&now);
    strftime(tag, 20, "%y%m%d", localtime(&now));
    return strdup(tag);
}

static char *get_log_filename_extension()
{
    return file_extn ? file_extn : "";
}

static void set_log_filename_extension(const char *name)
{
    if (name && *name != '\0')
    {
        file_extn = strdup(name);
    }
}

static char *construct_full_path(char *path)
{
    char *tag;
    tag = get_tag_name();
    sprintf(path, "%s%s_%s.%s", get_path(), get_log_filename(), tag, get_log_filename_extension());
    free(tag);
    return path;
}

void log_init(const char *path, const char *filename, const char *file_extension)
{
    char fullpath[MAX_PATH_LENGTH];
    if (path && *path != '\0' && filename && *filename != '\0')
    {
        set_path(path);
        set_log_filename(filename);
        set_log_filename_extension(file_extension);
        fp = fopen(construct_full_path(fullpath), "a");
        assert(fp != NULL);
        /* Khong mo file duoc thi in ra stdout */
        if (fp == NULL)
        {
            fp = stdout;
            fprintf(fp, "Failed to change logging target\n");
        }
    }
    else
    {
        if (fp != NULL && fp != stdout)
            fclose(fp);
        fp = stdout;
    }
}

static char *get_timestamp()
{
    // current system date
    char date_buf[50];
    time_t now = time(0);
    struct tm tm = *gmtime(&now);
    strftime(date_buf, sizeof(date_buf), "%a, %d %b %Y %H:%M:%S %Z/", &tm);
    return strdup(date_buf);
}

void log_print(const char *format, ...)
{
    char *date_buffer;
    va_list args;
    va_start(args, format);
    date_buffer = get_timestamp();
    fprintf(fp, "%s ", date_buffer);
    free(date_buffer);
    vfprintf(fp, format, args);
    va_end(args);
    log_flush();
}

void log_flush()
{
    fflush(fp);
}

void log_close()
{
    if (fp != stdout)
        fclose(fp);
    fp = NULL;

    if(logpath)
        free(logpath);
    
    if(filename)
        free(filename);

    if(file_extn)
        free(file_extn);
}