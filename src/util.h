#ifndef UTIL_H
#define UTIL_H

#include <stdlib.h>
#include <string>
#include <iostream>
#include <vector>
#include <sys/stat.h>
#include <algorithm>
#include <time.h>
#include <mutex>
#include "common.h"

using namespace std;

inline bool starts_with(string const& value, string const& starting)
{
	if (starting.size() > value.size()) return false;
	return  equal(starting.begin(), starting.end(), value.begin());
}

inline bool ends_with(string const& value, string const& ending)
{
	if (ending.size() > value.size()) return false;
	return  equal(ending.rbegin(), ending.rend(), value.rbegin());
}

inline string trim(const string& str)
{
	string::size_type pos = str.find_first_not_of(' ');
	if (pos == string::npos)
	{
		return string("");
	}
	string::size_type pos2 = str.find_last_not_of(' ');
	if (pos2 != string::npos)
	{
		return str.substr(pos, pos2 - pos + 1);
	}
	return str.substr(pos);
}

inline int split(const string& str, vector<string>& ret_, string sep = ",")
{
	if (str.empty())
	{
		return 0;
	}

	string tmp;
	string::size_type pos_begin = str.find_first_not_of(sep);
	string::size_type comma_pos = 0;

	while (pos_begin != string::npos)
	{
		comma_pos = str.find(sep, pos_begin);
		if (comma_pos != string::npos)
		{
			tmp = str.substr(pos_begin, comma_pos - pos_begin);
			pos_begin = comma_pos + sep.length();
		}
		else
		{
			tmp = str.substr(pos_begin);
			pos_begin = comma_pos;
		}

		ret_.push_back(tmp);
		tmp.clear();
	}
	return 0;
}

inline void error_exit (const string& msg) {
	cerr << "Error: " << msg << endl;
	exit(-1);
}

inline string basename(const string& filename) {
	string::size_type pos = filename.find_last_of('/');
	if (pos == string::npos)
		return filename;
	else if (pos == filename.length() - 1)
		return ""; // a bad filename
	else
		return filename.substr(pos + 1, filename.length() - pos - 1);
}

#define ASSERT(condition, error_message) ((condition)? 0: assertion(__FILE__, __func__, __LINE__, error_message))

inline int assertion(const string &filePath, const string &function, int line, const string &info){
	//get file name
	string filename = basename(filePath);
	string err = filename + " " + function + " " + std::to_string(line) + ">> " +info;
	//throw exception
	throw std::runtime_error(err);
}

inline string dirname(const string& filename) {
	string::size_type pos = filename.find_last_of('/');
	if (pos == string::npos) {
		return "./";
	}
	else
		return filename.substr(0, pos + 1);
}

//Check if a string is a file or directory
inline bool file_exists(const  string& s)
{
	bool exists = false;
	if (s.length() > 0) {
		struct stat status;
		int result = stat(s.c_str(), &status);
		if (result == 0) {
			exists = true;
		}
	}
	return exists;
}

//check if a string is a directory
inline bool is_directory(const string& path) {
	bool isdir = false;
	struct stat status;
	// visual studion use _S_IFDIR instead of S_IFDIR
	//  http://msdn.microsoft.com/en-us/library/14h5k7ff.aspx
#ifdef _MSC_VER
#define S_IFDIR _S_IFDIR
#endif // _MSC_VER
	stat(path.c_str(), &status);
	if (status.st_mode & S_IFDIR) {
		isdir = true;
	}
	return isdir;
}

inline void check_file_valid(const string& s) {
	if (!file_exists(s)) {
		cerr << "ERROR: file '" << s << "' doesn't exist, quit now" << endl;
		exit(-1);
	}
	if (is_directory(s)) {
		cerr << "ERROR: '" << s << "' is a folder, not a file, quit now" << endl;
		exit(-1);
	}
}

inline void check_file_writable(const  string& s) {
	string dir = dirname(s);
	if (!file_exists(dir)) {
		cerr << "ERROR: '" << dir << " doesn't exist. Create this folder and run this command again." << endl;
		exit(-1);
	}
	if (is_directory(s)) {
		cerr << "ERROR: '" << s << "' is not a writable file, quit now" << endl;
		exit(-1);
	}
}

inline char complement(char base) {
	switch (base) {
	case 'A':
	case 'a':
		return 'T';
	case 'T':
	case 't':
		return 'A';
	case 'C':
	case 'c':
		return 'G';
	case 'G':
	case 'g':
		return 'C';
	default:
		return 'N';
	}
}

inline string reverse(const string& str) {
	string ret(str.length(), 0);
	for (int pos = 0; pos < str.length(); pos++) {
		ret[pos] = str[str.length() - pos - 1];
	}
	return ret;
}

inline string reverseComplement(const string& str) {
	string rc(str.length(), 0);
	for (int pos = 0; pos < str.length(); pos++) {
		rc[pos] = complement(str[str.length() - pos - 1]);
	}
	return rc;
}

/*
			keep this 2 bits
							   ||
	A		65	01000|00|1	0
	C		67	01000|01|1	1
	G		71	01000|11|1	3
	T		84	01010|10|0	2
*/
inline uint64 seqEncode(const char* sequence, const int& seqStart, const int& seqLen, bool isRC=false) {
	uint64 n = 0;
	uint64  k = 0;
	for (int i = seqStart; i < seqLen; i++) {
		n = (sequence[i] & 6) >> 1; //6:  ob00000110
		if (isRC) {
			n = RC_BASE[n];
			k |= (n << ((seqLen-i-1) * 2));
		}
		else {
			k |= (n << (i * 2));
		}
	}
	return k;
}

inline string seqDecode(const uint64& seqInt, const int& seqLen) {
	uint8_t tint ;
	string seqs ="";
	for (int i = 0; i < seqLen; i++) {
		tint = (seqInt >> (i * 2)) & 3;
		seqs.push_back(ATCG_BASES[tint]);
	}
	return seqs;	
}

inline uint64 getPolyTint(const int& seqLen){
	char ployT[] = "TTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTT";
	return seqEncode(ployT, 0, seqLen);
}

inline int possibleMis(int bl,  int mismatch) {
	switch (mismatch) {
	case 1:
		return bl * 3;
	case 2:
		return bl * 3 + bl * (bl - 1) * 3 * 3 / 2;
	case 3:
		return bl * 3 + bl * (bl - 1) * 3 * 3 / 2 + bl * (bl - 1) * (bl - 2) * 3 * 3 * 3 / 6;
	default:
		return 0;
	}
}

extern mutex logmtx;
inline void loginfo(const string s) {
	logmtx.lock();
	time_t tt = time(NULL);
	tm* t = localtime(&tt);
	cerr << "[" << t->tm_hour << ":" << t->tm_min << ":" << t->tm_sec << "] " << s << endl;
	logmtx.unlock();
}

#endif
