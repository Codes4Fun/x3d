#g++ -g -I/usr/include -IsixenseSDK_linux_OSX/include -L/usr/lib64 -LsixenseSDK_linux_OSX/lib/linux_x64/release -lX11 -lXi -lGL -lGLU -lm -lsixense_x64 -lstdc++ -o phasetest Matrix.cpp Vector.cpp Hydra.cpp phasetest.cpp XWindow.cpp

g++ -g -Wl,--unresolved-symbols=ignore-in-shared-libs -I/usr/include -IsixenseSDK_linux_OSX/include -LsixenseSDK_linux_OSX/lib/linux_x64/release -lX11 -lXi -lGL -lGLU -lm -lsixense_x64 -o phasetest -DUSE_HYDRA Matrix.cpp Vector.cpp Hydra.cpp phasetest.cpp XWindow.cpp

#g++ -g -I/usr/include -IsixenseSDK_linux_OSX/include -lX11 -lXi -lGL -lGLU -lm -o phasetest Matrix.cpp Vector.cpp Hydra.cpp phasetest.cpp XWindow.cpp


