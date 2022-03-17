package site.ycsb.db;

import site.ycsb.Status;

import java.nio.charset.StandardCharsets;
import java.util.Properties;

/**
 * DRDB binding
 */
public class JDrdbClient extends KeyValueStore {
    static {
        System.loadLibrary("jdrdb-client");
    }

    public long nativePtr;
    public int getRequest = 0;
    public int putRequest = 0;
    public int delRequest = 0;

    public void destroy() {
        destroyDrdbClient(this.nativePtr);
        nativePtr = 0L;
    }

    public native long initDrdbClient(String serverAddress, int serverPort, String logDir);

    private native void destroyDrdbClient(long nativePtr);

    public native byte[] get(long nativePtr, String key, int getRequest);

    public native byte put(long nativePtr, String key, byte[] value, int putRequest);

    public native byte del(long nativePtr, String key, int delRequest);

    // For YCSB
    @Override
    public void init() {
        System.out.println("Intialising Properties");
        DrdbProperties drdbProperties = new DrdbProperties(getProperties());

        System.out.printf("Connecting to Drdb server at %s:%d with log_dir %s",
                drdbProperties.getServerAddress(), drdbProperties.getServerPort(), drdbProperties.getLogDir());

        nativePtr = initDrdbClient(drdbProperties.getServerAddress(), drdbProperties.getServerPort(), drdbProperties.getLogDir());
        System.out.println("Set nativePtr to " + nativePtr);

    }

    // For testing without YCSB
    public void init_testing(String serverAddress, int serverPort, String logDir) {
        nativePtr = initDrdbClient(serverAddress, serverPort, logDir);
    }

    @Override
    public byte[] get(String key) {
        getRequest++;

        byte[] keyBytes = key.getBytes(StandardCharsets.UTF_8);
        String utf8EncodedKey = new String(keyBytes, StandardCharsets.UTF_8);
        byte[] result = get(nativePtr, utf8EncodedKey, getRequest);

        if (result.length == 0) {
            return null;
        }

        return result;
    }

    @Override
    public Status put(String key, byte[] value) {
        putRequest++;

        byte reply = put(nativePtr, key, value, putRequest);

        if (reply == 0x0) {
            return Status.OK;
        }

        if (reply == 0x1) {
            return Status.NOT_FOUND;
        }

        return Status.ERROR;
    }

    @Override
    public Status delete(String key) {
        delRequest++;

        byte reply = del(nativePtr, key, delRequest);

        if (reply == 0x0) {
            return Status.OK;
        }

        if (reply == 0x1) {
            return Status.NOT_FOUND;
        }

        return Status.ERROR;
    }

    private static final class DrdbProperties {
        private static final String SERVER_ADDRESS = "drdb.address";
        private static final String SERVER_PORT = "drdb.port";
        private static final String LOG_DIR = "drdb.log_dir";

        private final String serverAddress;
        private final int serverPort;
        private final String logDir;

        DrdbProperties(final Properties properties) {
            serverAddress = properties.getProperty(SERVER_ADDRESS, "127.0.0.1");
            serverPort = Integer.parseInt(properties.getProperty(SERVER_PORT, "1797"));
            logDir = properties.getProperty(LOG_DIR, "/tmp/drdb/logs/jclient");
        }

        String getServerAddress() {
            return serverAddress;
        }

        int getServerPort() {
            return serverPort;
        }

        String getLogDir() {
            return logDir;
        }
    }

}
