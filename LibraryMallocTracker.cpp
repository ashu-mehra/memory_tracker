#include <iostream>
#include <fstream>
#include <vector>
#include <regex>
#include <string>
#include <unistd.h>
#include <inttypes.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>

using namespace std;

class LibraryInfo;
class AddrRange;

static const unsigned long long HEX_CONVERT_ERROR = 0xffffffffffffffff;
static string pageMapFileName;

static uint64_t hex2ull(const std::string& hexNumber);
static bool isPageInMemory(uint64_t ptr);
static uint64_t getInMemoryBytes(AddrRange &range);
static void parseLibMallocInfo(vector<LibraryInfo> &libinfo);

class AddrRange
{
private:
	uint64_t _start;
	uint64_t _end;
public:
	AddrRange(uint64_t start, uint64_t end) : _start(start), _end(end) { }
	uint64_t getStart() const { return _start; }
	uint64_t getEnd() const { return _end; }
};

class LibraryInfo
{
private:
	string _name;
	vector<AddrRange> _allocationList;

public:
	LibraryInfo(const string &name): _name(name) {}

	string getName() const { return _name; }
	vector<AddrRange> & getAllocationList() { return _allocationList; }

	void addRange(AddrRange &range) {
		_allocationList.push_back(range);
	}

};

static uint64_t 
hex2ull(const std::string& hexNumber)
{
	uint64_t res = 0;
	unsigned int start = 0;

	if (hexNumber.size() > 2 && hexNumber.at(0) == '0' && hexNumber.at(1) == 'x')
		start += 2; // jump over 0x

	for (unsigned int i = start; i < hexNumber.size(); i++)
	{
		unsigned char digit = hexNumber.at(i);
		if (digit >= '0' && digit <= '9')
			res = (res << 4) + digit - '0';
		else if (digit >= 'A' && digit <= 'F')
			res = (res << 4) + digit - 'A' + 10;
		else if (digit >= 'a' && digit <= 'f')
			res = (res << 4) + digit - 'a' + 10;
		else {
			std::cerr << "Conversion error for " << hexNumber << std::endl;
			return HEX_CONVERT_ERROR; // error
		}
	}
	return res;
}

static bool 
isPageInMemory(uint64_t ptr)
{
	static int pagesize = getpagesize();
	static int fd = open(pageMapFileName.c_str(), O_RDONLY);

	if (fd == -1) {
		cerr << "Eror in opening pagemap\n";	      
		return false;
	}
	
        uint64_t offset = (ptr / pagesize) * sizeof(uint64_t);
	if (-1 != lseek(fd, offset, SEEK_SET)) {
		uint64_t value = 0;		    
		read(fd, &value, sizeof(value));
		cout << "page: " << hex << ptr << "\tvalue: " << hex << value << dec << "\n";
		if (value & 0x8000000000000000) {
			return true;
		} else {
			return false;
		}
	} else {
		cerr << "lseek failed for page " << hex << ptr << dec << "\n";
		return false;
	}		    
	return false;
}

static uint64_t
getInMemoryBytes(AddrRange &range)
{
	uint64_t start = range.getStart();
	uint64_t end = range.getEnd();
	uint64_t pageAlignedStart = start;
	uint64_t pageAlignedEnd = end;
	int pagesize = getpagesize();		 
	uint64_t count = 0;

	if (start & (pagesize-1)) pageAlignedStart = (start + (pagesize-1)) & ~(pagesize-1); // round up start to next page boundary
	if (end & (pagesize-1)) pageAlignedEnd = end & ~(pagesize-1); // round down end to page boundary

	if (pageAlignedStart > pageAlignedEnd) {
		// this can happen if start and end are in same page
		if (isPageInMemory(pageAlignedEnd)) {
			return end - start;
		} else {
			return 0;
		}
	}

	uint64_t addr = pageAlignedStart;
	while (addr < pageAlignedEnd) {
		if (isPageInMemory(addr)) count += pagesize;
		addr += pagesize;
	}

	if (start != pageAlignedStart) {
		uint64_t page = start & ~(pagesize-1);
		if (isPageInMemory(page)) {		    
			count += (page + pagesize) - start;
		}
	}
	if (end != pageAlignedEnd) {
		uint64_t page = end & ~(pagesize-1);
		if (isPageInMemory(page)) {		    
			count += (end - page);
		}
	}
	return count;	 
}

static void 
parseLibMallocInfo(vector<LibraryInfo> &libinfo)
{
	// Open the file
	string filename = "malloc_tracing.txt";
	ifstream myfile(filename);

	// check if successfull
	if (!myfile.is_open()) {
		cerr << "Cannot open " << filename << endl;
		exit(-1);
	}

	//std::regex pattern("([0-9a-f]+)-([0-9a-f]+) (\\S\\S\\S\\S) ([0-9a-f]+) ([0-9a-f]+:[0-9a-f]+) (\\d+)\\s*(\\S*)");
	//std::regex pattern("malloc: (\\S*) (\\S*) 0x([0-9a-f]+) 0x([0-9a-f]+) ([0-9a-f]+)");
	std::regex pattern("0x([0-9a-f]+)\\s*0x([0-9a-f]+)\\s*at (\\S*) ([0-9a-f]+)");

	string line;
	while (myfile.good()) {
		getline(myfile, line);

		std::smatch result;
		if (std::regex_search(line, result, pattern)) {
			cerr << "Match found: start=" << result[1] << " size=" << result[2] << " lib=" << result[3] << endl;
			uint64_t start = hex2ull(result[1]);
			uint64_t size = hex2ull(result[2]);
			uint64_t end = start + size;
			AddrRange range(start, end);
			bool found = false;

			for (auto &lib: libinfo) {
				if (lib.getName() == result[3]) {
					found = true;
					lib.addRange(range);
					break;
				}
			}
			if (!found) {
				LibraryInfo lib(result[3]);
				lib.addRange(range);
				libinfo.push_back(lib);
			}
		}
	}
}

static void
printLibraryInfo(vector<LibraryInfo> &libinfo)
{
	cout << "Printing library info (list size=" << libinfo.size() << ")\n";
	for (LibraryInfo &info: libinfo) {
		vector<AddrRange> & rangeList = info.getAllocationList();
		cout << "\tlibrary: " << info.getName() << " (total allocations=" << rangeList.size() << "):" << endl;
		uint64_t total = 0;
		for (AddrRange &range: rangeList) {
			uint64_t start = range.getStart();
			uint64_t end = range.getEnd();
			uint64_t inMemory = getInMemoryBytes(range);
			cout << hex << "\t\tstart=" << start << " end=" << end << dec << " size=" << (end-start) << " bytes" << " rss=" << inMemory << " bytes" << "\n";
			total += inMemory;
		}
		cout << "\tlibrary: " << info.getName() << " (total rss=" << total << " bytes):" << endl;
		cout << endl;
	}
}

int 
main(int argc, char *argv[])
{
	if (argc < 2) {
		fprintf(stderr, "Insufficient arguments. Need pid\n");
		exit(-1);
	}
	string pagemap("/proc/");
	pagemap.append(argv[1]).append("/pagemap");
	pageMapFileName.assign(pagemap);
	vector<LibraryInfo> libinfo;
	parseLibMallocInfo(libinfo);
	printLibraryInfo(libinfo);
}
