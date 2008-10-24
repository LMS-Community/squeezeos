

all: evtest memtool jivectl testir testdisplay draw IQCtest

jivectl: jivectl.c
	${CC} ${CFLAGS} jivectl.c ${LDFLAGS} -o $@

testir: testir.c
	${CC} ${CFLAGS} testir.c ${LDFLAGS} -o $@

testdisplay: testdisplay.c
	${CC} ${CFLAGS} testdisplay.c ${LDFLAGS} -o $@ -l jpeg

draw: draw.c
	${CC} ${CFLAGS} draw.c ${LDFLAGS} -o $@ -l jpeg

IQCtest: IQCtest.c
	${CC} ${CFLAGS} IQCtest.c ${LDFLAGS} -o $@

evtest: evtest.c
	${CC} ${CFLAGS} evtest.c ${LDFLAGS} -o $@

memtool: memtool.c
	${CC} ${CFLAGS} memtool.c ${LDFLAGS} -o $@

install:
	install evtest ${PREFIX}/bin
	install memtool ${PREFIX}/bin
	install jivectl ${PREFIX}/bin
	install testir ${PREFIX}/bin
	install testdisplay ${PREFIX}/bin
	install draw ${PREFIX}/bin
	install IQCtest ${PREFIX}/bin

