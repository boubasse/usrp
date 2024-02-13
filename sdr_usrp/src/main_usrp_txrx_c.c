

#include <uhd.h>
#include <getopt.h>
#include <math.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <ctype.h>

#include <errno.h> //***
#include <stdint.h> //***
#include <stdio.h> //***
#include <stdlib.h> //***




#define DEVNAME_B200 "uhd_b200"
#define DEVNAME_X300 "uhd_x300"
#define DEVNAME_N300 "uhd_n300"
#define DEVNAME_E3X0 "uhd_e3x0"
#define DEVNAME_UNKNOWN "uhd_unknown"


// -----------------------------------------------------------------------------------------------
// ANSI escapes always start with \x1b, or \e, or \033. 

#define RESET     "\x1B[0m"      // Reset (default)
#define HIGHLIGHT "\x1B[1m"      // surligne (un fond jaune)
#define UNDERLINE "\x1B[4m"      // souligne
#define BLINK     "\x1B[5m"      // Reset (default)

#define BLACK    "\x1B[0;30m"    // Black
#define BBLACK    "\x1B[1;30m"    // Bold Black
#define RED      "\x1B[0;31m"    // Red
#define BRED     "\x1B[1;31m"    // Bold Red
#define GREEN    "\x1B[0;32m"    // Green
#define BGREEN   "\x1B[1;32m"    // Bold Green
#define YELLOW   "\x1B[0;33m"    // Yellow
#define BYELLOW  "\x1B[1;33m"    // Bold Yellow
#define BLUE     "\x1B[0;34m"    // Blue
#define BBLUE    "\x1B[1;34m"    // Bold Blue
#define MAGENTA  "\x1B[0;35m"    // Magenta
#define BMAGENTA "\x1B[1;35m"    // Bold Magenta
#define CYAN     "\x1B[0;36m"    // Cyan
#define BCYAN    "\x1B[1;36m"    // Bold Cyan
#define WHITE    "\x1B[0;37m"    // White
#define BWHITE   "\x1B[1;37m"    // Bold White
// -----------------------------------------------------------------------------------------------

enum FMT_DATA_TYPE {
	FMT_U8 = 0,
	FMT_I16,
	FMT_FLOAT,
	FMT_DOUBLE
};

void print_usage(){
    fprintf(stderr,
        "main_usrp_rx_c - A simple TX example using UHD's C API\n\n"

        "Options:\n"
        "    -i (input fid)\n"
		"    -o (output fid)\n"
        "    -f (center frequency in Hz)\n"
        "    -s (sample rate in Hz)\n"
		"    -b (bandwidth in Hz)\n"
        "    -g (gain ratio 0..100)\n"
		"    -t (dataType {u8,i16,float,double})\n"
        "    -h (print this help message)\n");
}

bool m_keep_running = false;

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
        fprintf(stderr, "\nCaught signal: %d\n", signo);

    m_keep_running = false;
}

static bool find_string(uhd_string_vector_handle h, char *str) {
  char buff[128];
  size_t n;
  uhd_string_vector_size(h, &n);
  for (size_t i=0;i<n;i++) {
    uhd_string_vector_at(h, i, buff, 128);
    if (strstr(buff, str)) {
      return true; 
    }
  }
  return false; 
}


