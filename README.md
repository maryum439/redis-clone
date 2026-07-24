# Redis Clone in C++

A Redis-inspired in-memory key-value store built from scratch in C++ to explore systems programming, networking, protocol parsing, and event-driven concurrency.

This project started as an implementation of a basic in-memory database and was gradually extended to support the RESP protocol, multiple Redis data types, key expiration, non-blocking TCP networking, and concurrent client handling using Linux `epoll`.

## Features

* TCP server built using POSIX sockets
* RESP (Redis Serialization Protocol) parsing
* In-memory key-value storage
* Multiple data types:

  * Strings
  * Lists
  * Hashes
* Key expiration and TTL support
* Non-blocking sockets
* Event-driven I/O using Linux `epoll`
* Multiple simultaneous client connections
* Per-client input buffering for partial TCP reads
* Per-client output buffering for partial writes
* Support for pipelined commands received in a single read
* Tested with multiple clients and servers

## Supported Commands

### Strings

```text
SET key value
GET key
```

### Lists

```text
LPUSH key value [value ...]
LRANGE key start stop
RPOP key
```

### Hashes

```text
HSET key field value
HGET key field
```

### Key Management

```text
DEL key [key ...]
EXISTS key [key ...]
DBSIZE
EXPIRE key seconds
TTL key
```

### Server

```text
PING
```

The server also returns Redis-style errors for invalid commands, incorrect argument counts, and operations performed on incompatible data types.

## Architecture

The server uses an event-driven architecture built around `epoll`.

```text
                    ┌────────────────────┐
                    │    epoll_wait()    │
                    └─────────┬──────────┘
                              │
               ┌──────────────┴──────────────┐
               │                             │
        Listening Socket                Client Socket
               │                             │
           EPOLLIN                       EPOLLIN
               │                             │
             accept()                     read()
               │                             │
               ▼                             ▼
        Register Client              Append to Input Buffer
                                              │
                                              ▼
                                      Parse RESP Commands
                                              │
                                              ▼
                                       Execute Command
                                              │
                                              ▼
                                      Append Response to
                                      Output Buffer
                                              │
                                              ▼
                                          EPOLLOUT
                                              │
                                              ▼
                                           write()
```

### Client Connection Handling

Each client has its own input and output buffer:

```cpp
std::unordered_map<int, std::string> clientBuffers;
std::unordered_map<int, std::string> clientOutputBuffers;
```

This is necessary because TCP is a byte stream. A single command may be split across multiple `read()` calls, or multiple commands may arrive in one read.

The server therefore:

1. Reads available bytes from a client.
2. Appends them to that client's input buffer.
3. Attempts to parse complete RESP commands.
4. Leaves incomplete commands in the buffer until more data arrives.
5. Executes complete commands.
6. Stores responses in the client's output buffer.
7. Enables `EPOLLOUT` while data remains to be written.
8. Disables `EPOLLOUT` once the output buffer is empty.

## RESP Parsing

Commands are sent using the Redis Serialization Protocol.

For example:

```text
SET name Maryum
```

is represented as:

```text
*3\r\n
$3\r\n
SET\r\n
$4\r\n
name\r\n
$6\r\n
Maryum\r\n
```

The parser handles:

* Arrays of bulk strings
* Partial messages
* Multiple commands received in a single buffer
* Correct CRLF termination
* Length-prefixed bulk strings

The input buffer is only consumed once a complete command has been successfully parsed.

## Key Expiration

Keys can be assigned an expiration time using:

```text
EXPIRE key seconds
```

Expiration times are stored separately from the main key-value store:

```cpp
std::unordered_map<std::string, long long> expiry;
```

Expiration is handled lazily when keys are accessed, as well as through periodic cleanup during relevant database operations.

Example:

```text
SET session active
EXPIRE session 10
TTL session
```

## Event-Driven Concurrency

Instead of creating a thread for every client, the server uses Linux `epoll` to monitor multiple file descriptors from a single event loop.

The server monitors:

* The listening socket for new connections
* Client sockets for incoming data
* Client sockets for outgoing data when responses are waiting to be written

All sockets are configured as non-blocking.

This allows the server to handle multiple clients without blocking while waiting for a particular client to send or receive data.

## Project Structure

The implementation currently consists of:

```text
Redis Clone
│
├── TCP Socket Server
│   ├── socket()
│   ├── bind()
│   ├── listen()
│   └── accept()
│
├── RESP Parser
│   └── Parses Redis-style array and bulk-string commands
│
├── In-Memory Data Store
│   ├── Strings
│   ├── Lists
│   └── Hashes
│
├── Expiration System
│   ├── EXPIRE
│   └── TTL
│
└── Event Loop
    ├── epoll_create1()
    ├── epoll_ctl()
    ├── epoll_wait()
    ├── Non-blocking reads
    └── Buffered non-blocking writes
```

## Running the Server

### Compile

```bash
g++ -std=c++17 -o redis_clone main.cpp
```

### Run

```bash
./redis_clone
```

The server listens on port:

```text
6379
```

A Redis-compatible client can then be used to connect to the server.

## Example

```text
SET name Maryum
OK

GET name
Maryum

LPUSH numbers 1 2 3

LRANGE numbers 0 -1

HSET user name Maryum

HGET user name
Maryum

EXPIRE name 60

TTL name
```

## What I Learned

This project provided hands-on experience with:

* TCP socket programming
* Client-server architecture
* The difference between blocking and non-blocking I/O
* Linux file descriptors
* Event-driven programming
* `epoll` and readiness-based I/O
* Handling partial reads and writes
* Network protocol design and parsing
* In-memory data structures
* Key expiration and time-based state
* Designing a server capable of handling multiple simultaneous clients

## Future Improvements

Potential future improvements include:

* More Redis commands and data types
* Improved command validation and error handling
* Performance benchmarking under concurrent load
* More comprehensive automated testing
* Persistence and data recovery
* Improved connection management

## Disclaimer

This is an educational Redis-inspired implementation built from scratch to explore networking, systems programming, protocol parsing, and event-driven concurrency. It is not intended to be a production replacement for Redis.

## Testing

The server was tested using raw RESP commands sent through `netcat` (`nc`).

### Basic Commands

```text
PING
→ +PONG

SET name Maryum
→ +OK

GET name
→ Maryum
```

### Multiple Commands in a Single TCP Payload

The server was tested with multiple RESP commands sent together in a single connection:

```text
SET name Maryum
GET name
```

Both commands were correctly parsed and executed, demonstrating that the server can handle multiple commands received in the same TCP read.

### Data Types

The following data types were tested successfully:

**Lists**

```text
LPUSH numbers 1 2 3
LRANGE numbers 0 -1
```

Result:

```text
3
2
1
```

**Hashes**

```text
HSET user name Maryum
HGET user name
```

Result:

```text
Maryum
```

### Key Expiration

Key expiration was also tested:

```text
SET session hello
EXPIRE session 5
TTL session
```

The key was correctly removed after its expiration time, and subsequent access returned the appropriate missing-key response.

### Testing Method

Commands were sent as raw RESP messages using:

```bash
printf '*1\r\n$4\r\nPING\r\n' | nc 127.0.0.1 6379
```

The server was also tested with multiple client connections to verify that multiple clients could communicate with the same server and shared in-memory datastore.
