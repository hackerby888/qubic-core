#define NO_UEFI

#include <random>
#include <map>
#include <iostream>
#include "contract_testing.h"

using namespace std;
static const id user(1, 2, 3, 4);
static const id user2(1, 2, 3, 3);
static const id FEIYU_CONTRACT_ID(FEIYU_CONTRACT_INDEX, 0, 0, 0);

typedef HashMap<uint64, ResolveData, MAX_NUMBER_OF_SUBDOMAINS> SubdomainMap;

template <uint64 LENGTH = MAX_NAME_LENGTH>
class UEFIString2 : public UEFIString<LENGTH> {
public:
	static UEFIString<LENGTH> fromCString(string cString) {
		assert(cString.length() <= LENGTH);
		UEFIString<LENGTH> output;
		for (int i = 0; i < cString.length(); i++) {
			output.set(i, cString.at(i));
		}
		
		return output;
	}

	static string toCString(UEFIString<LENGTH> output) {
		assert(output.capacity() <= LENGTH);
		string cString;
		cString.resize(output.capacity());
		uint8 uefiStringSize = 0;
		for (int i = 0; i < output.capacity(); i++) {
			if (output.get(i) == 0) {
				uefiStringSize = i + 1;
				break;
			}
		}
		cString.resize(uefiStringSize);
		for (int i = 0; i < output.capacity(); i++) {
			if (output.get(i) == 0) {
				break;
			}
			cString.at(i) = output.get(i);
		}

		return cString;
	}
};

class FeiyuChecker : FEIYU {
public:
	void removeDomainResolveData(Domain domain, bool isDeleteAll, SubdomainMap& gmap) {
		if (isDeleteAll) {
			resolveData.removeByKey(domain.getRootHashedvalue());
		}
		else {
			SubdomainMap map;
			resolveData.get(domain.getRootHashedvalue(), map);
			map.removeByKey(domain.getFullHashedValue());
			gmap = map;
		}
	}
};

class FeiyuTest : public ContractTesting {
public:
	FeiyuTest() {
		initEmptySpectrum();
		initEmptyUniverse();
		INIT_CONTRACT(FEIYU);
	}

	void removeDomainResolveData(Domain domain, bool isDeleteAll) {
		increaseEnergy(user, 1);
		SubdomainMap map;
		getState()->removeDomainResolveData(domain, isDeleteAll, map);
		FEIYU::UpdateSubdomainMap_input input;
		input.domain = domain;
		input.subdomainHashMap = map;
		FEIYU::UpdateSubdomainMap_output output;
		invokeUserProcedure(FEIYU_CONTRACT_INDEX, 12, input, output, user, 0);
		EXPECT_EQ(output.result, SUCCESS);
	}

	void testInsertDomain(Domain domain, sint8 expectedOutputCode = 0) {
		increaseEnergy(user, 1);

		FEIYU::RegisterDomain_input input;
		input.domain = domain;
		input.registrationYears = 1;
		FEIYU::RegisterDomain_output output;
		invokeUserProcedure(FEIYU_CONTRACT_INDEX, 8, input, output, user, 0);

		EXPECT_EQ(output.result, expectedOutputCode);
	}

	void testGetDomainResolveData(Domain domain, ResolveData expectedResolveData, uint8 expectedOutputCode) {
		increaseEnergy(user, 1);
		FEIYU::GetResolveData_input input;
		input.domain = domain;
		FEIYU::GetResolveData_output output;

		callFunction(FEIYU_CONTRACT_INDEX, 12, input, output);
		EXPECT_EQ(output.result, expectedOutputCode);
		if (output.result == SUCCESS) {
			EXPECT_EQ(output.data, expectedResolveData);
		}
	}

	void testSetResolveData(Domain domain, ResolveData resolveData, uint8 expectedOutputCode) {
		increaseEnergy(user, 1);
		FEIYU::SetResolveData_input input;
		input.data = resolveData;
		input.domain = domain;
		FEIYU::SetResolveData_output output;

		invokeUserProcedure(FEIYU_CONTRACT_INDEX, 9, input, output, user, 0);
		EXPECT_EQ(output.result, expectedOutputCode);
	}

	void testGetDomainRegistry(Domain domain, RegistryRecord record, uint8 expectedOutputCode) {
		increaseEnergy(user, 1);
		FEIYU::GetDomainRegistry_input input;
		input.domain = domain;
		FEIYU::GetDomainRegistry_output output;
		callFunction(FEIYU_CONTRACT_INDEX, 10, input, output);
		if (output.result == 0) {
			EXPECT_EQ(output.record.owner, record.owner);
			cout << output.record.registerEpoch << endl;
		}
	}

	void testTransferToNewOwner(Domain domain, id newOwner) {
		increaseEnergy(user, 1);
		FEIYU::TransferDomain_input input;
		input.domain = domain;
		input.newOwner = newOwner;
		FEIYU::TransferDomain_output output;
		invokeUserProcedure(FEIYU_CONTRACT_INDEX, 11, input, output, user, 0);
		EXPECT_EQ(output.result, 0);
	}

