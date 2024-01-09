@echo off
echo [Compiling] Daemon
g++ interactor\daemon.cpp -lws2_32 -o daemon
echo [Compiled] Daemon
echo ___
echo [Compiling] Controller
g++ interactor\controller.cpp -lws2_32 -o controller
echo ___
