#pragma once
#include <memory>
#include "ICacheManager.h"

template </*typename ABC,*/ template <typename, typename> typename CacheType, typename CacheKeyType, typename CacheValueType>
class CacheManager : public ICacheManager
{
private:
	std::shared_ptr<CacheType<CacheKeyType, CacheValueType>> m_ptrCache;
	//ABC* oe;
public:
    ~CacheManager()
    {

    }

    CacheManager(/*ABC* oe*/)
    {
		//this->oe = oe;
        m_ptrCache = std::make_shared<CacheType<CacheKeyType, CacheValueType>>(10000);
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

