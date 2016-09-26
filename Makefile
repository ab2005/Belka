OPENCLDIR=$(ANDROID_NDK)/sources/opencl

LDFLAGS=-lm -g -framework OpenCL -framework GLUT -framework OpenGL
CFLAGS=-O2 -g -c -Wall -I. -Isrc -Iopencl -DRCCB_PLUGIN -fPIC -D__CL_ENABLE_EXCEPTIONS -DNOANDROID -Wdeprecated-declarations -std=c++0x

RCCB_OBJ=main.o Pipeline.o image.o debug.o Window.o
RCCB_UNIX_OBJ=$(patsubst %,bin/%,$(RCCB_OBJ))

all: bin/benchmarks

clean:
	@/bin/rm -rf bin/* || echo "Cleaned bin/ folder"
	@rm -rf out/outCl.bmp

bin/benchmarks: $(RCCB_UNIX_OBJ)
	$(CXX) $(RCCB_UNIX_OBJ) $(LDFLAGS) -o $@

bin/%.o: src/%.c*
	$(CXX) $(CFLAGS) $< -o $@

bin/main.o: src/main.cpp
	$(CXX) $(CFLAGS) $< -o $@

bin/Pipeline.o: src/Pipeline.cpp
	$(CXX) $(CFLAGS) $< -o $@

png:
	ls out/*.bmp -1 |sed 's#out/\(.*\)\.bmp#\1#g' | xargs -i@ convert out/@.bmp out/@.png

jpg:
	ls out/*.bmp -1 |sed 's#out/\(.*\)\.bmp#\1#g' | xargs -i@ convert out/@.bmp out/@.jpg

t: 	bin/benchmarks
	bin/benchmarks \
    ~/dev/rccb-master/rccb/golden/CP_D65_5lux_AR1331r2_130611_bc_lsc.raw \
	-os out/outCl.bmp \
	-q -fd 12 -cc 1.8958 -0.5348 -0.361 -1.014 2.437 -0.423 0.1504 -1.0246 1.8742 -ce 2 -lr 1 -la 0.06 -cr 0.05 -ca 0.500 -se 0 -de 1 -sa 0.06 -sr 0.3 -pl 1 -pm 0.05 0.05 -pw 0.2778 0.13225 -bm 194.5090 184.6404 188.6242 194.5090 -po 0.12 -cs 3 63 0 -ptga 0.006 -ptgr 0.07 -stga 0.016 -stgr 0.00 -stgs 0.25 -ck 2.75 3.50 -wm 3.7670 3.5880 3.8749
	open out/outCl.bmp

test: bin/benchmarks
	bin/benchmarks -d -ce 3 \
    test/C_GTID30_SS_130lux_67ms_8Mp_1p4.raw \
	-os out/outCl.bmp \
	-pattern RCCB -cc 1.8958 -0.5348 -0.361 -1.014 2.437 -0.423 0.1504 -1.0246 1.8742 \
	-ww 708 1106 904 1282 \
	-bw 1704 1096 1808 1386 \
	-sa 0 -sr 0 \
	-la 0.04 -lr 1.0 \
	-ca 0.240 -cr 0.00 \
	-ia 0.003 -ir 0.02 \
	-pm 0.1 0.1 -ds 0.05 -ce 3
	open out/outCl.bmp

testv: bin/benchmarks
	bin/benchmarks -d -ce 3 \
    test/video.raw \
    -width 4016 -height 3016 \
    -os out/outCl.bmp \
    -cc 2.6527 -1.5577 -0.095 -1.5273 2.9438 -0.4165 0.2946 -2.1443 2.8497 \
    -bm 45 45 45 45
	open out/outCl.bmp

tv: bin/benchmarks
	bin/benchmarks test1/video.raw \
    -width 1920 -height 1080 \
    -os out/outCl.bmp \
    -q -fd 10 -cc 1.8958 -0.5348 -0.361 -1.014 2.437 -0.423 0.1504 -1.0246 1.8742 -ce 2 -lr 1 -la 0.06 -cr 0.05 -ca 0.500 -se 0 -de 1 -sa 0.06 -sr 0.3 -pl 1 -pm 0.05 0.05 -pw 0.2778 0.13225 -bt -0.05 -bm 20 29 29 20  -po 0.12 -cs 3 63 0 -ptga 0.006 -ptgr 0.07 -stga 0.016 -stgr 0.00 -stgs 0.25 -ck 2.75 3.50 -wm 3.7 2.0 3.8
	open out/outCl.bmp

#    -q -fd 10 -cc 1.8958 -0.5348 -0.361 -1.014 2.437 -0.423 0.1504 -1.0246 1.8742 -ce 2 -lr 1 -la 0.06 -cr 0.05 -ca 0.500 -se 0 -de 1 -sa 0.06 -sr 0.3 -pl 1 -pm 0.05 0.05 -pw 0.2778 0.13225 -bw 1255 818 1297 854  -po 0.12 -cs 3 63 0 -ptga 0.006 -ptgr 0.07 -stga 0.016 -stgr 0.00 -stgs 0.25 -ck 2.75 3.50 -ww 633 778 709 856

5: bin/benchmarks
	bin/benchmarks -os out/outCl.bmp \
    ../rccb-master/rccb/golden/CP_D65_5lux_AR1331r2_130611_bc_lsc.raw \
    -q -fd 12 -cc 1.8958 -0.5348 -0.361 -1.014 2.437 -0.423 0.1504 -1.0246 1.8742 -ce 2 -lr 1 -la 0.06 -cr 0.05 -ca 0.500 -se 0 -de 1 -sa 0.06 -sr 0.3 -pl 1 -pm 0.05 0.05 -pw 0.2778 0.13225 -bm 194.5090 184.6404 188.6242 194.5090 -po 0.12 -cs 3 63 0 -ptga 0.006 -ptgr 0.07 -stga 0.016 -stgr 0.00 -stgs 0.25 -ck 2.75 3.50 -wm 3.7670 3.5880 3.8749
	open out/outCl.bmp

ndk-build: bin/benchmarks
	export NDK_PROJECT_PATH=.;ndk-build

assets-install: test/*
	adb shell mkdir -p /sdcard/rccb
	adb push test /sdcard/rccb

adb-install:
	adb shell mkdir -p /data/local/tmp/rccb/kernels
	adb push libs/armeabi-v7a/benchmarks /data/local/tmp/rccb
	adb push kernels /data/local/tmp/rccb/kernels

adb-run:
	adb shell "sh /sdcard/rccb/benchmarks.sh"
	adb pull /data/local/tmp/rccb/outCl_android.bmp
	open outCl_android.bmp

d65: bin/benchmarks
	bin/benchmarks \
	test/RCCB_D65_200lux_Res_AR0833_120814_bc_lsc.raw \
	-os out/d65.bmp \
	-fd 12 -cc 1.8300 -0.7135 -0.1165 -1.1958 3.7949 -1.5991 0.0469 -1.0228 1.9759 -lr 1 -la 0.04 -cr 0 -ca 0.240 -se 6 -de 1 -sa 0.02 -sr 0.1 -pl 0 -pm 0.1 0.1 -ba 0.01 -bt 0.01 -bam 0.1 -po 0.03 -cs 1 63 -1
	open out/d65.bmp
