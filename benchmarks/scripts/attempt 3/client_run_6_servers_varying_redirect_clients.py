import subprocess
import pathlib
import socket

hostname = socket.gethostname()

# /ycsb/drdb/ycsb-drdb-binding-0.18.0-SNAPSHOT/bin/ycsb run drdb -P /home/moghaddam/workloads/a/fieldlength-varying/1 -p drdb.address=10.112.51.173 -p drdb.port=13337 -p insertstart=0 -p insertcount=2500 >> /media/ssd/drdb/benchmarks/varying-fieldlength/ssd/load/1/1

redirect_clients = "16"

threadcount= 25
ycsb_fieldlength = "1024"
ycsb_command = "/ycsb/drdb/ycsb-drdb-binding-0.18.0-SNAPSHOT/bin/ycsb"
ycsb_mode = "run"
ycsb_workload = f"/home/moghaddam/workloads/a/fieldlength-varying/{ycsb_fieldlength}"

# 6 Servers
servers =   ["drdb.address=10.112.51.168","drdb.address=10.112.51.169","drdb.address=10.112.51.170",
            "drdb.address=10.112.51.171","drdb.address=10.112.51.173","drdb.address=10.112.51.174"]
ports = ["drdb.port=13337","drdb.port=13338","drdb.port=13339","drdb.port=13340","drdb.port=13341","drdb.port=13342"]

# Create log dir
pathlib.Path("/media/ssd/drdb/logs/").mkdir(exist_ok=True, parents=True)

# Create output dir
pathlib.Path(f"/media/ssd/drdb/benchmarks/varying-redirect_clients/{redirect_clients}/{threadcount}").mkdir(exist_ok=True, parents=True)

commands = []
for i in range(0,threadcount):
    ycsb_log_dir = f"drdb.log_dir=/media/ssd/drdb/logs/{i}"
    ycsb_output = f"/media/ssd/drdb/benchmarks/varying-redirect_clients/{redirect_clients}/{threadcount}/{hostname}_{i}"

    # Alternate between all 6 servers
    ycsb_server = servers[i%6]
    ycsb_port = ports[i%6]

    command = [ycsb_command, ycsb_mode,"drdb","-P" ,ycsb_workload, "-p", ycsb_server, "-p", ycsb_port,"-p",ycsb_log_dir,">>", ycsb_output]

    command_str = ' '.join(command)
    commands.append(command_str)

final_command = ' & '.join(commands)
subprocess.run(final_command, shell=True)