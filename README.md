[tengine]: http://tengine.taobao.org/document_cn/http_log_cn.html

# ngx_http_datetime_variable_module

For support nginx rotating log, these are some ways to solve this as the following:

1. Using logrotate
2. Using access_log pipe patched by [tengine]

But I'm more prefer to using variable in access_log for nginx just like this as the following:

```bash
http {
  access_log logs/access-%YY-%MM-%DD-%HH-%mm-%ss.log
}
```

In nginx internals, there are no such variable like %YY, So i adding the variable by nginx module for nginx as the following:

1. $datetime_year: [1970-xxxx]
2. $datetime_month: [00-12]
3. $datetime_day: [0-31]
4. $datetime_hour: [0-23]
5. $datetime_minute: [0-59]
6. $datetime_second: [0-59]
7. $datetime_lyear: localtime for datetime_year
8. $datetime_lmonth: localtime for datetime_month
9. $datetime_lday: localtime for datetime_day
10. $datime_lhour: localtime for datetime_hour
11. $datetime_lminute: localtime for datetime_minute
12. $datetime_lsecond: localtime for datetime_second

