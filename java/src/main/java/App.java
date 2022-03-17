import site.ycsb.db.JDrdbClient;

import java.util.UUID;
import java.nio.charset.StandardCharsets;
import java.util.Vector;

public class App {
    public static void main(String[] args) {
        var jDrdbClient = new JDrdbClient();
        jDrdbClient.init_testing("server1", 13337, "tmp/drdb/logs");

        Vector<String> vector = new Vector<>();
        Vector<byte[]> vector2 = new Vector<>();

        // Prepare Data
        for (int i = 0; i < 100000; i++) {
            vector.add(UUID.randomUUID().toString());
            vector2.add(UUID.randomUUID().toString().getBytes(StandardCharsets.UTF_8));
        }

        // Put
        for (int i = 0; i < 100000; i++) {
            System.out.println("PUT " + i);
            jDrdbClient.put(vector.get(i), vector2.get(i));
        }

        // Get
        for (int i = 0; i < 100000; i++) {
            System.out.println("GET " + i);
            jDrdbClient.get(vector.get(i));
        }

//        String x = "x";
//        String y = "y";
//
//        String apple = "apple";
//        String banana = "banana";
//
//        byte[] apple_bytes = apple.getBytes(StandardCharsets.UTF_8);
//        byte[] banana_bytes = banana.getBytes(StandardCharsets.UTF_8);
//
//        jDrdbClient.get(x);
//        jDrdbClient.put(x, apple_bytes);
//        jDrdbClient.get(x);
//        jDrdbClient.delete(x);
//        jDrdbClient.get(x);

    }
}
