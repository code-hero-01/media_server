# Media Server

A self-hosted multithreaded http media streaming server written in C++.

- Streams text, image, audio, and video files from the specified directory over HTTP
- Supports HTTP Range request for video/audio seeking
- Directory Management - upload files, create new folders, delete files/folders, rename files/folders
- Supports HTTP GET and POST requests
- Uses POSIX TCP sockets
- Multithreaded architecture using thread pool

https://github.com/user-attachments/assets/f607e666-0769-4bdf-985f-5610971f5c21

## Build and Installation   

1. **Clone the repository**:   
   ```bash
   git clone https://github.com/code-hero-01/media_server
   cd media_server
   ```   

2. **Create a build directory**:  
   ```bash
   mkdir build
   ```

3. **Configure the project**:  
   ```bash
   cmake .
   ```

4. **Compile the source code**:    
 ```bash
   cmake --build build
   ```   

## Running the Application  

Once the compilation finishes, you can run the executable from inside your `build` directory:  

  ```bash
   ./build/media_server <optional: root_directory (default: ./media>)
  ```   

## Benchmarking and Testing  
while server is running in another terminal:
```bash
   cd test
   ./benchmark <number of clients> <number of requests per client>  <optional: server ip (default: 127.0.0.1 (localhost))>
```

## Future Plans  

- Replace threadpool with event driver I/O model (polling) to improve scalibility for thousands of concurrent connections  
- Allow uploading folders to the server
- Allow clients to download entire folder as a zip file
