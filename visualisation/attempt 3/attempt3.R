library(ggplot2)
data <- read.csv("attempt 3/attempt 3_data.csv")
data$redirect_clients.chars = as.character(data$redirect_clients)

data.t <- subset(data, data$type == "throughput")
data.u <- subset(data, data$type == "update_latency")
data.r <- subset(data, data$type == "read_latency")

throughput_boxplot <- ggplot(data.t, aes(x=reorder(redirect_clients.chars, redirect_clients), y=value)) +
  geom_boxplot() +
  labs(
    title = "YCSB - 6 Servers - 4 Clients á 25 Threads - Workload a",
    subtitle = "Throughput in ops/sec",
    y = "",
    x = "Redirect Clients per Server",
    caption = "n = 10.000"
  )

throughput_sum <- ggplot(data.t, aes(x=reorder(redirect_clients.chars, redirect_clients), y=value)) +
  stat_summary(fun = sum, geom = "point") +
  labs(
    title = "YCSB - 6 Servers - 4 Clients á 25 Threads - Workload a",
    subtitle = "Throughput in ops/sec",
    y = "",
    x = "Redirect Clients per Server",
    caption = "n = 10.000"
  )

print(throughput_boxplot)
print(throughput_sum)

update_latency_boxplot <- ggplot(data.u, aes(x=reorder(redirect_clients.chars, redirect_clients), y=value)) +
  geom_boxplot() +
  labs(
    title = "YCSB - 6 Servers - 4 Clients á 25 Threads - Workload a",
    subtitle = "Update Latency in ?",
    y = "",
    x = "Redirect Clients per Server",
    caption = "n = 10.000"
  )

print(update_latency_boxplot)


read_latency_boxplot <- ggplot(data.r, aes(x=reorder(redirect_clients.chars, redirect_clients), y=value)) +
  geom_boxplot() +
  labs(
    title = "YCSB - 6 Servers - 4 Clients á 25 Threads - Workload a",
    subtitle = "Read Latency in ?",
    y = "",
    x = "Redirect Clients per Server",
    caption = "n = 10.000"
  )

print(read_latency_boxplot)


ggsave(plot=throughput_boxplot,"attempt 3/Throughput_boxplot.png")
ggsave(plot=throughput_sum,"attempt 3/Throughput_sum.png")
ggsave(plot=update_latency_boxplot,"attempt 3/Update_boxplot.png")
ggsave(plot=read_latency_boxplot,"attempt 3/Read_boxplot.png")
