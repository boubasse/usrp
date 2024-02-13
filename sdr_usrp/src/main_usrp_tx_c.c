
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
	
	char tx_subdev_str[64] = {0};
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
		
		a = getopt(argc, argv, "i:s:b:f:g:t:h:");
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
							fprintf(stderr, "[SDR TX] Unable to open '%s': %s\n",optarg, strerror(errno));
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
					if(isprint(optopt)){ fprintf(stderr, "[SDR TX] limetx `-%c'.\n", optopt); }
					else               { fprintf(stderr, "[SDR TX] limetx: unknown option character `\\x%x'.\n", optopt); }
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
    if(isapipe){ fprintf(stderr, "[SDR TX] Using IQ live mode\n"); }
    else       { fprintf(stderr, "[SDR TX] Read from data File\n"); }	
    
    if(m_CenterFrequency<=0){ fprintf(stderr, "[SDR TX] Need set a frequency to tx\n"); exit(0);}
    if(m_SampleRate<=0){ fprintf(stderr, "[SDR TX] Need set a SampleRate \n"); exit(0);}
    if(m_gainRatio<=0){ fprintf(stderr, "[SDR TX] Need set a Gain \n"); exit(0);}
       
	   
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
        fprintf(stderr, "[SDR TX] devices[%li]: %s \n",i,buff_str);
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
    
    
    // Set transmitter subdevice spec string
    char *tx_subdev_ptr = strstr(device_args, "tx_subdev_spec=");
    if (tx_subdev_ptr) {
      //copy_subdev_string(tx_subdev_str, tx_subdev_ptr + strlen(tx_subdev_arg));
      //remove_substring(device_args, "tx_subdev_spec=");
    }
    
    // Check external clock argument
    if (strstr(device_args, "clock=external")) {
      //REMOVE_SUBSTRING_WITHCOMAS(device_args, "clock=external");
      clock_src = EXTERNAL;
    } else if (strstr(device_args, "clock=gpsdo")) {
      fprintf(stderr,"[SDR TX] Using GPSDO clock\n");
      //REMOVE_SUBSTRING_WITHCOMAS(device_args, "clock=gpsdo");
      clock_src = GPSDO;
    } else {
      clock_src = DEFAULT;
    }
    

