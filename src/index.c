
#include "libmosaik2.h"
#include "index.h"

int mosaik2_index(char *mosaik2_database_name,  uint32_t max_tiler_processes, uint32_t max_load_avg) {

	signal(SIGINT, signal_handler);
	
	max_tiler_processes = get_max_tiler_processes(max_tiler_processes);
	max_load_avg = get_max_load_avg(max_load_avg);

	mosaik2_context ctx;
	init_mosaik2_context(&ctx);
	ctx.max_tiler_processes = max_tiler_processes;
	ctx.max_load_avg = max_load_avg;

	struct mosaik2_database_struct md;
	init_mosaik2_database_struct(&md, mosaik2_database_name);
	read_database_id(&md);
	
	check_thumbs_db(&md);

	check_pid_file(&md);
	write_pid_file(&md);

	process_input_data(&ctx, &md);

	remove_pid_file(&md);

	return 0;
}

uint32_t get_max_tiler_processes(uint32_t max_tiler_processes) {
	if(max_tiler_processes < 1)
		return sysconf(_SC_NPROCESSORS_ONLN);
	return max_tiler_processes;
}

uint32_t get_max_load_avg(uint32_t max_load_avg) {
	if(max_load_avg<1)
		return sysconf(_SC_NPROCESSORS_ONLN);
	return max_load_avg;
}

void check_pid_file(struct mosaik2_database_struct *md) {
	if( access(md->pid_filename, F_OK) == 0) {
		fprintf(stderr, "The pid file (%s) exists, either an indexing process is running or the program did not exit correctly. In the first case, the pid file can be deleted.\n", md->pid_filename);
		exit(EXIT_FAILURE);
	}
}

void write_pid_file(struct mosaik2_database_struct *md) {
	FILE *f =	fopen(md->pid_filename,"w");
	if( f == NULL ) { fprintf( stderr, "could not create mosaik2 database file (%s)\n", md->pid_filename); exit(EXIT_FAILURE); }
	fprintf(f, "%i", getpid());
	if( fclose( f ) != 0 ) {
		fprintf( stderr, "could not close mosaik2 database file (%s)\n", md->pid_filename); exit(EXIT_FAILURE);
	}
}

void remove_pid_file(struct mosaik2_database_struct *md) {
	 if((remove(md->pid_filename)) < 0) {
      fprintf(stderr, "could not remove active pid file (%s)", md->pid_filename);
      exit(EXIT_FAILURE);
   }
}

void process_input_data(mosaik2_context *ctx, struct mosaik2_database_struct *md) {

	mosaik2_indextask task_list[ctx->max_tiler_processes];

	size_t linebuffer_len = 1000;
	char *linebuffer = malloc(linebuffer_len);

	if(linebuffer == NULL) {
		fprintf(stderr, "cannot allocate memory für linebuffer\n");
		exit(EXIT_FAILURE);
	}
	fprintf(stderr, "read line BUFSIZE:%i\n", BUFSIZ);
	ssize_t nread;
	ssize_t i = 0;
		if(errno) {
			fprintf(stderr,"error %i while reading lines from stdin (read:%li bytes, line:%s). %s\n", errno, nread, linebuffer, strerror(errno));
		}errno=0;
	while((nread = getline(&linebuffer, &linebuffer_len, stdin)) != -1 ) {
		if(errno) {
			fprintf(stderr,"error %i while reading lines from stdin (read:%li bytes, line:%s). %s\n", errno, nread, linebuffer, strerror(errno));
			free(linebuffer);
			exit(EXIT_FAILURE);
		}
		
		process_next_line(ctx, md, linebuffer, i++);
		// do something 
	}
	free(linebuffer);
}

void process_next_line(mosaik2_context *ctx, struct mosaik2_database_struct *md, char *line, ssize_t i) {
 
	if(ctx->debug)
		fprintf(stdout, "process stdin lines progress => idx:%li %s\n", i, line);

  if(ctx->exiting)
		fprintf(stdout, "stdin is not resumed, EXITing because of SIGTERM.");

	mosaik2_indextask task;

	size_t tabstops[] = {0,0};
	tabstops[0] = strcspn(line, "\t");
	tabstops[1] = strcspn(line+tabstops[0]+1, "\t")+tabstops[0];
	if(tabstops[0] == 0) {
		errx(EINVAL, "filename is empty");
	}
	if(tabstops[0]>=sizeof(task.filename)) {
		errx(EINVAL, "ups, this filename is to long. sorry you have to remove or rename anything, that exceeds %li characters\n", sizeof(task.filename));
	}
	if(tabstops[1]<=tabstops[0]) {
		errx(EINVAL, "invalid data format, have a look at README.filelist");
	}

	strncpy(task.filename, line, tabstops[0]);
	task.filename[tabstops[0]] = '\0';

	task.filesize = strtol(line+tabstops[0]+1, NULL, 10);
	if(errno)
		errx(errno, "error converting file size");

	task.lastmodified = strtoll(line+tabstops[1]+1, NULL, 10);
	if(errno)
		errx(errno, "error converting input unix timestamp");

	fopen(task.filename, "r");
	//process the data in serial and compare duration to old variant	
	return;
}

void signal_handler(int signal) {
	if(signal == SIGINT) {
		printf("SIGINT catched, do not exit\n");
	}
}

