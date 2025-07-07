using namespace QPI;

static constexpr uint8 QNS_SUCCESS_CODE = 0;
static constexpr uint64 MAX_NUMBER_OF_DOMAINS = 524'288;
static constexpr uint64 MAX_NUMBER_OF_SUBDOMAINS = 16;
static constexpr uint8 EPOCHS_IN_YEAR = 52; 
static constexpr uint8 MAX_NAME_LENGTH = 32;
static constexpr uint8 MIN_NAME_LENGTH = 1;
static constexpr uint64 MAX_TLD_LENGTH = 8;
static constexpr sint64 REGISTER_FEE_PER_YEAR = 2'000'000;
static constexpr sint64 TRANSFER_DOMAIN_FEE = 100;

struct QNSLogger
{
	uint32 _contractIndex;
	uint32 _type; // Assign a random unique (per contract) number to distinguish messages of different types
	char _terminator; // Only data before "_terminator" are logged
};

enum Error {
	INVALID_NAME = 1,
	INVALID_TLD,
	INVALID_SUBDOMAIN,
	NAME_NOT_FOUND,
	NAME_HAS_NO_RESOLVE_DATA,
	NAME_NOT_REGISTERED,
	NAME_ALREADY_REGISTERED,
	INVALID_DATE,
	NOT_THE_OWNER,
	INVALID_FUND
};

struct QNS2
{
};
struct QNS : public ContractBase
{
public:
	////// ******** Data Structures ******** //////

	template <uint64 LENGTH = MAX_NAME_LENGTH>
	struct UEFIString : public Array<sint8, LENGTH> {
	public:
		UEFIString() {
			for (uint64 i = 0; i < this->capacity(); i++) {
				this->set(i, 0);
			}
		}

		bool isEmpty() {
			return this->rangeEquals(0, this->capacity(), 0);
		}

		bool validate() {
			if (this->capacity() > MAX_NAME_LENGTH || this->capacity() < MIN_NAME_LENGTH) {
				return false;
			}

			uint64 nullTerminatorIndex = 0;
			for (uint64 i = 0; i < this->capacity(); i++) {
				if (this->get(i) == 0) {
					nullTerminatorIndex = i;
					break;
				}
			}
			if (nullTerminatorIndex < MIN_NAME_LENGTH) {
				return false;
			}

			// Ensure all characters behind the null terminator are also null
			if (!this->rangeEquals(nullTerminatorIndex, this->capacity(), 0)) {
				return false;
			}

			// Check if name is printable characters
			for (uint64 i = 0; i < this->capacity(); i++) {
				sint8 code = this->get(i);
				if (code == 0) {
					break;
				}
				if (!((code >= 97 && code <= 122) || // a-z
					(code >= 65 && code <= 90) || // A-Z
					(code >= 48 && code <= 57))) { // 0-9
					return false;
				}
			}

			return true;
		}

		static UEFIString getEmpty() {
			return UEFIString();
		}

		bool operator==(const UEFIString& other) const {
			if (this->capacity() != other.capacity()) {
				return false;
			}
			for (uint64 i = 0; i < this->capacity(); i++) {
				if (this->get(i) != other.get(i)) {
					return false;
				}
			}
			return true;
		}
	};

	struct Domain {
		UEFIString<> subDomain;
		UEFIString<> rootDomain;
		UEFIString<MAX_TLD_LENGTH> tld;

		static HashFunction<UEFIString<>> nameHasher;
		static HashFunction< UEFIString<MAX_TLD_LENGTH>> tldHasher;
		static HashFunction<uint64> uint64Hasher;

		void setSubDomain(UEFIString<> subDomain) {
			this->subDomain = subDomain;
		}

		bit operator==(Domain& domain) {
			if (!(subDomain == domain.subDomain))
				return false;
			if (!(rootDomain == domain.rootDomain))
				return false;
			if (!(tld == domain.tld))
				return false;

			return true;
		}