	FeiyuChecker* getState()
	{
		return (FeiyuChecker*)contractStates[FEIYU_CONTRACT_INDEX];
	}
};

//TEST(Aleo, TestSysCall) {
//	string text("pro player");
//	uefiString uefiText;
//	UEFIStringHelper::initEmptyUEFIString(uefiText);
//	UEFIStringHelper::cStringToUEFIString(text, uefiText);
//
//	string text2;
//	UEFIStringHelper::UEFIStringToCString(uefiText, text2);
//
//	cout << text2 << endl;
//
//	FeiyuTest test;
//	test.testSyscall();
//}

//TEST(Aleo, TestStateNumber) {
//	FeiyuTest test;
//	test.testStateNumber();
//}
//
//TEST(Aleo, TestMoveMoney) {
//	FeiyuTest test;
//	test.testMoveMoney();
//}
//
//TEST(Aleo, TestReturnBackMoney) {
//	FeiyuTest test;
//	test.testReturnBack();
//}
//
//TEST(Aleo, TestString) {
//	FeiyuTest test;
//	test.testString();
//}

//TEST(Aleo, StringValidate) {
//	string text("a");
//	uefiString uefiText;
//	UEFIStringHelper::initEmptyUEFIString(uefiText);
//	UEFIStringHelper::cStringToUEFIString(text, uefiText);
//
//	EXPECT_EQ(QNSHelper::validateName(uefiText), true);
//}

TEST(Aleo, TestAddDomain) {
	HashMap<uint64, uint64, 1> testMap;
	testMap.set(1, 2);
	testMap.set(2, 3);
	testMap.isEmptySlot(0); // should return true
	uint64 value;
	testMap.get(1, value);
	cout << "1" << value << endl;
	testMap.get(2, value);
	cout << "2" << value << endl;
	cout << "total bytes contract" << sizeof(HashMap<uint64, RegistryRecord, MAX_NUMBER_OF_DOMAINS>) + (sizeof(HashMap<uint64, SubdomainMap, MAX_NUMBER_OF_DOMAINS>)) << endl;
	FeiyuTest test;
	Domain domain(UEFIString2<>::getEmpty(), UEFIString2<>::fromCString("example"), UEFIString2<MAX_TLD_LENGTH>::fromCString("com"));
	Domain domain2(UEFIString2<>::getEmpty(), UEFIString2<>::fromCString("example2"), UEFIString2<MAX_TLD_LENGTH>::fromCString("com"));
	Domain domain3(UEFIString2<>::getEmpty(), UEFIString2<>::fromCString("example"), UEFIString2<MAX_TLD_LENGTH>::fromCString("org"));

	test.testInsertDomain(domain, SUCCESS);
	test.testInsertDomain(domain, Error::NAME_ALREADY_REGISTERED);

	// test get resolve data
	ResolveData resolveData;
	ResolveData resolveData2;
	test.testSetResolveData(domain, resolveData, SUCCESS);
	// example2.com is not registered, so it should return error
	test.testSetResolveData(domain2, resolveData, Error::NAME_NOT_REGISTERED);
	test.testSetResolveData(domain3, resolveData, Error::NAME_NOT_REGISTERED);
	//test.removeDomainResolveData(domain, false);
	test.testGetDomainResolveData(domain, resolveData, SUCCESS);
	test.testGetDomainResolveData(domain2, resolveData, Error::NAME_NOT_REGISTERED);

	// should get error because example.com is not registered (delete)
	test.removeDomainResolveData(domain, false);
	test.testGetDomainResolveData(domain, resolveData, Error::NAME_HAS_NO_RESOLVE_DATA);

	domain.setSubDomain(UEFIString2<>::fromCString("ok"));
	// only example.com should have resolve data not ok.example.com, so this should return error
	test.testGetDomainResolveData(domain, resolveData, Error::NAME_HAS_NO_RESOLVE_DATA);

	RegistryRecord record;
	record.owner = user;
	test.testGetDomainRegistry(domain, record, SUCCESS);
	domain.setSubDomain(UEFIString2<>::fromCString("ok"));
	test.testGetDomainRegistry(domain, record, SUCCESS);

	// test transfer domain
	test.testTransferToNewOwner(domain, user2);
	record.owner = user2;
	test.testGetDomainRegistry(domain, record, SUCCESS);
	//SubdomainMap subMap;
	//uint64 key = subMap.key(0);
	//ResolveData data = subMap.value(0);
	//ResolveData empty;
	//EXPECT_EQ(empty, data);
	//cout << "test key" << key << endl;

	//uint32 res;
	//QUOTTERY::packQuotteryDate(25, 6, 30, 0, 0, 0, res);
	//cout << "test get day" << QUOTTERY::qtryGetDay(res) << endl;
	//cout << "test datae" << res << endl;
}