import csv
import os
from collections import defaultdict
from pathlib import Path

relevant_data = defaultdict(dict)
field_lengths = []
data_type = "load"


def write_to_csv():
    # Create Data folder if it does not exist
    data_folder_path = str(Path(__file__).parent.absolute()) + "/Data/"
    Path(data_folder_path).mkdir(parents=True, exist_ok=True)

    file_name = data_folder_path + data_type + "_data.csv"
    with open(file_name, 'w', newline='') as csvfile:
        if data_type == "load":
            fieldnames = [
                "field_length",
                "throughput0", "throughput1", "throughput2", "throughput3",
                "insert_latency0", "insert_latency1", "insert_latency2", "insert_latency3",
            ]
        else:
            fieldnames = [
                "field_length",
                "throughput0", "throughput1", "throughput2", "throughput3",
                "update_latency0", "update_latency1", "update_latency2", "update_latency3",
                "read_latency0", "read_latency1", "read_latency2", "read_latency3",
            ]

        writer = csv.DictWriter(
            csvfile,
            fieldnames=fieldnames,
            quoting=csv.QUOTE_MINIMAL
        )

        # output csv by sorted integers
        field_lengths_as_int = sorted(map(int, field_lengths))

        writer.writeheader()
        for field_length in field_lengths_as_int:
            writer.writerow(relevant_data[str(field_length)])


def extract_data(line: str, field_length, client_id):
    line_split = line.split(" ")

    if line_split[0] == "[OVERALL]," and line_split[1] == "Throughput(ops/sec),":
        relevant_data[field_length][f"throughput{client_id}"] = line_split[2].strip()

    if line_split[0] == "[READ]," and line_split[1] == "AverageLatency(us),":
        relevant_data[field_length][f"read_latency{client_id}"] = line_split[2].strip()

    if line_split[0] == "[INSERT]," and line_split[1] == "AverageLatency(us),":
        relevant_data[field_length][f"insert_latency{client_id}"] = line_split[2].strip()

    if line_split[0] == "[UPDATE]," and line_split[1] == "AverageLatency(us),":
        relevant_data[field_length][f"update_latency{client_id}"] = line_split[2].strip()


def main():
    # Path of folder where the ycsb data is in
    data_folder_path = (Path(__file__).parent.absolute() / data_type).resolve()

    subfolder_paths: list = [f.path for f in os.scandir(data_folder_path) if f.is_dir()]

    for subfolder_path in subfolder_paths:
        # subfolder_path is absolute path, just need folder name
        subfolder_path_split = subfolder_path.split("/")
        field_length = subfolder_path_split[-1:][0]
        field_lengths.append(field_length)

        # Each subfolder has 1 data file for each of the 4 clients
        for i in range(0, 4):
            entry_path = subfolder_path + "/" + str(i)
            with open(entry_path) as file:
                lines = file.readlines()
                for line in lines:
                    extract_data(line, field_length, i)

    # add field lengths field
    for field_length in field_lengths:
        relevant_data[field_length]["field_length"] = field_length

    write_to_csv()


if __name__ == '__main__':
    main()
