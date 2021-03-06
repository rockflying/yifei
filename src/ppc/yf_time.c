#include <ppc/yf_header.h>
#include <base_struct/yf_core.h>

yf_times_t  yf_start_times;

//log buf last char is '\0'
#define yf_init_timedata(time_data) \
        if (time_data->log_time.data == NULL) { \
                time_data->log_time.data = time_data->log_buf; \
                time_data->log_time.len = YF_TIME_BUF_LEN; \
                time_data->log_buf[YF_TIME_BUF_LEN] = 0; \
        }

#if (YF_THREADS) && defined ___YF_THREAD_UNSUPPORT

YF_THREAD_DEF_KEY_ONCE(__key, __init_done, __thread_init);

yf_time_data_t* yf_time_data_addr()
{
        yf_time_data_t* time_data;
        
        yf_thread_get_specific(__key, __init_done, __thread_init, 
                        time_data, sizeof(yf_time_data_t));
        
        yf_init_timedata(time_data);
        return time_data;
}

#else
___YF_THREAD yf_time_data_t  yf_time_data_ins;
#endif

static void yf_update_log_time(yf_log_t* log);

#define yf_utime_to_time(times, utime) do { \
        (times).tv_sec = (utime).tv_sec; \
        (times).tv_msec = (utime).tv_usec / 1000; \
} while (0)


#if defined(HAVE_CLOCK_GETTIME) && defined(CLOCK_MONOTONIC)

#define  YF_WALL_GET_INTERVAL_TMS  120

yf_int_t  yf_init_time(yf_log_t* log)
{
        yf_time_data_t* time_data = yf_time_data;
        yf_init_timedata(time_data);
        
        if (gettimeofday(&yf_start_times.wall_utime, NULL) != 0)
        {
                yf_log_error(YF_LOG_WARN, log, yf_errno, "gettimeofday err");
                return YF_ERROR;
        }

        time_data->last_real_wall_utime = yf_start_times.wall_utime;

        //clock time
        struct timespec ts;
        
        if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
        {
                yf_log_error(YF_LOG_WARN, log, yf_errno, "gettime err");
                return YF_ERROR;
        }
        
        time_data->last_clock_utime.tv_sec = ts.tv_sec;
        time_data->last_clock_utime.tv_usec = ts.tv_nsec / 1000;

        yf_start_times.clock_utime = time_data->last_clock_utime;

        yf_utime_to_time(yf_start_times.clock_time, yf_start_times.clock_utime);

        yf_log_time.len = YF_TIME_BUF_LEN;
        yf_log_time.data = yf_alloc(yf_log_time.len);

        yf_update_log_time(log);

        return  YF_OK;
}


yf_int_t  yf_update_time(yf_time_reset_handler handle, void* data, yf_log_t* log)
{
        struct timespec ts;
        yf_time_data_t* time_data = yf_time_data;
        yf_utime_t*  wall_utime = &yf_now_times.wall_utime;
        yf_utime_t*  clock_utime = &yf_now_times.clock_utime;
        yf_time_t*  clock_time = &yf_now_times.clock_time;        
        
        if (unlikely(clock_gettime(CLOCK_MONOTONIC, &ts) != 0))
        {
                yf_log_error(YF_LOG_WARN, log, yf_errno, "gettime err");
                return YF_ERROR;
        }
        
        clock_utime->tv_sec = ts.tv_sec;
        clock_utime->tv_usec = ts.tv_nsec / 1000;

        yf_utime_to_time(*clock_time, *clock_utime);

        if (clock_utime->tv_sec < time_data->last_clock_utime.tv_sec
                + YF_WALL_GET_INTERVAL_TMS)
        {
                wall_utime->tv_usec = time_data->last_real_wall_utime.tv_usec 
                                + clock_utime->tv_usec 
                                - time_data->last_clock_utime.tv_usec;
                
                wall_utime->tv_sec = time_data->last_real_wall_utime.tv_sec
                                + clock_utime->tv_sec 
                                - time_data->last_clock_utime.tv_sec;
                
                if ((yf_s32_t)wall_utime->tv_usec < 0)
                {
                        wall_utime->tv_usec += 1000000;
                        --wall_utime->tv_sec;
                }
                else if (wall_utime->tv_usec >= 1000000) {
                        wall_utime->tv_usec -= 1000000;
                        ++wall_utime->tv_sec;
                }

                yf_update_log_time(log);
                return  YF_OK;
        }

        //TODO check why not called here...
        yf_log_debug0(YF_LOG_DEBUG, log, 0, "update real wall time now");

        time_data->last_clock_utime = *clock_utime;

        if (unlikely(gettimeofday(wall_utime, NULL) != 0))
        {
                yf_log_error(YF_LOG_WARN, log, yf_errno, "gettimeofday err");
                return YF_ERROR;
        }

        time_data->last_real_wall_utime = *wall_utime;
        yf_update_log_time(log);
        return  YF_OK;
}

