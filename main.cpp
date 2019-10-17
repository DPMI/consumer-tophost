#include "config.h"

#include <caputils/caputils.h>
#include <caputils/packet.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <assert.h>
#include <errno.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/tcp.h>
#include <sqlite3.h>
#include <iostream>
#include <string>

using namespace std;
#include "hosts.h"


/* global state */
static int running = 0;
static int verbose = 0;
static sqlite3* conn;
static sqlite3_stmt* gethosts = NULL;
static sqlite3_stmt* update = NULL;
static sqlite3_stmt* insert = NULL;

static int sampleSize=0;
static hosts* myHosts=0;

static const char* program_name = NULL;
static const char* iface = NULL;


static struct option long_options[]= {
	{"help", no_argument, 0, 'h'},
	{"manic", required_argument, NULL, 's'},
	{"listen", required_argument, 0, 'l'},
	{"port", required_argument, 0, 'p'},

	{0, 0, 0, 0} /* sentinel */
};

void show_usage(const char* program_name){
  printf("%s-%s (libcap_utils-%s)\n", program_name, VERSION,caputils_version(NULL));
	printf("Usage: %s [OPTION].. -<s INTERFACE> SOURCE..\n\n", program_name);
	printf(" Runs in two modes; live (manic) or from file. \n");
	printf(" Live mode; \n");
	printf(" %s [OPTIONS] -s <MANIC> <stream1> ... <streamN> \n",program_name);
	printf(" File mode; \n");
	printf(" %s [OPTIONS] <filename> \n\n", program_name);
	printf(" Options \n");
	printf("  -h, --help                  help (this text)\n");
	printf("  -s, --manic=INTERFACE       MA Interface.<optional>\n");
	printf("  -l, --listen=IP             listen ip [default: all] <optional>\n");
	printf("  -p, --port=PORT             listen port [default: 8081] <optional>\n");
	printf("  -v, --verbose               verbose\n");

}

/* called on SIGINT */
void terminate(int signum){
	if ( running == 0 ){
		/* if it doesn't respond the first time (stuck somewhere) just abort */
		fprintf(stderr, "\rcaught SIGINT again, aborting.\n");
		abort();
	}

	fprintf(stderr, "\rTerminating consumer.\n");
	running = 0;
}

void push(int signum){
	static char buffer[40960];
	char* ptr = buffer;

	string hostlist;
	string counters;

	hosts* tmphost=0;
	hosts* tmphost2=0;
	hosts* tmphost3=0;
	struct timeval theTime2;

	hosts* a=0;
	hosts* b=0;
	hosts* c=0;
	hosts* e=0;
	hosts* tmp=0;

	/* Sort the list */
	int depth=0;
	tmphost=myHosts;
	while(tmphost!=0){
		tmphost=tmphost->getNext();
		depth++;
	}


	while( myHosts!=0 && e != myHosts->getNext()) {
		c = a = myHosts;
		b = a->getNext();
		while(a != e) {
			if(a->count() < b->count()) {
				if(a == myHosts) {
					tmp = b -> getNext();
					b->setNext(a);
					a->setNext(tmp);
					myHosts = b;
					c = b;
				} else {
					tmp = b->getNext();
					b->setNext(a);
					a->setNext(tmp);
					c->setNext(b);
					c = b;
				}
			} else {
				c = a;
				a = a->getNext();
			}
			b = a->getNext();
			if(b == e)
				e = a;
		}
	}

	tmphost=myHosts;
	tmphost2=myHosts;
	tmphost3=myHosts;


	depth=0;
	ptr += sprintf(ptr, "");
	while(tmphost!=0 && depth<10){
		ptr+=sprintf(ptr,"%s \n", tmphost->printMe().c_str());
		tmphost=tmphost->getNext();
		depth++;
	}
	gettimeofday(&theTime2, NULL);

	ptr += sprintf(ptr-1, ""); /* -1 to remove last comma */

	sampleSize=0;
	/*
		if ( ret != SQLITE_DONE ){
		fprintf(stderr, "sqlite3_step[gethosts] returned: %s\n", sqlite3_errmsg(conn));
		return;
		}
	*/
	fprintf(stdout, "%s\n", buffer);

	delete myHosts;
	myHosts=0;

}

