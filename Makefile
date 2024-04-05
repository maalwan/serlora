wio: main.c wioe.h wioe.c ser.h ser.c term_interface.h term_interface.c
	gcc -o wio main.c wioe.c ser.c term_interface.c

clean :
	-rm wio 
