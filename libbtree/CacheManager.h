#pragma once
#include <memory>
#include "ICacheManager.h"

template <typename CacheType>
class CacheManager : public ICacheManager
{
private:
	std::shared_ptr<CacheType> m_ptrCache;

public:
    ~CacheManager()
    {
    }

    CacheManager()
    {
		m_ptrCache = std::make_shared<CacheType>(10);
    }

	template<typename KeyType, class ClassType>
	std::shared_ptr<ClassType> getObjectOfType(KeyType& objKey/*, Hints for nvm!!*/)
	{
		std::cout << "CacheManager::getObjectOfType" << std::endl;

		return nullptr;
	}

	template<typename KeyType, class ClassType, typename... ClassArgsType>
	KeyType createObjectOfType(ClassArgsType ... args)
	{
		std::cout << "CacheManager::createObjectOfType" << std::endl;
		//oe->init();
		return NULL;
	}
};

