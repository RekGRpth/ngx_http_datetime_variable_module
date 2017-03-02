use Test::Nginx::Socket 'no_plan';

run_tests();

__DATA__

=== TEST 1: testing utc time varibale
--- config
location /t {
    content_by_lua_block {
        ngx.say(ngx.var.datetime_year, ngx.var.datetime_month, ngx.var.datetime_day, ngx.var.datetime_hour)
    }
    access_log logs/access-$datetime_year-$datetime_month-$datetime_day-$datetime_hour-$datetime_minute.log;
}
--- request
GET /t

--- response_body eval
use POSIX;
POSIX::strftime("%Y%m%d%H\n", gmtime);