#else

#ifdef __GNUC__
#warning "cant find clock_gettime function, use gettimeofday"
#else
#pragma message("cant find clock_gettime function, use gettimeofday")
#endif

yf_int_t  yf_init_time(yf_log_t* log)
{
        if (gettimeofday(&yf_start_times.wall_utime, NULL) != 0)
        {
                yf_log_error(YF_LOG_WARN, log, yf_errno, "gettimeofday err");
                return YF_ERROR;
        }

        yf_start_times.clock_utime = yf_start_times.wall_utime;
        yf_utime_to_time(yf_start_times.clock_time, yf_start_times.clock_utime);

        yf_update_log_time(log);
        return  YF_OK;
}

yf_int_t  yf_update_time(yf_time_reset_handler handle, void* data, yf_log_t* log)
{
        yf_utime_t  now_time;
        yf_utime_t*  wall_utime = &yf_now_times.wall_utime;
        yf_utime_t*  clock_utime = &yf_now_times.clock_utime;
        yf_time_t*  clock_time = &yf_now_times.clock_time;
        
        if (unlikely(gettimeofday(&now_time, NULL) != 0))
        {
                yf_log_error(YF_LOG_WARN, log, yf_errno, "gettimeofday err");
                return YF_ERROR;
        }
        
        if (yf_utime_cmp(&now_time, wall_utime, >=))
        {
                *wall_utime = *clock_utime = now_time;
                yf_utime_to_time(*clock_time, *clock_utime);

                yf_update_log_time(log);
                return  YF_OK;
        }

        //correct time
        yf_time_t  diff_time;
        diff_time.tv_sec = wall_utime->tv_sec - now_time.tv_sec;
        diff_time.tv_msec = (wall_utime->tv_usec - now_time.tv_usec) / 1000;

        if (handle)
                handle(&now_time, &diff_time, data);
        
        *wall_utime = *clock_utime = now_time;
        yf_utime_to_time(*clock_time, *clock_utime);

        yf_start_times.wall_utime = yf_start_times.clock_utime = now_time;
        yf_start_times.clock_time = *clock_time;

        yf_update_log_time(log);
        return  YF_OK;
}

#endif


void yf_localtime(time_t s, yf_stm_t *tm)
{
#if (HAVE_LOCALTIME_R)
        (void) localtime_r(&s, tm);
#else
        yf_stm_t  *t;

        t = localtime(&s);
        *tm = *t;
#endif

        tm->yf_tm_mon++;
        tm->yf_tm_year += 1900;        
}

yf_int_t yf_real_walltime(yf_time_t* time)
{
        yf_utime_t  now_time;
        
        if (unlikely(gettimeofday(&now_time, NULL) != 0))
        {
                return YF_ERROR;
        }
        yf_utime_to_time((*time), now_time);
        return  YF_OK;
}


static void yf_update_log_time(yf_log_t* log)
{
        yf_time_data_t* time_data = yf_time_data;
        
        yf_stm_t  tm;
        yf_localtime(time_data->now_times.wall_utime.tv_sec, &tm);
        
        yf_init_timedata(time_data);

        yf_snprintf(time_data->log_time.data, YF_TIME_BUF_LEN, 
                "%2d/%02d/%02d %02d:%02d:%02d %03d",
                tm.yf_tm_year - 2000, tm.yf_tm_mon,
                tm.yf_tm_mday, tm.yf_tm_hour,
                tm.yf_tm_min, tm.yf_tm_sec, 
                time_data->now_times.wall_utime.tv_usec >> 10);
}

