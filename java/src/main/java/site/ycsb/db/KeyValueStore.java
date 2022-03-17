package site.ycsb.db;

import site.ycsb.ByteArrayByteIterator;
import site.ycsb.ByteIterator;
import site.ycsb.DB;
import site.ycsb.Status;

import java.util.HashMap;
import java.util.Map;
import java.util.Set;
import java.util.Vector;

/**
 * Simplifies the abstract class 'DB' to easily work with a key-value store.
 * The YCSB-property 'fieldcount' needs to be set to 1 in order for this class to work.
 * In order to simulate database tables, namespaced keys of the from 'table.object-key' are generated.
 *
 * @author Fabian Ruhland, 07.06.2019
 */
public abstract class KeyValueStore extends DB {

    /**
     * Initialize your HeineStore client and it connect it to your HeineStore server.
     */
    @Override
    public abstract void init();

    /**
     * Get an object from your HeineStore server.
     *
     * @param key The key, under which the object is stored.
     *
     * @return The object.
     */
    abstract byte[] get(String key);

    /**
     * Store an object on your HeineStore server.
     *
     * @param key The key, under which the object shall be stored
     * @param value The object to store
     *
     * @return A status code (e.g. Status.SUCCESS, if the operation was successful)
     */
    abstract Status put(String key, byte[] value);

    /**
     * Delete an object from your HeineStore server.
     *
     * @param key The key, under which the object is stored.
     *
     * @return A status code (e.g. Status.SUCCESS, if the operation was successful)
     */
    abstract Status delete(String key);

    @Override
    public Status read(String table, String key, Set<String> fields, Map<String, ByteIterator> result) {
        if(fields != null && fields.size() != 1) {
            System.err.println("Field counts other than 1 are not supported!");
            return Status.BAD_REQUEST;
        }

        byte[] value = get(generateKey(table, key));

        if(value == null) {
            return Status.NOT_FOUND;
        }

        if(fields == null) {
            result.put("field0", new ByteArrayByteIterator(value));
        } else {
            result.put(fields.iterator().next(), new ByteArrayByteIterator(value));
        }

        return Status.OK;
    }

    @Override
    public Status scan(String table, String startkey, int recordcount, Set<String> fields,
                       Vector<HashMap<String, ByteIterator>> result) {
        return Status.NOT_IMPLEMENTED;
    }

    @Override
    public Status update(String table, String key, Map<String, ByteIterator> values) {
        if(values.size() != 1) {
            System.err.println("Field counts other than 1 are not supported!");
            return Status.BAD_REQUEST;
        }

        return put(generateKey(table, key), values.values().iterator().next().toArray());
    }

    @Override
    public Status insert(String table, String key, Map<String, ByteIterator> values) {
        if(values.size() != 1) {
            System.err.println("Field counts other than 1 are not supported!");
            return Status.BAD_REQUEST;
        }

        return update(table, key, values);
    }

    @Override
    public Status delete(String table, String key) {
        return delete(generateKey(table, key));
    }

    /**
     * Generate a namespaced key for the key-value store.
     *
     * @param table The table, which the YCSB wants to access
     * @param key The key of the object, that the YCSB wants to access
     *
     * @return The generated key
     *
     * @author Fabian Ruhland, 07.06.2019
     */
    private String generateKey(String table, String key) {
        return String.format("%s.%s", table, key);
    }
}
