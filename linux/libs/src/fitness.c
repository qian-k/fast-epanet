#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "epanet2.h"
#include "toolkit.h"

//#define SENSOR_NUMBER 9
#define NODE_SECTION "Node Results"
#define EXTERN extern

typedef struct nodeinfo{
	int index;
	float *Cr;
} SensorNode;

static int SENSOR_NUMBER = 30;
static int TIME_STEPS    = 12;

SensorNode sensor_nodes[30];
//char *sensor[SENSOR_NUMBER]={"117", "141", "161", "181", "199", "209", "239", "261", "40"};
//char *sensor[10]={"10", "119", "143", "163", "183", "20", "213", "243", "265", "601"};
char *sensor[] = {"1032",  "1076",  "1088",  "1159",  "6060",  "6209",  "6229",  "6245",  "6260",  "11137",
				  "11165", "12047", "12131", "12209", "12237", "12275", "12351", "12381", "12412", "12748",
				  "12786", "12804", "12858", "12903", "12914", "61175", "61204", "61249", "61268", "61310",
				  "61590", "61609", "61624", "61625", "61648", "61682", "61758", "61967", "600082"
				};

int analyze_report(char *rpt){

	FILE *fp = NULL;
	int i = 0;
	int step = -1;
	int devour = 0;
	int in_node_section = 0;
	int node_index;
	char *node_id, *tmp, line[100];
	float concentration;

	if( (fp = fopen(rpt, "r")) == NULL )
		return -1;

	while (fgets(line, sizeof(line), fp) != NULL){

		if(devour > 0 ){
			devour--;
			continue;
		}

		if(in_node_section == 1){

			node_id = strtok(line, " \r\t\n");
			if(node_id == NULL){
				in_node_section = 0;
				continue;
			}

			for(i=0; i<SENSOR_NUMBER; i++){
				ENgetnodeindex(node_id, &node_index);
				if(sensor_nodes[i].index == node_index )
					break;
			}

			if(i >= SENSOR_NUMBER)
				continue;

			tmp = strtok(NULL, " \r\t\n");
			tmp = strtok(NULL, " \r\t\n");
			tmp = strtok(NULL, " \r\t\n");
			tmp = strtok(NULL, " \r\t\n");
			concentration = atof(tmp);
			sensor_nodes[i].Cr[step] = concentration;
		//printf("RPT-Set sensor %s at time %d: %f\n", sensor[i], step, concentration);
		}
		else if(strstr(line, NODE_SECTION) != NULL){
			in_node_section = 1;	
			devour = 4;
			step++;
		}

	}

	fclose(fp);

	return 0;
	
}

int analyze_csv(char *csv){

	FILE *fp = NULL;
	int i, step, node_index, node_index_pre, devour;
	char *node_id, *tmp, line[100];
	float concentration;

	devour = 1;
	step = -1;
	node_index_pre = -1;

	if ((fp = fopen(csv, "r")) == NULL)
		return -1;

	while (fgets(line, sizeof(line), fp) != NULL){
		if(devour > 0 ){
			devour--;
			continue;
		}
		node_id = strtok(line, ", \r\t\n");
		if(node_id == NULL){
			return -1;
		}
		ENgetnodeindex(node_id, &node_index);
		if(node_index_pre == -1 || node_index != node_index_pre)
			step = 0;
		else
			step++;
		node_index_pre = node_index;
		for(i=0; i<SENSOR_NUMBER; i++){
			if(sensor_nodes[i].index == node_index)
				break;
		}
		if(i >= SENSOR_NUMBER)
			continue;
		tmp = strtok(NULL, ", \t\r\n");
		tmp = strtok(NULL, ", \t\r\n");
		concentration = atof(tmp);
		//printf("CSV-Set sensor %s at time %d: %f\n", sensor[i], step, concentration);
		sensor_nodes[i].Cr[step] = concentration;
	}

	fclose(fp);

	return 0;
}

int ENFitnessPreset(int num, int steps){

	SENSOR_NUMBER = num;
	TIME_STEPS = steps;
	return 0;
}

