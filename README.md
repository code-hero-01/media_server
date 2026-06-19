# Media Server

A self-hosted multithraded http server written in C++.

- Serves text, image, music and video files from specified directory over HTTP
- Uses POSIX TCP sockets
- Support seeking in media files (supports http range requests)
- Multithreaded

## Build and Installation   

1. **Clone the repository**:   
   ```bash
   git clone https://github.com/code-hero-01/media_server
   cd media_server
   ```   

2. **Create a build directory**:  
   ```bash
   mkdir build && cd build
   ```

3. **Configure the project**:  
   ```bash
   cmake ..
   ```

4. **Compile the source code**:    
 ```bash
   cmake --build .
   ```   

## Running the Application  

Once the compilation finishes, you can run the executable from inside your `build` directory:  

  ```bash
  ./media_server
  ```   

## Benchmarking and Testing  
```cd test
   ./benchmark <number of clients> <number of requests per client>
   ```