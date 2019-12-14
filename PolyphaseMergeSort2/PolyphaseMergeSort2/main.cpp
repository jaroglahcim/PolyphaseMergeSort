#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <time.h>
#include <fstream>
#include <iostream>

using namespace std;

#define NUMBER_OF_FLOATS_IN_DISK_BLOCK 500
#define SIZE_OF_DISK_BLOCK sizeof(float)*NUMBER_OF_FLOATS_IN_DISK_BLOCK
#define NaN 0x7FF00000

struct buffer{
	float* buf;
	int index;
	int file_data_position;
	float previous_record;
	buffer() {
		buf = (float*) malloc(SIZE_OF_DISK_BLOCK);
		index = 0;
		file_data_position = 0;
		previous_record = FLT_MAX;
	}
};

struct rw_op{
	// stores counters of read/write operations to files
	int read_ctr;
	int write_ctr;
	rw_op() {
		read_ctr = 0;
		write_ctr = 0;
	}
};


//								AUXILIARY FUNCTIONS

bool rand_with_probability(float probability) {
	float a = ((float)rand() / (float)RAND_MAX);
	return (a < probability);
}

int file_size(const char* file_name) {
	ifstream file;
	file.open(file_name, ios::binary | ios::ate);
	return (int)file.tellg();
}

int other(int zero_or_one) {
	return zero_or_one == 0 ? 1 : 0;
}

int other(int myindex, int t) {
	if (myindex == 0 || t == 0) {
		if (myindex == 1 || t == 1)
			return 2;
		else
			return 1;
	}
	else
		return 0;
}

const char* get_tape_name_from_file_name(char* txt) {
	char* a = (char*)malloc(sizeof(txt));
	char* b = txt;
	char* c = a;
	for (int i = 0; i < sizeof(txt), *b != '.'; i++) {
		*c = toupper(*b);
		c++;
		b++;
	}
	*c = '\0';
	return a;
}

bool is_greater_or_equal(float a, float b) {
	//if a >= b
	if (fabsf(roundf(a) - a) <= 0.00001f) {
		// a is an integer
		if (fabsf(roundf(b) - b) <= 0.00001f) {
			// a is an integer, b is an integer
			return (a >= b);
		}
		else {
			// a is an integer, b is a non-integer
			return true;
		}
	}
	else if (fabsf(roundf(b) - b) <= 0.00001f) {
		// a is a non-integer, b is an integer
		return false;
	}
	else
		return (a >= b);
}


//						FUNCTIONS DEALING WITH DATA HANDLING

void load_data(float** records, const char* file_name, int data_position=0) {
	// loads all data from file file_name, starting at data_position (number of bytes where to start) to array of floats starting at *records
	int fsize = file_size(file_name);
	if (data_position >= fsize) {
		*records = NULL;
		return;
	}
	*records = (float*)malloc(fsize);
	ifstream file;
	file.open(file_name, ios::in | ios::binary);
	file.seekg(data_position);
	file.read((char*)*records, fsize - data_position);
	file.close();
}

void store_data(float* records, const char* file_name, int no_records) {
	// stores all data from file file_name which is thus newly created (replaces former file file_name if existed)
	ofstream file;
	file.open(file_name, ios::out | ios::binary);
	file.write((char*)records, sizeof(float) * no_records);
	file.close();
}

int load_buffer(buffer* buffer, const char* file_name, rw_op* rw_ctr) {
	ifstream file;
	file.open(file_name, ios::in | ios::binary);
	file.seekg(buffer->file_data_position);
	file.read((char*)buffer->buf, SIZE_OF_DISK_BLOCK);
	int read_size = (int)file.gcount();
	buffer->file_data_position += read_size;
	rw_ctr->read_ctr++;
	file.close();
	return read_size;
}

