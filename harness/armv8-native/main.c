#include "semihosting.h"
#include "str.h"
#include "heap.h"
#include "test.h"
#include "printf/printf.h"

#define ARRAY_SIZE(__arr) (sizeof(__arr) / sizeof(__arr[0]))

#include <stdint-gcc.h>

#define ITERATIONS 10

void puts(const char *c) {
	while(*c){
		angel_writec(*c);
		c++;
	}
}

char buffer[0x1000];

uint64_t random() {
	const  uint64_t key = 0x1f2f3f4f5f6f7f8full;
	static uint64_t prev = key ;
	
	prev *= key;
	return prev;
}

void generate_state(state_t *state) {
	for(int i = 0; i < ARRAY_SIZE(state->gregs); ++i) {
		state->gregs[i] = random();
	}
	
	for(int i = 0; i < ARRAY_SIZE(state->vregs); ++i) {
		state->vregs[i].lo = random();
		state->vregs[i].hi = random();
	}
	
	state->flags = random() & 0xf0000000;
}

void copy_test_to_slot(test_t *test) {
	if(test->size != 4) {
		printf("Error! Test has wrong size\n");
		while(1);
	}
	
	test_slot = test->data;
}

void test_results(state_t *input, state_t *output) {
	for(int i = 0; i < ARRAY_SIZE(input->gregs); ++i) {
		printf("X%u\t%lx\t%lx\n", i, input->gregs[i], output->gregs[i]);
	}
	for(int i = 0; i < ARRAY_SIZE(input->vregs); ++i) {
		printf("V%u\t%lx:%lx\t%lx:%lx\n", i, input->vregs[i].hi, input->vregs[i].lo, output->vregs[i].hi, output->vregs[i].lo);
	}
	printf("FLAGS\t%lx\t%lx\n", input->flags, output->flags);
}

void run_test(test_t *test) {
	printf("Test %x\n", test->data);
	
	state_t input_state;
	state_t output_state;
	generate_state(&input_state);
	
	bzero(&output_state, sizeof(output_state));
	
	copy_test_to_slot(test);
	execute_test(&input_state, &output_state);
	
	test_results(&input_state, &output_state);
	
	puts("\n");
}

const char *get_input_file_ptr(const char *args) {
	while(*args != ' ') args++;
	return args+1;
}

int main() {
	heap_init();
	
	// get filename to load
	angel_get_cmdline(buffer, sizeof(buffer));
	
	const char *filename = get_input_file_ptr(buffer);
	printf("Loading file %s\n", filename);
	
	// open test file
	int fd = angel_open(filename, 0);
	if(fd < 0) {
		printf("Could not open file\n");
		return 1;
	}
	//printf("Got fd %x\n", fd);
	
	// load file under test
	long size = angel_flen(fd);
	char *data = (char*)malloc(size);
	printf("Reading %u bytes...", size);
	angel_read(fd, data, size);
	printf("done\n");
	
	test_t *tests = (test_t*)data;
	int test_count = size / sizeof(test_t);
	printf("Found %u tests\n", test_count);
	
	for(int i = 0; i < test_count; ++i) {
		for(int j = 0; j < ITERATIONS; ++j) {
			run_test(&tests[i]);
		}
	}
	
	free(data);
	
	printf("Tests finished.\n");
	angel_reportexception(0x20026);
	return 0;
}