		bit validate() {
			if (!subDomain.isEmpty() && !subDomain.validate())
				return false;
			if (!rootDomain.validate())
				return false;
			if (!tld.validate())
				return false;

			return true;
		}

		// If domain is sub.example.com then will return the hash of sub.example.com
		uint64 getFullHashedValue() const {
			return uint64Hasher.hash(nameHasher.hash(subDomain) + nameHasher.hash(rootDomain) + tldHasher.hash(tld));
		}

		// If domain is sub.example.com then will return the hash of example.com
		uint64 getRootHashedvalue() const {
			return uint64Hasher.hash(nameHasher.hash(rootDomain) + tldHasher.hash(tld));
		}
	};

	struct RegistryRecord {
		id owner;
		uint32 registerDate;
		uint16 registerEpoch;
		uint16 registrationYears;

	public:
		RegistryRecord() {
			owner = NULL_ID;
			registrationYears = 0;
		}

		RegistryRecord(id owner, uint32 registerDate, uint16 registerEpoch, uint16 registrationYears) {
			this->owner = owner;
			this->registerDate = registerDate;
			this->registerEpoch = registerEpoch;
			this->registrationYears = registrationYears;
		}

		bit operator==(const RegistryRecord& record) const {
			return this->owner == record.owner && this->registerDate == record.registerDate && this->registerEpoch == record.registerEpoch && this->registrationYears == record.registrationYears;
		}
	};

	struct ResolveData {
		id address;
		UEFIString<> text;

		ResolveData() {
			this->address = id::zero();
		}

		ResolveData(const id& address, const UEFIString<>& text) {
			this->address = address;
			this->text = text;
		}

		bit operator==(const ResolveData& data) const {
			return address == data.address && text == data.text;
		}
	};


	////// ******** States ******** //////

	UEFIString<MAX_TLD_LENGTH> QUBIC_TLD;
	UEFIString<MAX_TLD_LENGTH> QNS_TLD;
	Array<UEFIString<MAX_TLD_LENGTH>, 2> TLDs;
	HashMap<uint64, RegistryRecord, MAX_NUMBER_OF_DOMAINS> registry;
	HashMap<uint64, HashMap<uint64, ResolveData, MAX_NUMBER_OF_SUBDOMAINS>, MAX_NUMBER_OF_DOMAINS> resolveData;

	////// ******** Functions & Procedures ******** //////

	struct RegisterDomain_input {
		Domain domain;
		uint16 registrationYears;
	};
	struct RegisterDomain_output {
		uint8 result;
	};
	struct RegisterDomain_locals {
		QNSLogger logger;
		RegistryRecord record;
		uint32 date;
		HashMap<uint64, ResolveData, MAX_NUMBER_OF_SUBDOMAINS> subdomainHashMap;
		uint16 registerEpoch;
		bit isTldSupported;
		uint64 i;
	};
	PUBLIC_PROCEDURE_WITH_LOCALS(RegisterDomain) {
		if (!input.domain.validate()) {
			output.result = Error::INVALID_NAME;
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			return;
		}

		if (state.registry.get(input.domain.getRootHashedvalue(), locals.record)) {
			output.result = Error::NAME_ALREADY_REGISTERED;
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			return;
		}

		// Check if tld is supported
		locals.isTldSupported = false;
		for (locals.i = 0; locals.i < state.TLDs.capacity(); locals.i++) {
			if (input.domain.tld == state.TLDs.get(locals.i)) {
				locals.isTldSupported = true;
				break;
			}
		}
		if (!locals.isTldSupported) {
			output.result = Error::INVALID_TLD;
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			return;
		}

		if (qpi.invocationReward() < input.registrationYears * REGISTER_FEE_PER_YEAR) {
			output.result = Error::INVALID_FUND;
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			return;
		}

		QUOTTERY::getCurrentDate(qpi, locals.date);
		output.result = QNS_SUCCESS_CODE;
		locals.logger = { SELF_INDEX, QNS_SUCCESS_CODE, 0 };
		LOG_INFO(locals.logger);
		locals.record.owner = qpi.invocator();
		locals.record.registerDate = locals.date;
		locals.record.registerEpoch = qpi.epoch();
		locals.record.registrationYears = input.registrationYears;
		state.registry.set(input.domain.getRootHashedvalue(), locals.record);
		state.resolveData.set(input.domain.getRootHashedvalue(), locals.subdomainHashMap);
	}

