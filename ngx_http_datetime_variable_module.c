#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>


typedef struct {
    ngx_str_t name;
    ngx_http_get_variable_pt get_handler;
    ngx_uint_t flags;
} ngx_http_datetime_variable_t;


static ngx_int_t ngx_http_datetime_add_variable(ngx_conf_t *cf);
static ngx_int_t ngx_http_variable_year(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_month(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_day(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_hour(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_minute(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_second(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_local_year(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_local_month(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_local_day(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_local_hour(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_local_minute(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_local_second(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);


static ngx_http_datetime_variable_t  ngx_http_datetime_variables[] = {
    {
        ngx_string("datetime_year"),
        ngx_http_variable_year,
        NGX_HTTP_VAR_NOCACHEABLE,
    },
    {
        ngx_string("datetime_month"),
        ngx_http_variable_month,
        NGX_HTTP_VAR_NOCACHEABLE,
    },
    {
        ngx_string("datetime_day"),
        ngx_http_variable_day,
        NGX_HTTP_VAR_NOCACHEABLE,
    },
    {
        ngx_string("datetime_hour"),
        ngx_http_variable_hour,
        NGX_HTTP_VAR_NOCACHEABLE,
    },
    {
        ngx_string("datetime_minute"),
        ngx_http_variable_minute,
        NGX_HTTP_VAR_NOCACHEABLE,
    },
    {
        ngx_string("datetime_second"),
        ngx_http_variable_second,
        NGX_HTTP_VAR_NOCACHEABLE,
    },
        {
        ngx_string("datetime_lyear"),
        ngx_http_variable_local_year,
        NGX_HTTP_VAR_NOCACHEABLE,
    },
    {
        ngx_string("datetime_lmonth"),
        ngx_http_variable_local_month,
        NGX_HTTP_VAR_NOCACHEABLE,
    },
    {
        ngx_string("datetime_lday"),
        ngx_http_variable_local_day,
        NGX_HTTP_VAR_NOCACHEABLE,
    },
    {
        ngx_string("datetime_lhour"),
        ngx_http_variable_local_hour,
        NGX_HTTP_VAR_NOCACHEABLE,
    },
    {
        ngx_string("datetime_lminute"),
        ngx_http_variable_local_minute,
        NGX_HTTP_VAR_NOCACHEABLE,
    },
    {
        ngx_string("datetime_lsecond"),
        ngx_http_variable_local_second,
        NGX_HTTP_VAR_NOCACHEABLE,
    }
};


static ngx_http_module_t ngx_http_datetime_variable_ctx = {
    ngx_http_datetime_add_variable,        /* preconfiguration */
    NULL,                                  /* postconfiguration */

    NULL,                                  /* create main configuration */
    NULL,                                  /* init main configuration */

    NULL,                                  /* create server configuration */
    NULL,                                  /* merge server configuration */

    NULL,                                  /* create location configuration */
    NULL                                   /* merge location configuration */
};


ngx_module_t  ngx_http_datetime_variable_module = {
    NGX_MODULE_V1,
    &ngx_http_datetime_variable_ctx,       /* module context */
    NULL,                                  /* module directives */
    NGX_HTTP_MODULE,                       /* module type */
    NULL,                                  /* init master */
    NULL,                                  /* init module */
    NULL,                                  /* init process */
    NULL,                                  /* init thread */
    NULL,                                  /* exit thread */
    NULL,                                  /* exit process */
    NULL,                                  /* exit master */
    NGX_MODULE_V1_PADDING
};


static ngx_int_t
ngx_http_datetime_fmt(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, ngx_int_t t, ngx_int_t len)
{
    u_char *p;

    p = ngx_pnalloc(r->pool, len);
    if (p == NULL) {
        return NGX_ERROR;
    }

    v->len = ngx_sprintf(p, "%02i", t) - p;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->data = p;

    return NGX_OK;
}


static void
ngx_http_datetime_gmtime(time_t t, struct tm *tp)
{
    int   yday;
    unsigned int  n, sec, min, hour, mday, mon, year, wday, days, leap;

    /* the calculation is valid for positive time_t only */

    n = (unsigned int) t;

    days = n / 86400;

    /* January 1, 1970 was Thursday */

    wday = (4 + days) % 7;

    n %= 86400;
    hour = n / 3600;
    n %= 3600;
    min = n / 60;
    sec = n % 60;

    /*
     * the algorithm based on Gauss' formula,
     * see src/http/ngx_http_parse_time.c
     */

    /* days since March 1, 1 BC */
    days = days - (31 + 28) + 719527;

    /*
     * The "days" should be adjusted to 1 only, however, some March 1st's go
     * to previous year, so we adjust them to 2.  This causes also shift of the
     * last February days to next year, but we catch the case when "yday"
     * becomes negative.
     */

    year = (days + 2) * 400 / (365 * 400 + 100 - 4 + 1);

    yday = days - (365 * year + year / 4 - year / 100 + year / 400);

    if (yday < 0) {
        leap = (year % 4 == 0) && (year % 100 || (year % 400 == 0));
        yday = 365 + leap + yday;
        year--;
    }

    /*
     * The empirical formula that maps "yday" to month.
     * There are at least 10 variants, some of them are:
     *     mon = (yday + 31) * 15 / 459
     *     mon = (yday + 31) * 17 / 520
     *     mon = (yday + 31) * 20 / 612
     */

    mon = (yday + 31) * 10 / 306;

    /* the Gauss' formula that evaluates days before the month */

    mday = yday - (367 * mon / 12 - 30) + 1;

    if (yday >= 306) {

        year++;
        mon -= 10;

        /*
         * there is no "yday" in Win32 SYSTEMTIME
         *
         * yday -= 306;
         */

    } else {

        mon += 2;

        /*
         * there is no "yday" in Win32 SYSTEMTIME
         *
         * yday += 31 + 28 + leap;
         */
    }

    tp->tm_sec = (int) sec;
    tp->tm_min = (int) min;
    tp->tm_hour = (int) hour;
    tp->tm_mday = (int) mday;
    tp->tm_mon = (int) mon;
    tp->tm_year = (int) year;
    tp->tm_wday = (int) wday;
}


static void
ngx_http_datetime_localtime(time_t t, struct tm *tp)
{
    ngx_localtime(t, tp);
}


static ngx_int_t
ngx_http_variable_local_year(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    struct tm  tm;
    time_t     elapsed = ngx_cached_time->sec;

    ngx_http_datetime_localtime(elapsed, &tm);

    return ngx_http_datetime_fmt(r, v, tm.tm_year, NGX_INT32_LEN);
}


static ngx_int_t
ngx_http_variable_local_month(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    struct tm  tm;
    time_t     elapsed = ngx_cached_time->sec;

    ngx_http_datetime_localtime(elapsed, &tm);

    return ngx_http_datetime_fmt(r, v, tm.tm_mon, NGX_INT32_LEN);
}


static ngx_int_t
ngx_http_variable_local_day(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    struct tm  tm;
    time_t     elapsed = ngx_cached_time->sec;

    ngx_http_datetime_localtime(elapsed, &tm);

    return ngx_http_datetime_fmt(r, v, tm.tm_mday, NGX_INT32_LEN);
}


static ngx_int_t
ngx_http_variable_local_hour(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    struct tm  tm;
    time_t     elapsed = ngx_cached_time->sec;

    ngx_http_datetime_localtime(elapsed, &tm);

    return ngx_http_datetime_fmt(r, v, tm.tm_hour, NGX_INT32_LEN);
}


static ngx_int_t
ngx_http_variable_local_minute(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    struct tm  tm;
    time_t     elapsed = ngx_cached_time->sec;

    ngx_http_datetime_localtime(elapsed, &tm);

    return ngx_http_datetime_fmt(r, v, tm.tm_min, NGX_INT32_LEN);
}


static ngx_int_t
ngx_http_variable_local_second(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    struct tm  tm;
    time_t     elapsed = ngx_cached_time->sec;

    ngx_http_datetime_localtime(elapsed, &tm);

    return ngx_http_datetime_fmt(r, v, tm.tm_sec, NGX_INT32_LEN);
}


static ngx_int_t
ngx_http_variable_year(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    struct tm  tm;
    time_t     elapsed = ngx_cached_time->sec;

    ngx_http_datetime_gmtime(elapsed, &tm);

    return ngx_http_datetime_fmt(r, v, tm.tm_year, NGX_INT32_LEN);
}


static ngx_int_t
ngx_http_variable_month(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    struct tm  tm;
    time_t     elapsed = ngx_cached_time->sec;

    ngx_http_datetime_gmtime(elapsed, &tm);

    return ngx_http_datetime_fmt(r, v, tm.tm_mon, NGX_INT32_LEN);
}


static ngx_int_t
ngx_http_variable_day(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    struct tm  tm;
    time_t     elapsed = ngx_cached_time->sec;

    ngx_http_datetime_gmtime(elapsed, &tm);

    return ngx_http_datetime_fmt(r, v, tm.tm_mday, NGX_INT32_LEN);
}


static ngx_int_t
ngx_http_variable_hour(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    struct tm  tm;
    time_t     elapsed = ngx_cached_time->sec;

    ngx_http_datetime_gmtime(elapsed, &tm);

    return ngx_http_datetime_fmt(r, v, tm.tm_hour, NGX_INT32_LEN);
}


static ngx_int_t
ngx_http_variable_minute(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    struct tm  tm;
    time_t     elapsed = ngx_cached_time->sec;

    ngx_http_datetime_gmtime(elapsed, &tm);

    return ngx_http_datetime_fmt(r, v, tm.tm_min, NGX_INT32_LEN);
}


static ngx_int_t
ngx_http_variable_second(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    struct tm  tm;
    time_t     elapsed = ngx_cached_time->sec;

    ngx_http_datetime_gmtime(elapsed, &tm);

    return ngx_http_datetime_fmt(r, v, tm.tm_sec, NGX_INT32_LEN);
}


static ngx_int_t
ngx_http_datetime_add_variable(ngx_conf_t *cf)
{
    ngx_uint_t            i;
    ngx_http_variable_t *var;

    for (i = 0; i < sizeof(ngx_http_datetime_variables) / sizeof(ngx_http_datetime_variable_t); i++) {
        var = ngx_http_add_variable(cf,
                                    &ngx_http_datetime_variables[i].name,
                                    ngx_http_datetime_variables[i].flags);
        if (var == NULL) {
            return NGX_ERROR;
        }

        var->get_handler = ngx_http_datetime_variables[i].get_handler;
        var->data = 0;

        if (ngx_http_get_variable_index(cf, &ngx_http_datetime_variables[i].name) == NGX_ERROR) {
            return NGX_ERROR;
        }
    }

    return NGX_OK;
}
