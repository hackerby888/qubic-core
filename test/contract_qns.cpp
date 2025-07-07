#define NO_UEFI

#include <random>
#include <map>
#include <iostream>
#include "contract_testing.h"

using namespace std;
static const id user(1, 2, 3, 4);
static const id user2(1, 2, 3, 3);
static const id QNS_CONTRACT_ID(QNS_CONTRACT_INDEX, 0, 0, 0);
constexpr sint64 BASE_MONEY = 1'000'000'000;

template <uint64 LENGTH = MAX_NAME_LENGTH>
using UEFIString = QNS::UEFIString<LENGTH>;
using ResolveData = QNS::ResolveData;
using Domain = QNS::Domain;
using RegistryRecord = QNS::RegistryRecord;
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
		INIT_CONTRACT(QNS);

		callSystemProcedure(QNS_CONTRACT_INDEX, INITIALIZE);
	}

	QNS::RegisterDomain_output registerDomain(Domain domain, uint16 registrationYears, sint64 fee) {
		QNS::RegisterDomain_input input;
		input.domain = domain;
		input.registrationYears = registrationYears;
		QNS::RegisterDomain_output output;
		EXPECT_TRUE(invokeUserProcedure(QNS_CONTRACT_INDEX, 8, input, output, getCaller(), fee));

		return output;
	}

	QNS::GetResolveTextData_output getDomainResolveTextData(Domain domain) {
		QNS::GetResolveTextData_input input;
		input.domain = domain;
		QNS::GetResolveTextData_output output;
		callFunction(QNS_CONTRACT_INDEX, 13, input, output);

		return output;
	}

	QNS::GetResolveAddressData_output getDomainResolveAddressData(Domain domain) {
		QNS::GetResolveAddressData_input input;
		input.domain = domain;
		QNS::GetResolveAddressData_output output;
		callFunction(QNS_CONTRACT_INDEX, 12, input, output);

		return output;
	}

	QNS::SetResolveAddressData_output setResolveAddressData(Domain domain, id address) {
		QNS::SetResolveAddressData_input input;
		input.address = address;
		input.domain = domain;
		QNS::SetResolveAddressData_output output;
		EXPECT_TRUE(invokeUserProcedure(QNS_CONTRACT_INDEX, 9, input, output, getCaller(), 0));

		return output;
	}

	QNS::SetResolveTextData_output setResolveTextData(Domain domain, UEFIString<> text) {
		QNS::SetResolveTextData_input input;
		input.text = text;
		input.domain = domain;
		QNS::SetResolveTextData_output output;
		EXPECT_TRUE(invokeUserProcedure(QNS_CONTRACT_INDEX, 10, input, output, getCaller(), 0));

		return output;
	}

	QNS::GetDomainRegistryRecord_output GetDomainRegistryRecord(Domain domain) {
		QNS::GetDomainRegistryRecord_input input;
		input.domain = domain;
		QNS::GetDomainRegistryRecord_output output;
		callFunction(QNS_CONTRACT_INDEX, 10, input, output);

		return output;
	}

	QNS::TransferDomain_output transferDomain(Domain domain, id newOwner, sint64 fee) {
		QNS::TransferDomain_input input;
		input.domain = domain;
		input.newOwner = newOwner;
		QNS::TransferDomain_output output;
		EXPECT_TRUE(invokeUserProcedure(QNS_CONTRACT_INDEX, 11, input, output, getCaller(), fee));
		return output;
	}

	QNS::RenewDomain_output renewDomain(Domain domain, uint16 yearsToRenew, sint64 fee) {
		QNS::RenewDomain_input input;
		input.domain = domain;
		input.yearsToRenew = yearsToRenew;
		QNS::RenewDomain_output output;
		EXPECT_TRUE(invokeUserProcedure(QNS_CONTRACT_INDEX, 13, input, output, getCaller(), fee));
		return output;
	}

	void endEpoch() {
		callSystemProcedure(QNS_CONTRACT_INDEX, BEGIN_EPOCH);
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
	QNS::RegisterDomain_output output;
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

	QNS::GetDomainRegistryRecord_output getDomainRegistryOutput;
	getDomainRegistryOutput = test.GetDomainRegistryRecord(domainValid);
	EXPECT_EQ(getDomainRegistryOutput.result, QNS_SUCCESS_CODE);
	EXPECT_EQ(getDomainRegistryOutput.record.owner, user);
	EXPECT_TRUE(getDomainRegistryOutput.record.registerEpoch == system.epoch ? 1 : 0);
	EXPECT_TRUE(getDomainRegistryOutput.record.registrationYears == yearsToRegister ? 1 : 0);
	uint32 date;
	QUOTTERY::packQuotteryDate(utcTime.Year - 2000, utcTime.Month, utcTime.Day, utcTime.Hour, utcTime.Minute, utcTime.Second, date);
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
	QNS::RegisterDomain_output output;
	output = test.registerDomain(domainValid, yearsToRegister, yearsToRegister * REGISTER_FEE_PER_YEAR);
	EXPECT_EQ(output.result, QNS_SUCCESS_CODE);

	ResolveData resolveData;
	resolveData.address = user2;
	resolveData.text = UEFIString2<>::fromCString("text data");
	test.setResolveAddressData(domainValid, resolveData.address);
	test.setResolveTextData(domainValid, resolveData.text);

	QNS::GetResolveTextData_output getResolveTextDataOutput;
	getResolveTextDataOutput = test.getDomainResolveTextData(domainValid);
	EXPECT_EQ(getResolveTextDataOutput.result, QNS_SUCCESS_CODE);
	EXPECT_EQ(getResolveTextDataOutput.text, resolveData.text);

	QNS::GetResolveAddressData_output getResolveAddressDataOutput;
	getResolveAddressDataOutput = test.getDomainResolveAddressData(domainValid);
	EXPECT_EQ(getResolveAddressDataOutput.result, QNS_SUCCESS_CODE);
	EXPECT_EQ(getResolveAddressDataOutput.address, resolveData.address);

	// Test for invalid domain
	Domain domainInvalid(UEFIString2<>::getEmpty(), UEFIString2<>::fromCString("invalid"), UEFIString2<MAX_TLD_LENGTH>::fromCString("qns"));
	QNS::GetResolveTextData_output getResolveTextDataInvalidOutput;
	getResolveTextDataInvalidOutput = test.getDomainResolveTextData(domainInvalid);
	EXPECT_EQ(getResolveTextDataInvalidOutput.result, Error::NAME_NOT_REGISTERED);
	QNS::GetResolveAddressData_output getResolveAddressDataInvalidOutput;
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
	QNS::RegisterDomain_output output;
	output = test.registerDomain(domainValid, yearsToRegister, yearsToRegister * REGISTER_FEE_PER_YEAR);
	EXPECT_EQ(output.result, QNS_SUCCESS_CODE);
	QNS::TransferDomain_output transferOutput;
	test.setCaller(user2);
	transferOutput = test.transferDomain(domainValid, user2, TRANSFER_DOMAIN_FEE);
	EXPECT_EQ(transferOutput.result, Error::NOT_THE_OWNER);
	test.setCaller(user);
	transferOutput = test.transferDomain(domainValid, user2, 1);
	EXPECT_EQ(transferOutput.result, Error::INVALID_FUND);
	test.setCaller(user);
	transferOutput = test.transferDomain(domainValid, user2, TRANSFER_DOMAIN_FEE);
	EXPECT_EQ(transferOutput.result, QNS_SUCCESS_CODE);
	QNS::GetDomainRegistryRecord_output getDomainRegistryOutput;
	getDomainRegistryOutput = test.GetDomainRegistryRecord(domainValid);
	EXPECT_EQ(getDomainRegistryOutput.result, QNS_SUCCESS_CODE);
	EXPECT_EQ(getDomainRegistryOutput.record.owner, user2);

	// Test renew domain
	QNS::RenewDomain_output renewOutput;
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
	QNS::RegisterDomain_output output;
	output = test.registerDomain(domainValid, yearsToRegister, yearsToRegister * REGISTER_FEE_PER_YEAR);
	EXPECT_EQ(output.result, QNS_SUCCESS_CODE);

	system.epoch = 100 + EPOCHS_IN_YEAR * yearsToRegister + 1; // make sure the domain is expried
	test.endEpoch(); // call end epoch to clean up expired domains
	QNS::GetDomainRegistryRecord_output getDomainRegistryOutput;
	getDomainRegistryOutput = test.GetDomainRegistryRecord(domainValid);
	EXPECT_EQ(getDomainRegistryOutput.result, Error::NAME_NOT_REGISTERED);
}