	struct RenewDomain_input {
		Domain domain;
		uint16 yearsToRenew;
	};
	struct RenewDomain_output {
		uint8 result;
	};
	struct RenewDomain_locals {
		QNSLogger logger;
		RegistryRecord record;
	};
	PUBLIC_PROCEDURE_WITH_LOCALS(RenewDomain) {
		if (!state.registry.get(input.domain.getRootHashedvalue(), locals.record)) {
			output.result = Error::NAME_NOT_REGISTERED;
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			return;
		}

		if (locals.record.owner != qpi.invocator()) {
			output.result = Error::NOT_THE_OWNER;
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			return;
		}

		if (qpi.invocationReward() < input.yearsToRenew * REGISTER_FEE_PER_YEAR) {
			output.result = Error::INVALID_FUND;
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			return;
		}

		locals.logger = { SELF_INDEX, QNS_SUCCESS_CODE, 0 };
		LOG_INFO(locals.logger);
		locals.record.registrationYears += input.yearsToRenew;
		state.registry.set(input.domain.getRootHashedvalue(), locals.record);
	}

	struct GetResolveAddressData_input {
		Domain domain;
	};
	struct GetResolveAddressData_output {
		uint8 result;
		id address;
	};
	struct GetResolveAddressData_locals {
		RegistryRecord record;
		ResolveData resolveData;
		HashMap<uint64, ResolveData, MAX_NUMBER_OF_SUBDOMAINS> subdomainHashMap;
	};
	PUBLIC_FUNCTION_WITH_LOCALS(GetResolveAddressData) {
		if (!state.registry.get(input.domain.getRootHashedvalue(), locals.record)) {
			output.result = Error::NAME_NOT_REGISTERED;
			return;
		}

		if (!state.resolveData.get(input.domain.getRootHashedvalue(), locals.subdomainHashMap)) {
			output.result = Error::NAME_HAS_NO_RESOLVE_DATA;
			return;
		}

		if (!locals.subdomainHashMap.get(input.domain.getFullHashedValue(), locals.resolveData)) {
			output.result = Error::NAME_HAS_NO_RESOLVE_DATA;
			return;
		}

		output.address = locals.resolveData.address;
		output.result = QNS_SUCCESS_CODE;
	}

	struct GetResolveTextData_input {
		Domain domain;
	};
	struct GetResolveTextData_output {
		uint8 result;
		UEFIString<> text;
	};
	struct GetResolveTextData_locals {
		RegistryRecord record;
		ResolveData resolveData;
		HashMap<uint64, ResolveData, MAX_NUMBER_OF_SUBDOMAINS> subdomainHashMap;
	};
	PUBLIC_FUNCTION_WITH_LOCALS(GetResolveTextData) {
		// Check if the domain exists in the registry
		if (!state.registry.get(input.domain.getRootHashedvalue(), locals.record)) {
			output.result = Error::NAME_NOT_REGISTERED;
			return;
		}
		// Check if the resolve data exists for the domain
		if (!state.resolveData.get(input.domain.getRootHashedvalue(), locals.subdomainHashMap)) {
			output.result = Error::NAME_HAS_NO_RESOLVE_DATA;
			return;
		}
		// Check if the resolve data for the full domain exists in the subdomain hashmap
		if (!locals.subdomainHashMap.get(input.domain.getFullHashedValue(), locals.resolveData)) {
			output.result = Error::NAME_HAS_NO_RESOLVE_DATA;
			return;
		}

		output.text = locals.resolveData.text;
		output.result = QNS_SUCCESS_CODE;
	}

