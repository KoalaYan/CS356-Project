# OperatingSystem Project2: OOM Killer
This is CS356 Operating System Course Design Project 2.\
Created by KoalaYan(P.S Yan).\
## Basic Type
If you want to test the basic type of oom killer, just follow the filePath.txt to replace the files in /kernel/goldfish folder with the files in my /oom_basic/kernelFile folder. After you complete the compilation configuration, execute command `make -j4` in kernel file. Then you can start your test.
##   Daemon Type
If you want to test the daemon type of oom killer, you shoud also follow the filePath.txt to replace the files in /kernel/goldfish folder with the files in my /oom_daemon/kernelFile folder. But then you need to compile the daemon executable file in /daemon/jni folder, which may also require to change the path in MakeFile. And complete the compilation configuration, you can execute command `make -j4` in kernel file. Then you can start your test.