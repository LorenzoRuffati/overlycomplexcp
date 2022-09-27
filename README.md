# OCP, OverlycomplicatedCP
This project reimplements the basic functionality of copying a
file from one location into another, this is achieved by launching
the command in two separates processes which will communicate over one
of two IPC methods to achieve the transfer of the required data

The two processes will act as either a "sender", tasked with reading data
from the source file and sending it to the other process, which will act
as a "receiver" to write the data onto the designated destination file

The two methods of IPC will be:
+ Pipes (FIFO)
+ Shared memory

## Structure of the project
The project will result in a single binary which will contain the necessary 
information for both roles and for both IPC methods

The src folder will be divided into a folder for the FIFO methods (`src/pipe`), a folder
for the shared memory methods (`src/shm`) and a folder for shared utilities (`src/shared`)

## Usage
Either process (sender or receiver) can be launched first, the processes will coordinate
to only start the copy process when they all are ready.

Pairing of sender and receiver is achieved through the `pass` parameter, which should
be a unique identifier for the operation desired, this allows running multiple copy operations
between different pairs of senders and receivers at the same time.  
The password must not begin with a `/`

The `file` option is interpreted as the input file when passed to the sender or as the
output destination when passed to the receiver

The `role` option can be either `s` for sender or `r` for receiver

The `method` option can be either `p` for PIPE (FIFO) based ICP or `s` for shared memory

Options:
```
--pass <pass>, -p <pass>, The unique ID for the copy/paste pair of executions
--file <name>, -f <name>, The program is going to write the received data into the file at <name>
--method <method>, -m <method>, The program is going to use the specified method to execute the copy/paste pair of executions
--role <role>, -r <role>, <role> can be either sender or receiver and specifies how the program is going to act
```
Example to copy file "test.txt" to destination "copy" using pipes:
```
ocp -r send -p example123 -m p -f test.txt
ocp -r receive -p example123 -m p -f copy
```

# Building

We used the meson build system, installation instructions for which can be found at [the official meson website](https://mesonbuild.com/Quick-guide.html)

Once meson is installed running the `setup.sh` script will create the build directory and executing the `meson compile ocp` in this new directory (`builddir`) will compile and link
the program creating the `ocp` binary

## Dependencies
On top of meson and gcc the following dependencies are required:
+ xxd
+ openssl
+ libssl-dev

On ubuntu these can be installed with: `sudo apt install xxd openssl libssl-dev`

## Warning
The program **must** be built with meson as it generates the header file for the help 
message, if not using meson then an equivalent header file containing the `___src_help_str_txt_len`
and ` ___src_help_str_txt` variables must be generated