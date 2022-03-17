import csv
import os
from collections import defaultdict
from pathlib import Path

data_folder = "attempt 3"
data_type = "run"

# redirect_clients, type, value
r_data = []


def write_to_csv():
    # Create Data folder if it does not exist
    data_folder_path = str(Path(__file__).parent.absolute()) + "/Data/"
    Path(data_folder_path).mkdir(parents=True, exist_ok=True)

    file_name = data_folder_path + data_folder + "_data.csv"
    with open(file_name, 'w', newline='') as csvfile:
        fieldnames = ["redirect_clients", "type", "value"]

        writer = csv.DictWriter(
            csvfile,
            fieldnames=fieldnames,
            quoting=csv.QUOTE_MINIMAL
        )

        # output csv by sorted integers
        writer.writeheader()
        for row in r_data:
            writer.writerow(row)


def extract_data(line: str, redirect_clients):
    line_split = line.split(" ")

    if line_split[0] == "[OVERALL]," and line_split[1] == "Throughput(ops/sec),":
        r_data.append({
            "redirect_clients": redirect_clients,
            "type": "throughput",
            "value": float(line_split[2].strip())
        })

    if line_split[0] == "[READ]," and line_split[1] == "AverageLatency(us),":
        r_data.append({
            "redirect_clients": redirect_clients,
            "type": "read_latency",
            "value": float(line_split[2].strip())
        })

    if line_split[0] == "[UPDATE]," and line_split[1] == "AverageLatency(us),":
        r_data.append({
            "redirect_clients": redirect_clients,
            "type": "update_latency",
            "value": float(line_split[2].strip())
        })


def main():
    # Path of folder where the ycsb data is in
    data_folder_path = (Path(__file__).parent.absolute() / data_folder).resolve()

    for root, dirs, files in os.walk(data_folder_path):
        for filename in files:

            if filename == ".DS_Store":
                continue

            file_path = os.path.join(root, filename)
            file_path_split = file_path.split("/")
            redirect_clients = file_path_split[-3]

            with open(file_path) as file:
                lines = file.readlines()
                for line in lines:
                    extract_data(line, redirect_clients)

    print(r_data)
    write_to_csv()


if __name__ == '__main__':
    main()
