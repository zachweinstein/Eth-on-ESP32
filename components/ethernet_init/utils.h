#ifndef UTILS_H
#define UTILS_H

#define WIFI_MODE BIT0
#define WIFI_ENABLE BIT1
#define WIFI_DISABLE BIT2

#define ETH_MODE BIT4
#define ETH_ENABLE BIT5
#define ETH_DISABLE BIT6

typedef enum t_states { INIT, READY, RUN, RESET};
typedef enum network_mode {WIFI, ETH, NONE};

#endif