	struct TransferDomain_input {
		Domain domain;
		id newOwner;
	};
	struct TransferDomain_output {
		uint8 result;
	};
	struct TransferDomain_locals {
		QNSLogger logger;
		RegistryRecord record;
	};
	PUBLIC_PROCEDURE_WITH_LOCALS(TransferDomain) {
		// Check if the owner paid enough fee
		if (qpi.invocationReward() < TRANSFER_DOMAIN_FEE) {
			output.result = Error::INVALID_FUND;
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			return;
		}
		// Check if the domain exists in the registry
		if (!state.registry.get(input.domain.getRootHashedvalue(), locals.record)) {
			output.result = Error::NAME_NOT_REGISTERED;
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			return;
		}
		// Check if the invocator is the owner of the domain
		if (locals.record.owner != qpi.invocator()) {
			output.result = Error::NOT_THE_OWNER;
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			return;
		}

		locals.logger = { SELF_INDEX, QNS_SUCCESS_CODE, 0 };
		LOG_INFO(locals.logger);
		// Set the new owner in the registry record
		locals.record.owner = input.newOwner;
		state.registry.set(input.domain.getRootHashedvalue(), locals.record);
		output.result = QNS_SUCCESS_CODE;
	}

	struct GetDomainRegistryRecord_input {
		Domain domain;
	};
	struct GetDomainRegistryRecord_output {
		uint8 result;
		RegistryRecord record;
	};
	struct GetDomainRegistryRecord_locals {
		RegistryRecord record;
		uint64 domainHashedvalue;
	};
	PUBLIC_FUNCTION_WITH_LOCALS(GetDomainRegistryRecord) {
		locals.domainHashedvalue = input.domain.getRootHashedvalue();
		if (!state.registry.get(locals.domainHashedvalue, locals.record)) {
			output.result = Error::NAME_NOT_REGISTERED;
			return;
		}
		output.result = QNS_SUCCESS_CODE;
		output.record = locals.record;
	}

	struct SetResolveAddressData_input {
		Domain domain;
		id address;
	};
	struct SetResolveAddressData_output {
		uint8 result;
	};
	struct SetResolveAddressData_locals {
		QNSLogger logger;
		id address;
		ResolveData resolveData;
		RegistryRecord record;
		HashMap<uint64, ResolveData, MAX_NUMBER_OF_SUBDOMAINS> subdomainHashMap;
	};
	PUBLIC_PROCEDURE_WITH_LOCALS(SetResolveAddressData) {
		// Check if the domain exists in the registry
		if (!state.registry.get(input.domain.getRootHashedvalue(), locals.record)) {
			output.result = Error::NAME_NOT_REGISTERED;
			return;
		}
		// Check if the invocator is the owner of the domain
		if (locals.record.owner != qpi.invocator()) {
			output.result = Error::NOT_THE_OWNER;
			return;
		}

		locals.logger = { SELF_INDEX, QNS_SUCCESS_CODE, 0 };
		LOG_INFO(locals.logger);
		// Get the subdomain hashmap out first
		state.resolveData.get(input.domain.getRootHashedvalue(), locals.subdomainHashMap);
		// Then extract the resolve data for the full domain from the subdomain hashmap
		locals.subdomainHashMap.get(input.domain.getFullHashedValue(), locals.resolveData);
		// Set the new address in the resolve data
		locals.resolveData.address = input.address;
		// Finally, set the updated resolve data back to the subdomain hashmap
		locals.subdomainHashMap.set(input.domain.getFullHashedValue(), locals.resolveData);
		// And set the updated subdomain hashmap back to the resolve data
		state.resolveData.set(input.domain.getRootHashedvalue(), locals.subdomainHashMap);
		output.result = QNS_SUCCESS_CODE;
	}

