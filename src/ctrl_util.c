#include <math.h>
#include <time.h>
#include <curl/curl.h>
#include <jsmn/jsmn.h>
#include "stow_controller.h"

#define POLL_PRD 10 //7200
#define SEC_HR 3600
#define LAT_REG 1404
#define LON_REG 1406
#define TM_REG 1408
#define TMSTMP_REG 1100
#define TZ_BUFF 50
#define COORD_LEN 12
#define TM_STMP_LEN 11

//https://maps.googleapis.com/maps/api/timezone/xml?location=39.6034810,-119.6822510&timestamp=1331161200&key=AIzaSyDkJIZDqW5Nn0gqlZeG_r1vTLICN7YbISo

#define URL1 "https://maps.googleapis.com/maps/api/timezone/json?location="
#define URL2 "&timestamp="
#define URL3 "&key="
#define API_KEY "AIzaSyDkJIZDqW5Nn0gqlZeG_r1vTLICN7YbISo"

struct MemoryStruct{
	char *memory;
	size_t size;
};

static short *poll_ctrl(char *addr, int reg, int reg_num);
static float mk_float(short hi_word, short lo_word);
static time_t mk_time(short hi_word, short lo_word);
static char *get_tmzone(float lat, float lon, int tm_stmp);
static size_t get_json_data(void *buffer, size_t size, size_t nmemb, void *userp);
static int jsoneq(const char *json, jsmntok_t *tok, const char *s);

void *ctrl_util(void *arg){

	short *dat;
	char *tmzone;
	time_t c_tm, tm_stmp;
	struct tm *cur_tm, *cur_tm_loc;
	float lat, lon;
	



	for(;;){
		dat = poll_ctrl(*tctr_cfg.tcp_adr, LAT_REG, 2);
		if(dat){
			lat = mk_float(*dat, *(dat+1));
			free(dat);
		}

		dat = poll_ctrl(*tctr_cfg.tcp_adr, LON_REG, 2);
		if(dat){
			lon = mk_float(*dat, *(dat+1));
			free(dat);
		}

		dat = poll_ctrl(*tctr_cfg.tcp_adr, TM_REG, 2);
		if(dat){
			c_tm = mk_time(*dat, *(dat+1));
			free(dat);
			cur_tm = localtime(&c_tm);
		}


		tmzone = get_tmzone(lat, lon, c_tm);

//		printf("%s %s\n", tmzone, cur_tm->tm_zone);

//		strcpy(cur_tm->tm_zone, tmzone);

//		printf("%s\n", cur_tm->tm_zone);

		//write timezone to configuration file

//		setenv("TZ", cur_tm->tm_zone, 1);
		//unset
//		printf("hour: %d | timezone: %s | offset: %d\n", cur_tm_loc->tm_hour, cur_tm_loc->tm_zone, cur_tm_loc->tm_gmtoff);
		//cur_tm_loc->tm_isdst = -1;
			//printf("%d\n", tm_stmp-c_tm);
//		unsetenv("TZ");


		if(dat){
//		stw_stat.tm_zone = cur_tm_loc->tm_gmtoff;
//			printf("Latitude: %f | Longitude: %f | Time: %s | Timestamp: %s | Timezone: %s | Offset: %d\n", lat, lon, asctime(cur_tm), asctime(cur_tm_loc), cur_tm_loc->tm_zone, cur_tm_loc->tm_gmtoff);
			pthread_mutex_lock(&lock_stat);
			stw_stat.lat = lat;
			stw_stat.lon = lon;
			stw_stat.gps_time = c_tm+stw_stat.tm_zone;
			pthread_mutex_unlock(&lock_stat);

			stime(&stw_stat.gps_time);
		}
		else{
	//		printf("Controller not responding\n");
			//log error to file
		}

		if(fabs(lat>0) && fabs(lon)>0)
			sleep(POLL_PRD);
		else
			sleep(5);
	}
}