void store_buffer(buffer* buffer, const char* file_name, int stored_records_size, rw_op* rw_ctr) {
	ofstream file;
	if (buffer->file_data_position == 0)
		file.open(file_name, ios::out | ios::binary);
	else
		file.open(file_name, ios::out | ios::app | ios::binary);
	file.seekp(buffer->file_data_position);
	file.write((char*)buffer->buf, stored_records_size);
	buffer->file_data_position += stored_records_size;
	rw_ctr->write_ctr++;
	file.close();
}

float get_record(buffer* buffer, const char* file_name, rw_op* rw_ctr) {
	float record;
	if (buffer->index < NUMBER_OF_FLOATS_IN_DISK_BLOCK && buffer->index > 0){
		record = buffer->buf[buffer->index];
		buffer->index++;
		return record;
	}
	if (buffer->index == -1) {
		record = buffer->buf[0];
		buffer->index = 1;
		return record;
	}
	int read_size = load_buffer(buffer, file_name, rw_ctr);
	if (read_size < SIZE_OF_DISK_BLOCK)
		buffer->buf[read_size / sizeof(float)] = NaN;
	buffer->index = 1;
	return buffer->buf[0];
}

void store_record(float record, buffer* buffer, const char* file_name, rw_op* rw_ctr) {
	if (buffer->index < NUMBER_OF_FLOATS_IN_DISK_BLOCK) {
		buffer->buf[buffer->index] = record;
		(buffer->index)++;
	}
	if (buffer->index == NUMBER_OF_FLOATS_IN_DISK_BLOCK) {
		store_buffer(buffer, file_name, SIZE_OF_DISK_BLOCK, rw_ctr);
		buffer->index = 0;
	}
}

//							OTHER USED MICRO LEVEL FUNCTIONS

void generate_data(float* records, int no_records, int range, float probablity_of_integer_generation) {
	if (records == NULL)
		return;
	srand((unsigned int)time(NULL));
	for (int i = 0; i < no_records; i++) {
		if (rand_with_probability(probablity_of_integer_generation))
			records[i] = (float)(rand() % range); //generating an integer
		else
			records[i] = ((float)rand()) / ((float)(float)RAND_MAX / (float)range); // generating a floating point number
	}
}

//								PRINT DATA FUNCTIONS

void print_records(float* records, int no_records) {
	printf("RECORDS:\n\n");
	if (records == NULL)
		return;
	int j = 0;
	for (int i = 0; i < no_records; i++) {
		if (j == 10) {
			printf("\n");
			j = 0;
		}
		
		if (i == 0 || !is_greater_or_equal(records[i], records[i - 1])/*records[i] < records[i - 1]*/) {
			j = 1;
			printf("\n\nRUN:  %f", records[i]);
		}
		else {
			j++;
			printf("\t%f", records[i]);
		}
	}
	printf("\n\n\n");
}

void read_and_print_tapes(int* file_data_position, char* file_name[], int no_files) {
	float* records;
	for (int i = 0; i < no_files; i++) {
		load_data(&records, file_name[i], file_data_position[i]);
		printf("\t\t\t\t\t%s\n\n", get_tape_name_from_file_name(file_name[i]));
		print_records(records, (file_size(file_name[i]) - file_data_position[i]) / sizeof(float));
		free(records);
	}
}

void read_and_print_tapes(char* file_name[], int no_files) {
	float* records;
	for (int i = 0; i < no_files; i++) {
		load_data(&records, file_name[i]);
		printf("\t\t\t\t\t%s\n\n", get_tape_name_from_file_name(file_name[i]));
		print_records(records, file_size(file_name[i]) / sizeof(float));
		free(records);
	}
}

void read_and_print_tape(char file_name[]) {
	float* records;
	load_data(&records, file_name);
	printf("\t\t%s\n\n", get_tape_name_from_file_name(file_name));
	print_records(records, file_size(file_name) / sizeof(float));
	free(records);
}

//									MACRO FUNCTIONS

void generate_and_store_records(int no_records, int range, const char* data, float probability) {
	float* records = (float*)malloc(sizeof(float) * no_records);

	generate_data(records, no_records, range, probability);
	store_data(records, data, no_records);

	free(records);
}

