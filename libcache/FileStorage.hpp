#pragma once
#include <memory>
#include <iostream>
#include <fcntl.h>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <variant>

#include "ErrorCodes.h"

class TypeProcessor {
public:
	void process(int value) const {
		std::cout << "Processing int: " << value << std::endl;
	}

	void process(double value) const {
		std::cout << "Processing double: " << value << std::endl;
	}

	void process(const std::string& value) const {
		std::cout << "Processing string: " << value << std::endl;
	}

};


template <size_t N, typename... Types>
struct NthType;

template <typename FirstType, typename... RemainingTypes>
struct NthType<0, FirstType, RemainingTypes...> {
	using type = FirstType;
};

template <size_t N, typename FirstType, typename... RemainingTypes>
struct NthType<N, FirstType, RemainingTypes...> {
	using type = typename NthType<N - 1, RemainingTypes...>::type;
};

template<typename KeyType, template <typename, typename...> typename ValueType, typename ValueCoreTypesMarshaller, typename... ValueCoreTypes>
class FileStorage
{
	using type = typename NthType<0, ValueCoreTypes...>::type;
	using type2 = typename NthType<1, ValueCoreTypes...>::type;
	//using type21 = typename NthType<2, ValueCoreTypes...>::type;


	typedef ValueType<ValueCoreTypesMarshaller, ValueCoreTypes...> StorageValueType;

private:
	size_t m_nFileSize;
	size_t m_nBlockSize;

	std::string m_stFilename;
	std::fstream m_fsStorage;

	std::vector<bool> m_vtAllocationTable;

public:
	FileStorage(size_t nBlockSize, size_t nFileSize, std::string stFilename)
		: m_nFileSize(nFileSize)
		, m_nBlockSize(nBlockSize)
		, m_stFilename(stFilename)
	{
		m_vtAllocationTable.resize(nBlockSize/nFileSize, false);

		//m_fsStorage.rdbuf()->pubsetbuf(0, 0);
		//m_fsStorage.open(stFilename.c_str(), std::ios::binary | std::ios::in | std::ios::out);

		if (!m_fsStorage.is_open())
		{
			//throw new exception("should not occur!");   // TODO: critical log.
		}
	}

	std::shared_ptr<StorageValueType> getObject(KeyType ptrKey)
	{
		char* szBuffer = new char[m_nBlockSize];

		m_fsStorage.read(szBuffer, m_nBlockSize);

		return nullptr;
	}

	CacheErrorCode addObject(KeyType ptrKey, std::shared_ptr<StorageValueType> ptrValue)
	{
		
		std::tuple<const std::byte*, size_t> _bytes = ptrValue->serialize();
		//std::string serializedValue = std::visit(ValueCoreTypesMarshaller{}, ptrValue);

		std::byte* __ = new std::byte[std::get<1>(_bytes)];

		memcpy(__, std::get<0>(_bytes), std::get<1>(_bytes));

		ptrValue->deserialize(__);

		std::variant<int, double, std::string> myVariant;

		TypeProcessor typeProcessor;

		std::visit([&typeProcessor](const auto& value) {
			typeProcessor.process(value);
			}, myVariant);	

		//ptrValue->data->get();
		return CacheErrorCode::Success;
	}
};

