#define NO_UEFI

#include <random>
#include <map>
#include <iostream>
#include "contract_testing.h"

using namespace std;
static const id user(1, 2, 3, 4);
static const id user2(1, 2, 3, 3);
static const id FEIYU_CONTRACT_ID(FEIYU_CONTRACT_INDEX, 0, 0, 0);

class UEFIString2 : public UEFIString {
public:
	static UEFIString fromCString(string cString) {
		UEFIString output;
		for (int i = 0; i < cString.length(); i++) {
			output.set(i, cString.at(i));
		}
		
		return output;
	}

	static string toCString(UEFIString output) {
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
	void checkState(sint16 toBe) {
		
	}
};

class FeiyuTest : public ContractTesting {
public:
	FeiyuTest() {
		initEmptySpectrum();
		initEmptyUniverse();
		INIT_CONTRACT(FEIYU);
	}

	void testSyscall() {
		increaseEnergy(user, 1);
		callSystemProcedure(FEIYU_CONTRACT_INDEX, BEGIN_EPOCH);
		getState()->checkState(20);


	}

	void testInsertDomain(Domain domain, sint8 expectedOutputCode = 0) {
		increaseEnergy(user, 1);

		FEIYU::RegisterDomain_input input;
		input.domain = domain;
		input.endEpoch = EPOCHS_IN_YEAR * 2;
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
	FeiyuTest test;
	Domain domain(UEFIString2::getEmpty(), UEFIString2::fromCString("example"), UEFIString2::fromCString("com"));
	Domain domain2(UEFIString2::getEmpty(), UEFIString2::fromCString("example2"), UEFIString2::fromCString("com"));
	Domain domain3(UEFIString2::getEmpty(), UEFIString2::fromCString("example"), UEFIString2::fromCString("org"));

	test.testInsertDomain(domain, SUCCESS);
	test.testInsertDomain(domain, Error::NAME_ALREADY_REGISTERED);

	// test get resolve data
	ResolveData resolveData;
	ResolveData resolveData2;
	test.testSetResolveData(domain, resolveData, SUCCESS);
	// example2.com is not registered, so it should return error
	test.testSetResolveData(domain2, resolveData, Error::NAME_NOT_REGISTERED);
	test.testSetResolveData(domain3, resolveData, Error::NAME_NOT_REGISTERED);
	test.testGetDomainResolveData(domain, resolveData, SUCCESS);
	test.testGetDomainResolveData(domain2, resolveData, Error::NAME_NOT_REGISTERED);
	domain.setSubDomain(UEFIString2::fromCString("ok"));
	// only example.com should have resolve data not ok.example.com, so this should return error
	test.testGetDomainResolveData(domain, resolveData, Error::NAME_HAS_NO_RESOLVE_DATA);

	RegistryRecord record;
	record.owner = user;
	test.testGetDomainRegistry(domain, record, SUCCESS);
	domain.setSubDomain(UEFIString2::fromCString("ok"));
	test.testGetDomainRegistry(domain, record, SUCCESS);

	// test transfer domain
	test.testTransferToNewOwner(domain, user2);
	record.owner = user2;
	test.testGetDomainRegistry(domain, record, SUCCESS);

	//uint32 res;
	//QUOTTERY::packQuotteryDate(25, 6, 30, 0, 0, 0, res);
	//cout << "test get day" << QUOTTERY::qtryGetDay(res) << endl;
	//cout << "test datae" << res << endl;
}