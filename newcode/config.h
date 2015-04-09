#ifndef _CONFIG_H
#define _CONFIG_H
#include <string>

typedef struct {
	int max_supp;
	int size;
	std::string fi_path;
	std::string path;
} ds_config;

typedef struct {
	bool verbose;
	double delta;
	double theta;
} mine_config;

enum count_method {
	COUNT_EXACT = 1,
	COUNT_FAST = 2,
	COUNT_SUKP = 3
};

enum bound_method {
	BOUND_EXACT = 1,
	BOUND_SCAN = 2
};

typedef struct {
	bool use_antichain;
	int evc_bound;
	int max_supp;
	count_method cnt_method;
	bound_method bnd_method;
} stats_config;

#endif