int main(int argc, char* argv[]){
	
	if(uhd_set_thread_priority(uhd_default_thread_priority, true)){
        fprintf(stderr, "Unable to set thread priority. Continuing anyway.\n");
    }
	
	
    FILE *input_fid = stdin;
	FILE *output_fid = stdout;
	
	enum {DEFAULT, EXTERNAL, GPSDO} clock_src = DEFAULT;
    
    size_t total_num_samps  = 0;
    int dataTypeIn = FMT_I16; //cpu_format
    size_t CHAN = 0;
    
    double m_CenterFrequency  = 2e9;
    double m_SampleRate       = 1e6;
    double m_gainRatio        = 0.7;
	double m_Bandwidth        = 0.0;
    

    int a;
	bool anyargs = false;
    while(1){
		
		a = getopt(argc, argv, "i:o:s:b:f:g:t:h:");
		if(a==-1){
			if(anyargs){ break; }
			else       { a = 'h'; } //print usage and exit
		}
		anyargs = true;

		switch(a){
			case 'i':{ // InputFile
					if(optarg != NULL){
						input_fid = fopen(optarg, "rb");
						if( input_fid == NULL ){
							fprintf(stderr, "[SDR TXRX] Unable to open '%s': %s\n",optarg, strerror(errno));
							exit(EXIT_FAILURE);
						}
					}
				}break;
			case 'o':{ // OutputFile
					if(optarg != NULL){
						output_fid = fopen(optarg, "wb");
						if( output_fid == NULL ){
							fprintf(stderr, "[SDR RX] Unable to open '%s': %s\n",optarg, strerror(errno));
							exit(EXIT_FAILURE);
						}
					}
				}break;
			case 's':{ // SymbolRate in KS
					m_SampleRate = atol(optarg);
				}break;
			case 'b':{ // Bandwidth in KS
					m_Bandwidth = atol(optarg);
				}break;
			case 'f':{ // TxFrequency in Mhz
					m_CenterFrequency = atol(optarg);
				}break;
			case 'g':{ // Gain 0..100
					m_gainRatio = atoi(optarg)/100.0;
				}break;
			case 't':{ // Input CPU Data Type
			        if (strcmp("double", optarg) == 0){ dataTypeIn = FMT_DOUBLE; }
					if (strcmp("float", optarg) == 0){ dataTypeIn = FMT_FLOAT; }
					if (strcmp("i16", optarg) == 0)  { dataTypeIn = FMT_I16;   }
					if (strcmp("u8", optarg) == 0)   { dataTypeIn = FMT_U8;    }
				}break;
			case 'h':{ // help
					print_usage();
					exit(0);
				}break;
			case -1:{
				}break;
			case '?':{
					if(isprint(optopt)){ fprintf(stderr, "[SDR TXRX] limetx `-%c'.\n", optopt); }
					else               { fprintf(stderr, "[SDR TXRX] limetx: unknown option character `\\x%x'.\n", optopt); }
					print_usage();
					exit(1);
				}break;
			default:{
					print_usage();
					exit(1);
				}break;
		}/* end switch a */
    }/* end while getopt() */
    
    bool isapipe = (fseek(input_fid, 0, SEEK_CUR) < 0); //Dirty trick to see ifit is a pipe or not
    if(isapipe){ fprintf(stderr, "[SDR TXRX] Using IQ live mode\n"); }
    else       { fprintf(stderr, "[SDR TXRX] Read from data File\n"); }	
    
    if(m_CenterFrequency<=0){ fprintf(stderr, "[SDR TXRX] Need set a frequency to tx\n"); exit(0);}
    if(m_SampleRate<=0){ fprintf(stderr, "[SDR TXRX] Need set a SampleRate \n"); exit(0);}
    if(m_gainRatio<=0){ fprintf(stderr, "[SDR TXRX] Need set a Gain \n"); exit(0);}
       
	   
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
    

    
    // ======================================================================================
    // https://gitlab.modalai.com/srs-opensource/srsLTE/blob/master/lib/src/phy/rf/rf_uhd_imp.c
    
    
    //copy_subdev_string
    //remove_substring
    //REMOVE_SUBSTRING_WITHCOMAS
    
    
    
    /* Find available devices */
    uhd_string_vector_handle devices_str;
    uhd_string_vector_make(&devices_str);
    uhd_usrp_find("", &devices_str);
    
    /* print available devices */
    size_t n;
    uhd_string_vector_size(devices_str, &n);
    char buff_str[128];
    for (size_t i=0;i<n;i++) {
        uhd_string_vector_at(devices_str, i, buff_str, 128);
        fprintf(stderr, "[SDR TXRX] devices[%li]: %s \n",i,buff_str);
    }
    
    /* If device type or name not given in args, choose a B200 */
	char *device_args = "type=x300,addr=192.168.10.2,fpga=HG,name=USRP1,serial=30AABE8,product=X310";
    char *devname = NULL;
	double current_master_clock = -1.0;
    /*bool dynamic_rate;*/
    if (device_args[0]=='\0') {
      if (find_string(devices_str, "type=b200") && !strstr(device_args, "recv_frame_size")) {
        // If B200 is available, use it
        device_args = "type=b200,master_clock_rate=30.72e6";
        current_master_clock = 30720000;
        devname = DEVNAME_B200;
      } else if (find_string(devices_str, "type=x300")) {
        // Else if X300 is available, set master clock rate now (can't be changed later)
        device_args = "type=x300,master_clock_rate=184.32e6";
        current_master_clock = 184320000;
        /*dynamic_rate = false;*/
        devname = DEVNAME_X300;
      } else if (find_string(devices_str, "type=e3x0")) {
        // Else if E3X0 is available, set master clock rate now (can't be changed later)
        device_args = "type=e3x0,master_clock_rate=30.72e6";
        /*dynamic_rate = false;*/
        devname = DEVNAME_E3X0;
      } else if (find_string(devices_str, "type=n3xx")) {
        device_args = "type=n3xx,master_clock_rate=122.88e6";
        current_master_clock = 122880000;
        /*dynamic_rate = false;*/
        devname = DEVNAME_N300;
        //srslte_use_standard_symbol_size(true);
      }
    } else {
      char args2[512];
      // If args is set and x300 type is specified, make sure master_clock_rate is defined
      if (strstr(device_args, "type=x300") && !strstr(device_args, "master_clock_rate")) {
        sprintf(args2, "%s,master_clock_rate=184.32e6",device_args);
        device_args = args2;
        current_master_clock = 184320000;
        /*dynamic_rate = false;*/
        devname = DEVNAME_X300;
      } else if (strstr(device_args, "type=n3xx")) {
        sprintf(args2, "%s,master_clock_rate=122.88e6", device_args);
        device_args = args2;
        current_master_clock = 122880000;
        /*dynamic_rate = false;*/
        devname = DEVNAME_N300;
        //srslte_use_standard_symbol_size(true);
     } else if (strstr(device_args, "type=e3x0")) {
        snprintf(args2, sizeof(args2), "%s,master_clock_rate=30.72e6", device_args);
        device_args = args2;
        devname = DEVNAME_E3X0;
      } else {
        snprintf(args2, sizeof(args2), "%s,master_clock_rate=30.72e6", device_args);
        device_args = args2;
        current_master_clock = 30720000;
        devname = DEVNAME_B200;
      }
    }

    uhd_string_vector_free(&devices_str);

// ===============================================================================
    
    // Check external clock argument
    if (strstr(device_args, "clock=external")) {
      //REMOVE_SUBSTRING_WITHCOMAS(device_args, "clock=external");
      clock_src = EXTERNAL;
    } else if (strstr(device_args, "clock=gpsdo")) {
      fprintf(stderr,"[SDR TXRX] Using GPSDO clock\n");
      //REMOVE_SUBSTRING_WITHCOMAS(device_args, "clock=gpsdo");
      clock_src = GPSDO;
    } else {
      clock_src = DEFAULT;
    }
    

// ===============================================================================
   size_t mboard = 0;

   fprintf(stderr, "[SDR TXRX] Device args: %s \n",device_args);
   
   uhd_error status;

   /* Create UHD handler */
    fprintf(stderr,"[SDR TXRX] Opening TX USRP with args: %s\n", device_args);
    uhd_usrp_handle tx_usrp;
    status = uhd_usrp_make(&tx_usrp, device_args); //EXECUTE_OR_GOTO(free_option_strings
    if(status != UHD_ERROR_NONE){
      fprintf(stderr, "[SDR TX] Error opening UHD TX: code %d\n", status);
      tx_usrp = NULL;
      return -1; 
    }
	
	if(tx_usrp != NULL){ fprintf(stderr, "[SDR TXRX] Device is detected with success ....\n"); }
	
	/* Create UHD handler */
    fprintf(stderr,"[SDR RX] Opening RX USRP with args: %s\n", device_args);
    uhd_usrp_handle rx_usrp;
    status = uhd_usrp_make(&rx_usrp, device_args); //EXECUTE_OR_GOTO(free_option_strings
    if(status != UHD_ERROR_NONE){
      fprintf(stderr, "[SDR TXRX] Error opening UHD RX: code %d\n", status);
      rx_usrp = NULL;
      return -1; 
    }
    
    if(rx_usrp != NULL){ fprintf(stderr, "[SDR TXRX] Device is detected with success ....\n"); }
	
	
	/* dev name */
	if (devname) {
      char dev_str[1024];
      uhd_usrp_get_mboard_name(tx_usrp, mboard, dev_str, 1024);
      if (strstr(dev_str, "B2") || strstr(dev_str, "B2")) {
        devname = DEVNAME_B200;
      } else if (strstr(dev_str, "X3") || strstr(dev_str, "X3")) {
        devname = DEVNAME_X300;    
      } else if (strstr(dev_str, "n3xx")) {
        devname = DEVNAME_N300;
      }
    }
    if (!devname) {
      devname = "uhd_unknown"; 
    }
	
	fprintf(stderr, "[SDR TX] usrp name = %s...\n", devname);
	
	
	// =================================================================
	
    
	// Set Master Clock
	if (current_master_clock>0){
		fprintf(stderr, "[SDR TXRX] Setting master clock rate: %f...\n", current_master_clock);
		status = uhd_usrp_set_master_clock_rate(tx_usrp,current_master_clock,mboard);
		status = uhd_usrp_set_master_clock_rate(rx_usrp,current_master_clock,mboard);
	}
	else{ fprintf(stderr,"[SDR TXRX] master clock rate setting is not specified \n"); }
	
	status = uhd_usrp_get_master_clock_rate(tx_usrp,mboard,&current_master_clock);
	fprintf(stderr, "[SDR TXRX] Actual TX master clock rate: %f...\n", current_master_clock);
	status = uhd_usrp_get_master_clock_rate(rx_usrp,mboard,&current_master_clock);
	fprintf(stderr, "[SDR TXRX] Actual RX master clock rate: %f...\n", current_master_clock);
    
    // Set Sample Rate
    fprintf(stderr, "[SDR TXRX] Setting TX/RX Rate: %f...\n", m_SampleRate);
    status = uhd_usrp_set_tx_rate(tx_usrp, m_SampleRate, CHAN);
	status = uhd_usrp_set_rx_rate(rx_usrp, m_SampleRate, CHAN);
	
    status = uhd_usrp_get_tx_rate(tx_usrp, CHAN, &m_SampleRate);
    fprintf(stderr, "[SDR TXRX] Actual TX Rate: %f...\n", m_SampleRate);
	status = uhd_usrp_get_rx_rate(rx_usrp, CHAN, &m_SampleRate);
    fprintf(stderr, "[SDR TXRX] Actual RX Rate: %f...\n", m_SampleRate);
    
    // Set center Frequency
    uhd_tune_request_t tune_request = {
        .target_freq     = m_CenterFrequency,
        .rf_freq_policy  = UHD_TUNE_REQUEST_POLICY_AUTO,
        .dsp_freq_policy = UHD_TUNE_REQUEST_POLICY_AUTO,
    };
    uhd_tune_result_t tune_result;
    fprintf(stderr, "[SDR TXRX] Setting TX/RX frequency: %f MHz...\n", m_CenterFrequency / 1e6);
    status = uhd_usrp_set_tx_freq(tx_usrp, &tune_request, CHAN, &tune_result);
	status = uhd_usrp_set_rx_freq(rx_usrp, &tune_request, CHAN, &tune_result);
    
	status = uhd_usrp_get_tx_freq(tx_usrp, CHAN, &m_CenterFrequency);// EXECUTE_OR_GOTO(free_rx_metadata,
    fprintf(stderr, "[SDR TXRX] Actual TX frequency: %f MHz...\n", m_CenterFrequency / 1e6);
	status = uhd_usrp_get_rx_freq(rx_usrp, CHAN, &m_CenterFrequency);// EXECUTE_OR_GOTO(free_rx_metadata,
    fprintf(stderr, "[SDR TXRX] Actual RX frequency: %f MHz...\n", m_CenterFrequency / 1e6);
	
	// Set gain : Set starting gain to half maximum in case of using AGC
	uhd_meta_range_handle metric_range = NULL;
    uhd_meta_range_make(&metric_range);
	double max_gain_TX,max_gain_RX;
	
	status = uhd_usrp_get_tx_gain_range(tx_usrp, "", CHAN, metric_range);          
    status = uhd_meta_range_stop(metric_range, &max_gain_TX);
	double gain_TX = max_gain_TX*m_gainRatio;
	
	status = uhd_usrp_get_rx_gain_range(rx_usrp, "", CHAN, metric_range);          
    status = uhd_meta_range_stop(metric_range, &max_gain_RX);
	double gain_RX = max_gain_RX*m_gainRatio;
	
    fprintf(stderr, "[SDR TXRX] Setting TX Gain: %f dB  RX Gain: %f...\n", gain_TX,gain_RX);
    status = uhd_usrp_set_tx_gain(tx_usrp, gain_TX, CHAN, "");
	status = uhd_usrp_set_rx_gain(rx_usrp, gain_RX, CHAN, "");
    
	status = uhd_usrp_get_tx_gain(tx_usrp, CHAN, "", &gain_TX);
    fprintf(stderr, "[SDR TXRX] Actual TX Gain: %f...\n", gain_TX);
	status = uhd_usrp_get_rx_gain(rx_usrp, CHAN, "", &gain_RX);
    fprintf(stderr, "[SDR TXRX] Actual RX Gain: %f...\n", gain_RX);
    
    //set the IF filter bandwidth
	fprintf(stderr, "[SDR RX] Setting RX Bandwidth: %f...\n", m_Bandwidth);
    status = uhd_usrp_set_tx_bandwidth(tx_usrp, m_Bandwidth, CHAN);
	status = uhd_usrp_set_rx_bandwidth(rx_usrp, m_Bandwidth, CHAN);
	
	status = uhd_usrp_get_tx_bandwidth(tx_usrp, CHAN, &m_Bandwidth);
	fprintf(stderr, "[SDR TXRX] Actual TX Bandwidth: %f...\n", m_Bandwidth);
    status = uhd_usrp_get_rx_bandwidth(rx_usrp, CHAN, &m_Bandwidth);
	fprintf(stderr, "[SDR TXRX] Actual RX Bandwidth: %f...\n", m_Bandwidth);
	
	// =================================================================
    
	// Create TX streamer
    uhd_tx_streamer_handle tx_streamer;
    status = uhd_tx_streamer_make(&tx_streamer); //EXECUTE_OR_GOTO(free_usrp,
    if(status != UHD_ERROR_NONE){ return -1; }
	
	uhd_rx_streamer_handle rx_streamer;
	status = uhd_rx_streamer_make(&rx_streamer); //EXECUTE_OR_GOTO(free_usrp,
    if(status != UHD_ERROR_NONE){ return -1; }
    
    // Set up streamer
    uhd_stream_args_t stream_args;
    
    stream_args.otw_format = "sc16";
    stream_args.args = "";
    stream_args.channel_list = &CHAN;
    stream_args.n_channels = 1;
	switch(dataTypeIn){
		case FMT_U8:{ stream_args.cpu_format = "sc8"; }break;
		case FMT_I16:{ stream_args.cpu_format = "sc16"; }break;
		case FMT_FLOAT:{ stream_args.cpu_format = "fc32"; }break;
		case FMT_DOUBLE:{ stream_args.cpu_format = "fc64"; }break;
	}
    
    status = uhd_usrp_get_tx_stream(tx_usrp, &stream_args, tx_streamer); // EXECUTE_OR_GOTO(free_tx_streamer
    if(status != UHD_ERROR_NONE){
      fprintf(stderr, "[SDR TXRX] Error opening TX stream: %d\n", status);
      return -1; 
    }
	
	 status = uhd_usrp_get_rx_stream(rx_usrp, &stream_args, rx_streamer); // EXECUTE_OR_GOTO(free_rx_streamer
    if(status != UHD_ERROR_NONE){
      fprintf(stderr, "[SDR TXRX] Error opening TX stream: %d\n", status);
      return -1; 
    }
	
	// Issue stream command
    fprintf(stderr, "[SDR RX] Issuing stream command.\n");
    
    uhd_stream_cmd_t stream_cmd;
    stream_cmd.stream_mode = (total_num_samps == 0) ? UHD_STREAM_MODE_START_CONTINUOUS : UHD_STREAM_MODE_NUM_SAMPS_AND_DONE;
    stream_cmd.num_samps   = total_num_samps;
    stream_cmd.stream_now  = true;
    
    status = uhd_rx_streamer_issue_stream_cmd(rx_streamer, &stream_cmd);
	

	// Create TX metadata
    uhd_tx_metadata_handle tx_metadata;
	bool has_time_spec = false;
    int64_t full_secs = 0;
    double frac_secs = 0.1;
	bool start_of_burst = true;
    bool end_of_burst = false;
    status = uhd_tx_metadata_make(&tx_metadata, has_time_spec, full_secs, frac_secs, start_of_burst, end_of_burst); //EXECUTE_OR_GOTO(free_tx_streamer,
    if(status != UHD_ERROR_NONE){ return -1; }
	
	// Create RX metadata
    uhd_rx_metadata_handle rx_metadata;
    status = uhd_rx_metadata_make(&rx_metadata); //EXECUTE_OR_GOTO(free_rx_streamer,
	if(status != UHD_ERROR_NONE){ return -1; }
	
	
	// Buffers
	
	size_t BUFF_SZ_TX;
    status = uhd_tx_streamer_max_num_samps(tx_streamer, &BUFF_SZ_TX); //EXECUTE_OR_GOTO(free_tx_streamer,
	if(status != UHD_ERROR_NONE){ return -1; }
	
	size_t BUFF_SZ_RX;
    status = uhd_rx_streamer_max_num_samps(rx_streamer, &BUFF_SZ_RX); //EXECUTE_OR_GOTO(free_rx_streamer,
	if(status != UHD_ERROR_NONE){ return -1; }
	
	size_t BUFFER_SIZE = (BUFF_SZ_TX>BUFF_SZ_RX) ? BUFF_SZ_RX : BUFF_SZ_TX;
	
	uint8_t* uBufferIQ_TX  = calloc(sizeof(uint8_t), BUFFER_SIZE * 2);
	short* iBufferIQ_TX  = calloc(sizeof(short), BUFFER_SIZE * 2);
	float* fBufferIQ_TX  = calloc(sizeof(float), BUFFER_SIZE * 2);
	double* dBufferIQ_TX  = calloc(sizeof(double), BUFFER_SIZE * 2);
    
    
	const void** uBufferIQ_ptr_TX = (const void**)&uBufferIQ_TX;
	const void** iBufferIQ_ptr_TX = (const void**)&iBufferIQ_TX;
    const void** fBufferIQ_ptr_TX = (const void**)&fBufferIQ_TX;
    const void** dBufferIQ_ptr_TX = (const void**)&dBufferIQ_TX;
	
	
	uint8_t* uBufferIQ_RX  = malloc(BUFFER_SIZE* 2*sizeof(uint8_t));
	short* iBufferIQ_RX  = malloc(BUFFER_SIZE* 2*sizeof(short));
	float* fBufferIQ_RX  = malloc(BUFFER_SIZE* 2*sizeof(float));
	double* dBufferIQ_RX  = malloc(BUFFER_SIZE* 2*sizeof(double));
    
    
	void** uBufferIQ_ptr_RX = (void**)&uBufferIQ_RX;
	void** iBufferIQ_ptr_RX = (void**)&iBufferIQ_RX;
    void** fBufferIQ_ptr_RX = (void**)&fBufferIQ_RX;
    void** dBufferIQ_ptr_RX = (void**)&dBufferIQ_RX;
	
	
	
	
	
	// Actual streaming
	float timeout_tx = 0.1;
	
	float settling = 0.2; float timeout_rx = settling + 0.1; //expected settling time + padding for first recv
	bool overflow_message_rx = true;
	
	m_keep_running = (status == UHD_ERROR_NONE);
	
	uint64_t num_acc_samps = 0;
	
	size_t num_tx_samps  = 0;
    size_t nRead = 0;
	size_t num_rx_samps = 0;
	size_t nWrite = 0;
	
	while(m_keep_running){
        
        if(total_num_samps > 0 && num_acc_samps >= total_num_samps){ break; }
		
		// Fill Tx Buffer
		switch(dataTypeIn){
			case FMT_U8:{ nRead = fread(uBufferIQ_TX, sizeof(uint8_t),2*BUFFER_SIZE,input_fid); }break;
			case FMT_I16:{ nRead = fread(iBufferIQ_TX, sizeof(short),2*BUFFER_SIZE,input_fid); }break;
			case FMT_FLOAT:{ nRead = fread(fBufferIQ_TX, sizeof(float),2*BUFFER_SIZE,input_fid); }break;
			case FMT_DOUBLE:{ nRead = fread(dBufferIQ_TX, sizeof(double),2*BUFFER_SIZE,input_fid); }break;
		}
        if(nRead<(2*BUFFER_SIZE)){
          if(nRead>0){ fprintf(stderr, "%s[SDR TX] Incomplete buffer %ld/%ld\n%s",YELLOW,nRead,(2*BUFFER_SIZE),RESET); }
          else{ break; }			  			  
        }
		
		switch(dataTypeIn){
			case FMT_U8:{ status = uhd_tx_streamer_send(tx_streamer, uBufferIQ_ptr_TX, BUFFER_SIZE, &tx_metadata, timeout_tx, &num_tx_samps); }break;
			case FMT_I16:{ status = uhd_tx_streamer_send(tx_streamer, iBufferIQ_ptr_TX, BUFFER_SIZE, &tx_metadata, timeout_tx, &num_tx_samps); }break;
			case FMT_FLOAT:{ status = uhd_tx_streamer_send(tx_streamer, fBufferIQ_ptr_TX, BUFFER_SIZE, &tx_metadata, timeout_tx, &num_tx_samps); }break;
			case FMT_DOUBLE:{ status = uhd_tx_streamer_send(tx_streamer, dBufferIQ_ptr_TX, BUFFER_SIZE, &tx_metadata, timeout_tx, &num_tx_samps); }break;
		}
		if(status != UHD_ERROR_NONE){ break; }
		
		if(num_tx_samps<=0){ fprintf(stderr,"%s[SDR TX] Error transmit symbols from USRP, quit \n%s",RED,RESET); break; }
        if(num_tx_samps!=BUFFER_SIZE){ fprintf(stderr,"%s[SDR RX] error: received samples: %ld/%ld \n%s",YELLOW,num_rx_samps,BUFFER_SIZE,RESET); }
		
		fprintf(stderr, "%s[SDR TX] Sent %zu samples\n%s", GREEN,num_tx_samps,RESET);
		
		// ====================================
	
		switch(dataTypeIn){
			case FMT_U8:{ status = uhd_rx_streamer_recv(rx_streamer, uBufferIQ_ptr_RX, BUFFER_SIZE, &rx_metadata, timeout_rx, false, &num_rx_samps); }break;
			case FMT_I16:{ status = uhd_rx_streamer_recv(rx_streamer, iBufferIQ_ptr_RX, BUFFER_SIZE, &rx_metadata, timeout_rx, false, &num_rx_samps); }break;
			case FMT_FLOAT:{ status = uhd_rx_streamer_recv(rx_streamer, fBufferIQ_ptr_RX, BUFFER_SIZE, &rx_metadata, timeout_rx, false, &num_rx_samps); }break;
			case FMT_DOUBLE:{ status = uhd_rx_streamer_recv(rx_streamer, dBufferIQ_ptr_RX, BUFFER_SIZE, &rx_metadata, timeout_rx, false, &num_rx_samps); }break;
		}
		if(status != UHD_ERROR_NONE){ break; }
		
		if(num_rx_samps<=0){ fprintf(stderr,"%s[SDR RX] Error received symbols from LimeSDR, quit \n%s",RED,RESET); break; }
        if(num_rx_samps!=BUFFER_SIZE){ fprintf(stderr,"%s[SDR RX] error: received samples: %ld/%ld \n%s",YELLOW,num_rx_samps,BUFFER_SIZE,RESET); }
		
		timeout_rx = 0.1;   //small timeout for subsequent recv
		
		uhd_rx_metadata_error_code_t error_code;
		uhd_rx_metadata_error_code(rx_metadata, &error_code);
		
		if (error_code == UHD_RX_METADATA_ERROR_CODE_TIMEOUT) {
			fprintf(stderr,"%s[SDR RX] Timeout while streaming.\n%s",RED,RESET);
			break;
		}
		
		if (error_code == UHD_RX_METADATA_ERROR_CODE_OVERFLOW) {
			if (overflow_message_rx){
				overflow_message_rx = false;				
				fprintf(stderr,"%s[SDR RX] Got an overflow indication. Please consider the following:\n Your write medium must sustain a rate of %fMB/s.\nDropped samples will not be written to the file.\nPlease modify this example for your purposes.\nThis message will not appear again.\n%s",YELLOW,m_SampleRate/1e6,RESET);             
			}
			continue;
		}
		
		if (error_code != UHD_RX_METADATA_ERROR_CODE_NONE) {
			fprintf(stderr,"%s[SDR RX] Error code 0x%x was returned during streaming. Aborting.\n%s",RED,error_code,RESET);
			break;
		}
		
		// Handle data
		switch(dataTypeIn){
			case FMT_U8:{ nWrite = fwrite(uBufferIQ_RX, sizeof(uint8_t) * 2, num_rx_samps, output_fid); }break;
			case FMT_I16:{ nWrite = fwrite(iBufferIQ_RX, sizeof(short) * 2, num_rx_samps, output_fid); }break;
			case FMT_FLOAT:{ nWrite = fwrite(fBufferIQ_RX, sizeof(float) * 2, num_rx_samps, output_fid); }break;
			case FMT_DOUBLE:{ nWrite = fwrite(dBufferIQ_RX, sizeof(double) * 2, num_rx_samps, output_fid); }break;
		}
        if(nWrite<num_rx_samps){ fprintf(stderr,"%s[SDR RX] Error writing data, quit \n%s",RED,RESET); break; }
		
		
		
		
		int64_t full_secs;
        double frac_secs;
        uhd_rx_metadata_time_spec(rx_metadata, &full_secs, &frac_secs);
        fprintf(stderr,"%s[SDR RX] Received packet: %zu samples, %.f full secs, %f frac secs\n%s",GREEN,num_rx_samps,difftime(full_secs, (int64_t)0),frac_secs,RESET);
		
		num_acc_samps += num_tx_samps;
		num_acc_samps += num_rx_samps;	
		
	}

    
	fclose(input_fid);
    fclose(output_fid);
	
	// Delete
	//****************************************************
	uhd_tx_metadata_free(&tx_metadata);
	uhd_tx_streamer_free(&tx_streamer);
	uhd_usrp_free(&tx_usrp);
	
	uhd_rx_metadata_free(&rx_metadata);
	uhd_rx_streamer_free(&rx_streamer);
	uhd_usrp_free(&rx_usrp);
	//****************************************************

    return EXIT_SUCCESS;
}