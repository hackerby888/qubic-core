#define NO_UEFI

#include <random>
#include <map>
#include <iostream>
#include "contract_testing.h"

using namespace std;
static const id user(1, 2, 3, 4);
static const id user2(1, 2, 3, 3);
static const id FEIYU_CONTRACT_ID(FEIYU_CONTRACT_INDEX, 0, 0, 0);
constexpr sint64 BASE_MONEY = 1'000'000'000;

template <uint64 LENGTH = MAX_NAME_LENGTH>
using UEFIString = FEIYU::UEFIString<LENGTH>;
using ResolveData = FEIYU::ResolveData;
using Domain = FEIYU::Domain;
using RegistryRecord = FEIYU::RegistryRecord;
using SubdomainMap = HashMap<uint64, ResolveData, MAX_NUMBER_OF_SUBDOMAINS>;

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

class FeiyuTest : public ContractTesting {
private:
	id __caller;
public:
	void setCaller(id caller) {
		__caller = caller;
	}

	id getCaller() {
		return __caller;
	}

	FeiyuTest() {
		initEmptySpectrum();
		initEmptyUniverse();
		INIT_CONTRACT(FEIYU);

		callSystemProcedure(FEIYU_CONTRACT_INDEX, INITIALIZE);
	}

	FEIYU::RegisterDomain_output registerDomain(Domain domain, uint16 registrationYears, sint64 fee) {
		FEIYU::RegisterDomain_input input;
		input.domain = domain;
		input.registrationYears = registrationYears;
		FEIYU::RegisterDomain_output output;
		EXPECT_TRUE(invokeUserProcedure(FEIYU_CONTRACT_INDEX, 8, input, output, getCaller(), fee));

		return output;
	}

	FEIYU::GetResolveTextData_output getDomainResolveTextData(Domain domain) {
		FEIYU::GetResolveTextData_input input;
		input.domain = domain;
		FEIYU::GetResolveTextData_output output;
		callFunction(FEIYU_CONTRACT_INDEX, 13, input, output);

		return output;
	}

	FEIYU::GetResolveAddressData_output getDomainResolveAddressData(Domain domain) {
		FEIYU::GetResolveAddressData_input input;
		input.domain = domain;
		FEIYU::GetResolveAddressData_output output;
		callFunction(FEIYU_CONTRACT_INDEX, 12, input, output);

		return output;
	}

	FEIYU::SetResolveAddressData_output setResolveAddressData(Domain domain, id address) {
		FEIYU::SetResolveAddressData_input input;
		input.address = address;
		input.domain = domain;
		FEIYU::SetResolveAddressData_output output;
		EXPECT_TRUE(invokeUserProcedure(FEIYU_CONTRACT_INDEX, 9, input, output, getCaller(), 0));

		return output;
	}

	FEIYU::SetResolveTextData_output setResolveTextData(Domain domain, UEFIString<> text) {
		FEIYU::SetResolveTextData_input input;
		input.text = text;
		input.domain = domain;
		FEIYU::SetResolveTextData_output output;
		EXPECT_TRUE(invokeUserProcedure(FEIYU_CONTRACT_INDEX, 10, input, output, getCaller(), 0));

		return output;
	}

	FEIYU::GetDomainRegistryRecord_output GetDomainRegistryRecord(Domain domain) {
		FEIYU::GetDomainRegistryRecord_input input;
		input.domain = domain;
		FEIYU::GetDomainRegistryRecord_output output;
		callFunction(FEIYU_CONTRACT_INDEX, 10, input, output);

		return output;
	}

	FEIYU::TransferDomain_output transferDomain(Domain domain, id newOwner, sint64 fee) {
		FEIYU::TransferDomain_input input;
		input.domain = domain;
		input.newOwner = newOwner;
		FEIYU::TransferDomain_output output;
		EXPECT_TRUE(invokeUserProcedure(FEIYU_CONTRACT_INDEX, 11, input, output, getCaller(), fee));
		return output;
	}

	FEIYU::RenewDomain_output renewDomain(Domain domain, uint16 yearsToRenew, sint64 fee) {
		FEIYU::RenewDomain_input input;
		input.domain = domain;
		input.yearsToRenew = yearsToRenew;
		FEIYU::RenewDomain_output output;
		EXPECT_TRUE(invokeUserProcedure(FEIYU_CONTRACT_INDEX, 13, input, output, getCaller(), fee));
		return output;
	}

	void endEpoch() {
		callSystemProcedure(FEIYU_CONTRACT_INDEX, BEGIN_EPOCH);
	}
};

