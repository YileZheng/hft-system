set -e 
set -u

host=root@143.89.131.98
files="$host:/mnt/*summary* $host:/mnt/timeline_trace.csv"
mkdir -p debug
sshpass -p root scp -P 10022 -r $files ./debug
