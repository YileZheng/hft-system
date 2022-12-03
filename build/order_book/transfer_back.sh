
set -e 
set -u

host=leon@143.89.131.98
dir=/home/leon/01-git/hft-system/build/order_book/hw/debug/
files="*summary* timeline_trace.csv"
ssh $host "mkdir -p $dir"
scp $files $host