TEST(QNS, RegisterDomain) {
	system.epoch = 77;
	utcTime.Second = 7;
	utcTime.Minute = 7;
	utcTime.Hour = 7;
	utcTime.Day = 7;
	utcTime.Month = 7;
	utcTime.Year = 2025;
	//updateTime();
	updateQpiTime();
	uint16 yearsToRegister = 1;
	FeiyuTest test;
	test.setCaller(user);
	Domain domainValid(UEFIString2<>::getEmpty(), UEFIString2<>::fromCString("example"), UEFIString2<MAX_TLD_LENGTH>::fromCString("qns"));
	Domain domainTldInvalid(UEFIString2<>::getEmpty(), UEFIString2<>::fromCString("example"), UEFIString2<MAX_TLD_LENGTH>::fromCString("com"));
	Domain domainRootEmpty(UEFIString2<>::getEmpty(), UEFIString2<>::fromCString(""), UEFIString2<MAX_TLD_LENGTH>::fromCString("qns"));

	increaseEnergy(user, BASE_MONEY);
	FEIYU::RegisterDomain_output output;
	output = test.registerDomain(domainValid, yearsToRegister, 1);
	EXPECT_EQ(output.result, Error::INVALID_FUND);
	output = test.registerDomain(domainValid, yearsToRegister, yearsToRegister * REGISTER_FEE_PER_YEAR);
	EXPECT_EQ(getBalance(user), BASE_MONEY - yearsToRegister * REGISTER_FEE_PER_YEAR);
	output = test.registerDomain(domainTldInvalid, yearsToRegister, yearsToRegister * REGISTER_FEE_PER_YEAR);
	EXPECT_EQ(output.result, Error::INVALID_TLD);
	output = test.registerDomain(domainRootEmpty, yearsToRegister, yearsToRegister * REGISTER_FEE_PER_YEAR);
	EXPECT_EQ(output.result, Error::INVALID_NAME);
	// After 2 fails to register, user should still have the same balance
	EXPECT_EQ(getBalance(user), BASE_MONEY - yearsToRegister * REGISTER_FEE_PER_YEAR);

	FEIYU::GetDomainRegistryRecord_output getDomainRegistryOutput;
	getDomainRegistryOutput = test.GetDomainRegistryRecord(domainValid);
	EXPECT_EQ(getDomainRegistryOutput.result, QNS_SUCCESS_CODE);
	EXPECT_EQ(getDomainRegistryOutput.record.owner, user);
	EXPECT_TRUE(getDomainRegistryOutput.record.registerEpoch == system.epoch ? 1 : 0);
	EXPECT_TRUE(getDomainRegistryOutput.record.registrationYears == yearsToRegister ? 1 : 0);
	uint32 date;
	QUOTTERY::packQuotteryDate(utcTime.Year-2000, utcTime.Month, utcTime.Day, utcTime.Hour, utcTime.Minute, utcTime.Second, date);
	EXPECT_TRUE(getDomainRegistryOutput.record.registerDate == date ? 1 : 0);
	getDomainRegistryOutput = test.GetDomainRegistryRecord(domainTldInvalid);
	EXPECT_EQ(getDomainRegistryOutput.result, Error::NAME_NOT_REGISTERED);
}

TEST(QNS, SetAndGetResolveData) {
	uint16 yearsToRegister = 1;
	FeiyuTest test;
	test.setCaller(user);
	Domain domainValid(UEFIString2<>::getEmpty(), UEFIString2<>::fromCString("example"), UEFIString2<MAX_TLD_LENGTH>::fromCString("qns"));
	increaseEnergy(user, BASE_MONEY);
	FEIYU::RegisterDomain_output output;
	output = test.registerDomain(domainValid, yearsToRegister, yearsToRegister * REGISTER_FEE_PER_YEAR);
	EXPECT_EQ(output.result, QNS_SUCCESS_CODE);

	ResolveData resolveData;
	resolveData.address = user2;
	resolveData.text = UEFIString2<>::fromCString("text data");
	test.setResolveAddressData(domainValid, resolveData.address);
	test.setResolveTextData(domainValid, resolveData.text);

	FEIYU::GetResolveTextData_output getResolveTextDataOutput;
	getResolveTextDataOutput = test.getDomainResolveTextData(domainValid);
	EXPECT_EQ(getResolveTextDataOutput.result, QNS_SUCCESS_CODE);
	EXPECT_EQ(getResolveTextDataOutput.text, resolveData.text);

	FEIYU::GetResolveAddressData_output getResolveAddressDataOutput;
	getResolveAddressDataOutput = test.getDomainResolveAddressData(domainValid);
	EXPECT_EQ(getResolveAddressDataOutput.result, QNS_SUCCESS_CODE);
	EXPECT_EQ(getResolveAddressDataOutput.address, resolveData.address);

	// Test for invalid domain
	Domain domainInvalid(UEFIString2<>::getEmpty(), UEFIString2<>::fromCString("invalid"), UEFIString2<MAX_TLD_LENGTH>::fromCString("qns"));
	FEIYU::GetResolveTextData_output getResolveTextDataInvalidOutput;
	getResolveTextDataInvalidOutput = test.getDomainResolveTextData(domainInvalid);
	EXPECT_EQ(getResolveTextDataInvalidOutput.result, Error::NAME_NOT_REGISTERED);
	FEIYU::GetResolveAddressData_output getResolveAddressDataInvalidOutput;
	getResolveAddressDataInvalidOutput = test.getDomainResolveAddressData(domainInvalid);
	EXPECT_EQ(getResolveAddressDataInvalidOutput.result, Error::NAME_NOT_REGISTERED);

}

