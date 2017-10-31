# Usage

## ThreadPool
```bash
cd ./rodenComputeLayer/ThreadPool
make
./ThreadPool
```

ThreadPool exposes a framework that synchronizes a thread pool.
Currently, it creates a pool of NUMTHREADS size, and each thread reads data from a locked queue, processes the data (API call), waits for all threads to finish, then waits 1 second (PER API REQUIREMENTS) then repeats the action until the locked queue is empty

## VideoProcessor
```bash
cd ./rodenComputeLayer/VideoProcessor
make
./VideoProcessor
```

VideoProcessor exposes a single method, `void splitVideo(String filename, String outputDirectory, int precision)`, which takes in a string path to a video file, an output directory to write frames to, and a precision (1fps)\*precision. There is a test method in here that runs in the binary. You can see the output in the `/VideoProcessor/tmp/` directory. pro tip: `make clean` will delete all the messy files