void distribution(buffer buffer[3], const char* data, int no_dummy_records[3], int no_runs_on_tape[3], const char* file_name[3], rw_op* rw_ctr) {
	int no_runs_to_be_on_tape[2] = { 1, 1 };
	bool without_dummies = true;

	int t = 0; //current tape number (values 0 or 1), signifies index in arrays

	int file_data_position[3] = { 0 };
	for (int i = 0; i < 3; i++)
		no_runs_on_tape[i] = 0;

	while (true) {
		float record = get_record(&buffer[2], data, rw_ctr);
		if (record == NaN)
			break;
		if (is_greater_or_equal(record, buffer[t].previous_record)) {
			// RUN CONTINUES
			store_record(record, &buffer[t], file_name[t], rw_ctr);
		}
		else {
			without_dummies = false;
			// NOT PART OF CURRENT RUN
			if (no_runs_on_tape[t] >= no_runs_to_be_on_tape[t]) {
				// CHANGE TAPE
				no_runs_to_be_on_tape[t] += no_runs_to_be_on_tape[other(t)];
				t = other(t);
			}
			store_record(record, &buffer[t], file_name[t], rw_ctr);
			if (!is_greater_or_equal(record, buffer[t].previous_record)) {
				//NOT COALESCING RUNS
				no_runs_on_tape[t]++;
			}
			else {
				// COALESCING RUNS; counters increase, but numbers of runs on tapes are fibonacci numbers; therefore, no need for dummies if this state persists
				without_dummies = true;
			}
		}
		buffer[t].previous_record = record;
	}
	for (int i = 0; i < 2; i++) {
		store_buffer(&buffer[i], file_name[i], buffer[i].index*sizeof(float), rw_ctr);
		if (no_runs_to_be_on_tape[i] < no_runs_to_be_on_tape[other(i)])
			no_dummy_records[i] = no_runs_to_be_on_tape[i] - no_runs_on_tape[i];
		else
			no_dummy_records[i] = 0;
	}
	if (without_dummies == true) {	//if no need for dummies overwrite previous distribution of dummy records
		no_dummy_records[0] = 0;
		no_dummy_records[1] = 0;
	}
	no_dummy_records[2] = 0;

	for (int i = 0; i < 3; i++) {
		buffer[i].index = 0;
		buffer[i].file_data_position = 0;
		buffer[i].previous_record = FLT_MAX;
	}
}

void rewrite_run(int i, int* rt, buffer buffer[3], const char* file_name[3], float record[3], bool* break_flag, rw_op* rw_ctr) {
	if (record[other(i, *rt)] == NaN)
		*break_flag = true;
	while ((is_greater_or_equal(record[other(i, *rt)], buffer[other(i, *rt)].previous_record)) && (record[other(i, *rt)] != NaN)){
		store_record(record[other(i, *rt)], &buffer[*rt], file_name[*rt], rw_ctr);
		buffer[other(i, *rt)].previous_record = record[other(i, *rt)];
		record[other(i, *rt)] = get_record(&buffer[other(i, *rt)], file_name[other(i, *rt)], rw_ctr);
		if (record[other(i, *rt)] == NaN)
			*break_flag = true;
	}
}