TEST(QNS, TransferAndRenewDomain) {
	uint16 yearsToRegister = 1;
	FeiyuTest test;
	test.setCaller(user);
	Domain domainValid(UEFIString2<>::getEmpty(), UEFIString2<>::fromCString("example"), UEFIString2<MAX_TLD_LENGTH>::fromCString("qns"));
	increaseEnergy(user, BASE_MONEY);
	increaseEnergy(user2, BASE_MONEY);
	FEIYU::RegisterDomain_output output;
	output = test.registerDomain(domainValid, yearsToRegister, yearsToRegister * REGISTER_FEE_PER_YEAR);
	EXPECT_EQ(output.result, QNS_SUCCESS_CODE);
	FEIYU::TransferDomain_output transferOutput;
	test.setCaller(user2);
	transferOutput = test.transferDomain(domainValid, user2, TRANSFER_DOMAIN_FEE);
	EXPECT_EQ(transferOutput.result, Error::NOT_THE_OWNER);
	test.setCaller(user);
	transferOutput = test.transferDomain(domainValid, user2, 1);
	EXPECT_EQ(transferOutput.result, Error::INVALID_FUND);
	test.setCaller(user);
	transferOutput = test.transferDomain(domainValid, user2, TRANSFER_DOMAIN_FEE);
	EXPECT_EQ(transferOutput.result, QNS_SUCCESS_CODE);
	FEIYU::GetDomainRegistryRecord_output getDomainRegistryOutput;
	getDomainRegistryOutput = test.GetDomainRegistryRecord(domainValid);
	EXPECT_EQ(getDomainRegistryOutput.result, QNS_SUCCESS_CODE);
	EXPECT_EQ(getDomainRegistryOutput.record.owner, user2);

	// Test renew domain
	FEIYU::RenewDomain_output renewOutput;
	test.setCaller(user);
	renewOutput = test.renewDomain(domainValid, 1, 1);
	EXPECT_EQ(renewOutput.result, Error::NOT_THE_OWNER);
	test.setCaller(user2);
	renewOutput = test.renewDomain(domainValid, 1, 1);
	EXPECT_EQ(renewOutput.result, Error::INVALID_FUND);
	test.setCaller(user2);
	renewOutput = test.renewDomain(domainValid, 1, REGISTER_FEE_PER_YEAR);
	EXPECT_EQ(renewOutput.result, QNS_SUCCESS_CODE);
	getDomainRegistryOutput = test.GetDomainRegistryRecord(domainValid);
	EXPECT_EQ(getDomainRegistryOutput.result, QNS_SUCCESS_CODE);
	EXPECT_TRUE(getDomainRegistryOutput.record.registrationYears == yearsToRegister + 1 ? 1 : 0);
}

TEST(QNS, DomainExpried) {
	system.epoch = 100;
	uint16 yearsToRegister = 1;
	FeiyuTest test;
	test.setCaller(user);
	Domain domainValid(UEFIString2<>::getEmpty(), UEFIString2<>::fromCString("example"), UEFIString2<MAX_TLD_LENGTH>::fromCString("qns"));
	increaseEnergy(user, BASE_MONEY);
	increaseEnergy(user2, BASE_MONEY);
	FEIYU::RegisterDomain_output output;
	output = test.registerDomain(domainValid, yearsToRegister, yearsToRegister * REGISTER_FEE_PER_YEAR);
	EXPECT_EQ(output.result, QNS_SUCCESS_CODE);

	system.epoch = 100 + EPOCHS_IN_YEAR * yearsToRegister + 1; // make sure the domain is expried
	test.endEpoch(); // call end epoch to clean up expired domains
	FEIYU::GetDomainRegistryRecord_output getDomainRegistryOutput;
	getDomainRegistryOutput = test.GetDomainRegistryRecord(domainValid);
	EXPECT_EQ(getDomainRegistryOutput.result, Error::NAME_NOT_REGISTERED);
}