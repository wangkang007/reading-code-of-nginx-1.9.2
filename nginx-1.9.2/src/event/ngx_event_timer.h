
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_EVENT_TIMER_H_INCLUDED_
#define _NGX_EVENT_TIMER_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_event.h>


#define NGX_TIMER_INFINITE  (ngx_msec_t) -1

#define NGX_TIMER_LAZY_DELAY  300


ngx_int_t ngx_event_timer_init(ngx_log_t *log);
ngx_msec_t ngx_event_find_timer(void);
void ngx_event_expire_timers(void);
void ngx_event_cancel_timers(void);


extern ngx_rbtree_t  ngx_event_timer_rbtree;


static ngx_inline void
ngx_event_del_timer(ngx_event_t *ev, const char *func, unsigned int line)
{
    char tmpbuf[128];

    snprintf(tmpbuf, sizeof(tmpbuf), "<%25s, %5d> ", func, line);
    ngx_log_debug3(NGX_LOG_DEBUG_EVENT, ev->log, 0,
                   "%s event timer del: %d: %M", tmpbuf,
                    ngx_event_ident(ev->data), ev->timer.key);
                    
    ngx_rbtree_delete(&ngx_event_timer_rbtree, &ev->timer);

#if (NGX_DEBUG)
    ev->timer.left = NULL;
    ev->timer.right = NULL;
    ev->timer.parent = NULL;
#endif

    ev->timer_set = 0;
}
//ngx_event_expire_timers中执行ev->handler
//在ngx_process_events_and_timers中，当有事件使epoll_wait返回，则会执行超时的定时器
//注意定时器的超时处理，不一定就是timer时间超时，超时误差可能为timer_resolution，如果没有设置timer_resolution则定时器可能永远不超时，因为epoll_wait不返回，无法更新时间
static ngx_inline void
ngx_event_add_timer(ngx_event_t *ev, ngx_msec_t timer, const char *func, unsigned int line)
{
    ngx_msec_t      key;
    ngx_msec_int_t  diff;
    char tmpbuf[128];

    key = ngx_current_msec + timer;
    
    if (ev->timer_set) { //如果之前该ev已经添加过，则先把之前的ev定时器del掉，然后在重新添加

        /*
         * Use a previous timer value if difference between it and a new
         * value is less than NGX_TIMER_LAZY_DELAY milliseconds: this allows
         * to minimize the rbtree operations for fast connections.
         */

        diff = (ngx_msec_int_t) (key - ev->timer.key);

        if (ngx_abs(diff) < NGX_TIMER_LAZY_DELAY) {
            snprintf(tmpbuf, sizeof(tmpbuf), "<%25s, %5d> ", func, line);
            ngx_log_debug4(NGX_LOG_DEBUG_EVENT, ev->log, 0,
                           "%s event timer: %d, old: %M, new: %M, ", tmpbuf,
                            ngx_event_ident(ev->data), ev->timer.key, key);
            return;
        }

        ngx_del_timer(ev, NGX_FUNC_LINE);
    }

    ev->timer.key = key;
    snprintf(tmpbuf, sizeof(tmpbuf), "<%25s, %5d> ", func, line);
    ngx_log_debug4(NGX_LOG_DEBUG_EVENT, ev->log, 0,
                   "%s event timer add: %d: %M:%M", tmpbuf,
                    ngx_event_ident(ev->data), timer, ev->timer.key);

    ngx_rbtree_insert(&ngx_event_timer_rbtree, &ev->timer);

    ev->timer_set = 1;
}


#endif /* _NGX_EVENT_TIMER_H_INCLUDED_ */
