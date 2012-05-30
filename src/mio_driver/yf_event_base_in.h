#ifndef  _YF_EVENT_BASE_IN_H
#define _YF_EVENT_BASE_IN_H

typedef  struct yf_fd_poll_s  yf_fd_poll_t;
typedef  struct  yf_fd_poll_actions_s  yf_fd_poll_actions_t;

typedef  struct  yf_fd_evt_link_s  yf_fd_evt_link_t;
typedef  struct  yf_fd_evt_in_s  yf_fd_evt_in_t;
typedef  struct  yf_fd_evt_driver_in_s  yf_fd_evt_driver_in_t;

typedef  struct  yf_timer_s  yf_timer_t;
typedef  struct  yf_tm_evt_link_s  yf_tm_evt_link_t;

#include <ppc/yf_header.h>
#include <base_struct/yf_core.h>
#include <mio_driver/yf_event.h>

#include "yf_tm_event_in.h"
#include "yf_fd_event_in.h"
#include "yf_poll_in.h"

typedef  struct
{
        yf_int_t  exit;

        yf_evt_driver_init_t  driver_ctx;
        
        yf_tm_evt_driver_in_t  tm_driver;
        yf_fd_evt_driver_in_t   fd_driver;
}
yf_evt_driver_in_t;


#endif