static char *get_tmzone(float lat, float lon, int tm_stmp){

	int i, r, str_sz, dstoff, rawoff;
	char *lat_c, *lon_c, *tm_stmp_c, *tzone, *url_req, *errbuf, *dstoffset_c, *rawoffset_c;
	struct MemoryStruct chunk;
	jsmn_parser p;
	jsmntok_t t[20];
	CURL *curl;
	CURLcode res;

	chunk.memory = malloc(1);
	chunk.size = 0;

	tzone = malloc(TZ_BUFF*sizeof(char));
	lat_c = malloc(COORD_LEN*sizeof(char));
	lon_c = malloc(COORD_LEN*sizeof(char));
	tm_stmp_c = malloc(TM_STMP_LEN*sizeof(char));
	url_req = malloc(256*sizeof(char));
	errbuf = malloc(CURL_ERROR_SIZE*sizeof(char));
	dstoffset_c = malloc(20*sizeof(char));

	//printf("%f %f\n", stw_stat.lat, stw_stat.lon);

	snprintf(lat_c, COORD_LEN, "%f", lat);
	snprintf(lon_c, COORD_LEN, "%f", lon);
	snprintf(tm_stmp_c, TM_STMP_LEN, "%d", tm_stmp);

	*url_req  = '\0';

	strncat(url_req, URL1, sizeof(URL1));
	strncat(url_req, lat_c, strlen(lat_c));
	strncat(url_req, ",", 1);
	strncat(url_req, lon_c, strlen(lon_c));
	strncat(url_req, URL2, sizeof(URL2));
	strncat(url_req, tm_stmp_c, strlen(tm_stmp_c));
	strncat(url_req, URL3, sizeof(URL3));
	strncat(url_req, API_KEY, sizeof(API_KEY));

	curl = curl_easy_init();
	if(!curl){
		curl_easy_cleanup(curl);
		free(lat_c);
		free(lon_c);
		free(tm_stmp_c);
		return(NULL);
	}

	curl_easy_setopt(curl, CURLOPT_URL, url_req);
	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, get_json_data);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
	res = curl_easy_perform(curl);
	if(res){
		curl_easy_cleanup(curl);
		free(lat_c);
		free(lon_c);
		free(tm_stmp_c);
		free(url_req);
		free(errbuf);
		free(dstoffset_c);
		return(NULL);
	}

	jsmn_init(&p);
	r = jsmn_parse(&p, chunk.memory, chunk.size, t, sizeof(t)/sizeof(t[0]));


	if(r<0){
		printf("Failed to parse JSON: %d\n", r);
		return(NULL);
	}

	str_sz = 0;

	for(i=0; i<r; i++){
		if(!jsoneq(chunk.memory, &t[i], "dstOffset")){
			//printf("%.*s\n", t[i+1].end-t[i+1].start, chunk.memory+t[i+1].start);
			dstoffset_c = malloc((t[i+1].end-t[i+1].start+1)*sizeof(char));
			strncpy(dstoffset_c, chunk.memory+t[i+1].start, t[i+1].end-t[i+1].start);
			strncat(dstoffset_c, "\0", 1);
			//printf("%d %s\n", t[i+1].end-t[i+1].start, dstoffset_c);
			dstoff = atoi(dstoffset_c);
			free(dstoffset_c);
			i++;
		}
		else if(!jsoneq(chunk.memory, &t[i], "rawOffset")){
			//printf("%.*s\n", t[i+1].end-t[i+1].start, chunk.memory+t[i+1].start);
			rawoffset_c = malloc((t[i+1].end-t[i+1].start+1)*sizeof(char));
			strncpy(rawoffset_c, chunk.memory+t[i+1].start, t[i+1].end-t[i+1].start);
			strncat(rawoffset_c, "\0", 1);
			//printf("%d %s\n", t[i+1].end-t[i+1].start, rawoffset_c);
			rawoff = atoi(rawoffset_c);
			free(rawoffset_c);
			i++;
		}
		else if(!jsoneq(chunk.memory, &t[i], "status")){
			//printf("%.*s\n", t[i+1].end-t[i+1].start, chunk.memory+t[i+1].start);
			if(strncmp(chunk.memory+t[i+1].start, "OK", t[i+1].end-t[i+1].start)){
				printf("Empty response to query\n");
				curl_easy_cleanup(curl);
				free(lat_c);
				free(lon_c);
				free(tm_stmp_c);
				free(url_req);
				free(errbuf);
				free(dstoffset_c);
				return(NULL);
			}
			i++;
		}
		else if(!jsoneq(chunk.memory, &t[i], "timeZoneId")){
			//printf("%.*s\n", t[i+1].end-t[i+1].start, chunk.memory+t[i+1].start);
			strncpy(tzone, chunk.memory+t[i+1].start, t[i+1].end-t[i+1].start);
			i++;
		}
		/*
		if(!jsoneq(chunk.memory, &t[i], "timeZoneName")){
			printf("%.*s\n", t[i+1].end-t[i+1].start, chunk.memory+t[i+1].start);
			i++;
		}
*/
	}

	pthread_mutex_lock(&lock_stat);
	stw_stat.tm_zone = 0;
	stw_stat.tm_zone = dstoff + rawoff;
	pthread_mutex_unlock(&lock_stat);

	//printf("%s\n", tzone);	

	curl_easy_cleanup(curl);
	free(lat_c);
	free(lon_c);
	free(tm_stmp_c);
	free(url_req);
	free(errbuf);
	free(dstoffset_c);
	return(tzone);
}

