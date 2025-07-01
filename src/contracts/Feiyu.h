using namespace QPI;

static constexpr uint64 MAX_NUMBER_OF_DOMAINS = 1024;
static constexpr uint64 MAX_NUMBER_OF_SUBDOMAINS = 1024;
static constexpr uint8 EPOCHS_IN_YEAR = 52; 
static constexpr uint8 MAX_NAME_LENGTH = 64;
static constexpr uint8 MIN_NAME_LENGTH = 1;


static constexpr uint8 SUCCESS = 0;

typedef HashMap<uint64, ResolveData, MAX_NUMBER_OF_SUBDOMAINS> SubdomainResolveHashMap;

struct QNSLogger
{
	uint32 _contractIndex;
	uint32 _type; // Assign a random unique (per contract) number to distinguish messages of different types
	char _terminator; // Only data before "_terminator" are logged
};

enum Error {
	NAME_INVALID = 1,
	NAME_NOT_FOUND,
	NAME_HAS_NO_RESOLVE_DATA,
	NAME_NOT_REGISTERED,
	NAME_ALREADY_REGISTERED,
	DATE_INVALID,
	NOT_THE_OWNER
};

struct UEFIString : public Array<sint8, MAX_NAME_LENGTH> {
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
		for (int i = 0; i < this->capacity(); i++) {
			if (this->get(i) != other.get(i)) {
				return false;
			}
		}
		return true;
	}
};

struct Domain {
	UEFIString subDomain;
	UEFIString rootDomain;
	UEFIString tld;

	void setSubDomain(UEFIString subDomain) {
		this->subDomain = subDomain;
	}

	bool operator==(Domain &domain) {
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

	uint64 getFullHashedValue() const {
		HashFunction<UEFIString> hasher;
		HashFunction<uint64> uint64Hasher;
		return uint64Hasher.hash(hasher.hash(subDomain) + hasher.hash(rootDomain) + hasher.hash(tld));
	}

	uint64 getRootHashedvalue() const {
		HashFunction<UEFIString> hasher;
		HashFunction<uint64> uint64Hasher;
		return uint64Hasher.hash(hasher.hash(rootDomain) + hasher.hash(tld));
	}
};

struct RegistryRecord {
	id owner;
	uint16 registerEpoch;
	uint16 endEpoch;
	uint32 registerDate;

public:
	RegistryRecord() {
		owner = NULL_ID;
		registerDate = 0;
	}

	RegistryRecord(id owner, uint16 registerEpoch, uint16 endEpoch, uint32 registerDate) {
		this->owner = owner;
		this->registerEpoch = registerEpoch;
		this->endEpoch = endEpoch;
		this->registerDate = registerDate;
	}

	bool operator==(const RegistryRecord& record) const {
		return owner == record.owner && endEpoch == record.endEpoch && registerDate == record.registerDate;
	}
};

struct ResolveData {
	id address;
	UEFIString text;

	ResolveData() {
		this->address = id::zero();
	}

	ResolveData(id address, UEFIString& text) {
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
	HashMap<uint64, RegistryRecord, MAX_NUMBER_OF_DOMAINS> registry;
	HashMap<uint64, SubdomainResolveHashMap, MAX_NUMBER_OF_DOMAINS> resolveData;
	Array<uint64, MAX_NUMBER_OF_DOMAINS> domainHashedValues;

	struct RegisterDomain_input {
		Domain domain;
		uint16 endEpoch;
	};

	struct RegisterDomain_output {
		uint8 result;
	};

	struct SetResolveData_input {
		Domain domain;
		ResolveData data;
	};

	struct SetResolveData_output {
		uint8 result;
	};

	struct GetDomainRegistry_input {
		Domain domain;
	};

	struct GetDomainRegistry_output {
		uint8 result;
		RegistryRecord record;
	};

	struct TransferDomain_input {
		Domain domain;
		id newOwner;
	};
	
	struct TransferDomain_output {
		uint8 result;
	};

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
		SubdomainResolveHashMap subdomainHashMap;
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

	struct TransferDomain_locals {
		RegistryRecord record;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(TransferDomain) {
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

	struct SetResolveData_locals {
		RegistryRecord record;
		SubdomainResolveHashMap subdomainHashMap;
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
		output.result = SUCCESS;
	}
	
	struct RegisterDomain_locals {
		QNSLogger logger;
		RegistryRecord record;
		uint32 date;
		SubdomainResolveHashMap subdomainHashMap;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(RegisterDomain) {
		if (!input.domain.validate()) {
			output.result = Error::NAME_INVALID;
			return;
		}

		if (state.registry.get(input.domain.getRootHashedvalue(), locals.record)) {
			output.result = Error::NAME_ALREADY_REGISTERED;
			return;
		}

		if (input.endEpoch % EPOCHS_IN_YEAR != 0 || input.endEpoch < qpi.epoch() + EPOCHS_IN_YEAR) {
			output.result = Error::DATE_INVALID;
			return;
		}
		
		QUOTTERY::getCurrentDate(qpi, locals.date);
		output.result = SUCCESS;
		locals.logger = { FEIYU_CONTRACT_INDEX, SUCCESS, 0 };
		LOG_INFO(locals.logger);
		state.registry.set(input.domain.getRootHashedvalue(), { qpi.invocator(), qpi.epoch(), input.endEpoch, locals.date });
		state.resolveData.set(input.domain.getRootHashedvalue(), locals.subdomainHashMap);
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

			if (locals.record.endEpoch < locals.newEpoch) {
				state.registry.removeByKey(locals.key);
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
	}
};