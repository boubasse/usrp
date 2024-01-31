#include <string.h>
#include <ctype.h>
#include <math.h>
#include <getopt.h>
#include <signal.h>
#include <errno.h>
#include <stdbool.h>
#include <unistd.h>


#define PROGRAM_VERSION "0.0.1"

#define BUFF_SIZE 2048
	
enum FMT_DATA_TYPE {
	FMT_U8 = 0,
	FMT_S16,
	FMT_FLOAT
};

typedef struct {
	uint8_t re;  
	uint8_t im;
}u8cmplx;


typedef struct {
	int16_t re;
	int16_t im;
}s16cmplx;


typedef struct{
	float re;
	float im;
}fcmplx;

FILE *input_fid;
FILE *output_fid;

static bool m_keep_running=false;

static void signal_handler(int signo){
	if (signo == SIGINT)
		fputs("\nCaught SIGINT\n", stderr);
	else if (signo == SIGTERM)
		fputs("\nCaught SIGTERM\n", stderr);
	else if (signo == SIGHUP)
		fputs("\nCaught SIGHUP\n", stderr);
	else if (signo == SIGPIPE)
		fputs("\nReceived SIGPIPE.\n", stderr);
	else
		fprintf(stderr, "[KISSPEC] \nCaught signal: %d\n", signo);
	
	m_keep_running = false;
}

void print_usage(){

	fprintf(stderr,\
	"loopRead -%s\n\
	Usage:loopRead -i File [-t IQType][-h] \n\
	-i            Input IQ File I16 format (default stdin) \n\
	-t            IQType {float,u8,i16} (default float) \n\
	-h            help (print this help).\n\
	Example : .\\loopRead -i - \n\
	\n", \
	PROGRAM_VERSION);

} /* end function print_usage */



int main(int argc, char *argv[]) {

	input_fid  = stdin;
	output_fid = stdout;
	int dataTypeIn = FMT_FLOAT;
	
	int a;
	bool anyargs = false;
	while(1){
		
		a = getopt(argc, argv, "i:t:h");
		if(a==-1){
			if(anyargs){ break; }
			else       { a = 'h'; } //print usage and exit
		}
		anyargs = true;
		
		switch(a){
			case 'i':{ // InputFile
				if(optarg != NULL){
					input_fid = fopen(optarg, "rb");
					if (input_fid == NULL){
						fprintf(stderr, "[LOOP RD] Unable to open '%s': %s\n",optarg, strerror(errno));
						exit(EXIT_FAILURE);
					}
				}
				}break;
			case 't':{ // Input Type
					if (strcmp("float", optarg) == 0){ dataTypeIn = FMT_FLOAT; }
					if (strcmp("s16", optarg) == 0)  { dataTypeIn = FMT_S16;   }
					if (strcmp("u8", optarg) == 0)   { dataTypeIn = FMT_U8;    }
				}break;
			case 'h':{ // help
				print_usage();
				exit(0);
				}break;
			case -1:{
				}break;
			case '?':{
					if(isprint(optopt)){ fprintf(stderr, "[LOOP RD] loopRead `-%c'.\n", optopt); }
					else               { fprintf(stderr, "[LOOP RD] loopRead: unknown option character `\\x%x'.\n", optopt); }
					print_usage();
					exit(1);
				}break;
			default:{
					print_usage();
					exit(1);
				}break;
		}/* end switch a */
	}/* end while getopt() */
	
	bool isapipe = (fseek(input_fid, 0, SEEK_CUR) < 0); //Dirty trick to see if it is a pipe or not
	if(isapipe){ fprintf(stderr, "[LOOP RD] Using IQ live mode\n"); }
	else       { fprintf(stderr, "[LOOP RD] Read from data File\n"); }
			
	// register signal handlers
	if (signal(SIGINT, signal_handler) == SIG_ERR)
		fputs("Warning: Can not install signal handler for SIGINT\n", stderr);
	if (signal(SIGTERM, signal_handler) == SIG_ERR)
		fputs("Warning: Can not install signal handler for SIGTERM\n", stderr);
	if (signal(SIGHUP, signal_handler) == SIG_ERR)
		fputs("Warning: Can not install signal handler for SIGHUP\n", stderr);
	if (signal(SIGPIPE, signal_handler) == SIG_ERR)
		fputs("Warning: Can not install signal handler for SIGPIPE\n", stderr);

	// =======================================================================================================================================================================================================
	// ########################################################################################################################################################################################################
	// =======================================================================================================================================================================================================
	fcmplx BufferIQ[BUFF_SIZE];    //buffer to hold complex values
	bool repeat_reading = true;
	unsigned long long FrameCount = 0;
	m_keep_running = true;

	while(m_keep_running = true;){
		int nRead=0; 
		switch(dataTypeIn){
			case FMT_U8:{
				u8cmplx iqin_u8[BUFF_SIZE];           
				nRead=fread(iqin_u8,sizeof(u8cmplx),BUFF_SIZE,input_fid);
				if(nRead<=0){ break; } 
				for(int i=0; i<nRead; i++){
					BufferIQ[i].re = (iqin_u8[i].re - 127.5)/127.0;
					BufferIQ[i].im = (iqin_u8[i].im - 127.5)/127.0;						
				}   
			}break;
			case FMT_S16:{
				i16cmplx iqin_s16[BUFF_SIZE];
				nRead = fread(iqin_s16, sizeof(i16cmplx),BUFF_SIZE,input_fid);           
				if(nRead<=0){ break; } 
				for(int i=0; i<nRead; i++){
					BufferIQ[i].re = iqin_i16[i].re/32767.0;
					BufferIQ[i].im = iqin_i16[i].im/32767.0;						
				}
			}break;
			case FMT_FLOAT:{
				fcmplx iqin_f32[BUFF_SIZE];         
				nRead=fread(iqin_f32,sizeof(fcmplx),BUFF_SIZE,input_fid); 
				if(nRead<=0){ break; }
				for(int i=0; i<nRead; i++){
					BufferIQ[i].re = iqin_f32[i].re;
					BufferIQ[i].im = iqin_f32[i].im;               
				}    
			}break;
		}
		
		if(nRead<=0){
			if(repeat_reading){ fseek(input_fid,0, SEEK_SET);  FrameCount = 0; }
			else{ fprintf(stderr,"[LOOP RD] No more samples to Read, quit \n");  break; }
		}
		else{
			if(nRead<BUFF_SIZE){ fprintf(stderr, "[LOOP RD] Incomplete buffer %d/%d\n",nRead,BUFF_SIZE); }
		
			// Write on file
			nWrite = fwrite(BufferIQ,sizeof(fcmplx),nRead,output_fid);
			if(nWrite<nRead){ fprintf(stderr,"[LOOP RD] Error writing data, quit \n"); break; }
		
			FrameCount++;
		
		}
	} // end while
	
	// close binary file
	fclose(input_fid);		
	fclose(output_fid);
	
	return 0;
	
}