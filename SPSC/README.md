# Market Data Publishing System

Low-latency market data publishing system in C++17 with three processes:
- **Publisher**: Generates market data and publishes to both TCP and Shared Memory
- **SHM Consumer**: Reads from shared memory ring buffer
- **TCP Consumer**: Reads from TCP loopback socket

## Build

```bash
make publisher shm_consumer tcp_consumer
```

## Run

Terminal 1 - Publisher:
```bash
./publisher
```

Terminal 2 - SHM Consumer:
```bash
./shm_consumer
```

Terminal 3 - TCP Consumer:
```bash
./tcp_consumer
```

## Features

- SPSC (Single Producer Single Consumer) lock-free ring buffer using shared memory
- TCP loopback server with optimized settings (Nagle disabled, non-blocking)
- Nanosecond precision timestamps
- Minimal, production-style code
- C++17 compliant

## Architecture

- **Message struct**: Fixed-size with instrument, bid, ask, timestamp_ns
- **Ring Buffer**: Cache-line aligned indices, lock-free using std::atomic
- **TCP**: JSON messages over loopback (127.0.0.1:8888)
- **SHM**: Binary struct copy via shared memory ring buffer