	struct SetResolveTextData_input {
		Domain domain;
		UEFIString<> text;
	};
	struct SetResolveTextData_output {
		uint8 result;
	};
	struct SetResolveTextData_locals {
		QNSLogger logger;
		RegistryRecord record;
		ResolveData resolveData;
		HashMap<uint64, ResolveData, MAX_NUMBER_OF_SUBDOMAINS> subdomainHashMap;
	};
	PUBLIC_PROCEDURE_WITH_LOCALS(SetResolveTextData) {
		// Check if the domain exists in the registry
		if (!state.registry.get(input.domain.getRootHashedvalue(), locals.record)) {
			output.result = Error::NAME_NOT_REGISTERED;
			return;
		}
		// Check if the invocator is the owner of the domain
		if (locals.record.owner != qpi.invocator()) {
			output.result = Error::NOT_THE_OWNER;
			return;
		}
		locals.logger = { SELF_INDEX, QNS_SUCCESS_CODE, 0 };
		LOG_INFO(locals.logger);
		// Get the subdomain hashmap out first
		state.resolveData.get(input.domain.getRootHashedvalue(), locals.subdomainHashMap);
		// Then extract the resolve data for the full domain from the subdomain hashmap
		locals.subdomainHashMap.get(input.domain.getFullHashedValue(), locals.resolveData);
		// Set the new text in the resolve data
		locals.resolveData.text = input.text;
		// Finally, set the updated resolve data back to the subdomain hashmap
		locals.subdomainHashMap.set(input.domain.getFullHashedValue(), locals.resolveData);
		// And set the updated subdomain hashmap back to the resolve data
		state.resolveData.set(input.domain.getRootHashedvalue(), locals.subdomainHashMap);
		output.result = QNS_SUCCESS_CODE;
	}

	struct INITIALIZE_locals {
		uint64 i;
	};
	INITIALIZE_WITH_LOCALS() {
		state.QUBIC_TLD = UEFIString<MAX_TLD_LENGTH>();
		state.QNS_TLD = UEFIString<MAX_TLD_LENGTH>();

		state.QUBIC_TLD.set(0, 113); // 'q'
		state.QUBIC_TLD.set(1, 117); // 'u'
		state.QUBIC_TLD.set(2, 98); // 'b'
		state.QUBIC_TLD.set(3, 105); // 'i'
		state.QUBIC_TLD.set(4, 99); // 'c'
		state.QUBIC_TLD.set(5, 0); // null terminator

		state.QNS_TLD.set(0, 113); // 'q'
		state.QNS_TLD.set(1, 110); // 'n'
		state.QNS_TLD.set(2, 115); // 's'
		state.QNS_TLD.set(3, 0); // null terminator

		state.TLDs.set(0, state.QUBIC_TLD);
		state.TLDs.set(1, state.QNS_TLD);
	}

	struct BEGIN_EPOCH_locals {
		uint64 i;
		uint64 key;
		RegistryRecord record;
		uint32 newEpoch;
	};
	BEGIN_EPOCH_WITH_LOCALS() {
		locals.newEpoch = qpi.epoch();
		for (locals.i = 0; locals.i < state.registry.capacity(); locals.i++) {
			locals.key = state.registry.key(locals.i);
			locals.record = state.registry.value(locals.i);

			// Check if the domain is expired and remove it if so
			if (static_cast<uint32>(locals.record.registerEpoch) + static_cast<uint32>(locals.record.registrationYears * EPOCHS_IN_YEAR) < locals.newEpoch) {
				state.registry.removeByKey(locals.key);
				state.resolveData.removeByKey(locals.key);
			}
		}
		// Clean up the registry and resolve data if needed for faster access
		state.registry.cleanupIfNeeded();
		state.resolveData.cleanupIfNeeded();
	}

	REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
	{
		REGISTER_USER_FUNCTION(GetDomainRegistryRecord, 10);
		REGISTER_USER_FUNCTION(GetResolveAddressData, 12);
		REGISTER_USER_FUNCTION(GetResolveTextData, 13);

		REGISTER_USER_PROCEDURE(RegisterDomain, 8);
		REGISTER_USER_PROCEDURE(SetResolveAddressData, 9);
		REGISTER_USER_PROCEDURE(SetResolveTextData, 10);
		REGISTER_USER_PROCEDURE(TransferDomain, 11);
		REGISTER_USER_PROCEDURE(RenewDomain, 13);
	}
};