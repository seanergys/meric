# Master/Slave init helper

This folder builds the `master` helper and uses it to generate a `slave.sh` script.

## Build (on the Master machine)

From this directory:

```bash
make
```

This produces the `./master` binary.

## Generate the slave script and start waiting (on the Master machine)

Run:

```bash
./master --init
```

This creates `slave.sh` in the current directory (with `MASTER_IP` filled in), and then the master keeps running and waits for `START` / `STOP` messages on TCP port `5000`.

If the IP is not the one you want (e.g., multiple NICs), edit `slave.sh` and set `MASTER_IP` manually.

## Configure what the Master runs during START..STOP

Edit `master.cpp` and put your measurement code inside the workload loop.

When `START` arrives, `master` starts a background loop and keeps running it until `STOP` arrives.

## Start the Master listener (on the Master machine)

In one terminal, run:

```bash
./master
```

This listens on TCP port `5000` and waits for `START` / `STOP` messages.

## Run the slave script (on the machine you want to monitor)

1. Copy `slave.sh` to the target machine if not NAS system.
2. Edit the workload section in `slave.sh` (replace the example `sleep 5` with your real command).
3. Ensure `nc` (netcat) is installed on the target machine.
4. Run:

```bash
./slave.sh
```

The script sends:
- `START` to `$MASTER_IP:5000`
- runs the workload
- `STOP` to `$MASTER_IP:5000`