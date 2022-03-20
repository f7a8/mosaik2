
#include "libmosaik2.h"

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

int mosaik2_index(char *mosaik2_database_name,  uint32_t max_tiler_processes, uint32_t max_load_avg) {
	
	max_tiler_processes = get_max_tiler_processes(max_tiler_processes);
	max_load_avg = get_max_load_avg(max_load_avg);


	struct mosaik2_database_struct md;
	init_mosaik2_database_struct(&md, mosaik2_database_name);
	read_database_id(&md);
	
	check_thumbs_db(&md);

	check_pid_file(&md);
	write_pid_file(&md);

	size_t linebuffer_len = 1024;
	char *linebuffer = malloc(linebuffer_len);
	if(linebuffer == NULL) {
		fprintf(stderr, "cannot allocate memory für linebuffer\n");
		exit(EXIT_FAILURE);
	}
	fprintf(stderr, "read line\n");
	ssize_t nread;
	while((nread = getline(&linebuffer, &linebuffer_len, stdin)) != -1 ) {
		if(errno) {
			fprintf(stderr,"error while reading lines from stdin. %s\n", strerror(errno));
			free(linebuffer);
			exit(EXIT_FAILURE);
		}
		// do something 
	}
	free(linebuffer);
	remove_pid_file(&md);

	return 0;
}

