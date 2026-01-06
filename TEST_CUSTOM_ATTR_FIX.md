# Manual Test for Custom Attribute Restoration Fix

This document describes how to manually test that custom attributes are properly restored when loading a GDBM flatfile into LMDB.

## Prerequisites
- Two builds of TinyMUSH: one with GDBM backend, one with LMDB backend
- Test database with custom attributes

## Test Steps

### 1. Create Test Database with GDBM Backend

```bash
# Build with GDBM backend
cd /path/to/TinyMUSH
mkdir build-gdbm && cd build-gdbm
cmake .. -DCMAKE_BUILD_TYPE=Debug -DUSE_GDBM=ON
make -j4

# Initialize minimal database
cd /path/to/TinyMUSH/game
./netmush -m netmush.conf

# Start server
./netmush netmush.conf
```

### 2. Create Custom Attributes

Connect to the server as #1 (GOD):
```
connect #1 <password>
&CUSTOMATTR me=It's a custom attribute
&TESTATTR me=Another test value
&NUMERIC_ATTR me=12345
ex me
```

Expected output should show:
```
CUSTOMATTR [#1]:It's a custom attribute
TESTATTR [#1]:Another test value  
NUMERIC_ATTR [#1]:12345
```

### 3. Shutdown and Create Flatfile

```bash
# From inside the game (as #1)
@shutdown

# Create flatfile from GDBM database
cd /path/to/TinyMUSH/game
./netmush --dbconvert db/netmush.db < db/netmush.db > test.flat

# Verify flatfile contains custom attribute definitions
grep "+A" test.flat | grep -i "CUSTOMATTR\|TESTATTR\|NUMERIC"

# Verify flatfile contains custom attribute values
grep ">.*It's a custom attribute" test.flat
```

Expected: You should see both the attribute definitions (+A lines) and the attribute values (> lines) in the flatfile.

### 4. Build with LMDB Backend

```bash
cd /path/to/TinyMUSH
mkdir build-lmdb && cd build-lmdb
cmake .. -DCMAKE_BUILD_TYPE=Debug -DUSE_LMDB=ON
make -j4
```

### 5. Load Flatfile into LMDB

```bash
cd /path/to/TinyMUSH/game

# Remove old GDBM database
rm -f db/netmush.db
rm -rf db/netmush.db.lmdb

# Load flatfile into LMDB
../build-lmdb/src/netmush/netmush --load-flatfile test.flat netmush.conf
```

Expected output:
```
Loading flatfile from: test.flat
Reading ...................
(database loading messages)
```

### 6. Verify Custom Attributes Are Restored

```bash
# Start server with LMDB backend
./netmush netmush.conf
```

Connect as #1 and verify:
```
connect #1 <password>
ex me
```

**EXPECTED RESULT (with fix):**
```
CUSTOMATTR [#1]:It's a custom attribute
TESTATTR [#1]:Another test value
NUMERIC_ATTR [#1]:12345
```

**BROKEN BEHAVIOR (without fix):**
The custom attributes would not be shown in the output.

### 7. Test Attribute Persistence

While still connected:
```
&NEWATTR me=Testing persistence
@shutdown
```

Start server again and verify:
```
./netmush netmush.conf
# Connect as #1
ex me
```

Expected: All attributes including NEWATTR should be present.

## Verification Points

1. ✓ Custom attribute definitions are in the flatfile
2. ✓ Custom attribute values are in the flatfile  
3. ✓ Custom attributes appear on objects after loading flatfile into LMDB
4. ✓ Custom attributes persist across server restarts with LMDB backend

## Clean Up

```bash
rm test.flat
rm -rf db/*
```

## Troubleshooting

If custom attributes are still missing after the fix:

1. Check that the flatfile contains the custom attributes:
   ```bash
   grep "+A" test.flat | grep CUSTOMATTR
   ```

2. Check the LMDB database was created:
   ```bash
   ls -la db/netmush.db.lmdb/
   ```

3. Check server logs for any errors during flatfile load

4. Verify `mushstate.standalone` is being set correctly by adding debug logging