// ===============================================================================
   size_t mboard = 0;

   fprintf(stderr, "[SDR TX] Device args: %s \n",device_args);
   
   uhd_error status;
   
   // UHD_API uhd_error uhd_usrp_get_pp_string(uhd_usrp_handle h, char* pp_string_out, size_t strbuffer_len);
   // UHD_API uhd_error uhd_usrp_get_mboard_name(uhd_usrp_handle h, size_t mboard, char* mboard_name_out, size_t strbuffer_len); (vv)
   // UHD_API uhd_error uhd_usrp_get_tx_subdev_name(uhd_usrp_handle h, size_t chan, char* tx_subdev_name_out, size_t strbuffer_len);
   // UHD_API uhd_error uhd_usrp_get_tx_num_CHANs(uhd_usrp_handle h, size_t* num_CHANs_out);
   // UHD_API uhd_error uhd_usrp_get_tx_lo_names(uhd_usrp_handle h, size_t chan, uhd_string_vector_handle* tx_lo_names_out);
   // UHD_API uhd_error uhd_usrp_get_tx_lo_source(uhd_usrp_handle h,const char* name,size_t chan,char* tx_lo_source_out,size_t strbuffer_len);
   // UHD_API uhd_error uhd_usrp_get_tx_lo_sources(uhd_usrp_handle h,const char* name,size_t chan,uhd_string_vector_handle* tx_lo_sources_out);
    
    
    
    /* Create UHD handler */
    fprintf(stderr,"[SDR TX] Opening USRP with args: %s\n", device_args);
    uhd_usrp_handle tx_usrp;
    status = uhd_usrp_make(&tx_usrp, device_args); //EXECUTE_OR_GOTO(free_option_strings
    if(status != UHD_ERROR_NONE){
      fprintf(stderr, "[SDR TX] Error opening UHD: code %d\n", status);
      tx_usrp = NULL;
      return -1; 
    }
    
    if(tx_usrp != NULL){ fprintf(stderr, "[SDR TX] Device is detected with success ....\n"); }
    
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
   
   /* has rssi ? */
    uhd_string_vector_handle tx_sensors;  
    uhd_string_vector_make(&tx_sensors);
    uhd_usrp_get_tx_sensor_names(tx_usrp, 0, &tx_sensors);
    bool has_rssi = find_string(tx_sensors, "rssi"); 
    uhd_string_vector_free(&tx_sensors);
    
    fprintf(stderr, "[SDR TX] usrp has_rssi = %d...\n", has_rssi);
    
    
    // =================================================================
	
	
    
    // Get USRP RF Characteristics
    uhd_meta_range_handle tx_metric_range = NULL;
    uhd_meta_range_make(&tx_metric_range);
	double min_value, max_value,step_value;
	
	status = uhd_usrp_get_fe_tx_freq_range(tx_usrp, CHAN, tx_metric_range);          
    status = uhd_meta_range_start(tx_metric_range, &min_value);
    status = uhd_meta_range_stop(tx_metric_range, &max_value);
	status = uhd_meta_range_step(tx_metric_range, &step_value);
    fprintf(stderr, "[SDR TX] usrp DAC Frequency range: %2.3f to %2.3f step %2.3f...\n", min_value,max_value,step_value);
	
	status = uhd_usrp_get_tx_rates(tx_usrp, CHAN, tx_metric_range);          
    status = uhd_meta_range_start(tx_metric_range, &min_value);
    status = uhd_meta_range_stop(tx_metric_range, &max_value);
	status = uhd_meta_range_step(tx_metric_range, &step_value);
    fprintf(stderr, "[SDR TX] usrp Sample Rate range: %2.3f to %2.3f step %2.3f...\n", min_value,max_value,step_value);
	
	status = uhd_usrp_get_tx_freq_range(tx_usrp, CHAN, tx_metric_range);           
    status = uhd_meta_range_start(tx_metric_range, &min_value);
    status = uhd_meta_range_stop(tx_metric_range, &max_value);
	status = uhd_meta_range_step(tx_metric_range, &step_value);
    fprintf(stderr, "[SDR TX] usrp Center Frequency range: %2.3f to %2.3f step %2.3f...\n", min_value,max_value,step_value);
	
	status = uhd_usrp_get_tx_bandwidth_range(tx_usrp, CHAN, tx_metric_range);          
    status = uhd_meta_range_start(tx_metric_range, &min_value);
    status = uhd_meta_range_stop(tx_metric_range, &max_value);
	status = uhd_meta_range_step(tx_metric_range, &step_value);
    fprintf(stderr, "[SDR TX] usrp Bandwidth range: %2.3f to %2.3f step %2.3f...\n", min_value,max_value,step_value);
	if(m_Bandwidth==0){ m_Bandwidth = 1.4*m_SampleRate; if(m_Bandwidth>max_value){ m_Bandwidth = max_value; } }
	
    status = uhd_usrp_get_tx_gain_range(tx_usrp, "", CHAN, tx_metric_range);          
    status = uhd_meta_range_start(tx_metric_range, &min_value);
    status = uhd_meta_range_stop(tx_metric_range, &max_value);
	status = uhd_meta_range_step(tx_metric_range, &step_value);
	double max_gain = max_value;
    fprintf(stderr, "[SDR TX] usrp Gain range: %2.3f to %2.3f step %2.3f...\n", min_value,max_value,step_value);
	
	//uhd_string_vector_handle antennas_out;
	//uhd_string_vector_make(&antennas_out);
	//status = uhd_usrp_get_tx_antennas(tx_usrp, CHAN, &antennas_out);
	//status = uhd_usrp_get_clock_sources(rx_usrp, mboard, &antennas_out);
	//uhd_string_vector_free(&antennas_out);
	
    uhd_meta_range_free(&tx_metric_range);
	
	
	// =================================================================
    
    // Set transmitter subdev spec if specified
    if (strlen(tx_subdev_str)) {
      fprintf(stderr,"[SDR TX] Setting tx_subdev_spec to '%s'\n", tx_subdev_str);
	  
      uhd_subdev_spec_handle subdev_spec_handle = {0};
      uhd_subdev_spec_make(&subdev_spec_handle, tx_subdev_str);
      status = uhd_usrp_set_tx_subdev_spec(tx_usrp, subdev_spec_handle, mboard);
      uhd_subdev_spec_free(&subdev_spec_handle);
    }
    else{ fprintf(stderr,"[SDR TX] tx_subdev_spec setting is not specified \n"); }
	
	/* uhd_subdev_spec_handle subdev_spec_out = {0};
	status = uhd_usrp_get_tx_subdev_spec(tx_usrp,mboard, subdev_spec_out);
	fprintf(stderr, "[SDR TX] Actual tx_subdev_spec: %s...\n", subdev_spec_out);
	uhd_subdev_spec_free(&subdev_spec_out); */

    // Set external clock reference
	fprintf(stderr, "[SDR TX] Setting TX Time Source: ...\n");
    if (clock_src == EXTERNAL)   { status = uhd_usrp_set_clock_source(tx_usrp, "external", mboard); } 
    else if (clock_src == GPSDO) { status = uhd_usrp_set_clock_source(tx_usrp, "gpsdo", mboard); }
    else{ fprintf(stderr,"[SDR TX] clock source is set to default'\n"); }
	
	/* char time_source_out[128];
	status = uhd_usrp_get_time_source(tx_usrp, mboard, time_source_out, 128);
	fprintf(stderr, "[SDR TX] Actual TX Time Source: %s...\n", time_source_out); */
	
	// Set Master Clock
	if (current_master_clock>0){
		fprintf(stderr, "[SDR TX] Setting master clock rate: %f...\n", current_master_clock);
		status = uhd_usrp_set_master_clock_rate(tx_usrp,current_master_clock,mboard);
	}
	else{ fprintf(stderr,"[SDR TX] master clock rate setting is not specified \n"); }
	
	status = uhd_usrp_get_master_clock_rate(tx_usrp,mboard,&current_master_clock);
	fprintf(stderr, "[SDR TX] Actual master clock rate: %f...\n", current_master_clock);
  
    // Set Sample Rate
    fprintf(stderr, "[SDR TX] Setting TX Rate: %f...\n", m_SampleRate);
    status = uhd_usrp_set_tx_rate(tx_usrp, m_SampleRate, CHAN); //EXECUTE_OR_GOTO(free_tx_metadata,
    status = uhd_usrp_get_tx_rate(tx_usrp, CHAN, &m_SampleRate); // EXECUTE_OR_GOTO(free_tx_metadata,
    fprintf(stderr, "[SDR TX] Actual TX Rate: %f...\n", m_SampleRate);
    
	// Set center Frequency
    uhd_tune_request_t tune_request = {
        .target_freq     = m_CenterFrequency,
        .rf_freq_policy  = UHD_TUNE_REQUEST_POLICY_AUTO,
        .dsp_freq_policy = UHD_TUNE_REQUEST_POLICY_AUTO,
    };
    uhd_tune_result_t tune_result;
    fprintf(stderr, "[SDR TX] Setting TX frequency: %f MHz...\n", m_CenterFrequency / 1e6);
    status = uhd_usrp_set_tx_freq(tx_usrp, &tune_request, CHAN, &tune_result);// EXECUTE_OR_GOTO(free_tx_metadata,
    status = uhd_usrp_get_tx_freq(tx_usrp, CHAN, &m_CenterFrequency);// EXECUTE_OR_GOTO(free_tx_metadata,
    fprintf(stderr, "[SDR TX] Actual TX frequency: %f MHz...\n", m_CenterFrequency / 1e6);
    
    // Set gain
    double gain = max_gain*m_gainRatio;
    fprintf(stderr, "[SDR TX] Setting TX Gain: %f dB...\n", gain);
    status = uhd_usrp_set_tx_gain(tx_usrp, gain, CHAN, ""); // EXECUTE_OR_GOTO(free_tx_metadata,
    status = uhd_usrp_get_tx_gain(tx_usrp, CHAN, "", &gain); // EXECUTE_OR_GOTO(free_tx_metadata,
    fprintf(stderr, "[SDR TX] Actual TX Gain: %f...\n", gain);
    
    //set the IF filter bandwidth
	fprintf(stderr, "[SDR TX] Setting RX Bandwidth: %f...\n", m_Bandwidth);
    status = uhd_usrp_set_tx_bandwidth(tx_usrp, m_Bandwidth, CHAN); //EXECUTE_OR_GOTO(free_tx_metadata,
	status = uhd_usrp_get_tx_bandwidth(tx_usrp, CHAN, &m_Bandwidth); // EXECUTE_OR_GOTO(free_tx_metadata,
	fprintf(stderr, "[SDR TX] Actual RX Bandwidth: %f...\n", m_Bandwidth);

	
    //set the antenna 
	/*char* AntLabel = "RX1";
	fprintf(stderr, "[SDR TX] Setting RX Antenna: %s...\n", AntLabel);
    status = uhd_usrp_set_tx_antenna(tx_usrp, AntLabel, CHAN);
	size_t strbuffer_len = 20;
	status = uhd_usrp_get_tx_antenna(tx_usrp, CHAN, AntLabel, strbuffer_len);
	fprintf(stderr, "[SDR TX] Actual RX Antenna: %s...\n", AntLabel);*/
		
	// =================================================================

    // Create TX streamer
    uhd_tx_streamer_handle tx_streamer;
    status = uhd_tx_streamer_make(&tx_streamer); //EXECUTE_OR_GOTO(free_usrp,
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
      fprintf(stderr, "[SDR TX] Error opening TX stream: %d\n", status);
      return -1; 
    }
    
    // Set up buffer
    size_t BUFFER_SIZE;
    status = uhd_tx_streamer_max_num_samps(tx_streamer, &BUFFER_SIZE); //EXECUTE_OR_GOTO(free_tx_streamer,
	if(status != UHD_ERROR_NONE){ return -1; }
    fprintf(stderr, "[SDR TX] Buffer size in samples: %zu\n", BUFFER_SIZE);
    
	
	uint8_t* uBufferIQ  = calloc(sizeof(uint8_t), BUFFER_SIZE * 2);
	short* iBufferIQ  = calloc(sizeof(short), BUFFER_SIZE * 2);
	float* fBufferIQ  = calloc(sizeof(float), BUFFER_SIZE * 2);
	double* dBufferIQ  = calloc(sizeof(double), BUFFER_SIZE * 2);
    
    
	const void** uBufferIQ_ptr = (const void**)&uBufferIQ;
	const void** iBufferIQ_ptr = (const void**)&iBufferIQ;
    const void** fBufferIQ_ptr = (const void**)&fBufferIQ;
    const void** dBufferIQ_ptr = (const void**)&dBufferIQ;
    
    // Create TX metadata
    uhd_tx_metadata_handle tx_metadata;
	bool has_time_spec = false;
    int64_t full_secs = 0;
    double frac_secs = 0.1;
	bool start_of_burst = true;
    bool end_of_burst = false;
    status = uhd_tx_metadata_make(&tx_metadata, has_time_spec, full_secs, frac_secs, start_of_burst, end_of_burst); //EXECUTE_OR_GOTO(free_tx_streamer,
    if(status != UHD_ERROR_NONE){ return -1; }
	
    // ===========================
	
	//reset usrp time to prepare for transmit/receive
	// std::cout << boost::format("Setting device timestamp to 0...") << std::endl;
	// uhd_tx_usrp_set_time_now(uhd::time_spec_t(0.0));
    
     
	float timeout = 0.1;
	 
    // Actual streaming
    m_keep_running = (status == UHD_ERROR_NONE);
    
	uint64_t num_acc_samps = 0;
    size_t num_tx_samps  = 0;
    size_t nRead = 0;
    while (m_keep_running) {

        if(total_num_samps > 0 && num_acc_samps >= total_num_samps){ break; }
		
        // Fill Tx Buffer
		switch(dataTypeIn){
			case FMT_U8:{ nRead = fread(uBufferIQ, sizeof(uint8_t),2*BUFFER_SIZE,input_fid); }break;
			case FMT_I16:{ nRead = fread(iBufferIQ, sizeof(short),2*BUFFER_SIZE,input_fid); }break;
			case FMT_FLOAT:{ nRead = fread(fBufferIQ, sizeof(float),2*BUFFER_SIZE,input_fid); }break;
			case FMT_DOUBLE:{ nRead = fread(dBufferIQ, sizeof(double),2*BUFFER_SIZE,input_fid); }break;
		}
        if(nRead<(2*BUFFER_SIZE)){
			continue;
          if(nRead>0){ fprintf(stderr, "%s[SDR TX] Incomplete buffer %ld/%ld\n%s",YELLOW,nRead,(2*BUFFER_SIZE),RESET); }
          else{ break; }			  			  
        }
        
		switch(dataTypeIn){
			case FMT_U8:{ status = uhd_tx_streamer_send(tx_streamer, uBufferIQ_ptr, BUFFER_SIZE, &tx_metadata, timeout, &num_tx_samps); }break;
			case FMT_I16:{ status = uhd_tx_streamer_send(tx_streamer, iBufferIQ_ptr, BUFFER_SIZE, &tx_metadata, timeout, &num_tx_samps); }break;
			case FMT_FLOAT:{ status = uhd_tx_streamer_send(tx_streamer, fBufferIQ_ptr, BUFFER_SIZE, &tx_metadata, timeout, &num_tx_samps); }break;
			case FMT_DOUBLE:{ status = uhd_tx_streamer_send(tx_streamer, dBufferIQ_ptr, BUFFER_SIZE, &tx_metadata, timeout, &num_tx_samps); }break;
		}
		if(status != UHD_ERROR_NONE){ break; }
		
		if(num_tx_samps<=0){ fprintf(stderr,"%s[SDR TX] Error transmit symbols from USRP, quit \n%s",RED,RESET); break; }
        if(num_tx_samps!=BUFFER_SIZE){ fprintf(stderr,"%s[SDR RX] error: received samples: %ld/%ld \n%s",YELLOW,num_tx_samps,BUFFER_SIZE,RESET); }
        
        fprintf(stderr, "%s[SDR TX] Sent %zu samples\n%s", GREEN,num_tx_samps,RESET);

        num_acc_samps += num_tx_samps;

    }
    
	fclose(input_fid);
	
	// Delete
	//****************************************************
	uhd_tx_metadata_free(&tx_metadata);
	uhd_tx_streamer_free(&tx_streamer);
	uhd_usrp_free(&tx_usrp);
	//****************************************************

    return EXIT_SUCCESS;
}
