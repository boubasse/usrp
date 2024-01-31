
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <getopt.h>
#include <signal.h>
#include <errno.h>
#include <stdbool.h>

#include <unistd.h>
#include <vector>
#include <complex>

#include "windowLibrary.h"
#include "fftLibrary.h"

#define PROGRAM_VERSION "0.0.1"

#define FFT_SIZE 2048


static constexpr float PI = 3.14159265358979323846f;

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



enum FMT_DATA_TYPE {
	FMT_U8 = 0,
	FMT_S16,
	FMT_FLOAT
};

enum FFT_WINDOW_TYPE {
	FFT_WINDOW_RECTANGULAR = 0  ,
	FFT_WINDOW_TRIANGULAR       ,
	FFT_WINDOW_HAMMING          ,
	FFT_WINDOW_HANNING          ,
	FFT_WINDOW_BLACKMAN         ,
	FFT_WINDOW_NUTTALL          ,
	FFT_WINDOW_BLACKMAN_NUTTALL ,
	FFT_WINDOW_BLACKMAN_HARRIS  ,
	FFT_WINDOW_FLAT_TOP_WINDOW  ,
	FFT_WINDOW_COSINE_WINDOW    ,
	FFT_WINDOW_WELCH            ,
	FFT_WINDOW_GAUSSIAN         ,
	FFT_WINDOW_BARLETT_HANN     ,
	FFT_WINDOW_LANCZOS          ,
	FFT_WINDOW_BOHMAN           ,
	FFT_WINDOW_TURKEY           ,
	FFT_WINDOW_KAISER           ,
};

enum SPECTRUM_SHOW_MODE {
	SPECTRUM_ANALYZER_MODE_CURRENT = 0  ,
	SPECTRUM_ANALYZER_MODE_MIN_HOLD     ,
	SPECTRUM_ANALYZER_MODE_MAX_HOLD     ,
	SPECTRUM_ANALYZER_MODE_AVERAGE      ,
};


FILE *input_fid;

static bool m_keep_running=false;