bool merge(buffer buffer[3], int no_dummy_records[3], int no_runs_on_tape[3], int first_byte_index_of_tape_display[3], const char* file_name[3], int* rt, rw_op* rw_ctr) {
	// rt - resulting tape

	bool finished = false;	//stores returned value, array of tapes where data will be left after this merge operation

	float record[3];
	bool continue_flag = false;
	bool break_flag = false;

	for (int i = 0; i < 3; i++) {
		if (i == *rt)
			continue;
		record[i] = get_record(&buffer[i], file_name[i], rw_ctr);
		if (record[i] == NaN)
			break_flag = true;
	}

	while (true) {
		if (break_flag == true)
			break;
		for (int i = 0; i < 3; i++) {
			if (i == *rt)
				continue;
			if (no_dummy_records[i] > 0) {
				//REWRITE RUN FROM records[other(i, *rt)] TO RESULTING_TAPE
				store_record(record[other(i, *rt)], &buffer[*rt], file_name[*rt], rw_ctr);
				buffer[other(i, *rt)].previous_record = record[other(i, *rt)];
				record[other(i, *rt)] = get_record(&buffer[other(i, *rt)], file_name[other(i, *rt)], rw_ctr);
				rewrite_run(i, rt, buffer, file_name, record, &break_flag, rw_ctr);
				no_dummy_records[i]--;
				continue_flag = true;
				break;
			}
			continue_flag = false;
		}
		if (continue_flag)
			continue;
		for (int i = 0; i < 3; i++) {
			if (i == *rt)
				continue;
			if (is_greater_or_equal(record[other(i, *rt)], record[i])/*record[i] <= record[other(i, *rt)]*/) {

 				store_record(record[i], &buffer[*rt], file_name[*rt], rw_ctr);
				buffer[i].previous_record = record[i];
				record[i] = get_record(&buffer[i], file_name[i], rw_ctr);

				if (record[i] == NaN)
					break_flag = true;

				if (!is_greater_or_equal(record[i], buffer[i].previous_record) || record[i] == NaN) { // if run finished
					//REWRITE RUN FROM records[other(i, *rt)] TO RESULTING_TAPE
					store_record(record[other(i, *rt)], &buffer[*rt], file_name[*rt], rw_ctr);
					buffer[other(i, *rt)].previous_record = record[other(i, *rt)];
					record[other(i, *rt)] = get_record(&buffer[other(i, *rt)], file_name[other(i, *rt)], rw_ctr);
					rewrite_run(i, rt, buffer, file_name, record, &break_flag, rw_ctr);
					break;
				}
			}
		}
	}
	store_buffer(&buffer[*rt], file_name[*rt], buffer[*rt].index * sizeof(float), rw_ctr);
	for (int i = 0; i < 3; i++) {
		if (i == *rt) {
			first_byte_index_of_tape_display[i] = 0;
			buffer[i].file_data_position = 0;
			buffer[i].index = 0;
			buffer[i].previous_record = -FLT_MAX;
		}
	}
	for (int i = 0; i < 3; i++) {
		if (i != *rt && buffer[i].buf[buffer[i].index - 1] != NaN) {
			if (buffer[i].file_data_position % (SIZE_OF_DISK_BLOCK) > 0)
				first_byte_index_of_tape_display[i] = buffer[i].file_data_position - (buffer[i].file_data_position % (SIZE_OF_DISK_BLOCK)) + (buffer[i].index - 1) * sizeof(float);
			else
				first_byte_index_of_tape_display[i] = buffer[i].file_data_position - SIZE_OF_DISK_BLOCK + ((buffer[i].index - 1) * sizeof(float));
			buffer[i].index--;
			if (buffer[i].index == 0)
				buffer[i].index = -1;
			buffer[i].previous_record = -FLT_MAX;
		}
	}

	int ctr = 0;
	for (int i = 0; i < 3; i++) {
		if (buffer[i].buf[buffer[i].index - 1] == NaN) {
			first_byte_index_of_tape_display[i] = buffer[i].file_data_position;
			buffer[i].file_data_position = 0;
			buffer[i].index = 0;
			buffer[i].previous_record = -FLT_MAX;
			*rt = i;
			ctr++;
			if (ctr == 2)
				finished = true;
		}
	}
	return finished;
}

