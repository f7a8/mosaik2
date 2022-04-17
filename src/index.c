
#include "libmosaik2.h"

  #include <sys/types.h>
       #include <sys/wait.h>

struct mosaik2_parser_input_data_column_struct {
	char name[20];
	int max_len;
	int current_len;
	int data_pos; // beginning of data position 
								// its int, because it marks the position in the buffer before copying it
								// wont exceed BUFSIZ
	int old_data_pos; 
	int split_pos; // position of split sign, data end 1 sign before
	char split_sign; // by which sign this colum wants to be splitted
	char data[1024]; // intermediate or finished copy
	int data_offset; // pointer in copy, where the data ends
	int finished; // if 1 the data does not need to appended
};
typedef struct mosaik2_parser_input_data_column_struct mosaik2_parser_input_data_column;

uint32_t get_max_tiler_processes(uint32_t max_tiler_processes);
uint32_t get_max_load_avg(uint32_t max_load_avg);
void check_pid_file(struct mosaik2_database_struct *md);
void write_pid_file(struct mosaik2_database_struct *md);
void remove_pid_file(struct mosaik2_database_struct *md);
void parse_input_data(mosaik2_context *ctx, struct mosaik2_database_struct *md);
void process_next_line(mosaik2_context *ctx, struct mosaik2_database_struct *md, char *line, ssize_t i);
//void signal_handler(int signal);
void mosaik2_index_add_tiler_pid(mosaik2_context *, pid_t);
void mosaik2_index_clean_tiler_pids(mosaik2_context *);
void print_usage(char *);
void print_col(mosaik2_parser_input_data_column[], char *key);
void print_col_i(mosaik2_parser_input_data_column[], int, char *key);




void /* Examine a wait() status using the W* macros */
printWaitStatus(const char *msg, int status)
{
if (msg != NULL)
printf("%s", msg);
if (WIFEXITED(status)) {
printf("child exited, status=%d\n", WEXITSTATUS(status));
} else if (WIFSIGNALED(status)) {
printf("child killed by signal %d (%s)",
WTERMSIG(status), strsignal(WTERMSIG(status)));
#ifdef WCOREDUMP /* Not in SUSv3, may be absent on some systems */
if (WCOREDUMP(status))
printf(" (core dumped)");
#endif
printf("\n");
} else if (WIFSTOPPED(status)) {
printf("child stopped by signal %d (%s)\n",
WSTOPSIG(status), strsignal(WSTOPSIG(status)));
#ifdef WIFCONTINUED /* SUSv3 has this, but older Linux versions and
some other UNIX implementations don't */
} else if (WIFCONTINUED(status)) {
printf("child continued\n");
#endif
} else { /* Should never happen */
printf("what happened to this child? (status=%x)\n",
(unsigned int) status);
}
}

int mosaik2_index(char *mosaik2_database_name,  uint32_t max_tiler_processes, uint32_t max_load_avg) {

	//signal(SIGINT, signal_handler);

	mosaik2_context ctx;
	init_mosaik2_context(&ctx);
	ctx.max_tiler_processes = get_max_tiler_processes(max_tiler_processes);
	ctx.max_load_avg = get_max_load_avg(max_load_avg);

	struct mosaik2_database_struct md;
	
	init_mosaik2_database_struct(&md, mosaik2_database_name);
	read_database_id(&md);
	check_thumbs_db(&md);
	check_pid_file(&md);
	write_pid_file(&md);

	parse_input_data(&ctx, &md);

	remove_pid_file(&md);

	return 0;
}

