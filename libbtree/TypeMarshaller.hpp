#pragma once
#include <variant>
#include <typeinfo>

#include <iostream>
#include <fstream>

#include "DataNode.hpp"
#include "IndexNode.hpp"
#include "VariadicNthType.h"
enum Types
{
	//DataNode = 0,
	//IndexNode = 0
} typedef TYPES_ID;

class TypeMarshaller 
{
public:
	template <typename... ObjectCoreTypes>
	static void serialize(std::fstream& os, const std::variant<std::shared_ptr<ObjectCoreTypes>...>& myVariant, uint8_t& uidObjectType, size_t& nDataSize)
	{
		std::visit([&os, &uidObjectType, &nDataSize](const auto& value) {
			value->writeToStream(os, uidObjectType, nDataSize);
			}, myVariant);
	}

	template <typename ObjectType, typename... ObjectCoreTypes>
	static void deserialize(std::fstream& is, std::shared_ptr<ObjectType>& ptrObject)
	{
		using TypeA = typename NthType<0, ObjectCoreTypes...>::type;
		using TypeB = typename NthType<1, ObjectCoreTypes...>::type;
		//using TypeC = typename NthType<2, ObjectCoreTypes...>::type;

		uint8_t nTag;
		is.read(reinterpret_cast<char*>(&nTag), sizeof(uint8_t));

		switch (nTag)
		{
		case TypeA::UID:
			ptrObject = std::make_shared<ObjectType>(std::make_shared<TypeA>(is));
			break;
		case TypeB::UID:
			ptrObject = std::make_shared<ObjectType>(std::make_shared<TypeB>(is));
			break;
		}
	}
};
