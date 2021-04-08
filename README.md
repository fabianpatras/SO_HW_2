# Assignment 2 SO: Stdio Library

This assignment was about reimplemening some of the basic functions that come with the `stdio.h`.  In this implementation buffering is present.

I really enjoyed doing this. I saw a bit of the the dirty stuff the is underneath the functions that we use everyday just to open and read from a file. There was a lot to learn.

## General approach

* The `SO_FILE` mainly has a buffer and a lot of fields that maintain informationa about the state of the buffer, keep track whether or not an error occured or the `EOF` was reached, the struct is associated with a file or a process.
* The functions are well documented, but the main points are:
    * `so_fputc` writes to the buffer, and the buffer is flushed into the `file descriptor` only if it is full
    * `so_fgetc` tries to read the whole buffer so that a next call would read from the buffer and not make a `syscall`
    * `so_fwrite` is implemented with `so_fputc`
    * `so_fread` is implemented with `so_fgetc`
    * There are `3 variables` that keep track of the internal state of the buffer: `one` that knows what the last operaion was and `two` pointers: `_left_ptr` and `_right_ptr` that are the bounds of the buffer in which you have real data
    * `so_ftell` is implemented with `so_fseek`

## Difficulties
This assignment was fun to work on, but I faced some difficulties along the way:
1) The buffer was a pain to work with. Reading has to flush the data that was written and writing after a read has to clear the buffer.
2) Also keeping track of the data read into the buffer that never got to be accessed with a `so_fgetc` or `so_fread` had to be done in order not to ruin the actual location of the `so_fseek`/`so_ftell`

## How to use
* include `so_stdio.h` in your project and use the functions available
* *OR*
* use `make` and that will create a shared library `libso_stdio.so`

