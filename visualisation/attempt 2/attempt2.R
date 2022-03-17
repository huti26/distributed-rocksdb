library(ggplot2)
data <- read.csv("attempt 2/run_data.csv")

data$throughput <- data$throughput1 + data$throughput2 + data$throughput3 + data$throughput0
data$read.latency <- (data$read_latency1 + data$read_latency2 + data$read_latency3 + data$read_latency0) / 4
data$update.latency <- (data$update_latency1 + data$update_latency2 + data$update_latency3 + data$update_latency0) / 4

data$field_length.chars <- as.character(data$field_length)

# dont use scientific notation on axis
options(scipen=10000)

g1 <- ggplot(data, aes(x=reorder(field_length.chars, field_length),y=throughput)) + 
  geom_point() +
  labs(
    title = "YCSB - 1 Server - 4 Client Threads - Workload a",
    subtitle = "Throughput in ops/sec",
    y = "",
    x = "fieldlength in bytes",
    caption = "n = 100.000"
  )

g2 <- ggplot(data, aes(x=reorder(field_length.chars, field_length),y=read.latency)) + 
  geom_point() +
  labs(
    title = "YCSB",
    subtitle = "Average Read Latency in microseconds",
    y = "",
    x = "fieldlength in bytes",
    caption = "n = 100.000"
  )

g3 <- ggplot(data, aes(x=reorder(field_length.chars, field_length),y=update.latency)) + 
  geom_point() +
  labs(
    title = "YCSB",
    subtitle = "Average Update Latency in microseconds",
    y = "",
    x = "fieldlength in bytes",
    caption = "n = 100.000"
  )

print(g1)
print(g2)
print(g3)

ggsave(plot=g1,"attempt 2/Throughput.png")
ggsave(plot=g2,"attempt 2/Read.png")
ggsave(plot=g3,"attempt 2/Update.png")
