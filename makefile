all: 
	gcc -o patient1output patient1.c -lsocket -lnsl -lresolv
	gcc -o patient2output patient2.c -lsocket -lnsl -lresolv
	gcc -o healthcenterserveroutput healthcenterserver.c -lsocket -lnsl -lresolv
	gcc -o doctoroutput doctor.c -lsocket -lnsl -lresolv