#include <string.h>  
#include <time.h>  
#include <sys/time.h>  
#include <stdarg.h>  
#include "log.h"  
#include <pthread.h>  
using namespace std;  
  
Log::Log()  
{  
    m_count = 0;  
    m_mutex = new pthread_mutex_t;  
    m_is_async = false;  
    pthread_mutex_init(m_mutex, NULL);  
}  
  
Log::~Log()  
{  
    if(m_fp != NULL)  
    {  
        fclose(m_fp);  
    }  
    pthread_mutex_destroy(m_mutex);  
  
    if(m_mutex != NULL)  
    {  
        delete m_mutex;  
    }  
}  
  
bool Log::init(const char* file_name, int log_buf_size, int split_lines, int max_queue_size)  
{  
    if(max_queue_size >= 1)  
    {  
        m_is_async = true;  
        m_log_queue = new block_queue<string>(max_queue_size);  
        pthread_t tid;  
        pthread_create(&tid, NULL, flush_log_thread, NULL);  
    }  
  
    m_log_buf_size = log_buf_size;  
    m_buf = new char[m_log_buf_size];  
    memset(m_buf, '\0', sizeof(m_buf));  
    m_split_lines = split_lines;  
  
    time_t t = time(NULL);  
    struct tm* sys_tm = localtime(&t);  
    struct tm my_tm = *sys_tm;  
    const char *p = strrchr(file_name, '/');  
    char log_full_name[256] = {0};  
    if(p == NULL)  
    {  
        snprintf(log_full_name, 255, "%d_%02d_%02d_%s",my_tm.tm_year+1900, my_tm.tm_mon+1, my_tm.tm_mday, file_name);     
    }  
    else  
    {  
        strcpy(log_name, p+1);  
        strncpy(dir_name, file_name, p - file_name + 1);  
        snprintf(log_full_name, 255, "%s%d_%02d_%02d_%s",dir_name, my_tm.tm_year+1900, my_tm.tm_mon+1, my_tm.tm_mday, log_name );   
    }  
  
    m_today = my_tm.tm_mday;  
  
    m_fp = fopen(log_full_name, "a");  
    if(m_fp == NULL)  
    {  
        return false;  
    }  
  
    return true;  
}  
  
void Log::write_log(int level, const char* format, ...)  
{  
    struct timeval now = {0,0};  
    gettimeofday(&now, NULL);  
    time_t t = now.tv_sec;  
    struct tm* sys_tm = localtime(&t);  
    struct tm my_tm = *sys_tm;  
    char s[16] = {0};  
    switch(level)  
    {  
        case 0 : strcpy(s, "[debug]:"); break;  
        case 1 : strcpy(s, "[info]:"); break;  
        case 2 : strcpy(s, "[warn]:"); break;  
        case 3 : strcpy(s, "[erro]:"); break;  
        default:  
               strcpy(s, "[info]:"); break;  
    }  
  
    pthread_mutex_lock(m_mutex);  
    m_count++;  
    if(m_today != my_tm.tm_mday || m_count % m_split_lines == 0) //everyday log  
    {  
        char new_log[256] = {0};  
        fflush(m_fp);  
        fclose(m_fp);  
        char tail[16] = {0};  
        snprintf(tail, 16,  "%d_%02d_%02d_", my_tm.tm_year+1900, my_tm.tm_mon+1, my_tm.tm_mday);  
        if(m_today != my_tm.tm_mday)  
        {  
            snprintf(new_log, 255, "%s%s%s", dir_name, tail, log_name);  
            m_today = my_tm.tm_mday;  
            m_count = 0;  
        }  
        else  
        {  
            snprintf(new_log, 255, "%s%s%s.%d", dir_name, tail, log_name, m_count/m_split_lines);  
        }  
        m_fp = fopen(new_log, "a");  
    }  
    pthread_mutex_unlock(m_mutex);  
      
    va_list valst;  
    va_start(valst, format);  
      
    string log_str;  
    pthread_mutex_lock(m_mutex);  
    int n = snprintf(m_buf, 48, "%d-%02d-%02d %02d:%02d:%02d.%06d %s ",  
            my_tm.tm_year+1900, my_tm.tm_mon+1, my_tm.tm_mday,  
            my_tm.tm_hour, my_tm.tm_min, my_tm.tm_sec, now.tv_usec, s);  
    int m = vsnprintf(m_buf + n, m_log_buf_size-1, format, valst);  
    m_buf[n + m - 1] = '\n';  
    log_str = m_buf;  
    pthread_mutex_unlock(m_mutex);  
  
    if(m_is_async && !m_log_queue->full())  
    {  
        m_log_queue->push(log_str);  
    }  
    else  
    {  
        pthread_mutex_lock(m_mutex);  
        fputs(log_str.c_str(), m_fp);  
        pthread_mutex_unlock(m_mutex);  
    }  
  
    va_end(valst);  
}  
  
void Log::flush(void)  
{  
    pthread_mutex_lock(m_mutex);  
    fflush(m_fp);  
    pthread_mutex_unlock(m_mutex);  
}  