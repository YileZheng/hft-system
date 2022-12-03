set -e 
set -u

host=root@127.0.0.1
files="$host:/mnt/*summary* $host:/mnt/timeline_trace.csv"
mkdir -p debug
sshpass -p root scp -P 1440 -r $files ./debug
