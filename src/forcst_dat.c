#include "stow_controller.h"
#include <curl/curl.h>
#include <time.h>

#define XML_DELIM "</>"
#define DATE_KEY "start-valid-time"
#define SPD_KEY "value"
#define DATE_LEN 20
#define SPD_LEN 3
#define COORD_LEN 12

#define URL1 "http://graphical.weather.gov/xml/SOAP_server/ndfdXMLclient.php?whichClient=NDFDgen&lat="
#define URL2 "&lon="
#define URL3 "&begin="
#define URL4 "&end="
#define URL5 "&product=time-series&Unit=m&wspd=wspd&Submit=Submit"

#define TM_FRMT "%Y-%m-%dT%H:%M:%S"

struct MemoryStruct {
	char *memory;
	size_t size;
};

static size_t get_data(void *buffer, size_t size, size_t nmemb, void *userp);
static int parse_xml_doc(char *buf, char ***dat, int **spd, int *sz);
static float *parse_date(char **buf, int len);
static int set_frcst_spd(float *tm, int *spd, int len);

void *forcst_dat(void *arg){

	char *url_req, *s_time, *e_time, *lat, *lon, *errbuf;
	struct MemoryStruct chunk;
	struct tm *tm_s, *tm_e;
	time_t cur_tm;
	CURL *curl;
	CURLcode res;
 
	chunk.memory = malloc(1);
	chunk.size = 0;

	errbuf = malloc(CURL_ERROR_SIZE*sizeof(char));
	url_req = malloc(256*sizeof(char));
	s_time = malloc(DATE_LEN*sizeof(char));
	e_time = malloc(DATE_LEN*sizeof(char));
	lat = malloc(COORD_LEN*sizeof(char));
	lon = malloc(COORD_LEN*sizeof(char));

	for(;;){

		cur_tm = time(NULL);

		tm_s = localtime(&cur_tm);
		if(tm_s == NULL)
			printf("Error getting current system time\n");

		if(!strftime(s_time, DATE_LEN, TM_FRMT, tm_s))
			fprintf(stderr, "strftime failed\n");

		cur_tm+= (wstw_cfg.frcst_delt+1)*3600;

		tm_e = localtime(&cur_tm);


		if(tm_e == NULL)
			printf("Error getting current system time\n");

		if(!strftime(e_time, DATE_LEN, TM_FRMT, tm_e))
			fprintf(stderr, "strftime failed\n");

		while(stw_stat.lat == 0.0 || stw_stat.lon == 0.0)
			sleep(2);

		snprintf(lat, COORD_LEN, "%f", stw_stat.lat);
		snprintf(lon, COORD_LEN, "%f", stw_stat.lon);
		//printf("Longitude: %s | Latitude: %s\n", lon, lat);

		*url_req = '\0';

		strncat(url_req, URL1, sizeof(URL1));
		strncat(url_req, lat, strlen(lat));
		strncat(url_req, URL2, sizeof(URL2));
		strncat(url_req, lon, strlen(lon));
		strncat(url_req, URL3, sizeof(URL3));
		strncat(url_req, s_time, DATE_LEN);
		strncat(url_req, URL4, sizeof(URL4));
		strncat(url_req, e_time, DATE_LEN);
		strncat(url_req, URL5, sizeof(URL5));

		//printf("%s\n", url_req);

		curl = curl_easy_init();
		if(!curl)
			printf("WFT\n");

		curl_easy_setopt(curl, CURLOPT_URL, url_req);
		curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, get_data);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
		res = curl_easy_perform(curl);

		if(res){
			//set_sctrl_stat(5, 1);

			cur_tm= time(NULL);
			tm_s = localtime(&cur_tm);
			//lock log file mutex
//			fprintf(fd_log, "%s: %s", asctime(tm_s), errbuf);
			//unlock log file mutex
			curl_easy_cleanup(curl);
		}
		else{
			curl_easy_cleanup(curl);
			//set_ctrl_stat(5, 0);
		}

		memset(url_req, '\0', 256);

		sleep(900);
	}
}