uint32_t get_max_tiler_processes(uint32_t max_tiler_processes) {
	if(max_tiler_processes < 1)
		return sysconf(_SC_NPROCESSORS_ONLN);
	if(max_tiler_processes<=MOSAIK2_CONTEXT_MAX_TILER_PROCESSES)
		return max_tiler_processes;
	fprintf(stderr, "max_tiler_processes is limited to %i\n", MOSAIK2_CONTEXT_MAX_TILER_PROCESSES);
	exit(EXIT_FAILURE);
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


void parse_input_data(mosaik2_context *ctx, struct mosaik2_database_struct *md) {
	
	//print_usage("prc_indat");

	mosaik2_indextask task_list[ctx->max_tiler_processes];
	int mosaik2_parser_input_data_column_count = 3;
	mosaik2_parser_input_data_column col[ mosaik2_parser_input_data_column_count ];
	
	md->tilecount = read_thumbs_conf_tilecount(md);
//	size_t linebuffer_len = BUFSIZ;
	size_t linebuffer_len = 16;
	char linebuffer[linebuffer_len];
	char old_linebuffer[linebuffer_len];

	//format from README.file_list is required

	//parser rules
	//iterate through all signs
	//does a parameter exeeds its maximum size, exit immediatly
	//copy input data directly into the target variable (mosaik2_indextask.filename)
	//replace split signs with null bytes, (to make string functions work directly with the input text data)
	//propagate unfinished text rests to the next fread call

	int nread;
	int i = 0;
	errno=0;
	size_t lineno = 0;
	size_t line_start = 0;
	size_t round = 0;
	
	int col_idx_filename = 0;
	int col_idx_filesize = 1;
	int col_idx_timestamp= 2;


	memset(col, 0, 3*sizeof(mosaik2_parser_input_data_column));

	strncpy(col[0].name, "filename", 9);
	strncpy(col[1].name, "filesize", 9);
	strncpy(col[2].name, "timestamp",10);

	col[0].split_sign = '\t';
	col[1].split_sign = '\t';
	col[2].split_sign = '\n';

	col[0].max_len = 1024;
	col[1].max_len = 11;
	col[2].max_len = 22;

	int s = 0; // idx for current split sign
	int dp = 1;  // idx for current data element ( (s + 1) % 3)

	while( ( nread = fread(&linebuffer, sizeof(char), linebuffer_len, stdin )) > 0 ) {
		fprintf(stderr, "fread %i\n", nread);
	
		for(int j=0;j<mosaik2_parser_input_data_column_count;j++) {
			col[j].data_pos = 0;
			col[j].old_data_pos = 0;
			col[j].split_pos = 0;
		}
	
		for(i=0;i<nread; i++) {

//			fprintf(stderr, "a %3li %3li dp:%i:[%i,%i,%i] s:%i:[%i,%i,%i] {%c %i} {%s,%s,%s}\n", round , i, dp, data_pos[0], data_pos[1], data_pos[2], s, split_pos[0], split_pos[1], split_pos[2], linebuffer[i], linebuffer[i], filename_copy, filesize_copy, timestamp_copy);
	
 			if( linebuffer[i] == col[s].split_sign ) {
				linebuffer[i] = '\0'; //replace split sign with null byte, because TODO maybe it is not needed, because every data is copied and explicit terminated with a null byte 

				col[s].split_pos = i; // save the split sign position 
															// and the end of this if, next data position is saved as well

				if(++s== mosaik2_parser_input_data_column_count ) { s=0; } //move s to the next split sign
			
				if(s==0&&i>0){ // line fully readed, all split signs have been read in one round
					
					print_col(col, "split_memcpy_1");
					for(int j=0;j<mosaik2_parser_input_data_column_count;j++) {
						if(col[j].finished == 1) {
							fprintf(stderr, "ignore finished\n");
							col[j].finished = 0;
							continue;
						}
						col[j].current_len = col[j].split_pos - col[j].data_pos;
						//check maximun text lengths before using the data
						if(col[j].current_len >= col[j].max_len) {
							fprintf(stderr, "%s is too long (line:%li)\n", col[j].name, lineno);
							exit(EXIT_FAILURE);
						}
						fprintf(stderr, "memcpy do:%2i, cl:%2i\n", col[j].data_offset, col[j].current_len);
						memcpy(col[j].data + col[j].data_offset, linebuffer + col[j].data_pos, col[j].current_len);
						col[j].data[  col[j].data_offset+col[j].current_len+1 ]='\0';
						col[j].old_data_pos = col[j].data_pos; // TODO col[s].data_pos was before?
						col[j].data_offset = 0;
						col[j].finished = 0;
					}

					print_col(col, "split_memcpy_2");
				}	
				if(i<nread-1) {
					col[dp].data_pos=i+1; // set the data position, if the linebuffer has still characters to process
				} else {
					// dont know what to do here, i guess i could ignore this else condition TODO
					fprintf(stderr, "cannot set data_pos beyond nread\n");
				}

				print_col(col, "split_sign_e");
				/*	fprintf(stdout, " %s - %s - %s", 
						&linebuffer[ data_pos[0] ], // at first 0
						&linebuffer[ data_pos[1] ],
						&linebuffer[ data_pos[2] ]); */
				if(++dp==mosaik2_parser_input_data_column_count) { dp = 0; } //move dp to the next data position index

			} else { // found NO split sign
				//do nothing, usually
				if(i==nread-1) { // but maybe the buffer has come to an end 

					// and intermediate results must be saved.
					for(int j=0;j<mosaik2_parser_input_data_column_count;j++) {
						col[j].current_len = col[j].split_pos - col[j].data_pos;

						if(col[j].data_offset > col[j].data_pos ) {
							// check if any size has exceeded TODO
						}

						if(col[j].current_len >= col[j].max_len) {
							fprintf(stderr, "%s is too long (line:%li charachters:%i)\n", col[j].name, lineno, col[j].current_len);
							exit(EXIT_FAILURE);
						}

						fprintf(stderr, "j:%i %20s\n", j, col[j].old_data_pos != col[j].data_pos?"data_pos changed ":"data_pos UNchanged");
						
						if(col[j].old_data_pos != col[j].data_pos) { // ah, there is data to save
							if(col[j].current_len>=0) { 
								fprintf(stderr, "memcpy entire odp:%2i dp:%2i do:%2i nread:%2i i:%2i nread-i:%2i cl:%2i\n",

										col[j].old_data_pos,col[j].data_pos,
										col[j].data_offset, nread, i, nread-i,col[j].current_len);
								memcpy(col[j].data + col[j].data_offset, linebuffer + col[j].data_pos, col[j].current_len);//save entire string	
								col[j].finished = 1;
							} else {// no split sign was found after data has started
							// This data section could not be read completely due to the buffer size. But the snippet is saved and .data_offset is set.
							fprintf(stderr,   "memcpy partl  odp:%2i dp:%2i do:%2i nread:%2i i:%2i nread-i:%2i cl:%2i\n", 
								col[j].old_data_pos, 
								col[j].data_pos, 
								col[j].data_offset, 
								nread, i, nread-i,
								col[j].current_len);
		
								int len = nread-col[j].data_pos;
								memcpy(col[j].data + col[j].data_offset, linebuffer + col[j].data_pos, len);
								col[j].data[ col[j].data_offset + len ] = '\0';
								col[j].data_offset = len;
								col[j].finished = 2;
							}
						} else {
							fprintf(stderr, "memset\n");
							// after last newline at the end of buffer in this column was nothing changed,
							// so it needs to be reset
							memset(col[j].data,0,col[j].max_len);
							col[j].data_offset = 0;
							col[j].current_len = 0;
							col[j].old_data_pos = col[j].data_pos;
							
						}
					}
					print_col(col, "linebuf END");

					continue;

					//fprintf(stderr, "[%s:%i] [%s:%i:%i] [%s:%i:%i]\n", filename_copy, current_filename_copy_len, &linebuffer[4], current_filesize_copy_len,data_pos[1], timestamp_copy, current_timestamp_copy_len, data_pos[2]);
				}
			} 

			//fprintf(stderr, "2 %3li %3li dp:%i:[%i,%i,%i] s:%i:[%i,%i,%i] {%c %i} {%s,%s,%s}\n", round , i, dp, data_pos[0], data_pos[1], data_pos[2], s, split_pos[0], split_pos[1], split_pos[2], linebuffer[i], linebuffer[i], filename_copy, filesize_copy, timestamp_copy);

		}	 // size_t size, size_t nmemb, FILE *stream);
		round++;
		fprintf(stdout, "next fread\n");
	}
	/*if(col[1].split_pos==0 || col[2].split_pos==0) {
		fprintf(stderr, "could not split stdin stream with this buffer size (lines too long?)\n");
		exit(EXIT_FAILURE);
	}*/
	for(i=0;i<nread;i++) {
			fprintf(stderr, "%3li %3i %c\n", round , i, linebuffer[i]);
	}

	//while((nread = getline(&linebuffer, &linebuffer_len, stdin)) != -1 ) {
	//	process_next_line(ctx, md, linebuffer, i++);
		// do something 
	//}
	if(errno == EINVAL||errno==ENOMEM) {
		err(errno, "while reading lines from stdin (read:%i bytes, line:%s).\n", nread, linebuffer);
		free(linebuffer);
		exit(errno);
	}
	fprintf(stderr, "wait\n");
	int wstatus=0;
	wait(&wstatus);
	fprintf(stderr, "waited\n");
}

void process_next_line(mosaik2_context *ctx, struct mosaik2_database_struct *md, char *line, ssize_t i) {
	
	fprintf(stdout, "%s start job #%li, jobs:%i %s %s", "now", i, ctx->current_tiler_processes, line , line);
	//char line[strlen(line0+1)];
//	strncpy(line, line0, strlen(line0+1));
 /*     if(this.count%1==0)console.log(new Date()+" finished  job #"+this.count+", jobs: "+ctx.runningTasks.length, " img/min:"+Math.round(  (ctx.count-this.ctx.initialCount)/ ((new Date()-this.ctx.t0)/60000),2),"px/ms:"+Math.round(this.ctx.pixel/((new Date()-this.ctx.t0))), "bytes/ms:"+ Math.round( this.ctx.bytes/((new Date()-this.ctx.t0)  ) )+", loadavg:"+this.ctx.loadavg);
      //console.log("running tasks 3",this.ctx.runningTasks.length,"age:",new Date() - age);
    } catch (e) {*/


  if(ctx->exiting)
		fprintf(stdout, "stdin is not resumed, EXITing because of SIGTERM.");

	mosaik2_indextask task;
//	memset(&task, 0, sizeof(task));
	
	char *token0 = strtok(line, "\t");
	char *token1 = strtok(NULL, "\t");
	char *token2 = strtok(NULL, "\n");
	if(token0 == NULL || token1 == NULL || token2 == NULL ) {
		errx(EINVAL, "could not split line by tabstop into three token (%s,%s,%s) filename is empty. (linenumber:%li, line:%s)\n",token0,token1,token2,i, line);
	}

	if(strlen(token0)>=sizeof(task.filename)) {
		errx(EINVAL, "ups, this filename is to long. sorry you have to remove or rename anything, that exceeds %li characters\n", sizeof(task.filename));
	}

	strncpy(task.filename, token0, strlen(token0)+1);
	

	task.filesize = atol(token1);
	task.lastmodified = atoll(token2);
	//here forken

	//fprintf(stdout,"%i cp: %i mp: %i\n", getpid(), ctx->current_tiler_processes, ctx->max_tiler_processes);

	while(ctx->current_tiler_processes >= ctx->max_tiler_processes) {
		//fprintf(stdout, "%i too many processes, wait for one to exit\n", getpid());
		mosaik2_index_clean_tiler_pids(ctx);
		//fprintf(stdout, "%i too many processes2, parent wait FINISHED\n", getpid());
	}

	//print_usage("will fork");
	pid_t pid = fork(); // 0.2 ms
	
	if(pid==0) {

		//print_usage("was forked");
		//fprintf(stdout, "%i mosaik2_tiler\n", getpid());
		mosaik2_tiler(md, &task);
		//fprintf(stdout, "%i mosaik2_tiler2\n", getpid());
		//print_usage("exit");
		exit(0);
	} else {
		//print_usage("has forked");
		//fprintf(stdout, "%i forked pid:%i\n", getpid(), pid);
		mosaik2_index_add_tiler_pid(ctx, pid);
		//print_usage("pid added");
	}

	


	//fopen(task.filename, "r");
	//process the data in serial and compare duration to old variant	
	return;
}

void mosaik2_index_write_to_disk(struct mosaik2_database_struct *md, mosaik2_indextask *task) {

	// duration 0.2 ms

	//print_usage("write idx");

	// lock the lockfile to make all other forked processes wait this process finishp

	FILE *lock_file = fopen( md->lock_filename, "r");
	if(lock_file == NULL )
		lock_file = fopen( md->lock_filename, "w");
	else 
		fprintf(stderr, "mosaik2 database lock file exists, cannot write to index\n");


	

	FILE *imagestddev_file = fopen( md->imagestddev_filename, "a");
	if(imagestddev_file == NULL) errx(errno, "cannot open mosaik2 database file (%s)", md->imagestddev_filename);
	FILE *imagecolors_file = fopen( md->imagecolors_filename, "a");
	if(imagecolors_file == NULL) errx(errno, "cannot open mosaik2 database file (%s)", md->imagecolors_filename);
	FILE *imagedims_file = fopen( md->imagedims_filename, "a");
	if(imagedims_file == NULL) errx(errno, "cannot open mosaik2 database file (%s)", md->imagedims_filename);
	FILE *filenames_file = fopen( md->filenames_filename, "a");
	if(filenames_file == NULL) errx(errno, "cannot open mosaik2 database file (%s)", md->filenames_filename);
	FILE *filehashes_file = fopen( md->filehashes_filename, "a");
	if(filehashes_file == NULL) errx(errno, "cannot open mosaik2 database file (%s)", md->filehashes_filename);
	FILE *timestamps_file = fopen( md->timestamps_filename, "a");
	if(timestamps_file == NULL) errx(errno, "cannot open mosaik2 database file (%s)", md->timestamps_filename);
	FILE *filesizes_file = fopen( md->filesizes_filename, "a");
	if(filesizes_file == NULL) errx(errno, "cannot open mosaik2 database file (%s)", md->filesizes_filename);
	FILE *tiledims_file = fopen( md->tiledims_filename, "a");
	if(tiledims_file == NULL) errx(errno, "cannot open mosaik2 database file (%s)", md->tiledims_filename);
	FILE *invalid_file = fopen( md->invalid_filename, "a");
	if(invalid_file == NULL) errx(errno, "cannot open mosaik2 database file (%s)", md->invalid_filename);
	FILE *duplicates_file = fopen( md->duplicates_filename, "a");
	if(duplicates_file == NULL) errx(errno, "cannot open mosaik2 database file (%s)", md->duplicates_filename);

  //size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);
	char null_value=0;
	char new_line='\n';
	
	fwrite(task->filename, 1, strlen(task->filename), filenames_file);
	fwrite(&new_line, 1, 1, filenames_file);

	fwrite(task->colors, 1, 3*task->total_tile_count, imagecolors_file);
	fwrite(task->colors_stddev, 1, 3*task->total_tile_count, imagestddev_file);

	
	fwrite(&null_value,1,1,invalid_file);
	fwrite(&null_value,1,1,duplicates_file);


	

	if( fclose( imagestddev_file ) != 0) errx(errno, "cannot close mosaik2 database file (%s)", md->imagestddev_filename);
	if( fclose( imagecolors_file ) != 0) errx(errno, "cannot close mosaik2 database file (%s)", md->imagecolors_filename);
	if( fclose( imagedims_file ) != 0) errx(errno, "cannot close mosaik2 database file (%s)", md->imagedims_filename);  
	if( fclose( filenames_file ) != 0) errx(errno, "cannot close mosaik2 database file (%s)", md->filenames_filename);  
	if( fclose( filehashes_file ) != 0) errx(errno, "cannot close mosaik2 database file (%s)", md->filehashes_filename); 
	if( fclose( timestamps_file ) != 0) errx(errno, "cannot close mosaik2 database file (%s)", md->timestamps_filename);
	if( fclose( filesizes_file ) != 0) errx(errno, "cannot close mosaik2 database file (%s)", md->filesizes_filename);
	if( fclose( tiledims_file ) != 0) errx(errno, "cannot close mosaik2 database file (%s)", md->tiledims_filename);
	if( fclose( invalid_file ) != 0) errx(errno, "cannot close mosaik2 database file (%s)", md->invalid_filename);
	if( fclose( duplicates_file ) != 0) errx(errno, "cannot close mosaik2 database file (%s)", md->duplicates_filename);

	if( unlink( md->lock_filename ) != 0) 
		errx(errno, "cannot unlink lock file! for indexing");
	//print_usage("wrote idx");
}

void mosaik2_index_add_tiler_pid(mosaik2_context *ctx, pid_t pid) {
	if(pid < 1) {
		fprintf(stderr, "pid of forked process has an invalid value\n");
		exit(EXIT_FAILURE);
	}
	int inserted=0;
	for(int i=0; i < MOSAIK2_CONTEXT_MAX_TILER_PROCESSES; i++) {
		if(ctx->pids[i] == 0) {
			ctx->pids[i] = pid;
			ctx->current_tiler_processes++;
			inserted = 1;
			break;
		}
	}
	if(inserted==0) {
		fprintf(stderr, "could not save pid, this should not have happend\n");
		exit(EXIT_FAILURE);
	}
}

void mosaik2_index_clean_tiler_pids(mosaik2_context *ctx) {
	//print_usage("clean0");
		int usleep_rc = usleep(10000);
	//print_usage("clean1");
	
	for(int i=0;i<10; i++) {
		if(ctx->pids[i]>0) {
			int status;
			//fprintf(stdout, "%i check pid %i\n", getpid(),ctx->pids[i]);
			//pid_t child_pid = waitpid(-1, &status, WUNTRACED | WCONTINUED);
			
			pid_t child_pid = waitpid(ctx->pids[i], &status, WNOHANG);
			//fprintf(stdout, "%i check pid %i return_pid:%i\n", getpid(),ctx->pids[i], child_pid);

			if(child_pid == 0)
				continue; //still running
			if(child_pid == ctx->pids[i]) {

			//if(WIFEXITED(status)) printf("%i exited:%d\n", getpid(), child_pid);
			//if(WIFSIGNALED(status)) printf("%i wifsignaled %d\n", getpid(), child_pid);
				
				ctx->pids[i]=0;
				ctx->current_tiler_processes--;
				
			} else if(child_pid == -1) {
				fprintf(stdout, "%i check pid %i\n", getpid(),ctx->pids[i]);
				ctx->pids[i]=0;
				ctx->current_tiler_processes--;
			} else {
				fprintf(stderr, "unexpected waitpid code of child process: %i\n", child_pid);
				exit(EXIT_FAILURE);
			}
		}
	}
	//print_usage("clean2");
			/*if(child_pid==-1)perror("child_pid");
			fprintf(stdout, "%i waitpid return value:%i,child_pid:%i\n", getpid(), ctx->pids[i], child_pid);
			continue;
			if(child_pid == ctx->pids[i]) {
				fprintf(stdout, "%i child state has changed ctx->pids[%i]:%i child_pid:%i\n", getpid(), i, ctx->pids[i], child_pid);
			} else if(child_pid==0) {
				fprintf(stdout, "%i child state has not changed\n", getpid());
			}
			//fprintf(stdout, "%i check pid2 %i %i\n", getpid(),ctx->pids[i]);
			if(child_pid == -1) {
				fprintf(stdout,"%i waitpid=-1\n", getpid());
				ctx->pids[i]=0;
				ctx->current_tiler_processes--;
				continue;
				//exit(EXIT_FAILURE);
			}
			//printf("%i waitpid() returned: PID=%ld; status=0x%04x (%d,%d)\n",
			//	getpid(),(long) child_pid,
			//	(unsigned int) status, status >> 8, status & 0xff);	
			if(WIFEXITED(status)) {
				ctx->pids[i]=0;
				ctx->current_tiler_processes--;
				//printf("%i exited:%d\n", getpid(), child_pid);
			}
			if(WIFSIGNALED(status))
				printf("%i wifsignaled %d\n", getpid(), child_pid);
		}*/
}

/*void signal_handler(int signal) {
	if(signal == SIGINT) {
		printf("SIGINT catched, do not exit\n");
	}
	if(signal==SIGUSR1) {
		fprintf(stdout, "parent SIGUSR1 received\n");
	}
}
*/

void print_col_i(mosaik2_parser_input_data_column c[], int i, char *key) {
		fprintf(stderr, "%15s %i %9s %10s sp:%2i dp:%2i odp:%2i cl:%2i do:%2i\n", key, i, c[i].name, c[i].data, c[i].split_pos, c[i].data_pos, c[i].old_data_pos, c[i].current_len, c[i].data_offset);
}

		
void print_col(mosaik2_parser_input_data_column c[], char *key) {
	for(int i=0;i<3;i++) {
		print_col_i(c, i, key);
	}
	fprintf(stderr, "\n");
}

		
