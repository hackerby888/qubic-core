using namespace QPI;

static constexpr uint64 MAX_NUMBER_OF_DOMAINS = 1'048'576;
static constexpr uint64 MAX_NUMBER_OF_SUBDOMAINS = 16;
static constexpr uint8 EPOCHS_IN_YEAR = 52; 
static constexpr uint8 MAX_NAME_LENGTH = 32;
static constexpr uint8 MIN_NAME_LENGTH = 1;
static constexpr uint64 MAX_TLD_LENGTH = 8;
static constexpr uint8 SUCCESS = 0;

static constexpr sint64 REGISTER_FEE_PER_YEAR = 2'000'000;
static constexpr sint64 TRANSFER_DOMAIN_FEE = 100;

struct QNSLogger
{
	uint32 _contractIndex;
	uint32 _type; // Assign a random unique (per contract) number to distinguish messages of different types
	char _terminator; // Only data before "_terminator" are logged
};

enum Error {
	NAME_INVALID = 1,
	TLD_INVALID,
	SUBDOMAIN_INVALID,
	NAME_NOT_FOUND,
	NAME_HAS_NO_RESOLVE_DATA,
	NAME_NOT_REGISTERED,
	NAME_ALREADY_REGISTERED,
	DATE_INVALID,
	NOT_THE_OWNER,
	INVALID_FUND
};

