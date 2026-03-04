#include "mgmt_transmit.h"


uint8_t MCS_INIT;
uint8_t NET_WORKMOD_INIT;
uint32_t FREQ_INIT;
uint16_t POWER_INIT;
uint8_t BW_INIT;
uint8_t RX_CHANNEL_MODE_INIT;
uint16_t MACMODE_INIT;
uint32_t HOP_FREQ_TB_INIT[HOP_FREQ_NUM];
Global_Radio_Param g_radio_param;


void read_node_xwg_file(const char * filename,Node_Xwg_Pairs* xwg_info,int num_pairs)
{
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("无法打开文件");
        return;
    }	

	for (int i = 0; i < num_pairs; i++) {
        xwg_info[i].found = 0;
    }

    char line[1024];
    while(fgets(line,sizeof(line),file)){
        //移除行尾的换行符
        line[strcspn(line, "\r\n")] = 0;
        //解析key=value
        char *key = strtok(line," ");
        char *value_str = strtok(NULL, " ");

        if(key !=NULL && value_str !=NULL)
        {
            for(int i=0;i<num_pairs;i++)
            {
                if(strcmp(key,xwg_info[i].key)==0)
                {
                    memcpy(xwg_info[i].value,value_str,100);
                    xwg_info[i].found = 1;
                    break;
                }
            }
        }
    }
    
}

const char *get_value(Node_Xwg_Pairs* pairs,const char *key) {
    for (int i = 0; i < MAX_XWG_PAIRS; i++) {
        if (strcmp(key, pairs[i].key) == 0 && pairs[i].found) {
            return pairs[i].value;
        }
    }
    return NULL;
}

double get_double_value(Node_Xwg_Pairs* pairs,const char *key)
{
	const char *value = get_value(pairs,key);
    if (value != NULL) {
        return atof(value);
    }
    return 0.0;

}

int get_int_value(Node_Xwg_Pairs* pairs,const char* key)
{
    const char *value = get_value(pairs,key);
    if (value != NULL) {
        return atoi(value);
    }
    return -1;
}