int main(int argc, char* argv[]){
	int option_index = 0;
	int op;
	int ret;
	verbose=0;

	/* defaults */
	const char* ip = "0.0.0.0";
	int port = 8081;
	const char* manic = NULL;

	const char* separator = strrchr(argv[0], '/');
	if ( separator ){
	  program_name = separator + 1;
	} else {
	  program_name = argv[0];
	}
	

	struct filter filter;
	if ( filter_from_argv(&argc, argv, &filter) != 0 ){
		return 0; /* error already shown */
	}

	/* parse arguments */
	while ( (op = getopt_long(argc, argv, "vhs:l:p:", long_options, &option_index)) != -1 )
		switch (op){
		case 'v': 
		  verbose=1;
		  break;

		case 0: /* longopt with flag set */
			break;

		case 's':
			manic = optarg;
			break;

		case 'l':
			ip = optarg;
			break;

		case 'p':
			port = atoi(optarg);
			break;

		case 'h':
			show_usage(argv[0]);
			exit(0);

		default:
			assert(0 && "declared but unhandled argument");
			break;
		}

	/* missing source */
	if ( optind == argc ){
		fprintf(stderr, "No source specified, see --help for usage.\n");
		exit(1);
	}

	/* missing manic */	
	if ( !manic  && verbose ){
	  fprintf(stderr, "Missing MAnic, see --help for usage.\n");
	} 
	


	/* open primary source */
	struct stream* stream;
	stream_addr_t src;
	
	
	if(verbose && filter.index>0) {
	  filter_print(&filter, stderr, 0);
	}

	fprintf(stderr, "opening streams\n");
	if (!manic) {

	  stream_addr_aton(&src, argv[optind++], STREAM_ADDR_GUESS, STREAM_ADDR_LOCAL);
	  
	  if( (ret=stream_open(&stream, &src, manic, 0)) != 0 ) {
	    fprintf(stderr, "stream_open failed(%s) with code 0x%08X: %s\n", src, ret, caputils_error_string(ret));
	    exit(1);
	  }	  
	} else {
	  if( (ret=stream_from_getopt(&stream, argv, optind, argc,manic,"-",program_name,0)) != 0 ) {
	    fprintf(stderr, "stream_open failed(%s) with code 0x%08X: %s\n", argv,ret, caputils_error_string(ret));
	    exit(1);
	  }
	}

	/* open secondary sources */
	for ( int i = optind; i < argc; i++ ){
		stream_addr_aton(&src, argv[i], STREAM_ADDR_GUESS, STREAM_ADDR_LOCAL);

		if( (ret=stream_add(stream, &src)) != 0 ) {
			fprintf(stderr, "stream_open failed with code 0x%08X: %s\n", ret, caputils_error_string(ret));
			exit(1);
		}
	}

	if(verbose){
	  fprintf(stderr,"Streams opened.\n");
	}
	  /* initialize sqlite database */
	if ( (ret=sqlite3_open("host.db", &conn)) != SQLITE_OK ){
		fprintf(stderr, "sqlite3_open() returned %d: %s\n", ret, sqlite3_errmsg(conn));
		exit(1);
	}
	if ( (ret=sqlite3_exec(conn, "CREATE TABLE IF NOT EXISTS hosts (host TEXT PRIMARY KEY, hit INT default 1)", NULL, NULL, NULL)) ){
		fprintf(stderr, "sqlite3_exec() returned %d: %s\n", ret, sqlite3_errmsg(conn));
		exit(1);
	}
	{
		const char* q;

		q = "SELECT host, hit FROM hosts ORDER BY hit DESC LIMIT 10";
		if ( (ret = sqlite3_prepare_v2(conn, q, (int)(strlen(q)+1), &gethosts, NULL)) != SQLITE_OK ){
			fprintf(stderr, "sqlite3_prepare_v2 returned %d: %s\n", ret, sqlite3_errmsg(conn));
			return ret;
		}

		q = "UPDATE hosts SET hit = hit+1 WHERE host = ?";
		if ( (ret = sqlite3_prepare_v2(conn, q, (int)(strlen(q)+1), &update, NULL)) != SQLITE_OK ){
			fprintf(stderr, "sqlite3_prepare_v2 returned %d: %s\n", ret, sqlite3_errmsg(conn));
			return ret;
		}

		q = "INSERT INTO hosts (host) VALUES (?)";
		if ( (ret = sqlite3_prepare_v2(conn, q, (int)(strlen(q)+1), &insert, NULL)) != SQLITE_OK ){
			fprintf(stderr, "sqlite3_prepare_v2 returned %d: %s\n", ret, sqlite3_errmsg(conn));
			return ret;
		}
	}


	/* setup signal handler to terminate the application */
	signal(SIGINT, terminate);

	/* timer */
	if (manic){
	  if(verbose){
	    fprintf(stderr," Reading from nic \n");
	  }
	  struct itimerval difftime;
	  difftime.it_interval.tv_sec = 10;
	  difftime.it_interval.tv_usec = 0;
	  difftime.it_value.tv_sec = 10;
	  difftime.it_value.tv_usec = 0;
	  signal(SIGALRM, push);
	  setitimer(ITIMER_REAL, &difftime, NULL);
	} else {
	  if(verbose){
	    fprintf(stderr," Reading from file \n");
	  }
	}

	/* ready to run */
	running = 1;

	cap_head* cp;
	long stret; /* using a separate so it won't be overwritten */
	char strstream[500];
	char stripsrc[INET_ADDRSTRLEN];
	char stripdst[INET_ADDRSTRLEN];
	qd_real pkt1;
	string query;
	hosts* tmphost;
	hosts* tmphost2;
	int depth;
	int maxDepth=0;
	
	

	while ( running ){
		struct timeval timeout = {1,0};
		sampleSize++;
		stret = stream_read(stream, &cp, NULL, &timeout);
		if ( stret == EAGAIN || stret == EINTR ){
		  continue;
		} else if ( stret != 0 ){
		  if(manic){
		    fprintf(stderr, "ERRNO = %s .\n", strerror(stret));
		  }
		  break;
		}
		
		const struct ip* ip = find_ipv4_header(cp->ethhdr,0);
		if ( !ip ){
			continue;
		}
		inet_ntop(AF_INET, &(ip->ip_src), stripsrc, INET_ADDRSTRLEN);
		inet_ntop(AF_INET, &(ip->ip_dst), stripdst, INET_ADDRSTRLEN);
		pkt1=(qd_real)(double)cp->ts.tv_sec+(qd_real)(double)(cp->ts.tv_psec/PICODIVIDER);

		//		snprintf(strstream,500,"%s - %d - %s",stripsrc,ip->ip_p,stripdst);
		snprintf(strstream,500,"%s",stripsrc);

		query.assign(strstream);

		if(myHosts==0){
			myHosts=new hosts(query,pkt1, ip->ip_len);
		} else {

			tmphost=myHosts;
			tmphost2=myHosts;
			depth=0;
			while(tmphost!=0 && tmphost->match(query)<1){
				tmphost2=tmphost;
				tmphost=tmphost->getNext();
				depth++;
			}

			if(tmphost==0){
				// No match
				tmphost=new hosts(query,pkt1,ip->ip_len);
				tmphost->setNext(myHosts);
				myHosts=tmphost;
				//      tmphost2->setNext(tmphost);
				if(depth>maxDepth){
					maxDepth=depth;

				}
			} else {
				tmphost->insert(query,pkt1, ip->ip_len);
			}




		}

	}

	if(manic && verbose){
	  if ( !(stret == 0 || stret == EAGAIN) ){
	    fprintf(stderr, "stream_read() returned 0x%08x: %s\n", ret, caputils_error_string(ret));
	  }
	}

	/* print when leaving */
	push(1);

	sqlite3_finalize(gethosts);
	sqlite3_finalize(insert);
	sqlite3_finalize(update);
	sqlite3_close(conn);

	/* terminate the server */
	stream_close(stream);

	return 0;
}