static size_t get_data(void *buffer, size_t size, size_t nmemb, void *userp){

	char **dates;
	int ret, i, *pred_spd, count;
	float *tim_diff;
	size_t act_size;
	struct MemoryStruct *mem;
	struct tm *tm_s;
	time_t cur_tm;
	

	act_size = size*nmemb;
	mem = (struct MemoryStruct *)userp;

	mem->memory = realloc(mem->memory, mem->size + act_size + 1);
	if(mem->memory == NULL) {
		//set_sctrl_stat(5, 1);

		cur_tm= time(NULL);
		tm_s = localtime(&cur_tm);
		//lock log file mutex
//		fprintf(fd_log, "%s: %s", asctime(tm_s), "Cannot allocate memory for XML document");
		//unlock log file mutex
	}
 
	memcpy(&(mem->memory[mem->size]), buffer, act_size);
	mem->size += act_size;
	mem->memory[mem->size] = 0;

	dates = malloc(sizeof(*dates));
	*dates = malloc(DATE_LEN*sizeof(char));

	pred_spd = malloc(sizeof(int));


	ret = parse_xml_doc(buffer, &dates, &pred_spd, &count);


	if(ret){

		tim_diff = parse_date(dates, count);

		//for(i=0; i<count; i++)
	             // printf("%f %s %d\n", *(tim_diff+i), *(dates+i), *(pred_spd+i));

		set_frcst_spd(tim_diff, pred_spd, count);
		free(tim_diff);
	}
	else{
		//set_sctrl_stat(5, 1);

		cur_tm= time(NULL);
		tm_s = localtime(&cur_tm);
		//lock log file mutex
//		fprintf(fd_log, "%s: %s", asctime(tm_s), "Malformed NWS Response");
		//unlock log file mutex
	}

	for(i=0; i<count; i++)
		free(*(dates+i));
	free(dates);
	free(pred_spd);
	free(mem->memory);

	return(act_size);
}

static int parse_xml_doc(char *buf, char ***dat, int **spd, int *sz){
	char *token, *token2;
	int i, j, flag, flag2, count;
	struct tm *tm_s;
	time_t cur_tm;

	flag = flag2 = 0;
	i = j = 0;
	*sz = 0;

	token = strtok(buf, XML_DELIM);
	do{
		if(!flag && !strcmp(token, DATE_KEY)){
			flag = 1;
			token = strtok(NULL, XML_DELIM);

			i++;
			*dat = realloc(*dat, (i+1)*sizeof(**dat));
			*(*dat+i-1) = malloc(DATE_LEN*sizeof(char));

			*(token+19) = '\0';
			strcpy(*(*dat+i-1), token);

		}
		else if(flag && !strcmp(token, DATE_KEY)){
			flag = 0;
			token = strtok(NULL, XML_DELIM);
		}
		else if(!flag2 && !strcmp(token, SPD_KEY)){
			flag2 = 1;
			token = strtok(NULL, XML_DELIM);
			
			j++;
			*spd = realloc(*spd, (j+1)*sizeof(int));

			*(*spd+j-1) = atoi(token);
		}
		else if(flag2 && !strcmp(token, SPD_KEY)){
			flag2 = 0;
			token = strtok(NULL, XML_DELIM);
		}
		else{
			token = strtok(NULL, XML_DELIM);
		}	
	}while(token != NULL);

	*sz = i;


	if(*sz)
		return(1);
	else
		return(0);

}

static float *parse_date(char **buf, int len){

	int i;
	float *tm_diff;
	time_t ret, cur_tm;
	struct tm tm;

	tm_diff = malloc(len*sizeof(float));

	cur_tm = time(NULL);

	for(i=0; i<len; i++){
		memset(&tm, 0, sizeof(struct tm));
		strptime(*(buf+i), TM_FRMT, &tm);

		ret = mktime(&tm);

		if(ret == -1){
			printf("Failed to convert time\n");
			*(tm_diff+i) = -1;
		}
		else{
			*(tm_diff+i) = (float)difftime(ret, cur_tm)/3600;
		}

	}
	return(tm_diff);
}

static int set_frcst_spd(float *tm, int *spd, int len){

	int i, idx;
	float tmp, *diff;

	diff = malloc(len*sizeof(int));

	for(i=0; i<len; i++)
		*(diff+i) = *(tm+i) - wstw_cfg.frcst_delt;

	tmp = *(diff+len-1);
	idx = len-1;

	for(i=len-2; i>0; i--){
		if(*(diff+i) < tmp && *(tm+i) > wstw_cfg.frcst_delt && *(diff+i) > 0){
			tmp = *(diff+i);
			idx = i;
		}
	}

	pthread_mutex_lock(&lock_stat);

	stw_stat.frcst_spd = *(spd+idx);

	pthread_mutex_unlock(&lock_stat);

//	printf("%d\n", stw_stat.frcst_spd);

	free(diff);

	return(1);
}

