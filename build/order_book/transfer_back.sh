
set -e 
set -u

host=leon@192.168.100.38
dir=/home/leon/01-git/hft-system/build/order_book/hw/debug/
files="*summary* timeline_trace.csv"
ssh $host "mkdir -p $dir"
scp $files $host
