# Media Server

A self-hosted multithraded http media streaming server written in C++.

- Streams text, image, music and video files from the specified directory over HTTP
- Upload files, create new folders, delete files, rename files
- Supports HTTP GET and POST requests
- Uses POSIX TCP sockets
- Multithreaded architecture using thread pool

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
```bash
   cd test
   ./benchmark <number of clients> <number of requests per client>
```

## Future Plans  

- Make the server polling instead of thread pool to dramatically increase client throughput  
- Allow uploading folders to the server
- Allow clients to download entire folder as a zip file