template <uint64 LENGTH = MAX_NAME_LENGTH>
struct UEFIString : public Array<sint8, LENGTH> {
public:
	UEFIString() {
		for (int i = 0; i < this->capacity(); i++) {
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

		uint8 nullTerminatorIndex = 0;
		for (int i = 0; i < this->capacity(); i++) {
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
		for (int i = 0; i < this->capacity(); i++) {
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
		for (int i = 0; i < this->capacity(); i++) {
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

	static HashFunction<UEFIString<>> hasher;
	static HashFunction< UEFIString<MAX_TLD_LENGTH>> tldHasher;
	static HashFunction<uint64> uint64Hasher;

	void setSubDomain(UEFIString<> subDomain) {
		this->subDomain = subDomain;
	}

	bool operator==(Domain& domain) {
		if (!(subDomain == domain.subDomain))
			return false;
		if (!(rootDomain == domain.rootDomain))
			return false;
		if (!(tld == domain.tld))
			return false;

		return true;
	}

	bool validate() {
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
		return uint64Hasher.hash(hasher.hash(subDomain) + hasher.hash(rootDomain) + tldHasher.hash(tld));
	}

	// If domain is sub.example.com then will return the hash of example.com
	uint64 getRootHashedvalue() const {
		return uint64Hasher.hash(hasher.hash(rootDomain) + tldHasher.hash(tld));
	}
};

struct RegistryRecord {
	id owner;
	uint16 registerEpoch;
	uint16 registrationYears;

public:
	RegistryRecord() {
		owner = NULL_ID;
		registrationYears = 0;
	}

	RegistryRecord(id owner, uint16 registerEpoch, uint16 registrationYears) {
		this->owner = owner;
		this->registerEpoch = registerEpoch;
		this->registrationYears = registrationYears;
	}

	bool operator==(const RegistryRecord& record) const {
		return owner == record.owner && registrationYears == record.registrationYears;
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

	bool operator==(const ResolveData& data) const {
		return address == data.address && text == data.text;
	}
};

struct FEIYU2
{
};
struct FEIYU : public ContractBase
{
public:
	////// ******** States ******** //////
	UEFIString<MAX_TLD_LENGTH> QUBIC_TLD;
	UEFIString<MAX_TLD_LENGTH> QNS_TLD;
	Array<UEFIString<MAX_TLD_LENGTH>, 2> TLDs;
	HashMap<uint64, RegistryRecord, MAX_NUMBER_OF_DOMAINS> registry;
	HashMap<uint64, HashMap<uint64, ResolveData, MAX_NUMBER_OF_SUBDOMAINS>, MAX_NUMBER_OF_DOMAINS> resolveData;

	////// ******** Functions & Procedures ******** //////
	struct UpdateSubdomainMap_input {
		Domain domain;
		HashMap<uint64, ResolveData, MAX_NUMBER_OF_SUBDOMAINS> subdomainHashMap;
	};
	struct UpdateSubdomainMap_output {
		uint8 result;
	};
	PUBLIC_PROCEDURE(UpdateSubdomainMap) {
		state.resolveData.set(input.domain.getRootHashedvalue(), input.subdomainHashMap);	
		output.result = SUCCESS;
	}

	struct RenewDomain_input {
		Domain domain;
		uint16 yearsToRenew;
	};
	struct RenewDomain_output {
		uint8 result;
	};
	struct RenewDomain_locals {
		RegistryRecord record;
	};
	PUBLIC_PROCEDURE_WITH_LOCALS(RenewDomain) {
		if (!state.registry.get(input.domain.getRootHashedvalue(), locals.record)) {
			output.result = Error::NAME_NOT_REGISTERED;
			return;
		}

		if (locals.record.owner != qpi.invocator()) {
			output.result = Error::NOT_THE_OWNER;
			return;
		}

		if (qpi.invocationReward() < input.yearsToRenew * REGISTER_FEE_PER_YEAR) {
			output.result = Error::INVALID_FUND;
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			return;
		}

		locals.record.registrationYears += input.yearsToRenew;
		state.registry.set(input.domain.getRootHashedvalue(), locals.record);
	}

	struct GetResolveData_input {
		Domain domain;
	};
	struct GetResolveData_output {
		uint8 result;
		ResolveData data;
	};
	struct GetResolveData_locals {
		RegistryRecord record;
		ResolveData resolveData;
		HashMap<uint64, ResolveData, MAX_NUMBER_OF_SUBDOMAINS> subdomainHashMap;
	};
	PUBLIC_FUNCTION_WITH_LOCALS(GetResolveData) {
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

		output.data = locals.resolveData;
		output.result = SUCCESS;
	}

	struct TransferDomain_input {
		Domain domain;
		id newOwner;
	};
	struct TransferDomain_output {
		uint8 result;
	};
	struct TransferDomain_locals {
		RegistryRecord record;
	};
	PUBLIC_PROCEDURE_WITH_LOCALS(TransferDomain) {
		if (qpi.invocationReward() < TRANSFER_DOMAIN_FEE) {
			output.result = Error::INVALID_FUND;
			// Give back money for user
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			return;
		}
		if (!state.registry.get(input.domain.getRootHashedvalue(), locals.record)) {
			output.result = Error::NAME_NOT_REGISTERED;
			return;
		}
		if (locals.record.owner != qpi.invocator()) {
			output.result = Error::NOT_THE_OWNER;
			return;
		}
		locals.record.owner = input.newOwner;
		state.registry.set(input.domain.getRootHashedvalue(), locals.record);
		output.result = SUCCESS;
	}

	struct GetDomainRegistry_input {
		Domain domain;
	};
	struct GetDomainRegistry_output {
		uint8 result;
		RegistryRecord record;
	};
	struct GetDomainRegistry_locals {
		RegistryRecord record;
		uint64 domainHashedvalue;
	};
	PUBLIC_FUNCTION_WITH_LOCALS(GetDomainRegistry) {
		locals.domainHashedvalue = input.domain.getRootHashedvalue();
		if (!state.registry.get(locals.domainHashedvalue, locals.record)) {
			output.result = Error::NAME_NOT_REGISTERED;
			return;
		}
		output.result = SUCCESS;
		output.record = locals.record;
	}

	struct SetResolveData_input {
		Domain domain;
		ResolveData data;
	};
	struct SetResolveData_output {
		uint8 result;
	};
	struct SetResolveData_locals {
		RegistryRecord record;
		HashMap<uint64, ResolveData, MAX_NUMBER_OF_SUBDOMAINS> subdomainHashMap;
	};
	PUBLIC_PROCEDURE_WITH_LOCALS(SetResolveData) {
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
		state.resolveData.get(input.domain.getRootHashedvalue(), locals.subdomainHashMap);
		locals.subdomainHashMap.set(input.domain.getFullHashedValue(), input.data);
		state.resolveData.set(input.domain.getRootHashedvalue(), locals.subdomainHashMap);
		output.result = SUCCESS;
	}

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
	};
	PUBLIC_PROCEDURE_WITH_LOCALS(RegisterDomain) {
		if (!input.domain.validate()) {
			output.result = Error::NAME_INVALID;
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			return;
		}

		if (state.registry.get(input.domain.getRootHashedvalue(), locals.record)) {
			output.result = Error::NAME_ALREADY_REGISTERED;
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			return;
		}

		// Check if tld is supported
		bool tldSupported = false;
		for (uint64 i = 0; i < state.TLDs.capacity(); i++) {
			if (input.domain.tld == state.TLDs.get(i)) {
				tldSupported = true;
				break;
			}
		}
		if (!tldSupported) {
			output.result = Error::TLD_INVALID;
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			return;
		}

		if (qpi.invocationReward() < input.registrationYears * REGISTER_FEE_PER_YEAR) {
			output.result = Error::INVALID_FUND;
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			return;
		}
		
		QUOTTERY::getCurrentDate(qpi, locals.date);
		output.result = SUCCESS;
		locals.logger = { FEIYU_CONTRACT_INDEX, SUCCESS, 0 };
		LOG_INFO(locals.logger);
		state.registry.set(input.domain.getRootHashedvalue(), { qpi.invocator(), qpi.epoch(), input.registrationYears });
		state.resolveData.set(input.domain.getRootHashedvalue(), locals.subdomainHashMap);

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

			if (static_cast<uint32>(locals.record.registerEpoch) + static_cast<uint32>(locals.record.registrationYears * EPOCHS_IN_YEAR) < locals.newEpoch) {
				state.registry.removeByKey(locals.key);
				state.resolveData.removeByKey(locals.key);
			}
		}
	}

	REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
	{
		REGISTER_USER_FUNCTION(GetDomainRegistry, 10);
		REGISTER_USER_FUNCTION(GetResolveData, 12);

		REGISTER_USER_PROCEDURE(RegisterDomain, 8);
		REGISTER_USER_PROCEDURE(SetResolveData, 9);
		REGISTER_USER_PROCEDURE(TransferDomain, 11);
		REGISTER_USER_PROCEDURE(UpdateSubdomainMap, 12);
		REGISTER_USER_PROCEDURE(RenewDomain, 13);
	}
};