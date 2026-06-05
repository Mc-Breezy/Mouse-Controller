CFLAGS = $(shell pkg-config --cflags gtk4 libevdev glib-2.0)
LIBS   = $(shell pkg-config --libs gtk4 libevdev glib-2.0)

mouse_controller: input_detection.o device_page.o editor_page.o remap_row.o remap.o utility.o main.o
	gcc $(CFLAGS) *.o $(LIBS) -o mouse_controller
main.o: main.c
	gcc $(CFLAGS) -c main.c -o main.o $(LIBS)
input_detection.o: input_detection.h input_detection.c
	gcc $(CFLAGS) -c input_detection.c -o input_detection.o $(LIBS)
device_page.o: device_page.h device_page.c
	gcc $(CFLAGS) -c device_page.c -o device_page.o $(LIBS)
editor_page.o: editor_page.h editor_page.c
	gcc $(CFLAGS) -c editor_page.c -o editor_page.o $(LIBS)
remap_row.o: remap_row.h remap_row.c
	gcc $(CFLAGS) -c remap_row.c -o remap_row.o $(LIBS)
remap.o: remap.h remap.c
	gcc $(CFLAGS) -c remap.c -o remap.o $(LIBS)
utility.o: utility.h utility.c
	gcc $(CFLAGS) -c utility.c -o utility.o $(LIBS)
clean:
	rm *.o mouse_controller