int ENFitnessInit(char *input, char *report){

	int i, len, err = 0;
	long duration, step, steps;
	char *suffix;

	if( (err = ENopen(input, "tmp.rpt", "")) != 0 )
		goto EXIT;
	if( (err = ENsolveH()) != 0 )
		goto EXIT;
	if( (err = ENgettimeparam(EN_DURATION, &duration)) != 0 )
		goto EXIT;
	if( (err = ENgettimeparam(EN_QUALSTEP, &step)) != 0 )
		goto EXIT;
	if( (err = ENopenQ()) != 0 )
		goto EXIT;

	steps = duration / step + 1;

	for (i=0; i<SENSOR_NUMBER; i++){

		ENgetnodeindex(sensor[i], &sensor_nodes[i].index );
		sensor_nodes[i].Cr = (float *) malloc( steps * sizeof(float) );
		if (sensor_nodes[i].Cr == NULL){
			err = -1;
			goto EXIT;
		}
	}

	len = strlen(report);
	if(len <=3 ){
		err = -1;
		goto EXIT;
	}
	suffix = &report[len-3];

	if(strncmp(suffix, "rpt", 3) == 0){
		if( (err = analyze_report(report)) != 0 )
			goto EXIT;
	}
	else if(strncmp(suffix, "csv", 3) == 0){
		if( (err = analyze_csv(report)) != 0 )
			goto EXIT;
	}
	else
		err = -1;

EXIT:
	printf("ENFitnessInit return with error code: %d\n", err);
	return err;
}


float ENFitnessEvaluate(int node, int stime, int duration, int concentration){

	int i, err; 
	int pindex, nindex;
	long t, tstep, step, start_step = -1; 
	float c, fitness, pattern[24];
	char node_id[16];
	int tt=-1;

	fitness = 0;
	step = 1;
	
	snprintf(node_id, 16, "%d", node);

	for(i=0; i<24;i++)
		if(i>=stime && i<stime+duration)
			pattern[i] = 1;
		else
			pattern[i] = 0;

	if((err = ENgetpatternindex("100", &pindex)) != 0)
		goto EXIT;
	if((err = ENgetnodeindex(node_id, &nindex)) != 0)
		goto EXIT;
	if( (err = ENsetqualtype(EN_CHEM, "chemical", "mg/L", "")) != 0 )
		goto EXIT;
	if((err = ENsetpattern(pindex, pattern, 24)) != 0)
		goto EXIT;
	if((err = ENsetnodevalue(nindex, EN_SOURCEQUAL, concentration)) != 0)
		goto EXIT;
	if((err = ENsetnodevalue(nindex, EN_SOURCETYPE, EN_SETPOINT)) != 0)
		goto EXIT;
	if((err = ENsetnodevalue(nindex, EN_SOURCEPAT, pindex)) != 0)
		goto EXIT;

	ENinitQ(0); 
	do { 
		ENrunQ(&t); 
  		ENstepQ(&tstep); 
		tt++;
		if (start_step != -1 && step - start_step >= TIME_STEPS)
			continue;
		for(i=0; i<SENSOR_NUMBER; i++){
			ENgetnodevalue(sensor_nodes[i].index, EN_QUALITY, &c);
			//if (c > 0 || sensor_nodes[i].Cr[step] > 0){
			if (sensor_nodes[i].Cr[step] > 0){
	//printf("step:%d 1-%f, 2-%f\n", step, sensor_nodes[i].Cr[step], c);
				fitness += (sensor_nodes[i].Cr[step] - c) * (sensor_nodes[i].Cr[step] - c);
				if (start_step == -1 && sensor_nodes[i].Cr[step] > 0){
					start_step = step;
				}
			}
		}
		step++;
	} while (tstep > 0); 
	//printf("Final step:%d\n", tt);

	if((err = ENsetnodevalue(nindex, EN_SOURCEQUAL, 0)) != 0)
		goto EXIT;

	return fitness;
EXIT:
	printf("ENFitnessEvaluate return with error code:%d\n", err);
	return -1;

}

int ENFitnessTerminate(){

	int i;

	for (i=0; i<SENSOR_NUMBER; i++){
		if (sensor_nodes[i].Cr != NULL)
			free(sensor_nodes[i].Cr);
	}
	
	ENcloseQ(); 
	ENclose();

	return 0;
}

