#ifndef OPERATION_HPP
#define OPERATION_HPP

#include <iostream>
#include <inttypes.h>

using namespace std;

class Operation
{
public:
	enum OperationType
	{
		MALLOC = 0,
		CALLOC,
		FREE,
		MMAP,
		MUNMAP,
		NUM_TYPES
	};

	Operation(OperationType type, uint64_t start, uint64_t size, string library, string caller, uint64_t callerAddr):
		_type(type), _start(start), _size(size), _callerAddr(callerAddr)
	{
		_library.assign(library);
		_caller.assign(caller);
	}

	Operation(const Operation &other)
	{
		this->_type = other._type;
		this->_start = other._start;
		this->_size = other._size;
		this->_library.assign(other._library);
		this->_caller.assign(other._caller);
		this->_callerAddr = other._callerAddr;
	}

	OperationType getType() const  { return _type; }
	uint64_t getStart() const { return _start; }
	uint64_t getSize() const { return _size; }
	string getLibrary() const { return _library; }
	string getCaller() const { return _caller; }
	uint64_t getCallerAddr() const { return _callerAddr; }

	void setStart(uint64_t start) { _start = start; }
	void setSize(uint64_t size) { _size = size; }

	friend ostream& operator<<(ostream& os, const Operation &info);

private:
	OperationType _type;
	uint64_t _start;
	uint64_t _size;
	string _library;
	string _caller;
	uint64_t _callerAddr;
};


class AddressComparator
{
public:
	bool operator() (const Operation *first, const Operation *second) {
		return first->getStart() < second->getStart();
	}
};

ostream& operator<<(ostream& os, const Operation &info)
{
	const char *typeNames[Operation::NUM_TYPES] = { "malloc", "calloc", "free", "mmap", "munmap" };
	os << typeNames[info.getType()] << ": 0x" <<  hex << info.getStart() << " 0x" << hex << info.getSize();
	os << " " << info.getLibrary() << " " << info.getCaller() << " " << hex << info.getCallerAddr() << dec;
	return os;
}
#endif