static size_t get_json_data(void *buffer, size_t size, size_t nmemb, void *userp){


	size_t act_size;
	struct MemoryStruct *mem;

	act_size = size*nmemb;
	mem = (struct MemoryStruct *)userp;

	mem->memory = realloc(mem->memory, mem->size + act_size + 1);
	if(mem->memory == NULL){
		//lock log file mutex
//              fprintf(fd_log, "%s: %s", asctime(tm_s), "Cannot allocate memory for JSON document");
                //unlock log file mutex
	}

	memcpy(&(mem->memory[mem->size]), buffer, act_size);
	mem->size += act_size;
	mem->memory[mem->size] = 0;

	return(act_size);
}

static int jsoneq(const char *json, jsmntok_t *tok, const char *s){

	if(tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start && !strncmp(json + tok->start, s, tok->end - tok->start))
		return(0);

	return -1;
}

static short *poll_ctrl(char *addr, int reg, int reg_num){

	int ret;
	float tmp;
	short *dat;
	modbus_t *mb;
	uint16_t tab_reg16[2];

	mb = modbus_new_tcp(addr, tctr_cfg.port);

	modbus_set_slave(mb, tctr_cfg.slv_adr);

	if(modbus_connect(mb) == -1){
		modbus_free(mb);
		//fprintf(stderr, "Connection failed: %s\n", modbus_strerror(errno));
		return(0);
	}

	dat = malloc(reg_num*sizeof(short));

	ret = modbus_read_registers(mb, reg-1, reg_num, tab_reg16);

	if(ret == -1){
		modbus_close(mb);
		modbus_free(mb);
		//fprintf(stderr, "Poll failed: %s\n", modbus_strerror(errno));
		return(0);
	}
	else{

		if(reg_num == 1){
			*dat = tab_reg16[0];
			modbus_close(mb);
			modbus_free(mb);
			return(dat);
		}
		else if(reg_num == 2){
			*dat = tab_reg16[0];
			*(dat+1) = tab_reg16[1];
			modbus_close(mb);
			modbus_free(mb);
			return(dat);

		}
		else{
			//printf("invalid register quantity\n");
			modbus_close(mb);
			modbus_free(mb);
			return(0);
		}

	}	
}

static float mk_float(short hi_word, short lo_word){

	float fdat;
	int idat, *iptr;

	iptr = (int*)&fdat;

	idat = ((int)hi_word<<16)|(lo_word&0x00FFFF);

	*iptr = idat;

	return(fdat);
}

static time_t mk_time(short hi_word, short lo_word){


	time_t tdat;
	int idat, *iptr;

	iptr = (int*)&tdat;

	idat = ((int)hi_word<<16)|(lo_word&0x00FFFF);
	*iptr = idat;

	return(tdat);
}
