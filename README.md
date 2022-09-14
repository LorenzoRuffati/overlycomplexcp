+ Pipes (FIFO)
+ Shared memory

One binary
```
- README.md
- src
    |- main.c
    |- helpers/
    |- reader/
    |- writer/
- test
```

+ Reader: reads the original file and sends it through the IPC
+ Writer: reads from the IPC and writes to disk

Arguments:
+ reader/writer
+ name of the file
+ method
+ ""password"" 

FIFO:
+ Create a file in /tmp
+ Wait until reader is attached
+ Write
...



Options:
```
--password <pass>, -p <pass>, The unique ID for the copy/paste pair of executions
--file-name <name>, -f <name>, The program is going to write the received data into the file at <name>
--method <method>, -m <method>, The program is going to use the specified method to execute the copy/paste pair of executions
--role <role>, -r <role>, <role> can be either sender or receiver and specifies how the program is going to act
```
Example to copy file "test.txt" to destination "copy" using pipes:
```
ocp -r send -p example123 -m p -f test.txt
ocp -r receive -p example123 -m p -f copy
```