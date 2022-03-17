import subprocess
import pathlib

# /ycsb/drdb/ycsb-drdb-binding-0.18.0-SNAPSHOT/bin/ycsb run drdb -P /home/moghaddam/workloads/a/fieldlength-varying/1 -p drdb.address=10.112.51.173 -p drdb.port=13337 -p insertstart=0 -p insertcount=2500 >> /media/ssd/drdb/benchmarks/varying-fieldlength/ssd/load/1/1

ycsb_fieldlength = "1"

ycsb_command = "/ycsb/drdb/ycsb-drdb-binding-0.18.0-SNAPSHOT/bin/ycsb"
ycsb_mode = "run"
ycsb_workload = f"/home/moghaddam/workloads/a/fieldlength-varying/{ycsb_fieldlength}"
ycsb_server = "drdb.address=10.112.51.173"
ycsb_port = "drdb.port=13337"

# Create log dir
pathlib.Path("/media/ssd/drdb/logs/").mkdir(exist_ok=True, parents=True)

# Create output dir
pathlib.Path(f"/media/ssd/drdb/benchmarks/varying-fieldlength/ssd/run/{ycsb_fieldlength}/").mkdir(exist_ok=True, parents=True)

commands = []
for i in range(0,4):
    ycsb_log_dir = f"drdb.log_dir=/media/ssd/drdb/logs/{i}"
    ycsb_output = f"/media/ssd/drdb/benchmarks/varying-fieldlength/ssd/run/{ycsb_fieldlength}/{i}"

    command = [ycsb_command, ycsb_mode,"drdb","-P" ,ycsb_workload, "-p", ycsb_server, "-p", ycsb_port,"-p",ycsb_log_dir,">>", ycsb_output]

    command_str = ' '.join(command)
    commands.append(command_str)

final_command = ' & '.join(commands)
subprocess.run(final_command, shell=True)