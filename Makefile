CC=arm-4412-linux-gnueabihf-gcc

objects=mpe4r.c configparse.c

m4pr:
	$(CC) -Wall $(objects) -o mpe4r $(shell pkg-config --cflags --libs gstreamer-rtsp-server-1.0)

clean:
	rm -f *.o mpe4r