static float bessifunc0(float x){
        
	double ax = fabs(x);       
	double y;
	float ans;
	 
	if(ax < 3.75) {
		y=ax/3.75;  y*=y;
		ans=1.0+y*(3.5156229+y*(3.0899424+y*(1.2067492
			+y*(0.2659732+y*(0.360768e-1+y*0.45813e-2)))));
	} else{
		y=3.75/ax;
		ans=(exp(ax)/sqrt(ax))*(0.39894228+y*(0.1328592e-1
			+y*(0.225319e-2+y*(-0.157565e-2+y*(0.916281e-2
			+y*(-0.2057706e-1+y*(0.2635537e-1+y*(-0.1647633e-1
			+y*0.392377e-2))))))));
	}
	return ans;
}
		    

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
"kisspectrum -%s\n\
Usage:kisspectrum -i File [-t IQType] [-s SampleRate] [-r FrameRate][-h] \n\
-i            Input IQ File I16 format (default stdin) \n\
-t            IQType {float,u8,i16} (default float) \n\
-s            SampleRate (default 1024K) \n\
-r            Display Framertae (default 25) \n\
-h            help (print this help).\n\
Example : .\\kisspectrum -i - \n\
\n", \
PROGRAM_VERSION);

} /* end function print_usage */

    
int main(int argc, char *argv[]) {

    input_fid = stdin;
	unsigned int SampleRate=1024000;
	unsigned int fps = 25;
	int dataType = FMT_FLOAT;
    
    int a;
	bool anyargs = false;
    while(1){
		
		a = getopt(argc, argv, "i:t:s:r:h");
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
						fprintf(stderr, "[KISSPEC] Unable to open '%s': %s\n",optarg, strerror(errno));
						exit(EXIT_FAILURE);
					}
				}
				}break;
			case 't':{ // Input Type
					if (strcmp("float", optarg) == 0){ dataType = FMT_FLOAT; }
					if (strcmp("s16", optarg) == 0)  { dataType = FMT_S16;   }
					if (strcmp("u8", optarg) == 0)   { dataType = FMT_U8;    }
				}break;
			case 's':{ // SampleRate in Symbol/s [Kbaus/s]
					SampleRate = atoi(optarg);  
				}break;
			case 'r':{ // Frame/s 
					fps = atoi(optarg);
					if(fps>25){ fps=25; } //Fixme should be teh framerate of screen mode  
				}break;
			case 'h':{ // help
				print_usage();
				exit(0);
				}break;
			case -1:{
				}break;
			case '?':{
					if(isprint(optopt)){ fprintf(stderr, "[KISSPEC] kisspectrum `-%c'.\n", optopt); }
					else               { fprintf(stderr, "[KISSPEC] kisspectrum: unknown option character `\\x%x'.\n", optopt); }
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
	if(isapipe){ fprintf(stderr, "[KISSPEC] Using IQ live mode\n"); }
	else       { fprintf(stderr, "[KISSPEC] Read from data File\n"); }
	
	if(SampleRate == 0){ fprintf(stderr, "[KISSPEC] Need set a SampleRate \n"); exit(0); }
	
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
    //int FFTWindowType = FFT_WINDOW_RECTANGULAR;
    //int spectrumAnalyzerMode = SPECTRUM_ANALYZER_MODE_CURRENT;
    
	int FFTWindowType = FFT_WINDOW_BLACKMAN_HARRIS;	
	int spectrumAnalyzerMode = SPECTRUM_ANALYZER_MODE_AVERAGE;
		
	if(FFT_SIZE & (FFT_SIZE-1)){ fprintf(stderr,"[KISSPEC] FFT size (error) - FFT size is not a multiple of 2 \n"); exit(-1); } // Check that FFT size is 2^N
     
    // -------------------------------------------------------------------------------------------------
	float intermediateResult[FFT_SIZE];
	float FFTwindow[FFT_SIZE];

	switch(FFTWindowType){
		case FFT_WINDOW_RECTANGULAR: {
				for(unsigned int n=0; n<FFT_SIZE; n++){
					FFTwindow[n] = 1;
				}
			}break;
		case FFT_WINDOW_TRIANGULAR: { // Also known as Bartlett or Fejér window
				float midSize = 0.5*(FFT_SIZE-1);
				for(unsigned int n=0; n<FFT_SIZE; n++){
					FFTwindow[n] = 1-fabs((n-midSize)/midSize);
				}
			}break;
		case FFT_WINDOW_HAMMING: {
				for(unsigned int n=0; n<FFT_SIZE; n++){
					FFTwindow[n] = 0.54-0.46*cos(2.0*PI*n/(FFT_SIZE-1));
				}
			}break;;
		case FFT_WINDOW_HANNING: {
				for(unsigned int n=0; n<FFT_SIZE; n++){
					FFTwindow[n] = 0.5*(1.0-cos(2.0*PI*n/(FFT_SIZE-1)));
				}
			}break;
		case FFT_WINDOW_BLACKMAN: {
				for(unsigned int n=0; n<FFT_SIZE; n++){
					FFTwindow[n] = 0.42-0.5*cos(2.0*PI*n/(FFT_SIZE-1))+0.08*cos(4.0*PI*n/(FFT_SIZE-1));
				}
			}break;
		case FFT_WINDOW_NUTTALL: {
				for(unsigned int n=0; n<FFT_SIZE; n++){
					FFTwindow[n] = 0.355768-0.487396*cos(2.0*PI*n/(FFT_SIZE-1))+0.144232*cos(4.0*PI*n/(FFT_SIZE-1))-0.012604*cos(6.0*PI*n/(FFT_SIZE-1));
				}
			}break;
		case FFT_WINDOW_BLACKMAN_NUTTALL: {
				for(unsigned int n=0; n<FFT_SIZE; n++){
					FFTwindow[n] = 0.3635819-0.4891775*cos(2.0*PI*n/(FFT_SIZE-1))+0.1365995*cos(4.0*PI*n/(FFT_SIZE-1))-0.0106411*cos(6.0*PI*n/(FFT_SIZE-1));
				}
			}break;
		case FFT_WINDOW_BLACKMAN_HARRIS: {
				for(unsigned int n=0; n<FFT_SIZE; n++){
					FFTwindow[n] = 0.35875-0.48829*cos(2.0*PI*n/(FFT_SIZE-1))+0.14128*cos(4.0*PI*n/(FFT_SIZE-1))-0.01168*cos(6.0*PI*n/(FFT_SIZE-1));
				}
			}break;
		case FFT_WINDOW_FLAT_TOP_WINDOW: {
				for(unsigned int n=0; n<FFT_SIZE; n++){
					FFTwindow[n] = 1.0-1.93*cos(2.0*PI*n/(FFT_SIZE-1))+1.29*cos(4.0*PI*n/(FFT_SIZE-1))-0.388*cos(6.0*PI*n/(FFT_SIZE-1))+0.028*cos(8.0*PI*n/(FFT_SIZE-1));
				}
			}break;
		case FFT_WINDOW_COSINE_WINDOW: { // Also known as sine window
				for(unsigned int n=0; n<FFT_SIZE; n++){
					FFTwindow[n] = sin(PI*n/(FFT_SIZE-1));
				}
			}break;
		case FFT_WINDOW_WELCH: {
				float midSize = 0.5*(FFT_SIZE-1);
				for(unsigned int n=0; n<FFT_SIZE; n++){
					FFTwindow[n] = 1-pow((n-midSize)/midSize, 2);
				}
			}break;
		case FFT_WINDOW_GAUSSIAN: {
				float sigma = 0.4; // To be configured (sigma <= 0.5)
				float midSize = 0.5*(FFT_SIZE-1);
				for(unsigned int n=0; n<FFT_SIZE; n++){
					FFTwindow[n] = exp(-0.5*pow((n-midSize)/(sigma*midSize), 2));
				}
			}break;
		case FFT_WINDOW_BARLETT_HANN: {
				for(unsigned int n=0; n<FFT_SIZE; n++){
					FFTwindow[n] = 0.62-0.48*fabs(n/(FFT_SIZE-1)-0.5)-0.38*cos(2*PI*n/(FFT_SIZE-1));
				}
			}break;
		case FFT_WINDOW_LANCZOS: {
				for(unsigned int n=0; n<FFT_SIZE; n++){
					float tmp = 2.0*n/(FFT_SIZE-1)-1.0;
					FFTwindow[n] = sin(PI*tmp)/(PI*tmp);
				}
			}break;
		case FFT_WINDOW_BOHMAN: {
				float midSize = 0.5*(FFT_SIZE-1);
				for(unsigned int n=0; n<FFT_SIZE; n++){
					float fx = fabs(2*(n-midSize)/(FFT_SIZE-1));
					FFTwindow[n] = (1-fx)*cos(PI*fx)+sin(PI*fx)/PI;
				}
			}break;
		case FFT_WINDOW_TURKEY: {
				float parameter = 0.25; // To be configured between 0 and 1
				float midSize = 0.5*(FFT_SIZE-1);
				float alphaN = parameter*midSize;
				for(unsigned int n=0; n<FFT_SIZE; n++){
					if(n<alphaN)
						FFTwindow[n] = 0.5*(1-cos(PI*n/alphaN));
					else if(n>FFT_SIZE-alphaN)
						FFTwindow[n] = 0.5*(1-cos(PI*(FFT_SIZE-1-n)/alphaN));
					else
						FFTwindow[n] = 1;
				}
			}break;
		case FFT_WINDOW_KAISER: {
				float alpha = 0.32; // To be configured higer than 0 (corresponds to beta = alpha*PI)
				float denum = bessifunc0(PI*alpha);
				for(unsigned int n=0; n<FFT_SIZE; n++){
					FFTwindow[n] = bessifunc0(PI*alpha*sqrt(1-pow(2.0*n/(FFT_SIZE-1)-1, 2)))/denum;
				}
			}break;
		default: {
				fprintf(stderr,"[KISSPEC] spectrumAnalyzer (error) - This windowing type is not implemented"); exit(-1);

			}break;
	}
	// Compute scaling factor
	float scalingFactor = 0;
	for(unsigned int i=0; i<FFT_SIZE; i++){ scalingFactor += (fabs(FFTwindow[i])*fabs(FFTwindow[i])); }
	scalingFactor /= (float)FFT_SIZE;
	// Normalize
	for(unsigned int i=0; i<FFT_SIZE; i++){ FFTwindow[i] /= sqrt(scalingFactor); }
	
	// -------------------------------------------------------------------------------------------------    
	
    // ==================================================================================
    std::vector<std::complex<float>> iqin(FFT_SIZE,0);
    float signalFFT_re[FFT_SIZE];    float signalFFT_im[FFT_SIZE];

    // ==================================================================================
    windowLibrary* windowLibraryObj = new windowLibrary();
    
    int x = 0;
    int y = 0;
    int width = 1280;
    int height = 720;
    
    windowLibraryObj->init("rawiq (iq)",x,y,width,height);
    
    width  = windowLibraryObj->m_width;
    height = windowLibraryObj->m_height;
    
    fprintf(stderr, "[KISSPEC] resolution = %dx%d \n",width,height);

    // ==================================================================================
    
    uint16_t rh = 64;
    uint16_t rw = 64;
    uint16_t sph = 24;
    uint16_t spw = 24;
   
    int OscilloPosX   = rw/2 + spw/2;
    int SpectrumPosX  = rw/2 + spw/2;
    
    int OscilloWidth  = (width-rw-spw);
    int SpectrumWidth = (width-rw-spw);
    
    int OscilloHeight = (height-rh-3*sph)/4;
    int SpectrumHeight = (height-rh-3*sph)/2;
    
    int OscilloIPosY   = rh/2 + sph/2;
    int OscilloQPosY   = OscilloIPosY + OscilloHeight + sph;
    int SpectrumPosY   = OscilloQPosY + OscilloHeight + sph;

    // ==================================================================================
	
	float K = 1.38064852*1e-23; // constante de Boltzman
    float T = 50; // temperature en Celsuis
    float N0_Hz = K*(T+273.15); // Densité spectrale de puissance de bruit thermique
    float N0_Hz_dBm = 10*log10(N0_Hz) + 30; // equal to -174 dBm/Hz for ambiant temperature

    float ENR = 82.0; // ENR: excess noise ratio [dB]
    float sigma2_n_dBm = (N0_Hz_dBm + 10*log10(SampleRate)) + ENR; // puissance de bruit thermique ajouté [dBm]
    float sigma2_n = pow(10,(sigma2_n_dBm -30)/10.0);
    //float sigma2_n = N0_Hz*SampleRate; // puissance de bruit thermique ajouté
	
	// -------------------------------------------------------------------------------------------------
	char StrPow[200];
	char sTitle[100];
    sprintf(sTitle,"HERMOD OSCILLO");
	
	float Fc = 1240e6;
    float alpha = 2.0;
    
	float Pe_dBm = 28.0;
	float Ge_dB = 10.0;
    float Gr_dB = 10.0;
       
    float Pe = pow(10,(Pe_dBm -30)/10.0);
    float Ge = pow(10,Ge_dB/10.0);
    float Gr = pow(10,Gr_dB/10.0);
	// -------------------------------------------------------------------------------------------------
		
    unsigned int TheoricFrameRate = (unsigned int)floor(SampleRate/FFT_SIZE);
    unsigned int ShowFrame = TheoricFrameRate/fps;
    if(ShowFrame<1){ ShowFrame = 1; }
    fprintf(stderr,"[KISSPEC] Warning displayed only 1 IQ buffer every %d\n",ShowFrame);  
    fprintf(stderr,"[KISSPEC] -TheoricFrameRate %d  -Showframe =%d\n",TheoricFrameRate,ShowFrame);
    
    int SkipSamples=(SampleRate-(FFT_SIZE*fps))/fps;
    SkipSamples=(SkipSamples/FFT_SIZE)*FFT_SIZE;
    if(SkipSamples<0){ SkipSamples = 0; }
    fprintf(stderr,"[KISSPEC] Skip Samples = %d\n",SkipSamples);
    
    // -------------------------------------------------------------------------------------------------

    
    
    m_keep_running = true;
    
    unsigned long long NumFrame = 0;
    while(m_keep_running){
	                
        int nRead=0; 
        switch(dataType){
            case FMT_U8:{
				u8cmplx iqin_u8[FFT_SIZE];           
                nRead=fread(iqin_u8,sizeof(u8cmplx),FFT_SIZE,input_fid);
                if(nRead<=0){ break; } 
                for(int i=0; i<nRead; i++){
                    iqin[i].real((iqin_u8[i].re - 127.5)/127.0);
                    iqin[i].imag((iqin_u8[i].im - 127.5)/127.0);             
                }   
            }break;
            case FMT_S16:{
				i16cmplx iqin_s16[FFT_SIZE];
				nRead = fread(iqin_s16, sizeof(i16cmplx),FFT_SIZE,input_fid);           
                if(nRead<=0){ break; } 
                for(int i=0; i<nRead; i++){
                    iqin[i].real(iqin_i16[i].re/32767.0);
                    iqin[i].imag(iqin_i16[i].im/32767.0);                  
                }
            }break;
            case FMT_FLOAT:{
				fcmplx iqin_f32[FFT_SIZE];         
                nRead=fread(iqin_f32,sizeof(fcmplx),FFT_SIZE,input_fid); 
                if(nRead<=0){ break; }
                for(int i=0; i<nRead; i++){
                    iqin[i].real(iqin_f32[i].re);
                    iqin[i].imag(iqin_f32[i].im);                
                }    
            }break;
        }
        
        if(nRead<=0){ break; }
		if(nRead<FFT_SIZE){
			if(nRead>0){ fprintf(stderr, "Incomplete buffer %d/%d\n",nRead,FFT_SIZE); }
			else{ break; }
		}
		else{

			// ===========================================================================================================			

			if((NumFrame%ShowFrame)==0){
			
			  // FFT			  
			  for(unsigned int i=0; i<FFT_SIZE; i++){
				 signalFFT_re[i] = iqin[i].real() * FFTwindow[i] ;   
			     signalFFT_im[i] = iqin[i].imag() * FFTwindow[i] ;  
			  }
			  
			  FFT_core(signalFFT_re,signalFFT_im,FFT_SIZE);
			  			
			  // ********************************************************************************************************
			  windowLibraryObj->clear();
			  
			  // Axes
			  windowLibraryObj->setfg(255, 128, 0);
			  
			  windowLibraryObj->line(OscilloPosX+SpectrumWidth/2,height-SpectrumPosY, OscilloPosX+SpectrumWidth/2,height-(SpectrumPosY+SpectrumHeight)); // vertical line
			  windowLibraryObj->line(OscilloPosX,height-(OscilloIPosY+OscilloHeight/2),OscilloPosX+OscilloWidth,height-(OscilloIPosY+OscilloHeight/2)); // horizontal line
			  windowLibraryObj->line(OscilloPosX,height-(OscilloQPosY+OscilloHeight/2),OscilloPosX+OscilloWidth,height-(OscilloQPosY+OscilloHeight/2)); // horizontal line
              
              windowLibraryObj->text(0, height-(SpectrumPosY+SpectrumHeight), "0.0");
			  windowLibraryObj->text(0, height-SpectrumPosY, "-110.0");
			  
			  windowLibraryObj->text(0, height-(OscilloQPosY+OscilloHeight), "+1.0");
			  windowLibraryObj->text(0, height-(OscilloQPosY+OscilloHeight/2), "0.0");
			  windowLibraryObj->text(0, height-OscilloQPosY, "-1.0");
			  
			  windowLibraryObj->text(0, height-(OscilloIPosY+OscilloHeight), "+1.0");
			  windowLibraryObj->text(0, height-(OscilloIPosY+OscilloHeight/2), "0.0");
			  windowLibraryObj->text(0, height-OscilloIPosY, "-1.0");
			  
			  windowLibraryObj->text(width-rw/2, height-(SpectrumPosY+SpectrumHeight/2), "DSP");
			  windowLibraryObj->text(width-rw/2, height-(OscilloQPosY+OscilloHeight/2), "Q_C");
			  windowLibraryObj->text(width-rw/2, height-(OscilloIPosY+OscilloHeight/2), "I_C");
			  
              // séparation 
			  windowLibraryObj->setfg(0, 255, 255);

			  windowLibraryObj->line(SpectrumPosX-spw/4,height-(SpectrumPosY+SpectrumHeight+sph/4),SpectrumPosX+SpectrumWidth+spw/4,height-(SpectrumPosY+SpectrumHeight+sph/4)); // horizontal line FFT+
			  windowLibraryObj->line(SpectrumPosX-spw/4,height-(SpectrumPosY-sph/4),SpectrumPosX+SpectrumWidth+spw/4,height-(SpectrumPosY-sph/4)); // horizontal line FFT-
			
			  windowLibraryObj->line(OscilloPosX-spw/4,height-(OscilloQPosY+OscilloHeight+sph/4),OscilloPosX+OscilloWidth+spw/4,height-(OscilloQPosY+OscilloHeight+sph/4)); // horizontal line Q+
			  windowLibraryObj->line(OscilloPosX-spw/4,height-(OscilloQPosY-sph/4),OscilloPosX+OscilloWidth+spw/4,height-(OscilloQPosY-sph/4)); // horizontal line Q-
			
			  windowLibraryObj->line(OscilloPosX-spw/4,height-(OscilloIPosY+OscilloHeight+sph/4),OscilloPosX+OscilloWidth+spw/4,height-(OscilloIPosY+OscilloHeight+sph/4)); // horizontal line I+
			  windowLibraryObj->line(OscilloPosX-spw/4,height-(OscilloIPosY-sph/4),OscilloPosX+OscilloWidth+spw/4,height-(OscilloIPosY-sph/4)); // horizontal line I-
	  
			  windowLibraryObj->setfg(0, 255, 0);
              // ********************************************************************************************************
              
			  // Initialization of points
			  unsigned int x01 = OscilloPosX;
			  unsigned int y01 = height-(OscilloIPosY+OscilloHeight/2);
			  
			  unsigned int x02 = OscilloPosX;
			  unsigned int y02 = height-(OscilloQPosY+OscilloHeight/2);
			  			  
			  unsigned int itime;
			  unsigned int ifreq;
			  unsigned int iCompI;
		      unsigned int iCompQ;
			  unsigned int idbGainF;
				
			  float dbGainF_max = 0.0f;
			  float dbGainF_min = -120.0f; 		  
			  
			  float PrFFT = 0;
			  
			  for(unsigned int i=0; i<FFT_SIZE; ++i ){
				  
				  float ratio;				  
				  
				  // =============================== TIME =================================
				  
				  ratio = i/(float)FFT_SIZE;
				  itime = (unsigned int)(ratio*OscilloWidth);
				  
				  ratio = 0.5*(1+iqin[i].real());
				  if(ratio>1){ ratio = 1; }
				  if(ratio<0){ ratio = 0; }
				  
				  iCompI = (unsigned int)(ratio*OscilloHeight); 
				  
				  ratio = 0.5*(1+iqin[i].imag());
				  if(ratio>1){ ratio = 1; }
				  if(ratio<0){ ratio = 0; }
				  
				  iCompQ = (unsigned int)(ratio*OscilloHeight);
				  
				  windowLibraryObj->line(x01,y01,OscilloPosX+itime,height-(OscilloIPosY+iCompI));
				  
				  windowLibraryObj->line(x02,y02,OscilloPosX+itime,height-(OscilloQPosY+iCompQ));
				   
				  // update points
				  x01 = OscilloPosX+itime;
				  y01 = height-(OscilloIPosY+iCompI);
				  x02 = OscilloPosX+itime;
				  y02 = height-(OscilloQPosY+iCompQ);
				  				  
				  // =============================== FREQUENCY =================================
				  
				  unsigned int n = (i<FFT_SIZE/2) ? i+FFT_SIZE/2 : i-FFT_SIZE/2 ;
				  
				  ratio = n/(float)FFT_SIZE;
				  ifreq = (unsigned int)(ratio*SpectrumWidth);
				  
				  std::complex<float> tempFFT(signalFFT_re[i],signalFFT_im[i]);
				  float temp = std::norm(tempFFT)/(float)(FFT_SIZE*FFT_SIZE);
				  float GainF = 0;
				  
				  switch(spectrumAnalyzerMode){
						case SPECTRUM_ANALYZER_MODE_CURRENT: {
								GainF = temp; 
							}break;
						case SPECTRUM_ANALYZER_MODE_MIN_HOLD: {
							    float minValue = (intermediateResult[i] < temp) ? intermediateResult[i] : temp;
							    GainF = (NumFrame) ? minValue : temp;
							    intermediateResult[i] = GainF;
							}break;
						case SPECTRUM_ANALYZER_MODE_MAX_HOLD: {
							    float maxValue = (intermediateResult[i] > temp) ? intermediateResult[i] : temp;
							    GainF = (NumFrame) ? maxValue : temp;
							    intermediateResult[i] = GainF;
							}break;;
						case SPECTRUM_ANALYZER_MODE_AVERAGE: {
							    GainF = (NumFrame*intermediateResult[i] + temp)/(NumFrame+1);
							    intermediateResult[i] = GainF;
							}break;
						default:
								fprintf(stderr,"[KISSPEC] asynchronousSpectrumAnalyzer (error) - This mode is not implemented"); exit(-1);
				  }
                  
                  PrFFT += GainF;

				  float dbGainF = 10*log10(GainF); // € [-220,0];
				  
				  ratio = (dbGainF-dbGainF_min)/(dbGainF_max-dbGainF_min); // ratio € [0,1]
				  if(ratio>1){ ratio = 1; }
				  if(ratio<0){ ratio = 0; }
				  
				  idbGainF = (unsigned int)(ratio*SpectrumHeight);
				  				  				  
				  windowLibraryObj->line(SpectrumPosX+ifreq, height-SpectrumPosY, SpectrumPosX+ifreq, height-(SpectrumPosY+idbGainF));	
				  
				  // =============================================================================				  
			  }
			  
			  
			  float PrFFT_dBm = 10*log10(PrFFT) + 30;
			  
			  // Path Loss parameters
			  float Eh = fabs(PrFFT-sigma2_n)/Pe;			
			  float PL = -10*log10(Eh);
			  float Dist = pow(Ge*Gr/Eh,1.0/alpha)*pow(3e8/(4*PI*Fc),2.0/alpha);
			
			  sprintf(StrPow,"Pr = %2.3f [dBm]  --  Path_loss = %2.3f [dB]  --  Distance = %2.3f [m].",PrFFT_dBm,PL,Dist);
			  
			  windowLibraryObj->setfg(0, 255, 255);
			  windowLibraryObj->text(rw/4, rh/4, StrPow);
			  
			  windowLibraryObj->show();
			  windowLibraryObj->sync();
			}
       
            // ===========================================================================================================
            
			NumFrame++;
			
		} // valid read file
       
    } // end while

    fclose (input_fid);
    
    iqin.clear();
    
    windowLibraryObj->~windowLibrary();

    exit(0);
}