void sort(bool show_flag, const char* data_file) {
	int no_dummy_records[3], no_runs_on_tape[3];
	int first_byte_index_of_tape_display[3] = { 0, 0, 0 };
	const char* file_name[2] = { "tape1.txt", "tape2.txt" };
	const char* file_name_all[3] = { "tape1.txt", "tape2.txt", "tape3.txt" };
	buffer buf[3] = {};
	rw_op rw_operations;
	bool finished = false;

	distribution(buf, data_file, no_dummy_records, no_runs_on_tape, file_name, &rw_operations);
	if (show_flag)
		read_and_print_tapes((char**)file_name, 2);
	printf("Runs\tTAPE1: %d\tTAPE2: %d\n", no_runs_on_tape[0], no_runs_on_tape[1]);
	if (no_runs_on_tape[0] + no_runs_on_tape[1] == 1)
		finished = true;
	printf("Dummy\tTAPE1: %d\tTAPE2: %d\n", no_dummy_records[0], no_dummy_records[1]);
	for (int i = 0; i < 3; i++)
		buf[i].previous_record = -FLT_MAX;
	int ret = 2;
	int phase_ctr = 0;
	while (finished == false)
	{
		phase_ctr++;
		finished = merge(buf, no_dummy_records, no_runs_on_tape, first_byte_index_of_tape_display, file_name_all, &ret, &rw_operations);
		if (show_flag && finished == false) {
			printf("\n\n\t\t\t\t\tMERGE %d:\n\n", phase_ctr);
			read_and_print_tapes(first_byte_index_of_tape_display, (char**)file_name_all, 3);
		}
	}
	printf("\n\n\t\t\t\t\tSORTED:\n\n");
	read_and_print_tapes(first_byte_index_of_tape_display, (char**)file_name_all, 3);
	printf("MEMORY OPERATIONS:\tREAD: %d\tWRITE: %d\t TOTAL: %d\n\n", rw_operations.read_ctr, rw_operations.write_ctr, rw_operations.read_ctr + rw_operations.write_ctr);
	printf("NUMBER OF PHASES (MERGE ONLY):\t%d\n\n", phase_ctr);
	printf("SIZE OF A DISK BLOCK:\t%d bytes = %d records\n\n\n", SIZE_OF_DISK_BLOCK, NUMBER_OF_FLOATS_IN_DISK_BLOCK);
}

//									TESTS

//							MAIN POINT OF ENTRY, INPUT HANDLING

int main(int argc, char* argv[]) {
	bool generate_flag = false; //generate data flag
	bool show_flag = false;		//show records after each phase flag
	bool file_flag = false;		//get records from file flag
	unsigned int no_records;
	unsigned int range = 1000;
	float probability;
	char* data = NULL;
	for (int i = 1; i < argc;) {
		if (strcmp(argv[i], "-r") == 0) {
			//GENERATE RANDOMLY
			generate_flag = true;
			i++;
			if (i < argc) {
				try {
					no_records = stoi(argv[i++]);
					range = stoi(argv[i++]);
					probability = stof(argv[i++]);
				}
				catch (const char* msg) {
					fprintf(stderr, "Invalid input for generation, ERROR: %s\n", msg);
					generate_flag = false;
				}
			}
			continue;
		}
		if (strcmp(argv[i], "-f") == 0) {
			//GET RECORDS FROM FILE
			if (i < argc) {
				try {
					i++;
					data = argv[i++];
					file_flag = true;
				}
				catch (const char* msg) {
					fprintf(stderr, "Invalid input file, ERROR: %s\n", msg);
				}
			}
			continue;
		}
		if (strcmp(argv[i], "-s") == 0) {
			//SHOW RECORDS AFTER EACH PHASE
			show_flag = true;
			i++;
			continue;
		}
		else
		{
			fprintf(stderr, "Incorrect argument(s)\n");
			i++;
		}
	}
	if (generate_flag) {
		generate_and_store_records(no_records, range, "data.txt", probability);
		
		read_and_print_tape((char*)"data.txt");

		sort(show_flag, "data.txt");
	}
	else if (file_flag) {
		read_and_print_tape(data);
		sort(show_flag, data);
	}
	else {
		cout << "Type the number of records: ";
		cin >> no_records;
		cout << "Type records: ";
		float* inputs = (float*) malloc(sizeof(float) * no_records);
		for (unsigned int i = 0; i < no_records; i++) {
			cin >> inputs[i];
		}
		cout << "Algorithm is working\n";
		store_data(inputs, "data.txt", no_records);
		sort(show_flag, "data.txt");
	}
	return 0;
}