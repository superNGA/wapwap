// Practical 1
// Description : Write a C++ program to calculate the grade of a student based on marks obtained in 5 subjects.as;dlfkja;lsdkfja;lsdkfj;alsdjf;alsdkfj;alksdfjalskdjf;alsdjflkasjdlfkja;sldfja;lsdkfjasd;fa;lsjdfasfd
//
// Created : 30/10/2026
#include <iostream>


int main(int argn, char **argc)
{
	const int MAX_MARKS_PER_SUBJ = 100;
	const int GRADE_VARIETY = 6;
	int marks[]     = {80, 80, 80, 80, 99};
	int subj_count  = sizeof(marks) / sizeof(marks[0]);
	int total_marks = 0;
	for (int i = 0; i < subj_count; ++i)
		total_marks += marks[i];
	int max_marks = MAX_MARKS_PER_SUBJ * subj_count;
	float marks_perc = (float)total_marks / (float)max_marks;
	std::cout << total_marks << " / " << max_marks << '\n';
	std::cout << (char)('A' + (int)((float)GRADE_VARIETY * (1.0 - marks_perc))) << '\n';
	return 0;
}
