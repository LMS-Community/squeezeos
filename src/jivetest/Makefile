

all: jivectl evtest testir testdisplay draw IQCtest

padtest: test.c
	${CC} ${CFLAGS} test.c ${LDFLAGS} -o $@

padtest2: test2.c
	${CC} ${CFLAGS} test2.c ${LDFLAGS} -o $@

padtest3: test3.c
	${CC} ${CFLAGS} test3.c ${LDFLAGS} -o $@

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

padtest-2.0: test_v2.0.c
	${CC} ${CFLAGS} test_v2.0.c ${LDFLAGS} -o $@

install:
#	install padtest ${PREFIX}/bin
#	install padtest2 ${PREFIX}/bin
#	install padtest3 ${PREFIX}/bin
#	install padtest-2.0 ${PREFIX}/bin
	install jivectl ${PREFIX}/bin
	install evtest ${PREFIX}/bin
	install testir ${PREFIX}/bin
	install testdisplay ${PREFIX}/bin
	install draw ${PREFIX}/bin
	install IQCtest ${PREFIX}/bin
	

