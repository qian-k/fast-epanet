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

static int SENSOR_NUMBER = 4;
static int TIME_STEPS    = 12;

SensorNode sensor_nodes[10];
//char *sensor[SENSOR_NUMBER]={"117", "141", "161", "181", "199", "209", "239", "261", "40"};
char *sensor[10]={"10", "119", "143", "163", "183", "20", "213", "243", "265", "601"};

int analyze_report(char *report){

	FILE *fp = NULL;
	int i = 0;
	int step = -1;
	int devour = 0;
	int in_node_section = 0;
	int node_index;
	char *node_id, *tmp, line[100];
	float concentration;

	if( (fp = fopen(report, "r")) == NULL )
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

			tmp = strtok(NULL, " \t");
			tmp = strtok(NULL, " \t");
			tmp = strtok(NULL, " \t");
			tmp = strtok(NULL, " \t");
			concentration = atof(tmp);
			sensor_nodes[i].Cr[step] = concentration;
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

int ENFitnessPreset(int num, int steps){

	SENSOR_NUMBER = num;
	TIME_STEPS = steps;
	return 0;
}

int ENFitnessInit(char *input, char *report){

	int i, err = 0;
	long duration, step, steps;

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

	if( (err = analyze_report(report)) != 0 )
		goto EXIT;

	
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
		if (start_step != -1 && step - start_step >= TIME_STEPS)
			continue;
		for(i=0; i<SENSOR_NUMBER; i++){
			ENgetnodevalue(sensor_nodes[i].index, EN_QUALITY, &c);
			//if (c > 0 || sensor_nodes[i].Cr[step] > 0){
			if (sensor_nodes[i].Cr[step] > 0){
				fitness += (sensor_nodes[i].Cr[step] - c) * (sensor_nodes[i].Cr[step] - c);
				if (start_step == -1 && sensor_nodes[i].Cr[step] > 0){
					start_step = step;
				}
			}
		}
		step++;
	} while (tstep > 0); 

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

