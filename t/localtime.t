use Test::Nginx::Socket 'no_plan';

run_tests();

__DATA__

=== TEST 1: testing localtime varibale
--- config
location /t {
    content_by_lua_block {
        ngx.say(ngx.var.datetime_lyear, ngx.var.datetime_lmonth, ngx.var.datetime_lday, ngx.var.datetime_lhour)
    }
    access_log logs/access-$datetime_year-$datetime_month-$datetime_day-$datetime_hour-$datetime_minute.log;
}
--- request
GET /t

--- response_body eval
use POSIX;
POSIX::strftime("%Y%m%d%H\n", localtime);
